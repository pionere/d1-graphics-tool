#include "saveasdialog.h"

#include <QMessageBox>

#include "ui_saveasdialog.h"

#include "d1cpp.h"
#include "d1gfx.h"
#include "d1min.h"
#include "d1sla.h"
#include "d1til.h"
#include "d1tla.h"
#include "mainwindow.h"

SaveAsDialog::SaveAsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SaveAsDialog)
{
    ui->setupUi(this);
}

SaveAsDialog::~SaveAsDialog()
{
    delete ui;
}

void SaveAsDialog::initialize(D1Gfx *g, D1Tileset *tileset, D1Gfxset *gfxset, D1Dun *dun, D1Tableset *tableset, D1Cpp *cpp)
{
    bool isTilesetGfx = tileset != nullptr;
    bool isDun = dun != nullptr;
    bool isTableset = tableset != nullptr;
    bool isGfxset = gfxset != nullptr;
    bool isCpp = cpp != nullptr;
    bool isSmk = g->getType() == D1CEL_TYPE::SMK;

    this->gfx = isGfxset ? gfxset->getGfx(0) : g;
    this->isTileset = isTilesetGfx;
    this->isDun = isDun;
    this->isGfxset = isGfxset;
    this->isTableset = isTableset;
    this->isCpp = isCpp;
    this->isSmk = isSmk;

    // initialize the main file-path
    QString filePath = isTableset ? tableset->distTbl->getFilePath() : (isCpp ? cpp->getFilePath() : this->gfx->getFilePath());
    this->ui->outputCelFileEdit->setText(filePath);
    // reset fields
    this->ui->celClippedAutoRadioButton->setChecked(true);
    this->ui->celGroupEdit->setText("0");

    this->ui->celPatchedAutoRadioButton->setChecked(true);
    this->ui->minUpscaledAutoRadioButton->setChecked(true);

    this->ui->outputClsFileEdit->setText(isTilesetGfx ? tileset->cls->getFilePath() : "");
    this->ui->outputMinFileEdit->setText(isTilesetGfx ? tileset->min->getFilePath() : "");
    this->ui->outputTilFileEdit->setText(isTilesetGfx ? tileset->til->getFilePath() : "");
    this->ui->outputSlaFileEdit->setText(isTilesetGfx ? tileset->sla->getFilePath() : "");
    this->ui->outputTlaFileEdit->setText(isTilesetGfx ? tileset->tla->getFilePath() : "");
    this->ui->tblFileEdit->setText(isTableset ? tableset->darkTbl->getFilePath() : "");

    if (!isDun) {
        this->ui->outputDunFileLabel->setVisible(false);
        this->ui->outputDunFileEdit->setVisible(false);
        this->ui->outputDunFileEdit->setText("");
        this->ui->outputDunFileBrowseButton->setVisible(false);
    } else {
        this->ui->outputDunFileEdit->setText(dun->getFilePath());
        switch (dun->getNumLayers()) {
        case 0:
            this->ui->dunLayerTilesRadioButton->setChecked(true);
            break;
        case 1:
            this->ui->dunLayerProtectionsRadioButton->setChecked(true);
            break;
        case 2:
            this->ui->dunLayerMonstersRadioButton->setChecked(true);
            break;
        case 3:
            this->ui->dunLayerObjectsRadioButton->setChecked(true);
            break;
        }
    }

    this->ui->celSettingsGroupBox->setEnabled(!isTilesetGfx && !isTableset && !isGfxset && !isCpp && !isSmk);
    this->ui->tilSettingsGroupBox->setEnabled(isTilesetGfx);
    this->ui->tblSettingsGroupBox->setEnabled(isTableset);
}

