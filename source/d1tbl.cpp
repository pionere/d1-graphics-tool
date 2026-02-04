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
#define LIGHT_COLUMN_WIDTH 32
#define LIGHT_BORDER_WIDTH 4
#define LIGHT_COLUMN_HEIGHT_UNIT 2
#define LIGHT_MAX_VALUE 256
#define LUM_COLUMN_WIDTH 32
#define LUM_BORDER_WIDTH 4
#define LUM_COLUMN_HEIGHT_UNIT 4
#define LUM_MAX_VALUE 100
#define DARK_COLUMN_WIDTH 8
#define DARK_BORDER_WIDTH 4
#define DARK_COLUMN_HEIGHT_UNIT 32

bool D1Tbl::load(const QString &filePath, const OpenAsParam &params)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.tblFilePath.isEmpty()) {
    //    filePath = params.tblFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly) && (params.celFilePath == filePath || params.tblFilePath == filePath)) {
            return false; // report read-error only if the file was explicitly requested
        }
    }

    // this->clear();
    this->tblFilePath = filePath;

    const QByteArray fileData = file.readAll();

    // Read TBL binary data
    unsigned fileSize = fileData.size();
    if (fileSize == sizeof(darkTable)) {
        // dark table
        this->type = D1TBL_TYPE::V1_DARK;
        memcpy(darkTable, fileData.data(), fileSize);
    } else if (fileSize == sizeof(distMatrix)) {
        // dist matrix
        this->type = D1TBL_TYPE::V1_DIST;
        memcpy(distMatrix, fileData.data(), fileSize);
    } else if (fileSize == 0) {
        // empty file
        this->type = D1TBL_TYPE::V1_UNDEF;
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
    QString targetFilePath = this->type == D1TBL_TYPE::V1_DIST ? params.celFilePath : params.tblFilePath;
    if (!targetFilePath.isEmpty()) {
        filePath = targetFilePath;
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

QImage D1Tbl::getTableImage(const D1Pal *pal, int radius, int xoff, int yoff, int dunType, int color, bool traceLight)
{
    if (this->lastDunType == -1) {
        // first run
        memset(dLight, MAXDARKNESS, sizeof(dLight));
    } else {
        DoUnLight(&LightList[MAXLIGHTS]);
    }

    if (this->lastDunType != dunType) {
        this->lastDunType = dunType;
        currLvl._dType = dunType;
        MakeLightTable();
    }

    int cx = (lengthof(dLight) + 1) / 2;
    int cy = (lengthof(dLight[0]) + 1) / 2;

    if (!traceLight) {
        LightList[MAXLIGHTS]._lx = cx;
        LightList[MAXLIGHTS]._ly = cy;
        LightList[MAXLIGHTS]._lradius = radius;
        LightList[MAXLIGHTS]._lxoff = xoff;
        LightList[MAXLIGHTS]._lyoff = yoff;
        LightList[MAXLIGHTS]._lunx = cx;
        LightList[MAXLIGHTS]._luny = cy;
        LightList[MAXLIGHTS]._lunr = radius;
        LightList[MAXLIGHTS]._lunxoff = xoff < 0 ? -1 : (xoff >= MAX_OFFSET ? 1 : 0);
        LightList[MAXLIGHTS]._lunyoff = yoff < 0 ? -1 : (yoff >= MAX_OFFSET ? 1 : 0);

        DoLighting(MAXLIGHTS);
    } else {
        TraceLightSource(cx, cy, radius);
        if (xoff != 0 || yoff != 0) {
            dProgressWarn() << tr("Ray-tracing with offset is not supported.");
        }
    }
#if 1
    int colors[2 * (DBORDERX + 2) + 1][2 * (DBORDERY + 2) + 1];
    memset(colors, 0xFF, sizeof(colors));
    if (radius == 1) {
        int dx, dy, i, j, tx, ty;
        const int8_t* cr;
        dx = DBORDERX + 1;
        dy = DBORDERY + 1;
        for (i = 0; i <= 15; i++) {
            cr = &CrawlTable[CrawlNum[i]];
            for (j = (BYTE)*cr; j > 0; j--) {
                tx = dx + *++cr;
                ty = dy + *++cr;
                colors[tx][ty] = i;
            }
        }
    } else if (radius == 2) {
        int dx, dy, tx, ty, r = 15, dist;
        dx = DBORDERX + 1;
        dy = DBORDERY + 1;
        for (tx = dx - r; tx <= dx + r; tx++) {
            for (ty = dx - r; ty <= dy + r; ty++) {
                dist = (tx - dx) * (tx - dx) + (ty - dy) * (ty - dy);
                dist = sqrt((double)dist) + 0.5f;
                if (dist <= 15)
                    colors[tx][ty] = dist;
            }
        }
    } else if (radius == 3) {
        int dx, dy, tx, ty, r = 15, dist;
        dx = DBORDERX + 1;
        dy = DBORDERY + 1;
        for (tx = dx - r; tx <= dx + r; tx++) {
            for (ty = dx - r; ty <= dy + r; ty++) {
                dist = (tx - dx) * (tx - dx) + (ty - dy) * (ty - dy);
                dist = sqrt((double)dist) + 0.5f;
                if (dist == 1 && (tx != dx || ty != dy))
                    dist++;
                if (dist <= 15)
                    colors[tx][ty] = dist;
            }
        }
    } else if (radius == 4) {
        int dx, dy, tx, ty, r = 15, dist;
        dx = DBORDERX + 1;
        dy = DBORDERY + 1;
        for (tx = dx - r; tx <= dx + r; tx++) {
            for (ty = dx - r; ty <= dy + r; ty++) {
                dist = std::max(abs(tx - dx), abs(ty - dy));
                if (abs(tx - dx) == abs(ty - dy) && (tx != dx || ty != dy))
                    dist++;
                if (dist <= 15)
                    colors[tx][ty] = dist;
            }
        }
    } else /*if (radius == 5)*/ {
        int dx, dy, tx, ty, r = 15, dist;
        dx = DBORDERX + 1;
        dy = DBORDERY + 1;
        for (tx = dx - r; tx <= dx + r; tx++) {
            for (ty = dx - r; ty <= dy + r; ty++) {
                dist = std::max(abs(tx - dx), abs(ty - dy));
                if (abs(tx - dx) == abs(ty - dy) && (tx != dx || ty != dy))
                    dist++;
                if (dist > 3) {
                    dist = (tx - dx) * (tx - dx) + (ty - dy) * (ty - dy);
                    dist = sqrt((double)dist) + 0.5f;
                }
                if (dist <= 15)
                    colors[tx][ty] = dist;
            }
        }
    }
#endif
    QImage image = QImage(D1Tbl::getTableImageWidth(), D1Tbl::getTableImageHeight(), QImage::Format_ARGB32);

    QRgb *destBits = reinterpret_cast<QRgb *>(image.scanLine(0));
    for (int x = 0; x < lengthof(dLight); x++) {
        for (int y = 0; y < lengthof(dLight[0]); y++) {
#if 0
            QColor c = pal->getColor(ColorTrns[dLight[x][y]][color]);
#else
            QColor c;
            if (colors[x][y] == -1) {
                c = QColor(Qt::transparent);
            } else {
                c = pal->getColor(colors[x][y]);
            }
#endif
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

int D1Tbl::getTableValueAt(int x, int y)
{
    int vx = x / TABLE_TILE_SIZE;
    int vy = y / TABLE_TILE_SIZE;

    return MAXDARKNESS - dLight[vx][vy];
}

int D1Tbl::getLightImageWidth()
{
    return LIGHT_COLUMN_WIDTH * (MAXDARKNESS + 1) + 2 * LIGHT_BORDER_WIDTH;
}

int D1Tbl::getLightImageHeight()
{
    return LIGHT_COLUMN_HEIGHT_UNIT * LIGHT_MAX_VALUE + 2 * LIGHT_BORDER_WIDTH;
}

QImage D1Tbl::getLightImage(const D1Pal *pal, int color)
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
        QColor c = pal->getColor(ColorTrns[i][color]);
        unsigned maxValue = std::max(std::max(c.red(), c.green()), c.blue());
        unsigned minValue = std::min(std::min(c.red(), c.green()), c.blue());
        int v = (maxValue + minValue) / 2;

        lightPainter.fillRect(LIGHT_BORDER_WIDTH + i * LIGHT_COLUMN_WIDTH, LIGHT_BORDER_WIDTH + (LIGHT_MAX_VALUE - v) * LIGHT_COLUMN_HEIGHT_UNIT,
            LIGHT_COLUMN_WIDTH, valueHeight, valueColor);
    }

    // lightPainter.end();
    return image;
}

int D1Tbl::getLightValueAt(const D1Pal *pal, int x, int color)
{
    int vx = x / LIGHT_COLUMN_WIDTH;

    QColor c = pal->getColor(ColorTrns[vx][color]);
    unsigned maxValue = std::max(std::max(c.red(), c.green()), c.blue());
    unsigned minValue = std::min(std::min(c.red(), c.green()), c.blue());
    return (maxValue + minValue) / 2;
}

int D1Tbl::getLumImageWidth()
{
    return LUM_COLUMN_WIDTH * (MAXDARKNESS + 1) + 2 * LUM_BORDER_WIDTH;
}

int D1Tbl::getLumImageHeight()
{
    return LUM_COLUMN_HEIGHT_UNIT * LUM_MAX_VALUE + 2 * LUM_BORDER_WIDTH;
}

/*
 * Calculate the linearized value from a color value
 *
 * @param cv: color value (in the range of 0.0 .. 1.0)
 * @returns: the linearized value (in the range of 0.0 .. 1.0)
 */
static double sRGBtoLin(double cv)
{
    if (cv <= 0.04045) {
        return cv / 12.92;
    } else {
        return pow(((cv + 0.055) / 1.055), 2.4);
    }
}

/*
 * Calculate the perceived lightness from a luminance.
 *
 * @param Y: a luminance value (in the range of 0.0 .. 1.0)
 * @returns: L* which is "perceptual lightness" (in the range of 0 .. 100)
 */
static int YtoLstar(double Y)
{

    if (Y <= (216.0 / 24389)) {    // The CIE standard states 0.008856 but 216/24389 is the intent for 0.008856451679036
        return Y * (24389.0 / 27); // The CIE standard states 903.3, but 24389/27 is the intent, making 903.296296296296296
    } else {
        return pow(Y, (1.0 / 3)) * 116 - 16;
    }
}

static int getLumValue(const QColor c)
{
    double vR = c.red() / 255.0;
    double vG = c.green() / 255.0;
    double vB = c.blue() / 255.0;

    double Y = 0.2126 * sRGBtoLin(vR) + 0.7152 * sRGBtoLin(vG) + 0.0722 * sRGBtoLin(vB);

    return YtoLstar(Y);
}

QImage D1Tbl::getLumImage(const D1Pal *pal, int color)
{
    constexpr int axisWidth = 2;
    constexpr int valueHeight = 2;

    QImage image = QImage(D1Tbl::getLumImageWidth(), D1Tbl::getLumImageHeight(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter lumPainter(&image);
    QColor valueColor = QColor(Config::getPaletteSelectionBorderColor());

    lumPainter.fillRect(0, D1Tbl::getLumImageHeight() - LUM_BORDER_WIDTH, D1Tbl::getLumImageWidth(), axisWidth, QColorConstants::Black);
    lumPainter.fillRect(LUM_BORDER_WIDTH - axisWidth, 0, axisWidth, D1Tbl::getLumImageHeight(), QColorConstants::Black);

    for (int i = 0; i <= MAXDARKNESS; i++) {
        QColor c = pal->getColor(ColorTrns[i][color]);
        int v = getLumValue(c);

        lumPainter.fillRect(LUM_BORDER_WIDTH + i * LUM_COLUMN_WIDTH, LUM_BORDER_WIDTH + (LUM_MAX_VALUE - v) * LUM_COLUMN_HEIGHT_UNIT,
            LUM_COLUMN_WIDTH, valueHeight, valueColor);
    }

    // lumPainter.end();
    return image;
}

int D1Tbl::getLumValueAt(const D1Pal *pal, int x, int color)
{
    int vx = x / LUM_COLUMN_WIDTH;

    QColor c = pal->getColor(ColorTrns[vx][color]);
    return getLumValue(c);
}

int D1Tbl::getDarkImageWidth()
{
    return DARK_COLUMN_WIDTH * lengthof(darkTable[0]) + 2 * DARK_BORDER_WIDTH;
}

int D1Tbl::getDarkImageHeight()
{
    return DARK_COLUMN_HEIGHT_UNIT * MAXDARKNESS + 2 * DARK_BORDER_WIDTH;
}

QImage D1Tbl::getDarkImage(int radius)
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

int D1Tbl::getDarkValueAt(int x, int radius)
{
    int vx = x / DARK_COLUMN_WIDTH;

    return MAXDARKNESS - darkTable[radius][vx];
}

void D1Tbl::setDarkValueAt(int x, int radius, int value)
{
    int vx = x / DARK_COLUMN_WIDTH;

    darkTable[radius][vx] = MAXDARKNESS - value;
    this->modified = true;
}

D1TBL_TYPE D1Tbl::getType() const
{
    return this->type;
}

void D1Tbl::setType(D1TBL_TYPE type)
{
    this->type = type;
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
