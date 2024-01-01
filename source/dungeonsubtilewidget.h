#pragma once

#include <QFrame>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>

namespace Ui {
class DungeonSubtileWidget;
} // namespace Ui

class LevelCelView;

class DungeonSubtileWidget : public QFrame {
    Q_OBJECT

public:
    explicit DungeonSubtileWidget(LevelCelView *parent);
    ~DungeonSubtileWidget();

    void initialize(int x, int y);

    void show(); // override;
    void hide(); // override;

private:
    void resetPos();
    void stopMove();
    void dungeonModified();

private slots:
    void on_closePushButtonClicked();
    void on_movePushButtonClicked();
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    Ui::DungeonSubtileWidget *ui;
    QGraphicsView *graphView;
    bool moving = false;
    bool moved = false;
    QPoint lastPos;
	int dunX;
	int dunY;
};
