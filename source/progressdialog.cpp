#include "progressdialog.h"

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
    this->setWindowTitle("");

    theDialog = this;
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::start(PROGRESS_DIALOG_STATE mode, const QString &label, int maxValue)
{
    if (mode == PROGRESS_DIALOG_STATE::BACKGROUND) {
        theDialog->ui->progressLabel->setVisible(false);
        theDialog->ui->progressBar->setVisible(false);
        theDialog->ui->progressButtonsWidget->setVisible(false);
        theDialog->ui->detailsGroupBox->setVisible(true);
        theDialog->ui->closePushButton->setVisible(true);
        theDialog->ui->outputTextEdit->clear();
        theDialog->textVersion = 0;
        theDialog->status = PROGRESS_STATE::RUNNING;

        theWidget->ui->messageLabel->setText(label);
        theWidget->update(theDialog->status, true);
        return;
    }

    if (mode == PROGRESS_DIALOG_STATE::OPEN) {
        theDialog->showNormal();
        return;
    }

    theDialog->showNormal();

    theDialog->ui->progressLabel->setVisible(true);
    theDialog->ui->progressLabel->setText(label);
    theDialog->ui->progressBar->setVisible(true);
    theDialog->ui->progressBar->setRange(0, maxValue);
    theDialog->ui->progressBar->setValue(0);
    theDialog->ui->cancelPushButton->setEnabled(true);
    theDialog->ui->progressButtonsWidget->setVisible(true);
    theDialog->ui->detailsGroupBox->setVisible(true);
    theDialog->on_detailsPushButton_clicked();
    theDialog->ui->closePushButton->setVisible(false);
    theDialog->ui->outputTextEdit->clear();
    theDialog->textVersion = 0;
    theDialog->status = PROGRESS_STATE::RUNNING;

    theWidget->ui->messageLabel->setText("");
    theWidget->update(theDialog->status, false);
}

void ProgressDialog::done()
{
    theDialog->hide();

    theDialog->ui->progressLabel->setVisible(false);
    theDialog->ui->progressBar->setVisible(false);
    theDialog->ui->progressButtonsWidget->setVisible(false);
    theDialog->ui->detailsGroupBox->setVisible(true);
    theDialog->ui->closePushButton->setVisible(true);
    theDialog->status = PROGRESS_STATE::DONE;

    theWidget->ui->messageLabel->setText("");
    theWidget->update(theDialog->status, !theDialog->ui->outputTextEdit->document()->isEmpty());
}

bool ProgressDialog::wasCanceled()
{
    return theDialog->status == PROGRESS_STATE::CANCELLED;
}

void ProgressDialog::incValue()
{
    theDialog->ui->progressBar->setValue(theDialog->ui->progressBar->value() + 1);
    QCoreApplication::processEvents();
}

ProgressDialog &dProgress()
{
    return *theDialog;
}

ProgressDialog &ProgressDialog::operator<<(const QString &text)
{
    this->ui->outputTextEdit->appendPlainText(text);
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
    if (this->status == PROGRESS_STATE::RUNNING) {
        this->status = PROGRESS_STATE::CANCELLED;
    }
    this->ui->cancelPushButton->setEnabled(false);
}

void ProgressDialog::on_closePushButton_clicked()
{
    this->hide();
}

void ProgressDialog::closeEvent(QCloseEvent *e)
{
    this->on_cancelPushButton_clicked();
    // QDialog::closeEvent(e);
}

void ProgressDialog::changeEvent(QEvent *e)
{
    /*if (event->type() == QEvent::WindowStateChange && this->isMinimized()) {
        this->hide();
    }*/
    return QDialog::changeEvent(e);
}

ProgressWidget::ProgressWidget(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::ProgressWidget())
{
    this->ui->setupUi(this);

    this->update(PROGRESS_STATE::DONE, false);

    theWidget = this;
}

ProgressWidget::~ProgressWidget()
{
    delete ui;
}

void ProgressWidget::update(PROGRESS_STATE status, bool active)
{
    this->ui->openPushButton->setEnabled(active);
    QStyle::StandardPixmap type;
    switch (status) {
    case PROGRESS_STATE::DONE:
        type = QStyle::SP_DialogYesButton; // QStyle::SP_DialogOkButton;
        break;
    case PROGRESS_STATE::CANCELLED:
        type = QStyle::SP_MessageBoxWarning; // QStyle::SP_BrowserStop;
        break;
    case PROGRESS_STATE::RUNNING:
        type = QStyle::SP_BrowserReload;
        break;
    }
    this->ui->openPushButton->setIcon(this->style()->standardIcon(type));
    this->repaint();
}

void ProgressWidget::on_openPushButton_clicked()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::OPEN, "", 0);
}
