#include "d1tileset.h"

#include <QApplication>

#include "d1celtileset.h"
#include "progressdialog.h"

D1Tileset::D1Tileset(D1Gfx *g)
    : gfx(g)
{
    this->min = new D1Min();
    this->til = new D1Til();
    this->sol = new D1Sol();
    this->amp = new D1Amp();
    this->tmi = new D1Tmi();
}

D1Tileset::~D1Tileset()
{
    delete min;
    delete til;
    delete sol;
    delete amp;
    delete tmi;
}

bool D1Tileset::load(const OpenAsParam &params)
{
    QString prevFilePath = this->gfx->getFilePath();

    // TODO: use in MainWindow::openFile?
    QString gfxFilePath = params.celFilePath;
    QString tilFilePath = params.tilFilePath;
    QString minFilePath = params.minFilePath;
    QString solFilePath = params.solFilePath;
    QString ampFilePath = params.ampFilePath;
    QString tmiFilePath = params.tmiFilePath;

    if (!gfxFilePath.isEmpty()) {
        QFileInfo celFileInfo = QFileInfo(gfxFilePath);

        QString basePath = celFileInfo.absolutePath() + "/" + celFileInfo.completeBaseName();

        if (tilFilePath.isEmpty()) {
            tilFilePath = basePath + ".til";
        }
        if (minFilePath.isEmpty()) {
            minFilePath = basePath + ".min";
        }
        if (solFilePath.isEmpty()) {
            solFilePath = basePath + ".sol";
        }
        if (ampFilePath.isEmpty()) {
            ampFilePath = basePath + ".amp";
        }
        if (tmiFilePath.isEmpty()) {
            tmiFilePath = basePath + ".tmi";
        }
    }

    std::map<unsigned, D1CEL_FRAME_TYPE> celFrameTypes;
    if (!this->sol->load(solFilePath)) {
        dProgressErr() << QApplication::tr("Failed loading SOL file: %1.").arg(QDir::toNativeSeparators(solFilePath));
    } else if (!this->min->load(minFilePath, this->gfx, this->sol, celFrameTypes, params)) {
        dProgressErr() << QApplication::tr("Failed loading MIN file: %1.").arg(QDir::toNativeSeparators(minFilePath));
    } else if (!this->til->load(tilFilePath, this->min)) {
        dProgressErr() << QApplication::tr("Failed loading TIL file: %1.").arg(QDir::toNativeSeparators(tilFilePath));
    } else if (!this->amp->load(ampFilePath, this->til->getTileCount(), params)) {
        dProgressErr() << QApplication::tr("Failed loading AMP file: %1.").arg(QDir::toNativeSeparators(ampFilePath));
    } else if (!this->tmi->load(tmiFilePath, this->sol, params)) {
        dProgressErr() << QApplication::tr("Failed loading TMI file: %1.").arg(QDir::toNativeSeparators(tmiFilePath));
    } else if (!D1CelTileset::load(*this->gfx, celFrameTypes, gfxFilePath, params)) {
        dProgressErr() << QApplication::tr("Failed loading Tileset-CEL file: %1.").arg(QDir::toNativeSeparators(gfxFilePath));
    } else {
        return !gfxFilePath.isEmpty() || !prevFilePath.isEmpty();
    }
    // clear possible inconsistent data
    // this->gfx->clear();
    this->min->clear();
    this->til->clear();
    this->sol->clear();
    this->amp->clear();
    this->tmi->clear();
    return false;
}

void D1Tileset::save(const SaveAsParam &params)
{
    this->min->save(params);
    this->til->save(params);
    this->sol->save(params);
    this->amp->save(params);
    this->tmi->save(params);
}

void D1Tileset::createSubtile()
{
    this->min->createSubtile();
    this->sol->createSubtile();
    this->tmi->createSubtile();
}

void D1Tileset::removeSubtile(int subtileIndex, int replacement)
{
    this->til->removeSubtile(subtileIndex, replacement);
    this->sol->removeSubtile(subtileIndex);
    this->tmi->removeSubtile(subtileIndex);
}

