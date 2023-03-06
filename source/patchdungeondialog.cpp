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
    QString filePath = d->getFilePath().toLower();
    if (filePath.endsWith(".dun")) {
        filePath.chop(4);
    }
    if (filePath.endsWith("skngdo")) {
        fileIndex = DUN_SKELKING_ENTRY;
    }
    if (filePath.endsWith("bonestr1")) {
        fileIndex = DUN_BONECHAMB_ENTRY_PRE;
    }
    if (filePath.endsWith("bonestr2")) {
        fileIndex = DUN_BONECHAMB_ENTRY_AFT;
    }
    if (filePath.endsWith("bonecha1")) {
        fileIndex = DUN_BONECHAMB_PRE;
    }
    if (filePath.endsWith("bonecha2")) {
        fileIndex = DUN_BONECHAMB_AFT;
    }
    if (filePath.endsWith("blind2")) {
        fileIndex = DUN_BLIND_PRE;
    }
    if (filePath.endsWith("blind1")) {
        fileIndex = DUN_BLIND_AFT;
    }
    if (filePath.endsWith("blood2")) {
        fileIndex = DUN_BLOOD_PRE;
    }
    if (filePath.endsWith("blood1")) {
        fileIndex = DUN_BLOOD_AFT;
    }
    if (filePath.endsWith("vile2")) {
        fileIndex = DUN_VILE_PRE;
    }
    if (filePath.endsWith("vile1")) {
        fileIndex = DUN_VILE_AFT;
    }
    this->ui->dungeonTypeComboBox->setCurrentIndex(fileIndex);
}

void PatchDungeonDialog::on_runButton_clicked()
{
    int dunFileIndex = this->ui->dungeonTypeComboBox->currentIndex();

    if (dunFileIndex == -1) {
        this->close();
        return;
    }

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
