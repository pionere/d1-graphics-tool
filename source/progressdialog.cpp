#include "progressdialog.h"

#include "ui_progressdialog.h"

static ProgressDialog *theDialog;

ProgressDialog::ProgressDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProgressDialog())
{
    this->ui->setupUi(this);

    // this->setWindowFlags((/*this->windowFlags() |*/ Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint) & ~(Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowContextHelpButtonHint | Qt::MacWindowToolBarButtonHint | Qt::WindowFullscreenButtonHint | Qt::WindowMinMaxButtonsHint));

    theDialog = this;
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::start(const QString &label, int maxValue)
{
    theDialog->ui->progressLabel->setText(label);
    // theDialog->ui->progressBar->setRange(0, maxValue);
    theDialog->setValue_impl(0);
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
    // theDialog->setValue_impl(theDialog->ui->progressBar->value() + 1);
    // theDialog->ui->progressBar->repaint();
    theDialog->show();
}

void ProgressDialog::on_cancelPushButton_clicked()
{
    this->cancelled = true;
}

void ProgressDialog::closeEvent(QCloseEvent *e)
{
    this->on_cancelPushButton_clicked();
    QDialog::closeEvent(e);
}

void ProgressDialog::setValue_impl(int value)
{
    // this->ui->progressBar->setValue(value);
    // this->ui->progressBar->setFormat(QString("%1%%").arg(value * 100 / this->ui->progressBar->maximum()));
}
