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
    this->font_color = palWidget->getCurrentSelection().first;
    if (this->font_color < 0)
        this->font_color = 0;

    this->ui->fileTypeMINRadioButton->setVisible(dm);
    this->ui->fileTypeTILRadioButton->setVisible(dm);
    this->ui->fileTypeSLARadioButton->setVisible(dm);
    this->ui->fileTypeTLARadioButton->setVisible(dm);
    this->ui->fileTypeDUNRadioButton->setVisible(dm);
    this->ui->fileTypeSCELRadioButton->setVisible(dm);
    this->ui->fileTypeCL2RadioButton->setVisible(!dm);
    this->ui->fileTypeFontRadioButton->setVisible(!dm);
    // if (!dm && this->ui->fileTypeDUNRadioButton->isChecked()) {
        this->ui->fileTypeAutoRadioButton->setChecked(true);
    // }

    this->updateFields();
}

void ImportDialog::updateFields()
{
    QString filePath = this->ui->inputFileEdit->text();
    bool hasInputFile = !filePath.isEmpty();
    bool isFont = this->ui->fileTypeFontRadioButton->isChecked() || (this->ui->fileTypeAutoRadioButton->isChecked() && filePath.toLower().endsWith("tf")); // .ttf or .otf

    this->ui->fontSettingsGroupBox->setEnabled(hasInputFile && isFont);

    if (hasInputFile && isFont) {
        this->ui->fontColorEdit->setText(QString::number(this->font_color));
        this->ui->fontSizeEdit->setText(QString::number(this->font_size));
        this->ui->fontRangeFromEdit->setText(QString::number(this->font_rangeFrom, this->font_rangeHex ? 16 : 10));
        this->ui->fontRangeToEdit->setText(QString::number(this->font_rangeTo, this->font_rangeHex ? 16 : 10));
        this->ui->fontRangeHexCheckBox->setChecked(this->font_rangeHex);
    } else {
        this->ui->fontColorEdit->setText("");
        this->ui->fontSizeEdit->setText("");
        this->ui->fontRangeFromEdit->setText("");
        this->ui->fontRangeToEdit->setText("");
        this->ui->fontRangeHexCheckBox->setChecked(false);
    }
}

void ImportDialog::on_inputFileBrowseButton_clicked()
{
    QString title;
    QString filter;
    if (this->dunMode) {
        title = tr("Select Dungeon or Graphics");
        filter = tr("DUN Files (*.dun *.DUN *.rdun *.RDUN);;CEL Files (*.cel *.CEL);;MIN Files (*.min *.MIN);;TIL Files (*.til *.TIL);;SLA Files (*.sla *.SLA);;TLA Files (*.tla *.TLA)");
    } else {
        title = tr("Select Graphics");
        filter = tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2);;TTF/OTF Files (*.ttf *.TTF *.otf *.OTF)");
    }

    QString filePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, title, filter);
    if (filePath.isEmpty()) {
        return;
    }

    this->ui->inputFileEdit->setText(filePath);

    this->updateFields();
}

void ImportDialog::on_fileTypeMINRadioButton_toggled(bool checked)
{
    this->updateFields();
}

void ImportDialog::on_fileTypeTILRadioButton_toggled(bool checked)
{
    this->updateFields();
}

void ImportDialog::on_fileTypeSLARadioButton_toggled(bool checked)
{
    this->updateFields();
}

void ImportDialog::on_fileTypeTLARadioButton_toggled(bool checked)
{
    this->updateFields();
}

void ImportDialog::on_fileTypeDUNRadioButton_toggled(bool checked)
{
    this->updateFields();
}

void ImportDialog::on_fileTypeSCELRadioButton_toggled(bool checked)
{
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
    bool ok;
    this->font_rangeFrom = this->ui->fontRangeFromEdit->text().toUShort(&ok, this->font_rangeHex ? 16 : 10);
    if (this->font_rangeTo < this->font_rangeFrom) {
        this->font_rangeTo = this->font_rangeFrom;
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
    bool ok;
    this->font_rangeTo = this->ui->fontRangeToEdit->text().toUShort(&ok, this->font_rangeHex ? 16 : 10);
    if (this->font_rangeTo < this->font_rangeFrom) {
        this->font_rangeFrom = this->font_rangeTo;
    }

    this->on_fontRangeToEdit_escPressed();
}

void ImportDialog::on_fontRangeToEdit_escPressed()
{
    // update fontRangeToEdit
    this->updateFields();
    this->ui->fontRangeToEdit->clearFocus();
}

void ImportDialog::on_fontRangeHexCheckBox_clicked()
{
    this->font_rangeHex = this->ui->fontRangeHexCheckBox->isChecked();

    this->updateFields();
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

    if (this->ui->fileTypeMINRadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::MIN;
    } else if (this->ui->fileTypeTILRadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::TIL;
    } else if (this->ui->fileTypeSLARadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::SLA;
    } else if (this->ui->fileTypeTLARadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::TLA;
    } else if (this->ui->fileTypeDUNRadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::DUNGEON;
    } else if (this->ui->fileTypeSCELRadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::SCEL;
    } else if (this->ui->fileTypeCELRadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::CEL;
    } else if (this->ui->fileTypeCL2RadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::CL2;
    } else if (this->ui->fileTypeFontRadioButton->isChecked()) {
        params.fileType = IMPORT_FILE_TYPE::FONT;
    } else {
        params.fileType = IMPORT_FILE_TYPE::AUTODETECT;
    }

    if (params.filePath.isEmpty() && params.fileType != IMPORT_FILE_TYPE::DUNGEON) {
        QMessageBox::warning(this, tr("Warning"), tr("Input file is missing, please choose an input file."));
        return;
    }

    params.fontSize = this->font_size;
    params.fontRangeFrom = font_rangeFrom;
    params.fontRangeTo = this->font_rangeTo;
    params.fontColor = this->font_color;

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
