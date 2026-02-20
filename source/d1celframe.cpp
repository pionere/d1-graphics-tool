#include "d1celframe.h"

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

int D1CelFrame::load(D1GfxFrame &frame, const QByteArray &rawData, const OpenAsParam &params)
{
    unsigned width = 0;
    bool clipped = false;
    if (params.clipped == OPEN_CLIPPED_TYPE::AUTODETECT) {
        // Try to compute frame width from frame header
        width = D1CelFrame::computeWidthFromHeader(rawData);
        clipped = width != 0 || (rawData.size() >= SUB_HEADER_SIZE && SwapLE16(*(const quint16 *)rawData.constData()) == SUB_HEADER_SIZE);
    } else {
        clipped = params.clipped == OPEN_CLIPPED_TYPE::TRUE;
        if (clipped) {
            // Try to compute frame width from frame header
            width = D1CelFrame::computeWidthFromHeader(rawData);
        }
    }
    if (params.celWidth != 0) {
        width = params.celWidth;
    }
    // If width could not be calculated with frame header,
    // attempt to calculate it from the frame data (by identifying pixel groups line wraps)
    if (width == 0) {
        width = D1CelFrame::computeWidthFromData(rawData, clipped);
    }
    // check if a positive width was found
    if (width == 0) {
        return rawData.size() == 0 ? (clipped ? 1 : 0) : -1;
    }
    // calculate the offset in case of a clipped frame
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
    // READ {CEL FRAME DATA}
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

        if (pixelLine.size() >= width) {
            if (pixelLine.size() > width) {
                break;
            }
            pixels.push_back(std::move(pixelLine));
            pixelLine.clear();
        }
    }
    if (!pixelLine.empty()) {
        if (params.clipped == OPEN_CLIPPED_TYPE::AUTODETECT) {
            OpenAsParam oParams = params;
            oParams.clipped = clipped ? OPEN_CLIPPED_TYPE::FALSE : OPEN_CLIPPED_TYPE::TRUE;
            return D1CelFrame::load(frame, rawData, oParams);
        }

        return -2;
    }

    for (auto it = pixels.rbegin(); it != pixels.rend(); ++it) {
        frame.pixels.push_back(std::move(*it));
    }
    frame.width = width;
    frame.height = frame.pixels.size();
    return clipped ? 1 : 0;
}

unsigned D1CelFrame::computeWidthFromHeader(const QByteArray &rawFrameData)
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
            if (celFrameWidth != 0)
                break;
            // last attempt using the size of the frame
            nextFrameOffset = rawFrameData.size();
        }

        unsigned pixelCount = 0;
        // ensure the offsets are consecutive
        if (lastFrameOffset >= nextFrameOffset)
            return 0; // invalid data
        // calculate width based on the data-block
        for (int j = lastFrameOffset; j < nextFrameOffset; j++) {
            if (data + j >= dataEnd)
                return 0; // invalid data

            quint8 readByte = data[j];
            if (readByte > 0x7F /*&& readByte <= 0xFF*/) {
                // Transparent pixels group
                pixelCount += (256 - readByte);
            } else /*if (readByte >= 0x00 && readByte <= 0x7F)*/ {
                // Palette indices group
                pixelCount += readByte;
                j += readByte;
            }
        }
        if (pixelCount % CEL_BLOCK_HEIGHT)
            return false; // invalid block
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

static bool isValidWidth(unsigned width, unsigned globalPixelCount, const std::vector<D1CelPixelGroup> &pixelGroups)
{
    if ((globalPixelCount % width) != 0)
        return false;

    unsigned pixelCount = 0;
    for (unsigned i = 1; i < pixelGroups.size(); i++) {
        unsigned cpc = pixelGroups[i - 1].getPixelCount();
        // gpc: [width]*lpc
        unsigned lpc = pixelCount % width; // pixels in the last line
        unsigned rpc = width - lpc; // possible remaining pixels in the last line
        if (cpc > rpc) {
            if (pixelGroups[i - 1].isTransparent()) {
                // cpc: [0x80]*[width]*[0x80]*pc
                if ((rpc % 0x80) != 0)
                    return false; // group does not align with width
            } else {
                // cpc: [0x7F]*[width]*[0x7F]*pc
                if ((rpc % 0x7F) != 0)
                    return false; // group does not align with width
            }
        }
        pixelCount += cpc;
        if (pixelGroups[i - 1].isTransparent() == pixelGroups[i].isTransparent()) {
            if ((pixelCount % width) != 0) {
                return false; // last line(?) does not fit to the width
            }
            pixelCount = 0;
        }
    }
    return true;
}

