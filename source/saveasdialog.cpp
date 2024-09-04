#include "saveasdialog.h"

#include <QMessageBox>

#include "ui_saveasdialog.h"

#include "d1gfx.h"
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

void SaveAsDialog::initialize(D1Gfx *g)
{
    this->gfx = g;

    // initialize the main file-path
    QString filePath = this->gfx->getFilePath();
    this->ui->outputCelFileEdit->setText(filePath);
    // reset fields
    this->ui->celClippedAutoRadioButton->setChecked(true);
    this->ui->celGroupEdit->setText("0");
}

void SaveAsDialog::on_outputCelFileBrowseButton_clicked()
{
    QString filePath = this->gfx->getFilePath();
    const QString filter = tr("HS/HSV Files (*.hs *.HS *.hsv *.HSV)");
    const QString title = tr("Save Hero as...");

    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, title, filter);

    if (saveFilePath.isEmpty()) {
        return;
    }

    this->ui->outputCelFileEdit->setText(saveFilePath);
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
