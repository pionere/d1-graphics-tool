#include "saveasdialog.h"

#include <QMessageBox>

#include "ui_saveasdialog.h"

#include "d1hro.h"
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

void SaveAsDialog::initialize(D1Hero *h)
{
    this->hero = h;

    // initialize the main file-path
    QString filePath = this->hero->getFilePath();
    this->ui->outputFileEdit->setText(filePath);
    // reset fields
    // this->ui->hellfireHeroAutoRadioButton->setChecked(true);
}

void SaveAsDialog::on_outputFileBrowseButton_clicked()
{
    const QString filter = tr("HRO Files (*.hro *.HRO)");
    const QString title = tr("Save Hero as...");

    QString saveFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::SAVE_NO_CONF, title, filter);

    if (saveFilePath.isEmpty()) {
        return;
    }

    this->ui->outputFileEdit->setText(saveFilePath);
}

void SaveAsDialog::on_saveButton_clicked()
{
    SaveAsParam params;
    // main cel file
    params.filePath = this->ui->outputFileEdit->text();
    // heroSettingsGroupBox: hellfireHero
    // if (this->ui->hellfireHeroYesRadioButton->isChecked()) {
    //     params.heroType = SAVE_HERO_TYPE::HELLFIRE_HERO;
    // } else if (this->ui->hellfireHeroNoRadioButton->isChecked()) {
    //     params.heroType = SAVE_HERO_TYPE::DIABLO_HERO;
    // } else {
        params.heroType = SAVE_HERO_TYPE::AUTODETECT;
    // }
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
