#include "importdialog.h"

#include <QMessageBox>

#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_importdialog.h"

ImportDialog::ImportDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ImportDialog())
{
    ui->setupUi(this);
}

ImportDialog::~ImportDialog()
{
    delete ui;
}

void ImportDialog::initialize(bool dm)
{
    this->dunMode = dm;

    this->ui->fileTypeAutoRadioButton->setChecked(true);

    this->updateFields();
}

void ImportDialog::updateFields()
{
}

void ImportDialog::on_inputFileBrowseButton_clicked()
{
    QString title;
    QString filter;
    if (this->dunMode) {
        title = tr("Select Dungeon or Graphics");
        filter = tr("DUN Files (*.dun *.DUN *.rdun *.RDUN);;CEL Files (*.cel *.CEL)");
    } else {
        title = tr("Select Graphics");
        filter = tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2)");
    }

    QString filePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, title, filter);
    if (filePath.isEmpty()) {
        return;
    }

    this->ui->inputFileEdit->setText(filePath);

    this->updateFields();
}

void ImportDialog::on_fileTypeCELRadioButton_toggled(bool checked)
{
    this->updateFields();
}

void ImportDialog::on_fileTypeCL2RadioButton_toggled(bool checked)
{
    this->updateFields();
}

void ImportDialog::on_fileTypeDUNRadioButton_toggled(bool checked)
{
    this->updateFields();
}

void ImportDialog::on_fileTypeAutoRadioButton_toggled(bool checked)
{
    this->updateFields();
}

void ImportDialog::on_importButton_clicked()
{
    ImportParam params;
    params.filePath = this->ui->inputFileEdit->text();
    if (params.filePath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Input file is missing, please choose an input file."));
        return;
    }

    if (this->ui->fileTypeCELRadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::CEL;
    } else if (this->ui->fileTypeCL2RadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::CL2;
    } else if (this->ui->fileTypeDUNRadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::DUNGEON;
    } else {
        params.fileType = IMPORT_FILE_TYPE::AUTODETECT;
    }

    this->close();

    dMainWindow().importFile(params);
}

void ImportDialog::on_importCancelButton_clicked()
{
    this->close();
}

void ImportDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
