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
    return false;
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
    if (blockSize != 16) {
        return;
    }
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

    this->min->setModified();

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
    this->gfx->setModified();

    D1CelTilesetFrame::selectFrameType(frameLeft0);
    D1CelTilesetFrame::selectFrameType(frameRight0);
    if (frameLeft0->getFrameType() != D1CEL_FRAME_TYPE::RightTriangle) {
        dProgressErr() << QApplication::tr("Pot floor subtile (%1) is not triangle after patch.").arg(potLeftSubtileRef);
    } else if (!silent) {
        dProgress() << QApplication::tr("Frame %1 and %2 are modified and moved from subtile %3 to subtile %4.").arg(leftFrameRef1).arg(leftFrameRef2).arg(potLeftSubtileRef).arg(potRightSubtileRef);
    }
}

void D1Tileset::patchHellExit(int tileIndex, bool silent)
{
    std::vector<int> &tilSubtiles = this->til->getSubtileIndices(tileIndex);

    if (tilSubtiles[0] != (137 - 1) || tilSubtiles[1] != (138 - 1) || tilSubtiles[2] != (139 - 1) || tilSubtiles[3] != (140 - 1)) {
        if (tilSubtiles[0] != (17 - 1) || tilSubtiles[1] != (18 - 1))
            dProgressErr() << QApplication::tr("The exit tile (%1) has invalid (not original) subtiles.").arg(tileIndex + 1);
        else if (!silent)
            dProgress() << QApplication::tr("The exit tile (%1) is already patched.").arg(tileIndex + 1);
        return;
    }

    this->til->setSubtileIndex(tileIndex, 0, 17 - 1);
    this->til->setSubtileIndex(tileIndex, 1, 18 - 1);

    std::vector<unsigned> &topLeftFrameReferences = this->min->getFrameReferences(137 - 1);
    std::vector<unsigned> &topRightFrameReferences = this->min->getFrameReferences(138 - 1);
    std::vector<unsigned> &bottomLeftFrameReferences = this->min->getFrameReferences(139 - 1);
    std::vector<unsigned> &bottomRightFrameReferences = this->min->getFrameReferences(140 - 1);

    unsigned blockSize = 16;
    if (topLeftFrameReferences.size() != blockSize || topRightFrameReferences.size() != blockSize || bottomLeftFrameReferences.size() != blockSize || bottomRightFrameReferences.size() != blockSize) {
        dProgressErr() << QApplication::tr("The exit tile (%1) has invalid (upscaled?) subtiles.").arg(tileIndex + 1);
        return;
    }
    unsigned topLeft_LeftIndex0 = MICRO_IDX(blockSize, 0);
    unsigned topLeft_LeftFrameRef0 = topLeftFrameReferences[topLeft_LeftIndex0]; // 368
    unsigned topLeft_RightIndex0 = MICRO_IDX(blockSize, 1);
    unsigned topLeft_RightFrameRef0 = topLeftFrameReferences[topLeft_RightIndex0]; // 369
    unsigned topRight_LeftIndex0 = MICRO_IDX(blockSize, 0);
    unsigned topRight_LeftFrameRef0 = topRightFrameReferences[topRight_LeftIndex0]; // 370
    unsigned bottomLeft_RightIndex0 = MICRO_IDX(blockSize, 1);
    unsigned bottomLeft_RightFrameRef0 = bottomLeftFrameReferences[bottomLeft_RightIndex0]; // 375
    unsigned bottomRight_RightIndex0 = MICRO_IDX(blockSize, 1);
    unsigned bottomRight_RightFrameRef0 = bottomRightFrameReferences[bottomRight_RightIndex0]; // 377
    unsigned bottomRight_LeftIndex0 = MICRO_IDX(blockSize, 0);
    unsigned bottomRight_LeftFrameRef0 = bottomRightFrameReferences[bottomRight_LeftIndex0]; // 376

    D1GfxFrame *topLeft_RightFrame = this->gfx->getFrame(topLeft_RightFrameRef0 - 1);         // 369
    D1GfxFrame *topRight_LeftFrame = this->gfx->getFrame(topRight_LeftFrameRef0 - 1);         // 370
    D1GfxFrame *bottomLeft_RightFrame = this->gfx->getFrame(bottomLeft_RightFrameRef0 - 1);   // 375
    D1GfxFrame *bottomRight_LeftFrame = this->gfx->getFrame(bottomRight_LeftFrameRef0 - 1);   // 376
    D1GfxFrame *topLeft_Left0Frame = this->gfx->getFrame(topLeft_LeftFrameRef0 - 1);          // 368
    D1GfxFrame *bottomRight_RightFrame = this->gfx->getFrame(bottomRight_RightFrameRef0 - 1); // 377

    if (topLeft_RightFrame->getWidth() != MICRO_WIDTH || topLeft_RightFrame->getHeight() != MICRO_HEIGHT) {
        return; // upscaled(?) frames -> assume it is already done
    }
    if ((topRight_LeftFrame->getWidth() != MICRO_WIDTH || topRight_LeftFrame->getHeight() != MICRO_HEIGHT)
        || (bottomLeft_RightFrame->getWidth() != MICRO_WIDTH || bottomLeft_RightFrame->getHeight() != MICRO_HEIGHT)
        || (bottomRight_LeftFrame->getWidth() != MICRO_WIDTH || bottomRight_LeftFrame->getHeight() != MICRO_HEIGHT)
        || (topLeft_Left0Frame->getWidth() != MICRO_WIDTH || topLeft_Left0Frame->getHeight() != MICRO_HEIGHT)
        || (bottomRight_RightFrame->getWidth() != MICRO_WIDTH || bottomRight_RightFrame->getHeight() != MICRO_HEIGHT)) {
        dProgressErr() << QApplication::tr("The exit tile (%1) has invalid (mismatching) frames.").arg(tileIndex + 1);
        return;
    }

    // move the frames to the bottom right subtile
    bottomRightFrameReferences[MICRO_IDX(blockSize, 3)] = topLeftFrameReferences[MICRO_IDX(blockSize, 1)]; // 369
    topLeftFrameReferences[MICRO_IDX(blockSize, 1)] = 0;

    bottomRightFrameReferences[MICRO_IDX(blockSize, 2)] = topLeftFrameReferences[MICRO_IDX(blockSize, 0)]; // 368
    bottomRightFrameReferences[MICRO_IDX(blockSize, 4)] = topLeftFrameReferences[MICRO_IDX(blockSize, 2)]; // 367
    topLeftFrameReferences[MICRO_IDX(blockSize, 0)] = 0;
    topLeftFrameReferences[MICRO_IDX(blockSize, 2)] = 0;

    // eliminate right frame of the bottom left subtile
    bottomLeftFrameReferences[MICRO_IDX(blockSize, 1)] = 0;

    this->min->setModified();

    // copy 'bone' from topRight_LeftFrame (370) to the other frames 369  /377
    for (int x = 0; x < 15; x++) {
        for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
            D1GfxPixel pixel = topRight_LeftFrame->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            if (pixel.getPaletteIndex() < 96) // filter floor pixels
                continue;
            topLeft_RightFrame->setPixel(x, y + MICRO_HEIGHT / 2, pixel); // 369
        }
    }
    for (int x = 0; x < 15; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel = topRight_LeftFrame->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            if (pixel.getPaletteIndex() < 96) // filter floor pixels
                continue;
            bottomRight_RightFrame->setPixel(x, y - MICRO_HEIGHT / 2, pixel); // 377
        }
    }
    // copy content from topRight_LeftFrame (375) to the other frames 368  /376
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
            D1GfxPixel pixel = bottomLeft_RightFrame->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            topLeft_Left0Frame->setPixel(x, y + MICRO_HEIGHT / 2, pixel); // 368
        }
    }
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel = bottomLeft_RightFrame->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            bottomRight_LeftFrame->setPixel(x, y - MICRO_HEIGHT / 2, pixel); // 376
        }
    }

    // eliminate floor pixels of the topLeft_Right frame (369)
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = 0; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel = topLeft_RightFrame->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            if (x >= 15 || pixel.getPaletteIndex() < 80) {
                topLeft_RightFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
            }
        }
    }
    // fix bad artifacts
    topLeft_RightFrame->setPixel(7, 7, D1GfxPixel::transparentPixel());                  // 369
    topLeft_RightFrame->setPixel(13, 22, D1GfxPixel::transparentPixel());                // 369
    topLeft_RightFrame->setPixel(12, MICRO_HEIGHT - 1 - 2, D1GfxPixel::colorPixel(122)); // 369 (370)
    bottomRight_RightFrame->setPixel(14, 0, D1GfxPixel::transparentPixel());             // 377 (370)

    // adjust the frame types
    D1CelTilesetFrame::selectFrameType(topLeft_Left0Frame);     // 368
    D1CelTilesetFrame::selectFrameType(topLeft_RightFrame);     // 369
    D1CelTilesetFrame::selectFrameType(bottomRight_LeftFrame);  // 376
    D1CelTilesetFrame::selectFrameType(bottomRight_RightFrame); // 377

    this->gfx->setModified();

    if (!silent) {
        dProgress() << QApplication::tr("The subtiles of Tile %1 are modified.").arg(tileIndex + 1);
    }
}

