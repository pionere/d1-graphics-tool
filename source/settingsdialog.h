#pragma once

#include <QDialog>
#include <QLabel>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent);
    ~SettingsDialog();

    void initialize();

private:
    void setIconColor(QLabel *imageLabel, const QString &colorText);
    void updateIcons();

private slots:
    void on_assetsFolderBrowseButton_clicked();
    void on_graphicsBackgroundColorPushButton_clicked();
    void on_graphicsBackgroundColorLineEdit_returnPressed();
    void on_graphicsBackgroundColorLineEdit_escPressed();
    void on_graphicsTransparentColorPushButton_clicked();
    void on_graphicsTransparentColorLineEdit_returnPressed();
    void on_graphicsTransparentColorLineEdit_escPressed();
    void on_undefinedPaletteColorPushButton_clicked();
    void on_undefinedPaletteColorLineEdit_returnPressed();
    void on_undefinedPaletteColorLineEdit_escPressed();
    void on_paletteSelectionBorderColorPushButton_clicked();
    void on_paletteSelectionBorderColorLineEdit_returnPressed();
    void on_paletteSelectionBorderColorLineEdit_escPressed();
    void on_settingsOkButton_clicked();
    void on_settingsCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::SettingsDialog *ui;

    QString assetsFolder;
    QString graphicsBackgroundColor;
    QString graphicsTransparentColor;
    QString undefinedPaletteColor;
    QString paletteSelectionBorderColor;
};
