#include "patchtilesetdialog.h"

#include "progressdialog.h"
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

void PatchTilesetDialog::initialize(D1Tileset *ts, D1Dun *d)
{
    this->tileset = ts;
    this->dun = d;

    // initialize the dropdown based on the filename
    int dungeonType = -1;
    QString baseName = QFileInfo(ts->til->getFilePath()).completeBaseName().toLower();
    if (baseName == "town") {
        dungeonType = DTYPE_TOWN;
    }
    if (baseName.length() == 2 && baseName[0] == 'l') {
        switch (baseName[1].digitValue()) {
        case 1:
            dungeonType = DTYPE_CATHEDRAL;
            break;
        case 2:
            dungeonType = DTYPE_CATACOMBS;
            break;
        case 3:
            dungeonType = DTYPE_CAVES;
            break;
        case 4:
            dungeonType = DTYPE_HELL;
            break;
        case 5:
            dungeonType = DTYPE_CRYPT;
            break;
        case 6:
            dungeonType = DTYPE_NEST;
            break;
        }
    }
    this->ui->dungeonTypeComboBox->setCurrentIndex(dungeonType);
}

void PatchTilesetDialog::on_runButton_clicked()
{
    int dungeonType = this->ui->dungeonTypeComboBox->currentIndex();

    this->close();

    if (dungeonType == -1) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->tileset->patch(dungeonType, false);

    if (this->dun != nullptr) {
        this->dun->refreshSubtiles();
    }

    // Clear loading message from status bar
    ProgressDialog::done();
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
