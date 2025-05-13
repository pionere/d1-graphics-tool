#include "d1celtilesetframe.h"

#include <QApplication>
#include <QMessageBox>

#include "d1gfx.h"
#include "progressdialog.h"

bool D1CelTilesetFrame::load(D1GfxFrame &frame, D1CEL_FRAME_TYPE type, QByteArray rawData, bool *patched)
{
    frame.width = MICRO_WIDTH;
    frame.height = MICRO_HEIGHT;
    frame.frameType = type;
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
        valid = D1CelTilesetFrame::LoadLeftTriangle(frame, rawData, patched);
        break;
    case D1CEL_FRAME_TYPE::RightTriangle:
        valid = D1CelTilesetFrame::LoadRightTriangle(frame, rawData, patched);
        break;
    case D1CEL_FRAME_TYPE::LeftTrapezoid:
        valid = D1CelTilesetFrame::LoadLeftTrapezoid(frame, rawData, patched);
        break;
    case D1CEL_FRAME_TYPE::RightTrapezoid:
        valid = D1CelTilesetFrame::LoadRightTrapezoid(frame, rawData, patched);
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
    if (rawData.size() != MICRO_WIDTH * MICRO_HEIGHT)
        return false;
    for (int i = MICRO_HEIGHT /* frame.height */ - 1; i >= 0; i--) {
        std::vector<D1GfxPixel> &pixelLine = frame.pixels[i];
        //if (rawData.size() < offset + MICRO_WIDTH /* frame.width */)
        //    return false;
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
        int j = 0;
        for ( ; j < MICRO_WIDTH /* frame.width */;) {
            if (rawData.size() <= offset)
                return false;
            qint8 readByte = rawData[offset++];
            if (readByte < 0) {
                readByte = -readByte;
                // transparent pixels
                D1GfxPixel pixel = D1GfxPixel::transparentPixel();
                for (int k = 0; k < readByte; k++) {
                    pixelLine.push_back(pixel);
                }
            } else {
                if (rawData.size() < offset + readByte)
                    return false;
                // color pixels
                for (int k = 0; k < readByte; k++) {
                    pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
                }
            }
            j += readByte;
        }
        if (j != MICRO_WIDTH)
            return false;
    }
    return true;
}

static bool LoadBottomLeftTriangle(D1GfxFrame &frame, const QByteArray &rawData, bool patched)
{
    int offset = 0;
    for (int i = 1; i <= MICRO_HEIGHT /* frame.height */ / 2; i++) {
        if (!patched) {
            offset += 2 * (i % 2);
        }
        std::vector<D1GfxPixel> &pixelLine = frame.getPixels()[MICRO_HEIGHT /* frame.height */ - i];
        for (int j = 0; j < MICRO_WIDTH /* frame.width */ - 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::transparentPixel());
        }
        //if (rawData.size() < offset + 2 * i)
        //    return false;
        for (int j = 0; j < 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
        }
    }
    return true;
}

static bool LoadBottomRightTriangle(D1GfxFrame &frame, const QByteArray &rawData, bool patched)
{
    int offset = 0;
    for (int i = 1; i <= MICRO_HEIGHT /* frame.height */ / 2; i++) {
        std::vector<D1GfxPixel> &pixelLine = frame.getPixels()[MICRO_HEIGHT /* frame.height */ - i];
        //if (rawData.size() < offset + 2 * i)
        //    return false;
        for (int j = 0; j < 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
        }
        for (int j = 0; j < MICRO_WIDTH /* frame.width */ - 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::transparentPixel());
        }
        if (!patched) {
            offset += 2 * (i % 2);
        }
    }
    return true;
}

static bool LoadTopLeftTriangle(D1GfxFrame &frame, const QByteArray &rawData, bool patched)
{
    int offset = 288;
    for (int i = 1; i <= MICRO_HEIGHT /* frame.height */ / 2; i++) {
        if (!patched) {
            offset += 2 * (i % 2);
        }
        std::vector<D1GfxPixel> &pixelLine = frame.getPixels()[MICRO_HEIGHT /* frame.height */ / 2 - i];
        for (int j = 0; j < 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::transparentPixel());
        }
        //if (rawData.size() < offset + MICRO_WIDTH /* frame.width */ - 2 * i)
        //    return false;
        for (int j = 0; j < MICRO_WIDTH /* frame.width */ - 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
        }
    }
    return true;
}

