#include "d1pcx.h"

#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QList>

#include "config.h"
#include "d1image.h"
#include "mainwindow.h"
#include "progressdialog.h"

#define D1PCX_COLORS 256

static constexpr quint8 PCX_MAX_SINGLE_PIXEL = 0xBF;
static constexpr quint8 PCX_RUNLENGTH_MASK = 0x3F;

typedef struct _PcxHeader {
    quint8 Manufacturer;
    quint8 Version;
    quint8 Encoding;
    quint8 BitsPerPixel;
    quint16 Xmin;
    quint16 Ymin;
    quint16 Xmax;
    quint16 Ymax;
    quint16 HDpi;
    quint16 VDpi;
    quint8 Colormap[48];
    quint8 Reserved;
    quint8 NPlanes;
    quint16 BytesPerLine;
    quint16 PaletteInfo;
    quint16 HscreenSize;
    quint16 VscreenSize;
    quint8 Filler[54];
} PCXHEADER;

typedef struct _PcxPalette {
    quint8 prefix;
    quint8 data[D1PCX_COLORS][3];
} PCXPALETTE;

bool D1Pcx::load(D1Gfx &gfx, D1Pal *pal, const QString &filePath, const OpenAsParam &params)
{
    QString celPath = filePath;

    // assert(filePath.toLower().endsWith(".pcx"));
    celPath.chop(4);
    celPath += ".cel";

    if (params.celWidth != 0) {
        dProgressWarn() << QApplication::tr("Width setting is ignored when a PCX file is loaded.");
    }
    bool clipped = params.clipped == OPEN_CLIPPED_TYPE::TRUE;

    QColor undefColor = pal->getUndefinedColor();
    for (int i = 0; i < D1PAL_COLORS; i++) {
        pal->setColor(i, undefColor);
    }

    gfx.groupFrameIndices.clear();
    gfx.groupFrameIndices.push_back(std::pair<int, int>(0, 0));

    gfx.type = D1CEL_TYPE::V1_REGULAR;
    gfx.clipped = clipped;

    // gfx.frames.clear();
    D1GfxFrame *frame = new D1GfxFrame();
    gfx.frames.append(frame);

    gfx.gfxFilePath = celPath;
    gfx.modified = true;

    bool dummy;
    return D1Pcx::load(*frame, filePath, pal, nullptr, &dummy);
}

