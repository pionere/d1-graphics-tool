#include "d1dun.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "d1til.h"
#include "d1tmi.h"
#include "progressdialog.h"

#define UNDEF_SUBTILE -1
#define UNDEF_TILE -1

bool D1Dun::load(const QString &filePath, D1Til *t, D1Tmi *m, const OpenAsParam &params)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.dunFilePath.isEmpty()) {
    //    filePath = params.dunFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly) && !params.ampFilePath.isEmpty()) {
            return false; // report read-error only if the file was explicitly requested
        }
    }

    this->til = t;
    this->tmi = m;

    const QByteArray fileData = file.readAll();

    // File size check
    unsigned fileSize = fileData.size();
    int dunWidth = 0;
    int dunHeight = 0;
    bool changed = fileSize == 0; // !file.isOpen();
    if (fileSize != 0) {
        if (fileSize < 2 * 2) {
            dProgressErr() << tr("Invalid DUN file.");
            return false;
        }

        // Read AMP binary data
        QDataStream in(fileData);
        in.setByteOrder(QDataStream::LittleEndian);

        quint16 readWord;
        in >> readWord;
        dunWidth = readWord;
        in >> readWord;
        dunHeight = readWord;

        // prepare the vectors
        tiles.resize(dunHeight);
        for (int i = 0; i < dunHeight; i++) {
            tiles[i].resize(dunWidth);
        }
        subtiles.resize(dunHeight * TILE_HEIGHT);
        for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
            subtiles[i].resize(dunWidth * TILE_WIDTH);
        }
        items.resize(dunHeight * TILE_HEIGHT);
        for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
            items[i].resize(dunWidth * TILE_WIDTH);
        }
        objects.resize(dunHeight * TILE_HEIGHT);
        for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
            objects[i].resize(dunWidth * TILE_WIDTH);
        }
        monsters.resize(dunHeight * TILE_HEIGHT);
        for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
            monsters[i].resize(dunWidth * TILE_WIDTH);
        }
        transvals.resize(dunHeight * TILE_HEIGHT);
        for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
            transvals[i].resize(dunWidth * TILE_WIDTH);
        }

        unsigned tileCount = dunWidth * dunHeight;
        if (fileSize < 2 * 2 + tileCount * 2) {
            dProgressErr() << tr("Invalid DUN header.");
            return false;
        }
        // read tiles
        for (int y = 0; y < dunHeight; y++) {
            for (int x = 0; x < dunWidth; x++) {
                in >> readWord;
                this->tiles[y][x] = readWord;
            }
        }

        if (fileSize > 2 * 2 + tileCount * 2) {
            unsigned subtileCount = tileCount * TILE_WIDTH * TILE_HEIGHT;
            if (fileSize != 2 * 2 + tileCount * 2 + subtileCount * 2 * 4) {
                dProgressWarn() << tr("Invalid DUN content.");
                changed = true;
            }
            if (fileSize >= 2 * 2 + tileCount * 2 + subtileCount * 2 * 4) {
                // read items
                for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                    for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                        in >> readWord;
                        this->items[y][x] = readWord;
                    }
                }

                // read monsters
                for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                    for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                        in >> readWord;
                        this->monsters[y][x] = readWord;
                    }
                }

                // read objects
                for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                    for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                        in >> readWord;
                        this->objects[y][x] = readWord;
                    }
                }

                // read transval
                for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                    for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                        in >> readWord;
                        this->transvals[y][x] = readWord;
                    }
                }
            }
        }

        // prepare subtiles
        for (int y = 0; y < dunHeight; y++) {
            for (int x = 0; x < dunWidth; x++) {
                this->updateSubtiles(posx, posy, this->tiles[y][x]);
            }
        }
    }

    this->width = dunWidth * TILE_WIDTH;
    this->height = dunHeight * TILE_HEIGHT;
    this->dunFilePath = filePath;
    this->modified = changed;
    return true;
}

