#include "d1cl2frame.h"

#include <QApplication>
#include <QDataStream>

#include "progressdialog.h"

bool D1Cl2Frame::load(D1GfxFrame &frame, const QByteArray rawData, const OpenAsParam &params)
{
    unsigned width = 0;
    frame.width = width;

    // check if a positive width was found
    if (frame.width == 0)
        return rawData.size() == 0;

    // READ {CL2 FRAME DATA}
    int frameDataStartOffset = 0;
    if (frame.clipped && rawData.size() >= SUB_HEADER_SIZE)
        frameDataStartOffset = SwapLE16(*(const quint16 *)rawData.constData());

    std::vector<std::vector<D1GfxPixel>> pixels;
    std::vector<D1GfxPixel> pixelLine;
    for (int o = frameDataStartOffset; o < rawData.size(); o++) {
        quint8 readByte = rawData[o];

        if (/*readByte >= 0x00 &&*/ readByte < 0x80) {
            // Transparent pixels
            if (readByte == 0x00) {
                dProgressWarn() << QApplication::tr("Invalid CL2 frame data (0x00 found)");
            }
            for (int i = 0; i < readByte; i++) {
                // Add transparent pixel
                pixelLine.push_back(D1GfxPixel::transparentPixel());

                if (pixelLine.size() == frame.width) {
                    pixels.push_back(std::move(pixelLine));
                    pixelLine.clear();
                }
            }
        } else if (/*readByte >= 0x80 &&*/ readByte < 0xBF) {
            // RLE encoded palette index
            // Go to the palette index offset
            o++;

            for (int i = 0; i < (0xBF - readByte); i++) {
                // Add opaque pixel
                pixelLine.push_back(D1GfxPixel::colorPixel(rawData[o]));

                if (pixelLine.size() == frame.width) {
                    pixels.push_back(std::move(pixelLine));
                    pixelLine.clear();
                }
            }
        } else /*if (readByte >= 0xBF && readByte <= 0xFF)*/ {
            // Palette indices
            for (int i = 0; i < (256 - readByte); i++) {
                // Go to the next palette index offset
                o++;
                // Add opaque pixel
                pixelLine.push_back(D1GfxPixel::colorPixel(rawData[o]));

                if (pixelLine.size() == frame.width) {
                    pixels.push_back(std::move(pixelLine));
                    pixelLine.clear();
                }
            }
        }
    }
    for (auto it = pixels.rbegin(); it != pixels.rend(); ++it) {
        frame.pixels.push_back(std::move(*it));
    }
    frame.height = frame.pixels.size();

    return true;
}
