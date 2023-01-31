#include "upscaletaskdialog.h"

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
#include "d1sol.h"
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
    const char *path;
    const char *palette;
    int numcolors;
    int fixcolors;
} AssetConfig;

UpscaleTaskDialog::UpscaleTaskDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UpscaleTaskDialog())
{
    ui->setupUi(this);
}

UpscaleTaskDialog::~UpscaleTaskDialog()
{
    delete ui;
}

void UpscaleTaskDialog::on_listfilesFileBrowseButton_clicked()
{
    MainWindow *qw = (MainWindow *)this->parentWidget();
    QString filePath = qw->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select listfiles.txt"), tr("TXT Files (*.txt *.TXT)"));

    if (filePath.isEmpty())
        return;

    this->ui->listfilesFileLineEdit->setText(filePath);
}

void UpscaleTaskDialog::on_assetsFolderBrowseButton_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this, tr("Select Assets Folder"), QString(), QFileDialog::ShowDirsOnly);

    if (dirPath.isEmpty())
        return;

    this->ui->assetsFolderLineEdit->setText(dirPath);
}

void UpscaleTaskDialog::on_outputFolderBrowseButton_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this, tr("Select Output Folder"), QString(), QFileDialog::ShowDirsOnly);

    if (dirPath.isEmpty())
        return;

    this->ui->outputFolderLineEdit->setText(dirPath);
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
    params.autoOverwrite = this->ui->autoOverwriteCheckBox->isChecked();

    this->close();

    ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Upscaling assets..."), 25 + 1);

    UpscaleTaskDialog::runTask(params);

    ProgressDialog::done();
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

bool UpscaleTaskDialog::loadCustomPal(const char *path, int numcolors, int fixcolors, const UpscaleTaskParam &params, D1Pal &pal, UpscaleParam &upParams)
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
    // upscale
    ProgressDialog::restartSub(gfx.getFrameCount() + 1);
    if (Upscaler::upscaleGfx(&gfx, upParams)) {
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
    // upscale
    ProgressDialog::restartSub(gfx.getFrameCount() + 1);
    if (Upscaler::upscaleGfx(&gfx, upParams)) {
        // store the result
        saParams.celFilePath = outFilePath;
        D1Cl2::save(gfx, saParams);
    }
}

void UpscaleTaskDialog::upscaleMin(const QString &path, D1Pal *pal, const UpscaleTaskParam &params, const OpenAsParam &opParams, const UpscaleParam &upParams, SaveAsParam &saParams)
{
    QString celFilePath = params.assetsFolder + "/" + path; // "f:\\MPQE\\Work\\%s"
    QString outFilePath = params.outFolder + "/" + path;    // "f:\\outcel\\%s"

    QString basePath = celFilePath;
    basePath.chop(4);
    QString minFilePath = basePath + ".min";
    QString solFilePath = basePath + ".sol";

    // QString outMinPath = params.outFolder + "/" + celFileInfo.completeBaseName() + ".min"; // "f:\\outcel\\%s"
    QString outMinPath = outFilePath;
    outMinPath.chop(4);
    outMinPath += ".min";

    D1Gfx gfx = D1Gfx();
    gfx.setPalette(pal);
    // Loading SOL
    D1Sol sol = D1Sol();
    if (sol.load(solFilePath)) {
        dProgressErr() << tr("Failed loading SOL file: %1.").arg(QDir::toNativeSeparators(solFilePath));
        return;
    }
    // Loading MIN
    D1Min min = D1Min();
    std::map<unsigned, D1CEL_FRAME_TYPE> celFrameTypes;
    if (!min.load(minFilePath, &gfx, &sol, celFrameTypes, opParams)) {
        dProgressErr() << tr("Failed loading MIN file: %1.").arg(QDir::toNativeSeparators(minFilePath));
        return;
    }
    // Loading CEL
    if (!D1CelTileset::load(gfx, celFrameTypes, celFilePath, opParams)) {
        dProgressErr() << tr("Failed loading Tileset-CEL file: %1.").arg(QDir::toNativeSeparators(celFilePath));
        return;
    }
    // upscale
    ProgressDialog::restartSub(min.getSubtileCount() + 1);
    if (Upscaler::upscaleTileset(&gfx, &min, upParams)) {
        // store the result
        saParams.celFilePath = outFilePath;
        D1CelTileset::save(gfx, saParams);
        saParams.minFilePath = outMinPath;
        min.save(saParams);
    }
}

