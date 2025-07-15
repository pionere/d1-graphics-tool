#pragma once

#include <vector>

#include <QComboBox>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QPoint>
#include <QTimer>
#include <QUndoStack>
#include <QWidget>

#include "celview.h"
#include "d1dun.h"
#include "d1gfx.h"
#include "d1pal.h"
#include "d1tileset.h"
#include "dungeondecoratedialog.h"
#include "dungeongeneratedialog.h"
#include "dungeonresourcedialog.h"
#include "dungeonsearchdialog.h"
#include "dungeonsubtilewidget.h"
#include "leveltabframewidget.h"
#include "leveltabsubtilewidget.h"
#include "leveltabtilewidget.h"
#include "pushbuttonwidget.h"
#include "tilesetlightdialog.h"

namespace Ui {
class LevelCelView;
} // namespace Ui

enum class IMAGE_FILE_MODE;

class LevelCelView : public QWidget {
    Q_OBJECT

public:
    explicit LevelCelView(QWidget *parent, QUndoStack *undoStack);
    ~LevelCelView();

    void initialize(D1Pal *pal, D1Tileset *tileset, D1Dun *dun, bool bottomPanelHidden);
    void setPal(D1Pal *pal);
    void setTileset(D1Tileset *tileset);
    void setGfx(D1Gfx *gfx);
    void setDungeon(D1Dun *dun);

    CelScene *getCelScene() const;
    int getCurrentFrameIndex() const;
    int getCurrentSubtileIndex() const;
    int getCurrentTileIndex() const;
    const QComboBox *getObjects() const;
    const QComboBox *getMonsters() const;

    void framePixelClicked(const QPoint &pos, int flags);
    void framePixelHovered(const QPoint &pos) const;

    void insertImageFiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);

    void addToCurrentFrame(const QString &imagefilePath);
    void createFrame(bool append);
    void duplicateCurrentFrame(bool wholeGroup);
    void replaceCurrentFrame(const QString &imagefilePath);
    void removeCurrentFrame(bool wholeGroup);
    void flipHorizontalCurrentFrame(bool wholeGroup);
    void flipVerticalCurrentFrame(bool wholeGroup);
    void mergeFrames(const MergeFramesParam &params);

    void createSubtile(bool append);
    void duplicateCurrentSubtile(bool deepCopy);
    void replaceCurrentSubtile(const QString &imagefilePath);
    void removeCurrentSubtile();

    void createTile(bool append);
    void duplicateCurrentTile(bool deepCopy);
    void replaceCurrentTile(const QString &imagefilePath);
    void removeCurrentTile();

    QString copyCurrentPixels(bool values) const;
    void pasteCurrentPixels(const QString &pixels);
    QImage copyCurrentImage() const;
    void pasteCurrentImage(const QImage &image);

    void coloredFrames(const std::pair<int, int>& colors) const;
    void coloredSubtiles(const std::pair<int, int>& colors) const;
    void coloredTiles(const std::pair<int, int>& colors) const;
    void activeFrames() const;
    void activeSubtiles() const;
    void activeTiles() const;
    void reportUsage() const;
    void inefficientFrames() const;
    void resetFrameTypes();
    void lightSubtiles();
    void checkSubtileFlags() const;
    void checkTileFlags() const;
    void checkTilesetFlags() const;
    void cleanupFrames();
    void cleanupSubtiles();
    void cleanupTileset();
    void compressSubtiles();
    void compressTiles();
    void compressTileset();
    void sortFrames();
    void sortSubtiles();
    void sortTileset();

    void reportDungeonUsage() const;
    void resetDungeonTiles();
    void resetDungeonSubtiles();
    void maskDungeonTiles(const D1Dun *srcDun);
    void protectDungeonTiles();
    void protectDungeonTilesFrom(const D1Dun *srcDun);
    void protectDungeonSubtiles();
    void checkTiles() const;
    void checkProtections() const;
    void checkItems() const;
    void checkMonsters() const;
    void checkObjects() const;
    void checkEntities() const;
    void removeTiles();
    void removeProtections();
    void removeItems();
    void removeMonsters();
    void removeObjects();
    void removeMissiles();
    void loadTiles(const D1Dun *srcDun);
    void loadProtections(const D1Dun *srcDun);
    void loadItems(const D1Dun *srcDun);
    void loadMonsters(const D1Dun *srcDun);
    void loadObjects(const D1Dun *srcDun);
    void generateDungeon();
    void decorateDungeon();
    void searchDungeon();

    void upscale(const UpscaleParam &params);

    static int findMonType(const QComboBox *comboBox, const DunMonsterType &value);

    void updateFields();
    void updateLabel();
    void updateTilesetIcon();
    void updateEntityOptions();
    void displayFrame();
    void toggleBottomPanel();

    void scrollTo(int posx, int posy); // initiate scrolling
    void scrollToCurrent();            // do the scrolling
    void selectPos(const QPoint &cell, int flags);

