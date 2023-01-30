#include "d1pcx.h"

#include <QApplication>
#include <QByteArray>
#include <QList>

#include "config.h"
#include "d1image.h"
#include "progressdialog.h"

#define D1PCX_COLORS 256

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

bool D1Pcx::load(D1GfxFrame &frame, QString pcxFilePath, bool clipped, D1Pal *basePal, D1Pal *resPal, bool *palMod)
{
    // Opening PCX file
    QFile file = QFile(pcxFilePath);

    if (!file.open(QIODevice::ReadOnly)) {
        dProgressErr() << QApplication::tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(pcxFilePath));
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
    if (pcxhdr.Manufacturer != 10) {
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

    // prepare D1GfxFrame
    frame.width = SwapLE16(pcxhdr.Xmax) - SwapLE16(pcxhdr.Xmin) + 1;
    frame.height = SwapLE16(pcxhdr.Ymax) - SwapLE16(pcxhdr.Ymin) + 1;
    frame.clipped = clipped;
    frame.pixels.clear();

    // read data
    QByteArray fileData = file.read(fileSize - sizeof(PCXHEADER));

    // width of PCX data is always even -> need to skip a bit if the width is odd
    const quint8 srcSkip = frame.width % 2;
    const quint8 PCX_MAX_SINGLE_PIXEL = 0xBF;
    const quint8 PCX_RUNLENGTH_MASK = 0x3F;
    auto dataPtr = fileData.cbegin();
    quint8 byte;
    if (pcxPalState == 1) {
        // read using the given palette
        QSet<quint8> usedColors;
        for (int y = 0; y < frame.height; y++) {
            QList<D1GfxPixel> pixelLine;

            for (int x = 0; x < frame.width; dataPtr++) {
                byte = *dataPtr;
                if (byte <= PCX_MAX_SINGLE_PIXEL) {
                    if (pcxPal->getColor(byte) != basePal->getUndefinedColor()) {
                        pixelLine.append(D1GfxPixel::colorPixel(byte));
                        usedColors.insert(byte);
                    } else {
                        pixelLine.append(D1GfxPixel::transparentPixel()); // color of the PCX-palette is undefined -> transparent
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
                    pixelLine.append(pCol);
                }
                x += byte;
            }
            frame.pixels.append(pixelLine);
            dataPtr += srcSkip;
        }

        // update the undefined colors of the palette
        for (int i = 0; i < D1PCX_COLORS; i++) {
            QColor col = basePal->getColor(i);
            // if (col != undefColor) {
            //     continue; // skip colors if already defined
            // }
            if (usedColors.find(i) == usedColors.end()) {
                continue; // ignore unused colors
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
        for (int y = 0; y < frame.height; y++) {
            for (int x = 0; x < frame.width; dataPtr++) {
                byte = *dataPtr;
                if (byte <= PCX_MAX_SINGLE_PIXEL) {
                    QColor color = pcxPal->getColor(byte);
                    if (color == resPal->getUndefinedColor()) {
                        color = Qt::transparent;
                    }
                    image.setPixelColor(x, y, color);
                    x++;
                    continue;
                }
                byte &= PCX_RUNLENGTH_MASK;
                dataPtr++;
                quint8 col = *dataPtr;
                QColor color = pcxPal->getColor(col);
                if (color == resPal->getUndefinedColor()) {
                    color = Qt::transparent;
                }
                for (int i = 0; i < byte; i++) {
                    image.setPixelColor(x + i, y, color);
                }
                x += byte;
            }
            dataPtr += srcSkip;
        }

        D1ImageFrame::load(frame, image, frame.clipped, resPal);
    }

    if (pcxPal != resPal) {
        delete pcxPal;
    }
    return true;
}