bool D1Dun::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.dunFilePath.isEmpty()) {
        filePath = params.dunFilePath;
        if (!params.autoOverwrite && QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to overwrite %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    }

    // validate tiles
    int dunWidth = this->width / TILE_WIDTH;
    int dunHeight = this->height / TILE_HEIGHT;
    for (int y = 0; y < dunHeight; y++) {
        for (int x = 0; x < dunWidth; x++) {
            if (this->tiles[y][x] == UNDEF_TILE) {
                dProgressFail() << tr("Undefined tiles (one at %1:%2) can not be saved in this format (DUN).").arg(x).arg(y);
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

    // write to file
    QDataStream out(&outFile);
    out.setByteOrder(QDataStream::LittleEndian);

    quint16 writeWord;
    writeWord = dunWidth;
    out << writeWord;
    writeWord = dunHeight;
    out << writeWord;

    // write tiles
    for (int y = 0; y < dunHeight; y++) {
        for (int x = 0; x < dunWidth; x++) {
            writeWord = this->tiles[y][x];
            out << writeWord;
        }
    }

    // write items
    for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
        for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
            writeWord = this->items[y][x];
            out << writeWord;
        }
    }

    // write monsters
    for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
        for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
            writeWord = this->monsters[y][x];
            out << writeWord;
        }
    }

    // write objects
    for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
        for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
            writeWord = this->objects[y][x];
            out << writeWord;
        }
    }

    // write transval
    for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
        for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
            writeWord = this->transvals[y][x];
            out << writeWord;
        }
    }

    this->dunFilePath = filePath; // this->load(filePath, allocate);
    this->modified = false;

    return true;
}

QString D1Dun::getFilePath() const
{
    return this->dunFilePath;
}

bool D1Dun::isModified() const
{
    return this->modified;
}

int D1Dun::getWidth() const
{
    return this->width;
}

