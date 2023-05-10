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
    QString line;
    RemapParam params;

    line = this->ui->colorFrom0LineEdit->text();
    params.colorFrom.first = line.toShort();
    line = this->ui->colorFrom1LineEdit->text();
    if (line.isEmpty()) {
        params.colorFrom.second = params.colorFrom.first;
    } else {
        params.colorFrom.second = line.toShort();
    }
    line = this->ui->colorTo0LineEdit->text();
    params.colorTo.first = line.toShort();
    line = this->ui->colorTo1LineEdit->text();
    if (line.isEmpty()) {
        params.colorTo.second = params.colorTo.first;
    } else {
        params.colorTo.second = line.toShort();
    }
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
