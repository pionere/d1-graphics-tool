#pragma once

#include <QDialog>

class D1Gfx;

enum class ANTI_ALIASING_MODE {
    BASIC,
    TILESET,
    NONE,
};

class UpscaleParam {
public:
    int multiplier;
    QString palPath;
    int firstfixcolor;
    int lastfixcolor;
    ANTI_ALIASING_MODE antiAliasingMode;
};

namespace Ui {
class UpscaleDialog;
}

class UpscaleDialog : public QDialog {
    Q_OBJECT

public:
    explicit UpscaleDialog(QWidget *parent = nullptr);
    ~UpscaleDialog();

    void initialize(D1Gfx *gfx);

private slots:
    void on_palFileBrowseButton_clicked();
    void on_palFileClearButton_clicked();
    void on_levelTypeComboBox_activated(int index);
    void on_upscaleButton_clicked();
    void on_upscaleCancelButton_clicked();

private:
    Ui::UpscaleDialog *ui;
};
