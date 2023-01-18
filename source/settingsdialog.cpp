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
    this->on_defaultPaletteColorLineEdit_escPressed();
    this->on_paletteSelectionBorderColorLineEdit_escPressed();
}

void SettingsDialog::on_defaultPaletteColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor();
    if (color.isValid()) {
        this->ui->defaultPaletteColorLineEdit->setText(color.name());
    }
}

void SettingsDialog::on_defaultPaletteColorLineEdit_returnPressed()
{
    QColor color = QColor(this->ui->defaultPaletteColorLineEdit->text());
    if (!color.isValid()) {
        this->on_defaultPaletteColorLineEdit_escPressed();
    }
}

void SettingsDialog::on_defaultPaletteColorLineEdit_escPressed()
{
    QColor palDefaultColor = QColor(Config::value("PaletteDefaultColor").toString());
    this->ui->defaultPaletteColorLineEdit->setText(palDefaultColor.name());
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
    QColor palSelectionBorderColor = QColor(Config::value("PaletteSelectionBorderColor").toString());
    this->ui->paletteSelectionBorderColorLineEdit->setText(palSelectionBorderColor.name());
}

void SettingsDialog::on_settingsOkButton_clicked()
{
    // PaletteDefaultColor
    QColor palDefaultColor = QColor(ui->defaultPaletteColorLineEdit->text());
    Config::insert("PaletteDefaultColor", palDefaultColor.name());

    // PaletteSelectionBorderColor
    QColor palSelectionBorderColor = QColor(ui->paletteSelectionBorderColorLineEdit->text());
    Config::insert("PaletteSelectionBorderColor", palSelectionBorderColor.name());

    Config::storeConfiguration();

    emit this->configurationSaved();

    this->close();
}

void SettingsDialog::on_settingsCancelButton_clicked()
{
    this->close();
}
