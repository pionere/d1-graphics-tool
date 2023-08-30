#include "trngeneratedialog.h"

#include <QMessageBox>

#include "d1trs.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_trngeneratedialog.h"

TrnGenerateDialog::TrnGenerateDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TrnGenerateDialog())
{
    this->ui->setupUi(this);

    QHBoxLayout *layout = this->ui->addRangeButtonLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogNewFolder, tr("Add"), this, &TrnGenerateDialog::on_actionAddRange_triggered);
    layout->addStretch();
    layout = this->ui->addPaletteButtonLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogNewFolder, tr("Add"), this, &TrnGenerateDialog::on_actionAddPalette_triggered);
    layout->addStretch();
}

TrnGenerateDialog::~TrnGenerateDialog()
{
    delete ui;
}

void TrnGenerateDialog::initialize(D1Pal *p)
{
    for (auto it = this->pals.begin(); it != this->pals.end(); ) {
        if (it->isNull()) {
            it = this->pals.erase(it);
        } else {
            it++;
        }
    }
    if (this->pals.empty()) {
        this->pals.push_back(p);
    }
}

void TrnGenerateDialog::on_actionAddRange_triggered()
{
    // QRandomGenerator *gen = QRandomGenerator::global();
    // this->ui->seedLineEdit->setText(QString::number((int)gen->generate()));
}

void TrnGenerateDialog::on_actionAddPalette_triggered()
{
    // QRandomGenerator *gen = QRandomGenerator::global();
    // this->ui->questSeedLineEdit->setText(QString::number((int)gen->generate()));
}

void TrnGenerateDialog::on_generateButton_clicked()
{
    GenerateTrnParam params;
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
    params.colors.push_back(color);

    for (const QPointer<D1Pal> pal : this->pals) {
        params.pals.push_back(pal);
    }

    this->close();

    dMainWindow().generateTrn(params);
}

void TrnGenerateDialog::on_cancelButton_clicked()
{
    this->close();
}
