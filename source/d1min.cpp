#include "d1min.h"

#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>

#include "d1image.h"
#include "progressdialog.h"

bool D1Min::load(QString filePath, D1Gfx *g, D1Sol *sol, std::map<unsigned, D1CEL_FRAME_TYPE> &celFrameTypes, const OpenAsParam &params)
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

    const QByteArray fileData = file.readAll();

    // calculate subtileWidth/Height
    unsigned fileSize = fileData.size();
    int subtileCount = sol->getSubtileCount();
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
        }
    }

    // File size check
    int subtileNumberOfCelFrames = width * height;
    if ((fileSize % (subtileNumberOfCelFrames * 2)) != 0) {
        dProgressErr() << tr("Subtile width/height does not align with MIN file.");
        return false;
    }

    bool upscaled = params.upscaled == OPEN_UPSCALED_TYPE::TRUE;
    if (params.upscaled == OPEN_UPSCALED_TYPE::AUTODETECT) {
        upscaled = width != 2;
    }
    g->upscaled = upscaled; // setUpscaled

    this->gfx = g;
    this->subtileWidth = width;
    this->subtileHeight = height;
    int minSubtileCount = fileSize / (subtileNumberOfCelFrames * 2);
    if (minSubtileCount != subtileCount) {
        dProgressWarn() << tr("The size of SOL file does not align with MIN file.");
        // add subtiles to sol if necessary
        while (minSubtileCount > subtileCount) {
            subtileCount++;
            sol->createSubtile();
        }
    }

    // prepare an empty list with zeros
    this->frameReferences.clear();
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
    this->minFilePath = filePath;
    this->modified = !file.isOpen();
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
    }

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    bool upscaled = this->gfx->isUpscaled();
    // update upscaled info -- done by d1celtileset
    // if (params.upscaled != SAVE_UPSCALED_TYPE::AUTODETECT) {
    //    upscaled = params.upscaled == SAVE_UPSCALED_TYPE::TRUE;
    //    this->gfx->setUpscaled(upscaled);
    // }
    // validate the limit of frame-references
    const unsigned limit = upscaled ? (UCHAR_MAX - 1) : ((1 << 12) - 1);
    for (const std::vector<unsigned> &frameReferencesList : this->frameReferences) {
        for (const unsigned frameRef : frameReferencesList) {
            if (frameRef > limit) {
                dProgressFail() << tr("The frame references can not be stored in this format. The limit is %1.").arg(limit);
                return false;
            }
        }
    }

    // write to file
    QDataStream out(&outFile);
    out.setByteOrder(QDataStream::LittleEndian);
    for (unsigned i = 0; i < this->frameReferences.size(); i++) {
        std::vector<unsigned> &frameReferencesList = this->frameReferences[i];
        int subtileNumberOfCelFrames = frameReferencesList.size();
        for (int j = 0; j < subtileNumberOfCelFrames; j++) {
            quint16 writeWord;
            if (!upscaled) {
                writeWord = frameReferencesList[j];
                if (writeWord != 0) {
                    writeWord |= ((quint16)this->gfx->getFrame(writeWord - 1)->getFrameType()) << 12;
                }
            } else {
                int width = this->subtileWidth;
                int idx = subtileNumberOfCelFrames - width - (j / width) * width + j % width;
                writeWord = frameReferencesList[idx];
            }
            out << writeWord;
        }
    }

    this->minFilePath = filePath; // this->load(filePath, subtileCount);
    this->modified = false;

    return true;
}

QImage D1Min::getSubtileImage(int subtileIndex) const
{
    if (subtileIndex < 0 || (unsigned)subtileIndex >= this->frameReferences.size())
        return QImage();

    unsigned subtileWidthPx = this->subtileWidth * MICRO_WIDTH;
    QImage subtile = QImage(subtileWidthPx,
        this->subtileHeight * MICRO_HEIGHT, QImage::Format_ARGB32);
    subtile.fill(Qt::transparent);
    QPainter subtilePainter(&subtile);

    unsigned dx = 0, dy = 0;
    int n = this->subtileWidth * this->subtileHeight;
    for (int i = 0; i < n; i++) {
        unsigned frameRef = this->frameReferences[subtileIndex][i];

        if (frameRef > 0)
            subtilePainter.drawImage(dx, dy,
                this->gfx->getFrameImage(frameRef - 1));

        dx += MICRO_WIDTH;
        if (dx == subtileWidthPx) {
            dx = 0;
            dy += MICRO_HEIGHT;
        }
    }

    subtilePainter.end();
    return subtile;
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

quint16 D1Min::getSubtileWidth() const
{
    return this->subtileWidth;
}

void D1Min::setSubtileWidth(int width)
{
    if (width == 0) {
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
                auto sit = frameReferencesList.begin() + y * width + prevWidth;
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
                auto sit = frameReferencesList.begin() + (y + 1) * width;
                frameReferencesList.erase(sit, sit + diff);
            }
        }
    }
    this->subtileWidth = width;
    this->modified = true;
}

quint16 D1Min::getSubtileHeight() const
{
    return this->subtileHeight;
}

void D1Min::setSubtileHeight(int height)
{
    if (height == 0) {
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
    return true;
}

void D1Min::removeFrame(int frameIndex)
{
    // remove the frame
    this->gfx->removeFrame(frameIndex);
    // shift references
    // - shift frame indices of the subtiles
    unsigned refIndex = frameIndex + 1;
    for (std::vector<unsigned> &frameRefs : this->frameReferences) {
        for (unsigned n = 0; n < frameRefs.size(); n++) {
            if (frameRefs[n] >= refIndex) {
                if (frameRefs[n] == refIndex) {
                    frameRefs[n] = 0;
                } else {
                    frameRefs[n] -= 1;
                }
                this->modified = true;
            }
        }
    }
}

void D1Min::insertSubtile(int subtileIndex, const std::vector<unsigned> &frameReferencesList)
{
    this->frameReferences.insert(this->frameReferences.begin() + subtileIndex, frameReferencesList);
    this->modified = true;
}

void D1Min::createSubtile()
{
    int n = this->subtileWidth * this->subtileHeight;

    this->frameReferences.push_back(std::vector<unsigned>(n));
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
