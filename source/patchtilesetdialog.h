#pragma once

#include <QDialog>

#include "d1tileset.h"

namespace Ui {
class PatchTilesetDialog;
}

class PatchTilesetDialog : public QDialog {
    Q_OBJECT

public:
    explicit PatchTilesetDialog(QWidget *parent = nullptr);
    ~PatchTilesetDialog();

    void initialize(D1Tileset *tileset);

private slots:
    void on_runButton_clicked();
    void on_cancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event);

private:
    Ui::PatchTilesetDialog *ui;

    D1Tileset *tileset;
};
