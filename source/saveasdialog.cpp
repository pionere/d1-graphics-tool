#include "saveasdialog.h"

#include <QMessageBox>

#include "ui_saveasdialog.h"

#include "d1amp.h"
#include "d1gfx.h"
#include "d1min.h"
#include "d1sol.h"
#include "d1til.h"
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

void SaveAsDialog::initialize(D1Gfx *g, D1Tileset *tileset, D1Dun *dun)
{
    bool isTilesetGfx = tileset != nullptr;

    this->gfx = g;
    this->isTileset = isTilesetGfx;

    // reset fields
    this->ui->outputCelFileEdit->setText(this->gfx->getFilePath());

    this->ui->celClippedAutoRadioButton->setChecked(true);
    this->ui->celGroupEdit->setText("0");

    this->ui->minUpscaledAutoRadioButton->setChecked(true);

    this->ui->outputMinFileEdit->setText(isTilesetGfx ? tileset->min->getFilePath() : "");
    this->ui->outputTilFileEdit->setText(isTilesetGfx ? tileset->til->getFilePath() : "");
    this->ui->outputSolFileEdit->setText(isTilesetGfx ? tileset->sol->getFilePath() : "");
    this->ui->outputAmpFileEdit->setText(isTilesetGfx ? tileset->amp->getFilePath() : "");
    this->ui->outputTmiFileEdit->setText(isTilesetGfx ? tileset->tmi->getFilePath() : "");

    if (dun == nullptr) {
        this->ui->outputDunFileEdit->setVisible(false);
        this->ui->outputDunFileEdit->setText("");
    } else {
        this->ui->outputDunFileEdit->setText(dun->getFilePath());
    }

    this->ui->celSettingsGroupBox->setEnabled(!isTilesetGfx);
    this->ui->tilSettingsGroupBox->setEnabled(isTilesetGfx);
}

void SaveAsDialog::on_outputCelFileBrowseButton_clicked()
{
    QString filePath = this->gfx->getFilePath();
    const QString filter = this->isTileset ? tr("CEL Files (*.cel *.CEL)") : tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2)");

    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, tr("Save Graphics as..."), filter);

    if (saveFilePath.isEmpty()) {
        return;
    }

    this->ui->outputCelFileEdit->setText(saveFilePath);

    if (this->isTileset) {
        int extPos = saveFilePath.lastIndexOf('.', saveFilePath.length() - 1);
        if (extPos >= 0 && extPos < saveFilePath.length() - 1) {
            bool upperCase = saveFilePath.at(extPos + 1).isUpper();
            saveFilePath.chop(saveFilePath.length() - 1 - extPos);
            this->ui->outputMinFileEdit->setText(saveFilePath + (upperCase ? "MIN" : "min"));
            this->ui->outputTilFileEdit->setText(saveFilePath + (upperCase ? "TIL" : "til"));
            this->ui->outputSolFileEdit->setText(saveFilePath + (upperCase ? "SOL" : "sol"));
            this->ui->outputAmpFileEdit->setText(saveFilePath + (upperCase ? "AMP" : "amp"));
            this->ui->outputTmiFileEdit->setText(saveFilePath + (upperCase ? "TMI" : "tmi"));
        }
    }
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

void SaveAsDialog::on_outputSolFileBrowseButton_clicked()
{
    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, tr("Save SOL as..."), tr("SOL Files (*.sol *.SOL)"));

    if (saveFilePath.isEmpty())
        return;

    this->ui->outputSolFileEdit->setText(saveFilePath);
}

void SaveAsDialog::on_outputAmpFileBrowseButton_clicked()
{
    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, tr("Save AMP as..."), tr("AMP Files (*.amp *.AMP)"));

    if (saveFilePath.isEmpty())
        return;

    this->ui->outputAmpFileEdit->setText(saveFilePath);
}

void SaveAsDialog::on_outputTmiFileBrowseButton_clicked()
{
    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, tr("Save TMI as..."), tr("TMI Files (*.tmi *.TMI)"));

    if (saveFilePath.isEmpty())
        return;

    this->ui->outputTmiFileEdit->setText(saveFilePath);
}

void SaveAsDialog::on_outputDunFileBrowseButton_clicked()
{
    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, tr("Save DUN as..."), tr("DUN Files (*.dun *.DUN *.rdun *.RDUN)"));

    if (saveFilePath.isEmpty())
        return;

    this->ui->outputDunFileEdit->setText(saveFilePath);
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
    // tilSettingsGroupBox: upscaled, min, til, sol and amp files
    if (this->ui->minUpscaledYesRadioButton->isChecked()) {
        params.upscaled = SAVE_UPSCALED_TYPE::TRUE;
    } else if (this->ui->minUpscaledNoRadioButton->isChecked()) {
        params.upscaled = SAVE_UPSCALED_TYPE::FALSE;
    } else {
        params.upscaled = SAVE_UPSCALED_TYPE::AUTODETECT;
    }
    params.minFilePath = this->ui->outputMinFileEdit->text();
    params.tilFilePath = this->ui->outputTilFileEdit->text();
    params.solFilePath = this->ui->outputSolFileEdit->text();
    params.ampFilePath = this->ui->outputAmpFileEdit->text();
    params.tmiFilePath = this->ui->outputTmiFileEdit->text();
    params.dunFilePath = this->ui->outputDunFileEdit->text();
    params.autoOverwrite = this->ui->autoOverwriteCheckBox->isChecked();

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