void UpscaleTaskDialog::runTask(const UpscaleTaskParam &params)
{
    D1Pal defaultPal;
    defaultPal.load(D1Pal::DEFAULT_PATH);

    ProgressDialog::startSub(1);

    { // upscale regular cel files of listfiles.txt
      //  - skips Levels(dungeon tiles), gendata(cutscenes) and cow.CEL manually
        QFile file(params.listfilesFile);

        if (!file.open(QIODevice::ReadOnly)) {
            dProgressErr() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(params.listfilesFile));
        }

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = params.autoOverwrite;

        while (file.canReadLine()) {
            QString line = file.readLine();
            size_t ls = line.size();
            if (ls <= 4)
                continue;
            if (line[0] == '_')
                continue;
            QString lineLower = line.toLower();
            if (!lineLower.endsWith(".cel"))
                continue;
            if (lineLower.startsWith("gendata"))
                continue;
            if (lineLower.startsWith("levels"))
                continue;
            if (lineLower.startsWith("nlevels"))
                continue;
            // if (lineLower.endsWith("cow.cel"))
            //    continue;

            if (ProgressDialog::wasCanceled()) {
                return;
            }
            dProgress() << QString(QApplication::tr("Upscaling asset %1.")).arg(QDir::toNativeSeparators(line));
            // ProgressDialog::incValue();

            UpscaleTaskDialog::upscaleCel(line, &defaultPal, params, opParams, upParams, saParams);
        }

        for (int i = 0; i < 4; i++)
            ProgressDialog::incMainValue();
    }
    { // upscale objects with level-specific palette
        const AssetConfig celPalPairs[] = {
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

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = params.autoOverwrite;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        for (int i = 0; i < lengthof(celPalPairs); i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            dProgress() << QString(QApplication::tr("Upscaling object CEL %1.")).arg(QDir::toNativeSeparators(celPalPairs[i].path));
            // ProgressDialog::incValue();

            D1Pal pal;
            if (!UpscaleTaskDialog::loadCustomPal(celPalPairs[i].palette, celPalPairs[i].numcolors, celPalPairs[i].fixcolors, params, pal, upParams))
                continue;

            UpscaleTaskDialog::upscaleCel(celPalPairs[i].path, &pal, params, opParams, upParams, saParams);
        }

        for (int i = 0; i < 1; i++)
            ProgressDialog::incMainValue();
    }
    { // upscale special cells of the levels
        const AssetConfig celPalPairs[] = {
            // clang-format off
            // celname,                      palette,                 numcolors, numfixcolors (protected colors)
            { "Levels\\TownData\\TownS.CEL", "Levels\\TownData\\Town.PAL",  128,  0 },
            { "Levels\\L1Data\\L1S.CEL",     "Levels\\L1Data\\L1_1.PAL",    128,  0 },
            { "Levels\\L2Data\\L2S.CEL",     "Levels\\L2Data\\L2_1.PAL",    128,  0 },
            { "NLevels\\L5Data\\L5S.CEL",    "NLevels\\L5Data\\L5base.PAL", 128, 32 },
            // clang-format on
        };

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = params.autoOverwrite;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        for (int i = 0; i < lengthof(celPalPairs); i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            dProgress() << QString(QApplication::tr("Upscaling special CEL %1.")).arg(QDir::toNativeSeparators(celPalPairs[i].path));
            // ProgressDialog::incValue();

            D1Pal pal;
            if (!UpscaleTaskDialog::loadCustomPal(celPalPairs[i].palette, celPalPairs[i].numcolors, celPalPairs[i].fixcolors, params, pal, upParams))
                continue;

            UpscaleTaskDialog::upscaleCel(celPalPairs[i].path, &pal, params, opParams, upParams, saParams);
        }

        for (int i = 0; i < 1; i++)
            ProgressDialog::incMainValue();
    }
    { // upscale cutscenes
        const char *celPalPairs[][2] = {
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

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = params.autoOverwrite;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        for (int i = 0; i < lengthof(celPalPairs); i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            dProgress() << QString(QApplication::tr("Upscaling cutscene CEL %1.")).arg(QDir::toNativeSeparators(celPalPairs[i][0]));
            // ProgressDialog::incValue();

            QString palPath = params.assetsFolder + "/" + celPalPairs[i][1]; // "f:\\MPQE\\Work\\%s"
            D1Pal pal;
            if (!pal.load(palPath)) {
                dProgressErr() << QApplication::tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(palPath));
                continue;
            }

            UpscaleTaskDialog::upscaleCel(celPalPairs[i][0], &pal, params, opParams, upParams, saParams);
        }

        for (int i = 0; i < 8; i++)
            ProgressDialog::incMainValue();
    }
    // UpscaleCelComp("f:\\MPQE\\Work\\towners\\animals\\cow.CEL", 2, &diapal[0][0], 128, 128, "f:\\outcel\\towners\\animals\\cow.cel");
    { // upscale non-standard CELs of the menu (converted from PCX)
        const char *celPalPairs[][2] = {
            // clang-format off
            // celname,               palette
            { "ui_art\\mainmenu.CEL", "ui_art\\menu.PAL" },
            { "ui_art\\title.CEL",    "ui_art\\menu.PAL" },
            { "ui_art\\logo.CEL",     "ui_art\\menu.PAL" },
            { "ui_art\\smlogo.CEL",   "ui_art\\menu.PAL" },
            { "ui_art\\credits.CEL",  "ui_art\\credits.PAL" },
            { "ui_art\\black.CEL",    "" },
            { "ui_art\\heros.CEL",    "" },
            { "ui_art\\selconn.CEL",  "" },
            { "ui_art\\selgame.CEL",  "" },
            { "ui_art\\selhero.CEL",  "" },
            // "ui_art\\focus.CEL", "ui_art\\focus16.CEL", "ui_art\\focus42.CEL",
            // "ui_art\\lrpopup.CEL", "ui_art\\spopup.CEL", "ui_art\\srpopup.CEL", "ui_art\\smbutton.CEL"
            // "ui_art\\prog_bg.CEL", "ui_art\\prog_fil.CEL",
            // "ui_art\\sb_arrow.CEL", "ui_art\\sb_bg.CEL", "ui_art\\sb_thumb.CEL",
            // clang-format on
        };

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = params.autoOverwrite;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        for (int i = 0; i < lengthof(celPalPairs); i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            dProgress() << QString(QApplication::tr("Upscaling ui-art CEL %1.")).arg(QDir::toNativeSeparators(celPalPairs[i][0]));
            // ProgressDialog::incValue();

            QString palPath = celPalPairs[i][1][0] == '\0' ? D1Pal::DEFAULT_PATH : (params.assetsFolder + "/" + celPalPairs[i][1]); // "f:\\MPQE\\Work\\%s"

            D1Pal pal;
            if (!pal.load(palPath)) {
                dProgressErr() << QApplication::tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(palPath));
                continue;
            }

            UpscaleTaskDialog::upscaleCel(celPalPairs[i][0], &pal, params, opParams, upParams, saParams);
        }

        for (int i = 0; i < 1; i++)
            ProgressDialog::incMainValue();
    }
    { // upscale all cl2 files of listfiles.txt
        QFile file(params.listfilesFile);

        if (!file.open(QIODevice::ReadOnly)) {
            dProgressErr() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(params.listfilesFile));
            return;
        }

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = params.autoOverwrite;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        while (file.canReadLine()) {
            QString line = file.readLine();
            size_t ls = line.size();
            if (ls <= 4)
                continue;
            if (line[0] == '_')
                continue;
            if (!line.toLower().endsWith(".cl2"))
                continue;

            if (ProgressDialog::wasCanceled()) {
                return;
            }
            dProgress() << QString(QApplication::tr("Upscaling asset %1.")).arg(QDir::toNativeSeparators(line));
            // ProgressDialog::incValue();

            UpscaleTaskDialog::upscaleCl2(line, &defaultPal, params, opParams, upParams, saParams);
        }

        for (int i = 0; i < 4; i++)
            ProgressDialog::incMainValue();
    }
    /*if (!params.cl2GfxFixed)*/ { // special cases to upscale cl2 files (must be done manually)
        // - width detection fails -> run in debug mode and update the width values, or alter the code to set it manually
        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = params.autoOverwrite;

        OpenAsParam opParams = OpenAsParam();
        opParams.celWidth = 96;

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::BASIC;

        const char *botchedCL2s[] = {
            "PlrGFX\\warrior\\wlb\\wlbat.CL2", "PlrGFX\\warrior\\wlb\\wmbat.CL2", "PlrGFX\\warrior\\wlb\\whbat.CL2"
        };

        for (int i = 0; i < lengthof(botchedCL2s); i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            dProgress() << QString(QApplication::tr("Upscaling botched asset %1.")).arg(QDir::toNativeSeparators(botchedCL2s[i]));
            // ProgressDialog::incValue();

            UpscaleTaskDialog::upscaleCl2(botchedCL2s[i], &defaultPal, params, opParams, upParams, saParams);
        }

        for (int i = 0; i < 1; i++)
            ProgressDialog::incValue();
    }
    { // // upscale tiles of the levels
        const AssetConfig celPalPairs[] = {
            // clang-format off
            // celname,                      palette                   numcolors, numfixcolors (protected colors)
            { "Levels\\TownData\\Town.CEL",  "Levels\\TownData\\Town.PAL",   128,  0 },
            { "Levels\\L1Data\\L1.CEL",      "Levels\\L1Data\\L1_1.PAL",     128,  0 },
            { "Levels\\L2Data\\L2.CEL",      "Levels\\L2Data\\L2_1.PAL",     128,  0 },
            { "Levels\\L3Data\\L3.CEL",      "Levels\\L3Data\\L3_1.PAL",     128, 32 },
            { "Levels\\L4Data\\L4.CEL",      "Levels\\L4Data\\L4_1.PAL",     128, 32 },
            { "NLevels\\L5Data\\L5.CEL",     "NLevels\\L5Data\\L5base.PAL",  128, 32 },
            { "NLevels\\L6Data\\L6.CEL",     "NLevels\\L6Data\\L6base1.PAL", 128, 32 },
            // clang-format on
        };

        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = params.autoOverwrite;

        OpenAsParam opParams = OpenAsParam();

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::TILESET;

        for (int i = 0; i < lengthof(celPalPairs); i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            dProgress() << QString(QApplication::tr("Upscaling tileset %1.")).arg(QDir::toNativeSeparators(celPalPairs[i].path));
            // ProgressDialog::incValue();

            D1Pal pal;
            if (!UpscaleTaskDialog::loadCustomPal(celPalPairs[i].palette, celPalPairs[i].numcolors, celPalPairs[i].fixcolors, params, pal, upParams))
                continue;

            UpscaleTaskDialog::upscaleMin(celPalPairs[i].path, &pal, params, opParams, upParams, saParams);
        }

        for (int i = 0; i < 4; i++)
            ProgressDialog::incMainValue();
    }
    /*if (!params.minGfxFixed)*/ { // special cases to upscale cl2 files (must be done manually)
        // - width detection fails -> run in debug mode and update the width values, or alter the code to set it manually
        SaveAsParam saParams = SaveAsParam();
        saParams.autoOverwrite = params.autoOverwrite;

        OpenAsParam opParams = OpenAsParam();
        opParams.minHeight = 8;

        UpscaleParam upParams = UpscaleParam();
        upParams.multiplier = params.multiplier;
        upParams.firstfixcolor = -1;
        upParams.lastfixcolor = -1;
        upParams.antiAliasingMode = ANTI_ALIASING_MODE::TILESET;

        const AssetConfig botchedMINs[] = {
            // clang-format off
            // celname,                      palette,                numcolors, numfixcolors (protected colors)
            { "NLevels\\TownData\\Town.CEL", "Levels\\TownData\\Town.PAL", 128,  0 },
            // clang-format on
        };

        for (int i = 0; i < lengthof(botchedMINs); i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            dProgress() << QString(QApplication::tr("Upscaling botched tileset %1.")).arg(QDir::toNativeSeparators(botchedMINs[i].path));
            // ProgressDialog::incValue();

            D1Pal pal;
            if (!UpscaleTaskDialog::loadCustomPal(botchedMINs[i].palette, botchedMINs[i].numcolors, botchedMINs[i].fixcolors, params, pal, upParams))
                continue;

            UpscaleTaskDialog::upscaleMin(botchedMINs[i].path, &pal, params, opParams, upParams, saParams);
        }

        for (int i = 0; i < 1; i++)
            ProgressDialog::incMainValue();
    }
    ProgressDialog::doneSub();
}
