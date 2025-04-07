#include "d1min.h"

#include <QApplication>
#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>

#include "d1image.h"
#include "d1tileset.h"
#include "progressdialog.h"

/*
 * Load a MIN file based on the SLA file. Adjusts gfx + sla if necessary.
 */
bool D1Min::load(const QString &filePath, D1Tileset *ts, std::map<unsigned, D1CEL_FRAME_TYPE> &celFrameTypes, const OpenAsParam &params)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.minFilePath.isEmpty()) {
    //    filePath = params.minFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
    }

    this->clear();
    this->minFilePath = filePath;
    this->tileset = ts;

    bool changed = !file.isOpen();

    const QByteArray fileData = file.readAll();

    // calculate subtileWidth/Height
    unsigned fileSize = fileData.size();
    int subtileCount = this->tileset->sla->getSubtileCount();
    int width = params.minWidth;
    if (width == 0) {
        width = 2;
    }
    int height = params.minHeight;
    if (height == 0) {
        if (fileSize == 0 || subtileCount == 0) {
            height = 5;
        } else {
            // guess subtileHeight based on the data
            height = fileSize / (subtileCount * width * 2);
            if (height > 8 && params.minWidth == 0) {
                // height is too much -> check if it is a standard upscaled tileset with padding
                int multiplier = sqrt(fileSize / (8 * 2 * 2 * subtileCount)); // assume padding to (8 * multiplier)
                int upHeight = 8 * multiplier;
                int upWidth = 2 * multiplier;
                if (upWidth * upHeight * 2 * subtileCount == fileSize) {
                    width = upWidth;
                    height = upHeight;
                }
            }
        }
    }

    // File size check
    int subtileNumberOfCelFrames = width * height; // TODO: check overflow
    if (subtileNumberOfCelFrames == 0 || (fileSize % (subtileNumberOfCelFrames * 2)) != 0) {
        dProgressErr() << tr("Subtile width/height does not align with MIN file. (SubtileCount:%1, W/H:%2/%3, FileSize:%4)").arg(subtileCount).arg(width).arg(height).arg(fileSize);
        return false;
    }

    bool upscaled = params.upscaled == OPEN_UPSCALED_TYPE::TRUE;
    if (params.upscaled == OPEN_UPSCALED_TYPE::AUTODETECT) {
        upscaled = width != 2;
    }
    this->tileset->gfx->upscaled = upscaled; // setUpscaled

    this->subtileWidth = width;
    this->subtileHeight = height;
    int minSubtileCount = fileSize / (subtileNumberOfCelFrames * 2);
    if (minSubtileCount != subtileCount) {
        // warn about misalignment if the files are not empty
        if (minSubtileCount != 0 && subtileCount != 0) {
            dProgressWarn() << tr("The size of SLA file does not align with MIN file.");
        }
        // add subtiles to sla if necessary
        while (minSubtileCount > subtileCount) {
            this->tileset->sla->insertSubtile(subtileCount);
            subtileCount++;
        }
        if (minSubtileCount < subtileCount) {
            changed = true;
        }
    }

    // prepare an empty list with zeros
    for (int i = 0; i < subtileCount; i++) {
        this->frameReferences.push_back(std::vector<unsigned>(subtileNumberOfCelFrames));
    }

    // Read MIN binary data
    QDataStream in(fileData);
    in.setByteOrder(QDataStream::LittleEndian);

    for (int i = 0; i < minSubtileCount; i++) {
        std::vector<unsigned> &frameReferencesList = this->frameReferences[i];
        for (int j = 0; j < subtileNumberOfCelFrames; j++) {
            quint16 readWord;
            in >> readWord;
            quint16 id = readWord;
            if (!upscaled) {
                id &= 0x0FFF;
                celFrameTypes[id] = static_cast<D1CEL_FRAME_TYPE>((readWord & 0x7000) >> 12);
                frameReferencesList[j] = id;
            } else {
                int idx = subtileNumberOfCelFrames - width - (j / width) * width + j % width;
                frameReferencesList[idx] = id;
            }
        }
    }
    this->modified = changed;
    return true;
}

