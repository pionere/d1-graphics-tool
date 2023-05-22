#include "settingsdialog.h"

#include <QColorDialog>
#include <QDir>
#include <QMessageBox>

#include "config.h"
#include "mainwindow.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog())
{
    ui->setupUi(this);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->graphicsBackgroundColorLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_graphicsBackgroundColorLineEdit_escPressed()));
    QObject::connect(this->ui->graphicsTransparentColorLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_graphicsTransparentColorLineEdit_escPressed()));
    QObject::connect(this->ui->undefinedPaletteColorLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_undefinedPaletteColorLineEdit_escPressed()));
    QObject::connect(this->ui->paletteSelectionBorderColorLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_paletteSelectionBorderColorLineEdit_escPressed()));
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

    // QDir dir(QApplication::applicationDirPath());
    QDir dir(":/");
    QStringList fileNames = dir.entryList(QStringList("lang_*.qm"));

    for (int i = 0; i < fileNames.size(); ++i) {
        // get locale extracted by filename
        QString localeName;
        localeName = fileNames[i]; // "lang_de_DE.qm"
        localeName.chop(3);        // "lang_de_DE"
        localeName.remove(0, 5);   // "de_DE"

        QLocale locale = QLocale(localeName);

        // this->ui->languageComboBox->addItem(locale.nativeLanguageName().toLower(), QVariant(localeName));
        this->ui->languageComboBox->addItem(locale.languageToString(locale.language()), QVariant(localeName));

        // set default language selected
        if (defaultLocale == localeName) {
            this->ui->languageComboBox->setCurrentIndex(i);
        }
    }
    // reset the color values
    this->on_graphicsBackgroundColorLineEdit_escPressed();
    this->on_graphicsTransparentColorLineEdit_escPressed();
    this->on_undefinedPaletteColorLineEdit_escPressed();
    this->on_paletteSelectionBorderColorLineEdit_escPressed();

    this->updateIcons();
}

void SettingsDialog::setIconColor(QLabel *imageLabel, const QString &colorText)
{
    QSize imageSize = imageLabel->size();
    QImage image = QImage(imageSize, QImage::Format_ARGB32);

    const QColor color = QColor(colorText);
    QRgb *destBits = reinterpret_cast<QRgb *>(image.bits());
    for (int y = 0; y < imageSize.height(); y++) {
        for (int x = 0; x < imageSize.width(); x++, destBits++) {
            // image.setPixelColor(x, y, color);
            *destBits = color.rgba();
        }
    }
    QPixmap pixmap = QPixmap::fromImage(std::move(image));
    imageLabel->setPixmap(pixmap);
}

void SettingsDialog::updateIcons()
{
    this->setIconColor(this->ui->graphicsBackgroundColorImageLabel, this->ui->graphicsBackgroundColorLineEdit->text());
    this->setIconColor(this->ui->graphicsTransparentColorImageLabel, this->ui->graphicsTransparentColorLineEdit->text());
    this->setIconColor(this->ui->paletteSelectionBorderColorImageLabel, this->ui->paletteSelectionBorderColorLineEdit->text());
    this->setIconColor(this->ui->undefinedPaletteColorImageLabel, this->ui->undefinedPaletteColorLineEdit->text());
}

void SettingsDialog::on_graphicsBackgroundColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor();
    if (color.isValid()) {
        this->ui->graphicsBackgroundColorLineEdit->setText(color.name());
    }
}

void SettingsDialog::on_graphicsBackgroundColorLineEdit_returnPressed()
{
    QColor color = QColor(this->ui->graphicsBackgroundColorLineEdit->text());
    if (color.isValid()) {
        this->updateIcons();
    } else {
        this->on_graphicsBackgroundColorLineEdit_escPressed();
    }
}

void SettingsDialog::on_graphicsBackgroundColorLineEdit_escPressed()
{
    this->ui->graphicsBackgroundColorLineEdit->setText(Config::getGraphicsBackgroundColor());
}

void SettingsDialog::on_graphicsTransparentColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor();
    if (color.isValid()) {
        this->ui->graphicsTransparentColorLineEdit->setText(color.name());
        this->updateIcons();
    }
}

void SettingsDialog::on_graphicsTransparentColorLineEdit_returnPressed()
{
    QColor color = QColor(this->ui->graphicsTransparentColorLineEdit->text());
    if (color.isValid()) {
        this->updateIcons();
    } else {
        this->on_graphicsTransparentColorLineEdit_escPressed();
    }
}

void SettingsDialog::on_graphicsTransparentColorLineEdit_escPressed()
{
    this->ui->graphicsTransparentColorLineEdit->setText(Config::getGraphicsTransparentColor());
}

void SettingsDialog::on_undefinedPaletteColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor();
    if (color.isValid()) {
        this->ui->undefinedPaletteColorLineEdit->setText(color.name());
        this->updateIcons();
    }
}

void SettingsDialog::on_undefinedPaletteColorLineEdit_returnPressed()
{
    QColor color = QColor(this->ui->undefinedPaletteColorLineEdit->text());
    if (color.isValid()) {
        this->updateIcons();
    } else {
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
        this->updateIcons();
    }
}

void SettingsDialog::on_paletteSelectionBorderColorLineEdit_returnPressed()
{
    QColor color = QColor(this->ui->paletteSelectionBorderColorLineEdit->text());
    if (color.isValid()) {
        this->updateIcons();
    } else {
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

    // GraphicsBackgroundColor
    QColor gfxBackgroundColor = QColor(this->ui->graphicsBackgroundColorLineEdit->text());
    Config::setGraphicsBackgroundColor(gfxBackgroundColor.name());

    // GraphicsTransparentColor
    QColor gfxTransparentColor = QColor(this->ui->graphicsTransparentColorLineEdit->text());
    Config::setGraphicsTransparentColor(gfxTransparentColor.name());

    // PaletteUndefinedColor
    QColor palUndefinedColor = QColor(this->ui->undefinedPaletteColorLineEdit->text());
    Config::setPaletteUndefinedColor(palUndefinedColor.name());

    // PaletteSelectionBorderColor
    QColor palSelectionBorderColor = QColor(this->ui->paletteSelectionBorderColorLineEdit->text());
    Config::setPaletteSelectionBorderColor(palSelectionBorderColor.name());

    if (!Config::storeConfiguration()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to store the config file in the application's directory."));
        return;
    }

    dMainWindow().reloadConfig();

    this->close();
}

void SettingsDialog::on_settingsCancelButton_clicked()
{
    this->close();
}

void SettingsDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
