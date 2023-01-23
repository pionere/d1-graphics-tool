#include "progressdialog.h"

#include "ui_progressdialog.h"

static ProgressDialog *theDialog;

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

void ProgressDialog::start(const QString &label, int maxValue)
{
    theDialog->show();

    theDialog->ui->progressLabel->setText(label);
    theDialog->ui->progressBar->setRange(0, maxValue);
    theDialog->ui->progressBar->setValue(0);
    theDialog->ui->outputTextEdit->clear();
    theDialog->ui->detailsGroupBox->setVisible(true);
    theDialog->on_detailsPushButton_clicked();
    theDialog->ui->cancelPushButton->setEnabled(true);
    theDialog->cancelled = false;
}

void ProgressDialog::done()
{
    theDialog->hide();
}

bool ProgressDialog::wasCanceled()
{
    return theDialog->cancelled;
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
    return *this;
}

ProgressDialog &ProgressDialog::operator<<(const QPair<QString, QString> &text)
{
    this->ui->outputTextEdit->setFocus();
    QTextCursor storeCursorPos = this->ui->outputTextEdit->textCursor();
    this->ui->outputTextEdit->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    this->ui->outputTextEdit->moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    this->ui->outputTextEdit->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
    if (this->ui->outputTextEdit->textCursor().selectedText() == text.first) {
        this->ui->outputTextEdit->textCursor().removeSelectedText();
        this->ui->outputTextEdit->textCursor().deletePreviousChar(); // Added to trim the newline char when removing last line
    }
    QTextCursor lastCursorPos = this->ui->outputTextEdit->textCursor();
    this->ui->outputTextEdit->appendPlainText(text.second);
    if (lastCursorPos > storeCursorPos) {
        this->ui->outputTextEdit->setTextCursor(storeCursorPos);
    }
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
    this->cancelled = true;
    this->ui->cancelPushButton->setEnabled(false);
}

void ProgressDialog::closeEvent(QCloseEvent *e)
{
    this->on_cancelPushButton_clicked();
    // QDialog::closeEvent(e);
}