void SaveAsDialog::on_outputCelFileBrowseButton_clicked()
{
    const QString filter = this->isTileset ? tr("CEL Files (*.cel *.CEL)") : (this->isTableset ? tr("TBL Files (*.tbl *.TBL)") : (this->isGfxset ? tr("CL2 Files (*.cl2 *.CL2)") : (this->isCpp ? tr("CPP Files (*.cpp *.CPP *.c *.C)") : (this->isSmk ? tr("SMK Files (*.smk *.SMK)") : tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2)")))));
    const QString title = this->isTableset ? tr("Save Dist TBL as...") : (this->isCpp ? tr("Save Source as...") : tr("Save Graphics as..."));

    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, title, filter);

    if (saveFilePath.isEmpty()) {
        return;
    }

    this->ui->outputCelFileEdit->setText(saveFilePath);

    if (this->isTileset) {
        int extPos = saveFilePath.lastIndexOf('.', saveFilePath.length() - 1);
        if (extPos >= 0 && extPos < saveFilePath.length() - 1) {
            bool upperCase = saveFilePath.at(extPos + 1).isUpper();
            saveFilePath.chop(saveFilePath.length() - extPos);
            this->ui->outputClsFileEdit->setText(saveFilePath + (upperCase ? "S.CEL" : "s.cel"));
            this->ui->outputMinFileEdit->setText(saveFilePath + (upperCase ? ".MIN" : ".min"));
            this->ui->outputTilFileEdit->setText(saveFilePath + (upperCase ? ".TIL" : ".til"));
            this->ui->outputSlaFileEdit->setText(saveFilePath + (upperCase ? ".SLA" : ".sla"));
            this->ui->outputTlaFileEdit->setText(saveFilePath + (upperCase ? ".TLA" : ".tla"));

            // if (this->isDun) {
                QString dunFilePath = this->ui->outputDunFileEdit->text();

                if (!dunFilePath.isEmpty()) {
                    QFileInfo dfi(dunFilePath);
                    QFileInfo sfi(saveFilePath);

                    dfi.setFile(sfi.absoluteDir(), dfi.fileName());
                    this->ui->outputDunFileEdit->setText(dfi.absoluteFilePath());
                }
            // }
        }
    }
}

void SaveAsDialog::on_outputClsFileBrowseButton_clicked()
{
    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, tr("Save Special-CEL as..."), tr("CEL Files (*.cel *.CEL)"));

    if (saveFilePath.isEmpty())
        return;

    this->ui->outputClsFileEdit->setText(saveFilePath);
}

void SaveAsDialog::on_outputMinFileBrowseButton_clicked()
{
    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, tr("Save MIN as..."), tr("MIN Files (*.min *.MIN)"));

    if (saveFilePath.isEmpty())
        return;

    this->ui->outputMinFileEdit->setText(saveFilePath);
}

void SaveAsDialog::on_outputTilFileBrowseButton_clicked()
{
    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, tr("Save TIL as..."), tr("TIL Files (*.til *.TIL)"));

    if (saveFilePath.isEmpty())
        return;

    this->ui->outputTilFileEdit->setText(saveFilePath);
}

void SaveAsDialog::on_outputSlaFileBrowseButton_clicked()
{
    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, tr("Save SLA as..."), tr("SLA Files (*.sla *.SLA)"));

    if (saveFilePath.isEmpty())
        return;

    this->ui->outputSlaFileEdit->setText(saveFilePath);
}

void SaveAsDialog::on_outputTlaFileBrowseButton_clicked()
{
    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, tr("Save TLA as..."), tr("TLA Files (*.tla *.TLA)"));

    if (saveFilePath.isEmpty())
        return;

    this->ui->outputTlaFileEdit->setText(saveFilePath);
}

void SaveAsDialog::on_outputDunFileBrowseButton_clicked()
{
    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, tr("Save DUN as..."), tr("DUN Files (*.dun *.DUN *.rdun *.RDUN)"));

    if (saveFilePath.isEmpty())
        return;

    this->ui->outputDunFileEdit->setText(saveFilePath);
}

void SaveAsDialog::on_tblFileBrowseButton_clicked()
{
    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, tr("Save Dark TBL as..."), tr("TBL Files (*.tbl *.TBL)"));

    if (saveFilePath.isEmpty())
        return;

    this->ui->tblFileEdit->setText(saveFilePath);
}

