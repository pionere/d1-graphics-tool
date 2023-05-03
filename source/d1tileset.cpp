#include "d1tileset.h"

#include <QApplication>
#include <QMessageBox>

#include "d1cel.h"
#include "d1celtileset.h"
#include "progressdialog.h"

D1Tileset::D1Tileset(D1Gfx *g)
    : gfx(g)
{
    this->cls = new D1Gfx();
    this->cls->setPalette(g->getPalette());
    this->min = new D1Min();
    this->til = new D1Til();
    this->sol = new D1Sol();
    this->amp = new D1Amp();
    this->spt = new D1Spt();
    this->tmi = new D1Tmi();
}

D1Tileset::~D1Tileset()
{
    delete cls;
    delete min;
    delete til;
    delete sol;
    delete amp;
    delete spt;
    delete tmi;
}

bool D1Tileset::loadCls(const QString &clsFilePath, const OpenAsParam &params)
{
    if (QFileInfo::exists(clsFilePath)) {
        return D1Cel::load(*this->cls, clsFilePath, params);
    }

    this->cls->setFilePath(clsFilePath);
    this->cls->setModified(false);
    return params.clsFilePath.isEmpty();
}

bool D1Tileset::load(const OpenAsParam &params)
{
    // QString prevFilePath = this->gfx->getFilePath();

    // TODO: use in MainWindow::openFile?
    QString gfxFilePath = params.celFilePath;
    QString clsFilePath = params.clsFilePath;
    QString tilFilePath = params.tilFilePath;
    QString minFilePath = params.minFilePath;
    QString solFilePath = params.solFilePath;
    QString ampFilePath = params.ampFilePath;
    QString sptFilePath = params.sptFilePath;
    QString tmiFilePath = params.tmiFilePath;

    if (!gfxFilePath.isEmpty()) {
        QFileInfo celFileInfo = QFileInfo(gfxFilePath);

        QString basePath = celFileInfo.absolutePath() + "/" + celFileInfo.completeBaseName();

        if (clsFilePath.isEmpty()) {
            clsFilePath = basePath + "s.cel";
        }
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
        if (sptFilePath.isEmpty()) {
            sptFilePath = basePath + ".spt";
        }
        if (tmiFilePath.isEmpty()) {
            tmiFilePath = basePath + ".tmi";
        }
    }

    std::map<unsigned, D1CEL_FRAME_TYPE> celFrameTypes;
    if (!this->sol->load(solFilePath)) {
        dProgressErr() << QApplication::tr("Failed loading SOL file: %1.").arg(QDir::toNativeSeparators(solFilePath));
    } else if (!this->min->load(minFilePath, this, celFrameTypes, params)) {
        dProgressErr() << QApplication::tr("Failed loading MIN file: %1.").arg(QDir::toNativeSeparators(minFilePath));
    } else if (!this->til->load(tilFilePath, this->min)) {
        dProgressErr() << QApplication::tr("Failed loading TIL file: %1.").arg(QDir::toNativeSeparators(tilFilePath));
    } else if (!this->amp->load(ampFilePath, this->til->getTileCount(), params)) {
        dProgressErr() << QApplication::tr("Failed loading AMP file: %1.").arg(QDir::toNativeSeparators(ampFilePath));
    } else if (!this->spt->load(sptFilePath, this->sol, params)) {
        dProgressErr() << QApplication::tr("Failed loading SPT file: %1.").arg(QDir::toNativeSeparators(sptFilePath));
    } else if (!this->tmi->load(tmiFilePath, this->sol, params)) {
        dProgressErr() << QApplication::tr("Failed loading TMI file: %1.").arg(QDir::toNativeSeparators(tmiFilePath));
    } else if (!this->loadCls(clsFilePath, params)) {
        dProgressErr() << QApplication::tr("Failed loading Special-CEL file: %1.").arg(QDir::toNativeSeparators(clsFilePath));
    } else if (!D1CelTileset::load(*this->gfx, celFrameTypes, gfxFilePath, params)) {
        dProgressErr() << QApplication::tr("Failed loading Tileset-CEL file: %1.").arg(QDir::toNativeSeparators(gfxFilePath));
    } else {
        return true; // !gfxFilePath.isEmpty() || !prevFilePath.isEmpty();
    }
    // clear possible inconsistent data
    // this->gfx->clear();
    // this->cls->clear();
    this->min->clear();
    this->til->clear();
    this->sol->clear();
    this->amp->clear();
    this->spt->clear();
    this->tmi->clear();
    return true;
}