bool D1Min::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.minFilePath.isEmpty()) {
        filePath = params.minFilePath;
        if (!params.autoOverwrite && QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to overwrite %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    } else if (!this->isModified()) {
        return false;
    }

    if (filePath.isEmpty()) {
        return false;
    }

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    bool upscaled = this->tileset->gfx->isUpscaled();
    // update upscaled info -- done by d1celtileset
    // if (params.upscaled != SAVE_UPSCALED_TYPE::AUTODETECT) {
    //    upscaled = params.upscaled == SAVE_UPSCALED_TYPE::TRUE;
    //    this->gfx->setUpscaled(upscaled);
    // }
    // validate the limit of frame-references
    const unsigned limit = upscaled ? UINT16_MAX : ((1 << 12) - 1);
    for (const std::vector<unsigned> &frameReferencesList : this->frameReferences) {
        for (const unsigned frameRef : frameReferencesList) {
            if (frameRef > limit) {
                dProgressFail() << tr("The frame references can not be stored in this format (MIN). The limit is %1.").arg(limit);
                return false;
            }
        }
    }
    int padding = 0;
    if (!upscaled) {
        if (this->subtileHeight != 5 && this->subtileHeight != 8) {
            dProgressWarn() << tr("Subtile height is not supported by the game (Diablo 1/DevilutionX).");
        }
    } else {
        padding = (8 * this->subtileWidth / 2 - this->subtileHeight) * this->subtileWidth;
        if (padding < 0) {
            dProgressWarn() << tr("Subtile height is not supported by the game (Diablo 1/DevilutionX).");
        } else if (padding > 0) {
            dProgressWarn() << tr("Empty frames are added to the saved document to match the required height of the game (DevilutionX).");
        }
    }

    // write to file
    QDataStream out(&outFile);
    out.setByteOrder(QDataStream::LittleEndian);
    const D1Gfx *gfx = this->tileset->gfx;
    for (unsigned i = 0; i < this->frameReferences.size(); i++) {
        std::vector<unsigned> &frameReferencesList = this->frameReferences[i];
        int subtileNumberOfCelFrames = frameReferencesList.size();
        for (int j = 0; j < subtileNumberOfCelFrames; j++) {
            quint16 writeWord;
            if (!upscaled) {
                writeWord = frameReferencesList[j];
                if (writeWord != 0) {
                    writeWord |= ((quint16)gfx->getFrame(writeWord - 1)->getFrameType()) << 12;
                }
            } else {
                int width = this->subtileWidth;
                int idx = subtileNumberOfCelFrames - width - (j / width) * width + j % width;
                writeWord = frameReferencesList[idx];
            }
            out << writeWord;
        }
        // pad with empty micros till this->subtileWidth * 8 * this->subtileWidth / 2
        quint16 writeWord = 0;
        for (int j = 0; j < padding; j++) {
            out << writeWord;
        }
    }

    this->minFilePath = filePath; // this->load(filePath, subtileCount);
    this->modified = false;

    return true;
}

void D1Min::clear()
{
    this->frameReferences.clear();
    this->subtileWidth = 2;
    this->subtileHeight = 5;
    // this->tileset = nullptr;
    this->modified = true;
}

