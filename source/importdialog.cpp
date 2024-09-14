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

    QObject::connect(this->ui->fontSizeEdit, SIGNAL(cancel_signal()), this, SLOT(on_fontSizeEdit_escPressed()));
    QObject::connect(this->ui->fontRangeFromEdit, SIGNAL(cancel_signal()), this, SLOT(on_fontRangeFromEdit_escPressed()));
    QObject::connect(this->ui->fontRangeToEdit, SIGNAL(cancel_signal()), this, SLOT(on_fontRangeToEdit_escPressed()));
    QObject::connect(this->ui->fontColorEdit, SIGNAL(cancel_signal()), this, SLOT(on_fontColorEdit_escPressed()));
}

ImportDialog::~ImportDialog()
{
    delete ui;
}

void ImportDialog::initialize(bool dm, const PaletteWidget *palWidget)
{
    this->dunMode = dm;
    this->color = palWidget->getCurrentSelection().first;

    this->ui->fileTypeDUNRadioButton->setVisible(dm);
    if (!dm && this->ui->fileTypeDUNRadioButton->isChecked()) {
        this->ui->fileTypeAutoRadioButton->setChecked(true);
    }

    this->updateFields();
}

void ImportDialog::updateFields()
{
    QString filePath = this->ui->inputFileEdit->text();
    bool hasInputFile = !filePath.isEmpty();
    bool isFont = this->ui->fileTypeFontRadioButton->isChecked() || (this->ui->fileTypeAutoRadioButton->isChecked() && filePath.toLower().endsWith("tf")); // ttf or otf

    this->ui->fontSettingsGroupBox->setEnabled(hasInputFile && isFont);

    if (hasInputFile && isFont) {
        this->ui->fontColorEdit->setText(QString::number(this->font_color));
        this->ui->fontSizeEdit->setText(QString::number(this->font_size));
        this->ui->fontRangeFromEdit->setText(QString::number(this->font_rangeFrom));
        this->ui->fontRangeToEdit->setText(QString::number(this->font_rangeTo));
    } else {
        this->ui->fontColorEdit->setText("");
        this->ui->fontSizeEdit->setText("");
        this->ui->fontRangeFromEdit->setText("");
        this->ui->fontRangeToEdit->setText("");
    }
}

void ImportDialog::on_inputFileBrowseButton_clicked()
{
    QString title;
    QString filter;
    if (this->dunMode) {
        title = tr("Load Dungeon");
        filter = tr("DUN Files (*.dun *.DUN *.rdun *.RDUN)");
    } else {
        title = tr("Load Graphics");
        filter = tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2);;TTF/OTF Files (*.ttf *.TTF *.otf *.OTF);;");
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

void ImportDialog::on_fileTypeFontRadioButton_toggled(bool checked)
{
    this->updateFields();
}

void ImportDialog::on_fileTypeAutoRadioButton_toggled(bool checked)
{
    this->updateFields();
}

void ImportDialog::on_fontSizeEdit_returnPressed()
{
    this->font_size = this->ui->fontSizeEdit->text().toUShort();

    this->on_fontSizeEdit_escPressed();
}

void ImportDialog::on_fontSizeEdit_escPressed()
{
    // update fontSizeEdit
    this->updateFields();
    this->ui->fontSizeEdit->clearFocus();
}

void ImportDialog::on_fontRangeFromEdit_returnPressed()
{
    this->font_rangeFrom = this->ui->fontRangeFromEdit->text().toUShort();
    if (this->font_rangeFrom == USHRT_MAX)
        this->font_rangeFrom = USHRT_MAX - 1;
    if (this->font_rangeTo <= this->font_rangeFrom) {
        this->font_rangeTo = this->font_rangeFrom + 1;
    }

    this->on_fontRangeFromEdit_escPressed();
}

void ImportDialog::on_fontRangeFromEdit_escPressed()
{
    // update fontRangeFromEdit
    this->updateFields();
    this->ui->fontRangeFromEdit->clearFocus();
}

void ImportDialog::on_fontRangeToEdit_returnPressed()
{
    this->font_rangeTo = this->ui->fontRangeToEdit->text().toUShort();
    if (this->font_rangeTo == 0)
        this->font_rangeTo = 1;
    if (this->font_rangeTo <= this->font_rangeFrom) {
        this->font_rangeFrom = this->font_rangeTo - 1;
    }

    this->on_fontRangeToEdit_escPressed();
}

void ImportDialog::on_fontRangeToEdit_escPressed()
{
    // update fontRangeToEdit
    this->updateFields();
    this->ui->fontRangeToEdit->clearFocus();
}

void ImportDialog::on_fontColorEdit_returnPressed()
{
    this->font_color = this->ui->fontColorEdit->text().toUShort();
    if (this->font_color >= D1PAL_COLORS)
        this->font_color = D1PAL_COLORS - 1;

    this->on_fontColorEdit_escPressed();
}

void ImportDialog::on_fontColorEdit_escPressed()
{
    // update fontColorEdit
    this->updateFields();
    this->ui->fontColorEdit->clearFocus();
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
    } else if (this->ui->fileTypeFontRadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::FONT;
    } else {
        params.fileType = IMPORT_FILE_TYPE::AUTODETECT;
    }

    params.fontSize = this->font_size;
    params.fontRangeFrom = font_rangeFrom;
    params.fontRangeTo = this->font_rangeTo;
    params.fontColor = this->font_color;

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
