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

    // initialize the dropdown based on the filename
    int fileIndex = -1;
    QString baseName = QFileInfo(d->getFilePath()).completeBaseName().toLower();
    if (baseName == "skngdo") {
        fileIndex = DUN_SKELKING_ENTRY;
    }
    if (baseName == "bonestr1") {
        fileIndex = DUN_BONECHAMB_ENTRY_PRE;
    }
    if (baseName == "bonestr2") {
        fileIndex = DUN_BONECHAMB_ENTRY_AFT;
    }
    if (baseName == "bonecha1") {
        fileIndex = DUN_BONECHAMB_PRE;
    }
    if (baseName == "bonecha2") {
        fileIndex = DUN_BONECHAMB_AFT;
    }
    if (baseName == "blind2") {
        fileIndex = DUN_BLIND_PRE;
    }
    if (baseName == "blind1") {
        fileIndex = DUN_BLIND_AFT;
    }
    if (baseName == "blood2") {
        fileIndex = DUN_BLOOD_PRE;
    }
    if (baseName == "blood1") {
        fileIndex = DUN_BLOOD_AFT;
    }
    if (baseName == "vile2") {
        fileIndex = DUN_VILE_PRE;
    }
    if (baseName == "vile1") {
        fileIndex = DUN_VILE_AFT;
    }
    if (baseName == "warlord") {
        fileIndex = DUN_WARLORD_PRE;
    }
    this->ui->dunFileComboBox->setCurrentIndex(fileIndex);
}

void PatchDungeonDialog::on_runButton_clicked()
{
    int fileIndex = this->ui->dunFileComboBox->currentIndex();

    this->close();

    if (fileIndex == -1) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->dun->patch(fileIndex);

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
