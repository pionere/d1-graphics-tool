#include "d1cl2frame.h"

#include <QApplication>
#include <QDataStream>

#include "progressdialog.h"

unsigned D1Cl2Frame::computeWidthFromHeader(const QByteArray &rawFrameData)
{
    QDataStream in(rawFrameData);
    in.setByteOrder(QDataStream::LittleEndian);

    quint16 celFrameHeaderSize;
    in >> celFrameHeaderSize;

    if (celFrameHeaderSize & 1)
        return 0; // invalid header

    unsigned celFrameWidth = 0;

    // Decode the 32 pixel-lines blocks to calculate the image width
    quint16 lastFrameOffset = celFrameHeaderSize;
    for (int i = 0; i < (celFrameHeaderSize / 2) - 1; i++) {
        quint16 pixelCount = 0;
        quint16 nextFrameOffset;
        in >> nextFrameOffset;
        if (nextFrameOffset == 0)
            break;

        for (int j = lastFrameOffset; j < nextFrameOffset; j++) {
            quint8 readByte = rawFrameData[j];

            if (/*readByte >= 0x00 &&*/ readByte < 0x80) {
                // Transparent pixels
                pixelCount += readByte;
            } else if (/*readByte >= 0x80 &&*/ readByte < 0xBF) {
                // RLE encoded palette index
                pixelCount += (0xBF - readByte);
                j++;
            } else /*if (readByte >= 0xBF && readByte <= 0xFF)*/ {
                // Palette indices
                pixelCount += (256 - readByte);
                j += (256 - readByte);
            }
        }

        unsigned width = pixelCount / CEL_BLOCK_HEIGHT;
        // The calculated width has to be the identical for each 32 pixel-line block
        // If it's not the case, 0 is returned
        if (celFrameWidth != 0 && celFrameWidth != width)
            return 0;

        celFrameWidth = width;
        lastFrameOffset = nextFrameOffset;
    }

    return celFrameWidth;
}

bool D1Cl2Frame::load(D1GfxFrame &frame, const QByteArray rawData, const OpenAsParam &params)
{
    if (rawData.size() == 0)
        return false;

    quint32 frameDataStartOffset = 0;

    unsigned width = 0;
    // frame.clipped = false;
    if (params.clipped == OPEN_CLIPPED_TYPE::AUTODETECT) {
        // Assume the presence of the {CEL FRAME HEADER}
        QDataStream in(rawData);
        in.setByteOrder(QDataStream::LittleEndian);
        quint16 offset;
        in >> offset;
        frameDataStartOffset += offset;
        // If header is present, try to compute frame width from frame header
        width = D1Cl2Frame::computeWidthFromHeader(rawData);
        frame.clipped = true;
    } else {
        if (params.clipped == OPEN_CLIPPED_TYPE::TRUE) {
            QDataStream in(rawData);
            in.setByteOrder(QDataStream::LittleEndian);
            quint16 offset;
            in >> offset;
            frameDataStartOffset += offset;
            // If header is present, try to compute frame width from frame header
            width = D1Cl2Frame::computeWidthFromHeader(rawData);
            frame.clipped = true;
        }
    }
    frame.width = params.celWidth == 0 ? width : params.celWidth;

    if (frame.width == 0)
        return false;

    // READ {CL2 FRAME DATA}
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
