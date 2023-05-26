#pragma once

#include <QDialog>

#include "d1dun.h"
#include "d1tileset.h"
#include "levelcelview.h"

namespace Ui {
class PatchTilesetDialog;
}

class PatchTilesetDialog : public QDialog {
    Q_OBJECT

public:
    explicit PatchTilesetDialog(QWidget *parent);
    ~PatchTilesetDialog();

    void initialize(D1Tileset *tileset, D1Dun *dun, LevelCelView *levelCelView);

private slots:
    void on_runButton_clicked();
    void on_cancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::PatchTilesetDialog *ui;

    D1Tileset *tileset;
    D1Dun *dun;
    LevelCelView *levelCelView;
};