bool D1Dun::setWidth(int newWidth)
{
    if (newWidth % TILE_WIDTH != 0) {
        return false;
    }
    if (newWidth <= 0) { // TODO: check overflow
        return false;
    }
    int height = this->height;
    int prevWidth = this->width;
    int diff = newWidth - prevWidth;
    if (diff == 0) {
        return false;
    }
    if (diff < 0) {
        // check if there are non-zero values
        bool hasContent = false;
        for (int y = 0; y < height; y++) {
            for (int x = newWidth; x < prevWidth; x++) {
                hasContent |= this->tiles[y / TILE_HEIGHT][x / TILE_WIDTH] > 0; // !0 && !UNDEF_TILE
                hasContent |= this->subtiles[y][x] > 0;                         // !0 && !UNDEF_SUBTILE
                hasContent |= this->items[y][x] != 0;
                hasContent |= this->monsters[y][x] != 0;
                hasContent |= this->objects[y][x] != 0;
                hasContent |= this->transvals[y][x] != 0;
            }
        }

        if (hasContent) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Some content are going to be eliminited. Are you sure you want to proceed?"), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    }

    // resize the dungeon
    for (std::vector<int> &tilesRow : this->tiles) {
        tilesRow.resize(newWidth / TILE_WIDTH);
    }
    for (std::vector<int> &itemsRow : this->items) {
        itemsRow.resize(newWidth);
    }
    for (std::vector<int> &monsRow : this->monsters) {
        monsRow.resize(newWidth);
    }
    for (std::vector<int> &objsRow : this->objects) {
        objsRow.resize(newWidth);
    }
    for (std::vector<int> &transRow : this->transvals) {
        transRow.resize(newWidth);
    }

    this->width = newWidth;
    this->modified = true;
    return true;
}

int D1Dun::getHeight() const
{
    return this->height;
}

bool D1Dun::setHeight(int newHeight)
{
    if (newHeight % TILE_HEIGHT != 0) {
        return false;
    }
    if (newHeight <= 0) { // TODO: check overflow
        return false;
    }
    int width = this->width;
    int prevHeight = this->height;
    int diff = newHeight - prevHeight;
    if (diff == 0) {
        return false;
    }
    if (diff < 0) {
        // check if there are non-zero values
        bool hasContent = false;
        for (int y = newHeight; y < prevHeight; y++) {
            for (int x = 0; x < width; x++) {
                hasContent |= this->tiles[y / TILE_HEIGHT][x / TILE_WIDTH] > 0; // !0 && !UNDEF_TILE
                hasContent |= this->subtiles[y][x] > 0;                         // !0 && !UNDEF_SUBTILE
                hasContent |= this->items[y][x] != 0;
                hasContent |= this->monsters[y][x] != 0;
                hasContent |= this->objects[y][x] != 0;
                hasContent |= this->transvals[y][x] != 0;
            }
        }

        if (hasContent) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Some content are going to be eliminited. Are you sure you want to proceed?"), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    }

    // resize the dungeon
    this->tiles.resize(newHeight / TILE_HEIGHT);
    this->subtiles.resize(newHeight);
    this->items.resize(newHeight);
    this->monsters.resize(newHeight);
    this->objects.resize(newHeight);
    this->transvals.resize(newHeight);
    for (int y = prevHeight; y < newHeight; y++) {
        this->tiles[y].resize(width / TILE_WIDTH);
        this->subtiles[y].resize(width);
        this->items[y].resize(width);
        this->monsters[y].resize(width);
        this->objects[y].resize(width);
        this->transvals[y].resize(width);
    }

    this->height = newHeight;
    this->modified = true;
    return true;
}

int D1Dun::getTileAt(int posx, int posy) const
{
    return this->tiles[posy / TILE_HEIGHT][posx / TILE_WIDTH];
}

bool D1Dun::setTileAt(int posx, int posy, int tileRef)
{
    if (tileRef < 0) {
        tileRef = UNDEF_TILE;
    }
    if (this->tiles[posy / TILE_HEIGHT][posx / TILE_WIDTH] == tileRef) {
        return false;
    }
    this->tiles[posy / TILE_HEIGHT][posx / TILE_WIDTH] = tileRef;
    // update subtile values
    this->updateSubtiles(posx, posy, tileRef);
    this->modified = true;
    return true;
}

int D1Dun::getSubtileAt(int posx, int posy) const
{
    return this->subtiles[posy][posx];
}

bool D1Dun::setSubtileAt(int posx, int posy, int subtileRef)
{
    if (subtileRef < 0) {
        subtileRef = UNDEF_SUBTILE;
    }
    if (this->subtiles[posy][posx] == subtileRef) {
        return false;
    }
    this->subtiles[posy][posx] = subtileRef;
    // update tile value
    int tileRef = 0;
    for (int y = 0; y < TILE_HEIGHT; y++) {
        for (int x = 0; x < TILE_WIDTH; x++) {
            if (this->subtiles[posy + y][posx + x] != 0) {
                tileRef = UNDEF_TILE;
            }
        }
    }

    this->tiles[posy / TILE_HEIGHT][posx / TILE_WIDTH] = tileRef;
    this->modified = true;
    return true;
}

int D1Dun::getItemAt(int posx, int posy) const
{
    return this->items[posy][posx];
}

bool D1Dun::setItemAt(int posx, int posy, int itemIndex)
{
    if (this->items[posy][posx] == itemIndex) {
        return false;
    }
    this->items[posy][posx] = itemIndex;
    this->modified = true;
    return true;
}

int D1Dun::getMonsterAt(int posx, int posy) const
{
    return this->monsters[posy][posx];
}

bool D1Dun::setMonsterAt(int posx, int posy, int monsterIndex)
{
    if (this->monsters[posy][posx] == monsterIndex) {
        return false;
    }
    this->monsters[posy][posx] = monsterIndex;
    this->modified = true;
    return true;
}

int D1Dun::getObjectAt(int posx, int posy) const
{
    return this->objects[posy][posx];
}

bool D1Dun::setObjectAt(int posx, int posy, int objectIndex)
{
    if (this->objects[posy][posx] == objectIndex) {
        return false;
    }
    this->objects[posy][posx] = objectIndex;
    this->modified = true;
    return true;
}

int D1Dun::getTransvalAt(int posx, int posy) const
{
    return this->transvals[posy][posx];
}

bool D1Dun::setTransvalAt(int posx, int posy, int transval)
{
    if (this->transvals[posy][posx] == transval) {
        return false;
    }
    this->transvals[posy][posx] = transval;
    this->modified = true;
    return true;
}

void D1Dun::updateSubtiles(int posx, int posy, int tileRef) {
    std::vector<int> subs = std::vector<int>(TILE_WIDTH * TILE_HEIGHT);
    if (tileRef != 0) {
        if (tileRef != UNDEF_TILE && this->til->getTileCount() >= tileRef) {
            subs = this->til->getSubtileIndices(tileRef - 1);
        } else {
            for (int &sub : subs) {
                sub = UNDEF_SUBTILE;
            }
        }
    }
    for (int y = 0; y < TILE_HEIGHT; y++) {
        for (int x = 0; x < TILE_WIDTH; x++) {
            this->subtiles[posy + y][posx + x] = subs[y * TILE_WIDTH + x];
        }
    }
}
