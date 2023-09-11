#include "d1trs.h"

#include <array>

#include <QApplication>
#include <QChar>
#include <QDir>
#include <QFile>
#include <QMessageBox>

#include "trngeneratedialog.h"
#include "dungeon/all.h"

bool D1Trs::load(const QString &filePath, D1Pal *pal, std::vector<D1Trn *> &trns)
{
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    size_t fileSize = file.size();
    if ((fileSize % D1TRN_TRANSLATIONS) != 0)
        return false;

    const QByteArray fileData = file.readAll();
    // Read SLA binary data
    QDataStream in(fileData);

    unsigned numTrns = fileSize / D1TRN_TRANSLATIONS;
    for (unsigned i = 0; i < numTrns; i++) {
        D1Trn *trn = new D1Trn();
        quint8 readByte;
        for (unsigned n = 0; n < D1TRN_TRANSLATIONS; n++) {
            in >> readByte;
            trn->setTranslation(n, readByte);
        }
        QString counter = QString::number(i);
        counter = counter.rightJustified(2, QChar('0'));
        QString trnFilePath = filePath;
        trnFilePath.insert(filePath.length() - 4, counter);
        trn->setFilePath(trnFilePath);
        trn->setPalette(pal);
        trn->refreshResultingPalette();
        trns.push_back(trn);
    }
    return true;
}

bool D1Trs::save(const QString &filePath, const std::vector<D1Trn *> &trns)
{
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath)));
        return false;
    }

    // write to file
    QDataStream out(&outFile);

    for (const D1Trn *trn : trns) {
        quint8 writeByte;
        for (unsigned n = 0; n < D1TRN_TRANSLATIONS; n++) {
            writeByte = trn->getTranslation(n);
            out << writeByte;
        }
    }

    return true;
}

static void MakeLightTableBase(const GenerateTrnParam &params)
{
    unsigned i, j, k, shade;
    BYTE col, max;
    BYTE* tbl;

    tbl = ColorTrns[0];
    for (i = 0; i < MAXDARKNESS; i++) {
        static_assert(MAXDARKNESS == 15, "Shade calculation requires MAXDARKNESS to be 15.");
        // shade calculation is simplified by using a fix MAXDARKNESS value.
        // otherwise the correct calculation would be as follows:
        //    shade = (i * 15 + (MAXDARKNESS + 1) / 2) / MAXDARKNESS;
        shade = i;
        // light trns of the level palette
        *tbl++ = 0;
        for (j = 0; j < 8; j++) {
            col = 16 * j + shade;
            max = 16 * j + 15;
            for (k = 0; k < 16; k++) {
                if (k != 0 || j != 0) {
                    *tbl++ = col;
                }
                if (col < max) {
                    col++;
                } else {
                    max = 0;
                    col = 0;
                }
            }
        }
        // light trns of the standard palette
        //  (PAL8_BLUE, PAL8_RED, PAL8_YELLOW, PAL8_ORANGE)
        for (j = 16; j < 20; j++) {
            col = 8 * j + (shade >> 1);
            max = 8 * j + 7;
            for (k = 0; k < 8; k++) {
                *tbl++ = col;
                if (col < max) {
                    col++;
                } else {
                    max = 0;
                    col = 0;
                }
            }
        }
        //  (PAL16_BEIGE, PAL16_BLUE, PAL16_YELLOW, PAL16_ORANGE, PAL16_RED, PAL16_GRAY)
        for (j = 10; j < 16; j++) {
            col = 16 * j + shade;
            max = 16 * j + 15;
            if (max == 255) {
                max = 254;
            }
            for (k = 0; k < 16; k++) {
                *tbl++ = col;
                if (col < max) {
                    col++;
                } else {
                    max = 0;
                    col = 0;
                }
            }
        }
    }

    // assert(tbl == ColorTrns[MAXDARKNESS]);
    tbl = ColorTrns[0];
    memset(ColorTrns[MAXDARKNESS], 0, sizeof(ColorTrns[MAXDARKNESS]));

    if (params.mode == DTYPE_HELL) {
        for (i = 0; i <= MAXDARKNESS; i++) {
            shade = i;
            col = 1;
            *tbl++ = 0;
            for (k = 1; k < 16; k++) {
                *tbl++ = col;
                if (shade > 0) {
                    shade--;
                } else {
                    col++;
                }
            }
            shade = i;
            col = 16 * 1 + shade;
            max = 16 * 1 + 15;
            for (k = 0; k < 16; k++) {
                *tbl++ = col;
                if (col < max) {
                    col++;
                } else {
                    max = 1;
                    col = 1;
                }
            }
            tbl += NUM_COLORS - 32;
        }
#ifdef HELLFIRE
    } else if (params.mode == DTYPE_CAVES || params.mode == DTYPE_CRYPT) {
#else
    } else if (params.mode == DTYPE_CAVES) {
#endif
        for (i = 0; i <= MAXDARKNESS; i++) {
            *tbl++ = 0;
            for (j = 1; j < 32; j++)
                *tbl++ = j;
            tbl += NUM_COLORS - 32;
        }
#ifdef HELLFIRE
    } else if (params.mode == DTYPE_NEST) {
        for (i = 0; i <= MAXDARKNESS; i++) {
            *tbl++ = 0;
            for (j = 1; j < 16; j++)
                *tbl++ = j;
            tbl += NUM_COLORS - 16;
        }
#endif
    }
}

