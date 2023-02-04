#include "d1celframe.h"

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
    if (rawData.size() == 0)
        return false;

    quint32 frameDataStartOffset = 0;
    unsigned width = 0;
    frame.clipped = false;
    if (params.clipped == OPEN_CLIPPED_TYPE::AUTODETECT) {
        // Checking the presence of the {CEL FRAME HEADER}
        if ((quint8)rawData[0] == 0x0A && (quint8)rawData[1] == 0x00) {
            frameDataStartOffset += 0x0A;
            // If header is present, try to compute frame width from frame header
            width = D1CelFrame::computeWidthFromHeader(rawData);
            frame.clipped = true;
        }
    } else {
        if (params.clipped == OPEN_CLIPPED_TYPE::TRUE) {
            QDataStream in(rawData);
            in.setByteOrder(QDataStream::LittleEndian);
            quint16 offset;
            in >> offset;
            frameDataStartOffset += offset;
            // If header is present, try to compute frame width from frame header
            width = D1CelFrame::computeWidthFromHeader(rawData);
            frame.clipped = true;
        }
    }
    if (params.celWidth != 0)
        width = params.celWidth;

    // If width could not be calculated with frame header,
    // attempt to calculate it from the frame data (by identifying pixel groups line wraps)
    if (width == 0)
        width = D1CelFrame::computeWidthFromData(rawData);

    // if CEL width was not found, return false
    if (width == 0)
        return false;

    // READ {CEL FRAME DATA}
    std::vector<std::vector<D1GfxPixel>> pixels;
    std::vector<D1GfxPixel> pixelLine;
    for (int o = frameDataStartOffset; o < rawData.size(); o++) {
        quint8 readByte = rawData[o];

        // Transparent pixels group
        if (readByte > 0x7F) {
            // A pixel line can't exceed the image width
            if ((pixelLine.size() + (256 - readByte)) > width)
                return false;

            for (int i = 0; i < (256 - readByte); i++)
                pixelLine.push_back(D1GfxPixel::transparentPixel());
        } else {
            // Palette indices group
            // A pixel line can't exceed the image width
            if ((pixelLine.size() + readByte) > width)
                return false;

            for (int i = 0; i < readByte; i++) {
                o++;
                pixelLine.push_back(D1GfxPixel::colorPixel(rawData[o]));
            }
        }

        if (pixelLine.size() == width) {
            pixels.push_back(std::move(pixelLine));
            pixelLine.clear();
        }
    }
    for (auto it = pixels.rbegin(); it != pixels.rend(); ++it) {
        frame.pixels.push_back(std::move(*it));
    }
    frame.width = width;
    frame.height = frame.pixels.size();

    return true;
}

unsigned D1CelFrame::computeWidthFromHeader(const QByteArray &rawFrameData)
{
    // Reading the frame header
    QDataStream in(rawFrameData);
    in.setByteOrder(QDataStream::LittleEndian);

    quint16 celFrameHeaderSize;
    in >> celFrameHeaderSize;

    if (celFrameHeaderSize & 1)
        return 0; // invalid header

    // Decode the 32 pixel-lines blocks to calculate the image width
    unsigned celFrameWidth = 0;
    quint16 lastFrameOffset = celFrameHeaderSize;
    for (int i = 0; i < (celFrameHeaderSize / 2) - 1; i++) {
        quint16 nextFrameOffset;
        in >> nextFrameOffset;
        if (nextFrameOffset == 0)
            break;

        unsigned pixelCount = 0;
        for (int j = lastFrameOffset; j < nextFrameOffset; j++) {
            quint8 readByte = rawFrameData[j];

            if (readByte > 0x7F) {
                pixelCount += (256 - readByte);
            } else {
                pixelCount += readByte;
                j += readByte;
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

unsigned D1CelFrame::computeWidthFromData(const QByteArray &rawFrameData)
{
    unsigned biggestGroupPixelCount = 0;
    unsigned pixelCount = 0;
    unsigned width = 0;
    std::vector<D1CelPixelGroup> pixelGroups;

    // Checking the presence of the {CEL FRAME HEADER}
    quint32 frameDataStartOffset = 0;
    if ((quint8)rawFrameData[0] == 0x0A && (quint8)rawFrameData[1] == 0x00)
        frameDataStartOffset = 0x0A;

    // Going through the frame data to find pixel groups
    unsigned globalPixelCount = 0;
    for (int o = frameDataStartOffset; o < rawFrameData.size(); o++) {
        quint8 readByte = rawFrameData[o];

        // Transparent pixels group
        if (readByte > 0x80) {
            pixelCount += (256 - readByte);
            pixelGroups.push_back(D1CelPixelGroup(true, pixelCount));
            globalPixelCount += pixelCount;
            if (pixelCount > biggestGroupPixelCount)
                biggestGroupPixelCount = pixelCount;
            pixelCount = 0;
        } else if (readByte == 0x80) {
            pixelCount += 0x80;
        }
        // Palette indices pixel group
        else if (readByte == 0x7F) {
            pixelCount += 0x7F;
            o += 0x7F;
        } else {
            pixelCount += readByte;
            pixelGroups.push_back(D1CelPixelGroup(false, pixelCount));
            globalPixelCount += pixelCount;
            if (pixelCount > biggestGroupPixelCount)
                biggestGroupPixelCount = pixelCount;
            pixelCount = 0;
            o += readByte;
        }
    }

    // Going through pixel groups to find pixel-lines wraps
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
            if (i == pixelGroups.size() - 1 && pixelGroups[i].getPixelCount() < pixelCount)
                width = pixelGroups[i].getPixelCount();

            pixelCount = 0;
        }

        // If last pixel group is being processed and width is still unknown
        // then set the width to the pixelCount of the last two pixel groups
        if (i == pixelGroups.size() - 1 && width == 0) {
            width = pixelGroups[i - 1].getPixelCount() + pixelGroups[i].getPixelCount();
        }
    }

    // If width wasnt found return 0
    if (width == 0) {
        return 0;
    }

    // If width is consistent
    if ((globalPixelCount % width) == 0) {
        return width;
    }

    // Try to find  relevant width by adding pixel groups' pixel counts iteratively
    pixelCount = 0;
    for (unsigned i = 0; i < pixelGroups.size(); i++) {
        pixelCount += pixelGroups[i].getPixelCount();
        if (pixelCount > 1
            && (globalPixelCount % pixelCount) == 0
            && pixelCount >= biggestGroupPixelCount) {
            return pixelCount;
        }
    }

    // If still no width found return 0
    return 0;
}
