#include "upscaletaskdialog.h"

#include <set>
#include <vector>

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

#include "config.h"
#include "d1cel.h"
#include "d1celtileset.h"
#include "d1cl2.h"
#include "d1gfx.h"
#include "d1min.h"
#include "d1pal.h"
#include "d1tileset.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_upscaletaskdialog.h"
#include "upscaler.h"

template <class T, int N>
constexpr int lengthof(T (&arr)[N])
{
    return N;
}

typedef struct AssetConfig {
    const QString path;
    const QString palette;
    int numcolors;
    int fixcolors;
} AssetConfig;

typedef struct MinAssetConfig {
    const QString path;    // CEL-path
    const QString palette; // path to the palette
    int numcolors;         // number of usable colors
    int fixcolors;         // fix (protected) colors at the beginning of the palette
    int dunType;           // dungeon_type
} MinAssetConfig;

UpscaleTaskDialog::UpscaleTaskDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UpscaleTaskDialog())
{
    this->ui->setupUi(this);

    // adjust the list-widget to fit its content
    QListWidget *listWidget = this->ui->skipStepListWidget;
    listWidget->setMinimumHeight(listWidget->sizeHintForRow(0) * NUM_STEPS + 2 * listWidget->frameWidth());
}

UpscaleTaskDialog::~UpscaleTaskDialog()
{
    delete ui;
}

void UpscaleTaskDialog::on_listfilesFileBrowseButton_clicked()
{
    QString filePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select listfiles.txt"), tr("TXT Files (*.txt *.TXT)"));

    if (filePath.isEmpty())
        return;

    this->ui->listfilesFileLineEdit->setText(filePath);
}

void UpscaleTaskDialog::on_selectAllButton_clicked()
{
    this->ui->skipStepListWidget->selectAll();
}

void UpscaleTaskDialog::on_deselectAllButton_clicked()
{
    this->ui->skipStepListWidget->clearSelection();
}

void UpscaleTaskDialog::on_assetsFolderBrowseButton_clicked()
{
    QString dirPath = dMainWindow().folderDialog(tr("Select Assets Folder"));

    if (dirPath.isEmpty())
        return;

    this->ui->assetsFolderLineEdit->setText(dirPath);
}

void UpscaleTaskDialog::on_outputFolderBrowseButton_clicked()
{
    QString dirPath = dMainWindow().folderDialog(tr("Select Output Folder"));

    if (dirPath.isEmpty())
        return;

    this->ui->outputFolderLineEdit->setText(dirPath);
}

static bool skipStep(const QList<QModelIndex> &skipSteps, int index)
{
    for (auto item : skipSteps) {
        if (item.row() == index) {
            return true;
        }
    }
    return false;
}

void UpscaleTaskDialog::on_upscaleButton_clicked()
{
    UpscaleTaskParam params;
    params.multiplier = this->ui->multiplierLineEdit->text().toInt();
    if (params.multiplier <= 1) {
        this->close();
        return;
    }
    params.listfilesFile = this->ui->listfilesFileLineEdit->text();
    if (params.listfilesFile.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Path of listfiles.txt is missing, please choose an output file."));
        return;
    }
    params.assetsFolder = this->ui->assetsFolderLineEdit->text();
    if (params.assetsFolder.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Assets folder is missing, please set the assets folder."));
        return;
    }
    params.outFolder = this->ui->outputFolderLineEdit->text();
    if (params.outFolder.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Output folder is missing, please choose an output folder."));
        return;
    }
    if (params.assetsFolder == params.outFolder) {
        QMessageBox::warning(this, tr("Warning"), tr("The assets folder must differ from output folder."));
        return;
    }
    {
        QList<QModelIndex> skipSteps = this->ui->skipStepListWidget->selectionModel()->selectedIndexes();
        for (int i = 0; i < NUM_STEPS; ++i) {
            params.steps[i] = !skipStep(skipSteps, i);
        }
    }
    params.patchGraphics = this->ui->patchGraphicsCheckBox->isChecked();
    params.patchTilesets = this->ui->patchTilesetsCheckBox->isChecked();

    this->close();

    /*ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Upscaling assets..."), 3, PAF_NONE);

    UpscaleTaskDialog::runTask(params);

    ProgressDialog::done();*/
    std::function<void()> func = [params]() {
        UpscaleTaskDialog::runTask(params);
    };
    ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::ACTIVE, tr("Upscaling assets..."), 3, PAF_NONE, std::move(func));
}