private:
    void collectFrameUsers(int frameIndex, std::vector<int> &users) const;
    void collectSubtileUsers(int subtileIndex, std::vector<int> &users) const;
    void warnOrReportSubtile(const QString &msg, int subtileIndex) const;
    void insertFrames(IMAGE_FILE_MODE mode, int index, const QImage &image);
    bool insertFrames(IMAGE_FILE_MODE mode, int index, const D1GfxFrame &frame);
    void insertFrames(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void insertFrames(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);
    void insertSubtile(int subtileIndex, const std::vector<unsigned> &frameReferencesList);
    void insertSubtile(int subtileIndex, const QImage &image);
    void insertSubtile(int subtileIndex, const D1GfxFrame &frame);
    void insertSubtiles(IMAGE_FILE_MODE mode, int index, const QImage &image);
    bool insertSubtiles(IMAGE_FILE_MODE mode, int index, const D1GfxFrame &frame);
    void insertSubtiles(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void insertSubtiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);
    void insertTile(int tileIndex, const std::vector<int> &subtileIndices);
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
    bool reuseTiles();
    bool sortFrames_impl();
    bool sortSubtiles_impl();
    void setFrameIndex(int frameIndex);
    void setSubtileIndex(int subtileIndex);
    void setTileIndex(int tileIndex);
    void swapSubtiles(unsigned subtileIndex0, unsigned subtileIndex1);
    void swapTiles(unsigned tileIndex0, unsigned tileIndex1);
    void setMonsterType(int monsterIndex, bool monsterUnique, bool monsterDead);
    void setObjectIndex(int objectIndex);
    void setCurrentMonster(const MapMonster &mon);
    void setCurrentMissile(const MapMissile &mis);

    void selectTilesetPath(QString path);
    void selectAssetPath(QString path);
    void setPosition(int posx, int posy);
    void shiftPosition(int dx, int dy);
    QPoint getCellPos(const QPoint &pos) const;
    bool subtilePos(QPoint &pos) const;
    bool tilePos(QPoint &pos) const;
    bool framePos(QPoint &pos) const;
    void showSubtileInfo();

signals:
    void frameRefreshed();
    void palModified();
    void dunResourcesModified();

public slots:
    void on_tilModified();
    void on_actionToggle_View_triggered();

private slots:
    void on_showSpecTileCheckBox_clicked();
    void on_showSpecSubtileCheckBox_clicked();

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
    void on_playStopButton_clicked();

    void on_moveLeftButton_clicked();
    void on_moveRightButton_clicked();
    void on_moveUpButton_clicked();
    void on_moveDownButton_clicked();
    void on_dungeonPosLineEdit_returnPressed();
    void on_dungeonPosLineEdit_escPressed();

    void on_dunWidthEdit_returnPressed();
    void on_dunWidthEdit_escPressed();
    void on_dunHeightEdit_returnPressed();
    void on_dunHeightEdit_escPressed();

    void on_showButtonGroup_idClicked();

    void on_showItemsCheckBox_clicked();
    void on_showMonstersCheckBox_clicked();
    void on_showObjectsCheckBox_clicked();
    void on_showMissilesCheckBox_clicked();

    void on_dunOverlayComboBox_activated(int index);

    void on_levelTypeComboBox_activated(int index);
    void on_dungeonDefaultTileLineEdit_returnPressed();
    void on_dungeonDefaultTileLineEdit_escPressed();
    void on_dungeonTileLineEdit_returnPressed();
    void on_dungeonTileLineEdit_escPressed();
    void on_dungeonTileProtectionCheckBox_clicked();
    void on_tilesetLoadPushButton_clicked();
    void on_tilesetClearPushButton_clicked();
    void on_assetLoadPushButton_clicked();
    void on_assetClearPushButton_clicked();
    void on_dungeonSubtileLineEdit_returnPressed();
    void on_dungeonSubtileLineEdit_escPressed();
    void on_dungeonSubtileMonProtectionCheckBox_clicked();
    void on_dungeonSubtileObjProtectionCheckBox_clicked();
    void on_dungeonObjectLineEdit_returnPressed();
    void on_dungeonObjectLineEdit_escPressed();
    void on_dungeonObjectComboBox_activated(int index);
    void on_dungeonObjectAddButton_clicked();
    void on_dungeonObjectPreCheckBox_clicked();
    void on_dungeonMonsterLineEdit_returnPressed();
    void on_dungeonMonsterLineEdit_escPressed();
    void on_dungeonMonsterCheckBox_clicked();
    void on_dungeonMonsterComboBox_activated(int index);
    void on_dungeonMonsterAddButton_clicked();
    void on_dungeonMonsterXOffSpinBox_valueChanged(int value);
    void on_dungeonMonsterYOffSpinBox_valueChanged(int value);
    void on_dungeonMonsterDeadCheckBox_clicked();
    void on_dungeonItemLineEdit_returnPressed();
    void on_dungeonItemLineEdit_escPressed();
    void on_dungeonItemComboBox_activated(int index);
    void on_dungeonItemAddButton_clicked();
    void on_dungeonRoomLineEdit_returnPressed();
    void on_dungeonRoomLineEdit_escPressed();
    void on_dungeonMissileLineEdit_returnPressed();
    void on_dungeonMissileLineEdit_escPressed();
    void on_dungeonMissileComboBox_activated(int index);
    void on_dungeonMissileAddButton_clicked();
    void on_dungeonMissileXOffSpinBox_valueChanged(int value);
    void on_dungeonMissileYOffSpinBox_valueChanged(int value);
    void on_dungeonMissilePreCheckBox_clicked();

    void on_dunZoomOutButton_clicked();
    void on_dunZoomInButton_clicked();
    void on_dunZoomEdit_returnPressed();
    void on_dunZoomEdit_escPressed();

    void on_dunPlayDelayEdit_returnPressed();
    void on_dunPlayDelayEdit_escPressed();
    void on_dunPlayStopButton_clicked();

    void timerEvent(QTimerEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void ShowContextMenu(const QPoint &pos);

    void on_actionInsert_DunTileRow_triggered();
    void on_actionInsert_DunTileColumn_triggered();
    void on_actionDel_DunTileRow_triggered();
    void on_actionDel_DunTileColumn_triggered();

private:
    Ui::LevelCelView *ui;
    QUndoStack *undoStack;
    CelScene celScene = CelScene(this);
    LevelTabTileWidget tabTileWidget = LevelTabTileWidget(this);
    LevelTabSubtileWidget tabSubtileWidget = LevelTabSubtileWidget(this);
    LevelTabFrameWidget tabFrameWidget = LevelTabFrameWidget(this);
    DungeonSubtileWidget *dungeonSubtileWidget = nullptr;
    PushButtonWidget *viewBtn;

    TilesetLightDialog tilesetLightDialog = TilesetLightDialog(this);
    DungeonGenerateDialog dungeonGenerateDialog = DungeonGenerateDialog(this);
    DungeonDecorateDialog dungeonDecorateDialog = DungeonDecorateDialog(this);
    DungeonResourceDialog dungeonResourceDialog = DungeonResourceDialog(this);
    DungeonSearchDialog dungeonSearchDialog = DungeonSearchDialog(this);

    D1Pal *pal;
    D1Gfx *gfx;
    D1Tileset *tileset;
    D1Gfx *cls;
    D1Min *min;
    D1Til *til;
    D1Sla *sla;
    D1Tla *tla;
    D1Dun *dun;
    bool dunView = false;
    bool isScrolling = false;
    int lastHorizScrollValue = 0;
    int lastVertScrollValue = 0;
    int currentFrameIndex = 0;
    int currentSubtileIndex = 0;
    int currentTileIndex = 0;
    unsigned tilesetPlayDelay = 50000; // microsec
    unsigned dunviewPlayDelay = 50000; // microsec
    int currentDunPosX = 0;
    int currentDunPosY = 0;
    int playTimer = 0;
    unsigned playCounter = 0;
};