static bool LoadTopRightTriangle(D1GfxFrame &frame, const QByteArray &rawData, bool patched)
{
    int offset = 288;
    for (int i = 1; i <= MICRO_HEIGHT /* frame.height */ / 2; i++) {
        std::vector<D1GfxPixel> &pixelLine = frame.getPixels()[MICRO_HEIGHT /* frame.height */ / 2 - i];
        //if (rawData.size() < offset + MICRO_WIDTH /* frame.width */ - 2 * i)
        //    return false;
        for (int j = 0; j < MICRO_WIDTH /* frame.width */ - 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
        }
        for (int j = 0; j < 2 * i; j++) {
            pixelLine.push_back(D1GfxPixel::transparentPixel());
        }
        if (!patched) {
            offset += 2 * (i % 2);
        }
    }
    return true;
}

bool D1CelTilesetFrame::LoadLeftTriangle(D1GfxFrame &frame, const QByteArray &rawData, bool *set_patched)
{
    bool patched;
    if (rawData.size() == MICRO_HEIGHT * MICRO_WIDTH / 2 + 32 /* 544 */) {
        patched = false;
        if (*set_patched) {
            dProgressWarn() << QApplication::tr("Unpatched left triangle in a patched tileset.");
        }
    } else if (rawData.size() == MICRO_HEIGHT * MICRO_WIDTH / 2 /* 512 */) {
        patched = true;
        *set_patched = true;
    } else {
        return false;
    }
    return LoadBottomLeftTriangle(frame, rawData, patched) && LoadTopLeftTriangle(frame, rawData, patched);
}

bool D1CelTilesetFrame::LoadRightTriangle(D1GfxFrame &frame, const QByteArray &rawData, bool *set_patched)
{
    bool patched;
    if (rawData.size() == MICRO_HEIGHT * MICRO_WIDTH / 2 + 32 /* 544 */) {
        patched = false;
        if (*set_patched) {
            dProgressWarn() << QApplication::tr("Unpatched right triangle in a patched tileset.");
        }
    } else if (rawData.size() == MICRO_HEIGHT * MICRO_WIDTH / 2 /* 512 */) {
        patched = true;
        *set_patched = true;
    } else {
        return false;
    }
    return LoadBottomRightTriangle(frame, rawData, patched) && LoadTopRightTriangle(frame, rawData, patched);
}

static bool LoadTopHalfSquare(D1GfxFrame &frame, const QByteArray &rawData)
{
    int offset = 288;
    for (int i = 1; i <= MICRO_HEIGHT /* frame.height */ / 2; i++) {
        std::vector<D1GfxPixel> &pixelLine = frame.getPixels()[MICRO_HEIGHT /* frame.height */ / 2 - i];
        //if (rawData.size() < offset + MICRO_WIDTH /* frame.width */)
        //    return false;
        for (int j = 0; j < MICRO_WIDTH /* frame.width */; j++) {
            pixelLine.push_back(D1GfxPixel::colorPixel(rawData[offset++]));
        }
    }
    return true;
}

bool D1CelTilesetFrame::LoadLeftTrapezoid(D1GfxFrame &frame, const QByteArray &rawData, bool *set_patched)
{
    bool patched;
    if (rawData.size() == MICRO_HEIGHT * MICRO_WIDTH / 2 + MICRO_HEIGHT * MICRO_WIDTH / 4 + 32 /* 800 */) {
        patched = false;
        if (*set_patched) {
            dProgressWarn() << QApplication::tr("Unpatched left trapezoid in a patched tileset.");
        }
    } else if (rawData.size() == MICRO_HEIGHT * MICRO_WIDTH / 2 + MICRO_HEIGHT * MICRO_WIDTH / 4 /* 768 */) {
        patched = true;
        *set_patched = true;
    } else {
        return false;
    }
    return LoadBottomLeftTriangle(frame, rawData, patched) && LoadTopHalfSquare(frame, rawData);
}

