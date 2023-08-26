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

    this->ui->celTypeButtonGroup->setId(this->ui->celTypeAutoRadioButton, (int)OPEN_GFX_TYPE::AUTODETECT);
    this->ui->celTypeButtonGroup->setId(this->ui->celTypeBasicRadioButton, (int)OPEN_GFX_TYPE::BASIC);
    this->ui->celTypeButtonGroup->setId(this->ui->celTypeTilesetRadioButton, (int)OPEN_GFX_TYPE::TILESET);
    this->ui->celTypeButtonGroup->setId(this->ui->celTypeGfxsetRadioButton, (int)OPEN_GFX_TYPE::GFXSET);
}

OpenAsDialog::~OpenAsDialog()
{
    delete ui;
}

void OpenAsDialog::initialize()
{
    // clear the input fields
    this->ui->inputFileEdit->setText("");
    this->ui->celTypeAutoRadioButton->setChecked(true);
    // - celSettingsGroupBox
    this->ui->celWidthEdit->setText("0");
    this->ui->celClippedAutoRadioButton->setChecked(true);
    // - tilSettingsGroupBox
    this->ui->minUpscaledAutoRadioButton->setChecked(true);
    this->ui->clsFileEdit->setText("");
    this->ui->tilFileEdit->setText("");
    this->ui->minFileEdit->setText("");
    this->ui->slaFileEdit->setText("");
    this->ui->tlaFileEdit->setText("");
    this->ui->dunFileEdit->setText("");
    this->ui->minWidthEdit->setText("0");
    this->ui->minHeightEdit->setText("0");
    this->ui->createDunCheckBox->setChecked(false);
    // - tblSettingsGroupBox
    this->ui->tblFileEdit->setText("");

    this->updateFields();
}

void OpenAsDialog::updateFields()
{
    QString filePath = this->ui->inputFileEdit->text();
    bool hasInputFile = !filePath.isEmpty();
    bool isTileset = this->ui->celTypeTilesetRadioButton->isChecked() || (this->ui->celTypeAutoRadioButton->isChecked() && !this->ui->tilFileEdit->text().isEmpty());
    bool isMeta = !isTileset && !this->ui->celTypeGfxsetRadioButton->isChecked() && filePath.toLower().endsWith(".tbl");

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
    if (this->ui->celTypeAutoRadioButton->isChecked()) {
        bool isTileset = false;
        QString basePath;
        QString tilFilePath;
        QString minFilePath;
        QString slaFilePath;
        if (openFilePath.toLower().endsWith(".cel")) {
            // If a SLA, MIN and TIL files exists then preset them
            basePath = openFilePath;
            basePath.chop(4);
            tilFilePath = basePath + ".til";
            minFilePath = basePath + ".min";
            slaFilePath = basePath + ".sla";
            isTileset = QFileInfo::exists(tilFilePath) && QFileInfo::exists(minFilePath) && QFileInfo::exists(slaFilePath);
        }
        if (isTileset) {
            QString clsFilePath = basePath + "s.cel";
            if (QFileInfo::exists(clsFilePath)) {
                this->ui->clsFileEdit->setText(clsFilePath);
            } else {
                this->ui->clsFileEdit->setText("");
            }
            this->ui->tilFileEdit->setText(tilFilePath);
            this->ui->minFileEdit->setText(minFilePath);
            this->ui->slaFileEdit->setText(slaFilePath);
            QString tlaFilePath = basePath + ".tla";
            if (QFileInfo::exists(tlaFilePath)) {
                this->ui->tlaFileEdit->setText(tlaFilePath);
            } else {
                this->ui->tlaFileEdit->setText("");
            }
        } else {
            this->ui->clsFileEdit->setText("");
            this->ui->tilFileEdit->setText("");
            this->ui->minFileEdit->setText("");
            this->ui->slaFileEdit->setText("");
            this->ui->tlaFileEdit->setText("");
        }
    }
    QString tblPath;
    if (openFilePath.toLower().endsWith(".tbl") && !this->ui->celTypeTilesetRadioButton->isChecked() && !this->ui->celTypeGfxsetRadioButton->isChecked()) {
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

    this->updateFields();
}

void OpenAsDialog::on_isTilesetYesRadioButton_toggled(bool checked)
{
    this->updateFields();
}

void OpenAsDialog::on_isTilesetNoRadioButton_toggled(bool checked)
{
    this->updateFields();
}

void OpenAsDialog::on_isTilesetAutoRadioButton_toggled(bool checked)
{
    this->updateFields();
}

void OpenAsDialog::on_clsFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Special-CEL file"), tr("CEL Files (*.cel *.CEL)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->clsFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_tilFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select TIL file"), tr("TIL Files (*.til *.TIL)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->tilFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_minFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select MIN file"), tr("MIN Files (*.min *.MIN)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->minFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_slaFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select SLA file"), tr("SOL Files (*.sla *.SLA)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->slaFileEdit->setText(openFilePath);
}

void OpenAsDialog::on_tlaFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select TLA file"), tr("TLA Files (*.tla *.TLA)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->tlaFileEdit->setText(openFilePath);
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

    params.gfxType = (OPEN_GFX_TYPE)this->ui->celTypeButtonGroup->checkedId();
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
    params.clsFilePath = this->ui->clsFileEdit->text();
    params.tilFilePath = this->ui->tilFileEdit->text();
    params.minFilePath = this->ui->minFileEdit->text();
    params.slaFilePath = this->ui->slaFileEdit->text();
    params.tlaFilePath = this->ui->tlaFileEdit->text();
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
