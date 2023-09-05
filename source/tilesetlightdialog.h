#pragma once

#include <QDialog>
#include <QVBoxLayout>

#include "tilesetlightentrywidget.h"

class D1Tileset;
class LevelCelView;

typedef struct SubtileLight {
    int firstsubtile;
    int lastsubtile;
    int radius;
} SubtileLight;

namespace Ui {
class TilesetLightDialog;
}

class TilesetLightDialog : public QDialog {
    Q_OBJECT

public:
    explicit TilesetLightDialog(LevelCelView *view);
    ~TilesetLightDialog();

    void initialize(D1Tileset *tileset);

    void on_actionAddRange_triggered();
    void on_actionDelRange_triggered(TilesetLightEntryWidget *caller);

private slots:
    void on_lightDecButton_clicked();
    void on_lightIncButton_clicked();
    void on_lightAcceptButton_clicked();
    void on_lightCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::TilesetLightDialog *ui;
    QVBoxLayout *rangesLayout;

    LevelCelView *view;
    D1Tileset *tileset;
};