bool D1CelTilesetFrame::LoadRightTrapezoid(D1GfxFrame &frame, const QByteArray &rawData, bool *set_patched)
{
    bool patched;
    if (rawData.size() == MICRO_HEIGHT * MICRO_WIDTH / 2 + MICRO_HEIGHT * MICRO_WIDTH / 4 + 32 /* 800 */) {
        patched = false;
        if (*set_patched) {
            dProgressWarn() << QApplication::tr("Unpatched right trapezoid in a patched tileset.");
        }
    } else if (rawData.size() == MICRO_HEIGHT * MICRO_WIDTH / 2 + MICRO_HEIGHT * MICRO_WIDTH / 4 /* 768 */) {
        patched = true;
        *set_patched = true;
    } else {
        return false;
    }
    return LoadBottomRightTriangle(frame, rawData, patched) && LoadTopHalfSquare(frame, rawData);
}

quint8 *D1CelTilesetFrame::writeFrameData(D1GfxFrame &frame, quint8 *pDst, bool patched)
{
    switch (frame.frameType) {
    case D1CEL_FRAME_TYPE::LeftTriangle:
        pDst = D1CelTilesetFrame::WriteLeftTriangle(frame, pDst, patched);
        break;
    case D1CEL_FRAME_TYPE::RightTriangle:
        pDst = D1CelTilesetFrame::WriteRightTriangle(frame, pDst, patched);
        break;
    case D1CEL_FRAME_TYPE::LeftTrapezoid:
        pDst = D1CelTilesetFrame::WriteLeftTrapezoid(frame, pDst, patched);
        break;
    case D1CEL_FRAME_TYPE::RightTrapezoid:
        pDst = D1CelTilesetFrame::WriteRightTrapezoid(frame, pDst, patched);
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
    // preserve 4-byte alignment
    pHead = pStart + (((size_t)pHead - (size_t)pStart + 3) & ~3);
    return pHead;
}

quint8 *D1CelTilesetFrame::WriteLeftTriangle(const D1GfxFrame &frame, quint8 *pDst, bool patched)
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
        if (!patched) {
            pDst += i & 2;
        }
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
        if (!patched) {
            pDst += i & 2;
        }
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

quint8 *D1CelTilesetFrame::WriteRightTriangle(const D1GfxFrame &frame, quint8 *pDst, bool patched)
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
        if (!patched) {
            pDst += i & 2;
        }
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
        if (!patched) {
            pDst += i & 2;
        }
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

quint8 *D1CelTilesetFrame::WriteLeftTrapezoid(const D1GfxFrame &frame, quint8 *pDst, bool patched)
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
        if (!patched) {
            pDst += i & 2;
        }
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

quint8 *D1CelTilesetFrame::WriteRightTrapezoid(const D1GfxFrame &frame, quint8 *pDst, bool patched)
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
        if (!patched) {
            pDst += i & 2;
        }
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

bool D1CelTilesetFrame::altFrame(const D1GfxFrame *frame1, const D1GfxFrame *frame2, int *limit)
{
    if (frame1->getWidth() != frame2->getWidth() || frame1->getHeight() != frame2->getHeight())
        return false;
    for (int x = 0; x < frame1->getWidth(); x++) {
        for (int y = 0; y < frame1->getHeight(); y++) {
            if (frame1->getPixel(x, y) != frame2->getPixel(x, y) && --*limit < 0) {
                return false;
            }
        }
    }
    return true;
}

D1CEL_FRAME_TYPE D1CelTilesetFrame::altFrameType(const D1GfxFrame *frame, int *limit)
{
    D1CEL_FRAME_TYPE frameType = D1CEL_FRAME_TYPE::TransparentSquare;
    QString tmp;
    int limitSquare = *limit, limitLeftTriangle = *limit, limitRightTriangle = *limit, limitLeftTrapezoid = *limit, limitRightTrapezoid = *limit, limitEmpty = *limit;

    if (frame->getWidth() == MICRO_WIDTH && frame->getHeight() == MICRO_HEIGHT) {
        if (D1CelTilesetFrame::validSquare(frame, tmp, &limitSquare)) {
            frameType = D1CEL_FRAME_TYPE::Square;
            *limit = limitSquare;
        } else if (D1CelTilesetFrame::validLeftTriangle(frame, tmp, &limitLeftTriangle)) {
            frameType = D1CEL_FRAME_TYPE::LeftTriangle;
            *limit = limitLeftTriangle;
        } else if (D1CelTilesetFrame::validRightTriangle(frame, tmp, &limitRightTriangle)) {
            frameType = D1CEL_FRAME_TYPE::RightTriangle;
            *limit = limitRightTriangle;
        } else if (D1CelTilesetFrame::validLeftTrapezoid(frame, tmp, &limitLeftTrapezoid)) {
            frameType = D1CEL_FRAME_TYPE::LeftTrapezoid;
            *limit = limitLeftTrapezoid;
        } else if (D1CelTilesetFrame::validRightTrapezoid(frame, tmp, &limitRightTrapezoid)) {
            frameType = D1CEL_FRAME_TYPE::RightTrapezoid;
            *limit = limitRightTrapezoid;
        } else if (limitEmpty > 0 && D1CelTilesetFrame::validEmpty(frame, tmp, &limitEmpty)) {
            frameType = D1CEL_FRAME_TYPE::Empty;
            *limit = limitEmpty;
        }
    }
    return frameType;
}

void D1CelTilesetFrame::selectFrameType(D1GfxFrame *frame)
{
    int limit = 0;
    D1CEL_FRAME_TYPE frameType = D1CelTilesetFrame::altFrameType(frame, &limit);
    frame->setFrameType(frameType);
}

void D1CelTilesetFrame::validate(const D1GfxFrame *frame, QString &error, QString &warning)
{
    QString tmp;

    if (frame->getWidth() != MICRO_WIDTH) {
        static_assert(MICRO_WIDTH == 32, "D1CelTilesetFrame::validate uses hardcoded width.");
        error = QApplication::tr("Frame width is not 32px.");
    } else if (frame->getHeight() != MICRO_HEIGHT) {
        static_assert(MICRO_HEIGHT == 32, "D1CelTilesetFrame::validate uses hardcoded height.");
        error = QApplication::tr("Frame height is not 32px.");
    } else {
        int limit = 0;
        switch (frame->getFrameType()) {
        case D1CEL_FRAME_TYPE::Square:
            D1CelTilesetFrame::validSquare(frame, error, &limit);
            break;
        case D1CEL_FRAME_TYPE::TransparentSquare:
            if (D1CelTilesetFrame::validSquare(frame, tmp, &limit)) {
                warning = QApplication::tr("Suggested type: 'Square'");
                break;
            }
            limit = 0;
            if (D1CelTilesetFrame::validLeftTriangle(frame, tmp, &limit)) {
                warning = QApplication::tr("Suggested type: 'Left Triangle'");
                break;
            }
            limit = 0;
            if (D1CelTilesetFrame::validRightTriangle(frame, tmp, &limit)) {
                warning = QApplication::tr("Suggested type: 'Right Triangle'");
                break;
            }
            limit = 0;
            if (D1CelTilesetFrame::validLeftTrapezoid(frame, tmp, &limit)) {
                warning = QApplication::tr("Suggested type: 'Left Trapezoid'");
                break;
            }
            limit = 0;
            if (D1CelTilesetFrame::validRightTrapezoid(frame, tmp, &limit)) {
                warning = QApplication::tr("Suggested type: 'Right Trapezoid'");
                break;
            }
            break;
        case D1CEL_FRAME_TYPE::LeftTriangle:
            D1CelTilesetFrame::validLeftTriangle(frame, error, &limit);
            break;
        case D1CEL_FRAME_TYPE::RightTriangle:
            D1CelTilesetFrame::validRightTriangle(frame, error, &limit);
            break;
        case D1CEL_FRAME_TYPE::LeftTrapezoid:
            D1CelTilesetFrame::validLeftTrapezoid(frame, error, &limit);
            break;
        case D1CEL_FRAME_TYPE::RightTrapezoid:
            D1CelTilesetFrame::validRightTrapezoid(frame, error, &limit);
            break;
        }
    }
}

static bool prepareMsgTransparent(QString &msg, int x, int y)
{
    msg = QApplication::tr("Invalid (transparent) pixel at (%1:%2)").arg(x).arg(y);
    return false;
}

static bool prepareMsgNonTransparent(QString &msg, int x, int y)
{
    msg = QApplication::tr("Invalid (non-transparent) pixel at (%1:%2)").arg(x).arg(y);
    return false;
}

bool D1CelTilesetFrame::validSquare(const D1GfxFrame *frame, QString &msg, int *limit)
{
    for (int y = 0; y < MICRO_HEIGHT; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            if (frame->getPixel(x, y).isTransparent() && --*limit < 0) {
                return prepareMsgTransparent(msg, x, y);
            }
        }
    }
    return true;
}