bool D1Tileset::reuseFrames(std::set<int> &removedIndices, bool silent)
{
    QString label = QApplication::tr("Reusing frames...");
    ProgressDialog::incBar(silent ? QStringLiteral("") : label, this->gfx->getFrameCount());

    QPair<int, QString> progress = { -1, label };
    if (silent) {
        dProgress() << progress;
    }
    int result = 0;
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        if (ProgressDialog::wasCanceled()) {
            result |= 2;
            break;
        }
        for (int j = i + 1; j < this->gfx->getFrameCount(); j++) {
            D1GfxFrame *frame0 = this->gfx->getFrame(i);
            D1GfxFrame *frame1 = this->gfx->getFrame(j);
            int width = frame0->getWidth();
            int height = frame0->getHeight();
            if (width != frame1->getWidth() || height != frame1->getHeight()) {
                continue; // should not happen, but better safe than sorry
            }
            bool match = true;
            for (int y = 0; y < height && match; y++) {
                for (int x = 0; x < width; x++) {
                    if (frame0->getPixel(x, y) == frame1->getPixel(x, y)) {
                        continue;
                    }
                    match = false;
                    break;
                }
            }
            if (!match) {
                continue;
            }
            // use frame0 instead of frame1
            this->min->removeFrame(j, i + 1);
            // calculate the original indices
            int originalIndexI = i;
            for (auto iter = removedIndices.cbegin(); iter != removedIndices.cend(); ++iter) {
                if (*iter <= originalIndexI) {
                    originalIndexI++;
                    continue;
                }
                break;
            }
            int originalIndexJ = j;
            for (auto iter = removedIndices.cbegin(); iter != removedIndices.cend(); ++iter) {
                if (*iter <= originalIndexJ) {
                    originalIndexJ++;
                    continue;
                }
                break;
            }
            removedIndices.insert(originalIndexJ);
            if (!silent) {
                dProgress() << QApplication::tr("Using frame %1 instead of %2.").arg(originalIndexI + 1).arg(originalIndexJ + 1);
            }
            result = 1;
            j--;
        }
        if (!ProgressDialog::incValue()) {
            result |= 2;
            break;
        }
    }
    auto amount = removedIndices.size();
    progress.second = QString(QApplication::tr("Reused %n frame(s).", "", amount)).arg(amount);
    dProgress() << progress;

    ProgressDialog::decBar();
    return result != 0;
}

bool D1Tileset::reuseSubtiles(std::set<int> &removedIndices)
{
    ProgressDialog::incBar(QApplication::tr("Reusing subtiles..."), this->min->getSubtileCount());
    int result = 0;
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        if (ProgressDialog::wasCanceled()) {
            result |= 2;
            break;
        }
        for (int j = i + 1; j < this->min->getSubtileCount(); j++) {
            std::vector<unsigned> &frameReferences0 = this->min->getFrameReferences(i);
            std::vector<unsigned> &frameReferences1 = this->min->getFrameReferences(j);
            if (frameReferences0.size() != frameReferences1.size()) {
                continue; // should not happen, but better safe than sorry
            }
            bool match = true;
            for (unsigned x = 0; x < frameReferences0.size(); x++) {
                if (frameReferences0[x] == frameReferences1[x]) {
                    continue;
                }
                match = false;
                break;
            }
            if (!match) {
                continue;
            }
            // use subtile 'i' instead of subtile 'j'
            this->removeSubtile(j, i);
            // calculate the original indices
            int originalIndexI = i;
            for (auto iter = removedIndices.cbegin(); iter != removedIndices.cend(); ++iter) {
                if (*iter <= originalIndexI) {
                    originalIndexI++;
                    continue;
                }
                break;
            }
            int originalIndexJ = j;
            for (auto iter = removedIndices.cbegin(); iter != removedIndices.cend(); ++iter) {
                if (*iter <= originalIndexJ) {
                    originalIndexJ++;
                    continue;
                }
                break;
            }
            removedIndices.insert(originalIndexJ);
            dProgress() << QApplication::tr("Using subtile %1 instead of %2.").arg(originalIndexI + 1).arg(originalIndexJ + 1);
            result = true;
            j--;
        }
        if (!ProgressDialog::incValue()) {
            result |= 2;
            break;
        }
    }
    auto amount = removedIndices.size();
    dProgress() << QString(QApplication::tr("Reused %n subtile(s).", "", amount)).arg(amount);

    ProgressDialog::decBar();
    return result != 0;
}

#define Blk2Mcr(n, x) RemoveMicro(min, n, x, deletedFrames, silent);
static void RemoveMicro(D1Min *min, int subtileRef, int microIndex, std::set<unsigned> &deletedFrames, bool silent)
{
    int subtileIndex = subtileRef - 1;
    std::vector<unsigned> &frameReferences = min->getFrameReferences(subtileIndex);
    // assert(min->getSubtileWidth() == 2);
    int index = frameReferences.size() - (2 + (microIndex & ~1)) + (microIndex & 1);
    unsigned frameReference = frameReferences[index];
    if (frameReference != 0) {
        deletedFrames.insert(frameReference);
        min->setFrameReference(subtileIndex, index, 0);
        if (!silent) {
            dProgress() << QApplication::tr("Removed frame (%1) @%2 in Subtile %3.").arg(frameReference).arg(index + 1).arg(subtileIndex + 1);
        }
    } else {
        dProgressWarn() << QApplication::tr("The frame @%1 in Subtile %2 is already empty.").arg(index + 1).arg(subtileIndex + 1);
    }
}

