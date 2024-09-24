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

    this->updateFields();
}

void OpenAsDialog::updateFields()
{
    QString filePath = this->ui->inputFileEdit->text();
    bool hasInputFile = !filePath.isEmpty();

    this->ui->heroSettingsGroupBox->setEnabled(hasInputFile);
}

void OpenAsDialog::on_inputFileBrowseButton_clicked()
{
    QString openFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Hero"), tr("HRO Files (*.hro *.HRO)"));

    if (openFilePath.isEmpty())
        return;

    this->ui->inputFileEdit->setText(openFilePath);

    this->updateFields();
}

void OpenAsDialog::on_openButton_clicked()
{
    OpenAsParam params;
    params.filePath = this->ui->inputFileEdit->text();
    if (params.filePath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Input file is missing, please choose an input file."));
        return;
    }
    // heroSettingsGroupBox: hellfireHero
    if (this->ui->hellfireHeroYesRadioButton->isChecked()) {
        params.heroType = OPEN_HERO_TYPE::HELLFIRE_HERO;
    } else if (this->ui->hellfireHeroNoRadioButton->isChecked()) {
        params.heroType = OPEN_HERO_TYPE::DIABLO_HERO;
    } else {
        params.heroType = OPEN_HERO_TYPE::AUTODETECT;
    }

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
