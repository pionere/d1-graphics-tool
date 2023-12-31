#pragma once

#include <vector>

#include <QFrame>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>
#include <QUndoCommand>

#include "levelcelview.h"

namespace Ui {
class DungeonMonsterWidget;
} // namespace Ui

struct MonsterStruct;

class DungeonMonsterWidget : public QFrame {
    Q_OBJECT

public:
    explicit DungeonMonsterWidget(LevelCelView *parent);
    ~DungeonMonsterWidget();

    void setMonster(MonsterStruct *mon);

    void show(); // override;
    void hide(); // override;

private:
    void resetPos();
    void stopMove();

private slots:
    void on_closePushButtonClicked();
    void on_movePushButtonClicked();
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    Ui::DungeonMonsterWidget *ui;
    QGraphicsView *graphView;
    bool moving = false;
    bool moved = false;
    QPoint lastPos;
    MonsterStruct *mon;
};