bool D1Pcx::load(D1GfxFrame &frame, const QString &filePath, D1Pal *basePal, D1Pal *resPal, bool *palMod)
{
    // Opening PCX file
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        dProgressErr() << QApplication::tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    const quint64 fileSize = file.size();
    PCXHEADER pcxhdr;
    PCXPALETTE pPalette;

    if (fileSize < sizeof(pcxhdr)) {
        dProgressErr() << QApplication::tr("Invalid PCX file.");
        return false;
    }

    // process the header
    file.read((char *)&pcxhdr, sizeof(pcxhdr));
    if (pcxhdr.Manufacturer != 0x0A) {
        dProgressErr() << QApplication::tr("Invalid PCX header.");
        return false;
    }
    if (pcxhdr.BitsPerPixel != 8) {
        dProgressErr() << QApplication::tr("Unsupported PCX format (number of bits per pixel).");
        return false;
    }
    if (pcxhdr.NPlanes != 1) {
        dProgressErr() << QApplication::tr("Unsupported PCX format (number of planes).");
        return false;
    }

    // read the palette
    *palMod = false;
    D1Pal *pcxPal = nullptr;
    int pcxPalState = 0; // 0: invalid; 1: valid and matches pal; 2: valid but does not match the pal
    if (fileSize >= sizeof(pcxhdr) + sizeof(pPalette)) {
        file.seek(fileSize - sizeof(pPalette));
        file.read((char *)&pPalette, sizeof(pPalette));

        pcxPalState = pPalette.prefix == 0x0C ? 1 : 0;
        if (pcxPalState == 1) {
            pcxPal = new D1Pal();
            static_assert(D1PAL_COLORS == D1PCX_COLORS, "D1Pcx::load uses D1Pal to store the palette of a PCX file.");
            for (int i = 0; i < D1PCX_COLORS; i++) {
                pcxPal->setColor(i, QColor(pPalette.data[i][0], pPalette.data[i][1], pPalette.data[i][2]));
            }

            for (int i = 0; i < D1PCX_COLORS; i++) {
                QColor col = basePal->getColor(i);
                if (col == basePal->getUndefinedColor()) {
                    continue; // color of the graphics-palette is undefined -> ignore
                }
                QColor pcxCol = pcxPal->getColor(i);
                if (pcxCol == basePal->getUndefinedColor()) {
                    continue; // color of the PCX-palette is undefined -> ignore
                }
                if (col != pcxCol) {
                    pcxPalState = 2;
                    break;
                }
            }
        }
        file.seek(sizeof(pcxhdr));
    }

    if (resPal == nullptr) {
        // loading a complete file
        pcxPalState = 1;
        if (pcxPal == nullptr) {
            pcxPal = basePal;
        }
    }

    // prepare D1GfxFrame
    frame.width = SwapLE16(pcxhdr.Xmax) - SwapLE16(pcxhdr.Xmin) + 1;
    frame.height = SwapLE16(pcxhdr.Ymax) - SwapLE16(pcxhdr.Ymin) + 1;
    frame.pixels.clear();

    // read data
    QByteArray fileData = file.read(fileSize - sizeof(PCXHEADER));

    const quint16 srcSkip = SwapLE16(pcxhdr.BytesPerLine) - frame.width;
    auto dataPtr = fileData.cbegin();
    quint8 byte;
    if (pcxPalState == 1) {
        // read using the given palette
        QSet<quint8> usedColors;
        for (int y = 0; y < frame.height; y++) {
            std::vector<D1GfxPixel> pixelLine;
            for (int x = 0; x < frame.width; dataPtr++) {
                byte = *dataPtr;
                if (byte <= PCX_MAX_SINGLE_PIXEL) {
                    if (pcxPal->getColor(byte) != basePal->getUndefinedColor()) {
                        pixelLine.push_back(D1GfxPixel::colorPixel(byte));
                        usedColors.insert(byte);
                    } else {
                        pixelLine.push_back(D1GfxPixel::transparentPixel()); // color of the PCX-palette is undefined -> transparent
                    }
                    x++;
                    continue;
                }
                byte &= PCX_RUNLENGTH_MASK;
                dataPtr++;
                quint8 col = *dataPtr;
                D1GfxPixel pCol = D1GfxPixel::transparentPixel(); // color of the PCX-palette is undefined -> transparent
                if (pcxPal->getColor(col) != basePal->getUndefinedColor()) {
                    pCol = D1GfxPixel::colorPixel(col);
                    usedColors.insert(col);
                }
                for (int i = 0; i < byte; i++) {
                    pixelLine.push_back(pCol);
                }
                x += byte;
            }
            frame.pixels.push_back(std::move(pixelLine));
            dataPtr += srcSkip;
        }

        // update the undefined colors of the palette
        for (int i = 0; i < D1PCX_COLORS; i++) {
            QColor col = basePal->getColor(i);
            // if (col != undefColor) {
            //     continue; // skip colors if already defined
            // }
            if (resPal != nullptr && usedColors.find(i) == usedColors.end()) {
                continue; // ignore unused colors if not loading a complete file
            }
            QColor pcxCol = pcxPal->getColor(i);
            if (pcxCol == col) {
                continue; // same as before
            }
            basePal->setColor(i, pcxCol);
            *palMod = true;
        }
    } else {
        // read as a standard image
        if (pcxPal == nullptr) {
            pcxPal = resPal;
        }
        QImage image = QImage(frame.width, frame.height, QImage::Format_ARGB32);
        QRgb *destBits = reinterpret_cast<QRgb *>(image.bits());
        for (int y = 0; y < frame.height; y++) {
            for (int x = 0; x < frame.width; dataPtr++) {
                byte = *dataPtr;
                if (byte <= PCX_MAX_SINGLE_PIXEL) {
                    QColor color = pcxPal->getColor(byte);
                    if (color == resPal->getUndefinedColor()) {
                        color = Qt::transparent;
                    }
                    // image.setPixelColor(x, y, color);
                    *destBits = color.rgba();
                    x++;
                    destBits++;
                    continue;
                }
                byte &= PCX_RUNLENGTH_MASK;
                dataPtr++;
                quint8 col = *dataPtr;
                QColor color = pcxPal->getColor(col);
                if (color == resPal->getUndefinedColor()) {
                    color = Qt::transparent;
                }
                for (int i = 0; i < byte; i++, destBits++) {
                    // image.setPixelColor(x + i, y, color);
                    *destBits = color.rgba();
                }
                x += byte;
            }
            dataPtr += srcSkip;
        }

        D1ImageFrame::load(frame, image, resPal);
    }

    if (pcxPal != resPal && pcxPal != basePal) {
        delete pcxPal;
    }
    return true;
}