void UpscaleTaskDialog::on_upscaleCancelButton_clicked()
{
    this->close();
}

void UpscaleTaskDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}

bool UpscaleTaskDialog::loadCustomPal(const QString &path, int numcolors, int fixcolors, const UpscaleTaskParam &params, D1Pal &pal, UpscaleParam &upParams)
{
    QString palPath = params.assetsFolder + "/" + path; // "f:\\MPQE\\Work\\%s"

    if (!pal.load(palPath)) {
        dProgressErr() << QApplication::tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(palPath));
        return false;
    }

    if (fixcolors != 0) {
        upParams.firstfixcolor = 1;
        upParams.lastfixcolor = fixcolors - 1;
    } else {
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
    }
    QColor undefColor = QColor(Config::getPaletteUndefinedColor());
    for (int n = numcolors; n < D1PAL_COLORS; n++) {
        pal.setColor(n, undefColor);
    }
    return true;
}

void UpscaleTaskDialog::upscaleCel(const QString &path, D1Pal *pal, const UpscaleTaskParam &params, const OpenAsParam &opParams, const UpscaleParam &upParams, SaveAsParam &saParams)
{
    QString celFilePath = params.assetsFolder + "/" + path; // "f:\\MPQE\\Work\\%s"
    QString outFilePath = params.outFolder + "/" + path;    // "f:\\outcel\\%s"

    // Loading CEL
    D1Gfx gfx;
    gfx.setPalette(pal);
    if (!D1Cel::load(gfx, celFilePath, opParams)) {
        dProgressErr() << QApplication::tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(celFilePath));
        return;
    }
    // Patch CEL if requested
    if (params.patchGraphics) {
        int fileIndex = D1Gfx::getPatchFileIndex(celFilePath);
        if (fileIndex >= 0) {
            gfx.patch(fileIndex, true);
        }
    }
    // upscale
    if (Upscaler::upscaleGfx(&gfx, upParams, true)) {
        // store the result
        saParams.celFilePath = outFilePath;
        D1Cel::save(gfx, saParams);
    }
}

void UpscaleTaskDialog::upscaleCl2(const QString &path, D1Pal *pal, const UpscaleTaskParam &params, const OpenAsParam &opParams, const UpscaleParam &upParams, SaveAsParam &saParams)
{
    QString cl2FilePath = params.assetsFolder + "/" + path; // "f:\\MPQE\\Work\\%s"
    QString outFilePath = params.outFolder + "/" + path;    // "f:\\outcel\\%s"

    // Loading CL2
    D1Gfx gfx;
    gfx.setPalette(pal);
    if (!D1Cl2::load(gfx, cl2FilePath, opParams)) {
        dProgressErr() << QApplication::tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(cl2FilePath));
        return;
    }
    // Patch CL2 if requested
    if (params.patchGraphics) {
        int fileIndex = D1Gfx::getPatchFileIndex(cl2FilePath);
        if (fileIndex >= 0) {
            gfx.patch(fileIndex, true);
        }
    }
    // upscale
    if (Upscaler::upscaleGfx(&gfx, upParams, true)) {
        // store the result
        saParams.celFilePath = outFilePath;
        D1Cl2::save(gfx, saParams);
    }
}

