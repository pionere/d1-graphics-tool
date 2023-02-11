#include "patchtilesetdialog.h"

#include "ui_patchtilesetdialog.h"

PatchTilesetDialog::PatchTilesetDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PatchTilesetDialog())
{
    ui->setupUi(this);
}

PatchTilesetDialog::~PatchTilesetDialog()
{
    delete ui;
}

void PatchTilesetDialog::initialize(D1Tileset *ts)
{
    this->tileset = ts;
}

void PatchTilesetDialog::on_runButton_clicked()
{
    int dungeonType = this->ui->dungeonTypeComboBox->currentIndex();

    this->close();

    this->tileset->patch(dungeonType, false);
}

void PatchTilesetDialog::on_cancelButton_clicked()
{
    this->close();
}

void PatchTilesetDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