void D1Tileset::patch(int dunType, bool silent)
{
    std::set<unsigned> deletedFrames;
    switch (dunType) {
    case DTYPE_TOWN:
        // patch dMiniTiles - Town.MIN
        // pointless tree micros (re-drawn by dSpecial)
        Blk2Mcr(117, 3);
        Blk2Mcr(117, 5);
        Blk2Mcr(128, 2);
        Blk2Mcr(128, 3);
        Blk2Mcr(128, 4);
        Blk2Mcr(128, 5);
        Blk2Mcr(128, 6);
        Blk2Mcr(128, 7);
        Blk2Mcr(129, 3);
        Blk2Mcr(129, 5);
        Blk2Mcr(129, 7);
        Blk2Mcr(130, 2);
        Blk2Mcr(130, 4);
        Blk2Mcr(130, 6);
        Blk2Mcr(156, 2);
        Blk2Mcr(156, 3);
        Blk2Mcr(156, 4);
        Blk2Mcr(156, 5);
        Blk2Mcr(156, 6);
        Blk2Mcr(156, 7);
        Blk2Mcr(156, 8);
        Blk2Mcr(156, 9);
        Blk2Mcr(156, 10);
        Blk2Mcr(156, 11);
        Blk2Mcr(157, 3);
        Blk2Mcr(157, 5);
        Blk2Mcr(157, 7);
        Blk2Mcr(157, 9);
        Blk2Mcr(157, 11);
        Blk2Mcr(158, 2);
        Blk2Mcr(158, 4);
        Blk2Mcr(160, 2);
        Blk2Mcr(160, 3);
        Blk2Mcr(160, 4);
        Blk2Mcr(160, 5);
        Blk2Mcr(160, 6);
        Blk2Mcr(160, 7);
        Blk2Mcr(160, 8);
        Blk2Mcr(160, 9);
        Blk2Mcr(162, 2);
        Blk2Mcr(162, 4);
        Blk2Mcr(162, 6);
        Blk2Mcr(162, 8);
        Blk2Mcr(162, 10);
        Blk2Mcr(212, 3);
        Blk2Mcr(212, 4);
        Blk2Mcr(212, 5);
        Blk2Mcr(212, 6);
        Blk2Mcr(212, 7);
        Blk2Mcr(212, 8);
        Blk2Mcr(212, 9);
        Blk2Mcr(212, 10);
        Blk2Mcr(212, 11);
        // Blk2Mcr(214, 4);
        // Blk2Mcr(214, 6);
        Blk2Mcr(216, 2);
        Blk2Mcr(216, 4);
        Blk2Mcr(216, 6);
        // Blk2Mcr(217, 4);
        // Blk2Mcr(217, 6);
        // Blk2Mcr(217, 8);
        // Blk2Mcr(358, 4);
        // Blk2Mcr(358, 5);
        // Blk2Mcr(358, 6);
        // Blk2Mcr(358, 7);
        // Blk2Mcr(358, 8);
        // Blk2Mcr(358, 9);
        // Blk2Mcr(358, 10);
        // Blk2Mcr(358, 11);
        // Blk2Mcr(358, 12);
        // Blk2Mcr(358, 13);
        // Blk2Mcr(360, 4);
        // Blk2Mcr(360, 6);
        // Blk2Mcr(360, 8);
        // Blk2Mcr(360, 10);
        // fix bad artifact
        Blk2Mcr(233, 6);
        // useless black micros
        Blk2Mcr(426, 1);
        Blk2Mcr(427, 0);
        Blk2Mcr(427, 1);
        Blk2Mcr(429, 1);
        // fix bad artifacts
        Blk2Mcr(828, 12);
        Blk2Mcr(828, 13);
        Blk2Mcr(1018, 2);
        // useless black micros
        Blk2Mcr(1143, 0);
        Blk2Mcr(1145, 0);
        Blk2Mcr(1145, 1);
        Blk2Mcr(1146, 0);
        Blk2Mcr(1153, 0);
        Blk2Mcr(1155, 1);
        Blk2Mcr(1156, 0);
        Blk2Mcr(1169, 1);
        Blk2Mcr(1170, 0);
        Blk2Mcr(1170, 1);
        Blk2Mcr(1172, 1);
        Blk2Mcr(1176, 1);
        Blk2Mcr(1199, 1);
        Blk2Mcr(1200, 0);
        Blk2Mcr(1200, 1);
        Blk2Mcr(1202, 1);
        Blk2Mcr(1203, 1);
        Blk2Mcr(1205, 1);
        Blk2Mcr(1212, 0);
        Blk2Mcr(1219, 0);
        if (this->min->getSubtileCount() > 1258) {
            // #ifdef HELLFIRE
            // fix bad artifacts
            Blk2Mcr(1273, 7);
            Blk2Mcr(1303, 7);
        }
        break;
    case DTYPE_CATHEDRAL:
        // patch dMiniTiles - L1.MIN
        // useless black micros
        Blk2Mcr(107, 0);
        Blk2Mcr(107, 1);
        Blk2Mcr(109, 1);
        Blk2Mcr(137, 1);
        Blk2Mcr(138, 0);
        Blk2Mcr(138, 1);
        Blk2Mcr(140, 1);
        break;
    case DTYPE_CATACOMBS:
        break;
    case DTYPE_CAVES:
        // patch dMiniTiles - L3.MIN
        // fix bad artifact
        Blk2Mcr(82, 4);
        break;
    case DTYPE_HELL:
        break;
    case DTYPE_NEST:
        // patch dMiniTiles - L6.MIN
        // useless black micros
        Blk2Mcr(21, 0);
        Blk2Mcr(21, 1);
        // fix bad artifacts
        Blk2Mcr(132, 7);
        Blk2Mcr(366, 1);
        break;
    case DTYPE_CRYPT:
        // patch dMiniTiles - L5.MIN
        // useless black micros
        Blk2Mcr(130, 0);
        Blk2Mcr(130, 1);
        Blk2Mcr(132, 1);
        Blk2Mcr(134, 0);
        Blk2Mcr(134, 1);
        Blk2Mcr(149, 0);
        Blk2Mcr(149, 1);
        Blk2Mcr(149, 2);
        Blk2Mcr(150, 0);
        Blk2Mcr(150, 1);
        Blk2Mcr(150, 2);
        Blk2Mcr(150, 4);
        Blk2Mcr(151, 0);
        Blk2Mcr(151, 1);
        Blk2Mcr(151, 3);
        Blk2Mcr(152, 0);
        Blk2Mcr(152, 1);
        Blk2Mcr(152, 3);
        Blk2Mcr(152, 5);
        Blk2Mcr(153, 0);
        Blk2Mcr(153, 1);
        // fix bad artifact
        Blk2Mcr(156, 2);
        // useless black micros
        Blk2Mcr(172, 0);
        Blk2Mcr(172, 1);
        Blk2Mcr(172, 2);
        Blk2Mcr(173, 0);
        Blk2Mcr(173, 1);
        Blk2Mcr(174, 0);
        Blk2Mcr(174, 1);
        Blk2Mcr(174, 2);
        Blk2Mcr(174, 4);
        Blk2Mcr(175, 0);
        Blk2Mcr(175, 1);
        Blk2Mcr(176, 0);
        Blk2Mcr(176, 1);
        Blk2Mcr(176, 3);
        Blk2Mcr(177, 0);
        Blk2Mcr(177, 1);
        Blk2Mcr(177, 3);
        Blk2Mcr(177, 5);
        Blk2Mcr(178, 0);
        Blk2Mcr(178, 1);
        Blk2Mcr(179, 0);
        Blk2Mcr(179, 1);
        break;
    }
    for (auto it = deletedFrames.crbegin(); it != deletedFrames.crend(); it++) {
        unsigned refIndex = *it;
        this->gfx->removeFrame(refIndex - 1);
        // shift references
        // - shift frame indices of the subtiles
        for (int i = 0; i < this->min->getSubtileCount(); i++) {
            std::vector<unsigned> &frameReferences = this->min->getFrameReferences(i);
            for (unsigned n = 0; n < frameReferences.size(); n++) {
                if (frameReferences[n] >= refIndex) {
                    if (frameReferences[n] == refIndex) {
                        frameReferences[n] = 0;
                        dProgressErr() << QApplication::tr("Frame %1 was removed, but it was still used in Subtile %2 @ %3").arg(refIndex).arg(i + 1).arg(n + 1);
                    } else {
                        frameReferences[n] -= 1;
                    }
                }
            }
        }
    }
}
