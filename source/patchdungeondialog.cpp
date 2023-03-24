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
    if (baseName == "sklkng2") {
        fileIndex = DUN_SKELKING_PRE;
    }
    if (baseName == "sklkng1") {
        fileIndex = DUN_SKELKING_AFT;
    }
    if (baseName == "banner2") {
        fileIndex = DUN_BANNER_PRE;
    }
    if (baseName == "banner1") {
        fileIndex = DUN_BANNER_AFT;
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
    if (baseName == "warlord2") {
        fileIndex = DUN_WARLORD_AFT;
    }
    if (baseName == "diab1") {
        fileIndex = DUN_DIAB_1;
    }
    if (baseName == "diab2a") {
        fileIndex = DUN_DIAB_2_PRE;
    }
    if (baseName == "diab2b") {
        fileIndex = DUN_DIAB_2_AFT;
    }
    if (baseName == "diab3a") {
        fileIndex = DUN_DIAB_3_PRE;
    }
    if (baseName == "diab3b") {
        fileIndex = DUN_DIAB_3_AFT;
    }
    if (baseName == "diab4a") {
        fileIndex = DUN_DIAB_4_PRE;
    }
    if (baseName == "diab4b") {
        fileIndex = DUN_DIAB_4_AFT;
    }
    if (baseName == "butcher") {
        fileIndex = DUN_BUTCHER;
    }
    // if (baseName == "vile1") {
    //     fileIndex = DUN_BETRAYER;
    // }
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
