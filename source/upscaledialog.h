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
    void on_levelTypeComboBox_activated(int index);
    void on_upscaleButton_clicked();
    void on_upscaleCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::UpscaleDialog *ui;
};
