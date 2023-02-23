#pragma once

#include <QByteArray>

#include "openasdialog.h"

#define MICRO_WIDTH 32
#define MICRO_HEIGHT 32

class D1GfxFrame;

enum class D1CEL_FRAME_TYPE {
    Square,            // opaque square (bitmap)
    TransparentSquare, // bitmap with transparent pixels
    LeftTriangle,      // opaque triangle on its left edge
    RightTriangle,     // opaque triangle on its right edge
    LeftTrapezoid,     // bottom half is a left triangle, upper half is a square
    RightTrapezoid,    // bottom half is a right triangle, upper half is a square
    Empty = -1,        // transparent frame (only for efficiency tests)
};

class D1CelTilesetFrame {
public:
    static bool load(D1GfxFrame &frame, D1CEL_FRAME_TYPE frameType, const QByteArray rawData, const OpenAsParam &params);

    static quint8 *writeFrameData(D1GfxFrame &frame, quint8 *pBuf);

private:
    static bool LoadSquare(D1GfxFrame &frame, const QByteArray &rawData);
    static bool LoadTransparentSquare(D1GfxFrame &frame, const QByteArray &rawData);
    static bool LoadLeftTriangle(D1GfxFrame &frame, const QByteArray &rawData);
    static bool LoadRightTriangle(D1GfxFrame &frame, const QByteArray &rawData);
    static bool LoadLeftTrapezoid(D1GfxFrame &frame, const QByteArray &rawData);
    static bool LoadRightTrapezoid(D1GfxFrame &frame, const QByteArray &rawData);

    static quint8 *WriteSquare(const D1GfxFrame &frame, quint8 *pBuf);
    static quint8 *WriteTransparentSquare(const D1GfxFrame &frame, quint8 *pBuf);
    static quint8 *WriteLeftTriangle(const D1GfxFrame &frame, quint8 *pBuf);
    static quint8 *WriteRightTriangle(const D1GfxFrame &frame, quint8 *pBuf);
    static quint8 *WriteLeftTrapezoid(const D1GfxFrame &frame, quint8 *pBuf);
    static quint8 *WriteRightTrapezoid(const D1GfxFrame &frame, quint8 *pBuf);
};