void SaveAsDialog::on_saveButton_clicked()
{
    SaveAsParam params;
    // main cel file
    params.celFilePath = this->ui->outputCelFileEdit->text();
    // celSettingsGroupBox: groupNum, clipped
    params.groupNum = this->ui->celGroupEdit->nonNegInt();
    if (this->ui->celClippedYesRadioButton->isChecked()) {
        params.clipped = SAVE_CLIPPED_TYPE::TRUE;
    } else if (this->ui->celClippedNoRadioButton->isChecked()) {
        params.clipped = SAVE_CLIPPED_TYPE::FALSE;
    } else {
        params.clipped = SAVE_CLIPPED_TYPE::AUTODETECT;
    }
    // tilSettingsGroupBox: patched, upscaled, min, til, sla and tla files
    if (this->ui->celPatchedYesRadioButton->isChecked()) {
        params.patched = SAVE_PATCHED_TYPE::TRUE;
    } else if (this->ui->celPatcheddNoRadioButton->isChecked()) {
        params.patched = SAVE_PATCHED_TYPE::FALSE;
    } else {
        params.patched = SAVE_PATCHED_TYPE::AUTODETECT;
    }
    if (this->ui->minUpscaledYesRadioButton->isChecked()) {
        params.upscaled = SAVE_UPSCALED_TYPE::TRUE;
    } else if (this->ui->minUpscaledNoRadioButton->isChecked()) {
        params.upscaled = SAVE_UPSCALED_TYPE::FALSE;
    } else {
        params.upscaled = SAVE_UPSCALED_TYPE::AUTODETECT;
    }
    params.clsFilePath = this->ui->outputClsFileEdit->text();
    params.minFilePath = this->ui->outputMinFileEdit->text();
    params.tilFilePath = this->ui->outputTilFileEdit->text();
    params.slaFilePath = this->ui->outputSlaFileEdit->text();
    params.tlaFilePath = this->ui->outputTlaFileEdit->text();
    params.dunFilePath = this->ui->outputDunFileEdit->text();
    if (this->ui->dunLayerTilesRadioButton->isChecked()) {
        params.dunLayerNum = 0;
    } else if (this->ui->dunLayerProtectionsRadioButton->isChecked()) {
        params.dunLayerNum = 1;
    } else if (this->ui->dunLayerMonstersRadioButton->isChecked()) {
        params.dunLayerNum = 2;
    } else if (this->ui->dunLayerObjectsRadioButton->isChecked()) {
        params.dunLayerNum = 3;
    }
    params.autoOverwrite = this->ui->autoOverwriteCheckBox->isChecked();
    params.tblFilePath = this->ui->tblFileEdit->text();

    int errors = 0;
    QString msg;
    if (this->celFilePath.isEmpty()) {
        msg = tr("File");
        errors++;
    }
    if (this->isTileset) {
        if (this->minFilePath.isEmpty()) {
            if (errors)
                msg += ", ";
            errors++;
            msg += tr("Min-file");
        }
        if (this->tilFilePath.isEmpty()) {
            if (errors)
                msg += ", ";
            errors++;
            msg += tr("Til-file");
        }
        if (this->slaFilePath.isEmpty()) {
            if (errors)
                msg += ", ";
            errors++;
            msg += tr("Sla-file");
        }
        if (this->tlaFilePath.isEmpty()) {
            if (errors)
                msg += ", ";
            errors++;
            msg += tr("Tla-file");
        }
        if (this->isDun) {
            if (this->dunFilePath.isEmpty()) {
                if (errors)
                    msg += ", ";
                errors++;
                msg += tr("Dun-file");
            }
        }
    }
    if (errors != 0) {
        QMessageBox::critical(this,  tr("Error"), tr("%1 path is missing.", "", errors).arg(msg));
        return;
    }

    this->close();

    dMainWindow().saveFile(params);
}

void SaveAsDialog::on_saveCancelButton_clicked()
{
    this->close();
}

void SaveAsDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
