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
    this->sla = new D1Sla();
    this->tla = new D1Tla();
}

D1Tileset::~D1Tileset()
{
    delete cls;
    delete min;
    delete til;
    delete sla;
    delete tla;
}

bool D1Tileset::loadCls(const QString &clsFilePath, const OpenAsParam &params)
{
    if (QFileInfo::exists(clsFilePath)) {
        return D1Cel::load(*this->cls, clsFilePath, params);
    }

    // fake D1CEL_TYPE to trigger clipped mode
    this->cls->setType(D1CEL_TYPE::V2_MONO_GROUP);
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
    QString slaFilePath = params.slaFilePath;
    QString minFilePath = params.minFilePath;
    QString tlaFilePath = params.tlaFilePath;

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
        if (slaFilePath.isEmpty()) {
            slaFilePath = basePath + ".sla";
        }
        if (tlaFilePath.isEmpty()) {
            tlaFilePath = basePath + ".tla";
        }
    }

    std::map<unsigned, D1CEL_FRAME_TYPE> celFrameTypes;
    if (!this->sla->load(slaFilePath)) {
        dProgressErr() << QApplication::tr("Failed loading SLA file: %1.").arg(QDir::toNativeSeparators(slaFilePath));
    } else if (!this->min->load(minFilePath, this, celFrameTypes, params)) {
        dProgressErr() << QApplication::tr("Failed loading MIN file: %1.").arg(QDir::toNativeSeparators(minFilePath));
    } else if (!this->til->load(tilFilePath, this->min)) {
        dProgressErr() << QApplication::tr("Failed loading TIL file: %1.").arg(QDir::toNativeSeparators(tilFilePath));
    } else if (!this->tla->load(tlaFilePath, this->til->getTileCount(), params)) {
        dProgressErr() << QApplication::tr("Failed loading TLA file: %1.").arg(QDir::toNativeSeparators(tlaFilePath));
    } else if (!this->loadCls(clsFilePath, params)) {
        dProgressErr() << QApplication::tr("Failed loading Special-CEL file: %1.").arg(QDir::toNativeSeparators(clsFilePath));
    } else if (!D1CelTileset::load(*this->gfx, celFrameTypes, gfxFilePath, params)) {
        dProgressErr() << QApplication::tr("Failed loading Tileset-CEL file: %1.").arg(QDir::toNativeSeparators(gfxFilePath));
    } else {
        return true; // !gfxFilePath.isEmpty() || !prevFilePath.isEmpty();
    }
    // clear possible inconsistent data
    // this->gfx->clear();
    this->cls->clear();
    this->min->clear();
    this->til->clear();
    this->sla->clear();
    this->tla->clear();
    return false;
}

void D1Tileset::saveCls(const SaveAsParam &params)
{
    QString filePath = this->cls->getFilePath();
    if (!params.clsFilePath.isEmpty()) {
        filePath = params.clsFilePath;
        if (!params.autoOverwrite && QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, QApplication::tr("Confirmation"), QApplication::tr("Are you sure you want to overwrite %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return;
            }
        }
    } else if (!this->cls->isModified()) {
        return;
    }

    // validate the frames
    for (int i = 0; i < this->cls->getFrameCount(); i++) {
        if (!this->cls->getFrame(i)->isClipped()) {
            dProgressWarn() << QApplication::tr("Non-clipped Special-CEL %1 file is not supported by the game (Diablo 1/DevilutionX).").arg(QDir::toNativeSeparators(filePath));
            break;
        }
    }

    if (this->cls->getFrameCount() != 0) {
        // S.CEL with content -> create or change
        SaveAsParam clsParams;
        clsParams.celFilePath = params.clsFilePath;
        clsParams.autoOverwrite = true;
        D1Cel::save(*this->cls, clsParams);
    } else {
        // S.CEL without content -> delete
        if (QFile::exists(filePath)) {
            if (!QFile::remove(filePath)) {
                dProgressFail() << QApplication::tr("Failed to remove file: %1.").arg(QDir::toNativeSeparators(filePath));
                return;
            }
        }
        this->cls->setFilePath(filePath); // this->cls->load(filePath, allocate);
        this->cls->setModified(false);
    }
}

void D1Tileset::save(const SaveAsParam &params)
{
    this->saveCls(params);
    this->min->save(params);
    this->til->save(params);
    this->sla->save(params);
    this->tla->save(params);
}

void D1Tileset::createFrame(int frameIndex)
{
    this->min->insertFrame(newFrameIndex);
}

void D1Tileset::insertTile(int tileIndex, const std::vector<int> &subtileIndices)
{
    this->til->insertTile(tileIndex, subtileIndices);
    this->tla->insertTile(tileIndex);
}

void D1Tileset::createTile(int tileIndex)
{
    int n = TILE_WIDTH * TILE_HEIGHT;
    this->insertTile(tileIndex, std::vector<int>(n));
}

int D1Tileset::duplicateTile(int tileIndex, bool deepCopy)
{
    this->createTile();

    int newTileIndex = this->til->getTileCount() - 1;
    // D1Til::duplicate
    std::vector<int> &baseSubtileIndices = this->til->getSubtileIndices(tileIndex);
    std::vector<int> &newSubtileIndices = this->til->getSubtileIndices(newTileIndex);
    newSubtileIndices = baseSubtileIndices;
    // D1Tla::duplicate
    this->tla->setTileProperties(newTileIndex, this->tla->getTileProperties(tileIndex));

    if (deepCopy) {
        for (unsigned i = 0; i < newSubtileIndices.size(); i++) {
            newSubtileIndices[i] = this->duplicateSubtile(newSubtileIndices[i], true);
        }
    }
    return newTileIndex;
}

void D1Tileset::removeTile(int tileIndex)
{
    this->til->removeTile(tileIndex);
    this->tla->removeTile(tileIndex);
}

void D1Tileset::insertSubtile(int subtileIndex, const std::vector<unsigned> &frameReferencesList)
{
    this->min->insertSubtile(subtileIndex, frameReferencesList);
    this->sla->insertSubtile(subtileIndex);
}

void D1Tileset::createSubtile(int subtileIndex)
{
    int n = this->min->getSubtileWidth() * this->min->getSubtileHeight();
    this->insertSubtile(subtileIndex, std::vector<int>(n));
}

int D1Tileset::duplicateSubtile(int subtileIndex, bool deepCopy)
{
    this->createSubtile();

    int newSubtileIndex = this->min->getSubtileCount() - 1;
    // D1Min::duplicate
    std::vector<unsigned> &baseFrameReferences = this->min->getFrameReferences(subtileIndex);
    std::vector<unsigned> &newFrameReferences = this->min->getFrameReferences(newSubtileIndex);
    newFrameReferences = baseFrameReferences;

    // D1Sla::duplicate
    this->sla->setSubProperties(newSubtileIndex, this->sla->getSubProperties(subtileIndex));
    this->sla->setTrapProperty(newSubtileIndex, this->sla->getTrapProperty(subtileIndex));
    this->sla->setSpecProperty(newSubtileIndex, this->sla->getSpecProperty(subtileIndex));
    this->sla->setRenderProperties(newSubtileIndex, this->sla->getRenderProperties(subtileIndex));
    this->sla->setMapType(newSubtileIndex, this->sla->getMapType(subtileIndex));
    this->sla->setMapProperties(newSubtileIndex, this->sla->getMapProperties(subtileIndex));

    if (deepCopy) {
        for (unsigned i = 0; i < newFrameReferences.size(); i++) {
            int frameRef = newFrameReferences[i];
            if (frameRef == 0) {
                continue;
            }
            newFrameReferences[i] = this->gfx->duplicateFrame(frameRef - 1, false) + 1;
        }
    }
    return newSubtileIndex;
}

void D1Tileset::removeSubtile(int subtileIndex, int replacement)
{
    this->til->removeSubtile(subtileIndex, replacement);
    this->sla->removeSubtile(subtileIndex);
}

void D1Tileset::resetSubtileFlags(int subtileIndex)
{
    this->sla->resetSubtileFlags(subtileIndex);
}

void D1Tileset::remapSubtiles(const std::map<unsigned, unsigned> &remap)
{
    this->min->remapSubtiles(remap);
    this->sla->remapSubtiles(remap);
}

void D1Tileset::swapFrames(unsigned frameIndex0, unsigned frameIndex1)
{
    this->gfx->swapFrames(frameIndex0, frameIndex1);
    // D1Min::swapFrames
    const unsigned numFrames = this->gfx->getFrameCount();
    if (frameIndex0 >= numFrames) {
        // move frameIndex1 to the front
        if (frameIndex1 == 0 || frameIndex1 >= numFrames) {
            return;
        }
        for (int i = 0; i < this->min->getSubtileCount(); i++) {
            std::vector<unsigned> &frameRefs = this->min->getFrameReferences(i);
            for (unsigned n = 0; n < frameRefs.size(); n++) {
                if (frameRefs[n] == frameIndex1 + 1) {
                    frameRefs[n] = 1;
                } else if (frameRefs[n] < frameIndex1 + 1 && frameRefs[n] != 0) {
                    frameRefs[n] = frameRefs[n] + 1;
                } else {
                    continue;
                }
                this->min->setModified();
            }
        }
    } else if (frameIndex1 >= numFrames) {
        // move frameIndex0 to the end
        if (frameIndex0 == numFrames - 1) {
            return;
        }
        for (int i = 0; i < this->min->getSubtileCount(); i++) {
            std::vector<unsigned> &frameRefs = this->min->getFrameReferences(i);
            for (unsigned n = 0; n < frameRefs.size(); n++) {
                if (frameRefs[n] == frameIndex0 + 1) {
                    frameRefs[n] = numFrames;
                } else if (frameRefs[n] > frameIndex0 + 1) {
                    frameRefs[n] = frameRefs[n] - 1;
                } else {
                    continue;
                }
                this->min->setModified();
            }
        }
    } else {
        // swap frameIndex0 and frameIndex1
        if (frameIndex0 == frameIndex1) {
            return;
        }
        for (int i = 0; i < this->min->getSubtileCount(); i++) {
            std::vector<unsigned> &frameRefs = this->min->getFrameReferences(i);
            for (unsigned n = 0; n < frameRefs.size(); n++) {
                if (frameRefs[n] == frameIndex0 + 1) {
                    frameRefs[n] = frameIndex1 + 1;
                } else if (frameRefs[n] == frameIndex1 + 1) {
                    frameRefs[n] = frameIndex0 + 1;
                } else {
                    continue;
                }
                this->min->setModified();
            }
        }
    }
}

void D1Tileset::swapSubtiles(unsigned subtileIndex0, unsigned subtileIndex1)
{
    std::map<unsigned, unsigned> remap;
    const unsigned numSubtiles = this->sla->getSubtileCount();
    if (subtileIndex0 >= numSubtiles) {
        // move subtileIndex1 to the front
        if (subtileIndex1 == 0 || subtileIndex1 >= numSubtiles) {
            return;
        }
        for (unsigned i = 0; i < subtileIndex1; i++) {
            remap[i] = i + 1;
        }
        remap[subtileIndex1] = 0;
        for (unsigned i = subtileIndex1 + 1; i < numSubtiles; i++) {
            remap[i] = i;
        }
    } else if (subtileIndex1 >= numSubtiles) {
        // move subtileIndex0 to the end
        if (subtileIndex0 == numSubtiles - 1) {
            return;
        }
        for (unsigned i = 0; i < subtileIndex0; i++) {
            remap[i] = i;
        }
        remap[subtileIndex0] = numSubtiles - 1;
        for (unsigned i = subtileIndex0 + 1; i < numSubtiles; i++) {
            remap[i] = i - 1;
        }
    } else {
        // swap subtileIndex0 and subtileIndex1
        if (subtileIndex0 == subtileIndex1) {
            return;
        }
        for (unsigned i = 0; i < numSubtiles; i++) {
            remap[i] = i;
        }
        remap[subtileIndex0] = subtileIndex1;
        remap[subtileIndex1] = subtileIndex0;
    }

    this->remapSubtiles(remap);
}

void D1Tileset::swapTiles(unsigned tileIndex0, unsigned tileIndex1)
{
    const unsigned numTiles = this->til->getTileCount();
    if (tileIndex0 >= numTiles) {
        // move tileIndex1 to the front
        if (tileIndex1 == 0 || tileIndex1 >= numTiles) {
            return;
        }
        for (unsigned i = tileIndex1; i > 0; i--) {
            // D1Til::swapTiles
            std::vector<int> &subtiles0 = this->til->getSubtileIndices(i);
            std::vector<int> &subtiles1 = this->til->getSubtileIndices(i - 1);
            subtiles0.swap(subtiles1);
            this->til->setModified();
            // D1Tla::swapTiles
            quint8 properties0 = this->tla->getTileProperties(i);
            quint8 properties1 = this->tla->getTileProperties(i - 1);
            this->tla->setTileProperties(i, properties1);
            this->tla->setTileProperties(i - 1, properties0);
        }
    } else if (tileIndex1 >= numTiles) {
        // move tileIndex0 to the end
        // if (tileIndex0 == numTiles - 1) {
        //    return;
        // }
        for (unsigned i = tileIndex0; i < numTiles - 1; i++) {
            // D1Til::swapTiles
            std::vector<int> &subtiles0 = this->til->getSubtileIndices(i);
            std::vector<int> &subtiles1 = this->til->getSubtileIndices(i + 1);
            subtiles0.swap(subtiles1);
            this->til->setModified();
            // D1Tla::swapTiles
            quint8 properties0 = this->tla->getTileProperties(i);
            quint8 properties1 = this->tla->getTileProperties(i + 1);
            this->tla->setTileProperties(i, properties1);
            this->tla->setTileProperties(i + 1, properties0);
        }
    } else {
        // swap tileIndex0 and tileIndex1
        if (tileIndex0 == tileIndex1) {
            return;
        }
        // D1Til::swapTiles
        std::vector<int> &subtiles0 = this->til->getSubtileIndices(tileIndex0);
        std::vector<int> &subtiles1 = this->til->getSubtileIndices(tileIndex1);
        subtiles0.swap(subtiles1);
        this->til->setModified();
        // D1Tla::swapTiles
        quint8 properties0 = this->tla->getTileProperties(tileIndex0);
        quint8 properties1 = this->tla->getTileProperties(tileIndex1);
        this->tla->setTileProperties(tileIndex0, properties1);
        this->tla->setTileProperties(tileIndex1, properties0);
    }
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
    progress.second = QApplication::tr("Reused %n frame(s).", "", amount);
    dProgress() << progress;

    ProgressDialog::decBar();
    return result != 0;
}

bool D1Tileset::reuseSubtiles(std::map<unsigned, unsigned> &remap)
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
            if (this->sla->getSpecProperty(i) != this->sla->getSpecProperty(j) || this->sla->getTrapProperty(i) != this->sla->getTrapProperty(j)) {
                dProgress() << QApplication::tr("Subtile %1 has the same frames as Subtile %2, but the Special-properties are different.").arg(i + 1).arg(j + 1);
                continue;
            }
            if (this->sla->getMapType(i) != this->sla->getMapType(j) || this->sla->getMapProperties(i) != this->sla->getMapProperties(j)) {
                dProgress() << QApplication::tr("Subtile %1 has the same frames as Subtile %2, but the Map-properties are different.").arg(i + 1).arg(j + 1);
                continue;
            }
            if (this->sla->getSubProperties(i) != this->sla->getSubProperties(j)) {
                dProgress() << QApplication::tr("Subtile %1 has the same frames as Subtile %2, but the Collision-properties are different.").arg(i + 1).arg(j + 1);
                continue;
            }
            if (this->sla->getRenderProperties(i) != this->sla->getRenderProperties(j)) {
                dProgress() << QApplication::tr("Subtile %1 has the same frames as Subtile %2, but the Render-properties are different.").arg(i + 1).arg(j + 1);
                continue;
            }
            // use subtile 'i' instead of subtile 'j'
            this->removeSubtile(j, i);
            // calculate the original indices
            unsigned originalIndexI = i;
            for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
                if (iter->first <= originalIndexI) {
                    originalIndexI++;
                    continue;
                }
                break;
            }
            unsigned originalIndexJ = j;
            for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
                if (iter->first <= originalIndexJ) {
                    originalIndexJ++;
                    continue;
                }
                break;
            }
            remap[originalIndexJ] = originalIndexI;
            dProgress() << QApplication::tr("Using subtile %1 instead of %2.").arg(originalIndexI + 1).arg(originalIndexJ + 1);
            result = 1;
            j--;
        }
        if (!ProgressDialog::incValue()) {
            result |= 2;
            break;
        }
    }
    auto amount = remap.size();
    dProgress() << QApplication::tr("Reused %n subtile(s).", "", amount);

    ProgressDialog::decBar();
    return result != 0;
}

bool D1Tileset::reuseTiles(std::map<unsigned, unsigned> &remap)
{
    ProgressDialog::incBar(QApplication::tr("Deduplicating tiles..."), this->til->getTileCount());
    int result = 0;
    for (int i = 0; i < this->til->getTileCount(); i++) {
        if (ProgressDialog::wasCanceled()) {
            result |= 2;
            break;
        }
        for (int j = i + 1; j < this->til->getTileCount(); j++) {
            std::vector<int> &subtileIndices0 = this->til->getSubtileIndices(i);
            std::vector<int> &subtileIndices1 = this->til->getSubtileIndices(j);
            if (subtileIndices0.size() != subtileIndices1.size()) {
                continue; // should not happen, but better safe than sorry
            }
            bool match = true;
            for (unsigned x = 0; x < subtileIndices0.size(); x++) {
                if (subtileIndices0[x] == subtileIndices1[x]) {
                    continue;
                }
                match = false;
                break;
            }
            if (!match) {
                continue;
            }
            if (this->tla->getTileProperties(i) != this->tla->getTileProperties(j)) {
                dProgress() << QApplication::tr("Tile %1 has the same subtiles as Tile %2, but the TLA-properties are different.").arg(i + 1).arg(j + 1);
                continue;
            }
            // remove tile 'j'
            this->removeTile(j);
            // calculate the original indices
            unsigned originalIndexI = i;
            for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
                if (iter->first <= originalIndexI) {
                    originalIndexI++;
                    continue;
                }
                break;
            }
            unsigned originalIndexJ = j;
            for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
                if (iter->first <= originalIndexJ) {
                    originalIndexJ++;
                    continue;
                }
                break;
            }
            remap[originalIndexJ] = originalIndexI;
            dProgress() << QApplication::tr("Removed Tile %1 because it was the same as Tile %2.").arg(originalIndexJ + 1).arg(originalIndexI + 1);
            result = 1;
            j--;
        }
        if (!ProgressDialog::incValue()) {
            result |= 2;
            break;
        }
    }
    auto amount = remap.size();
    dProgress() << QApplication::tr("Removed %n tile(s).", "", amount);

    ProgressDialog::decBar();
    return result != 0;
}

#define HideMcr(n, x) RemoveFrame(this->min, n, x, nullptr, silent);
#define Blk2Mcr(n, x) RemoveFrame(this->min, n, x, &deletedFrames, silent);
#define MICRO_IDX(blockSize, microIndex) ((blockSize) - (2 + ((microIndex) & ~1)) + ((microIndex)&1))
static void RemoveFrame(D1Min *min, int subtileRef, int microIndex, std::set<unsigned> *deletedFrames, bool silent)
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
        if (deletedFrames != nullptr) {
            deletedFrames->insert(frameReference);
        }
        min->setFrameReference(subtileIndex, index, 0);
        if (!silent) {
            dProgress() << QApplication::tr("Removed Frame (%1) @%2 in Subtile %3.").arg(frameReference).arg(index + 1).arg(subtileIndex + 1);
        }
    } else {
        ; // dProgressWarn() << QApplication::tr("The frame @%1 in Subtile %2 is already empty.").arg(index + 1).arg(subtileIndex + 1);
    }
}

static void ReplaceSubtile(D1Til *til, int tileIndex, unsigned index, int subtileIndex, bool silent)
{
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
        ; // dProgressWarn() << QApplication::tr("Subtile %1 of Tile%2 is already %3.").arg(index + 1).arg(tileIndex + 1).arg(subtileIndex + 1);
    }
}

#define ReplaceMcr(dn, dx, sn, sx) ReplaceFrame(this->min, dn, dx, sn, sx, &deletedFrames, silent);
#define SetMcr(dn, dx, sn, sx) ReplaceFrame(this->min, dn, dx, sn, sx, nullptr, silent);
#define MoveMcr(dn, dx, sn, sx) \
{ \
    ReplaceFrame(this->min, dn, dx, sn, sx, nullptr, silent); \
    RemoveFrame(this->min, sn, sx, nullptr, silent); \
}
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

typedef struct {
    int subtileIndex;
    unsigned microIndex;
    D1CEL_FRAME_TYPE res_encoding;
} CelMicro;

std::pair<unsigned, D1GfxFrame *> D1Tileset::getFrame(int subtileIndex, int blockSize, unsigned microIndex)
{
    // TODO: check if there are enough subtiles
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

    // move the image up 553[5] (1470) and 553[3] (1471)
    for (int x = MICRO_WIDTH / 2; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel = frameLeft2->getPixel(x, y);
            frameLeft2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
            frameLeft2->setPixel(x, y, D1GfxPixel::transparentPixel());
        }
    }
    for (int x = MICRO_WIDTH / 2; x < MICRO_WIDTH; x++) {
        for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
            D1GfxPixel pixel = frameLeft1->getPixel(x, y);
            frameLeft2->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
            frameLeft1->setPixel(x, y, D1GfxPixel::transparentPixel());
        }
    }
    for (int x = MICRO_WIDTH / 2; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel = frameLeft1->getPixel(x, y);
            frameLeft1->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
            frameLeft1->setPixel(x, y, D1GfxPixel::transparentPixel());
        }
    }
    // copy image to the other micros 553[1] (1473) -> 553[3] 1471, 554[0] 1475
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
            frameLeft0->setPixel(x, y, D1GfxPixel::transparentPixel());
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

// should run only once
bool D1Tileset::patchTownFloor(bool silent)
{
    const CelMicro micros[] = {
        { 731 - 1, 9, D1CEL_FRAME_TYPE::TransparentSquare },  // 1923 move micro
        { 755 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },       // 1975 change type
        { 974 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },       // 2805 change type
        { 1030 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },      // 2943 change type
        { 220 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },  // 514  move micro
        { 221 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },  // 516
        { 962 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },  // 2775
        { 218 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },  // 511 move micro
        { 219 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },  // 513
        { 1166 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // 3289 move micro
        { 1167 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare }, // 3292
        { 1171 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare }, // 3302
        { 1172 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // 3303
        { 1175 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare }, // 3311
        { 1176 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // 3317
        { 845 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },  // 2358
        //{ 493 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },   // 866 TODO: fix light?
        //{ 290 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },   // 662 TODO: fix grass?
        //{ 290 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },   // 663
        //{ 334 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },   // 750 TODO: fix grass? + (349 & nest)
        //{ 334 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },   // 751
    };

    constexpr unsigned blockSize = BLOCK_SIZE_TOWN;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // move the image up - 1923
        if (i == 0) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y + MICRO_HEIGHT / 2);
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }

        // mask and move down the image - 514, 516
        if (i == 4) {
            const CelMicro &microDst = micros[i + 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                // - mask
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    quint8 color = frame->getPixel(x, y).getPaletteIndex();
                    if (x < 26 && (x < 23 || color < 110)) { // 110, 112, 113, 119, 121, 126
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
                // - move to 516
                for (int y = MICRO_HEIGHT / 2; y < 21; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y - MICRO_HEIGHT / 2, pixel); // 516
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
                // - move down
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask and move down the image - 2775
        if (i == 6) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                // - mask
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y > x / 2 && y < MICRO_HEIGHT - x / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
                // - move down
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask and move down the image - 511, 513
        if (i == 7) {
            const CelMicro &microDst = micros[i + 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                // - mask
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    quint8 color = frame->getPixel(x, y).getPaletteIndex();
                    if (x > 10 && (x > 20 || (color < 110 && color != 59 && color != 86 && color != 91 && color != 99 && color != 101))) { // 110, 112, 113, 115, 117, 119, 121, 122, 124, 126
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
                // - move to 513
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y - MICRO_HEIGHT / 2, pixel); // 513
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
                // - move down
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    change |= frame->setPixel(x, y + MICRO_HEIGHT / 2, frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask and move down the image 1166[0] (3289), 1167[1] (3292)
        if (i == 9) {
            const CelMicro &microDst = micros[i + 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                // - mask
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    quint8 color = frame->getPixel(x, y).getPaletteIndex();
                    if (x > 3 && (x > 24 || (color < 112 && color != 0 && color != 59 && color != 86 && color != 91 && color != 99 && color != 101 && color != 110))) { // 110, 112, 113, 115, 117, 119, 121, 122, 124, 126
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
                // - move to 3292
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y - MICRO_HEIGHT / 2, pixel); // 3292
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
                // - move down
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    change |= frame->setPixel(x, y + MICRO_HEIGHT / 2, frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask the image 1171[1] (3302), 1172[0] (3303), 1175[1] (3311) and 1176[0] (3317)
        if (i >= 11 && i < 15) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (pixel.getPaletteIndex() == 107) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask the image 845[4] (2358)
        if (i == 15) {
            for (int x = 0; x < 10; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // fix artifacts of the new micros
        if (i == 9) { //  1166[0]
            change |= frame->setPixel(12, 28, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(19, 27, D1GfxPixel::transparentPixel());
        }
        if (i == 10) { //  1167[1]
            change |= frame->setPixel(17, 5, frame->getPixel(16, 6));
            change |= frame->setPixel(18, 5, frame->getPixel(16, 5));
            change |= frame->setPixel(8, 4, frame->getPixel(5, 2));
            change |= frame->setPixel(7, 4, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(7, 5, frame->getPixel(8, 5));
            change |= frame->setPixel(5, 1, D1GfxPixel::transparentPixel());
        }
        if (/*micro.res_encoding != D1CEL_FRAME_TYPE::Empty &&*/ frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }
    return true;
}

// should run only once
bool D1Tileset::patchTownDoor(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{ 724 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // 1903 move micro
/*  1 */{ 724 - 1, 1, D1CEL_FRAME_TYPE::Empty }, // 1904
/*  2 */{ 724 - 1, 3, D1CEL_FRAME_TYPE::Empty }, // 1902
/*  3 */{ 723 - 1, 1, D1CEL_FRAME_TYPE::Empty }, // 1901
/*  4 */{ 715 - 1, 11, D1CEL_FRAME_TYPE::Empty }, // 1848
/*  5 */{ 715 - 1, 9, D1CEL_FRAME_TYPE::Empty }, // 1849
/*  6 */{ 715 - 1, 7, D1CEL_FRAME_TYPE::Empty }, // 1850 - unused
/*  7 */{ 715 - 1, 5, D1CEL_FRAME_TYPE::Empty }, // 1851 - unused
/*  8 */{ 715 - 1, 3, D1CEL_FRAME_TYPE::Empty }, // 1852
/*  9 */{ 715 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // 1854
/* 10 */{ 721 - 1, 4, D1CEL_FRAME_TYPE::Square }, // 1893
/* 11 */{ 721 - 1, 2, D1CEL_FRAME_TYPE::Square }, // 1894
/* 12 */{ 719 - 1, 4, D1CEL_FRAME_TYPE::Square }, // 1875
/* 13 */{ 719 - 1, 2, D1CEL_FRAME_TYPE::Square }, // 1877
/* 14 */{ 727 - 1, 7, D1CEL_FRAME_TYPE::Square }, // 1911
/* 15 */{ 727 - 1, 5, D1CEL_FRAME_TYPE::Square }, // 1912
/* 16 */{ 725 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare }, // 1905
/* 17 */{ 725 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare }, // 1906
/* 18 */{ 725 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // 1907

/* 19 */{ 428 - 1, 4, D1CEL_FRAME_TYPE::Empty }, // 1049
/* 20 */{ 428 - 1, 2, D1CEL_FRAME_TYPE::Empty }, // 1050
/* 21 */{ 428 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // 1051
/* 22 */{ 418 - 1, 5, D1CEL_FRAME_TYPE::Square }, // 1005
/* 23 */{ 418 - 1, 3, D1CEL_FRAME_TYPE::Square }, // 1006
/* 24 */{ 418 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid }, // 1008
/* 25 */{ 426 - 1, 2, D1CEL_FRAME_TYPE::Empty }, // 1045
/* 26 */{ 426 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // 1046
/* 27 */{ 428 - 1, 1, D1CEL_FRAME_TYPE::Empty }, // 1052
/* 28 */{ 429 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // 1053
/* 29 */{ 419 - 1, 5, D1CEL_FRAME_TYPE::Square }, // 1013
/* 30 */{ 419 - 1, 3, D1CEL_FRAME_TYPE::Square }, // 1014
/* 31 */{ 419 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid }, // 1016

/* 32 */{ 911 - 1, 9, D1CEL_FRAME_TYPE::Empty }, // 2560
/* 33 */{ 911 - 1, 7, D1CEL_FRAME_TYPE::Empty }, // 2561
/* 34 */{ 911 - 1, 5, D1CEL_FRAME_TYPE::Empty }, // 2562
/* 35 */{ 931 - 1, 5, D1CEL_FRAME_TYPE::Square }, // 2643
/* 36 */{ 931 - 1, 3, D1CEL_FRAME_TYPE::Square }, // 2644
/* 37 */{ 931 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid }, // 2646
/* 38 */{ 402 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // 939
/* 39 */{ 954 - 1, 2, D1CEL_FRAME_TYPE::Empty }, // 2746
/* 40 */{ 919 - 1, 9, D1CEL_FRAME_TYPE::Empty }, // 2587
/* 41 */{ 919 - 1, 5, D1CEL_FRAME_TYPE::Empty }, // 2589
/* 42 */{ 927 - 1, 5, D1CEL_FRAME_TYPE::Square }, // 2625
/* 43 */{ 927 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid }, // 2627
/* 44 */{ 956 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle }, // 2760 - unused
        // { 956 - 1, 2, D1CEL_FRAME_TYPE::Empty }, // 2759
/* 45 */{ 954 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle }, // 2748 - unused
/* 46 */{ 919 - 1, 7, D1CEL_FRAME_TYPE::Square }, // 2588
/* 47 */{ 918 - 1, 9, D1CEL_FRAME_TYPE::Empty }, // 2578
/* 48 */{ 926 - 1, 5, D1CEL_FRAME_TYPE::Square }, // 2619
/* 49 */{ 927 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // 2626
/* 50 */{ 918 - 1, 3, D1CEL_FRAME_TYPE::Empty }, // 2584
/* 51 */{ 918 - 1, 2, D1CEL_FRAME_TYPE::Empty }, // 2583
/* 52 */{ 918 - 1, 5, D1CEL_FRAME_TYPE::Square }, // 2582
/* 53 */{ 929 - 1, 0, D1CEL_FRAME_TYPE::LeftTrapezoid }, // 2632
/* 54 */{ 929 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid }, // 2633
/* 55 */{ 918 - 1, 8, D1CEL_FRAME_TYPE::Empty }, // 2577
/* 56 */{ 926 - 1, 4, D1CEL_FRAME_TYPE::Square }, // 2618
/* 57 */{ 928 - 1, 4, D1CEL_FRAME_TYPE::Empty }, // 2631
/* 58 */{ 920 - 1, 8, D1CEL_FRAME_TYPE::Square }, // 2592
/* 59 */{ 551 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // 1467
/* 60 */{ 552 - 1, 1, D1CEL_FRAME_TYPE::Empty }, // 1469
/* 61 */{ 519 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // 1342
/* 62 */{ 509 - 1, 5, D1CEL_FRAME_TYPE::Square }, // 1315
/* 63 */{ 509 - 1, 3, D1CEL_FRAME_TYPE::Square }, // 1317
/* 64 */{ 509 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid }, // 1319

/* 65 */{ 510 - 1, 7, D1CEL_FRAME_TYPE::Empty }, // 1321
/* 66 */{ 510 - 1, 5, D1CEL_FRAME_TYPE::Empty }, // 1322
/* 67 */{ 551 - 1, 3, D1CEL_FRAME_TYPE::Square }, // 1466
/* 68 */{ 551 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid }, // 1468

/* 69 */{ 728 - 1, 9, D1CEL_FRAME_TYPE::Empty }, // 1916
/* 70 */{ 728 - 1, 7, D1CEL_FRAME_TYPE::Empty }, // 1917
/* 71 */{ 716 - 1, 13, D1CEL_FRAME_TYPE::TransparentSquare }, // 1855
/* 72 */{ 716 - 1, 11, D1CEL_FRAME_TYPE::Square }, // 1856

/* 73 */{ 910 - 1, 9, D1CEL_FRAME_TYPE::Empty }, // 2556
/* 74 */{ 910 - 1, 7, D1CEL_FRAME_TYPE::Empty }, // 2557
/* 75 */{ 930 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare }, // 2636
/* 76 */{ 930 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare }, // 2638

/* 77 */{ 537 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // 1429
/* 78 */{ 539 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // 1435
/* 79 */{ 529 - 1, 4, D1CEL_FRAME_TYPE::Square }, // 1394
/* 80 */{ 531 - 1, 4, D1CEL_FRAME_TYPE::Square }, // 1400

/* 81 */{ 478 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // 1230
/* 82 */{ 477 - 1, 1, D1CEL_FRAME_TYPE::Square }, // 1226
/* 83 */{ 480 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid }, // 1240
/* 84 */{ 479 - 1, 1, D1CEL_FRAME_TYPE::Empty }, // 1231
/* 85 */{ 477 - 1, 0, D1CEL_FRAME_TYPE::Square }, // 1225
/* 86 */{ 480 - 1, 0, D1CEL_FRAME_TYPE::LeftTrapezoid }, // 1239
    };

    constexpr unsigned blockSize = BLOCK_SIZE_TOWN;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // copy 724[0] (1903) to 721[2] (1894)
        if (i == 0) {
            const CelMicro &microDst = micros[0 + 11];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 724[1] (1903) to 719[2] (1875)
        if (i == 1) {
            const CelMicro &microDst = micros[1 + 12];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                    }
                }
            }
        }
        // copy 724[3] (1903) to 719[4] (1877) and 719[2] (1875)
        if (i == 2) {
            const CelMicro &microDst1 = micros[i + 10];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 11];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                        } else {
                            change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                        }
                    }
                }
            }
        }
        // copy 723[1] (1901) to 721[2] (1894) and 721[4] (1893)
        if (i == 3) {
            const CelMicro &microDst1 = micros[i + 7];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 8];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                        } else {
                            change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                        }
                    }
                }
            }
        }
        // copy 715[11] (1848) to 727[7] (1911)
        if (i == 4) {
            const CelMicro &microDst = micros[4 + 10];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 715[9] (1849) to 727[5] (1912)
        if (i == 5) {
            const CelMicro &microDst = micros[5 + 10];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 715[3] (1849) to 725[2] (1912) and 725[0] (1912)
        if (i == 8) {
            const CelMicro &microDst1 = micros[i + 9];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 10];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 9; x < 24; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                        } else {
                            change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                        }
                    }
                }
            }
        }
        // copy 715[1] (1849) to 725[0] (1912)
        if (i == 9) {
            const CelMicro &microDst = micros[9 + 9];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 9; x < 24; x++) {
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent() && y <= x / 2) {
                        change |= frameDst->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                    }
                }
            }
        }
        // copy 428[4] (1849) to 418[5] (1912)
        if (i == 19) {
            const CelMicro &microDst = micros[19 + 3];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (frameDst->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent()) {
                            change |= frameDst->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                        }
                    }
                }
            }
        }
        // copy 428[2] (1849) to 418[5] (1912) and 418[3] (1912)
        if (i == 20) {
            const CelMicro &microDst1 = micros[i + 2];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 3];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            if (frameDst1->getPixel(x, y + MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                            }
                        } else {
                            if (frameDst2->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                            }
                        }
                    }
                }
            }
        }
        // copy 428[0] (1849) to 418[3] (1912) and 418[1] (1912)
        if (i == 21) {
            const CelMicro &microDst1 = micros[i + 2];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 3];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            if (frameDst1->getPixel(x, y + MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                            }
                        } else {
                            if (frameDst2->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                            }
                        }
                    }
                }
            }
        }
        // copy 426[2] (1849) to 419[5] (1912)
        if (i == 25) {
            const CelMicro &microDst = micros[25 + 4];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (frameDst->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent()) {
                            change |= frameDst->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                        }
                    }
                }
            }
        }
        // copy 426[0] (1849) to 419[5] (1912) and 419[3] (1912)
        if (i == 26) {
            const CelMicro &microDst1 = micros[i + 3];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 4];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            if (frameDst1->getPixel(x, y + MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                            }
                        } else {
                            if (frameDst2->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                            }
                        }
                    }
                }
            }
        }
        // copy 428[1] (1849) to 419[3] (1912)
        if (i == 27) {
            const CelMicro &microDst = micros[27 + 3];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (frameDst->getPixel(x, y).isTransparent()) {
                            change |= frameDst->setPixel(x, y, pixel);
                        }
                    }
                }
            }
        }
        // copy 429[0] (1849) to 419[3] (1912) and 419[1] (1912)
        if (i == 28) {
            const CelMicro &microDst1 = micros[i + 2];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 3];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            if (frameDst1->getPixel(x, y + MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                            }
                        } else {
                            if (frameDst2->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                            }
                        }
                    }
                }
            }
        }
        // copy 911[9] (1849) to 931[5] (1912)
        // copy 911[7] (1849) to 931[3] (1912)
        // copy 911[5] (1849) to 931[1] (1912)
        if (i == 32 || i == 33 || i == 34) {
            const CelMicro &microDst = micros[i + 3];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 919[9] (1849) to 927[5] (1912)
        // copy 919[5] (1849) to 927[1] (1912)
        if (i == 40 || i == 41) {
            const CelMicro &microDst = micros[i + 2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 402[0] (1849) to 927[1] (1912)
        if (i == 38) {
            const CelMicro &microDst = micros[i + 5];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (frameDst->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent()) {
                            change |= frameDst->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                        }
                    }
                }
            }
        }
        // copy 954[2] (1849) to 919[7] (1912 -> 927[3]) and 927[1] (1912)
        if (i == 39) {
            const CelMicro &microDst1 = micros[i + 7];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 4];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                        } else {
                            change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                        }
                    }
                }
            }
        }
        // copy 918[9] (1849) to 926[5] (1912)
        if (i == 47) {
            const CelMicro &microDst = micros[47 + 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 927[0] (1849) to 918[5] (1912) and 929[1] (1912)
        if (i == 49) {
            const CelMicro &microDst1 = micros[i + 3];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 5];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            if (frameDst1->getPixel(x, y + MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                            }
                        } else {
                            if (frameDst2->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                            }
                        }
                    }
                }
            }
        }
        // copy 918[3] (1849) to 929[1] (1912)
        if (i == 50) {
            const CelMicro &microDst = micros[50 + 4];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 918[2] (1849) to 929[0] (1912)
        if (i == 51) {
            const CelMicro &microDst = micros[51 + 2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 918[8] (1849) to 926[4] (1912)
        if (i == 55) {
            const CelMicro &microDst = micros[55 + 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 928[4] (1849) to 920[8] (1912)
        if (i == 57) {
            const CelMicro &microDst = micros[57 + 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 551[0] (1849) to 509[5] (1912) and 509[3] (1912)
        if (i == 59) {
            const CelMicro &microDst1 = micros[i + 3];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 4];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            if (frameDst1->getPixel(x, y + MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                            }
                        } else {
                            if (frameDst2->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                            }
                        }
                    }
                }
            }
        }
        // copy 552[1] (1849) to 509[3] (1912)
        if (i == 60) {
            const CelMicro &microDst = micros[60 + 3];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 519[0] (1849) to 509[3] (1912) and 509[1] (1912)
        if (i == 61) {
            const CelMicro &microDst1 = micros[i + 2];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 3];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            if (frameDst1->getPixel(x, y + MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                            }
                        } else {
                            if (frameDst2->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                            }
                        }
                    }
                }
            }
        }
        // copy 510[7] (1849) to 551[3] (1912)
        // copy 510[5] (1849) to 551[1] (1912)
        if (i == 65 || i == 66) {
            const CelMicro &microDst = micros[i + 2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 728[9] (1849) to 716[13] (1912)
        // copy 728[7] (1849) to 716[11] (1912)
        if (i == 69 || i == 70) {
            const CelMicro &microDst = micros[i + 2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 910[9] (1849) to 930[5] (1912)
        // copy 910[7] (1849) to 930[3] (1912)
        if (i == 73 || i == 74) {
            const CelMicro &microDst = micros[i + 2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 537[0] (1849) to 529[4] (1912)
        // copy 539[0] (1849) to 531[4] (1912)
        if (i == 77 || i == 78) {
            const CelMicro &microDst = micros[i + 2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frameDst->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // copy 478[0] (1849) to 477[1] (1912) and 480[1] (1912)
        if (i == 81) {
            const CelMicro &microDst1 = micros[i + 1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 2];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                        } else {
                            change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                        }
                    }
                }
            }
        }
        // copy 479[1] (1849) to 477[0] (1912) and 480[0] (1912)
        if (i == 84) {
            const CelMicro &microDst1 = micros[i + 1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 2];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (y < MICRO_HEIGHT / 2) {
                            change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                        } else {
                            change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                        }
                    }
                }
            }
        }
        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }
    return true;
}

void D1Tileset::patchTownChop(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{ 180 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // 1854
/*  1 */{ 180 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854

/*  2 */{ 224 - 1, 0, D1CEL_FRAME_TYPE::LeftTrapezoid }, // 1854
/*  3 */{ 224 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854
/*  4 */{ 225 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854

/*  5 */{ 362 - 1, 9, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854
/*  6 */{ 383 - 1, 3, D1CEL_FRAME_TYPE::Square }, // 1854

/*  7 */{ 632 - 1, 11, D1CEL_FRAME_TYPE::Square }, // 1854
/*  8 */{ 632 - 1, 13, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854
/*  9 */{ 631 - 1, 11, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854

/* 10 */{ 832 - 1, 10, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854

/* 11 */{ 834 - 1, 10, D1CEL_FRAME_TYPE::Square }, // 1854
/* 12 */{ 834 - 1, 12, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854
/* 13 */{ 828 - 1, 12, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854

/* 14 */{ 864 - 1, 12, D1CEL_FRAME_TYPE::Square }, // 1854
/* 15 */{ 864 - 1, 14, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854

/* 16 */{ 926 - 1, 12, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854
/* 17 */{ 926 - 1, 13, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854

/* 18 */{ 944 - 1, 6, D1CEL_FRAME_TYPE::Square }, // 1854
/* 19 */{ 944 - 1, 8, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854
/* 20 */{ 942 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854

/* 21 */{ 955 - 1, 13, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854
/* 22 */{ 950 - 1, 13, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854
/* 23 */{ 951 - 1, 13, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854
/* 24 */{ 946 - 1, 13, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854
/* 25 */{ 947 - 1, 13, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854
/* 26 */{ 940 - 1, 12, D1CEL_FRAME_TYPE::TransparentSquare }, // 1854

/* 27 */{ 383 - 1, 5, D1CEL_FRAME_TYPE::Square }, // 1854
/* 28 */{ 383 - 1, 7, D1CEL_FRAME_TYPE::Square }, // 1854
    };

    constexpr unsigned blockSize = BLOCK_SIZE_TOWN;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return;
        }
        bool change = false;
        // fix bad artifacts
        if (i == 13) { // 828[12] (1854)
            change |= frame->setPixel(30, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 0, D1GfxPixel::transparentPixel());
        }
        if (i == 1) { // 180[3] (1854)
            change |= frame->setPixel(1, 23, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(1, 24, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(1, 25, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(1, 26, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(1, 27, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(0, 30, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(0, 31, D1GfxPixel::transparentPixel());
        }
        if (i == 2) { // 224[0] (1854)
            change |= frame->setPixel(0, 0, frame->getPixel(2, 0));
        }
        if (i == 4) { // + 225[2] (1854)
            change |= frame->setPixel(31, 17, frame->getPixel(29, 20));
            change |= frame->setPixel(31, 18, frame->getPixel(29, 21));
            change |= frame->setPixel(30, 18, frame->getPixel(28, 21));
        }
        if (i == 5) { // 362[9] (1854)
            change |= frame->setPixel(12, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(15, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(11, 1, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(13, 1, D1GfxPixel::transparentPixel());
        }
        if (i == 6) { // 383[3] (1854)
            change |= frame->setPixel(0, 3, frame->getPixel(5, 18));
        }
        if (i == 7) { // 632[11] (1854)
            int i = 7;
            change |= frame->setPixel(0, 0, frame->getPixel(7, 2));
            change |= frame->setPixel(1, 0, frame->getPixel(8, 2));
        }
        if (i == 8) { // + 632[13] (1854)
            change |= frame->setPixel(8, 30, frame->getPixel(10, 30));
            change |= frame->setPixel(4, 31, frame->getPixel(6, 31));
        }
        if (i == 10) { // 832[10] (1854)
            change |= frame->setPixel(22, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(21, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(22, 1, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(23, 2, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(24, 3, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(23, 3, frame->getPixel(20, 1));
            change |= frame->setPixel(22, 2, frame->getPixel(20, 0));
            change |= frame->setPixel(20, 0, frame->getPixel(20, 1));
            change |= frame->setPixel(19, 0, frame->getPixel(17, 0));
        }
        if (i == 11) { // 834[10] (1854)
            change |= frame->setPixel(0, 0, frame->getPixel(1, 1));
            change |= frame->setPixel(1, 0, frame->getPixel(2, 1));
        }
        if (i == 12) { // + 834[12] (1854)
            change |= frame->setPixel(3, 31, frame->getPixel(5, 31));
            change |= frame->setPixel(4, 31, frame->getPixel(6, 31));

            change |= frame->setPixel(6, 30, frame->getPixel(8, 30));
        }
        if (i == 13) { // + 828[12] (1854)
            change |= frame->setPixel(29, 17, frame->getPixel(28, 18));
            change |= frame->setPixel(30, 17, frame->getPixel(29, 18));

            change |= frame->setPixel(27, 18, frame->getPixel(30, 18));
            change |= frame->setPixel(24, 19, frame->getPixel(30, 18));
        }
        if (i == 14) { // 864[12] (1854)
            change |= frame->setPixel(0, 0, frame->getPixel(2, 0));
        }
        if (i == 15) { // + 864[14] (1854)
            change |= frame->setPixel(4, 30, frame->getPixel(5, 30));
            change |= frame->setPixel(2, 31, frame->getPixel(5, 30));
        }
        if (i == 16) { // . 926[12] (1854)
            change |= frame->setPixel(26, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(27, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(28, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(26, 1, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(25, 2, D1GfxPixel::transparentPixel());
        }
        if (i == 17) { // . 926[13] (1854)
            change |= frame->setPixel(7, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(6, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(14, 3, D1GfxPixel::transparentPixel());
        }
        if (i == 18) { // 944[6] (1854)
            change |= frame->setPixel(31, 0, frame->getPixel(29, 0));
        }
        if (i == 19) { // + 944[8] (1854)
            change |= frame->setPixel(30, 31, frame->getPixel(28, 31));
            change |= frame->setPixel(29, 30, frame->getPixel(28, 31));
        }
        if (i == 20) { // + 942[6] (1854)
            change |= frame->setPixel(0, 17, frame->getPixel(0, 18));
        }
        if (i == 21) { // . 955[13] (1854)
            change |= frame->setPixel(31, 14, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(30, 14, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(30, 13, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(29, 13, D1GfxPixel::transparentPixel());
        }
        if (i == 22) { // . 950[13] (1854)
            change |= frame->setPixel(3, 0, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(31, 14, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(29, 13, D1GfxPixel::transparentPixel());
        }
        if (i == 23) { // . 951[13] (1854)
            change |= frame->setPixel(3, 0, D1GfxPixel::transparentPixel());
        }
        if (i == 24) { // . 946[13] (1854)
            change |= frame->setPixel(2, 0, D1GfxPixel::transparentPixel());
        }
        if (i == 25) { // . 947[13] (1854)
            change |= frame->setPixel(3, 0, D1GfxPixel::transparentPixel());
        }
        if (i == 26) { // . 940[12] (1854)
            change |= frame->setPixel(0, 14, D1GfxPixel::transparentPixel());
        }
        if (i == 27) { // 383[5] (1854) <- 180[1]
            const CelMicro &microSrc = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            /*if (frameSrc == nullptr) {
                return;
            }*/
            D1GfxPixel pixel = frameSrc->getPixel(4, 8);
            change |= frame->setPixel(0, 0, pixel);
            change |= frame->setPixel(0, 9, pixel);
            change |= frame->setPixel(0, 10, pixel);
            change |= frame->setPixel(0, 20, pixel);
            change |= frame->setPixel(0, 30, pixel);
        }
        if (i == 28) { // 383[7] (1854) <- 180[1]
            const CelMicro &microSrc = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            /*if (frameSrc == nullptr) {
                return;
            }*/
            D1GfxPixel pixel = frameSrc->getPixel(4, 7);
            change |= frame->setPixel(0, 6, pixel);
            change |= frame->setPixel(0, 12, pixel);
            change |= frame->setPixel(0, 20, pixel);
            change |= frame->setPixel(0, 26, pixel);
        }
        if (/*micro.res_encoding != D1CEL_FRAME_TYPE::Empty &&*/ frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }
}

void D1Tileset::cleanupTown(std::set<unsigned> &deletedFrames, bool silent)
{
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
    // fix bad artifacts
    Blk2Mcr(233, 6);
    Blk2Mcr(828, 13);
    Blk2Mcr(1018, 2);
    // useless black (hidden) micros
    Blk2Mcr(426, 1);
    Blk2Mcr(427, 0);
    Blk2Mcr(427, 1);
    Blk2Mcr(429, 1);
    Blk2Mcr(494, 0);
    Blk2Mcr(494, 1);
    Blk2Mcr(550, 1);
    HideMcr(587, 0);
    Blk2Mcr(624, 1);
    Blk2Mcr(626, 1);
    HideMcr(926, 0);
    HideMcr(926, 1);
    HideMcr(928, 0);
    HideMcr(928, 1);
    // Blk2Mcr(1143, 0);
    // Blk2Mcr(1145, 0);
    // Blk2Mcr(1145, 1);
    // Blk2Mcr(1146, 0);
    // Blk2Mcr(1153, 0);
    // Blk2Mcr(1155, 1);
    // Blk2Mcr(1156, 0);
    // Blk2Mcr(1169, 1);
    Blk2Mcr(1172, 1);
    Blk2Mcr(1176, 1);
    Blk2Mcr(1199, 1);
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
    // ReplaceMcr(237, 0, 402, 0);
    // ReplaceMcr(237, 1, 402, 1);
    // patch subtiles around the pot of Adria to prevent graphical glitch when a player passes it
    this->patchTownPot(553, 554, silent);
    // patch subtiles of the cathedral to fix graphical glitch
    this->patchTownCathedral(805, 806, 807, silent);
    // patch subtiles to reduce its memory footprint
    if (this->patchTownFloor(silent)) {
        // use the micros created by patchTownFloor
        MoveMcr(732, 8, 731, 9);
        Blk2Mcr(974, 2);
        Blk2Mcr(1030, 2);
        ReplaceMcr(220, 0, 17, 0);
        SetMcr(221, 2, 220, 1);
        SetMcr(220, 1, 17, 1);
        ReplaceMcr(218, 1, 25, 1);
        SetMcr(219, 3, 218, 0);
        SetMcr(218, 0, 25, 0);
        ReplaceMcr(1166, 1, 281, 1);
        SetMcr(1167, 3, 1166, 0);
        SetMcr(1166, 0, 19, 0);
        ReplaceMcr(962, 0, 14, 0);
        SetMcr(963, 2, 962, 1);
        SetMcr(962, 1, 14, 1);
    }
    // merge subtiles of the 'entrances'
    if (this->patchTownDoor(silent)) {
        // use micros created by patchTownDoorCel
        Blk2Mcr(724, 0);
        Blk2Mcr(724, 1);
        Blk2Mcr(724, 3);
        Blk2Mcr(723, 1);
        Blk2Mcr(715, 11);
        Blk2Mcr(715, 9);
        Blk2Mcr(715, 3);

        Blk2Mcr(428, 4);
        Blk2Mcr(428, 2);
        Blk2Mcr(428, 0);
        Blk2Mcr(428, 1);
        Blk2Mcr(426, 2);
        Blk2Mcr(426, 0);
        Blk2Mcr(429, 0);

        Blk2Mcr(911, 9);
        Blk2Mcr(911, 7);
        Blk2Mcr(911, 5);
        Blk2Mcr(919, 9);
        Blk2Mcr(919, 5);

        // Blk2Mcr(402, 0);
        Blk2Mcr(954, 2);
        Blk2Mcr(956, 2);
        Blk2Mcr(918, 9);
        Blk2Mcr(927, 0);
        Blk2Mcr(918, 3);
        Blk2Mcr(918, 2);
        Blk2Mcr(918, 8);
        Blk2Mcr(928, 4);
        Blk2Mcr(237, 0);
        Blk2Mcr(237, 1);

        Blk2Mcr(551, 0);
        Blk2Mcr(552, 1);
        Blk2Mcr(519, 0);
        Blk2Mcr(510, 7);
        Blk2Mcr(510, 5);

        Blk2Mcr(728, 9);
        Blk2Mcr(728, 7);

        Blk2Mcr(910, 9);
        Blk2Mcr(910, 7);

        Blk2Mcr(537, 0);
        Blk2Mcr(539, 0);
        Blk2Mcr(478, 0);
        Blk2Mcr(479, 1);

        MoveMcr(927, 3, 919, 7);
        MoveMcr(929, 2, 918, 4);
        MoveMcr(929, 3, 918, 5);
        MoveMcr(929, 4, 918, 6);
        MoveMcr(929, 5, 918, 7);

        MoveMcr(551, 5, 510, 9);

        MoveMcr(529, 6, 537, 2);
        MoveMcr(529, 8, 537, 4);
        MoveMcr(529, 10, 537, 6);
        MoveMcr(529, 12, 537, 8);

        MoveMcr(480, 2, 477, 0);
        MoveMcr(480, 3, 477, 1);
        MoveMcr(480, 4, 477, 2);
        MoveMcr(480, 5, 477, 3);
        MoveMcr(480, 6, 477, 4);
        MoveMcr(480, 7, 477, 5);
        MoveMcr(480, 8, 477, 6);
        MoveMcr(480, 9, 477, 7);
        MoveMcr(480, 10, 477, 8);
    }
    // patch subtiles to reduce minor protrusions
    this->patchTownChop(silent);
    // eliminate micros after patchTownChop
    {
        Blk2Mcr(362, 11);
        Blk2Mcr(832, 12);
        Blk2Mcr(926, 14);
        Blk2Mcr(926, 15);
        Blk2Mcr(946, 15);
        Blk2Mcr(947, 15);
        Blk2Mcr(950, 15);
        Blk2Mcr(951, 15);
    }
    // better shadows
    ReplaceMcr(555, 0, 493, 0); // TODO: reduce edges on the right
    ReplaceMcr(728, 0, 872, 0);
    // adjust the shadow of the tree beside the church
    ReplaceMcr(767, 0, 117, 0);
    ReplaceMcr(767, 1, 117, 1);
    ReplaceMcr(768, 0, 158, 0);
    ReplaceMcr(768, 1, 159, 1);
    // reuse subtiles
    ReplaceMcr(129, 1, 2, 1);  // lost details
    ReplaceMcr(160, 0, 11, 0); // lost details
    ReplaceMcr(160, 1, 12, 1); // lost details
    ReplaceMcr(165, 0, 2, 0);
    ReplaceMcr(169, 0, 129, 0);
    ReplaceMcr(169, 1, 2, 1);
    ReplaceMcr(177, 0, 1, 0);
    ReplaceMcr(178, 1, 118, 1);
    ReplaceMcr(181, 0, 129, 0);
    ReplaceMcr(181, 1, 2, 1);
    ReplaceMcr(188, 0, 3, 0);  // lost details
    ReplaceMcr(198, 1, 1, 1);  // lost details
    ReplaceMcr(281, 0, 19, 0); // lost details
    ReplaceMcr(319, 0, 7, 0);  // lost details
    ReplaceMcr(414, 1, 9, 1);  // lost details
    ReplaceMcr(443, 1, 379, 1);
    ReplaceMcr(471, 0, 3, 0);  // lost details
    ReplaceMcr(472, 0, 5, 0);  // lost details
    ReplaceMcr(475, 0, 7, 0);  // lost details
    ReplaceMcr(476, 0, 4, 0);  // lost details
    ReplaceMcr(484, 1, 4, 1);  // lost details
    ReplaceMcr(486, 1, 20, 1); // lost details
    ReplaceMcr(488, 1, 14, 1); // lost details
    ReplaceMcr(493, 1, 3, 1);  // lost details
    ReplaceMcr(496, 1, 4, 1);  // lost details
    ReplaceMcr(507, 1, 61, 1); // lost details
    ReplaceMcr(512, 0, 3, 0);  // lost details
    ReplaceMcr(532, 1, 14, 1); // lost details
    ReplaceMcr(556, 0, 3, 0);  // lost details
    ReplaceMcr(559, 0, 59, 0); // lost details
    ReplaceMcr(559, 1, 59, 1); // lost details
    ReplaceMcr(563, 0, 2, 0);  // lost details
    ReplaceMcr(569, 1, 3, 1);  // lost details
    ReplaceMcr(592, 0, 11, 0); // lost details
    ReplaceMcr(611, 1, 9, 1);  // lost details
    ReplaceMcr(612, 0, 3, 0);  // lost details
    ReplaceMcr(614, 1, 14, 1); // lost details
    ReplaceMcr(619, 1, 13, 1); // lost details
    ReplaceMcr(624, 0, 1, 0);  // lost details
    ReplaceMcr(640, 1, 9, 1);  // lost details
    ReplaceMcr(653, 0, 1, 0);  // lost details
    ReplaceMcr(660, 0, 10, 0); // lost details
    ReplaceMcr(663, 1, 7, 1);  // lost details
    ReplaceMcr(683, 1, 731, 1);
    ReplaceMcr(685, 0, 15, 0); // lost details
    // ReplaceMcr(690, 1, 2, 1); // lost details
    ReplaceMcr(694, 0, 17, 0);
    ReplaceMcr(774, 1, 16, 1); // lost details
    ReplaceMcr(789, 1, 10, 1); // lost details
    ReplaceMcr(795, 1, 13, 1); // lost details
    ReplaceMcr(850, 1, 9, 1);  // lost details
    ReplaceMcr(826, 12, 824, 12);
    ReplaceMcr(892, 0, 92, 0);    // lost details
    ReplaceMcr(871, 11, 824, 12); // lost details
    ReplaceMcr(908, 0, 3, 0);     // lost details
    ReplaceMcr(905, 1, 8, 1);     // lost details
    ReplaceMcr(943, 1, 7, 1);     // lost details
    // ReplaceMcr(955, 15, 950, 15); // lost details
    ReplaceMcr(902, 1, 5, 1); // lost details
    // ReplaceMcr(962, 0, 10, 0);    // lost details
    ReplaceMcr(986, 0, 3, 0);  // lost details
    ReplaceMcr(1011, 0, 7, 0); // lost details
    ReplaceMcr(1028, 1, 3, 1); // lost details
    // ReplaceMcr(1030, 2, 974, 2);
    ReplaceMcr(1034, 1, 4, 1); // lost details
    ReplaceMcr(1042, 0, 8, 0); // lost details
    ReplaceMcr(1043, 1, 5, 1); // lost details
    ReplaceMcr(1119, 0, 9, 0); // lost details
    ReplaceMcr(1159, 1, 291, 1);
    // ReplaceMcr(1166, 1, 281, 1);
    ReplaceMcr(1180, 1, 2, 1);  // lost details
    ReplaceMcr(1187, 0, 29, 0); // lost details
    ReplaceMcr(1215, 0, 1207, 0);
    ReplaceMcr(1215, 9, 1207, 9);
    // ReplaceMcr(871, 11, 358, 12);
    // ReplaceMcr(947, 15, 946, 15);
    ReplaceMcr(1175, 4, 1171, 4);
    // ReplaceMcr(1218, 3, 1211, 3);
    // ReplaceMcr(1218, 5, 1211, 5);
    // eliminate micros of unused subtiles
    // Blk2Mcr(178, 538, 1133, 1134 ..); 
    Blk2Mcr(107, 1);
    Blk2Mcr(108, 1);
    Blk2Mcr(110, 0);
    Blk2Mcr(113, 0);
    Blk2Mcr(235, 0);
    Blk2Mcr(239, 0);
    Blk2Mcr(240, 0);
    Blk2Mcr(243, 0);
    Blk2Mcr(244, 0);
    Blk2Mcr(468, 0);
    Blk2Mcr(1023, 0);
    Blk2Mcr(1132, 2);
    Blk2Mcr(1132, 3);
    Blk2Mcr(1132, 4);
    Blk2Mcr(1132, 5);
    Blk2Mcr(1139, 0);
    Blk2Mcr(1139, 1);
    Blk2Mcr(1139, 2);
    Blk2Mcr(1139, 3);
    Blk2Mcr(1139, 4);
    Blk2Mcr(1139, 5);
    Blk2Mcr(1139, 6);
    Blk2Mcr(1152, 0);
    Blk2Mcr(1160, 1);
    Blk2Mcr(1162, 1);
    Blk2Mcr(1164, 1);
    Blk2Mcr(1168, 0);
    Blk2Mcr(1196, 0);
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
    Blk2Mcr(1239, 1);
    Blk2Mcr(1254, 0);
    Blk2Mcr(1258, 0);
    Blk2Mcr(1258, 1);
    const int unusedSubtiles[] = {
        40, 43, 49, 50, 51, 52, 66, 67, 69, 70, 71, 72, 73, 74, 75, 76, 77, 79, 80, 81, 83, 85, 86, 89, 90, 91, 93, 94, 95, 97, 99, 100, 101, 102, 103, 122, 123, 124, 136, 137, 140, 141, 142, 145, 147, 150, 151, 155, 161, 163, 164, 166, 167, 171, 176, 179, 183, 190, 191, 193, 194, 195, 196, 197, 199, 204, 205, 206, 208, 209, 228, 230, 236, 238, 241, 242, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 256, 278, 280, 291, 298, 299, 304, 305, 314, 316, 318, 320, 321, 328, 329, 335, 336, 337, 342, 350, 351, 352, 353, 354, 355, 356, 357, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 380, 392, 411, 413, 415, 417, 442, 444, 446, 447, 448, 449, 450, 451, 452, 453, 455, 456, 457, 460, 461, 462, 464, 467, 490, 491, 492, 497, 499, 500, 505, 506, 508, 534, 536, 544, 546, 548, 549, 558, 560, 565, 566, 567, 568, 570, 572, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582, 583, 584, 585, 589, 591, 594, 595, 597, 598, 599, 600, 602, 609, 615, 622, 625, 648, 650, 654, 662, 664, 666, 667, 679, 680, 681, 682, 688, 690, 691, 693, 695, 696, 698, 699, 700, 701, 702, 703, 705, 730, 735, 737, 741, 742, 747, 748, 749, 750, 751, 752, 753, 756, 758, 760, 765, 766, 769, 790, 792, 796, 798, 800, 801, 802, 804, 851, 857, 859, 860, 861, 863, 865, 876, 877, 878, 879, 880, 881, 882, 883, 884, 885, 887, 888, 889, 890, 891, 893, 894, 895, 896, 897, 901, 903, 937, 960, 961, 964, 965, 967, 968, 969, 972, 973, 976, 977, 979, 980, 981, 984, 985, 988, 989, 991, 992, 993, 996, 997, 1000, 1001, 1003, 1004, 1005, 1008, 1009, 1012, 1013, 1016, 1017, 1019, 1020, 1021, 1022, 1024, 1029, 1032, 1033, 1035, 1036, 1037, 1039, 1040, 1041, 1044, 1045, 1047, 1048, 1049, 1050, 1051, 1064, 1066, 1067, 1068, 1069, 1070, 1071, 1072, 1073, 1075, 1076, 1077, 1078, 1079, 1080, 1081, 1084, 1085, 1088, 1092, 1093, 1100, 1101, 1102, 1103, 1104, 1105, 1106, 1107, 1108, 1109, 1110, 1111, 1112, 1113, 1114, 1115, 1116, 1117, 1118, 1121, 1123, 1135, 1136, 1137, 1138, 1140, 1141, 1142, 1143, 1144, 1145, 1146, 1147, 1148, 1149, 1150, 1151, 1153, 1154, 1155, 1156, 1157, 1158, 1159, 1161, 1163, 1165, 1169, 1170, 1184, 1186, 1189, 1190, 1193, 1194, 1198, 1199, 1200, 1201, 1202, 1218, 1221, 1222, 1223, 1224, 1225, 1226, 1227, 1228, 1229, 1230, 1231, 1232, 1233, 1234, 1235, 1236, 1237, 1256
    };
    for (int n = 0; n < lengthof(unusedSubtiles); n++) {
        for (int i = 0; i < BLOCK_SIZE_TOWN; i++) {
            Blk2Mcr(unusedSubtiles[n], i);
        }
    }
    if (isHellfireTown) {
        // reuse subtiles
        ReplaceMcr(1269, 0, 302, 0);
        ReplaceMcr(1281, 0, 290, 0);
        ReplaceMcr(1273, 1, 2, 1);
        ReplaceMcr(1276, 1, 11, 1);
        ReplaceMcr(1265, 0, 1297, 0);
        ReplaceMcr(1314, 0, 293, 0);
        ReplaceMcr(1321, 0, 6, 0);
        ReplaceMcr(1304, 1, 4, 1);
        // eliminate micros of unused subtiles
        Blk2Mcr(1266, 0);
        Blk2Mcr(1267, 1);
        Blk2Mcr(1295, 1);
        Blk2Mcr(1298, 1);
        Blk2Mcr(1360, 0);
        Blk2Mcr(1370, 0);
        Blk2Mcr(1376, 0);
        const int unusedSubtilesHellfire[] = {
            1260, 1268, 1274, 1283, 1284, 1291, 1292, 1293, 1322, 1340, 1341, 1342, 1343, 1344, 1345, 1346, 1347, 1348, 1349, 1350, 1351, 1352, 1353, 1354, 1355, 1356, 1357, 1358, 1359, 1361, 1362, 1363, 1364, 1365, 1366, 1367, 1368, 1369, 1371, 1372, 1373, 1374, 1375, 1377, 1378, 1379
        };
        for (int n = 0; n < lengthof(unusedSubtilesHellfire); n++) {
            for (int i = 0; i < BLOCK_SIZE_TOWN; i++) {
                Blk2Mcr(unusedSubtilesHellfire[n], i);
            }
        }

        const int unusedPartialSubtilesHellfire[] = {
            1266, 1267, 1295, 1298, 1360, 1370, 1376
        };
        int n1 = lengthof(unusedSubtilesHellfire) - 1;
        int n2 = lengthof(unusedPartialSubtilesHellfire) - 1;
        while (n1 >= 0 || n2 >= 0) {
            int sn1 = n1 >= 0 ? unusedSubtilesHellfire[n1] : -1;
            int sn2 = n2 >= 0 ? unusedPartialSubtilesHellfire[n2] : -1;
            int subtileRef;
            if (sn1 > sn2) {
                subtileRef = sn1;
                n1--;
            } else {
                subtileRef = sn2;
                n2--;
            }
            this->removeSubtile(subtileRef - 1, 0);
            if (!silent) {
                dProgress() << QApplication::tr("Removed Subtile %1.").arg(subtileRef);
            }
        }
    }
    const int unusedPartialSubtiles[] = {
        107, 108, 110, 113, 178, 235, 239, 240, 243, 244, 468, 538, 1023, 1132, 1133, 1134, 1139, 1152, 1160, 1162, 1164, 1168, 1196, 1214, 1216, 1239, 1254, 1258
    };
    int n1 = lengthof(unusedSubtiles) - 1;
    int n2 = lengthof(unusedPartialSubtiles) - 1;
    while (n1 >= 0 || n2 >= 0) {
        int sn1 = n1 >= 0 ? unusedSubtiles[n1] : -1;
        int sn2 = n2 >= 0 ? unusedPartialSubtiles[n2] : -1;
        int subtileRef;
        if (sn1 > sn2) {
            subtileRef = sn1;
            n1--;
        } else {
            subtileRef = sn2;
            n2--;
        }
        this->removeSubtile(subtileRef - 1, 0);
        if (!silent) {
            dProgress() << QApplication::tr("Removed Subtile %1.").arg(subtileRef);
        }
    }
}

void D1Tileset::patchCathedralSpec(bool silent)
{
    constexpr int FRAME_WIDTH = 64;
    constexpr int FRAME_HEIGHT = 160;

    for (int i = 0; i < this->cls->getFrameCount(); i++) {
        D1GfxFrame *frame = this->cls->getFrame(i);
        if (frame->getWidth() != FRAME_WIDTH || frame->getHeight() != FRAME_HEIGHT) {
            dProgressErr() << QApplication::tr("Framesize of the Cathedal's Special-Cels does not match. (%1:%2 expected %3:%4. Index %5.)").arg(frame->getWidth()).arg(frame->getHeight()).arg(FRAME_WIDTH).arg(FRAME_HEIGHT).arg(i + 1);
            return;
        }

        bool change = false;
        // eliminate unnecessary pixels on top
        for (int y = 0; y < 47; y++) {
            for (int x = 0; x < FRAME_WIDTH; x++) {
                change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
            }
        }
        if (i == 7 - 1) {
            for (int y = 71; y < 82; y++) {
                for (int x = 28; x < 44; x++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && (color == 14 || color == 29 || color == 30 || color == 46 || color == 47 || y > 112 - x)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            change |= frame->setPixel(38, 74, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(35, 77, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(28, 81, D1GfxPixel::colorPixel(22));
            change |= frame->setPixel(29, 80, D1GfxPixel::colorPixel(10));
            change |= frame->setPixel(30, 79, D1GfxPixel::colorPixel(10));
            change |= frame->setPixel(31, 78, D1GfxPixel::colorPixel(23));
        }
        if (i == 8 - 1) {
            for (int y = 71; y < 82; y++) {
                for (int x = 19; x < 35; x++) {
                    if (x == 34 && y == 71) {
                        continue;
                    }
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && (color == 14 || color == 29 || color == 30 || color == 46 || color == 47)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            // change |= frame->setPixel(19, 70, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(32, 78, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(33, 79, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(34, 80, D1GfxPixel::colorPixel(28));
        }
        // eliminate pixels of the unused frames
        if (i == 3 - 1 || i == 6 - 1) {
            for (int y = 0; y < FRAME_HEIGHT; y++) {
                for (int x = 0; x < FRAME_WIDTH; x++) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }

        if (change && !silent) {
            this->cls->setModified();
            dProgress() << QApplication::tr("Special-Frame %1 is modified.").arg(i + 1);
        }
    }
}

bool D1Tileset::patchCathedralFloor(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{ 137 - 1, 5, D1CEL_FRAME_TYPE::Square },        // change type
/*  1 */{ 286 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // change type
/*  2 */{ 408 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },  // change type
/*  3 */{ 248 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },  // change type

/*  4 */{ 392 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // mask door
/*  5 */{ 392 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/*  6 */{ 394 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*  7 */{ 394 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },

/*  8 */{ 108 - 1, 1, D1CEL_FRAME_TYPE::Empty },            // used to block subsequent calls
/*  9 */{ 106 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 10 */{ 109 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 11 */{ 106 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },

/* 12 */{ 178 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },

/* 13 */{ 152 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare }, // blocks subsequent calls

/* 14 */{ 23 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 15 */{ 270 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },

/* 16 */{ 407 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // mask door

/* 17 */{ 156 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 18 */{ 160 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 19 */{ 152 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 20 */{ 159 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },

/* 21 */{ 163 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 22 */{ 137 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },

/* 23 */{ 176 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 24 */{ 171 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 25 */{ 153 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },

/* 26 */{ 231 - 1, 4, D1CEL_FRAME_TYPE::Empty },
/* 27 */{ 417 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 28 */{ 418 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 29 */{ 417 - 1, 2, D1CEL_FRAME_TYPE::Square },
    };

    constexpr unsigned blockSize = BLOCK_SIZE_L1;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // mask 392[0]
        if (i == 4) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x < 17 && (x != 16 || y != 24)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 392[2]
        if (i == 5) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x < 14 || (x == 14 && y > 1) || (x == 15 && y > 2) || (x == 16 && y > 4)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 394[1]
        // mask 394[3]
        if (i == 6 || i == 7) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x > 15 && (i == 6 || (x > 18 || y > 9 || (x == 17 && y > 5) || (x == 18 && y != 0)))) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // copy 108[1] to 106[0] and 109[0]
        if (i == 8) {
            const CelMicro &microDst1 = micros[i + 1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 2];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && color != 46) {
                        if (y < MICRO_HEIGHT / 2) {
                            change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel); // 106[0]
                        } else {
                            change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel); // 109[0]
                        }
                    }
                }
            }
        }
        // mask 106[0]
        // mask 109[0]
        // mask 106[1]
        if (i >= 9 && i < 12) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    quint8 color = frame->getPixel(x, y).getPaletteIndex();
                    if (color == 46 && (y > 8 || i != 11) && (x > 22 || y < 22 || i != 10)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // move pixels of 152[5] down to enable reuse as 153[6]
        if (i == 13) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // remove shadow from 270[0] using 23[0]
        if (i == 15) {
            const CelMicro &microSrc = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 22; x < 29; x++) {
                for (int y = 5; y < 12; y++) {
                    if (x == 28 && y == 11) {
                        continue;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 23[0]
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // mask 407[0]
        if (i == 16) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x < 17) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 137[0]
        if (i == 22) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x > 16) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        if (i == 27) { // 417[4] - add missing pixels after DRLP_L1_PatchSpec using 231[4]
            const CelMicro &microSrc = micros[i - 1]; // 231[4]
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            if (frameSrc == nullptr) {
                return false;
            }
            for (int x = 0; x < 12; x++) {
                for (int y = 23; y < 31; y++) {
                    if (y < 29 - x) {
                        continue;
                    }
                    if (x > 1 && y < 30 - x) {
                        continue;
                    }
                    if (x == 6 && y == 24) {
                        continue;
                    }
                    if (y == 23 && (x == 7 || x == 8)) {
                        continue;
                    }
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x, y));
                }
            }

            // fix glitch
            change |= frame->setPixel(5, 28, D1GfxPixel::colorPixel(29));
            change |= frame->setPixel(6, 28, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(7, 28, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(3, 29, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(4, 29, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(5, 29, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(6, 29, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(15, 27, D1GfxPixel::colorPixel(6));

            // fix shadow
            change |= frame->setPixel(9, 26, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(10, 26, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(12, 26, D1GfxPixel::colorPixel(11));
            change |= frame->setPixel(13, 26, D1GfxPixel::colorPixel(12));
            change |= frame->setPixel(14, 26, D1GfxPixel::colorPixel(28));

            change |= frame->setPixel(7, 29, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(8, 29, D1GfxPixel::colorPixel(42));
            change |= frame->setPixel(9, 29, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(5, 30, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(6, 30, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(7, 30, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(8, 30, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(9, 30, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(10, 30, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(11, 30, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(12, 30, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(4, 31, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(5, 31, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(6, 31, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(7, 31, D1GfxPixel::colorPixel(12));
            change |= frame->setPixel(8, 31, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(9, 31, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(10, 31, D1GfxPixel::colorPixel(44));
            change |= frame->setPixel(11, 31, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(12, 31, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(13, 31, D1GfxPixel::colorPixel(12));
            change |= frame->setPixel(14, 31, D1GfxPixel::colorPixel(26));
        }
        if (i == 28) { // 418[4] - add missing pixels after DRLP_L1_PatchSpec
            change |= frame->setPixel(31, 15, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(30, 16, D1GfxPixel::colorPixel(30));
            change |= frame->setPixel(29, 17, D1GfxPixel::colorPixel(30));

            // fix shadow
            change |= frame->setPixel(31, 23, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(30, 24, D1GfxPixel::colorPixel(29));
            change |= frame->setPixel(31, 24, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(30, 25, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(31, 25, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(30, 26, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(31, 26, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(28, 27, D1GfxPixel::colorPixel(29));
            change |= frame->setPixel(29, 27, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(30, 27, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(27, 28, D1GfxPixel::colorPixel(29));
            change |= frame->setPixel(28, 28, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(29, 28, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(27, 29, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(28, 29, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(26, 30, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(27, 30, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(28, 30, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(29, 30, D1GfxPixel::colorPixel(28));
        }
        if (i == 29) { // 417[2] - fix shadow
            change |= frame->setPixel(0, 9, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(0, 8, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(1, 8, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(2, 8, D1GfxPixel::colorPixel(26));

            change |= frame->setPixel(0, 7, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(1, 7, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(2, 7, D1GfxPixel::colorPixel(12));
            change |= frame->setPixel(4, 7, D1GfxPixel::colorPixel(27));

            change |= frame->setPixel(0, 6, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(1, 6, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(2, 6, D1GfxPixel::colorPixel(25));
            change |= frame->setPixel(3, 6, D1GfxPixel::colorPixel(25));
            change |= frame->setPixel(4, 6, D1GfxPixel::colorPixel(11));

            change |= frame->setPixel(0, 5, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(1, 5, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(2, 5, D1GfxPixel::colorPixel(12));
            change |= frame->setPixel(3, 5, D1GfxPixel::colorPixel(10));
            change |= frame->setPixel(4, 5, D1GfxPixel::colorPixel(10));
            change |= frame->setPixel(5, 5, D1GfxPixel::colorPixel(26));

            change |= frame->setPixel(1, 4, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(2, 4, D1GfxPixel::colorPixel(11));
            change |= frame->setPixel(3, 4, D1GfxPixel::colorPixel(10));
            change |= frame->setPixel(4, 4, D1GfxPixel::colorPixel(11));
            change |= frame->setPixel(5, 4, D1GfxPixel::colorPixel(10));
            change |= frame->setPixel(6, 4, D1GfxPixel::colorPixel(25));
            change |= frame->setPixel(7, 4, D1GfxPixel::colorPixel(27));

            change |= frame->setPixel(1, 3, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(2, 3, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(3, 3, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(4, 3, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(5, 3, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(6, 3, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(7, 3, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(8, 3, D1GfxPixel::colorPixel(26));

            change |= frame->setPixel(2, 2, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(3, 2, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(4, 2, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(5, 2, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(6, 2, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(7, 2, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(8, 2, D1GfxPixel::colorPixel(26));

            change |= frame->setPixel(3, 1, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(4, 1, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(5, 1, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(6, 1, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(7, 1, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(8, 1, D1GfxPixel::colorPixel(29));
            change |= frame->setPixel(9, 1, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(10, 1, D1GfxPixel::colorPixel(26));

            change |= frame->setPixel(4, 0, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(5, 0, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(6, 0, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(7, 0, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(8, 0, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(9, 0, D1GfxPixel::colorPixel(12));
            change |= frame->setPixel(10, 0, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(11, 0, D1GfxPixel::colorPixel(26));
        }
        // fix artifacts
        /*if (i == 4) { // 392[0]
            change |= frame->setPixel(16, 24, D1GfxPixel::colorPixel(42));
        }
        if (i == 5) { // 392[2]
            change |= frame->setPixel(14, 0, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(14, 1, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(15, 1, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(15, 2, D1GfxPixel::colorPixel(29));
            change |= frame->setPixel(16, 2, D1GfxPixel::colorPixel(44));
            change |= frame->setPixel(16, 3, D1GfxPixel::colorPixel(29));
            change |= frame->setPixel(16, 4, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(17, 28, D1GfxPixel::colorPixel(106));
        }*/
        if (i == 12) { // 178[2]
            change |= frame->setPixel(14, 1, D1GfxPixel::colorPixel(10));
            change |= frame->setPixel(15, 1, D1GfxPixel::colorPixel(20));
            change |= frame->setPixel(15, 2, D1GfxPixel::colorPixel(10));
            change |= frame->setPixel(16, 2, D1GfxPixel::colorPixel(10));
            change |= frame->setPixel(16, 3, D1GfxPixel::colorPixel(10));
            change |= frame->setPixel(17, 3, D1GfxPixel::colorPixel(10));
            change |= frame->setPixel(17, 1, D1GfxPixel::colorPixel(20));
            change |= frame->setPixel(18, 1, D1GfxPixel::colorPixel(10));
            change |= frame->setPixel(14, 4, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(15, 5, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(16, 6, D1GfxPixel::transparentPixel());
        }
        if (i == 16) { // 407[0]
            change |= frame->setPixel(31, 26, D1GfxPixel::colorPixel(57));
            change |= frame->setPixel(31, 27, D1GfxPixel::colorPixel(89));
        }
        if (i == 17) { // 156[0]
            change |= frame->setPixel(30, 31, D1GfxPixel::colorPixel(41));
        }
        if (i == 18) { // 160[1]
            change |= frame->setPixel(0, 1, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(0, 2, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(1, 2, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(1, 3, D1GfxPixel::colorPixel(116));
            change |= frame->setPixel(2, 3, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(3, 4, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(4, 4, D1GfxPixel::colorPixel(43));
        }
        if (i == 19) { // 152[1]
            change |= frame->setPixel(11, 6, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(13, 7, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(15, 8, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(17, 9, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(19, 10, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(20, 13, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(21, 13, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(21, 11, D1GfxPixel::colorPixel(43));
            change |= frame->setPixel(22, 16, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(23, 15, D1GfxPixel::colorPixel(43));
            change |= frame->setPixel(23, 16, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(25, 17, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(25, 19, D1GfxPixel::colorPixel(47));
        }
        if (i == 20) { // 159[0]
            change |= frame->setPixel(20, 26, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(21, 26, D1GfxPixel::colorPixel(119));
        }
        if (i == 21) { // 163[1]
            change |= frame->setPixel(0, 26, D1GfxPixel::colorPixel(110));
        }
        if (i == 23) { // 176[1] - fix connection
            change |= frame->setPixel( 8, 5, D1GfxPixel::colorPixel(20));
            change |= frame->setPixel( 9, 5, D1GfxPixel::colorPixel(20));
            change |= frame->setPixel(10, 6, D1GfxPixel::colorPixel(19));
            change |= frame->setPixel(11, 6, D1GfxPixel::colorPixel(5));
            change |= frame->setPixel(12, 7, D1GfxPixel::colorPixel(2));
            change |= frame->setPixel(13, 7, D1GfxPixel::colorPixel(4));
            change |= frame->setPixel(14, 8, D1GfxPixel::colorPixel(17));
            change |= frame->setPixel(15, 8, D1GfxPixel::colorPixel(6));
            change |= frame->setPixel(16, 9, D1GfxPixel::colorPixel(3));
            change |= frame->setPixel(17, 9, D1GfxPixel::colorPixel(41));
        }
        if (i == 24) { // 171[1]
            change |= frame->setPixel(17, 9, D1GfxPixel::colorPixel(20));
            change |= frame->setPixel(17, 10, D1GfxPixel::colorPixel(21));
            change |= frame->setPixel(18, 10, D1GfxPixel::colorPixel(22));
            change |= frame->setPixel(18, 11, D1GfxPixel::colorPixel(21));
            change |= frame->setPixel(19, 11, D1GfxPixel::colorPixel(21));
            change |= frame->setPixel(20, 11, D1GfxPixel::colorPixel(21));
            change |= frame->setPixel(21, 11, D1GfxPixel::colorPixel(22));
            change |= frame->setPixel(16, 12, D1GfxPixel::colorPixel(5));
            change |= frame->setPixel(17, 12, D1GfxPixel::colorPixel(20));
            change |= frame->setPixel(19, 12, D1GfxPixel::colorPixel(21));
            change |= frame->setPixel(20, 12, D1GfxPixel::colorPixel(22));
            change |= frame->setPixel(21, 12, D1GfxPixel::colorPixel(22));
            change |= frame->setPixel(22, 12, D1GfxPixel::colorPixel(21));
            change |= frame->setPixel(23, 12, D1GfxPixel::colorPixel(21));
            change |= frame->setPixel(16, 13, D1GfxPixel::colorPixel(20));
            change |= frame->setPixel(20, 13, D1GfxPixel::colorPixel(22));
            change |= frame->setPixel(21, 13, D1GfxPixel::colorPixel(22));
        }
        if (i == 25) { // 153[0]
            change |= frame->setPixel(27, 4, D1GfxPixel::colorPixel(43));
            change |= frame->setPixel(27, 6, D1GfxPixel::colorPixel(46));
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }
    return true;
}

static BYTE shadowColorCathedral(BYTE color)
{
    // assert(color < 128);
    if (color == 0) {
        return 0;
    }
    switch (color % 16) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        return (color & ~15) + 13;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        return (color & ~15) + 14;
    case 11:
    case 12:
    case 13:
        return (color & ~15) + 15;
    }
    return 0;
}

bool D1Tileset::fixCathedralShadows(bool silent)
{
    const CelMicro micros[] = {
        // add shadow of the grate
/*  0 */{ 306 - 1, 1, D1CEL_FRAME_TYPE::Empty },            // used to block subsequent calls
/*  1 */{ 304 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/*  2 */{ 304 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*  3 */{ 57 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  4 */{ 53 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/*  5 */{ 53 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
        // floor source
/*  6 */{  23 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/*  7 */{  23 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*  8 */{   2 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/*  9 */{   2 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 10 */{   7 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 11 */{   7 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 12 */{   4 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 13 */{   4 - 1, 1, D1CEL_FRAME_TYPE::Empty },
        // grate source
/* 14 */{  53 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 15 */{  47 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 16 */{  48 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 17 */{  48 - 1, 1, D1CEL_FRAME_TYPE::Empty },
        // base shadows
/* 18 */{ 296 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 19 */{ 296 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 20 */{ 297 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 21 */{ 297 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 22 */{ 313 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 23 */{ 313 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 24 */{ 299 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 25 */{ 299 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 26 */{ 301 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 27 */{ 301 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 28 */{ 302 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 29 */{ 302 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 30 */{ 307 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 31 */{ 307 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 32 */{ 308 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 33 */{ 308 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 34 */{ 328 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 35 */{ 328 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
        // complex shadows
/* 36 */{ 310 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 37 */{ 310 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 38 */{ 320 - 1, 0, D1CEL_FRAME_TYPE::LeftTrapezoid },
/* 39 */{ 320 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 40 */{ 321 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 41 */{ 321 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 42 */{ -1, 0, D1CEL_FRAME_TYPE::Empty },
/* 43 */{ 322 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid },
/* 44 */{ 323 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 45 */{ 323 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid },
/* 46 */{ 324 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 47 */{ 324 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 48 */{ 325 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 49 */{ 325 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 50 */{ 298 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 51 */{ -1, 1, D1CEL_FRAME_TYPE::Empty },
/* 52 */{ 304 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 53 */{ 334 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 54 */{ 334 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 55 */{ 334 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },

        // special shadows for the banner setpiece
/* 56 */{ 112 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 57 */{ 112 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 58 */{ 336 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 59 */{ 336 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 60 */{ 118 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 61 */{ 338 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },

/* 62 */{ 330 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
    };

    constexpr unsigned blockSize = BLOCK_SIZE_L1;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // copy 306[1] to 53[2] and 53[0]
        if (i == 0) {
            const CelMicro &microDst1 = micros[i + 4];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microDst1.subtileIndex, blockSize, microDst1.microIndex);
            D1GfxFrame *frameDst1 = mf1.second;
            if (frameDst1 == nullptr) {
                return false;
            }
            const CelMicro &microDst2 = micros[i + 5];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microDst2.subtileIndex, blockSize, microDst2.microIndex);
            D1GfxFrame *frameDst2 = mf2.second;
            if (frameDst2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && (color == 0 || color == 15 || color == 43 || color == 44 || color == 45 || color == 46 || color == 47 || color == 109 || color == 110 || color == 127)) {
                        if (y < MICRO_HEIGHT / 2) {
                            if (frameDst1->getPixel(x, y + MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst1->setPixel(x, y + MICRO_HEIGHT / 2, pixel);
                            }
                        } else {
                            if (frameDst2->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent()) {
                                change |= frameDst2->setPixel(x, y - MICRO_HEIGHT / 2, pixel);
                            }
                        }
                    }
                }
            }
        }
        // copy 304[0] to 53[2]
        if (i == 1) {
            const CelMicro &microDst = micros[i + 3];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 23; x < MICRO_WIDTH; x++) {
                for (int y = 25; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && (color == 0 || color == 15 || color == 43 || color == 44 || color == 45 || color == 46 || color == 47 || color == 109 || color == 110 || color == 127)) {
                        if (frameDst->getPixel(x, y).isTransparent()) {
                            change |= frameDst->setPixel(x, y, pixel); // 53[2]
                        }
                    }
                }
            }
        }
        // copy 304[1] to 57[0]
        if (i == 2) {
            const CelMicro &microDst = micros[i + 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microDst.subtileIndex, blockSize, microDst.microIndex);
            D1GfxFrame *frameDst = mf.second;
            if (frameDst == nullptr) {
                return false;
            }
            for (int x = 0; x < 7; x++) {
                for (int y = 19; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && (color == 0 || color == 15 || color == 43 || color == 44 || color == 45 || color == 46 || color == 47 || color == 109 || color == 110 || color == 127)) {
                        if (frameDst->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent()) {
                            change |= frameDst->setPixel(x, y - MICRO_HEIGHT / 2, pixel); // 57[0]
                        }
                    }
                }
            }
        }
        // fix artifacts I.
        if (i == 3) { // 57[0]
            change |= frame->setPixel(5, 4, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(6, 4, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(1, 9, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(1, 10, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(2, 10, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(1, 5, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(3, 4, D1GfxPixel::colorPixel(42));
            change |= frame->setPixel(10, 8, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(11, 8, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(12, 8, D1GfxPixel::colorPixel(43));
            change |= frame->setPixel(10, 9, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(11, 9, D1GfxPixel::colorPixel(44));
        }
        if (i == 4) { // 53[2]
            change |= frame->setPixel(15, 22, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(15, 23, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(15, 24, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(15, 25, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(9, 27, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(8, 29, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(9, 29, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(10, 27, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(25, 28, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(28, 26, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(19, 28, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(24, 30, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(19, 31, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(24, 31, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(25, 31, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(19, 16, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(18, 17, D1GfxPixel::colorPixel(44));
            change |= frame->setPixel(19, 17, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(20, 17, D1GfxPixel::colorPixel(43));
            change |= frame->setPixel(18, 18, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(19, 18, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(20, 18, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(19, 19, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(20, 19, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(20, 20, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(20, 21, D1GfxPixel::colorPixel(44));
            change |= frame->setPixel(20, 22, D1GfxPixel::colorPixel(43));

            change |= frame->setPixel(8, 30, D1GfxPixel::colorPixel(44));
            change |= frame->setPixel(19, 23, D1GfxPixel::colorPixel(44));
            change |= frame->setPixel(20, 23, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(16, 25, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(17, 25, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(18, 24, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(19, 24, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(27, 31, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(28, 27, D1GfxPixel::colorPixel(44));
            change |= frame->setPixel(29, 28, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(24, 20, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(24, 19, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(25, 19, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(26, 19, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(25, 18, D1GfxPixel::colorPixel(43));
            change |= frame->setPixel(26, 18, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(27, 18, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(28, 18, D1GfxPixel::colorPixel(44));
            change |= frame->setPixel(27, 17, D1GfxPixel::colorPixel(43));
            change |= frame->setPixel(28, 17, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(29, 17, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(28, 16, D1GfxPixel::colorPixel(43));
            change |= frame->setPixel(29, 16, D1GfxPixel::colorPixel(45));
        }
        if (i == 5) { // 53[0]
            change |= frame->setPixel(15, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(19, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(20, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(17, 2, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(9, 8, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(10, 8, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(8, 9, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(9, 9, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(8, 10, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(29, 0, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(27, 1, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(25, 2, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(19, 3, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(17, 4, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(11, 9, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(8, 10, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(8, 11, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(8, 12, D1GfxPixel::colorPixel(44));
            change |= frame->setPixel(9, 12, D1GfxPixel::colorPixel(43));
        }
        // reduce 313[1] using 7[1]
        if (i == 23) {
            const CelMicro &microSrc = micros[i - 12];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < (x + 1) / 2 + 18) {
                        change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 7[1]
                    }
                }
            }
        }
        // draw 298[0] using 48[0] and 297[0]
        /*if (i == 50) {
            const CelMicro &microSrc1 = micros[i - 30];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[i - 16];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc1->getPixel(x, y); // 48[0]
                    quint8 color = pixel.getPaletteIndex();
                    if (pixel.isTransparent() || (color != 0 && color != 45 && color != 46 && color != 47 && color != 109 && color != 110 && color != 111 && color != 127)) {
                        pixel = frameSrc1->getPixel(x, y); // 297[0]
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }*/
        // draw 304[1] using 47[1] and 296[1]
        if (i == 52) {
            const CelMicro &microSrc1 = micros[i - 33];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[i - 37];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc1->getPixel(x, y); // 296[1]
                    quint8 color = pixel.getPaletteIndex();
                    if (x <= 16 || pixel.isTransparent() || (color != 0 && color != 45 && color != 46 && color != 47 && color != 109 && color != 110 && color != 111 && color != 127)) {
                        pixel = frameSrc2->getPixel(x, y); // 47[1]
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // draw 334[0] using 53[0] and 301[0]
        if (i == 53) {
            const CelMicro &microSrc1 = micros[i - 48];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[i - 27];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc1->getPixel(x, y); // 53[0]
                    quint8 color = pixel.getPaletteIndex();
                    if (x > 7 && !pixel.isTransparent()) {
                        if ((x >= 12 && x <= 14) || (x >= 21 && x <= 23)) {
                            pixel = D1GfxPixel::colorPixel(shadowColorCathedral(color));
                        } else if (y > x / 2 + 1) {
                            D1GfxPixel pixel2 = frameSrc2->getPixel(x, y); // 301[0]
                            if (!pixel2.isTransparent()) {
                                pixel = pixel2;
                            } else {
                                pixel = D1GfxPixel::colorPixel(shadowColorCathedral(color));
                            }
                        }
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // draw 334[1] using 53[1] and 301[1]
        if (i == 54) {
            const CelMicro &microSrc1 = micros[i - 27];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[i - 40];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y > (x + 1) / 2 + 17) {
                        pixel = frameSrc1->getPixel(x, y); // 301[1]
                    } else {
                        pixel = frameSrc2->getPixel(x, y); // 53[1]
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // draw 334[2] using 53[2]
        if (i == 55) {
            const CelMicro &microSrc = micros[i - 51];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 53[2]
                    quint8 color = pixel.getPaletteIndex();
                    if (x > 8 && !pixel.isTransparent() && (color == 0 || color == 12 || color == 27 || color == 29 || color == 30 || (color >= 42 && color <= 45) || color == 47 || color == 74 || color == 75 || color == 104 || color == 118 || color == 119 || color == 123) && (y > x - 10)) {
                        pixel = D1GfxPixel::colorPixel(shadowColorCathedral(color));
                    } else if (x <= 8 || (x < 18 && y <= 26)) {
                        pixel = D1GfxPixel::transparentPixel();
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // draw 336[0] using 112[0]
        if (i == 58) {
            const CelMicro &microSrc = micros[i - 2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 112[0]
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && y > 33 - x / 2) {
                        pixel = D1GfxPixel::colorPixel(shadowColorCathedral(color));
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // draw 336[1] using 112[1]
        if (i == 59) {
            const CelMicro &microSrc = micros[i - 2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 112[1]
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && y > 17 - x / 2) {
                        pixel = D1GfxPixel::colorPixel(shadowColorCathedral(color));
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // draw 338[1] using 118[1] and 324[1]
        if (i == 61) {
            const CelMicro &microSrc1 = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[i - 14];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (x < 16) {
                        pixel = frameSrc1->getPixel(x, y); // 118[1]
                    } else {
                        pixel = frameSrc2->getPixel(x, y); // 324[1]
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // reduce 330[0] using 7[0]
        if (i == 62) {
            const CelMicro &microSrc = micros[i - 52];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < (x + 1) / 2 + 2) {
                        change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 7[0]
                    }
                }
            }
        }
        // fix artifacts II.
        if (i == 55) { // 334[2]
            change |= frame->setPixel(12, 29, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(13, 30, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(14, 22, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(15, 22, D1GfxPixel::colorPixel(0));
        }
        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }
    return true;
}

void D1Tileset::cleanupCathedral(std::set<unsigned> &deletedFrames, bool silent)
{
    constexpr int blockSize = BLOCK_SIZE_L1;
    // patch dMegaTiles - L1.TIL
    // make the inner tile at the entrance non-walkable II.
    ReplaceSubtile(this->til, 196 - 1, 3, 425 - 1, silent);
    // fix shadow (use common subtiles)
    ReplaceSubtile(this->til, 7 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 37 - 1, 1, 6 - 1, silent);
    // use common subtiles
    ReplaceSubtile(this->til, 9 - 1, 2, 7 - 1, silent);    // 18
    ReplaceSubtile(this->til, 9 - 1, 3, 4 - 1, silent);    // 19
    ReplaceSubtile(this->til, 21 - 1, 2, 32 - 1, silent);  // 39
    ReplaceSubtile(this->til, 23 - 1, 2, 32 - 1, silent);
    ReplaceSubtile(this->til, 27 - 1, 2, 3 - 1, silent);   // 49
    ReplaceSubtile(this->til, 43 - 1, 2, 3 - 1, silent);
    ReplaceSubtile(this->til, 32 - 1, 2, 53 - 1, silent);  // 58
    ReplaceSubtile(this->til, 37 - 1, 2, 53 - 1, silent);
    ReplaceSubtile(this->til, 38 - 1, 2, 53 - 1, silent);
    ReplaceSubtile(this->til, 33 - 1, 1, 48 - 1, silent);  // 60
    ReplaceSubtile(this->til, 35 - 1, 2, 53 - 1, silent);  // 63
    ReplaceSubtile(this->til, 58 - 1, 1, 2 - 1, silent);   // 111
    ReplaceSubtile(this->til, 60 - 1, 1, 2 - 1, silent);   // 119
    ReplaceSubtile(this->til, 61 - 1, 0, 23 - 1, silent);  // 122
    ReplaceSubtile(this->til, 62 - 1, 0, 23 - 1, silent);  // 124
    ReplaceSubtile(this->til, 73 - 1, 0, 23 - 1, silent);
    ReplaceSubtile(this->til, 74 - 1, 0, 23 - 1, silent);
    ReplaceSubtile(this->til, 75 - 1, 0, 23 - 1, silent);
    ReplaceSubtile(this->til, 77 - 1, 0, 23 - 1, silent);
    // ReplaceSubtile(this->til, 129 - 1, 0, 23 - 1, silent);
    ReplaceSubtile(this->til, 136 - 1, 0, 23 - 1, silent);
    ReplaceSubtile(this->til, 99 - 1, 1, 6 - 1, silent);   // 204
    ReplaceSubtile(this->til, 103 - 1, 1, 2 - 1, silent);  // 213
    ReplaceSubtile(this->til, 186 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 105 - 1, 0, 23 - 1, silent); // 220
    ReplaceSubtile(this->til, 114 - 1, 1, 6 - 1, silent);  // 242
    ReplaceSubtile(this->til, 117 - 1, 1, 6 - 1, silent);
    // ReplaceSubtile(this->til, 130 - 1, 0, 23 - 1, silent); // 275
    ReplaceSubtile(this->til, 133 - 1, 0, 23 - 1, silent); // 282
    ReplaceSubtile(this->til, 137 - 1, 0, 23 - 1, silent); // 293
    ReplaceSubtile(this->til, 128 - 1, 1, 2 - 1, silent);  // 271
    ReplaceSubtile(this->til, 134 - 1, 1, 2 - 1, silent);  // 285
    ReplaceSubtile(this->til, 136 - 1, 1, 2 - 1, silent);  // 290
    // ReplaceSubtile(this->til, 42 - 1, 2, 12 - 1, silent);
    ReplaceSubtile(this->til, 44 - 1, 2, 12 - 1, silent);  // 71
    // ReplaceSubtile(this->til, 159 - 1, 2, 12 - 1, silent); // 341
    ReplaceSubtile(this->til, 59 - 1, 2, 7 - 1, silent);   // 116
    ReplaceSubtile(this->til, 60 - 1, 2, 7 - 1, silent);   // 120
    ReplaceSubtile(this->til, 62 - 1, 2, 7 - 1, silent);   // 125
    ReplaceSubtile(this->til, 128 - 1, 2, 7 - 1, silent);  // 272
    // ReplaceSubtile(this->til, 129 - 1, 2, 7 - 1, silent);  // 273
    ReplaceSubtile(this->til, 136 - 1, 2, 7 - 1, silent);  // 291
    ReplaceSubtile(this->til, 58 - 1, 3, 4 - 1, silent);   // 113
    ReplaceSubtile(this->til, 59 - 1, 3, 4 - 1, silent);   // 117
    ReplaceSubtile(this->til, 60 - 1, 3, 4 - 1, silent);   // 121
    ReplaceSubtile(this->til, 74 - 1, 3, 4 - 1, silent);   // 158
    ReplaceSubtile(this->til, 76 - 1, 3, 4 - 1, silent);   // 161
    ReplaceSubtile(this->til, 97 - 1, 3, 4 - 1, silent);   // 200
    // ReplaceSubtile(this->til, 130 - 1, 3, 4 - 1, silent);  // 277
    ReplaceSubtile(this->til, 137 - 1, 3, 4 - 1, silent);  // 295
    ReplaceSubtile(this->til, 193 - 1, 3, 4 - 1, silent);  // 419
    ReplaceSubtile(this->til, 196 - 1, 2, 36 - 1, silent); // 428
    // use common subtiles instead of minor alterations
    // ReplaceSubtile(this->til, 93 - 1, 2, 177 - 1, silent); // 191
    // simplified door subtiles
    ReplaceSubtile(this->til, 25 - 1, 0, 392 - 1, silent);  // (43)
    ReplaceSubtile(this->til, 26 - 1, 0, 394 - 1, silent);  // (45)
    ReplaceSubtile(this->til, 103 - 1, 0, 407 - 1, silent); // 212
    ReplaceSubtile(this->til, 186 - 1, 2, 213 - 1, silent); // - to make 213 'accessible'
    ReplaceSubtile(this->til, 175 - 1, 2, 43 - 1, silent);  // - to make 43 'accessible'
    ReplaceSubtile(this->til, 176 - 1, 1, 45 - 1, silent);  // - to make 45 'accessible'
    // create separate pillar tile
    ReplaceSubtile(this->til, 28 - 1, 0, 61 - 1, silent);
    ReplaceSubtile(this->til, 28 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 28 - 1, 2, 7 - 1, silent);
    ReplaceSubtile(this->til, 28 - 1, 3, 4 - 1, silent);
    // create the new shadows
    // - use the shadows created by fixCathedralShadows
    ReplaceSubtile(this->til, 131 - 1, 0, 23 - 1, silent);
    ReplaceSubtile(this->til, 131 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 131 - 1, 2, 301 - 1, silent);
    ReplaceSubtile(this->til, 131 - 1, 3, 302 - 1, silent);
    ReplaceSubtile(this->til, 132 - 1, 0, 23 - 1, silent);
    ReplaceSubtile(this->til, 132 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 132 - 1, 2, 310 - 1, silent);
    ReplaceSubtile(this->til, 132 - 1, 3, 344 - 1, silent);
    ReplaceSubtile(this->til, 126 - 1, 0, 296 - 1, silent);
    ReplaceSubtile(this->til, 126 - 1, 1, 297 - 1, silent);
    ReplaceSubtile(this->til, 126 - 1, 2, 310 - 1, silent);
    ReplaceSubtile(this->til, 126 - 1, 3, 344 - 1, silent);
    // ReplaceSubtile(this->til, 139 - 1, 0, 296 - 1, silent);
    // ReplaceSubtile(this->til, 139 - 1, 1, 297 - 1, silent);
    ReplaceSubtile(this->til, 139 - 1, 2, 328 - 1, silent);
    // ReplaceSubtile(this->til, 139 - 1, 3, 299 - 1, silent);
    ReplaceSubtile(this->til, 140 - 1, 0, 23 - 1, silent);
    // ReplaceSubtile(this->til, 140 - 1, 1, 2 - 1, silent);
    // ReplaceSubtile(this->til, 140 - 1, 2, 301 - 1, silent);
    ReplaceSubtile(this->til, 140 - 1, 3, 330 - 1, silent);
    ReplaceSubtile(this->til, 141 - 1, 0, 23 - 1, silent);
    ReplaceSubtile(this->til, 141 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 141 - 1, 2, 310 - 1, silent);
    ReplaceSubtile(this->til, 141 - 1, 3, 299 - 1, silent);
    ReplaceSubtile(this->til, 127 - 1, 0, 296 - 1, silent);
    ReplaceSubtile(this->til, 127 - 1, 1, 297 - 1, silent);
    ReplaceSubtile(this->til, 127 - 1, 2, 310 - 1, silent);
    ReplaceSubtile(this->til, 127 - 1, 3, 299 - 1, silent);
    // ReplaceSubtile(this->til, 142 - 1, 0, 307 - 1, silent);
    // ReplaceSubtile(this->til, 142 - 1, 1, 308 - 1, silent);
    // ReplaceSubtile(this->til, 142 - 1, 2, 7 - 1, silent);
    // ReplaceSubtile(this->til, 142 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 143 - 1, 0, 23 - 1, silent);
    // ReplaceSubtile(this->til, 143 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 143 - 1, 2, 313 - 1, silent);
    ReplaceSubtile(this->til, 143 - 1, 3, 330 - 1, silent);
    ReplaceSubtile(this->til, 144 - 1, 0, 315 - 1, silent);
    // ReplaceSubtile(this->til, 144 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 144 - 1, 2, 317 - 1, silent);
    ReplaceSubtile(this->til, 144 - 1, 3, 299 - 1, silent);
    ReplaceSubtile(this->til, 145 - 1, 0, 21 - 1, silent);
    ReplaceSubtile(this->til, 145 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 145 - 1, 2, 321 - 1, silent);
    ReplaceSubtile(this->til, 145 - 1, 3, 302 - 1, silent);
    ReplaceSubtile(this->til, 146 - 1, 0, 1 - 1, silent);
    // ReplaceSubtile(this->til, 146 - 1, 1, 2 - 1, silent);
    // ReplaceSubtile(this->til, 146 - 1, 2, 320 - 1, silent);
    // ReplaceSubtile(this->til, 146 - 1, 3, 302 - 1, silent);
    ReplaceSubtile(this->til, 147 - 1, 0, 13 - 1, silent);
    // ReplaceSubtile(this->til, 147 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 147 - 1, 2, 320 - 1, silent);
    // ReplaceSubtile(this->til, 147 - 1, 3, 302 - 1, silent);
    // ReplaceSubtile(this->til, 148 - 1, 0, 322 - 1, silent);
    // ReplaceSubtile(this->til, 148 - 1, 1, 323 - 1, silent);
    ReplaceSubtile(this->til, 148 - 1, 2, 328 - 1, silent);
    // ReplaceSubtile(this->til, 148 - 1, 3, 299 - 1, silent);
    // ReplaceSubtile(this->til, 149 - 1, 0, 324 - 1, silent);
    // ReplaceSubtile(this->til, 149 - 1, 1, 325 - 1, silent);
    ReplaceSubtile(this->til, 149 - 1, 2, 328 - 1, silent);
    // ReplaceSubtile(this->til, 149 - 1, 3, 299 - 1, silent);
    ReplaceSubtile(this->til, 150 - 1, 0, 5 - 1, silent);
    ReplaceSubtile(this->til, 150 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 150 - 1, 2, 328 - 1, silent);
    ReplaceSubtile(this->til, 150 - 1, 3, 299 - 1, silent);
    ReplaceSubtile(this->til, 151 - 1, 0, 22 - 1, silent);
    ReplaceSubtile(this->til, 151 - 1, 1, 11 - 1, silent);
    // ReplaceSubtile(this->til, 151 - 1, 2, 328 - 1, silent);
    ReplaceSubtile(this->til, 151 - 1, 3, 299 - 1, silent);
    ReplaceSubtile(this->til, 152 - 1, 0, 64 - 1, silent);
    ReplaceSubtile(this->til, 152 - 1, 1, 48 - 1, silent);
    // ReplaceSubtile(this->til, 152 - 1, 2, 328 - 1, silent);
    ReplaceSubtile(this->til, 152 - 1, 3, 299 - 1, silent);
    ReplaceSubtile(this->til, 153 - 1, 0, 304 - 1, silent);
    ReplaceSubtile(this->til, 153 - 1, 1, 298 - 1, silent);
    ReplaceSubtile(this->til, 153 - 1, 2, 328 - 1, silent);
    ReplaceSubtile(this->til, 153 - 1, 3, 299 - 1, silent);
    ReplaceSubtile(this->til, 154 - 1, 0, 14 - 1, silent);
    ReplaceSubtile(this->til, 154 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 154 - 1, 2, 328 - 1, silent);
    ReplaceSubtile(this->til, 154 - 1, 3, 299 - 1, silent);
    ReplaceSubtile(this->til, 155 - 1, 0, 340 - 1, silent);
    ReplaceSubtile(this->til, 155 - 1, 1, 323 - 1, silent);
    ReplaceSubtile(this->til, 155 - 1, 2, 328 - 1, silent);
    ReplaceSubtile(this->til, 155 - 1, 3, 299 - 1, silent);
    ReplaceSubtile(this->til, 156 - 1, 0, 394 - 1, silent);
    ReplaceSubtile(this->til, 156 - 1, 1, 46 - 1, silent);
    ReplaceSubtile(this->til, 156 - 1, 2, 328 - 1, silent);
    // ReplaceSubtile(this->til, 156 - 1, 3, 299 - 1, silent);
    ReplaceSubtile(this->til, 157 - 1, 0, 62 - 1, silent);
    ReplaceSubtile(this->til, 157 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 157 - 1, 2, 334 - 1, silent);
    ReplaceSubtile(this->til, 157 - 1, 3, 302 - 1, silent);
    // ReplaceSubtile(this->til, 158 - 1, 0, 339 - 1, silent);
    ReplaceSubtile(this->til, 158 - 1, 1, 323 - 1, silent);
    // ReplaceSubtile(this->til, 158 - 1, 2, 3 - 1, silent);
    // ReplaceSubtile(this->til, 158 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 159 - 1, 0, 23 - 1, silent);
    ReplaceSubtile(this->til, 159 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 159 - 1, 2, 328 - 1, silent);
    ReplaceSubtile(this->til, 159 - 1, 3, 299 - 1, silent);
    // ReplaceSubtile(this->til, 160 - 1, 0, 342 - 1, silent);
    ReplaceSubtile(this->til, 160 - 1, 1, 323 - 1, silent);
    ReplaceSubtile(this->til, 160 - 1, 2, 12 - 1, silent);
    // ReplaceSubtile(this->til, 160 - 1, 3, 4 - 1, silent);
    // ReplaceSubtile(this->til, 161 - 1, 0, 343 - 1, silent);
    ReplaceSubtile(this->til, 161 - 1, 1, 323 - 1, silent);
    ReplaceSubtile(this->til, 161 - 1, 2, 53 - 1, silent);
    // ReplaceSubtile(this->til, 161 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 164 - 1, 0, 23 - 1, silent);
    ReplaceSubtile(this->til, 164 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 164 - 1, 2, 313 - 1, silent);
    ReplaceSubtile(this->til, 164 - 1, 3, 302 - 1, silent);
    ReplaceSubtile(this->til, 165 - 1, 0, 296 - 1, silent);
    ReplaceSubtile(this->til, 165 - 1, 1, 297 - 1, silent);
    ReplaceSubtile(this->til, 165 - 1, 2, 328 - 1, silent);
    ReplaceSubtile(this->til, 165 - 1, 3, 344 - 1, silent);
    // - shadows for the banner setpiece
    ReplaceSubtile(this->til, 56 - 1, 0, 1 - 1, silent);
    ReplaceSubtile(this->til, 56 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 56 - 1, 2, 3 - 1, silent);
    ReplaceSubtile(this->til, 56 - 1, 3, 126 - 1, silent);
    ReplaceSubtile(this->til, 55 - 1, 0, 1 - 1, silent);
    ReplaceSubtile(this->til, 55 - 1, 1, 123 - 1, silent);
    ReplaceSubtile(this->til, 55 - 1, 2, 3 - 1, silent);
    // ReplaceSubtile(this->til, 55 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 54 - 1, 0, 338 - 1, silent);
    ReplaceSubtile(this->til, 54 - 1, 1, 297 - 1, silent);
    ReplaceSubtile(this->til, 54 - 1, 2, 328 - 1, silent);
    ReplaceSubtile(this->til, 54 - 1, 3, 299 - 1, silent);
    ReplaceSubtile(this->til, 53 - 1, 0, 337 - 1, silent);
    ReplaceSubtile(this->til, 53 - 1, 1, 297 - 1, silent);
    ReplaceSubtile(this->til, 53 - 1, 2, 336 - 1, silent);
    ReplaceSubtile(this->til, 53 - 1, 3, 344 - 1, silent);
    // - shadows for the vile setmap
    ReplaceSubtile(this->til, 52 - 1, 0, 5 - 1, silent);
    ReplaceSubtile(this->til, 52 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 52 - 1, 2, 313 - 1, silent);
    ReplaceSubtile(this->til, 52 - 1, 3, 302 - 1, silent);
    ReplaceSubtile(this->til, 51 - 1, 0, 5 - 1, silent);
    ReplaceSubtile(this->til, 51 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 51 - 1, 2, 301 - 1, silent);
    ReplaceSubtile(this->til, 51 - 1, 3, 302 - 1, silent);
    ReplaceSubtile(this->til, 50 - 1, 0, 1 - 1, silent);
    // ReplaceSubtile(this->til, 50 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 50 - 1, 2, 320 - 1, silent);
    ReplaceSubtile(this->til, 50 - 1, 3, 330 - 1, silent);
    ReplaceSubtile(this->til, 49 - 1, 0, 335 - 1, silent);
    ReplaceSubtile(this->til, 49 - 1, 1, 308 - 1, silent);
    ReplaceSubtile(this->til, 49 - 1, 2, 7 - 1, silent);
    ReplaceSubtile(this->til, 49 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 48 - 1, 0, 21 - 1, silent);
    ReplaceSubtile(this->til, 48 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 48 - 1, 2, 321 - 1, silent);
    ReplaceSubtile(this->til, 48 - 1, 3, 330 - 1, silent);
    ReplaceSubtile(this->til, 47 - 1, 0, 5 - 1, silent);
    ReplaceSubtile(this->til, 47 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 47 - 1, 2, 301 - 1, silent);
    ReplaceSubtile(this->til, 47 - 1, 3, 330 - 1, silent);
    ReplaceSubtile(this->til, 46 - 1, 0, 14 - 1, silent);
    ReplaceSubtile(this->til, 46 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 46 - 1, 2, 301 - 1, silent);
    ReplaceSubtile(this->til, 46 - 1, 3, 302 - 1, silent);
    // eliminate subtiles of unused tiles
    const int unusedTiles[] = {
        30, 31, 34,/* 38,*/ 39, 40, 41, 42,/*43, 44,*/ 45, 79, 82, 86, 87, 88, 89, 90, 91, 92, 93, 95, 96, 119, 120, 129, 130, 177, 178, 179, 180, 181, 182, 183, 184, 185, 187, 188, 189, 190, 191, 192, 195, 197, 198, 199, 200, 201, 202, 203, 204, 205
    };
    constexpr int blankSubtile = 74 - 1;
    for (int n = 0; n < lengthof(unusedTiles); n++) {
        int tileId = unusedTiles[n];
        ReplaceSubtile(this->til, tileId - 1, 0, blankSubtile, silent);
        ReplaceSubtile(this->til, tileId - 1, 1, blankSubtile, silent);
        ReplaceSubtile(this->til, tileId - 1, 2, blankSubtile, silent);
        ReplaceSubtile(this->til, tileId - 1, 3, blankSubtile, silent);
    }

    // patch dMiniTiles - L1.MIN
    // use micros created by patchCathedralFloor
    if (patchCathedralFloor(silent)) {
        Blk2Mcr(108, 1);
        MoveMcr(153, 6, 152, 5);
        ReplaceMcr(160, 0, 23, 0);
    }
    // use micros created by fixCathedralShadows
    if (fixCathedralShadows(silent)) {
    Blk2Mcr(306, 1);
    ReplaceMcr(298, 0, 297, 0);
    ReplaceMcr(298, 1, 48, 1);
    SetMcr(298, 3, 48, 3);
    SetMcr(298, 5, 48, 5);
    SetMcr(298, 7, 48, 7);
    ReplaceMcr(304, 0, 8, 0);
    SetMcr(304, 3, 47, 3);
    SetMcr(304, 5, 47, 5);
    SetMcr(304, 7, 64, 7);
    SetMcr(334, 4, 53, 4);
    SetMcr(334, 6, 3, 6);

    SetMcr(339, 0, 9, 0);
    SetMcr(339, 1, 322, 1);
    SetMcr(339, 2, 9, 2);
    SetMcr(339, 3, 322, 3);
    ReplaceMcr(339, 4, 9, 4);
    ReplaceMcr(339, 5, 9, 5);
    SetMcr(339, 6, 9, 6);
    SetMcr(339, 7, 9, 7);

    ReplaceMcr(340, 0, 8, 0);
    SetMcr(340, 1, 322, 1);
    SetMcr(340, 2, 14, 2);
    SetMcr(340, 3, 322, 3);
    ReplaceMcr(340, 4, 14, 4);
    ReplaceMcr(340, 5, 5, 5);
    ReplaceMcr(340, 6, 14, 6);
    SetMcr(340, 7, 5, 7);

    SetMcr(342, 0, 8, 0);
    SetMcr(342, 1, 322, 1);
    SetMcr(342, 2, 24, 2);
    SetMcr(342, 3, 322, 3);
    ReplaceMcr(342, 4, 24, 4);
    SetMcr(342, 5, 24, 5);
    SetMcr(342, 6, 1, 6);
    SetMcr(342, 7, 24, 7);

    ReplaceMcr(343, 0, 65, 0);
    ReplaceMcr(343, 1, 322, 1);
    SetMcr(343, 2, 65, 2);
    ReplaceMcr(343, 3, 322, 3);
    ReplaceMcr(343, 4, 65, 4);
    ReplaceMcr(343, 5, 5, 5);
    SetMcr(343, 6, 65, 6);
    ReplaceMcr(343, 7, 9, 7);

    SetMcr(344, 0, 299, 0);
    ReplaceMcr(344, 1, 302, 1);
    ReplaceMcr(330, 1, 7, 1);
    // - special subtiles for the banner setpiece
    SetMcr(336, 2, 112, 2);
    Blk2Mcr(336, 3);
    SetMcr(336, 4, 112, 4);
    HideMcr(336, 5);
    HideMcr(336, 7);
    ReplaceMcr(337, 0, 110, 0);
    ReplaceMcr(337, 1, 324, 1);
    SetMcr(337, 2, 110, 2);
    ReplaceMcr(337, 3, 110, 3);
    SetMcr(337, 4, 110, 4);
    SetMcr(337, 5, 110, 5);
    HideMcr(337, 7);
    ReplaceMcr(338, 0, 118, 0);
    SetMcr(338, 2, 118, 2);
    ReplaceMcr(338, 3, 118, 3);
    SetMcr(338, 4, 118, 4);
    SetMcr(338, 5, 118, 5);
    HideMcr(338, 7);
    // - special subtile for the vile setmap
    ReplaceMcr(335, 0, 8, 0);
    ReplaceMcr(335, 1, 10, 1);
    ReplaceMcr(335, 2, 29, 2);
    ReplaceMcr(335, 4, 29, 4);
    SetMcr(335, 6, 29, 6);
    SetMcr(335, 3, 29, 3);
    SetMcr(335, 5, 29, 5);
    SetMcr(335, 7, 29, 7);
    }
    // subtile to make the inner tile at the entrance non-walkable II.
    Blk2Mcr(425, 0);
    ReplaceMcr(425, 1, 299, 1);
    // subtile for the separate pillar tile
    ReplaceMcr(61, 0, 8, 0);
    SetMcr(61, 1, 8, 1);
    ReplaceMcr(61, 2, 8, 2);
    SetMcr(61, 3, 8, 3);
    ReplaceMcr(61, 4, 8, 4);
    SetMcr(61, 5, 8, 5);
    SetMcr(61, 6, 8, 6);
    SetMcr(61, 7, 8, 7);
    // pointless door micros (re-drawn by dSpecial or the object)
    // - vertical doors    
    ReplaceMcr(392, 4, 231, 4);
    ReplaceMcr(407, 4, 231, 4);
    Blk2Mcr(214, 6);
    Blk2Mcr(214, 4);
    Blk2Mcr(214, 2);
    ReplaceMcr(214, 0, 408, 0);
    ReplaceMcr(214, 1, 408, 1);
    ReplaceMcr(213, 0, 408, 0);
    SetMcr(213, 1, 408, 1);
    // ReplaceMcr(212, 0, 407, 0);
    // ReplaceMcr(212, 2, 392, 2);
    // ReplaceMcr(212, 4, 231, 4);
    Blk2Mcr(408, 2);
    Blk2Mcr(408, 4);
    ReplaceMcr(44, 0, 7, 0);
    ReplaceMcr(44, 1, 7, 1);
    Blk2Mcr(44, 2);
    Blk2Mcr(44, 4);
    HideMcr(44, 6);
    ReplaceMcr(43, 0, 7, 0);
    ReplaceMcr(43, 1, 7, 1);
    Blk2Mcr(43, 2);
    Blk2Mcr(43, 4);
    HideMcr(43, 6);
    ReplaceMcr(393, 0, 7, 0);
    ReplaceMcr(393, 1, 7, 1);
    Blk2Mcr(393, 2);
    Blk2Mcr(393, 4);
    // ReplaceMcr(43, 0, 392, 0);
    // ReplaceMcr(43, 2, 392, 2);
    // ReplaceMcr(43, 4, 231, 4);
    // HideMcr(51, 6);
    // Blk2Mcr(51, 4);
    // Blk2Mcr(51, 2);
    // ReplaceMcr(51, 0, 7, 0);
    // ReplaceMcr(51, 1, 7, 1);
    // - horizontal doors
    ReplaceMcr(394, 5, 5, 5);
    Blk2Mcr(395, 3);
    Blk2Mcr(395, 5);
    ReplaceMcr(395, 1, 11, 1);
    ReplaceMcr(395, 0, 11, 0);
    ReplaceMcr(46, 0, 11, 0);
    ReplaceMcr(46, 1, 11, 1);
    Blk2Mcr(46, 3);
    Blk2Mcr(46, 5);
    HideMcr(46, 7);
    ReplaceMcr(45, 0, 11, 0);
    ReplaceMcr(45, 1, 11, 1);
    Blk2Mcr(45, 3);
    Blk2Mcr(45, 5);
    HideMcr(45, 7);
    // ReplaceMcr(45, 1, 394, 1); // lost details
    // ReplaceMcr(45, 3, 394, 3); // lost details
    // ReplaceMcr(45, 5, 5, 5);
    ReplaceMcr(72, 1, 394, 1);
    ReplaceMcr(72, 3, 394, 3);
    ReplaceMcr(72, 5, 5, 5);
    // useless black micros
    Blk2Mcr(107, 0);
    Blk2Mcr(107, 1);
    Blk2Mcr(109, 1);
    Blk2Mcr(137, 1);
    Blk2Mcr(138, 0);
    Blk2Mcr(138, 1);
    Blk2Mcr(140, 1);
    // pointless pixels
    // Blk2Mcr(152, 5); - TODO: Chop?
    // Blk2Mcr(241, 0);
    // // Blk2Mcr(250, 3);
    Blk2Mcr(148, 4);
    Blk2Mcr(190, 3);
    Blk2Mcr(190, 5);
    Blk2Mcr(247, 2);
    Blk2Mcr(247, 6);
    Blk2Mcr(426, 0);
    HideMcr(427, 1);
    // Blk2Mcr(428, 0);
    // Blk2Mcr(428, 1);
    // - pwater column
    ReplaceMcr(171, 6, 37, 6);
    ReplaceMcr(171, 7, 37, 7);
    Blk2Mcr(171, 4);
    Blk2Mcr(171, 5);
    ReplaceMcr(171, 3, 176, 3); // lost details
    // fix graphical glitch
    // ReplaceMcr(15, 1, 6, 1);
    ReplaceMcr(134, 1, 6, 1);
    ReplaceMcr(65, 7, 9, 7);
    ReplaceMcr(66, 7, 9, 7);
    // ReplaceMcr(68, 7, 9, 7);
    // ReplaceMcr(69, 7, 9, 7);
    ReplaceMcr(179, 7, 28, 7);
    ReplaceMcr(247, 3, 5, 3);
    ReplaceMcr(258, 1, 8, 1);
    // reuse subtiles
    ReplaceMcr(139, 6, 3, 6);
    ReplaceMcr(206, 6, 3, 6);
    ReplaceMcr(208, 6, 3, 6);
    // ReplaceMcr(214, 6, 3, 6);
    ReplaceMcr(228, 6, 3, 6);
    ReplaceMcr(232, 6, 3, 6);
    ReplaceMcr(236, 6, 3, 6);
    ReplaceMcr(240, 6, 3, 6);
    ReplaceMcr(257, 6, 3, 6);
    ReplaceMcr(261, 6, 3, 6);
    ReplaceMcr(269, 6, 3, 6);
    ReplaceMcr(320, 6, 3, 6);
    ReplaceMcr(358, 6, 3, 6);
    ReplaceMcr(362, 6, 3, 6);
    ReplaceMcr(366, 6, 3, 6);
    ReplaceMcr(32, 4, 39, 4);
    // ReplaceMcr(439, 4, 39, 4);
    ReplaceMcr(228, 4, 232, 4); // lost details
    ReplaceMcr(206, 4, 3, 4);   // lost details
    ReplaceMcr(208, 4, 3, 4);   // lost details
    ReplaceMcr(240, 4, 3, 4);   // lost details
    ReplaceMcr(257, 4, 3, 4);
    ReplaceMcr(261, 4, 3, 4);   // lost details
    ReplaceMcr(320, 4, 3, 4);   // lost details
    ReplaceMcr(261, 2, 240, 2); // lost details
    ReplaceMcr(208, 2, 3, 2);   // lost details
    // ReplaceMcr(63, 0, 53, 0);
    // ReplaceMcr(49, 0, 3, 0);
    ReplaceMcr(206, 0, 3, 0);
    ReplaceMcr(208, 0, 3, 0); // lost details
    ReplaceMcr(240, 0, 3, 0);
    // ReplaceMcr(63, 1, 53, 1);
    // ReplaceMcr(58, 1, 53, 1);
    ReplaceMcr(206, 1, 3, 1);
    // ReplaceMcr(15, 7, 6, 7); // lost details
    // ReplaceMcr(56, 7, 6, 7); // lost details
    // ReplaceMcr(60, 7, 6, 7); // lost details
    ReplaceMcr(127, 7, 6, 7);
    ReplaceMcr(134, 7, 6, 7); // lost details
    ReplaceMcr(138, 7, 6, 7);
    ReplaceMcr(198, 7, 6, 7);
    ReplaceMcr(202, 7, 6, 7);
    // ReplaceMcr(204, 7, 6, 7);
    ReplaceMcr(230, 7, 6, 7);
    ReplaceMcr(234, 7, 6, 7);
    ReplaceMcr(238, 7, 6, 7);
    // ReplaceMcr(242, 7, 6, 7);
    ReplaceMcr(244, 7, 6, 7);
    ReplaceMcr(246, 7, 6, 7);
    // ReplaceMcr(251, 7, 6, 7);
    ReplaceMcr(323, 7, 6, 7);
    ReplaceMcr(333, 7, 6, 7);
    ReplaceMcr(365, 7, 6, 7);
    ReplaceMcr(369, 7, 6, 7);
    ReplaceMcr(373, 7, 6, 7);

    // ReplaceMcr(15, 5, 6, 5);
    // ReplaceMcr(46, 5, 6, 5);
    // ReplaceMcr(56, 5, 6, 5);
    ReplaceMcr(127, 5, 6, 5);
    ReplaceMcr(134, 5, 6, 5);
    ReplaceMcr(198, 5, 6, 5);
    ReplaceMcr(202, 5, 6, 5);
    // ReplaceMcr(204, 5, 6, 5);
    ReplaceMcr(230, 5, 6, 5); // lost details
    ReplaceMcr(234, 5, 6, 5); // lost details
    // ReplaceMcr(242, 5, 6, 5);
    ReplaceMcr(244, 5, 6, 5);
    ReplaceMcr(246, 5, 6, 5);
    // ReplaceMcr(251, 5, 6, 5);
    ReplaceMcr(323, 5, 6, 5);
    ReplaceMcr(333, 5, 6, 5);
    // ReplaceMcr(416, 5, 6, 5);
    ReplaceMcr(6, 3, 15, 3);
    // ReplaceMcr(204, 3, 15, 3);
    // ReplaceMcr(242, 3, 15, 3);
    ReplaceMcr(244, 3, 15, 3); // lost details
    ReplaceMcr(246, 3, 15, 3); // lost details
    // ReplaceMcr(251, 3, 15, 3);
    // ReplaceMcr(416, 3, 15, 3);
    // ReplaceMcr(15, 1, 6, 1);
    ReplaceMcr(134, 1, 6, 1);
    ReplaceMcr(198, 1, 6, 1);
    ReplaceMcr(202, 1, 6, 1);
    ReplaceMcr(323, 1, 6, 1);
    // ReplaceMcr(416, 1, 6, 1);
    // ReplaceMcr(15, 0, 6, 0);

    ReplaceMcr(249, 1, 11, 1);
    ReplaceMcr(325, 1, 11, 1);
    // ReplaceMcr(344, 1, 11, 1);
    // ReplaceMcr(402, 1, 11, 1);
    ReplaceMcr(308, 0, 11, 0);
    ReplaceMcr(308, 1, 11, 1);

    // ReplaceMcr(180, 6, 8, 6);
    ReplaceMcr(178, 6, 14, 6);
    ReplaceMcr(10, 6, 1, 6);
    ReplaceMcr(13, 6, 1, 6);
    ReplaceMcr(16, 6, 1, 6);
    ReplaceMcr(21, 6, 1, 6);
    ReplaceMcr(24, 6, 1, 6);
    ReplaceMcr(41, 6, 1, 6);
    // ReplaceMcr(54, 6, 1, 6);
    ReplaceMcr(57, 6, 1, 6);
    // ReplaceMcr(70, 6, 1, 6);
    ReplaceMcr(73, 6, 1, 6);
    ReplaceMcr(137, 6, 1, 6);
    ReplaceMcr(176, 6, 1, 6);
    ReplaceMcr(205, 6, 1, 6);
    ReplaceMcr(207, 6, 1, 6);
    ReplaceMcr(209, 6, 1, 6);
    // ReplaceMcr(212, 6, 1, 6);
    ReplaceMcr(227, 6, 1, 6);
    ReplaceMcr(231, 6, 1, 6);
    ReplaceMcr(235, 6, 1, 6);
    ReplaceMcr(239, 6, 1, 6);
    ReplaceMcr(254, 6, 1, 6);
    ReplaceMcr(256, 6, 1, 6);
    ReplaceMcr(258, 6, 1, 6);
    ReplaceMcr(260, 6, 1, 6);
    ReplaceMcr(319, 6, 1, 6);
    ReplaceMcr(356, 6, 1, 6);
    ReplaceMcr(360, 6, 1, 6);
    ReplaceMcr(364, 6, 1, 6);
    ReplaceMcr(392, 6, 1, 6);
    ReplaceMcr(407, 6, 1, 6);
    ReplaceMcr(417, 6, 1, 6);

    ReplaceMcr(16, 4, 10, 4);
    ReplaceMcr(209, 4, 10, 4);
    ReplaceMcr(37, 4, 34, 4);
    ReplaceMcr(42, 4, 34, 4);
    ReplaceMcr(30, 4, 38, 4);
    // ReplaceMcr(437, 4, 38, 4);
    ReplaceMcr(62, 4, 52, 4);
    // ReplaceMcr(171, 4, 28, 4);
    // ReplaceMcr(180, 4, 28, 4);
    ReplaceMcr(133, 4, 14, 4);
    ReplaceMcr(176, 4, 167, 4);
    ReplaceMcr(13, 4, 1, 4);
    ReplaceMcr(205, 4, 1, 4);
    ReplaceMcr(207, 4, 1, 4);
    ReplaceMcr(239, 4, 1, 4);
    ReplaceMcr(256, 4, 1, 4);
    ReplaceMcr(260, 4, 1, 4);
    ReplaceMcr(319, 4, 1, 4);
    ReplaceMcr(356, 4, 1, 4);
    ReplaceMcr(227, 4, 231, 4);
    ReplaceMcr(235, 4, 231, 4);
    ReplaceMcr(13, 2, 1, 2);
    ReplaceMcr(207, 2, 1, 2);
    ReplaceMcr(256, 2, 1, 2);
    ReplaceMcr(319, 2, 1, 2);
    // ReplaceMcr(179, 2, 28, 2);
    ReplaceMcr(16, 2, 10, 2);
    ReplaceMcr(254, 2, 10, 2);
    // ReplaceMcr(426, 0, 23, 0);
    ReplaceMcr(10, 0, 8, 0); // TODO: 203 would be better?
    ReplaceMcr(14, 0, 8, 0);
    ReplaceMcr(16, 0, 8, 0);
    ReplaceMcr(17, 0, 8, 0);
    ReplaceMcr(21, 0, 8, 0);
    ReplaceMcr(24, 0, 8, 0);
    ReplaceMcr(25, 0, 8, 0);
    // ReplaceMcr(55, 0, 8, 0);
    ReplaceMcr(59, 0, 8, 0);
    // ReplaceMcr(70, 0, 8, 0);
    ReplaceMcr(73, 0, 8, 0);
    ReplaceMcr(178, 0, 8, 0);
    ReplaceMcr(203, 0, 8, 0);
    ReplaceMcr(5, 0, 8, 0); // TODO: triangle begin?
    ReplaceMcr(22, 0, 8, 0);
    // ReplaceMcr(45, 0, 8, 0);
    ReplaceMcr(64, 0, 8, 0);
    ReplaceMcr(201, 0, 8, 0);
    ReplaceMcr(229, 0, 8, 0);
    ReplaceMcr(233, 0, 8, 0);
    ReplaceMcr(237, 0, 8, 0);
    ReplaceMcr(243, 0, 8, 0);
    ReplaceMcr(245, 0, 8, 0);
    ReplaceMcr(247, 0, 8, 0);
    ReplaceMcr(322, 0, 8, 0);
    ReplaceMcr(324, 0, 8, 0);
    ReplaceMcr(394, 0, 8, 0);
    ReplaceMcr(420, 0, 8, 0); // TODO: triangle end?

    ReplaceMcr(52, 0, 57, 0);
    ReplaceMcr(66, 0, 57, 0);
    // ReplaceMcr(67, 0, 57, 0);
    ReplaceMcr(13, 0, 1, 0);
    ReplaceMcr(319, 0, 1, 0);

    // ReplaceMcr(168, 0, 167, 0);
    // ReplaceMcr(168, 1, 167, 1);
    // ReplaceMcr(168, 3, 167, 2); // lost details
    // ReplaceMcr(191, 0, 167, 0); // lost details
    // ReplaceMcr(191, 1, 167, 1); // lost details
    ReplaceMcr(175, 1, 167, 1); // lost details
    ReplaceMcr(177, 0, 167, 0); // lost details
    ReplaceMcr(177, 1, 167, 1); // lost details
    ReplaceMcr(177, 2, 167, 2); // lost details
    // ReplaceMcr(177, 4, 168, 4); // lost details
    ReplaceMcr(175, 4, 177, 4);
    // ReplaceMcr(191, 2, 177, 2); // lost details
    // ReplaceMcr(191, 4, 177, 4);

    // ReplaceMcr(181, 0, 169, 0);

    ReplaceMcr(170, 0, 169, 0);
    ReplaceMcr(170, 1, 169, 1);
    ReplaceMcr(170, 3, 169, 3); // lost details
    // ReplaceMcr(178, 1, 172, 1); // lost details
    // ReplaceMcr(172, 0, 190, 0);
    ReplaceMcr(176, 2, 190, 2);
    ReplaceMcr(176, 0, 190, 0); // lost details

    ReplaceMcr(176, 7, 13, 7);
    // ReplaceMcr(171, 7, 8, 7);
    ReplaceMcr(10, 7, 9, 7);
    ReplaceMcr(20, 7, 9, 7);
    ReplaceMcr(364, 7, 9, 7);
    ReplaceMcr(14, 7, 5, 7);
    ReplaceMcr(17, 7, 5, 7);
    ReplaceMcr(22, 7, 5, 7);
    ReplaceMcr(42, 7, 5, 7);
    // ReplaceMcr(55, 7, 5, 7);
    ReplaceMcr(59, 7, 5, 7);
    ReplaceMcr(133, 7, 5, 7);
    ReplaceMcr(178, 7, 5, 7);
    ReplaceMcr(197, 7, 5, 7);
    ReplaceMcr(201, 7, 5, 7);
    ReplaceMcr(203, 7, 5, 7);
    ReplaceMcr(229, 7, 5, 7);
    ReplaceMcr(233, 7, 5, 7);
    ReplaceMcr(237, 7, 5, 7);
    ReplaceMcr(241, 7, 5, 7);
    ReplaceMcr(243, 7, 5, 7);
    ReplaceMcr(245, 7, 5, 7);
    ReplaceMcr(247, 7, 5, 7);
    ReplaceMcr(248, 7, 5, 7);
    ReplaceMcr(322, 7, 5, 7);
    ReplaceMcr(324, 7, 5, 7);
    ReplaceMcr(368, 7, 5, 7);
    ReplaceMcr(372, 7, 5, 7);
    ReplaceMcr(394, 7, 5, 7);
    ReplaceMcr(420, 7, 5, 7);

    ReplaceMcr(37, 5, 30, 5);
    ReplaceMcr(41, 5, 30, 5);
    ReplaceMcr(42, 5, 34, 5);
    ReplaceMcr(59, 5, 47, 5);
    ReplaceMcr(64, 5, 47, 5);
    ReplaceMcr(178, 5, 169, 5);

    ReplaceMcr(17, 5, 10, 5);
    ReplaceMcr(22, 5, 10, 5);
    ReplaceMcr(66, 5, 10, 5);
    // ReplaceMcr(68, 5, 10, 5);
    ReplaceMcr(248, 5, 10, 5);
    ReplaceMcr(324, 5, 10, 5);
    // ReplaceMcr(180, 5, 29, 5);
    // ReplaceMcr(179, 5, 171, 5);
    ReplaceMcr(14, 5, 5, 5); // TODO: 243 would be better?
    // ReplaceMcr(45, 5, 5, 5);
    // ReplaceMcr(50, 5, 5, 5);
    // ReplaceMcr(55, 5, 5, 5);
    ReplaceMcr(65, 5, 5, 5);
    // ReplaceMcr(67, 5, 5, 5);
    // ReplaceMcr(69, 5, 5, 5);
    // ReplaceMcr(70, 5, 5, 5);
    // ReplaceMcr(72, 5, 5, 5);
    ReplaceMcr(133, 5, 5, 5);
    ReplaceMcr(197, 5, 5, 5);
    ReplaceMcr(201, 5, 5, 5);
    ReplaceMcr(203, 5, 5, 5);
    ReplaceMcr(229, 5, 5, 5);
    ReplaceMcr(233, 5, 5, 5);
    ReplaceMcr(237, 5, 5, 5);
    ReplaceMcr(241, 5, 5, 5);
    ReplaceMcr(243, 5, 5, 5);
    ReplaceMcr(245, 5, 5, 5);
    ReplaceMcr(322, 5, 5, 5);
    ReplaceMcr(52, 3, 47, 3);
    ReplaceMcr(59, 3, 47, 3);
    ReplaceMcr(64, 3, 47, 3);
    ReplaceMcr(73, 3, 47, 3);
    ReplaceMcr(17, 3, 10, 3);
    ReplaceMcr(20, 3, 10, 3);
    ReplaceMcr(22, 3, 10, 3);
    // ReplaceMcr(68, 3, 10, 3);
    ReplaceMcr(324, 3, 10, 3);
    ReplaceMcr(57, 3, 13, 3);
    // ReplaceMcr(180, 3, 29, 3);
    ReplaceMcr(9, 3, 5, 3);
    ReplaceMcr(14, 3, 5, 3);
    ReplaceMcr(24, 3, 5, 3);
    ReplaceMcr(65, 3, 5, 3);
    ReplaceMcr(133, 3, 5, 3); // lost details
    ReplaceMcr(245, 3, 5, 3); // lost details
    ReplaceMcr(52, 1, 47, 1);
    ReplaceMcr(59, 1, 47, 1);
    ReplaceMcr(64, 1, 47, 1);
    ReplaceMcr(73, 1, 47, 1);
    ReplaceMcr(17, 1, 10, 1);
    ReplaceMcr(20, 1, 10, 1);
    // ReplaceMcr(68, 1, 10, 1);
    ReplaceMcr(9, 1, 5, 1);
    ReplaceMcr(14, 1, 5, 1);
    ReplaceMcr(24, 1, 5, 1);
    ReplaceMcr(65, 1, 5, 1);
    ReplaceMcr(243, 1, 5, 1); // lost details
    ReplaceMcr(247, 1, 5, 1);
    ReplaceMcr(13, 1, 176, 1); // lost details
    ReplaceMcr(16, 1, 176, 1); // lost details
    // ReplaceMcr(54, 1, 176, 1); // lost details
    ReplaceMcr(57, 1, 176, 1);  // lost details
    ReplaceMcr(190, 1, 176, 1); // lost details
    // ReplaceMcr(397, 1, 176, 1); // lost details
    ReplaceMcr(25, 1, 8, 1);
    ReplaceMcr(110, 1, 8, 1); // lost details
    ReplaceMcr(1, 1, 8, 1);   // lost details TODO: triangle begin?
    ReplaceMcr(21, 1, 8, 1);  // lost details
    // ReplaceMcr(43, 1, 8, 1);  // lost details
    ReplaceMcr(62, 1, 8, 1);  // lost details
    ReplaceMcr(205, 1, 8, 1); // lost details
    ReplaceMcr(207, 1, 8, 1); // lost details
    // ReplaceMcr(212, 1, 8, 1); // lost details 
    ReplaceMcr(407, 1, 8, 1); // lost details 
    ReplaceMcr(227, 1, 8, 1); // lost details
    ReplaceMcr(231, 1, 8, 1); // lost details
    ReplaceMcr(235, 1, 8, 1); // lost details
    ReplaceMcr(239, 1, 8, 1); // lost details
    ReplaceMcr(254, 1, 8, 1); // lost details
    ReplaceMcr(256, 1, 8, 1); // lost details
    ReplaceMcr(260, 1, 8, 1); // lost details
    // ReplaceMcr(266, 1, 8, 1); // lost details
    ReplaceMcr(319, 1, 8, 1); // lost details
    ReplaceMcr(392, 1, 8, 1); // lost details
    // ReplaceMcr(413, 1, 8, 1); // lost details
    ReplaceMcr(417, 1, 8, 1); // lost details
    // ReplaceMcr(429, 1, 8, 1); // lost details TODO: triangle end?

    // ReplaceMcr(122, 0, 23, 0);
    // ReplaceMcr(122, 1, 23, 1);
    // ReplaceMcr(124, 0, 23, 0);
    ReplaceMcr(141, 0, 23, 0);
    // ReplaceMcr(220, 0, 23, 0);
    // ReplaceMcr(220, 1, 23, 1);
    // ReplaceMcr(293, 0, 23, 0); // lost details
    // ReplaceMcr(300, 0, 23, 0);
    // ReplaceMcr(309, 0, 23, 0);
    // ReplaceMcr(329, 0, 23, 0);
    // ReplaceMcr(329, 1, 23, 1);
    // ReplaceMcr(312, 0, 23, 0);
    // ReplaceMcr(275, 0, 23, 0);
    // ReplaceMcr(275, 1, 23, 1);
    // ReplaceMcr(282, 0, 23, 0);
    // ReplaceMcr(282, 1, 23, 1);
    ReplaceMcr(296, 0, 23, 0);
    // ReplaceMcr(303, 0, 23, 0);
    // ReplaceMcr(303, 1, 296, 1);
    ReplaceMcr(307, 0, 23, 0);
    ReplaceMcr(315, 1, 23, 1);
    // ReplaceMcr(326, 0, 23, 0);
    // ReplaceMcr(327, 0, 23, 0);
    // ReplaceMcr(111, 0, 2, 0);
    // ReplaceMcr(119, 0, 2, 0);
    // ReplaceMcr(119, 1, 2, 1);
    // ReplaceMcr(213, 0, 2, 0); // lost details
    // ReplaceMcr(271, 0, 2, 0);
    // ReplaceMcr(276, 0, 2, 0);
    // ReplaceMcr(279, 0, 2, 0);
    // ReplaceMcr(285, 0, 2, 0);
    // ReplaceMcr(290, 0, 2, 0);
    // ReplaceMcr(316, 0, 2, 0);
    // ReplaceMcr(316, 1, 2, 1);
    ReplaceMcr(12, 0, 7, 0);
    ReplaceMcr(12, 1, 7, 1);
    // ReplaceMcr(18, 0, 7, 0);
    // ReplaceMcr(18, 1, 7, 1);
    // ReplaceMcr(71, 0, 7, 0);
    // ReplaceMcr(71, 1, 7, 1);
    // ReplaceMcr(341, 0, 7, 0);
    // ReplaceMcr(341, 1, 7, 1);
    // ReplaceMcr(405, 0, 7, 0);
    // ReplaceMcr(405, 1, 7, 1);
    // ReplaceMcr(116, 1, 7, 1);
    // ReplaceMcr(120, 0, 7, 0);
    // ReplaceMcr(120, 1, 7, 1);
    // ReplaceMcr(125, 0, 7, 0);
    // ReplaceMcr(125, 1, 7, 1);
    ReplaceMcr(157, 1, 7, 1);
    ReplaceMcr(211, 0, 7, 0);
    ReplaceMcr(222, 0, 7, 0); // lost details
    ReplaceMcr(226, 0, 7, 0);
    ReplaceMcr(259, 0, 7, 0);
    // ReplaceMcr(272, 1, 7, 1);
    // ReplaceMcr(273, 0, 7, 0);
    // ReplaceMcr(273, 1, 7, 1);
    // ReplaceMcr(291, 1, 7, 1);
    ReplaceMcr(452, 0, 7, 0);
    ReplaceMcr(145, 1, 147, 1);
    // ReplaceMcr(19, 0, 4, 0);
    // ReplaceMcr(19, 1, 4, 1);
    // ReplaceMcr(113, 0, 4, 0);
    // ReplaceMcr(113, 1, 4, 1);
    // ReplaceMcr(117, 0, 4, 0);
    // ReplaceMcr(117, 1, 4, 1);
    // ReplaceMcr(121, 0, 4, 0);
    // ReplaceMcr(121, 1, 4, 1);
    // ReplaceMcr(158, 1, 4, 1);
    // ReplaceMcr(161, 0, 4, 0);
    // ReplaceMcr(200, 0, 4, 0);
    // ReplaceMcr(200, 1, 4, 1);
    ReplaceMcr(215, 1, 4, 1);
    // ReplaceMcr(277, 1, 4, 1);
    // ReplaceMcr(295, 0, 4, 0);
    // ReplaceMcr(318, 1, 4, 1);
    // ReplaceMcr(419, 0, 4, 0);
    ReplaceMcr(321, 0, 301, 0);
    ReplaceMcr(321, 1, 301, 1);
    // ReplaceMcr(305, 0, 298, 0);
    // ReplaceMcr(332, 1, 306, 1);

    // ReplaceMcr(168, 2, 167, 2); // lost details

    // ReplaceMcr(400, 6, 1, 6);
    // ReplaceMcr(406, 6, 1, 6);
    // ReplaceMcr(410, 6, 1, 6);

    // eliminate micros of unused subtiles
    // Blk2Mcr(39, 311 ...),
    Blk2Mcr(15, 0);
    Blk2Mcr(15, 1);
    Blk2Mcr(15, 5);
    Blk2Mcr(15, 7);
    Blk2Mcr(49, 0);
    Blk2Mcr(50, 0);
    Blk2Mcr(50, 1);
    Blk2Mcr(50, 2);
    Blk2Mcr(50, 3);
    Blk2Mcr(50, 4);
    Blk2Mcr(50, 5);
    Blk2Mcr(51, 0);
    Blk2Mcr(51, 1);
    Blk2Mcr(51, 2);
    Blk2Mcr(51, 4);
    Blk2Mcr(54, 0);
    Blk2Mcr(54, 1);
    Blk2Mcr(54, 2);
    Blk2Mcr(54, 4);
    Blk2Mcr(54, 6);
    Blk2Mcr(55, 0);
    Blk2Mcr(55, 1);
    Blk2Mcr(55, 3);
    Blk2Mcr(55, 5);
    Blk2Mcr(55, 7);
    Blk2Mcr(56, 0);
    Blk2Mcr(56, 1);
    Blk2Mcr(56, 3);
    Blk2Mcr(56, 5);
    Blk2Mcr(56, 7);
    Blk2Mcr(58, 1);
    Blk2Mcr(60, 7);
    // Blk2Mcr(61, 0);
    // Blk2Mcr(61, 2);
    // Blk2Mcr(61, 4);
    Blk2Mcr(63, 0);
    Blk2Mcr(63, 1);
    Blk2Mcr(67, 0);
    Blk2Mcr(67, 1);
    Blk2Mcr(67, 3);
    Blk2Mcr(67, 5);
    Blk2Mcr(68, 0);
    Blk2Mcr(68, 1);
    Blk2Mcr(68, 2);
    Blk2Mcr(68, 3);
    Blk2Mcr(68, 4);
    Blk2Mcr(68, 5);
    Blk2Mcr(68, 7);
    Blk2Mcr(69, 0);
    Blk2Mcr(69, 1);
    Blk2Mcr(69, 2);
    Blk2Mcr(69, 3);
    Blk2Mcr(69, 4);
    Blk2Mcr(69, 5);
    Blk2Mcr(69, 7);
    Blk2Mcr(70, 0);
    Blk2Mcr(70, 1);
    Blk2Mcr(70, 3);
    Blk2Mcr(70, 5);
    Blk2Mcr(70, 6);
    Blk2Mcr(204, 3);
    Blk2Mcr(204, 5);
    Blk2Mcr(204, 7);
    Blk2Mcr(242, 3);
    Blk2Mcr(242, 5);
    Blk2Mcr(242, 7);
    Blk2Mcr(354, 0);
    Blk2Mcr(354, 2);
    Blk2Mcr(354, 4);
    Blk2Mcr(355, 1);
    Blk2Mcr(355, 3);
    Blk2Mcr(355, 5);
    Blk2Mcr(411, 1);
    Blk2Mcr(411, 3);
    Blk2Mcr(411, 5);
    Blk2Mcr(412, 0);
    Blk2Mcr(412, 2);
    Blk2Mcr(412, 4);

    Blk2Mcr(124, 0);
    Blk2Mcr(111, 0);
    Blk2Mcr(116, 1);
    Blk2Mcr(158, 1);
    Blk2Mcr(161, 0);
    Blk2Mcr(271, 0);
    Blk2Mcr(272, 1);
    Blk2Mcr(277, 1);
    Blk2Mcr(285, 0);
    Blk2Mcr(290, 0);
    Blk2Mcr(291, 1);
    Blk2Mcr(293, 0);
    Blk2Mcr(295, 0);
    Blk2Mcr(300, 0);
    Blk2Mcr(309, 0);
    Blk2Mcr(312, 0);
    Blk2Mcr(326, 0);
    Blk2Mcr(327, 0);
    Blk2Mcr(419, 0);

    Blk2Mcr(168, 0);
    Blk2Mcr(168, 1);
    Blk2Mcr(168, 2);
    Blk2Mcr(168, 4);
    Blk2Mcr(172, 0);
    Blk2Mcr(172, 1);
    Blk2Mcr(172, 2);
    Blk2Mcr(172, 3);
    Blk2Mcr(172, 4);
    Blk2Mcr(172, 5);
    Blk2Mcr(173, 0);
    Blk2Mcr(173, 1);
    Blk2Mcr(173, 3);
    Blk2Mcr(173, 5);
    Blk2Mcr(174, 0);
    Blk2Mcr(174, 1);
    Blk2Mcr(174, 2);
    Blk2Mcr(174, 4);
    Blk2Mcr(179, 1);
    Blk2Mcr(179, 2);
    Blk2Mcr(179, 3);
    Blk2Mcr(179, 5);
    Blk2Mcr(180, 3);
    Blk2Mcr(180, 4);
    Blk2Mcr(180, 5);
    Blk2Mcr(180, 6);
    Blk2Mcr(181, 0);
    Blk2Mcr(181, 1);
    Blk2Mcr(181, 3);
    Blk2Mcr(182, 0);
    Blk2Mcr(182, 1);
    Blk2Mcr(182, 2);
    Blk2Mcr(182, 4);
    Blk2Mcr(183, 0);
    Blk2Mcr(183, 1);
    Blk2Mcr(183, 2);
    Blk2Mcr(183, 4);
    Blk2Mcr(184, 0);
    Blk2Mcr(184, 2);
    Blk2Mcr(184, 4);
    Blk2Mcr(185, 0);
    Blk2Mcr(185, 1);
    Blk2Mcr(185, 2);
    Blk2Mcr(185, 4);
    Blk2Mcr(186, 1);
    Blk2Mcr(186, 3);
    Blk2Mcr(186, 5);
    Blk2Mcr(187, 0);
    Blk2Mcr(187, 1);
    Blk2Mcr(187, 3);
    Blk2Mcr(187, 5);
    Blk2Mcr(188, 0);
    Blk2Mcr(188, 1);
    Blk2Mcr(188, 3);
    Blk2Mcr(188, 5);
    Blk2Mcr(189, 0);
    Blk2Mcr(189, 1);
    Blk2Mcr(189, 3);
    Blk2Mcr(191, 0);
    Blk2Mcr(191, 1);
    Blk2Mcr(191, 2);
    Blk2Mcr(191, 4);
    Blk2Mcr(194, 1);
    Blk2Mcr(194, 3);
    Blk2Mcr(194, 5);
    Blk2Mcr(195, 0);
    Blk2Mcr(195, 1);
    Blk2Mcr(195, 3);
    Blk2Mcr(195, 5);
    Blk2Mcr(196, 0);
    Blk2Mcr(196, 2);
    Blk2Mcr(196, 4);
    Blk2Mcr(196, 5);
    // reused micros in fixCathedralShadows
    // Blk2Mcr(330, 0);
    // Blk2Mcr(330, 1);
    // Blk2Mcr(334, 0);
    // Blk2Mcr(334, 1);
    // Blk2Mcr(334, 2);
    // Blk2Mcr(335, 0);
    // Blk2Mcr(335, 1);
    // Blk2Mcr(335, 2);
    // Blk2Mcr(335, 4);
    // Blk2Mcr(336, 0);
    // Blk2Mcr(336, 1);
    // Blk2Mcr(336, 3);
    // Blk2Mcr(337, 0);
    // Blk2Mcr(337, 1);
    // Blk2Mcr(337, 3);
    // Blk2Mcr(338, 0);
    // Blk2Mcr(338, 1);
    // Blk2Mcr(338, 3);
    // Blk2Mcr(339, 4);
    // Blk2Mcr(339, 5);
    // Blk2Mcr(340, 0);
    // Blk2Mcr(340, 4);
    // Blk2Mcr(340, 5);
    // Blk2Mcr(340, 6);
    // Blk2Mcr(342, 4);
    // Blk2Mcr(343, 0);
    // Blk2Mcr(343, 1);
    // Blk2Mcr(343, 3);
    // Blk2Mcr(343, 4);
    // Blk2Mcr(343, 5);
    // Blk2Mcr(343, 7);
    // Blk2Mcr(344, 1);
    Blk2Mcr(345, 0);
    Blk2Mcr(345, 1);
    Blk2Mcr(345, 2);
    Blk2Mcr(345, 4);
    Blk2Mcr(354, 5);
    Blk2Mcr(355, 4);

    Blk2Mcr(402, 1);

    Blk2Mcr(279, 0);

    Blk2Mcr(251, 0);
    Blk2Mcr(251, 3);
    Blk2Mcr(251, 5);
    Blk2Mcr(251, 7);
    Blk2Mcr(252, 0);
    Blk2Mcr(252, 1);
    Blk2Mcr(252, 3);
    Blk2Mcr(252, 5);
    Blk2Mcr(252, 7);
    Blk2Mcr(266, 0);
    Blk2Mcr(266, 1);
    Blk2Mcr(266, 2);
    Blk2Mcr(266, 4);
    Blk2Mcr(266, 6);
    Blk2Mcr(269, 0);
    Blk2Mcr(269, 1);
    Blk2Mcr(269, 2);
    Blk2Mcr(269, 4);
    Blk2Mcr(274, 0);

    // Blk2Mcr(306, 1);
    Blk2Mcr(314, 0);
    Blk2Mcr(332, 1);
    Blk2Mcr(333, 0);
    Blk2Mcr(333, 1);

    Blk2Mcr(396, 4);
    Blk2Mcr(396, 5);
    Blk2Mcr(396, 6);
    Blk2Mcr(396, 7);
    Blk2Mcr(397, 1);
    Blk2Mcr(397, 4);
    Blk2Mcr(397, 6);
    Blk2Mcr(398, 0);
    Blk2Mcr(398, 5);
    Blk2Mcr(398, 7);
    Blk2Mcr(399, 4);
    Blk2Mcr(399, 6);
    Blk2Mcr(400, 0);
    Blk2Mcr(400, 5);
    Blk2Mcr(400, 6);
    Blk2Mcr(400, 7);
    Blk2Mcr(401, 1);
    Blk2Mcr(401, 3);
    Blk2Mcr(401, 4);
    Blk2Mcr(401, 5);
    Blk2Mcr(401, 6);
    Blk2Mcr(401, 7);
    Blk2Mcr(403, 1);
    Blk2Mcr(403, 3);
    Blk2Mcr(403, 4);
    Blk2Mcr(403, 5);
    Blk2Mcr(403, 6);
    Blk2Mcr(403, 7);
    Blk2Mcr(404, 0);
    Blk2Mcr(404, 5);
    Blk2Mcr(404, 6);
    Blk2Mcr(404, 7);
    Blk2Mcr(406, 5);
    Blk2Mcr(406, 6);
    Blk2Mcr(406, 7);
    Blk2Mcr(409, 4);
    Blk2Mcr(409, 5);
    Blk2Mcr(409, 6);
    Blk2Mcr(410, 4);
    Blk2Mcr(410, 5);
    Blk2Mcr(410, 6);
    Blk2Mcr(410, 7);
    Blk2Mcr(411, 4);
    Blk2Mcr(411, 6);
    Blk2Mcr(412, 5);
    Blk2Mcr(412, 7);
    Blk2Mcr(413, 0);
    Blk2Mcr(413, 1);
    Blk2Mcr(413, 2);
    Blk2Mcr(413, 4);
    Blk2Mcr(414, 0);
    Blk2Mcr(414, 1);
    Blk2Mcr(414, 2);
    Blk2Mcr(414, 4);
    Blk2Mcr(415, 0);
    Blk2Mcr(415, 1);
    Blk2Mcr(415, 3);
    Blk2Mcr(415, 5);
    Blk2Mcr(416, 0);
    Blk2Mcr(416, 1);
    Blk2Mcr(416, 3);
    Blk2Mcr(416, 5);
    Blk2Mcr(429, 0);
    Blk2Mcr(429, 1);
    Blk2Mcr(429, 4);
    Blk2Mcr(429, 6);
    Blk2Mcr(431, 0);
    Blk2Mcr(433, 0);
    Blk2Mcr(433, 1);
    Blk2Mcr(433, 3);
    Blk2Mcr(433, 7);
    Blk2Mcr(434, 0);
    Blk2Mcr(434, 1);

    Blk2Mcr(422, 0);
    Blk2Mcr(422, 1);
    Blk2Mcr(423, 0);
    Blk2Mcr(423, 1);
    Blk2Mcr(424, 0);
    Blk2Mcr(424, 1);
    Blk2Mcr(428, 0);
    Blk2Mcr(428, 1);
    Blk2Mcr(437, 0);
    Blk2Mcr(437, 1);
    Blk2Mcr(437, 4);
    Blk2Mcr(438, 0);
    Blk2Mcr(439, 1);
    Blk2Mcr(439, 4);
    Blk2Mcr(441, 0);
    Blk2Mcr(441, 1);
    Blk2Mcr(442, 0);
    Blk2Mcr(443, 1);
    Blk2Mcr(444, 0);
    Blk2Mcr(444, 1);
    Blk2Mcr(444, 4);
    Blk2Mcr(444, 5);
    Blk2Mcr(445, 0);
    Blk2Mcr(445, 1);
    Blk2Mcr(446, 1);
    Blk2Mcr(447, 0);
    Blk2Mcr(447, 1);
    Blk2Mcr(448, 0);
    Blk2Mcr(448, 1);
    Blk2Mcr(448, 5);
    Blk2Mcr(448, 6);
    Blk2Mcr(449, 0);
    Blk2Mcr(449, 1);
    Blk2Mcr(449, 4);
    Blk2Mcr(449, 5);
    Blk2Mcr(449, 7);

    const int unusedSubtiles[] = {
        18, 19, 71, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 113, 117, 119, 120, 121, 122, 125, 200, 212, 220, 250, 253, 267, 268, 273, 275, 276, 278, 280, 281, 282, 303, 305, 316, 318, 329, 331, 341, 405, 430, 432, 435, 436, 440
    };
    for (int n = 0; n < lengthof(unusedSubtiles); n++) {
        for (int i = 0; i < blockSize; i++) {
            Blk2Mcr(unusedSubtiles[n], i);
        }
    }
}

void D1Tileset::patchCatacombsSpec(bool silent)
{
    typedef struct {
        int frameIndex;
        int frameWidth;
        int frameHeight;
    } CelFrame;
    const CelFrame frames[] = {
        { 0, 64, 160 },
        { 1, 64, 160 },
        { 4, 64, 160 }
    };

    int idx = 0;
    for (int i = 0; i < this->cls->getFrameCount(); i++) {
        const CelFrame &cFrame = frames[idx];
        if (i == cFrame.frameIndex) {
            D1GfxFrame *frame = this->cls->getFrame(i);
            bool change = false;
            if (idx == 0) {
                change |= frame->setPixel(10, 52, D1GfxPixel::colorPixel(55));
                change |= frame->setPixel(11, 52, D1GfxPixel::colorPixel(53));
                change |= frame->setPixel(13, 53, D1GfxPixel::colorPixel(53));
                change |= frame->setPixel(19, 55, D1GfxPixel::colorPixel(55));
                change |= frame->setPixel(23, 57, D1GfxPixel::colorPixel(53));
                change |= frame->setPixel(25, 58, D1GfxPixel::colorPixel(53));
                change |= frame->setPixel(26, 59, D1GfxPixel::colorPixel(55));
                change |= frame->setPixel(27, 60, D1GfxPixel::colorPixel(53));
                change |= frame->setPixel(28, 61, D1GfxPixel::colorPixel(54));

                change |= frame->setPixel(29, 97, D1GfxPixel::colorPixel(76));
                change |= frame->setPixel(30, 95, D1GfxPixel::colorPixel(60));
                change |= frame->setPixel(30, 96, D1GfxPixel::colorPixel(61));
                change |= frame->setPixel(31, 93, D1GfxPixel::colorPixel(57));
            }

            if (idx == 1) {
                change |= frame->setPixel( 2, 104, D1GfxPixel::colorPixel(76));
            }

            if (idx == 2) {
                change |= frame->setPixel( 9, 148, D1GfxPixel::colorPixel(39));
                change |= frame->setPixel(10, 148, D1GfxPixel::colorPixel(66));
                change |= frame->setPixel(10, 149, D1GfxPixel::colorPixel(50));
                change |= frame->setPixel(11, 149, D1GfxPixel::colorPixel(36));
            }

            if (change && !silent) {
                this->cls->setModified();
                dProgress() << QApplication::tr("Special-Frame %1 is modified.").arg(i + 1);
            }

            idx++;
        }
    }
}

static BYTE shadowColorCatacombs(BYTE color)
{
    // assert(color < 128);
    if (color == 0) {
        return 0;
    }
    switch (color % 16) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        return (color & ~15) + 13;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        return (color & ~15) + 14;
    case 11:
    case 12:
    case 13:
        return (color & ~15) + 15;
    }
    return 0;
}
bool D1Tileset::fixCatacombsShadows(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{ 151 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // used to block subsequent calls
/*  1 */{  33 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/*  2 */{ 268 - 1, 0, D1CEL_FRAME_TYPE::LeftTrapezoid },
/*  3 */{  33 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*  4 */{ 268 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid },
/*  5 */{  23 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*  6 */{ 148 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  7 */{   6 - 1, 3, D1CEL_FRAME_TYPE::Empty },
/*  8 */{ 152 - 1, 0, D1CEL_FRAME_TYPE::Square },
/*  9 */{   6 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 10 */{ 250 - 1, 0, D1CEL_FRAME_TYPE::RightTrapezoid },
/* 11 */{   5 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 12 */{ 514 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid },
/* 13 */{ 515 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 14 */{ 155 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
    };
    constexpr unsigned blockSize = BLOCK_SIZE_L2;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        // if (micro.subtileIndex < 0) {
        //    continue;
        // }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // draw new shadow micros 268[0], 268[1], 148[1], 152[0], 250[0] using base micros 33[0], 33[1], 23[1], 6[3], 6[1]
        if (i >= 2 && i < 11 && (i & 1) == 0) {
            const CelMicro &microSrc = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x, y));
                }
            }
            // draw shadow 268[0]
            if (i == 2) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    for (int y = 0; y < MICRO_HEIGHT; y++) {
                        if (y > 49 - x) {
                            D1GfxPixel pixel = frame->getPixel(x, y);
                            if (!pixel.isTransparent()) {
                                quint8 color = pixel.getPaletteIndex();
                                pixel = D1GfxPixel::colorPixel(shadowColorCatacombs(color));
                                change |= frame->setPixel(x, y, pixel);
                            }
                        }
                    }
                }
            }
            // draw shadows 268[1], 152[0]
            if (i == 4 || i == 8) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    for (int y = 0; y < MICRO_HEIGHT; y++) {
                        D1GfxPixel pixel = frame->getPixel(x, y);
                        if (!pixel.isTransparent()) {
                            quint8 color = pixel.getPaletteIndex();
                            pixel = D1GfxPixel::colorPixel(shadowColorCatacombs(color));
                            change |= frame->setPixel(x, y, pixel);
                        }
                    }
                }
            }
            // draw shadow 148[1]
            if (i == 6) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    for (int y = 0; y < MICRO_HEIGHT; y++) {
                        if (y > 22 - x / 2
                         || (x < 6 && y > 14)) { // extend shadow to make the micro more usable
                            D1GfxPixel pixel = frame->getPixel(x, y);
                            if (!pixel.isTransparent()) {
                                quint8 color = pixel.getPaletteIndex();
                                pixel = D1GfxPixel::colorPixel(shadowColorCatacombs(color));
                                change |= frame->setPixel(x, y, pixel);
                            }
                        }
                    }
                }
            }
            // draw shadow 250[0]
            if (i == 10) {
                for (int x = 0; x < 5; x++) {
                    for (int y = 0; y < 12; y++) {
                        if (y < (4 - x) * 3) {
                            D1GfxPixel pixel = frame->getPixel(x, y);
                            quint8 color = pixel.getPaletteIndex();
                            pixel = D1GfxPixel::colorPixel(shadowColorCatacombs(color));
                            change |= frame->setPixel(x, y, pixel);
                        }
                    }
                }
            }
        }
        // fix shadow on 514[1] using 5[1]
        if (i == 12) {
            const CelMicro &microSrc = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 26; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        quint8 color = pixel.getPaletteIndex();
                        pixel = D1GfxPixel::colorPixel(shadowColorCatacombs(color));
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // fix shadow on 515[0]
        if (i == 13) {
            for (int x = 20; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        if (x < 26) {
                            if (y > 37 - x) {
                                continue;
                            }
                        } else if (x < 29) {
                            if (y > 15 - x / 8) {
                                continue;
                            }
                        } else {
                            if (y > 40 - x) {
                                continue;
                            }
                        }
                        quint8 color = pixel.getPaletteIndex();
                        pixel = D1GfxPixel::colorPixel(shadowColorCatacombs(color));
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // draw shadow 155[1]
        if (i == 14) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y > 22 - x / 2) { // extend shadow to make the micro more usable
                        D1GfxPixel pixel = frame->getPixel(x, y);
                        if (!pixel.isTransparent()) {
                            quint8 color = pixel.getPaletteIndex();
                            pixel = D1GfxPixel::colorPixel(shadowColorCatacombs(color));
                            change |= frame->setPixel(x, y, pixel);
                        }
                    }
                }
            }
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

bool D1Tileset::patchCatacombsFloor(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{ 323 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare }, // used to block subsequent calls
/*  1 */{ 134 - 1, 5, D1CEL_FRAME_TYPE::Square },     // change type
/*  2 */{ 283 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },  // change type
/*  3 */{ 482 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },  // change type

/*  4 */{ 17 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare }, // mask door
/*  5 */{ 17 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // unused
/*  6 */{ 17 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare }, // unused
/*  7 */{ 17 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare }, // unused
/*  8 */{ 551 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  9 */{ 551 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 10 */{ 551 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 11 */{ 551 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 12 */{ 13 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 13 */{ 13 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare }, // unused
/* 14 */{ 13 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare }, // unused
/* 15 */{ 13 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare }, // unused
/* 16 */{ 553 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 17 */{ 553 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 18 */{ 553 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 19 */{ 553 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/* 20 */{ 289 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // mask column
/* 21 */{ 288 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 22 */{ 287 - 1, 0, D1CEL_FRAME_TYPE::LeftTrapezoid },
/* 23 */{ 21 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 24 */{ 21 - 1, 3, D1CEL_FRAME_TYPE::Empty },
/* 25 */{ 21 - 1, 4, D1CEL_FRAME_TYPE::Empty },
/* 26 */{ 21 - 1, 5, D1CEL_FRAME_TYPE::Empty },
/* 27 */{ 287 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 28 */{ 287 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 29 */{ 287 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 30 */{ 287 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/* 31 */{ 323 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle }, // redraw floor
/* 22 */{ 323 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 33 */{ 324 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle }, // unused
/* 34 */{ 324 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // unused
/* 35 */{ 332 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 36 */{ 332 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 37 */{ 331 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 38 */{ 331 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 39 */{ 325 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 40 */{ 325 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 41 */{ 342 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 42 */{ 342 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 43 */{ 348 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 44 */{ 348 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

// unify the columns
/* 45 */{ 267 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 46 */{ 23 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 47 */{ 135 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 48 */{ 26 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 49 */{ 21 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 50 */{ 134 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid },
/* 51 */{ 10 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 52 */{ 135 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 53 */{ 9 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 54 */{ 146 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 55 */{ 147 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 56 */{ 167 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 57 */{ 270 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // change type
/* 58 */{ 271 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle }, // reduce shadow
    };

    constexpr unsigned blockSize = BLOCK_SIZE_L2;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        // if (micro.subtileIndex < 0) {
        //    continue;
        // }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // mask 17[1]
        if (i == 4) {
            for (int x = 8; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < (x - 8) / 2 + 22) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 17[1]
                    }
                }
            }
        }
        // mask 17[0]
        /*if (i == 5) {
            for (int x = 19; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < 15 - (x + 1) / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 17[0]
                    }
                }
            }
        }
        // mask 17[2]
        if (i == 6) {
            for (int x = 19; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y != 31 || (x != 30 && x != 31)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 17[2]
                    }
                }
            }
        }
        // mask 17[4]
        if (i == 7) {
            for (int x = 19; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 17[4]
                }
            }
        }*/
        // mask 551[0]
        if (i == 8) {
            for (int x = 0; x < 21; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < 20 - (x - 20) / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 551[0]
                    }
                }
            }
        }
        // mask 551[2]
        if (i == 9) {
            for (int x = 0; x < 21; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 551[2]
                }
            }
        }
        // mask 551[4]
        if (i == 10) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x < 21 || y < 22 - (x - 21) / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 551[4]
                    }
                }
            }
        }
        // mask 551[5]
        if (i == 11) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < 17 - (x + 1) / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 551[5]
                    }
                }
            }
        }
        // mask 13[0]
        if (i == 12) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < 23; y++) {
                    // if ((x < 21 && y < 30 - (x + 1) / 2) || (x > 24 && (y < (x + 1) / 2 - 12))) {
                    if (x < 21 && y < 30 - (x + 1) / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 13[0]
                    }
                }
            }
        }
        // mask 13[1]
        /*if (i == 13) {
            for (int x = 0; x < 10; x++) {
                for (int y = 0; y < 7; y++) {
                    if (y < 4 + x / 2 && (y != 6 || (x != 8 && x != 9))) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 13[1]
                    }
                }
            }
        }
        // mask 13[3], 13[5]
        if (i >= 14 && i < 16) {
            for (int x = 0; x < 10; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }*/
        // mask 553[1]
        if (i == 16) {
            for (int x = 8; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < (x - 8) / 2 + 22) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 553[1]
                    }
                }
            }
        }
        // mask 553[3]
        if (i == 17) {
            for (int x = 8; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 553[3]
                }
            }
        }
        // mask 553[4]
        if (i == 18) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < 2 + x / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 553[4]
                    }
                }
            }
        }
        // mask 553[5]
        if (i == 19) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x > 7 || y < 18 + x / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 553[5]
                    }
                }
            }
        }

        // mask 289[0]
        if (i == 20) {
            for (int x = 15; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < 9; y++) {
                    if ((y < 8 - (x - 14) / 2 && (x != 19 || y != 5) && (x != 20 || y != 4) && (x != 21 || y != 4) && (x != 22 || y != 3) && (x != 23 || y != 3) && (x != 27 || y != 1))
                        || (x == 16 && y == 7) || (x == 18 && y == 6)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 289[0]
                    }
                }
            }
        }
        // mask 288[1]
        if (i == 21) {
            for (int x = 0; x < 17; x++) {
                for (int y = 0; y < 9; y++) {
                    if ((x < 9 && (y < x / 2 - 2 || (x == 3 && y == 0))) || (x >= 9 && y < x / 2)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); // 288[1]
                    }
                }
            }
        }
        // mask 287[2, 3, 4, 5] using 21[2, 3, 4, 5]
        if (i >= 27 &&  i < 31) {
            const CelMicro &microSrc = micros[i - 4];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }

            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y);
                    if (pixel.isTransparent()) {
                        if (i == 27 && ((x == 4 && ((y > 3 && y < 7) || (y > 10 && y < 16)))) || (x == 5 && y != 17)) {
                            continue; // 287[2]
                        }
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()); 
                    }
                }
            }
        }

        // redraw 323[0]
        if (i == 31) {
            for (int x = 4; x < MICRO_WIDTH; x++) {
                for (int y = 18 - x / 2; y > 12 - x / 2 && y >= 0; y--) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    D1GfxPixel pixel2 = frame->getPixel(x, y + 6);
                    if (!pixel2.isTransparent()) {
                        change |= frame->setPixel(x, y + 6, pixel); // 323[0]
                    }
                }
            }
        }
        // redraw 323[1]
        if (i == 32) {
            for (int x = 24; x < MICRO_WIDTH; x++) {
                for (int y = 13; y < 20; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x - 24, y - 12, pixel); // 323[1]
                    }
                }
            }
        }
        // redraw 332[0]
        if (i == 35) {
            // move border down
            for (int x = 6; x < MICRO_WIDTH - 3; x++) {
                for (int y = 10 + (x + 1) / 2; y < 11 + (x + 1) / 2; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    D1GfxPixel pixel3 = frame->getPixel(x + 2, y - 1);
                    change |= frame->setPixel(x, y + 4, pixel);
                    change |= frame->setPixel(x, y, pixel3);
                    if ((x & 1) == 0) {
                        D1GfxPixel pixel3_1 = frame->getPixel(x + 2 + 1, y - 1);
                        change |= frame->setPixel(x + 1, y, pixel3_1);
                    }
                }
            }
            // fix artifacts
            {
                change |= frame->setPixel( 3, 16, D1GfxPixel::colorPixel(73));
                change |= frame->setPixel( 4, 16, D1GfxPixel::colorPixel(40));
                change |= frame->setPixel( 5, 17, D1GfxPixel::colorPixel(55));

                change |= frame->setPixel(30, 29, D1GfxPixel::colorPixel(71));
                change |= frame->setPixel(31, 30, D1GfxPixel::colorPixel(73));

                change |= frame->setPixel(29, 25, D1GfxPixel::colorPixel(26));
                change |= frame->setPixel(31, 25, D1GfxPixel::colorPixel(27));
                change |= frame->setPixel(31, 26, D1GfxPixel::colorPixel(26));
                change |= frame->setPixel(28, 29, D1GfxPixel::colorPixel(41));
                change |= frame->setPixel(26, 25, D1GfxPixel::colorPixel(68));
            }
            // extend border
            for (int x = 0; x < 8; x++) {
                for (int y = 13; y < 20; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    D1GfxPixel pixel2 = frame->getPixel(x + 24, y - 12);
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x + 24, y - 12, pixel);
                    }
                }
            }
        }
        // redraw 332[1]
        if (i == 36) {
            for (int x = 0; x < 30; x++) {
                for (int y = x / 2 + 2; y > x / 2 - 4 && y >= 0; y--) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    D1GfxPixel pixel2 = frame->getPixel(x, y + 6);
                    if (!pixel2.isTransparent()) {
                        change |= frame->setPixel(x, y + 6, pixel); // 332[1]
                    }
                }
            }
            // fix artifacts
            {
                change |= frame->setPixel( 2, 27, D1GfxPixel::colorPixel(25));
                change |= frame->setPixel( 3, 27, D1GfxPixel::colorPixel(26));
                change |= frame->setPixel( 4, 28, D1GfxPixel::colorPixel(25));
                change |= frame->setPixel( 5, 29, D1GfxPixel::colorPixel(26));
                change |= frame->setPixel( 3, 28, D1GfxPixel::colorPixel(26));
                change |= frame->setPixel( 0, 31, D1GfxPixel::colorPixel(41));
                change |= frame->setPixel( 1, 30, D1GfxPixel::colorPixel(68));
                change |= frame->setPixel( 3, 30, D1GfxPixel::colorPixel(68));
            }
        }
        // redraw 331[0]
        if (i == 37) {
            for (int x = 4; x < MICRO_WIDTH; x++) {
                for (int y = 18 - x / 2; y > 12 - x / 2 && y >= 0; y--) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    D1GfxPixel pixel2 = frame->getPixel(x, y + 6);
                    if (!pixel2.isTransparent()) {
                        change |= frame->setPixel(x, y + 6, pixel); // 331[0]
                    }
                }
            }
        }
        // redraw 331[1]
        if (i == 38) {
            for (int x = 0; x < 30; x++) {
                for (int y = x / 2 + 2; y > x / 2 - 4 && y >= 0; y--) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    D1GfxPixel pixel2 = frame->getPixel(x, y + 6);
                    if (!pixel2.isTransparent()) {
                        change |= frame->setPixel(x, y + 6, pixel); // 331[1]
                    }
                }
            }
        }
        // redraw 325[0]
        if (i == 39) {
            for (int x = 4; x < MICRO_WIDTH; x++) {
                for (int y = 18 - x / 2; y > 12 - x / 2 && y >= 0; y--) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    D1GfxPixel pixel2 = frame->getPixel(x, y + 6);
                    if (!pixel2.isTransparent()) {
                        change |= frame->setPixel(x, y + 6, pixel); // 325[0]
                    }
                }
            }
        }
        // redraw 325[1]
        if (i == 40) {
            // reduce border on the right
            for (int x = 14; x < 25; x++) {
                for (int y = 16; y < 23; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    D1GfxPixel pixel2 = frame->getPixel(x, y - 6);
                    if (!pixel2.isTransparent() && !pixel.isTransparent() && (color == 34 || color == 37 || (color > 51 && color < 60) || (color > 77 && color < 75))) {
                        change |= frame->setPixel(x, y, pixel2); // 325[1]
                    }
                }
            }
            // extend border on top
            for (int x = 24; x < MICRO_WIDTH; x++) {
                for (int y = 13; y < 20; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x - 24, y - 12, pixel);  // 325[1]
                    }
                }
            }
        }
        // redraw 342[0] - move border on top
        if (i == 41) {
            // remove border on the right
            for (int x = 26; x < MICRO_WIDTH; x++) {
                for (int y = 1; y < 7; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && color != 34 && color != 37 && color != 68 && color != 70) {
                        change |= frame->setPixel(x, y + 6, pixel);      // 342[0]
                        change |= frame->setPixel(57 - x, y + 3, pixel); // 342[0]
                    }
                }
            }
            // add border on top
            for (int x = 20; x < 26; x++) {
                for (int y = 4; y < 10; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x + 6, y - 3, pixel); // 342[0]
                    }
                }
            }
            // reduce border on top
            for (int x = 20; x < 26; x++) {
                for (int y = 4; y < 11; y++) {
                    D1GfxPixel pixel = frame->getPixel(x - 8, y + 4);
                    change |= frame->setPixel(x, y, pixel); // 342[0]
                }
            }
        }
        // redraw 342[1] - move border on top
        if (i == 42) {
            // remove border on the left
            for (int x = 0; x < 6; x++) {
                for (int y = 1; y < 7; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && color != 34 && color != 39 && color != 54 && (color < 70 || color > 72)) {
                        change |= frame->setPixel(x, y + 6, pixel);     // 342[1]
                        change |= frame->setPixel(5 - x, y + 3, pixel); // 342[1]
                    }
                }
            }
            // add border on top
            for (int x = 6; x < 12; x++) {
                for (int y = 4; y < 10; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x - 6, y - 3, pixel); // 342[1]
                    }
                }
            }
            // reduce border on top
            for (int x = 6; x < 12; x++) {
                for (int y = 4; y < 11; y++) {
                    D1GfxPixel pixel = frame->getPixel(x + 8, y + 4);
                    change |= frame->setPixel(x, y, pixel); // 342[1]
                }
            }
        }
        // redraw 348[0]
        if (i == 43) {
            // reduce border on the left
            for (int x = 8; x < 20; x++) {
                for (int y = 16; y < 23; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    D1GfxPixel pixel2 = frame->getPixel(x, y - 7);
                    if (!pixel2.isTransparent() && !pixel.isTransparent() && (color == 35 || color == 37 || color == 39 || (color > 49 && color < 57) || (color > 66 && color < 72))) {
                        change |= frame->setPixel(x, y, pixel2); // 348[0]
                    }
                }
            }
            // fix artifacts
            {
                change |= frame->setPixel( 2, 15, D1GfxPixel::colorPixel(35));
                change |= frame->setPixel( 2, 16, D1GfxPixel::colorPixel(37));
                change |= frame->setPixel( 3, 16, D1GfxPixel::colorPixel(39));
                change |= frame->setPixel( 4, 16, D1GfxPixel::colorPixel(50));
                change |= frame->setPixel( 5, 17, D1GfxPixel::colorPixel(54));
                change |= frame->setPixel( 6, 17, D1GfxPixel::colorPixel(56));
                change |= frame->setPixel( 7, 18, D1GfxPixel::colorPixel(55));
                change |= frame->setPixel( 8, 18, D1GfxPixel::colorPixel(54));
            }
            // extend border on top
            for (int x = 0; x < 6; x++) {
                for (int y = 13; y < 20; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x + 24, y - 12, pixel); // 348[0]
                    }
                }
            }
        }
        // redraw 348[1]
        if (i == 44) {
            for (int x = 0; x < 30; x++) {
                for (int y = x / 2 + 2; y > x / 2 - 4 && y >= 0; y--) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    D1GfxPixel pixel2 = frame->getPixel(x, y + 6);
                    if (!pixel2.isTransparent()) {
                        change |= frame->setPixel(x, y + 6, pixel); // 348[1]
                    }
                }
            }
        }
        // erase stone in 23[1] using 267[1]
        if (i == 46) {
            const CelMicro &microSrc = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x > 11 || y > 18) {
                        D1GfxPixel pixel = frameSrc->getPixel(x, y); // 267[1]
                        change |= frame->setPixel(x, y, pixel);      // 23[1]
                    }
                }
            }
        }
        // erase stone in 26[0] using 135[0]
        if (i == 48) {
            const CelMicro &microSrc = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x > 24 && y < 11) {
                        D1GfxPixel pixel = frameSrc->getPixel(x, y); // 135[0]
                        change |= frame->setPixel(x, y, pixel);      // 26[0]
                    }
                }
            }
        }
        // erase stone in 134[1] using 21[1]
        if (i == 50) {
            const CelMicro &microSrc = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x < 15 && y > 19) {
                        D1GfxPixel pixel = frameSrc->getPixel(x, y); // 134[1]
                        change |= frame->setPixel(x, y, pixel);      // 21[1]
                    }
                }
            }
        }
        // erase stone in 135[1] using 10[1]
        if (i == 52) {
            const CelMicro &microSrc = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x < 17 && y > 18) {
                        D1GfxPixel pixel = frameSrc->getPixel(x, y); // 10[1]
                        change |= frame->setPixel(x, y, pixel);      // 135[1]
                    }
                }
            }
        }
        // erase stone in 146[0] using 9[0]
        if (i == 54) {
            const CelMicro &microSrc = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x < 20) {
                        D1GfxPixel pixel = frameSrc->getPixel(x, y); // 9[0]
                        change |= frame->setPixel(x, y, pixel);      // 146[0]
                    }
                }
            }
        }
        // fix shadow in 167[0] using 147[0]
        if (i == 56) {
            const CelMicro &microSrc = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 28; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x == 28 && y < 20) {
                        continue;
                    }
                    if (x == 29 && y < 21) {
                        continue;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 147[0]
                    change |= frame->setPixel(x, y, pixel);      // 167[0]
                }
            }
        }
        // fix artifacts
        if (i == 22) { // 287[0]
            change |= frame->setPixel( 0, 13, D1GfxPixel::colorPixel(62));
            change |= frame->setPixel( 0, 14, D1GfxPixel::colorPixel(76));
            change |= frame->setPixel( 0, 15, D1GfxPixel::colorPixel(77));
            change |= frame->setPixel( 0, 16, D1GfxPixel::colorPixel(77));
            change |= frame->setPixel( 1, 13, D1GfxPixel::colorPixel(62));
            change |= frame->setPixel( 1, 14, D1GfxPixel::colorPixel(77));
            change |= frame->setPixel( 1, 15, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel( 1, 16, D1GfxPixel::colorPixel(78));
        }
        if (i == 30) { // 287[5]
            change |= frame->setPixel(27, 15, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(27, 14, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(27, 13, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(26, 14, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(26, 13, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(26, 12, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(26, 11, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(25, 12, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(25, 11, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(27, 8, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(27, 7, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(27, 6, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(27, 5, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(27, 4, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(26, 7, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(26, 6, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(26, 5, D1GfxPixel::transparentPixel());
        }
        if (i == 58) { // 271[0]
            change |= frame->setPixel(30, 1, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(31, 1, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(31, 2, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(31, 3, D1GfxPixel::colorPixel(43));
        }
        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }
    return true;
}

void D1Tileset::cleanupCatacombs(std::set<unsigned> &deletedFrames, bool silent)
{
    constexpr int blockSize = BLOCK_SIZE_L2;

    // unified columns
    ReplaceSubtile(this->til, 6 - 1, 1, 26 - 1, silent);
    ReplaceSubtile(this->til, 6 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 39 - 1, 1, 26 - 1, silent);
    ReplaceSubtile(this->til, 39 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 40 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 41 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 77 - 1, 1, 26 - 1, silent);
    ReplaceSubtile(this->til, 9 - 1, 2, 23 - 1, silent);
    ReplaceSubtile(this->til, 50 - 1, 3, 12 - 1, silent);
    // use common subtile
    ReplaceSubtile(this->til, 13 - 1, 1, 42 - 1, silent);
    ReplaceSubtile(this->til, 13 - 1, 2, 39 - 1, silent);
    // ReplaceSubtile(this->til, 14 - 1, 0, 45 - 1, silent);
    ReplaceSubtile(this->til, 14 - 1, 2, 39 - 1, silent);
    // ReplaceSubtile(this->til, 15 - 1, 0, 45 - 1, silent);
    ReplaceSubtile(this->til, 15 - 1, 1, 42 - 1, silent);
    ReplaceSubtile(this->til, 15 - 1, 2, 43 - 1, silent);
    // ReplaceSubtile(this->til, 16 - 1, 0, 45 - 1, silent);
    ReplaceSubtile(this->til, 16 - 1, 1, 38 - 1, silent);
    ReplaceSubtile(this->til, 16 - 1, 2, 43 - 1, silent);
    ReplaceSubtile(this->til, 24 - 1, 1, 77 - 1, silent);
    ReplaceSubtile(this->til, 25 - 1, 1, 77 - 1, silent);
    ReplaceSubtile(this->til, 25 - 1, 2, 82 - 1, silent);
    ReplaceSubtile(this->til, 27 - 1, 2, 78 - 1, silent);
    ReplaceSubtile(this->til, 40 - 1, 2, 23 - 1, silent);
    ReplaceSubtile(this->til, 41 - 1, 1, 135 - 1, silent);
    // ReplaceSubtile(this->til, 41 - 1, 3, 137 - 1, silent);
    ReplaceSubtile(this->til, 43 - 1, 2, 27 - 1, silent);
    ReplaceSubtile(this->til, 43 - 1, 3, 28 - 1, silent);
    ReplaceSubtile(this->til, 150 - 1, 2, 15 - 1, silent);
    ReplaceSubtile(this->til, 45 - 1, 2, 11 - 1, silent);
    ReplaceSubtile(this->til, 45 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 46 - 1, 0, 9 - 1, silent);
    //ReplaceSubtile(this->til, 47 - 1, 0, 9 - 1, silent);
    //ReplaceSubtile(this->til, 47 - 1, 3, 152 - 1, silent);
    //ReplaceSubtile(this->til, 48 - 1, 2, 155 - 1, silent);
    //ReplaceSubtile(this->til, 48 - 1, 3, 156 - 1, silent);
    //ReplaceSubtile(this->til, 49 - 1, 0, 9 - 1, silent);
    //ReplaceSubtile(this->til, 49 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 68 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 70 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 71 - 1, 0, 9 - 1, silent);
    ReplaceSubtile(this->til, 71 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 77 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 140 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 140 - 1, 3, 162 - 1, silent);
    ReplaceSubtile(this->til, 142 - 1, 3, 162 - 1, silent);
    // - doors
    ReplaceSubtile(this->til, 4 - 1, 0, 537 - 1, silent); // - to make 537 'accessible'
    ReplaceSubtile(this->til, 5 - 1, 0, 539 - 1, silent); // - to make 539 'accessible'
    ReplaceSubtile(this->til, 54 - 1, 0, 173 - 1, silent); // - to make 173 'accessible'
    ReplaceSubtile(this->til, 58 - 1, 0, 172 - 1, silent); // - to make 172 'accessible'
    ReplaceSubtile(this->til, 58 - 1, 1, 179 - 1, silent); // - to make 172 'accessible'
    ReplaceSubtile(this->til, 58 - 1, 2, 180 - 1, silent); // - to make 172 'accessible'
    ReplaceSubtile(this->til, 58 - 1, 3, 181 - 1, silent); // - to make 172 'accessible'
    // use common subtiles instead of minor alterations
    ReplaceSubtile(this->til, 1 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 2 - 1, 2, 11 - 1, silent);
    ReplaceSubtile(this->til, 4 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 150 - 1, 1, 10 - 1, silent);
    // ReplaceSubtile(this->til, 152 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 5 - 1, 2, 11 - 1, silent);
    ReplaceSubtile(this->til, 151 - 1, 2, 11 - 1, silent);
    // ReplaceSubtile(this->til, 153 - 1, 2, 11 - 1, silent);
    ReplaceSubtile(this->til, 5 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 151 - 1, 3, 12 - 1, silent);
    // ReplaceSubtile(this->til, 153 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 21 - 1, 2, 11 - 1, silent);
    ReplaceSubtile(this->til, 21 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 31 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 32 - 1, 2, 11 - 1, silent);
    ReplaceSubtile(this->til, 32 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 42 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 8 - 1, 2, 15 - 1, silent);
    ReplaceSubtile(this->til, 68 - 1, 2, 244 - 1, silent);
    ReplaceSubtile(this->til, 73 - 1, 2, 11 - 1, silent);
    ReplaceSubtile(this->til, 73 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 75 - 1, 3, 12 - 1, silent);
    // ReplaceSubtile(this->til, 78 - 1, 0, 21 - 1, silent);
    ReplaceSubtile(this->til, 106 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 110 - 1, 0, 9 - 1, silent);
    ReplaceSubtile(this->til, 111 - 1, 0, 9 - 1, silent);
    ReplaceSubtile(this->til, 111 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 121 - 1, 0, 9 - 1, silent);
    ReplaceSubtile(this->til, 112 - 1, 2, 11 - 1, silent);
    ReplaceSubtile(this->til, 138 - 1, 2, 11 - 1, silent);
    ReplaceSubtile(this->til, 138 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 139 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 139 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 134 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 134 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 135 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 135 - 1, 3, 12 - 1, silent);
    ReplaceSubtile(this->til, 51 - 1, 2, 155 - 1, silent);
    ReplaceSubtile(this->til, 141 - 1, 2, 155 - 1, silent);
    ReplaceSubtile(this->til, 141 - 1, 3, 169 - 1, silent);
    ReplaceSubtile(this->til, 142 - 1, 2, 155 - 1, silent);
    // - reduce pointless bone-chamber complexity II.
    // -- bones
    ReplaceSubtile(this->til, 54 - 1, 1, 181 - 1, silent);
    ReplaceSubtile(this->til, 54 - 1, 3, 185 - 1, silent);
    ReplaceSubtile(this->til, 53 - 1, 1, 179 - 1, silent);
    ReplaceSubtile(this->til, 53 - 1, 3, 189 - 1, silent);
    ReplaceSubtile(this->til, 56 - 1, 2, 179 - 1, silent);
    ReplaceSubtile(this->til, 56 - 1, 3, 181 - 1, silent);
    ReplaceSubtile(this->til, 57 - 1, 2, 177 - 1, silent);
    ReplaceSubtile(this->til, 57 - 1, 3, 179 - 1, silent);

    ReplaceSubtile(this->til, 59 - 1, 0, 177 - 1, silent);
    ReplaceSubtile(this->til, 59 - 1, 1, 185 - 1, silent);
    ReplaceSubtile(this->til, 59 - 1, 2, 199 - 1, silent);
    ReplaceSubtile(this->til, 59 - 1, 3, 185 - 1, silent);

    ReplaceSubtile(this->til, 60 - 1, 0, 188 - 1, silent);
    ReplaceSubtile(this->til, 60 - 1, 1, 185 - 1, silent);
    ReplaceSubtile(this->til, 60 - 1, 2, 185 - 1, silent);
    ReplaceSubtile(this->til, 60 - 1, 3, 189 - 1, silent);

    ReplaceSubtile(this->til, 62 - 1, 0, 175 - 1, silent);
    ReplaceSubtile(this->til, 62 - 1, 1, 199 - 1, silent);
    ReplaceSubtile(this->til, 62 - 1, 2, 179 - 1, silent);
    ReplaceSubtile(this->til, 62 - 1, 3, 177 - 1, silent);

    ReplaceSubtile(this->til, 63 - 1, 0, 189 - 1, silent);
    ReplaceSubtile(this->til, 63 - 1, 1, 177 - 1, silent);
    ReplaceSubtile(this->til, 63 - 1, 2, 185 - 1, silent);
    ReplaceSubtile(this->til, 63 - 1, 3, 189 - 1, silent);
    // -- flat floor
    ReplaceSubtile(this->til, 92 - 1, 3, 332 - 1, silent);
    ReplaceSubtile(this->til, 94 - 1, 2, 323 - 1, silent);
    ReplaceSubtile(this->til, 94 - 1, 3, 324 - 1, silent);
    ReplaceSubtile(this->til, 97 - 1, 0, 324 - 1, silent);
    ReplaceSubtile(this->til, 97 - 1, 1, 323 - 1, silent);
    ReplaceSubtile(this->til, 97 - 1, 2, 332 - 1, silent);
    ReplaceSubtile(this->til, 99 - 1, 0, 332 - 1, silent);
    ReplaceSubtile(this->til, 99 - 1, 2, 324 - 1, silent);
    ReplaceSubtile(this->til, 99 - 1, 3, 323 - 1, silent);
    // create separate pillar tile
    ReplaceSubtile(this->til, 52 - 1, 0, 55 - 1, silent);
    ReplaceSubtile(this->til, 52 - 1, 1, 26 - 1, silent);
    ReplaceSubtile(this->til, 52 - 1, 2, 23 - 1, silent);
    ReplaceSubtile(this->til, 52 - 1, 3, 12 - 1, silent);
    // create the new shadows
    // - horizontal door for a pillar
    ReplaceSubtile(this->til, 17 - 1, 0, 540 - 1, silent);
    ReplaceSubtile(this->til, 17 - 1, 1, 18 - 1, silent);
    ReplaceSubtile(this->til, 17 - 1, 2, 155 - 1, silent);
    ReplaceSubtile(this->til, 17 - 1, 3, 162 - 1, silent);
    // - horizontal hallway for a pillar
    ReplaceSubtile(this->til, 18 - 1, 0, 553 - 1, silent);
    ReplaceSubtile(this->til, 18 - 1, 1, 99 - 1, silent);
    ReplaceSubtile(this->til, 18 - 1, 2, 155 - 1, silent);
    ReplaceSubtile(this->til, 18 - 1, 3, 162 - 1, silent);
    // - corner tile for a pillar
    ReplaceSubtile(this->til, 34 - 1, 0, 21 - 1, silent);
    ReplaceSubtile(this->til, 34 - 1, 1, 26 - 1, silent);
    ReplaceSubtile(this->til, 34 - 1, 2, 148 - 1, silent);
    ReplaceSubtile(this->til, 34 - 1, 3, 169 - 1, silent);
    // - vertical wall end for a horizontal arch
    ReplaceSubtile(this->til, 35 - 1, 0, 25 - 1, silent);
    ReplaceSubtile(this->til, 35 - 1, 1, 26 - 1, silent);
    ReplaceSubtile(this->til, 35 - 1, 2, 512 - 1, silent);
    ReplaceSubtile(this->til, 35 - 1, 3, 162 - 1, silent);
    // - horizontal wall end for a pillar
    ReplaceSubtile(this->til, 36 - 1, 0, 33 - 1, silent);
    ReplaceSubtile(this->til, 36 - 1, 1, 34 - 1, silent);
    ReplaceSubtile(this->til, 36 - 1, 2, 148 - 1, silent);
    ReplaceSubtile(this->til, 36 - 1, 3, 162 - 1, silent);
    // - horizontal wall end for a horizontal arch
    ReplaceSubtile(this->til, 37 - 1, 0, 268 - 1, silent);
    ReplaceSubtile(this->til, 37 - 1, 1, 515 - 1, silent);
    ReplaceSubtile(this->til, 37 - 1, 2, 148 - 1, silent);
    ReplaceSubtile(this->til, 37 - 1, 3, 169 - 1, silent);
    // - floor tile with vertical arch
    ReplaceSubtile(this->til, 44 - 1, 0, 150 - 1, silent);
    ReplaceSubtile(this->til, 44 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 44 - 1, 2, 153 - 1, silent);
    ReplaceSubtile(this->til, 44 - 1, 3, 12 - 1, silent);
    // - floor tile with shadow of a vertical arch + horizontal arch
    ReplaceSubtile(this->til, 46 - 1, 0, 9 - 1, silent);
    ReplaceSubtile(this->til, 46 - 1, 1, 154 - 1, silent);
    ReplaceSubtile(this->til, 46 - 1, 2, 161 - 1, silent);
    ReplaceSubtile(this->til, 46 - 1, 3, 162 - 1, silent);
    // - floor tile with shadow of a pillar + vertical arch
    ReplaceSubtile(this->til, 47 - 1, 0, 9 - 1, silent);
    // ReplaceSubtile(this->til, 47 - 1, 1, 154 - 1, silent);
    // ReplaceSubtile(this->til, 47 - 1, 2, 155 - 1, silent);
    ReplaceSubtile(this->til, 47 - 1, 3, 162 - 1, silent);
    // - floor tile with shadow of a pillar
    // ReplaceSubtile(this->til, 48 - 1, 0, 9 - 1, silent);
    // ReplaceSubtile(this->til, 48 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 48 - 1, 2, 155 - 1, silent);
    ReplaceSubtile(this->til, 48 - 1, 3, 162 - 1, silent);
    // - floor tile with shadow of a horizontal arch
    ReplaceSubtile(this->til, 49 - 1, 0, 9 - 1, silent);
    ReplaceSubtile(this->til, 49 - 1, 1, 10 - 1, silent);
    // ReplaceSubtile(this->til, 49 - 1, 2, 161 - 1, silent);
    // ReplaceSubtile(this->til, 49 - 1, 3, 162 - 1, silent);
    // - floor tile with shadow(49) with vertical arch
    ReplaceSubtile(this->til, 95 - 1, 0, 158 - 1, silent);
    ReplaceSubtile(this->til, 95 - 1, 1, 165 - 1, silent);
    ReplaceSubtile(this->til, 95 - 1, 2, 155 - 1, silent);
    ReplaceSubtile(this->til, 95 - 1, 3, 162 - 1, silent);
    // - floor tile with shadow(49) with vertical arch
    ReplaceSubtile(this->til, 96 - 1, 0, 150 - 1, silent);
    ReplaceSubtile(this->til, 96 - 1, 1, 10 - 1, silent);
    ReplaceSubtile(this->til, 96 - 1, 2, 156 - 1, silent);
    ReplaceSubtile(this->til, 96 - 1, 3, 162 - 1, silent);
    // - floor tile with shadow(51) with horizontal arch
    ReplaceSubtile(this->til, 100 - 1, 0, 158 - 1, silent);
    ReplaceSubtile(this->til, 100 - 1, 1, 165 - 1, silent);
    ReplaceSubtile(this->til, 100 - 1, 2, 155 - 1, silent);
    ReplaceSubtile(this->til, 100 - 1, 3, 169 - 1, silent);
    // - shadow for the separate pillar
    ReplaceSubtile(this->til, 101 - 1, 0, 55 - 1, silent);
    ReplaceSubtile(this->til, 101 - 1, 1, 26 - 1, silent);
    ReplaceSubtile(this->til, 101 - 1, 2, 148 - 1, silent);
    ReplaceSubtile(this->til, 101 - 1, 3, 169 - 1, silent);
    // fix graphical glitch
    ReplaceSubtile(this->til, 157 - 1, 1, 99 - 1, silent);
    // fix the upstairs II.
    ReplaceSubtile(this->til, 72 - 1, 1, 56 - 1, silent);  // make the back of the stairs non-walkable
    ReplaceSubtile(this->til, 72 - 1, 0, 9 - 1, silent);   // use common subtile
    ReplaceSubtile(this->til, 72 - 1, 2, 11 - 1, silent);  // use common subtile
    // ReplaceSubtile(this->til, 76 - 1, 1, 10 - 1, silent);  // use common subtile
    // ReplaceSubtile(this->til, 158 - 1, 0, 9 - 1, silent);  // use common subtile
    // ReplaceSubtile(this->til, 158 - 1, 1, 56 - 1, silent); // make the back of the stairs non-walkable
    // ReplaceSubtile(this->til, 158 - 1, 2, 11 - 1, silent); // use common subtile
    // ReplaceSubtile(this->til, 159 - 1, 0, 9 - 1, silent);  // use common subtile
    // ReplaceSubtile(this->til, 159 - 1, 1, 10 - 1, silent); // use common subtile
    // ReplaceSubtile(this->til, 159 - 1, 2, 11 - 1, silent); // use common subtile
    // eliminate subtiles of unused tiles
    const int unusedTiles[] = {
        61, 64, 65, 66, 67, 76, 93, 98, 102, 103, 104, 143, 144, 145, 146, 147, 148, 149, 152, 153, 154, 155, 158, 159, 160
    };
    constexpr int blankSubtile = 2 - 1;
    for (int n = 0; n < lengthof(unusedTiles); n++) {
        int tileId = unusedTiles[n];
        ReplaceSubtile(this->til, tileId - 1, 0, blankSubtile, silent);
        ReplaceSubtile(this->til, tileId - 1, 1, blankSubtile, silent);
        ReplaceSubtile(this->til, tileId - 1, 2, blankSubtile, silent);
        ReplaceSubtile(this->til, tileId - 1, 3, blankSubtile, silent);
    }


    // fix the upstairs III.
    //if (pSubtiles[MICRO_IDX(265 - 1, blockSize, 3)] != 0) {
    if (this->patchCatacombsStairs(72 - 1, 158 - 1, 76 - 1, 159 - 1, 267, 559, silent)) {
        // move the frames to the back subtile
        // - left side
        MoveMcr(252, 2, 265, 3);
        HideMcr(556, 3); // optional

        // - right side
        MoveMcr(252, 1, 265, 5);
        HideMcr(556, 5); // optional

        MoveMcr(252, 3, 267, 2);
        // Blk2Mcr(559, 2);

        MoveMcr(252, 5, 267, 4);
        // Blk2Mcr(559, 4);

        MoveMcr(252, 7, 267, 6);
        HideMcr(559, 6); // optional
    }
    // adjust the frame types
    if (this->patchCatacombsFloor(silent)) {
        // unify the columns
        Blk2Mcr(22, 3);
        Blk2Mcr(26, 3);
        Blk2Mcr(132, 2);
        Blk2Mcr(270, 3);
        ReplaceMcr(25, 1, 21, 1);
        ReplaceMcr(26, 1, 10, 1);
        ReplaceMcr(132, 0, 23, 0);
        ReplaceMcr(132, 1, 23, 1);
        ReplaceMcr(135, 0, 26, 0);
        Blk2Mcr(22, 0);
        Blk2Mcr(22, 1);
        Blk2Mcr(35, 0);
        Blk2Mcr(35, 1);
        // TODO: add decorations 26[1], 22, 35
        // cleaned broken column
        ReplaceMcr(287, 6, 33, 6);
        ReplaceMcr(287, 7, 25, 7);
        Blk2Mcr(288, 3);
        Blk2Mcr(289, 8); // pointless from the beginning
        Blk2Mcr(289, 6);
        Blk2Mcr(289, 4);
        Blk2Mcr(289, 2);
        // separate subtiles for automap
        // - 55 := 21
        ReplaceMcr(55, 0, 33, 0);
        SetMcr(55, 1, 21, 1);
        ReplaceMcr(55, 2, 33, 2);
        SetMcr(55, 3, 25, 3);
        ReplaceMcr(55, 4, 33, 4);
        ReplaceMcr(55, 5, 25, 5);
        ReplaceMcr(55, 6, 33, 6);
        ReplaceMcr(55, 7, 25, 7);
        // - 269 := 21
        ReplaceMcr(269, 0, 33, 0);
        ReplaceMcr(269, 1, 21, 1);
        ReplaceMcr(269, 2, 33, 2);
        ReplaceMcr(269, 3, 25, 3);
        ReplaceMcr(269, 4, 33, 4);
        ReplaceMcr(269, 5, 25, 5);
        ReplaceMcr(269, 6, 33, 6);
        ReplaceMcr(269, 7, 25, 7);
        // - 48 := 45
        ReplaceMcr(48, 4, 45, 4);
        ReplaceMcr(48, 5, 45, 5);
        ReplaceMcr(48, 6, 45, 6);
        ReplaceMcr(48, 7, 45, 7);
        // - 50 := 45
        ReplaceMcr(50, 4, 45, 4);
        ReplaceMcr(50, 5, 45, 5);
        ReplaceMcr(50, 6, 45, 6);
        ReplaceMcr(50, 7, 45, 7);
        // - 53 := 45
        ReplaceMcr(53, 4, 45, 4);
        ReplaceMcr(53, 5, 45, 5);
        ReplaceMcr(53, 6, 45, 6);
        ReplaceMcr(53, 7, 45, 7);
    }
    if (this->fixCatacombsShadows(silent)) {
        Blk2Mcr(161, 0);
        MoveMcr(161, 0, 151, 0);
        Blk2Mcr(151, 1);

        HideMcr(24, 1);
        HideMcr(133, 1);

        ReplaceMcr(147, 1, 154, 1);
        ReplaceMcr(167, 1, 154, 1);

        ReplaceMcr(150, 0, 9, 0);
        SetMcr(150, 1, 9, 1);
        SetMcr(153, 0, 11, 0);
        ReplaceMcr(153, 1, 11, 1);
        ReplaceMcr(156, 0, 161, 0);
        SetMcr(156, 1, 161, 1);
        ReplaceMcr(158, 0, 166, 0);
        SetMcr(158, 1, 166, 1);
        SetMcr(165, 0, 167, 0);
        ReplaceMcr(165, 1, 167, 1);
        ReplaceMcr(164, 0, 147, 0);
        ReplaceMcr(164, 1, 147, 1);

        SetMcr(268, 2, 33, 2);
        SetMcr(268, 3, 29, 3);
        SetMcr(268, 4, 33, 4);
        SetMcr(268, 5, 29, 5);
        SetMcr(268, 6, 33, 6);
        SetMcr(268, 7, 29, 7);

        MoveMcr(515, 3, 152, 0);
        MoveMcr(515, 1, 250, 0);

        // extend the shadow to make the micro more usable
        SetMcr(148, 0, 155, 0);
    }
    // pointless door micros (re-drawn by dSpecial or the object)
    // - vertical doors    
    // Blk2Mcr(13, 2);
    ReplaceMcr(538, 0, 13, 0);
    ReplaceMcr(538, 1, 13, 1);
    ReplaceMcr(538, 2, 13, 2);
    ReplaceMcr(538, 3, 13, 3);
    // Blk2Mcr(538, 2);
    Blk2Mcr(538, 4);
    ReplaceMcr(538, 5, 13, 5);
    Blk2Mcr(538, 6);
    Blk2Mcr(538, 7);
    ReplaceMcr(537, 0, 13, 0);
    ReplaceMcr(537, 1, 13, 1);
    SetMcr(537, 2, 13, 2);
    SetMcr(537, 3, 13, 3);
    HideMcr(537, 4);
    SetMcr(537, 5, 13, 5);
    Blk2Mcr(537, 6);
    HideMcr(537, 7);
    // -- new vertical doors
    ReplaceMcr(172, 0, 178, 0);
    ReplaceMcr(172, 1, 178, 1);
    ReplaceMcr(172, 2, 178, 2);
    ReplaceMcr(172, 3, 178, 3);
    Blk2Mcr(172, 4);
    ReplaceMcr(172, 5, 178, 5);
    Blk2Mcr(172, 6);
    ReplaceMcr(173, 0, 178, 0);
    ReplaceMcr(173, 1, 178, 1);
    SetMcr(173, 2, 178, 2);
    SetMcr(173, 3, 178, 3);
    SetMcr(173, 5, 178, 5);
    // - horizontal doors
    // Blk2Mcr(17, 3);
    ReplaceMcr(540, 0, 17, 0);
    ReplaceMcr(540, 1, 17, 1);
    ReplaceMcr(540, 2, 17, 2);
    ReplaceMcr(540, 3, 17, 3);
    ReplaceMcr(540, 4, 17, 4);
    Blk2Mcr(540, 5);
    Blk2Mcr(540, 6);
    Blk2Mcr(540, 7);
    SetMcr(539, 0, 17, 0);
    ReplaceMcr(539, 1, 17, 1);
    SetMcr(539, 2, 17, 2);
    SetMcr(539, 3, 17, 3);
    SetMcr(539, 4, 17, 4);
    HideMcr(539, 6);
    // - reduce pointless bone-chamber complexity I.
    Blk2Mcr(323, 2);
    Blk2Mcr(325, 2);
    Blk2Mcr(331, 2);
    Blk2Mcr(331, 3);
    Blk2Mcr(332, 3);
    Blk2Mcr(348, 3);
    Blk2Mcr(326, 0);
    Blk2Mcr(326, 1);
    Blk2Mcr(333, 0);
    Blk2Mcr(333, 1);
    Blk2Mcr(333, 2);
    Blk2Mcr(340, 0);
    Blk2Mcr(340, 1);
    Blk2Mcr(341, 0);
    Blk2Mcr(341, 1);
    Blk2Mcr(347, 0);
    Blk2Mcr(347, 1);
    Blk2Mcr(347, 3);
    Blk2Mcr(350, 0);
    Blk2Mcr(350, 1);

    ReplaceMcr(324, 0, 339, 0);
    Blk2Mcr(334, 0);
    Blk2Mcr(334, 1);
    Blk2Mcr(339, 1);
    Blk2Mcr(349, 0);
    Blk2Mcr(349, 1);
    // pointless pixels
    Blk2Mcr(103, 6);
    Blk2Mcr(107, 6);
    Blk2Mcr(111, 2);
    Blk2Mcr(283, 3);
    Blk2Mcr(283, 7);
    Blk2Mcr(295, 6);
    Blk2Mcr(299, 4);
    Blk2Mcr(494, 6);
    Blk2Mcr(551, 7);
    Blk2Mcr(482, 3);
    Blk2Mcr(482, 7);
    Blk2Mcr(553, 6);
    // fix bad artifact
    Blk2Mcr(288, 7);
    // fix graphical glitch
    Blk2Mcr(279, 7);
    // ReplaceMcr(548, 0, 99, 0);
    ReplaceMcr(552, 1, 244, 1);

    // reuse subtiles
    ReplaceMcr(27, 6, 3, 6);
    ReplaceMcr(62, 6, 3, 6);
    ReplaceMcr(66, 6, 3, 6);
    ReplaceMcr(78, 6, 3, 6);
    ReplaceMcr(82, 6, 3, 6);
    // ReplaceMcr(85, 6, 3, 6);
    ReplaceMcr(88, 6, 3, 6);
    // ReplaceMcr(92, 6, 3, 6);
    ReplaceMcr(96, 6, 3, 6);
    // ReplaceMcr(117, 6, 3, 6);
    // ReplaceMcr(120, 6, 3, 6);
    ReplaceMcr(129, 6, 3, 6);
    ReplaceMcr(132, 6, 3, 6);
    // ReplaceMcr(172, 6, 3, 6);
    ReplaceMcr(176, 6, 3, 6);
    ReplaceMcr(184, 6, 3, 6);
    // ReplaceMcr(236, 6, 3, 6);
    ReplaceMcr(240, 6, 3, 6);
    ReplaceMcr(244, 6, 3, 6);
    ReplaceMcr(277, 6, 3, 6);
    ReplaceMcr(285, 6, 3, 6);
    ReplaceMcr(305, 6, 3, 6);
    ReplaceMcr(416, 6, 3, 6);
    ReplaceMcr(420, 6, 3, 6);
    ReplaceMcr(480, 6, 3, 6);
    ReplaceMcr(484, 6, 3, 6);

    ReplaceMcr(27, 4, 3, 4);
    ReplaceMcr(62, 4, 3, 4);
    ReplaceMcr(78, 4, 3, 4);
    ReplaceMcr(82, 4, 3, 4);
    // ReplaceMcr(85, 4, 3, 4);
    ReplaceMcr(88, 4, 3, 4);
    // ReplaceMcr(92, 4, 3, 4);
    ReplaceMcr(96, 4, 3, 4);
    // ReplaceMcr(117, 4, 3, 4);
    // ReplaceMcr(120, 4, 3, 4);
    ReplaceMcr(129, 4, 3, 4);
    // ReplaceMcr(172, 4, 3, 4);
    ReplaceMcr(176, 4, 3, 4);
    // ReplaceMcr(236, 4, 66, 4);
    ReplaceMcr(240, 4, 3, 4);
    ReplaceMcr(244, 4, 66, 4);
    ReplaceMcr(277, 4, 66, 4);
    ReplaceMcr(281, 4, 3, 4);
    ReplaceMcr(285, 4, 3, 4);
    ReplaceMcr(305, 4, 3, 4);
    ReplaceMcr(480, 4, 3, 4);
    ReplaceMcr(552, 4, 3, 4);

    ReplaceMcr(27, 2, 3, 2);
    ReplaceMcr(62, 2, 3, 2);
    ReplaceMcr(78, 2, 3, 2);
    ReplaceMcr(82, 2, 3, 2);
    // ReplaceMcr(85, 2, 3, 2);
    ReplaceMcr(88, 2, 3, 2);
    // ReplaceMcr(92, 2, 3, 2);
    ReplaceMcr(96, 2, 3, 2);
    // ReplaceMcr(117, 2, 3, 2);
    ReplaceMcr(129, 2, 3, 2);
    // ReplaceMcr(172, 2, 3, 2);
    ReplaceMcr(176, 2, 3, 2);
    ReplaceMcr(180, 2, 3, 2);
    // ReplaceMcr(236, 2, 66, 2);
    ReplaceMcr(244, 2, 66, 2);
    ReplaceMcr(277, 2, 66, 2);
    ReplaceMcr(281, 2, 3, 2);
    ReplaceMcr(285, 2, 3, 2);
    ReplaceMcr(305, 2, 3, 2);
    ReplaceMcr(448, 2, 3, 2);
    ReplaceMcr(480, 2, 3, 2);
    ReplaceMcr(552, 2, 3, 2);

    ReplaceMcr(78, 0, 3, 0);
    ReplaceMcr(88, 0, 3, 0);
    // ReplaceMcr(92, 0, 3, 0);
    ReplaceMcr(96, 0, 62, 0);
    // ReplaceMcr(117, 0, 62, 0);
    // ReplaceMcr(120, 0, 62, 0);
    // ReplaceMcr(236, 0, 62, 0);
    ReplaceMcr(240, 0, 62, 0);
    ReplaceMcr(244, 0, 62, 0);
    ReplaceMcr(277, 0, 66, 0);
    ReplaceMcr(281, 0, 62, 0);
    ReplaceMcr(285, 0, 62, 0);
    ReplaceMcr(305, 0, 62, 0);
    ReplaceMcr(448, 0, 3, 0);
    ReplaceMcr(480, 0, 62, 0);
    ReplaceMcr(552, 0, 62, 0);

    // ReplaceMcr(85, 1, 82, 1);
    // ReplaceMcr(117, 1, 244, 1);
    // ReplaceMcr(120, 1, 244, 1);
    // ReplaceMcr(236, 1, 244, 1);
    ReplaceMcr(240, 1, 62, 1);
    ReplaceMcr(452, 1, 244, 1);
    // ReplaceMcr(539, 1, 15, 1);

    // TODO: ReplaceMcr(30, 7, 6, 7); ?
    ReplaceMcr(34, 7, 30, 7);
    ReplaceMcr(69, 7, 6, 7);
    ReplaceMcr(73, 7, 30, 7);
    ReplaceMcr(99, 7, 6, 7);
    ReplaceMcr(104, 7, 6, 7);
    ReplaceMcr(108, 7, 6, 7);
    ReplaceMcr(112, 7, 6, 7);
    ReplaceMcr(128, 7, 30, 7);
    ReplaceMcr(135, 7, 6, 7);
    // ReplaceMcr(139, 7, 6, 7);
    ReplaceMcr(187, 7, 6, 7);
    ReplaceMcr(191, 7, 6, 7);
    // ReplaceMcr(195, 7, 6, 7);
    ReplaceMcr(254, 7, 6, 7);
    ReplaceMcr(258, 7, 30, 7);
    ReplaceMcr(262, 7, 6, 7);
    ReplaceMcr(292, 7, 30, 7);
    ReplaceMcr(296, 7, 6, 7);
    ReplaceMcr(300, 7, 6, 7);
    ReplaceMcr(304, 7, 30, 7);
    ReplaceMcr(423, 7, 6, 7);
    ReplaceMcr(427, 7, 6, 7);
    ReplaceMcr(455, 7, 6, 7);
    ReplaceMcr(459, 7, 6, 7);
    ReplaceMcr(495, 7, 6, 7);
    ReplaceMcr(499, 7, 6, 7);

    ReplaceMcr(30, 5, 6, 5);
    ReplaceMcr(34, 5, 6, 5);
    ReplaceMcr(69, 5, 6, 5);
    ReplaceMcr(99, 5, 6, 5);
    ReplaceMcr(108, 5, 104, 5);
    ReplaceMcr(112, 5, 6, 5);
    ReplaceMcr(128, 5, 6, 5);
    ReplaceMcr(183, 5, 6, 5);
    ReplaceMcr(187, 5, 6, 5);
    ReplaceMcr(191, 5, 6, 5);
    // ReplaceMcr(195, 5, 6, 5);
    ReplaceMcr(254, 5, 6, 5);
    ReplaceMcr(258, 5, 73, 5);
    ReplaceMcr(262, 5, 6, 5);
    ReplaceMcr(292, 5, 73, 5);
    ReplaceMcr(296, 5, 6, 5);
    ReplaceMcr(300, 5, 6, 5);
    ReplaceMcr(304, 5, 6, 5);
    ReplaceMcr(455, 5, 6, 5);
    ReplaceMcr(459, 5, 6, 5);
    ReplaceMcr(499, 5, 6, 5);
    // ReplaceMcr(548, 5, 6, 5);

    ReplaceMcr(30, 3, 6, 3);
    ReplaceMcr(34, 3, 6, 3);
    ReplaceMcr(69, 3, 6, 3);
    ReplaceMcr(99, 3, 6, 3);
    ReplaceMcr(108, 3, 104, 3);
    ReplaceMcr(112, 3, 6, 3);
    ReplaceMcr(128, 3, 6, 3);
    ReplaceMcr(183, 3, 6, 3);
    ReplaceMcr(187, 3, 6, 3);
    ReplaceMcr(191, 3, 6, 3);
    // ReplaceMcr(195, 3, 6, 3);
    ReplaceMcr(254, 3, 6, 3);
    ReplaceMcr(258, 3, 73, 3);
    ReplaceMcr(262, 3, 6, 3);
    ReplaceMcr(292, 3, 73, 3);
    ReplaceMcr(296, 3, 6, 3);
    ReplaceMcr(300, 3, 6, 3);
    ReplaceMcr(304, 3, 6, 3);
    ReplaceMcr(455, 3, 6, 3);
    ReplaceMcr(459, 3, 6, 3);
    ReplaceMcr(499, 3, 6, 3);
    // ReplaceMcr(548, 3, 6, 3);

    ReplaceMcr(30, 1, 34, 1);
    ReplaceMcr(69, 1, 6, 1);
    ReplaceMcr(104, 1, 99, 1);
    ReplaceMcr(112, 1, 99, 1);
    ReplaceMcr(128, 1, 34, 1);
    ReplaceMcr(254, 1, 6, 1);
    ReplaceMcr(258, 1, 73, 1);
    ReplaceMcr(262, 1, 99, 1);
    ReplaceMcr(292, 1, 73, 1);
    ReplaceMcr(296, 1, 99, 1);
    ReplaceMcr(300, 1, 99, 1);
    ReplaceMcr(304, 1, 34, 1);
    ReplaceMcr(427, 1, 6, 1);
    ReplaceMcr(459, 1, 6, 1);
    ReplaceMcr(499, 1, 6, 1);
    // ReplaceMcr(548, 1, 6, 1);

    ReplaceMcr(1, 6, 60, 6);
    ReplaceMcr(21, 6, 33, 6);
    ReplaceMcr(29, 6, 25, 6);
    // ReplaceMcr(48, 6, 45, 6);
    // ReplaceMcr(50, 6, 45, 6);
    // ReplaceMcr(53, 6, 45, 6);
    ReplaceMcr(80, 6, 60, 6);
    ReplaceMcr(84, 6, 60, 6);
    ReplaceMcr(94, 6, 60, 6);
    ReplaceMcr(127, 6, 25, 6);
    ReplaceMcr(131, 6, 25, 6);
    ReplaceMcr(134, 6, 33, 6);
    ReplaceMcr(138, 6, 25, 6);
    ReplaceMcr(141, 6, 25, 6);
    ReplaceMcr(143, 6, 25, 6);
    ReplaceMcr(174, 6, 60, 6);
    ReplaceMcr(182, 6, 25, 6);
    ReplaceMcr(234, 6, 60, 6);
    ReplaceMcr(238, 6, 60, 6);
    ReplaceMcr(242, 6, 60, 6);
    ReplaceMcr(275, 6, 60, 6);
    ReplaceMcr(279, 6, 60, 6);
    ReplaceMcr(283, 6, 60, 6);
    ReplaceMcr(303, 6, 25, 6);
    ReplaceMcr(414, 6, 60, 6);
    ReplaceMcr(418, 6, 60, 6);
    ReplaceMcr(446, 6, 60, 6);
    ReplaceMcr(450, 6, 60, 6);
    ReplaceMcr(478, 6, 60, 6);
    ReplaceMcr(482, 6, 60, 6);
    ReplaceMcr(510, 6, 60, 6);

    ReplaceMcr(21, 4, 33, 4);
    ReplaceMcr(29, 4, 25, 4);
    ReplaceMcr(60, 4, 1, 4);
    ReplaceMcr(94, 4, 1, 4);
    ReplaceMcr(102, 4, 98, 4);
    ReplaceMcr(127, 4, 25, 4);
    ReplaceMcr(134, 4, 33, 4);
    ReplaceMcr(143, 4, 25, 4);
    ReplaceMcr(174, 4, 1, 4);
    ReplaceMcr(182, 4, 25, 4);
    ReplaceMcr(238, 4, 1, 4);
    ReplaceMcr(242, 4, 64, 4);
    ReplaceMcr(275, 4, 64, 4);
    ReplaceMcr(418, 4, 1, 4);

    ReplaceMcr(21, 2, 33, 2);
    ReplaceMcr(29, 2, 25, 2);
    ReplaceMcr(60, 2, 1, 2);
    ReplaceMcr(94, 2, 1, 2);
    ReplaceMcr(102, 2, 98, 2);
    ReplaceMcr(107, 2, 103, 2);
    ReplaceMcr(127, 2, 25, 2);
    ReplaceMcr(134, 2, 33, 2);
    ReplaceMcr(141, 2, 131, 2);
    ReplaceMcr(143, 2, 25, 2);
    ReplaceMcr(174, 2, 1, 2);
    ReplaceMcr(182, 2, 25, 2);

    ReplaceMcr(21, 0, 33, 0);
    ReplaceMcr(29, 0, 25, 0);
    ReplaceMcr(84, 0, 80, 0);
    ReplaceMcr(127, 0, 25, 0);
    ReplaceMcr(131, 0, 33, 0);
    ReplaceMcr(134, 0, 33, 0);
    ReplaceMcr(138, 0, 33, 0);
    ReplaceMcr(141, 0, 33, 0);
    ReplaceMcr(143, 0, 25, 0);
    ReplaceMcr(182, 0, 25, 0);
    ReplaceMcr(234, 0, 64, 0);
    ReplaceMcr(446, 0, 1, 0);
    ReplaceMcr(450, 0, 1, 0);
    ReplaceMcr(478, 0, 283, 0);

    ReplaceMcr(84, 1, 80, 1);
    ReplaceMcr(127, 1, 33, 1);
    ReplaceMcr(234, 1, 64, 1);
    ReplaceMcr(253, 1, 111, 1);
    ReplaceMcr(454, 1, 68, 1);
    ReplaceMcr(458, 1, 111, 1);

    ReplaceMcr(21, 7, 25, 7);
    ReplaceMcr(131, 7, 25, 7);
    ReplaceMcr(266, 7, 25, 7);
    ReplaceMcr(33, 7, 29, 7);
    ReplaceMcr(98, 7, 5, 7);
    ReplaceMcr(102, 7, 5, 7);
    ReplaceMcr(103, 7, 5, 7);
    ReplaceMcr(107, 7, 5, 7);
    ReplaceMcr(111, 7, 5, 7);
    ReplaceMcr(127, 7, 29, 7);
    ReplaceMcr(134, 7, 29, 7);
    ReplaceMcr(138, 7, 29, 7);
    ReplaceMcr(141, 7, 29, 7);
    ReplaceMcr(143, 7, 29, 7);
    ReplaceMcr(295, 7, 5, 7);
    ReplaceMcr(299, 7, 5, 7);
    ReplaceMcr(494, 7, 5, 7);
    ReplaceMcr(68, 7, 5, 7);
    ReplaceMcr(72, 7, 5, 7);
    ReplaceMcr(186, 7, 5, 7);
    ReplaceMcr(190, 7, 5, 7);
    ReplaceMcr(253, 7, 5, 7);
    ReplaceMcr(257, 7, 5, 7);
    ReplaceMcr(261, 7, 5, 7);
    ReplaceMcr(291, 7, 5, 7);
    ReplaceMcr(422, 7, 5, 7);
    ReplaceMcr(426, 7, 5, 7);
    ReplaceMcr(454, 7, 5, 7);
    ReplaceMcr(458, 7, 5, 7);
    ReplaceMcr(498, 7, 5, 7);

    ReplaceMcr(21, 5, 25, 5);
    ReplaceMcr(33, 5, 29, 5);
    ReplaceMcr(111, 5, 5, 5);
    ReplaceMcr(127, 5, 29, 5);
    ReplaceMcr(131, 5, 25, 5);
    ReplaceMcr(141, 5, 29, 5);
    ReplaceMcr(299, 5, 5, 5);
    ReplaceMcr(68, 5, 5, 5);
    ReplaceMcr(186, 5, 5, 5);
    ReplaceMcr(190, 5, 5, 5);
    ReplaceMcr(253, 5, 5, 5);
    ReplaceMcr(257, 5, 72, 5);
    ReplaceMcr(266, 5, 25, 5);
    ReplaceMcr(422, 5, 5, 5);
    ReplaceMcr(454, 5, 5, 5);
    ReplaceMcr(458, 5, 5, 5);

    ReplaceMcr(21, 3, 25, 3);
    ReplaceMcr(33, 3, 29, 3);
    ReplaceMcr(111, 3, 5, 3);
    ReplaceMcr(127, 3, 29, 3);
    ReplaceMcr(131, 3, 25, 3);
    ReplaceMcr(141, 3, 29, 3);
    ReplaceMcr(68, 3, 5, 3);
    ReplaceMcr(186, 3, 5, 3);
    ReplaceMcr(190, 3, 5, 3);
    ReplaceMcr(266, 3, 25, 3);
    ReplaceMcr(454, 3, 5, 3);
    ReplaceMcr(458, 3, 5, 3);
    ReplaceMcr(514, 3, 5, 3);

    ReplaceMcr(28, 1, 12, 1); // lost details
    ReplaceMcr(36, 0, 12, 0); // lost details
    ReplaceMcr(61, 1, 10, 1); // lost details
    ReplaceMcr(63, 1, 12, 1); // lost details
    ReplaceMcr(65, 1, 10, 1); // lost details
    ReplaceMcr(67, 1, 12, 1); // lost details
    ReplaceMcr(74, 0, 11, 0); // lost details
    ReplaceMcr(75, 0, 12, 0); // lost details
    ReplaceMcr(77, 1, 10, 1); // lost details
    ReplaceMcr(79, 1, 12, 1); // lost details
    ReplaceMcr(87, 1, 10, 1); // lost details
    ReplaceMcr(83, 1, 12, 1); // lost details
    ReplaceMcr(89, 1, 12, 1); // lost details
    ReplaceMcr(91, 1, 10, 1); // lost details
    ReplaceMcr(93, 1, 12, 1); // lost details
    ReplaceMcr(105, 0, 11, 0); // lost details
    ReplaceMcr(113, 0, 11, 0); // lost details
    // ReplaceMcr(136, 1, 23, 1); // lost details
    ReplaceMcr(239, 1, 10, 1); // lost details
    ReplaceMcr(241, 1, 12, 1); // lost details
    ReplaceMcr(245, 1, 4, 1); // lost details
    ReplaceMcr(248, 0, 11, 0); // lost details
    ReplaceMcr(260, 0, 12, 0); // lost details
    ReplaceMcr(263, 0, 11, 0); // lost details
    ReplaceMcr(293, 0, 11, 0); // lost details
    ReplaceMcr(273, 1, 10, 1); // lost details
    ReplaceMcr(301, 0, 11, 0); // lost details
    ReplaceMcr(371, 1, 9, 1); // lost details
    ReplaceMcr(373, 1, 11, 1); // lost details
    ReplaceMcr(377, 0, 11, 0); // lost details
    ReplaceMcr(380, 1, 10, 1); // lost details
    ReplaceMcr(383, 1, 9, 1); // lost details
    ReplaceMcr(408, 0, 11, 0); // lost details
    ReplaceMcr(411, 1, 10, 1); // lost details
    ReplaceMcr(419, 1, 10, 1); // lost details
    ReplaceMcr(431, 1, 10, 1); // lost details
    ReplaceMcr(436, 0, 11, 0); // lost details
    ReplaceMcr(443, 1, 10, 1); // lost details
    ReplaceMcr(451, 1, 10, 1); // lost details
    ReplaceMcr(456, 0, 11, 0); // lost details
    ReplaceMcr(468, 0, 11, 0); // lost details
    ReplaceMcr(471, 1, 10, 1); // lost details
    ReplaceMcr(490, 1, 9, 1); // lost details
    ReplaceMcr(508, 0, 11, 0); // lost details
    ReplaceMcr(510, 1, 1, 1); // lost details
    ReplaceMcr(544, 1, 10, 1); // lost details
    ReplaceMcr(546, 1, 16, 1); // lost details
    ReplaceMcr(549, 0, 11, 0); // lost details
    ReplaceMcr(550, 0, 12, 0); // lost details

    // eliminate micros of unused subtiles
    // Blk2Mcr(554,  ...),
    Blk2Mcr(24, 0);
    Blk2Mcr(31, 0);
    Blk2Mcr(31, 1);
    Blk2Mcr(31, 2);
    Blk2Mcr(31, 4);
    Blk2Mcr(31, 6);
    Blk2Mcr(46, 5);
    Blk2Mcr(46, 6);
    Blk2Mcr(46, 7);
    Blk2Mcr(47, 4);
    Blk2Mcr(47, 6);
    Blk2Mcr(47, 7);
    Blk2Mcr(49, 4);
    Blk2Mcr(49, 6);
    Blk2Mcr(49, 7);
    Blk2Mcr(51, 5);
    Blk2Mcr(51, 6);
    Blk2Mcr(51, 7);
    Blk2Mcr(52, 7);
    Blk2Mcr(54, 6);
    Blk2Mcr(81, 0);
    Blk2Mcr(81, 1);
    Blk2Mcr(85, 1);
    Blk2Mcr(85, 2);
    Blk2Mcr(85, 4);
    Blk2Mcr(85, 6);
    Blk2Mcr(92, 0);
    Blk2Mcr(92, 1);
    Blk2Mcr(92, 2);
    Blk2Mcr(92, 4);
    Blk2Mcr(92, 6);
    Blk2Mcr(115, 0);
    Blk2Mcr(115, 1);
    Blk2Mcr(115, 2);
    Blk2Mcr(115, 3);
    Blk2Mcr(115, 4);
    Blk2Mcr(115, 5);
    Blk2Mcr(119, 0);
    Blk2Mcr(119, 1);
    Blk2Mcr(119, 2);
    Blk2Mcr(119, 4);
    Blk2Mcr(121, 0);
    Blk2Mcr(121, 1);
    Blk2Mcr(121, 2);
    Blk2Mcr(121, 3);
    Blk2Mcr(121, 4);
    Blk2Mcr(121, 5);
    Blk2Mcr(121, 6);
    Blk2Mcr(121, 7);
    Blk2Mcr(125, 0);
    Blk2Mcr(125, 1);
    Blk2Mcr(125, 3);
    Blk2Mcr(125, 5);
    Blk2Mcr(125, 7);
    Blk2Mcr(133, 0);
    Blk2Mcr(136, 1);
    Blk2Mcr(139, 0);
    Blk2Mcr(139, 1);
    Blk2Mcr(139, 7);
    Blk2Mcr(142, 0);
    Blk2Mcr(144, 1);
    Blk2Mcr(144, 2);
    Blk2Mcr(144, 4);
    Blk2Mcr(144, 6);
    // reused for the new shadows
    // Blk2Mcr(148, 1);
    // Blk2Mcr(150, 0);
    // Blk2Mcr(151, 0);
    // Blk2Mcr(151, 1);
    // Blk2Mcr(152, 0);
    // Blk2Mcr(153, 1);
    // Blk2Mcr(156, 0);
    // Blk2Mcr(158, 0);
    // Blk2Mcr(164, 0);
    // Blk2Mcr(164, 1);
    // Blk2Mcr(165, 1);
    // Blk2Mcr(250, 0);
    // Blk2Mcr(268, 0);
    // Blk2Mcr(268, 1);
    Blk2Mcr(251, 0);
    Blk2Mcr(251, 1);
    Blk2Mcr(265, 1);
    // Blk2Mcr(269, 0);
    // Blk2Mcr(269, 1);
    // Blk2Mcr(269, 2);
    // Blk2Mcr(269, 3);
    // Blk2Mcr(269, 4);
    // Blk2Mcr(269, 5);
    // Blk2Mcr(269, 6);
    // Blk2Mcr(269, 7);
    Blk2Mcr(365, 1);
    Blk2Mcr(395, 1);
    Blk2Mcr(513, 0);
    Blk2Mcr(517, 1);
    Blk2Mcr(519, 0);
    Blk2Mcr(520, 0);
    Blk2Mcr(520, 1);
    Blk2Mcr(521, 0);
    Blk2Mcr(521, 1);
    Blk2Mcr(522, 0);
    Blk2Mcr(522, 1);
    Blk2Mcr(523, 0);
    Blk2Mcr(523, 1);
    Blk2Mcr(524, 0);
    Blk2Mcr(524, 1);
    Blk2Mcr(525, 0);
    Blk2Mcr(525, 1);
    Blk2Mcr(526, 0);
    Blk2Mcr(526, 1);
    Blk2Mcr(527, 0);
    Blk2Mcr(527, 1);
    Blk2Mcr(528, 0);
    Blk2Mcr(528, 1);
    Blk2Mcr(529, 0);
    Blk2Mcr(529, 1);
    Blk2Mcr(529, 5);
    Blk2Mcr(529, 6);
    Blk2Mcr(529, 7);
    Blk2Mcr(530, 0);
    Blk2Mcr(530, 1);
    Blk2Mcr(530, 4);
    Blk2Mcr(530, 6);
    Blk2Mcr(530, 7);
    Blk2Mcr(532, 0);
    Blk2Mcr(532, 1);
    Blk2Mcr(532, 4);
    Blk2Mcr(532, 6);
    Blk2Mcr(532, 7);
    Blk2Mcr(534, 0);
    Blk2Mcr(534, 1);
    Blk2Mcr(534, 5);
    Blk2Mcr(534, 6);
    Blk2Mcr(534, 7);
    Blk2Mcr(535, 0);
    Blk2Mcr(535, 1);
    Blk2Mcr(535, 7);
    Blk2Mcr(542, 0);
    Blk2Mcr(542, 2);
    Blk2Mcr(542, 3);
    Blk2Mcr(542, 4);
    Blk2Mcr(542, 5);
    Blk2Mcr(542, 6);
    Blk2Mcr(542, 7);
    Blk2Mcr(543, 0);
    Blk2Mcr(543, 1);
    Blk2Mcr(543, 2);
    Blk2Mcr(543, 3);
    Blk2Mcr(543, 4);
    Blk2Mcr(543, 5);
    Blk2Mcr(543, 6);
    Blk2Mcr(543, 7);
    Blk2Mcr(545, 0);
    Blk2Mcr(545, 1);
    Blk2Mcr(545, 2);
    Blk2Mcr(545, 4);
    Blk2Mcr(547, 0);
    Blk2Mcr(547, 1);
    Blk2Mcr(547, 2);
    Blk2Mcr(547, 3);
    Blk2Mcr(547, 4);
    Blk2Mcr(547, 5);
    Blk2Mcr(547, 6);
    Blk2Mcr(547, 7);
    Blk2Mcr(548, 0);
    Blk2Mcr(548, 1);
    Blk2Mcr(548, 3);
    Blk2Mcr(548, 5);
    Blk2Mcr(554, 0);
    Blk2Mcr(554, 1);
    Blk2Mcr(555, 1);
    Blk2Mcr(556, 0);
    Blk2Mcr(556, 1);
    Blk2Mcr(557, 1);
    Blk2Mcr(558, 0);
    Blk2Mcr(558, 1);
    Blk2Mcr(558, 2);
    Blk2Mcr(558, 3);
    Blk2Mcr(558, 4);
    Blk2Mcr(558, 5);
    Blk2Mcr(558, 7);
    Blk2Mcr(559, 2);
    Blk2Mcr(559, 4);
    const int unusedSubtiles[] = {
        2, 7, 14, 19, 20, 56, 57, 58, 59, 70, 71, 106, 109, 110, 116, 117, 118, 120, 122, 123, 124, 126, 137, 140, 145, 149, 157, 159, 160, 168, 170, 171, 192, 193, 194, 195, 196, 197, 198, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 235, 236, 243, 246, 247, 255, 256, 264, 327, 328, 329, 330, 335, 336, 337, 338, 343, 344, 345, 346, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 366, 367, 368, 369, 370, 376, 391, 397, 400, 434, 487, 489, 491, 493, 504, 505, 507, 509, 511, 516, 518, 531, 533, 536, 541,
    };

    for (int n = 0; n < lengthof(unusedSubtiles); n++) {
        for (int i = 0; i < blockSize; i++) {
            Blk2Mcr(unusedSubtiles[n], i);
        }
    }
}

bool D1Tileset::patchCatacombsStairs(int backTileIndex1, int backTileIndex2, int extTileIndex1, int extTileIndex2, int stairsSubtileRef1, int stairsSubtileRef2, bool silent)
{
    constexpr unsigned blockSize = BLOCK_SIZE_L2;

    constexpr int backSubtileRef0 = 250;
    constexpr int backSubtileRef2 = 251;
    constexpr int backSubtileRef3 = 252;

    constexpr int ext1SubtileRef1 = 265;
    constexpr int ext2SubtileRef1 = 556;
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
        return false;
    }

    const unsigned microIndex0 = MICRO_IDX(blockSize, 0);
    const unsigned microIndex1 = MICRO_IDX(blockSize, 1);
    const unsigned microIndex2 = MICRO_IDX(blockSize, 2);
    const unsigned microIndex3 = MICRO_IDX(blockSize, 3);
    const unsigned microIndex4 = MICRO_IDX(blockSize, 4);
    const unsigned microIndex5 = MICRO_IDX(blockSize, 5);
    const unsigned microIndex6 = MICRO_IDX(blockSize, 6);

    unsigned back3_FrameRef0 = back3FrameReferences[microIndex0]; // 252[0]
    unsigned back2_FrameRef1 = back2FrameReferences[microIndex1]; // 251[1]
    unsigned back0_FrameRef0 = back0FrameReferences[microIndex0]; // 250[0]

    unsigned stairs_FrameRef0 = stairs1FrameReferences[microIndex0]; // 267[0]
    unsigned stairs_FrameRef2 = stairs1FrameReferences[microIndex2]; // 267[2]
    unsigned stairs_FrameRef4 = stairs1FrameReferences[microIndex4]; // 267[4]
    unsigned stairs_FrameRef6 = stairs1FrameReferences[microIndex6]; // 267[6]

    unsigned stairsExt_FrameRef1 = stairsExt1FrameReferences[microIndex1]; // 265[1]
    unsigned stairsExt_FrameRef3 = stairsExt1FrameReferences[microIndex3]; // 265[3]
    unsigned stairsExt_FrameRef5 = stairsExt1FrameReferences[microIndex5]; // 265[5]

    if (back3_FrameRef0 == 0 || back2_FrameRef1 == 0 || back0_FrameRef0 == 0) {
        dProgressErr() << QApplication::tr("The back-stairs tile (%1) has invalid (missing) frames.").arg(backTileIndex1 + 1);
        return false;
    }

    if (stairs_FrameRef0 == 0 || stairs_FrameRef2 == 0 || stairs_FrameRef4 == 0 || stairs_FrameRef6 == 0
        || stairsExt_FrameRef1 == 0 || stairsExt_FrameRef3 == 0 || stairsExt_FrameRef5 == 0) {
        if (!silent)
            dProgressWarn() << QApplication::tr("The stairs subtiles (%1) are already patched.").arg(stairsSubtileRef1);
        return false;
    }

    D1GfxFrame *back3_LeftFrame = this->gfx->getFrame(back3_FrameRef0 - 1);  // 252[0]
    D1GfxFrame *back2_LeftFrame = this->gfx->getFrame(back2_FrameRef1 - 1);  // 251[1]
    D1GfxFrame *back0_RightFrame = this->gfx->getFrame(back0_FrameRef0 - 1); // 250[0]

    D1GfxFrame *stairs_LeftFrame0 = this->gfx->getFrame(stairs_FrameRef0 - 1); // 267[0]
    D1GfxFrame *stairs_LeftFrame2 = this->gfx->getFrame(stairs_FrameRef2 - 1); // 267[2]
    D1GfxFrame *stairs_LeftFrame4 = this->gfx->getFrame(stairs_FrameRef4 - 1); // 267[4]
    D1GfxFrame *stairs_LeftFrame6 = this->gfx->getFrame(stairs_FrameRef6 - 1); // 267[6]

    D1GfxFrame *stairsExt_RightFrame1 = this->gfx->getFrame(stairsExt_FrameRef1 - 1); // 265[1]
    D1GfxFrame *stairsExt_RightFrame3 = this->gfx->getFrame(stairsExt_FrameRef3 - 1); // 265[3]
    D1GfxFrame *stairsExt_RightFrame5 = this->gfx->getFrame(stairsExt_FrameRef5 - 1); // 265[5]

    if (back3_LeftFrame->getWidth() != MICRO_WIDTH || back3_LeftFrame->getHeight() != MICRO_HEIGHT) {
        return false; // upscaled(?) frames -> assume it is already done
    }
    if ((back2_LeftFrame->getWidth() != MICRO_WIDTH || back2_LeftFrame->getHeight() != MICRO_HEIGHT)
        || (back0_RightFrame->getWidth() != MICRO_WIDTH || back0_RightFrame->getHeight() != MICRO_HEIGHT)) {
        dProgressErr() << QApplication::tr("The back-stairs tile (%1) has invalid (mismatching) frames.").arg(backTileIndex1 + 1);
        return false;
    }
    if ((stairs_LeftFrame0->getWidth() != MICRO_WIDTH || stairs_LeftFrame0->getHeight() != MICRO_HEIGHT)
        || (stairs_LeftFrame2->getWidth() != MICRO_WIDTH || stairs_LeftFrame2->getHeight() != MICRO_HEIGHT)
        || (stairs_LeftFrame4->getWidth() != MICRO_WIDTH || stairs_LeftFrame4->getHeight() != MICRO_HEIGHT)
        || (stairs_LeftFrame6->getWidth() != MICRO_WIDTH || stairs_LeftFrame6->getHeight() != MICRO_HEIGHT)) {
        dProgressErr() << QApplication::tr("The stairs subtile (%1) has invalid (mismatching) frames I.").arg(stairsSubtileRef1);
        return false;
    }
    if ((stairsExt_RightFrame1->getWidth() != MICRO_WIDTH || stairsExt_RightFrame1->getHeight() != MICRO_HEIGHT)
        || (stairsExt_RightFrame3->getWidth() != MICRO_WIDTH || stairsExt_RightFrame3->getHeight() != MICRO_HEIGHT)
        || (stairsExt_RightFrame5->getWidth() != MICRO_WIDTH || stairsExt_RightFrame5->getHeight() != MICRO_HEIGHT)) {
        dProgressErr() << QApplication::tr("The stairs subtile (%1) has invalid (mismatching) frames II.").arg(ext1SubtileRef1);
        return false;
    }

    // move external-stairs
    // - from 265[1] to the right side of 252[0]
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
            D1GfxPixel pixel = stairsExt_RightFrame1->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            if (!back2_LeftFrame->getPixel(x, y).isTransparent()) // use 251[1] as a mask
                continue;
            back3_LeftFrame->setPixel(x, y + MICRO_HEIGHT / 2, pixel);             // 252[0]
            stairsExt_RightFrame1->setPixel(x, y, D1GfxPixel::transparentPixel()); // 265[1]
        }
    }
    // - from 265[3] to the right side of 252[0]
    //  and shift the external-stairs down
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel0 = stairsExt_RightFrame3->getPixel(x, y);
            if (!pixel0.isTransparent()) {
                back3_LeftFrame->setPixel(x, y - MICRO_HEIGHT / 2, pixel0); // 252[0]
            }
            D1GfxPixel pixel1 = stairsExt_RightFrame3->getPixel(x, y - MICRO_HEIGHT / 2);
            stairsExt_RightFrame3->setPixel(x, y, pixel1); // 265[3]
        }
    }
    // shift the rest of the external-stairs down
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel0 = stairsExt_RightFrame5->getPixel(x, y);
            stairsExt_RightFrame3->setPixel(x, y - MICRO_HEIGHT / 2, pixel0); // 265[3]
            D1GfxPixel pixel1 = stairsExt_RightFrame5->getPixel(x, y - MICRO_HEIGHT / 2);
            stairsExt_RightFrame5->setPixel(x, y, pixel1); // 265[5]
        }
    }

    // move external-stairs
    // move stairs from 267[0] to 265[5]
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
            D1GfxPixel pixel = stairs_LeftFrame0->getPixel(x, y);
            if (!back0_RightFrame->getPixel(x, y).isTransparent()) // use 250[0] as a mask
                continue;
            stairsExt_RightFrame5->setPixel(x, y + MICRO_HEIGHT / 2, pixel);   // 265[5]
            stairs_LeftFrame0->setPixel(x, y, D1GfxPixel::transparentPixel()); // 267[0]
        }
    }
    // shift the stairs down
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel0 = stairs_LeftFrame2->getPixel(x, y);
            stairsExt_RightFrame5->setPixel(x, y - MICRO_HEIGHT / 2, pixel0); // 265[5]
            D1GfxPixel pixel1 = stairs_LeftFrame2->getPixel(x, y - MICRO_HEIGHT / 2);
            stairs_LeftFrame2->setPixel(x, y, pixel1); // 267[2]
        }
    }
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel0 = stairs_LeftFrame4->getPixel(x, y);
            stairs_LeftFrame2->setPixel(x, y - MICRO_HEIGHT / 2, pixel0); // 267[2]
            D1GfxPixel pixel1 = stairs_LeftFrame4->getPixel(x, y - MICRO_HEIGHT / 2);
            stairs_LeftFrame4->setPixel(x, y, pixel1); // 267[4]
        }
    }
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel0 = stairs_LeftFrame6->getPixel(x, y);
            stairs_LeftFrame4->setPixel(x, y - MICRO_HEIGHT / 2, pixel0); // 267[4]
            D1GfxPixel pixel1 = stairs_LeftFrame6->getPixel(x, y - MICRO_HEIGHT / 2);
            stairs_LeftFrame6->setPixel(x, y, pixel1); // 267[6]
        }
    }
    for (int x = 0; x < MICRO_WIDTH; x++) {
        for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
            stairs_LeftFrame6->setPixel(x, y, D1GfxPixel::transparentPixel()); // 267[6]
        }
    }

    // copy shadow
    // - from 251[1] to 252[0]
    for (int x = 22; x < MICRO_WIDTH; x++) {
        for (int y = MICRO_HEIGHT / 2; y < 20; y++) {
            D1GfxPixel pixel = back2_LeftFrame->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            if (pixel.getPaletteIndex() % 16 < 11)
                continue;
            if (!back3_LeftFrame->getPixel(x, y - MICRO_HEIGHT / 2).isTransparent())
                continue;
            back3_LeftFrame->setPixel(x, y - MICRO_HEIGHT / 2, pixel); // 252[0]
        }
    }
    // - from 251[1] to 265[3]
    for (int x = 22; x < MICRO_WIDTH; x++) {
        for (int y = 12; y < MICRO_HEIGHT / 2; y++) {
            D1GfxPixel pixel = back2_LeftFrame->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            if (pixel.getPaletteIndex() % 16 < 11)
                continue;
            if (!stairsExt_RightFrame3->getPixel(x, y + MICRO_HEIGHT / 2).isTransparent())
                continue;
            stairsExt_RightFrame3->setPixel(x, y + MICRO_HEIGHT / 2, pixel); // 265[3]
        }
    }
    // - from 250[0] to 265[3]
    for (int x = 22; x < MICRO_WIDTH; x++) {
        for (int y = 20; y < MICRO_HEIGHT; y++) {
            D1GfxPixel pixel = back0_RightFrame->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            if (pixel.getPaletteIndex() % 16 < 11)
                continue;
            if (!stairsExt_RightFrame3->getPixel(x, y).isTransparent())
                continue;
            stairsExt_RightFrame3->setPixel(x, y, pixel); // 265[3]
        }
    }

    // complete stairs-micro to square
    stairs_LeftFrame2->setPixel(0, 0, D1GfxPixel::colorPixel(40)); // 267[2]

    // fix bad artifacts
    stairsExt_RightFrame3->setPixel(23, 20, D1GfxPixel::transparentPixel()); // 265[3]
    stairsExt_RightFrame3->setPixel(24, 20, D1GfxPixel::transparentPixel()); // 265[3]
    stairsExt_RightFrame3->setPixel(22, 21, D1GfxPixel::transparentPixel()); // 265[3]
    stairsExt_RightFrame3->setPixel(23, 21, D1GfxPixel::transparentPixel()); // 265[3]
    stairsExt_RightFrame3->setPixel(22, 30, D1GfxPixel::colorPixel(78));     // 265[3]

    stairs_LeftFrame4->setPixel(5, 22, D1GfxPixel::colorPixel(55)); // 267[4]
    stairs_LeftFrame4->setPixel(19, 7, D1GfxPixel::colorPixel(71)); // 267[4]
    stairs_LeftFrame4->setPixel(19, 9, D1GfxPixel::colorPixel(36)); // 267[4]

    back3_LeftFrame->setPixel(22, 4, D1GfxPixel::colorPixel(78)); // 252[0]
    back3_LeftFrame->setPixel(23, 4, D1GfxPixel::colorPixel(31)); // 252[0]

    // adjust the frame types
    D1CelTilesetFrame::selectFrameType(stairs_LeftFrame0);     // 267[0]
    D1CelTilesetFrame::selectFrameType(stairs_LeftFrame2);     // 267[2]
    D1CelTilesetFrame::selectFrameType(stairsExt_RightFrame5); // 265[5]
    D1CelTilesetFrame::selectFrameType(stairsExt_RightFrame1); // 265[1]
    D1CelTilesetFrame::selectFrameType(back3_LeftFrame);       // 252[0]

    this->gfx->setModified();

    if (!silent) {
        dProgress() << QApplication::tr("The back-stair tiles (%1, %2) and the stair-subtiles (%2, %3, %4, %5) are modified.").arg(backTileIndex1 + 1).arg(backTileIndex2 + 1).arg(extTileIndex1 + 1).arg(extTileIndex2 + 1).arg(stairsSubtileRef1).arg(stairsSubtileRef2);
    }
    return true;
}

static BYTE shadowColorCaves(BYTE color)
{
    // assert(color < 128);
    if (color == 0) {
        return 0;
    }
    switch (color % 16) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        return (color & ~15) + 13;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        return (color & ~15) + 14;
    case 11:
    case 12:
    case 13:
        return (color & ~15) + 15;
    }
    return 0;
}
static bool shadowColorCaves(D1GfxFrame* frame, int x, int y)
{
    D1GfxPixel pixel = frame->getPixel(x, y);
    quint8 color = pixel.getPaletteIndex();
    return frame->setPixel(x, y, D1GfxPixel::colorPixel(shadowColorCaves(color)));
}
bool D1Tileset::patchCavesFloor(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{ 540 - 1, 3, D1CEL_FRAME_TYPE::Empty },             // used to block subsequent calls
/*  1 */{ /*537*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },         // fix/mask door
/*  2 */{ /*537*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*  3 */{ 538 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  4 */{ 538 - 1, 1, D1CEL_FRAME_TYPE::Empty },             // unused
/*  5 */{ 538 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/*  6 */{ 538 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/*  7 */{ 539 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  8 */{ 540 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  9 */{ 540 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 10 */{ 541 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 11 */{ 541 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 12 */{ 541 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 13 */{ 541 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 14 */{ 542 - 1, 1, D1CEL_FRAME_TYPE::Empty },            // unused

/* 15 */{ 197 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },    // change type
/* 16 */{ 501 - 1, 0, D1CEL_FRAME_TYPE::Empty },            // unused
/* 17 */{ 494 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 18 */{ 496 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 19 */{ 495 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 20 */{ 100 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },     // one-subtile islands
/* 21 */{ 126 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 22 */{  83 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 23 */{ 544 - 1, 1, D1CEL_FRAME_TYPE::Empty },            // new shadows
/* 24 */{ 202 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 25 */{ 551 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 26 */{ 506 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 27 */{ 543 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 28 */{ 528 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },

/* 29 */{ 543 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 30 */{ 506 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 31 */{ 528 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 32 */{ 476 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // fix shadows
/* 33 */{ 484 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 34 */{ 490 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 35 */{ 492 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 36 */{ 492 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 37 */{ 497 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 38 */{ 499 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 39 */{ 499 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 40 */{ 500 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 41 */{ 500 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 42 */{ 502 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 43 */{ 502 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 44 */{ 535 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 45 */{ 535 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 46 */{ 542 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 47 */{ 542 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 48 */{ 493 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 49 */{ 493 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 50 */{ 501 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 51 */{ 504 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 52 */{ 504 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 53 */{ 479 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 54 */{ 479 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 55 */{ 488 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 56 */{ 517 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 57 */{ 517 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 58 */{ 516 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 59 */{ 507 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 60 */{ 500 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 61 */{ 473 - 1, 0, D1CEL_FRAME_TYPE::LeftTrapezoid },
/* 62 */{ 544 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 63 */{ 435 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 64 */{  33 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 65 */{ 449 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 66 */{ 441 - 1, 2, D1CEL_FRAME_TYPE::Square },
/* 67 */{  32 - 1, 0, D1CEL_FRAME_TYPE::LeftTrapezoid },
/* 68 */{ 457 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 69 */{ 469 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 70 */{  49 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },

/* 71 */{  387 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 72 */{  387 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 73 */{  238 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 74 */{  238 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 75 */{  242 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },

/* 76 */{  210 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 77 */{  210 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 78 */{  233 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },

/* 79 */{  25 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // improve connections
/* 80 */{  26 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 81 */{  26 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 82 */{ 205 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 83 */{ 385 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 84 */{ 385 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 85 */{ 386 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 86 */{ 387 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 87 */{ 388 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 88 */{ 390 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 89 */{ 395 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 90 */{ 395 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 91 */{ 511 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/* 92 */{ 171 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
    };

    constexpr unsigned blockSize = BLOCK_SIZE_L3;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // add shadow 537[1]
        /*if (i == 2) {
            for (int x = 2; x < 6; x++) {
                for (int y = 26; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && (y != 26 || x == 2)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(shadowColorCaves(color))); // 537[1]
                    }
                }
            }
        }*/

        // remove door 538[0]
        if (i == 3) {
            for (int x = 21; x < 30; x++) {
                for (int y = 0; y < 9; y++) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // remove door 538[2]
        if (i == 5) {
            for (int x = 21; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // remove door 538[4]
        if (i == 6) {
            for (int x = 21; x < MICRO_WIDTH; x++) {
                for (int y = 4; y < MICRO_HEIGHT; y++) {
                    if (y < 8 && x < 24) {
                        continue;
                    }
                    if (y == 8 && x == 21) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // remove door 540[1]
        if (i == 9) {
            for (int x = 7; x < MICRO_WIDTH; x++) {
                for (int y = 13; y < 21; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (y > 9 + (x + 1) / 2) {
                        continue;
                    }
                    if (!pixel.isTransparent() && (color % 16) < 13) {
                        change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(shadowColorCaves(color)));
                    }
                }
            }
        }
        // remove door 541[0]
        if (i == 10) {
            for (int x = 22; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < 6; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && (color % 16) < 13) {
                        change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(shadowColorCaves(color)));
                    }
                }
            }
        }
        // remove door 541[1]
        if (i == 11) {
            for (int x = 0; x < 11; x++) {
                for (int y = 0; y < 10; y++) {
                    if (x < 4) {
                        D1GfxPixel pixel = frame->getPixel(x, y);
                        quint8 color = pixel.getPaletteIndex();
                        if (!pixel.isTransparent() && (color % 16) < 13) {
                            change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(shadowColorCaves(color)));
                        }
                    } else {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // remove door 541[3]
        if (i == 12) {
            for (int x = 0; x < 11; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // remove door 541[5]
        if (i == 13) {
            for (int x = 0; x < 11; x++) {
                for (int y = 5; y < MICRO_HEIGHT; y++) {
                    if (y < 7 && x > 7) {
                        continue;
                    }
                    if (y == 7 && x > 8) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // fix shadow 496[0] using 494[0]
        if (i == 18) {
            const CelMicro &microSrc = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 15; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < 15; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 494[0]
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && (color == 0 || color == 47 || color == 60 || color == 93 || color == 125)) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // create new shadow 551[1] using 202[1] and 544[1]
        if (i == 25) {
            const CelMicro &microSrc1 = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[i - 2];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel1 = frameSrc1->getPixel(x, y); // 202[1]
                    D1GfxPixel pixel2 = frameSrc2->getPixel(x, y); // 544[1]
                    if (pixel1.isTransparent()) {
                        continue;
                    }
                    quint8 color1 = pixel1.getPaletteIndex();
                    quint8 color2 = pixel2.getPaletteIndex();
                    quint8 color;
                    if (x < 15 && y > 23) {
                        color = color1;
                        if (x == 15 && y == 24) {
                            color = shadowColorCaves(color);
                        }
                    } else if (x < 5 && y < 10 - x) {
                        color = color1;
                        //} else if (x < 3 && y > 11 && y < 15 && (x != 2 || y != 12)) {
                    } else if (x < 9 && y > 8 && y < 15 && y > 8 + x / 2) {
                        color = color1;
                    } else if (x < 4 && y > 16 && y < 24 && y > 16 + 2 * x) {
                        color = color1;
                    } else if ((y == 20 && x >= 16 && x <= 19) || (y == 21 && x >= 12 && x <= 17)
                        || (y == 22 && x >= 11 && x <= 15) || (y == 23 && x >= 8 && x <= 14)) {
                        color = color1;
                    } else {
                        color = color2;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(color));
                }
            }
        }
        // add missing leg to 543[0] using 506[0]
        if (i == 27) {
            const CelMicro &microSrc = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < 14; y++) {
                    if (x > 26 && y < 8 && y < x - 22) {
                        continue;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 506[0]
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }

        // create new shadow 528[0] using 506[0] and 543[0]
        if (i == 28) {
            const CelMicro &microSrc1 = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[i - 2];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (x > 26 && y < 8 && y < x - 22) {
                        pixel = frameSrc1->getPixel(x, y); // 543[0]
                    } else {
                        pixel = frameSrc2->getPixel(x, y); // 506[0]
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }

        // create new shadow 528[1] using 506[1] and 543[1]
        if (i == 31) {
            const CelMicro &microSrc1 = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[i - 2];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y > 25) {
                        pixel = frameSrc1->getPixel(x, y); // 506[1]
                    } else {
                        pixel = frameSrc2->getPixel(x, y); // 543[1]
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // fix shadow of 497[0]
        if (i == 37) {
            for (int x = 20; x < MICRO_WIDTH; x++) {
                for (int y = 21; y < 29; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (y > 10 + x / 2 && !pixel.isTransparent() && (color % 16) < 13) {
                        change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(shadowColorCaves(color))); // 497[0]
                    }
                }
            }
        }
        // fix shadow of 500[1]
        if (i == 41) {
            for (int x = 0; x < 20; x++) {
                for (int y = 11; y < 24; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    // reduce bottom-shadow
                    if (x < 13 && y > 14 + x / 2 && (color == 0 || color == 60 || color == 122 || color == 123 || color == 125)) {
                        pixel = frame->getPixel(x, y + (6 - 2 * (y - (15 + x / 2)))); // 500[1]
                        change |= frame->setPixel(x, y, pixel);
                    }
                    // add top-shadow
                    if (x > 15 && y > 2 * x - 22 && !pixel.isTransparent() && (color % 16) < 13) {
                        change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(shadowColorCaves(color)));
                    }
                }
            }
        }

        // fix shadow of 542[0]
        if (i == 46) {
            for (int x = 14; x < 25; x++) {
                for (int y = 9; y < 15; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (y < 22 - (x + 1) / 2 && !pixel.isTransparent() && (color % 16) < 13) {
                        change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(shadowColorCaves(color))); // 542[0]
                    }
                }
            }
        }

        // fix 238[0], 238[1] using 387[0], 387[1]
        if (i == 73 || i == 74) {
            const CelMicro &microSrc = micros[i - 2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < 24) {
                        D1GfxPixel pixel = frameSrc->getPixel(x, y); // 387
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        // extend shadow of 242[0]
        if (i == 75) {
            for (int x = 0; x < 12; x++) {
                for (int y = 0; y < 19; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && (color % 16) < 13) {
                        change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(shadowColorCaves(color))); // 242[0]
                    }
                }
            }
        }

        // fix artifacts
        /*if (i == 1) { // 537[0]
            change |= frame->setPixel(30, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 0, D1GfxPixel::transparentPixel());
        }*/
        if (i == 3) { // 538[0]
            change |= frame->setPixel(30, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 0, D1GfxPixel::transparentPixel());
        }
        if (i == 7) { // 539[0]
            // - trunk color
            change |= frame->setPixel(25, 0, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(25, 1, D1GfxPixel::colorPixel(93));
            change |= frame->setPixel(25, 2, D1GfxPixel::colorPixel(124));
            // - overdraw
            change |= frame->setPixel(20, 6, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(19, 7, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(20, 7, D1GfxPixel::colorPixel(124));
            change |= frame->setPixel(21, 7, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(22, 7, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(22, 8, D1GfxPixel::colorPixel(124));
        }
        if (i == 8) { // 540[0]
            change |= shadowColorCaves(frame, 14, 21);
            change |= shadowColorCaves(frame, 15, 20);
            change |= shadowColorCaves(frame, 15, 19);

            change |= frame->setPixel(13, 20, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(13, 21, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(14, 21, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(14, 16, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(14, 15, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(15, 15, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(14, 14, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(15, 14, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(16, 14, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(17, 14, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(15, 13, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(16, 13, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(17, 13, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(17, 12, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(18, 12, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(18, 13, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(19, 13, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(20, 13, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(18, 14, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(19, 14, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(20, 14, D1GfxPixel::colorPixel(91));
            change |= frame->setPixel(21, 14, D1GfxPixel::colorPixel(118));
            change |= frame->setPixel(20, 15, D1GfxPixel::colorPixel(93));
            change |= frame->setPixel(21, 15, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(22, 15, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(23, 15, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(22, 16, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(24, 16, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(25, 15, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(26, 15, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(27, 15, D1GfxPixel::colorPixel(89));
            change |= frame->setPixel(25, 14, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(28, 14, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(29, 14, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(27, 13, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(28, 13, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(29, 13, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(30, 13, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(31, 13, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(30, 12, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(31, 12, D1GfxPixel::colorPixel(70));
        }
        if (i == 9) { // 540[1]
            change |= frame->setPixel(19, 21, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(20, 21, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(21, 21, D1GfxPixel::colorPixel(75));
        }
        if (i == 10) { // 541[0]
            change |= frame->setPixel(22, 6, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(23, 6, D1GfxPixel::colorPixel(74));
        }
        if (i == 11) { // 541[1]
            change |= frame->setPixel(0, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(1, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(2, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(2, 1, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(3, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(3, 1, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(5, 14, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(6, 14, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(5, 15, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(6, 15, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(7, 15, D1GfxPixel::colorPixel(119));
        }
        if (i == 18) { // 496[0]
            change |= shadowColorCaves(frame, 14, 13);
            change |= shadowColorCaves(frame, 14, 12);
            change |= shadowColorCaves(frame, 15, 12);
            change |= shadowColorCaves(frame, 16, 12);
            change |= shadowColorCaves(frame, 14, 11);
            change |= shadowColorCaves(frame, 15, 11);
            change |= shadowColorCaves(frame, 16, 11);
            change |= shadowColorCaves(frame, 17, 11);
            change |= shadowColorCaves(frame, 18, 11);
            change |= shadowColorCaves(frame, 17, 10);
            change |= shadowColorCaves(frame, 18, 10);
            change |= shadowColorCaves(frame, 19, 10);
            change |= shadowColorCaves(frame, 20, 10);
            change |= shadowColorCaves(frame, 21, 9);
            change |= shadowColorCaves(frame, 22, 9);
        }
        if (i == 19) { // 495[1]
            change |= frame->setPixel(18, 21, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(19, 21, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(20, 21, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(14, 22, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(15, 22, D1GfxPixel::colorPixel(83));
            change |= frame->setPixel(16, 22, D1GfxPixel::colorPixel(118));
            change |= frame->setPixel(17, 22, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(18, 22, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(19, 22, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(14, 23, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(15, 23, D1GfxPixel::colorPixel(74));
            change |= frame->setPixel(16, 23, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(17, 23, D1GfxPixel::colorPixel(89));
            change |= frame->setPixel(14, 24, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(15, 24, D1GfxPixel::colorPixel(91));

            change |= frame->setPixel(0, 9, D1GfxPixel::colorPixel(116));
            change |= frame->setPixel(1, 9, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(2, 9, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(3, 9, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(0, 10, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(1, 10, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(2, 10, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(3, 10, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(0, 11, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(1, 11, D1GfxPixel::colorPixel(75));

            change |= frame->setPixel(0, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 12, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(2, 11, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(3, 11, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(4, 11, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(5, 10, D1GfxPixel::colorPixel(125));
            // eliminate ugly leftovers(?)
            change |= frame->setPixel(31, 16, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(30, 16, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(29, 16, D1GfxPixel::colorPixel(118));
            change |= frame->setPixel(29, 15, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(28, 16, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(28, 15, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(27, 15, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(27, 14, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(26, 15, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(26, 14, D1GfxPixel::colorPixel(118));
            change |= frame->setPixel(25, 14, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(25, 13, D1GfxPixel::colorPixel(116));
            change |= frame->setPixel(24, 14, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(24, 13, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(23, 13, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(23, 12, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(22, 12, D1GfxPixel::colorPixel(119));

            change |= frame->setPixel(20, 21, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(22, 20, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(21, 21, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(23, 20, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(25, 19, D1GfxPixel::colorPixel(120));
        }
        if (i == 20) { // 100[0]
            change |= frame->setPixel(14, 23, D1GfxPixel::colorPixel(119));
        }
        if (i == 21) { // 126[0]
            change |= frame->setPixel(12, 22, D1GfxPixel::colorPixel(13));
            change |= frame->setPixel(13, 22, D1GfxPixel::colorPixel(86));
            change |= frame->setPixel(18, 25, D1GfxPixel::colorPixel(15));
            change |= frame->setPixel(16, 24, D1GfxPixel::colorPixel(61));
        }
        if (i == 22) { // 83[1]
            change |= frame->setPixel(11, 8, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(12, 8, D1GfxPixel::colorPixel(25));
        }
        if (i == 25) { // 551[1]
            change |= frame->setPixel(18, 21, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(19, 21, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(16, 22, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(17, 22, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(18, 22, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(15, 23, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(16, 23, D1GfxPixel::colorPixel(73));
        }
        if (i == 32) { // 476[1]
            change |= shadowColorCaves(frame, 26, 15);
            change |= shadowColorCaves(frame, 27, 16);
            change |= shadowColorCaves(frame, 28, 16);
            change |= shadowColorCaves(frame, 29, 16);
            change |= shadowColorCaves(frame, 29, 17);
            change |= shadowColorCaves(frame, 24, 17);
            change |= shadowColorCaves(frame, 25, 18);
            change |= shadowColorCaves(frame, 25, 19);

            change |= frame->setPixel(0, 10, D1GfxPixel::colorPixel(73)); // make it reusable in 484
        }
        if (i == 33) { // 484[0] - after reuse
            change |= frame->setPixel(30, 9, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(31, 9, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(31, 10, D1GfxPixel::colorPixel(68));
        }
        if (i == 34) { // 490[0]
            change |= shadowColorCaves(frame, 18, 25);
            change |= shadowColorCaves(frame, 19, 25);
            change |= shadowColorCaves(frame, 20, 26);
            change |= shadowColorCaves(frame, 21, 26);
            change |= shadowColorCaves(frame, 22, 27);
            change |= shadowColorCaves(frame, 23, 27);
            change |= shadowColorCaves(frame, 24, 28);
            change |= shadowColorCaves(frame, 26, 29);

            change |= frame->setPixel(10, 15, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(10, 16, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(11, 15, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(11, 16, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(12, 14, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(13, 14, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(12, 15, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(13, 15, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(14, 15, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(15, 15, D1GfxPixel::colorPixel(113));
            change |= frame->setPixel(12, 16, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(13, 16, D1GfxPixel::colorPixel(65));
            change |= frame->setPixel(14, 16, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(15, 16, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(16, 16, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(17, 16, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(12, 17, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(13, 17, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(14, 17, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(15, 17, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(16, 17, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(17, 17, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(18, 17, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(17, 18, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(18, 18, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(19, 18, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(18, 19, D1GfxPixel::colorPixel(65));
            change |= frame->setPixel(19, 19, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(20, 19, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(21, 19, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(20, 20, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(21, 20, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(22, 20, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(23, 20, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(24, 20, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(25, 20, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(22, 21, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(23, 21, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(24, 21, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(25, 21, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(26, 21, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(27, 21, D1GfxPixel::colorPixel(65));
            change |= frame->setPixel(25, 22, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(26, 22, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(27, 22, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(28, 22, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(27, 23, D1GfxPixel::colorPixel(65));
            change |= frame->setPixel(28, 23, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(29, 23, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(31, 24, D1GfxPixel::colorPixel(70));
        }
        if (i == 35) { // 492[0]
            change |= frame->setPixel(27, 4, D1GfxPixel::colorPixel(121));

            change |= frame->setPixel(28, 2, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(28, 3, D1GfxPixel::colorPixel(119));

            change |= frame->setPixel(30, 3, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(31, 3, D1GfxPixel::colorPixel(70));
        }
        if (i == 36) { // 492[1]
            change |= frame->setPixel(3, 4, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(4, 4, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(4, 5, D1GfxPixel::colorPixel(125));

            change |= shadowColorCaves(frame, 25, 18);
            change |= shadowColorCaves(frame, 25, 19);

            change |= frame->setPixel(16, 9, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(17, 9, D1GfxPixel::colorPixel(127));
            change |= shadowColorCaves(frame, 17, 10);
            change |= shadowColorCaves(frame, 18, 10);
            change |= shadowColorCaves(frame, 19, 10);
            change |= shadowColorCaves(frame, 19, 11);
            change |= shadowColorCaves(frame, 20, 11);
            change |= shadowColorCaves(frame, 21, 11);
            change |= shadowColorCaves(frame, 20, 12);
            change |= shadowColorCaves(frame, 21, 12);
            change |= shadowColorCaves(frame, 22, 12);
            change |= shadowColorCaves(frame, 23, 12);
            change |= shadowColorCaves(frame, 22, 13);
            change |= shadowColorCaves(frame, 23, 13);
            change |= shadowColorCaves(frame, 24, 13);
            change |= shadowColorCaves(frame, 25, 13);
            change |= shadowColorCaves(frame, 24, 14);
            change |= shadowColorCaves(frame, 25, 14);
            change |= shadowColorCaves(frame, 26, 14);
            change |= shadowColorCaves(frame, 27, 14);
            change |= shadowColorCaves(frame, 26, 15);
            change |= shadowColorCaves(frame, 27, 15);
            change |= shadowColorCaves(frame, 28, 15);
            change |= shadowColorCaves(frame, 27, 16);
            change |= shadowColorCaves(frame, 28, 16);
            change |= shadowColorCaves(frame, 29, 16);
            change |= shadowColorCaves(frame, 29, 17);

            change |= frame->setPixel(16, 11, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(17, 11, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(17, 12, D1GfxPixel::colorPixel(67));
        }
        if (i == 38) { // 499[0]
            change |= frame->setPixel(28, 2, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(29, 2, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(29, 3, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(30, 3, D1GfxPixel::colorPixel(71));

            change |= frame->setPixel(31, 4, D1GfxPixel::colorPixel(70));
        }
        if (i == 39) { // 499[1]
            change |= shadowColorCaves(frame, 0, 10);
            change |= shadowColorCaves(frame, 1, 10);
            change |= shadowColorCaves(frame, 4, 11);
            change |= shadowColorCaves(frame, 8, 12);
            change |= shadowColorCaves(frame, 20, 11);
            change |= shadowColorCaves(frame, 21, 11);
        }
        if (i == 40) { // 500[0]
            change |= frame->setPixel(27, 13, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(28, 14, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(29, 14, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(30, 14, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(30, 15, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(31, 15, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(31, 16, D1GfxPixel::colorPixel(70));

            change |= frame->setPixel(25, 15, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(25, 16, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(25, 17, D1GfxPixel::colorPixel(70));

            change |= frame->setPixel(26, 16, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(26, 17, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(26, 18, D1GfxPixel::colorPixel(70));

            change |= frame->setPixel(27, 17, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(27, 18, D1GfxPixel::colorPixel(70));

            change |= frame->setPixel(28, 17, D1GfxPixel::colorPixel(71));
        }
        if (i == 41) { // 500[1]
            change |= shadowColorCaves(frame, 5, 12);
            change |= shadowColorCaves(frame, 6, 13);

            change |= frame->setPixel(7, 11, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(7, 12, D1GfxPixel::colorPixel(71));

            change |= frame->setPixel(12, 22, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(12, 21, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(13, 20, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(13, 21, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(13, 22, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(14, 22, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(14, 23, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(15, 23, D1GfxPixel::colorPixel(73));

            change |= frame->setPixel(25, 16, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(25, 17, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(26, 16, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(26, 17, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(26, 18, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(27, 17, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(27, 18, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(28, 17, D1GfxPixel::colorPixel(73));

            change |= frame->setPixel(0, 17, D1GfxPixel::colorPixel(117));
        }
        if (i == 42) { // 502[0]
            change |= frame->setPixel(3, 17, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(4, 17, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(5, 17, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(4, 18, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(5, 18, D1GfxPixel::colorPixel(0));

            change |= frame->setPixel(23, 5, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(29, 8, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(31, 7, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(25, 10, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(26, 10, D1GfxPixel::colorPixel(71));
        }
        if (i == 43) { // 502[1]
            change |= frame->setPixel(4, 5, D1GfxPixel::colorPixel(124));
            change |= frame->setPixel(1, 6, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(3, 6, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(2, 6, D1GfxPixel::colorPixel(117));
        }
        if (i == 44) { // 535[0]
            change |= frame->setPixel(14, 20, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(15, 20, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(15, 19, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(16, 19, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(17, 19, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(15, 18, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(16, 18, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(17, 18, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(18, 18, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(19, 18, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(20, 17, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(21, 17, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(22, 16, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(23, 16, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(24, 15, D1GfxPixel::colorPixel(73));

            change |= frame->setPixel(24, 11, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(24, 10, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(24, 9, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(25, 9, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(26, 8, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(27, 8, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(28, 8, D1GfxPixel::colorPixel(71));

            change |= frame->setPixel(30, 9, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(31, 9, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(31, 10, D1GfxPixel::colorPixel(122));
        }
        if (i == 45) { // 535[1]
            change |= frame->setPixel(0, 9, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(1, 9, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(2, 9, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(3, 9, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(0, 10, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(1, 10, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(2, 10, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(3, 10, D1GfxPixel::colorPixel(69));

            change |= frame->setPixel(0, 11, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(1, 11, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(2, 11, D1GfxPixel::colorPixel(70));

            change |= frame->setPixel(0, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 12, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(2, 12, D1GfxPixel::colorPixel(91));
            change |= frame->setPixel(3, 12, D1GfxPixel::colorPixel(125));

            change |= frame->setPixel(14, 24, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(14, 23, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(15, 23, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(16, 23, D1GfxPixel::colorPixel(70));

            change |= frame->setPixel(18, 22, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(19, 22, D1GfxPixel::colorPixel(69));

            change |= shadowColorCaves(frame, 21, 13);
            change |= shadowColorCaves(frame, 22, 13);
            change |= shadowColorCaves(frame, 22, 14);
            change |= shadowColorCaves(frame, 22, 15);
            change |= shadowColorCaves(frame, 23, 15);
            change |= shadowColorCaves(frame, 23, 16);
            change |= shadowColorCaves(frame, 24, 16);
            change |= shadowColorCaves(frame, 24, 17);
            change |= shadowColorCaves(frame, 24, 18);
            change |= shadowColorCaves(frame, 24, 19);
            change |= shadowColorCaves(frame, 23, 20);
        }
        if (i == 46) { // 542[0]
            change |= shadowColorCaves(frame, 14, 15);
            change |= shadowColorCaves(frame, 15, 14);
            change |= shadowColorCaves(frame, 17, 13);
        }
        if (i == 47) { // 542[2]
            change |= frame->setPixel(21, 28, D1GfxPixel::colorPixel(124));
        }
        if (i == 48) { // 493[0]
            change |= frame->setPixel(13, 20, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(13, 21, D1GfxPixel::colorPixel(106));
            change |= frame->setPixel(13, 22, D1GfxPixel::colorPixel(89));

            change |= frame->setPixel(14, 19, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(14, 20, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(14, 21, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(15, 18, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(15, 19, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(15, 20, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(15, 21, D1GfxPixel::colorPixel(121));

            change |= frame->setPixel(16, 20, D1GfxPixel::colorPixel(90));
            change |= frame->setPixel(17, 20, D1GfxPixel::colorPixel(123));

            change |= frame->setPixel(16, 19, D1GfxPixel::colorPixel(88));
            change |= frame->setPixel(17, 19, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(18, 19, D1GfxPixel::colorPixel(120));

            change |= frame->setPixel(16, 18, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(17, 18, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(18, 18, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(19, 18, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(20, 18, D1GfxPixel::colorPixel(72));

            change |= frame->setPixel(16, 17, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(17, 17, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(18, 17, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(19, 17, D1GfxPixel::colorPixel(74));
            change |= frame->setPixel(20, 17, D1GfxPixel::colorPixel(74));
            change |= frame->setPixel(21, 17, D1GfxPixel::colorPixel(88));
            change |= frame->setPixel(22, 17, D1GfxPixel::colorPixel(90));

            change |= frame->setPixel(16, 16, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(17, 16, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(18, 16, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(19, 16, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(20, 16, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(21, 16, D1GfxPixel::colorPixel(74));
            change |= frame->setPixel(22, 16, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(23, 16, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(24, 16, D1GfxPixel::colorPixel(90));
            change |= frame->setPixel(25, 16, D1GfxPixel::colorPixel(123));

            change |= frame->setPixel(19, 15, D1GfxPixel::colorPixel(85));
            change |= frame->setPixel(20, 15, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(21, 15, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(22, 15, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(23, 15, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(21, 14, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(22, 14, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(23, 14, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(24, 14, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(25, 14, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(24, 15, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(25, 15, D1GfxPixel::colorPixel(121));

            change |= frame->setPixel(23, 13, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(24, 13, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(23, 11, D1GfxPixel::colorPixel(89));

            change |= frame->setPixel(26, 6, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(24, 8, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(25, 7, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(26, 7, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(27, 7, D1GfxPixel::colorPixel(115));

            change |= frame->setPixel(25, 8, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(26, 8, D1GfxPixel::colorPixel(118));
            change |= frame->setPixel(27, 8, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(28, 8, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(29, 8, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(24, 9, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(29, 9, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(30, 9, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(31, 9, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(30, 10, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(31, 10, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(31, 11, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(31, 12, D1GfxPixel::colorPixel(125));

            change |= frame->setPixel(24, 10, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(25, 9, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(27, 9, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(28, 9, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(25, 10, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(26, 10, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(25, 12, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(25, 13, D1GfxPixel::colorPixel(89));
            change |= frame->setPixel(26, 13, D1GfxPixel::colorPixel(87));
            change |= frame->setPixel(27, 13, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(27, 10, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(27, 14, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(26, 9, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(26, 11, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(26, 12, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(26, 14, D1GfxPixel::colorPixel(86));
            change |= frame->setPixel(26, 15, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(25, 11, D1GfxPixel::colorPixel(68));
            // eliminate ugly leftovers(?)
            change |= frame->setPixel(23, 27, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(24, 26, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(24, 27, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(24, 28, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(25, 27, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(25, 28, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(26, 28, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(26, 29, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(27, 28, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(27, 29, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(28, 29, D1GfxPixel::colorPixel(116));
            change |= frame->setPixel(28, 30, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(29, 30, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(30, 31, D1GfxPixel::colorPixel(120));
        }
        if (i == 49) { // 493[1]
            change |= frame->setPixel(0, 10, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(0, 11, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(1, 9, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(1, 10, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(1, 11, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(2, 9, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(2, 10, D1GfxPixel::colorPixel(88));
            change |= frame->setPixel(3, 10, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(4, 9, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(4, 10, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(5, 9, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(6, 8, D1GfxPixel::colorPixel(74));
            change |= frame->setPixel(6, 9, D1GfxPixel::colorPixel(121));
        }
        if (i == 50) { // 501[1]
            change |= frame->setPixel(2, 8, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(3, 8, D1GfxPixel::colorPixel(72));

            change |= frame->setPixel(0, 9, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(1, 9, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(2, 9, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(3, 9, D1GfxPixel::colorPixel(121));

            change |= frame->setPixel(0, 10, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(1, 10, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(2, 10, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(3, 10, D1GfxPixel::colorPixel(75));

            change |= frame->setPixel(0, 11, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(1, 11, D1GfxPixel::colorPixel(71));

            change |= frame->setPixel(3, 22, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(4, 19, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(4, 20, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(4, 21, D1GfxPixel::colorPixel(91));
            change |= frame->setPixel(4, 22, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(4, 23, D1GfxPixel::colorPixel(123));

            change |= frame->setPixel(5, 23, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(5, 24, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(5, 25, D1GfxPixel::colorPixel(90));
            change |= frame->setPixel(5, 26, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(5, 27, D1GfxPixel::colorPixel(90));
            change |= frame->setPixel(5, 28, D1GfxPixel::colorPixel(125));
        }
        if (i == 51) { // 504[0]
            change |= frame->setPixel(16, 13, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(15, 13, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(14, 13, D1GfxPixel::colorPixel(124));
            change |= frame->setPixel(14, 14, D1GfxPixel::colorPixel(123));

            change |= frame->setPixel(26, 3, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(24, 4, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(25, 4, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(22, 5, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(23, 5, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(20, 6, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(21, 6, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(22, 6, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(20, 7, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(21, 7, D1GfxPixel::colorPixel(0));
        }
        if (i == 52) { // 504[1]
            change |= frame->setPixel(2, 2, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(3, 2, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(2, 3, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(3, 3, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(4, 3, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(5, 3, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(3, 4, D1GfxPixel::colorPixel(124));
            change |= frame->setPixel(4, 4, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(5, 4, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(6, 4, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(4, 5, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(5, 5, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(5, 6, D1GfxPixel::colorPixel(124));
            change |= frame->setPixel(6, 5, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(6, 6, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(6, 7, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(6, 8, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(7, 9, D1GfxPixel::colorPixel(120));
        }
        if (i == 53) { // 479[0]
            change |= frame->setPixel(31, 26, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(31, 27, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(31, 28, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(31, 29, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(31, 30, D1GfxPixel::colorPixel(73));
        }
        if (i == 54) { // 479[1]
            change |= frame->setPixel(1, 31, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(0, 29, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(0, 30, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(0, 31, D1GfxPixel::colorPixel(126));
        }
        if (i == 55) { // 488[0]
            change |= frame->setPixel(6, 19, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(7, 19, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(7, 18, D1GfxPixel::colorPixel(0));
        }
        if (i == 56) { // 517[0]
            change |= frame->setPixel(14, 11, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(16, 11, D1GfxPixel::colorPixel(119));

            change |= frame->setPixel(6, 13, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(6, 14, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(6, 15, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(6, 16, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(6, 17, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(6, 18, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(6, 19, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(7, 15, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(7, 16, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(7, 17, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(7, 18, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(7, 19, D1GfxPixel::colorPixel(0));
        }
        if (i == 57) { // 517[1]
            change |= frame->setPixel(25, 16, D1GfxPixel::colorPixel(87));
            change |= frame->setPixel(25, 17, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(26, 16, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(26, 17, D1GfxPixel::colorPixel(116));
            change |= frame->setPixel(26, 18, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(27, 17, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(27, 18, D1GfxPixel::colorPixel(68));
        }
        if (i == 58) { // 516[0]
            change |= frame->setPixel(17, 8, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(18, 7, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(18, 8, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(18, 9, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(19, 7, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(19, 8, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(19, 9, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(19, 10, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(19, 11, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(20, 8, D1GfxPixel::colorPixel(0));

            change |= frame->setPixel(19, 9, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(18, 10, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(19, 10, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(19, 11, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(20, 13, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(20, 15, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(20, 16, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(19, 17, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(20, 17, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(19, 18, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(16, 19, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(17, 18, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(17, 19, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(18, 18, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(15, 20, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(16, 20, D1GfxPixel::colorPixel(123));

            change |= frame->setPixel(28, 8, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(29, 9, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(29, 10, D1GfxPixel::colorPixel(74));

            change |= frame->setPixel(27, 8, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(27, 9, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(27, 10, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(27, 11, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(28, 10, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(28, 11, D1GfxPixel::colorPixel(0));
        }
        if (i == 59) { // 507[0]
            change |= frame->setPixel(16, 8, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(16, 9, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(17, 8, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(17, 9, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(17, 10, D1GfxPixel::colorPixel(120));

            change |= frame->setPixel(18, 7, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(18, 8, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(18, 9, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(18, 10, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(18, 11, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(19, 8, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(19, 9, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(19, 10, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(19, 11, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(19, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(19, 13, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(20, 9, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(20, 10, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(20, 11, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(20, 12, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(20, 13, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(20, 14, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(20, 15, D1GfxPixel::colorPixel(120));

            change |= frame->setPixel(21, 11, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(21, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(21, 13, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(21, 14, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(21, 15, D1GfxPixel::colorPixel(125));

            change |= frame->setPixel(22, 11, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(22, 12, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(22, 13, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(22, 14, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(22, 15, D1GfxPixel::colorPixel(123));

            change |= frame->setPixel(23, 12, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(23, 13, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(23, 14, D1GfxPixel::colorPixel(92));
            change |= frame->setPixel(23, 15, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(23, 16, D1GfxPixel::colorPixel(92));

            change |= frame->setPixel(24, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(24, 13, D1GfxPixel::colorPixel(92));
            change |= frame->setPixel(24, 14, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(24, 15, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(24, 16, D1GfxPixel::colorPixel(125));

            change |= frame->setPixel(25, 12, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(25, 13, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(25, 14, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(25, 15, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(25, 16, D1GfxPixel::colorPixel(75));

            change |= frame->setPixel(26, 12, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(26, 13, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(26, 14, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(26, 15, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(26, 16, D1GfxPixel::colorPixel(75));

            change |= frame->setPixel(27, 11, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(27, 12, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(27, 13, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(27, 14, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(27, 15, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(27, 16, D1GfxPixel::colorPixel(122));

            change |= frame->setPixel(28, 11, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(28, 12, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(28, 13, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(28, 14, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(28, 15, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(28, 16, D1GfxPixel::colorPixel(89));

            change |= frame->setPixel(29, 9, D1GfxPixel::colorPixel(92));
            change |= frame->setPixel(29, 10, D1GfxPixel::colorPixel(91));
            change |= frame->setPixel(29, 11, D1GfxPixel::colorPixel(91));
            change |= frame->setPixel(29, 12, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(29, 13, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(29, 14, D1GfxPixel::colorPixel(93));
            change |= frame->setPixel(29, 15, D1GfxPixel::colorPixel(119));
        }
        if (i == 60) { // 500[2]
            change |= frame->setPixel(4, 11, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(5, 11, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(6, 11, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(6, 12, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(6, 13, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(7, 12, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(7, 13, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(7, 14, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(8, 14, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(8, 15, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(8, 16, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(9, 17, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(9, 18, D1GfxPixel::colorPixel(125));
        }
        if (i == 61) { // 473[0]
            change |= frame->setPixel(25, 19, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(26, 20, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(27, 20, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(27, 21, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(27, 22, D1GfxPixel::colorPixel(0));

            change |= frame->setPixel(28, 21, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(28, 22, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(28, 23, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(29, 21, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(29, 22, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(29, 23, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(29, 24, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(29, 25, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(30, 22, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(30, 23, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(30, 24, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(30, 25, D1GfxPixel::colorPixel(0));

            change |= frame->setPixel(29, 28, D1GfxPixel::colorPixel(90));
            change |= frame->setPixel(30, 28, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(31, 28, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(30, 29, D1GfxPixel::colorPixel(70));
        }
        if (i == 62) { // 544[1]
            change |= frame->setPixel(3, 22, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(4, 24, D1GfxPixel::colorPixel(73));
        }
        if (i == 64) { // 33[3]
            change |= frame->setPixel(4, 31, D1GfxPixel::colorPixel(71));
        }
        if (i == 65) { // 449[1]
            change |= frame->setPixel(30, 14, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(31, 13, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(31, 14, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(31, 15, D1GfxPixel::colorPixel(68));
        }
        if (i == 66) { // 441[2]
            change |= frame->setPixel(22, 16, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(23, 16, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(24, 16, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(23, 15, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(24, 15, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(24, 14, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(25, 14, D1GfxPixel::colorPixel(0));
        }
        if (i == 67) { // 32[2]
            change |= frame->setPixel(33, 19, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(33, 20, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(33, 21, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(33, 22, D1GfxPixel::colorPixel(66));
            change |= frame->setPixel(33, 23, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(33, 24, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(33, 25, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(33, 26, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(33, 27, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(33, 28, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(33, 29, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(33, 30, D1GfxPixel::colorPixel(69));
        }
        if (i == 68) { // 457[0] - fix glitch after reuse of 465[2]
            change |= frame->setPixel(16, 0, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(17, 0, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(18, 0, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(19, 0, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(20, 0, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(21, 0, D1GfxPixel::colorPixel(73));
        }
        if (i == 69) { // 469[2] - fix glitch after reuse of 465[4]
            change |= frame->setPixel(18, 0, D1GfxPixel::colorPixel(57));
            change |= frame->setPixel(18, 1, D1GfxPixel::colorPixel(58));
            change |= frame->setPixel(18, 2, D1GfxPixel::colorPixel(91));
        }
        if (i == 70) { // 49[7] - fix glitch
            change |= frame->setPixel(20, 23, D1GfxPixel::transparentPixel());
        }
        if (i == 76) { // 210[0] - fix shadow
            change |= frame->setPixel(27, 6, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(28, 7, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(31, 9, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(30, 21, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(31, 21, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(31, 21, D1GfxPixel::colorPixel(71));
        }
        if (i == 77) { // 210[1] - fix shadow
            change |= frame->setPixel(0, 10, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(0, 15, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(0, 16, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(0, 17, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 11, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(1, 12, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(1, 13, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 14, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 15, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 16, D1GfxPixel::colorPixel(91));
            change |= frame->setPixel(1, 17, D1GfxPixel::colorPixel(122));
        }
        if (i == 78) { // 233[0] - fix shadow
            change |= frame->setPixel(17, 8, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(17, 9, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(18, 8, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(19, 8, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(20, 7, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(20, 8, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(24, 7, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(25, 6, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(25, 5, D1GfxPixel::colorPixel(120));
        }
        // improve connections
        if (i == 79) { // 25[1]
            change |= frame->setPixel(18, 22, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(15, 24, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(30, 16, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(29, 16, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(16, 23, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(22, 20, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(21, 20, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(21, 19, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(25, 19, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(24, 19, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(23, 20, D1GfxPixel::colorPixel(68));

            change |= frame->setPixel(8, 27, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(9, 27, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(10, 26, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(11, 26, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(9, 26, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(7, 27, D1GfxPixel::colorPixel(70));

            change |= frame->setPixel(12, 7, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(11, 6, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(8, 5, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(14, 8, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(18, 10, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(5, 3, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(2, 2, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(16, 9, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(15, 8, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(13, 7, D1GfxPixel::colorPixel(67));
        }
        if (i == 80) { // 26[0]
            change |= frame->setPixel(28, 4, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(29, 3, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(25, 9, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(26, 9, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(28, 9, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(29, 9, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(29, 10, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(30, 10, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(30, 11, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(31, 10, D1GfxPixel::colorPixel(70));

            change |= frame->setPixel(25, 7, D1GfxPixel::colorPixel(118));
            change |= frame->setPixel(26, 7, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(27, 8, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(27, 7, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(29, 7, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(28, 7, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(29, 8, D1GfxPixel::colorPixel(87));
            change |= frame->setPixel(30, 8, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(20, 6, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(18, 7, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(16, 8, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(12, 10, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(11, 11, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(12, 12, D1GfxPixel::colorPixel(69));

            change |= frame->setPixel(5, 14, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(4, 15, D1GfxPixel::colorPixel(69));
        }
        if (i == 81) { // 26[1]
            change |= frame->setPixel(0, 10, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(0, 11, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(3, 4, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(4, 4, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(5, 5, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(1, 7, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(3, 6, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(2, 7, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(4, 7, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(6, 7, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(3, 8, D1GfxPixel::colorPixel(87));
            change |= frame->setPixel(4, 8, D1GfxPixel::colorPixel(88));
            change |= frame->setPixel(6, 8, D1GfxPixel::colorPixel(53));
            change |= frame->setPixel(1, 8, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(2, 8, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(1, 9, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(2, 9, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(5, 9, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(6, 9, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(1, 10, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(2, 10, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(1, 11, D1GfxPixel::colorPixel(68));
        }
        if (i == 82) { // 205[1]
            change |= frame->setPixel(17, 9, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(13, 7, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(10, 6, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(7, 4, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(21, 11, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(25, 13, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(24, 13, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(15, 8, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(12, 7, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(15, 9, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(11, 7, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(11, 6, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(12, 8, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(16, 10, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(20, 13, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(21, 13, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(20, 12, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(20, 11, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(19, 12, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(14, 24, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(14, 23, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(13, 25, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(13, 24, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(13, 23, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(12, 23, D1GfxPixel::colorPixel(68));
        }
        if (i == 83) { // 385[0]
            change |= frame->setPixel(24, 4, D1GfxPixel::colorPixel(68));
        }
        if (i == 84) { // 385[1]
            change |= frame->setPixel(14, 8, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(13, 8, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(14, 9, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(15, 9, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(12, 8, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(13, 9, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(11, 8, D1GfxPixel::colorPixel(68));
        }
        if (i == 85) { // 386[1]
            change |= frame->setPixel(25, 13, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(27, 14, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(25, 14, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(14, 13, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(23, 13, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(23, 12, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(26, 14, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(26, 15, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(10, 6, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(9, 5, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(11, 6, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(7, 28, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(6, 28, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(5, 28, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(15, 24, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(11, 7, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(11, 8, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(12, 7, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(13, 7, D1GfxPixel::colorPixel(68));
        }
        if (i == 86) { // 387[0]
            change |= frame->setPixel(26, 3, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(21, 6, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(16, 8, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(12, 10, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(8, 12, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(18, 7, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(4, 14, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(22, 5, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(27, 3, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(28, 2, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(15, 9, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(20, 6, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(24, 4, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(24, 5, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(28, 4, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(29, 2, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(29, 3, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(14, 9, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(14, 10, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(3, 17, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(23, 5, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(24, 5, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(24, 6, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(23, 6, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(19, 7, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(24, 7, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(25, 6, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(26, 6, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(20, 7, D1GfxPixel::colorPixel(68));
        }
        if (i == 87) { // 388[1]
            change |= frame->setPixel(4, 29, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(5, 29, D1GfxPixel::colorPixel(69));
        }
        if (i == 88) { // 390[1]
            change |= frame->setPixel(27, 14, D1GfxPixel::colorPixel(68));

            change |= frame->setPixel(26, 14, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(27, 18, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(26, 18, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(7, 28, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(9, 27, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(8, 26, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(6, 27, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(7, 27, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(8, 27, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(11, 6, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(10, 6, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(9, 5, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(13, 7, D1GfxPixel::colorPixel(68));
        }
        if (i == 89) { // 395[0]
            change |= frame->setPixel(5, 14, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(3, 15, D1GfxPixel::colorPixel(69));
        }
        if (i == 90) { // 395[1]
            change |= frame->setPixel(0, 10, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(0, 11, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(3, 4, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(4, 4, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(5, 5, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(1, 7, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(3, 6, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(2, 7, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(4, 7, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(6, 7, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(3, 8, D1GfxPixel::colorPixel(87));
            change |= frame->setPixel(4, 8, D1GfxPixel::colorPixel(88));
            change |= frame->setPixel(6, 8, D1GfxPixel::colorPixel(53));
            change |= frame->setPixel(1, 8, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(2, 8, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(1, 9, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(2, 9, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(5, 9, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(6, 9, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(1, 10, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(2, 10, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(1, 11, D1GfxPixel::colorPixel(68));
        }
        if (i == 91) { // fix 511[5] after reuse
            for (int y = 15; y < MICRO_HEIGHT; y++) {
                change |= frame->setPixel(15, y, D1GfxPixel::transparentPixel());
            }
            /*change |= frame->setPixel( 8, 24, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel( 8, 25, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel( 8, 26, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel( 8, 27, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel( 8, 28, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel( 8, 29, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel( 8, 30, D1GfxPixel::colorPixel(94));
            change |= frame->setPixel( 8, 31, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel( 9, 31, D1GfxPixel::colorPixel(59));*/
        }
        if (i == 92) { // move pixels to 171[3]
            change |= frame->setPixel(31, 14, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(31, 15, D1GfxPixel::colorPixel(93));
        }

        // create new shadow 435[1] using 544[1] and 551[1]
        if (i == 63) {
            const CelMicro &microSrc1 = micros[i - 1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[i - 38];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (x < 9 && y > 18) {
                        pixel = frameSrc1->getPixel(x, y); // 544[1]
                    } else {
                        pixel = frameSrc2->getPixel(x, y); // 551[1]
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

bool D1Tileset::patchCavesStairs(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{ 180 - 1, 2, D1CEL_FRAME_TYPE::Empty },              // sync stairs (used to block subsequent calls)
/*  1 */{ 180 - 1, 4, D1CEL_FRAME_TYPE::Empty },
/*  2 */{ 171 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  3 */{ 171 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid },
/*  4 */{ 171 - 1, 3, D1CEL_FRAME_TYPE::Square },
/*  5 */{ 173 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  6 */{ 174 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  7 */{ 174 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/*  8 */{ 174 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/*  9 */{ 174 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 10 */{ 176 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 11 */{ 176 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 12 */{ 163 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
    };
    constexpr unsigned blockSize = BLOCK_SIZE_L3;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // move pixels to 171[0] from 176[2]
        if (i == 2) {
            const CelMicro &microSrc = micros[11];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            if (frameSrc == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    if (y > 16 + x / 2 || y < 16 - x / 2) {
                        continue;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 176[2]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        // move pixels to 171[1] from 174[0] and 174[2]
        if (i == 3) {
            const CelMicro &microSrc1 = micros[7];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            if (frameSrc1 == nullptr) {
                return false;
            }
            const CelMicro &microSrc2 = micros[6];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            if (frameSrc2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y > 31 - x / 2) {
                        continue;
                    }
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 174[2]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 174[0]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 171[3] from 174[2]
        if (i == 4) {
            const CelMicro &microSrc = micros[7];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            if (frameSrc == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 174[2]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 173[1] from 176[0] and 176[2]
        if (i == 5) {
            const CelMicro &microSrc1 = micros[11];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            if (frameSrc1 == nullptr) {
                return false;
            }
            const CelMicro &microSrc2 = micros[10];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            if (frameSrc2 == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y > 31 - x / 2 || y < 1 + x / 2) {
                        continue;
                    }
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 176[2]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 176[0]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 174[5] from 180[4] and 180[2]
        if (i == 8) {
            const CelMicro &microSrc1 = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 180[4]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 180[2]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 174[7] from 180[4]
        if (i == 9) {
            const CelMicro &microSrc = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH / 2; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 180[4]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

bool D1Tileset::patchCavesWall1(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{ 446 - 1, 1, D1CEL_FRAME_TYPE::Empty },              // mask walls leading to north east
/*  1 */{ 446 - 1, 3, D1CEL_FRAME_TYPE::Empty },
/*  2 */{  35 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  3 */{  34 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*  4 */{  43 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  5 */{  42 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*  6 */{  55 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  7 */{  54 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*  8 */{  91 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  9 */{  90 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 10 */{ 165 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 11 */{ 164 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 12 */{ /*251*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 13 */{ /*250*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 14 */{ 316 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 15 */{ 315 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 16 */{ 322 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 17 */{ 321 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 18 */{ /*336*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 19 */{ 335 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 20 */{ 343 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 21 */{ 342 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 22 */{ 400 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 23 */{ 399 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 24 */{ 444 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 25 */{ 443 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 26 */{ 476 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 27 */{ 475 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },

/* 28 */{ 440 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 29 */{ 439 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 30 */{ /*448*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 31 */{ 447 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 32 */{ /*452*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 33 */{ 451 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 34 */{ 456 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 35 */{ 455 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 36 */{ 484 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 37 */{ 483 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },

/* 38 */{  32 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 39 */{  32 - 1, 3, D1CEL_FRAME_TYPE::Empty },
/* 40 */{  32 - 1, 5, D1CEL_FRAME_TYPE::Empty },
/* 41 */{  32 - 1, 7, D1CEL_FRAME_TYPE::Empty },
/* 42 */{ /*1*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 43 */{ /*1*/ - 1, 3, D1CEL_FRAME_TYPE::Empty },
/* 44 */{ /*1*/ - 1, 5, D1CEL_FRAME_TYPE::Empty },
/* 45 */{   1 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 46 */{  44 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 47 */{  44 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 48 */{  44 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 49 */{ /*44*/ - 1, 7, D1CEL_FRAME_TYPE::Empty },
/* 50 */{  34 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 51 */{  34 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 52 */{  34 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 53 */{  34 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 54 */{  42 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 55 */{  42 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 56 */{  42 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 57 */{  42 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 58 */{  54 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 59 */{  54 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 60 */{  54 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 61 */{  54 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 62 */{  90 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 63 */{  90 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 64 */{  90 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 65 */{  90 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 66 */{ 164 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 67 */{ 164 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 68 */{ /*164*/ - 1, 4, D1CEL_FRAME_TYPE::Empty },
/* 69 */{ /*164*/ - 1, 6, D1CEL_FRAME_TYPE::Empty },
/* 70 */{ /*250*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 71 */{ /*250*/ - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 72 */{ /*250*/ - 1, 4, D1CEL_FRAME_TYPE::Empty },
/* 73 */{ /*250*/ - 1, 6, D1CEL_FRAME_TYPE::Empty },
/* 74 */{ 315 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 75 */{ 315 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 76 */{ 315 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 77 */{ 315 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 78 */{ 321 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 79 */{ 321 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 80 */{ 321 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 81 */{ 321 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 82 */{ 335 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 83 */{ 335 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 84 */{ 335 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 85 */{ 335 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 86 */{ 342 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 87 */{ 342 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 88 */{ 342 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 89 */{ /*342*/ - 1, 6, D1CEL_FRAME_TYPE::Empty },
/* 90 */{ 399 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 91 */{ 399 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 92 */{ 399 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 93 */{ 399 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 94 */{ 443 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 95 */{ 443 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 96 */{ 443 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 97 */{ 443 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 98 */{ /*475*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 99 */{ 475 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/*100 */{ 475 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/*101 */{ /*475*/ - 1, 6, D1CEL_FRAME_TYPE::Empty },

/*102 */{ 441 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*103 */{ 441 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/*104 */{ 441 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/*105 */{ 441 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/*106 */{ 439 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*107 */{ 439 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/*108 */{ 439 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/*109 */{ 439 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/*110 */{ 483 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*111 */{ 483 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/*112 */{ 483 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/*113 */{ /*483*/ - 1, 6, D1CEL_FRAME_TYPE::Empty },

/*114 */{ 162 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*115 */{  89 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*116 */{  88 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*117 */{  88 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/*118 */{  88 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/*119 */{   8 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/*120 */{ 373 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/*121 */{  20 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/*122 */{ /*20*/ - 1, 4, D1CEL_FRAME_TYPE::Empty },
/*123 */{  16 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },

/*124 */{  52 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/*125 */{ /*2*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*126 */{ /*6*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*127 */{ /*18*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*128 */{ 510 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },

/*129 */{  48 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*130 */{  11 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*131 */{ /*15*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/*132 */{  19 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*133 */{ 376 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*134 */{ /*380*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/*135 */{ 513 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },

/*136 */{  49 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },

/*137 */{  511 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/*138 */{  450 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*139 */{  482 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*140 */{ /*438*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
    };
    constexpr unsigned blockSize = BLOCK_SIZE_L3;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // mask walls leading to north east [1]
        if (i >= 2 && i < 38 && (i % 2) == (2 % 2)) {
            const CelMicro &microSrc1 = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 446[3]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 446[1]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        if (i >= 3 && i < 38 && (i % 2) == (3 % 2)) {
            const CelMicro &microSrc = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 446[3]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask walls leading to north east [0]
        // mask 1[1] 44[1]
        if (i >= 42 && i < 50 && (i % 4) == (42 % 4)) {
            const CelMicro &microSrc = micros[38];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y);
                    if (pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 1[3] 44[3]
        if (i >= 43 && i < 50 && (i % 4) == (43 % 4)) {
            const CelMicro &microSrc = micros[39];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y);
                    if (pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 1[5] 44[5]
        if (i >= 44 && i < 50 && (i % 4) == (44 % 4)) {
            const CelMicro &microSrc = micros[40];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y);
                    if (pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 1[7] 44[7]
        if (i >= 45 && i < 50 && (i % 4) == (45 % 4)) {
            const CelMicro &microSrc = micros[41];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y);
                    if (pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // mask 34[0] 42[0] 54[0] 90[0] 164[0] 250[0] 315[0] 321[0] 335[0] 342[0] 399[0] 443[0] 475[0]
        if (i >= 50 && i < 102 && (i % 4) == (50 % 4)) {
            const CelMicro &microSrc1 = micros[39];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[38];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2);
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2);
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 34[2] 42[2] 54[2] 90[2] 164[2] 250[2] 315[2] 321[2] 335[2] 342[2] 399[2] 443[2] 475[2]
        if (i >= 51 && i < 102 && (i % 4) == (51 % 4)) {
            const CelMicro &microSrc1 = micros[40];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[39];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2);
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2);
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 34[4] 42[4] 54[4] 90[4] 164[4] 250[4] 315[4] 321[4] 335[4] 342[4] 399[4] 443[4] 475[4]
        if (i >= 52 && i < 102 && (i % 4) == (52 % 4)) {
            const CelMicro &microSrc1 = micros[41];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[40];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2);
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2);
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 34[6] 42[6] 54[6] 90[6] 164[6] 250[6] 315[6] 321[6] 335[6] 342[6] 399[6] 443[6] 475[6]
        if (i >= 53 && i < 102 && (i % 4) == (53 % 4)) {
            const CelMicro &microSrc = micros[41];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2);
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        if (i == 103) { // prepare mask 441[3]
            change |= frame->setPixel(13, 1, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(13, 2, D1GfxPixel::transparentPixel());
        }
        if (i == 104) { // prepare mask 441[5]
            change |= frame->setPixel(12, 29, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(12, 30, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(12, 31, D1GfxPixel::transparentPixel());
        }
        if (i == 105) { // prepare mask 441[7]
            change |= frame->setPixel(5, 11, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(5, 19, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(5, 20, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(5, 21, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(5, 22, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(5, 23, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(5, 24, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(5, 25, D1GfxPixel::transparentPixel());
        }

        // mask 439[0] 483[0] using 441[3] and 441[1]
        if (i >= 106 && i < 114 && (i % 4) == (106 % 4)) {
            const CelMicro &microSrc1 = micros[103];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[102];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 441[3]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 441[1]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 439[2] 483[2] using 441[5] and 441[3]
        if (i >= 107 && i < 114 && (i % 4) == (107 % 4)) {
            const CelMicro &microSrc1 = micros[104];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[103];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 441[5]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 441[3]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 439[4] 483[4] using 441[7] and 441[5]
        if (i >= 108 && i < 114 && (i % 4) == (108 % 4)) {
            const CelMicro &microSrc1 = micros[105];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[104];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 441[7]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 441[5]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 439[6] using 441[7]
        if (i >= 109 && i < 114 && (i % 4) == (109 % 4)) {
            const CelMicro &microSrc = micros[105];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 441[7]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // fix 88[5] using 32[5]
        if (i == 118) {
            const CelMicro &microSrc = micros[40];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y);
                    if (frame->getPixel(x, y).isTransparent() && !pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        // mask 8[5], 373[5] 20[5]
        if (i >= 119 && i < 122) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y > 7 - x / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // mask 20[4], 16[4]
        if (i >= 122 && i < 124) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y > x / 2 - 8) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // mask 2[1], 6[1], 18[1], 510[1] with 52[0]
        if (i >= 125 && i < 129) {
            const CelMicro &microSrc = micros[124];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 52[0]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 11[0], 15[0], 19[0], 376[0], 380[0], 513[0] with 48[1]
        if (i >= 130 && i < 136) {
            const CelMicro &microSrc = micros[129];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 48[1]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // mask 511[5] using 54[4] and 54[6]
        if (i == 137) {
            const CelMicro &microSrc1 = micros[61];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[60];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 54[6]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 54[4]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                    if (x < 7 && y > 11) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 1[1] using 54[2] and 54[4]
        if (i == 42) {
            const CelMicro &microSrc1 = micros[60];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[59];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 54[4]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 54[2]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 1[3] using 54[4] and 54[6] 
        if (i == 43) {
            const CelMicro &microSrc1 = micros[61];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[60];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 54[6]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 54[4]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // fix artifacts
        /*if (i == 34) { // 456[0]
            change |= frame->setPixel(12, 17, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(13, 18, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(14, 19, D1GfxPixel::colorPixel(68));
        }*/
        if (i == 115) { // 89[1]
            change |= frame->setPixel(24, 12, D1GfxPixel::colorPixel(72));
        }
        if (i == 116) { // 88[1]
            change |= frame->setPixel(28, 0, D1GfxPixel::colorPixel(70));
        }
        if (i == 117) { // 88[3]
            change |= frame->setPixel(25, 28, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(24, 27, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(23, 25, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(22, 24, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(22, 23, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(21, 22, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(20, 20, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(18, 17, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(17, 15, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(16, 14, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(16, 13, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(16, 12, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(15, 11, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(15, 10, D1GfxPixel::colorPixel(88));
            change |= frame->setPixel(15, 9, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(14, 7, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(14, 6, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(14, 5, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(13, 4, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(13, 3, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(13, 2, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(13, 1, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(12, 1, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(12, 0, D1GfxPixel::colorPixel(69));
        }
        if (i == 59) { // 54[2]
            change |= frame->setPixel(30, 19, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(31, 19, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(28, 18, D1GfxPixel::colorPixel(89));
            change |= frame->setPixel(29, 18, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(30, 18, D1GfxPixel::colorPixel(88));
            change |= frame->setPixel(31, 18, D1GfxPixel::colorPixel(71));
        }
        if (i == 123) { // 16[4]
            change |= frame->setPixel(24, 4, D1GfxPixel::colorPixel(73));
        }
        if (i == 136) { // 49[3]
            change |= frame->setPixel(0, 19, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(3, 18, D1GfxPixel::colorPixel(73));
        }
        if (i == 138) { // 450[1]
            change |= frame->setPixel(12, 1, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(13, 2, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(14, 3, D1GfxPixel::colorPixel(70));

            change |= frame->setPixel(18, 6, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(19, 7, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(20, 8, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(21, 9, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(22, 9, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(22, 10, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(23, 10, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(23, 11, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(24, 11, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(25, 12, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(26, 12, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(26, 13, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(27, 13, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(28, 14, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(29, 14, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(30, 15, D1GfxPixel::transparentPixel());
        }
        if (i == 139) { // 482[1]
            change |= frame->setPixel(10, 0, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(11, 1, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(12, 2, D1GfxPixel::colorPixel(70));

            change |= frame->setPixel(12, 1, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(13, 2, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(14, 3, D1GfxPixel::colorPixel(70));

            change |= frame->setPixel(15, 4, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(16, 5, D1GfxPixel::colorPixel(70));

            change |= frame->setPixel(21, 9, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(22, 10, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(23, 11, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(25, 12, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(26, 13, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(28, 14, D1GfxPixel::transparentPixel());
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

bool D1Tileset::patchCavesWall2(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{   38 - 1, 0, D1CEL_FRAME_TYPE::Empty },             // mask walls leading to north west
/*  1 */{   38 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/*  2 */{  175 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  3 */{  177 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*  4 */{  320 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  5 */{  322 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*  6 */{  324 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  7 */{  326 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*  8 */{  341 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  9 */{  343 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 10 */{  345 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 11 */{  347 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 12 */{  402 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 13 */{  404 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 14 */{   37 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 15 */{   39 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 16 */{   41 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 17 */{   43 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 18 */{   49 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 19 */{   51 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 20 */{   93 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 21 */{   95 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 22 */{  466 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 23 */{  468 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 24 */{  478 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 25 */{  /*480*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },

/* 26 */{  36 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 27 */{  36 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 28 */{  36 - 1, 4, D1CEL_FRAME_TYPE::Empty },
/* 29 */{  36 - 1, 6, D1CEL_FRAME_TYPE::Empty },
/* 30 */{ 175 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 31 */{ 175 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 32 */{ 175 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 33 */{ 320 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 34 */{ 320 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 35 */{ 320 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 36 */{ 324 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 37 */{ 324 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 38 */{ 324 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 39 */{ 341 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 40 */{ 341 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 41 */{ 341 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 42 */{ 345 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 43 */{ 345 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 44 */{ 345 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 45 */{ 402 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 46 */{ 402 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 47 */{ 402 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 48 */{  37 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 49 */{  37 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 50 */{  37 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 51 */{  41 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 52 */{  41 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 53 */{  41 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 54 */{  49 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 55 */{  49 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 56 */{  49 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 57 */{  93 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 58 */{  93 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 59 */{  93 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 60 */{ 466 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 61 */{ 466 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 62 */{ 466 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 63 */{ 478 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 64 */{ 478 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 65 */{ 478 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/* 66 */{ 471 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 67 */{ /*471*/ - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 68 */{ 486 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 69 */{ 488 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 70 */{ 470 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 71 */{ 472 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 72 */{ 462 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 73 */{ 464 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 74 */{ 454 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 75 */{ 456 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },

/* 76 */{ 470 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 77 */{ 470 - 1, 3, D1CEL_FRAME_TYPE::Empty },
/* 78 */{ 470 - 1, 5, D1CEL_FRAME_TYPE::Empty },
/* 79 */{ 470 - 1, 7, D1CEL_FRAME_TYPE::Empty },
/* 84 */{ 486 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 85 */{ 486 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 86 */{ 486 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 87 */{ /*486*/ - 1, 7, D1CEL_FRAME_TYPE::Empty },

/* 88 */{ 467 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // sync overlapping micros
/* 89 */{ 459 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },

/* 90 */{ 9 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // fix micros after masking
/* 91 */{ 11 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 92 */{ 92 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 93 */{ 92 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 94 */{ 94 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 95 */{ 252 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 96 */{ 252 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 97 */{ 252 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 98 */{ 376 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 99 */{ 374 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*100 */{ 374 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
    };
    constexpr unsigned blockSize = BLOCK_SIZE_L3;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // mask walls leading to north west
        // mask 175[0], 320[0], 324[0], 341[0], 345[0], 402[0] + 37[0], 41[0], 49[0], 93[0], 466[0], 478[0] using 38[2]
        if (i >= 2 && i < 26 && (i % 2) == (2 % 2)) {
            const CelMicro &microSrc = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 38[2]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 177[1], 322[1], 326[1], 343[1], 347[1], 404[1] + 39[1], 43[1], 51[1], 95[1], 468[1] using 38[0] and 38[2]
        if (i >= 3 && i < 26 && (i % 2) == (3 % 2)) {
            const CelMicro &microSrc1 = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 38[2]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 38[0]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // mask 175[1], 320[1], 324[1], 341[1], 345[1], 402[1] + 37[1], 41[1], 49[1], 93[1], 466[1], 478[1] using 36[0] and 36[2]
        if (i >= 30 && i < 66 && (i % 3) == (30 % 3)) {
            const CelMicro &microSrc1 = micros[27];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[26];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 36[2]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 36[0]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 175[3], 320[3], 324[3], 341[3], 345[3], 402[3] + 37[3], 41[3], 49[3], 93[3], 466[3], 478[3] using 36[2] and 36[4]
        if (i >= 31 && i < 66 && (i % 3) == (31 % 3)) {
            const CelMicro &microSrc1 = micros[28];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[27];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 36[4]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 36[2]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 175[5], 320[5], 324[5], 341[5], 345[5], 402[5] + 37[5], 41[5], 49[5], 93[5], 466[5], 478[5] using 36[4] and 36[6]
        if (i >= 32 && i < 66 && (i % 3) == (32 % 3)) {
            const CelMicro &microSrc1 = micros[29];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[28];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 36[6]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 36[4]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // mask 486[0], 470[0], 462[0], 454[0] using 38[2]
        if (i >= 68 && i < 76 && (i % 2) == (68 % 2)) {
            const CelMicro &microSrc = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 38[2]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 488[1], 472[1], 464[1], 456[1] using 471[0] and 38[2]
        if (i >= 69 && i < 76 && (i % 2) == (69 % 2)) {
            const CelMicro &microSrc1 = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[66];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 38[2]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 471[0]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // mask 486[1] using 470[1]
        if (i >= 80 && i < 84 && (i % 4) == (80 % 4)) {
            const CelMicro &microSrc = micros[76];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 470[1]
                    if (pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 478[3] 486[3] using 470[3]
        if (i >= 81 && i < 84 && (i % 4) == (81 % 4)) {
            const CelMicro &microSrc = micros[77];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 470[3]
                    if (pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 478[5] 486[5] using 470[5]
        if (i >= 82 && i < 84 && (i % 4) == (82 % 4)) {
            const CelMicro &microSrc = micros[78];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 470[5]
                    if (pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 478[7] 486[7] using 470[7]
        if (i >= 83 && i < 84 && (i % 4) == (83 % 4)) {
            const CelMicro &microSrc = micros[79];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 470[7]
                    if (pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // sync overlapping micros 467[0], 459[0] with 471[0]
        if (i >= 84 && i < 86) {
            const CelMicro &microSrc = micros[66];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 471[0]
                    if (pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // fix micros after masking
        if (i == 86) { // 9[0]
            change |= frame->setPixel(1, 2, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(2, 1, D1GfxPixel::colorPixel(73));
        }
        if (i == 87) { // 11[0]
            change |= frame->setPixel(12, 7, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(5, 13, D1GfxPixel::colorPixel(70));
        }
        if (i == 88) { // 92[0]
            change |= frame->setPixel(0, 4, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(1, 2, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(1, 3, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(2, 1, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(2, 2, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(3, 0, D1GfxPixel::colorPixel(120));
        }
        if (i == 89) { // 92[2]
            change |= frame->setPixel(4, 30, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(4, 31, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(5, 30, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(5, 29, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(6, 28, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(7, 27, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(7, 26, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(8, 26, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(8, 25, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(8, 24, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(9, 24, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(9, 23, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(10, 23, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(10, 22, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(10, 21, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(11, 21, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(11, 20, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(11, 19, D1GfxPixel::colorPixel(71));

            change |= frame->setPixel(12, 19, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(12, 18, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(13, 17, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(13, 16, D1GfxPixel::colorPixel(68));

            change |= frame->setPixel(14, 16, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(14, 15, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(14, 14, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(15, 14, D1GfxPixel::colorPixel(68));
            change |= frame->setPixel(15, 13, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(15, 12, D1GfxPixel::colorPixel(91));
            change |= frame->setPixel(15, 11, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(16, 12, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(16, 11, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(16, 10, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(16, 9, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(16, 8, D1GfxPixel::colorPixel(88));
            change |= frame->setPixel(17, 9, D1GfxPixel::colorPixel(74));
            change |= frame->setPixel(17, 8, D1GfxPixel::colorPixel(88));
            change |= frame->setPixel(17, 7, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(17, 6, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(17, 5, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(18, 5, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(18, 4, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(18, 3, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(18, 2, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(18, 1, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(19, 2, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(19, 1, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(19, 0, D1GfxPixel::colorPixel(88));
        }
        if (i == 90) { // 94[0]
            change |= frame->setPixel(7, 12, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(8, 11, D1GfxPixel::colorPixel(67));
            change |= frame->setPixel(9, 10, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(10, 9, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(11, 8, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(12, 7, D1GfxPixel::colorPixel(69));

            change |= frame->setPixel(16, 4, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(17, 3, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(18, 2, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(19, 1, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(20, 0, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(5, 13, D1GfxPixel::colorPixel(70));
        }
        if (i == 91) { // 252[2]
            change |= frame->setPixel(11, 19, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(15, 11, D1GfxPixel::colorPixel(91));
        }
        if (i == 92) { // 252[4]
            change |= frame->setPixel(19, 29, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(20, 24, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(20, 25, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(21, 17, D1GfxPixel::colorPixel(0));
            change |= frame->setPixel(21, 18, D1GfxPixel::colorPixel(0));
        }
        if (i == 93) { // 252[6]
            change |= frame->setPixel(25, 20, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(25, 21, D1GfxPixel::colorPixel(75));
            change |= frame->setPixel(25, 22, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(25, 23, D1GfxPixel::colorPixel(73));
            change |= frame->setPixel(26, 20, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(26, 21, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(26, 22, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(26, 23, D1GfxPixel::colorPixel(121));
        }
        if (i == 94) { // 376[0]
            change |= frame->setPixel(12, 7, D1GfxPixel::colorPixel(69));
            change |= frame->setPixel(5, 13, D1GfxPixel::colorPixel(70));
        }
        if (i == 95) { // 374[0]
            change |= frame->setPixel(1, 2, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(2, 1, D1GfxPixel::colorPixel(73));
        }
        if (i == 96) { // 374[6]
            change |= frame->setPixel(24, 26, D1GfxPixel::colorPixel(89));
            change |= frame->setPixel(24, 27, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(24, 28, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(24, 29, D1GfxPixel::colorPixel(93));
            change |= frame->setPixel(24, 30, D1GfxPixel::colorPixel(123));
            change |= frame->setPixel(24, 31, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(25, 20, D1GfxPixel::colorPixel(121));
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

void D1Tileset::cleanupCaves(std::set<unsigned> &deletedFrames, bool silent)
{
    constexpr int blockSize = BLOCK_SIZE_L3;

    // reuse subtiles
    // fix shadow
    ReplaceSubtile(this->til, 4 - 1, 2, 19 - 1, silent);  // 15
    ReplaceSubtile(this->til, 104 - 1, 2, 19 - 1, silent);  // 380
    ReplaceSubtile(this->til, 133 - 1, 2, 479 - 1, silent); // 487
    ReplaceSubtile(this->til, 137 - 1, 2, 495 - 1, silent); // 503
    // fix shine
    ReplaceSubtile(this->til, 85 - 1, 1, 398 - 1, silent); // 314
    // separate subtiles for the automap
    ReplaceSubtile(this->til, 51 - 1, 3, 258 - 1, silent);
    ReplaceSubtile(this->til, 111 - 1, 0, 28 - 1, silent);
    // useless pixels
    ReplaceSubtile(this->til, 42 - 1, 3, 8 - 1, silent); // 149
    ReplaceSubtile(this->til, 43 - 1, 3, 16 - 1, silent); // 153
    ReplaceSubtile(this->til, 104 - 1, 3, 16 - 1, silent); // 381
    ReplaceSubtile(this->til, 105 - 1, 3, 24 - 1, silent); // 384
    // use common subtiles
    ReplaceSubtile(this->til, 42 - 1, 2, 7 - 1, silent); // 148
    ReplaceSubtile(this->til, 43 - 1, 1, 14 - 1, silent); // 151
    ReplaceSubtile(this->til, 57 - 1, 0, 393 - 1, silent); // 204
    // ReplaceSubtile(this->til, 101 - 1, 1, 2 - 1, silent); // 367
    ReplaceSubtile(this->til, 101 - 1, 3, 4 - 1, silent); // 369
    ReplaceSubtile(this->til, 102 - 1, 0, 5 - 1, silent); // 370
    // ReplaceSubtile(this->til, 102 - 1, 1, 6 - 1, silent); // 371
    ReplaceSubtile(this->til, 103 - 1, 3, 16 - 1, silent);  // 377
    ReplaceSubtile(this->til, 143 - 1, 1, 506 - 1, silent); // 519
    ReplaceSubtile(this->til, 122 - 1, 1, 446 - 1, silent); // 442
    // - doors
    ReplaceSubtile(this->til, 146 - 1, 0, 490 - 1, silent); // 529
    ReplaceSubtile(this->til, 146 - 1, 2, 538 - 1, silent); // 531
    ReplaceSubtile(this->til, 146 - 1, 3, 539 - 1, silent); // 532
    ReplaceSubtile(this->til, 147 - 1, 0, 540 - 1, silent); // 533
    ReplaceSubtile(this->til, 147 - 1, 3, 542 - 1, silent); // 536
    ReplaceSubtile(this->til, 148 - 1, 0, 490 - 1, silent); // 537
    ReplaceSubtile(this->til, 148 - 1, 2, 537 - 1, silent); // - to make 537 'accessible'
    ReplaceSubtile(this->til, 149 - 1, 1, 533 - 1, silent); // (541) - to make 533 'accessible'
    // use common subtiles instead of minor alterations
    ReplaceSubtile(this->til, 6 - 1, 0, 393 - 1, silent); // 21 TODO: invisible subtiles(6, 105, 112)?
    ReplaceSubtile(this->til, 7 - 1, 0, 393 - 1, silent);
    ReplaceSubtile(this->til, 105 - 1, 0, 393 - 1, silent);
    ReplaceSubtile(this->til, 42 - 1, 1, 69 - 1, silent); // 147 (another option: 108)
    ReplaceSubtile(this->til, 43 - 1, 2, 86 - 1, silent); // 152
    ReplaceSubtile(this->til, 54 - 1, 3, 231 - 1, silent); // 195
    // ReplaceSubtile(this->til, 66 - 1, 0, 224 - 1, silent); // 240
    // ReplaceSubtile(this->til, 66 - 1, 1, 225 - 1, silent); // 241
    // ReplaceSubtile(this->til, 66 - 1, 3, 227 - 1, silent); // 243
    ReplaceSubtile(this->til, 68 - 1, 2, 34 - 1, silent); // 250
    ReplaceSubtile(this->til, 68 - 1, 3, 35 - 1, silent); // 251
    ReplaceSubtile(this->til, 69 - 1, 1, 37 - 1, silent); // 253
    ReplaceSubtile(this->til, 69 - 1, 3, 39 - 1, silent); // 255
    ReplaceSubtile(this->til, 50 - 1, 1, 29 - 1, silent); // 179
    ReplaceSubtile(this->til, 89 - 1, 1, 29 - 1, silent); // 328
    ReplaceSubtile(this->til, 92 - 1, 1, 29 - 1, silent); // 338
    ReplaceSubtile(this->til, 95 - 1, 1, 29 - 1, silent); // 349
    ReplaceSubtile(this->til, 98 - 1, 1, 29 - 1, silent); // 358
    ReplaceSubtile(this->til, 99 - 1, 1, 29 - 1, silent); // 361
    ReplaceSubtile(this->til, 100 - 1, 1, 29 - 1, silent); // 364
    ReplaceSubtile(this->til, 84 - 1, 2, 30 - 1, silent); // 312
    ReplaceSubtile(this->til, 90 - 1, 2, 30 - 1, silent); // 332
    ReplaceSubtile(this->til, 92 - 1, 2, 30 - 1, silent); // 339
    ReplaceSubtile(this->til, 96 - 1, 2, 30 - 1, silent); // 353
    ReplaceSubtile(this->til, 97 - 1, 2, 30 - 1, silent); // 356
    ReplaceSubtile(this->til, 100 - 1, 2, 30 - 1, silent); // 365
    ReplaceSubtile(this->til, 123 - 1, 3, 440 - 1, silent); // 448
    // ignore invisible parts
    ReplaceSubtile(this->til, 1 - 1, 1, 334 - 1, silent); // 2
    ReplaceSubtile(this->til, 101 - 1, 1, 334 - 1, silent); // 367
    ReplaceSubtile(this->til, 2 - 1, 1, 53 - 1, silent); // 6
    ReplaceSubtile(this->til, 5 - 1, 1, 53 - 1, silent); // 18
    ReplaceSubtile(this->til, 102 - 1, 1, 53 - 1, silent); // 371

    // - shadows
    // ReplaceSubtile(this->til, 57 - 1, 0, 385 - 1, silent);
    ReplaceSubtile(this->til, 57 - 1, 1, 386 - 1, silent); // 205
    ReplaceSubtile(this->til, 61 - 1, 0, 385 - 1, silent); // 220
    ReplaceSubtile(this->til, 61 - 1, 1, 386 - 1, silent); // 221
    ReplaceSubtile(this->til, 58 - 1, 0, 389 - 1, silent); // 208
    ReplaceSubtile(this->til, 58 - 1, 1, 390 - 1, silent); // 209
    ReplaceSubtile(this->til, 58 - 1, 3, 392 - 1, silent); // 211
    ReplaceSubtile(this->til, 60 - 1, 2, 391 - 1, silent); // 218
    ReplaceSubtile(this->til, 62 - 1, 0, 389 - 1, silent); // 224
    ReplaceSubtile(this->til, 62 - 1, 1, 390 - 1, silent); // 225
    ReplaceSubtile(this->til, 62 - 1, 2, 242 - 1, silent); // 226 - after patchCavesFloor
    ReplaceSubtile(this->til, 62 - 1, 3, 392 - 1, silent); // 227
    ReplaceSubtile(this->til, 61 - 1, 2, 238 - 1, silent); // 222 - after patchCavesFloor
    ReplaceSubtile(this->til, 66 - 1, 0, 389 - 1, silent); // 240
    ReplaceSubtile(this->til, 66 - 1, 1, 390 - 1, silent); // 241
    ReplaceSubtile(this->til, 66 - 1, 3, 392 - 1, silent); // 243

    // - lava
    ReplaceSubtile(this->til, 44 - 1, 1, 57 - 1, silent); // 155
    ReplaceSubtile(this->til, 44 - 1, 3, 63 - 1, silent); // 157
    ReplaceSubtile(this->til, 45 - 1, 2, 74 - 1, silent); // 160
    ReplaceSubtile(this->til, 45 - 1, 3, 71 - 1, silent); // 161
    // - one-subtile islands
    ReplaceSubtile(this->til, 26 - 1, 2, 117 - 1, silent); // 102
    ReplaceSubtile(this->til, 34 - 1, 2, 117 - 1, silent); // 128

    // create new fences
    ReplaceSubtile(this->til, 144 - 1, 0, 505 - 1, silent);
    ReplaceSubtile(this->til, 144 - 1, 1, 528 - 1, silent);
    ReplaceSubtile(this->til, 144 - 1, 2, 516 - 1, silent);
    ReplaceSubtile(this->til, 144 - 1, 3, 536 - 1, silent);
    ReplaceSubtile(this->til, 145 - 1, 0, 505 - 1, silent);
    ReplaceSubtile(this->til, 145 - 1, 1, 515 - 1, silent);
    ReplaceSubtile(this->til, 145 - 1, 2, 551 - 1, silent);
    ReplaceSubtile(this->til, 145 - 1, 3, 532 - 1, silent);

    // eliminate subtiles of unused tiles
    const int unusedTiles[] = {
        70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 112, 113, 114, 115, 116, 117, 118, 119, 120, 153, 154, 155, 156,
    };
    constexpr int blankSubtile = 2 - 1;
    for (int n = 0; n < lengthof(unusedTiles); n++) {
        int tileId = unusedTiles[n];
        ReplaceSubtile(this->til, tileId - 1, 0, blankSubtile, silent);
        ReplaceSubtile(this->til, tileId - 1, 1, blankSubtile, silent);
        ReplaceSubtile(this->til, tileId - 1, 2, blankSubtile, silent);
        ReplaceSubtile(this->til, tileId - 1, 3, blankSubtile, silent);
    }

    patchCavesFloor(silent);
    patchCavesStairs(silent);
    patchCavesWall1(silent);
    patchCavesWall2(silent);

    // use the new shadows
    SetMcr(551, 0, 26, 0);

    SetMcr(536, 0, 520, 0);
    ReplaceMcr(536, 1, 508, 1);
    ReplaceMcr(536, 2, 520, 2);
    ReplaceMcr(536, 3, 545, 3);
    SetMcr(536, 4, 508, 4);
    ReplaceMcr(536, 5, 545, 5);
    SetMcr(536, 6, 508, 6);
    ReplaceMcr(536, 7, 545, 7);

    ReplaceMcr(532, 0, 508, 0);
    SetMcr(532, 1, 517, 1);
    ReplaceMcr(532, 2, 545, 2);
    SetMcr(532, 3, 517, 3);
    ReplaceMcr(532, 4, 545, 4);
    SetMcr(532, 5, 517, 5);
    ReplaceMcr(532, 6, 545, 6);
    SetMcr(532, 7, 517, 7);

    // fix shadow
    ReplaceMcr(501, 0, 493, 0);
    ReplaceMcr(512, 0, 530, 0);
    ReplaceMcr(544, 0, 495, 0);
    SetMcr(544, 1, 435, 1);

    // fix shine
    ReplaceMcr(325, 0, 346, 0);
    ReplaceMcr(325, 2, 38, 2);

    // fix bad artifact
    Blk2Mcr(82, 4);

    // pointless door micros (re-drawn by dSpecial or the object)
    // - vertical doors    
    Blk2Mcr(540, 3);
    Blk2Mcr(540, 5);
    ReplaceMcr(534, 0, 541, 0);
    ReplaceMcr(534, 1, 541, 1);
    Blk2Mcr(534, 2);
    ReplaceMcr(534, 3, 541, 3);
    Blk2Mcr(534, 4);
    ReplaceMcr(534, 5, 541, 5);
    ReplaceMcr(534, 7, 541, 7);
    ReplaceMcr(533, 0, 541, 0);
    ReplaceMcr(533, 1, 541, 1);
    SetMcr(533, 3, 541, 3);
    SetMcr(533, 5, 541, 5);
    SetMcr(533, 7, 541, 7);
    // - horizontal doors
    // Blk2Mcr(537, 2);
    // Blk2Mcr(537, 4);
    // ReplaceMcr(531, 0, 538, 0);
    // ReplaceMcr(531, 1, 538, 1);
    // ReplaceMcr(531, 2, 538, 2);
    // Blk2Mcr(531, 3);
    // ReplaceMcr(531, 4, 538, 4);
    // Blk2Mcr(531, 5);
    ReplaceMcr(537, 0, 538, 0);
    ReplaceMcr(537, 1, 538, 1);
    ReplaceMcr(537, 2, 538, 2);
    ReplaceMcr(537, 4, 538, 4);
    SetMcr(537, 6, 538, 6);
    // pointless pixels
    Blk2Mcr(7, 7);
    Blk2Mcr(14, 6);
    Blk2Mcr(82, 4);
    Blk2Mcr(382, 6);
    Blk2Mcr(4, 1);
    Blk2Mcr(4, 3);
    Blk2Mcr(4, 4);
    Blk2Mcr(4, 6);
    Blk2Mcr(8, 1);
    Blk2Mcr(8, 3);
    Blk2Mcr(8, 4);
    Blk2Mcr(8, 6);
    Blk2Mcr(12, 1);
    Blk2Mcr(12, 3);
    Blk2Mcr(12, 5);
    Blk2Mcr(12, 7);
    Blk2Mcr(16, 5);
    Blk2Mcr(16, 7);
    Blk2Mcr(20, 1);
    Blk2Mcr(20, 3);
    Blk2Mcr(24, 4);
    Blk2Mcr(24, 5);
    Blk2Mcr(24, 6);
    Blk2Mcr(24, 7);
    Blk2Mcr(373, 1);
    Blk2Mcr(373, 3);
    Blk2Mcr(373, 4);
    // Blk2Mcr(381, 5);
    // Blk2Mcr(381, 7);
    Blk2Mcr(511, 1);
    Blk2Mcr(511, 4);
    Blk2Mcr(511, 6);
    HideMcr(514, 5);
    HideMcr(514, 7);
    Blk2Mcr(16, 0);
    Blk2Mcr(16, 2);
    Blk2Mcr(20, 0);
    Blk2Mcr(20, 2);
    // Blk2Mcr(377, 0);
    // Blk2Mcr(377, 2);
    // Blk2Mcr(377, 5);
    // Blk2Mcr(381, 0);
    // Blk2Mcr(381, 2);
    Blk2Mcr(514, 0);
    // - moved pixels
    HideMcr(146, 2);
    Blk2Mcr(146, 3);
    Blk2Mcr(146, 4);
    HideMcr(146, 5);
    Blk2Mcr(150, 2);
    HideMcr(150, 3);
    Blk2Mcr(150, 5);
    Blk2Mcr(174, 4);
    // -  by patchCavesStairs
    Blk2Mcr(180, 2);
    ReplaceMcr(180, 4, 30, 4);
    Blk2Mcr(174, 2);
    Blk2Mcr(176, 2);
    Blk2Mcr(163, 3);

    // mask walls
    // after patchCavesWallCel
    Blk2Mcr(162, 3);
    Blk2Mcr(162, 5);
    Blk2Mcr(162, 7);

    // separate subtiles for the automap
    Blk2Mcr(258, 0);
    Blk2Mcr(406, 0);
    Blk2Mcr(406, 1);
    Blk2Mcr(406, 3);
    ReplaceMcr(406, 5, 29, 5);
    ReplaceMcr(406, 7, 29, 7);
    Blk2Mcr(407, 1);

    // reuse subtiles
    // ReplaceMcr(209, 1, 25, 1);

    ReplaceMcr(481, 6, 437, 6);
    ReplaceMcr(481, 7, 437, 7);
    ReplaceMcr(482, 3, 438, 3);
    ReplaceMcr(484, 1, 476, 1); // lost details

    ReplaceMcr(516, 2, 507, 2);
    ReplaceMcr(516, 4, 507, 4);
    ReplaceMcr(516, 6, 507, 6);

    ReplaceMcr(520, 1, 508, 1);
    ReplaceMcr(520, 3, 508, 3);
    ReplaceMcr(517, 4, 508, 4);
    ReplaceMcr(520, 4, 508, 4);
    ReplaceMcr(546, 4, 508, 4);
    ReplaceMcr(517, 6, 508, 6);
    ReplaceMcr(520, 6, 508, 6);
    ReplaceMcr(546, 6, 508, 6);
    ReplaceMcr(546, 2, 520, 2);
    ReplaceMcr(547, 5, 517, 5);

    ReplaceMcr(44, 7, 1, 7);
    ReplaceMcr(1, 5, 44, 5);

    ReplaceMcr(166, 7, 32, 7);
    ReplaceMcr(166, 6, 32, 6);
    ReplaceMcr(313, 7, 32, 7);
    ReplaceMcr(333, 7, 32, 7);
    ReplaceMcr(473, 7, 32, 7);
    ReplaceMcr(473, 6, 32, 6);
    ReplaceMcr(473, 5, 32, 5);

    ReplaceMcr(401, 7, 36, 7);
    ReplaceMcr(401, 6, 36, 6);
    ReplaceMcr(477, 7, 36, 7);
    ReplaceMcr(477, 6, 36, 6);
    ReplaceMcr(477, 4, 36, 4);

    ReplaceMcr(252, 3, 36, 3);
    ReplaceMcr(252, 1, 36, 1);

    ReplaceMcr(478, 1, 37, 1);
    ReplaceMcr(478, 7, 37, 7);
    ReplaceMcr(480, 1, 39, 1);

    ReplaceMcr(457, 6, 465, 6);
    ReplaceMcr(469, 6, 465, 6);
    ReplaceMcr(457, 4, 465, 4);
    ReplaceMcr(469, 4, 465, 4);
    ReplaceMcr(457, 2, 465, 2);

    ReplaceMcr(485, 6, 465, 6);
    ReplaceMcr(485, 4, 465, 4);
    ReplaceMcr(485, 7, 457, 7);
    // ReplaceMcr(487, 2, 38, 2);

    ReplaceMcr(486, 7, 458, 7);

    ReplaceMcr(88, 7, 32, 7);
    ReplaceMcr(397, 6, 32, 6);
    ReplaceMcr(397, 7, 32, 7);
    ReplaceMcr(397, 5, 32, 5);
    ReplaceMcr(313, 5, 88, 5);

    ReplaceMcr(342, 6, 42, 6); // lost details

    ReplaceMcr(437, 7, 441, 7);
    ReplaceMcr(437, 5, 441, 5);
    ReplaceMcr(481, 7, 441, 7);
    ReplaceMcr(481, 5, 441, 5);
    ReplaceMcr(483, 6, 439, 6);

    ReplaceMcr(164, 6, 34, 6);
    ReplaceMcr(164, 4, 34, 4);
    ReplaceMcr(475, 6, 34, 6);
    ReplaceMcr(475, 0, 34, 0);

    ReplaceMcr(92, 6, 374, 6);
    ReplaceMcr(92, 4, 461, 4);
    ReplaceMcr(344, 6, 461, 6);
    ReplaceMcr(374, 4, 461, 4);
    ReplaceMcr(374, 2, 461, 2);
    ReplaceMcr(44, 6, 461, 6);
    ReplaceMcr(44, 4, 461, 4);
    ReplaceMcr(9, 6, 461, 6);
    ReplaceMcr(9, 4, 461, 4);
    ReplaceMcr(9, 2, 36, 2);

    ReplaceMcr(438, 3, 446, 3);
    ReplaceMcr(45, 3, 446, 3);
    ReplaceMcr(450, 3, 446, 3);
    ReplaceMcr(482, 3, 446, 3);
    ReplaceMcr(89, 3, 446, 3);
    // ReplaceMcr(2, 3, 446, 3);
    ReplaceMcr(334, 3, 446, 3);

    // ReplaceMcr(292, 4, 254, 4);
    // ReplaceMcr(163, 3, 33, 3);
    ReplaceMcr(167, 3, 33, 3);
    ReplaceMcr(249, 3, 33, 3);
    ReplaceMcr(398, 3, 33, 3);
    ReplaceMcr(474, 3, 33, 3);

    ReplaceMcr(11, 2, 38, 2);
    ReplaceMcr(467, 2, 38, 2);
    ReplaceMcr(471, 2, 38, 2);
    ReplaceMcr(459, 2, 38, 2);
    ReplaceMcr(479, 2, 38, 2);
    ReplaceMcr(46, 2, 38, 2);
    ReplaceMcr(94, 2, 38, 2);
    ReplaceMcr(172, 2, 38, 2);
    ReplaceMcr(346, 2, 38, 2);
    ReplaceMcr(376, 2, 38, 2);
    ReplaceMcr(403, 2, 38, 2);
    ReplaceMcr(463, 2, 38, 2);

    // ReplaceMcr(146, 4, 8, 6);
    // ReplaceMcr(146, 3, 8, 5);
    ReplaceMcr(146, 1, 104, 1);

    // ReplaceMcr(150, 5, 8, 6);
    // ReplaceMcr(150, 2, 8, 5);
    ReplaceMcr(150, 0, 56, 0); // or 140

    ReplaceMcr(4, 5, 8, 5); // lost details

    ReplaceMcr(511, 3, 496, 3);
    ReplaceMcr(509, 1, 493, 1);

    ReplaceMcr(438, 1, 450, 1);
    ReplaceMcr(336, 0, 35, 0);
    ReplaceMcr(452, 0, 440, 0); // lost details

    ReplaceMcr(12, 4, 16, 4);
    ReplaceMcr(20, 4, 16, 4);
    // ReplaceMcr(381, 4, 16, 4);
    // ReplaceMcr(377, 4, 16, 4); // lost details
    // ReplaceMcr(12, 5, 16, 5);
    // ReplaceMcr(381, 5, 16, 5);

    ReplaceMcr(13, 0, 47, 0); // lost details
    ReplaceMcr(378, 0, 17, 0); // lost details

    // ReplaceMcr(24, 4, 28, 2);
    // ReplaceMcr(24, 5, 28, 3);
    ReplaceMcr(3, 4, 30, 4);
    ReplaceMcr(3, 6, 30, 6);
    ReplaceMcr(368, 4, 30, 4);
    ReplaceMcr(368, 6, 30, 6);
    ReplaceMcr(183, 6, 30, 6);
    ReplaceMcr(350, 6, 30, 6);
    ReplaceMcr(359, 4, 30, 4);
    ReplaceMcr(362, 4, 30, 4);

    ReplaceMcr(10, 7, 29, 7);
    ReplaceMcr(10, 5, 29, 5);
    ReplaceMcr(375, 7, 29, 7);
    ReplaceMcr(375, 5, 29, 5);
    ReplaceMcr(355, 5, 29, 5);

    ReplaceMcr(40, 7, 28, 7);
    ReplaceMcr(453, 7, 28, 7);

    ReplaceMcr(317, 3, 28, 3); // lost details
    // ReplaceMcr(317, 7, 28, 7); // lost details + adjust 317[5]?
    ReplaceMcr(327, 2, 28, 2);
    ReplaceMcr(327, 3, 28, 3);
    ReplaceMcr(327, 5, 28, 5); // lost details

    ReplaceMcr(337, 3, 28, 3);
    ReplaceMcr(337, 7, 28, 7);

    ReplaceMcr(348, 2, 28, 2); // lost details
    ReplaceMcr(348, 3, 28, 3);
    ReplaceMcr(348, 6, 28, 6);
    ReplaceMcr(348, 7, 28, 7);

    ReplaceMcr(351, 2, 28, 2);
    ReplaceMcr(351, 3, 28, 3);
    ReplaceMcr(351, 6, 28, 6);
    ReplaceMcr(351, 7, 28, 7);

    ReplaceMcr(357, 2, 28, 2);
    ReplaceMcr(357, 3, 28, 3);
    ReplaceMcr(357, 5, 28, 5);

    ReplaceMcr(360, 2, 28, 2);
    ReplaceMcr(360, 3, 28, 3);
    ReplaceMcr(360, 5, 28, 5);
    ReplaceMcr(360, 7, 28, 7);

    ReplaceMcr(363, 2, 28, 2);
    ReplaceMcr(363, 3, 28, 3);

    ReplaceMcr(354, 2, 28, 2);
    ReplaceMcr(354, 3, 28, 3);
    ReplaceMcr(354, 6, 28, 6);

    ReplaceMcr(178, 2, 28, 2);
    ReplaceMcr(178, 3, 28, 3);
    ReplaceMcr(178, 5, 28, 5);

    ReplaceMcr(181, 6, 28, 6);
    // ReplaceMcr(384, 4, 28, 2);
    // ReplaceMcr(384, 5, 28, 3);

    ReplaceMcr(311, 7, 29, 7);
    ReplaceMcr(310, 2, 28, 2);
    ReplaceMcr(310, 6, 28, 6);
    ReplaceMcr(310, 7, 28, 7);

    ReplaceMcr(330, 2, 28, 2);
    ReplaceMcr(330, 3, 28, 3);
    ReplaceMcr(330, 4, 28, 4);
    ReplaceMcr(330, 6, 28, 6);

    ReplaceMcr(202, 0, 26, 0);
    ReplaceMcr(234, 0, 391, 0);
    ReplaceMcr(219, 0, 392, 0);
    ReplaceMcr(242, 1, 391, 1);

    ReplaceMcr(53, 1, 386, 1); // lost details
    ReplaceMcr(189, 0, 213, 0); // lost details
    ReplaceMcr(189, 1, 213, 1); // lost details
    ReplaceMcr(189, 3, 213, 3); // lost details
    ReplaceMcr(194, 0, 230, 0); // lost details
    ReplaceMcr(194, 1, 230, 1); // lost details
    ReplaceMcr(194, 2, 230, 2); // lost details
    ReplaceMcr(229, 0, 193, 0);
    ReplaceMcr(229, 1, 193, 1);
    ReplaceMcr(229, 3, 193, 3); // lost details
    ReplaceMcr(228, 0, 192, 0); // lost details
    ReplaceMcr(228, 1, 192, 1); // lost details
    ReplaceMcr(228, 3, 192, 3); // lost details

    // ignore invisible parts
    ReplaceMcr(1, 1, 333, 1); // lost details
    ReplaceMcr(1, 3, 333, 3); // lost details
    ReplaceMcr(366, 1, 333, 1); // lost details
    ReplaceMcr(366, 3, 333, 3); // lost details
    ReplaceMcr(5, 1, 52, 1); // lost details
    ReplaceMcr(17, 1, 52, 1); // lost details
    // ReplaceMcr(370, 1, 52, 1); // lost details

    // - one-subtile islands
    ReplaceMcr(81, 0, 97, 0);

    // eliminate micros of unused subtiles
    // Blk2Mcr(148, 151,  ...),
    Blk2Mcr(102, 1);
    Blk2Mcr(128, 1);
    Blk2Mcr(283, 0);
    Blk2Mcr(353, 4);
    Blk2Mcr(370, 1);
    Blk2Mcr(405, 1);
    Blk2Mcr(405, 3);
    Blk2Mcr(405, 5);
    Blk2Mcr(409, 1);
    Blk2Mcr(436, 1);
    Blk2Mcr(436, 3);
    Blk2Mcr(436, 4);
    Blk2Mcr(436, 5);
    Blk2Mcr(436, 6);
    Blk2Mcr(519, 0);
    Blk2Mcr(531, 0);
    Blk2Mcr(531, 1);
    Blk2Mcr(531, 2);
    Blk2Mcr(531, 3);
    Blk2Mcr(531, 4);
    Blk2Mcr(531, 5);
    Blk2Mcr(548, 2);
    Blk2Mcr(548, 5);
    Blk2Mcr(549, 0);
    Blk2Mcr(549, 1);
    Blk2Mcr(549, 3);
    Blk2Mcr(435, 0);
    // reused for the new shadows
    // Blk2Mcr(435, 1);
    // Blk2Mcr(551, 1);
    // Blk2Mcr(528, 0 .. 1);
    // Blk2Mcr(532, 0);
    // Blk2Mcr(532, 2);
    // Blk2Mcr(532, 4);
    // Blk2Mcr(532, 6);
    // Blk2Mcr(536, 1);
    // Blk2Mcr(536, 2);
    // Blk2Mcr(536, 3);
    // Blk2Mcr(536, 5);
    // Blk2Mcr(536, 7);
    Blk2Mcr(552, 0);
    Blk2Mcr(552, 2);
    Blk2Mcr(552, 3);
    Blk2Mcr(552, 4);
    Blk2Mcr(552, 5);
    Blk2Mcr(553, 5);
    Blk2Mcr(555, 1);
    Blk2Mcr(556, 2);
    Blk2Mcr(556, 3);
    Blk2Mcr(556, 5);
    Blk2Mcr(556, 6);
    Blk2Mcr(559, 4);
    Blk2Mcr(559, 5);
    Blk2Mcr(559, 6);
    Blk2Mcr(559, 7);

    const int unusedSubtiles[] = {
        2, 6, 15, 18, 21, 147, 149, 152, 153, 155, 157, 160, 161, 179, 195, 204, 205, 208, 209, 211, 218, 220, 221, 222, 224, 225, 226, 227, 240, 241, 243, 250, 251, 253, 255, 256, 257, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 312, 314, 328, 332, 338, 339, 349, 356, 358, 361, 364, 365, 367, 369, 371, 377, 380, 381, 384, 408, 410, 411, 412, 413, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423, 424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 442, 448, 487, 503, 521, 522, 523, 524, 525, 526, 527, 529, 550, 554, 557, 558, 560
    };

    for (int n = 0; n < lengthof(unusedSubtiles); n++) {
        for (int i = 0; i < blockSize; i++) {
            Blk2Mcr(unusedSubtiles[n], i);
        }
    }
}

bool D1Tileset::patchHellChaos(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{  54 - 1, 5, D1CEL_FRAME_TYPE::RightTriangle },   // redraw subtiles 
/*  1 */{  54 - 1, 7, D1CEL_FRAME_TYPE::Empty },           // (used to block subsequent calls)
/*  2 */{  55 - 1, 4, D1CEL_FRAME_TYPE::LeftTriangle },
/*  3 */{ /*55*/ - 1, 6, D1CEL_FRAME_TYPE::Empty },
/*  4 */{  53 - 1, 2, D1CEL_FRAME_TYPE::LeftTrapezoid },
/*  5 */{  53 - 1, 3, D1CEL_FRAME_TYPE::RightTrapezoid },
/*  6 */{  53 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/*  7 */{  53 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/*  8 */{ /*53*/ - 1, 6, D1CEL_FRAME_TYPE::Empty },
/*  9 */{ /*53*/ - 1, 7, D1CEL_FRAME_TYPE::Empty },

/* 10 */{  48 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 11 */{  52 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 12 */{  58 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 13 */{  67 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },

/* 14 */{  52 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 15 */{  67 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 16 */{  76 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },

/* 17 */{  46 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 18 */{  50 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 19 */{  56 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 20 */{  62 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 21 */{  65 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 22 */{  68 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 23 */{  74 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },

/* 24 */{  46 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 25 */{  50 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 26 */{  56 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 27 */{  59 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 28 */{  65 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 29 */{  77 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/* 30 */{  74 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 31 */{ /*77*/ - 1, 6, D1CEL_FRAME_TYPE::Empty },

/* 32 */{ /*74*/ - 1, 7, D1CEL_FRAME_TYPE::Empty },
/* 33 */{  77 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },

/* 34 */{  47 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 35 */{  51 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 36 */{  66 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/* 37 */{  47 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 38 */{  66 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 39 */{  /*78*/ - 1, 7, D1CEL_FRAME_TYPE::Empty },
    };

    constexpr unsigned blockSize = BLOCK_SIZE_L4;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // mask 54[5]
        if (i == 0) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < 1 + x / 2 || y > 31 - x / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 55[4]
        if (i == 2) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < 16 - x / 2 || y > 16 + x / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 53[2]
        if (i == 4) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y > 16 + x / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 53[3]
        if (i == 5) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y > 31 - x / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 53[4]
        if (i == 6) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < 16 - x / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 53[5]
        if (i == 7) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < 1 + x / 2) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // redraw 48[4], 52[4], 58[4], 67[4] using 55[4]
        if (i >= 10 && i < 14) {
            const CelMicro &microSrc = micros[2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    D1GfxPixel pixelSrc = frameSrc->getPixel(x, y); // 55[4]
                    if (!pixel.isTransparent() && pixel.getPaletteIndex() != 0 && pixel.getPaletteIndex() < 32 /*&& !pixelSrc.isTransparent()*/ || (pixel.isTransparent() && !pixelSrc.isTransparent())) {
                        change |= frame->setPixel(x, y, pixelSrc);
                    }
                }
            }
        }
        // redraw 52[6], 67[6], 76[6] using '55[6]'
        if (i >= 14 && i < 17) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (pixel.getPaletteIndex() != 0 && pixel.getPaletteIndex() < 32) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // redraw 46[4], 50[4], 56[4], 62[4], 65[4], 68[4], 74[4] using 53[4]
        if (i >= 17 && i < 24) {
            const CelMicro &microSrc = micros[6];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    D1GfxPixel pixelSrc = frameSrc->getPixel(x, y); // 53[4]
                    if (!pixel.isTransparent() && pixel.getPaletteIndex() != 0 && pixel.getPaletteIndex() < 32 /*&& !pixelSrc.isTransparent()*/ || (pixel.isTransparent() && !pixelSrc.isTransparent())) {
                        change |= frame->setPixel(x, y, pixelSrc);
                    }
                }
            }
        }
        // redraw 46[5], 50[5], 56[5], 59[5], 65[5], 77[5] using 53[5]
        if (i >= 24 && i < 30) {
            const CelMicro &microSrc = micros[7];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    D1GfxPixel pixelSrc = frameSrc->getPixel(x, y); // 53[5]
                    if (!pixel.isTransparent() && pixel.getPaletteIndex() != 0 && pixel.getPaletteIndex() < 32 /*&& !pixelSrc.isTransparent()*/ || (pixel.isTransparent() && !pixelSrc.isTransparent())) {
                        if (pixelSrc.isTransparent() && x < 17) {
                            continue; // preserve 'missing wall-pixel' of 46[5]
                        }
                        change |= frame->setPixel(x, y, pixelSrc);
                    }
                }
            }
        }
        // redraw 74[6], 77[6] using '53[6]'
        if (i >= 30 && i < 32) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (pixel.getPaletteIndex() != 0 && pixel.getPaletteIndex() < 32) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // redraw 74[7], 77[7] using '53[7]'
        if (i >= 32 && i < 34) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent() && pixel.getPaletteIndex() != 0 && pixel.getPaletteIndex() < 32) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // redraw 47[5], 51[5], 66[5] using 54[5]
        if (i >= 34 && i < 37) {
            const CelMicro &microSrc = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    D1GfxPixel pixelSrc = frameSrc->getPixel(x, y); // 54[5]
                    if (!pixel.isTransparent() && pixel.getPaletteIndex() != 0 && pixel.getPaletteIndex() < 32 /*&& !pixelSrc.isTransparent()*/ || (pixel.isTransparent() && !pixelSrc.isTransparent())) {
                        change |= frame->setPixel(x, y, pixelSrc);
                    }
                }
            }
        }
        // redraw 47[7], 66[7] (using 54[7])
        if (i >= 37 && i < 40) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent() && pixel.getPaletteIndex() != 0 && pixel.getPaletteIndex() < 32) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        if (i == 29) { // fix 77[5]
            change |= frame->setPixel(28, 9, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(29, 1, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(29, 2, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(29, 3, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(29, 4, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(29, 5, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(29, 6, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(29, 7, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(30, 3, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(30, 4, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(30, 5, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(30, 6, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(30, 7, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 6, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 7, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 8, D1GfxPixel::transparentPixel());
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

bool D1Tileset::patchHellFloor(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{ 129 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // fix micros
/*  1 */{ 171 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  2 */{ 172 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  3 */{ 187 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  4 */{ 200 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid },
/*  5 */{ 200 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/*  6 */{ 225 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  7 */{ 227 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  8 */{ 228 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/*  9 */{ 228 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 10 */{ /*265*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 11 */{ 272 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 12 */{ 277 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 13 */{ 289 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 14 */{ 302 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 15 */{ 337 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 16 */{ 371 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 17 */{ 340 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 18 */{ 373 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },

/* 19 */{   1 - 1, 5, D1CEL_FRAME_TYPE::Empty },
/* 20 */{ 188 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/* 21 */{   5 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 22 */{   8 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 23 */{  15 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 24 */{  50 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 25 */{  50 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 26 */{ 180 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 27 */{ 212 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 28 */{ 251 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 29 */{   1 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 30 */{  11 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 31 */{ 196 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 32 */{ 204 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 33 */{ 208 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 34 */{ 254 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },

/* 35 */{   4 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 36 */{ 155 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 37 */{ 152 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 38 */{ 238 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 39 */{ 238 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
    };
    constexpr unsigned blockSize = BLOCK_SIZE_L4;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        if (i == 0) { // 129[1] - adjust after reuse
            change |= frame->setPixel(0, 25, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(0, 26, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(0, 27, D1GfxPixel::colorPixel(42));
            change |= frame->setPixel(0, 28, D1GfxPixel::colorPixel(40));
        }
        if (i == 1) { // 171[1] - adjust shadow after reuse
            change |= frame->setPixel(29, 15, D1GfxPixel::colorPixel(42));
        }
        if (i == 3) { // 187[1] - fix glitch
            change |= frame->setPixel(11, 26, D1GfxPixel::colorPixel(40));
        }

        if (i == 4) { // 200[1] - fix glitch(?)
            change |= frame->setPixel(0, 11, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(1, 10, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(1, 11, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(1, 12, D1GfxPixel::colorPixel(115));

            change |= frame->setPixel(0, 14, D1GfxPixel::colorPixel(118));
            change |= frame->setPixel(0, 15, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(0, 16, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(0, 17, D1GfxPixel::colorPixel(116));

            change |= frame->setPixel(0, 26, D1GfxPixel::colorPixel(43));
            change |= frame->setPixel(1, 26, D1GfxPixel::colorPixel(44));
            change |= frame->setPixel(1, 25, D1GfxPixel::colorPixel(41));
        }
        if (i == 5) { // 200[4] - adjust after reuse
            change |= frame->setPixel(14, 0, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(15, 0, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(16, 0, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(17, 0, D1GfxPixel::colorPixel(116));

            change |= frame->setPixel(21, 0, D1GfxPixel::colorPixel(113));
            change |= frame->setPixel(22, 0, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(23, 0, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(24, 0, D1GfxPixel::colorPixel(116));
            change |= frame->setPixel(25, 0, D1GfxPixel::colorPixel(115));

            change |= frame->setPixel(15, 1, D1GfxPixel::colorPixel(89));
            change |= frame->setPixel(16, 1, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(17, 1, D1GfxPixel::colorPixel(116));
            change |= frame->setPixel(16, 2, D1GfxPixel::colorPixel(119));
        }
        if (i == 6) { // 225[0] - adjust after reuse
            change |= frame->setPixel(30, 5, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(31, 5, D1GfxPixel::colorPixel(39));
            change |= frame->setPixel(31, 8, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(31, 15, D1GfxPixel::colorPixel(39));
        }
        if (i == 7) { // 227[1] - fix shadow
            change |= frame->setPixel(8, 10, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(8, 11, D1GfxPixel::colorPixel(111));
            change |= frame->setPixel(8, 12, D1GfxPixel::colorPixel(111));
            change |= frame->setPixel(8, 13, D1GfxPixel::colorPixel(126));
        }
        if (i == 8) { // 228[3] - fix 'shadow'
            change |= frame->setPixel(8, 10, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(6, 19, D1GfxPixel::colorPixel(108));
            change |= frame->setPixel(6, 20, D1GfxPixel::colorPixel(107));
            change |= frame->setPixel(6, 23, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(6, 25, D1GfxPixel::colorPixel(109));
            change |= frame->setPixel(6, 26, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(6, 27, D1GfxPixel::colorPixel(109));
            change |= frame->setPixel(6, 28, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(6, 29, D1GfxPixel::colorPixel(107));
        }
        if (i == 9) { // 228[5] - fix shadow after reuse + fix glare
            change |= frame->setPixel(15, 24, D1GfxPixel::colorPixel(126));

            change |= frame->setPixel(0, 24, D1GfxPixel::colorPixel(116));
        }
        if (i == 11) { // 272[0] - adjust after reuse
            change |= frame->setPixel(31, 15, D1GfxPixel::colorPixel(43));
            change |= frame->setPixel(30, 3, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(31, 3, D1GfxPixel::colorPixel(43));
        }
        if (i == 13) { // 289[1] - adjust after reuse
            change |= frame->setPixel(0, 2, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(1, 2, D1GfxPixel::colorPixel(42));
            change |= frame->setPixel(0, 25, D1GfxPixel::colorPixel(53));
            change |= frame->setPixel(0, 21, D1GfxPixel::colorPixel(42));
            change |= frame->setPixel(0, 22, D1GfxPixel::colorPixel(40));
        }
        if (i == 14) { // 302[0] - adjust after reuse
            change |= frame->setPixel(31, 5, D1GfxPixel::colorPixel(39));
            change |= frame->setPixel(31, 12, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(31, 13, D1GfxPixel::colorPixel(39));
        }
        // 337[0], 371[0] - adjust after reuse
        if (i >= 15 && i < 17) {
            change |= frame->setPixel(31, 7, D1GfxPixel::colorPixel(38));
            change |= frame->setPixel(31, 8, D1GfxPixel::colorPixel(39));
            change |= frame->setPixel(31, 9, D1GfxPixel::colorPixel(40));

            change |= frame->setPixel(30, 25, D1GfxPixel::colorPixel(42));
            change |= frame->setPixel(31, 25, D1GfxPixel::colorPixel(40));

            change |= frame->setPixel(29, 28, D1GfxPixel::colorPixel(39));
            change |= frame->setPixel(30, 28, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(31, 28, D1GfxPixel::colorPixel(42));
        }
        // 340[0], 373[0] - adjust after reuse
        if (i >= 17 && i < 19) {
            change |= frame->setPixel(31, 10, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(29, 10, D1GfxPixel::colorPixel(39));

            change |= frame->setPixel(30, 11, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(31, 12, D1GfxPixel::colorPixel(40));

            change |= frame->setPixel(31, 20, D1GfxPixel::colorPixel(39));
            change |= frame->setPixel(30, 21, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(31, 21, D1GfxPixel::colorPixel(38));

            change |= frame->setPixel(30, 28, D1GfxPixel::colorPixel(42));
            change |= frame->setPixel(30, 29, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(31, 29, D1GfxPixel::colorPixel(39));
            change |= frame->setPixel(30, 30, D1GfxPixel::colorPixel(39));
            change |= frame->setPixel(31, 30, D1GfxPixel::colorPixel(38));
        }

        // 188[5] - adjust after reuse/fix using 1[5]
        if (i == 20) {
            const CelMicro &microSrc = micros[19];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < 6 && y < x + 3) {
                        change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 1[5]
                    }
                }
            }
        }
        if (i == 21) { // 5[6] - adjust after reuse + sync
            change |= frame->setPixel(31, 30, D1GfxPixel::transparentPixel());

            // change |= frame->setPixel(17, 19, D1GfxPixel::colorPixel(102));
            // change |= frame->setPixel(16, 19, D1GfxPixel::colorPixel(98));
            change |= frame->setPixel(20, 20, D1GfxPixel::colorPixel(103));
            change |= frame->setPixel(15, 20, D1GfxPixel::colorPixel(96));
            change |= frame->setPixel(14, 21, D1GfxPixel::colorPixel(96));
            change |= frame->setPixel(13, 22, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(11, 25, D1GfxPixel::colorPixel(113));
            change |= frame->setPixel(10, 27, D1GfxPixel::colorPixel(115));

            change |= frame->setPixel(12, 23, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(10, 26, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(9, 29, D1GfxPixel::colorPixel(109));
            change |= frame->setPixel(9, 30, D1GfxPixel::colorPixel(110));
        }
        if (i == 22) { // 8[7] - adjust after reuse
            change |= frame->setPixel(0, 30, D1GfxPixel::transparentPixel());
        }
        if (i == 23) { // 15[6] - sync
            change |= frame->setPixel(31, 29, D1GfxPixel::transparentPixel());
            // change |= frame->setPixel(17, 19, D1GfxPixel::colorPixel(102));
            // change |= frame->setPixel(16, 19, D1GfxPixel::colorPixel(98));
            change |= frame->setPixel(12, 23, D1GfxPixel::colorPixel(112));
        }
        if (i == 24) { // 50[4] - sync
            change |= frame->setPixel(12, 3, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(13, 4, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(14, 6, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(15, 7, D1GfxPixel::colorPixel(111));
            change |= frame->setPixel(15, 8, D1GfxPixel::colorPixel(126));
        }
        if (i == 25) { // 50[7] - adjust after reuse
            change |= frame->setPixel(0, 31, D1GfxPixel::transparentPixel());
        }
        if (i == 26) { // 180[6] - sync
            change |= frame->setPixel(31, 30, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(28, 28, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(27, 27, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(26, 26, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(25, 25, D1GfxPixel::colorPixel(106));
            change |= frame->setPixel(24, 24, D1GfxPixel::colorPixel(106));
            change |= frame->setPixel(23, 23, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(22, 22, D1GfxPixel::colorPixel(104));
            change |= frame->setPixel(21, 21, D1GfxPixel::colorPixel(103));

            change |= frame->setPixel(17, 19, D1GfxPixel::transparentPixel()); // 118
            change |= frame->setPixel(16, 19, D1GfxPixel::transparentPixel()); // 114
            change |= frame->setPixel(20, 20, D1GfxPixel::colorPixel(103));
            change |= frame->setPixel(15, 20, D1GfxPixel::colorPixel(96));
            change |= frame->setPixel(16, 20, D1GfxPixel::colorPixel(82));
            change |= frame->setPixel(14, 21, D1GfxPixel::colorPixel(96));
            change |= frame->setPixel(15, 21, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(14, 22, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(13, 22, D1GfxPixel::colorPixel(97));
            change |= frame->setPixel(11, 25, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(11, 26, D1GfxPixel::colorPixel(113));
            change |= frame->setPixel(18, 24, D1GfxPixel::colorPixel(82));
            change |= frame->setPixel(16, 27, D1GfxPixel::colorPixel(83));
            change |= frame->setPixel(19, 20, D1GfxPixel::colorPixel(120));

            change |= frame->setPixel(12, 23, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(10, 26, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(9, 29, D1GfxPixel::colorPixel(124));
            change |= frame->setPixel(9, 30, D1GfxPixel::colorPixel(110));
        }
        if (i == 27) { // 212[6] - sync
            // change |= frame->setPixel(17, 19, D1GfxPixel::colorPixel(102));
            // change |= frame->setPixel(16, 19, D1GfxPixel::colorPixel(98));
            change |= frame->setPixel(20, 20, D1GfxPixel::colorPixel(103));
            change |= frame->setPixel(15, 20, D1GfxPixel::colorPixel(96));
            change |= frame->setPixel(14, 21, D1GfxPixel::colorPixel(96));
            change |= frame->setPixel(13, 22, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(11, 25, D1GfxPixel::colorPixel(113));

            change |= frame->setPixel(12, 23, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(10, 26, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(9, 29, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(9, 30, D1GfxPixel::colorPixel(110));

            change |= frame->setPixel(11, 27, D1GfxPixel::colorPixel(118));
            change |= frame->setPixel(10, 27, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(10, 28, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(13, 25, D1GfxPixel::colorPixel(116));
            change |= frame->setPixel(15, 22, D1GfxPixel::colorPixel(96));
        }
        if (i == 28) { // 251[6] - sync
            // change |= frame->setPixel(16, 19, D1GfxPixel::colorPixel(98));
            change |= frame->setPixel(15, 20, D1GfxPixel::colorPixel(96));
            change |= frame->setPixel(14, 21, D1GfxPixel::colorPixel(96));
            change |= frame->setPixel(13, 22, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(11, 25, D1GfxPixel::colorPixel(113));
            change |= frame->setPixel(10, 27, D1GfxPixel::colorPixel(117));

            change |= frame->setPixel(12, 23, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(10, 26, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(9, 29, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(9, 30, D1GfxPixel::colorPixel(110));

            change |= frame->setPixel(10, 28, D1GfxPixel::colorPixel(104));
        }
        // 1[7], 11[7], 196[7], 204[7], 208[7], 254[7] - sync
        if (i >= 29 && i < 35) {
            change |= frame->setPixel(14, 19, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(15, 19, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(11, 20, D1GfxPixel::colorPixel(108));
        }

        // fix shadow on 227[1] using 4[1]
        if (i == 7) {
            const CelMicro &microSrc = micros[35];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            if (frameSrc == nullptr) {
                return false;
            }
            for (int x = 7; x < 13; x++) {
                for (int y = 22; y < 28; y++) {
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 4[1]
                }
            }
        }
        // fix shadow on 152[1] using 155[1]
        if (i == 37) {
            const CelMicro &microSrc = micros[36];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            if (frameSrc == nullptr) {
                return false;
            }
            for (int x = 7; x < 13; x++) {
                for (int y = 22; y < 28; y++) {
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 155[1]
                }
            }
        }
        // fix shadow on 238[0] to make it more usable
        if (i == 38) {
            for (int x = 26; x < MICRO_WIDTH; x++) {
                for (int y = 26; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && color != 0 && (color % 16) < 12) {
                        change |= frame->setPixel(x, y, D1GfxPixel::colorPixel((color & ~15) | (12 + (color % 16) / 4)));
                    }
                }
            }
        }
        // fix shadow on 238[1] to make it more usable
        if (i == 39) {
            for (int x = 0; x < 8; x++) {
                for (int y = 21; y < 29; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    quint8 color = pixel.getPaletteIndex();
                    if (!pixel.isTransparent() && color != 0 && (color % 16) < 12) {
                        change |= frame->setPixel(x, y, D1GfxPixel::colorPixel((color & ~15) | (12 + (color % 16) / 4)));
                    }
                }
            }
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

bool D1Tileset::patchHellStairs(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{ 139 - 1, 1, D1CEL_FRAME_TYPE::Empty },              // merge subtiles (used to block subsequent calls)
/*  1 */{ 138 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/*  2 */{ 137 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/*  3 */{ 137 - 1, 0, D1CEL_FRAME_TYPE::Square },
/*  4 */{ 140 - 1, 0, D1CEL_FRAME_TYPE::LeftTrapezoid },
/*  5 */{ 137 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*  6 */{ 140 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },

/*  7 */{ 136 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*  8 */{ 126 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },

/*  9 */{  98 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 10 */{  86 - 1, 4, D1CEL_FRAME_TYPE::Square },
/* 11 */{  86 - 1, 2, D1CEL_FRAME_TYPE::Square },
/* 12 */{  95 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid },

/* 13 */{  91 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // eliminate pointless pixels
/* 14 */{  91 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 15 */{ 110 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 16 */{ 110 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 17 */{ 112 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 18 */{ 112 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 19 */{ 113 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 20 */{ 113 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 21 */{ 130 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 22 */{ 132 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 23 */{ 133 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },

/* 24 */{ 100 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 25 */{ 135 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 26 */{ 127 - 1, 9, D1CEL_FRAME_TYPE::TransparentSquare },
/* 27 */{ 134 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 28 */{ 134 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 29 */{ 134 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
    };
    constexpr unsigned blockSize = BLOCK_SIZE_L4;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;

        // mask 138[0]
        if (i == 1) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (pixel.isTransparent()) {
                        continue;
                    }
                    if (x >= 15 || pixel.getPaletteIndex() < 80) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // move pixels to 137[0] from 139[1]
        if (i == 3) {
            const CelMicro &microSrc = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 139[1]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 140[0] from 139[1]
        if (i == 4) {
            const CelMicro &microSrc = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y + MICRO_HEIGHT / 2); // 139[1]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        // mask 137[1]
        if (i == 5) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (pixel.isTransparent()) {
                        continue;
                    }
                    if (x >= 15 || pixel.getPaletteIndex() < 80) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // move pixels to 137[1] from 138[0]
        if (i == 5) {
            const CelMicro &microSrc = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 138[0]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 140[1] from 138[0]
        if (i == 6) {
            const CelMicro &microSrc = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            if (frameSrc == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y + MICRO_HEIGHT / 2); // 138[0]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 126[3] from 136[1]
        if (i == 8) {
            const CelMicro &microSrc = micros[7];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            if (frameSrc == nullptr) {
                return false;
            }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 136[1]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 86[4] from 98[0]
        if (i == 10) {
            const CelMicro &microSrc = micros[9];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 98[0]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 86[2] from 91[1]
        {
            if (i == 11) { // 86[2]
                change |= frame->setPixel(0, 31, D1GfxPixel::colorPixel(125));
            }
            if (i == 14) { // 91[1]
                change |= frame->setPixel(0, 15, D1GfxPixel::transparentPixel());
            }

        }
        // move pixels to 95[1] from 113[0]
        {
            if (i == 12) { // 95[1]
                change |= frame->setPixel(30, 0, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(31, 0, D1GfxPixel::colorPixel(126));
            }
            if (i == 19) { // 113[0]
                change |= frame->setPixel(30, 16, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(31, 16, D1GfxPixel::transparentPixel());
            }
        }

        // eliminate pointless pixels of 91[0], 91[1], 110[0], 110[1], 112[0], 112[1], 113[0], 113[1], 130[0], 132[1], 133[0]
        if (i >= 13 && i < 24) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (pixel.getPaletteIndex() == 80) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // eliminate pointless pixels of 100[2]
        if (i == 24) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (y >= 16 - x / 2 && (pixel.getPaletteIndex() % 16) > 12) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // move pixels to 127[9] from 135[5]
        {
            if (i == 26) { // 127[9]
                change |= frame->setPixel(31, 26, D1GfxPixel::colorPixel(115));
                change |= frame->setPixel(27, 27, D1GfxPixel::colorPixel(106));
                change |= frame->setPixel(28, 27, D1GfxPixel::colorPixel(104));
                change |= frame->setPixel(29, 27, D1GfxPixel::colorPixel(115));
                change |= frame->setPixel(30, 27, D1GfxPixel::colorPixel(113));
                change |= frame->setPixel(31, 27, D1GfxPixel::colorPixel(113));
            }
            if (i == 25) { // 135[5]
                change |= frame->setPixel(31, 26, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(27, 27, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(28, 27, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(29, 27, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(30, 27, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(31, 27, D1GfxPixel::transparentPixel());
            }
        }

        // fix bad artifacts
        if (i == 5) { // 137[1] (140[3])
            change |= frame->setPixel(7, 7, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(12, 29, D1GfxPixel::colorPixel(122));
            change |= frame->setPixel(13, 22, D1GfxPixel::transparentPixel());
        }
        if (i == 6) { // 140[1]
            change |= frame->setPixel(14, 0, D1GfxPixel::transparentPixel());
        }
        if (i == 23) { // 133[0] - restore eliminated pixel
            change |= frame->setPixel(31, 3, D1GfxPixel::colorPixel(80));
        }
        // move pixels to 100[2] from 110[1]
        {
            if (i == 24) { // 100[2]
                change |= frame->setPixel( 2, 5, D1GfxPixel::colorPixel(106));
                change |= frame->setPixel( 6, 7, D1GfxPixel::colorPixel(120));
            }
            if (i == 16) { // 110[1]
                change |= frame->setPixel( 2, 21, D1GfxPixel::transparentPixel());
                change |= frame->setPixel( 6, 23, D1GfxPixel::transparentPixel());
            }
        }
        if (i == 27) { // 134[0]
            change |= frame->setPixel(29, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(30, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 0, D1GfxPixel::transparentPixel());
        }
        if (i == 28) { // 134[1]
            change |= frame->setPixel(0, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(1, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(2, 0, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(3, 0, D1GfxPixel::transparentPixel());
        }
        if (i == 29) { // 134[2]
            change |= frame->setPixel(0, 31, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(1, 31, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(2, 31, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(3, 31, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(4, 31, D1GfxPixel::transparentPixel());
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

bool D1Tileset::patchHellWall1(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{   8 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // mask walls leading to north east
/*  1 */{   8 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*  2 */{   8 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/*  3 */{  21 - 1, 3, D1CEL_FRAME_TYPE::Empty },
/*  4 */{   8 - 1, 4, D1CEL_FRAME_TYPE::Empty },
/*  5 */{   8 - 1, 5, D1CEL_FRAME_TYPE::Empty },

/*  6 */{  21 - 1, 0, D1CEL_FRAME_TYPE::LeftTrapezoid },
/*  7 */{  21 - 1, 2, D1CEL_FRAME_TYPE::Square },
/*  8 */{  21 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },

/*  9 */{   1 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 10 */{   1 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 11 */{   1 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 12 */{   1 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 13 */{  11 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 14 */{  11 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 15 */{  11 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 16 */{  11 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 17 */{ 167 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 18 */{ 167 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 19 */{ 167 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 20 */{ /*167*/ - 1, 7, D1CEL_FRAME_TYPE::Empty },
/* 21 */{ /*188*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 22 */{ /*188*/ - 1, 3, D1CEL_FRAME_TYPE::Empty },
/* 23 */{ 188 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 24 */{ /*188*/ - 1, 7, D1CEL_FRAME_TYPE::Empty },
/* 25 */{ /*196*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 26 */{ /*196*/ - 1, 3, D1CEL_FRAME_TYPE::Empty },
/* 27 */{ 196 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 28 */{ 196 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 29 */{ 204 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 30 */{ /*204*/ - 1, 3, D1CEL_FRAME_TYPE::Empty },
/* 31 */{ 204 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 32 */{ 204 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 33 */{ 208 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 34 */{ 208 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 35 */{ 208 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 36 */{ 208 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 37 */{ /*251*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 38 */{ 251 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 39 */{ 251 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 40 */{ 251 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 41 */{ /*254*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 42 */{ /*254*/ - 1, 3, D1CEL_FRAME_TYPE::Empty },
/* 43 */{ 254 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 44 */{ 254 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },

/* 45 */{   3 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 46 */{   3 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 47 */{ /*190*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 48 */{ 190 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 49 */{ 198 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 50 */{ /*198*/ - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 51 */{ 206 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 52 */{ 206 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 53 */{ 210 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 54 */{ /*210*/ - 1, 2, D1CEL_FRAME_TYPE::Empty },

/* 55 */{  23 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 56 */{ 190 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 57 */{ 198 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 58 */{ 206 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 59 */{ 210 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 60 */{  48 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 61 */{  58 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },

/* 62 */{   6 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 63 */{  51 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 64 */{ 193 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 65 */{ 201 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 66 */{ 217 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/* 67 */{ /*244*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 68 */{ 244 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 69 */{ 244 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },

/* 70 */{ /*246*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 71 */{ 246 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 72 */{ 246 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/* 73 */{ 257 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
    };
    constexpr unsigned blockSize = BLOCK_SIZE_L4;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // mask 11[1], 167[1] using 1[1]
        if (i >= 13 && i < 21 && (i % 4) == (13 % 4)) {
            const CelMicro &microSrc = micros[9];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 1[1]
                    if (pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        if (i == 10) { // mask 1[3]
            // change |= frame->setPixel(17, 20, D1GfxPixel::transparentPixel());
            // change |= frame->setPixel(17, 21, D1GfxPixel::transparentPixel());
            // change |= frame->setPixel(17, 22, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(17, 31, D1GfxPixel::transparentPixel());
        }
        if (i == 34) { // mask 208[3]
            change |= frame->setPixel(17,  8, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(17,  9, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(17, 10, D1GfxPixel::transparentPixel());

            // change |= frame->setPixel(17, 20, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(17, 21, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(17, 22, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(17, 31, D1GfxPixel::transparentPixel());
        }
        // mask 167[3]
        if (i == 18) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x < 17) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask 11[3] using 21[3]
        if (i == 14) {
            const CelMicro &microSrc = micros[3];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x < 17) {
                        continue;
                    }
                    D1GfxPixel pixelSrc = frameSrc->getPixel(x, y); // 21[3]
                    if (!pixelSrc.isTransparent()) {
                        continue;
                    }
                    quint8 color = frame->getPixel(x, y).getPaletteIndex();
                    if (color != 63 && color != 78 && color != 79 && color != 94 && color != 95 && (color < 107 || color > 111) && color != 126 && color != 127) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 251[3]
        if (i == 38) {
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x < 17) {
                        continue;
                    }
                    if (y < x - 18) {
                        continue;
                    }
                    if (y == 14 || y == 15) {
                        continue; // keep pixels continuous
                    }
                    quint8 color = frame->getPixel(x, y).getPaletteIndex();
                    if (color != 63 && color != 78 && color != 79 && color != 94 && color != 95 && (color < 107 || color > 111) && color != 126 && color != 127) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 1[5], 167[5], 188[5], 196[5], 204[5], 208[5]
        if (i >= 11 && i < 37 && (i % 4) == (11 % 4)) {
            if (i == 19) { // 167[5]
                for (int x = 17; x < MICRO_WIDTH; x++) {
                    for (int y = 0; y < MICRO_HEIGHT; y++) {
                        if (y > 21 - x) {
                            change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }
                }
            } else if (i != 15) { // 1[5], 188[5], 196[5], 204[5], 208[5]
                change |= frame->setPixel(20, 2, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(19, 3, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(18, 4, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(17, 5, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(17, 6, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(17, 7, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(17, 8, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(17, 9, D1GfxPixel::transparentPixel());
                if (i != 35) { // 208[5]
                    change |= frame->setPixel(17, 10, D1GfxPixel::transparentPixel());
                    change |= frame->setPixel(17, 11, D1GfxPixel::transparentPixel());
                    change |= frame->setPixel(17, 12, D1GfxPixel::transparentPixel());
                }
                if (i == 31) { // 204[5]
                    change |= frame->setPixel(17, 13, D1GfxPixel::transparentPixel());
                    change |= frame->setPixel(17, 14, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask 11[7] using 1[7]
        if (i == 16) {
            const CelMicro &microSrc = micros[12];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (x >= 20 || y < 14) {
                        continue; // preserve the tusk
                    }
                    if (x >= 9 && y >= 26) {
                        continue; // preserve the shadow
                    }
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 1[7]
                }
            }
        }
        // mask 3[0], 198[0], 206[0], 210[0] using 1[1] and 167[3]
        if (i >= 45 && i < 55 && (i % 2) == (45 % 2)) {
            const CelMicro &microSrc1 = micros[18];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[9];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {

                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 167[3]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 1[1]
                    }

                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            { // remove shadow using '167[3]'
                change |= frame->setPixel(17, 0, D1GfxPixel::colorPixel(72));
                change |= frame->setPixel(17, 1, D1GfxPixel::colorPixel(72));
                change |= frame->setPixel(17, 2, D1GfxPixel::colorPixel(71));
                change |= frame->setPixel(17, 3, D1GfxPixel::colorPixel(70));
                change |= frame->setPixel(17, 4, D1GfxPixel::colorPixel(121));
                change |= frame->setPixel(17, 5, D1GfxPixel::colorPixel(85));
                change |= frame->setPixel(17, 6, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(18, 0, D1GfxPixel::colorPixel(72));
                change |= frame->setPixel(18, 1, D1GfxPixel::colorPixel(72));
                change |= frame->setPixel(18, 2, D1GfxPixel::colorPixel(70));
                change |= frame->setPixel(18, 3, D1GfxPixel::colorPixel(71));
                change |= frame->setPixel(18, 4, D1GfxPixel::colorPixel(120));
                change |= frame->setPixel(18, 5, D1GfxPixel::colorPixel(86));
                change |= frame->setPixel(18, 6, D1GfxPixel::colorPixel(119));
                change |= frame->setPixel(19, 0, D1GfxPixel::colorPixel(72));
                change |= frame->setPixel(19, 1, D1GfxPixel::colorPixel(69));
                change |= frame->setPixel(19, 2, D1GfxPixel::colorPixel(71));
                change |= frame->setPixel(19, 3, D1GfxPixel::colorPixel(72));
                change |= frame->setPixel(19, 4, D1GfxPixel::colorPixel(70));
                change |= frame->setPixel(19, 5, D1GfxPixel::colorPixel(104));
                change |= frame->setPixel(19, 6, D1GfxPixel::colorPixel(104));
            }
        }
        // mask 3[2], 190[2], 206[2]
        if (i >= 46 && i < 55 && (i % 2) == (46 % 2)) {
            for (int x = 0; x < 17; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
            { // remove shadow using '167[3]'
                change |= frame->setPixel(17, 21, D1GfxPixel::colorPixel(76));
                change |= frame->setPixel(17, 22, D1GfxPixel::colorPixel(74));
                change |= frame->setPixel(17, 23, D1GfxPixel::colorPixel(117));
                change |= frame->setPixel(17, 24, D1GfxPixel::colorPixel(105));
                change |= frame->setPixel(17, 25, D1GfxPixel::colorPixel(104));
                change |= frame->setPixel(17, 26, D1GfxPixel::colorPixel(105));
                change |= frame->setPixel(17, 27, D1GfxPixel::colorPixel(63));
                change |= frame->setPixel(17, 28, D1GfxPixel::colorPixel(71));
                change |= frame->setPixel(17, 29, D1GfxPixel::colorPixel(121));
                change |= frame->setPixel(17, 30, D1GfxPixel::colorPixel(90));
                change |= frame->setPixel(17, 31, D1GfxPixel::colorPixel(60));
                change |= frame->setPixel(18, 21, D1GfxPixel::colorPixel(77));
                change |= frame->setPixel(18, 22, D1GfxPixel::colorPixel(120));
                change |= frame->setPixel(18, 23, D1GfxPixel::colorPixel(114));
                change |= frame->setPixel(18, 24, D1GfxPixel::colorPixel(120));
                change |= frame->setPixel(18, 25, D1GfxPixel::colorPixel(103));
                change |= frame->setPixel(18, 26, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(18, 27, D1GfxPixel::colorPixel(74));
                change |= frame->setPixel(18, 28, D1GfxPixel::colorPixel(68));
                change |= frame->setPixel(18, 29, D1GfxPixel::colorPixel(76));
                change |= frame->setPixel(18, 30, D1GfxPixel::colorPixel(73));
                change |= frame->setPixel(18, 31, D1GfxPixel::colorPixel(72));
                change |= frame->setPixel(19, 21, D1GfxPixel::colorPixel(73));
                change |= frame->setPixel(19, 22, D1GfxPixel::colorPixel(98));
                change |= frame->setPixel(19, 23, D1GfxPixel::colorPixel(102));
                change |= frame->setPixel(19, 24, D1GfxPixel::colorPixel(104));
                change |= frame->setPixel(19, 25, D1GfxPixel::colorPixel(108));
                change |= frame->setPixel(19, 26, D1GfxPixel::colorPixel(78));
                change |= frame->setPixel(19, 27, D1GfxPixel::colorPixel(73));
                change |= frame->setPixel(19, 28, D1GfxPixel::colorPixel(91));
                change |= frame->setPixel(19, 29, D1GfxPixel::colorPixel(71));
                change |= frame->setPixel(19, 30, D1GfxPixel::colorPixel(71));
                change |= frame->setPixel(19, 31, D1GfxPixel::colorPixel(71));
            }
        }
        // mask 23[4], 190[4], 198[4], 206[4] + 48[4], 58[4] using 1[7]
        if (i >= 55 && i < 62) {
            const CelMicro &microSrc = micros[12];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y < MICRO_HEIGHT / 2) {
                        D1GfxPixel pixel = frameSrc->getPixel(x, y + MICRO_HEIGHT / 2); // 1[7]
                        if (!pixel.isTransparent()) {
                            change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    } else {
                        if (x < 17 || (y < 38 - x)) {
                            change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }
                }
            }
        }
        // fix 11[1], 246[1] using 8[1]
        if (i == 13 || i == 70) {
            const CelMicro &microSrc = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < 3; x++) {
                for (int y = 0; y < 3; y++) {
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 8[1]
                }
            }
        }
        // fix 11[3], 246[3] using 21[3]
        if (i == 14 || i == 71) {
            const CelMicro &microSrc = micros[3];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < 5; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 21[3]
                }
            }
        }
        // fix 11[5], 246[5] using 8[5]
        if (i == 15 || i == 72) {
            const CelMicro &microSrc = micros[5];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < 5; x++) {
                for (int y = 24; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 8[5]
                }
            }
        }
        // remove shadow 254[5] using 8[5]
        if (i == 43) {
            const CelMicro &microSrc = micros[5];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < 5; x++) {
                for (int y = 24; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 8[5]
                }
            }
        }
        // fix 21[0], 244[0] using 8[0]
        if (i == 6 || i == 67) {
            const CelMicro &microSrc = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 28; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < 3; y++) {
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 8[0]
                }
            }
        }
        // fix 21[2], 244[2] using 8[2]
        if (i == 7 || i == 68) {
            const CelMicro &microSrc = micros[2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 27; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 8[2]
                }
            }
        }
        // fix 21[4], 244[4] using 8[4]
        if (i == 8 || i == 69) {
            const CelMicro &microSrc = micros[4];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 27; x < MICRO_WIDTH; x++) {
                for (int y = 24; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x, y)); // 8[4]
                }
            }
        }

        // fix 11[3] - erase inconsistent shadow
        if (i == 14) {
            for (int x = 18; x < 25; x++) {
                for (int y = 0; y < 10; y++) {
                    if (y < 2 * x - 31) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            change |= frame->setPixel(30, 20, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 19, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(17, 23, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(18, 23, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(17, 24, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(18, 24, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(19, 24, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(20, 24, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(20, 25, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(21, 25, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(14, 12, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(16, 11, D1GfxPixel::colorPixel(63));
        }
        // fix 11[5], 254[5] - erase inconsistent shadow
        if (i == 15 || i == 43) {
            for (int x = 18; x < 24; x++) {
                for (int y = 26; y < MICRO_HEIGHT; y++) {
                    if (y < 28) {
                        continue; // keep pixels continuous
                    }
                    if (y > 47 - x && y > x + 5) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            // change |= frame->setPixel(20, 26, D1GfxPixel::transparentPixel()); - keep pixels continuous
            // change |= frame->setPixel(20, 27, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(24, 30, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(24, 31, D1GfxPixel::transparentPixel());
        }
        // fix 251[3]
        if (i == 38) {
            for (int x = 18; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < 10; y++) {
                    // erase non-symmetric tusk
                    if (y < 3 * x / 2 - 39) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                    // erase inconsistent shadow
                    if (y == 0) {
                        continue; // keep pixels continuous
                    }
                    if (y < 2 * x - 31 && y > x - 19) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            change |= frame->setPixel(17, 1, D1GfxPixel::transparentPixel());
            // change |= frame->setPixel(19,  0, D1GfxPixel::transparentPixel()); - keep pixels continuous

            change |= frame->setPixel(31, 3, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 4, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 5, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 6, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 7, D1GfxPixel::colorPixel(116));
            change |= frame->setPixel(31, 12, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(30, 20, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(31, 19, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(17, 23, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(18, 23, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(17, 24, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(18, 24, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(19, 24, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(20, 24, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(20, 25, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(21, 25, D1GfxPixel::transparentPixel());
        }
        // fix 251[5] - erase non-symmetric tusk
        if (i == 39) {
            for (int x = 25; x < MICRO_WIDTH; x++) {
                for (int y = 22; y < MICRO_HEIGHT; y++) {
                    if (y < 2 * x - 21) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        if (i == 10) { // 1[3] - add shadow
            change |= frame->setPixel(19, 7, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(18, 8, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(19, 8, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(18, 9, D1GfxPixel::colorPixel(108));
            change |= frame->setPixel(19, 9, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(18, 10, D1GfxPixel::colorPixel(107));
            change |= frame->setPixel(19, 10, D1GfxPixel::colorPixel(111));
            change |= frame->setPixel(18, 11, D1GfxPixel::colorPixel(111));
            change |= frame->setPixel(19, 11, D1GfxPixel::colorPixel(95));
            change |= frame->setPixel(18, 12, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(19, 12, D1GfxPixel::colorPixel(95));
            change |= frame->setPixel(18, 13, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(19, 13, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(18, 14, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(19, 14, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(18, 15, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(19, 15, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(18, 16, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(19, 16, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(17, 17, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(18, 17, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(19, 17, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(17, 18, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(18, 18, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(19, 18, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(17, 19, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(18, 19, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(19, 19, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(17, 20, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(18, 20, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(19, 20, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(17, 21, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(18, 21, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(19, 21, D1GfxPixel::colorPixel(74));
            change |= frame->setPixel(17, 22, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(18, 22, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(19, 22, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(17, 23, D1GfxPixel::colorPixel(109));
            change |= frame->setPixel(18, 23, D1GfxPixel::colorPixel(105));
            // keep pixels continuous
            change |= frame->setPixel(17, 7, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(18, 7, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(17, 8, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(17, 9, D1GfxPixel::colorPixel(104));
            change |= frame->setPixel(17, 10, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(17, 11, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(17, 12, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(17, 13, D1GfxPixel::colorPixel(121));
            change |= frame->setPixel(17, 14, D1GfxPixel::colorPixel(90));
            change |= frame->setPixel(17, 15, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(17, 16, D1GfxPixel::colorPixel(72));
        }
        if (i == 34) { // 208[3] - add shadow
            change |= frame->setPixel(17, 12, D1GfxPixel::colorPixel(77));
            change |= frame->setPixel(17, 13, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(17, 14, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(17, 15, D1GfxPixel::colorPixel(76));
            change |= frame->setPixel(17, 16, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(17, 17, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(17, 18, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(17, 19, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(17, 20, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(18, 16, D1GfxPixel::colorPixel(78));
            change |= frame->setPixel(18, 17, D1GfxPixel::colorPixel(76));
            change |= frame->setPixel(18, 18, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(18, 19, D1GfxPixel::colorPixel(109));
        }
        if (i == 56) { // 190[4] - fix connection
            change |= frame->setPixel(18, 5, D1GfxPixel::colorPixel(96));
            change |= frame->setPixel(17, 4, D1GfxPixel::colorPixel(97));
            change |= frame->setPixel(18, 4, D1GfxPixel::colorPixel(113));
            change |= frame->setPixel(19, 4, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(19, 5, D1GfxPixel::colorPixel(96));
            change |= frame->setPixel(20, 5, D1GfxPixel::colorPixel(81));
            change |= frame->setPixel(19, 6, D1GfxPixel::colorPixel(81));
            change |= frame->setPixel(20, 6, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(20, 7, D1GfxPixel::colorPixel(113));
            change |= frame->setPixel(20, 8, D1GfxPixel::colorPixel(113));
            change |= frame->setPixel(21, 9, D1GfxPixel::colorPixel(113));
            change |= frame->setPixel(22, 11, D1GfxPixel::colorPixel(99));
            change |= frame->setPixel(22, 12, D1GfxPixel::colorPixel(101));

            change |= frame->setPixel(13, 3, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(14, 3, D1GfxPixel::colorPixel(103));
        }
        if (i == 57) { // 198[4] - fix connection
            change |= frame->setPixel(20, 8, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(22, 10, D1GfxPixel::colorPixel(117));
            change |= frame->setPixel(22, 11, D1GfxPixel::colorPixel(83));
            change |= frame->setPixel(22, 12, D1GfxPixel::colorPixel(101));
            change |= frame->setPixel(23, 11, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(23, 12, D1GfxPixel::colorPixel(122));

            change |= frame->setPixel(13, 3, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(14, 3, D1GfxPixel::colorPixel(103));
            change |= frame->setPixel(15, 3, D1GfxPixel::colorPixel(100));
            change |= frame->setPixel(16, 3, D1GfxPixel::colorPixel(98));
            change |= frame->setPixel(14, 2, D1GfxPixel::colorPixel(104));
        }
        if (i == 58) { // 206[4] - fix connection
            change |= frame->setPixel(17, 1, D1GfxPixel::colorPixel(118));
            change |= frame->setPixel(17, 2, D1GfxPixel::colorPixel(85));
            change |= frame->setPixel(16, 2, D1GfxPixel::colorPixel(100));
            change |= frame->setPixel(17, 3, D1GfxPixel::colorPixel(84));
            change |= frame->setPixel(17, 4, D1GfxPixel::colorPixel(81));
            change |= frame->setPixel(18, 4, D1GfxPixel::colorPixel(82));
            change |= frame->setPixel(19, 5, D1GfxPixel::colorPixel(81));
            change |= frame->setPixel(20, 6, D1GfxPixel::colorPixel(113));
            change |= frame->setPixel(21, 8, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(23, 8, D1GfxPixel::colorPixel(113));
            change |= frame->setPixel(21, 9, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(22, 9, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(23, 9, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(24, 9, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(23, 10, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(22, 10, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(22, 11, D1GfxPixel::colorPixel(98));
            change |= frame->setPixel(22, 12, D1GfxPixel::colorPixel(101));

            change |= frame->setPixel(13, 3, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(14, 3, D1GfxPixel::colorPixel(101));
            change |= frame->setPixel(15, 3, D1GfxPixel::colorPixel(100));
            change |= frame->setPixel(16, 3, D1GfxPixel::colorPixel(100));
        }
        if (i == 59) { // 210[4] - fix connection
            change |= frame->setPixel(16, 2, D1GfxPixel::colorPixel(85));
            change |= frame->setPixel(17, 3, D1GfxPixel::colorPixel(85));
            change |= frame->setPixel(17, 4, D1GfxPixel::colorPixel(82));
            change |= frame->setPixel(18, 2, D1GfxPixel::colorPixel(86));
            change |= frame->setPixel(18, 3, D1GfxPixel::colorPixel(84));
            change |= frame->setPixel(18, 4, D1GfxPixel::colorPixel(83));
            change |= frame->setPixel(18, 5, D1GfxPixel::colorPixel(82));
            change |= frame->setPixel(19, 2, D1GfxPixel::colorPixel(85));
            change |= frame->setPixel(19, 3, D1GfxPixel::colorPixel(83));
            change |= frame->setPixel(19, 6, D1GfxPixel::colorPixel(113));
            change |= frame->setPixel(20, 7, D1GfxPixel::colorPixel(81));
            change |= frame->setPixel(22, 10, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(22, 11, D1GfxPixel::colorPixel(99));
            change |= frame->setPixel(22, 12, D1GfxPixel::colorPixel(101));

            change |= frame->setPixel(13, 3, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(14, 3, D1GfxPixel::colorPixel(101));
            change |= frame->setPixel(15, 3, D1GfxPixel::colorPixel(100));
            change |= frame->setPixel(16, 3, D1GfxPixel::colorPixel(100));
        }
        // 48[4], 58[4] - fix connection
        if (i >= 60 && i < 62) {
            change |= frame->setPixel(13, 3, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(14, 3, D1GfxPixel::colorPixel(103));
            change |= frame->setPixel(15, 3, D1GfxPixel::colorPixel(99));

            change |= frame->setPixel(14, 2, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(15, 2, D1GfxPixel::colorPixel(101));
            change |= frame->setPixel(16, 2, D1GfxPixel::colorPixel(98));
        }
        if (i == 55) { // 23[4] - fix connection
            change |= frame->setPixel(13, 3, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(14, 3, D1GfxPixel::colorPixel(103));
            change |= frame->setPixel(15, 3, D1GfxPixel::colorPixel(99));
            change |= frame->setPixel(15, 2, D1GfxPixel::colorPixel(101));
            change |= frame->setPixel(16, 2, D1GfxPixel::colorPixel(98));
        }
        if (i == 12) { // 1[7] - fix connection
            change |= frame->setPixel(21, 28, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(21, 29, D1GfxPixel::colorPixel(101));
            change |= frame->setPixel(22, 29, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(21, 30, D1GfxPixel::colorPixel(102));
            change |= frame->setPixel(22, 30, D1GfxPixel::colorPixel(108));
            change |= frame->setPixel(21, 31, D1GfxPixel::colorPixel(106));
        }
        if (i == 28) { // 196[7] - fix connection
            change |= frame->setPixel(21, 28, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(21, 29, D1GfxPixel::colorPixel(101));
            change |= frame->setPixel(22, 29, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(21, 30, D1GfxPixel::colorPixel(102));
            change |= frame->setPixel(22, 30, D1GfxPixel::colorPixel(108));
            change |= frame->setPixel(21, 31, D1GfxPixel::colorPixel(106));
        }
        if (i == 32) { // 204[7] - fix connection
            change |= frame->setPixel(21, 28, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(21, 29, D1GfxPixel::colorPixel(101));
            change |= frame->setPixel(22, 29, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(21, 30, D1GfxPixel::colorPixel(102));
            change |= frame->setPixel(22, 30, D1GfxPixel::colorPixel(108));
            change |= frame->setPixel(21, 31, D1GfxPixel::colorPixel(106));
        }
        if (i == 36) { // 208[7] - fix connection
            change |= frame->setPixel(21, 28, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(21, 29, D1GfxPixel::colorPixel(101));
            change |= frame->setPixel(22, 29, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(21, 30, D1GfxPixel::colorPixel(102));
            change |= frame->setPixel(22, 30, D1GfxPixel::colorPixel(108));
            change |= frame->setPixel(21, 31, D1GfxPixel::colorPixel(106));
        }
        if (i == 44) { // 251[7] - fix connection
            change |= frame->setPixel(21, 28, D1GfxPixel::colorPixel(115));
            change |= frame->setPixel(21, 29, D1GfxPixel::colorPixel(101));
            change |= frame->setPixel(22, 29, D1GfxPixel::colorPixel(105));
            change |= frame->setPixel(21, 30, D1GfxPixel::colorPixel(102));
            change |= frame->setPixel(22, 30, D1GfxPixel::colorPixel(108));
            change |= frame->setPixel(21, 31, D1GfxPixel::colorPixel(106));
        }
        // 6[5], 51[5], 193[5], 201[5], 217[5] - fix connection
        if (i >= 62 && i < 67) {
            change |= frame->setPixel(9, 12, D1GfxPixel::colorPixel(106));
        }
        if (i == 64) { // 193[5] - fix connection+
            change |= frame->setPixel(13, 5, D1GfxPixel::colorPixel(96));
            change |= frame->setPixel(14, 4, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(16, 3, D1GfxPixel::colorPixel(98));
            change |= frame->setPixel(17, 3, D1GfxPixel::colorPixel(100));
            change |= frame->setPixel(18, 3, D1GfxPixel::colorPixel(104));
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

bool D1Tileset::patchHellWall2(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{   8 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // mask walls leading to north west
/*  1 */{   8 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/*  2 */{   8 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/*  3 */{  21 - 1, 3, D1CEL_FRAME_TYPE::Empty },
/*  4 */{   8 - 1, 4, D1CEL_FRAME_TYPE::Empty },
/*  5 */{   5 - 1, 6, D1CEL_FRAME_TYPE::Empty },

/*  6 */{   5 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  7 */{   5 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/*  8 */{   5 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/*  9 */{  15 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 10 */{  15 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 11 */{  15 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 12 */{  38 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 13 */{ /*38*/ - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 14 */{ /*38*/ - 1, 4, D1CEL_FRAME_TYPE::Empty },
/* 15 */{ 180 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 16 */{ 180 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 17 */{ 180 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 18 */{ 192 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 19 */{ /*192*/ - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 20 */{ 192 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 21 */{ 200 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 22 */{ 200 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 23 */{ 200 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 24 */{ 212 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 25 */{ /*212*/ - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 26 */{ 212 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 27 */{ 216 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 28 */{ /*216*/ - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 29 */{ 216 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 30 */{ /*251*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 31 */{ 251 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 32 */{ 251 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },

/* 33 */{   6 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 34 */{   6 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 35 */{ /*193*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 36 */{ /*193*/ - 1, 3, D1CEL_FRAME_TYPE::Empty },
/* 37 */{ /*201*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 38 */{ /*201*/ - 1, 3, D1CEL_FRAME_TYPE::Empty },
/* 39 */{ 213 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 40 */{ /*213*/ - 1, 3, D1CEL_FRAME_TYPE::Empty },
/* 41 */{ /*217*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 42 */{ 217 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },

/* 43 */{   6 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 44 */{ 193 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 45 */{ 201 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 46 */{ 213 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 47 */{ 217 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 48 */{  51 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
    };
    constexpr unsigned blockSize = BLOCK_SIZE_L4;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;
        // mask 15[0], 38[0], 180[0] using 5[0]
        if (i >= 9 && i < 18 && (i % 3) == (9 % 3)) {
            const CelMicro &microSrc = micros[6];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixelSrc = frameSrc->getPixel(x, y); // 5[0]
                    if (pixelSrc.isTransparent()) {
                        quint8 color = frame->getPixel(x, y).getPaletteIndex();
                        if (x < 11 && y < 9 && color > 100 && (color < 112 || color == 126)) {
                            continue; // preserve the shadow
                        }
                        if ((i == 9 || i == 12) && x == 10 && y == 7) {
                            continue; // keep pixels continuous
                        }
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // mask 5[2], 180[2], 200[2]
        if (i >= 7 && i < 24 && (i % 3) == (7 % 3)) {
            if (i == 7 || i == 16 || i == 22) {
                for (int x = 14; x < 16; x++) {
                    for (int y = 0; y < 23; y++) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        if (i == 22) { // mask++ 180[2]
            change |= frame->setPixel(15, 23, D1GfxPixel::transparentPixel());
        }

        // mask 15[2] using 8[2]
        if (i == 10) {
            const CelMicro &microSrc = micros[2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return false;
            // }
            for (int x = 0; x < 16; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y > 29 && (x < 3 || x == 15)) {
                        continue;
                    }
                    if (y == 29 && (x == 2 || x == 3)) {
                        continue;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 8[2]
                    if (pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // mask 251[2]
        if (i == 31) {
            for (int x = 0; x < 16; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y > 29 && (x < 3 || x == 15)) {
                        continue; // preserve 'extra' shadow
                    }
                    if (y == 29 && (x == 2 || x == 3)) {
                        continue; // preserve 'extra' shadow
                    }
                    if (y < 2) {
                        continue; // keep pixels continuous
                    }
                    if (y > 12 || y > 14 - x) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            change |= frame->setPixel(4, 10, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(3, 11, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(2, 12, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(0, 12, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(1, 12, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(12, 2, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(13, 0, D1GfxPixel::colorPixel(126));
        }

        // mask 5[4], 15[4], 192[4], 200[4], 212[4], 216[4] + 180[4]
        if (i >= 8 && i < 30 && (i % 3) == (8 % 3)) {
            if (i == 11) { // 15[4] using 8[4]
                const CelMicro &microSrc = micros[4];
                std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
                D1GfxFrame *frameSrc = mf.second;
                // if (frameSrc == nullptr) {
                //    return false;
                // }
                for (int x = 0; x < 16; x++) {
                    for (int y = 0; y < MICRO_HEIGHT; y++) {
                        D1GfxPixel pixel = frameSrc->getPixel(x, y); // 8[4]
                        if (pixel.isTransparent()) {
                            change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }
                }
            } else if (i == 8 || i == 20 || i == 23 || i == 26 || i == 29) { // 5[4], 192[4], 200[4], 212[4], 216[4] + 180[4]
             /*for (int x = 11; x < 16; x++) {
                 for (int y = 1; y < 13; y++) {
                     if (y > x - 11) {
                         if (i == 26 && y > 9) {
                             continue; // 212[4]
                         }
                         change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                     }
                 }
             }*/
            } else if (i == 17) { // 180[4]
             /*for (int x = 11; x < 16; x++) {
                 for (int y = 2; y < 18; y++) {
                     if (y > 2 * x - 24) {
                         change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                     }
                 }
             }

             change |= frame->setPixel(11,  1, D1GfxPixel::transparentPixel());
             change |= frame->setPixel(10,  0, D1GfxPixel::transparentPixel());*/
                change |= frame->setPixel(10, 0, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(11, 2, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(12, 3, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(13, 4, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(13, 5, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(14, 7, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(15, 17, D1GfxPixel::transparentPixel());
            }
        }

        // mask 6[1], 213[1] using 5[0] and 5[2]
        if (i >= 33 && i < 43 && (i % 2) == (33 % 2)) {
            const CelMicro &microSrc1 = micros[7];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[6];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 5[2]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 5[0]
                    }

                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // mask 6[3], 217[3]
        if (i >= 34 && i < 43 && (i % 2) == (34 % 2)) {
            for (int x = 16; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
            // fix shadow
            change |= frame->setPixel(15, 23, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(15, 24, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(15, 25, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(15, 26, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(14, 24, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(14, 25, D1GfxPixel::colorPixel(126));
            change |= frame->setPixel(14, 26, D1GfxPixel::colorPixel(111));
        }

        // mask 6[5], 193[5], 201[5], 213[5], 217[5] + 51[5] using 180[4] and 5[6]
        if (i >= 43 && i < 49) {
            const CelMicro &microSrc1 = micros[5];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[17];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    /*if (y < MICRO_HEIGHT / 2) {
                        D1GfxPixel pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 5[6]
                        if (!pixel.isTransparent()) {
                            change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    } else {
                        if (x > 15 || (y < x + 6)) {
                            change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }*/
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 5[6]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 180[4]
                    }

                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            /*// fix shadow
            if (i != 48) {
                // change |= frame->setPixel(15, 21, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(15, 22, D1GfxPixel::colorPixel(111));
                change |= frame->setPixel(15, 23, D1GfxPixel::colorPixel(111));
                change |= frame->setPixel(15, 24, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(15, 25, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(15, 26, D1GfxPixel::colorPixel(63));
                change |= frame->setPixel(15, 27, D1GfxPixel::colorPixel(63));
                change |= frame->setPixel(15, 28, D1GfxPixel::colorPixel(125));
            }*/
        }

        if (i == 15) { // 180[0] - fix glitch(?) + sync
            change |= frame->setPixel(14, 0, D1GfxPixel::colorPixel(107));
            change |= frame->setPixel(14, 1, D1GfxPixel::colorPixel(99));
        }
        if (i == 24) { // 212[0] - fix glitch(?)
            change |= frame->setPixel(14, 4, D1GfxPixel::colorPixel(100));
            change |= frame->setPixel(13, 5, D1GfxPixel::colorPixel(116));
            change |= frame->setPixel(14, 5, D1GfxPixel::colorPixel(119));
            change |= frame->setPixel(12, 6, D1GfxPixel::colorPixel(98));
            change |= frame->setPixel(13, 6, D1GfxPixel::colorPixel(118));
            change |= frame->setPixel(12, 7, D1GfxPixel::colorPixel(116));
            change |= frame->setPixel(11, 8, D1GfxPixel::colorPixel(115));
        }
        if (i == 27) { // 216[0] - fix glitch(?)
            change |= frame->setPixel(14, 5, D1GfxPixel::colorPixel(99));
        }
        if (i == 39) { // 213[1] - fix glitch
            change |= frame->setPixel(13, 19, D1GfxPixel::colorPixel(112));
            change |= frame->setPixel(12, 20, D1GfxPixel::colorPixel(114));
            change |= frame->setPixel(11, 22, D1GfxPixel::colorPixel(97));
            change |= frame->setPixel(10, 23, D1GfxPixel::colorPixel(97));
            change |= frame->setPixel(9, 24, D1GfxPixel::colorPixel(97));
            change |= frame->setPixel(8, 25, D1GfxPixel::colorPixel(96));
            change |= frame->setPixel(7, 26, D1GfxPixel::colorPixel(116));
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

void D1Tileset::cleanupHell(std::set<unsigned> &deletedFrames, bool silent)
{
    constexpr int blockSize = BLOCK_SIZE_L4;
    // patch stairs III.
    ReplaceSubtile(this->til, 45 - 1, 0, 17 - 1, silent);  // 137
    ReplaceSubtile(this->til, 45 - 1, 1, 18 - 1, silent);  // 138

    // fix shadow
    ReplaceSubtile(this->til, 36 - 1, 3, 155 - 1, silent); // 105
    ReplaceSubtile(this->til, 72 - 1, 0, 17 - 1, silent);  // 224
    ReplaceSubtile(this->til, 55 - 1, 2, 154 - 1, silent); // 175

    // create the new shadows
    ReplaceSubtile(this->til,  61 - 1, 0, 5 - 1, silent); // copy from tile 2
    ReplaceSubtile(this->til,  61 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til,  61 - 1, 2, 35 - 1, silent);
    ReplaceSubtile(this->til,  61 - 1, 3, 239 - 1, silent);
    ReplaceSubtile(this->til,  62 - 1, 0, 5 - 1, silent); // copy from tile 2
    ReplaceSubtile(this->til,  62 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til,  62 - 1, 2, 7 - 1, silent);
    ReplaceSubtile(this->til,  62 - 1, 3, 176 - 1, silent);
    ReplaceSubtile(this->til,  76 - 1, 0, 41 - 1, silent); // copy from tile 15
    ReplaceSubtile(this->til,  76 - 1, 1, 31 - 1, silent);
    ReplaceSubtile(this->til,  76 - 1, 2, 13 - 1, silent);
    ReplaceSubtile(this->til,  76 - 1, 3, 239 - 1, silent);
    ReplaceSubtile(this->til, 129 - 1, 0, 41 - 1, silent); // copy from tile 15
    ReplaceSubtile(this->til, 129 - 1, 1, 31 - 1, silent);
    ReplaceSubtile(this->til, 129 - 1, 2, 10 - 1, silent);
    ReplaceSubtile(this->til, 129 - 1, 3, 176 - 1, silent);
    ReplaceSubtile(this->til, 130 - 1, 0, 177 - 1, silent); // copy from tile 56
    ReplaceSubtile(this->til, 130 - 1, 1, 31 - 1, silent);
    ReplaceSubtile(this->til, 130 - 1, 2, 37 - 1, silent);
    ReplaceSubtile(this->til, 130 - 1, 3, 239 - 1, silent);
    ReplaceSubtile(this->til, 131 - 1, 0, 177 - 1, silent); // copy from tile 56
    ReplaceSubtile(this->til, 131 - 1, 1, 31 - 1, silent);
    ReplaceSubtile(this->til, 131 - 1, 2, 179 - 1, silent);
    ReplaceSubtile(this->til, 131 - 1, 3, 176 - 1, silent);
    ReplaceSubtile(this->til, 132 - 1, 0, 24 - 1, silent); // copy from tile 8
    ReplaceSubtile(this->til, 132 - 1, 1, 25 - 1, silent);
    ReplaceSubtile(this->til, 132 - 1, 2, 13 - 1, silent);
    ReplaceSubtile(this->til, 132 - 1, 3, 239 - 1, silent);
    ReplaceSubtile(this->til, 133 - 1, 0, 24 - 1, silent); // copy from tile 8
    ReplaceSubtile(this->til, 133 - 1, 1, 25 - 1, silent);
    ReplaceSubtile(this->til, 133 - 1, 2, 10 - 1, silent);
    ReplaceSubtile(this->til, 133 - 1, 3, 176 - 1, silent);
    ReplaceSubtile(this->til, 134 - 1, 0, 38 - 1, silent); // copy from tile 14
    ReplaceSubtile(this->til, 134 - 1, 1, 31 - 1, silent);
    ReplaceSubtile(this->til, 134 - 1, 2, 26 - 1, silent);
    ReplaceSubtile(this->til, 134 - 1, 3, 239 - 1, silent);
    ReplaceSubtile(this->til, 135 - 1, 0, 38 - 1, silent); // copy from tile 14
    ReplaceSubtile(this->til, 135 - 1, 1, 31 - 1, silent);
    ReplaceSubtile(this->til, 135 - 1, 2, 16 - 1, silent);
    ReplaceSubtile(this->til, 135 - 1, 3, 176 - 1, silent);
    // separate subtiles for the automap
    ReplaceSubtile(this->til, 44 - 1, 2, 136 - 1, silent);
    ReplaceSubtile(this->til, 136 - 1, 0, 149 - 1, silent);
    ReplaceSubtile(this->til, 136 - 1, 1, 153 - 1, silent);
    ReplaceSubtile(this->til, 136 - 1, 2, 97 - 1, silent);
    ReplaceSubtile(this->til, 136 - 1, 3, 136 - 1, silent);
    // use common subtiles
    ReplaceSubtile(this->til,  4 - 1, 2, 10 - 1, silent);  // 13
    ReplaceSubtile(this->til, 81 - 1, 2, 10 - 1, silent);
    ReplaceSubtile(this->til,  6 - 1, 3, 4 - 1, silent);   // 20
    ReplaceSubtile(this->til,  8 - 1, 2, 10 - 1, silent);  // 26
    ReplaceSubtile(this->til, 15 - 1, 2, 10 - 1, silent);
    ReplaceSubtile(this->til,  7 - 1, 1, 9 - 1, silent);   // 22
    ReplaceSubtile(this->til,  9 - 1, 2, 23 - 1, silent);  // 29
    ReplaceSubtile(this->til, 10 - 1, 2, 23 - 1, silent);
    ReplaceSubtile(this->til, 12 - 1, 1, 12 - 1, silent);  // 35
    ReplaceSubtile(this->til, 14 - 1, 2, 16 - 1, silent);  // 40
    ReplaceSubtile(this->til, 16 - 1, 1, 9 - 1, silent);   // 43
    ReplaceSubtile(this->til, 16 - 1, 2, 33 - 1, silent);  // 44
    ReplaceSubtile(this->til, 22 - 1, 2, 58 - 1, silent);  // 61
    ReplaceSubtile(this->til, 23 - 1, 1, 57 - 1, silent);  // 63
    ReplaceSubtile(this->til, 25 - 1, 1, 66 - 1, silent);  // 69
    ReplaceSubtile(this->til, 26 - 1, 1, 60 - 1, silent);  // 72
    ReplaceSubtile(this->til, 26 - 1, 2, 67 - 1, silent);  // 73
    ReplaceSubtile(this->til, 27 - 1, 1, 60 - 1, silent);  // 75
    ReplaceSubtile(this->til, 32 - 1, 1, 157 - 1, silent); // 87
    ReplaceSubtile(this->til, 32 - 1, 3, 158 - 1, silent); // 89
    ReplaceSubtile(this->til, 36 - 1, 0, 149 - 1, silent); // 102
    ReplaceSubtile(this->til, 36 - 1, 1, 153 - 1, silent); // 103
    ReplaceSubtile(this->til, 36 - 1, 2, 154 - 1, silent); // 104
    ReplaceSubtile(this->til, 37 - 1, 2, 147 - 1, silent); // 108
    ReplaceSubtile(this->til, 39 - 1, 1, 157 - 1, silent); // 115
    ReplaceSubtile(this->til, 39 - 1, 2, 147 - 1, silent); // 116
    ReplaceSubtile(this->til, 39 - 1, 3, 158 - 1, silent); // 117
    ReplaceSubtile(this->til, 40 - 1, 3, 4 - 1, silent);   // 121
    ReplaceSubtile(this->til, 41 - 1, 0, 149 - 1, silent); // 122
    ReplaceSubtile(this->til, 41 - 1, 1, 153 - 1, silent); // 123
    ReplaceSubtile(this->til, 41 - 1, 3, 155 - 1, silent); // 125
    ReplaceSubtile(this->til, 42 - 1, 2, 19 - 1, silent);  // 128
    ReplaceSubtile(this->til, 46 - 1, 2, 19 - 1, silent);  // 143
    ReplaceSubtile(this->til, 46 - 1, 3, 4 - 1, silent);   // 144
    ReplaceSubtile(this->til, 47 - 1, 3, 158 - 1, silent); // 148
    ReplaceSubtile(this->til, 54 - 1, 2, 161 - 1, silent); // 173
    ReplaceSubtile(this->til, 54 - 1, 3, 162 - 1, silent); // 174
    ReplaceSubtile(this->til, 57 - 1, 2, 19 - 1, silent);  // 182
    ReplaceSubtile(this->til, 71 - 1, 2, 19 - 1, silent);  // 222
    ReplaceSubtile(this->til, 71 - 1, 3, 4 - 1, silent);   // 223
    ReplaceSubtile(this->til, 73 - 1, 3, 4 - 1, silent);   // 231
    ReplaceSubtile(this->til, 74 - 1, 2, 19 - 1, silent);  // 234
    ReplaceSubtile(this->til, 75 - 1, 0, 156 - 1, silent); // 236
    ReplaceSubtile(this->til, 75 - 1, 1, 157 - 1, silent); // 237
    ReplaceSubtile(this->til, 77 - 1, 1, 31 - 1, silent);  // 245
    ReplaceSubtile(this->til, 79 - 1, 2, 23 - 1, silent);  // 250
    ReplaceSubtile(this->til, 84 - 1, 0, 159 - 1, silent); // 261
    ReplaceSubtile(this->til, 85 - 1, 2, 147 - 1, silent); // 267
    ReplaceSubtile(this->til, 86 - 1, 1, 153 - 1, silent); // 270
    ReplaceSubtile(this->til, 87 - 1, 3, 4 - 1, silent);   // 276
    ReplaceSubtile(this->til, 88 - 1, 2, 19 - 1, silent);  // 279
    ReplaceSubtile(this->til, 89 - 1, 1, 160 - 1, silent); // 282
    ReplaceSubtile(this->til, 90 - 1, 1, 153 - 1, silent); // 286
    ReplaceSubtile(this->til, 91 - 1, 2, 19 - 1, silent);  // 291
    ReplaceSubtile(this->til, 94 - 1, 2, 154 - 1, silent); // 303
    ReplaceSubtile(this->til, 94 - 1, 3, 155 - 1, silent); // 304
    ReplaceSubtile(this->til, 98 - 1, 0, 149 - 1, silent); // 317
    ReplaceSubtile(this->til, 107 - 1, 0, 149 - 1, silent);
    ReplaceSubtile(this->til, 101 - 1, 2, 147 - 1, silent); // 331
    ReplaceSubtile(this->til, 110 - 1, 2, 147 - 1, silent);
    ReplaceSubtile(this->til, 106 - 1, 3, 4 - 1, silent); // 352
    ReplaceSubtile(this->til, 115 - 1, 3, 4 - 1, silent);

    // use common subtiles instead of minor alterations
    ReplaceSubtile(this->til,  9 - 1, 1, 25 - 1, silent);  // 28
    ReplaceSubtile(this->til, 11 - 1, 1, 25 - 1, silent);
    ReplaceSubtile(this->til, 13 - 1, 1, 12 - 1, silent);  // 37
    ReplaceSubtile(this->til, 14 - 1, 1, 31 - 1, silent);  // 39
    ReplaceSubtile(this->til, 15 - 1, 1, 31 - 1, silent);
    ReplaceSubtile(this->til, 29 - 1, 1, 60 - 1, silent);  // 80
    ReplaceSubtile(this->til, 29 - 1, 2, 70 - 1, silent);  // 81
    ReplaceSubtile(this->til, 31 - 1, 2, 19 - 1, silent);  // 84
    ReplaceSubtile(this->til, 31 - 1, 3, 4 - 1, silent);   // 85
    ReplaceSubtile(this->til, 52 - 1, 2, 33 - 1, silent);  // 165
    ReplaceSubtile(this->til, 52 - 1, 3, 4 - 1, silent);   // 166
    ReplaceSubtile(this->til, 53 - 1, 1, 119 - 1, silent); // 168
    ReplaceSubtile(this->til, 53 - 1, 3, 4 - 1, silent);   // 170
    ReplaceSubtile(this->til, 56 - 1, 1, 31 - 1, silent);  // 178
    ReplaceSubtile(this->til, 63 - 1, 3, 158 - 1, silent); // 191
    ReplaceSubtile(this->til, 64 - 1, 3, 158 - 1, silent); // 195
    ReplaceSubtile(this->til, 65 - 1, 3, 155 - 1, silent); // 199
    ReplaceSubtile(this->til, 66 - 1, 3, 162 - 1, silent); // 203
    ReplaceSubtile(this->til, 67 - 1, 3, 162 - 1, silent); // 207
    ReplaceSubtile(this->til, 68 - 1, 3, 158 - 1, silent); // 211
    ReplaceSubtile(this->til, 69 - 1, 3, 155 - 1, silent); // 215
    ReplaceSubtile(this->til, 70 - 1, 3, 4 - 1, silent);   // 219
    // ReplaceSubtile(this->til, 76 - 1, 3, 155 - 1, silent); // 243
    ReplaceSubtile(this->til, 77 - 1, 3, 155 - 1, silent); // 243
    ReplaceSubtile(this->til, 78 - 1, 3, 155 - 1, silent);
    ReplaceSubtile(this->til, 79 - 1, 1, 25 - 1, silent);  // 249
    ReplaceSubtile(this->til,  4 - 1, 3, 4 - 1, silent);   // 14
    ReplaceSubtile(this->til, 81 - 1, 3, 4 - 1, silent);   // 256
    ReplaceSubtile(this->til, 82 - 1, 3, 4 - 1, silent);   // 259
    ReplaceSubtile(this->til, 83 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 93 - 1, 1, 119 - 1, silent);  // 298
    ReplaceSubtile(this->til, 95 - 1, 3, 162 - 1, silent);  // 308
    ReplaceSubtile(this->til, 97 - 1, 1, 153 - 1, silent);  // 314
    ReplaceSubtile(this->til, 97 - 1, 3, 155 - 1, silent);  // 316
    ReplaceSubtile(this->til, 96 - 1, 1, 153 - 1, silent);  // 310
    ReplaceSubtile(this->til, 96 - 1, 3, 155 - 1, silent);  // 312
    ReplaceSubtile(this->til, 103 - 1, 1, 119 - 1, silent); // 338
    ReplaceSubtile(this->til, 112 - 1, 1, 119 - 1, silent);

    // eliminate subtiles of unused tiles
    const int unusedTiles[] = {
        20, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 137
    };
    constexpr int blankSubtile = 14;
    for (int n = 0; n < lengthof(unusedTiles); n++) {
        int tileId = unusedTiles[n];
        ReplaceSubtile(this->til, tileId - 1, 0, blankSubtile - 1, silent);
        ReplaceSubtile(this->til, tileId - 1, 1, blankSubtile - 1, silent);
        ReplaceSubtile(this->til, tileId - 1, 2, blankSubtile - 1, silent);
        ReplaceSubtile(this->til, tileId - 1, 3, blankSubtile - 1, silent);
    }


    this->patchHellChaos(silent);
    this->patchHellFloor(silent);
    // patch stairs II.
    // if (pSubtiles[MICRO_IDX(137 - 1, blockSize, 1)] != NULL) {
    if (this->patchHellStairs(silent)) {
        MoveMcr(140, 3, 137, 1);
        MoveMcr(140, 2, 137, 0);
        MoveMcr(140, 4, 137, 2);

        Blk2Mcr(139, 1);
        Blk2Mcr(136, 1);

        Blk2Mcr(98, 0);

        ReplaceMcr(111, 0, 131, 0);
    }
    this->patchHellWall1(silent);
    this->patchHellWall2(silent);

    // useless pixels
    Blk2Mcr(111, 1);
    Blk2Mcr(132, 0);
    Blk2Mcr(172, 3);

    // create new shadow
    SetMcr(35, 0, 238, 0);
    SetMcr(35, 1, 238, 1);
    SetMcr(35, 4, 96, 6);
    SetMcr(35, 6, 7, 6);
    HideMcr(35, 5);
    HideMcr(35, 7);
    ReplaceMcr(26, 0, 238, 0);
    ReplaceMcr(26, 1, 238, 1);
    SetMcr(26, 4, 96, 6);
    SetMcr(26, 6, 16, 6);
    ReplaceMcr(37, 0, 238, 0);
    SetMcr(37, 1, 238, 1);
    HideMcr(37, 5);
    HideMcr(37, 7);
    SetMcr(37, 8, 179, 8);
    SetMcr(37, 10, 179, 10);
    ReplaceMcr(13, 0, 238, 0);
    ReplaceMcr(13, 1, 238, 1);

    // fix chaos
    // - reuse
    // ReplaceMcr(48, 2, 55, 2);
    ReplaceMcr(46, 2, 53, 2);
    ReplaceMcr(46, 3, 53, 3);
    ReplaceMcr(50, 2, 53, 2);
    ReplaceMcr(50, 3, 53, 3);
    ReplaceMcr(50, 6, 5, 6);
    // ReplaceMcr(51, 3, 54, 3);

    // ReplaceMcr(58, 2, 55, 2);
    ReplaceMcr(56, 2, 53, 2);
    ReplaceMcr(56, 3, 53, 3);
    // ReplaceMcr(57, 3, 54, 3);
    ReplaceMcr(57, 5, 51, 5);

    // ReplaceMcr(61, 2, 55, 2);
    ReplaceMcr(59, 2, 53, 2);
    ReplaceMcr(59, 3, 53, 3);
    // ReplaceMcr(60, 3, 54, 3);
    ReplaceMcr(60, 5, 51, 5);

    // ReplaceMcr(64, 2, 55, 2);
    ReplaceMcr(64, 4, 58, 4);
    ReplaceMcr(62, 2, 53, 2);
    ReplaceMcr(62, 3, 53, 3);
    ReplaceMcr(62, 5, 56, 5); // lost details
    // ReplaceMcr(63, 3, 54, 3);
    // ReplaceMcr(63, 5, 51, 5);

    ReplaceMcr(65, 2, 53, 2);
    ReplaceMcr(65, 3, 53, 3);

    // ReplaceMcr(70, 2, 55, 2);
    ReplaceMcr(70, 4, 48, 4);
    ReplaceMcr(68, 2, 53, 2);
    ReplaceMcr(68, 3, 53, 3);
    ReplaceMcr(68, 5, 65, 5); // lost details
    ReplaceMcr(68, 7, 11, 7); // lost details
    ReplaceMcr(68, 9, 11, 9);
    // ReplaceMcr(69, 5, 66, 5); // lost details
    // ReplaceMcr(69, 7, 66, 7); // lost details

    // ReplaceMcr(73, 4, 58, 4);
    // ReplaceMcr(73, 6, 64, 6);
    ReplaceMcr(71, 2, 53, 2);
    ReplaceMcr(71, 3, 53, 3);
    ReplaceMcr(71, 5, 59, 5);
    ReplaceMcr(71, 7, 1, 7);
    ReplaceMcr(71, 4, 65, 4);
    ReplaceMcr(71, 6, 65, 6);
    // ReplaceMcr(72, 3, 54, 3);
    // ReplaceMcr(72, 5, 51, 5);

    ReplaceMcr(76, 4, 67, 4);
    ReplaceMcr(74, 2, 53, 2);
    ReplaceMcr(74, 3, 53, 3);
    ReplaceMcr(74, 5, 59, 5);
    ReplaceMcr(74, 7, 1, 7);

    ReplaceMcr(77, 2, 53, 2);
    ReplaceMcr(77, 3, 53, 3);
    ReplaceMcr(77, 4, 62, 4);
    ReplaceMcr(77, 6, 5, 6);
    ReplaceMcr(78, 5, 66, 5); // lost details
    ReplaceMcr(78, 7, 9, 7);

    ReplaceMcr(79, 2, 53, 2);
    ReplaceMcr(79, 3, 53, 3);
    ReplaceMcr(79, 4, 62, 4);
    ReplaceMcr(79, 5, 59, 5);
    // - mask 1
    Blk2Mcr(53, 0);
    Blk2Mcr(53, 1);
    Blk2Mcr(53, 6);
    Blk2Mcr(53, 7);
    Blk2Mcr(54, 3);
    Blk2Mcr(54, 7);
    Blk2Mcr(55, 2);
    Blk2Mcr(55, 6);

    Blk2Mcr(48, 2);
    Blk2Mcr(58, 2);
    // Blk2Mcr(61, 2);
    Blk2Mcr(64, 2);
    Blk2Mcr(70, 2);
    Blk2Mcr(52, 2); Blk2Mcr(67, 2); Blk2Mcr(73, 2); Blk2Mcr(76, 2); Blk2Mcr(391, 2); Blk2Mcr(394, 2); Blk2Mcr(406, 2); /*Blk2Mcr(412, 2);*/ Blk2Mcr(415, 2); Blk2Mcr(454, 2);
    Blk2Mcr(51, 3);
    Blk2Mcr(57, 3);
    Blk2Mcr(60, 3);
    // Blk2Mcr(63, 3);
    Blk2Mcr(46, 0); Blk2Mcr(50, 0); Blk2Mcr(56, 0); Blk2Mcr(59, 0); Blk2Mcr(62, 0); Blk2Mcr(65, 0); Blk2Mcr(68, 0); Blk2Mcr(71, 0); Blk2Mcr(74, 0); Blk2Mcr(77, 0); Blk2Mcr(79, 0); // Blk2Mcr(456, 6);
    Blk2Mcr(46, 1); Blk2Mcr(50, 1); Blk2Mcr(56, 1); Blk2Mcr(59, 1); Blk2Mcr(62, 1); Blk2Mcr(65, 1); Blk2Mcr(68, 1); Blk2Mcr(71, 1); Blk2Mcr(74, 1); Blk2Mcr(77, 1); Blk2Mcr(79, 1); // Blk2Mcr(456, 7);

    Blk2Mcr(392, 6); // Blk2Mcr(453, 10);
    Blk2Mcr(392, 7); // Blk2Mcr(453, 11);

    Blk2Mcr(393, 7); Blk2Mcr(455, 11);
    Blk2Mcr(394, 6); Blk2Mcr(454, 6); Blk2Mcr(454, 10);

    Blk2Mcr(47, 3); Blk2Mcr(66, 3); /*Blk2Mcr(69, 3); */ Blk2Mcr(78, 3); Blk2Mcr(386, 3); Blk2Mcr(393, 3); Blk2Mcr(405, 3); /*Blk2Mcr(408, 3); Blk2Mcr(417, 3);*/ Blk2Mcr(455, 7);

    // reuse micros
    ReplaceMcr(109, 0, 158, 0);
    ReplaceMcr(99, 1, 157, 1);
    ReplaceMcr(86, 1, 156, 1);
    ReplaceMcr(82, 0, 141, 0);
    ReplaceMcr(289, 0, 141, 0);
    ReplaceMcr(124, 1, 154, 1);
    ReplaceMcr(202, 0, 161, 0);
    ReplaceMcr(307, 0, 161, 0); // lost details
    ReplaceMcr(311, 0, 154, 0);
    ReplaceMcr(205, 0, 160, 0); // lost details
    ReplaceMcr(205, 1, 160, 1); // lost details
    ReplaceMcr(189, 0, 157, 0); // lost details
    ReplaceMcr(189, 1, 157, 1); // lost details
    ReplaceMcr(209, 0, 157, 0); // lost details
    ReplaceMcr(209, 1, 157, 1); // lost details
    ReplaceMcr(197, 0, 153, 0); // lost details
    ReplaceMcr(197, 1, 153, 1); // lost details
    ReplaceMcr(241, 1, 153, 1); // lost details
    ReplaceMcr(302, 1, 153, 1); // lost details
    ReplaceMcr(164, 0, 2, 0); // lost details
    ReplaceMcr(164, 1, 2, 1); // lost details
    ReplaceMcr(225, 1, 2, 1);
    // ReplaceMcr(43, 0, 22, 0); // lost details
    ReplaceMcr(169, 0, 19, 0); // lost details
    ReplaceMcr(255, 0, 258, 0); // lost details
    ReplaceMcr(299, 0, 19, 0);
    ReplaceMcr(272, 1, 155, 1);
    ReplaceMcr(288, 1, 155, 1);
    ReplaceMcr(265, 0, 156, 0);
    // ReplaceMcr(281, 1, 159, 1);
    ReplaceMcr(280, 0, 4, 0);
    ReplaceMcr(129, 0, 4, 0);
    ReplaceMcr(340, 1, 4, 1);
    ReplaceMcr(373, 1, 4, 1);
    ReplaceMcr(337, 1, 17, 1);
    ReplaceMcr(371, 1, 17, 1);
    ReplaceMcr(145, 0, 156, 0);
    ReplaceMcr(329, 0, 156, 0);
    ReplaceMcr(364, 0, 156, 0);
    ReplaceMcr(332, 0, 158, 0);
    ReplaceMcr(366, 0, 158, 0);

    ReplaceMcr(194, 0, 147, 0);
    ReplaceMcr(194, 1, 147, 1);
    ReplaceMcr(214, 0, 154, 0);
    ReplaceMcr(214, 1, 154, 1);
    ReplaceMcr(242, 0, 154, 0);
    ReplaceMcr(242, 1, 154, 1);

    ReplaceMcr(232, 0, 17, 0);
    ReplaceMcr(235, 0, 4, 0);
    ReplaceMcr(239, 1, 158, 1);

    ReplaceMcr(10, 0, 19, 0); // lost details
    ReplaceMcr(10, 1, 19, 1); // lost details
    ReplaceMcr(16, 0, 19, 0); // lost details
    ReplaceMcr(16, 1, 19, 1); // lost details
    ReplaceMcr(253, 0, 19, 0); // lost details
    ReplaceMcr(253, 1, 19, 1); // lost details

    ReplaceMcr(7, 0, 19, 0); // lost details
    ReplaceMcr(7, 1, 19, 1); // lost details
    // ReplaceMcr(26, 0, 19, 0); // lost details
    // ReplaceMcr(26, 1, 19, 1); // lost details
    // ReplaceMcr(40, 0, 19, 0); // lost details
    // ReplaceMcr(40, 1, 19, 1); // lost details
    ReplaceMcr(179, 0, 19, 0); // lost details
    ReplaceMcr(179, 1, 19, 1); // lost details
    ReplaceMcr(218, 0, 19, 0); // lost details
    ReplaceMcr(218, 1, 19, 1); // lost details
    ReplaceMcr(230, 0, 19, 0); // lost details
    ReplaceMcr(230, 1, 19, 1); // lost details

    ReplaceMcr(119, 0, 2, 0);
    ReplaceMcr(119, 1, 2, 1);
    // ReplaceMcr(168, 0, 2, 0); // lost details
    // ReplaceMcr(168, 1, 2, 1); // lost details
    // ReplaceMcr(298, 0, 2, 0); // lost details
    // ReplaceMcr(298, 1, 2, 1); // lost details
    // ReplaceMcr(338, 0, 2, 0); // lost details
    // ReplaceMcr(338, 1, 2, 1); // lost details

    // ReplaceMcr(37, 0, 9, 0); // lost details
    ReplaceMcr(181, 0, 9, 0); // lost details
    ReplaceMcr(229, 0, 9, 0); // lost details
    ReplaceMcr(229, 1, 2, 1); // lost details

    ReplaceMcr(258, 0, 255, 0); // lost details

    ReplaceMcr(34, 1, 11, 1);
    ReplaceMcr(34, 3, 11, 3);
    ReplaceMcr(34, 5, 11, 5);
    ReplaceMcr(34, 7, 11, 7);
    ReplaceMcr(36, 1, 11, 1);
    ReplaceMcr(36, 3, 11, 3);
    ReplaceMcr(36, 5, 11, 5);
    ReplaceMcr(36, 7, 11, 7);
    ReplaceMcr(65, 7, 11, 7);
    // ReplaceMcr(68, 9, 11, 9);
    ReplaceMcr(251, 1, 11, 1);
    ReplaceMcr(254, 1, 11, 1);
    ReplaceMcr(254, 3, 11, 3);

    ReplaceMcr(38, 7, 1, 7);

    ReplaceMcr(23, 0, 3, 0);
    ReplaceMcr(33, 0, 3, 0);
    ReplaceMcr(190, 0, 3, 0);
    ReplaceMcr(247, 0, 3, 0);
    ReplaceMcr(23, 1, 3, 1);
    ReplaceMcr(33, 1, 3, 1);
    ReplaceMcr(3, 4, 23, 4);

    ReplaceMcr(190, 8, 3, 8); // lost details
    ReplaceMcr(48, 6, 3, 6);

    ReplaceMcr(198, 2, 3, 2); // lost details
    ReplaceMcr(210, 2, 3, 2); // lost details
    ReplaceMcr(253, 4, 242, 4); // lost details

    ReplaceMcr(193, 9, 6, 9);

    ReplaceMcr(47, 7, 2, 7);
    ReplaceMcr(189, 7, 2, 7);
    ReplaceMcr(201, 7, 6, 7);
    ReplaceMcr(258, 7, 9, 7);
    ReplaceMcr(66, 7, 12, 7);
    ReplaceMcr(252, 7, 12, 7);
    ReplaceMcr(255, 7, 12, 7);

    ReplaceMcr(9, 5, 2, 5);
    ReplaceMcr(189, 5, 2, 5);
    ReplaceMcr(258, 5, 2, 5);
    ReplaceMcr(255, 5, 12, 5);

    ReplaceMcr(241, 5, 252, 5);

    ReplaceMcr(201, 3, 6, 3);
    ReplaceMcr(213, 3, 6, 3);
    ReplaceMcr(193, 3, 217, 3);

    ReplaceMcr(25, 1, 6, 1);
    ReplaceMcr(31, 1, 6, 1);
    ReplaceMcr(193, 1, 6, 1);
    ReplaceMcr(201, 1, 6, 1);
    // ReplaceMcr(245, 1, 6, 1);

    ReplaceMcr(217, 1, 213, 1);

    ReplaceMcr(25, 0, 6, 0);
    ReplaceMcr(31, 0, 6, 0);
    ReplaceMcr(217, 0, 6, 0);
    // ReplaceMcr(245, 0, 6, 0); // lost details

    ReplaceMcr(21, 8, 260, 8);
    ReplaceMcr(27, 8, 260, 8);
    ReplaceMcr(30, 8, 260, 8);
    ReplaceMcr(56, 8, 260, 8);
    ReplaceMcr(59, 8, 260, 8);
    ReplaceMcr(248, 8, 244, 8);

    ReplaceMcr(192, 6, 5, 6); // fix glitch (connection)
    ReplaceMcr(216, 6, 5, 6);
    ReplaceMcr(38, 6, 15, 6);
    ReplaceMcr(65, 6, 15, 6);
    ReplaceMcr(71, 6, 15, 6);
    ReplaceMcr(74, 6, 8, 6);
    ReplaceMcr(200, 6, 180, 6);
    ReplaceMcr(67, 6, 16, 6);

    ReplaceMcr(248, 4, 21, 4);
    ReplaceMcr(36, 4, 32, 4);
    ReplaceMcr(34, 4, 15, 4);
    ReplaceMcr(38, 4, 15, 4);

    ReplaceMcr(36, 2, 21, 2);
    ReplaceMcr(204, 2, 188, 2);
    ReplaceMcr(192, 2, 5, 2);
    ReplaceMcr(212, 2, 5, 2);
    ReplaceMcr(216, 2, 5, 2);
    ReplaceMcr(34, 2, 15, 2);
    ReplaceMcr(38, 2, 15, 2);
    ReplaceMcr(163, 2, 1, 2); // lost details
    ReplaceMcr(196, 2, 1, 2);
    ReplaceMcr(208, 2, 1, 2);

    ReplaceMcr(11, 0, 8, 0);
    ReplaceMcr(244, 0, 8, 0);
    ReplaceMcr(254, 0, 8, 0);
    ReplaceMcr(27, 0, 21, 0);
    ReplaceMcr(30, 0, 21, 0);
    ReplaceMcr(32, 0, 21, 0);
    ReplaceMcr(45, 0, 21, 0);
    ReplaceMcr(246, 0, 21, 0);
    ReplaceMcr(248, 0, 21, 0);
    ReplaceMcr(36, 0, 21, 0); // fix glitch(?)
    ReplaceMcr(34, 0, 15, 0);
    ReplaceMcr(251, 0, 15, 0);
    ReplaceMcr(41, 0, 24, 0);

    ReplaceMcr(24, 9, 260, 9);
    ReplaceMcr(27, 9, 260, 9);
    ReplaceMcr(32, 9, 260, 9);
    ReplaceMcr(56, 9, 260, 9);
    ReplaceMcr(62, 9, 260, 9);
    ReplaceMcr(257, 9, 260, 9);
    ReplaceMcr(248, 9, 246, 9);

    ReplaceMcr(21, 7, 8, 7);
    ReplaceMcr(42, 7, 8, 7);
    ReplaceMcr(77, 7, 8, 7);
    ReplaceMcr(46, 7, 1, 7);
    ReplaceMcr(167, 7, 1, 7);
    ReplaceMcr(188, 7, 1, 7); // fix glitch (connection)

    ReplaceMcr(248, 5, 24, 5);

    ReplaceMcr(24, 3, 27, 3);
    ReplaceMcr(188, 3, 1, 3);
    ReplaceMcr(196, 3, 1, 3);
    ReplaceMcr(204, 3, 1, 3);
    ReplaceMcr(212, 3, 5, 3);
    ReplaceMcr(216, 3, 192, 3);
    ReplaceMcr(8, 3, 21, 3);
    ReplaceMcr(15, 3, 21, 3);
    ReplaceMcr(257, 3, 21, 3);
    ReplaceMcr(260, 3, 21, 3);

    ReplaceMcr(188, 1, 1, 1);
    ReplaceMcr(196, 1, 1, 1);
    ReplaceMcr(24, 1, 27, 1);
    ReplaceMcr(38, 1, 27, 1);
    ReplaceMcr(41, 1, 27, 1);
    ReplaceMcr(244, 1, 27, 1);
    ReplaceMcr(15, 1, 8, 1);
    ReplaceMcr(21, 1, 8, 1);
    ReplaceMcr(42, 1, 8, 1);
    ReplaceMcr(246, 1, 8, 1);
    ReplaceMcr(257, 1, 8, 1);
    ReplaceMcr(260, 1, 8, 1);

    // ReplaceMcr(194, 4, 7, 4); // lost details
    ReplaceMcr(7, 4, 96, 6); // lost details
    ReplaceMcr(194, 4, 96, 6); // lost details
    ReplaceMcr(16, 4, 96, 6); // lost details
    // ReplaceMcr(40, 4, 96, 6); // lost details
    ReplaceMcr(194, 6, 7, 6); // lost details

    // ReplaceMcr(175, 0, 147, 0); // after patchHellFloorCel

    // eliminate micros of unused subtiles
    // Blk2Mcr(240,  ...),
    // moved to other subtile
    // Blk2Mcr(137, 0);
    // Blk2Mcr(137, 1);
    // Blk2Mcr(137, 2);
    // reused for the new shadow
    // 35
    // Blk2Mcr(26, 0);
    // Blk2Mcr(26, 1);
    // Blk2Mcr(37, 0);
    // Blk2Mcr(13, 0);
    // Blk2Mcr(13, 1);
    Blk2Mcr(385, 0);
    Blk2Mcr(385, 1);
    Blk2Mcr(385, 2);
    Blk2Mcr(385, 3);
    Blk2Mcr(385, 7);
    Blk2Mcr(386, 0);
    Blk2Mcr(386, 1);
    Blk2Mcr(386, 7);
    Blk2Mcr(387, 0);
    Blk2Mcr(387, 1);
    Blk2Mcr(387, 2);
    Blk2Mcr(387, 6);
    Blk2Mcr(389, 0);
    Blk2Mcr(389, 1);
    Blk2Mcr(389, 2);
    Blk2Mcr(389, 3);
    Blk2Mcr(389, 6);
    Blk2Mcr(390, 0);
    Blk2Mcr(390, 1);
    Blk2Mcr(390, 3);
    Blk2Mcr(391, 0);
    Blk2Mcr(391, 1);
    Blk2Mcr(392, 0);
    Blk2Mcr(392, 1);
    Blk2Mcr(393, 0);
    Blk2Mcr(393, 1);
    Blk2Mcr(394, 0);
    Blk2Mcr(394, 1);
    Blk2Mcr(395, 0);
    Blk2Mcr(395, 1);
    Blk2Mcr(395, 2);
    Blk2Mcr(395, 3);
    Blk2Mcr(395, 8);
    Blk2Mcr(395, 9);
    Blk2Mcr(396, 0);
    Blk2Mcr(396, 1);
    Blk2Mcr(396, 3);
    Blk2Mcr(396, 5);
    Blk2Mcr(397, 0);
    Blk2Mcr(397, 1);
    Blk2Mcr(397, 2);
    Blk2Mcr(398, 0);
    Blk2Mcr(398, 1);
    Blk2Mcr(398, 2);
    Blk2Mcr(398, 3);
    Blk2Mcr(398, 8);
    Blk2Mcr(399, 0);
    Blk2Mcr(399, 1);
    Blk2Mcr(399, 3);
    Blk2Mcr(399, 5);
    Blk2Mcr(400, 0);
    Blk2Mcr(400, 1);
    Blk2Mcr(400, 2);
    Blk2Mcr(400, 4);
    Blk2Mcr(401, 0);
    Blk2Mcr(401, 1);
    Blk2Mcr(401, 2);
    Blk2Mcr(401, 3);
    Blk2Mcr(401, 5);
    Blk2Mcr(401, 9);
    Blk2Mcr(402, 0);
    Blk2Mcr(402, 1);
    Blk2Mcr(402, 3);
    Blk2Mcr(402, 5);
    Blk2Mcr(403, 0);
    Blk2Mcr(403, 1);
    Blk2Mcr(403, 2);
    Blk2Mcr(403, 4);
    Blk2Mcr(404, 0);
    Blk2Mcr(404, 1);
    Blk2Mcr(404, 2);
    Blk2Mcr(404, 3);
    Blk2Mcr(404, 6);
    Blk2Mcr(404, 7);
    Blk2Mcr(405, 0);
    Blk2Mcr(405, 1);
    Blk2Mcr(405, 7);
    Blk2Mcr(406, 0);
    Blk2Mcr(406, 1);
    Blk2Mcr(406, 6);
    Blk2Mcr(407, 0);
    Blk2Mcr(407, 1);
    Blk2Mcr(407, 2);
    Blk2Mcr(407, 3);
    Blk2Mcr(407, 5);
    Blk2Mcr(407, 7);
    Blk2Mcr(409, 0);
    Blk2Mcr(409, 1);
    Blk2Mcr(409, 2);
    Blk2Mcr(409, 4);
    Blk2Mcr(410, 0);
    Blk2Mcr(410, 1);
    Blk2Mcr(410, 2);
    Blk2Mcr(410, 3);
    Blk2Mcr(410, 4);
    Blk2Mcr(410, 5);
    Blk2Mcr(410, 6);
    Blk2Mcr(410, 7);
    Blk2Mcr(411, 0);
    Blk2Mcr(411, 1);
    Blk2Mcr(411, 3);
    Blk2Mcr(411, 5);
    Blk2Mcr(413, 0);
    Blk2Mcr(413, 1);
    Blk2Mcr(413, 2);
    Blk2Mcr(413, 3);
    Blk2Mcr(413, 5);
    Blk2Mcr(413, 6);
    Blk2Mcr(413, 7);
    Blk2Mcr(415, 0);
    Blk2Mcr(415, 1);
    Blk2Mcr(415, 4);
    Blk2Mcr(416, 0);
    Blk2Mcr(416, 1);
    Blk2Mcr(416, 2);
    Blk2Mcr(416, 3);
    Blk2Mcr(416, 4);
    Blk2Mcr(416, 6);
    Blk2Mcr(416, 7);
    Blk2Mcr(417, 0);
    Blk2Mcr(417, 1);
    Blk2Mcr(417, 3);
    Blk2Mcr(417, 5);
    Blk2Mcr(417, 7);
    Blk2Mcr(418, 0);
    Blk2Mcr(418, 1);
    Blk2Mcr(418, 2);
    Blk2Mcr(418, 3);
    Blk2Mcr(418, 4);
    Blk2Mcr(418, 5);
    Blk2Mcr(419, 0);
    Blk2Mcr(419, 1);
    Blk2Mcr(419, 3);
    Blk2Mcr(419, 5);
    Blk2Mcr(420, 0);
    Blk2Mcr(420, 1);
    Blk2Mcr(420, 2);
    Blk2Mcr(420, 4);
    Blk2Mcr(422, 0);
    Blk2Mcr(425, 1);
    Blk2Mcr(427, 0);
    Blk2Mcr(429, 0);
    Blk2Mcr(430, 1);
    Blk2Mcr(431, 0);
    Blk2Mcr(432, 0);
    Blk2Mcr(433, 1);
    Blk2Mcr(434, 1);
    Blk2Mcr(434, 5);
    Blk2Mcr(435, 0);
    Blk2Mcr(436, 0);
    Blk2Mcr(437, 0);
    Blk2Mcr(437, 3);
    Blk2Mcr(437, 4);
    Blk2Mcr(438, 0);
    Blk2Mcr(438, 1);
    Blk2Mcr(439, 0);
    Blk2Mcr(439, 4);
    Blk2Mcr(439, 6);
    Blk2Mcr(445, 0);
    Blk2Mcr(451, 1);
    Blk2Mcr(454, 9);
    Blk2Mcr(454, 11);
    Blk2Mcr(455, 8);
    Blk2Mcr(455, 10);

    Blk2Mcr(22, 0);
    Blk2Mcr(22, 5);
    Blk2Mcr(28, 0);
    Blk2Mcr(28, 1);
    Blk2Mcr(29, 0);
    Blk2Mcr(29, 1);
    Blk2Mcr(39, 0);
    Blk2Mcr(39, 1);
    Blk2Mcr(40, 0);
    Blk2Mcr(40, 1);
    Blk2Mcr(40, 4);
    Blk2Mcr(43, 0);
    Blk2Mcr(43, 5);
    Blk2Mcr(44, 0);
    Blk2Mcr(44, 1);
    Blk2Mcr(61, 2);
    Blk2Mcr(61, 4);
    Blk2Mcr(63, 3);
    Blk2Mcr(63, 5);
    Blk2Mcr(72, 3);
    Blk2Mcr(72, 5);
    Blk2Mcr(80, 3);
    Blk2Mcr(80, 5);
    Blk2Mcr(81, 2);
    Blk2Mcr(81, 4);
    Blk2Mcr(148, 1);
    Blk2Mcr(173, 1);
    Blk2Mcr(178, 1);
    Blk2Mcr(178, 3);
    Blk2Mcr(178, 5);
    Blk2Mcr(240, 0);
    Blk2Mcr(240, 1);
    Blk2Mcr(245, 0);
    Blk2Mcr(245, 1);
    Blk2Mcr(249, 0);
    Blk2Mcr(249, 1);
    Blk2Mcr(249, 7);
    Blk2Mcr(250, 0);
    Blk2Mcr(250, 1);
    Blk2Mcr(250, 6);
    Blk2Mcr(259, 1);

    const int unusedSubtiles[] = {
        14, 20, 69, 73, 75, 84, 85, 87, 89, 102, 103, 104, 105, 108, 115, 116, 117, 121, 122, 123, 125, 128, 138, 143, 144, 165, 166, 168, 170, 174, 175, 182, 191, 195, 199, 203, 207, 211, 215, 219, 222, 223, 224, 231, 234, 236, 237, 243, 256, 261, 267, 270, 276, 279, 282, 286, 291, 298, 303, 304, 308, 310, 312, 314, 316, 317, 331, 338, 352, 388, 408, 412, 414, 421, 423, 424, 426, 428, 440, 441, 442, 443, 444, 446, 447, 448, 449, 450, 452, 453, 456
    };

    for (int n = 0; n < lengthof(unusedSubtiles); n++) {
        for (int i = 0; i < blockSize; i++) {
            Blk2Mcr(unusedSubtiles[n], i);
        }
    }
}

bool D1Tileset::patchNestFloor(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{   1 - 1, 4, D1CEL_FRAME_TYPE::Empty },              // merge subtiles (used to block subsequent calls)
/*  1 */{   3 - 1, 7, D1CEL_FRAME_TYPE::Empty },
/*  2 */{   3 - 1, 5, D1CEL_FRAME_TYPE::Empty },
/*  3 */{   4 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },

/*  4 */{ 296 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, // change types
/*  5 */{ 224 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  6 */{ 223 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  7 */{ 209 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },

/*  8 */{  61 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare }, // fix glitch
/*  9 */{ 195 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 10 */{ 195 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 11 */{ 203 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 12 */{ 221 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },

/* 13 */{  25 - 1, 0, D1CEL_FRAME_TYPE::Empty }, // fix shadows
/* 14 */{  25 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 15 */{  26 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 16 */{  26 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 17 */{  27 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 18 */{  27 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 19 */{  28 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 20 */{  28 - 1, 1, D1CEL_FRAME_TYPE::Empty },

/* 21 */{ 103 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 22 */{ 103 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 23 */{ 104 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 24 */{ 104 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 25 */{ 105 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 26 */{ 105 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 27 */{ 106 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 28 */{ 106 - 1, 1, D1CEL_FRAME_TYPE::Empty },

/* 29 */{ 261 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 30 */{ 274 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 31 */{ 287 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 32 */{ 288 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 33 */{ 251 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 34 */{ 272 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 35 */{ 271 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 36 */{ 269 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 37 */{ 302 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 38 */{ 302 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 39 */{ 311 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 40 */{ 310 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 41 */{ 314 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 42 */{ 319 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 43 */{ 320 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 44 */{ 320 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 45 */{ 334 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare }, // fix glitch
/* 46 */{ 217 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 47 */{ 207 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },

/* 48 */{ 24 - 1, 7, D1CEL_FRAME_TYPE::Empty },              // merge subtiles
/* 49 */{ 22 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 50 */{ 22 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 51 */{ 24 - 1, 6, D1CEL_FRAME_TYPE::Empty },
/* 52 */{ 23 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 53 */{ 23 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 54 */{ 16 - 1, 7, D1CEL_FRAME_TYPE::Empty },
/* 55 */{ 14 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 56 */{ 14 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
/* 57 */{  8 - 1, 6, D1CEL_FRAME_TYPE::Empty },
/* 58 */{  7 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 59 */{  7 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
    };
    constexpr unsigned blockSize = BLOCK_SIZE_L6;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;

        // move pixels to 4[6] from 1[4], 3[7], 3[5]
        if (i == 3) {
            const CelMicro &microSrc1 = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc3 = micros[2];
            std::pair<unsigned, D1GfxFrame *> mf3 = this->getFrame(microSrc3.subtileIndex, blockSize, microSrc3.microIndex);
            D1GfxFrame *frameSrc3 = mf3.second;
            // if (frameSrc3 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel1 = frameSrc1->getPixel(x, y); // 1[4]
                    D1GfxPixel pixel2 = D1GfxPixel::transparentPixel();
                    D1GfxPixel pixel3 = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel2 = frameSrc2->getPixel(x, y + MICRO_HEIGHT / 2); // 3[7]
                    } else {
                        pixel3 = frameSrc3->getPixel(x, y - MICRO_HEIGHT / 2); // 3[5]
                    }
                    if (!pixel1.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel1);
                    }
                    if (!pixel2.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel2);
                    }
                    if (!pixel3.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel3);
                    }
                }
            }
        }

        if (i == 8) { // 61[7] - fix glitch
            change |= frame->setPixel(7, 12, D1GfxPixel::colorPixel(62));
            change |= frame->setPixel(8, 12, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(8, 13, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(9, 13, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(10, 13, D1GfxPixel::colorPixel(61));
            change |= frame->setPixel(11, 13, D1GfxPixel::colorPixel(61));
            change |= frame->setPixel(8, 14, D1GfxPixel::colorPixel(61));
            change |= frame->setPixel(9, 14, D1GfxPixel::colorPixel(61));
            change |= frame->setPixel(10, 14, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(11, 14, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(12, 14, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(10, 15, D1GfxPixel::colorPixel(61));
            change |= frame->setPixel(11, 15, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(12, 15, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(8, 16, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(9, 16, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(10, 16, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(11, 16, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(12, 16, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(13, 16, D1GfxPixel::colorPixel(61));
            change |= frame->setPixel(6, 17, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(7, 17, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(8, 17, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(9, 17, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(10, 17, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(11, 17, D1GfxPixel::colorPixel(61));
            change |= frame->setPixel(12, 17, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(13, 17, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(6, 18, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(7, 18, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(8, 18, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(9, 18, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(10, 18, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(11, 18, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(12, 18, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(13, 18, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(7, 19, D1GfxPixel::colorPixel(31));
            change |= frame->setPixel(8, 19, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(9, 19, D1GfxPixel::colorPixel(62));
            change |= frame->setPixel(10, 19, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(11, 19, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(12, 19, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(13, 19, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(7, 20, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(8, 20, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(9, 20, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(10, 20, D1GfxPixel::colorPixel(57));
            change |= frame->setPixel(11, 20, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(12, 20, D1GfxPixel::colorPixel(57));
            change |= frame->setPixel(13, 20, D1GfxPixel::colorPixel(61));
            change |= frame->setPixel(7, 21, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(8, 21, D1GfxPixel::colorPixel(58));
            change |= frame->setPixel(9, 21, D1GfxPixel::colorPixel(57));
            change |= frame->setPixel(10, 21, D1GfxPixel::colorPixel(55));
            change |= frame->setPixel(11, 21, D1GfxPixel::colorPixel(51));
            change |= frame->setPixel(12, 21, D1GfxPixel::colorPixel(58));
            change |= frame->setPixel(13, 21, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(7, 22, D1GfxPixel::colorPixel(59));
            change |= frame->setPixel(8, 22, D1GfxPixel::colorPixel(56));
            change |= frame->setPixel(9, 22, D1GfxPixel::colorPixel(55));
            change |= frame->setPixel(10, 22, D1GfxPixel::colorPixel(70));
            change |= frame->setPixel(11, 22, D1GfxPixel::colorPixel(71));
            change |= frame->setPixel(12, 22, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(7, 23, D1GfxPixel::colorPixel(58));
            change |= frame->setPixel(8, 23, D1GfxPixel::colorPixel(58));
            change |= frame->setPixel(9, 23, D1GfxPixel::colorPixel(57));
            change |= frame->setPixel(10, 23, D1GfxPixel::colorPixel(72));
            change |= frame->setPixel(11, 23, D1GfxPixel::colorPixel(74));
            change |= frame->setPixel(12, 23, D1GfxPixel::colorPixel(60));
            change |= frame->setPixel(7, 24, D1GfxPixel::colorPixel(61));
            change |= frame->setPixel(8, 24, D1GfxPixel::colorPixel(76));
            change |= frame->setPixel(9, 24, D1GfxPixel::colorPixel(74));
            change |= frame->setPixel(10, 24, D1GfxPixel::colorPixel(26));
            change |= frame->setPixel(11, 24, D1GfxPixel::colorPixel(76));
            change |= frame->setPixel(12, 24, D1GfxPixel::colorPixel(61));
            change |= frame->setPixel(7, 25, D1GfxPixel::colorPixel(29));
            change |= frame->setPixel(8, 25, D1GfxPixel::colorPixel(79));
            change |= frame->setPixel(9, 25, D1GfxPixel::colorPixel(27));
            change |= frame->setPixel(10, 25, D1GfxPixel::colorPixel(28));
            change |= frame->setPixel(11, 25, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(7, 26, D1GfxPixel::colorPixel(62));
            change |= frame->setPixel(8, 26, D1GfxPixel::colorPixel(30));
            change |= frame->setPixel(9, 26, D1GfxPixel::colorPixel(30));
            change |= frame->setPixel(10, 26, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(7, 27, D1GfxPixel::colorPixel(62));
            change |= frame->setPixel(8, 27, D1GfxPixel::colorPixel(62));
            change |= frame->setPixel(9, 27, D1GfxPixel::colorPixel(62));

            change |= frame->setPixel(7, 28, D1GfxPixel::colorPixel(29));
            change |= frame->setPixel(8, 28, D1GfxPixel::colorPixel(62));
            change |= frame->setPixel(7, 29, D1GfxPixel::colorPixel(29));
            change |= frame->setPixel(8, 29, D1GfxPixel::colorPixel(62));
            change |= frame->setPixel(7, 30, D1GfxPixel::colorPixel(29));
            change |= frame->setPixel(7, 31, D1GfxPixel::colorPixel(29));
        }
        if (i == 9) { // 195[0] - fix glitch
            change |= frame->setPixel(30, 1, D1GfxPixel::colorPixel(88));
            change |= frame->setPixel(31, 1, D1GfxPixel::colorPixel(89));
            change |= frame->setPixel(28, 2, D1GfxPixel::colorPixel(91));
            change |= frame->setPixel(29, 2, D1GfxPixel::colorPixel(91));
            change |= frame->setPixel(30, 2, D1GfxPixel::colorPixel(93));
            change |= frame->setPixel(31, 2, D1GfxPixel::colorPixel(95));
        }
        if (i == 10) { // 195[1] - fix glitch
            change |= frame->setPixel(5, 4, D1GfxPixel::colorPixel(120));
            change |= frame->setPixel(6, 4, D1GfxPixel::colorPixel(84));
            change |= frame->setPixel(7, 4, D1GfxPixel::colorPixel(85));
            change |= frame->setPixel(21, 3, D1GfxPixel::transparentPixel());
        }
        if (i == 11) { // 203[0] - fix glitch
            change |= frame->setPixel(18, 7, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(19, 7, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(20, 6, D1GfxPixel::colorPixel(127));
        }
        if (i == 12) { // 221[0] - fix glitch
            change |= frame->setPixel(14, 9, D1GfxPixel::colorPixel(106));
        }
        if (i == 45) { // 334[0] - fix glitch
            change |= frame->setPixel(17, 8, D1GfxPixel::colorPixel(90));
        }
        if (i == 46) { // 217[0] - fix glitch + shadow
            change |= frame->setPixel(14, 9, D1GfxPixel::colorPixel(125));

            change |= frame->setPixel(15, 9, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(16, 8, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(16, 9, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(12, 10, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(13, 10, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(14, 10, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(12, 11, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(13, 11, D1GfxPixel::colorPixel(125));
        }
        if (i == 47) { // 207[0] - fix glitch
            change |= frame->setPixel(24, 4, D1GfxPixel::colorPixel(45));
        }

        // create 'new' shadow 261[1] using 106[1]
        if (i == 29) {
            const CelMicro &microSrc = micros[28];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }

            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    bool useShadow = false;
                    switch (y) {
                    case 11: useShadow = x >= 15 && x <= 18; break;
                    case 12: useShadow = x >= 12 && x <= 19; break;
                    case 13: useShadow = x >= 12 && x <= 20; break;
                    case 14: useShadow = x >= 13 && x <= 20; break;
                    case 15: useShadow = x >= 13 && x <= 20; break;
                    case 16: useShadow = x >= 14 && x <= 20; break;
                    case 17: useShadow = x >= 14 && x <= 20; break;
                    case 18: useShadow = x >= 15 && x <= 19; break;
                    case 19: useShadow = x >= 15 && x <= 19; break;
                    case 20: useShadow = x >= 16 && x <= 18; break;
                    case 21: useShadow = x >= 16 && x <= 18; break;
                    case 22: useShadow = x >= 17 && x <= 18; break;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 106[1]
                    if (useShadow) {
                        pixel = D1GfxPixel::colorPixel(125);
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // create 'new' shadow 274[1] using 104[1]
        if (i == 30) {
            const CelMicro &microSrc = micros[24];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    bool useShadow = false;
                    switch (y) {
                    case 12: useShadow = x >= 22 && x <= 23; break;
                    case 13: useShadow = x >= 21 && x <= 25; break;
                    case 14: useShadow = x >= 20 && x <= 25; break;
                    case 15: useShadow = x >= 18 && x <= 24; break;
                    case 16: useShadow = x >= 18 && x <= 24; break;
                    case 17: useShadow = x >= 17 && x <= 23; break;
                    case 18: useShadow = x >= 16 && x <= 23; break;
                    case 19: useShadow = x >= 16 && x <= 22 && (x < 18 || x > 20); break;
                    case 20: useShadow = x >= 15 && x <= 21 && (x < 17 || x > 19); break;
                    case 21: useShadow = x >= 14 && x <= 21 && (x < 16 || x > 18); break;
                    case 22: useShadow = x >= 14 && x <= 14; break;
                    case 23: useShadow = x >= 14 && x <= 14; break;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 104[1]
                    if (useShadow) {
                        pixel = D1GfxPixel::colorPixel(125);
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
            change |= frame->setPixel(11, 8, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(12, 8, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(10, 9, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(11, 9, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(9, 10, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(10, 10, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(10, 11, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(11, 10, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(9, 11, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(8, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(8, 13, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(7, 14, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(8, 14, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(6, 15, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(7, 15, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(5, 16, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(6, 16, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(5, 17, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(4, 18, D1GfxPixel::colorPixel(125));
        }
        // create 'new' shadow 287[0] using 104[0]
        if (i == 31) {
            const CelMicro &microSrc = micros[23];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    bool useShadow = false;
                    switch (y) {
                    case  9: useShadow = x >= 28 && x <= 29; break;
                    case 10: useShadow = x >= 27 && x <= 29; break;
                    case 11: useShadow = x >= 27 && x <= 28; break;
                    case 12: useShadow = x >= 28 && x <= 28; break;
                    case 13: useShadow = x >= 23 && x <= 24; break;
                    case 14: useShadow = x >= 22 && x <= 25; break;
                    case 15: useShadow = x >= 22 && x <= 24; break;
                    case 16: useShadow = x >= 21 && x <= 25; break;
                    case 17: useShadow = x >= 20 && x <= 25; break;
                    case 18: useShadow = x >= 20 && x <= 25; break;
                    case 19: useShadow = x >= 19 && x <= 23; break;
                    case 20: useShadow = x >= 18 && x <= 22; break;
                    case 21: useShadow = x >= 18 && x <= 23; break;
                    case 22: useShadow = x >= 18 && x <= 23; break;
                    case 23: useShadow = x >= 18 && x <= 23; break;
                    case 24: useShadow = x >= 21 && x <= 23; break;
                    case 25: useShadow = x >= 21 && x <= 23; break;
                    case 26: useShadow = x >= 20 && x <= 22; break;
                    case 27: useShadow = x >= 22 && x <= 22; break;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 104[0]
                    if (useShadow) {
                        pixel = D1GfxPixel::colorPixel(125);
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // create 'new' shadow 288[1] using 106[1]
        if (i == 32) {
            const CelMicro &microSrc = micros[28];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 106[1]
                    change |= frame->setPixel(x, y, pixel);
                }
            }
            change |= frame->setPixel(17, 20, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(17, 21, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(18, 19, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(18, 18, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(18, 16, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(18, 14, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(19, 17, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(19, 16, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(19, 15, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(19, 14, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(19, 13, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(19, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(20, 15, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(20, 14, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(20, 13, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(20, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(20, 11, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(21, 13, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(21, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(21, 11, D1GfxPixel::colorPixel(125));
        }
        // create 'new' shadow 251[1] using 104[1]
        if (i == 33) {
            const CelMicro &microSrc = micros[24];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    bool useShadow = false;
                    switch (y) {
                    case 10: useShadow = x >= 4 && x <= 17; break;
                    case 11: useShadow = x >= 4 && x <= 19; break;
                    case 12: useShadow = x >= 2 && x <= 19; break;
                    case 13: useShadow = x >= 1 && x <= 20; break;
                    case 14: useShadow = x >= 1 && x <= 21; break;
                    case 15: useShadow = x >= 0 && x <= 22; break;
                    case 16: useShadow = x >= 0 && x <= 15; break;
                    case 17: useShadow = x >= 1 && x <= 10; break;
                    case 18: useShadow = x >= 3 && x <= 7; break;
                    case 19: useShadow = x >= 3 && x <= 5; break;
                    case 20: useShadow = x >= 3 && x <= 5; break;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 104[1]
                    if (useShadow) {
                        pixel = D1GfxPixel::colorPixel(125);
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // create 'new' shadow 272[1] using 28[1]
        if (i == 34) {
            const CelMicro &microSrc = micros[20];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 28[1]
                    change |= frame->setPixel(x, y, pixel);
                }
            }
            change |= frame->setPixel(10, 21, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(11, 21, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(9, 22, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(10, 22, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(11, 22, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(9, 23, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(10, 23, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(11, 23, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(9, 24, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(10, 24, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(8, 25, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(9, 25, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(10, 25, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(8, 26, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(9, 26, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(7, 27, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(8, 27, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(9, 27, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(7, 28, D1GfxPixel::colorPixel(125));
        }
        // create 'new' shadow 271[1] using 26[1]
        if (i == 35) {
            const CelMicro &microSrc = micros[16];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    bool useShadow = false;
                    switch (y) {
                    case  5: useShadow = x >= 8 && x <= 9; break;
                    case  6: useShadow = x >= 7 && x <= 11; break;
                    case  7: useShadow = x >= 5 && x <= 13; break;
                    case  8: useShadow = x >= 4 && x <= 15; break;
                    case  9: useShadow = x >= 3 && x <= 17; break;
                    case 10: useShadow = x >= 3 && x <= 16; break;
                    case 11: useShadow = x >= 2 && x <= 16; break;
                    case 12: useShadow = x >= 2 && x <= 15; break;
                    case 13: useShadow = x >= 2 && x <= 15; break;
                    case 14: useShadow = x >= 2 && x <= 13; break;
                    case 15: useShadow = x >= 2 && x <= 11; break;
                    case 16: useShadow = x >= 3 && x <= 11; break;
                    case 17: useShadow = x >= 3 && x <= 11; break;
                    case 18: useShadow = x >= 3 && x <= 10; break;
                    case 19: useShadow = x >= 4 && x <= 9; break;
                    case 20: useShadow = x >= 4 && x <= 7; break;
                    case 21: useShadow = x >= 5 && x <= 6; break;
                    case 22: useShadow = x >= 5 && x <= 5; break;
                    case 23: useShadow = x >= 5 && x <= 5; break;
                    case 24: useShadow = x >= 4 && x <= 5; break;
                    case 25: useShadow = x >= 4 && x <= 5; break;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 26[1]
                    if (useShadow) {
                        pixel = D1GfxPixel::colorPixel(125);
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // create 'new' shadow 269[0] using 104[0]
        if (i == 36) {
            const CelMicro &microSrc = micros[23];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 104[0]
                    change |= frame->setPixel(x, y, pixel);
                }
            }
            change |= frame->setPixel(10, 21, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(11, 21, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(12, 21, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(13, 21, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(15, 21, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(16, 21, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(17, 21, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(12, 22, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(13, 22, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(14, 22, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(15, 22, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(17, 22, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(14, 23, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(15, 23, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(16, 23, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(17, 23, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(16, 24, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(17, 24, D1GfxPixel::colorPixel(125));
        }
        // create 'new' shadow 302[0] using 104[0]
        if (i == 37) {
            const CelMicro &microSrc = micros[23];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    bool useShadow = false;
                    switch (y) {
                    case  3: useShadow = x >= 28 && x <= 31; break;
                    case  4: useShadow = x >= 25 && x <= 31; break;
                    case  5: useShadow = x >= 24 && x <= 31; break;
                    case  6: useShadow = x >= 21 && x <= 31; break;
                    case  7: useShadow = x >= 21 && x <= 31; break;
                    case  8: useShadow = x >= 22 && x <= 31; break;
                    case  9: useShadow = x >= 23 && x <= 31; break;
                    case 10: useShadow = x >= 24 && x <= 31; break;
                    case 11: useShadow = x >= 24 && x <= 31; break;
                    case 12: useShadow = x >= 24 && x <= 31; break;
                    case 13: useShadow = x >= 24 && x <= 31; break;
                    case 14: useShadow = x >= 25 && x <= 31; break;
                    case 15: useShadow = x >= 25 && x <= 31; break;
                    case 16: useShadow = x >= 26 && x <= 31; break;
                    case 17: useShadow = x >= 26 && x <= 31; break;
                    case 18: useShadow = x >= 27 && x <= 31; break;
                    case 19: useShadow = x >= 27 && x <= 31; break;
                    case 20: useShadow = x >= 28 && x <= 31; break;
                    case 21: useShadow = x >= 28 && x <= 31; break;
                    case 22: useShadow = x >= 29 && x <= 31; break;
                    case 23: useShadow = x >= 29 && x <= 31; break;
                    case 24: useShadow = x >= 29 && x <= 31; break;
                    case 25: useShadow = x >= 29 && x <= 31; break;
                    case 26: useShadow = x >= 28 && x <= 30; break;
                    case 27: useShadow = x >= 28 && x <= 29; break;
                    case 28: useShadow = x >= 28 && x <= 28; break;
                    case 29: useShadow = x >= 28 && x <= 28; break;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 104[0]
                    if (useShadow) {
                        pixel = D1GfxPixel::colorPixel(125);
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // create 'new' shadow 302[1] using 104[1]
        if (i == 38) {
            const CelMicro &microSrc = micros[24];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 104[1]
                    change |= frame->setPixel(x, y, pixel);
                }
            }
            change |= frame->setPixel(1, 4, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(2, 4, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(3, 4, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 5, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 5, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(2, 5, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(3, 5, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(4, 5, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 6, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 6, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(2, 6, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(3, 6, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 7, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 7, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 8, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 8, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 9, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 10, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 11, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 11, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(2, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(3, 12, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 13, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 13, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(2, 13, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(3, 13, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 14, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 14, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(2, 14, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 15, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 15, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(2, 15, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 16, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 16, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(2, 16, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 17, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 17, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(2, 17, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 18, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 18, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 19, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 19, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 20, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 20, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 21, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 22, D1GfxPixel::colorPixel(125));
        }
        // create 'new' shadow 311[1] using 106[1]
        if (i == 39) {
            const CelMicro &microSrc = micros[28];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    bool useShadow = false;
                    switch (y) {
                    case 14: useShadow = x >= 10 && x <= 12; break;
                    case 15: useShadow = x >= 9 && x <= 15; break;
                    case 16: useShadow = x >= 9 && x <= 17; break;
                    case 17: useShadow = x >= 9 && x <= 20; break;
                    case 18: useShadow = x >= 11 && x <= 19; break;
                    case 19: useShadow = x >= 15 && x <= 19; break;
                    case 20: useShadow = x >= 17 && x <= 19; break;
                    case 21: useShadow = x >= 17 && x <= 18; break;
                    case 22: useShadow = x >= 16 && x <= 17; break;
                    case 23: useShadow = x >= 15 && x <= 15; break;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 106[1]
                    if (useShadow) {
                        pixel = D1GfxPixel::colorPixel(125);
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // create 'new' shadow 310[1] using 104[1]
        if (i == 40) {
            const CelMicro &microSrc = micros[24];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    bool useShadow = false;
                    switch (y) {
                    case 13: useShadow = x >= 24 && x <= 25; break;
                    case 14: useShadow = x >= 25 && x <= 27; break;
                    case 15: useShadow = x >= 25 && x <= 29; break;
                    case 16: useShadow = x >= 25 && x <= 31; break;
                    case 17: useShadow = x >= 26 && x <= 29; break;
                    case 18: useShadow = x >= 26 && x <= 27; break;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 104[1]
                    if (useShadow) {
                        pixel = D1GfxPixel::colorPixel(125);
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // create 'new' shadow 314[0] using 28[0]
        if (i == 41) {
            const CelMicro &microSrc = micros[19];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    bool useShadow = false;
                    switch (y) {
                    case 18: useShadow = x >= 20 && x <= 28; break;
                    case 19: useShadow = x >= 20 && x <= 29; break;
                    case 20: useShadow = x >= 21 && x <= 29; break;
                    case 21: useShadow = x >= 21 && x <= 30; break;
                    case 22: useShadow = x >= 21 && x <= 31; break;
                    case 23: useShadow = x >= 22 && x <= 31; break;
                    case 24: useShadow = x >= 22 && x <= 31; break;
                    case 25: useShadow = x >= 23 && x <= 31; break;
                    case 26: useShadow = x >= 23 && x <= 31; break;
                    case 27: useShadow = x >= 23 && x <= 31; break;
                    case 28: useShadow = x >= 24 && x <= 31; break;
                    case 29: useShadow = x >= 26 && x <= 31; break;
                    case 30: useShadow = x >= 28 && x <= 31; break;
                    case 31: useShadow = x >= 30 && x <= 31; break;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 28[0]
                    if (useShadow) {
                        pixel = D1GfxPixel::colorPixel(125);
                    }
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }
        // fix shadow 319[1] using 25[1]
        if (i == 42) {
            const CelMicro &microSrc = micros[14];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 25[1]
                    if (x > 14 && y > 13 && y < 22) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // fix shadow 320[0] using 26[0]
        if (i == 43) {
            const CelMicro &microSrc = micros[15];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 26[0]
                    if (x > 19 && y < 13) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // fix shadow 320[1] using 26[1]
        if (i == 44) {
            const CelMicro &microSrc = micros[16];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 26[1]
                    change |= frame->setPixel(x, y, pixel);
                }
            }
            change |= frame->setPixel(0, 16, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 17, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 18, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 19, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 20, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 21, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(0, 22, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 16, D1GfxPixel::colorPixel(125));
            change |= frame->setPixel(1, 17, D1GfxPixel::colorPixel(125));
        }

        // move pixels to 22[4] from 24[7]
        if (i == 49) {
            const CelMicro &microSrc = micros[48];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y + MICRO_HEIGHT / 2); // 24[7]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 22[6] from 24[7]
        if (i == 50) {
            const CelMicro &microSrc = micros[48];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 24[7]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        // move pixels to 23[5] from 24[6]
        if (i == 52) {
            const CelMicro &microSrc = micros[51];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y + MICRO_HEIGHT / 2); // 24[6]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 23[7] from 24[6]
        if (i == 53) {
            const CelMicro &microSrc = micros[51];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 24[6]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        // move pixels to 23[5] from 16[7]
        if (i == 55) {
            const CelMicro &microSrc = micros[54];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y + MICRO_HEIGHT / 2); // 16[7]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 23[7] from 16[7]
        if (i == 56) {
            const CelMicro &microSrc = micros[54];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 16[7]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        // move pixels to 7[5] from 8[6]
        if (i == 58) {
            const CelMicro &microSrc = micros[57];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y + MICRO_HEIGHT / 2); // 8[6]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 7[7] from 8[6]
        if (i == 59) {
            const CelMicro &microSrc = micros[57];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 8[6]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

bool D1Tileset::patchNestStairs(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{ 75 - 1, 2, D1CEL_FRAME_TYPE::Empty },              // redraw stairs (used to block subsequent calls)
/*  1 */{ 75 - 1, 4, D1CEL_FRAME_TYPE::Empty },

/*  2 */{ 66 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  3 */{ 66 - 1, 1, D1CEL_FRAME_TYPE::RightTrapezoid },
/*  4 */{ 66 - 1, 3, D1CEL_FRAME_TYPE::Square },
/*  5 */{ 68 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  6 */{ 69 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  7 */{ 69 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/*  8 */{ 69 - 1, 5, D1CEL_FRAME_TYPE::Square },
/*  9 */{ 69 - 1, 7, D1CEL_FRAME_TYPE::TransparentSquare },
/* 10 */{ 71 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 11 */{ 71 - 1, 2, D1CEL_FRAME_TYPE::Empty },

/* 12 */{ 64 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 13 */{ 63 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 14 */{ 58 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 15 */{ 58 - 1, 3, D1CEL_FRAME_TYPE::Empty },
    };
    constexpr unsigned blockSize = BLOCK_SIZE_L6;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;

        // move pixels to 66[0] from 71[2]
        if (i == 2) {
            const CelMicro &microSrc = micros[11];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    if (y > 16 + x / 2 || y < 16 - x / 2) {
                        continue;
                    }
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 71[2]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        // move pixels to 66[1] from 69[0] and 69[2]
        if (i == 3) {
            const CelMicro &microSrc1 = micros[7];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[6];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y > 31 - x / 2) {
                        continue;
                    }
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 69[2]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 69[0]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 66[3] from 69[2]
        if (i == 4) {
            const CelMicro &microSrc = micros[7];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 69[2]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 68[1] from 71[0] and 71[2]
        if (i == 5) {
            const CelMicro &microSrc1 = micros[11];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[10];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    if (y > 31 - x / 2 || y < 1 + x / 2) {
                        continue;
                    }
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 71[2]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 71[0]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 69[5] from 75[4] and 75[2]
        if (i == 8) {
            const CelMicro &microSrc1 = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 75[4]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 75[2]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 69[7] from 75[4]
        if (i == 9) {
            const CelMicro &microSrc = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH / 2; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 75[4]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        // move pixels to 64[0] from 58[1] and 58[3]
        if (i == 12) {
            const CelMicro &microSrc1 = micros[15];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[14];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 58[3]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 58[1]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }
        // move pixels to 63[1] from 58[3]
        if (i == 13) {
            const CelMicro &microSrc = micros[15];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 58[3]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

bool D1Tileset::patchNestWall1(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{   49 - 1, 1, D1CEL_FRAME_TYPE::Empty },               // mask walls leading to north east
/*  1 */{   49 - 1, 3, D1CEL_FRAME_TYPE::Empty },
/*  2 */{   50 - 1, 3, D1CEL_FRAME_TYPE::Empty },
/*  3 */{   50 - 1, 5, D1CEL_FRAME_TYPE::Empty },
/*  4 */{   50 - 1, 7, D1CEL_FRAME_TYPE::Empty },

/*  5 */{   11 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  6 */{   11 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*  7 */{   11 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/*  8 */{   15 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  9 */{   15 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 10 */{   -1, 2, D1CEL_FRAME_TYPE::Empty },

/* 11 */{    9 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 12 */{   13 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 13 */{ /*17*/ - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 14 */{    9 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
    };
    constexpr unsigned blockSize = BLOCK_SIZE_L6;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;

        // mask 11[0], 15[0] using 49[1] and 49[3]
        if (i >= 5 && i < 11 && (i % 3) == (5 % 3)) {
            const CelMicro &microSrc1 = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 49[3]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 49[1]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 11[1], 15[1] using 50[3]
        if (i >= 6 && i < 11 && (i % 3) == (6 % 3)) {
            const CelMicro &microSrc = micros[2];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 50[3]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 11[2], 15[2] using 49[3]
        if (i >= 7 && i < 11 && (i % 3) == (7 % 3)) {
            const CelMicro &microSrc = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 49[3]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // mask 9[0], 13[0], 17[0] using 50[3] and 50[5]
        if (i >= 11 && i < 14) {
            const CelMicro &microSrc1 = micros[3];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[2];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 50[5]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 50[3]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 9[2] using 50[5] and 50[7]
        if (i == 14) {
            const CelMicro &microSrc1 = micros[4];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[3];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 50[7]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 50[5]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

bool D1Tileset::patchNestWall2(bool silent)
{
    const CelMicro micros[] = {
/*  0 */{  67 - 1, 0, D1CEL_FRAME_TYPE::Empty },               // mask walls leading to north west
/*  1 */{  67 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/*  2 */{  38 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  3 */{  40 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*  4 */{  42 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  5 */{  44 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*  6 */{  50 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  7 */{  52 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/*  8 */{  70 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/*  9 */{  72 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 10 */{ 140 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 11 */{ 142 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 12 */{ 144 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 13 */{ 146 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 14 */{ 148 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 15 */{ 150 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 16 */{ 152 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 17 */{ 154 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 18 */{ 156 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 19 */{ 158 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 20 */{ 160 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 21 */{ 162 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 22 */{ 164 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 23 */{ 166 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 24 */{ 176 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 25 */{ 178 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 26 */{ 180 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 27 */{ 182 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 28 */{ 184 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 29 */{ 186 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 30 */{ 188 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 31 */{ 190 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 32 */{ 502 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 33 */{ 504 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
//                
/* 34 */{  65 - 1, 0, D1CEL_FRAME_TYPE::Empty },
/* 35 */{  65 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 36 */{  65 - 1, 4, D1CEL_FRAME_TYPE::Empty },
/* 37 */{  65 - 1, 6, D1CEL_FRAME_TYPE::Empty },
/* 38 */{  38 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 39 */{  38 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 40 */{  38 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 41 */{  42 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 42 */{  42 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 43 */{  42 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 44 */{  50 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 45 */{  50 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 46 */{  50 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 47 */{  70 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 48 */{  70 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 49 */{  70 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 50 */{ 140 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 51 */{ 140 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 52 */{ 140 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 53 */{ 144 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 54 */{ 144 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 55 */{ 144 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 56 */{ 148 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 57 */{ 148 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 58 */{ 148 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 59 */{ 152 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 60 */{ 152 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 61 */{ 152 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 62 */{ 156 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 63 */{ 156 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 64 */{ 156 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 65 */{ 160 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 66 */{ 160 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 67 */{ 160 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 68 */{ 164 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 69 */{ 164 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 70 */{ 164 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 71 */{ 176 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 72 */{ 176 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 73 */{ 176 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 74 */{ 180 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 75 */{ 180 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 76 */{ 180 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 77 */{ 184 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 78 */{ 184 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 79 */{ 184 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 80 */{ 188 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 81 */{ 188 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 82 */{ 188 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 83 */{ /*502*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 84 */{ /*502*/ - 1, 3, D1CEL_FRAME_TYPE::Empty },
/* 85 */{ /*502*/ - 1, 5, D1CEL_FRAME_TYPE::Empty },

/* 86 */{  53 - 1, 0, D1CEL_FRAME_TYPE::Empty },               // mask walls leading to north east
/* 87 */{  53 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 88 */{  55 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 89 */{  55 - 1, 4, D1CEL_FRAME_TYPE::Empty },
/* 90 */{  55 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare }, // fix glitch

/* 91 */{   2 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 92 */{   2 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 93 */{   6 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 94 */{   6 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },

/* 95 */{   1 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 96 */{   5 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 97 */{ /*17*/ - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 98 */{   1 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },

/* 99 */{  37 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare }, // fix micros after masking
/*100 */{ 159 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },

/*101 */{   4 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare }, // move pixels to enable fix
    };
    constexpr unsigned blockSize = BLOCK_SIZE_L6;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return false;
        }
        bool change = false;

        // mask walls leading to north west
        // mask 38[0], 42[0], 50[0], 70[0], 140[0], 144[0], 148[0], 152[0], 156[0], 160[0], 164[0], 176[0], 180[0], 184[0], 188[0], 502[0] using 67[1]
        if (i >= 2 && i < 34 && (i % 2) == (2 % 2)) {
            const CelMicro &microSrc = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 67[1]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 40[1], 44[1], 52[1], 72[1], 142[1], 146[1], 150[1], 154[1], 158[1], 162[1], 166[1], 178[1], 182[1], 186[1], 190[1], 504[1] using 67[1] and 67[0]
        if (i >= 3 && i < 34 && (i % 2) == (3 % 2)) {
            const CelMicro &microSrc1 = micros[1];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[0];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 67[1]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 67[0]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 38[1], 42[1], 50[1], 70[1], 140[1], 144[1], 148[1], 152[1], 156[1], 160[1], 164[1], 176[1], 180[1], 184[1], 188[1], 502[1] using 65[0] and 65[2]
        if (i >= 38 && i < 86 && (i % 3) == (38 % 3)) {
            const CelMicro &microSrc1 = micros[35];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[34];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 65[2]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 65[0]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 38[3], 42[3], 50[3], 70[3], 140[3], 144[3], 148[3], 152[3], 156[3], 160[3], 164[3], 176[3], 180[3], 184[3], 188[3], 502[3] using 65[2] and 65[4]
        if (i >= 39 && i < 86 && (i % 3) == (39 % 3)) {
            const CelMicro &microSrc1 = micros[36];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[35];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 65[4]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 65[2]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 38[5], 42[5], 50[5], 70[5], 140[5], 144[5], 148[5], 152[5], 156[5], 160[5], 164[5], 176[5], 180[5], 184[5], 188[5], 502[5] using 65[4] and 65[6]
        if (i >= 40 && i < 86 && (i % 3) == (40 % 3)) {
            const CelMicro &microSrc1 = micros[37];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[36];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 65[6]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 65[4]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // 55[6] - fix glitch (prepare mask)
        if (i == 90) {
            change |= frame->setPixel(20, 25, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(20, 26, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(20, 27, D1GfxPixel::colorPixel(61));
            change |= frame->setPixel(20, 28, D1GfxPixel::colorPixel(61));
        }
        // mask 2[0], 6[0] with 55[2]
        if (i >= 91 && i < 95 && (i % 2) == (91 % 2)) {
            const CelMicro &microSrc = micros[88];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y - MICRO_HEIGHT / 2); // 55[2]
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 2[1], 6[1] with 53[0] and 53[2]
        if (i >= 92 && i < 95 && (i % 2) == (92 % 2)) {
            const CelMicro &microSrc1 = micros[87];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[86];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 53[2]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 53[0]
                    }

                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 1[1], 5[1], 17[1] with 55[4] and 55[2]
        if (i >= 95 && i < 98) {
            const CelMicro &microSrc1 = micros[89];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[88];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 55[4]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 55[2]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask 1[3] with 55[6] and 55[4]
        if (i == 98) {
            const CelMicro &microSrc1 = micros[90];
            std::pair<unsigned, D1GfxFrame *> mf1 = this->getFrame(microSrc1.subtileIndex, blockSize, microSrc1.microIndex);
            D1GfxFrame *frameSrc1 = mf1.second;
            // if (frameSrc1 == nullptr) {
            //    return;
            // }
            const CelMicro &microSrc2 = micros[89];
            std::pair<unsigned, D1GfxFrame *> mf2 = this->getFrame(microSrc2.subtileIndex, blockSize, microSrc2.microIndex);
            D1GfxFrame *frameSrc2 = mf2.second;
            // if (frameSrc2 == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                    if (y < MICRO_HEIGHT / 2) {
                        pixel = frameSrc1->getPixel(x, y + MICRO_HEIGHT / 2); // 55[6]
                    } else {
                        pixel = frameSrc2->getPixel(x, y - MICRO_HEIGHT / 2); // 55[4]
                    }
                    if (!pixel.isTransparent()) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }

        // fix micros after masking
        if (i >= 99 && i < 101) {
            change |= frame->setPixel(26, 25, D1GfxPixel::colorPixel(30));
            change |= frame->setPixel(26, 26, D1GfxPixel::colorPixel(31));
            change |= frame->setPixel(25, 24, D1GfxPixel::colorPixel(31));
            change |= frame->setPixel(25, 25, D1GfxPixel::colorPixel(31));
            change |= frame->setPixel(25, 26, D1GfxPixel::colorPixel(31));
            change |= frame->setPixel(25, 27, D1GfxPixel::colorPixel(31));
            change |= frame->setPixel(25, 28, D1GfxPixel::colorPixel(31));
            change |= frame->setPixel(24, 26, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(24, 27, D1GfxPixel::colorPixel(62));
            change |= frame->setPixel(24, 28, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(24, 29, D1GfxPixel::colorPixel(46));
        }

        // move pixels to 4[5] from 1[3]
        if (i == 101) {
            const CelMicro &microSrc = micros[98];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            // if (frameSrc == nullptr) {
            //    return;
            // }
            for (int x = 0; x < MICRO_WIDTH; x++) {
                for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
                    D1GfxPixel pixel = frameSrc->getPixel(x, y); // 1[3]
                    if (!pixel.isTransparent()) {
                        change |= frameSrc->setPixel(x, y, D1GfxPixel::transparentPixel()); // source is changed!
                        change |= frame->setPixel(x, y, pixel);
                    }
                }
            }
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }

    return true;
}

void D1Tileset::cleanupNest(std::set<unsigned> &deletedFrames, bool silent)
{
    constexpr int blockSize = BLOCK_SIZE_L6;

    // separate subtiles for the automap
    ReplaceSubtile(this->til, 20 - 1, 3, 87 - 1, silent);
    ReplaceSubtile(this->til, 23 - 1, 0, 29 - 1, silent);
    ReplaceSubtile(this->til, 23 - 1, 2, 31 - 1, silent);
    ReplaceSubtile(this->til, 23 - 1, 3, 89 - 1, silent);

    // use common subtiles
    ReplaceSubtile(this->til, 5 - 1, 1, 6 - 1, silent);     // 18
    ReplaceSubtile(this->til, 5 - 1, 2, 15 - 1, silent);    // 19
    ReplaceSubtile(this->til, 121 - 1, 0, 446 - 1, silent); // 457

    // use common subtiles instead of minor alterations
    ReplaceSubtile(this->til, 19 - 1, 1, 30 - 1, silent);  // 74
    // ReplaceSubtile(this->til, 20 - 1, 2, 31 - 1, silent);  // 78
    ReplaceSubtile(this->til, 26 - 1, 1, 30 - 1, silent);  // 95
    ReplaceSubtile(this->til, 27 - 1, 1, 30 - 1, silent);  // 98
    ReplaceSubtile(this->til, 27 - 1, 2, 31 - 1, silent);  // 99
    ReplaceSubtile(this->til, 28 - 1, 1, 30 - 1, silent);  // 101
    ReplaceSubtile(this->til, 28 - 1, 2, 96 - 1, silent);  // 102
    // - shadows
    ReplaceSubtile(this->til, 66 - 1, 2, 105 - 1, silent); // 252
    ReplaceSubtile(this->til, 72 - 1, 2, 105 - 1, silent);
    // ReplaceSubtile(this->til, 74 - 1, 2, 105 - 1, silent);
    ReplaceSubtile(this->til, 76 - 1, 2, 105 - 1, silent);
    ReplaceSubtile(this->til, 82 - 1, 2, 105 - 1, silent);
    ReplaceSubtile(this->til, 99 - 1, 2, 105 - 1, silent);
    ReplaceSubtile(this->til, 80 - 1, 2, 105 - 1, silent); // 303
    // ReplaceSubtile(this->til, 79 - 1, 0, 25 - 1, silent);  // 297
    // ReplaceSubtile(this->til, 79 - 1, 1, 26 - 1, silent);  // 298
    // ReplaceSubtile(this->til, 79 - 1, 2, 27 - 1, silent);  // 299
    ReplaceSubtile(this->til, 70 - 1, 0, 103 - 1, silent); // 266
    ReplaceSubtile(this->til, 70 - 1, 1, 104 - 1, silent); // 267
    ReplaceSubtile(this->til, 71 - 1, 2, 27 - 1, silent);  // 264
    ReplaceSubtile(this->til, 85 - 1, 2, 27 - 1, silent);
    // ReplaceSubtile(this->til, 77 - 1, 0, 25 - 1, silent);  // 289
    // ReplaceSubtile(this->til, 77 - 1, 1, 26 - 1, silent);  // 290
    // ReplaceSubtile(this->til, 77 - 1, 3, 28 - 1, silent);  // 292
    // ReplaceSubtile(this->til, 73 - 1, 0, 25 - 1, silent);  // 276
    // ReplaceSubtile(this->til, 73 - 1, 1, 26 - 1, silent);  // 277
    // ReplaceSubtile(this->til, 67 - 1, 0, 25 - 1, silent); // 254
    // ReplaceSubtile(this->til, 67 - 1, 1, 26 - 1, silent); // 255
    // ReplaceSubtile(this->til, 75 - 1, 0, 25 - 1, silent);  // 262
    ReplaceSubtile(this->til, 83 - 1, 0, 25 - 1, silent);
    // ReplaceSubtile(this->til, 75 - 1, 1, 26 - 1, silent);  // 283
    ReplaceSubtile(this->til, 83 - 1, 1, 26 - 1, silent);  // 312
    // ReplaceSubtile(this->til, 84 - 1, 0, 103 - 1, silent); // 315
    // ReplaceSubtile(this->til, 84 - 1, 1, 104 - 1, silent); // 316
    // ReplaceSubtile(this->til, 84 - 1, 2, 105 - 1, silent); // 317
    ReplaceSubtile(this->til, 100 - 1, 0, 25 - 1, silent); // 374
    ReplaceSubtile(this->til, 100 - 1, 1, 26 - 1, silent); // 375
    // - adjusted shadows
    ReplaceSubtile(this->til, 66 - 1, 0, 103 - 1, silent); // 250
    ReplaceSubtile(this->til, 66 - 1, 3, 106 - 1, silent); // 253
    ReplaceSubtile(this->til, 67 - 1, 0, 25 - 1, silent);  // 254 same as tile 7, but needed for L6MITE-placements
    ReplaceSubtile(this->til, 67 - 1, 1, 26 - 1, silent);  // 255
    ReplaceSubtile(this->til, 67 - 1, 2, 27 - 1, silent);  // 256
    ReplaceSubtile(this->til, 67 - 1, 3, 28 - 1, silent);  // 257
    ReplaceSubtile(this->til, 68 - 1, 0, 103 - 1, silent); // 258
    ReplaceSubtile(this->til, 68 - 1, 1, 104 - 1, silent); // 259
    ReplaceSubtile(this->til, 68 - 1, 2, 105 - 1, silent); // 260
    ReplaceSubtile(this->til, 70 - 1, 2, 105 - 1, silent); // 268
    ReplaceSubtile(this->til, 71 - 1, 0, 25 - 1, silent);  // 270
    ReplaceSubtile(this->til, 72 - 1, 0, 103 - 1, silent); // 273
    ReplaceSubtile(this->til, 72 - 1, 3, 106 - 1, silent); // 275
    ReplaceSubtile(this->til, 76 - 1, 0, 103 - 1, silent); // 286
    ReplaceSubtile(this->til, 80 - 1, 0, 103 - 1, silent); // 301
    ReplaceSubtile(this->til, 80 - 1, 3, 106 - 1, silent); // 304
    ReplaceSubtile(this->til, 83 - 1, 2, 27 - 1, silent);  // 313
    // - one-subtile islands
    ReplaceSubtile(this->til, 115 - 1, 2, 455 - 1, silent); // 436

    // eliminate subtiles of unused tiles
    const int unusedTiles[] = {
        21, 22, 24, 30, 31, 32, 61, 62, 63, 64, 65, 69, 73, 74, 75, 77, 79, 81, 84, 86, 87, 88, 90, 91, 92, 93, 94, 95, 96, 97, 98, 101, 102, 103, 118, 125, 127, 128, 129, 130, 132, 133, 134, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166
    };
    constexpr int blankSubtile = 18;
    for (int n = 0; n < lengthof(unusedTiles); n++) {
        int tileId = unusedTiles[n];
        ReplaceSubtile(this->til, tileId - 1, 0, blankSubtile - 1, silent);
        ReplaceSubtile(this->til, tileId - 1, 1, blankSubtile - 1, silent);
        ReplaceSubtile(this->til, tileId - 1, 2, blankSubtile - 1, silent);
        ReplaceSubtile(this->til, tileId - 1, 3, blankSubtile - 1, silent);
    }

    patchNestFloor(silent);
    patchNestStairs(silent);
    patchNestWall1(silent);
    patchNestWall2(silent);

    // useless black micros
    Blk2Mcr(21, 0);
    Blk2Mcr(21, 1);
    // useless pixels
    Blk2Mcr(334, 2);
    // -  by patchNestStairsCel
    Blk2Mcr(75, 2);
    ReplaceMcr(75, 4, 31, 4);
    Blk2Mcr(71, 2);
    Blk2Mcr(69, 2);
    Blk2Mcr(69, 4);
    Blk2Mcr(58, 3);

    // fix glitch
    ReplaceMcr(155, 6, 159, 6);
    ReplaceMcr(175, 6, 159, 6);
    ReplaceMcr(14, 6, 22, 6);
    ReplaceMcr(14, 7, 22, 7);
    // - by patchNestWall2Cel
    ReplaceMcr(8, 5, 4, 5);

    // new shadows
    ReplaceMcr(261, 0, 106, 0);
    ReplaceMcr(287, 1, 104, 1);
    ReplaceMcr(294, 1, 104, 1);
    ReplaceMcr(251, 0, 104, 0);
    ReplaceMcr(274, 0, 104, 0);
    ReplaceMcr(271, 0, 26, 0);

    // fix bad artifacts
    Blk2Mcr(132, 7);
    // Blk2Mcr(366, 1);

    // redraw corner tile
    ReplaceMcr(1, 6, 100, 6);
    ReplaceMcr(4, 4, 29, 2);
    ReplaceMcr(12, 4, 29, 2);
    ReplaceMcr(12, 5, 29, 3);
    Blk2Mcr(12, 7);
    ReplaceMcr(9, 5, 29, 5);
    ReplaceMcr(9, 7, 29, 7);
    // - after patchNestFloorCel
    Blk2Mcr(1, 4);
    Blk2Mcr(3, 7);
    Blk2Mcr(3, 5);
    Blk2Mcr(24, 7);
    Blk2Mcr(24, 6);
    Blk2Mcr(16, 7);
    Blk2Mcr(8, 6);
    // - separate subtiles for the automap
    Blk2Mcr(10, 4);
    Blk2Mcr(10, 6);
    ReplaceMcr(10, 5, 30, 5);
    ReplaceMcr(10, 7, 30, 7);
    ReplaceMcr(74, 5, 30, 5);
    ReplaceMcr(74, 7, 30, 7);
    HideMcr(89, 5);
    Blk2Mcr(89, 7);
    ReplaceMcr(78, 4, 31, 4);
    ReplaceMcr(78, 6, 31, 6);
    Blk2Mcr(87, 2);
    Blk2Mcr(87, 3);
    Blk2Mcr(87, 4);
    Blk2Mcr(87, 5);
    Blk2Mcr(87, 6);
    Blk2Mcr(87, 7);

    ReplaceMcr(17, 0, 13, 0); // mostly invisible
    ReplaceMcr(17, 1, 5, 1);

    ReplaceMcr(431, 1, 26, 1);
    ReplaceMcr(474, 1, 26, 1);

    ReplaceMcr(501, 2, 179, 2); // lost details
    ReplaceMcr(501, 4, 179, 4);
    ReplaceMcr(501, 5, 179, 5);
    ReplaceMcr(501, 6, 179, 6);
    ReplaceMcr(501, 7, 29, 7);

    ReplaceMcr(38, 7, 176, 7); // lost details
    ReplaceMcr(42, 7, 176, 7); // lost details
    ReplaceMcr(70, 7, 176, 7); // lost details
    ReplaceMcr(140, 7, 176, 7); // lost details
    ReplaceMcr(144, 7, 176, 7); // lost details
    ReplaceMcr(148, 7, 176, 7); // lost details
    ReplaceMcr(152, 7, 176, 7); // lost details
    ReplaceMcr(156, 7, 176, 7); // lost details
    ReplaceMcr(160, 7, 176, 7); // lost details
    ReplaceMcr(180, 7, 176, 7);
    ReplaceMcr(184, 7, 176, 7); // lost details
    ReplaceMcr(188, 7, 176, 7); // lost details
    ReplaceMcr(502, 7, 176, 7); // lost details

    ReplaceMcr(41, 6, 29, 6);
    ReplaceMcr(41, 7, 29, 7);
    ReplaceMcr(139, 6, 29, 6);
    ReplaceMcr(139, 7, 29, 7);
    ReplaceMcr(143, 7, 29, 7);
    ReplaceMcr(147, 7, 29, 7);
    ReplaceMcr(151, 7, 29, 7);
    ReplaceMcr(155, 7, 29, 7);
    ReplaceMcr(159, 7, 29, 7);
    ReplaceMcr(163, 6, 29, 6);
    ReplaceMcr(163, 7, 29, 7);
    ReplaceMcr(175, 7, 29, 7);
    ReplaceMcr(179, 7, 29, 7);
    ReplaceMcr(183, 6, 29, 6);
    ReplaceMcr(183, 7, 29, 7);
    ReplaceMcr(187, 6, 29, 6);
    ReplaceMcr(187, 7, 29, 7);

    ReplaceMcr(167, 6, 131, 6); // lost details
    ReplaceMcr(131, 7, 45, 7);
    ReplaceMcr(167, 7, 45, 7);

    ReplaceMcr(171, 7, 119, 7);

    ReplaceMcr(127, 6, 57, 6);
    ReplaceMcr(127, 7, 57, 7);
    ReplaceMcr(129, 6, 59, 6);

    ReplaceMcr(189, 6, 173, 6);

    ReplaceMcr(3, 4, 31, 4);
    ReplaceMcr(3, 6, 31, 6);
    ReplaceMcr(7, 4, 31, 4);

    ReplaceMcr(8, 4, 29, 2);
    // ReplaceMcr(8, 5, 29, 3);

    // ReplaceMcr(14, 4, 29, 2);
    ReplaceMcr(14, 5, 29, 3);
    ReplaceMcr(16, 5, 30, 5);
    // ReplaceMcr(16, 7, 30, 7);

    // ReplaceMcr(22, 4, 16, 4);
    ReplaceMcr(22, 5, 30, 5);
    ReplaceMcr(23, 4, 29, 2);
    ReplaceMcr(24, 4, 29, 2);
    ReplaceMcr(24, 5, 29, 3);

    ReplaceMcr(73, 5, 29, 5); // lost details
    ReplaceMcr(73, 4, 29, 4); // lost details
    ReplaceMcr(73, 3, 29, 3);
    ReplaceMcr(73, 2, 29, 2);

    ReplaceMcr(76, 6, 100, 6); // lost details
    // ReplaceMcr(78, 4, 31, 4);

    ReplaceMcr(93, 4, 31, 4);
    ReplaceMcr(91, 2, 29, 2); // lost details
    ReplaceMcr(91, 3, 29, 3);
    ReplaceMcr(92, 5, 30, 5);

    ReplaceMcr(96, 4, 31, 4);
    ReplaceMcr(94, 2, 29, 2); // lost details
    ReplaceMcr(94, 3, 29, 3);
    ReplaceMcr(94, 7, 29, 7); // lost details

    ReplaceMcr(97, 2, 29, 2);
    ReplaceMcr(97, 3, 29, 3);
    ReplaceMcr(97, 6, 29, 6); // lost details
    ReplaceMcr(97, 7, 29, 7); // lost details

    ReplaceMcr(100, 2, 29, 2);

    ReplaceMcr(260, 0, 105, 0);
    ReplaceMcr(295, 0, 105, 0);
    // ReplaceMcr(303, 0, 105, 0);
    // ReplaceMcr(317, 0, 105, 0);
    ReplaceMcr(412, 0, 105, 0);

    ReplaceMcr(253, 0, 106, 0);
    ReplaceMcr(275, 0, 106, 0);
    ReplaceMcr(282, 0, 106, 0);
    ReplaceMcr(288, 0, 106, 0);
    ReplaceMcr(304, 0, 106, 0);
    ReplaceMcr(311, 0, 106, 0);
    ReplaceMcr(324, 0, 106, 0);
    ReplaceMcr(373, 0, 106, 0);

    ReplaceMcr(321, 0, 28, 0);
    // ReplaceMcr(385, 0, 28, 0);

    ReplaceMcr(257, 1, 28, 1);
    ReplaceMcr(269, 1, 106, 1);
    ReplaceMcr(279, 1, 28, 1);
    ReplaceMcr(285, 1, 28, 1);
    // ReplaceMcr(302, 1, 104, 1);
    ReplaceMcr(314, 1, 28, 1);
    ReplaceMcr(377, 1, 28, 1);

    ReplaceMcr(250, 0, 103, 0);
    // ReplaceMcr(266, 0, 103, 0);
    ReplaceMcr(273, 0, 103, 0);
    ReplaceMcr(280, 0, 103, 0);
    ReplaceMcr(309, 0, 103, 0);
    ReplaceMcr(322, 0, 103, 0);

    // ReplaceMcr(262, 0, 25, 0);
    ReplaceMcr(270, 0, 25, 0);
    // ReplaceMcr(276, 0, 25, 0);

    ReplaceMcr(435, 1, 104, 1);

    ReplaceMcr(497, 4, 119, 4);
    ReplaceMcr(497, 6, 119, 6);
    ReplaceMcr(497, 7, 119, 7);
    ReplaceMcr(498, 3, 120, 3);
    ReplaceMcr(499, 0, 121, 0);
    ReplaceMcr(499, 2, 121, 2);
    ReplaceMcr(499, 4, 121, 4);
    ReplaceMcr(499, 6, 121, 6);

    // ReplaceMcr(501, 7, 29, 7);
    ReplaceMcr(502, 1, 152, 1);
    ReplaceMcr(502, 3, 152, 3);
    ReplaceMcr(502, 5, 180, 5);
    ReplaceMcr(503, 2, 153, 2);
    ReplaceMcr(503, 6, 181, 6);

    // - one-subtile islands
    ReplaceMcr(419, 0, 447, 0);

    // eliminate micros of unused subtiles
    // Blk2Mcr(560,  ...),
    Blk2Mcr(79, 0);
    Blk2Mcr(79, 1);
    Blk2Mcr(79, 2);
    Blk2Mcr(79, 3);
    Blk2Mcr(79, 4);
    Blk2Mcr(79, 5);
    Blk2Mcr(79, 7);
    Blk2Mcr(88, 4);
    Blk2Mcr(88, 5);
    Blk2Mcr(88, 6);
    Blk2Mcr(88, 7);
    Blk2Mcr(95, 7);
    Blk2Mcr(99, 6);
    Blk2Mcr(250, 1);
    Blk2Mcr(253, 1);
    Blk2Mcr(257, 0);
    Blk2Mcr(260, 1);
    Blk2Mcr(270, 1);
    Blk2Mcr(273, 1);
    Blk2Mcr(275, 1);
    Blk2Mcr(279, 0);
    Blk2Mcr(280, 1);
    Blk2Mcr(282, 1);
    Blk2Mcr(285, 0);
    Blk2Mcr(304, 1);
    Blk2Mcr(322, 1);
    Blk2Mcr(324, 1);
    Blk2Mcr(370, 6);
    Blk2Mcr(436, 1);
    Blk2Mcr(457, 1);
    Blk2Mcr(472, 0);
    Blk2Mcr(479, 1);
    Blk2Mcr(481, 1);
    Blk2Mcr(482, 0);
    Blk2Mcr(485, 1);
    Blk2Mcr(493, 0);
    Blk2Mcr(494, 1);
    Blk2Mcr(496, 1);
    Blk2Mcr(558, 6);
    Blk2Mcr(559, 1);
    Blk2Mcr(559, 2);
    Blk2Mcr(559, 3);
    Blk2Mcr(559, 4);
    Blk2Mcr(559, 5);
    Blk2Mcr(559, 6);
    Blk2Mcr(559, 7);
    Blk2Mcr(561, 4);
    Blk2Mcr(561, 6);
    Blk2Mcr(562, 0);
    Blk2Mcr(565, 6);
    Blk2Mcr(566, 4);
    Blk2Mcr(566, 5);
    Blk2Mcr(566, 6);
    Blk2Mcr(566, 7);
    Blk2Mcr(567, 5);
    Blk2Mcr(567, 7);
    Blk2Mcr(568, 0);
    Blk2Mcr(568, 4);
    Blk2Mcr(568, 6);
    Blk2Mcr(570, 2);
    Blk2Mcr(570, 3);
    Blk2Mcr(570, 4);
    Blk2Mcr(570, 5);
    Blk2Mcr(570, 6);
    Blk2Mcr(570, 7);
    Blk2Mcr(571, 3);
    Blk2Mcr(571, 5);
    Blk2Mcr(571, 7);
    Blk2Mcr(572, 1);
    Blk2Mcr(579, 1);
    Blk2Mcr(579, 2);
    Blk2Mcr(579, 4);
    Blk2Mcr(579, 6);
    Blk2Mcr(579, 7);
    Blk2Mcr(580, 2);
    Blk2Mcr(580, 4);
    Blk2Mcr(580, 6);
    Blk2Mcr(581, 0);
    Blk2Mcr(585, 2);
    Blk2Mcr(585, 3);
    Blk2Mcr(585, 4);
    Blk2Mcr(585, 5);
    Blk2Mcr(585, 6);
    Blk2Mcr(585, 7);
    Blk2Mcr(586, 5);
    Blk2Mcr(586, 7);
    Blk2Mcr(587, 6);
    Blk2Mcr(588, 0);
    Blk2Mcr(589, 2);
    Blk2Mcr(589, 3);
    Blk2Mcr(589, 4);
    Blk2Mcr(589, 5);
    Blk2Mcr(589, 6);
    Blk2Mcr(589, 7);
    Blk2Mcr(590, 5);
    Blk2Mcr(590, 7);
    Blk2Mcr(591, 0);
    Blk2Mcr(592, 1);
    Blk2Mcr(599, 2);
    Blk2Mcr(599, 3);
    Blk2Mcr(599, 4);
    Blk2Mcr(599, 5);
    Blk2Mcr(599, 7);

    Blk2Mcr(368, 3);
    Blk2Mcr(368, 4);
    Blk2Mcr(368, 5);
    Blk2Mcr(368, 6);
    Blk2Mcr(368, 7);
    Blk2Mcr(508, 0);
    Blk2Mcr(508, 3);
    Blk2Mcr(508, 4);
    Blk2Mcr(508, 5);
    Blk2Mcr(601, 3);
    Blk2Mcr(601, 4);
    Blk2Mcr(601, 5);
    Blk2Mcr(601, 6);
    Blk2Mcr(601, 7);
    Blk2Mcr(604, 3);
    Blk2Mcr(604, 4);
    Blk2Mcr(604, 5);
    Blk2Mcr(604, 6);
    Blk2Mcr(604, 7);

    const int unusedSubtiles[] = {
        18, 19, 80, 81, 82, 83, 84, 85, 86, 90, 98, 101, 102, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 252, 254, 255, 256, 258, 259, 262, 263, 264, 265, 266, 267, 268, 276, 277, 278, 281, 283, 284, 286, 289, 290, 291, 292, 297, 298, 299, 300, 301, 303, 305, 306, 307, 308, 312, 313, 315, 316, 317, 318, 323, 325, 326, 327, 328, 329, 330, 331, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 369, 374, 375, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389, 448, 449, 471, 477, 478, 480, 483, 484, 486, 487, 488, 492, 495, 505, 506, 507, 509, 510, 511, 512, 513, 514, 515, 516, 517, 518, 519, 520, 521, 522, 523, 524, 525, 526, 527, 528, 529, 530, 531, 532, 533, 534, 535, 536, 537, 538, 539, 540, 541, 542, 543, 544, 545, 546, 547, 548, 549, 550, 551, 552, 553, 554, 555, 556, 557, 563, 564, 569, 573, 574, 575, 576, 577, 578, 582, 583, 584, 593, 594, 595, 596, 597, 598, 600, 602, 603, 605, 606
    };

    for (int n = 0; n < lengthof(unusedSubtiles); n++) {
        for (int i = 0; i < blockSize; i++) {
            Blk2Mcr(unusedSubtiles[n], i);
        }
    }
}

void D1Tileset::patchCryptSpec(bool silent)
{
    typedef struct {
        int subtileIndex0;
        int8_t microIndices0[5];
        int subtileIndex1;
        int8_t microIndices1[5];
    } SCelFrame;
    const SCelFrame frames[] = {
/*  0 */{ 206 - 1, {-1,-1, 4,-1,-1 }, /*204*/ - 1, {-1,-1, 4, 6,-1 } },
/*  1 */{     - 1, { 0 },             /*208*/ - 1, {-1,-1, 5, 7,-1 } },
/*  2 */{  31 - 1, { 0, 2, 4, 6, 8 },  29 - 1, {-1,-1, 4, 6,-1 } },
/*  3 */{ 274 - 1, { 0, 2, 4, 6, 8 }, 272 - 1, {-1,-1, 4, 6,-1 } },
/*  4 */{ 558 - 1, { 0, 2, 4, 6, 8 }, 557 - 1, {-1,-1, 4, 6,-1 } },
/*  5 */{ 299 - 1, { 0, 2, 4, 6, 8 }, 298 - 1, {-1,-1, 4, 6, 8 } },
/*  6 */{ 301 - 1, {-1,-1, 4, 6, 8 }, 300 - 1, {-1,-1, 4, 6,-1 } },
/*  7 */{ 333 - 1, { 0, 2, 4, 6, 8 }, 331 - 1, {-1,-1, 4, 6,-1 } },
/*  8 */{ 356 - 1, { 0, 2, 4, 6, 8 }, 355 - 1, {-1,-1, 4, 6,-1 } },
/*  9 */{ 404 - 1, { 0, 2, 4, 6, 8 },  29 - 1, {-1,-1, 4, 6,-1 } },
/* 10 */{ 415 - 1, { 0, 2, 4, 6, 8 }, 413 - 1, {-1,-1, 4, 6,-1 } },
/* 11 */{ 456 - 1, { 0, 2, 4, 6, 8 }, 454 - 1, {-1,-1, 4, 6,-1 } },

/* 12 */{  18 - 1, { 1, 3, 5, 7, 9 },  17 - 1, {-1,-1, 5, 7,-1 } },
/* 13 */{ 459 - 1, { 1, 3, 5, 7, 9 }, 458 - 1, {-1,-1, 5, 7,-1 } },
/* 14 */{ 352 - 1, { 1, 3, 5, 7, 9 }, 351 - 1, {-1,-1, 5, 7,-1 } },
/* 15 */{ 348 - 1, { 1, 3, 5, 7, 9 }, 347 - 1, {-1,-1, 5, 7,-1 } },
/* 16 */{ 406 - 1, { 1, 3, 5, 7, 9 },  17 - 1, {-1,-1, 5, 7,-1 } },
/* 17 */{ 444 - 1, { 1, 3, 5, 7, 9 },  17 - 1, {-1,-1, 5, 7,-1 } },
/* 18 */{ 471 - 1, { 1, 3, 5, 7, 9 }, 470 - 1, {-1,-1, 5, 7,-1 } },
/* 19 */{ 562 - 1, { 1, 3, 5, 7, 9 }, 561 - 1, {-1,-1, 5, 7,-1 } },
    };

    const unsigned blockSize = BLOCK_SIZE_L5;
    constexpr int FRAME_WIDTH = 64;
    constexpr int FRAME_HEIGHT = 160;
    constexpr int BASE_CEL_ENTRIES = 2;

    if (this->cls->getFrameCount() != BASE_CEL_ENTRIES) {
        return; // assume it is already done
    }

    for (int i = 0; i < lengthof(frames); i++) {
        const SCelFrame &sframe = frames[i];

        bool change = false;
        D1GfxFrame *frame;
        if (i < BASE_CEL_ENTRIES) {
            frame = this->cls->getFrame(i);
            if (frame->getWidth() != FRAME_WIDTH || frame->getHeight() != FRAME_HEIGHT) {
                dProgressErr() << QApplication::tr("Framesize of the Crypt's Special-Cels does not match. (%1:%2 expected %3:%4. Index %5.)").arg(frame->getWidth()).arg(frame->getHeight()).arg(FRAME_WIDTH).arg(FRAME_HEIGHT).arg(i + 1);
                return;
            }
        } else {
            // TODO: move to d1gfx.cpp?
            frame = this->cls->insertFrame(i);
            for (int y = 0; y < FRAME_HEIGHT; y++) {
                std::vector<D1GfxPixel> pixelLine;
                for (int x = 0; x < FRAME_WIDTH; x++) {
                    pixelLine.push_back(D1GfxPixel::transparentPixel());
                }
                frame->addPixelLine(std::move(pixelLine));
            }
            change = true;
        }

        for (int n = 0; n < lengthof(sframe.microIndices0) && sframe.subtileIndex0 >= 0; n++) {
            if (sframe.microIndices0[n] < 0) {
                continue;
            }

            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(sframe.subtileIndex0, blockSize, sframe.microIndices0[n]);
            D1GfxFrame *frameSrc = mf.second;
            if (frameSrc == nullptr) {
                dProgressErr() << QApplication::tr("Missing 'template' subtile-frame (%1:%2) to create Crypt's Special-Cels.").arg(sframe.subtileIndex0 + 1).arg(sframe.microIndices0[n]);
                return;
            }
            if (frameSrc->getWidth() != MICRO_WIDTH || frameSrc->getHeight() != MICRO_HEIGHT) {
                dProgressErr() << QApplication::tr("Framesize of the 'template' subtile-frame does not fit to create Crypt's Special-Cels. (%1:%2 expected %3:%4. Index %5:%6.)").arg(frameSrc->getWidth()).arg(frameSrc->getHeight()).arg(MICRO_WIDTH).arg(MICRO_HEIGHT).arg(sframe.subtileIndex1 + 1).arg(sframe.microIndices1[n] + 1);
                return;
            }

            if (i < 12 && i != 1) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    for (int x = 0; x < MICRO_WIDTH; x++) {
                        if (i == 0) { // 206[4]
                            if (y > 14 - x / 2) {
                                continue;
                            }
                        }
                        if ((i == 2 || i == 3 || i == 4 || i == 5 || i == 7 || i == 8 || i == 10 || i == 11) && n == 0) { // 31[0] - mask floor with left column
                            if (x > 23) {
                                continue;
                            }
                            if (x == 23 && (y != 17 && y != 18)) {
                                continue;
                            }
                            if (x == 22 && (y < 16 || y > 19)) {
                                continue;
                            }
                            if (x == 21 && (y == 21 || y == 22)) {
                                continue;
                            }
                        }
                        if (i == 9 && n == 0) { // 404[0] - mask floor with left column
                            if (x > 23) {
                                continue;
                            }
                            if (x == 23 && y > 18) {
                                continue;
                            }
                            if (x == 22 && y > 19) {
                                continue;
                            }
                            if (x == 21 && (y == 21 || y == 22)) {
                                continue;
                            }
                        }
                        if (i == 6 && n == 2) { // 301[4] eliminate 'bad' pixels
                            if (y > 12 - (x + 1) / 2) {
                                continue;
                            }
                        }
                        if (i == 6 && n == 3) { // 301[6] eliminate 'bad' pixels
                            if (y > 44 - (x + 1) / 2) {
                                continue;
                            }
                        }
                        D1GfxPixel pixelSrc = frameSrc->getPixel(x, y);
                        if (!pixelSrc.isTransparent()) {
                            int yy = FRAME_HEIGHT - MICRO_HEIGHT * (n + 1) + y;
                            D1GfxPixel pixel = frame->getPixel(x, yy);
                            if (pixel.isTransparent()) {
                                change |= frame->setPixel(x, yy, pixelSrc);
                            }
                        }
                    }
                }
            } else { //i >= 12 || i == 1
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    for (int x = 0; x < MICRO_WIDTH; x++) {
                        if (i != 18 && i != 17 && i != 1 && n == 0) { // 18[1]
                            if (x < 8) {
                                continue;
                            }
                            if (x == 8 && (y != 18 && y != 19)) {
                                continue;
                            }
                            if (x == 9 && (y < 17 || y > 20)) {
                                continue;
                            }
                            if (x == 10 && (y == 22 || y == 23)) {
                                continue;
                            }
                        }

                        if (i == 17 && n == 0) { // 444[1] -- remove additional pixels due to brocken column
                            if (x < 10) {
                                continue;
                            }
                            if (x == 10 && (y < 17 || y > 19)) {
                                continue;
                            }
                            if (x == 11 && (y < 16 || y == 22 || y == 23 || y == 26)) {
                                continue;
                            }
                            if (x == 12 && y < 15) {
                                continue;
                            }
                            if (x == 13 && y < 13) {
                                continue;
                            }
                            if (x == 14 && y < 11) {
                                continue;
                            }
                            if (x == 15 && y < 10) {
                                continue;
                            }
                        }

                        if (i == 18 && n == 0) { // 471[1]
                            if (x < 5) {
                                continue;
                            }
                            if (x == 5 && (y < 14 || y > 17)) {
                                continue;
                            }
                            if (x == 6 && (y < 11 || y > 18)) {
                                continue;
                            }
                            if (x == 7 && y > 20) {
                                continue;
                            }
                            if (x == 8 && y > 22) {
                                continue;
                            }
                            if (x == 9 && y > 23) {
                                continue;
                            }
                        }

                        if (i == 17 && n == 1) { // 444[3] - remove pixels from the left side of the arch
                            if (x < 2) {
                                continue;
                            }
                        }
                        D1GfxPixel pixelSrc = frameSrc->getPixel(x, y);
                        quint8 color = pixelSrc.getPaletteIndex();
                        if (i == 19 && n == 0 && ((color < 16 && color != 0) /*|| (x == 8 && y == 18)*/)) {
                            continue; // remove bright lava-pixels from 562[1]
                        }
                        if (!pixelSrc.isTransparent()) {
                            int yy = FRAME_HEIGHT - MICRO_HEIGHT * (n + 1) + y;
                            D1GfxPixel pixel = frame->getPixel(x + MICRO_WIDTH, yy);
                            if (pixel.isTransparent()) {
                                change |= frame->setPixel(x + MICRO_WIDTH, yy, pixelSrc);
                            }
                        }
                    }
                }
            }
        }

        for (int n = 0; n < lengthof(sframe.microIndices1) && sframe.subtileIndex1 >= 0; n++) {
            if (sframe.microIndices1[n] < 0) {
                continue;
            }
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(sframe.subtileIndex1, blockSize, sframe.microIndices1[n]);
            D1GfxFrame *frameSrc = mf.second;
            if (frameSrc == nullptr) {
                dProgressErr() << QApplication::tr("Missing 'template' subtile-frame (%1:%2) to create Crypt's Special-Cels.").arg(sframe.subtileIndex1 + 1).arg(sframe.microIndices1[n] + 1);
                return;
            }
            if (frameSrc->getWidth() != MICRO_HEIGHT || frameSrc->getWidth() != MICRO_WIDTH) {
                dProgressErr() << QApplication::tr("Framesize of the 'template' subtile-frame does not fit to create Crypt's Special-Cels. (%1:%2 expected %3:%4. Index %5:%6.)").arg(frame->getWidth()).arg(frame->getHeight()).arg(FRAME_WIDTH).arg(FRAME_HEIGHT).arg(sframe.subtileIndex1 + 1).arg(sframe.microIndices1[n] + 1);
                return;
            }
            if (i < 12 && i != 1) {
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    for (int x = 0; x < MICRO_WIDTH; x++) {
                        /*if (i == 0 && n == 2) { // 204[4]
                            if (y > 14 - x / 2) {
                                continue;
                            }
                        }*/

                        if ((i == 2 || i == 3 || i == 4 || i == 7 || i == 8 || i == 9 || i == 11) && n == 2) { // 29[4]
                            if (y > 16) {
                                continue;
                            }
                            if (y > 25 - x) {
                                continue;
                            }
                            /*if (x > 18 && y > 24 - x) {
                                continue;
                            }*/
                        }
                        if ((i == 2 || i == 3 || i == 4 || i == 7 || i == 8 || i == 9 || i == 11) && n == 3) { // 29[6]
                            if (y > 57 - x) {
                                continue;
                            }
                            if (x > 27) {
                                continue;
                            }
                            if (y < x / 2 - 1) {
                                continue;
                            }
                            if (y < 5 - x / 2) {
                                continue;
                            }
                        }
                        if (i == 10 && n == 2) { // 413[4]
                            if (y > 10 - x) {
                                continue;
                            }
                        }
                        if (i == 10 && n == 3) { // 413[6]
                            if (x > 26) {
                                continue;
                            }
                            if (y > 45 - x) {
                                continue;
                            }
                            if ((x < 19 || x > 21) && y > 44 - x) {
                                continue;
                            }
                            if ((x < 16 || x > 22) && y > 43 - x) {
                                continue;
                            }
                            if (x < 14 && y > 42 - x) {
                                continue;
                            }
                            if (y < x / 2 - 1) {
                                continue;
                            }
                            if (y < 5 - x / 2) {
                                continue;
                            }
                        }
                        if (i == 6 && n == 2) { // 331[4]
                            if (y > 12 - (x + 1) / 2) {
                                continue;
                            }
                        }
                        if (i == 6 && n == 3) { // 331[6]
                            if (x == 27 && y == 31) {
                                continue;
                            }
                            if (x > 27) {
                                continue;
                            }
                            if (y < x / 2 - 1) {
                                continue;
                            }
                            if (y < 5 - x / 2) {
                                continue;
                            }
                        }

                        D1GfxPixel pixelSrc = frameSrc->getPixel(x, y);
                        quint8 color = pixelSrc.getPaletteIndex();
                        if ((i == 3 || i == 4) && n == 0 && ((color < 16 && color != 0) /*|| (x == 23 && y == 18)*/)) {
                            continue; // remove bright lava-pixels from 274[0], 558[0]
                        }
                        if (!pixelSrc.isTransparent()) {
                            int yy = FRAME_HEIGHT - MICRO_HEIGHT * (n + 1) + y;
                            D1GfxPixel pixel = frame->getPixel(x + MICRO_WIDTH, yy - 16);
                            if (pixel.isTransparent()) {
                                change |= frame->setPixel(x + MICRO_WIDTH, yy - 16, pixelSrc);
                            }

                        }
                    }
                }
            } else { // i >= 12 || i == 1
                for (int y = 0; y < MICRO_HEIGHT; y++) {
                    for (int x = 0; x < MICRO_WIDTH; x++) {
                        if (i != 1 && n == 2) { // 17[5]
                            if (y > 16) {
                                continue;
                            }
                            if (y > x - 5) {
                                continue;
                            }
                        }
                        if (i != 1 && n == 3) { // 17[7]
                            if (y > x + 27) {
                                continue;
                            }
                            if (x < 4) {
                                continue;
                            }
                            if (y < x / 2 - 10) {
                                continue;
                            }
                            if (y < 14 - x / 2) {
                                continue;
                            }
                        }

                        D1GfxPixel pixelSrc = frameSrc->getPixel(x, y);
                        if (!pixelSrc.isTransparent()) {
                            int yy = FRAME_HEIGHT - MICRO_HEIGHT * (n + 1) + y;
                            D1GfxPixel pixel = frame->getPixel(x, yy - 16);
                            if (pixel.isTransparent()) {
                                change |= frame->setPixel(x, yy - 16, pixelSrc);
                            }
                        }
                    }
                }
            }
        }

        if (i == 0) {
            change |= frame->setPixel(31, 63, D1GfxPixel::colorPixel(44));
            change |= frame->setPixel(22, 155, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(59, 45, D1GfxPixel::colorPixel(44));
            change |= frame->setPixel(58, 46, D1GfxPixel::colorPixel(42));
            change |= frame->setPixel(58, 47, D1GfxPixel::colorPixel(41));
        }

        if (i == 6) {
            change |= frame->setPixel(10, 32, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(12, 31, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(14, 30, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(16, 29, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(18, 28, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(20, 27, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(22, 26, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(24, 25, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(26, 24, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(28, 23, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(30, 22, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(32, 21, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(34, 20, D1GfxPixel::colorPixel(42));
        }
        if (change && !silent) {
            this->cls->setModified();
            dProgress() << QApplication::tr("Special-Frame %1 is modified.").arg(i + 1);
        }
    }
}

void D1Tileset::patchCryptFloor(bool silent)
{
    const CelMicro micros[] = {
        // clang-format off
/*  0 */ { 159 - 1, 3, D1CEL_FRAME_TYPE::Square },
/*  1 */ { 336 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  2 */ { 409 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  3 */ { 481 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  4 */ { 492 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  5 */ { 519 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  6 */ { 595 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  7 */ { 368 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  8 */ { 162 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/*  9 */ {  63 - 1, 4, D1CEL_FRAME_TYPE::Square },
/* 10 */ { 450 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },

/* 11 */{ 206 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },      // mask doors
/* 12 */{ 204 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 13 */{ 204 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 14 */{ 204 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 15 */{ 209 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 16 */{ 208 - 1, 1, D1CEL_FRAME_TYPE::TransparentSquare },
/* 17 */{ 208 - 1, 3, D1CEL_FRAME_TYPE::TransparentSquare },
/* 18 */{ 208 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/* 19 */{ 290 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 20 */{ 292 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 21 */{ 292 - 1, 2, D1CEL_FRAME_TYPE::TransparentSquare },
/* 22 */{ 292 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 23 */{ 294 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 24 */{ 296 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 25 */{ 296 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },

/* 26 */{ 274 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle }, // with the new special cels
/* 27 */{ 299 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 28 */{ 404 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 29 */{ 415 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 30 */{ 456 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/* 31 */{  18 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  *///{ 277 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle }, -- altered in fixCryptShadows
/* 32 */{ 444 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 33 */{ 471 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/* 34 */{  29 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 35 */{ 272 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 36 */{ 454 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 37 */{ 557 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 38 */{ 559 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 39 */{ 413 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 40 */{ 300 - 1, 4, D1CEL_FRAME_TYPE::TransparentSquare },
/* 41 */{  33 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 42 */{ 347 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 43 */{ 357 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 44 */{ 400 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 45 */{ 458 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 46 */{ 563 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },
/* 47 */{ 276 - 1, 5, D1CEL_FRAME_TYPE::TransparentSquare },

/* 48 */{ 258 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare }, // fix micros after reuse
/* 49 */{ 256 - 1, 6, D1CEL_FRAME_TYPE::TransparentSquare },
        // clang-format on
    };

    constexpr unsigned blockSize = BLOCK_SIZE_L5;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return;
        }
        bool change = false;
        if (i == 1) { // 336[0]
            change |= frame->setPixel(30, 1, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(31, 1, D1GfxPixel::colorPixel(76));
        }
        if (i == 5) { // 519[0]
            change |= frame->setPixel(0, 16, D1GfxPixel::colorPixel(43));
        }
        if (i == 7) { // 368[1]
            change |= frame->setPixel(0, 7, D1GfxPixel::colorPixel(43));
            change |= frame->setPixel(0, 9, D1GfxPixel::colorPixel(41));
        }
        if (i == 8) { // 162[2]
            change |= frame->setPixel(31, 13, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(31, 14, D1GfxPixel::colorPixel(36));
            change |= frame->setPixel(31, 18, D1GfxPixel::colorPixel(36));
            change |= frame->setPixel(30, 19, D1GfxPixel::colorPixel(35));
            change |= frame->setPixel(31, 20, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(31, 21, D1GfxPixel::colorPixel(63));
            change |= frame->setPixel(31, 22, D1GfxPixel::colorPixel(40));
            change |= frame->setPixel(31, 29, D1GfxPixel::colorPixel(36));
        }
        if (i == 9) { // 63[4]
            change |= frame->setPixel(0, 19, D1GfxPixel::colorPixel(91));
            change |= frame->setPixel(0, 20, D1GfxPixel::colorPixel(93));
        }
        if (i == 10) { // 450[0]
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

        // mask doors 204[0]
        if (i == 12) {
            for (int y = 0; y < MICRO_HEIGHT; y++) {
                for (int x = 0; x < 16; x++) {
                    if (x < 14 || (x == 14 && y > 4) || (x == 15 && y > 14 && y < 23)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask doors 204[2]
        if (i == 13) {
            for (int y = 0; y < MICRO_HEIGHT; y++) {
                for (int x = 0; x < 14; x++) {
                    if (x < 12 || (x == 12 && y > 12) || (x == 13 && y > 26)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask doors 204[4]
        if (i == 14) {
            for (int y = 0; y < MICRO_HEIGHT; y++) {
                for (int x = 0; x < 26; x++) {
                    if (x > 10 && y > 14 - x / 2) {
                        continue;
                    }
                    if (x == 10 && y > 9 && y < 17) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask doors 208[1]
        if (i == 16) {
            for (int y = 0; y < MICRO_HEIGHT; y++) {
                for (int x = 17; x < MICRO_WIDTH; x++) {
                    if (x > 17 || (x == 17 && y > 5 && y < 23)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask doors 208[3]
        if (i == 17) {
            for (int y = 0; y < MICRO_HEIGHT; y++) {
                for (int x = 18; x < MICRO_WIDTH; x++) {
                    if (x > 19 || (x == 19 && y > 0) || (x == 18 && y > 17)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
        }
        // mask doors 208[5]
        if (i == 18) {
            for (int y = 0; y < MICRO_HEIGHT; y++) {
                for (int x = 6; x < MICRO_WIDTH; x++) {
                    if (x < 20 && y > x / 2 - 1) {
                        continue;
                    }
                    if (x == 20 && y > 9 && y < 17) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask 'doors' 290[4]
        if (i == 19) {
            for (int y = 0; y < 13; y++) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    if (y > 13 - (x + 1) / 2) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask 'doors' 294[4], 296[4]
        if (i >= 23 && i < 25) {
            for (int y = 0; y < 13; y++) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    if (y > 12 - (x + 1) / 2) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask 'doors' 296[6]
        if (i == 25) {
            for (int y = 0; y < MICRO_HEIGHT; y++) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    if (y > 44 - (x + 1) / 2) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask 'doors' 292[0]
        if (i == 20) {
            for (int y = 0; y < MICRO_HEIGHT; y++) {
                for (int x = 0; x < 24; x++) {
                    if (x == 23 && (y < 17 || y > 18)) {
                        continue;
                    }
                    if (x == 22 && (y < 16 || y > 19)) {
                        continue;
                    }
                    if (x == 21 && (y == 21 || y == 22)) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask 'doors' 292[2]
        if (i == 21) {
            for (int y = 0; y < MICRO_HEIGHT; y++) {
                for (int x = 0; x < 26; x++) {
                    if (x == 25 && y > 7) {
                        continue;
                    }
                    if (x == 24 && y > 13) {
                        continue;
                    }
                    if (x == 23 && y > 20) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask 'doors' 292[4]
        if (i == 22) {
            for (int y = 0; y < MICRO_HEIGHT; y++) {
                for (int x = 0; x < 30; x++) {
                    if (x == 29 && (y < 14 || y > 17)) {
                        continue;
                    }
                    if (x == 28 && (y < 13 || y > 18)) {
                        continue;
                    }
                    if (x == 27 && ((y > 1 && y < 13) || y > 18)) {
                        continue;
                    }
                    if (x == 26 && ((y > 2 && y < 10) || y > 30)) {
                        continue;
                    }
                    if (x == 25 && (y > 3 && y < 6)) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask the new special cels 29[4], 272[4], 454[4], 557[4], 559[4]
        if (i >= 34 && i < 39) {
            for (int y = 0; y < 17; y++) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    if (y > 25 - x) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask the new special cels 413[4]
        if (i == 39) {
            for (int y = 0; y < 9; y++) {
                for (int x = 0; x < 11; x++) {
                    if (y > 10 - x) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask the new special cels 300[4]
        if (i == 40) {
            for (int y = 0; y < 14; y++) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    if (x > 3 && y > 12 - ((x + 1) / 2)) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // mask the new special cels 33[5]
        if (i >= 41 && i < 48) {
            for (int y = 0; y < 17; y++) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    if (y > x - 5) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        if (i == 11) { // 206[0]
            change |= frame->setPixel(26, 4, D1GfxPixel::colorPixel(45));
        }
        if (i == 38) { // 559[4] - fix bad artifact after masking
            change |= frame->setPixel(15, 11, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(14, 12, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(13, 13, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(12, 14, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(11, 15, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(10, 16, D1GfxPixel::transparentPixel());
        }
        if (i == 46) { // 563[5] - fix bad artifact after masking
            change |= frame->setPixel(17, 13, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(17, 14, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(18, 14, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(18, 15, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(19, 15, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(19, 16, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(20, 16, D1GfxPixel::transparentPixel());
        }
        if (i == 48) { // 258[6] - fix micros after reuse
            change |= frame->setPixel(3, 3, D1GfxPixel::colorPixel(42));
            change |= frame->setPixel(4, 3, D1GfxPixel::colorPixel(42));
            change |= frame->setPixel(6, 2, D1GfxPixel::colorPixel(42));
            change |= frame->setPixel(8, 1, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(10, 0, D1GfxPixel::colorPixel(44));
        }
        if (i == 49) { // 256[6] - fix micros after reuse
            change |= frame->setPixel(0, 5, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(1, 5, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(2, 4, D1GfxPixel::colorPixel(42));
            change |= frame->setPixel(3, 4, D1GfxPixel::colorPixel(41));
            change |= frame->setPixel(3, 0, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(3, 1, D1GfxPixel::colorPixel(45));
            change |= frame->setPixel(3, 2, D1GfxPixel::colorPixel(45));
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
            this->gfx->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of subtile %2 is modified.").arg(microFrame.first).arg(micro.subtileIndex + 1);
            }
        }
    }
}

void D1Tileset::maskCryptBlacks(bool silent)
{
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
    const CelMicro micros[] = {
        // clang-format off
/*  0 */{ 626 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  1 */{ 626 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  2 */{ /*627*/ - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  3 */{ 638 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  4 */{ 639 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  5 */{ 639 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  6 */{ /*631*/ - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/*  7 */{ 634 - 1, 0, D1CEL_FRAME_TYPE::LeftTriangle },
/*  8 */{ 634 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },

/*  9 */{ 277 - 1, 1, D1CEL_FRAME_TYPE::RightTriangle },
/* 10 */{ 303 - 1, 1, D1CEL_FRAME_TYPE::Empty },

/* 11 */{ 620 - 1, 0, D1CEL_FRAME_TYPE::RightTriangle },
/* 12 */{ 621 - 1, 1, D1CEL_FRAME_TYPE::Square },
/* 13 */{ 625 - 1, 0, D1CEL_FRAME_TYPE::RightTriangle },
/* 14 */{ 624 - 1, 0, D1CEL_FRAME_TYPE::TransparentSquare },
/* 15 */{  15 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 16 */{  15 - 1, 2, D1CEL_FRAME_TYPE::Empty },
/* 17 */{  89 - 1, 1, D1CEL_FRAME_TYPE::Empty },
/* 18 */{  89 - 1, 2, D1CEL_FRAME_TYPE::Empty },

/* 19 */{ 619 - 1, 1, D1CEL_FRAME_TYPE::LeftTrapezoid },
        // clang-format on
    };

    constexpr unsigned blockSize = BLOCK_SIZE_L5;
    const D1GfxPixel SHADOW_COLOR = D1GfxPixel::colorPixel(0); // 79;
    for (int i = 0; i < lengthof(micros); i++) {
        const CelMicro &micro = micros[i];
        if (micro.subtileIndex < 0) {
            continue;
        }
        std::pair<unsigned, D1GfxFrame *> microFrame = this->getFrame(micro.subtileIndex, blockSize, micro.microIndex);
        D1GfxFrame *frame = microFrame.second;
        if (frame == nullptr) {
            return;
        }
        bool change = false;
        // extend shadows of 626[0], 626[1], 638[1], 639[0], 639[1], 634[0], 634[1]
        if (i < 9) {
            for (int y = 0; y < MICRO_WIDTH; y++) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (pixel.isTransparent()) {
                        continue;
                    }
                    quint8 color = pixel.getPaletteIndex();
                    if (color != 79) {
                        // extend the shadows to NE: 626[0], 626[1]
                        if (i == 0 && y <= (x / 2) + 13 - MICRO_HEIGHT / 2) { // 626[0]
                            continue;
                        }
                        if (i == 1 && y <= (x / 2) + 13) { // 626[1]
                            continue;
                        }
                        // if (i == 2 && y <= (x / 2) + 13 - MICRO_HEIGHT / 2) { // 1808
                        //    continue;
                        // }
                        // extend the shadows to NW: 211[0][1], 211[1][0]
                        if (i == 3 && y <= 13 - (x / 2)) { // 638[1]
                            continue;
                        }
                        if (i == 4 && (x > 19 || y > 23) && (x != 16 || y != 24)) { // 639[0]
                            continue;
                        }
                        if (i == 5 && x <= 7) { // 639[1]
                            continue;
                        }
                        // extend the shadows to NW: 631[1]
                        // if (i == 6 && y <= (x / 2) + 15) { // 631[1]
                        //    continue;
                        // }
                        // extend the shadows to NE: 634[0], 634[1]
                        if (i == 7 && (y <= (x / 2) - 3 || (x >= 20 && y >= 14 && color >= 59 && color <= 95 && (color >= 77 || color <= 63)))) { // 634[0]
                            continue;
                        }
                        if (i == 8 && (y <= (x / 2) + 13 || (x <= 8 && y >= 12 && color >= 62 && color <= 95 && (color >= 80 || color <= 63)))) { // 634[1]
                            continue;
                        }
                    }
                    change |= frame->setPixel(x, y, SHADOW_COLOR);
                }
            }
        }

        //  use consistent lava + shadow micro II. 277[1]
        if (i == 9) {
            const CelMicro &microSrc = micros[10]; // 303[1]
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            if (frameSrc == nullptr) {
                return;
            }
            for (int y = 0; y < MICRO_WIDTH; y++) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    if (x > 11) {
                        continue;
                    }
                    if (x == 11 && (y < 9 || (y >= 17 && y <= 20))) {
                        continue;
                    }
                    if (x == 10 && (y == 18 || y == 19)) {
                        continue;
                    }
                    D1GfxPixel pixelSrc = frameSrc->getPixel(x, y); // 303[1]
                    if (pixelSrc.isTransparent()) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, pixelSrc);
                }
            }
        }

        // draw the new micros
        if (i >= 11 && i < 15) {
            const CelMicro &microSrc = micros[i + 4];
            std::pair<unsigned, D1GfxFrame *> mf = this->getFrame(microSrc.subtileIndex, blockSize, microSrc.microIndex);
            D1GfxFrame *frameSrc = mf.second;
            if (frameSrc == nullptr) {
                return;
            }
            for (int y = 0; y < MICRO_WIDTH; y++) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    D1GfxPixel srcPixel = frameSrc != nullptr ? frameSrc->getPixel(x, y) : SHADOW_COLOR;
                    if (i == 11 && !srcPixel.isTransparent()) { // 15[1] -> 620[0]
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
                            if (y > 14 + (x - 1) / 2) {
                                srcPixel = SHADOW_COLOR;
                            }
                        }
                    }
                    if (i == 13 && !srcPixel.isTransparent()) { // 89[1] -> 625[0]
                        // grate/floor in shadow
                        if (x <= 1 && y >= 7 * x) {
                            srcPixel = SHADOW_COLOR;
                        }
                        if (x > 1 && y > 14 + (x - 1) / 2) {
                            srcPixel = SHADOW_COLOR;
                        }
                    }
                    if (i == 12 || i == 14) { // 15[2], 89[2] -> 621[1], 624[0]
                        // wall/grate in shadow
                        if (y >= 7 * (x - 27) && !srcPixel.isTransparent()) {
                            srcPixel = SHADOW_COLOR;
                        }
                    }

                    change |= frame->setPixel(x, y, srcPixel);
                }
            }
        }
        // create shadow micro - 619[1]
        if (i == 19) {
            for (int y = 0; y < MICRO_WIDTH; y++) {
                for (int x = 0; x < MICRO_WIDTH; x++) {
                    D1GfxPixel pixel = y > 16 + x / 2 ? D1GfxPixel::transparentPixel() : SHADOW_COLOR;
                    change |= frame->setPixel(x, y, pixel);
                }
            }
        }

        // fix bad artifacts
        if (i == 7) { // 634[0]
            change |= frame->setPixel(22, 20, SHADOW_COLOR);
        }

        if (micro.res_encoding != D1CEL_FRAME_TYPE::Empty && frame->getFrameType() != micro.res_encoding) {
            change = true;
            frame->setFrameType(micro.res_encoding);
            std::vector<FramePixel> pixels;
            D1CelTilesetFrame::collectPixels(frame, micro.res_encoding, pixels);
            for (const FramePixel &pix : pixels) {
                D1GfxPixel resPix = pix.pixel.isTransparent() ? D1GfxPixel::colorPixel(0) : D1GfxPixel::transparentPixel();
                change |= frame->setPixel(pix.pos.x(), pix.pos.y(), resPix);
            }
        }
        if (change) {
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

    // fix automap of the entrance I.
    ReplaceSubtile(this->til, 53 - 1, 1, 148 - 1, silent);
    ReplaceSubtile(this->til, 53 - 1, 3, 148 - 1, silent);
    ReplaceSubtile(this->til, 54 - 1, 3, 148 - 1, silent);
    // fix 'bad' artifact
    ReplaceSubtile(this->til, 136 - 1, 1, 2 - 1, silent);   // 398
    // fix graphical glitch
    ReplaceSubtile(this->til, 6 - 1, 1, 2 - 1, silent);     // 22
    ReplaceSubtile(this->til, 134 - 1, 1, 2 - 1, silent);
    // use common subtiles
    ReplaceSubtile(this->til, 4 - 1, 1, 6 - 1, silent); // 14
    ReplaceSubtile(this->til, 14 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 115 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 132 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 1 - 1, 2, 15 - 1, silent); // 3
    ReplaceSubtile(this->til, 27 - 1, 2, 15 - 1, silent);
    ReplaceSubtile(this->til, 43 - 1, 2, 15 - 1, silent);
    // ReplaceSubtile(this->til, 79 - 1, 2, 15 - 1, silent);
    ReplaceSubtile(this->til, 6 - 1, 2, 15 - 1, silent);    // 23
    ReplaceSubtile(this->til, 7 - 1, 2, 7 - 1, silent);    // 27
    ReplaceSubtile(this->til, 9 - 1, 2, 7 - 1, silent);
    ReplaceSubtile(this->til, 33 - 1, 2, 7 - 1, silent);
    ReplaceSubtile(this->til, 137 - 1, 2, 7 - 1, silent);
    ReplaceSubtile(this->til, 106 - 1, 3, 48 - 1, silent);  // 304
    ReplaceSubtile(this->til, 108 - 1, 0, 45 - 1, silent);  // 309
    ReplaceSubtile(this->til, 127 - 1, 2, 4 - 1, silent);   // 372
    ReplaceSubtile(this->til, 132 - 1, 2, 15 - 1, silent);  // 388
    ReplaceSubtile(this->til, 156 - 1, 2, 31 - 1, silent);  // 468
    ReplaceSubtile(this->til, 179 - 1, 1, 261 - 1, silent); // 545
    ReplaceSubtile(this->til, 183 - 1, 1, 269 - 1, silent); // 554
    ReplaceSubtile(this->til, 205 - 1, 3, 626 - 1, silent); // 627
    // - doors
    ReplaceSubtile(this->til, 25 - 1, 0, 204 - 1, silent); // 75
    ReplaceSubtile(this->til, 26 - 1, 0, 208 - 1, silent); // (79)
    ReplaceSubtile(this->til, 69 - 1, 2, 76 - 1, silent);  // 206 - to make 76 'accessible'
    ReplaceSubtile(this->til, 70 - 1, 1, 79 - 1, silent);  // 209 - to make 79 'accessible'
    // ReplaceSubtile(this->til, 71 - 1, 2, 206 - 1, silent);
    // ReplaceSubtile(this->til, 72 - 1, 2, 206 - 1, silent);
    // use better subtiles
    // - increase glow
    ReplaceSubtile(this->til, 96 - 1, 3, 293 - 1, silent); // 279
    ReplaceSubtile(this->til, 187 - 1, 3, 293 - 1, silent);
    ReplaceSubtile(this->til, 188 - 1, 3, 293 - 1, silent);
    ReplaceSubtile(this->til, 90 - 1, 1, 297 - 1, silent); // 253
    ReplaceSubtile(this->til, 175 - 1, 1, 297 - 1, silent);
    // - reduce glow
    ReplaceSubtile(this->til, 162 - 1, 1, 297 - 1, silent); // 489
    ReplaceSubtile(this->til, 162 - 1, 2, 266 - 1, silent); // 490
    // create separate pillar tile
    ReplaceSubtile(this->til, 28 - 1, 0, 91 - 1, silent);
    ReplaceSubtile(this->til, 28 - 1, 1, 60 - 1, silent);
    ReplaceSubtile(this->til, 28 - 1, 2, 4 - 1, silent);
    ReplaceSubtile(this->til, 28 - 1, 3, 12 - 1, silent);
    // create the new shadows
    // - use the shadows created by fixCryptShadows
    ReplaceSubtile(this->til, 203 - 1, 0, 638 - 1, silent); // 619
    ReplaceSubtile(this->til, 203 - 1, 1, 639 - 1, silent); // 620
    ReplaceSubtile(this->til, 203 - 1, 2, 623 - 1, silent); // 47
    ReplaceSubtile(this->til, 203 - 1, 3, 626 - 1, silent); // 621
    ReplaceSubtile(this->til, 204 - 1, 0, 638 - 1, silent); // 622
    ReplaceSubtile(this->til, 204 - 1, 1, 639 - 1, silent); // 46
    ReplaceSubtile(this->til, 204 - 1, 2, 636 - 1, silent); // 623
    ReplaceSubtile(this->til, 204 - 1, 3, 626 - 1, silent); // 624
    ReplaceSubtile(this->til, 108 - 1, 2, 631 - 1, silent); // 810
    ReplaceSubtile(this->til, 108 - 1, 3, 626 - 1, silent); // 811
    ReplaceSubtile(this->til, 210 - 1, 3, 371 - 1, silent); // 637

    ReplaceSubtile(this->til, 109 - 1, 0, 1 - 1, silent);   // 312
    ReplaceSubtile(this->til, 109 - 1, 1, 2 - 1, silent);   // 313
    ReplaceSubtile(this->til, 109 - 1, 2, 3 - 1, silent);   // 314
    ReplaceSubtile(this->til, 109 - 1, 3, 626 - 1, silent); // 315
    ReplaceSubtile(this->til, 110 - 1, 0, 21 - 1, silent);  // 316
    ReplaceSubtile(this->til, 110 - 1, 1, 2 - 1, silent);   // 313
    ReplaceSubtile(this->til, 110 - 1, 2, 3 - 1, silent);   // 314
    ReplaceSubtile(this->til, 110 - 1, 3, 626 - 1, silent); // 315
    ReplaceSubtile(this->til, 111 - 1, 0, 39 - 1, silent);  // 317
    ReplaceSubtile(this->til, 111 - 1, 1, 4 - 1, silent);   // 318
    ReplaceSubtile(this->til, 111 - 1, 2, 242 - 1, silent); // 319
    ReplaceSubtile(this->til, 111 - 1, 3, 626 - 1, silent); // 320
    ReplaceSubtile(this->til, 215 - 1, 0, 101 - 1, silent); // 645
    ReplaceSubtile(this->til, 215 - 1, 1, 4 - 1, silent);   // 646
    ReplaceSubtile(this->til, 215 - 1, 2, 178 - 1, silent); // 45
    ReplaceSubtile(this->til, 215 - 1, 3, 626 - 1, silent); // 647
    // - 'add' new shadow-types with glow
    ReplaceSubtile(this->til, 216 - 1, 0, 39 - 1, silent);  // 622
    ReplaceSubtile(this->til, 216 - 1, 1, 4 - 1, silent);   // 46
    ReplaceSubtile(this->til, 216 - 1, 2, 238 - 1, silent); // 648
    ReplaceSubtile(this->til, 216 - 1, 3, 635 - 1, silent); // 624
    ReplaceSubtile(this->til, 217 - 1, 0, 638 - 1, silent); // 625
    ReplaceSubtile(this->til, 217 - 1, 1, 639 - 1, silent); // 46
    ReplaceSubtile(this->til, 217 - 1, 2, 634 - 1, silent); // 649
    ReplaceSubtile(this->til, 217 - 1, 3, 635 - 1, silent); // 650
    // - 'add' new shadow-types with horizontal arches
    ReplaceSubtile(this->til, 71 - 1, 0, 5 - 1, silent); // copy from tile 2
    ReplaceSubtile(this->til, 71 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 71 - 1, 2, 631 - 1, silent);
    ReplaceSubtile(this->til, 71 - 1, 3, 626 - 1, silent);
    ReplaceSubtile(this->til, 80 - 1, 0, 5 - 1, silent); // copy from tile 2
    ReplaceSubtile(this->til, 80 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 80 - 1, 2, 623 - 1, silent);
    ReplaceSubtile(this->til, 80 - 1, 3, 626 - 1, silent);

    ReplaceSubtile(this->til, 81 - 1, 0, 42 - 1, silent); // copy from tile 12
    ReplaceSubtile(this->til, 81 - 1, 1, 18 - 1, silent);
    ReplaceSubtile(this->til, 81 - 1, 2, 631 - 1, silent);
    ReplaceSubtile(this->til, 81 - 1, 3, 626 - 1, silent);
    ReplaceSubtile(this->til, 82 - 1, 0, 42 - 1, silent); // copy from tile 12
    ReplaceSubtile(this->til, 82 - 1, 1, 18 - 1, silent);
    ReplaceSubtile(this->til, 82 - 1, 2, 623 - 1, silent);
    ReplaceSubtile(this->til, 82 - 1, 3, 626 - 1, silent);

    ReplaceSubtile(this->til, 83 - 1, 0, 104 - 1, silent); // copy from tile 36
    ReplaceSubtile(this->til, 83 - 1, 1, 84 - 1, silent);
    ReplaceSubtile(this->til, 83 - 1, 2, 631 - 1, silent);
    ReplaceSubtile(this->til, 83 - 1, 3, 626 - 1, silent);
    ReplaceSubtile(this->til, 84 - 1, 0, 104 - 1, silent); // copy from tile 36
    ReplaceSubtile(this->til, 84 - 1, 1, 84 - 1, silent);
    ReplaceSubtile(this->til, 84 - 1, 2, 623 - 1, silent);
    ReplaceSubtile(this->til, 84 - 1, 3, 626 - 1, silent);

    ReplaceSubtile(this->til, 85 - 1, 0, 25 - 1, silent); // copy from tile 7
    ReplaceSubtile(this->til, 85 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 85 - 1, 2, 631 - 1, silent);
    ReplaceSubtile(this->til, 85 - 1, 3, 626 - 1, silent);
    ReplaceSubtile(this->til, 86 - 1, 0, 25 - 1, silent); // copy from tile 7
    ReplaceSubtile(this->til, 86 - 1, 1, 6 - 1, silent);
    ReplaceSubtile(this->til, 86 - 1, 2, 623 - 1, silent);
    ReplaceSubtile(this->til, 86 - 1, 3, 626 - 1, silent);

    ReplaceSubtile(this->til, 87 - 1, 0, 208 - 1, silent); // copy from tile 26
    ReplaceSubtile(this->til, 87 - 1, 1, 80 - 1, silent);
    ReplaceSubtile(this->til, 87 - 1, 2, 623 - 1, silent);
    ReplaceSubtile(this->til, 87 - 1, 3, 626 - 1, silent);
    ReplaceSubtile(this->til, 88 - 1, 0, 208 - 1, silent); // copy from tile 26
    ReplaceSubtile(this->til, 88 - 1, 1, 80 - 1, silent);
    ReplaceSubtile(this->til, 88 - 1, 2, 631 - 1, silent);
    ReplaceSubtile(this->til, 88 - 1, 3, 626 - 1, silent);

    // use common subtiles instead of minor alterations
    ReplaceSubtile(this->til, 7 - 1, 1, 6 - 1, silent);    // 26
    ReplaceSubtile(this->til, 159 - 1, 1, 6 - 1, silent);  // 479
    ReplaceSubtile(this->til, 133 - 1, 2, 31 - 1, silent); // 390
    ReplaceSubtile(this->til, 10 - 1, 1, 18 - 1, silent);  // 37
    ReplaceSubtile(this->til, 138 - 1, 1, 18 - 1, silent);
    ReplaceSubtile(this->til, 188 - 1, 1, 277 - 1, silent); // 564
    ReplaceSubtile(this->til, 178 - 1, 2, 258 - 1, silent); // 543
    ReplaceSubtile(this->til, 5 - 1, 2, 31 - 1, silent);    // 19
    ReplaceSubtile(this->til, 14 - 1, 2, 31 - 1, silent);
    ReplaceSubtile(this->til, 159 - 1, 2, 31 - 1, silent);
    ReplaceSubtile(this->til, 185 - 1, 2, 274 - 1, silent); // 558
    ReplaceSubtile(this->til, 186 - 1, 2, 274 - 1, silent); // 560
    ReplaceSubtile(this->til, 139 - 1, 0, 39 - 1, silent);  // 402
    ReplaceSubtile(this->til, 2 - 1, 3, 4 - 1, silent);  // 8
    ReplaceSubtile(this->til, 3 - 1, 1, 60 - 1, silent); // 10
    ReplaceSubtile(this->til, 114 - 1, 1, 7 - 1, silent);
    ReplaceSubtile(this->til, 3 - 1, 2, 4 - 1, silent); // 11
    ReplaceSubtile(this->til, 114 - 1, 2, 4 - 1, silent);
    ReplaceSubtile(this->til, 5 - 1, 3, 7 - 1, silent); // 20
    ReplaceSubtile(this->til, 14 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 133 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 125 - 1, 3, 7 - 1, silent); // 50
    ReplaceSubtile(this->til, 159 - 1, 3, 7 - 1, silent);
    ReplaceSubtile(this->til, 4 - 1, 3, 7 - 1, silent); // 16
    ReplaceSubtile(this->til, 132 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 9 - 1, 1, 18 - 1, silent);    // 34
    ReplaceSubtile(this->til, 12 - 1, 1, 18 - 1, silent);
    ReplaceSubtile(this->til, 38 - 1, 1, 18 - 1, silent);
    ReplaceSubtile(this->til, 137 - 1, 1, 18 - 1, silent);
    ReplaceSubtile(this->til, 10 - 1, 3, 7 - 1, silent); // 38
    ReplaceSubtile(this->til, 138 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 11 - 1, 3, 4 - 1, silent); // 41
    ReplaceSubtile(this->til, 122 - 1, 3, 7 - 1, silent);
    ReplaceSubtile(this->til, 121 - 1, 3, 4 - 1, silent); // 354
    ReplaceSubtile(this->til, 8 - 1, 3, 4 - 1, silent);   // 32
    ReplaceSubtile(this->til, 136 - 1, 3, 7 - 1, silent);
    ReplaceSubtile(this->til, 91 - 1, 1, 47 - 1, silent); // 257
    ReplaceSubtile(this->til, 178 - 1, 1, 47 - 1, silent);
    ReplaceSubtile(this->til, 91 - 1, 3, 48 - 1, silent); // 259
    ReplaceSubtile(this->til, 177 - 1, 3, 7 - 1, silent);
    ReplaceSubtile(this->til, 178 - 1, 3, 48 - 1, silent);
    ReplaceSubtile(this->til, 95 - 1, 3, 4 - 1, silent); // 275
    ReplaceSubtile(this->til, 185 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 186 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 118 - 1, 1, 324 - 1, silent); // 340
    ReplaceSubtile(this->til, 125 - 1, 2, 333 - 1, silent); // 365
    ReplaceSubtile(this->til, 130 - 1, 2, 395 - 1, silent); // 381
    ReplaceSubtile(this->til, 157 - 1, 2, 4 - 1, silent);   // 472
    ReplaceSubtile(this->til, 177 - 1, 1, 4 - 1, silent);   // 540
    ReplaceSubtile(this->til, 195 - 1, 2, 285 - 1, silent); // 590
    ReplaceSubtile(this->til, 211 - 1, 3, 48 - 1, silent);  // 621
    ReplaceSubtile(this->til, 205 - 1, 0, 45 - 1, silent);  // 625
    ReplaceSubtile(this->til, 207 - 1, 0, 45 - 1, silent);  // 630
    ReplaceSubtile(this->til, 207 - 1, 3, 626 - 1, silent); // 632
    ReplaceSubtile(this->til, 208 - 1, 0, 45 - 1, silent);  // 633

    ReplaceSubtile(this->til, 27 - 1, 3, 4 - 1, silent); // 85
    // ReplaceSubtile(this->til, 28 - 1, 3, 4 - 1, silent); // 87
    ReplaceSubtile(this->til, 29 - 1, 3, 4 - 1, silent); // 90
    // ReplaceSubtile(this->til, 30 - 1, 3, 4 - 1, silent); // 92
    // ReplaceSubtile(this->til, 31 - 1, 3, 4 - 1, silent); // 94
    ReplaceSubtile(this->til, 32 - 1, 3, 4 - 1, silent); // 96
    ReplaceSubtile(this->til, 33 - 1, 3, 4 - 1, silent); // 98
    // ReplaceSubtile(this->til, 34 - 1, 3, 4 - 1, silent); // 100
    ReplaceSubtile(this->til, 37 - 1, 3, 4 - 1, silent); // 108
    ReplaceSubtile(this->til, 38 - 1, 3, 4 - 1, silent); // 110
    // ReplaceSubtile(this->til, 39 - 1, 3, 4 - 1, silent); // 112
    // ReplaceSubtile(this->til, 40 - 1, 3, 4 - 1, silent); // 114
    // ReplaceSubtile(this->til, 41 - 1, 3, 4 - 1, silent); // 116
    // ReplaceSubtile(this->til, 42 - 1, 3, 4 - 1, silent); // 118
    ReplaceSubtile(this->til, 43 - 1, 3, 4 - 1, silent); // 120
    ReplaceSubtile(this->til, 44 - 1, 3, 4 - 1, silent); // 122
    ReplaceSubtile(this->til, 45 - 1, 3, 4 - 1, silent); // 124
    // ReplaceSubtile(this->til, 71 - 1, 3, 4 - 1, silent); // 214
    // ReplaceSubtile(this->til, 72 - 1, 3, 4 - 1, silent); // 217
    // ReplaceSubtile(this->til, 73 - 1, 3, 4 - 1, silent); // 219
    // ReplaceSubtile(this->til, 74 - 1, 3, 4 - 1, silent); // 221
    // ReplaceSubtile(this->til, 75 - 1, 3, 4 - 1, silent); // 223
    // ReplaceSubtile(this->til, 76 - 1, 3, 4 - 1, silent); // 225
    // ReplaceSubtile(this->til, 77 - 1, 3, 4 - 1, silent); // 227
    // ReplaceSubtile(this->til, 78 - 1, 3, 4 - 1, silent); // 229
    // ReplaceSubtile(this->til, 79 - 1, 3, 4 - 1, silent); // 231
    // ReplaceSubtile(this->til, 80 - 1, 3, 4 - 1, silent); // 233
    // ReplaceSubtile(this->til, 81 - 1, 3, 4 - 1, silent); // 235
    ReplaceSubtile(this->til, 15 - 1, 1, 4 - 1, silent); // 52
    ReplaceSubtile(this->til, 15 - 1, 2, 4 - 1, silent); // 53
    ReplaceSubtile(this->til, 15 - 1, 3, 4 - 1, silent); // 54
    ReplaceSubtile(this->til, 16 - 1, 1, 4 - 1, silent); // 56
    ReplaceSubtile(this->til, 144 - 1, 1, 4 - 1, silent);
    ReplaceSubtile(this->til, 16 - 1, 2, 4 - 1, silent); // 57
    ReplaceSubtile(this->til, 16 - 1, 3, 4 - 1, silent); // 58
    ReplaceSubtile(this->til, 144 - 1, 3, 7 - 1, silent);
    ReplaceSubtile(this->til, 94 - 1, 2, 60 - 1, silent); // 270
    ReplaceSubtile(this->til, 183 - 1, 2, 60 - 1, silent);
    ReplaceSubtile(this->til, 184 - 1, 2, 60 - 1, silent);
    ReplaceSubtile(this->til, 17 - 1, 2, 4 - 1, silent); // 61
    ReplaceSubtile(this->til, 128 - 1, 2, 4 - 1, silent);
    ReplaceSubtile(this->til, 92 - 1, 2, 62 - 1, silent); // 262
    ReplaceSubtile(this->til, 179 - 1, 2, 62 - 1, silent);
    ReplaceSubtile(this->til, 25 - 1, 1, 4 - 1, silent); // 76
    ReplaceSubtile(this->til, 25 - 1, 3, 4 - 1, silent); // 78
    ReplaceSubtile(this->til, 35 - 1, 1, 4 - 1, silent); // 102
    ReplaceSubtile(this->til, 35 - 1, 3, 4 - 1, silent); // 103
    ReplaceSubtile(this->til, 69 - 1, 1, 4 - 1, silent); // 205
    ReplaceSubtile(this->til, 69 - 1, 3, 4 - 1, silent); // 207
    ReplaceSubtile(this->til, 26 - 1, 2, 4 - 1, silent); // 81
    ReplaceSubtile(this->til, 26 - 1, 3, 4 - 1, silent); // 82
    ReplaceSubtile(this->til, 36 - 1, 2, 4 - 1, silent); // 105
    ReplaceSubtile(this->til, 36 - 1, 3, 4 - 1, silent); // 106
    ReplaceSubtile(this->til, 46 - 1, 2, 4 - 1, silent); // 127
    ReplaceSubtile(this->til, 46 - 1, 3, 4 - 1, silent); // 128
    ReplaceSubtile(this->til, 70 - 1, 2, 4 - 1, silent); // 210
    ReplaceSubtile(this->til, 70 - 1, 3, 4 - 1, silent); // 211
    ReplaceSubtile(this->til, 49 - 1, 1, 4 - 1, silent); // 137
    ReplaceSubtile(this->til, 167 - 1, 1, 4 - 1, silent);
    ReplaceSubtile(this->til, 49 - 1, 2, 4 - 1, silent); // 138
    ReplaceSubtile(this->til, 167 - 1, 2, 4 - 1, silent);
    ReplaceSubtile(this->til, 49 - 1, 3, 4 - 1, silent); // 139
    ReplaceSubtile(this->til, 167 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 50 - 1, 1, 4 - 1, silent);  // 141
    ReplaceSubtile(this->til, 50 - 1, 3, 4 - 1, silent);  // 143
    ReplaceSubtile(this->til, 51 - 1, 3, 4 - 1, silent);  // 147
    ReplaceSubtile(this->til, 103 - 1, 1, 4 - 1, silent); // 295
    ReplaceSubtile(this->til, 105 - 1, 1, 4 - 1, silent);
    ReplaceSubtile(this->til, 127 - 1, 3, 4 - 1, silent); // 373
    ReplaceSubtile(this->til, 89 - 1, 3, 4 - 1, silent);  // 251
    ReplaceSubtile(this->til, 173 - 1, 3, 7 - 1, silent);
    ReplaceSubtile(this->til, 174 - 1, 3, 7 - 1, silent);
    ReplaceSubtile(this->til, 6 - 1, 3, 4 - 1, silent); // 24
    ReplaceSubtile(this->til, 134 - 1, 3, 7 - 1, silent);
    ReplaceSubtile(this->til, 7 - 1, 3, 7 - 1, silent); // 28
    ReplaceSubtile(this->til, 8 - 1, 1, 2 - 1, silent); // 30
    // ReplaceSubtile(this->til, 30 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 32 - 1, 1, 2 - 1, silent);
    // ReplaceSubtile(this->til, 72 - 1, 1, 2 - 1, silent);
    ReplaceSubtile(this->til, 9 - 1, 3, 4 - 1, silent); // 35
    ReplaceSubtile(this->til, 137 - 1, 3, 4 - 1, silent);
    ReplaceSubtile(this->til, 11 - 1, 1, 4 - 1, silent); // 40
    ReplaceSubtile(this->til, 122 - 1, 1, 4 - 1, silent);
    ReplaceSubtile(this->til, 12 - 1, 2, 4 - 1, silent); // 43
    ReplaceSubtile(this->til, 123 - 1, 2, 4 - 1, silent);
    ReplaceSubtile(this->til, 12 - 1, 3, 4 - 1, silent); // 44
    ReplaceSubtile(this->til, 123 - 1, 3, 7 - 1, silent);
    ReplaceSubtile(this->til, 95 - 1, 1, 4 - 1, silent); // 273
    ReplaceSubtile(this->til, 185 - 1, 1, 7 - 1, silent);
    ReplaceSubtile(this->til, 186 - 1, 1, 4 - 1, silent);
    ReplaceSubtile(this->til, 89 - 1, 1, 293 - 1, silent); // 249
    ReplaceSubtile(this->til, 173 - 1, 1, 293 - 1, silent);
    ReplaceSubtile(this->til, 174 - 1, 1, 293 - 1, silent);
    ReplaceSubtile(this->til, 92 - 1, 3, 271 - 1, silent); // 263
    ReplaceSubtile(this->til, 179 - 1, 3, 271 - 1, silent);
    ReplaceSubtile(this->til, 96 - 1, 2, 12 - 1, silent); // 278
    ReplaceSubtile(this->til, 187 - 1, 2, 12 - 1, silent);
    ReplaceSubtile(this->til, 188 - 1, 2, 12 - 1, silent);
    // eliminate subtiles of unused tiles
    const int unusedTiles[] = {
        /* 29,*/ 30, 31,/* 32,*/ 34,/* 38,*/ 39, 40, 41, 42,/* 43, 44,*/ 52, 58, 61, 62, 63, 64, 65, 66, 67, 68, 72, 73, 74, 75, 76, 77, 78, 79, 212, 213, 214
    };
    constexpr int blankSubtile = 8 - 1;
    for (int n = 0; n < lengthof(unusedTiles); n++) {
        int tileId = unusedTiles[n];
        ReplaceSubtile(this->til, tileId - 1, 0, blankSubtile, silent);
        ReplaceSubtile(this->til, tileId - 1, 1, blankSubtile, silent);
        ReplaceSubtile(this->til, tileId - 1, 2, blankSubtile, silent);
        ReplaceSubtile(this->til, tileId - 1, 3, blankSubtile, silent);
    }
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
    // ReplaceMcr(242, 6, 31, 6);
    // SetMcr(242, 8, 31, 8);
    Blk2Mcr(242, 6); // - with the new special cels
    HideMcr(242, 8);
    ReplaceMcr(178, 0, 619, 1);
    ReplaceMcr(178, 1, 625, 0);
    SetMcr(178, 2, 624, 0);
    Blk2Mcr(178, 4);
    ReplaceMcr(178, 6, 89, 6);
    ReplaceMcr(178, 8, 89, 8);
    ReplaceMcr(238, 0, 634, 0);
    ReplaceMcr(238, 1, 634, 1);
    Blk2Mcr(238, 4);
    // SetMcr(238, 6, 31, 6);
    // SetMcr(238, 8, 31, 8);
    HideMcr(238, 6); // - with the new special cels
    HideMcr(238, 8);
    // pointless door micros (re-drawn by dSpecial or the object)
    ReplaceMcr(77, 0, 206, 0);
    ReplaceMcr(77, 1, 206, 1);
    Blk2Mcr(77, 2);
    Blk2Mcr(77, 4);
    Blk2Mcr(77, 6);
    Blk2Mcr(77, 8);
    ReplaceMcr(76, 0, 206, 0);
    ReplaceMcr(76, 1, 206, 1);
    // ReplaceMcr(75, 0, 204, 0);
    // ReplaceMcr(75, 1, 204, 1);
    // ReplaceMcr(75, 2, 204, 2);
    // ReplaceMcr(75, 4, 204, 4);
    // ReplaceMcr(91, 0, 204, 0);
    // ReplaceMcr(91, 2, 204, 2);
    // ReplaceMcr(91, 4, 204, 4);
    ReplaceMcr(99, 0, 204, 0);
    ReplaceMcr(99, 2, 204, 2);
    ReplaceMcr(99, 4, 204, 4);
    ReplaceMcr(99, 6, 119, 6);
    ReplaceMcr(113, 0, 204, 0);
    ReplaceMcr(113, 2, 204, 2);
    ReplaceMcr(113, 4, 204, 4);
    ReplaceMcr(113, 6, 119, 6);
    ReplaceMcr(115, 0, 204, 0);
    ReplaceMcr(115, 2, 204, 2);
    ReplaceMcr(115, 4, 204, 4);
    ReplaceMcr(115, 6, 119, 6);
    ReplaceMcr(204, 6, 119, 6);
    ReplaceMcr(80, 0, 209, 0);
    ReplaceMcr(80, 1, 209, 1);
    Blk2Mcr(80, 3);
    Blk2Mcr(80, 5);
    Blk2Mcr(80, 7);
    Blk2Mcr(80, 9);
    ReplaceMcr(79, 0, 209, 0);
    ReplaceMcr(79, 1, 209, 1);
    Blk2Mcr(79, 3);
    Blk2Mcr(79, 5);
    Blk2Mcr(79, 7);
    Blk2Mcr(79, 9);
    // ReplaceMcr(79, 0, 208, 0);
    // ReplaceMcr(79, 1, 208, 1);
    // ReplaceMcr(79, 3, 208, 3);
    // ReplaceMcr(79, 5, 208, 5);
    ReplaceMcr(93, 1, 208, 1);
    ReplaceMcr(93, 3, 208, 3);
    ReplaceMcr(93, 5, 208, 5);
    ReplaceMcr(111, 1, 208, 1);
    ReplaceMcr(111, 3, 208, 3);
    ReplaceMcr(111, 5, 208, 5);
    ReplaceMcr(117, 1, 208, 1);
    ReplaceMcr(117, 3, 208, 3);
    ReplaceMcr(117, 5, 208, 5);
    ReplaceMcr(119, 1, 208, 1);
    ReplaceMcr(119, 3, 208, 3);
    ReplaceMcr(119, 5, 208, 5);
    Blk2Mcr(206, 2);
    Blk2Mcr(206, 4);
    Blk2Mcr(206, 6);
    Blk2Mcr(206, 8);
    Blk2Mcr(209, 3);
    Blk2Mcr(209, 5);
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
    // Blk2Mcr(398, 5);
    // fix graphical glitch
    ReplaceMcr(21, 1, 55, 1);
    ReplaceMcr(25, 0, 33, 0);
    // ReplaceMcr(22, 0, 2, 0);
    // ReplaceMcr(22, 1, 2, 1);
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
    // ReplaceMcr(564, 0, 303, 0);
    ReplaceMcr(635, 0, 308, 0);
    // - extend shadow to make more usable (after fixCryptShadows)
    // ReplaceMcr(627, 0, 626, 0);
    // SetMcr(627, 1, 626, 1);
    // prepare new subtiles for the shadows
    ReplaceMcr(623, 0, 631, 0);
    ReplaceMcr(623, 1, 638, 1);
    ReplaceMcr(636, 0, 626, 0);
    ReplaceMcr(636, 1, 638, 1);
    // fix automap of the entrance I.
    Blk2Mcr(148, 0);
    Blk2Mcr(148, 1);
    // with the new special cels
    ReplaceMcr(31, 0, 4, 0);
    // ReplaceMcr(468, 0, 4, 0);
    ReplaceMcr(333, 0, 31, 0); // lost details
    ReplaceMcr(345, 0, 31, 0); // lost details
    ReplaceMcr(356, 0, 7, 0);
    ReplaceMcr(445, 0, 7, 0); // lost details
    ReplaceMcr(404, 0, 474, 0); // lost details
    // TODO: 274[0], 277[1] ?
    Blk2Mcr(31, 2);
    // Blk2Mcr(468, 2);
    Blk2Mcr(274, 2);
    Blk2Mcr(299, 2);
    Blk2Mcr(333, 2);
    Blk2Mcr(345, 2);
    Blk2Mcr(356, 2);
    Blk2Mcr(404, 2);
    Blk2Mcr(415, 2);
    Blk2Mcr(445, 2);
    Blk2Mcr(456, 2);
    ReplaceMcr(331, 4, 29, 4);
    ReplaceMcr(343, 4, 29, 4);
    ReplaceMcr(355, 4, 29, 4);
    ReplaceMcr(363, 4, 29, 4);
    ReplaceMcr(443, 4, 29, 4);
    Blk2Mcr(31, 4);
    // Blk2Mcr(468, 4);
    Blk2Mcr(274, 4);
    Blk2Mcr(298, 4);
    Blk2Mcr(299, 4);
    Blk2Mcr(301, 4);
    Blk2Mcr(333, 4);
    Blk2Mcr(345, 4);
    Blk2Mcr(356, 4);
    Blk2Mcr(404, 4);
    Blk2Mcr(415, 4);
    Blk2Mcr(445, 4);
    Blk2Mcr(456, 4);
    Blk2Mcr(31, 6);
    // Blk2Mcr(468, 6);
    Blk2Mcr(274, 6);
    Blk2Mcr(298, 6);
    Blk2Mcr(299, 6);
    ReplaceMcr(300, 6, 119, 6); // lost details
    ReplaceMcr(294, 6, 119, 6); // lost details
    ReplaceMcr(454, 6, 252, 6); // lost details
    ReplaceMcr(413, 6, 25, 6); // lost details
    ReplaceMcr(331, 6, 25, 6); // lost details
    ReplaceMcr(343, 6, 25, 6); // lost details
    ReplaceMcr(355, 6, 25, 6); // lost details
    ReplaceMcr(363, 6, 25, 6); // lost details
    ReplaceMcr(557, 6, 25, 6); // lost details
    ReplaceMcr(559, 6, 25, 6); // lost details
    ReplaceMcr(290, 6, 296, 6);
    ReplaceMcr(292, 6, 296, 6);
    Blk2Mcr(301, 6);
    Blk2Mcr(333, 6);
    Blk2Mcr(345, 6);
    Blk2Mcr(356, 6);
    Blk2Mcr(404, 6);
    Blk2Mcr(415, 6);
    Blk2Mcr(445, 6);
    Blk2Mcr(456, 6);
    Blk2Mcr(31, 8);
    // Blk2Mcr(468, 8);
    Blk2Mcr(274, 8);
    Blk2Mcr(290, 8);
    Blk2Mcr(292, 8);
    Blk2Mcr(296, 8);
    Blk2Mcr(298, 8);
    Blk2Mcr(299, 8);
    Blk2Mcr(301, 8);
    Blk2Mcr(333, 8);
    Blk2Mcr(345, 8);
    Blk2Mcr(356, 8);
    Blk2Mcr(404, 8);
    Blk2Mcr(415, 8);
    Blk2Mcr(445, 8);
    Blk2Mcr(456, 8);

    ReplaceMcr(348, 1, 18, 1);
    ReplaceMcr(352, 1, 18, 1);
    ReplaceMcr(358, 1, 18, 1);
    ReplaceMcr(406, 1, 18, 1);
    ReplaceMcr(459, 1, 18, 1);
    Blk2Mcr(18, 3);
    Blk2Mcr(277, 3);
    Blk2Mcr(332, 3);
    Blk2Mcr(348, 3);
    Blk2Mcr(352, 3);
    Blk2Mcr(358, 3);
    Blk2Mcr(406, 3);
    Blk2Mcr(444, 3); // lost details
    Blk2Mcr(459, 3);
    Blk2Mcr(463, 3);
    Blk2Mcr(471, 3);
    Blk2Mcr(562, 3);
    ReplaceMcr(470, 5, 276, 5);
    Blk2Mcr(18, 5);
    Blk2Mcr(277, 5);
    Blk2Mcr(332, 5);
    Blk2Mcr(348, 5);
    Blk2Mcr(352, 5);
    Blk2Mcr(358, 5);
    Blk2Mcr(406, 5);
    Blk2Mcr(444, 5);
    Blk2Mcr(459, 5);
    Blk2Mcr(463, 5);
    Blk2Mcr(471, 5);
    Blk2Mcr(562, 5);
    ReplaceMcr(347, 7, 13, 7); // lost details
    ReplaceMcr(276, 7, 13, 7); // lost details
    ReplaceMcr(458, 7, 13, 7); // lost details
    ReplaceMcr(561, 7, 13, 7); // lost details
    ReplaceMcr(563, 7, 13, 7); // lost details
    Blk2Mcr(18, 7);
    Blk2Mcr(277, 7);
    Blk2Mcr(332, 7);
    Blk2Mcr(348, 7);
    Blk2Mcr(352, 7);
    Blk2Mcr(358, 7);
    Blk2Mcr(406, 7);
    Blk2Mcr(444, 7);
    Blk2Mcr(459, 7);
    Blk2Mcr(463, 7);
    Blk2Mcr(471, 7);
    Blk2Mcr(562, 7);
    Blk2Mcr(18, 9);
    Blk2Mcr(277, 9);
    Blk2Mcr(332, 9);
    Blk2Mcr(348, 9);
    Blk2Mcr(352, 9);
    Blk2Mcr(358, 9);
    Blk2Mcr(406, 9);
    Blk2Mcr(444, 9);
    Blk2Mcr(459, 9);
    Blk2Mcr(463, 9);
    Blk2Mcr(471, 9);
    Blk2Mcr(562, 9);
    // subtile for the separate pillar tile
    // - 91 == 9
    ReplaceMcr(91, 0, 33, 0);
    ReplaceMcr(91, 1, 55, 1);
    ReplaceMcr(91, 2, 29, 2);
    SetMcr(91, 3, 21, 3);
    ReplaceMcr(91, 4, 25, 4);
    SetMcr(91, 5, 21, 5);
    ReplaceMcr(91, 6, 25, 6);
    SetMcr(91, 7, 21, 7);
    ReplaceMcr(91, 8, 9, 8);
    ReplaceMcr(91, 9, 9, 9);
    // reuse subtiles
    ReplaceMcr(631, 1, 626, 1);
    ReplaceMcr(149, 4, 1, 4);
    ReplaceMcr(150, 6, 15, 6);
    ReplaceMcr(324, 7, 6, 7);
    ReplaceMcr(432, 7, 6, 7);
    ReplaceMcr(440, 7, 6, 7);
    // ReplaceMcr(26, 9, 6, 9);
    // ReplaceMcr(340, 9, 6, 9);
    ReplaceMcr(394, 9, 6, 9);
    ReplaceMcr(451, 9, 6, 9);
    // ReplaceMcr(14, 7, 6, 7); // lost shine
    // ReplaceMcr(26, 7, 6, 7); // lost shine
    // ReplaceMcr(80, 7, 6, 7);
    ReplaceMcr(451, 7, 6, 7); // lost shine
    // ReplaceMcr(340, 7, 6, 7); // lost shine
    ReplaceMcr(364, 7, 6, 7); // lost crack
    ReplaceMcr(394, 7, 6, 7); // lost shine
    // ReplaceMcr(554, 7, 269, 7);
    ReplaceMcr(608, 7, 6, 7);   // lost details
    ReplaceMcr(616, 7, 6, 7);   // lost details
    ReplaceMcr(269, 5, 554, 5); // lost details
    ReplaceMcr(556, 5, 554, 5);
    ReplaceMcr(440, 5, 432, 5); // lost details
    // ReplaceMcr(14, 5, 6, 5); // lost details
    // ReplaceMcr(26, 5, 6, 5); // lost details
    ReplaceMcr(451, 5, 6, 5); // lost details
    // ReplaceMcr(80, 5, 6, 5); // lost details
    ReplaceMcr(324, 5, 432, 5); // lost details
    // ReplaceMcr(340, 5, 432, 5); // lost details
    ReplaceMcr(364, 5, 432, 5); // lost details
    ReplaceMcr(380, 5, 432, 5); // lost details
    ReplaceMcr(394, 5, 432, 5); // lost details
    ReplaceMcr(6, 3, 14, 3);    // lost details
    // ReplaceMcr(26, 3, 14, 3); // lost details
    // ReplaceMcr(80, 3, 14, 3); // lost details
    ReplaceMcr(269, 3, 14, 3); // lost details
    ReplaceMcr(414, 3, 14, 3); // lost details
    ReplaceMcr(451, 3, 14, 3); // lost details
    // ReplaceMcr(554, 3, 14, 3); // lost details
    ReplaceMcr(556, 3, 14, 3); // lost details
    // ? ReplaceMcr(608, 3, 103, 3); // lost details
    ReplaceMcr(324, 3, 380, 3); // lost details
    // ReplaceMcr(340, 3, 380, 3); // lost details
    ReplaceMcr(364, 3, 380, 3); // lost details
    ReplaceMcr(432, 3, 380, 3); // lost details
    ReplaceMcr(440, 3, 380, 3); // lost details
    ReplaceMcr(6, 0, 14, 0);
    // ReplaceMcr(26, 0, 14, 0);
    ReplaceMcr(269, 0, 14, 0);
    // ReplaceMcr(554, 0, 14, 0);  // lost details
    // ReplaceMcr(340, 0, 324, 0); // lost details
    ReplaceMcr(364, 0, 324, 0); // lost details
    ReplaceMcr(451, 0, 324, 0); // lost details
    // ReplaceMcr(14, 1, 6, 1);
    // ReplaceMcr(26, 1, 6, 1);
    // ReplaceMcr(80, 1, 6, 1);
    ReplaceMcr(269, 1, 6, 1);
    ReplaceMcr(380, 1, 6, 1);
    ReplaceMcr(451, 1, 6, 1);
    // ReplaceMcr(554, 1, 6, 1);
    ReplaceMcr(556, 1, 6, 1);
    ReplaceMcr(324, 1, 340, 1);
    ReplaceMcr(364, 1, 340, 1);
    ReplaceMcr(394, 1, 340, 1); // lost details
    ReplaceMcr(432, 1, 340, 1); // lost details
    ReplaceMcr(551, 5, 265, 5);
    ReplaceMcr(551, 0, 265, 0);
    ReplaceMcr(551, 1, 265, 1);
    ReplaceMcr(261, 0, 14, 0); // lost details
    // ReplaceMcr(545, 0, 14, 0); // lost details
    // ReplaceMcr(18, 9, 6, 9);   // lost details
    // ReplaceMcr(34, 9, 6, 9);   // lost details
    // ReplaceMcr(37, 9, 6, 9);
    // ReplaceMcr(277, 9, 6, 9);   // lost details
    // ReplaceMcr(332, 9, 6, 9);   // lost details
    // ReplaceMcr(348, 9, 6, 9);   // lost details
    // ReplaceMcr(352, 9, 6, 9);   // lost details
    // ReplaceMcr(358, 9, 6, 9);   // lost details
    // ReplaceMcr(406, 9, 6, 9);   // lost details
    // ReplaceMcr(444, 9, 6, 9);   // lost details
    // ReplaceMcr(459, 9, 6, 9);   // lost details
    // ReplaceMcr(463, 9, 6, 9);   // lost details
    // ReplaceMcr(562, 9, 6, 9);   // lost details
    // ReplaceMcr(564, 9, 6, 9);   // lost details
    // ReplaceMcr(277, 7, 18, 7);  // lost details
    // ReplaceMcr(562, 7, 18, 7);  // lost details
    // ReplaceMcr(277, 5, 459, 5); // lost details
    // ReplaceMcr(562, 5, 459, 5); // lost details
    // ReplaceMcr(277, 3, 459, 3); // lost details
    ReplaceMcr(562, 1, 277, 1); // lost details
    // ReplaceMcr(564, 1, 277, 1); // lost details
    ReplaceMcr(585, 1, 284, 1);
    // ReplaceMcr(590, 1, 285, 1); // lost details
    ReplaceMcr(598, 1, 289, 1); // lost details
    // ReplaceMcr(564, 7, 18, 7); // lost details
    // ReplaceMcr(564, 5, 459, 5); // lost details
    // ReplaceMcr(564, 3, 459, 3); // lost details
    // ReplaceMcr(34, 7, 18, 7); // lost details
    // ReplaceMcr(37, 7, 18, 7);
    // ReplaceMcr(84, 7, 18, 7);   // lost details
    // ReplaceMcr(406, 7, 18, 7);  // lost details
    // ReplaceMcr(444, 7, 18, 7);  // lost details
    // ReplaceMcr(463, 7, 18, 7);  // lost details
    // ReplaceMcr(332, 7, 18, 7);  // lost details
    // ReplaceMcr(348, 7, 18, 7);  // lost details
    // ReplaceMcr(352, 7, 18, 7);  // lost details
    // ReplaceMcr(358, 7, 18, 7);  // lost details
    // ReplaceMcr(459, 7, 18, 7);  // lost details
    // ReplaceMcr(34, 5, 18, 5);   // lost details
    // ReplaceMcr(348, 5, 332, 5); // lost details
    // ReplaceMcr(352, 5, 332, 5); // lost details
    // ReplaceMcr(358, 5, 332, 5); // lost details
    // ReplaceMcr(34, 3, 18, 3);   // lost details
    // ReplaceMcr(358, 3, 18, 3);  // lost details
    // ReplaceMcr(348, 3, 332, 3); // lost details
    // ReplaceMcr(352, 3, 332, 3); // lost details
    // ReplaceMcr(34, 0, 18, 0);
    ReplaceMcr(352, 0, 18, 0);
    ReplaceMcr(358, 0, 18, 0);
    ReplaceMcr(406, 0, 18, 0); // lost details
    // ReplaceMcr(34, 1, 18, 1);
    ReplaceMcr(332, 1, 18, 1);
    // ReplaceMcr(348, 1, 352, 1);
    // ReplaceMcr(358, 1, 352, 1);
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
    // ReplaceMcr(554, 9, 6, 9); // lost details
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
    // ReplaceMcr(292, 8, 3, 8);
    // ReplaceMcr(299, 8, 3, 8);
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
    ReplaceMcr(13, 6, 36, 6);
    ReplaceMcr(13, 4, 36, 4);
    ReplaceMcr(387, 6, 36, 6);
    // ReplaceMcr(390, 2, 19, 2);
    ReplaceMcr(29, 5, 21, 5);
    ReplaceMcr(95, 5, 21, 5);
    // ReplaceMcr(24, 0, 32, 0); // lost details
    // ReplaceMcr(354, 0, 32, 0); // lost details
    // ReplaceMcr(398, 0, 2, 0);
    // ReplaceMcr(398, 1, 2, 1); // lost details
    // ReplaceMcr(540, 0, 257, 0);
    // ReplaceMcr(30, 0, 2, 0);
    // ReplaceMcr(76, 0, 2, 0);
    // ReplaceMcr(205, 0, 2, 0);
    ReplaceMcr(407, 0, 2, 0); // lost details
    ReplaceMcr(379, 0, 5, 0);
    // ReplaceMcr(27, 0, 7, 0);
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
    // ReplaceMcr(275, 0, 32, 0);
    // ReplaceMcr(309, 0, 45, 0);
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
    // ReplaceMcr(41, 1, 4, 1);
    // ReplaceMcr(24, 1, 4, 1); // lost details
    ReplaceMcr(32, 1, 4, 1); // lost details
    // ReplaceMcr(92, 1, 4, 1); // lost details
    // ReplaceMcr(96, 1, 4, 1); // lost details
    // ReplaceMcr(217, 1, 4, 1); // lost details
    // ReplaceMcr(275, 1, 4, 1); // lost details
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
    // ReplaceMcr(304, 1, 48, 1);
    // ReplaceMcr(27, 1, 7, 1);
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
    // ReplaceMcr(471, 7, 265, 7);
    ReplaceMcr(547, 7, 261, 7);
    // ReplaceMcr(471, 9, 6, 9);
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
    // ReplaceMcr(388, 2, 15, 2);
    // ReplaceMcr(479, 5, 14, 5);
    ReplaceMcr(389, 6, 17, 6); // lost details
    // ReplaceMcr(19, 8, 89, 8);  // lost details
    // ReplaceMcr(390, 8, 89, 8);  // lost details
    // ReplaceMcr(31, 8, 89, 8);
    ReplaceMcr(254, 8, 89, 8); // lost details
    ReplaceMcr(534, 8, 89, 8); // lost details
    ReplaceMcr(537, 8, 89, 8); // lost details
    // ReplaceMcr(333, 8, 89, 8); // lost details
    // ReplaceMcr(345, 8, 89, 8);
    // ReplaceMcr(365, 8, 89, 8); // lost details
    // ReplaceMcr(456, 8, 89, 8);
    // ReplaceMcr(274, 8, 89, 8); // lost details
    // ReplaceMcr(558, 8, 89, 8); // lost details
    // ReplaceMcr(560, 8, 89, 8); // lost details
    ReplaceMcr(258, 8, 3, 8); // lost details
    ReplaceMcr(541, 8, 3, 8); // lost details
    // ReplaceMcr(543, 8, 3, 8); // lost details
    // ReplaceMcr(31, 6, 89, 6);  // lost details
    // ReplaceMcr(274, 6, 89, 6); // lost details
    // ReplaceMcr(558, 6, 89, 6); // lost details
    // ReplaceMcr(560, 6, 89, 6); // lost details
    // ReplaceMcr(356, 6, 89, 6);  // lost details
    // ReplaceMcr(333, 6, 445, 6); // lost details
    // ReplaceMcr(345, 6, 445, 6); // lost details
    // ReplaceMcr(365, 6, 445, 6); // lost details
    // ReplaceMcr(274, 4, 31, 4);  // lost details
    // ReplaceMcr(560, 4, 31, 4); // lost details
    // ReplaceMcr(333, 4, 345, 4); // lost details
    // ReplaceMcr(365, 4, 345, 4); // lost details
    // ReplaceMcr(445, 4, 345, 4); // lost details
    // ReplaceMcr(299, 2, 274, 2); // lost details
    // ReplaceMcr(560, 2, 274, 2); // lost details
    // ReplaceMcr(333, 2, 345, 2); // lost details
    // ReplaceMcr(365, 2, 345, 2); // lost details
    // ReplaceMcr(415, 2, 345, 2); // lost details
    // ReplaceMcr(445, 2, 345, 2); // lost details
    // ReplaceMcr(333, 0, 31, 0);  // lost details
    // ReplaceMcr(345, 0, 31, 0);  // lost details
    // ReplaceMcr(365, 0, 31, 0);  // lost details
    // ReplaceMcr(445, 0, 31, 0);  // lost details
    ReplaceMcr(333, 1, 31, 1);  // lost details
    // ReplaceMcr(365, 1, 31, 1);

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

    ReplaceMcr(1, 8, 95, 8);  // lost details
    ReplaceMcr(21, 8, 95, 8); // lost details
    ReplaceMcr(36, 8, 95, 8); // lost details
    // ReplaceMcr(75, 8, 95, 8);
    ReplaceMcr(83, 8, 95, 8); // lost details
    // ReplaceMcr(91, 8, 95, 8);
    ReplaceMcr(99, 8, 95, 8);  // lost details
    ReplaceMcr(113, 8, 95, 8); // lost details
    ReplaceMcr(115, 8, 95, 8); // lost details
    ReplaceMcr(119, 8, 95, 8); // lost details
    ReplaceMcr(149, 8, 95, 8); // lost details
    ReplaceMcr(151, 8, 95, 8); // lost details
    // ReplaceMcr(172, 8, 95, 8);
    ReplaceMcr(204, 8, 95, 8);
    // ReplaceMcr(215, 8, 95, 8);
    // ReplaceMcr(220, 8, 95, 8); // lost details
    // ReplaceMcr(224, 8, 95, 8); // lost details
    // ReplaceMcr(226, 8, 95, 8); // lost details
    // ReplaceMcr(230, 8, 95, 8); // lost details
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
    // ReplaceMcr(75, 6, 99, 6);   // lost details
    // ReplaceMcr(91, 6, 99, 6);   // lost details
    // ReplaceMcr(115, 6, 99, 6);  // lost details
    // ReplaceMcr(204, 6, 99, 6);  // lost details
    // ReplaceMcr(215, 6, 99, 6);  // lost details

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

    // ReplaceMcr(300, 6, 294, 6); // lost details
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
    // ReplaceMcr(230, 2, 256, 2); // lost details

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
    // ReplaceMcr(218, 8, 95, 8); // lost details
    // ReplaceMcr(222, 8, 95, 8); // lost details
    // ReplaceMcr(228, 8, 95, 8); // lost details
    ReplaceMcr(272, 8, 95, 8);
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
    ReplaceMcr(557, 8, 95, 8);
    ReplaceMcr(559, 8, 95, 8);
    ReplaceMcr(615, 8, 95, 8); // lost details
    ReplaceMcr(421, 8, 55, 8); // lost details
    ReplaceMcr(154, 8, 55, 8); // lost details
    ReplaceMcr(154, 9, 13, 9); // lost details

    ReplaceMcr(9, 6, 25, 6);   // lost details
    ReplaceMcr(33, 6, 25, 6);  // lost details
    ReplaceMcr(51, 6, 25, 6);  // lost details
    ReplaceMcr(93, 6, 25, 6);  // lost details
    ReplaceMcr(97, 6, 25, 6);  // lost details
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
    // ReplaceMcr(222, 6, 95, 6); // lost details
    // ReplaceMcr(228, 6, 95, 6); // lost details
    ReplaceMcr(272, 6, 95, 6); // lost details
    ReplaceMcr(389, 6, 95, 6); // lost details
    ReplaceMcr(397, 6, 95, 6); // lost details
    // ReplaceMcr(402, 6, 95, 6); // lost details
    ReplaceMcr(443, 6, 95, 6);  // lost details
    ReplaceMcr(466, 6, 95, 6);  // lost details
    ReplaceMcr(478, 6, 95, 6);  // lost details
    // ReplaceMcr(347, 6, 393, 6); // lost details
    ReplaceMcr(399, 6, 393, 6); // lost details
    ReplaceMcr(417, 6, 393, 6); // lost details
    // ReplaceMcr(331, 6, 343, 6); // lost details
    // ReplaceMcr(355, 6, 343, 6); // lost details
    // ReplaceMcr(363, 6, 343, 6); // lost details
    // ReplaceMcr(557, 6, 343, 6); // lost details
    // ReplaceMcr(559, 6, 343, 6); // lost details

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
    // ReplaceMcr(331, 4, 343, 4); // lost details
    // ReplaceMcr(355, 4, 343, 4); // lost details
    // ReplaceMcr(363, 4, 343, 4); // lost details
    // ReplaceMcr(443, 4, 343, 4); // lost details
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
    // ReplaceMcr(218, 2, 29, 2);  // lost details
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
    // ReplaceMcr(218, 0, 33, 0);
    // ReplaceMcr(228, 0, 33, 0);  // lost details
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
    // ReplaceMcr(79, 9, 13, 9);
    ReplaceMcr(93, 9, 13, 9);
    ReplaceMcr(151, 9, 13, 9);
    ReplaceMcr(208, 9, 13, 9);
    // ReplaceMcr(218, 9, 13, 9);
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
    // ReplaceMcr(79, 7, 13, 7);
    ReplaceMcr(93, 7, 13, 7);
    ReplaceMcr(107, 7, 13, 7);
    ReplaceMcr(111, 7, 13, 7);
    ReplaceMcr(115, 7, 13, 7);
    ReplaceMcr(117, 7, 13, 7);
    ReplaceMcr(119, 7, 13, 7);
    ReplaceMcr(208, 7, 13, 7);
    // ReplaceMcr(218, 7, 13, 7);
    // ReplaceMcr(222, 7, 13, 7);
    // ReplaceMcr(226, 7, 13, 7);
    // ReplaceMcr(228, 7, 13, 7);
    // ReplaceMcr(230, 7, 13, 7);
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
    // ReplaceMcr(226, 5, 25, 5);
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
    // ReplaceMcr(226, 3, 25, 3);
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
    // ReplaceMcr(91, 9, 13, 9); // lost details
    ReplaceMcr(95, 9, 13, 9);
    ReplaceMcr(97, 9, 13, 9);  // lost details
    ReplaceMcr(99, 9, 13, 9);  // lost details
    ReplaceMcr(104, 9, 13, 9); // lost details
    ReplaceMcr(109, 9, 13, 9); // lost details
    ReplaceMcr(113, 9, 13, 9); // lost details
    ReplaceMcr(121, 9, 13, 9); // lost details
    // ReplaceMcr(215, 9, 13, 9); // lost details
    // ReplaceMcr(220, 9, 13, 9); // lost details
    // ReplaceMcr(224, 9, 13, 9); // lost details
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
    // ReplaceMcr(220, 7, 17, 7);  // lost details
    // ReplaceMcr(224, 7, 17, 7);  // lost details
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
    // ReplaceMcr(91, 1, 55, 1);   // lost details
    ReplaceMcr(95, 1, 55, 1);   // lost details
    // ReplaceMcr(215, 1, 55, 1);  // lost details
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
    HideMcr(37, 1);
    Blk2Mcr(37, 3);
    Blk2Mcr(37, 5);
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
    Blk2Mcr(196, 1);
    Blk2Mcr(197, 0);
    Blk2Mcr(200, 1);
    Blk2Mcr(201, 0);
    Blk2Mcr(86, 0);
    Blk2Mcr(86, 1);
    Blk2Mcr(86, 2);
    Blk2Mcr(86, 3);
    Blk2Mcr(86, 4);
    Blk2Mcr(86, 5);
    Blk2Mcr(86, 6);
    Blk2Mcr(86, 7);
    Blk2Mcr(86, 8);
    HideMcr(212, 0);
    HideMcr(212, 1);
    HideMcr(212, 2);
    HideMcr(212, 3);
    HideMcr(212, 4);
    HideMcr(212, 5);
    Blk2Mcr(212, 6);
    Blk2Mcr(212, 7);
    Blk2Mcr(212, 8);
    HideMcr(232, 0);
    HideMcr(232, 2);
    HideMcr(232, 4);
    Blk2Mcr(232, 1);
    Blk2Mcr(232, 3);
    Blk2Mcr(232, 5);
    Blk2Mcr(232, 6);
    Blk2Mcr(232, 7);
    Blk2Mcr(232, 8);
    Blk2Mcr(234, 0);
    Blk2Mcr(234, 2);
    Blk2Mcr(234, 4);
    Blk2Mcr(234, 6);
    Blk2Mcr(234, 7);
    Blk2Mcr(234, 8);
    HideMcr(234, 1);
    HideMcr(234, 3);
    HideMcr(234, 5);
    HideMcr(234, 9);
    Blk2Mcr(213, 0);
    Blk2Mcr(213, 2);
    Blk2Mcr(213, 4);
    Blk2Mcr(213, 6);
    Blk2Mcr(213, 8);
    HideMcr(215, 0);
    HideMcr(215, 2);
    HideMcr(215, 4);
    Blk2Mcr(215, 1);
    Blk2Mcr(215, 6);
    Blk2Mcr(215, 8);
    Blk2Mcr(215, 9);
    Blk2Mcr(216, 0);
    Blk2Mcr(216, 2);
    Blk2Mcr(216, 4);
    Blk2Mcr(216, 6);
    Blk2Mcr(216, 8);
    HideMcr(218, 1);
    HideMcr(218, 3);
    HideMcr(218, 5);
    HideMcr(218, 6);
    Blk2Mcr(218, 0);
    Blk2Mcr(218, 2);
    Blk2Mcr(218, 7);
    Blk2Mcr(218, 8);
    Blk2Mcr(218, 9);
    HideMcr(220, 0);
    HideMcr(220, 2);
    HideMcr(220, 4);
    Blk2Mcr(220, 6);
    Blk2Mcr(220, 7);
    Blk2Mcr(220, 8);
    Blk2Mcr(220, 9);
    HideMcr(222, 1);
    HideMcr(222, 3);
    HideMcr(222, 5);
    Blk2Mcr(222, 6);
    Blk2Mcr(222, 7);
    Blk2Mcr(222, 8);
    HideMcr(224, 0);
    HideMcr(224, 2);
    HideMcr(224, 4);
    Blk2Mcr(224, 6);
    Blk2Mcr(224, 7);
    Blk2Mcr(224, 8);
    Blk2Mcr(224, 9);
    Blk2Mcr(226, 3);
    Blk2Mcr(226, 5);
    Blk2Mcr(226, 6);
    Blk2Mcr(226, 7);
    Blk2Mcr(226, 8);
    HideMcr(226, 0);
    HideMcr(226, 2);
    HideMcr(226, 4);
    Blk2Mcr(228, 0);
    Blk2Mcr(228, 6);
    Blk2Mcr(228, 7);
    Blk2Mcr(228, 8);
    HideMcr(228, 1);
    HideMcr(228, 3);
    HideMcr(228, 5);
    HideMcr(230, 1);
    HideMcr(230, 3);
    HideMcr(230, 5);
    Blk2Mcr(230, 2);
    Blk2Mcr(230, 7);
    Blk2Mcr(230, 8);
    Blk2Mcr(304, 1);
    Blk2Mcr(309, 0);
    Blk2Mcr(340, 0);
    // Blk2Mcr(340, 1); - used in 324...
    Blk2Mcr(340, 3);
    Blk2Mcr(340, 5);
    Blk2Mcr(340, 7);
    Blk2Mcr(340, 9);
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
    Blk2Mcr(545, 0);
    Blk2Mcr(554, 0);
    Blk2Mcr(554, 1);
    Blk2Mcr(554, 3);
    // Blk2Mcr(554, 5); - used in 269
    Blk2Mcr(554, 7);
    Blk2Mcr(554, 9);
    Blk2Mcr(558, 0);
    Blk2Mcr(558, 2);
    Blk2Mcr(558, 4);
    Blk2Mcr(558, 6);
    Blk2Mcr(558, 8);
    Blk2Mcr(590, 1);
    Blk2Mcr(41, 1);
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
    Blk2Mcr(627, 0);
    Blk2Mcr(630, 0);
    Blk2Mcr(632, 0);
    Blk2Mcr(633, 0);
    Blk2Mcr(637, 0);
    Blk2Mcr(642, 1);
    Blk2Mcr(644, 0);
    Blk2Mcr(645, 1);
    Blk2Mcr(646, 0);
    Blk2Mcr(649, 1);
    Blk2Mcr(650, 0);
    const int unusedSubtiles[] = {
        8, 10, 11, 16, 19, 20, 22, 23, 24, 26, 27, 28, 30, 34, 35, 38, 40, 43, 44, 50, 52, 56, 75, 78, 81, 82, 87, 90, 92, 94, 96, 98, 100, 102, 103, 105, 106, 108, 110, 112, 114, 116, 124, 127, 128, 137, 138, 139, 141, 143, 147, 167, 172, 174, 176, 177, 193, 202, 205, 207, 210, 211, 214, 217, 219, 221, 223, 225, 227, 233, 235, 239, 249, 251, 253, 257, 259, 262, 263, 270, 273, 275, 278, 279, 295, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 354, 365, 373, 381, 390, 398, 468, 472, 489, 490, 540, 560, 564, 640, 643, 648
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
        if (this->min->getSubtileCount() < 1258) {
            // dProgressErr() << QApplication::tr("Invalid MIN file. Subtile-count is less than %1").arg(1258);
            break; // -- assume it is already done
        }
        this->cleanupTown(deletedFrames, silent);
    } break;
    case DTYPE_CATHEDRAL:
        // patch dMiniTiles and dMegaTiles - L1.MIN and L1.TIL
        if (this->min->getSubtileCount() < 453) {
            dProgressErr() << QApplication::tr("Invalid MIN file. Subtile-count is less than %1").arg(453);
            break;
        }
        this->cleanupCathedral(deletedFrames, silent);
        // patch pSpecialsCel - L1S.CEL
        this->patchCathedralSpec(silent);
        break;
    case DTYPE_CATACOMBS:
        // patch dMiniTiles and dMegaTiles - L2.MIN and L2.TIL
        if (this->min->getSubtileCount() < 559) {
            dProgressErr() << QApplication::tr("Invalid MIN file. Subtile-count is less than %1").arg(559);
            break;
        }
        this->cleanupCatacombs(deletedFrames, silent);
        // patch pSpecialsCel - L2S.CEL
        this->patchCatacombsSpec(silent);
        break;
    case DTYPE_CAVES:
        // patch dMiniTiles and dMegaTiles - L3.MIN and L3.TIL
        if (this->min->getSubtileCount() < 560) {
            dProgressErr() << QApplication::tr("Invalid MIN file. Subtile-count is less than %1").arg(560);
            break;
        }
        this->cleanupCaves(deletedFrames, silent);
        break;
    case DTYPE_HELL:
        // patch dMiniTiles and dMegaTiles - L3.MIN and L3.TIL
        if (this->min->getSubtileCount() < 456) {
            dProgressErr() << QApplication::tr("Invalid MIN file. Subtile-count is less than %1").arg(456);
            break;
        }
        this->cleanupHell(deletedFrames, silent);
        break;
    case DTYPE_NEST:
        // patch dMiniTiles and dMegaTiles - L6.MIN and L6.TIL
        if (this->min->getSubtileCount() < 606) {
            dProgressErr() << QApplication::tr("Invalid MIN file. Subtile-count is less than %1").arg(606);
            break;
        }
        this->cleanupNest(deletedFrames, silent);
        break;
    case DTYPE_CRYPT:
        // patch pSpecialsCel - L5S.CEL
        this->patchCryptSpec(silent);
        // patch dMiniTiles and dMegaTiles - L5.MIN and L5.TIL
        if (this->min->getSubtileCount() < 650) {
            dProgressErr() << QApplication::tr("Invalid MIN file. Subtile-count is less than %1").arg(650);
            break;
        }
        this->patchCryptFloor(silent);
        this->maskCryptBlacks(silent);
        this->fixCryptShadows(silent);
        this->cleanupCrypt(deletedFrames, silent);
        break;
    }
    for (auto it = deletedFrames.crbegin(); it != deletedFrames.crend(); it++) {
        unsigned refIndex = *it;
        this->gfx->removeFrame(refIndex - 1, false);
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
