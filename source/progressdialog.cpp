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
    theDialog->ui->progressLabel->setText(label);
    theDialog->ui->progressBar->setRange(0, maxValue);
    theDialog->ui->progressBar->setValue(0);
    theDialog->ui->outputTextEdit->setText("");
    theDialog->ui->detailsGroupBox->setVisible(false);
    theDialog->on_detailsPushButton_clicked();
    theDialog->ui->cancelPushButton->setEnabled(true);
    theDialog->cancelled = false;
    theDialog->show();
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

ProgressDialog &ProgressDialog::operator<<(QString text)
{
    this->ui->outputTextEdit->appendPlainText(text);
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