static bool validBottomLeftTriangle(const D1GfxFrame *frame, QString &msg, int *limit)
{
    for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            if (frame->getPixel(x, y).isTransparent()) {
                if (x >= (y * 2 - MICRO_WIDTH) && --*limit < 0) {
                    return prepareMsgTransparent(msg, x, y);
                }
            } else {
                if (x < (y * 2 - MICRO_WIDTH) && --*limit < 0) {
                    return prepareMsgNonTransparent(msg, x, y);
                }
            }
        }
    }
    return true;
}

static bool validBottomRightTriangle(const D1GfxFrame *frame, QString &msg, int *limit)
{
    for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            if (frame->getPixel(x, y).isTransparent()) {
                if (x < (2 * MICRO_WIDTH - y * 2) && --*limit < 0) {
                    return prepareMsgTransparent(msg, x, y);
                }
            } else {
                if (x >= (2 * MICRO_WIDTH - y * 2) && --*limit < 0) {
                    return prepareMsgNonTransparent(msg, x, y);
                }
            }
        }
    }
    return true;
}

static bool validTopLeftTriangle(const D1GfxFrame *frame, QString &msg, int *limit)
{
    for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            if (frame->getPixel(x, y).isTransparent()) {
                if (x >= (MICRO_WIDTH - y * 2) && --*limit < 0) {
                    return prepareMsgTransparent(msg, x, y);
                }
            } else {
                if (x < (MICRO_WIDTH - y * 2) && --*limit < 0) {
                    return prepareMsgNonTransparent(msg, x, y);
                }
            }
        }
    }
    return true;
}