bool D1Pcx::save(const std::vector<std::vector<D1GfxPixel>> &pixels, const D1Pal *pal, const QString &filePath, const ExportParam &params)
{
    const QSize imageSize = D1PixelImage::getImageSize(pixels);

    if (imageSize.width() > UINT16_MAX || imageSize.height() > UINT16_MAX) {
        dProgressFail() << QApplication::tr("PCX format can not store the image due to its dimensions: %1x%2.").arg(imageSize.width()).arg(imageSize.height());
        return false;
    }

    // select color for the transparent pixels
    bool usedColors[D1PAL_COLORS] = { false };
    bool hasTransparent = false;
    for (int y = 0; y < imageSize.height(); y++) {
        for (int x = 0; x < imageSize.width(); x++) {
            if (pixels[y][x].isTransparent())
                hasTransparent = true;
            else
                usedColors[pixels[y][x].getPaletteIndex()] = true;
        }
    }

    int transparentIndex = params.transparentIndex;
    if (hasTransparent) {
        // select the first unused, undefined color
        for (int i = 0; i < D1PAL_COLORS && transparentIndex >= D1PAL_COLORS; i++) {
            if (!usedColors[i] && pal->getColor(i) == pal->getUndefinedColor()) {
                transparentIndex = i;
            }
        }
        // select the first unused color
        for (int i = 0; i < D1PAL_COLORS && transparentIndex >= D1PAL_COLORS; i++) {
            if (!usedColors[i]) {
                transparentIndex = i;
            }
        }
        if (transparentIndex >= D1PAL_COLORS) {
            dProgressFail() << QApplication::tr("Could not find a palette entry to use for the transparent colors.");
            return false;
        }
    }

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    // write to file
    QDataStream out(&outFile);
    // out.setByteOrder(QDataStream::LittleEndian);

    // - header
    PCXHEADER header = { 0 };
    header.Manufacturer = 0x0A;
    header.Version = 5;
    header.Encoding = 1;
    header.BitsPerPixel = 8;
    header.Xmax = SwapLE16(imageSize.width() - 1);
    header.Ymax = SwapLE16(imageSize.height() - 1);
    // header.HDpi = SwapLE16(imageSize.width());
    // header.VDpi = SwapLE16(imageSize.height());
    header.NPlanes = 1;
    header.BytesPerLine = SwapLE16(imageSize.width());

    out.writeRawData((char *)&header, sizeof(header));

    // - image

    for (int y = 0; y < imageSize.height(); y++) {
        int width = imageSize.width();
        int x = 0;
        // write one line
        quint8 rleLength;
        quint8 rlePixel;

        while (width != 0) {
            rlePixel = pixels[y][x].isTransparent() ? transparentIndex : pixels[y][x].getPaletteIndex();
            rleLength = 1;

            x++;
            width--;

            while (width != 0 && rlePixel == (pixels[y][x].isTransparent() ? transparentIndex : pixels[y][x].getPaletteIndex())) {
                if (rleLength >= PCX_RUNLENGTH_MASK)
                    break;
                rleLength++;

                x++;
                width--;
            }

            if (rleLength > 1 || rlePixel > PCX_MAX_SINGLE_PIXEL) {
                rleLength |= 0xC0;
                out << rleLength;
            }

            out << rlePixel;
        }
    }

    // - palette
    if (!hasTransparent) {
        transparentIndex = D1PCX_COLORS;
    }
    out << 0x0C;
    for (int i = 0; i < D1PCX_COLORS; i++) {
        static_assert(D1PAL_COLORS == D1PCX_COLORS, "D1Pcx::save uses PCX colors to store the content of a D1Pal.");
        QColor color = pal->getColor(i);
        if (transparentIndex == i) {
            if (params.transparentColor != QColor(Qt::transparent))
                color = params.transparentColor;
            else if (params.transparentIndex != transparentIndex)
                color = pal->getUndefinedColor();
        }
        out << (uint8_t)color.red();
        out << (uint8_t)color.green();
        out << (uint8_t)color.blue();
    }
    return true;
}

void D1Pcx::compare(D1Gfx &gfx, D1Pal *pal, const LoadFileContent *fileContent, bool patchData)
{
    QString header = QApplication::tr("Graphics:");
    gfx.compareTo(fileContent->gfx, header, patchData);
    header = QApplication::tr("Palette:");
    pal->compareTo(fileContent->gfx->getPalette(), header);
}
