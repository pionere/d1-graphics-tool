#include "settingsdialog.h"

#include <QColorDialog>

#include "config.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog())
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::initialize()
{
    this->on_undefinedPaletteColorLineEdit_escPressed();
    this->on_paletteSelectionBorderColorLineEdit_escPressed();
}

void SettingsDialog::on_undefinedPaletteColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor();
    if (color.isValid()) {
        this->ui->undefinedPaletteColorLineEdit->setText(color.name());
    }
}

void SettingsDialog::on_undefinedPaletteColorLineEdit_returnPressed()
{
    QColor color = QColor(this->ui->undefinedPaletteColorLineEdit->text());
    if (!color.isValid()) {
        this->on_undefinedPaletteColorLineEdit_escPressed();
    }
}

void SettingsDialog::on_undefinedPaletteColorLineEdit_escPressed()
{
    this->ui->undefinedPaletteColorLineEdit->setText(Config::getPaletteUndefinedColor());
}

void SettingsDialog::on_paletteSelectionBorderColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor();
    if (color.isValid()) {
        this->ui->paletteSelectionBorderColorLineEdit->setText(color.name());
    }
}

void SettingsDialog::on_paletteSelectionBorderColorLineEdit_returnPressed()
{
    QColor color = QColor(this->ui->paletteSelectionBorderColorLineEdit->text());
    if (!color.isValid()) {
        this->on_paletteSelectionBorderColorLineEdit_escPressed();
    }
}

void SettingsDialog::on_paletteSelectionBorderColorLineEdit_escPressed()
{
    this->ui->paletteSelectionBorderColorLineEdit->setText(Config::getPaletteSelectionBorderColor());
}

void SettingsDialog::on_settingsOkButton_clicked()
{
    // PaletteUndefinedColor
    QColor palUndefinedColor = QColor(this->ui->undefinedPaletteColorLineEdit->text());
    Config::setPaletteUndefinedColor(palUndefinedColor.name());

    // PaletteSelectionBorderColor
    QColor palSelectionBorderColor = QColor(this->ui->paletteSelectionBorderColorLineEdit->text());
    Config::setPaletteSelectionBorderColor(palSelectionBorderColor.name());

    Config::storeConfiguration();

    emit this->configurationSaved();

    this->close();
}

void SettingsDialog::on_settingsCancelButton_clicked()
{
    this->close();
}
