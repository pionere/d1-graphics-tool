#include "openasdialog.h"

#include <QFileDialog>
#include <QMessageBox>

#include "ui_openasdialog.h"

#include "mainwindow.h"

OpenAsDialog::OpenAsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OpenAsDialog)
{
    this->ui->setupUi(this);
}

OpenAsDialog::~OpenAsDialog()
{
    delete ui;
}

void OpenAsDialog::initialize()
{
    // clear the input fields
    this->ui->inputFileEdit->setText("");
    this->ui->isTilesetAutoRadioButton->setChecked(true);
    // - celSettingsGroupBox
    this->ui->celWidthEdit->setText("0");
    this->ui->celClippedAutoRadioButton->setChecked(true);
    // - tilSettingsGroupBox
    this->ui->minUpscaledAutoRadioButton->setChecked(true);
    this->ui->tilFileEdit->setText("");
    this->ui->minFileEdit->setText("");
    this->ui->solFileEdit->setText("");
    this->ui->ampFileEdit->setText("");
    this->ui->sptFileEdit->setText("");
    this->ui->tmiFileEdit->setText("");
    this->ui->dunFileEdit->setText("");
    this->ui->minWidthEdit->setText("0");
    this->ui->minHeightEdit->setText("0");
    this->ui->createDunCheckBox->setChecked(false);
    // - tblSettingsGroupBox
    this->ui->tblFileEdit->setText("");

    this->update();
}

void OpenAsDialog::update()
{
    QString filePath = this->ui->inputFileEdit->text();
    bool hasInputFile = !filePath.isEmpty();
    bool isTileset = this->ui->isTilesetYesRadioButton->isChecked() || (this->ui->isTilesetAutoRadioButton->isChecked() && !this->ui->tilFileEdit->text().isEmpty());
    bool isMeta = !isTileset && filePath.toLower().endsWith(".tbl");

    this->ui->celSettingsGroupBox->setEnabled(hasInputFile && !isTileset && !isMeta);
    this->ui->tilSettingsGroupBox->setEnabled(hasInputFile && isTileset);
    this->ui->tblSettingsGroupBox->setEnabled(hasInputFile && isMeta);
}

void OpenAsDialog::on_inputFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Graphics"), tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2);;PCX Files (*.pcx *.PCX);;TBL Files (*.tbl *.TBL)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->inputFileEdit->setText(openFilePath);
    // activate optional fields based on the extension
    if (this->ui->isTilesetAutoRadioButton->isChecked()) {
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
            this->ui->tilFileEdit->setText(tilFilePath);
            this->ui->minFileEdit->setText(minFilePath);
            this->ui->solFileEdit->setText(solFilePath);
            QString ampFilePath = basePath + "amp";
            if (QFileInfo::exists(ampFilePath)) {
                this->ui->ampFileEdit->setText(ampFilePath);
            } else {
                this->ui->ampFileEdit->setText("");
            }
            QString sptFilePath = basePath + "spt";
            if (QFileInfo::exists(sptFilePath)) {
                this->ui->sptFileEdit->setText(sptFilePath);
            } else {
                this->ui->sptFileEdit->setText("");
            }
            QString tmiFilePath = basePath + "tmi";
            if (QFileInfo::exists(tmiFilePath)) {
                this->ui->tmiFileEdit->setText(tmiFilePath);
            } else {
                this->ui->tmiFileEdit->setText("");
            }
        } else {
            this->ui->tilFileEdit->setText("");
            this->ui->minFileEdit->setText("");
            this->ui->solFileEdit->setText("");
            this->ui->ampFileEdit->setText("");
            this->ui->sptFileEdit->setText("");
            this->ui->tmiFileEdit->setText("");
        }
    }
    QString tblPath;
    if (openFilePath.toLower().endsWith(".tbl") && !this->ui->isTilesetYesRadioButton->isChecked()) {
        if (openFilePath.toLower().endsWith("dark.tbl")) {
            tblPath = openFilePath;
            tblPath.replace(tblPath.length() - (sizeof("dark.tbl") - 2), 3, "ist");
        } else if (openFilePath.toLower().endsWith("dist.tbl")) {
            tblPath = openFilePath;
            tblPath.replace(tblPath.length() - (sizeof("dist.tbl") - 2), 3, "ark");
        }
        if (!QFileInfo::exists(tblPath))
            tblPath = "";
    }
    this->ui->tblFileEdit->setText(tblPath);

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

    this->ui->minFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_solFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select SOL file"), tr("SOL Files (*.sol *.SOL)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->solFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_ampFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select AMP file"), tr("AMP Files (*.amp *.AMP)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->ampFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_sptFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select SPT file"), tr("SPT Files (*.spt *.SPT)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->sptFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_tmiFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select TMI file"), tr("TMI Files (*.tmi *.TMI)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->tmiFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_dunFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select DUN file"), tr("DUN Files (*.dun *.DUN *.rdun *.RDUN)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->dunFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_tblFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select TBL file"), tr("TBL Files (*.tbl *.TBL)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->tblFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_openButton_clicked()
{
    OpenAsParam params;
    params.celFilePath = this->ui->inputFileEdit->text();
    if (params.celFilePath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Input file is missing, please choose an input file."));
        return;
    }
    if (this->ui->isTilesetYesRadioButton->isChecked()) {
        params.isTileset = OPEN_TILESET_TYPE::TRUE;
    } else if (this->ui->isTilesetNoRadioButton->isChecked()) {
        params.isTileset = OPEN_TILESET_TYPE::FALSE;
    } else {
        params.isTileset = OPEN_TILESET_TYPE::AUTODETECT;
    }
    // cel/cl2/pcx: clipped, width
    params.celWidth = this->ui->celWidthEdit->nonNegInt();
    if (this->ui->celClippedYesRadioButton->isChecked()) {
        params.clipped = OPEN_CLIPPED_TYPE::TRUE;
    } else if (this->ui->celClippedNoRadioButton->isChecked()) {
        params.clipped = OPEN_CLIPPED_TYPE::FALSE;
    } else {
        params.clipped = OPEN_CLIPPED_TYPE::AUTODETECT;
    }
    // tileset: upscaled, width, height
    if (this->ui->minUpscaledYesRadioButton->isChecked()) {
        params.upscaled = OPEN_UPSCALED_TYPE::TRUE;
    } else if (this->ui->minUpscaledNoRadioButton->isChecked()) {
        params.upscaled = OPEN_UPSCALED_TYPE::FALSE;
    } else {
        params.upscaled = OPEN_UPSCALED_TYPE::AUTODETECT;
    }
    params.tilFilePath = this->ui->tilFileEdit->text();
    params.minFilePath = this->ui->minFileEdit->text();
    params.solFilePath = this->ui->solFileEdit->text();
    params.ampFilePath = this->ui->ampFileEdit->text();
    params.sptFilePath = this->ui->sptFileEdit->text();
    params.tmiFilePath = this->ui->tmiFileEdit->text();
    params.dunFilePath = this->ui->dunFileEdit->text();
    params.createDun = this->ui->createDunCheckBox->isChecked();
    params.minWidth = this->ui->minWidthEdit->nonNegInt();
    params.minHeight = this->ui->minHeightEdit->nonNegInt();
    params.tblFilePath = this->ui->tblFileEdit->text();

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