/*static void MakeLightTableNew(const GenerateTrnParam& params)
{
    unsigned i, j, k;
    /*GenerateTrnParam params;
    {
        GenerateTrnColor black;
        black.firstcolor = 0;
        black.lastcolor = 0;
        black.shadecolor = false;
        params.colors.push_back(black);
    }
    for (i = 0; i < 8; i++) {
        GenerateTrnColor levelColor;
        levelColor.firstcolor = i == 0 ? 1 : i * 16;
        levelColor.lastcolor = (i + 1) * 16 - 1;
        levelColor.shadecolor = true;
        params.colors.push_back(levelColor);
    }
    for (i = 0; i < 4; i++) {
        GenerateTrnColor stdColor;
        stdColor.firstcolor = 16 * 8 + i * 8;
        stdColor.lastcolor = stdColor.firstcolor + 8 - 1;
        stdColor.shadecolor = true;
        params.colors.push_back(stdColor);
    }
    for (i = 0; i < 6; i++) {
        GenerateTrnColor stdColor;
        stdColor.firstcolor = 16 * 8 + 8 * 4 + i * 16;
        stdColor.lastcolor = i == 5 ? 254 : (stdColor.firstcolor + 15);
        stdColor.shadecolor = true;
        params.colors.push_back(stdColor);
    }
    {
        GenerateTrnColor white;
        white.firstcolor = 255;
        white.lastcolor = 255;
        white.shadecolor = false;
        params.colors.push_back(white);
    }
    switch (type) {
    case DTYPE_TOWN:
        break;
    case DTYPE_CAVES:
        params.colors.erase(params.colors.begin() + 2);
        params.colors[1].lastcolor = 31;
        params.colors[1].shadecolor = false;
        // FIXME: maxdarkness?
        break;
    case DTYPE_HELL:
        break;
    case DTYPE_NEST:
        params.colors[1].shadecolor = false;
        // FIXME: maxdarkness?
        break;
    }* /

    // BYTE colors[NUM_COLORS];
    // int numColors;
    D1Pal *basePal = params.pals.front();
    for (i = 0; i <= MAXDARKNESS; i++) {
        for (k = 0; k < NUM_COLORS; k++) {
            for (j = 0; j < params.colors.size(); j++) {
                if ((int)k < params.colors[j].firstcolor || (int)k > params.colors[j].lastcolor) {
                    continue;
                }
                if (!params.colors[j].shadesteps < 0) {
                    ColorTrns[i][k] = k;
                } else {
                    int numColors = params.colors[j].lastcolor - params.colors[j].firstcolor + 1;
                    int col = (numColors * i) / (MAXDARKNESS + 1);
                    QColor colorFirst = basePal->getColor(params.colors[j].firstcolor);
                    QColor colorLast = basePal->getColor(params.colors[j].lastcolor);
                    if (colorFirst.lightness() > colorLast.lightness()) {
                        col = k + col;
                        if (col > params.colors[j].lastcolor) {
                            col = 0;
                        }
                    } else {
                        col = k - col;
                        if (col < params.colors[j].firstcolor) {
                            col = 0;
                        }
                    }
                    ColorTrns[i][k] = col;
                }
                break;
            }
            if (j != params.colors.size()) {
                continue;
            }
            // FIXME:...
        }
    }
}*/

