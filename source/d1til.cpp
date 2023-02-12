#include "d1til.h"

#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>

#include "progressdialog.h"

#define TILE_SIZE (TILE_WIDTH * TILE_HEIGHT)

bool D1Til::load(const QString &filePath, D1Min *m)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.tilFilePath.isEmpty()) {
    //    filePath = params.tilFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
    }

    const QByteArray fileData = file.readAll();

    // File size check
    unsigned fileSize = fileData.size();
    if (fileSize % (2 * TILE_SIZE) != 0) {
        dProgressErr() << tr("Invalid TIL file.");
        return false;
    }

    this->min = m;

    int tileCount = fileSize / (2 * TILE_SIZE);

    // Read TIL binary data
    QDataStream in(fileData);
    in.setByteOrder(QDataStream::LittleEndian);

    this->subtileIndices.clear();
    for (int i = 0; i < tileCount; i++) {
        std::vector<int> subtileIndicesList;
        for (int j = 0; j < TILE_SIZE; j++) {
            quint16 readWord;
            in >> readWord;
            subtileIndicesList.push_back(readWord);
        }
        this->subtileIndices.push_back(subtileIndicesList);
    }
    this->tilFilePath = filePath;
    this->modified = !file.isOpen();
    return true;
}

bool D1Til::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.tilFilePath.isEmpty()) {
        filePath = params.tilFilePath;
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

    // validate the limit of subtile-indices
    const unsigned limit = UINT16_MAX - 1;
    for (const std::vector<int> &subtileIndicesList : this->subtileIndices) {
        for (const int subtileIndex : subtileIndicesList) {
            if (subtileIndex > limit) {
                dProgressFail() << tr("The subtile indices can not be stored in this format. The limit is %1.").arg(limit);
                return false;
            }
        }
    }

    // write to file
    QDataStream out(&outFile);
    out.setByteOrder(QDataStream::LittleEndian);
    for (const std::vector<int> &subtileIndicesList : this->subtileIndices) {
        for (int j = 0; j < TILE_SIZE; j++) {
            quint16 writeWord = subtileIndicesList[j];
            out << writeWord;
        }
    }

    this->tilFilePath = filePath; // this->load(filePath);
    this->modified = false;

    return true;
}

QImage D1Til::getTileImage(int tileIndex) const
{
    if (tileIndex < 0 || (unsigned)tileIndex >= this->subtileIndices.size())
        return QImage();

    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;
    // assert(TILE_WIDTH == 2 &&  TILE_HEIGHT == 2);
    unsigned subtileShiftY = subtileWidth / 4;
    QImage tile = QImage(subtileWidth * 2,
        subtileHeight + 2 * subtileShiftY, QImage::Format_ARGB32);
    tile.fill(Qt::transparent);
    QPainter tilePainter(&tile);
    //      0
    //    2   1
    //      3
    tilePainter.drawImage(subtileWidth / 2, 0,
        this->min->getSubtileImage(
            this->subtileIndices.at(tileIndex).at(0)));

    tilePainter.drawImage(subtileWidth, subtileShiftY,
        this->min->getSubtileImage(
            this->subtileIndices.at(tileIndex).at(1)));

    tilePainter.drawImage(0, subtileShiftY,
        this->min->getSubtileImage(
            this->subtileIndices.at(tileIndex).at(2)));

    tilePainter.drawImage(subtileWidth / 2, 2 * subtileShiftY,
        this->min->getSubtileImage(
            this->subtileIndices.at(tileIndex).at(3)));

    tilePainter.end();
    return tile;
}

QImage D1Til::getFlatTileImage(int tileIndex) const
{
    if (tileIndex < 0 || (unsigned)tileIndex >= this->subtileIndices.size())
        return QImage();

    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;
    QImage tile = QImage(subtileWidth * TILE_SIZE, subtileHeight, QImage::Format_ARGB32);
    tile.fill(Qt::transparent);
    QPainter tilePainter(&tile);

    for (int i = 0; i < TILE_SIZE; i++) {
        tilePainter.drawImage(subtileWidth * i, 0,
            this->min->getSubtileImage(
                this->subtileIndices.at(tileIndex).at(i)));
    }

    tilePainter.end();
    return tile;
}

QString D1Til::getFilePath() const
{
    return this->tilFilePath;
}

const D1Min *D1Til::getMin() const
{
    return this->min;
}

bool D1Til::isModified() const
{
    return this->modified;
}

void D1Til::setModified()
{
    this->modified = true;
}

int D1Til::getTileCount() const
{
    return this->subtileIndices.size();
}

std::vector<int> &D1Til::getSubtileIndices(int tileIndex) const
{
    return const_cast<std::vector<int> &>(this->subtileIndices.at(tileIndex));
}

bool D1Til::setSubtileIndex(int tileIndex, int index, int subtileIndex)
{
    if (this->subtileIndices[tileIndex][index] == subtileIndex) {
        return false;
    }
    this->subtileIndices[tileIndex][index] = subtileIndex;
    this->modified = true;
    return true;
}

void D1Til::insertTile(int tileIndex, const std::vector<int> &subtileIndices)
{
    this->subtileIndices.insert(this->subtileIndices.begin() + tileIndex, subtileIndices);
    this->modified = true;
}

void D1Til::createTile()
{
    this->subtileIndices.push_back(std::vector<int>(TILE_SIZE));
    this->modified = true;
}

void D1Til::removeTile(int tileIndex)
{
    this->subtileIndices.erase(this->subtileIndices.begin() + tileIndex);
    this->modified = true;
}
