#include "remapdialog.h"

#include "mainwindow.h"
#include "ui_remapdialog.h"

RemapDialog::RemapDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RemapDialog())
{
    ui->setupUi(this);
}

RemapDialog::~RemapDialog()
{
    delete ui;
}

void RemapDialog::initialize(PaletteWidget *palWidget)
{
    std::pair<int, int> currentColors = palWidget->getCurrentSelection();

    this->ui->colorFrom0LineEdit->setText(QString::number(currentColors.first));
    this->ui->colorFrom1LineEdit->setText(QString::number(currentColors.second));
    this->ui->colorTo0LineEdit->setText("");
    this->ui->colorTo1LineEdit->setText("");
    // this->ui->range0LineEdit->setText("");
    // this->ui->range1LineEdit->setText("");
}

void RemapDialog::on_remapButton_clicked()
{
    RemapParam params;

    params.colorFrom.first = this->ui->colorFrom0LineEdit->nonNegInt();
    params.colorFrom.second = this->ui->colorFrom1LineEdit->nonNegInt();
    params.colorTo.first = this->ui->colorTo0LineEdit->nonNegInt();
    params.colorTo.second = this->ui->colorTo1LineEdit->nonNegInt();
    params.frames.first = this->ui->range0LineEdit->nonNegInt();
    params.frames.second = this->ui->range1LineEdit->nonNegInt();

    if (params.colorFrom.first > D1PAL_COLORS) {
        params.colorFrom.first = D1PAL_COLORS;
    }
    if (params.colorFrom.second > D1PAL_COLORS) {
        params.colorFrom.second = D1PAL_COLORS;
    }
    if (params.colorTo.first > D1PAL_COLORS) {
        params.colorTo.first = D1PAL_COLORS;
    }
    if (params.colorTo.second > D1PAL_COLORS) {
        params.colorTo.second = D1PAL_COLORS;
    }

    this->close();

    dMainWindow().changeColors(params);
}

void RemapDialog::on_remapCancelButton_clicked()
{
    this->close();
}

void RemapDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
