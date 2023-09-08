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

void TrnGenerateColEntryWidget::initialize(const GenerateTrnColor &color)
{
    this->ui->firstColorLineEdit->setText(QString::number(color.firstcolor));
    this->ui->lastColorLineEdit->setText(QString::number(color.lastcolor));
    this->ui->shadeStepsLineEdit->setText(color.shadesteps == 0.0 ? QString("") : QString::number(color.shadesteps));
    this->ui->deltaStepsCheckBox->setChecked(color.deltasteps);
    this->ui->shadeStepsMplLineEdit->setText(color.shadestepsmpl == 1.0 ? QString("") : QString::number(color.shadestepsmpl));
    this->ui->protColCheckBox->setChecked(color.protcolor);
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
    if (color.lastcolor >= D1TRN_TRANSLATIONS) {
        color.lastcolor = D1TRN_TRANSLATIONS - 1;
    }

    color.shadesteps = this->ui->shadeStepsLineEdit->text().toDouble();
    color.deltasteps = this->ui->deltaStepsCheckBox->isChecked();
    bool mplOk;
    color.shadestepsmpl = this->ui->shadeStepsMplLineEdit->text().toDouble(&mplOk);
    if (!mplOk) {
        color.shadestepsmpl = 1.0;
    }
    color.protcolor = this->ui->protColCheckBox->isChecked();
    return color;
}

void TrnGenerateColEntryWidget::on_deletePushButtonClicked()
{
    this->view->on_actionDelRange_triggered(this);
}
