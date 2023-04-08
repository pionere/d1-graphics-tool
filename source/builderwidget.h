#pragma once

#include <QFrame>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPoint>
#include <QUndoCommand>

#include "d1dun.h"

typedef enum builder_edit_mode {
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
    explicit EditDungeonCommand(D1Dun *dun, int cellX, int cellY, int value);
    ~EditFrameCommand() = default;

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

class BuilderWidget : public QFrame {
    Q_OBJECT

public:
    explicit BuilderWidget(QWidget *parent, D1Dun *dun, LevelCelView *levelCelView);
    ~BuilderWidget();

    void show(); // override;
    void hide(); // override;

private:
    void stopMove();

    void setTileIndex(int tileIndex);
    void setSubtileIndex(int subtileIndex);
    void setObjectIndex(int objectIndex);
    void setMonsterIndex(int monsterIndex);

public slots:
    bool dunClicked(int cellX, int cellY, bool first);
    void colorModified();
    void dunResourcesModified();

private slots:
    void on_closePushButtonClicked();
    void on_movePushButtonClicked();
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    void on_builderModeComboBox_activated(int index);

    void on_tileLineEdit_escPressed();
    void on_tileLineEdit_returnPressed();
    void on_subtileLineEdit_escPressed();
    void on_subtileLineEdit_returnPressed();
    void on_objectLineEdit_escPressed();
    void on_objectLineEdit_returnPressed();
    void on_objectComboBox_activated(int index);
    void on_monsterLineEdit_escPressed();
    void on_monsterLineEdit_returnPressed();
    void on_monsterComboBox_activated(int index);

private:
    Ui::BuilderWidget *ui;
    QUndoStack *undoStack;
    D1Dun *dun;
    LevelCelView *levelCelView;
    QGraphicsView *graphView;
    bool moving;
    bool moved;
    int mode; // builder_edit_mode

    int currentTileIndex = 0;
    int currentSubtileIndex = 0;
    int currentObjectIndex = 0;
    int currentMonsterIndex = 0;
};
