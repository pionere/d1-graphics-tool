#include "d1cl2frame.h"

#include <QApplication>
#include <QDataStream>

#include "progressdialog.h"

unsigned D1Cl2Frame::computeWidthFromHeader(const QByteArray &rawFrameData)
{
    // Reading the frame header {CEL FRAME HEADER}
    const quint8 *data = (const quint8 *)rawFrameData.constData();
    const quint16 *header = (const quint16 *)data;
    const quint8 *dataEnd = data + rawFrameData.size();

    if (rawFrameData.size() < SUB_HEADER_SIZE)
        return 0; // invalid header
    unsigned celFrameHeaderSize = SwapLE16(header[0]);
    if (celFrameHeaderSize & 1)
        return 0; // invalid header
    if (celFrameHeaderSize < SUB_HEADER_SIZE)
        return 0; // invalid header
    if (data + celFrameHeaderSize > dataEnd)
        return 0; // invalid header
    // Decode the 32 pixel-lines blocks to calculate the image width
    unsigned celFrameWidth = 0;
    quint16 lastFrameOffset = celFrameHeaderSize;
    celFrameHeaderSize /= 2;
    for (unsigned i = 1; i < celFrameHeaderSize; i++) {
        quint16 nextFrameOffset = SwapLE16(header[i]);
        if (nextFrameOffset == 0) {
            // check if the remaining entries are zero
            while (++i < celFrameHeaderSize) {
                if (SwapLE16(header[i]) != 0)
                    return 0; // invalid header
            }
            break;
        }

        unsigned pixelCount = 0;
        // ensure the offsets are consecutive
        if (lastFrameOffset >= nextFrameOffset)
            return 0; // invalid data
        for (int j = lastFrameOffset; j < nextFrameOffset; j++) {
            if (data + j >= dataEnd)
                return 0; // invalid data

            quint8 readByte = data[j];
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
        // The calculated width has to be identical for each 32 pixel-line block
        if (celFrameWidth == 0) {
            if (width == 0)
                return 0; // invalid data
        } else {
            if (celFrameWidth != width)
                return 0; // mismatching width values
        }

        celFrameWidth = width;
        lastFrameOffset = nextFrameOffset;
    }

    return celFrameWidth;
}

int D1Cl2Frame::load(D1GfxFrame &frame, const QByteArray rawData, const OpenAsParam &params, unsigned *rle_len_min, unsigned *rle_len_max)
{
    unsigned width = 0;
    bool clipped = false;
    if (params.clipped == OPEN_CLIPPED_TYPE::AUTODETECT) {
        // Try to compute frame width from frame header
        width = D1Cl2Frame::computeWidthFromHeader(rawData);
        clipped = true; // Assume the presence of the {CEL FRAME HEADER}
    } else {
        clipped = params.clipped == OPEN_CLIPPED_TYPE::TRUE;
        if (clipped) {
            // Try to compute frame width from frame header
            width = D1Cl2Frame::computeWidthFromHeader(rawData);
        }
    }
    frame.width = params.celWidth == 0 ? width : params.celWidth;

    // check if a positive width was found
    if (frame.width == 0)
        return rawData.size() == 0 ? (clipped ? 1 : 0) : -1;

    // READ {CL2 FRAME DATA}
    int frameDataStartOffset = 0;
    if (clipped) {
        if (rawData.size() != 0) {
            if (rawData.size() == 1)
                return -2;
            frameDataStartOffset = SwapLE16(*(const quint16 *)rawData.constData());
            if (frameDataStartOffset > rawData.size())
                return -2;
        }
    }

    std::vector<std::vector<D1GfxPixel>> pixels;
    std::vector<D1GfxPixel> pixelLine;
    unsigned rle_min = 0;
    unsigned rle_max = UINT_MAX;
    for (int o = frameDataStartOffset; o < rawData.size(); o++) {
        quint8 readByte = rawData[o];

        if (/*readByte >= 0x00 &&*/ readByte < 0x80) {
            // Transparent pixels
            /*if (readByte == 0x00) {
                dProgressWarn() << QApplication::tr("Invalid CL2 frame data (0x00 found)");
            }*/
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
            unsigned len = 0xBF - readByte;
            if (len < rle_max) {
                rle_max = len;
                if (len < 3) {
                    dProgressWarn() << QApplication::tr("FIXME Short rle-encoding (%1).").arg(len);
                }
            }
            // Go to the palette index offset
            o++;

            for (unsigned i = 0; i < len; i++) {
                // Add opaque pixel
                pixelLine.push_back(D1GfxPixel::colorPixel(rawData[o]));

                if (pixelLine.size() == frame.width) {
                    pixels.push_back(std::move(pixelLine));
                    pixelLine.clear();
                }
            }
        } else /*if (readByte >= 0xBF && readByte <= 0xFF)*/ {
            // Palette indices
            unsigned nrle = 0;
            unsigned lastColor = 256;
            for (int i = 0; i < (256 - readByte); i++) {
                // Go to the next palette index offset
                o++;
                if (lastColor == rawData[o]) {
                    nrle++;
                    if (nrle > rle_min) {
                        rle_min = nrle;
                    }
                } else {
                    lastColor = rawData[o];
                    nrle = 2;
                }
                // Add opaque pixel
                pixelLine.push_back(D1GfxPixel::colorPixel(rawData[o]));

                if (pixelLine.size() == frame.width) {
                    pixels.push_back(std::move(pixelLine));
                    pixelLine.clear();
                }
            }
        }
    }
    if (!pixelLine.empty()) {
        if (params.clipped == OPEN_CLIPPED_TYPE::AUTODETECT) {
            OpenAsParam oParams = params;
            oParams.clipped = clipped ? OPEN_CLIPPED_TYPE::FALSE : OPEN_CLIPPED_TYPE::TRUE;
            return D1Cl2Frame::load(frame, rawData, oParams, rle_len_min, rle_len_max);
        }

        return -2;
    }

    for (auto it = pixels.rbegin(); it != pixels.rend(); ++it) {
        frame.pixels.push_back(std::move(*it));
    }
    frame.height = frame.pixels.size();

    if (rle_min <= rle_max) {
        *rle_len_min = rle_min;
        *rle_len_max = rle_max;
    } else {
        dProgressWarn() << QApplication::tr("Inconsistent rle-encoding (%1..%2).").arg(rle_min).arg(rle_max);
    }
    return clipped ? 1 : 0;
}