void D1Min::compareTo(const D1Min *min, const std::map<unsigned, D1CEL_FRAME_TYPE> &celFrameTypes) const
{
    if (!min->tileset->gfx->isUpscaled()) {
        for (int i = 0; i < this->tileset->gfx->getFrameCount(); i++) {
            /*if (celFrameTypes.count(i) == 0) {
                continue;
            }
            D1CEL_FRAME_TYPE frameType = celFrameTypes[i];*/
            auto it = celFrameTypes.find(i + 1);
            if (it == celFrameTypes.end())
                continue;
            D1CEL_FRAME_TYPE frameType = it->second;
            D1CEL_FRAME_TYPE myFrameType = this->tileset->gfx->getFrame(i)->getFrameType();
            if (myFrameType != frameType) {
                dProgress() << tr("The type of frame %1 is %2 (was %3)").arg(i + 1).arg(D1GfxFrame::frameTypeToStr(myFrameType)).arg(D1GfxFrame::frameTypeToStr(frameType));
            }
        }
    }

    int width = min->subtileWidth;
    if (this->subtileWidth != width) {
        dProgress() << tr("Subtile-width is %1 (was %2)").arg(this->subtileWidth).arg(width);
        return;
    }
    int height = min->subtileHeight;
    int myHeight = this->subtileHeight;
    if (myHeight != height) {
        dProgress() << tr("Subtile-height is %1 (was %2)").arg(myHeight).arg(height);
        height = std::min(myHeight, height);
    }

    unsigned frameCount = min->frameReferences.size();
    unsigned myFrameCount = this->frameReferences.size();
    if (myFrameCount != frameCount) {
        dProgress() << tr("Number of subtiles is %1 (was %2)").arg(myFrameCount).arg(frameCount);
        frameCount = std::min(myFrameCount, frameCount);
    }

    for (unsigned i = 0; i < frameCount; i++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned index = y * width + x;
                unsigned micro = min->frameReferences[i][index];
                unsigned myMicro = this->frameReferences[i][index];
                if (myMicro != micro) {
                    dProgress() << tr("The micro :%1: of subtile %2 is %3 (was %4)").arg(x + y * width).arg(i + 1).arg(myMicro).arg(micro);
                }
            }
        }
    }
}

QImage D1Min::getSubtileImage(int subtileIndex) const
{
    if (subtileIndex < 0 || (unsigned)subtileIndex >= this->frameReferences.size()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Invalid subtile %1 requested. Frame-references count: %2").arg(subtileIndex).arg(this->frameReferences.size()));
#endif
        return QImage();
    }

    unsigned subtileWidthPx = this->subtileWidth * MICRO_WIDTH;
    unsigned subtileHeightPx = this->subtileHeight * MICRO_HEIGHT;
    QImage subtile = QImage(subtileWidthPx, subtileHeightPx, QImage::Format_ARGB32_Premultiplied); // QImage::Format_ARGB32
    subtile.fill(Qt::transparent);
    QPainter subtilePainter(&subtile);

    unsigned dx = 0, dy = 0;
    const std::vector<unsigned> &frameRefs = this->frameReferences[subtileIndex];
    const D1Gfx *gfx = this->tileset->gfx;
    for (unsigned frameRef : frameRefs) {
        if (frameRef > 0) {
            subtilePainter.drawImage(dx, dy, gfx->getFrameImage(frameRef - 1));
        }

        dx += MICRO_WIDTH;
        if (dx == subtileWidthPx) {
            dx = 0;
            dy += MICRO_HEIGHT;
        }
    }

    return subtile;
}

QImage D1Min::getSpecSubtileImage(int subtileIndex) const
{
    QImage subtile = this->getSubtileImage(subtileIndex);

    unsigned specRef = this->tileset->sla->getSpecProperty(subtileIndex);
    if (specRef != 0 && (unsigned)this->tileset->cls->getFrameCount() >= specRef) {
        QImage specImage = this->tileset->cls->getFrameImage(specRef - 1);
        QPainter subtilePainter(&subtile);
        subtilePainter.drawImage(0, subtile.height() - specImage.height(), specImage);
    }

    return subtile;
}

