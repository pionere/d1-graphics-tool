#pragma once

#include <QDialog>

enum class OPEN_CLIPPED_TYPE {
    AUTODETECT,
    TRUE,
    FALSE,
};

enum class OPEN_GFX_TYPE {
    AUTODETECT,
    BASIC,
    TILESET,
    GFXSET,
};

enum class OPEN_UPSCALED_TYPE {
    AUTODETECT,
    TRUE,
    FALSE,
};

class OpenAsParam {
public:
    QString celFilePath;
    OPEN_GFX_TYPE gfxType = OPEN_GFX_TYPE::AUTODETECT;

    int celWidth = 0;
    OPEN_CLIPPED_TYPE clipped = OPEN_CLIPPED_TYPE::AUTODETECT;

    OPEN_UPSCALED_TYPE upscaled = OPEN_UPSCALED_TYPE::AUTODETECT;
    QString clsFilePath;
    QString tilFilePath;
    QString minFilePath;
    QString solFilePath;
    QString ampFilePath;
    QString sptFilePath;
    QString tmiFilePath;
    QString dunFilePath;
    QString tblFilePath;
    int minWidth = 0;
    int minHeight = 0;
    bool createDun = false;
};

namespace Ui {
class OpenAsDialog;
}

class OpenAsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OpenAsDialog(QWidget *parent);
    ~OpenAsDialog();

    void initialize();

private:
    void updateFields();

private slots:
    void on_inputFileBrowseButton_clicked();
    void on_isTilesetYesRadioButton_toggled(bool checked);
    void on_isTilesetNoRadioButton_toggled(bool checked);
    void on_isTilesetAutoRadioButton_toggled(bool checked);
    void on_clsFileBrowseButton_clicked();
    void on_tilFileBrowseButton_clicked();
    void on_minFileBrowseButton_clicked();
    void on_solFileBrowseButton_clicked();
    void on_ampFileBrowseButton_clicked();
    void on_sptFileBrowseButton_clicked();
    void on_tmiFileBrowseButton_clicked();
    void on_dunFileBrowseButton_clicked();
    void on_tblFileBrowseButton_clicked();
    void on_openButton_clicked();
    void on_openCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::OpenAsDialog *ui;
};