static double redWeight;
static double greenWeight;
static double blueWeight;
static double lightWeight;
static void (*colorSelector)(const std::vector<PaletteColor> &dynColors, QColor color, std::array<int, NUM_COLORS> &palOptions);
static void getPalColorSquare(const std::vector<PaletteColor> &dynColors, QColor color, std::array<int, NUM_COLORS> &palOptions)
{
    for (const PaletteColor &palColor : dynColors) {
        int currR = color.red() - palColor.red();
        int currG = color.green() - palColor.green();
        int currB = color.blue() - palColor.blue();
        int curr = currR * currR + currG * currG + currB * currB;
        palOptions[palColor.index()] = curr;
    }
}
static void getPalColorQuad(const std::vector<PaletteColor> &dynColors, QColor color, std::array<int, NUM_COLORS> &palOptions)
{
    for (const PaletteColor &palColor : dynColors) {
        int currR = color.red() - palColor.red();
        int currG = color.green() - palColor.green();
        int currB = color.blue() - palColor.blue();
        double curr = currR * currR * currR * currR + currG * currG * currG * currG + currB * currB * currB * currB;
        palOptions[palColor.index()] = sqrt(curr);
    }
}
static void getPalColorRiemersma(const std::vector<PaletteColor> &dynColors, QColor color, std::array<int, NUM_COLORS> &palOptions)
{
    for (const PaletteColor &palColor : dynColors) {
        int r = color.red() + palColor.red();
        int currR = color.red() - palColor.red();
        int currG = color.green() - palColor.green();
        int currB = color.blue() - palColor.blue();
        int curr = (1024 + r) * currR * currR + 4 * 512 * currG * currG + (1024 + 510 - r) * currB * currB;
        palOptions[palColor.index()] = curr;
    }
}
static void getPalColorWeightedSquare(const std::vector<PaletteColor> &dynColors, QColor color, std::array<int, NUM_COLORS> &palOptions)
{
    for (const PaletteColor &palColor : dynColors) {
        int currR = color.red() - palColor.red();
        int currG = color.green() - palColor.green();
        int currB = color.blue() - palColor.blue();
        int curr = redWeight * currR * currR + greenWeight * currG * currG + blueWeight * currB * currB;
        palOptions[palColor.index()] = curr;
    }
}
static void getPalColorWeightedQuad(const std::vector<PaletteColor> &dynColors, QColor color, std::array<int, NUM_COLORS> &palOptions)
{
    for (const PaletteColor &palColor : dynColors) {
        int currR = color.red() - palColor.red();
        int currG = color.green() - palColor.green();
        int currB = color.blue() - palColor.blue();
        double curr = redWeight * currR * currR * currR * currR + greenWeight * currG * currG * currG * currG + blueWeight * currB * currB * currB * currB;
        palOptions[palColor.index()] = sqrt(curr);
    }
}
static void getPalColorNew(const std::vector<PaletteColor> &dynColors, QColor color, std::array<int, NUM_COLORS> &palOptions)
{
    int minC = color.red();
    if (color.green() < minC) {
        minC = color.green();
    }
    if (color.blue() < minC) {
        minC = color.blue();
    }
    for (const PaletteColor &palColor : dynColors) {
        int minP = palColor.red();
        if (palColor.green() < minP) {
            minP = palColor.green();
        }
        if (palColor.blue() < minP) {
            minP = palColor.blue();
        }
        int currR = (color.red() - minC) - (palColor.red() - minP);
        int currG = (color.green() - minC) - (palColor.green() - minP);
        int currG = (color.blue() - minC) - (palColor.blue() - minP);
        int currL = minC - minP;
        int curr = redWeight * currR * currR + greenWeight * currG * currG + blueWeight * currB * currB + lightWeight * currL * currL;
        palOptions[palColor.index()] = curr;
    }
}

static BYTE selectColor(BYTE colorIdx, int shade, double stepsIn, bool deltaSteps, double stepsMpl, const std::array<bool, NUM_COLORS> &dynColors, const std::vector<D1Pal *> &pals)
{
    std::vector<std::array<int, NUM_COLORS>> options;

    for (const D1Pal *pal : pals) {
        QColor color = pal->getColor(colorIdx);
        std::array<int, NUM_COLORS> palOptions = { 0 };
        if (color == pal->getUndefinedColor()) {
            options.push_back(palOptions);
            continue;
        }

        // auto v = color.valueF();
        auto v = color.lightnessF();
        // TODO: use color.lightnessF() instead?

        auto steps = v * (MAXDARKNESS + 1);
        if (stepsIn != 0) {
            if (deltaSteps) {
                steps += stepsIn;
            } else {
                steps = stepsIn;
            }
        }
        steps *= stepsMpl;
        if (steps <= shade) {
            color = QColorConstants::Black;
        } else {
            color = color.darker(100 * steps / (steps - shade));
        }

        std::vector<PaletteColor> dynPalColors;
        pal->getValidColors(dynPalColors);
        for (auto it = dynPalColors.begin(); it != dynPalColors.end(); ) {
            if (dynColors[it->index()]) {
                it++;
            } else {
                it = dynPalColors.erase(it);
            }
        }
        for (unsigned i = 0; i < NUM_COLORS; i++) {
            palOptions[i] = INT_MAX;
        }
        colorSelector(dynPalColors, color, palOptions);

        options.push_back(palOptions);
    }

    int best = INT_MAX;
    BYTE res = 0;
    for (unsigned i = 0; i < NUM_COLORS; i++) {
        int curr = 0;
        for (const auto palOptions : options) {
            int dist = palOptions[i];
            if (dist >= INT_MAX - curr) {
                curr = INT_MAX;
                break;
            }
            curr += dist;
        }
        if (curr < best) {
            res = i;
            best = curr;
        }
    }
    return res;
}

