#include "d2c6frame.h"

#include <QDataStream>

#include "progressdialog.h"

bool D2C6Frame::load(D1GfxFrame &frame, const QByteArray &rawData, const OpenAsParam &params)
{
    QDataStream in(rawData);
    in.setByteOrder(QDataStream::LittleEndian);

    if (rawData.length() < 8 * 4)
        return false;

    quint32 flippedDword;
    in >> flippedDword;

    quint32 widthDword;
    in >> widthDword;

    quint32 heightDword;
    in >> heightDword;

    qint32 offsetXDword;
    in >> offsetXDword;

    qint32 offsetYDword;
    in >> offsetYDword;

    quint32 unknownDword;
    in >> unknownDword;

    quint32 nextBlockDword;
    in >> nextBlockDword;

    quint32 lengthDword;
    in >> lengthDword;

    if (rawData.length() < 8 * 4 + lengthDword + 3)
        return false;

    // calculate the offset
    unsigned frameDataStartOffset = 8 * 4;
    // READ {CEL FRAME DATA}
    std::vector<std::vector<D1GfxPixel>> pixels;
    std::vector<D1GfxPixel> pixelLine;
    for (unsigned o = frameDataStartOffset; o < frameDataStartOffset + lengthDword; o++) {
        quint8 readByte = rawData[o];

        if (readByte == 0x80) {
            // end of line
            while (pixelLine.size() < widthDword) {
                pixelLine.push_back(D1GfxPixel::transparentPixel());
            }
            pixels.push_back(std::move(pixelLine));
            pixelLine.clear();
        } else if (readByte > 0x7F /*&& readByte <= 0xFF*/) {
            // Transparent pixels group
            for (int i = 0; i < (readByte & 0x7F); i++) {
                // Add transparent pixel
                pixelLine.push_back(D1GfxPixel::transparentPixel());
            }
        } else /*if (readByte >= 0x00 && readByte <= 0x7F)*/ {
            // Palette indices group
            if ((o + readByte) >= rawData.size()) {
                pixelLine.push_back(D1GfxPixel::transparentPixel()); // ensure pixelLine is not empty to report error at the end
                break;
            }

            // if (readByte == 0x00) {
            //    dProgressWarn() << QString("Invalid CEL frame data (0x00 found)");
            // }
            for (int i = 0; i < readByte; i++) {
                // Go to the next palette index offset
                o++;
                // Add opaque pixel
                pixelLine.push_back(D1GfxPixel::colorPixel(rawData[o]));
            }
        }
    }
    if (!pixelLine.empty()) {
        return false;
    }

    for (auto it = pixels.rbegin(); it != pixels.rend(); ++it) {
        frame.pixels.push_back(std::move(*it));
    }
    frame.width = widthDword;
    frame.height = frame.pixels.size();
    if (heightDword != frame.height) {
        dProgressWarn() << QString("Invalid C6 frame height %1 (expected %2)").arg(heightDword).arg(frame.height);
    }
    return true;
}
