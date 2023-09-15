#pragma once

#include <vector>

#include <QDialog>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPointer>

#include "trngeneratecolentrywidget.h"
#include "trngeneratepalentrywidget.h"

class D1Pal;
class D1Trn;
class TrnGeneratePalPopupDialog;

typedef struct GenerateTrnColor {
    int firstcolor;
    int lastcolor;
    double shadesteps;
    bool deltasteps;
    double shadestepsmpl;
    bool protcolor;
} GenerateTrnColor;

class GenerateTrnParam {
public:
    std::vector<GenerateTrnColor> colors;
    std::vector<D1Pal *> pals;
    int mode;
    int colorSelector;
    double redWeight;
    double greenWeight;
    double blueWeight;
    double lightWeight;
};

namespace Ui {
class TrnGenerateDialog;
}

class TrnGenerateDialog;

class PalScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit PalScene(QWidget *view);
    ~PalScene();

    void initialize(D1Pal *pal, D1Trn *trn);
    void setSelectedIndex(int index);
    void displayColors();

private:
    void colorSelected(int index);
    void mouseEvent(QGraphicsSceneMouseEvent *event, int flags);
    void mouseHoverEvent(QGraphicsSceneMouseEvent *event);

private slots:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

//signals:
//    void colorClicked(quint8 index);

private:
    QWidget *view;
    TrnGeneratePalPopupDialog *popup = nullptr;

    D1Pal *pal;
    D1Trn *trn;
    int selectedIndex;
};

class TrnGenerateDialog : public QDialog {
    Q_OBJECT

public:
    explicit TrnGenerateDialog(QWidget *parent);
    ~TrnGenerateDialog();

    void initialize(D1Pal *pal);

    void on_actionDelRange_triggered(TrnGenerateColEntryWidget *caller);
    void on_actionDelPalette_triggered(TrnGeneratePalEntryWidget *caller);
    void updatePals();

private:
    void clearLists();

private slots:
    void on_actionAddRange_triggered();
    void on_actionAddPalette_triggered();
    void on_levelTypeComboBox_activated(int index);
    void on_colorDistanceComboBox_activated(int index);
    void on_selectButtonGroup_idClicked(int index);

    void on_shadeComboBox_activated(int index);

    void on_generateButton_clicked();
    void on_doneButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::TrnGenerateDialog *ui;
    QButtonGroup selectButtonGroup = QButtonGroup(this);
    PalScene shadeScene = PalScene(this);
    PalScene lightScene = PalScene(this);

    D1Pal *pal;
    std::vector<std::vector<D1Pal *>> shadePals;
    std::vector<D1Trn *> lightTrns;
};
