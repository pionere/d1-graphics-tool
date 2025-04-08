#pragma once

#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMimeData>
#include <QPair>
#include <QStringList>
#include <QTranslator>
#include <QUndoCommand>

#include "builderwidget.h"
#include "celview.h"
#include "cppview.h"
#include "d1cpp.h"
#include "d1dun.h"
#include "d1gfx.h"
#include "d1gfxset.h"
#include "d1pal.h"
#include "d1palhits.h"
#include "d1tableset.h"
#include "d1tileset.h"
#include "d1trn.h"
#include "exportdialog.h"
#include "gfxsetview.h"
#include "importdialog.h"
#include "levelcelview.h"
#include "mergeframesdialog.h"
#include "openasdialog.h"
#include "paintwidget.h"
#include "paletteshowdialog.h"
#include "palettewidget.h"
#include "patchdungeondialog.h"
#include "patchgfxdialog.h"
#include "patchtilesetdialog.h"
#include "progressdialog.h"
#include "remapdialog.h"
#include "resizedialog.h"
#include "trngeneratedialog.h"
#include "saveasdialog.h"
#include "settingsdialog.h"
#include "tblview.h"
#include "upscaledialog.h"
#include "upscaletaskdialog.h"

#define D1_GRAPHICS_TOOL_TITLE "Diablo 1 Graphics Tool"
#define D1_GRAPHICS_TOOL_VERSION "0.5.0"

#define MemFree(p)   \
    {                \
        delete p;    \
        p = nullptr; \
    }

enum class FILE_DIALOG_MODE {
    OPEN,         // open existing
    SAVE_CONF,    // save with confirm
    SAVE_NO_CONF, // save without confirm
};

enum class IMAGE_FILE_MODE {
    FRAME,   // open as frames
    SUBTILE, // open as subtiles
    TILE,    // open as tiles
    AUTO,    // auto-detect
};

namespace Ui {
class MainWindow;
}

enum class FILE_CONTENT {
    EMPTY,
    CEL,
    CL2,
    PCX,
    TBL,
    CPP,
    SMK,
    DUN,
    UNKNOWN = -1
};

typedef struct LoadFileContent
{
    FILE_CONTENT fileType;
    bool isTileset;
    bool isGfxset;
    QString baseDir;
    D1Pal *pal;
    D1Trn *trnUnique;
    D1Trn *trnBase;
    D1Gfx *gfx;
    D1Tileset *tileset;
    D1Gfxset *gfxset;
    D1Dun *dun;
    D1Tableset *tableset;
    D1Cpp *cpp;
    QMap<QString, D1Pal *> pals;
} LoadFileContent;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow();
    ~MainWindow();

    friend MainWindow &dMainWindow();

    void reloadConfig();
    void updateWindow();

    void openArgFile(const char *arg);
    void openNew(OPEN_GFX_TYPE gfxType, OPEN_CLIPPED_TYPE clipped, bool createDun);
    void openFile(const OpenAsParam &params);
    void openFiles(const QStringList &filePaths);
    void openImageFiles(IMAGE_FILE_MODE mode, QStringList filePaths, bool append);
    void openPalFiles(const QStringList &filePaths, PaletteWidget *widget);
    void importFile(const ImportParam &params);
    void saveFile(const SaveAsParam &params);
    void updateTrns(const std::vector<D1Trn *> &newTrns);
    void resize(const ResizeParam &params);
    void upscale(const UpscaleParam &params);
    void mergeFrames(const MergeFramesParam &params);

    void gfxChanged(D1Gfx *gfx);
    void paletteWidget_callback(PaletteWidget *widget, PWIDGET_CALLBACK_TYPE type);
    void updatePalette(const D1Pal* pal);
    void remapColors(const RemapParam &params);
    void colorModified();
    void frameClicked(D1GfxFrame *frame, const QPoint &pos, int flags);
    void pointHovered(const QPoint &pos);
    void dunClicked(const QPoint &cell, int flags);
    void dunHovered(const QPoint &cell);
    int getDunBuilderMode() const;
    void frameModified(D1GfxFrame *frame);

    void initPaletteCycle();
    void nextPaletteCycle(D1PAL_CYCLE_TYPE type);
    void resetPaletteCycle();

    QString fileDialog(FILE_DIALOG_MODE mode, const QString &title, const QString &filter);
    QStringList filesDialog(const QString &title, const QString &filter);
    QString folderDialog(const QString &title);

    static bool hasImageUrl(const QMimeData *mimeData);
    static bool isResourcePath(const QString &path);
    static void supportedImageFormats(QStringList &allSupportedImageFormats);
    static QString FileContentTypeToStr(FILE_CONTENT fileType);

