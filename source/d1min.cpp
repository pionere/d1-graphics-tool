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

    QByteArray fileData = file.readAll();
    QBuffer fileBuffer(&fileData);

    if (!fileBuffer.open(QIODevice::ReadOnly)) {
        return false;
    }

    // calculate subtileWidth/Height
    auto fileSize = file.size();
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
        QList<quint16> frameReferencesList;
        for (int j = 0; j < subtileNumberOfCelFrames; j++) {
            frameReferencesList.append(0);
        }
        this->frameReferences.append(frameReferencesList);
    }

    // Read MIN binary data
    QDataStream in(&fileBuffer);
    in.setByteOrder(QDataStream::LittleEndian);

    for (int i = 0; i < minSubtileCount; i++) {
        QList<quint16> &frameReferencesList = this->frameReferences[i];
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
            dProgressWarn() << tr("Empty subtiles are added to the saved document to match the required height of the game (DevilutionX).");
        }
    }

    // write to file
    QDataStream out(&outFile);
    out.setByteOrder(QDataStream::LittleEndian);
    for (int i = 0; i < this->frameReferences.size(); i++) {
        QList<quint16> &frameReferencesList = this->frameReferences[i];
        int subtileNumberOfCelFrames = frameReferencesList.count();
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

QImage D1Min::getSubtileImage(int subtileIndex) const
{
    if (subtileIndex < 0 || subtileIndex >= this->frameReferences.size())
        return QImage();

    unsigned subtileWidthPx = this->subtileWidth * MICRO_WIDTH;
    QImage subtile = QImage(subtileWidthPx,
        this->subtileHeight * MICRO_HEIGHT, QImage::Format_ARGB32);
    subtile.fill(Qt::transparent);
    QPainter subtilePainter(&subtile);

    unsigned dx = 0, dy = 0;
    int n = this->subtileWidth * this->subtileHeight;
    for (int i = 0; i < n; i++) {
        quint16 frameRef = this->frameReferences.at(subtileIndex).at(i);

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
    return this->frameReferences.count();
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
        for (int i = 0; i < this->frameReferences.size(); i++) {
            QList<quint16> &frameReferencesList = this->frameReferences[i];
            for (int y = 0; y < this->subtileHeight; y++) {
                for (int dx = 0; dx < diff; dx++) {
                    frameReferencesList.insert(y * width + prevWidth, 0);
                }
            }
        }
    } else if (diff < 0) {
        diff = -diff;
        // check if there is a non-zero frame in the subtiles
        bool hasFrame = false;
        for (int i = 0; i < this->frameReferences.size() && !hasFrame; i++) {
            QList<quint16> &frameReferencesList = this->frameReferences[i];
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
        for (int i = 0; i < this->frameReferences.size(); i++) {
            QList<quint16> &frameReferencesList = this->frameReferences[i];
            for (int y = 0; y < this->subtileHeight; y++) {
                for (int dx = 0; dx < diff; dx++) {
                    frameReferencesList.takeAt((y + 1) * width);
                }
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
        for (int i = 0; i < this->frameReferences.size(); i++) {
            QList<quint16> &frameReferencesList = this->frameReferences[i];
            for (int j = 0; j < n; j++) {
                frameReferencesList.push_front(0);
            }
        }
    } else if (diff < 0) {
        diff = -diff;
        // check if there is a non-zero frame in the subtiles
        bool hasFrame = false;
        int n = diff * width;
        for (int i = 0; i < this->frameReferences.size() && !hasFrame; i++) {
            QList<quint16> &frameReferencesList = this->frameReferences[i];
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
        for (int i = 0; i < this->frameReferences.size(); i++) {
            QList<quint16> &frameReferencesList = this->frameReferences[i];
            frameReferencesList.erase(frameReferencesList.begin(), frameReferencesList.begin() + n);
        }
    }
    this->subtileHeight = height;
    this->modified = true;
}

QList<quint16> &D1Min::getFrameReferences(int subtileIndex) const
{
    return const_cast<QList<quint16> &>(this->frameReferences.at(subtileIndex));
}

bool D1Min::setFrameReference(int subtileIndex, int index, int frameRef)
{
    if (this->frameReferences[subtileIndex][index] == frameRef) {
        return false;
    }
    this->frameReferences[subtileIndex][index] = frameRef;
    return true;
}

void D1Min::insertSubtile(int subtileIndex, const QList<quint16> &frameReferencesList)
{
    this->frameReferences.insert(subtileIndex, frameReferencesList);
    this->modified = true;
}

void D1Min::createSubtile()
{
    QList<quint16> frameReferencesList;
    int n = this->subtileWidth * this->subtileHeight;

    for (int i = 0; i < n; i++) {
        frameReferencesList.append(0);
    }
    this->frameReferences.append(frameReferencesList);
    this->modified = true;
}

void D1Min::removeSubtile(int subtileIndex)
{
    this->frameReferences.removeAt(subtileIndex);
    this->modified = true;
}

void D1Min::remapSubtiles(const QMap<unsigned, unsigned> &remap)
{
    QList<QList<quint16>> newFrameReferences;

    for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
        newFrameReferences.append(this->frameReferences.at(iter.value()));
    }
    this->frameReferences.swap(newFrameReferences);
    this->modified = true;
}
