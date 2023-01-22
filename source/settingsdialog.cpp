#include "settingsdialog.h"

#include <QColorDialog>
#include <QDir>

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
    // initialize the available languages
    this->ui->languageComboBox->clear();

    QString defaultLocale = Config::getLocale(); // e.g. "de_DE"

    QDir dir(QApplication::applicationDirPath());
    QStringList fileNames = dir.entryList(QStringList("lang_*.qm"));

    for (int i = 0; i < fileNames.size(); ++i) {
        // get locale extracted by filename
        QString localeName;
        localeName = fileNames[i]; // "lang_de_DE.qm"
        localeName.chop(3);        // "lang_de_DE"
        localeName.remove(0, 5);   // "de_DE"

        QLocale locale = QLocale(localeName);

        this->ui->languageComboBox->addItem(locale.nativeLanguageName(), QVariant(localeName));

        // set default language selected
        if (defaultLocale == localeName) {
            this->ui->languageComboBox->setCurrentIndex(i);
        }
    }
    // reset the color values
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
    // Locale
    QString locale = this->ui->languageComboBox->currentData().value<QString>();
    Config::setLocale(locale);

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
