#pragma once

#include <QMainWindow>
#include <QMenu>
#include <QMimeData>
#include <QStringList>
#include <QTranslator>
#include <QUndoCommand>

#include "celview.h"
#include "d1gfx.h"
#include "d1pal.h"
#include "d1palhits.h"
#include "d1tileset.h"
#include "d1trn.h"
#include "exportdialog.h"
#include "levelcelview.h"
#include "openasdialog.h"
#include "paintdialog.h"
#include "palettewidget.h"
#include "patchtilesetdialog.h"
#include "progressdialog.h"
#include "saveasdialog.h"
#include "settingsdialog.h"
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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow();
    ~MainWindow();

    friend MainWindow &dMainWindow();

    void reloadConfig();
    void updateWindow();

    void openFile(const OpenAsParam &params);
    void openImageFiles(IMAGE_FILE_MODE mode, QStringList filePaths, bool append);
    void openPalFiles(const QStringList &filePaths, PaletteWidget *widget);
    void saveFile(const SaveAsParam &params);
    void upscale(const UpscaleParam &params);

    void paletteWidget_callback(PaletteWidget *widget, PWIDGET_CALLBACK_TYPE type);
    void changeColor(const std::vector<std::pair<D1GfxPixel, D1GfxPixel>> &replacements, bool all);
    void frameClicked(D1GfxFrame *frame, const QPoint &pos, unsigned counter);
    void frameModified();

    void initPaletteCycle();
    void nextPaletteCycle(D1PAL_CYCLE_TYPE type);
    void resetPaletteCycle();

    QString fileDialog(FILE_DIALOG_MODE mode, const QString &title, const QString &filter);
    QStringList filesDialog(const QString &title, const QString &filter);
    QString folderDialog(const QString &title);

    static bool hasImageUrl(const QMimeData *mimeData);
    static bool isResourcePath(const QString &path);

private:
    void failWithError(const QString &error);

    void setPal(const QString &palFilePath);
    void setUniqueTrn(const QString &trnfilePath);
    void setBaseTrn(const QString &trnfilePath);

    bool loadPal(const QString &palFilePath);
    bool loadUniqueTrn(const QString &trnfilePath);
    bool loadBaseTrn(const QString &trnfilePath);

    void colorModified();

    void addFrames(bool append);
    void addSubtiles(bool append);
    void addTiles(bool append);

public slots:
    void on_actionAddTo_Frame_triggered();
    void on_actionInsert_Frame_triggered();
    void on_actionAppend_Frame_triggered();
    void on_actionReplace_Frame_triggered();
    void on_actionDel_Frame_triggered();

    void on_actionCreate_Subtile_triggered();
    void on_actionInsert_Subtile_triggered();
    void on_actionAppend_Subtile_triggered();
    void on_actionReplace_Subtile_triggered();
    void on_actionDel_Subtile_triggered();

    void on_actionCreate_Tile_triggered();
    void on_actionInsert_Tile_triggered();
    void on_actionAppend_Tile_triggered();
    void on_actionReplace_Tile_triggered();
    void on_actionDel_Tile_triggered();

    void on_actionStart_Draw_triggered();
    // void on_actionStop_Draw_triggered();

private slots:
    void on_actionNew_CEL_triggered();
    void on_actionNew_CL2_triggered();
    void on_actionNew_Tileset_triggered();

    void on_actionOpen_triggered();
    void on_actionOpenAs_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionClose_triggered();
    void on_actionExport_triggered();
    void on_actionSettings_triggered();
    void on_actionQuit_triggered();

    void on_actionUpscale_triggered();

    void on_actionReportUse_Tileset_triggered();
    void on_actionInefficientFrames_Tileset_triggered();
    void on_actionResetFrameTypes_Tileset_triggered();
    void on_actionPatchTileset_Tileset_triggered();
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

    void on_actionNew_Translation_Base_triggered();
    void on_actionOpen_Translation_Base_triggered();
    void on_actionSave_Translation_Base_triggered();
    void on_actionSave_Translation_Base_as_triggered();
    void on_actionClose_Translation_Base_triggered();

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

    QMenu newMenu = QMenu("New");
    QMenu frameMenu = QMenu("Frame");
    QMenu subtileMenu = QMenu("Subtile");
    QMenu tileMenu = QMenu("Tile");

    QAction *upscaleAction;

    QUndoStack *undoStack;
    QAction *undoAction;
    QAction *redoAction;

    CelView *celView = nullptr;
    LevelCelView *levelCelView = nullptr;
    PaintDialog *paintDialog = nullptr;

    PaletteWidget *palWidget = nullptr;
    PaletteWidget *trnUniqueWidget = nullptr;
    PaletteWidget *trnBaseWidget = nullptr;

    ProgressDialog progressDialog = ProgressDialog(this);
    ProgressWidget progressWidget = ProgressWidget(this);
    OpenAsDialog *openAsDialog = nullptr;
    SaveAsDialog *saveAsDialog = nullptr;
    SettingsDialog *settingsDialog = nullptr;
    ExportDialog *exportDialog = nullptr;
    UpscaleDialog *upscaleDialog = nullptr;
    PatchTilesetDialog *patchTilesetDialog = nullptr;
    UpscaleTaskDialog *upscaleTaskDialog = nullptr;

    D1Pal *pal = nullptr;
    D1Trn *trnUnique = nullptr;
    D1Trn *trnBase = nullptr;
    D1Gfx *gfx = nullptr;
    D1Tileset *tileset = nullptr;

    QMap<QString, D1Pal *> pals;       // key: path, value: pointer to palette
    QMap<QString, D1Trn *> uniqueTrns; // key: path, value: pointer to translation
    QMap<QString, D1Trn *> baseTrns;   // key: path, value: pointer to translation

    // Palette hits are instantiated in main window to make them available to the three PaletteWidgets
    D1PalHits *palHits = nullptr;
    // buffer to store the original colors in case of color cycling
    QColor origCyclePalette[32];
};

MainWindow &dMainWindow();