void UpscaleTaskDialog::upscaleMin(D1Pal *pal, const UpscaleTaskParam &params, OpenAsParam &opParams, const UpscaleParam &upParams, SaveAsParam &saParams, int dunType)
{
    QString celFilePath = params.assetsFolder + "/" + opParams.celFilePath;
    QString baseOutPath = params.outFolder + "/" + opParams.celFilePath;
    baseOutPath.chop(4);

    D1Gfx gfx = D1Gfx();
    gfx.setPalette(pal);
    D1Tileset tileset = D1Tileset(&gfx);
    opParams.celFilePath = celFilePath;
    if (!tileset.load(opParams)) {
        return;
    }
    // Patch MIN if requested
    if (params.patchTilesets) {
        tileset.patch(dunType, true);
    }
    // upscale
    // - upscale the tileset graphics
    if (Upscaler::upscaleTileset(tileset.gfx, tileset.min, upParams, true)) {
        // compress subtiles
        std::set<int> removedIndices;
        tileset.reuseFrames(removedIndices, true);
        // store the result
        // - store the CEL
        saParams.celFilePath = baseOutPath + ".cel";
        D1CelTileset::save(*tileset.gfx, saParams);
        // - store the tileset
        saParams.minFilePath = baseOutPath + ".min";
        // if (params.patchTilesets) {
            saParams.tilFilePath = baseOutPath + ".til";
        // }
        tileset.save(saParams);
    }
    // - upscale the special CEL
    if (tileset.cls->getFrameCount() != 0 && Upscaler::upscaleGfx(tileset.cls, upParams, true)) {
        // store the result
        // - store the S.CEL
        saParams.celFilePath = baseOutPath + "s.cel";
        D1Cel::save(*tileset.cls, saParams);
    }
}

static bool isRegularCel(const QString &asset)
{
    QString assetLower = asset.toLower();
    if (!assetLower.endsWith(".cel"))
        return false;
    if (assetLower.startsWith("gendata"))
        return false;
    if (assetLower.startsWith("levels"))
        return false;
    if (assetLower.startsWith("nlevels"))
        return false;
    // if (lineLower.endsWith("cow.cel"))
    //    return false;
    return true;
}

static int isRegularCl2(const QString &asset)
{
    QString assetLower = asset.toLower();
    if (!assetLower.endsWith(".cl2"))
        return 0;
    if (assetLower.startsWith("missiles"))
        return 1;
    if (assetLower.startsWith("monsters"))
        return 2;
    return 3;
}

static const AssetConfig objects[] = {
    // clang-format off
    // celname,                palette,                 numcolors, numfixcolors (protected colors)
    { "Objects\\L1Doors.CEL",  "Levels\\L1Data\\L1_1.PAL",    128,  0 },
    { "Objects\\L2Doors.CEL",  "Levels\\L2Data\\L2_1.PAL",    128,  0 },
    { "Objects\\L3Doors.CEL",  "Levels\\L3Data\\L3_1.PAL",    128, 32 },
    { "Objects\\L5Door.CEL",   "NLevels\\L5Data\\L5base.PAL", 128, 32 },
    { "Objects\\L5Books.CEL",  "NLevels\\L5Data\\L5base.PAL", 256, 32 },
    { "Objects\\L5Lever.CEL",  "NLevels\\L5Data\\L5base.PAL", 128, 32 },
    { "Objects\\L5Light.CEL",  "NLevels\\L5Data\\L5base.PAL", 128, 32 },
    { "Objects\\L5Sarco.CEL",  "NLevels\\L5Data\\L5base.PAL", 256, 32 },
    { "Objects\\Urnexpld.CEL", "NLevels\\L5Data\\L5base.PAL", 256, 32 },
    { "Objects\\Urn.CEL",      "NLevels\\L5Data\\L5base.PAL", 256, 32 },
    // clang-format on
};

