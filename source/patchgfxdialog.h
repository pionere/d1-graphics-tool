#pragma once

#include <QDialog>

#include "d1gfx.h"

class CelView;
class GfxsetView;

namespace Ui {
class PatchGfxDialog;
}

class PatchGfxDialog : public QDialog {
    Q_OBJECT

public:
    explicit PatchGfxDialog(QWidget *parent);
    ~PatchGfxDialog();

    void initialize(D1Gfx *gfx, CelView *celView, GfxsetView *gfxsetView);

private slots:
    void on_runButton_clicked();
    void on_cancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::PatchGfxDialog *ui;

    D1Gfx *gfx;
    CelView *celView;
    GfxsetView *gfxsetView;
};
