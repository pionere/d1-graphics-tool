#include "d1wav.h"

#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QFile>

#include "config.h"
#include "d1smk.h"
#include "progressdialog.h"

typedef struct _WavHeader {
    quint32 RiffMarker;
    quint32 FileSize;
    quint32 WaveMarker;
    quint32 FmtMarker;
    quint32 HeaderLength;
    quint16 FormatType;
    quint16 ChannelCount;
    quint32 SampleRate;
    quint32 SampleRateBytesChannels;
    quint16 BytesChannels;
    quint16 BitsPerSample;
    quint32 DataMarker;
    quint32 DataSize;
} WAVHEADER;

/*bool D1Wav::load(D1Gfx &gfx, D1Pal *pal, const QString &filePath, const OpenAsParam &params)
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

    // gfx.frames.clear();
    D1GfxFrame *frame = new D1GfxFrame();
    gfx.frames.append(frame);

    gfx.gfxFilePath = celPath;
    gfx.modified = true;

    bool dummy;
    return D1Wav::load(*frame, filePath, clipped, pal, nullptr, &dummy);
}

bool D1Wav::load(D1GfxFrame &frame, const QString &filePath, bool clipped, D1Pal *basePal, D1Pal *resPal, bool *palMod)
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
            static_assert(D1PAL_COLORS == D1PCX_COLORS, "D1Wav::load uses D1Pal to store the palette of a PCX file.");
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
    frame.clipped = clipped;
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

        D1ImageFrame::load(frame, image, frame.clipped, resPal);
    }

    if (pcxPal != resPal && pcxPal != basePal) {
        delete pcxPal;
    }
    return true;
}*/

bool D1Wav::save(const D1SmkAudioData *audioData, int track, const QString &filePath, const ExportParam &params)
{
    unsigned long len;
    uint8_t* data = audioData->getAudio(track, &len);

    /*if (len == 0) {
        dProgressFail() << QApplication::tr("WAV format can not store the image due to its dimensions: %1x%2.").arg(imageSize.width()).arg(imageSize.height());
        return false;
    }*/

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
    WAVHEADER header = { 0 };
    header.RiffMarker = SwapLE32(*((uint32_t*)"RIFF"));
    header.FileSize = SwapLE32(len + sizeof(header));
    header.WaveMarker = SwapLE32(*((uint32_t*)"WAVE"));
    header.FmtMarker = SwapLE32(*((uint32_t*)"fmt "));
    header.HeaderLength = SwapLE32(16);
    header.FormatType = SwapLE16(1);
    header.ChannelCount = SwapLE16(audioData->getChannels());
    header.SampleRate = SwapLE32(audioData->getBitRate());
    header.SampleRateBytesChannels = SwapLE32(audioData->getBitRate() * audioData->getChannels() * audioData->getBitDepth() / 8);
    header.BytesChannels = SwapLE16(audioData->getChannels() * audioData->getBitDepth() / 8);
    header.BitsPerSample = SwapLE16(audioData->getBitDepth());
    header.DataMarker = SwapLE32("data");
    header.DataSize = SwapLE32(len  + 8);

    out.writeRawData((char *)&header, sizeof(header));

    // - data
    out.writeRawData((char *)data, len);

    return true;
}
