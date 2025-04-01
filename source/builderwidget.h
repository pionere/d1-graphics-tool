#pragma once

#include <vector>

#include <QFrame>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>
#include <QUndoCommand>

#include "d1dun.h"

typedef enum builder_edit_mode {
    BEM_SELECT,
    BEM_TILE,
    BEM_TILE_PROTECTION,
    BEM_SUBTILE,
    BEM_SUBTILE_PROTECTION,
    BEM_OBJECT,
    BEM_MONSTER,
} builder_edit_mode;

typedef struct DunPos {
    DunPos(int cellX, int cellY, int value);

    int cellX;
    int cellY;
    int value;
} DunPos;

class EditDungeonCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditDungeonCommand(D1Dun *dun, int cellX, int cellY, int value, int valueType);
    explicit EditDungeonCommand(D1Dun *dun, const std::vector<DunPos> &modValues, int valueType);
    ~EditDungeonCommand() = default;

    void undo() override;
    void redo() override;

signals:
    void modified();

private:
    QPointer<D1Dun> dun;
    int valueType; // builder_edit_mode
    std::vector<DunPos> modValues;
};

namespace Ui {
class BuilderWidget;
} // namespace Ui

class LevelCelView;
class D1Min;
class D1Tileset;

class BuilderWidget : public QFrame {
    Q_OBJECT

public:
    explicit BuilderWidget(QWidget *parent, QUndoStack *us, D1Dun *dun, LevelCelView *levelCelView, D1Tileset *tileset);
    ~BuilderWidget();

    void setDungeon(D1Dun *dun);

    void show(); // override;
    void hide(); // override;

    int getOverlayType() const;

private:
    void resetPos();
    void stopMove();

    void setTileIndex(int tileIndex);
    void setSubtileIndex(int subtileIndex);
    void setObjectIndex(int objectIndex);
    void setMonsterType(DunMonsterType monType);

    void redrawOverlay(bool forceRedraw);

public slots:
    bool dunClicked(const QPoint &cell, int flags);
    void dunHovered(const QPoint &cell);
    void colorModified();
    void dunSceneModified();
    void dunResourcesModified();

private slots:
    void on_closePushButtonClicked();
    void on_movePushButtonClicked();
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    void on_builderModeComboBox_activated(int index);

    void on_firstTileButton_clicked();
    void on_previousTileButton_clicked();
    void on_nextTileButton_clicked();
    void on_lastTileButton_clicked();
    void on_tileLineEdit_escPressed();
    void on_tileLineEdit_returnPressed();

    void on_tileProtectionModeComboBox_activated(int index);

    void on_firstSubtileButton_clicked();
    void on_previousSubtileButton_clicked();
    void on_nextSubtileButton_clicked();
    void on_lastSubtileButton_clicked();
    void on_subtileLineEdit_escPressed();
    void on_subtileLineEdit_returnPressed();

    void on_subtileProtectionModeComboBox_activated(int index);

    void on_objectLineEdit_escPressed();
    void on_objectLineEdit_returnPressed();
    void on_objectComboBox_activated(int index);
    void on_monsterLineEdit_escPressed();
    void on_monsterLineEdit_returnPressed();
    void on_monsterCheckBox_clicked();
    void on_monsterComboBox_activated(int index);

private:
    Ui::BuilderWidget *ui;
    QUndoStack *undoStack;
    D1Dun *dun;
    LevelCelView *levelCelView;
    D1Tileset *tileset;
    QGraphicsView *graphView;
    bool moving = false;
    bool moved = false;
    QPoint lastPos;
    QPoint lastHoverPos;
    int mode = BEM_TILE;  // builder_edit_mode
    int overlayType = -1; // builder_edit_mode

    int currentTileIndex = 0;
    int currentSubtileIndex = 0;
    int currentObjectIndex = 0;
    DunMonsterType currentMonsterType = { 0, false };
};
