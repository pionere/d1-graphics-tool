#include "patchgfxdialog.h"

#include "progressdialog.h"
#include "ui_patchgfxdialog.h"

PatchGfxDialog::PatchGfxDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PatchGfxDialog())
{
    ui->setupUi(this);
}

PatchGfxDialog::~PatchGfxDialog()
{
    delete ui;
}

void PatchGfxDialog::initialize(D1Gfx *g, CelView *cv)
{
    this->gfx = g;
    this->celView = cv;

    // initialize the dropdown based on the filename
    int fileIndex = -1;
    QString baseName = QFileInfo(g->getFilePath()).completeBaseName().toLower();
    if (baseName == "l1doors") {
        fileIndex = GFX_OBJ_L1DOORS;
    }
    if (baseName == "l2doors") {
        fileIndex = GFX_OBJ_L2DOORS;
    }
    if (baseName == "l3doors") {
        fileIndex = GFX_OBJ_L3DOORS;
    }
    if (baseName == "mcirl") {
        fileIndex = GFX_OBJ_MCIRL;
    }
    if (baseName == "lshrineg") {
        fileIndex = GFX_OBJ_LSHR;
    }
    if (baseName == "rshrineg") {
        fileIndex = GFX_OBJ_RSHR;
    }
    if (baseName == "l5light") {
        fileIndex = GFX_OBJ_L5LIGHT;
    }
    if (baseName == "wmhas") {
        fileIndex = GFX_PLR_WMHAS;
    }
    if (baseName == "spelicon") {
        fileIndex = GFX_SPL_ICONS;
    }
    this->ui->gfxFileComboBox->setCurrentIndex(fileIndex);
}

void PatchGfxDialog::on_runButton_clicked()
{
    int fileIndex = this->ui->gfxFileComboBox->currentIndex();

    this->close();

    if (fileIndex == -1) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->gfx->patch(fileIndex, false);

    // trigger the update of the selected indices
    this->celView->setGfx(this->gfx);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void PatchGfxDialog::on_cancelButton_clicked()
{
    this->close();
}

void PatchGfxDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
