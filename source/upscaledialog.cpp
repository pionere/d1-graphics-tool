#include "upscaledialog.h"

#include "d1gfx.h"
#include "mainwindow.h"
#include "ui_upscaledialog.h"

const int fixColors[NUM_DUNGEON_TYPES][2] = {
    // clang-format off
/* DTYPE_TOWN      */ { -1, -1 },
/* DTYPE_CATHEDRAL */ { -1, -1 },
/* DTYPE_CATACOMBS */ { -1, -1 },
/* DTYPE_CAVES     */ {  1, 31 },
/* DTYPE_HELL      */ {  1, 31 },
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

void UpscaleDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
