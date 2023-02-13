#include "d1celtilesetframe.h"

#include <QApplication>
#include <QMessageBox>

#include "d1gfx.h"
#include "progressdialog.h"

bool D1CelTilesetFrame::load(D1GfxFrame &frame, D1CEL_FRAME_TYPE type, QByteArray rawData, const OpenAsParam &params)
{
    (void)params; // unused

    frame.width = MICRO_WIDTH;
    frame.height = MICRO_HEIGHT;
    frame.frameType = type;
    frame.clipped = false;
    for (int i = 0; i < MICRO_HEIGHT /* frame.height */; i++) {
        frame.pixels.push_back(std::vector<D1GfxPixel>());
    }
    bool valid = false;
    switch (type) {
    case D1CEL_FRAME_TYPE::Square:
        valid = D1CelTilesetFrame::LoadSquare(frame, rawData);
        break;
    case D1CEL_FRAME_TYPE::TransparentSquare:
        valid = D1CelTilesetFrame::LoadTransparentSquare(frame, rawData);
        break;
    case D1CEL_FRAME_TYPE::LeftTriangle:
        valid = D1CelTilesetFrame::LoadLeftTriangle(frame, rawData);
        break;
    case D1CEL_FRAME_TYPE::RightTriangle:
        valid = D1CelTilesetFrame::LoadRightTriangle(frame, rawData);
        break;
    case D1CEL_FRAME_TYPE::LeftTrapezoid:
        valid = D1CelTilesetFrame::LoadLeftTrapezoid(frame, rawData);
        break;
    case D1CEL_FRAME_TYPE::RightTrapezoid:
        valid = D1CelTilesetFrame::LoadRightTrapezoid(frame, rawData);
        break;
    }
    // ensure the result is a valid frame
    if (!valid) {
        for (std::vector<D1GfxPixel> &pixelLine : frame.pixels) {
            while (pixelLine.size() < MICRO_WIDTH /* frame.width */) {
                pixelLine.push_back(D1GfxPixel::transparentPixel());
            }
        }
    }

    return valid;
}

bool D1CelTilesetFrame::LoadSquare(D1GfxFrame &frame, const QByteArray &rawData)
{
    int offset = 0;
    for (int i = MICRO_HEIGHT /* frame.height */ - 1; i >= 0; i--) {
        std::vector<D1GfxPixel> &pixelLine = frame.pixels[i];
        if (rawData.size() < offset + MICRO_WIDTH /* frame.width */)
            return false;
        for (int j = 0; j < MICRO_WIDTH /* frame.width */; j++) {
            pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
        }
    }
    return true;
}

bool D1CelTilesetFrame::LoadTransparentSquare(D1GfxFrame &frame, const QByteArray &rawData)
{
    int offset = 0;
    for (int i = MICRO_HEIGHT /* frame.height */ - 1; i >= 0; i--) {
        std::vector<D1GfxPixel> &pixelLine = frame.pixels[i];
        for (int j = 0; j < MICRO_WIDTH /* frame.width */;) {
            if (rawData.size() <= offset)
                return false;
            qint8 readByte = rawData[offset++];
            if (readByte < 0) {
                readByte = -readByte;
                // transparent pixels
                D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                for (int j = 0; j < readByte; j++) {
                    pixelLine.push_back(pixel);
                }
            } else {
                if (rawData.size() < offset + readByte)
                    return false;
                // color pixels
                for (int j = 0; j < readByte; j++) {
                    pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
                }
            }
            j += readByte;
        }
    }
    return true;
}

bool D1CelTilesetFrame::LoadBottomLeftTriangle(D1GfxFrame &frame, const QByteArray &rawData)
{
    int offset = 0;
    for (int i = 1; i <= MICRO_HEIGHT /* frame.height */ / 2; i++) {
        offset += 2 * (i % 2);
        std::vector<D1GfxPixel> &pixelLine = frame.pixels[MICRO_HEIGHT /* frame.height */ - i];
        for (int j = 0; j < MICRO_WIDTH /* frame.width */ - 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::transparentPixel());
        }
        if (rawData.size() < offset + 2 * i)
            return false;
        for (int j = 0; j < 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
        }
    }
    return true;
}

