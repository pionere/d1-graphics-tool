#pragma once

#include <QMainWindow>
#include <QMenu>
#include <QMimeData>
#include <QStringList>
#include <QUndoCommand>

#include "celview.h"
#include "d1gfx.h"
#include "d1min.h"
#include "d1pal.h"
#include "d1palhits.h"
#include "d1sol.h"
#include "d1til.h"
#include "d1tmi.h"
#include "d1trn.h"
#include "exportdialog.h"
#include "levelcelview.h"
#include "openasdialog.h"
#include "palettewidget.h"
#include "saveasdialog.h"
#include "settingsdialog.h"
#include "upscaledialog.h"

#define D1_GRAPHICS_TOOL_TITLE "Diablo 1 Graphics Tool"
#define D1_GRAPHICS_TOOL_VERSION "0.5.0"

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

    void openFile(const OpenAsParam &params);
    void openImageFiles(IMAGE_FILE_MODE mode, QStringList filePaths, bool append);
    void openPalFiles(QStringList filePaths, PaletteWidget *widget);
    void saveFile(const SaveAsParam &params);
    void upscale(const UpscaleParam &params);

    void paletteWidget_callback(PaletteWidget *widget, PWIDGET_CALLBACK_TYPE type);

    void initPaletteCycle();
    void nextPaletteCycle(D1PAL_CYCLE_TYPE type);
    void resetPaletteCycle();

    QString getLastFilePath();
    QString fileDialog(FILE_DIALOG_MODE mode, const char *title, const char *filter);
    QStringList filesDialog(const char *title, const char *filter);

    static bool hasImageUrl(const QMimeData *mimeData);

private:
    void updateWindow();

    void setPal(QString palFilePath);
    void setUniqueTrn(QString trnfilePath);
    void setBaseTrn(QString trnfilePath);

    bool loadPal(QString palFilePath);
    bool loadUniqueTrn(QString trnfilePath);
    bool loadBaseTrn(QString trnfilePath);

    void pixelClicked(const D1GfxPixel &pixel);
    void colorModified();
    void reloadConfig();

    void addFrames(bool append);
    void addSubtiles(bool append);
    void addTiles(bool append);

public slots:
    void on_actionInsert_Frame_triggered();
    void on_actionAdd_Frame_triggered();
    void on_actionReplace_Frame_triggered();
    void on_actionDel_Frame_triggered();

    void on_actionCreate_Subtile_triggered();
    void on_actionInsert_Subtile_triggered();
    void on_actionAdd_Subtile_triggered();
    void on_actionReplace_Subtile_triggered();
    void on_actionDel_Subtile_triggered();

    void on_actionCreate_Tile_triggered();
    void on_actionInsert_Tile_triggered();
    void on_actionAdd_Tile_triggered();
    void on_actionReplace_Tile_triggered();
    void on_actionDel_Tile_triggered();

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
    void on_actionResetFrameTypes_Tileset_triggered();
    void on_actionInefficientFrames_Tileset_triggered();
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

    void on_actionAbout_triggered();
    void on_actionAbout_Qt_triggered();

    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

private:
    Ui::MainWindow *ui;
    QString lastFilePath;

    QMenu newMenu = QMenu("New");
    QMenu frameMenu = QMenu("Frame");
    QMenu subtileMenu = QMenu("Subtile");
    QMenu tileMenu = QMenu("Tile");

    QUndoStack *undoStack;
    QAction *undoAction;
    QAction *redoAction;

    QPointer<CelView> celView;
    QPointer<LevelCelView> levelCelView;

    QPointer<PaletteWidget> palWidget;
    QPointer<PaletteWidget> trnUniqueWidget;
    QPointer<PaletteWidget> trnBaseWidget;

    OpenAsDialog openAsDialog = OpenAsDialog(this);
    SaveAsDialog saveAsDialog = SaveAsDialog(this);
    SettingsDialog settingsDialog = SettingsDialog(this);
    ExportDialog exportDialog = ExportDialog(this);
    UpscaleDialog upscaleDialog = UpscaleDialog(this);

    QPointer<D1Pal> pal;
    QPointer<D1Trn> trnUnique;
    QPointer<D1Trn> trnBase;
    QPointer<D1Gfx> gfx;
    QPointer<D1Min> min;
    QPointer<D1Til> til;
    QPointer<D1Sol> sol;
    QPointer<D1Amp> amp;
    QPointer<D1Tmi> tmi;

    QMap<QString, D1Pal *> pals;       // key: path, value: pointer to palette
    QMap<QString, D1Trn *> uniqueTrns; // key: path, value: pointer to translation
    QMap<QString, D1Trn *> baseTrns;   // key: path, value: pointer to translation

    // Palette hits are instantiated in main window to make them available to the three PaletteWidgets
    QPointer<D1PalHits> palHits;
    // buffer to store the original colors in case of color cycling
    QColor origCyclePalette[32];
};