static const std::pair<QString, QString> menuarts[] = {
    // clang-format off
    // celname,               palette
    { "ui_art\\mainmenu.CEL", "ui_art\\menu.PAL" },
    { "ui_art\\title.CEL",    "ui_art\\menu.PAL" },
    // { "ui_art\\logo.CEL",     "ui_art\\menu.PAL" },
    // { "ui_art\\smlogo.CEL",   "ui_art\\menu.PAL" },
    { "ui_art\\credits.CEL",  "ui_art\\credits.PAL" },
    // { "ui_art\\black.CEL",    "" },
    { "ui_art\\heros.CEL",    "" },
    // { "ui_art\\selconn.CEL",  "" },
    // { "ui_art\\selgame.CEL",  "" },
    // { "ui_art\\selhero.CEL",  "" },
    // "ui_art\\focus.CEL", "ui_art\\focus16.CEL", "ui_art\\focus42.CEL",
    // "ui_art\\lrpopup.CEL", "ui_art\\spopup.CEL", "ui_art\\srpopup.CEL", "ui_art\\smbutton.CEL"
    // "ui_art\\prog_bg.CEL", "ui_art\\prog_fil.CEL",
    // "ui_art\\sb_arrow.CEL", "ui_art\\sb_bg.CEL", "ui_art\\sb_thumb.CEL",
    // clang-format on
};

static const MinAssetConfig botchedMINs[] = {
    // celname,                      palette                   numcolors, numfixcolors, dunType
    { "Levels\\TownData\\Town.CEL",  "Levels\\TownData\\Town.PAL",   128,  0, DTYPE_TOWN      },
    { "NLevels\\TownData\\Town.CEL", "Levels\\TownData\\Town.PAL",   256,  0, DTYPE_TOWN      },
};

static const QString botchedCL2s[] = {
    "PlrGFX\\warrior\\wlb\\wlbat.CL2", "PlrGFX\\warrior\\wmb\\wmbat.CL2", "PlrGFX\\warrior\\whb\\whbat.CL2"
};

static bool isListedAsset(const QList<QString> &assets, const QString &asset)
{
    QString assetLower = asset.toLower();
    for (const QString &name : assets) {
        if (name.toLower() == assetLower)
            return true;
    }
    return false;
}

static bool isListedAsset(const QString *assets, int numAssets, const QString &asset)
{
    QString assetLower = asset.toLower();
    for (int i = 0; i < numAssets; i++) {
        const QString &name = assets[i];
        if (name.toLower() == assetLower)
            return true;
    }
    return false;
}

static bool isListedAsset(const std::pair<QString, QString> *assets, int numAssets, const QString &asset)
{
    QString assetLower = asset.toLower();
    for (int i = 0; i < numAssets; i++) {
        const QString &name = assets[i].first;
        if (name.toLower() == assetLower)
            return true;
    }
    return false;
}

static bool isListedAsset(const AssetConfig *assets, int numAssets, const QString &asset)
{
    QString assetLower = asset.toLower();
    for (int i = 0; i < numAssets; i++) {
        const QString &name = assets[i].path;
        if (name.toLower() == assetLower)
            return true;
    }
    return false;
}

static bool isListedAsset(const MinAssetConfig *assets, int numAssets, const QString &asset)
{
    QString assetLower = asset.toLower();
    for (int i = 0; i < numAssets; i++) {
        const QString &name = assets[i].path;
        if (name.toLower() == assetLower)
            return true;
    }
    return false;
}

