#include "progressdialog.h"

#include <QFontMetrics>
#include <QMessageBox>
#include <QScrollBar>
#include <QStyle>
#include <QTextBlock>

#include "ui_progressdialog.h"
#include "ui_progresswidget.h"

static ProgressDialog *theDialog;
static ProgressWidget *theWidget;

static int taskTextVersion;
static QString taskTextLastLine;
static PROGRESS_TEXT_MODE taskTextMode;

ProgressDialog::ProgressDialog(QWidget *parent)
    : QDialog(parent, Qt::WindowStaysOnTopHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint)
    , ui(new Ui::ProgressDialog())
{
    this->ui->setupUi(this);

    theDialog = this;
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::openDialog()
{
    theDialog->showNormal();
    theDialog->adjustSize();
}

void ProgressDialog::start(PROGRESS_DIALOG_STATE mode, const QString &label, int numBars, int flags)
{
    bool background = mode == PROGRESS_DIALOG_STATE::BACKGROUND;

    taskTextVersion = 0;
    taskTextLastLine.clear();
    // taskTextMode = PROGRESS_TEXT_MODE::NORMAL;

    theDialog->afterFlags = flags;
    theDialog->setWindowTitle(label);
    theDialog->ui->outputTextEdit->clear();
    theDialog->activeBars = 0;
    theDialog->status = PROGRESS_STATE::RUNNING;
    theDialog->ui->progressLabel->setVisible(!background);
    theDialog->ui->progressLabel->setText("");
    theDialog->ui->detailsGroupBox->setVisible(background);
    theDialog->ui->detailsPushButton->setText(background ? tr("Hide details") : tr("Show details"));
    theDialog->ui->cancelPushButton->setEnabled(true);
    theDialog->ui->progressButtonsWidget_1->setVisible(!background);
    theDialog->ui->progressButtonsWidget_2->setVisible(background);
    for (int i = 0; i < numBars; i++) {
        QProgressBar *progressBar = new QProgressBar();
        progressBar->setEnabled(false);
        progressBar->setTextVisible(false);
        theDialog->ui->verticalLayout->insertWidget(1 + i, progressBar);
        theDialog->progressBars.append(progressBar);
    }
    theDialog->adjustSize();

    if (background) {
        theWidget->updateWidget(theDialog->status, true, label);

        theDialog->showMinimized();
        return;
    }
    theWidget->updateWidget(theDialog->status, false, "");

    theDialog->showNormal();
}

void ProgressDialog::done()
{
    if (theDialog->afterFlags & PAF_UPDATE_WINDOW) {
        emit theDialog->updateWindow();
    }

    theDialog->setWindowTitle(" ");
    theDialog->ui->progressLabel->setVisible(false);
    for (QProgressBar *progressBar : theDialog->progressBars) {
        theDialog->ui->verticalLayout->removeWidget(progressBar);
    }
    qDeleteAll(theDialog->progressBars);
    theDialog->progressBars.clear();
    // theDialog->activeBars = 0;
    theDialog->ui->progressButtonsWidget_1->setVisible(false);
    bool detailsOpen = theDialog->ui->detailsGroupBox->isVisible();
    theDialog->ui->detailsGroupBox->setVisible(true);
    theDialog->ui->progressButtonsWidget_2->setVisible(true);
    if (theDialog->status == PROGRESS_STATE::RUNNING) {
        theDialog->status = PROGRESS_STATE::DONE;
    } else if (theDialog->status == PROGRESS_STATE::CANCEL) {
        ProgressDialog::addResult_impl(PROGRESS_TEXT_MODE::NORMAL, QApplication::tr("Process cancelled."), false);
    }
    if (theDialog->status != PROGRESS_STATE::FAIL && (!detailsOpen || !theDialog->isVisible() || theDialog->isMinimized()) && !(theDialog->afterFlags & PAF_OPEN_DIALOG)) {
        theDialog->hide();
    } else {
        theDialog->showNormal();
        theDialog->adjustSize();
        theDialog->ui->closePushButton->setFocus();
    }

    theWidget->updateWidget(theDialog->status, !theDialog->ui->outputTextEdit->document()->isEmpty(), "");
}

void ProgressDialog::incBar(const QString &label, int maxValue)
{
    if (!label.isEmpty()) {
        theDialog->ui->progressLabel->setText(label);
    }
    QProgressBar *newProgressBar = theDialog->progressBars[theDialog->activeBars];

    theDialog->activeBars++;
    newProgressBar->setRange(0, maxValue);
    newProgressBar->setValue(0);
    newProgressBar->setToolTip("0%");
    newProgressBar->setEnabled(true);

    theDialog->update();
}

void ProgressDialog::decBar()
{
    theDialog->activeBars--;

    QProgressBar *oldProgressBar = theDialog->progressBars[theDialog->activeBars];

    oldProgressBar->setEnabled(false);
}

bool ProgressDialog::wasCanceled()
{
    QCoreApplication::processEvents();

    return theDialog->status >= PROGRESS_STATE::CANCEL;
}

bool ProgressDialog::incValue()
{
    int index = theDialog->activeBars - 1;
    int amount = 1;
    QProgressBar *progressBar = theDialog->progressBars[index];

    amount += progressBar->value();
    progressBar->setValue(amount);
    progressBar->setToolTip(QString("%1%").arg(amount * 100 / progressBar->maximum()));
    return !ProgressDialog::wasCanceled();
}

ProgressDialog &dProgress()
{
    taskTextMode = PROGRESS_TEXT_MODE::NORMAL;
    return *theDialog;
}

ProgressDialog &dProgressWarn()
{
    taskTextMode = PROGRESS_TEXT_MODE::WARNING;
    return *theDialog;
}

ProgressDialog &dProgressErr()
{
    taskTextMode = PROGRESS_TEXT_MODE::ERROR;
    return *theDialog;
}

ProgressDialog &dProgressFail()
{
    taskTextMode = PROGRESS_TEXT_MODE::FAIL;
    return *theDialog;
}

void ProgressDialog::addResult_impl(PROGRESS_TEXT_MODE mode, const QString &line, bool replace)
{
    QPlainTextEdit *textEdit = theDialog->ui->outputTextEdit;
    QTextCursor cursor = textEdit->textCursor();
    int scrollValue = textEdit->verticalScrollBar()->value();
    // The user has selected text or scrolled away from the bottom: maintain position.
    const bool active = cursor.hasSelection() || scrollValue != textEdit->verticalScrollBar()->maximum();

    // remove last line if replacing the text
    if (replace) {
        // remove last line
        QTextCursor lastCursor = QTextCursor(textEdit->document()->lastBlock());
        lastCursor.select(QTextCursor::BlockUnderCursor);
        lastCursor.removeSelectedText();
    }
    // Append the text at the end of the document.
    if (mode == PROGRESS_TEXT_MODE::NORMAL) {
        // using appendHtml instead of appendPlainText because Qt can not handle mixed appends...
        // (the new block inherits the properties of previous/selected block which might be colored due to the code below)
        textEdit->appendHtml(line);
    } else { // Using <pre> tag to allow multiple spaces
        QString htmlText = QString("<p style=\"color:%1;white-space:pre\">%2</p>").arg(mode == PROGRESS_TEXT_MODE::WARNING ? "orange" : "red").arg(line);
        textEdit->appendHtml(htmlText);
    }
    // update status
    PROGRESS_STATE refStatus;
    switch (mode) {
    case PROGRESS_TEXT_MODE::NORMAL:
        refStatus = PROGRESS_STATE::RUNNING;
        break;
    case PROGRESS_TEXT_MODE::WARNING:
        refStatus = PROGRESS_STATE::WARN;
        break;
    case PROGRESS_TEXT_MODE::ERROR:
        refStatus = PROGRESS_STATE::ERROR;
        break;
    case PROGRESS_TEXT_MODE::FAIL:
        refStatus = PROGRESS_STATE::FAIL;
        break;
    }
    if (theDialog->status < refStatus) {
        theDialog->status = refStatus;
    }

    if (!active) {
        // The user hasn't selected any text and the scrollbar is at the bottom: scroll to the bottom.
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        scrollValue = textEdit->verticalScrollBar()->maximum();
    }
    textEdit->setTextCursor(cursor);
    textEdit->verticalScrollBar()->setValue(scrollValue);
}

ProgressDialog &ProgressDialog::operator<<(const QString &text)
{
    ProgressDialog::addResult_impl(taskTextMode, text, false);
    taskTextLastLine = text;
    taskTextVersion++;
    return *this;
}

ProgressDialog &ProgressDialog::operator<<(const QPair<QString, QString> &text)
{
    bool replace = taskTextLastLine == text.first;
    ProgressDialog::addResult_impl(taskTextMode, text.second, replace);
    taskTextVersion++;
    taskTextLastLine = text.second;
    return *this;
}

ProgressDialog &ProgressDialog::operator<<(QPair<int, QString> &idxText)
{
    bool replace = taskTextVersion == idxText.first;
    ProgressDialog::addResult_impl(taskTextMode, idxText.second, replace);
    taskTextVersion++;
    taskTextLastLine = idxText.second;
    idxText.first = taskTextVersion;
    return *this;
}

void ProgressDialog::on_detailsPushButton_clicked()
{
    bool currVisible = this->ui->detailsGroupBox->isVisible();
    this->ui->detailsGroupBox->setVisible(!currVisible);
    this->ui->detailsPushButton->setText(currVisible ? tr("Show details") : tr("Hide details"));
    this->adjustSize();
}

void ProgressDialog::on_cancelPushButton_clicked()
{
    if (this->status < PROGRESS_STATE::CANCEL) {
        this->status = PROGRESS_STATE::CANCEL;
    }
    this->ui->cancelPushButton->setEnabled(false);
}

void ProgressDialog::on_closePushButton_clicked()
{
    this->hide();
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
    this->on_cancelPushButton_clicked();
    // QDialog::closeEvent(event);
}

void ProgressDialog::changeEvent(QEvent *event)
{
    /*if (event->type() == QEvent::WindowStateChange && this->isMinimized()) {
        this->hide();
    }*/
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    return QDialog::changeEvent(event);
}

ProgressWidget::ProgressWidget(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::ProgressWidget())
{
    this->ui->setupUi(this);
    int fontHeight = this->fontMetrics().height() + 2;
    this->setMinimumHeight(fontHeight);
    this->setMaximumHeight(fontHeight);
    this->ui->openPushButton->setMinimumSize(fontHeight, fontHeight);
    this->ui->openPushButton->setMaximumSize(fontHeight, fontHeight);
    this->ui->openPushButton->setIconSize(QSize(fontHeight, fontHeight));
    this->updateWidget(PROGRESS_STATE::DONE, false, "");

    theWidget = this;
}

ProgressWidget::~ProgressWidget()
{
    delete ui;
}

void ProgressWidget::updateWidget(PROGRESS_STATE status, bool active, const QString &label)
{
    this->ui->openPushButton->setEnabled(active);
    QStyle::StandardPixmap type;
    switch (status) {
    case PROGRESS_STATE::DONE:
        type = active ? QStyle::SP_ArrowUp : QStyle::SP_DialogApplyButton;
        break;
    case PROGRESS_STATE::RUNNING:
        type = QStyle::SP_BrowserReload;
        break;
    case PROGRESS_STATE::WARN:
        type = QStyle::SP_MessageBoxWarning;
        break;
    case PROGRESS_STATE::ERROR:
        type = QStyle::SP_MessageBoxCritical;
        break;
    case PROGRESS_STATE::CANCEL:
        type = QStyle::SP_TitleBarCloseButton; // QStyle::SP_DialogCancelButton;
        break;
    case PROGRESS_STATE::FAIL:
        type = QStyle::SP_BrowserStop;
        break;
    }
    this->ui->openPushButton->setIcon(this->style()->standardIcon(type));
    this->ui->messageLabel->setText(label);
    this->adjustSize();
    this->repaint();
    // QFrame::update();
}

void ProgressWidget::on_openPushButton_clicked()
{
    ProgressDialog::openDialog();
}
