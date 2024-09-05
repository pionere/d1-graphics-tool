#pragma once

#include <QDialog>

enum class OPEN_CLIPPED_TYPE {
    AUTODETECT,
    TRUE,
    FALSE,
};

enum class OPEN_GFX_TYPE {
    AUTODETECT,
    DIABLO,
    HELLFIRE,
    BASIC,
    TILESET,
    GFXSET,
};

enum class OPEN_HERO_TYPE {
    AUTODETECT,
    DIABLO,
    HELLFIRE,
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
    OPEN_HERO_TYPE heroType = OPEN_HERO_TYPE::AUTODETECT;
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
    void on_openButton_clicked();
    void on_openCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::OpenAsDialog *ui;
};
