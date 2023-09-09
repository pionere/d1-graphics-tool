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

static void getPalColor(const std::vector<PaletteColor> &dynColors, QColor color, std::array<int, NUM_COLORS> &palOptions)
{
    for (const PaletteColor &palColor : dynColors) {
        int currR = color.red() - palColor.red();
        int currG = color.green() - palColor.green();
        int currB = color.blue() - palColor.blue();
        int curr = currR * currR + currG * currG + currB * currB;
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
        getPalColor(dynPalColors, color, palOptions);

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