void D1Tileset::save(const SaveAsParam &params)
{
    // this->cls->save(params);
    this->min->save(params);
    this->til->save(params);
    this->sol->save(params);
    this->amp->save(params);
    this->spt->save(params);
    this->tmi->save(params);
}

void D1Tileset::createTile()
{
    this->til->createTile();
    this->amp->createTile();
}

void D1Tileset::insertSubtile(int subtileIndex, const std::vector<unsigned> &frameReferencesList)
{
    this->min->insertSubtile(subtileIndex, frameReferencesList);
    this->sol->insertSubtile(subtileIndex);
    this->spt->insertSubtile(subtileIndex);
    this->tmi->insertSubtile(subtileIndex);
}

void D1Tileset::createSubtile()
{
    this->min->createSubtile();
    this->sol->createSubtile();
    this->spt->createSubtile();
    this->tmi->createSubtile();
}

void D1Tileset::removeSubtile(int subtileIndex, int replacement)
{
    this->til->removeSubtile(subtileIndex, replacement);
    this->sol->removeSubtile(subtileIndex);
    this->spt->removeSubtile(subtileIndex);
    this->tmi->removeSubtile(subtileIndex);
}

void D1Tileset::resetSubtileFlags(int subtileIndex)
{
    this->sol->setSubtileProperties(subtileIndex, 0);
    this->spt->setSubtileTrapProperty(subtileIndex, 0);
    this->spt->setSubtileSpecProperty(subtileIndex, 0);
    this->tmi->setSubtileProperties(subtileIndex, 0);
}

