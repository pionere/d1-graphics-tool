#include "progressdialog.h"

#include <QFontMetrics>
#include <QMessageBox>
#include <QStyle>

#include "ui_progressdialog.h"
#include "ui_progresswidget.h"

static ProgressDialog *theDialog;
static ProgressWidget *theWidget;

ProgressDialog::ProgressDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProgressDialog())
{
    this->ui->setupUi(this);

    this->setWindowFlags((/*this->windowFlags() |*/ Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint) & ~(Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowContextHelpButtonHint | Qt::MacWindowToolBarButtonHint | Qt::WindowFullscreenButtonHint | Qt::WindowMinMaxButtonsHint));

    theDialog = this;
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::start(PROGRESS_DIALOG_STATE mode, const QString &label, int maxValue)
{
    if (mode == PROGRESS_DIALOG_STATE::BACKGROUND) {
        theDialog->setWindowTitle(label);
        theDialog->ui->progressLabel->setVisible(false);
        theDialog->ui->progressBar_0->setVisible(false);
        theDialog->ui->progressBar_1->setVisible(false);
        theDialog->ui->progressBar_2->setVisible(false);
        theDialog->ui->progressButtonsWidget_1->setVisible(false);
        theDialog->ui->detailsGroupBox->setVisible(true);
        theDialog->ui->progressButtonsWidget_2->setVisible(true);
        theDialog->ui->outputTextEdit->clear();
        theDialog->textVersion = 0;
        theDialog->barCount = 0;
        theDialog->errorOnFail = false;
        theDialog->status = PROGRESS_STATE::RUNNING;

        theWidget->update(theDialog->status, true, label);
        return;
    }

    if (mode == PROGRESS_DIALOG_STATE::OPEN) {
        theDialog->showNormal();
        theDialog->adjustSize();
        return;
    }

    theDialog->showNormal();

    theDialog->setWindowTitle(" ");
    theDialog->ui->progressLabel->setVisible(true);
    theDialog->ui->progressLabel->setText(label);
    theDialog->ui->progressBar_0->setVisible(true);
    theDialog->ui->progressBar_0->setRange(0, maxValue);
    theDialog->ui->progressBar_0->setValue(0);
    theDialog->ui->progressBar_1->setVisible(false);
    theDialog->ui->progressBar_2->setVisible(false);
    theDialog->ui->cancelPushButton->setEnabled(true);
    theDialog->ui->progressButtonsWidget_1->setVisible(true);
    theDialog->ui->detailsGroupBox->setVisible(true);
    theDialog->ui->progressButtonsWidget_2->setVisible(false);
    theDialog->on_detailsPushButton_clicked();
    theDialog->ui->outputTextEdit->clear();
    theDialog->textVersion = 0;
    theDialog->barCount = 1;
    theDialog->errorOnFail = false;
    theDialog->status = PROGRESS_STATE::RUNNING;

    theWidget->update(theDialog->status, false, "");
}

void ProgressDialog::done(bool forceOpen)
{
    theDialog->setWindowTitle(" ");
    theDialog->ui->progressLabel->setVisible(false);
    theDialog->ui->progressBar_0->setVisible(false);
    theDialog->ui->progressBar_1->setVisible(false);
    theDialog->ui->progressBar_2->setVisible(false);
    theDialog->ui->progressButtonsWidget_1->setVisible(false);
    bool detailsOpen = theDialog->ui->detailsGroupBox->isVisible();
    theDialog->ui->detailsGroupBox->setVisible(true);
    theDialog->ui->progressButtonsWidget_2->setVisible(true);
    if (theDialog->status == PROGRESS_STATE::RUNNING) {
        theDialog->status = PROGRESS_STATE::DONE;
    }
    if (theDialog->status != PROGRESS_STATE::FAIL && (!detailsOpen || !theDialog->isVisible()) && !forceOpen) {
        theDialog->hide();
    } else {
        theDialog->showNormal();
        theDialog->adjustSize();
    }

    theWidget->update(theDialog->status, !theDialog->ui->outputTextEdit->document()->isEmpty(), "");
}

void ProgressDialog::startSub(int maxValue)
{
    QProgressBar *newProgressBar = theDialog->barCount == 1 ? theDialog->ui->progressBar_1 : theDialog->ui->progressBar_2;

    theDialog->errorOnFail = true;
    theDialog->barCount++;
    newProgressBar->setVisible(true);
    newProgressBar->setRange(0, maxValue);
    newProgressBar->setValue(0);
    theDialog->adjustSize();
}

void ProgressDialog::doneSub()
{
    QProgressBar *oldProgressBar = theDialog->barCount == 2 ? theDialog->ui->progressBar_1 : theDialog->ui->progressBar_2;

    theDialog->errorOnFail = false;
    theDialog->barCount--;
    oldProgressBar->setVisible(false);
    theDialog->adjustSize();
}

bool ProgressDialog::wasCanceled()
{
    return theDialog->status >= PROGRESS_STATE::CANCEL;
}

void ProgressDialog::incValue()
{
    QProgressBar *currProgressBar;

    switch (theDialog->barCount) {
    case 0:
    case 1:
        currProgressBar = theDialog->ui->progressBar_0;
        break;
    case 2:
        currProgressBar = theDialog->ui->progressBar_1;
        break;
    default: // case 3:
        currProgressBar = theDialog->ui->progressBar_2;
        break;
    }

    currProgressBar->setValue(currProgressBar->value() + 1);
    QCoreApplication::processEvents();
}

ProgressDialog &dProgress()
{
    theDialog->textMode = PROGRESS_TEXT_MODE::NORMAL;
    return *theDialog;
}

ProgressDialog &dProgressWarn()
{
    if (theDialog->status < PROGRESS_STATE::WARN) {
        theDialog->status = PROGRESS_STATE::WARN;
    }
    theDialog->textMode = PROGRESS_TEXT_MODE::WARNING;
    return *theDialog;
}

ProgressDialog &dProgressErr()
{
    if (theDialog->status < PROGRESS_STATE::ERROR) {
        theDialog->status = PROGRESS_STATE::ERROR;
    }
    theDialog->textMode = PROGRESS_TEXT_MODE::ERROR;
    return *theDialog;
}

ProgressDialog &dProgressFail()
{
    if (theDialog->errorOnFail) {
        return dProgressErr();
    }
    if (theDialog->status < PROGRESS_STATE::FAIL) {
        theDialog->status = PROGRESS_STATE::FAIL;
    }
    theDialog->textMode = PROGRESS_TEXT_MODE::ERROR;
    return *theDialog;
}

ProgressDialog &ProgressDialog::operator<<(const QString &text)
{
    PROGRESS_TEXT_MODE mode = theDialog->textMode;

    if (mode == PROGRESS_TEXT_MODE::NORMAL) {
        this->ui->outputTextEdit->appendPlainText(text);
    } else { // Using <pre> tag to allow multiple spaces
        QString htmlText = QString("<p style=\"color:%1;white-space:pre\">%2</p>").arg(mode == PROGRESS_TEXT_MODE::ERROR ? "red" : "orange").arg(text);
        this->ui->outputTextEdit->appendHtml(htmlText);
    }
    this->textVersion++;
    return *this;
}

void ProgressDialog::removeLastLine()
{
    this->ui->outputTextEdit->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    this->ui->outputTextEdit->moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    this->ui->outputTextEdit->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
    this->ui->outputTextEdit->textCursor().removeSelectedText();
    this->ui->outputTextEdit->textCursor().deletePreviousChar(); // Added to trim the newline char when removing last line
}

ProgressDialog &ProgressDialog::operator<<(const QPair<QString, QString> &text)
{
    // this->ui->outputTextEdit->setFocus();
    QTextCursor storeCursorPos = this->ui->outputTextEdit->textCursor();
    if (this->ui->outputTextEdit->textCursor().selectedText() == text.first) {
        this->removeLastLine();
    }
    QTextCursor lastCursorPos = this->ui->outputTextEdit->textCursor();
    this->ui->outputTextEdit->appendPlainText(text.second);
    if (lastCursorPos > storeCursorPos) {
        this->ui->outputTextEdit->setTextCursor(storeCursorPos);
    }
    this->textVersion++;
    return *this;
}

ProgressDialog &ProgressDialog::operator<<(QPair<int, QString> &idxText)
{
    // this->ui->outputTextEdit->setFocus();
    QTextCursor storeCursorPos = this->ui->outputTextEdit->textCursor();
    if (this->textVersion == idxText.first) {
        this->removeLastLine();
    }
    QTextCursor lastCursorPos = this->ui->outputTextEdit->textCursor();
    this->ui->outputTextEdit->appendPlainText(idxText.second);
    if (lastCursorPos > storeCursorPos) {
        this->ui->outputTextEdit->setTextCursor(storeCursorPos);
    }
    this->textVersion++;
    idxText.first = this->textVersion;
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
        dProgress() << tr("Process cancelled.");
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
    this->update(PROGRESS_STATE::DONE, false, "");

    theWidget = this;
}

ProgressWidget::~ProgressWidget()
{
    delete ui;
}

void ProgressWidget::update(PROGRESS_STATE status, bool active, const QString &label)
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
    ProgressDialog::start(PROGRESS_DIALOG_STATE::OPEN, "", 0);
}
