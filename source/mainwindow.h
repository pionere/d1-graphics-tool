#pragma once

#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMimeData>
#include <QPair>
#include <QStringList>
#include <QTranslator>
#include <QUndoCommand>

#include "d1hro.h"
#include "d1pal.h"
#include "d1trn.h"
#include "heroview.h"
#include "itemselectordialog.h"
#include "openasdialog.h"
#include "palettewidget.h"
#include "progressdialog.h"
#include "saveasdialog.h"
#include "settingsdialog.h"
#include "sidepanelwidget.h"

#define D1_HERO_PLANNER_TITLE "Diablo 1 Hero Planner"
#define D1_HERO_PLANNER_VERSION "0.5.0"

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
    HRO,
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
    QString baseDir;
    D1Pal *pal;
    D1Trn *trnUnique;
    D1Trn *trnBase;
    D1Hero *hero;
    QMap<QString, D1Pal *> pals;
} LoadFileContent;

struct ItemStruct;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow();
    ~MainWindow();

    friend MainWindow &dMainWindow();

    void reloadConfig();
    void updateWindow();

    void openArgFile(const char *arg);
    void openNew(OPEN_HERO_CLASS heroClass);
    void openFile(const OpenAsParam &params);
    void openFiles(const QStringList &filePaths);
    void openPalFiles(const QStringList &filePaths, PaletteWidget *widget);
    void saveFile(const SaveAsParam &params);
    void updateTrns(const std::vector<D1Trn *> &newTrns);

    void heroChanged(D1Hero *hero);
    void heroSkillsClicked();
    void heroMonstersClicked();
    void heroPvPClicked();
    void heroItemClicked(int ii);
    void selectHeroItem(int ii);
    void addHeroItem(int ii, ItemStruct *is);
    void paletteWidget_callback(PaletteWidget *widget, PWIDGET_CALLBACK_TYPE type);
    void updatePalette(const D1Pal* pal);
    void colorModified();
    void pointHovered(const QPoint &pos);

    void initPaletteCycle();
    void nextPaletteCycle(D1PAL_CYCLE_TYPE type);
    void resetPaletteCycle();

    QString fileDialog(FILE_DIALOG_MODE mode, const QString &title, const QString &filter);
    QStringList filesDialog(const QString &title, const QString &filter);
    QString folderDialog(const QString &title);

    static bool hasHeroUrl(const QMimeData *mimeData);
    static bool isResourcePath(const QString &path);
    static void supportedImageFormats(QStringList &allSupportedImageFormats);
    static QString FileContentTypeTxt(FILE_CONTENT fileType);

private:
    static void loadFile(const OpenAsParam &params, MainWindow *instance, LoadFileContent *result);
    static void failWithError(MainWindow *instance, LoadFileContent *result, const QString &error);

    void setPal(const QString &palFilePath);
    void setUniqueTrn(const QString &trnfilePath);
    void setBaseTrn(const QString &trnfilePath);

    bool loadPal(const QString &palFilePath);
    bool loadUniqueTrn(const QString &trnfilePath);
    bool loadBaseTrn(const QString &trnfilePath);

public slots:
    void on_actionToggle_View_triggered();

private slots:
    void on_actionNew_Warrior_triggered();
    void on_actionNew_Rogue_triggered();
    void on_actionNew_Sorcerer_triggered();
    void on_actionNew_Monk_triggered();
    void on_actionNew_Bard_triggered();
    void on_actionNew_Barbarian_triggered();

    void on_actionOpen_triggered();
    void on_actionOpenAs_triggered();
    void on_actionLoad_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionClose_triggered();
    void on_actionDiff_triggered();
    void on_actionSettings_triggered();
    void on_actionQuit_triggered();

    void on_actionTogglePalTrn_triggered();
    void on_actionToggleBottomPanel_triggered();

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

    HeroView *heroView = nullptr;
    SidePanelWidget *sideView = nullptr;

    PaletteWidget *palWidget = nullptr;
    PaletteWidget *trnUniqueWidget = nullptr;
    PaletteWidget *trnBaseWidget = nullptr;

    ProgressDialog progressDialog = ProgressDialog(this);
    ProgressWidget progressWidget = ProgressWidget(this);
    OpenAsDialog *openAsDialog = nullptr;
    SaveAsDialog *saveAsDialog = nullptr;
    SettingsDialog *settingsDialog = nullptr;
    ItemSelectorDialog *itemSelectorDialog = nullptr;

    D1Pal *pal = nullptr;
    D1Trn *trnUnique = nullptr;
    D1Trn *trnBase = nullptr;
    D1Hero *hero = nullptr;

    QMap<QString, D1Pal *> pals;       // key: path, value: pointer to palette
    QMap<QString, D1Trn *> uniqueTrns; // key: path, value: pointer to translation
    QMap<QString, D1Trn *> baseTrns;   // key: path, value: pointer to translation

    // buffer to store the original colors in case of color cycling
    QColor origCyclePalette[32];
    // state of the panels visibility
    bool bottomPanelHidden = false;
};

MainWindow &dMainWindow();
