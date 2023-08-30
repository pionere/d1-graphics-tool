#include "trngeneratecolentrywidget.h"

#include <QApplication>
#include <QStyle>

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
    PushButtonWidget::addButton(this, layout, QStyle::SP_TrashIcon, tr("Remove"), this, &TrnGenerateColEntryWidget::on_deletePushButtonClicked);
}

TrnGenerateColEntryWidget::~TrnGenerateColEntryWidget()
{
    delete ui;
}

GenerateTrnColor TrnGenerateColEntryWidget::getTrnColor() const
{
    GenerateTrnColor color;
    bool firstOk, lastOk;
    color.firstfixcolor = this->ui->firstFixColorLineEdit->text().toUShort(&firstOk);
    color.lastfixcolor = this->ui->lastFixColorLineEdit->text().toUShort(&lastOk);
    if (!lastOk) {
        if (!firstOk) {
            color.lastfixcolor = -1;
        } else {
            color.lastfixcolor = D1PAL_COLORS - 1;
        }
    }
    color.shadefixcolor = this->ui->shadeFixColorCheckBox->isChecked();
    return color;
}

void TrnGenerateColEntryWidget::on_deletePushButtonClicked()
{
    this->view->on_actionDelRange_triggered(this);
}
