#include "progressdialog.h"

static ProgressDialog *theDialog;

ProgressDialog::ProgressDialog(QWidget *parent)
    : QProgressDialog("", "Cancel", 0, 0, parent)
{
    this->setWindowFlags((/*this->windowFlags() |*/ Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint) & ~(Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowContextHelpButtonHint | Qt::MacWindowToolBarButtonHint | Qt::WindowFullscreenButtonHint | Qt::WindowMinMaxButtonsHint));
    this->setMinimumDuration(0);
    this->setMinimum(0);

    theDialog = this;
}

void ProgressDialog::show(const QString &label, int maxValue)
{
    theDialog->setLabelText(label);
    theDialog->setMaximum(maxValue);
    theDialog->setValue(0);
    theDialog->show();
}

void ProgressDialog::hide()
{
    theDialog->hide();
}

bool ProgressDialog::wasCanceled()
{
    return theDialog->wasCanceled();
}

void ProgressDialog::incValue()
{
    theDialog->setValue(theDialog->value() + 1);
}
