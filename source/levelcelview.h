#pragma once

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QPoint>
#include <QTimer>
#include <QWidget>

#include "d1amp.h"
#include "d1gfx.h"
#include "d1min.h"
#include "d1sol.h"
#include "d1til.h"
#include "d1tmi.h"
#include "leveltabframewidget.h"
#include "leveltabsubtilewidget.h"
#include "leveltabtilewidget.h"

#define CEL_SCENE_SPACING 8

namespace Ui {
class LevelCelScene;
class LevelCelView;
} // namespace Ui

enum class IMAGE_FILE_MODE;

class LevelCelScene : public QGraphicsScene {
    Q_OBJECT

public:
    LevelCelScene(QWidget *view);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
    void dropEvent(QGraphicsSceneDragDropEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

signals:
    void framePixelClicked(unsigned x, unsigned y);

private:
    QWidget *view;
};

class LevelCelView : public QWidget {
    Q_OBJECT

public:
    explicit LevelCelView(QWidget *parent = nullptr);
    ~LevelCelView();

    void initialize(D1Gfx *gfx, D1Min *min, D1Til *til, D1Sol *sol, D1Amp *amp, D1Tmi *tmi);

    int getCurrentFrameIndex();
    int getCurrentSubtileIndex();
    int getCurrentTileIndex();

    void framePixelClicked(unsigned x, unsigned y);

    void insertImageFiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);

    void replaceCurrentFrame(const QString &imagefilePath);
    void removeCurrentFrame();

    void createSubtile();
    void replaceCurrentSubtile(const QString &imagefilePath);
    void removeCurrentSubtile();

    void createTile();
    void replaceCurrentTile(const QString &imagefilePath);
    void removeCurrentTile();

    void collectFrameUsers(int frameIndex, QList<int> users);
    void collectSubtileUsers(int subtileIndex, QList<int> users);
    void reportUsage();

    void displayFrame();

private:
    void update();
    void insertFrame(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void insertFrames(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);
    void insertSubtile(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void insertSubtiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);
    void insertTile(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void insertTiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);
    void assignFrames(const QImage &image, int subtileIndex, int frameIndex);
    void assignSubtiles(const QImage &image, int tileIndex, int subtileIndex);

signals:
    void frameRefreshed();
    void colorIndexClicked(quint8);

public slots:
    void ShowContextMenu(const QPoint &pos);

private slots:
    void on_firstFrameButton_clicked();
    void on_previousFrameButton_clicked();
    void on_nextFrameButton_clicked();
    void on_lastFrameButton_clicked();
    void on_frameIndexEdit_returnPressed();

    void on_firstSubtileButton_clicked();
    void on_previousSubtileButton_clicked();
    void on_nextSubtileButton_clicked();
    void on_lastSubtileButton_clicked();
    void on_subtileIndexEdit_returnPressed();

    void on_firstTileButton_clicked();
    void on_previousTileButton_clicked();
    void on_nextTileButton_clicked();
    void on_lastTileButton_clicked();
    void on_tileIndexEdit_returnPressed();

    void on_minFrameWidthEdit_returnPressed();
    void on_minFrameHeightEdit_returnPressed();

    void on_zoomOutButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomEdit_returnPressed();

    void on_playDelayEdit_textChanged(const QString &text);
    void on_playButton_clicked();
    void on_stopButton_clicked();
    void playGroup();

    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

private:
    Ui::LevelCelView *ui;
    LevelCelScene *celScene;
    LevelTabTileWidget *tabTileWidget = new LevelTabTileWidget();
    LevelTabSubTileWidget *tabSubTileWidget = new LevelTabSubTileWidget();
    LevelTabFrameWidget *tabFrameWidget = new LevelTabFrameWidget();

    D1Gfx *gfx;
    D1Min *min;
    D1Til *til;
    D1Sol *sol;
    D1Amp *amp;
    D1Tmi *tmi;
    int currentFrameIndex = 0;
    int currentSubtileIndex = 0;
    int currentTileIndex = 0;
    quint8 currentZoomFactor = 1;
    quint16 currentPlayDelay = 50;

    QTimer playTimer;
};