bool D1CelTilesetFrame::LoadBottomRightTriangle(D1GfxFrame &frame, const QByteArray &rawData)
{
    int offset = 0;
    for (int i = 1; i <= MICRO_HEIGHT /* frame.height */ / 2; i++) {
        std::vector<D1GfxPixel> &pixelLine = frame.pixels[MICRO_HEIGHT /* frame.height */ - i];
        if (rawData.size() < offset + 2 * i)
            return false;
        for (int j = 0; j < 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
        }
        for (int j = 0; j < MICRO_WIDTH /* frame.width */ - 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::transparentPixel());
        }
        offset += 2 * (i % 2);
    }
    return true;
}

bool D1CelTilesetFrame::LoadLeftTriangle(D1GfxFrame &frame, const QByteArray &rawData)
{
    if (!D1CelTilesetFrame::LoadBottomLeftTriangle(frame, rawData))
        return false;

    int offset = 288;
    for (int i = 1; i <= MICRO_HEIGHT /* frame.height */ / 2; i++) {
        offset += 2 * (i % 2);
        std::vector<D1GfxPixel> &pixelLine = frame.pixels[MICRO_HEIGHT /* frame.height */ / 2 - i];
        for (int j = 0; j < 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::transparentPixel());
        }
        if (rawData.size() < offset + MICRO_WIDTH /* frame.width */ - 2 * i)
            return false;
        for (int j = 0; j < MICRO_WIDTH /* frame.width */ - 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
        }
    }
    return true;
}

bool D1CelTilesetFrame::LoadRightTriangle(D1GfxFrame &frame, const QByteArray &rawData)
{
    if (!D1CelTilesetFrame::LoadBottomRightTriangle(frame, rawData))
        return false;

    int offset = 288;
    for (int i = 1; i <= MICRO_HEIGHT /* frame.height */ / 2; i++) {
        std::vector<D1GfxPixel> &pixelLine = frame.pixels[MICRO_HEIGHT /* frame.height */ / 2 - i];
        if (rawData.size() < offset + MICRO_WIDTH /* frame.width */ - 2 * i)
            return false;
        for (int j = 0; j < MICRO_WIDTH /* frame.width */ - 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
        }
        for (int j = 0; j < 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::transparentPixel());
        }
        offset += 2 * (i % 2);
    }
    return true;
}

bool D1CelTilesetFrame::LoadTopHalfSquare(D1GfxFrame &frame, const QByteArray &rawData)
{
    int offset = 288;
    for (int i = 1; i <= MICRO_HEIGHT /* frame.height */ / 2; i++) {
        std::vector<D1GfxPixel> &pixelLine = frame.pixels[MICRO_HEIGHT /* frame.height */ / 2 - i];
        if (rawData.size() < offset + MICRO_WIDTH /* frame.width */)
            return false;
        for (int j = 0; j < MICRO_WIDTH /* frame.width */; j++) {
            pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
        }
    }
    return true;
}

bool D1CelTilesetFrame::LoadLeftTrapezoid(D1GfxFrame &frame, const QByteArray &rawData)
{
    return D1CelTilesetFrame::LoadBottomLeftTriangle(frame, rawData) && D1CelTilesetFrame::LoadTopHalfSquare(frame, rawData);
}

bool D1CelTilesetFrame::LoadRightTrapezoid(D1GfxFrame &frame, const QByteArray &rawData)
{
    return D1CelTilesetFrame::LoadBottomRightTriangle(frame, rawData) && D1CelTilesetFrame::LoadTopHalfSquare(frame, rawData);
}

quint8 *D1CelTilesetFrame::writeFrameData(D1GfxFrame &frame, quint8 *pDst)
{
    switch (frame.frameType) {
    case D1CEL_FRAME_TYPE::LeftTriangle:
        pDst = D1CelTilesetFrame::WriteLeftTriangle(frame, pDst);
        break;
    case D1CEL_FRAME_TYPE::RightTriangle:
        pDst = D1CelTilesetFrame::WriteRightTriangle(frame, pDst);
        break;
    case D1CEL_FRAME_TYPE::LeftTrapezoid:
        pDst = D1CelTilesetFrame::WriteLeftTrapezoid(frame, pDst);
        break;
    case D1CEL_FRAME_TYPE::RightTrapezoid:
        pDst = D1CelTilesetFrame::WriteRightTrapezoid(frame, pDst);
        break;
    case D1CEL_FRAME_TYPE::Square:
        pDst = D1CelTilesetFrame::WriteSquare(frame, pDst);
        break;
    case D1CEL_FRAME_TYPE::TransparentSquare:
        pDst = D1CelTilesetFrame::WriteTransparentSquare(frame, pDst);
        break;
    default:
        // case D1CEL_FRAME_TYPE::Unknown:
        dProgressErr() << QApplication::tr("Unknown frame type.");
        break;
    }
    return pDst;
}

