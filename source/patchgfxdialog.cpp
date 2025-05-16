#include "patchgfxdialog.h"

#include "celview.h"
#include "gfxsetview.h"
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

void PatchGfxDialog::initialize(D1Gfx *g, CelView *cv, GfxsetView *gv)
{
    this->gfx = g;
    this->celView = cv;
    this->gfxsetView = gv;

    // initialize the dropdown based on the filename
    QString filePath = g->getFilePath();
    int fileIndex = D1Gfx::getPatchFileIndex(filePath);
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
    if (this->celView != nullptr) {
        this->celView->setGfx(this->gfx);
    }
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->setGfx(this->gfx);
    }

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