void D1Tileset::patchCatacombsStairs(int backTileIndex1, int backTileIndex2, int extTileIndex1, int extTileIndex2, int stairsSubtileRef1, int stairsSubtileRef2, bool silent)
{
    constexpr unsigned blockSize = 10;

    constexpr int backSubtileRef0 = 250;
    constexpr int backSubtileRef2 = 251;
    constexpr int backSubtileRef3 = 252;
    constexpr int backSubtileRef0Replacement = 9;
    constexpr int backSubtileRef2Replacement = 11;
    { // check the back subtiles
        std::vector<int> &backSubtiles = this->til->getSubtileIndices(backTileIndex1);
        if (backSubtiles[0] != (backSubtileRef0 - 1) || backSubtiles[2] != (backSubtileRef2 - 1) || backSubtiles[3] != (backSubtileRef3 - 1)) {
            if (backSubtiles[0] != (backSubtileRef0Replacement - 1) || backSubtiles[2] != (backSubtileRef2Replacement - 1) || backSubtiles[3] != (backSubtileRef3 - 1))
                dProgressErr() << QApplication::tr("The back-stairs tile (%1) has invalid (not original) subtiles.").arg(backTileIndex1 + 1);
            else if (!silent)
                dProgress() << QApplication::tr("The back-stairs tile (%1) is already patched.").arg(backTileIndex1 + 1);
            return;
        }
    }

    constexpr int ext1SubtileRef1 = 265;
    constexpr int ext2SubtileRef1 = 556;
    constexpr int extSubtileRef1Replacement = 10;
    { // check the external subtiles
        std::vector<int> &ext1Subtiles = this->til->getSubtileIndices(extTileIndex1);
        if (ext1Subtiles[1] != (ext1SubtileRef1 - 1)) {
            if (ext1Subtiles[1] != (extSubtileRef1Replacement - 1))
                dProgressErr() << QApplication::tr("The ext-stairs tile (%1) has invalid (not original) subtiles.").arg(extTileIndex1 + 1);
            else if (!silent)
                dProgress() << QApplication::tr("The ext-stairs tile (%1) is already patched.").arg(extTileIndex1 + 1);
            return;
        }
        std::vector<int> &ext2Subtiles = this->til->getSubtileIndices(extTileIndex2);
        if (ext2Subtiles[1] != (ext2SubtileRef1 - 1)) {
            if (ext2Subtiles[1] != (extSubtileRef1Replacement - 1))
                dProgressErr() << QApplication::tr("The ext-stairs tile (%1) has invalid (not original) subtiles.").arg(extTileIndex2 + 1);
            else if (!silent)
                dProgress() << QApplication::tr("The ext-stairs tile (%1) is already patched.").arg(extTileIndex2 + 1);
            return;
        }
    }

    std::vector<unsigned> &back0FrameReferences = this->min->getFrameReferences(backSubtileRef0 - 1);
    std::vector<unsigned> &back2FrameReferences = this->min->getFrameReferences(backSubtileRef2 - 1);
    std::vector<unsigned> &back3FrameReferences = this->min->getFrameReferences(backSubtileRef3 - 1);
    std::vector<unsigned> &stairs1FrameReferences = this->min->getFrameReferences(stairsSubtileRef1 - 1);
    std::vector<unsigned> &stairs2FrameReferences = this->min->getFrameReferences(stairsSubtileRef2 - 1);
    std::vector<unsigned> &stairsExt1FrameReferences = this->min->getFrameReferences(ext1SubtileRef1 - 1);
    std::vector<unsigned> &stairsExt2FrameReferences = this->min->getFrameReferences(ext2SubtileRef1 - 1);

    if (back0FrameReferences.size() != blockSize || back2FrameReferences.size() != blockSize || back3FrameReferences.size() != blockSize
        || stairs1FrameReferences.size() != blockSize || stairs2FrameReferences.size() != blockSize
        || stairsExt1FrameReferences.size() != blockSize || stairsExt2FrameReferences.size() != blockSize) {
        dProgressErr() << QApplication::tr("At least one of the upstairs-subtiles (%1, %2, %3, %4, %5) is invalid (upscaled?).").arg(backSubtileRef0).arg(backSubtileRef2).arg(backSubtileRef3).arg(stairsSubtileRef1).arg(stairsSubtileRef2).arg(ext1SubtileRef1).arg(ext2SubtileRef1);
        return;
    }

    const unsigned microIndex0 = MICRO_IDX(blockSize, 0);
    const unsigned microIndex1 = MICRO_IDX(blockSize, 1);
    const unsigned microIndex2 = MICRO_IDX(blockSize, 2);
    const unsigned microIndex3 = MICRO_IDX(blockSize, 3);
    const unsigned microIndex4 = MICRO_IDX(blockSize, 4);
    const unsigned microIndex5 = MICRO_IDX(blockSize, 5);
    const unsigned microIndex6 = MICRO_IDX(blockSize, 6);

    unsigned back3_FrameRef0 = back3FrameReferences[microIndex0]; // 719
    unsigned back2_FrameRef1 = back2FrameReferences[microIndex1]; // 718
    unsigned back0_FrameRef0 = back0FrameReferences[microIndex0]; // 716

    unsigned stairs_FrameRef0 = stairs1FrameReferences[microIndex0]; // 770
    unsigned stairs_FrameRef2 = stairs1FrameReferences[microIndex2]; // 769
    unsigned stairs_FrameRef4 = stairs1FrameReferences[microIndex4]; // 768
    unsigned stairs_FrameRef6 = stairs1FrameReferences[microIndex6]; // 767

    unsigned stairsExt_FrameRef1 = stairsExt1FrameReferences[microIndex1]; // 762
    unsigned stairsExt_FrameRef3 = stairsExt1FrameReferences[microIndex3]; // 761
    unsigned stairsExt_FrameRef5 = stairsExt1FrameReferences[microIndex5]; // 760

    if (back3_FrameRef0 == 0 || back2_FrameRef1 == 0 || back0_FrameRef0 == 0) {
        dProgressErr() << QApplication::tr("The back-stairs tile (%1) has invalid (missing) frames.").arg(backTileIndex1 + 1);
        return;
    }

    if (stairs_FrameRef0 == 0 || stairs_FrameRef2 == 0 || stairs_FrameRef4 == 0 || stairs_FrameRef6 == 0
        || stairsExt_FrameRef1 == 0 || stairsExt_FrameRef3 == 0 || stairsExt_FrameRef5 == 0) {
        if (!silent)
            dProgress() << QApplication::tr("The stairs subtiles (%1) are already patched.").arg(stairsSubtileRef1);
        return;
    }

    if (stairs2FrameReferences[microIndex0] != stairs_FrameRef0) {
        dProgressErr() << QApplication::tr("The stairs subtiles (%1, %2) have invalid (mismatching) floor frames.").arg(stairsSubtileRef1).arg(stairsSubtileRef2);
        return;
    }
    if (stairsExt2FrameReferences[microIndex1] != stairsExt_FrameRef1
        || stairsExt2FrameReferences[microIndex3] != stairsExt_FrameRef3
        || stairsExt2FrameReferences[microIndex5] != stairsExt_FrameRef5) {
        dProgressErr() << QApplication::tr("The stairs external subtiles (%1, %2) have invalid (mismatching) frames.").arg(ext1SubtileRef1).arg(ext2SubtileRef1);
        return;
    }

    D1GfxFrame *back3_LeftFrame = this->gfx->getFrame(back3_FrameRef0 - 1);  // 719
    D1GfxFrame *back2_LeftFrame = this->gfx->getFrame(back2_FrameRef1 - 1);  // 718
    D1GfxFrame *back0_RightFrame = this->gfx->getFrame(back0_FrameRef0 - 1); // 716

    D1GfxFrame *stairs_LeftFrame0 = this->gfx->getFrame(stairs_FrameRef0 - 1); // 770
    D1GfxFrame *stairs_LeftFrame2 = this->gfx->getFrame(stairs_FrameRef2 - 1); // 769
    D1GfxFrame *stairs_LeftFrame4 = this->gfx->getFrame(stairs_FrameRef4 - 1); // 768
    D1GfxFrame *stairs_LeftFrame6 = this->gfx->getFrame(stairs_FrameRef6 - 1); // 767

    D1GfxFrame *stairsExt_RightFrame1 = this->gfx->getFrame(stairsExt_FrameRef1 - 1); // 762
    D1GfxFrame *stairsExt_RightFrame3 = this->gfx->getFrame(stairsExt_FrameRef3 - 1); // 761
    D1GfxFrame *stairsExt_RightFrame5 = this->gfx->getFrame(stairsExt_FrameRef5 - 1); // 760

    if (back3_LeftFrame->getWidth() != MICRO_WIDTH || back3_LeftFrame->getHeight() != MICRO_HEIGHT) {
        return; // upscaled(?) frames -> assume it is already done
    }
    if ((back2_LeftFrame->getWidth() != MICRO_WIDTH || back2_LeftFrame->getHeight() != MICRO_HEIGHT)
        || (back0_RightFrame->getWidth() != MICRO_WIDTH || back0_RightFrame->getHeight() != MICRO_HEIGHT)) {
        dProgressErr() << QApplication::tr("The back-stairs tile (%1) has invalid (mismatching) frames.").arg(backTileIndex1 + 1);
        return;
    }
    if ((stairs_LeftFrame0->getWidth() != MICRO_WIDTH || stairs_LeftFrame0->getHeight() != MICRO_HEIGHT)
        || (stairs_LeftFrame2->getWidth() != MICRO_WIDTH || stairs_LeftFrame2->getHeight() != MICRO_HEIGHT)
        || (stairs_LeftFrame4->getWidth() != MICRO_WIDTH || stairs_LeftFrame4->getHeight() != MICRO_HEIGHT)
        || (stairs_LeftFrame6->getWidth() != MICRO_WIDTH || stairs_LeftFrame6->getHeight() != MICRO_HEIGHT)) {
        dProgressErr() << QApplication::tr("The stairs subtile (%1) has invalid (mismatching) frames I.").arg(stairsSubtileRef1);
        return;
    }
    if ((stairsExt_RightFrame1->getWidth() != MICRO_WIDTH || stairsExt_RightFrame1->getHeight() != MICRO_HEIGHT)
        || (stairsExt_RightFrame3->getWidth() != MICRO_WIDTH || stairsExt_RightFrame3->getHeight() != MICRO_HEIGHT)
        || (stairsExt_RightFrame5->getWidth() != MICRO_WIDTH || stairsExt_RightFrame5->getHeight() != MICRO_HEIGHT)) {
        dProgressErr() << QApplication::tr("The stairs subtile (%1) has invalid (mismatching) frames II.").arg(ext1SubtileRef1);
        return;
    }

    // move the frames to the back subtile
    const unsigned microIndex7 = MICRO_IDX(blockSize, 7);
    // - left side
    back3FrameReferences[microIndex2] = stairsExt1FrameReferences[microIndex3]; // 761
    stairsExt1FrameReferences[microIndex3] = 0;
    stairsExt2FrameReferences[microIndex3] = 0;

    // - right side
    back3FrameReferences[microIndex1] = stairsExt1FrameReferences[microIndex5]; // 760
    stairsExt1FrameReferences[microIndex5] = 0;
    stairsExt2FrameReferences[microIndex5] = 0;

    back3FrameReferences[microIndex3] = stairs1FrameReferences[microIndex2]; // 769
    stairs1FrameReferences[microIndex2] = 0;
    stairs2FrameReferences[microIndex2] = 0; // 1471

    back3FrameReferences[microIndex5] = stairs1FrameReferences[microIndex4]; // 768
    stairs1FrameReferences[microIndex4] = 0;
    stairs2FrameReferences[microIndex4] = 0; // 1470

    back3FrameReferences[microIndex7] = stairs1FrameReferences[microIndex6]; // 767
    stairs1FrameReferences[microIndex6] = 0;
    stairs2FrameReferences[microIndex6] = 0;

    this->min->setModified();

    // move external-stairs
    // - from stairsExt_RightFrame1 (762) to the right side of the back3 frame 719
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
            D1GfxPixel pixel = stairsExt_RightFrame1->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            if (!back2_LeftFrame->getPixel(x, y).isTransparent()) // use 718 as a mask
                continue;
            back3_LeftFrame->setPixel(x, y + MICRO_HEIGHT / 2, pixel);             // 719
            stairsExt_RightFrame1->setPixel(x, y, D1GfxPixel::transparentPixel()); // 762
        }
    }
    // - from stairsExt_RightFrame3 (761) to the right side of the back3 frame 719
    //  and shift the external-stairs down
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel0 = stairsExt_RightFrame3->getPixel(x, y);
            if (!pixel0.isTransparent()) {
                back3_LeftFrame->setPixel(x, y - MICRO_HEIGHT / 2, pixel0); // 719
            }
            D1GfxPixel pixel1 = stairsExt_RightFrame3->getPixel(x, y - MICRO_HEIGHT / 2);
            stairsExt_RightFrame3->setPixel(x, y, pixel1); // 761
        }
    }
    // shift the rest of the external-stairs down
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel0 = stairsExt_RightFrame5->getPixel(x, y);
            stairsExt_RightFrame3->setPixel(x, y - MICRO_HEIGHT / 2, pixel0); // 761
            D1GfxPixel pixel1 = stairsExt_RightFrame5->getPixel(x, y - MICRO_HEIGHT / 2);
            stairsExt_RightFrame5->setPixel(x, y, pixel1); // 760
        }
    }

    // move external-stairs
    // move stairs from stairs_LeftFrame0 (770) to the new back3 right frame 760
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
            D1GfxPixel pixel = stairs_LeftFrame0->getPixel(x, y);
            if (!back0_RightFrame->getPixel(x, y).isTransparent()) // use 716 as a mask
                continue;
            stairsExt_RightFrame5->setPixel(x, y + MICRO_HEIGHT / 2, pixel);   // 760
            stairs_LeftFrame0->setPixel(x, y, D1GfxPixel::transparentPixel()); // 770
        }
    }
    // shift the stairs down
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel0 = stairs_LeftFrame2->getPixel(x, y);
            stairsExt_RightFrame5->setPixel(x, y - MICRO_HEIGHT / 2, pixel0); // 760
            D1GfxPixel pixel1 = stairs_LeftFrame2->getPixel(x, y - MICRO_HEIGHT / 2);
            stairs_LeftFrame2->setPixel(x, y, pixel1); // 769
        }
    }
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel0 = stairs_LeftFrame4->getPixel(x, y);
            stairs_LeftFrame2->setPixel(x, y - MICRO_HEIGHT / 2, pixel0); // 769
            D1GfxPixel pixel1 = stairs_LeftFrame4->getPixel(x, y - MICRO_HEIGHT / 2);
            stairs_LeftFrame4->setPixel(x, y, pixel1); // 768
        }
    }
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel0 = stairs_LeftFrame6->getPixel(x, y);
            stairs_LeftFrame4->setPixel(x, y - MICRO_HEIGHT / 2, pixel0); // 768
            D1GfxPixel pixel1 = stairs_LeftFrame6->getPixel(x, y - MICRO_HEIGHT / 2);
            stairs_LeftFrame6->setPixel(x, y, pixel1); // 767
        }
    }
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
            stairs_LeftFrame6->setPixel(x, y, D1GfxPixel::transparentPixel()); // 767
        }
    }

    // copy shadow
    // - from back2_LeftFrame 718 to the back3 left frame 719
    for (int x = 22; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < 20; y++) {
            D1GfxPixel pixel = back2_LeftFrame->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            if (pixel.getPaletteIndex() % 16 < 11)
                continue;
            if (!back3_LeftFrame->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent())
                continue;
            back3_LeftFrame->setPixel(x, y - MICRO_HEIGHT / 2, pixel); // 719
        }
    }
    // - from back2_LeftFrame 718 to the new back3 left frame 761
    for (int x = 22; x < MICRO_WIDTH; x++) {
        for (int y = 12; y < MICRO_HEIGHT / 2; y++) {
            D1GfxPixel pixel = back2_LeftFrame->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            if (pixel.getPaletteIndex() % 16 < 11)
                continue;
            if (!stairsExt_RightFrame3->getPixel(x, y + MICRO_HEIGHT / 2).isTransparent())
                continue;
            stairsExt_RightFrame3->setPixel(x, y + MICRO_HEIGHT / 2, pixel); // 761
        }
    }
    // - from back0_RightFrame 716 to the new back3 left frame 761
    for (int x = 22; x < MICRO_WIDTH; x++) {
        for (int y = 20; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel = back0_RightFrame->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            if (pixel.getPaletteIndex() % 16 < 11)
                continue;
            if (!stairsExt_RightFrame3->getPixel(x, y).isTransparent())
                continue;
            stairsExt_RightFrame3->setPixel(x, y, pixel); // 761
        }
    }

    // fix bad artifacts
    stairsExt_RightFrame3->setPixel(23, 20, D1GfxPixel::transparentPixel()); // 761
    stairsExt_RightFrame3->setPixel(24, 20, D1GfxPixel::transparentPixel()); // 761
    stairsExt_RightFrame3->setPixel(22, 21, D1GfxPixel::transparentPixel()); // 761
    stairsExt_RightFrame3->setPixel(23, 21, D1GfxPixel::transparentPixel()); // 761

    // adjust the frame types
    D1CelTilesetFrame::selectFrameType(stairs_LeftFrame0);     // 770
    D1CelTilesetFrame::selectFrameType(stairs_LeftFrame2);     // 769
    D1CelTilesetFrame::selectFrameType(stairsExt_RightFrame5); // 760
    D1CelTilesetFrame::selectFrameType(stairsExt_RightFrame1); // 762
    D1CelTilesetFrame::selectFrameType(back3_LeftFrame);       // 719

    this->gfx->setModified();

    // patch TMI
    quint8 properties;
    this->tmi->setSubtileProperties(backSubtileRef3 - 1, TMIF_LEFT_REDRAW | TMIF_RIGHT_REDRAW);
    this->tmi->setSubtileProperties(stairsSubtileRef1 - 1, 0);
    this->tmi->setSubtileProperties(stairsSubtileRef2 - 1, 0);
    this->tmi->setSubtileProperties(ext1SubtileRef1 - 1, 0);
    this->tmi->setSubtileProperties(ext2SubtileRef1 - 1, 0);

    // patch SOL
    properties = this->sol->getSubtileProperties(backSubtileRef3 - 1);
    properties |= (PFLAG_BLOCK_PATH | PFLAG_BLOCK_LIGHT | PFLAG_BLOCK_MISSILE);
    this->sol->setSubtileProperties(backSubtileRef3 - 1, properties);
    properties = this->sol->getSubtileProperties(stairsSubtileRef1 - 1);
    properties &= ~(PFLAG_BLOCK_LIGHT);
    this->sol->setSubtileProperties(stairsSubtileRef1 - 1, properties);
    this->sol->setSubtileProperties(stairsSubtileRef2 - 1, properties);

    // replace subtiles
    ReplaceSubtile(this->til, backTileIndex1, 0, backSubtileRef0Replacement, silent); // use common subtile
    ReplaceSubtile(this->til, backTileIndex1, 1, 56, silent);                         // make the back of the stairs non-walkable
    ReplaceSubtile(this->til, backTileIndex1, 2, backSubtileRef2Replacement, silent); // use common subtile
    ReplaceSubtile(this->til, extTileIndex1, 1, extSubtileRef1Replacement, silent);   // use common subtile

    ReplaceSubtile(this->til, backTileIndex2, 0, backSubtileRef0Replacement, silent); // use common subtile
    ReplaceSubtile(this->til, backTileIndex2, 1, 56, silent);                         // make the back of the stairs non-walkable
    ReplaceSubtile(this->til, backTileIndex2, 2, backSubtileRef2Replacement, silent); // use common subtile
    ReplaceSubtile(this->til, extTileIndex2, 0, backSubtileRef0Replacement, silent);  // use common subtile
    ReplaceSubtile(this->til, extTileIndex2, 1, extSubtileRef1Replacement, silent);   // use common subtile
    ReplaceSubtile(this->til, extTileIndex2, 2, backSubtileRef2Replacement, silent);  // use common subtile

    if (!silent) {
        dProgress() << QApplication::tr("The back-stair tiles (%1, %2) and the stair-subtiles (%2, %3, %4, %5) are modified.").arg(backTileIndex1 + 1).arg(backTileIndex2 + 1).arg(extTileIndex1 + 1).arg(extTileIndex2 + 1).arg(stairsSubtileRef1).arg(stairsSubtileRef2);
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
        this->patchTownPot(553, 554, silent);
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
        if (this->min->getSubtileCount() < 560) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(560 - 1, 2);
        }
        CopyFrame(this->min, this->gfx, 560, 0, 9, 0, silent);
        CopyFrame(this->min, this->gfx, 560, 1, 9, 1, silent);
        if (this->min->getSubtileCount() < 561) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(561 - 1, 1);
        }
        CopyFrame(this->min, this->gfx, 561, 0, 11, 0, silent);
        CopyFrame(this->min, this->gfx, 561, 1, 11, 1, silent);
        if (this->min->getSubtileCount() < 562) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(562 - 1, 3);
        }
        CopyFrame(this->min, this->gfx, 562, 0, 9, 0, silent);
        CopyFrame(this->min, this->gfx, 562, 1, 9, 1, silent);
        if (this->min->getSubtileCount() < 563) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(563 - 1, 4);
        }
        CopyFrame(this->min, this->gfx, 563, 0, 10, 0, silent);
        CopyFrame(this->min, this->gfx, 563, 1, 10, 1, silent);
        if (this->min->getSubtileCount() < 564) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(564 - 1, 2);
        }
        CopyFrame(this->min, this->gfx, 564, 0, 159, 0, silent);
        CopyFrame(this->min, this->gfx, 564, 1, 159, 1, silent);
        if (this->min->getSubtileCount() < 565) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(565 - 1, 1);
        }
        CopyFrame(this->min, this->gfx, 565, 0, 161, 0, silent);
        CopyFrame(this->min, this->gfx, 565, 1, 161, 1, silent);
        if (this->min->getSubtileCount() < 566) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(566 - 1, 3);
        }
        CopyFrame(this->min, this->gfx, 566, 0, 166, 0, silent);
        CopyFrame(this->min, this->gfx, 566, 1, 166, 1, silent);
        if (this->min->getSubtileCount() < 567) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(567 - 1, 4);
        }
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
        // fix the upstairs
        this->patchCatacombsStairs(72 - 1, 158 - 1, 76 - 1, 159 - 1, 267, 559, silent);
        // fix bad artifact
        // Blk2Mcr(288, 7);
        // patch dAutomapData - L2.AMP
        this->amp->setTileProperties(42 - 1, this->amp->getTileProperties(42 - 1) & ~(MAPFLAG_HORZARCH >> 8));
        this->amp->setTileProperties(156 - 1, this->amp->getTileProperties(156 - 1) & ~(MAPFLAG_VERTDOOR >> 8));
        this->amp->setTileType(156 - 1, 0);
        this->amp->setTileProperties(157 - 1, this->amp->getTileProperties(157 - 1) & ~(MAPFLAG_HORZDOOR >> 8));
        this->amp->setTileType(157 - 1, 0);
        break;
    case DTYPE_CAVES:
        // patch dMiniTiles - L3.MIN
        // fix bad artifact
        Blk2Mcr(82, 4);
        break;
    case DTYPE_HELL:
        this->patchHellExit(45 - 1, silent);
        // patch dAutomapData - L4.AMP
        this->amp->setTileProperties(52 - 1, this->amp->getTileProperties(52 - 1) | (MAPFLAG_VERTGRATE >> 8));
        this->amp->setTileProperties(56 - 1, this->amp->getTileProperties(56 - 1) | (MAPFLAG_HORZGRATE >> 8));
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
        ReplaceSubtile(this->til, 71 - 1, 2, 206, silent);
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