quint8 *D1CelTilesetFrame::WriteSquare(const D1GfxFrame &frame, quint8 *pDst)
{
    int x, y;
    // int length = MICRO_WIDTH * MICRO_HEIGHT;

    // add opaque pixels
    for (y = MICRO_HEIGHT - 1; y >= 0; y--) {
        for (x = 0; x < MICRO_WIDTH; ++x) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid transparent pixel in a Square frame.");
                // return pDst;
            }
            *pDst = pixel.getPaletteIndex();
            ++pDst;
        }
    }
    return pDst;
}

quint8 *D1CelTilesetFrame::WriteTransparentSquare(const D1GfxFrame &frame, quint8 *pDst)
{
    int x, y;
    // int length = MICRO_WIDTH * MICRO_HEIGHT;
    bool hasColor = false;
    quint8 *pStart = pDst;
    quint8 *pHead = pDst;
    pDst++;
    for (y = MICRO_HEIGHT - 1; y >= 0; y--) {
        bool alpha = false;
        for (x = 0; x < MICRO_WIDTH; x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (pixel.isTransparent()) {
                // add transparent pixel
                if ((char)(*pHead) > 0) {
                    pHead = pDst;
                    pDst++;
                }
                --*pHead;
                alpha = true;
            } else {
                // add opaque pixel
                if (alpha) {
                    alpha = false;
                    pHead = pDst;
                    pDst++;
                }
                *pDst = pixel.getPaletteIndex();
                pDst++;
                ++*pHead;
                hasColor = true;
            }
        }
        pHead = pDst;
        pDst++;
    }
    // if (!hasColor) {
    //     qDebug() << "Empty transparent frame"; -- TODO: log empty frame?
    // }
    // pHead = (quint8 *)(((qsizetype)pHead + 3) & (~(qsizetype)3));
    // preserve 4-byte alignment
    pHead += (4 - (((qsizetype)pHead - (qsizetype)pStart) & 3)) & 3;
    return pHead;
}

quint8 *D1CelTilesetFrame::WriteLeftTriangle(const D1GfxFrame &frame, quint8 *pDst)
{
    int i, x, y;
    // int length = MICRO_WIDTH * MICRO_HEIGHT / 2 + MICRO_HEIGHT;

    // memset(pDst, 0, length); -- unnecessary for the game, and the current user did this anyway
    y = MICRO_HEIGHT - 1;
    for (i = MICRO_HEIGHT - 2; i >= 0; i -= 2, y--) {
        // check transparent pixels
        for (x = 0; x < i; x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (!pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid non-transparent pixel in the bottom part of the Left Triangle frame.");
                // return pDst;
            }
        }
        pDst += i & 2;
        // add opaque pixels
        for (x = i; x < MICRO_WIDTH; x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid transparent pixel in the bottom part of the Left Triangle frame.");
                // return pDst;
            }
            *pDst = pixel.getPaletteIndex();
            ++pDst;
        }
    }

    for (i = 2; i != MICRO_HEIGHT; i += 2, y--) {
        // check transparent pixels
        for (x = 0; x < i; x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (!pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid non-transparent pixel in the top part of the Left Triangle frame.");
                // return pDst;
            }
        }
        pDst += i & 2;
        // add opaque pixels
        for (x = i; x < MICRO_WIDTH; ++x) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid transparent pixel in the top part of the Left Triangle frame.");
                // return pDst;
            }
            *pDst = pixel.getPaletteIndex();
            ++pDst;
        }
    }
    return pDst;
}