unsigned D1CelFrame::computeWidthFromData(const QByteArray &rawFrameData, bool clipped)
{
    unsigned pixelCount, width, globalPixelCount;
    std::vector<D1CelPixelGroup> pixelGroups;

    // Checking the presence of the {CEL FRAME HEADER}
    int frameDataStartOffset = 0;
    if (clipped && rawFrameData.size() >= SUB_HEADER_SIZE)
        frameDataStartOffset = SwapLE16(*(const quint16 *)rawFrameData.constData());

    // Going through the frame data to find pixel groups
    pixelCount = 0;
    bool alpha = false;
    for (int o = frameDataStartOffset; o < rawFrameData.size(); o++) {
        quint8 readByte = rawFrameData[o];

        if (readByte >= 0x80 /*&& readByte <= 0xFF*/) {
            // Transparent pixels group
            if (!alpha && pixelCount != 0) {
                pixelGroups.push_back(D1CelPixelGroup(false, pixelCount));
                pixelCount = 0;
            }
            alpha = true;
            pixelCount += (256 - readByte);
            if (readByte != 0x80) {
                pixelGroups.push_back(D1CelPixelGroup(true, pixelCount));
                pixelCount = 0;
            }
        } else /*if (readByte >= 0x00 && readByte <= 0x7F)*/ {
            // Palette indices pixel group
            if (alpha && pixelCount != 0) {
                pixelGroups.push_back(D1CelPixelGroup(true, pixelCount));
                pixelCount = 0;
            }
            alpha = false;
            pixelCount += readByte;
            if (readByte != 0x7F) {
                pixelGroups.push_back(D1CelPixelGroup(false, pixelCount));
                pixelCount = 0;
            }
            o += readByte;
        }
    }
    if (pixelCount != 0) {
        pixelGroups.push_back(D1CelPixelGroup(alpha, pixelCount));
    }

    if (pixelGroups.size() <= 1) {
        if (pixelGroups.size() == 0)
            return 0; // empty frame
        // single group -> completely transparent or completely opaque frame with the maximum possible (or its multiple) width (or a single line)
        globalPixelCount = pixelGroups.front().getPixelCount();
        width = pixelGroups.front().isTransparent() ? 0x80 : 0x7F; // select the smallest possible width
        if ((globalPixelCount % width) == 0) {
            return width;
        }
        return globalPixelCount; // single line
    }

    // Going through pixel groups to find pixel-lines wraps
    width = 0;
    pixelCount = 0;
    for (unsigned i = 1; i < pixelGroups.size(); i++) {
        pixelCount += pixelGroups[i - 1].getPixelCount();

        if (pixelGroups[i - 1].isTransparent() == pixelGroups[i].isTransparent()) {
            // If width == 0 then it's the first pixel-line wrap and width needs to be set
            // If pixelCount is less than width then the width has to be set to the new value
            if (width == 0 || pixelCount < width)
                width = pixelCount;

            // If the pixelCount of the last group is less than the current pixel group
            // then width is equal to this last pixel group's pixel count.
            // Mostly useful for small frames like the "J" frame in smaltext.cel
            if (i == pixelGroups.size() - 1 && pixelGroups[i].getPixelCount() < width)
                width = pixelGroups[i].getPixelCount();

            pixelCount = 0;
        }
    }

    globalPixelCount = 0;
    for (const D1CelPixelGroup &pixelGroup : pixelGroups) {
        pixelCount = pixelGroup.getPixelCount();
        globalPixelCount += pixelCount;
    }

    if (width != 0 && isValidWidth(width, globalPixelCount, pixelGroups)) {
        return width; // width is consistent -> done
    }

    // try possible widths
    for (width = 2; width <= globalPixelCount / 2; width++) {
        if (isValidWidth(width, globalPixelCount, pixelGroups)) {
            return width;
        }
    }

    // If still no width found return 0
    return 0;
}
