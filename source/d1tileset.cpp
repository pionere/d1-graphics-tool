#include "d1tileset.h"

#include <QApplication>
#include <QMessageBox>

#include "d1cel.h"
#include "d1celtileset.h"
#include "progressdialog.h"

#include "dungeon/all.h"

static constexpr int BLOCK_SIZE_TOWN = 16;
static constexpr int BLOCK_SIZE_L1 = 10;
static constexpr int BLOCK_SIZE_L2 = 10;
static constexpr int BLOCK_SIZE_L3 = 10;
static constexpr int BLOCK_SIZE_L4 = 16;
static constexpr int BLOCK_SIZE_L5 = 10;
static constexpr int BLOCK_SIZE_L6 = 10;

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

#define Blk2Mcr(n, x) RemoveFrame(this->min, n, x, deletedFrames, silent);
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
        ; // dProgressWarn() << QApplication::tr("The frame @%1 in Subtile %2 is already empty.").arg(index + 1).arg(subtileIndex + 1);
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

#define ReplaceMcr(dn, dx, sn, sx) ReplaceFrame(this->min, dn, dx, sn, sx, &deletedFrames, silent);
#define SetMcr(dn, dx, sn, sx) ReplaceFrame(this->min, dn, dx, sn, sx, nullptr, silent);
static void ReplaceFrame(D1Min *min, int dstSubtileRef, int dstMicroIndex, int srcSubtileRef, int srcMicroIndex, std::set<unsigned> *deletedFrames, bool silent)
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
    unsigned currFrameReference = dstFrameReferences[dstIndex];
    if (min->setFrameReference(dstSubtileIndex, dstIndex, srcFrameRef)) {
        if (currFrameReference != 0 && deletedFrames != nullptr) {
            deletedFrames->insert(currFrameReference);
        }
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

static void ChangeTileMapFlags(D1Amp *amp, int tileIndex, int mapFlag, bool add, bool silent)
{
    quint8 currProperties = amp->getTileProperties(tileIndex);
    quint8 newProperties = currProperties;
    if (add) {
        newProperties = currProperties | (mapFlag >> 8);
    } else {
        newProperties = currProperties & ~(mapFlag >> 8);
    }
    if (amp->setTileProperties(tileIndex, newProperties)) {
        if (!silent) {
            dProgress() << QApplication::tr("The automap flags of Tile %1 is changed from %2 to %3.").arg(tileIndex + 1).arg(currProperties).arg(newProperties);
        }
    }
}

static void ChangeTileMapType(D1Amp *amp, int tileIndex, int mapType, bool silent)
{
    quint8 currMapType = amp->getTileType(tileIndex);
    if (amp->setTileType(tileIndex, mapType)) {
        if (!silent) {
            dProgress() << QApplication::tr("The automap type of Tile %1 is changed from %2 to %3.").arg(tileIndex + 1).arg(currMapType).arg(mapType);
        }
    }
}

static void ChangeSubtileSolFlags(D1Sol *sol, int subtileIndex, int solFlag, bool add, bool silent)
{
    quint8 currProperties = sol->getSubtileProperties(subtileIndex);
    quint8 newProperties = currProperties;
    if (add) {
        newProperties = currProperties | solFlag;
    } else {
        newProperties = currProperties & ~solFlag;
    }
    if (sol->setSubtileProperties(subtileIndex, newProperties)) {
        if (!silent) {
            dProgress() << QApplication::tr("The SOL flags of Subtile %1 is changed from %2 to %3.").arg(subtileIndex + 1).arg(currProperties).arg(newProperties);
        }
    }
}

void D1Tileset::patchTownPot(int potLeftSubtileRef, int potRightSubtileRef, bool silent)
{
    std::vector<unsigned> &leftFrameReferences = this->min->getFrameReferences(potLeftSubtileRef - 1);
    std::vector<unsigned> &rightFrameReferences = this->min->getFrameReferences(potRightSubtileRef - 1);

    constexpr unsigned blockSize = BLOCK_SIZE_TOWN;
    if (leftFrameReferences.size() != blockSize || rightFrameReferences.size() != blockSize) {
        dProgressErr() << QApplication::tr("The pot subtiles (%1, %2) are invalid (upscaled?).").arg(potLeftSubtileRef).arg(potRightSubtileRef);
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

void D1Tileset::patchTownCathedral(int cathedralTopLeftRef, int cathedralTopRightRef, int cathedralBottomLeftRef, bool silent)
{
    std::vector<unsigned> &topLeftFrameReferences = this->min->getFrameReferences(cathedralTopLeftRef - 1);
    std::vector<unsigned> &topRightFrameReferences = this->min->getFrameReferences(cathedralTopRightRef - 1);
    std::vector<unsigned> &bottomLeftFrameReferences = this->min->getFrameReferences(cathedralBottomLeftRef - 1);

    constexpr unsigned blockSize = BLOCK_SIZE_TOWN;
    if (topLeftFrameReferences.size() != blockSize || topRightFrameReferences.size() != blockSize || bottomLeftFrameReferences.size() != blockSize) {
        dProgressErr() << QApplication::tr("The cathedral subtiles (%1, %2, %3) are invalid (upscaled?).").arg(cathedralTopLeftRef).arg(cathedralTopRightRef).arg(cathedralBottomLeftRef);
        return;
    }

    unsigned leftIndex0 = MICRO_IDX(blockSize, 12); // 2145
    unsigned leftFrameRef0 = bottomLeftFrameReferences[leftIndex0];
    unsigned leftIndex1 = MICRO_IDX(blockSize, 12); // 2123
    unsigned leftFrameRef1 = topLeftFrameReferences[leftIndex1];
    unsigned rightIndex0 = MICRO_IDX(blockSize, 13); // 2124
    unsigned rightFrameRef0 = topLeftFrameReferences[rightIndex0];
    unsigned rightIndex1 = MICRO_IDX(blockSize, 13); // 2137
    unsigned rightFrameRef1 = topRightFrameReferences[rightIndex1];

    if (leftFrameRef0 == 0 || leftFrameRef1 == 0 || rightFrameRef0 == 0 || rightFrameRef1 == 0) {
        dProgressErr() << QApplication::tr("Invalid (empty) cathedral subtiles (%1).").arg(leftFrameRef0 == 0 ? cathedralBottomLeftRef : (rightFrameRef1 == 0 ? cathedralTopRightRef : cathedralTopLeftRef));
        return;
    }

    D1GfxFrame *frameLeft0 = this->gfx->getFrame(leftFrameRef0 - 1);   // 2145
    D1GfxFrame *frameLeft1 = this->gfx->getFrame(leftFrameRef1 - 1);   // 2123
    D1GfxFrame *frameRight0 = this->gfx->getFrame(rightFrameRef0 - 1); // 2124
    D1GfxFrame *frameRight1 = this->gfx->getFrame(rightFrameRef1 - 1); // 2137

    if ((frameLeft0->getWidth() != MICRO_WIDTH || frameLeft0->getHeight() != MICRO_HEIGHT)
        || (frameLeft1->getWidth() != MICRO_WIDTH || frameLeft1->getHeight() != MICRO_HEIGHT)
        || (frameRight0->getWidth() != MICRO_WIDTH || frameRight0->getHeight() != MICRO_HEIGHT)
        || (frameRight1->getWidth() != MICRO_WIDTH || frameRight1->getHeight() != MICRO_HEIGHT)) {
        dProgressErr() << QApplication::tr("Invalid (non standard dimensions) cathedral subtiles (%1, %2, %3).").arg(cathedralTopLeftRef).arg(cathedralTopRightRef).arg(cathedralBottomLeftRef);
        return;
    }

    // draw extra line to each frame
    bool change = false;
    for (int x = 0; x < MICRO_WIDTH; x++) {
        int y = MICRO_HEIGHT / 2 - x / 2;
        change |= frameLeft0->setPixel(x, y, frameLeft0->getPixel(x, y + 6)); // 2145
    }
    for (int x = 0; x < MICRO_WIDTH - 4; x++) {
        int y = MICRO_HEIGHT / 2 - x / 2;
        change |= frameLeft1->setPixel(x, y, frameLeft1->getPixel(x + 4, y + 4)); // 2123 I.
    }
    for (int x = MICRO_WIDTH - 4; x < MICRO_WIDTH; x++) {
        int y = MICRO_HEIGHT / 2 - x / 2;
        change |= frameLeft1->setPixel(x, y, frameLeft1->getPixel(x, y + 2)); // 2123 II.
    }
    for (int x = 0; x < 20; x++) {
        int y = 1 + x / 2;
        change |= frameRight0->setPixel(x, y, frameRight0->getPixel(x, y + 1)); // 2124 I.
    }
    for (int x = 20; x < MICRO_WIDTH; x++) {
        int y = 1 + x / 2;
        change |= frameRight0->setPixel(x, y, frameRight0->getPixel(x - 12, y - 6)); // 2124 II.
    }
    for (int x = 0; x < MICRO_WIDTH; x++) {
        int y = 1 + x / 2;
        change |= frameRight1->setPixel(x, y, frameRight0->getPixel(x, y)); // 2137
    }

    if (change) {
        this->gfx->setModified();

        if (!silent) {
            dProgress() << QApplication::tr("Frames %1, %2, %3 and %4 of subtiles %5, %6 and %7 are modified.").arg(leftFrameRef0).arg(leftFrameRef1).arg(rightFrameRef0).arg(rightFrameRef1).arg(cathedralTopLeftRef).arg(cathedralTopRightRef).arg(cathedralBottomLeftRef);
        }
    }
}

void D1Tileset::patchHellExit(int tileIndex, bool silent)
{
    std::vector<int> &tilSubtiles = this->til->getSubtileIndices(tileIndex);

    if (tilSubtiles[0] != (137 - 1) || tilSubtiles[1] != (138 - 1) || tilSubtiles[2] != (139 - 1) || tilSubtiles[3] != (140 - 1)) {
        if (tilSubtiles[0] != (17 - 1) || tilSubtiles[1] != (18 - 1))
            dProgressErr() << QApplication::tr("The exit tile (%1) has invalid (not original) subtiles.").arg(tileIndex + 1);
        else if (!silent)
            dProgressWarn() << QApplication::tr("The exit tile (%1) is already patched.").arg(tileIndex + 1);
        return;
    }

    this->til->setSubtileIndex(tileIndex, 0, 17 - 1);
    this->til->setSubtileIndex(tileIndex, 1, 18 - 1);

    std::vector<unsigned> &topLeftFrameReferences = this->min->getFrameReferences(137 - 1);
    std::vector<unsigned> &topRightFrameReferences = this->min->getFrameReferences(138 - 1);
    std::vector<unsigned> &bottomLeftFrameReferences = this->min->getFrameReferences(139 - 1);
    std::vector<unsigned> &bottomRightFrameReferences = this->min->getFrameReferences(140 - 1);

    constexpr unsigned blockSize = BLOCK_SIZE_L4;
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
    constexpr unsigned blockSize = BLOCK_SIZE_L2;

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
                dProgressWarn() << QApplication::tr("The back-stairs tile (%1) is already patched.").arg(backTileIndex1 + 1);
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
                dProgressWarn() << QApplication::tr("The ext-stairs tile (%1) is already patched.").arg(extTileIndex1 + 1);
            return;
        }
        std::vector<int> &ext2Subtiles = this->til->getSubtileIndices(extTileIndex2);
        if (ext2Subtiles[1] != (ext2SubtileRef1 - 1)) {
            if (ext2Subtiles[1] != (extSubtileRef1Replacement - 1))
                dProgressErr() << QApplication::tr("The ext-stairs tile (%1) has invalid (not original) subtiles.").arg(extTileIndex2 + 1);
            else if (!silent)
                dProgressWarn() << QApplication::tr("The ext-stairs tile (%1) is already patched.").arg(extTileIndex2 + 1);
            return;
        }
    }
    // TODO: check if there are enough subtiles
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
            dProgressWarn() << QApplication::tr("The stairs subtiles (%1) are already patched.").arg(stairsSubtileRef1);
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

    quint8 properties;
    // patch TMI
    /*this->tmi->setSubtileProperties(backSubtileRef3 - 1, TMIF_LEFT_REDRAW | TMIF_RIGHT_REDRAW);
    this->tmi->setSubtileProperties(stairsSubtileRef1 - 1, 0);
    this->tmi->setSubtileProperties(stairsSubtileRef2 - 1, 0);
    this->tmi->setSubtileProperties(ext1SubtileRef1 - 1, 0);
    this->tmi->setSubtileProperties(ext2SubtileRef1 - 1, 0);*/

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

std::pair<unsigned, D1GfxFrame *> D1Tileset::getFrame(int subtileIndex, int blockSize, unsigned microIndex)
{
    std::vector<unsigned> &frameReferences = this->min->getFrameReferences(subtileIndex);
    if (frameReferences.size() != blockSize) {
        dProgressErr() << QApplication::tr("Subtile (%1) is invalid (upscaled?).").arg(subtileIndex + 1);
        return std::pair<unsigned, D1GfxFrame *>(0, nullptr);
    }
    microIndex = MICRO_IDX(blockSize, microIndex);

    unsigned frameRef = frameReferences[microIndex];
    if (frameRef == 0) {
        dProgressErr() << QApplication::tr("Subtile (%1) has invalid (missing) frames.").arg(subtileIndex + 1);
        return std::pair<unsigned, D1GfxFrame *>(0, nullptr);
    }
    D1GfxFrame *frame = this->gfx->getFrame(frameRef - 1);
    if (frame->getWidth() != MICRO_WIDTH || frame->getWidth() != MICRO_WIDTH) {
        dProgressErr() << QApplication::tr("Subtile (%1) is invalid (upscaled?).").arg(subtileIndex + 1);
        return std::pair<unsigned, D1GfxFrame *>(0, nullptr);
    }
    return std::pair<unsigned, D1GfxFrame *>(frameRef, frame);
}

void D1Tileset::fillCryptShapes(bool silent)
{
    typedef struct {
        unsigned subtileIndex;
        unsigned microIndex;
        D1CEL_FRAME_TYPE res_encoding;
    } CelMicro;
    const CelMicro micros[] = {
        // clang-format off
        { 159 - 1, 3, D1CEL_FRAME_TYPE::Square },            // 473
//      { 159 - 1, 3, D1CEL_FRAME_TYPE::RightTrapezoid },    // 475
        { 336 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },      // 907
        { 409 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },      // 1168
        { 481 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },     // 1406
        { 492 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },      // 1436
        { 519 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },      // 1493
        { 595 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },     // 1710
        { 368 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },     // 1034
        { 162 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare }, // 483
        {  63 - 1, 4, D1CEL_FRAME_TYPE::Square },            // 239
        { 450 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // 1315
        { 206 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // 571
        // clang-format on
    };

    // TODO: check if there are enough subtiles
    constexpr unsigned blockSize = BLOCK_SIZE_L5;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return;
        }
        bool change = false;
        if (i == 1) { // 907
            change |= frame->setPixel(30, 1, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(31, 1, D1GfxPixel::colorPixel(76));
        }
        if (i == 5) { // 1493
            change |= frame->setPixel(0, 16, D1GfxPixel::colorPixel(43));
        }
        if (i == 7) { // 1043
            change |= frame->setPixel(0, 7, D1GfxPixel::colorPixel(43));
            change |= frame->setPixel(0, 9, D1GfxPixel::colorPixel(41));
        }
        if (i == 8) { // 483
            change |= frame->setPixel(31, 13, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(31, 14, D1GfxPixel::colorPixel(36));
            change |= frame->setPixel(31, 18, D1GfxPixel::colorPixel(36));
            change |= frame->setPixel(30, 19, D1GfxPixel::colorPixel(35));
            change |= frame->setPixel(31, 20, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(31, 21, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(31, 22, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(31, 29, D1GfxPixel::colorPixel(36));
        }
        if (i == 9) { // 239
            change |= frame->setPixel(0, 19, D1GfxPixel::colorPixel(91));
            change |= frame->setPixel(0, 20, D1GfxPixel::colorPixel(93));
        }
        if (i == 10) { // 1315
            for (int y = 13; y < 16; y++) {
                for (int x = 2; x < 8; x++) {
                    if (y > 14 - (x - 2) / 2) {
                        quint8 color = 43;
                        if (y == 14 - (x - 2) / 2) {
                            if ((x & 1) == 0) {
                                color = 44;
                            } else if (x == 7) {
                                color = 77;
                            }
                        }
                        change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(color));
                    }
                }
            }
        }
        if (i == 11) { // 571
            change |= frame->setPixel(24, 4, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(23, 5, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(26, 4, D1GfxPixel::colorPixel(60));
        }
        std::vector<FramePixel> pixels;
        D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
        for (const FramePixel &pix : pixels) {
            D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
            change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
        }
        if (change) {
            frame->setFrameType(micro.res_encoding);
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }
}

void D1Tileset::maskCryptBlacks(bool silent)
{
    typedef struct {
        unsigned subtileIndex;
        unsigned microIndex;
    } CelMicro;
    const CelMicro micros[] = {
        // clang-format off
        { 126 - 1, 1 }, // 347
        { 129 - 1, 0 }, // 356
        { 129 - 1, 1 }, // 357
        { 131 - 1, 0 }, // 362
        { 131 - 1, 1 }, // 363
        { 132 - 1, 0 }, // 364
        { 133 - 1, 0 }, // 371
        { 133 - 1, 1 }, // 372
        { 134 - 1, 3 }, // 375
        { 135 - 1, 0 }, // 379
        { 135 - 1, 1 }, // 380
        { 142 - 1, 0 }, // 403
//      { 146 - 1, 0 }, // 356
//      { 149 - 1, 4 }, // 356
//      { 150 - 1, 6 }, // 356
//      { 151 - 1, 2 }, // 438
        { 151 - 1, 4 }, // 436
        { 151 - 1, 5 }, // 437
        { 152 - 1, 7 }, // 439
        { 153 - 1, 2 }, // 442
        { 153 - 1, 4 }, // 441
        { 159 - 1, 1 }, // 475
        // clang-format on
    };

    // TODO: check if there are enough subtiles
    constexpr unsigned blockSize = BLOCK_SIZE_L5;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return;
        }

        // mask the black pixels
        bool change = false;
        for (int y = 0; y < MICRO_WIDTH; y++) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                D1GfxPixel pixel = frame->getPixel(x, y);
                if (pixel.getPaletteIndex() == 79) {
                    if (i == 0 && x < 9) { // 126, 1
                        continue;
                    }
                    if (i == 1 && y < 10) { // 129, 0
                        continue;
                    }
                    if (i == 6 && y < 10) { // 133, 0
                        continue;
                    }
                    if (i == 7 && y == 0) { // 133, 1
                        continue;
                    }
                    if (i == 12 && (y < MICRO_HEIGHT - (x - 4) / 2)) { // 151, 4
                        continue;
                    }
                    if (i == 13 && ((x < 6 && y < 18) || (x >= 6 && y < 18 - (x - 6) / 2))) { // 151, 5
                        continue;
                    }
                    if (i == 14 && (y < 16 - (x - 3) / 2)) { // 152, 7
                        continue;
                    }
                    if (i == 16 && y < 21) { // 153, 4
                        continue;
                    }
                    if (i == 17 && x < 10 && y < 3) { // 159, 1
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        if (change) {
            frame->setFrameType(D1CEL_FRAME_TYPE::TransparentSquare);
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }
}

void D1Tileset::fixCryptShadows(bool silent)
{
    typedef struct {
        int subtileIndex;
        unsigned microIndex;
        D1CEL_FRAME_TYPE res_encoding;
    } CelMicro;
    const CelMicro micros[] = {
        // clang-format off
        { 626 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },  // 1806 - 205
        { 626 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // 1807
        // { 627 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },  // 1808
        { 638 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // 1824 - 211
        { 639 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },  // 1825
        { 639 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // 1799
        // { 631 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // 1815 - 207
        { 634 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },  // 1818 - 208
        { 634 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // 1819
        { 213 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },  // 589 - 71

        { 630 - 1, 0, D1CEL_FRAME_TYPE::RightTriangle },     // 1813 - '28'
        { 632 - 1, 0, D1CEL_FRAME_TYPE::Square },            // 1816
        { 633 - 1, 0, D1CEL_FRAME_TYPE::RightTriangle },     // 1817
        { 277 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare }, // 722 - 96
        { 620 - 1, 0, D1CEL_FRAME_TYPE::RightTriangle },     // 1798 - '109'
        { 621 - 1, 1, D1CEL_FRAME_TYPE::Square },            // 1800
        { 625 - 1, 0, D1CEL_FRAME_TYPE::RightTriangle },     // 1805 - '215'
        { 624 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // 1804
        { 619 - 1, 1, D1CEL_FRAME_TYPE::LeftTrapezoid },     // 1797 - '109' + '215'
        {  77 - 1, 1, D1CEL_FRAME_TYPE::Empty }, // 268
        {  77 - 1, 2, D1CEL_FRAME_TYPE::Empty }, // 266
        { 206 - 1, 1, D1CEL_FRAME_TYPE::Empty }, // 572
        { 303 - 1, 1, D1CEL_FRAME_TYPE::Empty }, // 797
        {  15 - 1, 1, D1CEL_FRAME_TYPE::Empty }, // 14
        {  15 - 1, 2, D1CEL_FRAME_TYPE::Empty }, // 12
        {  89 - 1, 1, D1CEL_FRAME_TYPE::Empty }, // 311
        {  89 - 1, 2, D1CEL_FRAME_TYPE::Empty }, // 309
        // clang-format on
    };

    // TODO: check if there are enough subtiles
    constexpr unsigned blockSize = BLOCK_SIZE_L5;
    const D1GfxPixel SHADOW_COLOR = D1GfxPixel::colorPixel(0); // 79;
    for (int i = 0; i < 8; i++) {
        const CelMicro &micro = micros[i];
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return;
        }

        bool change = false;
        for (int y = 0; y < MICRO_WIDTH; y++) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                D1GfxPixel pixel = frame->getPixel(x, y);
                if (pixel.isTransparent()) {
                    continue;
                }
                quint8 color = pixel.getPaletteIndex();
                if (color != 79) {
                    // extend the shadows to NE: 205[2][0, 1], 205[3][0]
                    if (i == 0 && y <= (x / 2) + 13 - MICRO_HEIGHT / 2) { // 1806
                        continue;
                    }
                    if (i == 1 && y <= (x / 2) + 13) { // 1807
                        continue;
                    }
                    // if (i == 2 && y <= (x / 2) + 13 - MICRO_HEIGHT / 2) { // 1808
                    //    continue;
                    // }
                    // extend the shadows to NW: 211[0][1], 211[1][0]
                    if (i == 2 && y <= 13 - (x / 2)) { // 1824
                        continue;
                    }
                    if (i == 3 && (x > 19 || y > 23) && (x != 16 || y != 24)) { // 1825
                        continue;
                    }
                    if (i == 4 && x <= 7) { // 1799
                        continue;
                    }
                    // extend the shadows to NW: 207[2][1]
                    // if (i == 6 && y <= (x / 2) + 15) { // 1815
                    //    continue;
                    // }
                    // extend the shadows to NE: 208[2][0, 1]
                    if (i == 5 && (y <= (x / 2) - 3 || (x >= 20 && y >= 14 && color >= 59 && color <= 95 && (color >= 77 || color <= 63)))) { // 1818
                        continue;
                    }
                    if (i == 6 && (y <= (x / 2) + 13 || (x <= 8 && y >= 12 && color >= 62 && color <= 95 && (color >= 80 || color <= 63)))) { // 1819
                        continue;
                    }
                    // add shadow to the floor
                    if (i == 7 && y <= (x / 2) - 3) { // 589
                        continue;
                    }
                }
                change |= frame->setPixel(x, y, SHADOW_COLOR);
            }
        }

        // fix bad artifacts
        if (i == 5) { // 1818
            change |= frame->setPixel(22, 20, SHADOW_COLOR);
        }

        if (change) {
            // frame->setFrameType(micro.res_encoding);
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }
    for (int i = 8; i < 17; i++) {
        const CelMicro &micro = micros[i];
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return;
        }

        D1GfxFrame *frameSrc = nullptr;
        if (i != 8 + 8) { // 1797
            const CelMicro &microSrc = micros[i + 9];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            frameSrc = mf.second;
            if (frameSrc == nullptr) {
                return;
            }
        }
        bool change = false;
        for (int y = 0; y < MICRO_WIDTH; y++) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                //  use consistent lava + shadow micro II.
                if (i == 8 + 3) { // 722
                    if (x > 11) {
                        continue;
                    }
                    if (x == 11 && (y < 9 || (y >= 17 && y <= 20))) {
                        continue;
                    }
                    if (x == 10 && (y == 18 || y == 19)) {
                        continue;
                    }
                    D1GfxPixel pixelSrc = frameSrc->getPixel(x, y);
                    if (pixelSrc.isTransparent()) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, pixelSrc);
                    continue;
                }

                D1GfxPixel srcPixel = frameSrc != nullptr ? frameSrc->getPixel(x, y) : SHADOW_COLOR;
                if (i == 8 + 8) { // 1797
                    srcPixel = y > 16 + x / 2 ? D1GfxPixel::transparentPixel() : SHADOW_COLOR;
                }
                if (i == 8 + 4 && !srcPixel.isTransparent()) { // 14 -> 1798
                    // wall/floor in shadow
                    if (x <= 1) {
                        if (y >= 4 * x) {
                            srcPixel = SHADOW_COLOR;
                        }
                    } else if (x <= 3) {
                        if (y > 6 + (x - 1) / 2) {
                            srcPixel = SHADOW_COLOR;
                        }
                    } else if (x <= 5) {
                        if (y >= 7 + 4 * (x - 3)) {
                            srcPixel = SHADOW_COLOR;
                        }
                    } else {
                        if (y >= 14 + (x - 1) / 2) {
                            srcPixel = SHADOW_COLOR;
                        }
                    }
                }
                if (i == 8 + 6 && !srcPixel.isTransparent()) { // 311 -> 1805
                    // grate/floor in shadow
                    if (x <= 1 && y >= 7 * x) {
                        srcPixel = SHADOW_COLOR;
                    }
                    if (x > 1 && y > 14 + (x - 1) / 2) {
                        srcPixel = SHADOW_COLOR;
                    }
                }
                if (i == 8 + 0 && !srcPixel.isTransparent()) { // 266 -> 1813
                    // closed door in shadow
                    if (x < 3) {
                        if (y >= 7 * x - 1) {
                            srcPixel = SHADOW_COLOR;
                        }
                    } else if (x < 7) {
                        if (y >= 13 + (x - 3) / 2) {
                            srcPixel = SHADOW_COLOR;
                        }
                    } else if (x < 8) {
                        if (y >= 15 + 4 * (x - 7)) {
                            srcPixel = SHADOW_COLOR;
                        }
                    } else {
                        if (y >= 14 + (x - 1) / 2) {
                            srcPixel = SHADOW_COLOR;
                        }
                    }
                }
                if (i == 8 + 2 && !srcPixel.isTransparent()) { // 572 -> 1817
                    // open door in shadow
                    if (y > 14 + (x - 1) / 2) {
                        srcPixel = SHADOW_COLOR;
                    }
                }
                if (i == 8 + 1 || i == 8 + 5 || i == 8 + 7) { // 12, 309 -> 1800, 1804
                    // door/wall/grate in shadow
                    if (y >= 7 * (x - 27) && !srcPixel.isTransparent()) {
                        srcPixel = SHADOW_COLOR;
                    }
                }

                change |= frame->setPixel(x, y, srcPixel);
            }
        }
        if (change) {
            frame->setFrameType(micro.res_encoding);
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }
}

void D1Tileset::cleanupCrypt(std::set<unsigned> &deletedFrames, bool silent)
{
    constexpr int blockSize = BLOCK_SIZE_L5;

    // use common subtiles of doors
    ReplaceSubtile(this->til, 71 - 1, 2, 206, silent);
    ReplaceSubtile(this->til, 72 - 1, 2, 206, silent);
    // use common subtiles
    ReplaceSubtile(this->til, 4 - 1, 1, 6, silent); // 14
    ReplaceSubtile(this->til, 14 - 1, 1, 6, silent);
    ReplaceSubtile(this->til, 115 - 1, 1, 6, silent);
    ReplaceSubtile(this->til, 132 - 1, 1, 6, silent);
    ReplaceSubtile(this->til, 1 - 1, 2, 15, silent); // 3
    ReplaceSubtile(this->til, 27 - 1, 2, 15, silent);
    ReplaceSubtile(this->til, 43 - 1, 2, 15, silent);
    ReplaceSubtile(this->til, 79 - 1, 2, 15, silent);
    ReplaceSubtile(this->til, 6 - 1, 2, 15, silent);   // 23
    ReplaceSubtile(this->til, 127 - 1, 2, 4, silent);  // 372
    ReplaceSubtile(this->til, 132 - 1, 2, 15, silent); // 388
    ReplaceSubtile(this->til, 156 - 1, 2, 31, silent); // 468
    // use better subtiles
    // - increase glow
    ReplaceSubtile(this->til, 96 - 1, 3, 293, silent); // 279
    ReplaceSubtile(this->til, 187 - 1, 3, 293, silent);
    ReplaceSubtile(this->til, 188 - 1, 3, 293, silent);
    ReplaceSubtile(this->til, 90 - 1, 1, 297, silent); // 253
    ReplaceSubtile(this->til, 175 - 1, 1, 297, silent);
    // - reduce glow
    ReplaceSubtile(this->til, 162 - 1, 1, 297, silent); // 489
    ReplaceSubtile(this->til, 162 - 1, 2, 266, silent); // 490
    // create the new shadows
    // - use the shadows created by fixCryptShadows
    ReplaceSubtile(this->til, 203 - 1, 0, 638, silent); // 619
    ReplaceSubtile(this->til, 203 - 1, 1, 639, silent); // 620
    ReplaceSubtile(this->til, 203 - 1, 2, 623, silent); // 47
    ReplaceSubtile(this->til, 203 - 1, 3, 627, silent); // 621
    ReplaceSubtile(this->til, 204 - 1, 0, 638, silent); // 622
    ReplaceSubtile(this->til, 204 - 1, 1, 639, silent); // 46
    ReplaceSubtile(this->til, 204 - 1, 2, 636, silent); // 623
    ReplaceSubtile(this->til, 204 - 1, 3, 627, silent); // 624
    ReplaceSubtile(this->til, 108 - 1, 2, 631, silent); // 810
    ReplaceSubtile(this->til, 108 - 1, 3, 626, silent); // 811
    ReplaceSubtile(this->til, 210 - 1, 3, 371, silent); // 637

    ReplaceSubtile(this->til, 28 - 1, 0, 75, silent);   // 86
    ReplaceSubtile(this->til, 28 - 1, 1, 4, silent);    // 80
    ReplaceSubtile(this->til, 28 - 1, 2, 216, silent);  // 77
    ReplaceSubtile(this->til, 28 - 1, 3, 4, silent);    // 87
    ReplaceSubtile(this->til, 109 - 1, 0, 1, silent);   // 312
    ReplaceSubtile(this->til, 109 - 1, 1, 2, silent);   // 313
    ReplaceSubtile(this->til, 109 - 1, 2, 3, silent);   // 314
    ReplaceSubtile(this->til, 109 - 1, 3, 627, silent); // 315
    ReplaceSubtile(this->til, 110 - 1, 0, 21, silent);  // 316
    ReplaceSubtile(this->til, 110 - 1, 1, 22, silent);  // 313
    ReplaceSubtile(this->til, 110 - 1, 2, 3, silent);   // 314
    ReplaceSubtile(this->til, 110 - 1, 3, 627, silent); // 315
    ReplaceSubtile(this->til, 111 - 1, 0, 39, silent);  // 317
    ReplaceSubtile(this->til, 111 - 1, 1, 4, silent);   // 318
    ReplaceSubtile(this->til, 111 - 1, 2, 242, silent); // 319
    ReplaceSubtile(this->til, 111 - 1, 3, 627, silent); // 320
    ReplaceSubtile(this->til, 215 - 1, 0, 101, silent); // 645
    ReplaceSubtile(this->til, 215 - 1, 1, 4, silent);   // 646
    ReplaceSubtile(this->til, 215 - 1, 2, 178, silent); // 45
    ReplaceSubtile(this->til, 215 - 1, 3, 627, silent); // 647
    // - 'add' new shadow-types with glow
    ReplaceSubtile(this->til, 216 - 1, 0, 39, silent);  // 622
    ReplaceSubtile(this->til, 216 - 1, 1, 4, silent);   // 46
    ReplaceSubtile(this->til, 216 - 1, 2, 238, silent); // 648
    ReplaceSubtile(this->til, 216 - 1, 3, 635, silent); // 624
    ReplaceSubtile(this->til, 217 - 1, 0, 638, silent); // 625
    ReplaceSubtile(this->til, 217 - 1, 1, 639, silent); // 46
    ReplaceSubtile(this->til, 217 - 1, 2, 634, silent); // 649
    ReplaceSubtile(this->til, 217 - 1, 3, 635, silent); // 650
    // - 'add' new shadow-types with horizontal arches
    ReplaceSubtile(this->til, 71 - 1, 0, 5, silent); // copy from tile 2
    ReplaceSubtile(this->til, 71 - 1, 1, 6, silent);
    ReplaceSubtile(this->til, 71 - 1, 2, 631, silent);
    ReplaceSubtile(this->til, 71 - 1, 3, 627, silent);
    ReplaceSubtile(this->til, 80 - 1, 0, 5, silent); // copy from tile 2
    ReplaceSubtile(this->til, 80 - 1, 1, 6, silent);
    ReplaceSubtile(this->til, 80 - 1, 2, 623, silent);
    ReplaceSubtile(this->til, 80 - 1, 3, 627, silent);

    ReplaceSubtile(this->til, 81 - 1, 0, 42, silent); // copy from tile 12
    ReplaceSubtile(this->til, 81 - 1, 1, 34, silent);
    ReplaceSubtile(this->til, 81 - 1, 2, 631, silent);
    ReplaceSubtile(this->til, 81 - 1, 3, 627, silent);
    ReplaceSubtile(this->til, 82 - 1, 0, 42, silent); // copy from tile 12
    ReplaceSubtile(this->til, 82 - 1, 1, 34, silent);
    ReplaceSubtile(this->til, 82 - 1, 2, 623, silent);
    ReplaceSubtile(this->til, 82 - 1, 3, 627, silent);

    ReplaceSubtile(this->til, 83 - 1, 0, 104, silent); // copy from tile 36
    ReplaceSubtile(this->til, 83 - 1, 1, 84, silent);
    ReplaceSubtile(this->til, 83 - 1, 2, 631, silent);
    ReplaceSubtile(this->til, 83 - 1, 3, 627, silent);
    ReplaceSubtile(this->til, 84 - 1, 0, 104, silent); // copy from tile 36
    ReplaceSubtile(this->til, 84 - 1, 1, 84, silent);
    ReplaceSubtile(this->til, 84 - 1, 2, 623, silent);
    ReplaceSubtile(this->til, 84 - 1, 3, 627, silent);

    ReplaceSubtile(this->til, 85 - 1, 0, 25, silent); // copy from tile 7
    ReplaceSubtile(this->til, 85 - 1, 1, 6, silent);
    ReplaceSubtile(this->til, 85 - 1, 2, 631, silent);
    ReplaceSubtile(this->til, 85 - 1, 3, 627, silent);
    ReplaceSubtile(this->til, 86 - 1, 0, 25, silent); // copy from tile 7
    ReplaceSubtile(this->til, 86 - 1, 1, 6, silent);
    ReplaceSubtile(this->til, 86 - 1, 2, 623, silent);
    ReplaceSubtile(this->til, 86 - 1, 3, 627, silent);

    ReplaceSubtile(this->til, 87 - 1, 0, 79, silent); // copy from tile 26
    ReplaceSubtile(this->til, 87 - 1, 1, 80, silent);
    ReplaceSubtile(this->til, 87 - 1, 2, 623, silent);
    ReplaceSubtile(this->til, 87 - 1, 3, 627, silent);
    ReplaceSubtile(this->til, 88 - 1, 0, 79, silent); // copy from tile 26
    ReplaceSubtile(this->til, 88 - 1, 1, 80, silent);
    ReplaceSubtile(this->til, 88 - 1, 2, 631, silent);
    ReplaceSubtile(this->til, 88 - 1, 3, 627, silent);

    // use common subtiles instead of minor alterations
    ReplaceSubtile(this->til, 7 - 1, 1, 6, silent);    // 26
    ReplaceSubtile(this->til, 159 - 1, 1, 6, silent);  // 479
    ReplaceSubtile(this->til, 133 - 1, 2, 31, silent); // 390
    ReplaceSubtile(this->til, 10 - 1, 1, 18, silent);  // 37
    ReplaceSubtile(this->til, 138 - 1, 1, 18, silent);
    ReplaceSubtile(this->til, 188 - 1, 1, 277, silent); // 564
    ReplaceSubtile(this->til, 178 - 1, 2, 258, silent); // 564
    ReplaceSubtile(this->til, 5 - 1, 2, 31, silent);    // 19
    ReplaceSubtile(this->til, 14 - 1, 2, 31, silent);
    ReplaceSubtile(this->til, 159 - 1, 2, 31, silent);
    ReplaceSubtile(this->til, 185 - 1, 2, 274, silent); // 558
    ReplaceSubtile(this->til, 186 - 1, 2, 274, silent); // 560
    ReplaceSubtile(this->til, 139 - 1, 0, 39, silent);  // 402

    ReplaceSubtile(this->til, 2 - 1, 3, 4, silent);  // 8
    ReplaceSubtile(this->til, 3 - 1, 1, 60, silent); // 10
    ReplaceSubtile(this->til, 114 - 1, 1, 32, silent);
    ReplaceSubtile(this->til, 3 - 1, 2, 4, silent); // 11
    ReplaceSubtile(this->til, 114 - 1, 2, 4, silent);
    ReplaceSubtile(this->til, 5 - 1, 3, 7, silent); // 20
    ReplaceSubtile(this->til, 14 - 1, 3, 4, silent);
    ReplaceSubtile(this->til, 133 - 1, 3, 4, silent);
    ReplaceSubtile(this->til, 125 - 1, 3, 7, silent); // 50
    ReplaceSubtile(this->til, 159 - 1, 3, 7, silent);
    ReplaceSubtile(this->til, 4 - 1, 3, 7, silent); // 16
    ReplaceSubtile(this->til, 132 - 1, 3, 4, silent);
    ReplaceSubtile(this->til, 10 - 1, 3, 7, silent); // 38
    ReplaceSubtile(this->til, 138 - 1, 3, 4, silent);
    ReplaceSubtile(this->til, 121 - 1, 3, 4, silent); // 354
    ReplaceSubtile(this->til, 8 - 1, 3, 4, silent);   // 32
    ReplaceSubtile(this->til, 136 - 1, 3, 7, silent);
    ReplaceSubtile(this->til, 91 - 1, 1, 47, silent); // 257
    ReplaceSubtile(this->til, 178 - 1, 1, 47, silent);
    ReplaceSubtile(this->til, 91 - 1, 3, 48, silent); // 259
    ReplaceSubtile(this->til, 177 - 1, 3, 7, silent);
    ReplaceSubtile(this->til, 178 - 1, 3, 48, silent);
    ReplaceSubtile(this->til, 130 - 1, 2, 395, silent); // 381
    ReplaceSubtile(this->til, 157 - 1, 2, 4, silent);   // 472
    ReplaceSubtile(this->til, 177 - 1, 1, 4, silent);   // 540
    ReplaceSubtile(this->til, 211 - 1, 3, 48, silent);  // 621
    ReplaceSubtile(this->til, 205 - 1, 0, 45, silent);  // 625
    ReplaceSubtile(this->til, 207 - 1, 0, 45, silent);  // 630
    ReplaceSubtile(this->til, 207 - 1, 3, 627, silent); // 632
    ReplaceSubtile(this->til, 208 - 1, 0, 45, silent);  // 633

    ReplaceSubtile(this->til, 27 - 1, 3, 4, silent); // 85
    // ReplaceSubtile(this->til, 28 - 1, 3, 4, silent); // 87
    ReplaceSubtile(this->til, 29 - 1, 3, 4, silent); // 90
    ReplaceSubtile(this->til, 30 - 1, 3, 4, silent); // 92
    ReplaceSubtile(this->til, 31 - 1, 3, 4, silent); // 94
    ReplaceSubtile(this->til, 32 - 1, 3, 4, silent); // 96
    ReplaceSubtile(this->til, 33 - 1, 3, 4, silent); // 98
    ReplaceSubtile(this->til, 34 - 1, 3, 4, silent); // 100
    ReplaceSubtile(this->til, 37 - 1, 3, 4, silent); // 108
    ReplaceSubtile(this->til, 38 - 1, 3, 4, silent); // 110
    ReplaceSubtile(this->til, 39 - 1, 3, 4, silent); // 112
    ReplaceSubtile(this->til, 40 - 1, 3, 4, silent); // 114
    ReplaceSubtile(this->til, 41 - 1, 3, 4, silent); // 116
    ReplaceSubtile(this->til, 42 - 1, 3, 4, silent); // 118
    ReplaceSubtile(this->til, 43 - 1, 3, 4, silent); // 120
    ReplaceSubtile(this->til, 44 - 1, 3, 4, silent); // 122
    ReplaceSubtile(this->til, 45 - 1, 3, 4, silent); // 124
    // ReplaceSubtile(this->til, 71 - 1, 3, 4, silent); // 214
    ReplaceSubtile(this->til, 72 - 1, 3, 4, silent); // 217
    ReplaceSubtile(this->til, 73 - 1, 3, 4, silent); // 219
    ReplaceSubtile(this->til, 74 - 1, 3, 4, silent); // 221
    ReplaceSubtile(this->til, 75 - 1, 3, 4, silent); // 223
    ReplaceSubtile(this->til, 76 - 1, 3, 4, silent); // 225
    ReplaceSubtile(this->til, 77 - 1, 3, 4, silent); // 227
    ReplaceSubtile(this->til, 78 - 1, 3, 4, silent); // 229
    ReplaceSubtile(this->til, 79 - 1, 3, 4, silent); // 231
    // ReplaceSubtile(this->til, 80 - 1, 3, 4, silent); // 233
    // ReplaceSubtile(this->til, 81 - 1, 3, 4, silent); // 235
    ReplaceSubtile(this->til, 15 - 1, 1, 4, silent); // 52
    ReplaceSubtile(this->til, 15 - 1, 2, 4, silent); // 53
    ReplaceSubtile(this->til, 15 - 1, 3, 4, silent); // 54
    ReplaceSubtile(this->til, 16 - 1, 1, 4, silent); // 56
    ReplaceSubtile(this->til, 144 - 1, 1, 4, silent);
    ReplaceSubtile(this->til, 16 - 1, 2, 4, silent); // 57
    ReplaceSubtile(this->til, 16 - 1, 3, 4, silent); // 58
    ReplaceSubtile(this->til, 144 - 1, 3, 7, silent);
    ReplaceSubtile(this->til, 94 - 1, 2, 60, silent); // 270
    ReplaceSubtile(this->til, 183 - 1, 2, 60, silent);
    ReplaceSubtile(this->til, 184 - 1, 2, 60, silent);
    ReplaceSubtile(this->til, 17 - 1, 2, 4, silent); // 61
    ReplaceSubtile(this->til, 128 - 1, 2, 4, silent);
    ReplaceSubtile(this->til, 92 - 1, 2, 62, silent); // 262
    ReplaceSubtile(this->til, 179 - 1, 2, 62, silent);
    ReplaceSubtile(this->til, 25 - 1, 1, 4, silent); // 76
    ReplaceSubtile(this->til, 25 - 1, 3, 4, silent); // 78
    ReplaceSubtile(this->til, 35 - 1, 1, 4, silent); // 102
    ReplaceSubtile(this->til, 35 - 1, 3, 4, silent); // 103
    ReplaceSubtile(this->til, 69 - 1, 1, 4, silent); // 205
    ReplaceSubtile(this->til, 69 - 1, 3, 4, silent); // 207
    ReplaceSubtile(this->til, 26 - 1, 2, 4, silent); // 81
    ReplaceSubtile(this->til, 26 - 1, 3, 4, silent); // 82
    ReplaceSubtile(this->til, 36 - 1, 2, 4, silent); // 105
    ReplaceSubtile(this->til, 36 - 1, 3, 4, silent); // 106
    ReplaceSubtile(this->til, 46 - 1, 2, 4, silent); // 127
    ReplaceSubtile(this->til, 46 - 1, 3, 4, silent); // 128
    ReplaceSubtile(this->til, 70 - 1, 2, 4, silent); // 210
    ReplaceSubtile(this->til, 70 - 1, 3, 4, silent); // 211
    ReplaceSubtile(this->til, 49 - 1, 1, 4, silent); // 137
    ReplaceSubtile(this->til, 167 - 1, 1, 4, silent);
    ReplaceSubtile(this->til, 49 - 1, 2, 4, silent); // 138
    ReplaceSubtile(this->til, 167 - 1, 2, 4, silent);
    ReplaceSubtile(this->til, 49 - 1, 3, 4, silent); // 139
    ReplaceSubtile(this->til, 167 - 1, 3, 4, silent);
    ReplaceSubtile(this->til, 50 - 1, 1, 4, silent);  // 141
    ReplaceSubtile(this->til, 50 - 1, 3, 4, silent);  // 143
    ReplaceSubtile(this->til, 51 - 1, 3, 4, silent);  // 147
    ReplaceSubtile(this->til, 103 - 1, 1, 4, silent); // 295
    ReplaceSubtile(this->til, 105 - 1, 1, 4, silent);
    ReplaceSubtile(this->til, 127 - 1, 3, 4, silent); // 373
    ReplaceSubtile(this->til, 89 - 1, 3, 4, silent);  // 251
    ReplaceSubtile(this->til, 173 - 1, 3, 7, silent);
    ReplaceSubtile(this->til, 174 - 1, 3, 7, silent);
    ReplaceSubtile(this->til, 6 - 1, 3, 4, silent); // 24
    ReplaceSubtile(this->til, 134 - 1, 3, 7, silent);
    ReplaceSubtile(this->til, 7 - 1, 3, 7, silent); // 28
    ReplaceSubtile(this->til, 8 - 1, 1, 2, silent); // 30
    ReplaceSubtile(this->til, 30 - 1, 1, 2, silent);
    ReplaceSubtile(this->til, 32 - 1, 1, 2, silent);
    ReplaceSubtile(this->til, 72 - 1, 1, 2, silent);
    ReplaceSubtile(this->til, 9 - 1, 3, 4, silent); // 35
    ReplaceSubtile(this->til, 137 - 1, 3, 4, silent);
    ReplaceSubtile(this->til, 11 - 1, 1, 4, silent); // 40
    ReplaceSubtile(this->til, 122 - 1, 1, 4, silent);
    ReplaceSubtile(this->til, 12 - 1, 2, 4, silent); // 43
    ReplaceSubtile(this->til, 123 - 1, 2, 4, silent);
    ReplaceSubtile(this->til, 12 - 1, 3, 4, silent); // 44
    ReplaceSubtile(this->til, 123 - 1, 3, 7, silent);
    ReplaceSubtile(this->til, 95 - 1, 1, 4, silent); // 273
    ReplaceSubtile(this->til, 185 - 1, 1, 7, silent);
    ReplaceSubtile(this->til, 186 - 1, 1, 4, silent);
    ReplaceSubtile(this->til, 89 - 1, 1, 293, silent); // 249
    ReplaceSubtile(this->til, 173 - 1, 1, 293, silent);
    ReplaceSubtile(this->til, 174 - 1, 1, 293, silent);
    ReplaceSubtile(this->til, 92 - 1, 3, 271, silent); // 263
    ReplaceSubtile(this->til, 179 - 1, 3, 271, silent);
    ReplaceSubtile(this->til, 96 - 1, 2, 12, silent); // 278
    ReplaceSubtile(this->til, 187 - 1, 2, 12, silent);
    ReplaceSubtile(this->til, 188 - 1, 2, 12, silent);
    // patch dMiniTiles - L5.MIN
    // prepare subtiles after fixCryptShadows
    ReplaceMcr(3, 0, 619, 1);
    ReplaceMcr(3, 1, 620, 0);
    ReplaceMcr(3, 2, 621, 1);
    Blk2Mcr(3, 4);
    ReplaceMcr(3, 6, 15, 6);
    // SetMcr(242, 0, 630, 0);
    SetMcr(242, 0, 626, 0);
    ReplaceMcr(242, 1, 626, 1);
    // SetMcr(242, 2, 31, 2);
    Blk2Mcr(242, 4);
    ReplaceMcr(242, 6, 31, 6);
    SetMcr(242, 8, 31, 8);
    ReplaceMcr(178, 0, 619, 1);
    ReplaceMcr(178, 1, 625, 0);
    SetMcr(178, 2, 624, 0);
    Blk2Mcr(178, 4);
    ReplaceMcr(178, 6, 31, 6);
    ReplaceMcr(178, 8, 31, 8);
    ReplaceMcr(238, 0, 634, 0);
    ReplaceMcr(238, 1, 634, 1);
    Blk2Mcr(238, 4);
    SetMcr(238, 6, 31, 6);
    SetMcr(238, 8, 31, 8);
    SetMcr(213, 1, 633, 0);
    Blk2Mcr(213, 6);
    Blk2Mcr(213, 8);
    ReplaceMcr(216, 0, 619, 1);
    SetMcr(216, 1, 630, 0);
    SetMcr(216, 2, 632, 0);
    Blk2Mcr(216, 6);
    Blk2Mcr(216, 8);
    // pointless door micros (re-drawn by dSpecial)
    Blk2Mcr(77, 6);
    Blk2Mcr(77, 8);
    Blk2Mcr(80, 7);
    Blk2Mcr(80, 9);
    Blk2Mcr(206, 6);
    Blk2Mcr(206, 8);
    Blk2Mcr(209, 7);
    Blk2Mcr(209, 9);
    // Blk2Mcr(213, 6);
    // Blk2Mcr(213, 8);
    // Blk2Mcr(216, 6);
    // Blk2Mcr(216, 8);
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
    // Blk2Mcr(172, 0);
    // Blk2Mcr(172, 1);
    // Blk2Mcr(172, 2);
    Blk2Mcr(173, 0);
    Blk2Mcr(173, 1);
    // Blk2Mcr(174, 0);
    // Blk2Mcr(174, 1);
    // Blk2Mcr(174, 2);
    // Blk2Mcr(174, 4);
    Blk2Mcr(175, 0);
    Blk2Mcr(175, 1);
    // Blk2Mcr(176, 0);
    // Blk2Mcr(176, 1);
    // Blk2Mcr(176, 3);
    // Blk2Mcr(177, 0);
    // Blk2Mcr(177, 1);
    // Blk2Mcr(177, 3);
    // Blk2Mcr(177, 5);
    // Blk2Mcr(178, 0);
    // Blk2Mcr(178, 1);
    Blk2Mcr(179, 0);
    Blk2Mcr(179, 1);
    // fix 'bad' artifact
    Blk2Mcr(398, 5);
    // fix graphical glitch
    ReplaceMcr(21, 1, 55, 1);
    ReplaceMcr(25, 0, 33, 0);
    ReplaceMcr(22, 0, 2, 0);
    ReplaceMcr(22, 1, 2, 1);
    ReplaceMcr(336, 1, 4, 1);
    ReplaceMcr(339, 0, 33, 0);
    // ReplaceMcr(391, 1, 335, 1); - the whole right side is replaced
    ReplaceMcr(393, 0, 33, 0);
    ReplaceMcr(421, 0, 399, 0);
    ReplaceMcr(421, 2, 399, 2);
    // - fix crack in the chair
    ReplaceMcr(162, 5, 154, 5);
    ReplaceMcr(162, 3, 154, 3);
    // - use consistent lava + shadow micro I.
    ReplaceMcr(277, 0, 303, 0);
    ReplaceMcr(562, 0, 303, 0);
    ReplaceMcr(564, 0, 303, 0);
    ReplaceMcr(635, 0, 308, 0);
    // - extend shadow to make more usable (after fixCryptShadows)
    ReplaceMcr(627, 0, 626, 0);
    SetMcr(627, 1, 626, 1);
    // prepare new subtiles for the shadows
    ReplaceMcr(623, 0, 631, 0);
    ReplaceMcr(623, 1, 638, 1);
    ReplaceMcr(636, 0, 626, 0);
    ReplaceMcr(636, 1, 638, 1);
    // reuse subtiles
    ReplaceMcr(631, 1, 626, 1);
    ReplaceMcr(149, 4, 1, 4);
    ReplaceMcr(150, 6, 15, 6);
    ReplaceMcr(324, 7, 6, 7);
    ReplaceMcr(432, 7, 6, 7);
    ReplaceMcr(440, 7, 6, 7);
    // ReplaceMcr(26, 9, 6, 9);
    ReplaceMcr(340, 9, 6, 9);
    ReplaceMcr(394, 9, 6, 9);
    ReplaceMcr(451, 9, 6, 9);
    // ReplaceMcr(14, 7, 6, 7); // lost shine
    // ReplaceMcr(26, 7, 6, 7); // lost shine
    // ReplaceMcr(80, 7, 6, 7);
    ReplaceMcr(451, 7, 6, 7); // lost shine
    ReplaceMcr(340, 7, 6, 7); // lost shine
    ReplaceMcr(364, 7, 6, 7); // lost crack
    ReplaceMcr(394, 7, 6, 7); // lost shine
    ReplaceMcr(554, 7, 269, 7);
    ReplaceMcr(608, 7, 6, 7);   // lost details
    ReplaceMcr(616, 7, 6, 7);   // lost details
    ReplaceMcr(269, 5, 554, 5); // lost details
    ReplaceMcr(556, 5, 554, 5);
    ReplaceMcr(440, 5, 432, 5); // lost details
    // ReplaceMcr(14, 5, 6, 5); // lost details
    // ReplaceMcr(26, 5, 6, 5); // lost details
    ReplaceMcr(451, 5, 6, 5);   // lost details
    ReplaceMcr(80, 5, 6, 5);    // lost details
    ReplaceMcr(324, 5, 432, 5); // lost details
    ReplaceMcr(340, 5, 432, 5); // lost details
    ReplaceMcr(364, 5, 432, 5); // lost details
    ReplaceMcr(380, 5, 432, 5); // lost details
    ReplaceMcr(394, 5, 432, 5); // lost details
    ReplaceMcr(6, 3, 14, 3);    // lost details
    // ReplaceMcr(26, 3, 14, 3);   // lost details
    ReplaceMcr(80, 3, 14, 3);  // lost details
    ReplaceMcr(269, 3, 14, 3); // lost details
    ReplaceMcr(414, 3, 14, 3); // lost details
    ReplaceMcr(451, 3, 14, 3); // lost details
    ReplaceMcr(554, 3, 14, 3); // lost details
    ReplaceMcr(556, 3, 14, 3); // lost details
    // ? ReplaceMcr(608, 3, 103, 3); // lost details
    ReplaceMcr(324, 3, 380, 3); // lost details
    ReplaceMcr(324, 3, 380, 3); // lost details
    ReplaceMcr(340, 3, 380, 3); // lost details
    ReplaceMcr(364, 3, 380, 3); // lost details
    ReplaceMcr(432, 3, 380, 3); // lost details
    ReplaceMcr(440, 3, 380, 3); // lost details
    ReplaceMcr(6, 0, 14, 0);
    // ReplaceMcr(26, 0, 14, 0);
    ReplaceMcr(269, 0, 14, 0);
    ReplaceMcr(554, 0, 14, 0);  // lost details
    ReplaceMcr(340, 0, 324, 0); // lost details
    ReplaceMcr(364, 0, 324, 0); // lost details
    ReplaceMcr(451, 0, 324, 0); // lost details
    // ReplaceMcr(14, 1, 6, 1);
    // ReplaceMcr(26, 1, 6, 1);
    ReplaceMcr(80, 1, 6, 1);
    ReplaceMcr(269, 1, 6, 1);
    ReplaceMcr(380, 1, 6, 1);
    ReplaceMcr(451, 1, 6, 1);
    ReplaceMcr(554, 1, 6, 1);
    ReplaceMcr(556, 1, 6, 1);
    ReplaceMcr(324, 1, 340, 1);
    ReplaceMcr(364, 1, 340, 1);
    ReplaceMcr(394, 1, 340, 1); // lost details
    ReplaceMcr(432, 1, 340, 1); // lost details
    ReplaceMcr(551, 5, 265, 5);
    ReplaceMcr(551, 0, 265, 0);
    ReplaceMcr(551, 1, 265, 1);
    ReplaceMcr(261, 0, 14, 0); // lost details
    ReplaceMcr(545, 0, 14, 0); // lost details
    ReplaceMcr(18, 9, 6, 9);   // lost details
    ReplaceMcr(34, 9, 6, 9);   // lost details
    // ReplaceMcr(37, 9, 6, 9);
    ReplaceMcr(277, 9, 6, 9);   // lost details
    ReplaceMcr(332, 9, 6, 9);   // lost details
    ReplaceMcr(348, 9, 6, 9);   // lost details
    ReplaceMcr(352, 9, 6, 9);   // lost details
    ReplaceMcr(358, 9, 6, 9);   // lost details
    ReplaceMcr(406, 9, 6, 9);   // lost details
    ReplaceMcr(444, 9, 6, 9);   // lost details
    ReplaceMcr(459, 9, 6, 9);   // lost details
    ReplaceMcr(463, 9, 6, 9);   // lost details
    ReplaceMcr(562, 9, 6, 9);   // lost details
    ReplaceMcr(564, 9, 6, 9);   // lost details
    ReplaceMcr(277, 7, 18, 7);  // lost details
    ReplaceMcr(562, 7, 18, 7);  // lost details
    ReplaceMcr(277, 5, 459, 5); // lost details
    ReplaceMcr(562, 5, 459, 5); // lost details
    ReplaceMcr(277, 3, 459, 3); // lost details
    ReplaceMcr(562, 1, 277, 1); // lost details
    ReplaceMcr(564, 1, 277, 1); // lost details
    ReplaceMcr(585, 1, 284, 1);
    ReplaceMcr(590, 1, 285, 1); // lost details
    ReplaceMcr(598, 1, 289, 1); // lost details
    // ReplaceMcr(564, 7, 18, 7); // lost details
    // ReplaceMcr(564, 5, 459, 5); // lost details
    // ReplaceMcr(564, 3, 459, 3); // lost details
    ReplaceMcr(34, 7, 18, 7); // lost details
    // ReplaceMcr(37, 7, 18, 7);
    ReplaceMcr(84, 7, 18, 7);   // lost details
    ReplaceMcr(406, 7, 18, 7);  // lost details
    ReplaceMcr(444, 7, 18, 7);  // lost details
    ReplaceMcr(463, 7, 18, 7);  // lost details
    ReplaceMcr(332, 7, 18, 7);  // lost details
    ReplaceMcr(348, 7, 18, 7);  // lost details
    ReplaceMcr(352, 7, 18, 7);  // lost details
    ReplaceMcr(358, 7, 18, 7);  // lost details
    ReplaceMcr(459, 7, 18, 7);  // lost details
    ReplaceMcr(34, 5, 18, 5);   // lost details
    ReplaceMcr(348, 5, 332, 5); // lost details
    ReplaceMcr(352, 5, 332, 5); // lost details
    ReplaceMcr(358, 5, 332, 5); // lost details
    ReplaceMcr(34, 3, 18, 3);   // lost details
    ReplaceMcr(358, 3, 18, 3);  // lost details
    ReplaceMcr(348, 3, 332, 3); // lost details
    ReplaceMcr(352, 3, 332, 3); // lost details
    ReplaceMcr(34, 0, 18, 0);
    ReplaceMcr(352, 0, 18, 0);
    ReplaceMcr(358, 0, 18, 0);
    ReplaceMcr(406, 0, 18, 0); // lost details
    ReplaceMcr(34, 1, 18, 1);
    ReplaceMcr(332, 1, 18, 1);
    ReplaceMcr(348, 1, 352, 1);
    ReplaceMcr(358, 1, 352, 1);
    // ReplaceMcr(209, 7, 6, 7);
    // ReplaceMcr(80, 9, 6, 9);
    // ReplaceMcr(209, 9, 6, 9);
    ReplaceMcr(616, 9, 6, 9);
    // ReplaceMcr(14, 9, 6, 9);  // lost details
    ReplaceMcr(68, 9, 6, 9);  // lost details
    ReplaceMcr(84, 9, 6, 9);  // lost details
    ReplaceMcr(152, 9, 6, 9); // lost details
    // ReplaceMcr(241, 9, 6, 9); // lost details
    ReplaceMcr(265, 9, 6, 9); // lost details
    ReplaceMcr(269, 9, 6, 9); // lost details
    ReplaceMcr(364, 9, 6, 9); // lost details
    ReplaceMcr(551, 9, 6, 9); // lost details
    ReplaceMcr(554, 9, 6, 9); // lost details
    ReplaceMcr(556, 9, 6, 9); // lost details
    ReplaceMcr(608, 9, 6, 9); // lost details
    ReplaceMcr(15, 8, 3, 8);
    // ReplaceMcr(23, 8, 3, 8);
    ReplaceMcr(65, 8, 3, 8);
    // ReplaceMcr(77, 8, 3, 8);
    ReplaceMcr(153, 8, 3, 8);
    // ReplaceMcr(206, 8, 3, 8);
    // ReplaceMcr(238, 8, 3, 8);
    ReplaceMcr(250, 8, 3, 8);
    ReplaceMcr(292, 8, 3, 8);
    ReplaceMcr(299, 8, 3, 8);
    ReplaceMcr(329, 8, 3, 8);
    ReplaceMcr(337, 8, 3, 8);
    ReplaceMcr(353, 8, 3, 8);
    ReplaceMcr(392, 8, 3, 8);
    ReplaceMcr(401, 8, 3, 8);
    ReplaceMcr(448, 8, 3, 8);
    ReplaceMcr(464, 8, 3, 8);
    ReplaceMcr(530, 8, 3, 8);
    ReplaceMcr(532, 8, 3, 8);
    ReplaceMcr(605, 8, 3, 8);
    ReplaceMcr(613, 8, 3, 8);
    // ReplaceMcr(3, 6, 15, 6);
    // ReplaceMcr(23, 6, 15, 6);
    ReplaceMcr(329, 6, 15, 6);
    ReplaceMcr(377, 6, 15, 6);
    ReplaceMcr(441, 6, 15, 6);
    ReplaceMcr(532, 6, 15, 6);
    ReplaceMcr(605, 6, 15, 6);
    // ReplaceMcr(206, 6, 77, 6);
    ReplaceMcr(534, 6, 254, 6);
    ReplaceMcr(537, 6, 254, 6);
    ReplaceMcr(541, 6, 258, 6);
    ReplaceMcr(250, 6, 353, 6);
    ReplaceMcr(322, 6, 353, 6);
    ReplaceMcr(337, 6, 353, 6);
    ReplaceMcr(392, 6, 353, 6);
    ReplaceMcr(429, 6, 353, 6);
    ReplaceMcr(530, 6, 353, 6);
    ReplaceMcr(613, 6, 353, 6);
    // ReplaceMcr(3, 4, 15, 4);
    // ReplaceMcr(23, 4, 15, 4);
    ReplaceMcr(401, 4, 15, 4);
    ReplaceMcr(605, 4, 15, 4);
    ReplaceMcr(322, 4, 337, 4);
    ReplaceMcr(353, 4, 337, 4);
    ReplaceMcr(377, 4, 337, 4);
    // ReplaceMcr(3, 2, 15, 2);
    // ReplaceMcr(23, 2, 15, 2);
    ReplaceMcr(464, 2, 15, 2);
    ReplaceMcr(541, 2, 258, 2);
    // ReplaceMcr(3, 0, 15, 0);
    // ReplaceMcr(23, 0, 15, 0);
    ReplaceMcr(337, 0, 15, 0);
    ReplaceMcr(322, 0, 392, 0);
    ReplaceMcr(353, 0, 392, 0);
    // ReplaceMcr(3, 1, 15, 1);
    // ReplaceMcr(23, 1, 15, 1);
    ReplaceMcr(250, 1, 15, 1);
    ReplaceMcr(258, 1, 15, 1);
    // ReplaceMcr(543, 1, 15, 1);
    ReplaceMcr(322, 1, 15, 1);
    ReplaceMcr(534, 1, 254, 1);
    ReplaceMcr(541, 1, 530, 1);
    ReplaceMcr(49, 5, 5, 5);
    ReplaceMcr(329, 6, 15, 6);
    ReplaceMcr(13, 6, 36, 6);
    ReplaceMcr(13, 4, 36, 4);
    ReplaceMcr(387, 6, 36, 6);
    // ReplaceMcr(390, 2, 19, 2);
    ReplaceMcr(29, 5, 21, 5);
    ReplaceMcr(95, 5, 21, 5);
    // ReplaceMcr(24, 0, 32, 0); // lost details
    // ReplaceMcr(354, 0, 32, 0); // lost details
    ReplaceMcr(398, 0, 2, 0);
    ReplaceMcr(398, 1, 2, 1); // lost details
    // ReplaceMcr(540, 0, 257, 0);
    // ReplaceMcr(30, 0, 2, 0);
    // ReplaceMcr(76, 0, 2, 0);
    // ReplaceMcr(205, 0, 2, 0);
    ReplaceMcr(407, 0, 2, 0); // lost details
    ReplaceMcr(379, 0, 5, 0);
    ReplaceMcr(27, 0, 7, 0);
    // ReplaceMcr(81, 0, 7, 0);
    // ReplaceMcr(210, 0, 7, 0);
    ReplaceMcr(266, 0, 7, 0);
    ReplaceMcr(341, 0, 7, 0);
    ReplaceMcr(349, 0, 7, 0);
    // ReplaceMcr(381, 0, 7, 0);
    ReplaceMcr(460, 0, 7, 0);
    // ReplaceMcr(548, 0, 7, 0); // lost details
    ReplaceMcr(609, 0, 7, 0);
    ReplaceMcr(617, 0, 7, 0); // lost details
    ReplaceMcr(12, 0, 53, 0);
    // ReplaceMcr(54, 0, 53, 0);
    ReplaceMcr(62, 0, 53, 0);
    ReplaceMcr(368, 0, 53, 0); // lost details
    // ReplaceMcr(44, 0, 28, 0);
    // ReplaceMcr(82, 0, 28, 0);
    // ReplaceMcr(106, 0, 28, 0);
    // ReplaceMcr(211, 0, 28, 0);
    // ReplaceMcr(279, 0, 28, 0);
    ReplaceMcr(46, 0, 47, 0);
    // ReplaceMcr(102, 0, 40, 0);
    // ReplaceMcr(273, 0, 40, 0);
    // ReplaceMcr(56, 0, 52, 0);
    // ReplaceMcr(16, 0, 32, 0);
    // ReplaceMcr(38, 0, 32, 0);
    ReplaceMcr(275, 0, 32, 0);
    ReplaceMcr(309, 0, 45, 0);
    ReplaceMcr(567, 0, 45, 0); // lost details
    ReplaceMcr(622, 0, 45, 0);
    // ReplaceMcr(625, 0, 45, 0);
    // ReplaceMcr(630, 0, 45, 0);
    // ReplaceMcr(633, 0, 45, 0);
    // ReplaceMcr(90, 0, 32, 0);
    // ReplaceMcr(96, 0, 32, 0);
    // ReplaceMcr(103, 0, 32, 0);
    // ReplaceMcr(108, 0, 32, 0);
    // ReplaceMcr(110, 0, 32, 0);
    // ReplaceMcr(112, 0, 32, 0);
    // ReplaceMcr(223, 0, 32, 0);
    // ReplaceMcr(373, 0, 58, 0);
    ReplaceMcr(548, 0, 166, 0);
    // ReplaceMcr(105, 0, 43, 0);
    // ReplaceMcr(105, 1, 43, 1);
    // ReplaceMcr(278, 0, 43, 0);
    // ReplaceMcr(278, 1, 43, 1);
    // ReplaceMcr(10, 1, 12, 1);
    // ReplaceMcr(11, 1, 12, 1);
    // ReplaceMcr(53, 1, 12, 1);
    ReplaceMcr(577, 1, 12, 1); // lost details
    ReplaceMcr(31, 1, 4, 1);   // lost details
    // ReplaceMcr(279, 1, 28, 1);
    // ReplaceMcr(35, 1, 28, 1);
    // ReplaceMcr(35, 1, 4, 1); // lost details
    // ReplaceMcr(44, 1, 28, 1);
    // ReplaceMcr(110, 1, 28, 1);
    // ReplaceMcr(114, 1, 28, 1);
    // ReplaceMcr(211, 1, 28, 1);
    // ReplaceMcr(225, 1, 28, 1);
    // ReplaceMcr(273, 1, 28, 1); // lost details
    ReplaceMcr(281, 1, 12, 1); // lost details
    ReplaceMcr(356, 1, 7, 1);  // lost details
    ReplaceMcr(574, 1, 4, 1);  // lost details
    ReplaceMcr(612, 1, 4, 1);  // lost details
    // ReplaceMcr(76, 1, 2, 1);
    // ReplaceMcr(205, 1, 2, 1);
    ReplaceMcr(428, 1, 2, 1);
    ReplaceMcr(41, 1, 4, 1);
    // ReplaceMcr(24, 1, 4, 1); // lost details
    ReplaceMcr(32, 1, 4, 1); // lost details
    // ReplaceMcr(92, 1, 4, 1); // lost details
    // ReplaceMcr(96, 1, 4, 1); // lost details
    // ReplaceMcr(217, 1, 4, 1); // lost details
    ReplaceMcr(275, 1, 4, 1); // lost details
    // ReplaceMcr(78, 1, 4, 1);
    ReplaceMcr(604, 1, 4, 1); // lost details
    // ReplaceMcr(85, 0, 4, 0);
    // ReplaceMcr(120, 0, 4, 0);
    // ReplaceMcr(231, 0, 4, 0);
    ReplaceMcr(145, 0, 4, 0); // lost details
    ReplaceMcr(145, 1, 4, 1); // lost details
    ReplaceMcr(293, 0, 4, 0); // lost details
    // ReplaceMcr(628, 0, 4, 0); // lost details
    ReplaceMcr(536, 1, 297, 1); // lost details
    // ReplaceMcr(372, 1, 57, 1);
    // ReplaceMcr(8, 1, 85, 1);
    // ReplaceMcr(108, 1, 85, 1);
    // ReplaceMcr(116, 1, 85, 1);
    // ReplaceMcr(227, 1, 85, 1);
    // ReplaceMcr(82, 1, 85, 1);
    // ReplaceMcr(87, 1, 85, 1);
    // ReplaceMcr(94, 1, 85, 1);
    // ReplaceMcr(112, 1, 85, 1);
    // ReplaceMcr(118, 1, 85, 1);
    // ReplaceMcr(120, 1, 85, 1);
    // ReplaceMcr(233, 1, 85, 1);
    // ReplaceMcr(16, 1, 85, 1);
    // ReplaceMcr(50, 1, 85, 1);
    // ReplaceMcr(103, 1, 24, 1);
    // ReplaceMcr(275, 1, 24, 1);
    ReplaceMcr(304, 1, 48, 1);
    ReplaceMcr(27, 1, 7, 1);
    // ReplaceMcr(81, 1, 7, 1);
    // ReplaceMcr(210, 1, 7, 1);
    // ReplaceMcr(202, 1, 46, 1);
    ReplaceMcr(47, 1, 46, 1);
    // ReplaceMcr(468, 1, 31, 1);
    // ReplaceMcr(472, 1, 43, 1);
    ReplaceMcr(360, 1, 46, 1);
    // ReplaceMcr(52, 1, 46, 1); // lost details
    // ReplaceMcr(56, 1, 46, 1); // lost details
    // ReplaceMcr(373, 1, 46, 1); // lost details
    ReplaceMcr(592, 1, 46, 1);
    ReplaceMcr(505, 1, 46, 1); // lost details
    // ReplaceMcr(202, 0, 47, 0);
    // ReplaceMcr(195, 0, 48, 0);
    // ReplaceMcr(203, 0, 48, 0);
    // ReplaceMcr(194, 1, 46, 1);
    // ReplaceMcr(198, 1, 46, 1);
    // ReplaceMcr(199, 0, 48, 0);
    ReplaceMcr(572, 0, 48, 0);
    ReplaceMcr(507, 1, 48, 1);
    // ReplaceMcr(206, 6, 77, 6);
    ReplaceMcr(471, 7, 265, 7);
    ReplaceMcr(547, 7, 261, 7);
    ReplaceMcr(471, 9, 6, 9);
    ReplaceMcr(569, 0, 283, 0);
    ReplaceMcr(565, 0, 283, 0);
    // ReplaceMcr(621, 1, 48, 1);
    ReplaceMcr(647, 1, 48, 1);
    // ReplaceMcr(627, 0, 624, 0);
    // ReplaceMcr(632, 0, 627, 0);
    ReplaceMcr(628, 0, 2, 0);
    ReplaceMcr(629, 1, 639, 1);
    // ReplaceMcr(637, 0, 624, 0);
    // ReplaceMcr(643, 0, 631, 0);
    ReplaceMcr(75, 8, 95, 8);
    ReplaceMcr(91, 8, 95, 8);
    // ReplaceMcr(172, 8, 95, 8);
    ReplaceMcr(204, 8, 95, 8);
    ReplaceMcr(215, 8, 95, 8);
    ReplaceMcr(272, 8, 95, 8);
    ReplaceMcr(557, 8, 95, 8);
    ReplaceMcr(559, 8, 95, 8);
    ReplaceMcr(345, 8, 31, 8);
    ReplaceMcr(456, 8, 31, 8);
    // ReplaceMcr(388, 2, 15, 2);
    ReplaceMcr(401, 4, 15, 4);
    // ReplaceMcr(479, 5, 14, 5);
    ReplaceMcr(389, 6, 17, 6); // lost details
    // ReplaceMcr(19, 8, 31, 8);  // lost details
    // ReplaceMcr(390, 8, 31, 8);  // lost details
    ReplaceMcr(89, 8, 31, 8);
    ReplaceMcr(254, 8, 31, 8); // lost details
    ReplaceMcr(534, 8, 31, 8); // lost details
    ReplaceMcr(537, 8, 31, 8); // lost details
    ReplaceMcr(333, 8, 31, 8); // lost details
    ReplaceMcr(345, 8, 31, 8);
    ReplaceMcr(365, 8, 31, 8); // lost details
    ReplaceMcr(456, 8, 31, 8);
    ReplaceMcr(274, 8, 31, 8); // lost details
    // ReplaceMcr(558, 8, 31, 8); // lost details
    // ReplaceMcr(560, 8, 31, 8); // lost details
    ReplaceMcr(258, 8, 296, 8); // lost details
    ReplaceMcr(541, 8, 296, 8); // lost details
    // ReplaceMcr(543, 8, 296, 8); // lost details
    ReplaceMcr(89, 6, 31, 6);  // lost details
    ReplaceMcr(274, 6, 31, 6); // lost details
    // ReplaceMcr(558, 6, 31, 6); // lost details
    // ReplaceMcr(560, 6, 31, 6); // lost details
    ReplaceMcr(356, 6, 31, 6);  // lost details
    ReplaceMcr(333, 6, 445, 6); // lost details
    ReplaceMcr(345, 6, 445, 6); // lost details
    ReplaceMcr(365, 6, 445, 6); // lost details
    ReplaceMcr(274, 4, 31, 4);  // lost details
    // ReplaceMcr(560, 4, 31, 4); // lost details
    ReplaceMcr(333, 4, 345, 4); // lost details
    ReplaceMcr(365, 4, 345, 4); // lost details
    ReplaceMcr(445, 4, 345, 4); // lost details
    ReplaceMcr(299, 2, 274, 2); // lost details
    // ReplaceMcr(560, 2, 274, 2); // lost details
    ReplaceMcr(333, 2, 345, 2); // lost details
    ReplaceMcr(365, 2, 345, 2); // lost details
    ReplaceMcr(415, 2, 345, 2); // lost details
    ReplaceMcr(445, 2, 345, 2); // lost details
    ReplaceMcr(333, 0, 31, 0);  // lost details
    ReplaceMcr(345, 0, 31, 0);  // lost details
    ReplaceMcr(365, 0, 31, 0);  // lost details
    ReplaceMcr(445, 0, 31, 0);  // lost details
    ReplaceMcr(333, 1, 31, 1);  // lost details
    ReplaceMcr(365, 1, 31, 1);

    ReplaceMcr(125, 0, 136, 0); // lost details
    ReplaceMcr(125, 1, 136, 1); // lost details
    ReplaceMcr(125, 2, 136, 2); // lost details
    // ReplaceMcr(125, 3, 136, 3); // lost details
    ReplaceMcr(125, 3, 129, 3); // lost details
    ReplaceMcr(508, 2, 136, 2); // lost details
    ReplaceMcr(508, 3, 129, 3); // lost details
    ReplaceMcr(146, 0, 142, 0); // lost details
    ReplaceMcr(146, 1, 15, 1);  // lost details
    ReplaceMcr(136, 3, 129, 3); // lost details TODO: add missing pixels?
    ReplaceMcr(140, 1, 136, 1); // lost details

    ReplaceMcr(63, 8, 95, 8); // lost details
    ReplaceMcr(70, 8, 95, 8); // lost details
    ReplaceMcr(71, 8, 95, 8); // lost details
    ReplaceMcr(73, 8, 95, 8); // lost details
    ReplaceMcr(74, 8, 95, 8); // lost details

    ReplaceMcr(1, 8, 95, 8);   // lost details
    ReplaceMcr(21, 8, 95, 8);  // lost details
    ReplaceMcr(36, 8, 95, 8);  // lost details
    ReplaceMcr(75, 8, 95, 8);  // lost details
    ReplaceMcr(83, 8, 95, 8);  // lost details
    ReplaceMcr(91, 8, 95, 8);  // lost details
    ReplaceMcr(99, 8, 95, 8);  // lost details
    ReplaceMcr(113, 8, 95, 8); // lost details
    ReplaceMcr(115, 8, 95, 8); // lost details
    ReplaceMcr(119, 8, 95, 8); // lost details
    ReplaceMcr(149, 8, 95, 8); // lost details
    ReplaceMcr(151, 8, 95, 8); // lost details
    ReplaceMcr(204, 8, 95, 8); // lost details
    ReplaceMcr(215, 8, 95, 8); // lost details
    ReplaceMcr(220, 8, 95, 8); // lost details
    ReplaceMcr(224, 8, 95, 8); // lost details
    ReplaceMcr(226, 8, 95, 8); // lost details
    ReplaceMcr(230, 8, 95, 8); // lost details
    ReplaceMcr(248, 8, 95, 8); // lost details
    ReplaceMcr(252, 8, 95, 8); // lost details
    ReplaceMcr(256, 8, 95, 8); // lost details
    ReplaceMcr(294, 8, 95, 8); // lost details
    ReplaceMcr(300, 8, 95, 8); // lost details
    ReplaceMcr(321, 8, 95, 8); // lost details
    ReplaceMcr(328, 8, 95, 8); // lost details
    ReplaceMcr(335, 8, 95, 8); // lost details
    ReplaceMcr(351, 8, 95, 8); // lost details
    ReplaceMcr(375, 8, 95, 8); // lost details
    ReplaceMcr(387, 8, 95, 8); // lost details
    ReplaceMcr(391, 8, 95, 8); // lost details
    ReplaceMcr(400, 8, 95, 8); // lost details
    ReplaceMcr(427, 8, 95, 8); // lost details
    ReplaceMcr(439, 8, 95, 8); // lost details
    ReplaceMcr(446, 8, 95, 8); // lost details
    ReplaceMcr(462, 8, 95, 8); // lost details
    ReplaceMcr(529, 8, 95, 8); // lost details
    ReplaceMcr(531, 8, 95, 8); // lost details
    ReplaceMcr(533, 8, 95, 8); // lost details
    ReplaceMcr(535, 8, 95, 8); // lost details
    ReplaceMcr(539, 8, 95, 8); // lost details

    ReplaceMcr(542, 8, 95, 8); // lost details
    ReplaceMcr(603, 8, 95, 8); // lost details
    ReplaceMcr(611, 8, 95, 8); // lost details

    ReplaceMcr(1, 6, 119, 6);   // lost details
    ReplaceMcr(13, 6, 119, 6);  // lost details
    ReplaceMcr(21, 6, 119, 6);  // lost details
    ReplaceMcr(36, 6, 119, 6);  // lost details
    ReplaceMcr(83, 6, 119, 6);  // lost details
    ReplaceMcr(149, 6, 119, 6); // lost details
    ReplaceMcr(387, 6, 119, 6); // lost details
    ReplaceMcr(400, 6, 119, 6); // lost details
    ReplaceMcr(439, 6, 119, 6); // lost details
    ReplaceMcr(462, 6, 119, 6); // lost details
    ReplaceMcr(603, 6, 119, 6); // lost details
    ReplaceMcr(611, 6, 119, 6); // lost details
    ReplaceMcr(75, 6, 99, 6);   // lost details
    ReplaceMcr(91, 6, 99, 6);   // lost details
    ReplaceMcr(115, 6, 99, 6);  // lost details
    ReplaceMcr(204, 6, 99, 6);  // lost details
    ReplaceMcr(215, 6, 99, 6);  // lost details

    ReplaceMcr(71, 6, 63, 6);
    ReplaceMcr(71, 7, 67, 7);
    ReplaceMcr(69, 6, 67, 6); // lost details
    ReplaceMcr(69, 4, 63, 2); // lost details
    ReplaceMcr(65, 4, 63, 2); // lost details
    ReplaceMcr(64, 7, 63, 7); // lost details
    ReplaceMcr(64, 5, 63, 3); // lost details
    ReplaceMcr(68, 5, 63, 3); // lost details
    ReplaceMcr(63, 5, 63, 4); // lost details
    ReplaceMcr(67, 5, 67, 4); // lost details
    ReplaceMcr(70, 5, 70, 4); // lost details
    ReplaceMcr(71, 5, 71, 4); // lost details
    ReplaceMcr(72, 5, 72, 4); // lost details
    ReplaceMcr(73, 5, 73, 4); // lost details
    ReplaceMcr(74, 5, 74, 4); // lost details

    ReplaceMcr(529, 6, 248, 6); // lost details
    ReplaceMcr(531, 6, 248, 6); // lost details
    ReplaceMcr(533, 6, 252, 6); // lost details

    ReplaceMcr(542, 6, 256, 6); // lost details

    ReplaceMcr(300, 6, 294, 6); // lost details
    ReplaceMcr(321, 6, 328, 6); // lost details
    ReplaceMcr(335, 6, 328, 6); // lost details
    ReplaceMcr(351, 6, 328, 6); // lost details
    ReplaceMcr(375, 6, 328, 6); // lost details
    ReplaceMcr(391, 6, 328, 6); // lost details

    ReplaceMcr(13, 4, 1, 4);    // lost details
    ReplaceMcr(21, 4, 1, 4);    // lost details
    ReplaceMcr(36, 4, 1, 4);    // lost details
    ReplaceMcr(328, 4, 1, 4);   // lost details
    ReplaceMcr(375, 4, 1, 4);   // lost details
    ReplaceMcr(531, 4, 248, 4); // lost details
    ReplaceMcr(533, 4, 252, 4); // lost details

    ReplaceMcr(1, 2, 256, 2);   // lost details
    ReplaceMcr(13, 2, 256, 2);  // lost details
    ReplaceMcr(21, 2, 256, 2);  // lost details
    ReplaceMcr(36, 2, 256, 2);  // lost details
    ReplaceMcr(248, 2, 256, 2); // lost details
    ReplaceMcr(83, 2, 256, 2);  // lost details
    ReplaceMcr(119, 2, 256, 2); // lost details
    ReplaceMcr(230, 2, 256, 2); // lost details

    ReplaceMcr(13, 0, 1, 0);  // lost details
    ReplaceMcr(21, 0, 1, 0);  // lost details
    ReplaceMcr(36, 0, 1, 0);  // lost details
    ReplaceMcr(248, 0, 1, 0); // lost details
    ReplaceMcr(256, 0, 1, 0); // lost details
    ReplaceMcr(328, 0, 1, 0); // lost details

    ReplaceMcr(13, 8, 95, 8);  // lost details
    ReplaceMcr(17, 8, 95, 8);  // lost details
    ReplaceMcr(25, 8, 95, 8);  // lost details
    ReplaceMcr(29, 8, 95, 8);  // lost details
    ReplaceMcr(33, 8, 95, 8);  // lost details
    ReplaceMcr(39, 8, 95, 8);  // lost details
    ReplaceMcr(49, 8, 95, 8);  // lost details
    ReplaceMcr(51, 8, 95, 8);  // lost details
    ReplaceMcr(88, 8, 95, 8);  // lost details
    ReplaceMcr(93, 8, 95, 8);  // lost details
    ReplaceMcr(97, 8, 95, 8);  // lost details
    ReplaceMcr(101, 8, 95, 8); // lost details
    ReplaceMcr(107, 8, 95, 8); // lost details
    ReplaceMcr(109, 8, 95, 8); // lost details
    ReplaceMcr(111, 8, 95, 8); // lost details
    ReplaceMcr(117, 8, 95, 8); // lost details
    ReplaceMcr(121, 8, 95, 8); // lost details
    ReplaceMcr(218, 8, 95, 8); // lost details
    ReplaceMcr(222, 8, 95, 8); // lost details
    ReplaceMcr(228, 8, 95, 8); // lost details
    ReplaceMcr(272, 8, 95, 8); // lost details
    ReplaceMcr(331, 8, 95, 8); // lost details
    ReplaceMcr(339, 8, 95, 8); // lost details
    ReplaceMcr(343, 8, 95, 8); // lost details
    ReplaceMcr(347, 8, 95, 8); // lost details
    ReplaceMcr(355, 8, 95, 8); // lost details
    ReplaceMcr(363, 8, 95, 8); // lost details
    ReplaceMcr(366, 8, 95, 8); // lost details
    ReplaceMcr(389, 8, 95, 8); // lost details
    ReplaceMcr(393, 8, 95, 8); // lost details
    ReplaceMcr(397, 8, 95, 8); // lost details
    ReplaceMcr(399, 8, 95, 8); // lost details
    // ReplaceMcr(402, 8, 95, 8); // lost details
    ReplaceMcr(413, 8, 95, 8); // lost details
    ReplaceMcr(417, 8, 95, 8); // lost details
    ReplaceMcr(443, 8, 95, 8); // lost details
    ReplaceMcr(450, 8, 95, 8); // lost details
    ReplaceMcr(454, 8, 95, 8); // lost details
    ReplaceMcr(458, 8, 95, 8); // lost details
    ReplaceMcr(466, 8, 95, 8); // lost details
    ReplaceMcr(478, 8, 95, 8); // lost details
    ReplaceMcr(557, 8, 95, 8); // lost details
    ReplaceMcr(559, 8, 95, 8); // lost details
    ReplaceMcr(615, 8, 95, 8); // lost details
    ReplaceMcr(421, 8, 55, 8); // lost details
    ReplaceMcr(154, 8, 55, 8); // lost details
    ReplaceMcr(154, 9, 13, 9); // lost details

    ReplaceMcr(9, 6, 25, 6);   // lost details
    ReplaceMcr(33, 6, 25, 6);  // lost details
    ReplaceMcr(51, 6, 25, 6);  // lost details
    ReplaceMcr(93, 6, 25, 6);  // lost details
    ReplaceMcr(97, 6, 25, 6);  // lost details
    ReplaceMcr(218, 6, 25, 6); // lost details
    ReplaceMcr(327, 6, 25, 6); // lost details
    ReplaceMcr(339, 6, 25, 6); // lost details
    ReplaceMcr(366, 6, 25, 6); // lost details
    ReplaceMcr(383, 6, 25, 6); // lost details
    ReplaceMcr(435, 6, 25, 6); // lost details
    ReplaceMcr(458, 6, 25, 6); // lost details
    ReplaceMcr(615, 6, 25, 6); // lost details

    ReplaceMcr(17, 6, 95, 6);  // lost details
    ReplaceMcr(29, 6, 95, 6);  // lost details
    ReplaceMcr(39, 6, 95, 6);  // lost details
    ReplaceMcr(49, 6, 95, 6);  // lost details
    ReplaceMcr(88, 6, 95, 6);  // lost details
    ReplaceMcr(107, 6, 95, 6); // lost details
    ReplaceMcr(109, 6, 95, 6); // lost details
    ReplaceMcr(111, 6, 95, 6); // lost details
    ReplaceMcr(117, 6, 95, 6); // lost details
    ReplaceMcr(121, 6, 95, 6); // lost details
    ReplaceMcr(222, 6, 95, 6); // lost details
    ReplaceMcr(228, 6, 95, 6); // lost details
    ReplaceMcr(272, 6, 95, 6); // lost details
    ReplaceMcr(389, 6, 95, 6); // lost details
    ReplaceMcr(397, 6, 95, 6); // lost details
    // ReplaceMcr(402, 6, 95, 6); // lost details
    ReplaceMcr(443, 6, 95, 6);  // lost details
    ReplaceMcr(466, 6, 95, 6);  // lost details
    ReplaceMcr(478, 6, 95, 6);  // lost details
    ReplaceMcr(347, 6, 393, 6); // lost details
    ReplaceMcr(399, 6, 393, 6); // lost details
    ReplaceMcr(417, 6, 393, 6); // lost details
    ReplaceMcr(331, 6, 343, 6); // lost details
    ReplaceMcr(355, 6, 343, 6); // lost details
    ReplaceMcr(363, 6, 343, 6); // lost details
    ReplaceMcr(557, 6, 343, 6); // lost details
    ReplaceMcr(559, 6, 343, 6); // lost details

    ReplaceMcr(17, 4, 29, 4);   // lost details
    ReplaceMcr(49, 4, 29, 4);   // lost details
    ReplaceMcr(389, 4, 29, 4);  // lost details
    ReplaceMcr(478, 4, 29, 4);  // lost details
    ReplaceMcr(9, 4, 25, 4);    // lost details
    ReplaceMcr(51, 4, 25, 4);   // lost details
    ReplaceMcr(55, 4, 25, 4);   // lost details
    ReplaceMcr(59, 4, 25, 4);   // lost details
    ReplaceMcr(366, 4, 25, 4);  // lost details
    ReplaceMcr(370, 4, 25, 4);  // lost details
    ReplaceMcr(374, 4, 25, 4);  // lost details
    ReplaceMcr(383, 4, 25, 4);  // lost details
    ReplaceMcr(423, 4, 25, 4);  // lost details
    ReplaceMcr(331, 4, 343, 4); // lost details
    ReplaceMcr(355, 4, 343, 4); // lost details
    ReplaceMcr(363, 4, 343, 4); // lost details
    ReplaceMcr(443, 4, 343, 4); // lost details
    ReplaceMcr(339, 4, 347, 4); // lost details

    ReplaceMcr(393, 4, 347, 4); // lost details
    ReplaceMcr(417, 4, 347, 4); // lost details
    ReplaceMcr(435, 4, 347, 4); // lost details
    ReplaceMcr(450, 4, 347, 4); // lost details
    ReplaceMcr(615, 4, 347, 4); // lost details
    ReplaceMcr(458, 4, 154, 4); // lost details

    ReplaceMcr(9, 2, 29, 2);    // lost details
    ReplaceMcr(17, 2, 29, 2);   // lost details
    ReplaceMcr(25, 2, 29, 2);   // lost details
    ReplaceMcr(33, 2, 29, 2);   // lost details
    ReplaceMcr(49, 2, 29, 2);   // lost details
    ReplaceMcr(51, 2, 29, 2);   // lost details
    ReplaceMcr(55, 2, 29, 2);   // lost details
    ReplaceMcr(59, 2, 29, 2);   // lost details
    ReplaceMcr(93, 2, 29, 2);   // lost details
    ReplaceMcr(97, 2, 29, 2);   // lost details
    ReplaceMcr(218, 2, 29, 2);  // lost details
    ReplaceMcr(343, 2, 29, 2);  // lost details
    ReplaceMcr(363, 2, 29, 2);  // lost details
    ReplaceMcr(366, 2, 29, 2);  // lost details
    ReplaceMcr(413, 2, 29, 2);  // lost details
    ReplaceMcr(478, 2, 29, 2);  // lost details
    ReplaceMcr(339, 2, 347, 2); // lost details
    ReplaceMcr(355, 2, 347, 2); // lost details
    ReplaceMcr(393, 2, 347, 2); // lost details
    ReplaceMcr(443, 2, 347, 2); // lost details

    ReplaceMcr(9, 0, 33, 0);  // lost details
    ReplaceMcr(17, 0, 33, 0); // lost details
    ReplaceMcr(29, 0, 33, 0); // lost details
    ReplaceMcr(39, 0, 33, 0); // lost details
    ReplaceMcr(49, 0, 33, 0); // lost details
    ReplaceMcr(51, 0, 33, 0); // lost details
    ReplaceMcr(59, 0, 33, 0); // lost details
    ReplaceMcr(93, 0, 33, 0);
    ReplaceMcr(117, 0, 33, 0); // lost details
    ReplaceMcr(121, 0, 33, 0); // lost details
    ReplaceMcr(218, 0, 33, 0);
    ReplaceMcr(228, 0, 33, 0);  // lost details
    ReplaceMcr(397, 0, 33, 0);  // lost details
    ReplaceMcr(466, 0, 33, 0);  // lost details
    ReplaceMcr(478, 0, 33, 0);  // lost details
    ReplaceMcr(55, 0, 33, 0);   // lost details
    ReplaceMcr(331, 0, 33, 0);  // lost details
    ReplaceMcr(339, 0, 33, 0);  // lost details
    ReplaceMcr(355, 0, 33, 0);  // lost details
    ReplaceMcr(363, 0, 33, 0);  // lost details
    ReplaceMcr(370, 0, 33, 0);  // lost details
    ReplaceMcr(443, 0, 33, 0);  // lost details
    ReplaceMcr(559, 0, 272, 0); // lost details
    ReplaceMcr(5, 9, 13, 9);    // lost details
    ReplaceMcr(25, 9, 13, 9);   // lost details
    ReplaceMcr(79, 9, 13, 9);
    ReplaceMcr(93, 9, 13, 9);
    ReplaceMcr(151, 9, 13, 9);
    ReplaceMcr(208, 9, 13, 9);
    ReplaceMcr(218, 9, 13, 9);
    ReplaceMcr(260, 9, 13, 9); // lost details
    ReplaceMcr(264, 9, 13, 9); // lost details
    ReplaceMcr(268, 9, 13, 9); // lost details
    ReplaceMcr(323, 9, 13, 9); // lost details
    ReplaceMcr(339, 9, 13, 9); // lost details
    ReplaceMcr(379, 9, 13, 9); // lost details
    ReplaceMcr(393, 9, 13, 9); // lost details
    ReplaceMcr(413, 9, 13, 9); // lost details
    ReplaceMcr(431, 9, 13, 9); // lost details
    ReplaceMcr(439, 9, 13, 9); // lost details
    ReplaceMcr(450, 9, 13, 9); // lost details
    ReplaceMcr(544, 9, 13, 9); // lost details
    ReplaceMcr(546, 9, 13, 9); // lost details
    ReplaceMcr(550, 9, 13, 9); // lost details
    ReplaceMcr(552, 9, 13, 9); // lost details
    ReplaceMcr(553, 9, 13, 9); // lost details
    ReplaceMcr(555, 9, 13, 9); // lost details
    ReplaceMcr(607, 9, 13, 9); // lost details
    ReplaceMcr(615, 9, 13, 9);
    ReplaceMcr(162, 9, 158, 9);
    ReplaceMcr(5, 7, 13, 7);
    ReplaceMcr(25, 7, 13, 7); // 25 would be better?
    ReplaceMcr(49, 7, 13, 7);
    ReplaceMcr(79, 7, 13, 7);
    ReplaceMcr(93, 7, 13, 7);
    ReplaceMcr(107, 7, 13, 7);
    ReplaceMcr(111, 7, 13, 7);
    ReplaceMcr(115, 7, 13, 7);
    ReplaceMcr(117, 7, 13, 7);
    ReplaceMcr(119, 7, 13, 7);
    ReplaceMcr(208, 7, 13, 7);
    ReplaceMcr(218, 7, 13, 7);
    ReplaceMcr(222, 7, 13, 7);
    ReplaceMcr(226, 7, 13, 7);
    ReplaceMcr(228, 7, 13, 7);
    ReplaceMcr(230, 7, 13, 7);
    ReplaceMcr(363, 7, 13, 7);
    ReplaceMcr(393, 7, 13, 7);
    ReplaceMcr(413, 7, 13, 7);
    ReplaceMcr(431, 7, 13, 7);
    ReplaceMcr(450, 7, 13, 7);
    ReplaceMcr(478, 7, 13, 7);
    ReplaceMcr(607, 7, 13, 7);
    ReplaceMcr(615, 7, 13, 7);
    ReplaceMcr(546, 7, 260, 7);
    ReplaceMcr(553, 7, 268, 7);
    ReplaceMcr(328, 7, 323, 7);
    ReplaceMcr(339, 7, 323, 7);
    ReplaceMcr(379, 7, 323, 7);
    ReplaceMcr(439, 7, 323, 7);
    ReplaceMcr(5, 5, 25, 5);
    ReplaceMcr(13, 5, 25, 5);
    ReplaceMcr(49, 5, 25, 5);
    ReplaceMcr(107, 5, 25, 5);
    ReplaceMcr(115, 5, 25, 5); // 18
    ReplaceMcr(226, 5, 25, 5);
    ReplaceMcr(260, 5, 268, 5);
    ReplaceMcr(546, 5, 268, 5);
    ReplaceMcr(323, 5, 328, 5);
    ReplaceMcr(339, 5, 328, 5);
    ReplaceMcr(363, 5, 328, 5);
    ReplaceMcr(379, 5, 328, 5);
    ReplaceMcr(5, 3, 25, 3);
    ReplaceMcr(13, 3, 25, 3);
    ReplaceMcr(49, 3, 25, 3);
    ReplaceMcr(107, 3, 25, 3);
    ReplaceMcr(115, 3, 25, 3);
    ReplaceMcr(226, 3, 25, 3);
    ReplaceMcr(260, 3, 25, 3);
    ReplaceMcr(268, 3, 25, 3);
    ReplaceMcr(328, 3, 323, 3);
    ReplaceMcr(339, 3, 323, 3);
    ReplaceMcr(546, 3, 323, 3);
    ReplaceMcr(13, 1, 5, 1);
    ReplaceMcr(25, 1, 5, 1);
    ReplaceMcr(49, 1, 5, 1);
    ReplaceMcr(260, 1, 5, 1);
    ReplaceMcr(268, 1, 5, 1);
    ReplaceMcr(544, 1, 5, 1);
    ReplaceMcr(323, 1, 328, 1);
    ReplaceMcr(17, 9, 13, 9); // lost details
    ReplaceMcr(21, 9, 13, 9); // lost details
    ReplaceMcr(29, 9, 13, 9);
    ReplaceMcr(33, 9, 13, 9); // lost details
    ReplaceMcr(36, 9, 13, 9); // lost details
    ReplaceMcr(42, 9, 13, 9); // lost details
    ReplaceMcr(51, 9, 13, 9); // lost details
    ReplaceMcr(55, 9, 13, 9); // lost details
    ReplaceMcr(59, 9, 13, 9); // lost details
    ReplaceMcr(83, 9, 13, 9); // lost details
    ReplaceMcr(88, 9, 13, 9); // lost details
    ReplaceMcr(91, 9, 13, 9); // lost details
    ReplaceMcr(95, 9, 13, 9);
    ReplaceMcr(97, 9, 13, 9);  // lost details
    ReplaceMcr(99, 9, 13, 9);  // lost details
    ReplaceMcr(104, 9, 13, 9); // lost details
    ReplaceMcr(109, 9, 13, 9); // lost details
    ReplaceMcr(113, 9, 13, 9); // lost details
    ReplaceMcr(121, 9, 13, 9); // lost details
    ReplaceMcr(215, 9, 13, 9); // lost details
    ReplaceMcr(220, 9, 13, 9); // lost details
    ReplaceMcr(224, 9, 13, 9); // lost details
    ReplaceMcr(276, 9, 13, 9); // lost details
    ReplaceMcr(331, 9, 13, 9); // lost details
    ReplaceMcr(335, 9, 13, 9); // lost details
    ReplaceMcr(343, 9, 13, 9);
    ReplaceMcr(347, 9, 13, 9); // lost details
    ReplaceMcr(351, 9, 13, 9); // lost details
    ReplaceMcr(357, 9, 13, 9); // lost details
    ReplaceMcr(366, 9, 13, 9); // lost details
    ReplaceMcr(370, 9, 13, 9); // lost details
    ReplaceMcr(374, 9, 13, 9); // lost details
    ReplaceMcr(389, 9, 13, 9); // lost details
    ReplaceMcr(391, 9, 13, 9); // lost details
    ReplaceMcr(397, 9, 13, 9);
    ReplaceMcr(399, 9, 13, 9); // lost details
    ReplaceMcr(400, 9, 13, 9); // lost details
    ReplaceMcr(417, 9, 13, 9); // lost details
    ReplaceMcr(421, 9, 13, 9); // lost details
    ReplaceMcr(423, 9, 13, 9); // lost details
    ReplaceMcr(443, 9, 13, 9); // lost details
    ReplaceMcr(446, 9, 13, 9); // lost details
    ReplaceMcr(454, 9, 13, 9);
    ReplaceMcr(458, 9, 13, 9);  // lost details
    ReplaceMcr(462, 9, 13, 9);  // lost details
    ReplaceMcr(470, 9, 13, 9);  // lost details
    ReplaceMcr(484, 9, 13, 9);  // lost details
    ReplaceMcr(488, 9, 13, 9);  // lost details
    ReplaceMcr(561, 9, 13, 9);  // lost details
    ReplaceMcr(563, 9, 13, 9);  // lost details
    ReplaceMcr(611, 9, 13, 9);  // lost details
    ReplaceMcr(33, 7, 17, 7);   // lost details
    ReplaceMcr(36, 7, 17, 7);   // lost details
    ReplaceMcr(42, 7, 17, 7);   // lost details
    ReplaceMcr(83, 7, 17, 7);   // lost details
    ReplaceMcr(88, 7, 17, 7);   // lost details
    ReplaceMcr(97, 7, 17, 7);   // lost details
    ReplaceMcr(99, 7, 17, 7);   // lost details
    ReplaceMcr(104, 7, 17, 7);  // lost details
    ReplaceMcr(109, 7, 17, 7);  // lost details
    ReplaceMcr(113, 7, 17, 7);  // lost details
    ReplaceMcr(121, 7, 17, 7);  // lost details
    ReplaceMcr(220, 7, 17, 7);  // lost details
    ReplaceMcr(224, 7, 17, 7);  // lost details
    ReplaceMcr(331, 7, 17, 7);  // lost details
    ReplaceMcr(351, 7, 17, 7);  // lost details
    ReplaceMcr(357, 7, 17, 7);  // lost details
    ReplaceMcr(399, 7, 17, 7);  // lost details
    ReplaceMcr(400, 7, 17, 7);  // lost details
    ReplaceMcr(443, 7, 17, 7);  // lost details
    ReplaceMcr(462, 7, 17, 7);  // lost details
    ReplaceMcr(9, 7, 21, 7);    // lost details
    ReplaceMcr(29, 7, 21, 7);   // lost details
    ReplaceMcr(51, 7, 21, 7);   // lost details
    ReplaceMcr(95, 7, 21, 7);   // lost details
    ReplaceMcr(335, 7, 21, 7);  // lost details
    ReplaceMcr(366, 7, 21, 7);  // lost details
    ReplaceMcr(383, 7, 21, 7);  // lost details
    ReplaceMcr(391, 7, 21, 7);  // lost details
    ReplaceMcr(397, 7, 21, 7);  // lost details
    ReplaceMcr(611, 7, 21, 7);  // lost details
    ReplaceMcr(417, 7, 55, 7);  // lost details
    ReplaceMcr(421, 7, 55, 7);  // lost details
    ReplaceMcr(446, 7, 55, 7);  // lost details
    ReplaceMcr(454, 7, 488, 7); // lost details
    ReplaceMcr(484, 7, 488, 7); // lost details
    ReplaceMcr(470, 7, 264, 7); // TODO: 470 would be better?
    ReplaceMcr(458, 7, 276, 7); // lost details
    ReplaceMcr(561, 7, 276, 7); // lost details
    ReplaceMcr(563, 7, 276, 7); // lost details
    ReplaceMcr(17, 5, 33, 5);   // lost details
    ReplaceMcr(36, 5, 33, 5);   // lost details
    ReplaceMcr(331, 5, 33, 5);  // lost details
    ReplaceMcr(351, 5, 33, 5);  // lost details
    ReplaceMcr(389, 5, 33, 5);  // lost details
    ReplaceMcr(462, 5, 33, 5);  // lost details
    ReplaceMcr(9, 5, 21, 5);    // lost details
    ReplaceMcr(29, 5, 21, 5);   // lost details
    ReplaceMcr(51, 5, 21, 5);   // lost details
    ReplaceMcr(55, 5, 21, 5);   // lost details
    ReplaceMcr(59, 5, 21, 5);   // lost details
    ReplaceMcr(95, 5, 21, 5);   // lost details
    ReplaceMcr(335, 5, 21, 5);  // lost details
    ReplaceMcr(366, 5, 21, 5);  // lost details
    ReplaceMcr(383, 5, 21, 5);  // lost details
    ReplaceMcr(391, 5, 21, 5);  // lost details
    ReplaceMcr(421, 5, 21, 5);  // lost details
    ReplaceMcr(423, 5, 21, 5);  // lost details
    ReplaceMcr(611, 5, 21, 5);  // lost details
    ReplaceMcr(561, 5, 276, 5); // lost details
    ReplaceMcr(17, 3, 33, 3);   // lost details
    ReplaceMcr(36, 3, 33, 3);   // lost details
    ReplaceMcr(462, 3, 33, 3);  // lost details
    ReplaceMcr(9, 3, 21, 3);    // lost details
    ReplaceMcr(51, 3, 21, 3);   // lost details
    ReplaceMcr(55, 3, 21, 3);   // lost details
    ReplaceMcr(59, 3, 21, 3);   // lost details
    ReplaceMcr(335, 3, 21, 3);  // lost details
    ReplaceMcr(366, 3, 21, 3);  // lost details
    ReplaceMcr(391, 3, 21, 3);  // lost details
    ReplaceMcr(421, 3, 21, 3);  // lost details
    ReplaceMcr(423, 3, 21, 3);  // lost details
    ReplaceMcr(470, 3, 276, 3); // lost details
    ReplaceMcr(331, 3, 347, 3); // lost details
    ReplaceMcr(351, 3, 347, 3); // lost details
    ReplaceMcr(488, 3, 484, 3); // lost details
    ReplaceMcr(9, 1, 55, 1);    // lost details
    ReplaceMcr(29, 1, 55, 1);   // lost details
    ReplaceMcr(51, 1, 55, 1);   // lost details
    ReplaceMcr(59, 1, 55, 1);   // lost details
    ReplaceMcr(91, 1, 55, 1);   // lost details
    ReplaceMcr(95, 1, 55, 1);   // lost details
    ReplaceMcr(215, 1, 55, 1);  // lost details
    ReplaceMcr(335, 1, 55, 1);  // lost details
    ReplaceMcr(391, 1, 55, 1);  // lost details
    ReplaceMcr(331, 1, 357, 1); // lost details
    ReplaceMcr(347, 1, 357, 1); // lost details
    ReplaceMcr(351, 1, 357, 1); // lost details
    ReplaceMcr(17, 1, 33, 1);   // lost details
    ReplaceMcr(36, 1, 33, 1);   // lost details
    ReplaceMcr(470, 1, 276, 1); // lost details
    ReplaceMcr(561, 1, 276, 1); // lost details
    ReplaceMcr(151, 2, 151, 5); // added details
    // eliminate micros of unused subtiles
    // Blk2Mcr(32, 202, 641, ..);
    Blk2Mcr(14, 1);
    Blk2Mcr(14, 5);
    Blk2Mcr(14, 7);
    Blk2Mcr(14, 9);
    Blk2Mcr(37, 7);
    Blk2Mcr(37, 9);
    Blk2Mcr(236, 0);
    Blk2Mcr(236, 1);
    Blk2Mcr(236, 5);
    Blk2Mcr(236, 8);
    Blk2Mcr(237, 0);
    Blk2Mcr(237, 1);
    Blk2Mcr(237, 5);
    Blk2Mcr(237, 7);
    Blk2Mcr(240, 0);
    Blk2Mcr(240, 1);
    Blk2Mcr(240, 5);
    Blk2Mcr(241, 0);
    Blk2Mcr(241, 1);
    Blk2Mcr(241, 5);
    Blk2Mcr(241, 9);
    Blk2Mcr(243, 0);
    Blk2Mcr(243, 1);
    Blk2Mcr(243, 5);
    Blk2Mcr(243, 8);
    Blk2Mcr(244, 0);
    Blk2Mcr(244, 1);
    Blk2Mcr(244, 5);
    Blk2Mcr(244, 6);
    Blk2Mcr(244, 7);
    Blk2Mcr(244, 8);
    Blk2Mcr(245, 0);
    Blk2Mcr(245, 1);
    Blk2Mcr(245, 5);
    Blk2Mcr(246, 0);
    Blk2Mcr(246, 1);
    Blk2Mcr(246, 5);
    Blk2Mcr(246, 8);
    Blk2Mcr(247, 0);
    Blk2Mcr(247, 1);
    Blk2Mcr(247, 5);
    Blk2Mcr(247, 8);
    Blk2Mcr(194, 1);
    Blk2Mcr(195, 0);
    Blk2Mcr(198, 1);
    Blk2Mcr(199, 0);
    Blk2Mcr(203, 0);
    Blk2Mcr(180, 0);
    Blk2Mcr(180, 8);
    Blk2Mcr(180, 9);
    Blk2Mcr(181, 5);
    Blk2Mcr(182, 0);
    Blk2Mcr(183, 0);
    Blk2Mcr(184, 1);
    Blk2Mcr(185, 1);
    Blk2Mcr(186, 0);
    Blk2Mcr(187, 1);
    Blk2Mcr(188, 0);
    Blk2Mcr(188, 3);
    Blk2Mcr(188, 5);
    Blk2Mcr(188, 9);
    Blk2Mcr(189, 0);
    Blk2Mcr(190, 0);
    Blk2Mcr(191, 0);
    Blk2Mcr(192, 0);
    Blk2Mcr(193, 1);
    Blk2Mcr(196, 1);
    Blk2Mcr(197, 0);
    Blk2Mcr(200, 1);
    Blk2Mcr(201, 0);
    Blk2Mcr(86, 6);
    Blk2Mcr(86, 7);
    Blk2Mcr(86, 8);
    Blk2Mcr(212, 6);
    Blk2Mcr(212, 7);
    Blk2Mcr(212, 8);
    Blk2Mcr(232, 6);
    Blk2Mcr(232, 7);
    Blk2Mcr(232, 8);
    Blk2Mcr(234, 6);
    Blk2Mcr(234, 7);
    Blk2Mcr(234, 8);
    Blk2Mcr(388, 2);
    Blk2Mcr(388, 8);
    Blk2Mcr(390, 2);
    Blk2Mcr(390, 4);
    Blk2Mcr(390, 6);
    Blk2Mcr(402, 0);
    Blk2Mcr(402, 6);
    Blk2Mcr(402, 8);
    Blk2Mcr(479, 1);
    Blk2Mcr(479, 3);
    Blk2Mcr(479, 5);
    Blk2Mcr(479, 7);
    Blk2Mcr(479, 9);
    Blk2Mcr(543, 1);
    Blk2Mcr(543, 2);
    Blk2Mcr(543, 4);
    Blk2Mcr(543, 8);
    Blk2Mcr(558, 0);
    Blk2Mcr(558, 2);
    Blk2Mcr(558, 4);
    Blk2Mcr(558, 6);
    Blk2Mcr(558, 8);
    Blk2Mcr(468, 1);
    Blk2Mcr(53, 1);
    Blk2Mcr(54, 0);
    Blk2Mcr(57, 1);
    Blk2Mcr(58, 0);
    Blk2Mcr(61, 1);
    Blk2Mcr(85, 1);
    Blk2Mcr(87, 0);
    Blk2Mcr(118, 1);
    Blk2Mcr(120, 1);
    Blk2Mcr(122, 1);
    Blk2Mcr(229, 1);
    Blk2Mcr(231, 1);
    Blk2Mcr(372, 1);
    // reused micros in fixCryptShadows
    // Blk2Mcr(619, 1);
    // Blk2Mcr(620, 0);
    // Blk2Mcr(621, 1);
    // Blk2Mcr(624, 0);
    // Blk2Mcr(625, 0);
    // Blk2Mcr(630, 0);
    // Blk2Mcr(632, 0);
    // Blk2Mcr(633, 0);
    Blk2Mcr(637, 0);
    Blk2Mcr(642, 1);
    Blk2Mcr(644, 0);
    Blk2Mcr(645, 1);
    Blk2Mcr(646, 0);
    Blk2Mcr(649, 1);
    Blk2Mcr(650, 0);
    int unusedSubtiles[] = {
        8, 10, 11, 16, 19, 20, 23, 24, 26, 28, 30, 35, 38, 40, 43, 44, 50, 52, 56, 76, 78, 81, 82, 87, 90, 92, 94, 96, 98, 100, 102, 103, 105, 106, 108, 110, 112, 114, 116, 124, 127, 128, 137, 138, 139, 141, 143, 147, 148, 172, 174, 176, 177, 202, 205, 207, 210, 211, 214, 217, 219, 221, 223, 225, 227, 233, 235, 239, 249, 251, 253, 257, 259, 262, 263, 270, 273, 278, 279, 295, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 354, 373, 381, 390, 472, 489, 490, 540, 560, 640, 643, 648
    };
    for (int n = 0; n < lengthof(unusedSubtiles); n++) {
        for (int i = 0; i < blockSize; i++) {
            Blk2Mcr(unusedSubtiles[n], i);
        }
    }
}

void D1Tileset::patch(int dunType, bool silent)
{
    std::set<unsigned> deletedFrames;
    switch (dunType) {
    case DTYPE_TOWN: {
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
        bool isHellfireTown = this->min->getSubtileCount() == 1379;
        if (isHellfireTown) {
            // #ifdef HELLFIRE
            // fix bad artifacts
            Blk2Mcr(1273, 7);
            Blk2Mcr(1303, 7);
        }
        // patch dMicroCels - TOWN.CEL
        // - overwrite subtile 557 and 558 with subtile 939 and 940 to make the inner tile of Griswold's house non-walkable
        ReplaceMcr(237, 0, 402, 0);
        ReplaceMcr(237, 1, 402, 1);
        // patch subtiles around the pot of Adria to prevent graphical glitch when a player passes it
        this->patchTownPot(553, 554, silent);
        // eliminate micros of unused subtiles
        ReplaceMcr(169, 1, 129, 1);
        ReplaceMcr(178, 1, 118, 1);
        ReplaceMcr(181, 1, 129, 1);
        ReplaceMcr(1159, 1, 291, 1);
        // ReplaceMcr(871, 11, 358, 12);
        Blk2Mcr(358, 12);
        ReplaceMcr(947, 15, 946, 15);
        ReplaceMcr(1175, 4, 1171, 4);
        ReplaceMcr(1218, 3, 1211, 3);
        ReplaceMcr(1218, 5, 1211, 5);
        Blk2Mcr(110, 0);
        Blk2Mcr(113, 0);
        Blk2Mcr(183, 0);
        Blk2Mcr(235, 0);
        Blk2Mcr(239, 0);
        Blk2Mcr(240, 0);
        Blk2Mcr(243, 0);
        Blk2Mcr(244, 0);
        Blk2Mcr(1132, 2);
        Blk2Mcr(1132, 3);
        Blk2Mcr(1132, 4);
        Blk2Mcr(1132, 5);
        Blk2Mcr(1152, 0);
        Blk2Mcr(1139, 0);
        Blk2Mcr(1139, 1);
        Blk2Mcr(1139, 2);
        Blk2Mcr(1139, 3);
        Blk2Mcr(1139, 4);
        Blk2Mcr(1139, 5);
        Blk2Mcr(1139, 6);
        Blk2Mcr(1140, 0);
        Blk2Mcr(1164, 1);
        Blk2Mcr(1258, 0);
        Blk2Mcr(1258, 1);
        Blk2Mcr(1214, 1);
        Blk2Mcr(1214, 2);
        Blk2Mcr(1214, 3);
        Blk2Mcr(1214, 4);
        Blk2Mcr(1214, 5);
        Blk2Mcr(1214, 6);
        Blk2Mcr(1214, 7);
        Blk2Mcr(1214, 8);
        Blk2Mcr(1214, 9);
        Blk2Mcr(1216, 8);
        int unusedSubtiles[] = {
            71, 79, 80, 166, 176, 228, 230, 236, 238, 241, 242, 245, 246, 247, 248, 249, 250, 251, 252, 253, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 577, 578, 579, 580, 750, 751, 752, 753, 1064, 1115, 1116, 1117, 1118, 1135, 1136, 1137, 1138, 1141, 1142, 1143, 1144, 1145, 1146, 1147, 1148, 1149, 1150, 1151, 1153, 1199, 1200, 1201, 1202, 1221, 1222, 1223, 1224, 1225, 1226, 1227, 1228, 1229, 1230, 1231, 1232, 1233, 1234, 1235, 1236
        };
        for (int n = 0; n < lengthof(unusedSubtiles); n++) {
            for (int i = 0; i < BLOCK_SIZE_TOWN; i++) {
                Blk2Mcr(unusedSubtiles[n], i);
            }
        }
        if (isHellfireTown) {
            Blk2Mcr(1344, 1);
            Blk2Mcr(1360, 0);
            Blk2Mcr(1370, 0);
            Blk2Mcr(1376, 0);
            Blk2Mcr(1295, 1);
            int unusedSubtilesHellfire[] = {
                1293, 1341, 1342, 1343, 1345, 1346, 1347, 1348, 1349, 1350, 1351, 1352, 1353, 1354, 1355, 1356, 1357, 1358, 1359, 1361, 1362, 1363, 1364, 1365, 1366, 1367, 1368, 1369, 1371, 1372, 1373, 1374, 1375, 1377, 1378, 1379
            };
            for (int n = 0; n < lengthof(unusedSubtilesHellfire); n++) {
                for (int i = 0; i < BLOCK_SIZE_TOWN; i++) {
                    Blk2Mcr(unusedSubtilesHellfire[n], i);
                }
            }
        }
        // patch subtiles of the cathedral to fix graphical glitch
        this->patchTownCathedral(805, 806, 807, silent);
    } break;
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
        // patch dSolidTable - L1.SOL
        ChangeSubtileSolFlags(this->sol, 8 - 1, PFLAG_BLOCK_MISSILE, false, silent); // the only column which was blocking missiles
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
        ReplaceMcr(560, 0, 9, 0);
        ReplaceMcr(560, 1, 9, 1);
        if (this->min->getSubtileCount() < 561) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(561 - 1, 1);
        }
        ReplaceMcr(561, 0, 11, 0);
        ReplaceMcr(561, 1, 11, 1);
        if (this->min->getSubtileCount() < 562) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(562 - 1, 3);
        }
        ReplaceMcr(562, 0, 9, 0);
        ReplaceMcr(562, 1, 9, 1);
        if (this->min->getSubtileCount() < 563) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(563 - 1, 4);
        }
        ReplaceMcr(563, 0, 10, 0);
        ReplaceMcr(563, 1, 10, 1);
        if (this->min->getSubtileCount() < 564) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(564 - 1, 2);
        }
        ReplaceMcr(564, 0, 159, 0);
        ReplaceMcr(564, 1, 159, 1);
        if (this->min->getSubtileCount() < 565) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(565 - 1, 1);
        }
        ReplaceMcr(565, 0, 161, 0);
        ReplaceMcr(565, 1, 161, 1);
        if (this->min->getSubtileCount() < 566) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(566 - 1, 3);
        }
        ReplaceMcr(566, 0, 166, 0);
        ReplaceMcr(566, 1, 166, 1);
        if (this->min->getSubtileCount() < 567) {
            this->createSubtile();
            this->spt->setSubtileSpecProperty(567 - 1, 4);
        }
        ReplaceMcr(567, 0, 167, 0);
        ReplaceMcr(567, 1, 167, 1);
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
        Blk2Mcr(288, 7);
        // patch dAutomapData - L2.AMP
        ChangeTileMapFlags(this->amp, 42 - 1, MAPFLAG_HORZARCH, false, silent);
        ChangeTileMapFlags(this->amp, 156 - 1, MAPFLAG_VERTDOOR, false, silent);
        ChangeTileMapType(this->amp, 156 - 1, 0, silent);
        ChangeTileMapFlags(this->amp, 157 - 1, MAPFLAG_HORZDOOR, false, silent);
        ChangeTileMapType(this->amp, 157 - 1, 0, silent);
        break;
    case DTYPE_CAVES:
        // patch dMiniTiles - L3.MIN
        // fix bad artifact
        Blk2Mcr(82, 4);
        // patch dSolidTable - L3.SOL
        ChangeSubtileSolFlags(this->sol, 249 - 1, PFLAG_BLOCK_PATH, false, silent); // sync tile 68 and 69 by making subtile 249 of tile 68 walkable.
        break;
    case DTYPE_HELL:
        this->patchHellExit(45 - 1, silent);
        // patch dAutomapData - L4.AMP
        ChangeTileMapFlags(this->amp, 52 - 1, MAPFLAG_VERTGRATE, true, silent);
        ChangeTileMapFlags(this->amp, 56 - 1, MAPFLAG_HORZGRATE, true, silent);
        // patch dSolidTable - L4.SOL
        ChangeSubtileSolFlags(this->sol, 141 - 1, PFLAG_BLOCK_MISSILE, false, silent); // fix missile-blocking tile of down-stairs.
        // fix missile-blocking tile of down-stairs + fix non-walkable tile of down-stairs
        ChangeSubtileSolFlags(this->sol, 137 - 1, PFLAG_BLOCK_PATH | PFLAG_BLOCK_MISSILE, false, silent);
        ChangeSubtileSolFlags(this->sol, 130 - 1, PFLAG_BLOCK_PATH, true, silent); // make the inner tiles of the down-stairs non-walkable I.
        ChangeSubtileSolFlags(this->sol, 132 - 1, PFLAG_BLOCK_PATH, true, silent); // make the inner tiles of the down-stairs non-walkable II.
        ChangeSubtileSolFlags(this->sol, 131 - 1, PFLAG_BLOCK_PATH, true, silent); // make the inner tiles of the down-stairs non-walkable III.
        // fix all-blocking tile on the diablo-level
        ChangeSubtileSolFlags(this->sol, 211 - 1, PFLAG_BLOCK_PATH | PFLAG_BLOCK_LIGHT | PFLAG_BLOCK_MISSILE, false, silent);
        break;
    case DTYPE_NEST:
        // patch dMiniTiles - L6.MIN
        // useless black micros
        Blk2Mcr(21, 0);
        Blk2Mcr(21, 1);
        // fix bad artifacts
        Blk2Mcr(132, 7);
        Blk2Mcr(366, 1);
        // patch dSolidTable - L6.SOL
        ChangeSubtileSolFlags(this->sol, 390 - 1, PFLAG_BLOCK_PATH, false, silent); // make a pool tile walkable I.
        ChangeSubtileSolFlags(this->sol, 413 - 1, PFLAG_BLOCK_PATH, false, silent); // make a pool tile walkable II.
        ChangeSubtileSolFlags(this->sol, 416 - 1, PFLAG_BLOCK_PATH, false, silent); // make a pool tile walkable III.
        break;
    case DTYPE_CRYPT:
        this->fillCryptShapes(silent);
        this->maskCryptBlacks(silent);
        this->fixCryptShadows(silent);
        this->cleanupCrypt(deletedFrames, silent);
        // patch dSolidTable - L5.SOL
        ChangeSubtileSolFlags(this->sol, 143 - 1, PFLAG_BLOCK_PATH, false, silent); // make right side of down-stairs consistent (walkable)
        ChangeSubtileSolFlags(this->sol, 148 - 1, PFLAG_BLOCK_PATH, false, silent); // make the back of down-stairs consistent (walkable)
        // make collision-checks more reasonable
        //  - prevent non-crossable floor-tile configurations I.
        ChangeSubtileSolFlags(this->sol, 461 - 1, PFLAG_BLOCK_PATH, false, silent);
        //  - set top right tile of an arch non-walkable (full of lava) - skip to prevent lockout
        // ChangeSubtileSolFlags(this->sol, 471 - 1, PFLAG_BLOCK_PATH, true, silent);
        //  - set top right tile of a pillar walkable (just a small obstacle)
        ChangeSubtileSolFlags(this->sol, 481 - 1, PFLAG_BLOCK_PATH, false, silent);
        //  - tile 491 is the same as tile 594 which is not solid
        //  - prevents non-crossable floor-tile configurations
        ChangeSubtileSolFlags(this->sol, 491 - 1, PFLAG_BLOCK_PATH, false, silent);
        //  - set bottom left tile of a rock non-walkable (rather large obstacle, feet of the hero does not fit)
        //  - prevents non-crossable floor-tile configurations
        ChangeSubtileSolFlags(this->sol, 523 - 1, PFLAG_BLOCK_PATH, true, silent);
        //  - set the top right tile of a floor mega walkable (similar to 594 which is not solid)
        ChangeSubtileSolFlags(this->sol, 570 - 1, PFLAG_BLOCK_PATH, false, silent);
        //  - prevent non-crossable floor-tile configurations II.
        ChangeSubtileSolFlags(this->sol, 598 - 1, PFLAG_BLOCK_PATH, false, silent);
        ChangeSubtileSolFlags(this->sol, 600 - 1, PFLAG_BLOCK_PATH, false, silent);
        // - adjust SOL after cleanupCrypt
        ChangeSubtileSolFlags(this->sol, 238 - 1, PFLAG_BLOCK_PATH | PFLAG_BLOCK_LIGHT | PFLAG_BLOCK_MISSILE, false, silent);
        ChangeSubtileSolFlags(this->sol, 178 - 1, PFLAG_BLOCK_LIGHT | PFLAG_BLOCK_MISSILE, false, silent);
        ChangeSubtileSolFlags(this->sol, 242 - 1, PFLAG_BLOCK_PATH | PFLAG_BLOCK_LIGHT | PFLAG_BLOCK_MISSILE, false, silent);
        // - adjust AMP after cleanupCrypt
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