private:
    static IMPORT_FILE_TYPE guessFileType(const QString& filePath, bool dunMode);
    static void loadFile(const OpenAsParam &params, MainWindow *instance, LoadFileContent *result);
    static void failWithError(MainWindow *instance, LoadFileContent *result, const QString &error);

    void setPal(const QString &palFilePath);
    void setUniqueTrn(const QString &trnfilePath);
    void setBaseTrn(const QString &trnfilePath);

    bool loadPal(const QString &palFilePath);
    bool loadUniqueTrn(const QString &trnfilePath);
    bool loadBaseTrn(const QString &trnfilePath);
    D1Dun *loadDun(const QString &title);

    void addFrames(bool append);
    void addSubtiles(bool append);
    void addTiles(bool append);

public slots:
    void on_actionMerge_Frame_triggered();
    void on_actionAddTo_Frame_triggered();
    void on_actionCreate_Frame_triggered();
    void on_actionInsert_Frame_triggered();
    void on_actionDuplicate_Frame_triggered();
    void on_actionReplace_Frame_triggered();
    void on_actionDel_Frame_triggered();

    void on_actionCreate_Subtile_triggered();
    void on_actionInsert_Subtile_triggered();
    void on_actionDuplicate_Subtile_triggered();
    void on_actionReplace_Subtile_triggered();
    void on_actionDel_Subtile_triggered();

    void on_actionCreate_Tile_triggered();
    void on_actionInsert_Tile_triggered();
    void on_actionDuplicate_Tile_triggered();
    void on_actionReplace_Tile_triggered();
    void on_actionDel_Tile_triggered();

    void on_actionToggle_View_triggered();
    void on_actionToggle_Painter_triggered();
    void on_actionToggle_Builder_triggered();