QImage D1Min::getFloorImage(int subtileIndex) const
{
    if (subtileIndex < 0 || (unsigned)subtileIndex >= this->frameReferences.size()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Invalid subtile %1 requested. Frame-references count: %2").arg(subtileIndex).arg(this->frameReferences.size()));
#endif
        return QImage();
    }

    unsigned width = this->subtileWidth;
    unsigned n = width * width / 2;
    const std::vector<unsigned> &frameRefs = this->frameReferences[subtileIndex];
    unsigned numFrame = frameRefs.size();
    if ((width & 1) || numFrame < n) {
        return QImage();
    }
    unsigned firstFrame = numFrame - n;
    unsigned subtileWidthPx = width * MICRO_WIDTH;
    unsigned subtileHeightPx = (width / 2) * MICRO_HEIGHT;
    QImage subtile = QImage(subtileWidthPx, subtileHeightPx, QImage::Format_ARGB32_Premultiplied); // QImage::Format_ARGB32
    subtile.fill(Qt::transparent);
    QPainter subtilePainter(&subtile);

    unsigned dx = 0, dy = 0;
    const D1Gfx *gfx = this->tileset->gfx;
    for (unsigned i = firstFrame; i < numFrame; i++) {
        unsigned frameRef = frameRefs[i];

        if (frameRef > 0) {
            subtilePainter.drawImage(dx, dy, gfx->getFrameImage(frameRef - 1));
        }

        dx += MICRO_WIDTH;
        if (dx == subtileWidthPx) {
            dx = 0;
            dy += MICRO_HEIGHT;
        }
    }

    return subtile;
}

std::vector<std::vector<D1GfxPixel>> D1Min::getSubtilePixelImage(int subtileIndex) const
{
    unsigned subtileWidthPx = this->subtileWidth * MICRO_WIDTH;
    unsigned subtileHeightPx = this->subtileHeight * MICRO_HEIGHT;

    std::vector<std::vector<D1GfxPixel>> subtile;
    D1PixelImage::createImage(subtile, subtileWidthPx, subtileHeightPx);

    unsigned dx = 0, dy = 0;
    const std::vector<unsigned> &frameRefs = this->frameReferences[subtileIndex];
    const D1Gfx *gfx = this->tileset->gfx;
    for (unsigned frameRef : frameRefs) {
        if (frameRef > 0) {
            D1PixelImage::drawImage(subtile, dx, dy, gfx->getFramePixelImage(frameRef - 1));
        }

        dx += MICRO_WIDTH;
        if (dx == subtileWidthPx) {
            dx = 0;
            dy += MICRO_HEIGHT;
        }
    }

    return subtile;
}

void D1Min::getFrameUses(std::vector<bool> &frameUses) const
{
    frameUses.resize(this->tileset->gfx->getFrameCount());
    for (const std::vector<unsigned> &frameRefs : this->frameReferences) {
        for (unsigned frameRef : frameRefs) {
            if (frameRef != 0) {
                frameUses[frameRef - 1] = true;
            }
        }
    }
}

QString D1Min::getFilePath() const
{
    return this->minFilePath;
}

bool D1Min::isModified() const
{
    return this->modified;
}

void D1Min::setModified()
{
    this->modified = true;
}

int D1Min::getSubtileCount() const
{
    return this->frameReferences.size();
}

int D1Min::getSubtileWidth() const
{
    return this->subtileWidth;
}

