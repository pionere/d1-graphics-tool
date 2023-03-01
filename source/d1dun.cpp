#include "d1dun.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QMessageBox>

#include "config.h"
#include "d1til.h"
#include "d1tmi.h"
#include "progressdialog.h"

void D1Dun::initVectors(int dunWidth, int dunHeight)
{
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
}

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

    unsigned fileSize = fileData.size();
    int dunWidth = 0;
    int dunHeight = 0;
    bool changed = fileSize == 0; // !file.isOpen();
    if (fileSize != 0) {
        if (filePath.toLower().endsWith(".dun")) {
            // File size check
            if (fileSize < 2 * 2) {
                dProgressErr() << tr("Invalid DUN file.");
                return false;
            }

            // Read DUN binary data
            QDataStream in(fileData);
            in.setByteOrder(QDataStream::LittleEndian);

            quint16 readWord;
            in >> readWord;
            dunWidth = readWord;
            in >> readWord;
            dunHeight = readWord;

            unsigned tileCount = dunWidth * dunHeight;
            if (fileSize < 2 * 2 + tileCount * 2) {
                dProgressErr() << tr("Invalid DUN header.");
                return false;
            }

            // prepare the vectors
            this->initVectors(dunWidth, dunHeight);

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
                    this->updateSubtiles(x, y, this->tiles[y][x]);
                }
            }
        } else {
            // rdun
            dunWidth = dunHeight = sqrt(fileSize / (TILE_WIDTH * TILE_HEIGHT * 4));
            if (fileSize != dunWidth * dunHeight * (TILE_WIDTH * TILE_HEIGHT * 4)) {
                dProgressErr() << tr("Invalid RDUN file.");
                return false;
            }

            // prepare the vectors
            this->initVectors(dunWidth, dunHeight);

            // Read RDUN binary data
            QDataStream in(fileData);
            in.setByteOrder(QDataStream::LittleEndian);

            quint32 readDword;
            // read subtiles
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    in >> readDword;
                    this->subtiles[y][x] = readDword + 1;
                }
            }

            // prepare tiles
            for (int y = 0; y < dunHeight; y++) {
                for (int x = 0; x < dunWidth; x++) {
                    this->tiles[y][x] = UNDEF_TILE;
                }
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

    bool baseDun = filePath.toLower().endsWith(".dun");
    // validate data
    if (baseDun) {
        // dun - tiles must be defined
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
    } else {
        // rdun - subtiles must be defined
        int dunWidth = this->width;
        int dunHeight = this->height;
        for (int y = 0; y < dunHeight; y++) {
            for (int x = 0; x < dunWidth; x++) {
                if (this->subtiles[y][x] == UNDEF_SUBTILE || this->subtiles[y][x] == 0) {
                    dProgressFail() << tr("Undefined or zero subtiles (one at %1:%2) can not be saved in this format (RDUN).").arg(x).arg(y);
                    return false;
                }
            }
        }
        // report warning of unsaved information
        for (int y = 0; y < dunHeight / TILE_HEIGHT; y++) {
            for (int x = 0; x < dunWidth / TILE_WIDTH; x++) {
                if (this->tiles[y][x] != UNDEF_TILE && this->tiles[y][x] != 0) {
                    dProgressWarn() << tr("Defined tile at %1:%2 is not saved in this format (RDUN).").arg(x).arg(y);
                }
            }
        }
        for (int y = 0; y < dunHeight; y++) {
            for (int x = 0; x < dunWidth; x++) {
                if (this->items[y][x] != 0) {
                    dProgressWarn() << tr("Defined item at %1:%2 is not saved in this format (RDUN).").arg(x).arg(y);
                }
                if (this->monsters[y][x] != 0) {
                    dProgressWarn() << tr("Defined monster at %1:%2 is not saved in this format (RDUN).").arg(x).arg(y);
                }
                if (this->objects[y][x] != 0) {
                    dProgressWarn() << tr("Defined object at %1:%2 is not saved in this format (RDUN).").arg(x).arg(y);
                }
                if (this->transvals[y][x] != 0) {
                    dProgressWarn() << tr("Defined transval at %1:%2 is not saved in this format (RDUN).").arg(x).arg(y);
                }
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

    if (baseDun) {
        // dun
        int dunWidth = this->width / TILE_WIDTH;
        int dunHeight = this->height / TILE_HEIGHT;

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
    } else {
        // rdun
        int dunWidth = this->width;
        int dunHeight = this->height;

        quint32 writeDword;
        // write subtiles
        for (int y = 0; y < dunHeight; y++) {
            for (int x = 0; x < dunWidth; x++) {
                writeDword = this->subtiles[y][x] - 1;
                out << writeDword;
            }
        }
    }

    this->dunFilePath = filePath; // this->load(filePath, allocate);
    this->modified = false;

    return true;
}

void D1Dun::drawImage(QPainter &dungeon, QImage &backImage, int drawCursorX, int drawCursorY, int dunCursorX, int dunCursorY, Qt::CheckState tileState, bool showItems, bool showMonsters, bool showObjects) const
{
    if (tileState != Qt::Unchecked) {
        // draw the background
        dungeon.drawImage(drawCursorX, drawCursorY, backImage);
        // draw the subtile
        int subtileRef = this->subtiles[dunCursorY][dunCursorX];
        if (subtileRef != 0) {
            if (subtileRef == UNDEF_SUBTILE || subtileRef > this->til->getMin()->getSubtileCount()) {
                QString text = tr("Subtile%1");
                if (subtileRef == UNDEF_SUBTILE) {
                    text = text.arg("???");
                } else {
                    text = text.arg(subtileRef - 1);
                }
                // dungeon.setFont(font);
                // dungeon.setPen(font);
                QFontMetrics fm(dungeon.font());
                QRect rect = fm.boundingRect(text);
                dungeon.drawText(drawCursorX + backImage.width() / 2 - rect.width() / 2, drawCursorY - backImage.height() / 2 - rect.height() / 2, text);
            } else {
                QImage subtileImage = this->til->getMin()->getSubtileImage(subtileRef - 1);
                if (tileState != Qt::Checked) {
                    // crop image in case of partial-draw
                    QRect rect = backImage.rect();
                    rect.setY(subtileImage.height() - rect.height());
                    subtileImage = subtileImage.copy(rect);
                }
                dungeon.drawImage(drawCursorX, drawCursorY, subtileImage);
            }
        }
        // draw tile text
        static_assert(TILE_WIDTH == 2 && TILE_HEIGHT == 2, "D1Dun::drawImage skips boundary checks.");
        int tileRef = this->tiles[dunCursorY / TILE_HEIGHT][dunCursorX / TILE_WIDTH];
        if (tileRef != 0 && (drawCursorX & 1) && (drawCursorY & 1)) {
            bool skipTile = true;
            for (int dy = TILE_HEIGHT - 1; dy >= 0; dy--) {
                for (int dx = TILE_WIDTH - 1; dx >= 0; dx--) {
                    int subtileRef = this->subtiles[dunCursorY - dy][dunCursorX - dx];
                    if (subtileRef == UNDEF_SUBTILE || subtileRef > this->til->getMin()->getSubtileCount()) {
                        skipTile = false;
                    }
                }
            }
            if (!skipTile) {
                QString text = tr("Tile%1");
                if (tileRef == UNDEF_TILE) {
                    text = text.arg("???");
                } else {
                    text = text.arg(tileRef - 1);
                }
                // dungeon.setFont(font);
                // dungeon.setPen(font);
                QFontMetrics fm(dungeon.font());
                QRect rect = fm.boundingRect(text);
                dungeon.drawText(drawCursorX + backImage.width() / 2 - rect.width() / 2, drawCursorY - backImage.height() - rect.height() / 2, text);
            }
        }
    }
}

QImage D1Dun::getImage(Qt::CheckState tileState, bool showItems, bool showMonsters, bool showObjects) const
{
    int maxDunSize = std::max(this->width, this->height);
    int minDunSize = std::min(this->width, this->height);
    int maxTilSize = std::max(TILE_WIDTH, TILE_HEIGHT);

    unsigned subtileWidth = this->til->getMin()->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->til->getMin()->getSubtileHeight() * MICRO_HEIGHT;

    unsigned cellWidth = subtileWidth;
    unsigned cellHeight = cellWidth / 2;

    QImage dungeon = QImage(maxDunSize * cellWidth,
        (maxDunSize - 1) * cellHeight + subtileHeight, QImage::Format_ARGB32);
    dungeon.fill(Qt::transparent);

    // create template of the background image
    QImage backImage = QImage(cellWidth, cellHeight, QImage::Format_ARGB32);
    if (tileState != Qt::Unchecked) {
        backImage.fill(Qt::transparent);
        QColor backColor = QColor(Config::getGraphicsTransparentColor());
        unsigned len = 0;
        for (unsigned y = 1; y < cellHeight / 2; y++) {
            len += 2;
            for (unsigned x = cellWidth / 2 - len; x < cellWidth / 2 + len; x++) {
                backImage.setPixelColor(x, y, backColor);
            }
        }
        for (unsigned y = cellHeight / 2; y < cellHeight; y++) {
            len -= 2;
            for (unsigned x = cellWidth / 2 - len; x < cellWidth / 2 + len; x++) {
                backImage.setPixelColor(x, y, backColor);
            }
        }
    }

    QPainter dunPainter(&dungeon);

    int drawCursorX = (maxDunSize * cellWidth - 1) / 2 - (this->width - this->height) * cellWidth;
    int drawCursorY = subtileHeight;
    int dunCursorX;
    int dunCursorY = 0;

    // draw top triangle
    for (int i = 0; i < minDunSize; i++) {
        dunCursorX = 0;
        dunCursorY = i;
        while (dunCursorY >= 0) {
            this->drawImage(dunPainter, backImage, drawCursorX, drawCursorY, dunCursorX, dunCursorY, tileState, showItems, showMonsters, showObjects);
            dunCursorY--;
            dunCursorX++;

            drawCursorX += cellWidth;
        }
        // move back to start
        drawCursorX -= cellWidth * (i + 1);
        // move down one row (+ half left)
        drawCursorX -= cellWidth / 2;
        drawCursorY += cellHeight / 2;
    }
    // draw middle 'square'
    if (this->width > this->height) {
        drawCursorX += cellWidth;
        for (int i = 0; i < this->width - this->height; i++) {
            dunCursorX = i;
            dunCursorY = this->height - 1;
            while (dunCursorY >= 0) {
                this->drawImage(dunPainter, backImage, drawCursorX, drawCursorY, dunCursorX, dunCursorY, tileState, showItems, showMonsters, showObjects);
                dunCursorY--;
                dunCursorX++;

                drawCursorX += cellWidth;
            }
            // move back to start
            drawCursorX -= cellWidth * this->height;
            // move down one row (+ half right)
            drawCursorX += cellWidth / 2;
            drawCursorY += cellHeight / 2;
        }
        // sync drawCursorX with the other branches
        drawCursorX -= cellWidth;
    } else if (this->width < this->height) {
        for (int i = 0; i < this->height - this->width; i++) {
            dunCursorX = 0;
            dunCursorY = this->height + i;
            while (dunCursorX < this->width) {
                this->drawImage(dunPainter, backImage, drawCursorX, drawCursorY, dunCursorX, dunCursorY, tileState, showItems, showMonsters, showObjects);
                dunCursorY--;
                dunCursorX++;

                drawCursorX += cellWidth;
            }
            // move back to start
            drawCursorX -= cellWidth * this->width;
            // move down one row (+ half left)
            drawCursorX -= cellWidth / 2;
            drawCursorY += cellHeight / 2;
        }
    }
    // draw bottom triangle
    drawCursorX += cellWidth;
    for (int i = minDunSize - 1; i > 0; i--) {
        dunCursorX = this->width - i;
        dunCursorY = this->height - 1;
        while (dunCursorX < this->width) {
            this->drawImage(dunPainter, backImage, drawCursorX, drawCursorY, dunCursorX, dunCursorY, tileState, showItems, showMonsters, showObjects);
            dunCursorY--;
            dunCursorX++;

            drawCursorX += cellWidth;
        }
        // move back to start
        drawCursorX -= cellWidth * i;
        // move down one row (+ half right)
        drawCursorX += cellWidth / 2;
        drawCursorY += cellHeight / 2;
    }

    // dunPainter.end();
    return dungeon;
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

void D1Dun::updateSubtiles(int posx, int posy, int tileRef)
{
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