void UpscaleTaskDialog::runTask(const UpscaleTaskParam &params)
{
    D1Pal defaultPal;
    defaultPal.load(D1Pal::DEFAULT_PATH);

    QList<QString> assets;
    int numRegularCels = 0;
    int numRegularCl2s_Missiles = 0;
    int numRegularCl2s_Monsters = 0;
    int numRegularCl2s_PlrGfx = 0;
    {
        QFile file(params.listfilesFile);

        if (!file.open(QIODevice::ReadOnly)) {
            dProgressFail() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(params.listfilesFile));
            return;
        }

        QTextStream in(&file);
        QString line;
        while (in.readLineInto(&line)) {
            if (line.isEmpty() || line[0] == '_')
                continue;
            assets.append(line);
            if (isRegularCel(line))
                numRegularCels++;
            switch (isRegularCl2(line)) {
            case 0:
                break;
            case 1:
                numRegularCl2s_Missiles++;
                break;
            case 2:
                numRegularCl2s_Monsters++;
                break;
            default:
                numRegularCl2s_PlrGfx++;
                break;
            }
        }
    }
    int totalSteps = 0;
    const int stepWeights[NUM_STEPS] = {
        // clang-format off
        2, // Regular CEL Files
        1, // Object CEL Files
        3, // Cutscenes
        2, // Art CEL Files
        2, // Regular CL2 Files - Missiles
        44, // Regular CL2 Files - Monsters
        120, // Regular CL2 Files - PlrGfx
        1, // Fixed CL2 Files
        16, // Tilesets
        2, // Fixed Tilesets
        // clang-format on
    };
    {
        for (int i = 0; i < NUM_STEPS; ++i) {
            if (params.steps[i]) {
                totalSteps += stepWeights[i];
            }
        }
    }

    ProgressDialog::incBar("", totalSteps + 1);

    // dProgress() << QString(QApplication::tr("Time:%1")).arg(QDateTime::currentDateTime().toString("hh:mm:ss,zzz"));

    if (params.steps[REGULAR_CEL]) {
        // upscale regular cel files of listfiles.txt
        //  - skips Levels(dungeon tiles), gendata(cutscenes) and cow.CEL manually
        ProgressDialog::incBar("Regular CEL Files", numRegularCels);

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.downscale = false;
        upParams.multiplier = params.multiplier;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = true;

        for (QString &asset : assets) {
            if (!isRegularCel(asset)) {
                continue;
            }
            if (params.steps[OBJECT_CEL] && isListedAsset(objects, lengthof(objects), asset)) {
                ProgressDialog::incValue();
                continue;
            }
            if (params.steps[ART_CEL] && isListedAsset(menuarts, lengthof(menuarts), asset)) {
                ProgressDialog::incValue();
                continue;
            }

            dProgress() << QString(QApplication::tr("Upscaling asset %1.")).arg(asset);
            if (ProgressDialog::wasCanceled()) {
                return;
            }

            UpscaleTaskDialog::upscaleCel(asset, &defaultPal, params, opParams, upParams, saParams);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }

        ProgressDialog::decBar();
        ProgressDialog::addValue(stepWeights[REGULAR_CEL]);
    }
    // dProgress() << QString(QApplication::tr("Time:%1")).arg(QDateTime::currentDateTime().toString("hh:mm:ss,zzz"));
    if (params.steps[OBJECT_CEL]) {
        // upscale objects with level-specific palette
        ProgressDialog::incBar("Object CEL Files", lengthof(objects));

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = true;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.downscale = false;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        for (int i = 0; i < lengthof(objects); i++) {
            if (!isListedAsset(assets, objects[i].path)) {
                continue;
            }

            dProgress() << QString(QApplication::tr("Upscaling asset %1.")).arg(objects[i].path);
            if (ProgressDialog::wasCanceled()) {
                return;
            }

            D1Pal pal;
            if (UpscaleTaskDialog::loadCustomPal(objects[i].palette, objects[i].numcolors, objects[i].fixcolors, params, pal, upParams))
                UpscaleTaskDialog::upscaleCel(objects[i].path, &pal, params, opParams, upParams, saParams);

            if (!ProgressDialog::incValue()) {
                return;
            }
        }

        ProgressDialog::decBar();
        ProgressDialog::addValue(stepWeights[OBJECT_CEL]);
    }
    // dProgress() << QString(QApplication::tr("Time:%1")).arg(QDateTime::currentDateTime().toString("hh:mm:ss,zzz"));
    if (params.steps[CUTSCENE]) {
        // upscale cutscenes
        const QString celPalPairs[][2] = {
            // clang-format off
            // celname,                palette
            { "Gendata\\Cut2.CEL",     "Gendata\\Cut2.pal" },
            { "Gendata\\Cut3.CEL",     "Gendata\\Cut3.pal" },
            { "Gendata\\Cut4.CEL",     "Gendata\\Cut4.pal" },
            { "Gendata\\Cutgate.CEL",  "Gendata\\Cutgate.pal" },
            { "Gendata\\Cutl1d.CEL",   "Gendata\\Cutl1d.pal" },
            { "Gendata\\Cutportl.CEL", "Gendata\\Cutportl.pal" },
            { "Gendata\\Cutportr.CEL", "Gendata\\Cutportr.pal" },
            { "Gendata\\Cutstart.CEL", "Gendata\\Cutstart.pal" },
            { "Gendata\\Cuttt.CEL",    "Gendata\\Cuttt.pal" },
            { "NLevels\\CutL5.CEL",    "NLevels\\CutL5.pal" },
            { "NLevels\\CutL6.CEL",    "NLevels\\CutL6.pal" },
            // clang-format on
        };
        ProgressDialog::incBar("Cutscenes", lengthof(celPalPairs));

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = true;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.downscale = false;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        for (int i = 0; i < lengthof(celPalPairs); i++) {
            if (!isListedAsset(assets, celPalPairs[i][0])) {
                continue;
            }

            dProgress() << QString(QApplication::tr("Upscaling asset %1.")).arg(celPalPairs[i][0]);
            if (ProgressDialog::wasCanceled()) {
                return;
            }

            QString palPath = params.assetsFolder + "/" + celPalPairs[i][1]; // "f:\\MPQE\\Work\\%s"
            D1Pal pal;
            if (pal.load(palPath)) {
                UpscaleTaskDialog::upscaleCel(celPalPairs[i][0], &pal, params, opParams, upParams, saParams);
            } else {
                dProgressErr() << QApplication::tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(palPath));
            }

            if (!ProgressDialog::incValue()) {
                return;
            }
        }

        ProgressDialog::decBar();
        ProgressDialog::addValue(stepWeights[CUTSCENE]);
    }
    // UpscaleCelComp("f:\\MPQE\\Work\\towners\\animals\\cow.CEL", 2, &diapal[0][0], 128, 128, "f:\\outcel\\towners\\animals\\cow.cel");
    // dProgress() << QString(QApplication::tr("Time:%1")).arg(QDateTime::currentDateTime().toString("hh:mm:ss,zzz"));
    if (params.steps[ART_CEL]) {
        // upscale non-standard CELs of the menu (converted from PCX)
        ProgressDialog::incBar("Art CEL Files", lengthof(menuarts));

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = true;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.downscale = false;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        for (int i = 0; i < lengthof(menuarts); i++) {
            if (!isListedAsset(assets, menuarts[i].first)) {
                continue;
            }

            dProgress() << QString(QApplication::tr("Upscaling asset %1.")).arg(menuarts[i].first);
            if (ProgressDialog::wasCanceled()) {
                return;
            }

            QString palPath = menuarts[i].second.isEmpty() ? D1Pal::DEFAULT_PATH : (params.assetsFolder + "/" + menuarts[i].second); // "f:\\MPQE\\Work\\%s"

            D1Pal pal;
            if (pal.load(palPath)) {
                UpscaleTaskDialog::upscaleCel(menuarts[i].first, &pal, params, opParams, upParams, saParams);
            } else {
                dProgressErr() << QApplication::tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(palPath));
            }

            if (!ProgressDialog::incValue()) {
                return;
            }
        }

        ProgressDialog::decBar();
        ProgressDialog::addValue(stepWeights[ART_CEL]);
    }
    // dProgress() << QString(QApplication::tr("Time:%1")).arg(QDateTime::currentDateTime().toString("hh:mm:ss,zzz"));
    if (params.steps[REGULAR_CL2_MISSILES]) {
        // upscale missiles cl2 files of listfiles.txt
        ProgressDialog::incBar("Regular CL2 Files - Missiles", numRegularCl2s_Missiles);

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = true;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.downscale = false;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        for (QString &asset : assets) {
            if (isRegularCl2(asset) != 1) {
                continue;
            }

            dProgress() << QString(QApplication::tr("Upscaling asset %1.")).arg(asset);
            if (ProgressDialog::wasCanceled()) {
                return;
            }

            UpscaleTaskDialog::upscaleCl2(asset, &defaultPal, params, opParams, upParams, saParams);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }

        ProgressDialog::decBar();
        ProgressDialog::addValue(stepWeights[REGULAR_CL2_MISSILES]);
    }
    // dProgress() << QString(QApplication::tr("Time:%1")).arg(QDateTime::currentDateTime().toString("hh:mm:ss,zzz"));
    if (params.steps[REGULAR_CL2_MONSTERS]) {
        // upscale monsters cl2 files of listfiles.txt
        ProgressDialog::incBar("Regular CL2 Files - Monsters", numRegularCl2s_Monsters);

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = true;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.downscale = false;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        for (QString &asset : assets) {
            if (isRegularCl2(asset) != 2) {
                continue;
            }

            dProgress() << QString(QApplication::tr("Upscaling asset %1.")).arg(asset);
            if (ProgressDialog::wasCanceled()) {
                return;
            }

            UpscaleTaskDialog::upscaleCl2(asset, &defaultPal, params, opParams, upParams, saParams);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }

        ProgressDialog::decBar();
        ProgressDialog::addValue(stepWeights[REGULAR_CL2_MONSTERS]);
    }
    // dProgress() << QString(QApplication::tr("Time:%1")).arg(QDateTime::currentDateTime().toString("hh:mm:ss,zzz"));
    if (params.steps[REGULAR_CL2_PLRGFX]) {
        // upscale plrgfx cl2 files of listfiles.txt
        ProgressDialog::incBar("Regular CL2 Files - PlrGfx", numRegularCl2s_PlrGfx);

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = true;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.downscale = false;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        for (QString &asset : assets) {
            if (isRegularCl2(asset) != 3) {
                continue;
            }
            if (params.steps[FIXED_CL2] && isListedAsset(botchedCL2s, lengthof(botchedCL2s), asset)) {
                ProgressDialog::incValue();
                continue;
            }

            dProgress() << QString(QApplication::tr("Upscaling asset %1.")).arg(asset);
            if (ProgressDialog::wasCanceled()) {
                return;
            }

            UpscaleTaskDialog::upscaleCl2(asset, &defaultPal, params, opParams, upParams, saParams);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }

        ProgressDialog::decBar();
        ProgressDialog::addValue(stepWeights[REGULAR_CL2_PLRGFX]);
    }
    // dProgress() << QString(QApplication::tr("Time:%1")).arg(QDateTime::currentDateTime().toString("hh:mm:ss,zzz"));
    if (params.steps[FIXED_CL2]) {
        // special cases to upscale cl2 files (must be done manually, because width detection fails)
        ProgressDialog::incBar("Fixed CL2 Files", lengthof(botchedCL2s));

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = true;

        OpenAsParam opParams = OpenAsParam();
        opParams.celWidth = 96;

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.downscale = false;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        for (int i = 0; i < lengthof(botchedCL2s); i++) {
            if (!isListedAsset(assets, botchedCL2s[i])) {
                continue;
            }

            dProgress() << QString(QApplication::tr("Upscaling asset %1.")).arg(botchedCL2s[i]);
            if (ProgressDialog::wasCanceled()) {
                return;
            }

            UpscaleTaskDialog::upscaleCl2(botchedCL2s[i], &defaultPal, params, opParams, upParams, saParams);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }

        ProgressDialog::decBar();
        ProgressDialog::addValue(stepWeights[FIXED_CL2]);
    }
    // dProgress() << QString(QApplication::tr("Time:%1")).arg(QDateTime::currentDateTime().toString("hh:mm:ss,zzz"));
    if (params.steps[TILESET]) {
        // upscale tiles of the levels
        const MinAssetConfig celPalPairs[] = {
            // clang-format off
            // celname,                      palette                   numcolors, numfixcolors, dunType
            { "Levels\\TownData\\Town.CEL",  "Levels\\TownData\\Town.PAL",   128,  0, DTYPE_TOWN      },
            { "Levels\\L1Data\\L1.CEL",      "Levels\\L1Data\\L1_1.PAL",     256,  0, DTYPE_CATHEDRAL },
            { "Levels\\L2Data\\L2.CEL",      "Levels\\L2Data\\L2_1.PAL",     128,  0, DTYPE_CATACOMBS },
            { "Levels\\L3Data\\L3.CEL",      "Levels\\L3Data\\L3_1.PAL",     128, 32, DTYPE_CAVES     },
            { "Levels\\L4Data\\L4.CEL",      "Levels\\L4Data\\L4_1.PAL",     128, 32, DTYPE_HELL      },
            { "NLevels\\TownData\\Town.CEL", "Levels\\TownData\\Town.PAL",   256,  0, DTYPE_TOWN      },
            { "NLevels\\L5Data\\L5.CEL",     "NLevels\\L5Data\\L5base.PAL",  128, 32, DTYPE_CRYPT     },
            { "NLevels\\L6Data\\L6.CEL",     "NLevels\\L6Data\\L6base1.PAL", 128, 32, DTYPE_NEST      },
            // clang-format on
        };
        ProgressDialog::incBar("Tilesets", lengthof(celPalPairs));

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = true;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.downscale = false;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::TILESET;

        for (int i = 0; i < lengthof(celPalPairs); i++) {
            opParams.celFilePath = celPalPairs[i].path;
            if (!isListedAsset(assets, opParams.celFilePath)) {
                continue;
            }
            if (params.steps[FIXED_TILESET] && isListedAsset(botchedMINs, lengthof(botchedMINs), opParams.celFilePath)) {
                continue;
            }

            dProgress() << QString(QApplication::tr("Upscaling tileset %1.")).arg(opParams.celFilePath);
            if (ProgressDialog::wasCanceled()) {
                return;
            }

            D1Pal pal;
            if (UpscaleTaskDialog::loadCustomPal(celPalPairs[i].palette, celPalPairs[i].numcolors, celPalPairs[i].fixcolors, params, pal, upParams)) {
                UpscaleTaskDialog::upscaleMin(&pal, params, opParams, upParams, saParams, celPalPairs[i].dunType);
            }

            if (!ProgressDialog::incValue()) {
                return;
            }
        }

        ProgressDialog::decBar();
        ProgressDialog::addValue(stepWeights[TILESET]);
    }
    // dProgress() << QString(QApplication::tr("Time:%1")).arg(QDateTime::currentDateTime().toString("hh:mm:ss,zzz"));
    if (params.steps[FIXED_TILESET]) {
        // special cases to upscale tilesets (must be done manually, because height detection fails)
        ProgressDialog::incBar("Fixed Tilesets", lengthof(botchedMINs));

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = true;

        OpenAsParam opParams = OpenAsParam();
        opParams.minHeight = 8;

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.downscale = false;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::TILESET;

        for (int i = 0; i < lengthof(botchedMINs); i++) {
            opParams.celFilePath = botchedMINs[i].path;
            if (!isListedAsset(assets, opParams.celFilePath)) {
                continue;
            }
            dProgress() << QString(QApplication::tr("Upscaling tileset %1.")).arg(opParams.celFilePath);
            if (ProgressDialog::wasCanceled()) {
                return;
            }

            D1Pal pal;
            if (UpscaleTaskDialog::loadCustomPal(botchedMINs[i].palette, botchedMINs[i].numcolors, botchedMINs[i].fixcolors, params, pal, upParams)) {
                UpscaleTaskDialog::upscaleMin(&pal, params, opParams, upParams, saParams, botchedMINs[i].dunType);
            }
            if (!ProgressDialog::incValue()) {
                return;
            }
        }

        ProgressDialog::decBar();
        ProgressDialog::addValue(stepWeights[FIXED_TILESET]);
    }
    // dProgress() << QString(QApplication::tr("Time:%1")).arg(QDateTime::currentDateTime().toString("hh:mm:ss,zzz"));*/

    ProgressDialog::decBar();
}
