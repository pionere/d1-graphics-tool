#include "progressdialog.h"

#include <QFontMetrics>
#include <QMessageBox>
#include <QScrollBar>
#include <QStyle>

#include "ui_progressdialog.h"
#include "ui_progresswidget.h"

static ProgressDialog *theDialog;
static ProgressWidget *theWidget;

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

void ProgressDialog::start(PROGRESS_DIALOG_STATE mode, const QString &label, int maxValue)
{
    if (mode == PROGRESS_DIALOG_STATE::BACKGROUND) {
        theDialog->setWindowTitle(label);
        theDialog->ui->progressLabel->setVisible(false);
        theDialog->ui->progressBar->setVisible(false);
        theDialog->ui->progressButtonsWidget_1->setVisible(false);
        theDialog->ui->detailsGroupBox->setVisible(true);
        theDialog->ui->progressButtonsWidget_2->setVisible(true);
        theDialog->ui->outputTextEdit->clear();
        theDialog->textVersion = 0;
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
    theDialog->ui->progressBar->setVisible(true);
    theDialog->ui->progressBar->setRange(0, maxValue);
    theDialog->ui->progressBar->setValue(0);
    theDialog->ui->cancelPushButton->setEnabled(true);
    theDialog->ui->progressButtonsWidget_1->setVisible(true);
    theDialog->ui->detailsGroupBox->setVisible(true);
    theDialog->ui->progressButtonsWidget_2->setVisible(false);
    theDialog->on_detailsPushButton_clicked();
    theDialog->ui->outputTextEdit->clear();
    theDialog->textVersion = 0;
    theDialog->status = PROGRESS_STATE::RUNNING;

    theWidget->update(theDialog->status, false, "");
}

void ProgressDialog::done(bool forceOpen)
{
    theDialog->setWindowTitle(" ");
    theDialog->ui->progressLabel->setVisible(false);
    theDialog->ui->progressBar->setVisible(false);
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

bool ProgressDialog::wasCanceled()
{
    return theDialog->status >= PROGRESS_STATE::CANCEL;
}

void ProgressDialog::incValue()
{
    theDialog->ui->progressBar->setValue(theDialog->ui->progressBar->value() + 1);
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
    if (theDialog->status < PROGRESS_STATE::FAIL) {
        theDialog->status = PROGRESS_STATE::FAIL;
    }
    theDialog->textMode = PROGRESS_TEXT_MODE::ERROR;
    return *theDialog;
}

void ProgressDialog::appendLine(const QString &line, bool replace)
{
    QPlainTextEdit *textEdit = this->ui->outputTextEdit;
    QTextCursor cursor = textEdit->textCursor();
    int scrollValue = textEdit->verticalScrollBar()->value();
    // The user has selected text or scrolled away from the bottom: maintain position.
    const bool active = cursor.hasSelection() || scrollValue != textEdit->verticalScrollBar()->maximum();

    // remove last line if replacing the text
    if (replace) {
        this->removeLastLine();
    }
    // Append the text at the end of the document.
    PROGRESS_TEXT_MODE mode = this->textMode;
    if (mode == PROGRESS_TEXT_MODE::NORMAL) {
        textEdit->appendPlainText(line);
    } else { // Using <pre> tag to allow multiple spaces
        QString htmlText = QString("<p style=\"color:%1;white-space:pre\">%2</p>").arg(mode == PROGRESS_TEXT_MODE::ERROR ? "red" : "orange").arg(line);
        textEdit->appendHtml(htmlText);
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
    this->appendLine(text, false);
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
    this->appendLine(text.second, this->ui->outputTextEdit->textCursor().selectedText() == text.first);
    this->textVersion++;
    return *this;
}

ProgressDialog &ProgressDialog::operator<<(QPair<int, QString> &idxText)
{
    // this->ui->outputTextEdit->setFocus();
    this->appendLine(idxText.second, this->textVersion == idxText.first);
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
