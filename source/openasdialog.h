#pragma once

#include <QDialog>

enum class OPEN_CLIPPED_TYPE {
    AUTODETECT,
    TRUE,
    FALSE,
};

enum class OPEN_TILESET_TYPE {
    AUTODETECT,
    TRUE,
    FALSE,
};

enum class OPEN_UPSCALED_TYPE {
    AUTODETECT,
    TRUE,
    FALSE,
};

class OpenAsParam {
public:
    QString celFilePath;
    OPEN_TILESET_TYPE isTileset = OPEN_TILESET_TYPE::AUTODETECT;

    quint16 celWidth = 0;
    OPEN_CLIPPED_TYPE clipped = OPEN_CLIPPED_TYPE::AUTODETECT;

    OPEN_UPSCALED_TYPE upscaled = OPEN_UPSCALED_TYPE::AUTODETECT;
    QString tilFilePath;
    QString minFilePath;
    QString solFilePath;
    QString ampFilePath;
    QString tmiFilePath;
    quint16 minWidth = 0;
    quint16 minHeight = 0;
};

namespace Ui {
class OpenAsDialog;
}

class OpenAsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OpenAsDialog(QWidget *parent = nullptr);
    ~OpenAsDialog();

    void initialize();

private:
    void update();

private slots:
    void on_inputFileBrowseButton_clicked();
    void on_isTilesetYesRadioButton_toggled(bool checked);
    void on_isTilesetNoRadioButton_toggled(bool checked);
    void on_isTilesetAutoRadioButton_toggled(bool checked);
    void on_tilFileBrowseButton_clicked();
    void on_minFileBrowseButton_clicked();
    void on_solFileBrowseButton_clicked();
    void on_ampFileBrowseButton_clicked();
    void on_tmiFileBrowseButton_clicked();
    void on_openButton_clicked();
    void on_openCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::OpenAsDialog *ui;
};
