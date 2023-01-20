#include "progressdialog.h"

#include <QProgressDialog>

class ProgressDialogWindow : public QProgressDialog {
    Q_OBJECT

public:
    explicit ProgressDialogWindow();
    ~ProgressDialogWindow() = default;
};

static ProgressDialogWindow theDialog;

ProgressDialogWindow::ProgressDialogWindow()
    : QProgressDialog("", "Cancel", 0, 0, nullptr)
{
    this->setWindowFlags((/*this->windowFlags() |*/ Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint) & ~(Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowContextHelpButtonHint | Qt::MacWindowToolBarButtonHint | Qt::WindowFullscreenButtonHint | Qt::WindowMinMaxButtonsHint));
    this->setMinimumDuration(0);
    this->setMinimum(0);
}

void ProgressDialog::initialize(QWidget *window)
{
    theDialog.setParent(window);
}

void ProgressDialog::show(const QString &label, int maxValue)
{
    theDialog.setLabelText(label);
    theDialog.setMaximum(maxValue);
    theDialog.setValue(0);
    theDialog.show();
}

void ProgressDialog::hide()
{
    theDialog.hide();
}

bool ProgressDialog::wasCanceled()
{
    return theDialog.wasCanceled();
}

void ProgressDialog::incValue()
{
    theDialog.setValue(theDialog.value() + 1);
}

#include "progressdialog.moc"