void D1Min::setSubtileWidth(int width)
{
    if (width == 0) { // TODO: check overflow
        return;
    }
    int prevWidth = this->subtileWidth;
    int diff = width - prevWidth;
    if (diff == 0) {
        return;
    }
    if (diff > 0) {
        // extend the subtile-width
        for (unsigned i = 0; i < this->frameReferences.size(); i++) {
            std::vector<unsigned> &frameReferencesList = this->frameReferences[i];
            for (int y = 0; y < this->subtileHeight; y++) {
                unsigned idx = y * width + prevWidth;
                auto sit = frameReferencesList.begin() + idx;
                frameReferencesList.insert(sit, diff, 0);
            }
        }
    } else if (diff < 0) {
        diff = -diff;
        // check if there is a non-zero frame in the subtiles
        bool hasFrame = false;
        for (unsigned i = 0; i < this->frameReferences.size() && !hasFrame; i++) {
            std::vector<unsigned> &frameReferencesList = this->frameReferences[i];
            for (int y = 0; y < this->subtileHeight; y++) {
                for (int x = width; x < prevWidth; x++) {
                    if (frameReferencesList[y * prevWidth + x] != 0) {
                        hasFrame = true;
                        break;
                    }
                }
            }
        }
        if (hasFrame) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Non-transparent frames are going to be eliminited. Are you sure you want to proceed?"), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return;
            }
        }
        // reduce the subtile-width
        for (unsigned i = 0; i < this->frameReferences.size(); i++) {
            std::vector<unsigned> &frameReferencesList = this->frameReferences[i];
            for (int y = 0; y < this->subtileHeight; y++) {
                unsigned idx = (y + 1) * width;
                auto sit = frameReferencesList.begin() + idx;
                frameReferencesList.erase(sit, sit + diff);
            }
        }
    }
    this->subtileWidth = width;
    this->modified = true;
}

int D1Min::getSubtileHeight() const
{
    return this->subtileHeight;
}

void D1Min::setSubtileHeight(int height)
{
    if (height == 0) { // TODO: check overflow
        return;
    }
    int width = this->subtileWidth;
    int diff = height - this->subtileHeight;
    if (diff == 0) {
        return;
    }
    if (diff > 0) {
        // extend the subtile-height
        int n = diff * width;
        for (unsigned i = 0; i < this->frameReferences.size(); i++) {
            std::vector<unsigned> &frameReferencesList = this->frameReferences[i];
            frameReferencesList.insert(frameReferencesList.begin(), n, 0);
        }
    } else if (diff < 0) {
        diff = -diff;
        // check if there is a non-zero frame in the subtiles
        bool hasFrame = false;
        int n = diff * width;
        for (unsigned i = 0; i < this->frameReferences.size() && !hasFrame; i++) {
            std::vector<unsigned> &frameReferencesList = this->frameReferences[i];
            for (int j = 0; j < n; j++) {
                if (frameReferencesList[j] != 0) {
                    hasFrame = true;
                    break;
                }
            }
        }
        if (hasFrame) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Non-transparent frames are going to be eliminited. Are you sure you want to proceed?"), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return;
            }
        }
        // reduce the subtile-height
        for (unsigned i = 0; i < this->frameReferences.size(); i++) {
            std::vector<unsigned> &frameReferencesList = this->frameReferences[i];
            frameReferencesList.erase(frameReferencesList.begin(), frameReferencesList.begin() + n);
        }
    }
    this->subtileHeight = height;
    this->modified = true;
}

std::vector<unsigned> &D1Min::getFrameReferences(int subtileIndex) const
{
    return const_cast<std::vector<unsigned> &>(this->frameReferences.at(subtileIndex));
}

bool D1Min::setFrameReference(int subtileIndex, int index, unsigned frameRef)
{
    if (this->frameReferences[subtileIndex][index] == frameRef) {
        return false;
    }
    this->frameReferences[subtileIndex][index] = frameRef;
    this->modified = true;
    return true;
}

void D1Min::insertSubtile(int subtileIndex, const std::vector<unsigned> &frameReferencesList)
{
    this->frameReferences.insert(this->frameReferences.begin() + subtileIndex, frameReferencesList);
    this->modified = true;
}

void D1Min::removeSubtile(int subtileIndex)
{
    this->frameReferences.erase(this->frameReferences.begin() + subtileIndex);
    this->modified = true;
}

void D1Min::remapSubtiles(const std::map<unsigned, unsigned> &remap)
{
    std::vector<std::vector<unsigned>> newFrameReferences;

    for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
        newFrameReferences.push_back(this->frameReferences.at(iter->second));
    }
    this->frameReferences.swap(newFrameReferences);
    this->modified = true;
}
