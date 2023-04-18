#include "d1tbl.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMessageBox>
#include <QPainter>

#include "progressdialog.h"

#include "dungeon/all.h"

bool D1Tbl::load(const QString &filePath)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.tblFilePath.isEmpty()) {
    //    filePath = params.tblFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
    }

    // this->clear();
    this->tblFilePath = filePath;

    const QByteArray fileData = file.readAll();

    // Read TBL binary data
    unsigned fileSize = fileData.size();
    if (fileSize == sizeof(darkTable)) {
        // dark table
        memcpy(darkTable, fileData.data(), fileSize);
    } else if (fileSize == sizeof(distMatrix)) {
        // dist matrix
        memcpy(distMatrix, fileData.data(), fileSize);
    } else {
        dProgressErr() << tr("Invalid TBL file.");
        return false;
    }
    this->modified = !file.isOpen();
    return true;
}

bool D1Tbl::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.celFilePath.isEmpty()) {
        filePath = params.celFilePath;
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

    // write to file
    QDataStream out(&outFile);
    if (this->type == D1TBL_TYPE::V1_DARK) {
        out.writeRawData((const char *)darkTable, sizeof(darkTable));
    } else {
        out.writeRawData((const char *)distMatrix, sizeof(distMatrix));
    }

    this->tblFilePath = filePath;
    this->modified = false;

    return true;
}

QImage D1Tbl::getTableImage(int radius, int dunType, int color) const
{
    constexpr int tileSize = 32;
    currLvl._dType = dunType;
    MakeLightTable();

    memset(dLight, MAXDARKNESS, sizeof(dLight));
    DoLighting(MAX_LIGHT_RAD + 1, MAX_LIGHT_RAD + 1, radius, MAXLIGHTS);

    QImage image = QImage(lengthof(dLight) * tileSize, lengthof(dLight[0]) * tileSize, QImage::Format_ARGB32);

    QRgb *destBits = reinterpret_cast<QRgb *>(image.scanLine(0));
    for (int x = 0; x < lengthof(dLight); x++) {
        for (int y = 0; y < lengthof(dLight[0]); y++) {
            QColor c = this->pal->getColor(ColorTrns[dLight[x][y]][color]);
            for (int xx = x * tileSize; xx < x * tileSize + tileSize; xx++) {
                for (int yy = y * tileSize; yy < y * tileSize + tileSize; yy++) {
                    // image.setPixelColor(xx, yy, color);
                    destBits[yy * lengthof(dLight) * tileSize + xx] = c.rgba();
                }
            }
        }
    }

    return image;
}

QImage D1Tbl::getDarkImage(int radius) const
{
    constexpr int columWidth = 16;
    constexpr int columHeightUnit = 32;

    QImage image = QImage(columWidth * lengthof(darkTable[0]), columHeightUnit * MAXDARKNESS, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter darkPainter(&image);
    darkPainter.setPen(QColor(Config::getPaletteSelectionBorderColor()));

    for (int i = 0; i < lengthof(darkTable[0]); i++) {
        darkPainter.drawRect(i * columWidth, 0 + columHeightUnit * MAXDARKNESS - columHeightUnit * darkTable[radius][i], columWidth, columHeightUnit * darkTable[radius][i]);
    }

    // darkPainter.end();
    return image;
}

QString D1Tbl::getFilePath() const
{
    return this->tblFilePath;
}

bool D1Tbl::isModified() const
{
    return this->modified;
}

void D1Tbl::setModified()
{
    this->modified = true;
}

void D1Tbl::setPalette(D1Pal *pal)
{
    this->pal = pal;
}
