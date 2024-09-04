#include "d1celframe.h"

#include <QApplication>
#include <QDataStream>

#include "progressdialog.h"

D1CelPixelGroup::D1CelPixelGroup(bool t, unsigned c)
    : transparent(t)
    , pixelCount(c)
{
}

bool D1CelPixelGroup::isTransparent() const
{
    return this->transparent;
}

unsigned D1CelPixelGroup::getPixelCount() const
{
    return this->pixelCount;
}

bool D1CelFrame::load(D1GfxFrame &frame, const QByteArray &rawData, const OpenAsParam &params)
{
    unsigned width = 0;

    // READ {CEL FRAME DATA}
    int frameDataStartOffset = 0;

    std::vector<std::vector<D1GfxPixel>> pixels;
    std::vector<D1GfxPixel> pixelLine;
    for (int o = frameDataStartOffset; o < rawData.size(); o++) {
        quint8 readByte = rawData[o];

        if (readByte > 0x7F /*&& readByte <= 0xFF*/) {
            // Transparent pixels group
            for (int i = 0; i < (256 - readByte); i++) {
                // Add transparent pixel
                pixelLine.push_back(D1GfxPixel::transparentPixel());
            }
        } else /*if (readByte >= 0x00 && readByte <= 0x7F)*/ {
            // Palette indices group
            if ((o + readByte) >= rawData.size()) {
                pixelLine.push_back(D1GfxPixel::transparentPixel()); // ensure pixelLine is not empty to report error at the end
                break;
            }

            if (readByte == 0x00) {
                dProgressWarn() << QApplication::tr("Invalid CEL frame data (0x00 found)");
            }
            for (int i = 0; i < readByte; i++) {
                // Go to the next palette index offset
                o++;
                // Add opaque pixel
                pixelLine.push_back(D1GfxPixel::colorPixel(rawData[o]));
            }
        }

        if (pixelLine.size() >= width) {
            if (pixelLine.size() > width) {
                break;
            }
            pixels.push_back(std::move(pixelLine));
            pixelLine.clear();
        }
    }
    for (auto it = pixels.rbegin(); it != pixels.rend(); ++it) {
        frame.pixels.push_back(std::move(*it));
    }
    frame.width = width;
    frame.height = frame.pixels.size();

    return pixelLine.empty();
}