static void MakeLightTableCustom(const GenerateTrnParam &params)
{
    unsigned i, j, k;

    std::array<bool, NUM_COLORS> dynColors;
    for (unsigned i = 0; i < NUM_COLORS; i++) {
        dynColors[i] = true;
    }

    for (k = 0; k < NUM_COLORS; k++) {
        for (j = 0; j < params.colors.size(); j++) {
            for (int c = params.colors[j].firstcolor; c <= params.colors[j].lastcolor && params.colors[j].protcolor; c++) {
                dynColors[c] = false;
            }
        }
    }

    D1Pal *basePal = params.pals.front();
    for (i = 0; i <= MAXDARKNESS; i++) {
        for (k = 0; k < NUM_COLORS; k++) {
            for (j = 0; j < params.colors.size(); j++) {
                if ((int)k < params.colors[j].firstcolor || (int)k > params.colors[j].lastcolor) {
                    continue;
                }
                if (params.colors[j].shadesteps < 0) {
                    ColorTrns[i][k] = k;
                } else if (params.colors[j].shadesteps == 0 && params.colors[j].shadestepsmpl == 1.0) {
                    int numColors = params.colors[j].lastcolor - params.colors[j].firstcolor + 1;
                    int col = (numColors * i) / (MAXDARKNESS + 1);
                    QColor colorFirst = basePal->getColor(params.colors[j].firstcolor);
                    QColor colorLast = basePal->getColor(params.colors[j].lastcolor);
                    if (colorFirst.lightness() > colorLast.lightness()) {
                        col = k + col;
                        if (col > params.colors[j].lastcolor) {
                            col = 0;
                        }
                    } else {
                        col = k - col;
                        if (col < params.colors[j].firstcolor) {
                            col = 0;
                        }
                    }
                    ColorTrns[i][k] = col;
                } else {
                    ColorTrns[i][k] = selectColor(k, i, params.colors[j].shadesteps, params.colors[j].deltasteps, params.colors[j].shadestepsmpl, dynColors, params.pals);
                }
                break;
            }
            if (j != params.colors.size()) {
                continue;
            }
            ColorTrns[i][k] = selectColor(k, i, 0.0, false, 1.0, dynColors, params.pals);
        }
    }
}

void D1Trs::generateLightTranslations(const GenerateTrnParam &params, D1Pal *pal, std::vector<D1Trn *> &trns)
{
    redWeight = params.redWeight;
    greenWeight = params.greenWeight;
    blueWeight = params.blueWeight;
    lightWeight = params.lightWeight;
    switch (params.colorSelector) {
    case 0:
        colorSelector = getPalColorSquare;
        break;
    case 1:
        colorSelector = getPalColorQuad;
        break;
    case 2:
        colorSelector = getPalColorRiemersma;
        break;
    case 3:
        colorSelector = getPalColorWeightedSquare;
        break;
    case 4:
        colorSelector = getPalColorWeightedQuad;
        break;
    case 5:
        colorSelector = getPalColorNew;
        break;
    }

    MakeLightTableCustom(params);

    QString filePath = QApplication::tr("Light%1.trn");
    for (int i = 0; i <= MAXDARKNESS; i++) {
        D1Trn *trn = new D1Trn();
        for (unsigned n = 0; n < D1TRN_TRANSLATIONS; n++) {
            trn->setTranslation(n, ColorTrns[i][n]);
        }
        QString trnFilePath = filePath.arg(i, 2, 10, QChar('0'));
        trn->setFilePath(trnFilePath);
        trn->setPalette(pal);
        trn->refreshResultingPalette();
        trns.push_back(trn);
    }
}
