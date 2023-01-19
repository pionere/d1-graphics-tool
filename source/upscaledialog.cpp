#include "upscaledialog.h"

#include "ui_upscaledialog.h"

typedef enum dungeon_type {
    DTYPE_TOWN,
    DTYPE_CATHEDRAL,
    DTYPE_CATACOMBS,
    DTYPE_CAVES,
    DTYPE_HELL,
    DTYPE_CRYPT,
    DTYPE_NEST,
    DTYPE_NONE,
    NUM_DUNGEON_TYPES,
} dungeon_type;

const int fixColors[NUM_DUNGEON_TYPES][2] = {
    // clang-format off
/* DTYPE_TOWN      */ { -1, -1 },
/* DTYPE_CATHEDRAL */ { -1, -1 },
/* DTYPE_CATACOMBS */ { -1, -1 },
/* DTYPE_CAVES     */ {  1, 31 },
/* DTYPE_HELL      */ { -1, -1 },
/* DTYPE_CRYPT     */ {  1, 31 },
/* DTYPE_NEST      */ {  1, 15 },
/* DTYPE_NONE      */ { -1, -1 },    
    // clang-format on
};

UpscaleDialog::UpscaleDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UpscaleDialog())
{
    ui->setupUi(this);
}

UpscaleDialog::~UpscaleDialog()
{
    delete ui;
}

void UpscaleDialog::initialize(D1Gfx *gfx)
{
    if (this->ui->multiplierLineEdit->text().isEmpty()) {
        this->ui->multiplierLineEdit->setText("2");

        if (gfx->getType() == D1CEL_TYPE::V1_LEVEL) {
            this->ui->antiAliasingModeComboBox->setCurrentIndex(1); // ANTI_ALIASING_MODE::TILESET
        }

        this->ui->firstFixColorLineEdit->setText("-1");
        this->ui->lastFixColorLineEdit->setText("-1");
    }
}

void UpscaleDialog::on_palFileBrowseButton_clicked()
{
    QString palFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, "Select Palette File", "PAL Files (*.pal *.PAL)");

    if (!palFilePath.isEmpty()) {
        this->ui->palFileLineEdit->setText(palFilePath);
    }
}

void UpscaleDialog::on_palFileClearButton_clicked()
{
    this->ui->palFileLineEdit->setText("");
}

void UpscaleDialog::on_levelTypeComboBox_activated(int index)
{
    this->ui->firstFixColorLineEdit->setText(QString::number(fixColors[index][0]));
    this->ui->lastFixColorLineEdit->setText(QString::number(fixColors[index][1]));
}

void UpscaleDialog::on_upscaleButton_clicked()
{
    UpscaleParam params;
    params.multiplier = this->ui->multiplierLineEdit->text().toInt();
    if (params.multiplier <= 1) {
        this->close();
        return;
    }
    params.palPath = this->ui->palFileLineEdit->text();
    bool firstOk, lastOk;
    params.firstfixcolor = this->ui->firstFixColorLineEdit->text().toUInt(&firstOk);
    params.lastfixcolor = this->ui->lastFixColorLineEdit->text().toUInt(&lastOk);
    params.antiAliasingMode = (ANTI_ALIASING_MODE)this->ui->antiAliasingModeComboBox->currentIndex();

    if (!lastOk) {
        if (!firstOk) {
            params.lastfixcolor = -1;
        } else {
            params.lastfixcolor = D1PAL_COLORS - 1;
        }
    }

    MainWindow *qw = (MainWindow *)this->parentWidget();
    this->close();

    qw->upscale(params);
}

void UpscaleDialog::on_upscaleCancelButton_clicked()
{
    this->close();
}
