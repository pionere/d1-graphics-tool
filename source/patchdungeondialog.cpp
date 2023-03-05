#include "patchdungeondialog.h"

#include "progressdialog.h"
#include "ui_patchdungeondialog.h"

PatchDungeonDialog::PatchDungeonDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PatchDungeonDialog())
{
    ui->setupUi(this);
}

PatchDungeonDialog::~PatchDungeonDialog()
{
    delete ui;
}

void PatchDungeonDialog::initialize(D1Dun *d)
{
    this->dun = d;
}

void PatchDungeonDialog::on_runButton_clicked()
{
    int dunFileIndex = this->ui->dungeonTypeComboBox->currentIndex();

    this->close();

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->dun->patch(dunFileIndex);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void PatchDungeonDialog::on_cancelButton_clicked()
{
    this->close();
}

void PatchDungeonDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
