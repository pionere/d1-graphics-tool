#pragma once

#include <vector>

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

#include "celview.h"
#include "d1dun.h"
#include "d1gfx.h"
#include "d1pal.h"
#include "d1tileset.h"
#include "leveltabframewidget.h"
#include "leveltabsubtilewidget.h"
#include "leveltabtilewidget.h"
#include "pushbuttonwidget.h"
#include "upscaledialog.h"

namespace Ui {
class LevelCelView;
} // namespace Ui

enum class IMAGE_FILE_MODE;

class LevelCelView : public QWidget {
    Q_OBJECT

public:
    explicit LevelCelView(QWidget *parent);
    ~LevelCelView();

    void initialize(D1Pal *pal, D1Tileset *tileset, D1Dun *dun);
    void setPal(D1Pal *pal);

    CelScene *getCelScene() const;
    int getCurrentFrameIndex() const;
    int getCurrentSubtileIndex() const;
    int getCurrentTileIndex() const;

    void framePixelClicked(const QPoint &pos, bool first);

    void insertImageFiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);

    void addToCurrentFrame(const QString &imagefilePath);
    void replaceCurrentFrame(const QString &imagefilePath);
    void removeCurrentFrame();

    void createSubtile();
    void replaceCurrentSubtile(const QString &imagefilePath);
    void removeCurrentSubtile();

    void createTile();
    void replaceCurrentTile(const QString &imagefilePath);
    void removeCurrentTile();

    QImage copyCurrent() const;
    void pasteCurrent(const QImage &image);

    void reportUsage();
    void resetFrameTypes();
    void inefficientFrames();
    void checkSubtileFlags();
    void checkTileFlags();
    void checkTilesetFlags();
    void cleanupFrames();
    void cleanupSubtiles();
    void cleanupTileset();
    void compressSubtiles();
    void compressTiles();
    void compressTileset();
    void sortFrames();
    void sortSubtiles();
    void sortTileset();

    void upscale(const UpscaleParam &params);

    void update();
    void updateLabel();
    void displayFrame();

