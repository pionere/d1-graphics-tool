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
    bool changed = fileSize != 0; // !file.isOpen();
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
        items.resize(dunHeight * TILE_HEIGHT);
        for (int i = 0; i < dunHeight; i++) {
            items[i].resize(dunWidth * TILE_WIDTH);
        }
        objects.resize(dunHeight * TILE_HEIGHT);
        for (int i = 0; i < dunHeight; i++) {
            objects[i].resize(dunWidth * TILE_WIDTH);
        }
        monsters.resize(dunHeight * TILE_HEIGHT);
        for (int i = 0; i < dunHeight; i++) {
            monsters[i].resize(dunWidth * TILE_WIDTH);
        }
        transvals.resize(dunHeight * TILE_HEIGHT);
        for (int i = 0; i < dunHeight; i++) {
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
    }

    this->width = dunWidth;
    this->height = dunHeight;
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

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    // write to file
    QDataStream out(&outFile);
    out.setByteOrder(QDataStream::LittleEndian);

    quint16 dunWidth = this->width;
    quint16 dunHeight = this->height;

    out << dunWidth;
    out << dunHeight;

    // write tiles
    quint16 writeWord;
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
