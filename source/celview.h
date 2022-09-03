#pragma once

#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QTimer>
#include <QWidget>

#include "d1celbase.h"

#define CEL_SCENE_SPACING 8

namespace Ui {
class CelScene;
class CelView;
} // namespace Ui

class CelScene : public QGraphicsScene {
    Q_OBJECT

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

signals:
    void framePixelClicked(quint16, quint16);
};

class CelView : public QWidget {
    Q_OBJECT

public:
    explicit CelView(QWidget *parent = nullptr);
    ~CelView();

    void initialize(D1CelBase *c);
    D1CelBase *getCel();
    QString getCelPath();
    quint32 getCurrentFrameIndex();
    void framePixelClicked(quint16, quint16);

    void displayFrame();
    bool checkGroupNumber();
    void setGroupNumber();

signals:
    void frameRefreshed();
    void colorIndexClicked(quint8);

private slots:
    void on_firstFrameButton_clicked();
    void on_previousFrameButton_clicked();
    void on_nextFrameButton_clicked();
    void on_lastFrameButton_clicked();
    void on_frameIndexEdit_returnPressed();

    void on_firstGroupButton_clicked();
    void on_previousGroupButton_clicked();
    void on_groupIndexEdit_returnPressed();
    void on_nextGroupButton_clicked();
    void on_lastGroupButton_clicked();

    void on_zoomOutButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomEdit_returnPressed();

    void on_playButton_clicked();
    void on_stopButton_clicked();
    void playGroup();

private:
    Ui::CelView *ui;
    CelScene *celScene;

    D1CelBase *cel;
    quint16 currentGroupIndex = 0;
    quint32 currentFrameIndex = 0;
    quint8 currentZoomFactor = 1;

    QTimer playTimer;
};