static bool validTopRightTriangle(const D1GfxFrame *frame, QString &msg, int *limit)
{
    for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            if (frame->getPixel(x, y).isTransparent()) {
                if (x < y * 2 && --*limit < 0) {
                    return prepareMsgTransparent(msg, x, y);
                }
            } else {
                if (x >= y * 2 && --*limit < 0) {
                    return prepareMsgNonTransparent(msg, x, y);
                }
            }
        }
    }
    return true;
}

bool D1CelTilesetFrame::validLeftTriangle(const D1GfxFrame *frame, QString &msg, int *limit)
{
    return validBottomLeftTriangle(frame, msg, limit) && validTopLeftTriangle(frame, msg, limit);
}

bool D1CelTilesetFrame::validRightTriangle(const D1GfxFrame *frame, QString &msg, int *limit)
{
    return validBottomRightTriangle(frame, msg, limit) && validTopRightTriangle(frame, msg, limit);
}

static bool validTopHalfSquare(const D1GfxFrame *frame, QString &msg, int *limit)
{
    for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            if (frame->getPixel(x, y).isTransparent() && --*limit < 0) {
                return prepareMsgTransparent(msg, x, y);
            }
        }
    }
    return true;
}

bool D1CelTilesetFrame::validLeftTrapezoid(const D1GfxFrame *frame, QString &msg, int *limit)
{
    return validBottomLeftTriangle(frame, msg, limit) && validTopHalfSquare(frame, msg, limit);
}

bool D1CelTilesetFrame::validRightTrapezoid(const D1GfxFrame *frame, QString &msg, int *limit)
{
    return validBottomRightTriangle(frame, msg, limit) && validTopHalfSquare(frame, msg, limit);
}

bool D1CelTilesetFrame::validEmpty(const D1GfxFrame *frame, QString &msg, int *limit)
{
    for (int y = 0; y < MICRO_HEIGHT; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            if (!frame->getPixel(x, y).isTransparent() && --*limit < 0) {
                return prepareMsgNonTransparent(msg, x, y);
            }
        }
    }
    return true;
}

void D1CelTilesetFrame::collectPixels(const D1GfxFrame *frame, D1CEL_FRAME_TYPE mask, std::vector<FramePixel> &pixels)
{
    switch (mask) {
    case D1CEL_FRAME_TYPE::Square:
        D1CelTilesetFrame::collectSquare(frame, pixels);
        break;
    case D1CEL_FRAME_TYPE::TransparentSquare:
        break;
    case D1CEL_FRAME_TYPE::LeftTriangle:
        D1CelTilesetFrame::collectLeftTriangle(frame, pixels);
        break;
    case D1CEL_FRAME_TYPE::RightTriangle:
        D1CelTilesetFrame::collectRightTriangle(frame, pixels);
        break;
    case D1CEL_FRAME_TYPE::LeftTrapezoid:
        D1CelTilesetFrame::collectLeftTrapezoid(frame, pixels);
        break;
    case D1CEL_FRAME_TYPE::RightTrapezoid:
        D1CelTilesetFrame::collectRightTrapezoid(frame, pixels);
        break;
    }
}

