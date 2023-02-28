#include "openasdialog.h"

#include <QFileDialog>
#include <QMessageBox>

#include "ui_openasdialog.h"

#include "mainwindow.h"

OpenAsDialog::OpenAsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OpenAsDialog)
{
    ui->setupUi(this);
}

OpenAsDialog::~OpenAsDialog()
{
    delete ui;
}

void OpenAsDialog::initialize()
{
    // clear the input fields
    ui->inputFileEdit->setText("");
    ui->isTilesetAutoRadioButton->setChecked(true);
    // - celSettingsGroupBox
    ui->celWidthEdit->setText("0");
    ui->celClippedAutoRadioButton->setChecked(true);
    // - tilSettingsGroupBox
    ui->minUpscaledAutoRadioButton->setChecked(true);
    ui->tilFileEdit->setText("");
    ui->minFileEdit->setText("");
    ui->solFileEdit->setText("");
    ui->ampFileEdit->setText("");
    ui->tmiFileEdit->setText("");
    ui->dunFileEdit->setText("");
    ui->minWidthEdit->setText("0");
    ui->minHeightEdit->setText("0");

    this->update();
}

void OpenAsDialog::update()
{
    bool hasInputFile = !ui->inputFileEdit->text().isEmpty();
    bool isTileset = ui->isTilesetYesRadioButton->isChecked() || (ui->isTilesetAutoRadioButton->isChecked() && !ui->tilFileEdit->text().isEmpty());

    ui->celSettingsGroupBox->setEnabled(hasInputFile && !isTileset);
    ui->tilSettingsGroupBox->setEnabled(hasInputFile && isTileset);
}

void OpenAsDialog::on_inputFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Graphics"), tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2);;PCX Files (*.pcx *.PCX)"));

    if (openFilePath.isEmpty())
        return;

    ui->inputFileEdit->setText(openFilePath);
    // activate optional fields based on the extension
    if (ui->isTilesetAutoRadioButton->isChecked()) {
        bool isTileset = false;
        QString basePath;
        QString tilFilePath;
        QString minFilePath;
        QString solFilePath;
        if (openFilePath.toLower().endsWith(".cel")) {
            // If a SOL, MIN and TIL files exists then preset them
            basePath = openFilePath;
            basePath.chop(3);
            tilFilePath = basePath + "til";
            minFilePath = basePath + "min";
            solFilePath = basePath + "sol";
            isTileset = QFileInfo::exists(tilFilePath) && QFileInfo::exists(minFilePath) && QFileInfo::exists(solFilePath);
        }
        if (isTileset) {
            ui->tilFileEdit->setText(tilFilePath);
            ui->minFileEdit->setText(minFilePath);
            ui->solFileEdit->setText(solFilePath);
            QString ampFilePath = basePath + "amp";
            if (QFileInfo::exists(ampFilePath)) {
                ui->ampFileEdit->setText(ampFilePath);
            } else {
                ui->ampFileEdit->setText("");
            }
            QString tmiFilePath = basePath + "tmi";
            if (QFileInfo::exists(tmiFilePath)) {
                ui->tmiFileEdit->setText(tmiFilePath);
            } else {
                ui->tmiFileEdit->setText("");
            }
        } else {
            ui->tilFileEdit->setText("");
            ui->minFileEdit->setText("");
            ui->solFileEdit->setText("");
            ui->ampFileEdit->setText("");
            ui->tmiFileEdit->setText("");
        }
    }

    this->update();
}

void OpenAsDialog::on_isTilesetYesRadioButton_toggled(bool checked)
{
    this->update();
}

void OpenAsDialog::on_isTilesetNoRadioButton_toggled(bool checked)
{
    this->update();
}

void OpenAsDialog::on_isTilesetAutoRadioButton_toggled(bool checked)
{
    this->update();
}

void OpenAsDialog::on_tilFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select TIL file"), tr("TIL Files (*.til *.TIL)"));

    if (openFilePath.isEmpty())
        return;

    ui->tilFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_minFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select MIN file"), tr("MIN Files (*.min *.MIN)"));

    if (openFilePath.isEmpty())
        return;

    ui->minFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_solFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select SOL file"), tr("SOL Files (*.sol *.SOL)"));

    if (openFilePath.isEmpty())
        return;

    ui->solFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_ampFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select AMP file"), tr("AMP Files (*.amp *.AMP)"));

    if (openFilePath.isEmpty())
        return;

    ui->ampFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_tmiFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select TMI file"), tr("TMI Files (*.tmi *.TMI)"));

    if (openFilePath.isEmpty())
        return;

    ui->tmiFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_dunFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select DUN file"), tr("DUN Files (*.dun *.DUN)"));

    if (openFilePath.isEmpty())
        return;

    ui->dunFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_openButton_clicked()
{
    OpenAsParam params;
    params.celFilePath = ui->inputFileEdit->text();
    if (params.celFilePath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Input file is missing, please choose an input file."));
        return;
    }
    if (ui->isTilesetYesRadioButton->isChecked()) {
        params.isTileset = OPEN_TILESET_TYPE::TRUE;
    } else if (ui->isTilesetNoRadioButton->isChecked()) {
        params.isTileset = OPEN_TILESET_TYPE::FALSE;
    } else {
        params.isTileset = OPEN_TILESET_TYPE::AUTODETECT;
    }
    // cel/cl2/pcx: clipped, width
    params.celWidth = this->ui->celWidthEdit->nonNegInt();
    if (ui->celClippedYesRadioButton->isChecked()) {
        params.clipped = OPEN_CLIPPED_TYPE::TRUE;
    } else if (ui->celClippedNoRadioButton->isChecked()) {
        params.clipped = OPEN_CLIPPED_TYPE::FALSE;
    } else {
        params.clipped = OPEN_CLIPPED_TYPE::AUTODETECT;
    }
    // tileset: upscaled, width, height
    if (ui->minUpscaledYesRadioButton->isChecked()) {
        params.upscaled = OPEN_UPSCALED_TYPE::TRUE;
    } else if (ui->minUpscaledNoRadioButton->isChecked()) {
        params.upscaled = OPEN_UPSCALED_TYPE::FALSE;
    } else {
        params.upscaled = OPEN_UPSCALED_TYPE::AUTODETECT;
    }
    params.tilFilePath = ui->tilFileEdit->text();
    params.minFilePath = ui->minFileEdit->text();
    params.solFilePath = ui->solFileEdit->text();
    params.ampFilePath = ui->ampFileEdit->text();
    params.tmiFilePath = ui->tmiFileEdit->text();
    params.dunFilePath = ui->dunFileEdit->text();
    params.minWidth = ui->minWidthEdit->nonNegInt();
    params.minHeight = ui->minHeightEdit->nonNegInt();

    this->close();

    dMainWindow().openFile(params);
}

void OpenAsDialog::on_openCancelButton_clicked()
{
    this->close();
}

void OpenAsDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
