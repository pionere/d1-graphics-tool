#include "trngeneratecolentrywidget.h"

#include <QApplication>
#include <QStyle>

#include "d1trn.h"
#include "trngeneratedialog.h"
#include "pushbuttonwidget.h"
#include "ui_trngeneratecolentrywidget.h"

TrnGenerateColEntryWidget::TrnGenerateColEntryWidget(TrnGenerateDialog *parent)
    : QWidget(parent)
    , ui(new Ui::TrnGenerateColEntryWidget())
    , view(parent)
{
    ui->setupUi(this);

    QLayout *layout = this->ui->entryHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_TitleBarCloseButton, tr("Remove"), this, &TrnGenerateColEntryWidget::on_deletePushButtonClicked);
}

TrnGenerateColEntryWidget::~TrnGenerateColEntryWidget()
{
    delete ui;
}

GenerateTrnColor TrnGenerateColEntryWidget::getTrnColor() const
{
    GenerateTrnColor color;
    bool firstOk, lastOk;
    color.firstcolor = this->ui->firstColorLineEdit->text().toUShort(&firstOk);
    color.lastcolor = this->ui->lastColorLineEdit->text().toUShort(&lastOk);
    if (!lastOk) {
        if (!firstOk) {
            color.lastcolor = -1;
        } else {
            color.lastcolor = D1TRN_TRANSLATIONS - 1;
        }
    }
    color.shadecolor = this->ui->shadeColorCheckBox->isChecked();
    return color;
}

void TrnGenerateColEntryWidget::on_deletePushButtonClicked()
{
    this->view->on_actionDelRange_triggered(this);
}