private:
    void collectFrameUsers(int frameIndex, std::vector<int> &users) const;
    void collectSubtileUsers(int subtileIndex, std::vector<int> &users) const;
    void insertFrames(IMAGE_FILE_MODE mode, int index, const QImage &image);
    bool insertFrames(IMAGE_FILE_MODE mode, int index, const D1GfxFrame &frame);
    void insertFrames(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void insertFrames(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);
    void insertSubtile(int subtileIndex, const QImage &image);
    void insertSubtile(int subtileIndex, const D1GfxFrame &frame);
    void insertSubtiles(IMAGE_FILE_MODE mode, int index, const QImage &image);
    bool insertSubtiles(IMAGE_FILE_MODE mode, int index, const D1GfxFrame &frame);
    void insertSubtiles(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void insertSubtiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);
    void insertTile(int tileIndex, const QImage &image);
    void insertTile(int tileIndex, const D1GfxFrame &frame);
    void insertTiles(IMAGE_FILE_MODE mode, int index, const QImage &image);
    bool insertTiles(IMAGE_FILE_MODE mode, int index, const D1GfxFrame &frame);
    void insertTiles(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void insertTiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);
    void assignFrames(const QImage &image, int subtileIndex, int frameIndex);
    void assignFrames(const D1GfxFrame &frame, int subtileIndex, int frameIndex);
    void assignSubtiles(const QImage &image, int tileIndex, int subtileIndex);
    void assignSubtiles(const D1GfxFrame &frame, int tileIndex, int subtileIndex);
    void removeFrame(int frameIndex);
    void removeSubtile(int subtileIndex);
    bool removeUnusedFrames();
    bool removeUnusedSubtiles();
    bool reuseFrames();
    bool reuseSubtiles();
    bool sortFrames_impl();
    bool sortSubtiles_impl();
    void setFrameIndex(int frameIndex);
    void setSubtileIndex(int subtileIndex);
    void setTileIndex(int tileIndex);

signals:
    void frameRefreshed();
    void palModified();

private slots:
    void on_firstFrameButton_clicked();
    void on_previousFrameButton_clicked();
    void on_nextFrameButton_clicked();
    void on_lastFrameButton_clicked();
    void on_frameIndexEdit_returnPressed();
    void on_frameIndexEdit_escPressed();

    void on_firstSubtileButton_clicked();
    void on_previousSubtileButton_clicked();
    void on_nextSubtileButton_clicked();
    void on_lastSubtileButton_clicked();
    void on_subtileIndexEdit_returnPressed();
    void on_subtileIndexEdit_escPressed();

    void on_firstTileButton_clicked();
    void on_previousTileButton_clicked();
    void on_nextTileButton_clicked();
    void on_lastTileButton_clicked();
    void on_tileIndexEdit_returnPressed();
    void on_tileIndexEdit_escPressed();

    void on_minFrameWidthEdit_returnPressed();
    void on_minFrameWidthEdit_escPressed();
    void on_minFrameHeightEdit_returnPressed();
    void on_minFrameHeightEdit_escPressed();

    void on_zoomOutButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomEdit_returnPressed();
    void on_zoomEdit_escPressed();

    void on_playDelayEdit_returnPressed();
    void on_playDelayEdit_escPressed();
    void on_playButton_clicked();
    void on_stopButton_clicked();
    void playGroup();

    void on_actionToggle_View_triggered();

    void on_dungeonPosXLineEdit_returnPressed();
    void on_dungeonPosXLineEdit_escPressed();
    void on_dungeonPosYLineEdit_returnPressed();
    void on_dungeonPosYLineEdit_escPressed();

    void on_dunWidthEdit_returnPressed();
    void on_dunWidthEdit_escPressed();
    void on_dunHeightEdit_returnPressed();
    void on_dunHeightEdit_escPressed();

    void on_showTilesCheckBox_clicked();
    void on_dungeonTileLineEdit_returnPressed();
    void on_dungeonTileLineEdit_escPressed();
    void on_dungeonSubtileLineEdit_returnPressed();
    void on_dungeonSubtileLineEdit_escPressed();
    void on_showItemsCheckBox_clicked();
    void on_dungeonItemLineEdit_returnPressed();
    void on_dungeonItemLineEdit_escPressed();
    void on_showMonstersCheckBox_clicked();
    void on_dungeonMonsterLineEdit_returnPressed();
    void on_dungeonMonsterLineEdit_escPressed();
    void on_showObjectsCheckBox_clicked();
    void on_dungeonObjectLineEdit_returnPressed();
    void on_dungeonObjectLineEdit_escPressed();
    void on_dungeonTransvalLineEdit_returnPressed();
    void on_dungeonTransvalLineEdit_escPressed();

    void on_dunZoomOutButton_clicked();
    void on_dunZoomInButton_clicked();
    void on_dunZoomEdit_returnPressed();
    void on_dunZoomEdit_escPressed();

    void on_dunPlayDelayEdit_returnPressed();
    void on_dunPlayDelayEdit_escPressed();
    void on_dunPlayButton_clicked();
    void on_dunStopButton_clicked();

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void ShowContextMenu(const QPoint &pos);

private:
    Ui::LevelCelView *ui;
    CelScene celScene = CelScene(this);
    LevelTabTileWidget tabTileWidget = LevelTabTileWidget(this);
    LevelTabSubtileWidget tabSubtileWidget = LevelTabSubtileWidget(this);
    LevelTabFrameWidget tabFrameWidget = LevelTabFrameWidget(this);
    PushButtonWidget *viewBtn;

    D1Pal *pal;
    D1Gfx *gfx;
    D1Tileset *tileset;
    D1Min *min;
    D1Til *til;
    D1Sol *sol;
    D1Amp *amp;
    D1Tmi *tmi;
    D1Dun *dun;
    bool dunView = false;
    int currentFrameIndex = 0;
    int currentSubtileIndex = 0;
    int currentTileIndex = 0;
    quint16 tilesetPlayDelay = 50;
    quint16 dunviewPlayDelay = 50;
    int currentDunPosX = 0;
    int currentDunPosY = 0;

    QTimer playTimer;
};
