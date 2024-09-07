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
    // initialize the assets folder
    this->assetsFolder = Config::getAssetsFolder();
    this->ui->assetsFolderEdit->setText(this->assetsFolder);
    // reset the color values
    this->graphicsBackgroundColor = Config::getGraphicsBackgroundColor();
    this->graphicsTransparentColor = Config::getGraphicsTransparentColor();
    this->undefinedPaletteColor = Config::getPaletteUndefinedColor();
    this->paletteSelectionBorderColor = Config::getPaletteSelectionBorderColor();
    this->on_graphicsBackgroundColorLineEdit_escPressed();
    this->on_graphicsTransparentColorLineEdit_escPressed();
    this->on_undefinedPaletteColorLineEdit_escPressed();
    this->on_paletteSelectionBorderColorLineEdit_escPressed();

    this->updateIcons();
}

void SettingsDialog::on_assetsFolderBrowseButton_clicked()
{
    QString dirPath = dMainWindow().folderDialog(tr("Select Assets Folder"));

    if (!dirPath.isEmpty()) {
        this->assetsFolder = dirPath;
        this->ui->assetsFolderEdit->setText(this->assetsFolder);
    }
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
    this->setIconColor(this->ui->graphicsBackgroundColorImageLabel, this->graphicsBackgroundColor);
    this->setIconColor(this->ui->graphicsTransparentColorImageLabel, this->graphicsTransparentColor);
    this->setIconColor(this->ui->undefinedPaletteColorImageLabel, this->undefinedPaletteColor);
    this->setIconColor(this->ui->paletteSelectionBorderColorImageLabel, this->paletteSelectionBorderColor);
}

void SettingsDialog::on_graphicsBackgroundColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(this->graphicsBackgroundColor);
    if (color.isValid()) {
        this->graphicsBackgroundColor = color.name();
        this->ui->graphicsBackgroundColorLineEdit->setText(this->graphicsBackgroundColor);
        this->updateIcons();
    }
}

void SettingsDialog::on_graphicsBackgroundColorLineEdit_returnPressed()
{
    QString colorText = this->ui->graphicsBackgroundColorLineEdit->text();
    QColor color = QColor(colorText);
    if (color.isValid()) {
        this->graphicsBackgroundColor = colorText;
        this->updateIcons();
    } else {
        this->on_graphicsBackgroundColorLineEdit_escPressed();
    }
}

void SettingsDialog::on_graphicsBackgroundColorLineEdit_escPressed()
{
    this->ui->graphicsBackgroundColorLineEdit->setText(this->graphicsBackgroundColor);
}

void SettingsDialog::on_graphicsTransparentColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(this->graphicsTransparentColor);
    if (color.isValid()) {
        this->graphicsTransparentColor = color.name();
        this->ui->graphicsTransparentColorLineEdit->setText(this->graphicsTransparentColor);
        this->updateIcons();
    }
}

void SettingsDialog::on_graphicsTransparentColorLineEdit_returnPressed()
{
    QString colorText = this->ui->graphicsTransparentColorLineEdit->text();
    QColor color = QColor(colorText);
    if (color.isValid()) {
        this->graphicsTransparentColor = colorText;
        this->updateIcons();
    } else {
        this->on_graphicsTransparentColorLineEdit_escPressed();
    }
}

void SettingsDialog::on_graphicsTransparentColorLineEdit_escPressed()
{
    this->ui->graphicsTransparentColorLineEdit->setText(this->graphicsTransparentColor);
}

void SettingsDialog::on_undefinedPaletteColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(this->undefinedPaletteColor);
    if (color.isValid()) {
        this->undefinedPaletteColor = color.name();
        this->ui->undefinedPaletteColorLineEdit->setText(this->undefinedPaletteColor);
        this->updateIcons();
    }
}

void SettingsDialog::on_undefinedPaletteColorLineEdit_returnPressed()
{
    QString colorText = this->ui->undefinedPaletteColorLineEdit->text();
    QColor color = QColor(colorText);
    if (color.isValid()) {
        this->undefinedPaletteColor = colorText;
        this->updateIcons();
    } else {
        this->on_undefinedPaletteColorLineEdit_escPressed();
    }
}

void SettingsDialog::on_undefinedPaletteColorLineEdit_escPressed()
{
    this->ui->undefinedPaletteColorLineEdit->setText(this->undefinedPaletteColor);
}

void SettingsDialog::on_paletteSelectionBorderColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(this->paletteSelectionBorderColor);
    if (color.isValid()) {
        this->paletteSelectionBorderColor = color.name();
        this->ui->paletteSelectionBorderColorLineEdit->setText(this->paletteSelectionBorderColor);
        this->updateIcons();
    }
}

void SettingsDialog::on_paletteSelectionBorderColorLineEdit_returnPressed()
{
    QString colorText = this->ui->paletteSelectionBorderColorLineEdit->text();
    QColor color = QColor(colorText);
    if (color.isValid()) {
        this->paletteSelectionBorderColor = colorText;
        this->updateIcons();
    } else {
        this->on_paletteSelectionBorderColorLineEdit_escPressed();
    }
}

void SettingsDialog::on_paletteSelectionBorderColorLineEdit_escPressed()
{
    this->ui->paletteSelectionBorderColorLineEdit->setText(this->paletteSelectionBorderColor);
}

void SettingsDialog::on_settingsOkButton_clicked()
{
    // Locale
    QString locale = this->ui->languageComboBox->currentData().value<QString>();
    Config::setLocale(locale);

    // Assets Folder
    Config::setAssetsFolder(this->assetsFolder);

    // GraphicsBackgroundColor
    QColor gfxBackgroundColor = QColor(this->graphicsBackgroundColor);
    Config::setGraphicsBackgroundColor(gfxBackgroundColor.name());

    // GraphicsTransparentColor
    QColor gfxTransparentColor = QColor(this->graphicsTransparentColor);
    Config::setGraphicsTransparentColor(gfxTransparentColor.name());

    // PaletteUndefinedColor
    QColor palUndefinedColor = QColor(this->undefinedPaletteColor);
    Config::setPaletteUndefinedColor(palUndefinedColor.name());

    // PaletteSelectionBorderColor
    QColor palSelectionBorderColor = QColor(this->paletteSelectionBorderColor);
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