quint8 *D1CelTilesetFrame::WriteRightTriangle(const D1GfxFrame &frame, quint8 *pDst)
{
    int i, x, y;
    // int length = MICRO_WIDTH * MICRO_HEIGHT / 2 + MICRO_HEIGHT;

    // memset(pDst, 0, length); -- unnecessary for the game, and the current user did this anyway
    y = MICRO_HEIGHT - 1;
    for (i = MICRO_HEIGHT - 2; i >= 0; i -= 2, y--) {
        // add opaque pixels
        for (x = 0; x < (MICRO_WIDTH - i); x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid transparent pixel in the bottom part of the Right Triangle frame.");
                // return pDst;
            }
            *pDst = pixel.getPaletteIndex();
            ++pDst;
        }
        pDst += i & 2;
        // check transparent pixels
        for (x = MICRO_WIDTH - i; x < MICRO_WIDTH; x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (!pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid non-transparent pixel in the bottom part of the Right Triangle frame.");
                // return pDst;
            }
        }
    }

    for (i = 2; i != MICRO_HEIGHT; i += 2, y--) {
        // add opaque pixels
        for (x = 0; x < (MICRO_WIDTH - i); x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid transparent pixel in the top part of the Right Triangle frame.");
                // return pDst;
            }
            *pDst = pixel.getPaletteIndex();
            ++pDst;
        }
        pDst += i & 2;
        // check transparent pixels
        for (x = MICRO_WIDTH - i; x < MICRO_WIDTH; x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (!pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid non-transparent pixel in the top part of the Right Triangle frame.");
                // return pDst;
            }
        }
    }
    return pDst;
}

quint8 *D1CelTilesetFrame::WriteLeftTrapezoid(const D1GfxFrame &frame, quint8 *pDst)
{
    int i, x, y;
    // int length = (MICRO_WIDTH * MICRO_HEIGHT) / 2 + MICRO_HEIGHT * (2 + MICRO_HEIGHT) / 4 + MICRO_HEIGHT / 2;

    // memset(pDst, 0, length); -- unnecessary for the game, and the current user did this anyway
    y = MICRO_HEIGHT - 1;
    for (i = MICRO_HEIGHT - 2; i >= 0; i -= 2, y--) {
        // check transparent pixels
        for (x = 0; x < i; x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (!pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid non-transparent pixel in the bottom part of the Left Trapezoid frame.");
                // return pDst;
            }
        }
        pDst += i & 2;
        // add opaque pixels
        for (x = i; x < MICRO_WIDTH; x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid transparent pixel in the bottom part of the Left Trapezoid frame.");
                // return pDst;
            }
            *pDst = pixel.getPaletteIndex();
            ++pDst;
        }
    }
    // add opaque pixels
    for (i = MICRO_HEIGHT / 2; i != 0; i--, y--) {
        for (x = 0; x < MICRO_WIDTH; x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid transparent pixel in the top part of the Left Trapezoid frame.");
                // return pDst;
            }
            *pDst = pixel.getPaletteIndex();
            ++pDst;
        }
    }
    return pDst;
}

quint8 *D1CelTilesetFrame::WriteRightTrapezoid(const D1GfxFrame &frame, quint8 *pDst)
{
    int i, x, y;
    // int length = (MICRO_WIDTH * MICRO_HEIGHT) / 2 + MICRO_HEIGHT * (2 + MICRO_HEIGHT) / 4 + MICRO_HEIGHT / 2;

    // memset(pDst, 0, length); -- unnecessary for the game, and the current user did this anyway
    y = MICRO_HEIGHT - 1;
    for (i = MICRO_HEIGHT - 2; i >= 0; i -= 2, y--) {
        // add opaque pixels
        for (x = 0; x < (MICRO_WIDTH - i); x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid transparent pixel in the bottom part of the Right Trapezoid frame.");
                // return pDst;
            }
            *pDst = pixel.getPaletteIndex();
            ++pDst;
        }
        pDst += i & 2;
        // check transparent pixels
        for (x = MICRO_WIDTH - i; x < MICRO_WIDTH; x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (!pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid non-transparent pixel in the bottom part of the Right Trapezoid frame.");
                // return pDst;
            }
        }
    }
    // add opaque pixels
    for (i = MICRO_HEIGHT / 2; i != 0; i--, y--) {
        for (x = 0; x < MICRO_WIDTH; x++) {
            D1GfxPixel pixel = frame.getPixel(x, y);
            if (pixel.isTransparent()) {
                dProgressErr() << QApplication::tr("Invalid transparent pixel in the top part of the Right Trapezoid frame.");
                // return pDst;
            }
            *pDst = pixel.getPaletteIndex();
            ++pDst;
        }
    }
    return pDst;
}