void D1Tileset::remapSubtiles(const std::map<unsigned, unsigned> &remap)
{
    this->min->remapSubtiles(remap);
    this->sol->remapSubtiles(remap);
    this->spt->remapSubtiles(remap);
    this->tmi->remapSubtiles(remap);
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

#define Blk2Mcr(n, x) RemoveFrame(min, n, x, deletedFrames, silent);
#define MICRO_IDX(blockSize, microIndex) ((blockSize) - (2 + ((microIndex) & ~1)) + ((microIndex)&1))
static void RemoveFrame(D1Min *min, int subtileRef, int microIndex, std::set<unsigned> &deletedFrames, bool silent)
{
    int subtileIndex = subtileRef - 1;
    std::vector<unsigned> &frameReferences = min->getFrameReferences(subtileIndex);
    // assert(min->getSubtileWidth() == 2);
    unsigned index = MICRO_IDX(frameReferences.size(), microIndex);
    if (index >= frameReferences.size()) {
        dProgressErr() << QApplication::tr("Not enough frames in Subtile %1.").arg(subtileIndex + 1);
        return;
    }
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

static void ReplaceSubtile(D1Til *til, int tileIndex, unsigned index, int subtileRef, bool silent)
{
    int subtileIndex = subtileRef - 1;
    std::vector<int> &tilSubtiles = til->getSubtileIndices(tileIndex);
    if (index >= tilSubtiles.size()) {
        dProgressErr() << QApplication::tr("Not enough subtiles in Tile %1.").arg(tileIndex + 1);
        return;
    }
    unsigned subtileReference = tilSubtiles[index];
    if (til->setSubtileIndex(tileIndex, index, subtileIndex)) {
        if (!silent) {
            dProgress() << QApplication::tr("Subtile %1 of Tile%2 is set to %3.").arg(index + 1).arg(tileIndex + 1).arg(subtileIndex + 1);
        }
    } else {
        dProgressWarn() << QApplication::tr("Subtile %1 of Tile%2 is already %3.").arg(index + 1).arg(tileIndex + 1).arg(subtileIndex + 1);
    }
}

static void CopyFrame(D1Min *min, D1Gfx *gfx, int dstSubtileRef, int dstMicroIndex, int srcSubtileRef, int srcMicroIndex, bool silent)
{
    int srcSubtileIndex = srcSubtileRef - 1;
    std::vector<unsigned> &srcFrameReferences = min->getFrameReferences(srcSubtileIndex);
    // assert(min->getSubtileWidth() == 2);
    unsigned srcIndex = MICRO_IDX(srcFrameReferences.size(), srcMicroIndex);
    if (srcIndex >= srcFrameReferences.size()) {
        dProgressErr() << QApplication::tr("Not enough frames in Subtile %1.").arg(srcSubtileIndex + 1);
        return;
    }
    unsigned srcFrameRef = srcFrameReferences[srcIndex];
    if (srcFrameRef == 0) {
        dProgressErr() << QApplication::tr("Frame %1 of Subtile %2 is empty.").arg(srcIndex + 1).arg(srcSubtileIndex + 1);
        return;
    }

    int dstSubtileIndex = dstSubtileRef - 1;
    std::vector<unsigned> &dstFrameReferences = min->getFrameReferences(dstSubtileIndex);
    // assert(min->getSubtileWidth() == 2);
    unsigned dstIndex = MICRO_IDX(dstFrameReferences.size(), dstMicroIndex);
    if (dstIndex >= dstFrameReferences.size()) {
        dProgressErr() << QApplication::tr("Not enough frames in Subtile %1.").arg(dstSubtileIndex + 1);
        return;
    }
    if (min->setFrameReference(dstSubtileIndex, dstIndex, srcFrameRef)) {
        if (!silent) {
            dProgress() << QApplication::tr("Frame %1 of Subtile %2 is set to Frame %3.").arg(dstIndex + 1).arg(dstSubtileIndex + 1).arg(srcFrameRef);
        }
    }
    /*unsigned dstFrameRef = dstFrameReferences[dstIndex];
    if (dstFrameRef == 0) {
        dstFrameReferences[dstIndex] = srcFrameRef;
        if (!silent) {
            dProgress() << QApplication::tr("Frame %1 of Subtile %2 is set to Frame %3.").arg(dstIndex + 1).arg(dstSubtileIndex + 1).arg(srcFrameRef);
        }
        return;
    }

    D1GfxFrame *frameDst = gfx->getFrame(dstFrameRef - 1);
    const D1GfxFrame *frameSrc = gfx->getFrame(srcFrameRef - 1);
    if (frameDst->getWidth() != frameSrc->getWidth() || frameDst->getHeight() != frameSrc->getHeight()) {
        dProgressErr() << QApplication::tr("Mismatching frame-dimensions. %1:%2 vs %3:%4 in frame %5 and %6").arg(frameDst->getWidth()).arg(frameDst->getHeight()).arg(frameSrc->getWidth()).arg(frameSrc->getHeight()).arg(dstFrameRef).arg(srcFrameRef);
        return;
    }
    bool change = false;
    for (int x = 0; x < frameSrc->getWidth(); x++) {
        for (int y = 0; y < frameSrc->getHeight(); y++) {
            change |= frameDst->setPixel(x, y, frameSrc->getPixel(x, y));
        }
    }
    if (change) {
        if (!silent) {
            dProgress() << QApplication::tr("The content of Frame %1 overwritten by Frame %2.").arg(dstFrameRef).arg(srcFrameRef);
        }
    } else {
        dProgressWarn() << QApplication::tr("The contents of Frame %1 and Frame %2 are already the same.").arg(dstFrameRef).arg(srcFrameRef);
    }*/
}

void D1Tileset::patchTownPot(int potLeftSubtileRef, int potRightSubtileRef, bool silent)
{
    std::vector<unsigned> &leftFrameReferences = this->min->getFrameReferences(potLeftSubtileRef - 1);
    std::vector<unsigned> &rightFrameReferences = this->min->getFrameReferences(potRightSubtileRef - 1);

    unsigned blockSize = leftFrameReferences.size();
    unsigned leftIndex0 = MICRO_IDX(blockSize, 1);
    unsigned leftFrameRef0 = leftFrameReferences[leftIndex0];
    unsigned leftIndex1 = MICRO_IDX(blockSize, 3);
    unsigned leftFrameRef1 = leftFrameReferences[leftIndex1];
    unsigned leftIndex2 = MICRO_IDX(blockSize, 5);
    unsigned leftFrameRef2 = leftFrameReferences[leftIndex2];

    if (leftFrameRef1 == 0 || leftFrameRef2 == 0) {
        // TODO: report error if not empty both? + additional checks
        return; // left frames are empty -> assume it is already done
    }
    if (leftFrameRef0 == 0) {
        dProgressErr() << QApplication::tr("Invalid (empty) pot floor subtile (%1).").arg(potLeftSubtileRef);
        return;
    }

    D1GfxFrame *frameLeft0 = this->gfx->getFrame(leftFrameRef0 - 1);
    D1GfxFrame *frameLeft1 = this->gfx->getFrame(leftFrameRef1 - 1);
    D1GfxFrame *frameLeft2 = this->gfx->getFrame(leftFrameRef2 - 1);

    if (frameLeft0->getWidth() != MICRO_WIDTH || frameLeft0->getHeight() != MICRO_HEIGHT) {
        return; // upscaled(?) frames -> assume it is already done
    }
    if ((frameLeft1->getWidth() != MICRO_WIDTH || frameLeft1->getHeight() != MICRO_HEIGHT)
        || (frameLeft2->getWidth() != MICRO_WIDTH || frameLeft2->getHeight() != MICRO_HEIGHT)) {
        dProgressErr() << QApplication::tr("Invalid (mismatching frames) pot floor subtile (%1).").arg(potLeftSubtileRef);
        return;
    }

    if (rightFrameReferences.size() != blockSize) {
        dProgressErr() << QApplication::tr("Invalid (mismatching subtiles) pot floor subtiles (%1, %2).").arg(potLeftSubtileRef).arg(potRightSubtileRef);
        return;
    }

    unsigned rightIndex0 = MICRO_IDX(blockSize, 0);
    unsigned rightFrameRef0 = rightFrameReferences[rightIndex0];
    unsigned rightIndex1 = MICRO_IDX(blockSize, 2);
    unsigned rightFrameRef1 = rightFrameReferences[rightIndex1];
    unsigned rightIndex2 = MICRO_IDX(blockSize, 4);
    unsigned rightFrameRef2 = rightFrameReferences[rightIndex2];

    if (rightFrameRef1 != 0 || rightFrameRef2 != 0) {
        // TODO: report error if not empty both? + additional checks
        return; // right frames are not empty -> assume it is already done
    }
    if (rightFrameRef0 == 0) {
        dProgressErr() << QApplication::tr("Invalid (empty) pot floor subtile (%1).").arg(potRightSubtileRef);
        return;
    }

    // move the frames to the right side
    rightFrameReferences[rightIndex1] = leftFrameRef1;
    rightFrameReferences[rightIndex2] = leftFrameRef2;
    leftFrameReferences[leftIndex1] = 0;
    leftFrameReferences[leftIndex2] = 0;

    D1GfxFrame *frameRight0 = this->gfx->getFrame(rightFrameRef0 - 1);

    for (int x = MICRO_WIDTH / 2; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel = frameLeft2->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            frameLeft2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
            frameLeft2->setPixel(x, y, D1GfxPixel::transparentPixel());
        }
    }
    for (int x = MICRO_WIDTH / 2; x < MICRO_WIDTH; x++) {
        for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
            D1GfxPixel pixel = frameLeft1->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            frameLeft2->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
            frameLeft1->setPixel(x, y, D1GfxPixel::transparentPixel());
        }
    }
    for (int x = MICRO_WIDTH / 2; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel = frameLeft1->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            frameLeft1->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
            frameLeft1->setPixel(x, y, D1GfxPixel::transparentPixel());
        }
    }
    for (int x = MICRO_WIDTH / 2 + 2; x < MICRO_WIDTH - 4; x++) {
        for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
            D1GfxPixel pixel = frameLeft0->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            frameLeft1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
            frameLeft0->setPixel(x, y, D1GfxPixel::transparentPixel());
        }
    }
    for (int x = MICRO_WIDTH / 2 + 2; x < MICRO_WIDTH - 4; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT / 2 + 8; y++) {
            D1GfxPixel pixel = frameLeft0->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            frameRight0->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
            // frameLeft0->setPixel(x, y, D1GfxPixel::transparentPixel());
        }
    }
    // convert the left floor to triangle
    std::vector<FramePixel> pixels;
    D1CelTilesetFrame::collectPixels(frameLeft0, D1CEL_FRAME_TYPE::RightTriangle, pixels);
    for (const FramePixel framePixel : pixels) {
        if (framePixel.pixel.isTransparent()) {
            frameLeft0->setPixel(framePixel.pos.x(), framePixel.pos.y(), D1GfxPixel::colorPixel(0));
        } else {
            frameLeft0->setPixel(framePixel.pos.x(), framePixel.pos.y(), D1GfxPixel::transparentPixel());
        }
    }
    D1CelTilesetFrame::selectFrameType(frameLeft0);
    D1CelTilesetFrame::selectFrameType(frameRight0);
    if (frameLeft0->getFrameType() != D1CEL_FRAME_TYPE::RightTriangle) {
        dProgressErr() << QApplication::tr("Pot floor subtile (%1) is not triangle after patch.").arg(potLeftSubtileRef);
    } else if (!silent) {
        dProgress() << QApplication::tr("Frame %1 and %2 are modified and moved from subtile %3 to subtile %4.").arg(leftFrameRef1).arg(leftFrameRef2).arg(potLeftSubtileRef).arg(potRightSubtileRef);
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
        Blk2Mcr(214, 4);
        Blk2Mcr(214, 6);
        Blk2Mcr(216, 2);
        Blk2Mcr(216, 4);
        Blk2Mcr(216, 6);
        Blk2Mcr(217, 4);
        Blk2Mcr(217, 6);
        Blk2Mcr(217, 8);
        Blk2Mcr(358, 4);
        Blk2Mcr(358, 5);
        Blk2Mcr(358, 6);
        Blk2Mcr(358, 7);
        Blk2Mcr(358, 8);
        Blk2Mcr(358, 9);
        Blk2Mcr(358, 10);
        Blk2Mcr(358, 11);
        Blk2Mcr(358, 12);
        Blk2Mcr(358, 13);
        Blk2Mcr(360, 4);
        Blk2Mcr(360, 6);
        Blk2Mcr(360, 8);
        Blk2Mcr(360, 10);
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
        // patch dMicroCels - TOWN.CEL
        // - overwrite subtile 557 and 558 with subtile 939 and 940 to make the inner tile of Griswold's house non-walkable
        // CopyFrame(this->gfx, 557, 939, silent);
        // CopyFrame(this->gfx, 558, 940, silent);
        CopyFrame(this->min, this->gfx, 237, 0, 402, 0, silent);
        CopyFrame(this->min, this->gfx, 237, 1, 402, 1, silent);
        // patch subtiles around the pot of Adria to prevent graphical glitch when a player passes it
        patchTownPot(553, 554, silent);
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
        // patch dMegaTiles and dMiniTiles - L2.TIL, L2.MIN
        // reuse subtiles
        ReplaceSubtile(this->til, 41 - 1, 1, 135, silent);
        // add separate tiles and subtiles for the arches
        if (this->min->getSubtileCount() < 560)
            this->createSubtile();
        CopyFrame(this->min, this->gfx, 560, 0, 9, 0, silent);
        CopyFrame(this->min, this->gfx, 560, 1, 9, 1, silent);
        if (this->min->getSubtileCount() < 561)
            this->createSubtile();
        CopyFrame(this->min, this->gfx, 561, 0, 11, 0, silent);
        CopyFrame(this->min, this->gfx, 561, 1, 11, 1, silent);
        if (this->min->getSubtileCount() < 562)
            this->createSubtile();
        CopyFrame(this->min, this->gfx, 562, 0, 9, 0, silent);
        CopyFrame(this->min, this->gfx, 562, 1, 9, 1, silent);
        if (this->min->getSubtileCount() < 563)
            this->createSubtile();
        CopyFrame(this->min, this->gfx, 563, 0, 10, 0, silent);
        CopyFrame(this->min, this->gfx, 563, 1, 10, 1, silent);
        if (this->min->getSubtileCount() < 564)
            this->createSubtile();
        CopyFrame(this->min, this->gfx, 564, 0, 159, 0, silent);
        CopyFrame(this->min, this->gfx, 564, 1, 159, 1, silent);
        if (this->min->getSubtileCount() < 565)
            this->createSubtile();
        CopyFrame(this->min, this->gfx, 565, 0, 161, 0, silent);
        CopyFrame(this->min, this->gfx, 565, 1, 161, 1, silent);
        if (this->min->getSubtileCount() < 566)
            this->createSubtile();
        CopyFrame(this->min, this->gfx, 566, 0, 166, 0, silent);
        CopyFrame(this->min, this->gfx, 566, 1, 166, 1, silent);
        if (this->min->getSubtileCount() < 567)
            this->createSubtile();
        CopyFrame(this->min, this->gfx, 567, 0, 167, 0, silent);
        CopyFrame(this->min, this->gfx, 567, 1, 167, 1, silent);
        // - floor tile(3) with vertical arch
        if (this->til->getTileCount() < 161)
            this->createTile();
        ReplaceSubtile(this->til, 161 - 1, 0, 560, silent);
        ReplaceSubtile(this->til, 161 - 1, 1, 10, silent);
        ReplaceSubtile(this->til, 161 - 1, 2, 561, silent);
        ReplaceSubtile(this->til, 161 - 1, 3, 12, silent);
        // - floor tile(3) with horizontal arch
        if (this->til->getTileCount() < 162)
            this->createTile();
        ReplaceSubtile(this->til, 162 - 1, 0, 562, silent);
        ReplaceSubtile(this->til, 162 - 1, 1, 563, silent);
        ReplaceSubtile(this->til, 162 - 1, 2, 11, silent);
        ReplaceSubtile(this->til, 162 - 1, 3, 12, silent);
        // - floor tile with shadow(49) with vertical arch
        if (this->til->getTileCount() < 163)
            this->createTile();
        ReplaceSubtile(this->til, 163 - 1, 0, 564, silent); // - 159
        ReplaceSubtile(this->til, 163 - 1, 1, 160, silent);
        ReplaceSubtile(this->til, 163 - 1, 2, 565, silent); // - 161
        ReplaceSubtile(this->til, 163 - 1, 3, 162, silent);
        // - floor tile with shadow(51) with horizontal arch
        if (this->til->getTileCount() < 164)
            this->createTile();
        ReplaceSubtile(this->til, 164 - 1, 0, 566, silent); // - 166
        ReplaceSubtile(this->til, 164 - 1, 1, 567, silent); // - 167
        ReplaceSubtile(this->til, 164 - 1, 2, 168, silent);
        ReplaceSubtile(this->til, 164 - 1, 3, 169, silent);
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
        // patch dMegaTiles - L5.TIL
        // use common subtiles of doors
        // assert(pMegaTiles[4 * (71 - 1) + 2] == SwapLE16(213 - 1));
        ReplaceSubtile(this->til, 71 - 1, 2, 206, silent);
        // assert(pMegaTiles[4 * (72 - 1) + 2] == SwapLE16(216 - 1));
        ReplaceSubtile(this->til, 72 - 1, 2, 206, silent);
        // patch dMiniTiles - L5.MIN
        // pointless door micros (re-drawn by dSpecial)
        Blk2Mcr(77, 6);
        Blk2Mcr(77, 8);
        Blk2Mcr(80, 7);
        Blk2Mcr(80, 9);
        Blk2Mcr(206, 6);
        Blk2Mcr(206, 8);
        Blk2Mcr(209, 7);
        Blk2Mcr(209, 9);
        Blk2Mcr(213, 6);
        Blk2Mcr(213, 8);
        Blk2Mcr(216, 6);
        Blk2Mcr(216, 8);
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