private slots:
    void on_actionNew_CEL_triggered();
    void on_actionNew_CL2_triggered();
    void on_actionNew_Tileset_triggered();
    void on_actionNew_Gfxset_triggered();
    void on_actionNew_Dungeon_triggered();

    void on_actionOpen_triggered();
    void on_actionOpenAs_triggered();
    void on_actionImport_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionClose_triggered();
    void on_actionExport_triggered();
    void on_actionDiff_triggered();
    void on_actionSettings_triggered();
    void on_actionQuit_triggered();

    void on_actionTogglePalTrn_triggered();
    void on_actionToggleBottomPanel_triggered();

    void on_actionPatch_triggered();
    void on_actionResize_triggered();
    void on_actionUpscale_triggered();
    void on_actionMerge_triggered();

    void on_actionReportUse_Tileset_triggered();
    void on_actionReportActiveSubtiles_Tileset_triggered();
    void on_actionReportActiveTiles_Tileset_triggered();
    void on_actionReportInefficientFrames_Tileset_triggered();
    void on_actionResetFrameTypes_Tileset_triggered();
    void on_actionPatchTileset_Tileset_triggered();
    void on_actionLightSubtiles_Tileset_triggered();
    void on_actionCheckSubtileFlags_Tileset_triggered();
    void on_actionCheckTileFlags_Tileset_triggered();
    void on_actionCheckTilesetFlags_Tileset_triggered();
    void on_actionCleanupFrames_Tileset_triggered();
    void on_actionCleanupSubtiles_Tileset_triggered();
    void on_actionCleanupTileset_Tileset_triggered();
    void on_actionCompressSubtiles_Tileset_triggered();
    void on_actionCompressTiles_Tileset_triggered();
    void on_actionCompressTileset_Tileset_triggered();
    void on_actionSortFrames_Tileset_triggered();
    void on_actionSortSubtiles_Tileset_triggered();
    void on_actionSortTileset_Tileset_triggered();

    void on_actionReportUse_Dungeon_triggered();
    void on_actionPatchDungeon_Dungeon_triggered();
    void on_actionResetTiles_Dungeon_triggered();
    void on_actionResetSubtiles_Dungeon_triggered();
    void on_actionMaskTiles_Dungeon_triggered();
    void on_actionProtectTiles_Dungeon_triggered();
    void on_actionProtectTilesFrom_Dungeon_triggered();
    void on_actionProtectSubtiles_Dungeon_triggered();
    void on_actionCheckTiles_Dungeon_triggered();
    void on_actionCheckProtections_Dungeon_triggered();
    void on_actionCheckItems_Dungeon_triggered();
    void on_actionCheckMonsters_Dungeon_triggered();
    void on_actionCheckObjects_Dungeon_triggered();
    void on_actionCheckEntities_Dungeon_triggered();
    void on_actionRemoveTiles_Dungeon_triggered();
    void on_actionLoadTiles_Dungeon_triggered();
    void on_actionRemoveProtections_Dungeon_triggered();
    void on_actionLoadProtections_Dungeon_triggered();
    void on_actionRemoveItems_Dungeon_triggered();
    void on_actionLoadItems_Dungeon_triggered();
    void on_actionRemoveMonsters_Dungeon_triggered();
    void on_actionLoadMonsters_Dungeon_triggered();
    void on_actionRemoveObjects_Dungeon_triggered();
    void on_actionLoadObjects_Dungeon_triggered();
    void on_actionGenerate_Dungeon_triggered();
    void on_actionSearch_Dungeon_triggered();

    void on_actionNew_PAL_triggered();
    void on_actionOpen_PAL_triggered();
    void on_actionSave_PAL_triggered();
    void on_actionSave_PAL_as_triggered();
    void on_actionClose_PAL_triggered();

    void on_actionNew_Translation_Unique_triggered();
    void on_actionOpen_Translation_Unique_triggered();
    void on_actionSave_Translation_Unique_triggered();
    void on_actionSave_Translation_Unique_as_triggered();
    void on_actionClose_Translation_Unique_triggered();
    void on_actionPatch_Translation_Unique_triggered();

    void on_actionNew_Translation_Base_triggered();
    void on_actionOpen_Translation_Base_triggered();
    void on_actionSave_Translation_Base_triggered();
    void on_actionSave_Translation_Base_as_triggered();
    void on_actionClose_Translation_Base_triggered();
    void on_actionPatch_Translation_Base_triggered();

    void on_actionDisplay_Colors_triggered();
    void on_actionRemap_Colors_triggered();
    void on_actionSmack_Colors_triggered();
    void on_actionGenTrns_Colors_triggered();
    void on_actionLoadTrns_Colors_triggered();
    void on_actionSaveTrns_Colors_triggered();

    void on_actionAddColumn_Table_triggered();
    void on_actionInsertColumn_Table_triggered();
    void on_actionDuplicateColumn_Table_triggered();
    void on_actionDelColumn_Table_triggered();
    void on_actionHideColumn_Table_triggered();
    void on_actionMoveLeftColumn_Table_triggered();
    void on_actionMoveRightColumn_Table_triggered();
    void on_actionDelColumns_Table_triggered();
    void on_actionHideColumns_Table_triggered();
    void on_actionShowColumns_Table_triggered();
    void on_actionAddRow_Table_triggered();
    void on_actionInsertRow_Table_triggered();
    void on_actionDuplicateRow_Table_triggered();
    void on_actionDelRow_Table_triggered();
    void on_actionHideRow_Table_triggered();
    void on_actionMoveUpRow_Table_triggered();
    void on_actionMoveDownRow_Table_triggered();
    void on_actionDelRows_Table_triggered();
    void on_actionHideRows_Table_triggered();
    void on_actionShowRows_Table_triggered();

    void on_actionUpscaleTask_triggered();

    void on_actionAbout_triggered();
    void on_actionAbout_Qt_triggered();

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::MainWindow *ui;
    QTranslator translator;   // translations for this application
    QTranslator translatorQt; // translations for qt
    QString currLang;         // currently loaded language e.g. "de_DE"
    QString lastFilePath;

    QUndoStack *undoStack;
    QAction *undoAction;
    QAction *redoAction;

    CelView *celView = nullptr;
    LevelCelView *levelCelView = nullptr;
    GfxsetView *gfxsetView = nullptr;
    TblView *tblView = nullptr;
    CppView *cppView = nullptr;
    PaintWidget *paintWidget = nullptr;
    BuilderWidget *builderWidget = nullptr;

    PaletteWidget *palWidget = nullptr;
    PaletteWidget *trnUniqueWidget = nullptr;
    PaletteWidget *trnBaseWidget = nullptr;

    ProgressDialog progressDialog = ProgressDialog(this);
    ProgressWidget progressWidget = ProgressWidget(this);
    OpenAsDialog *openAsDialog = nullptr;
    SaveAsDialog *saveAsDialog = nullptr;
    SettingsDialog *settingsDialog = nullptr;
    ImportDialog *importDialog = nullptr;
    ExportDialog *exportDialog = nullptr;
    ResizeDialog *resizeDialog = nullptr;
    UpscaleDialog *upscaleDialog = nullptr;
    MergeFramesDialog *mergeFramesDialog = nullptr;
    PatchDungeonDialog *patchDungeonDialog = nullptr;
    PatchGfxDialog *patchGfxDialog = nullptr;
    PatchTilesetDialog *patchTilesetDialog = nullptr;
    TrnGenerateDialog *trnGenerateDialog = nullptr;
    RemapDialog *remapDialog = nullptr;
    PaletteShowDialog *paletteShowDialog = nullptr;
    UpscaleTaskDialog *upscaleTaskDialog = nullptr;

    D1Pal *pal = nullptr;
    D1Trn *trnUnique = nullptr;
    D1Trn *trnBase = nullptr;
    D1Gfx *gfx = nullptr;
    D1Tileset *tileset = nullptr;
    D1Gfxset *gfxset = nullptr;
    D1Dun *dun = nullptr;
    D1Tableset *tableset = nullptr;
    D1Cpp *cpp = nullptr;

    QMap<QString, D1Pal *> pals;       // key: path, value: pointer to palette
    QMap<QString, D1Trn *> uniqueTrns; // key: path, value: pointer to translation
    QMap<QString, D1Trn *> baseTrns;   // key: path, value: pointer to translation

    // Palette hits are instantiated in main window to make them available to the three PaletteWidgets
    D1PalHits *palHits = nullptr;
    // buffer to store the original colors in case of color cycling
    QColor origCyclePalette[32];
    // state of the panels visibility
    bool bottomPanelHidden = false;
};

MainWindow &dMainWindow();
