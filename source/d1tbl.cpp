#include "d1tbl.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMessageBox>
#include <QPainter>

#include "config.h"
#include "progressdialog.h"

#include "dungeon/all.h"

#define TABLE_TILE_SIZE 32
#define LIGHT_COLUMN_WIDTH 8
#define LIGHT_BORDER_WIDTH 4
#define LIGHT_COLUMN_HEIGHT_UNIT 2
#define DARK_COLUMN_WIDTH 8
#define DARK_BORDER_WIDTH 4
#define DARK_COLUMN_HEIGHT_UNIT 32

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

int D1Tbl::getTableImageWidth()
{
    return lengthof(dLight) * TABLE_TILE_SIZE;
}

int D1Tbl::getTableImageHeight()
{
    return lengthof(dLight[0]) * TABLE_TILE_SIZE;
}

QImage D1Tbl::getTableImage(int radius, int dunType, int color) const
{
    currLvl._dType = dunType;
    MakeLightTable();

    memset(dLight, MAXDARKNESS, sizeof(dLight));
    LightList[MAXLIGHTS]._lx = MAX_LIGHT_RAD + 1;
    LightList[MAXLIGHTS]._ly = MAX_LIGHT_RAD + 1;
    LightList[MAXLIGHTS]._lradius = radius;
    LightList[MAXLIGHTS]._lxoff = 0;
    LightList[MAXLIGHTS]._lyoff = 0;
    DoLighting(MAXLIGHTS);

    QImage image = QImage(D1Tbl::getTableImageWidth(), D1Tbl::getTableImageHeight(), QImage::Format_ARGB32);

    QRgb *destBits = reinterpret_cast<QRgb *>(image.scanLine(0));
    for (int x = 0; x < lengthof(dLight); x++) {
        for (int y = 0; y < lengthof(dLight[0]); y++) {
            QColor c = this->pal->getColor(ColorTrns[dLight[x][y]][color]);
            for (int xx = x * TABLE_TILE_SIZE; xx < x * TABLE_TILE_SIZE + TABLE_TILE_SIZE; xx++) {
                for (int yy = y * TABLE_TILE_SIZE; yy < y * TABLE_TILE_SIZE + TABLE_TILE_SIZE; yy++) {
                    // image.setPixelColor(xx, yy, color);
                    destBits[yy * D1Tbl::getTableImageWidth() + xx] = c.rgba();
                }
            }
        }
    }

    return image;
}

int D1Tbl::getTableValueAt(int x, int y) const
{
    int vx = x / TABLE_TILE_SIZE;
    int vy = y / TABLE_TILE_SIZE;

    return dLight[vx][vy];
}

int D1Tbl::getLightImageWidth()
{
    return LIGHT_COLUMN_WIDTH * (MAXDARKNESS + 1) + 2 * LIGHT_BORDER_WIDTH;
}

int D1Tbl::getLightImageHeight()
{
    return LIGHT_COLUMN_HEIGHT_UNIT * 256 + 2 * LIGHT_BORDER_WIDTH;
}

QImage D1Tbl::getLightImage(int color) const
{
    constexpr int axisWidth = 2;
    constexpr int valueHeight = 2;

    QImage image = QImage(D1Tbl::getLightImageWidth(), D1Tbl::getLightImageHeight(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter lightPainter(&image);
    QColor valueColor = QColor(Config::getPaletteSelectionBorderColor());

    lightPainter.fillRect(0, D1Tbl::getLightImageHeight() - LIGHT_BORDER_WIDTH, D1Tbl::getLightImageWidth(), axisWidth, QColorConstants::Black);
    lightPainter.fillRect(LIGHT_BORDER_WIDTH - axisWidth, 0, axisWidth, D1Tbl::getLightImageHeight(), QColorConstants::Black);

    for (int i = 0; i <= MAXDARKNESS; i++) {
        QColor c = this->pal->getColor(ColorTrns[i][color]);
        unsigned maxValue = std::max(std::max(c.red(), c.green()), c.blue());
        unsigned minValue = std::min(std::min(c.red(), c.green()), c.blue());
        int v = (maxValue + minValue) / 2;

        lightPainter.fillRect(LIGHT_BORDER_WIDTH + i * LIGHT_COLUMN_WIDTH, LIGHT_BORDER_WIDTH + (256 - v) * LIGHT_COLUMN_HEIGHT_UNIT,
            LIGHT_COLUMN_WIDTH, valueHeight, valueColor);
    }

    // lightPainter.end();
    return image;
}

int D1Tbl::getDarkImageWidth()
{
    return DARK_COLUMN_WIDTH * lengthof(darkTable[0]) + 2 * DARK_BORDER_WIDTH;
}

int D1Tbl::getDarkImageHeight()
{
    return DARK_COLUMN_HEIGHT_UNIT * MAXDARKNESS + 2 * DARK_BORDER_WIDTH;
}

QImage D1Tbl::getDarkImage(int radius) const
{
    constexpr int axisWidth = 2;

    QImage image = QImage(D1Tbl::getDarkImageWidth(), D1Tbl::getDarkImageHeight(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter darkPainter(&image);
    QColor valueColor = QColor(Config::getPaletteSelectionBorderColor());

    darkPainter.fillRect(0, D1Tbl::getDarkImageHeight() - DARK_BORDER_WIDTH, D1Tbl::getDarkImageWidth(), axisWidth, QColorConstants::Black);
    darkPainter.fillRect(DARK_BORDER_WIDTH - axisWidth, 0, axisWidth, D1Tbl::getDarkImageHeight(), QColorConstants::Black);

    for (int i = 0; i < lengthof(darkTable[0]); i++) {
        if (MAXDARKNESS != darkTable[radius][i])
            darkPainter.fillRect(i * DARK_COLUMN_WIDTH + DARK_BORDER_WIDTH, DARK_BORDER_WIDTH + darkTable[radius][i] * DARK_COLUMN_HEIGHT_UNIT,
                DARK_COLUMN_WIDTH, DARK_COLUMN_HEIGHT_UNIT * (MAXDARKNESS - darkTable[radius][i]), valueColor);
    }

    // darkPainter.end();
    return image;
}

int D1Tbl::getDarkValueAt(int x, int radius) const
{
    int vx = x / DARK_COLUMN_WIDTH;

    return darkTable[radius][vx];
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