void D1CelTilesetFrame::collectSquare(const D1GfxFrame *frame, std::vector<FramePixel> &pixels)
{
    for (int y = 0; y < MICRO_HEIGHT; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            D1GfxPixel px = frame->getPixel(x, y);
            if (px.isTransparent()) {
                pixels.push_back(FramePixel(QPoint(x, y), px));
            }
        }
    }
}

static void collectBottomLeftTriangle(const D1GfxFrame *frame, std::vector<FramePixel> &pixels)
{
    for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            D1GfxPixel px = frame->getPixel(x, y);
            if (px.isTransparent()) {
                if (x >= (y * 2 - MICRO_WIDTH)) {
                    pixels.push_back(FramePixel(QPoint(x, y), px));
                }
            } else {
                if (x < (y * 2 - MICRO_WIDTH)) {
                    pixels.push_back(FramePixel(QPoint(x, y), px));
                }
            }
        }
    }
}

static void collectBottomRightTriangle(const D1GfxFrame *frame, std::vector<FramePixel> &pixels)
{
    for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            D1GfxPixel px = frame->getPixel(x, y);
            if (px.isTransparent()) {
                if (x < (2 * MICRO_WIDTH - y * 2)) {
                    pixels.push_back(FramePixel(QPoint(x, y), px));
                }
            } else {
                if (x >= (2 * MICRO_WIDTH - y * 2)) {
                    pixels.push_back(FramePixel(QPoint(x, y), px));
                }
            }
        }
    }
}

static void collectTopLeftTriangle(const D1GfxFrame *frame, std::vector<FramePixel> &pixels)
{
    for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            D1GfxPixel px = frame->getPixel(x, y);
            if (px.isTransparent()) {
                if (x >= (MICRO_WIDTH - y * 2)) {
                    pixels.push_back(FramePixel(QPoint(x, y), px));
                }
            } else {
                if (x < (MICRO_WIDTH - y * 2)) {
                    pixels.push_back(FramePixel(QPoint(x, y), px));
                }
            }
        }
    }
}

static void collectTopRightTriangle(const D1GfxFrame *frame, std::vector<FramePixel> &pixels)
{
    for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            D1GfxPixel px = frame->getPixel(x, y);
            if (px.isTransparent()) {
                if (x < y * 2) {
                    pixels.push_back(FramePixel(QPoint(x, y), px));
                }
            } else {
                if (x >= y * 2) {
                    pixels.push_back(FramePixel(QPoint(x, y), px));
                }
            }
        }
    }
}

void D1CelTilesetFrame::collectLeftTriangle(const D1GfxFrame *frame, std::vector<FramePixel> &pixels)
{
    collectBottomLeftTriangle(frame, pixels);
    collectTopLeftTriangle(frame, pixels);
}

void D1CelTilesetFrame::collectRightTriangle(const D1GfxFrame *frame, std::vector<FramePixel> &pixels)
{
    collectBottomRightTriangle(frame, pixels);
    collectTopRightTriangle(frame, pixels);
}

static void collectTopHalfSquare(const D1GfxFrame *frame, std::vector<FramePixel> &pixels)
{
    for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            D1GfxPixel px = frame->getPixel(x, y);
            if (px.isTransparent()) {
                pixels.push_back(FramePixel(QPoint(x, y), px));
            }
        }
    }
}

void D1CelTilesetFrame::collectLeftTrapezoid(const D1GfxFrame *frame, std::vector<FramePixel> &pixels)
{
    collectBottomLeftTriangle(frame, pixels);
    collectTopHalfSquare(frame, pixels);
}

void D1CelTilesetFrame::collectRightTrapezoid(const D1GfxFrame *frame, std::vector<FramePixel> &pixels)
{
    collectBottomRightTriangle(frame, pixels);
    collectTopHalfSquare(frame, pixels);
}
