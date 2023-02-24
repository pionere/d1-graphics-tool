#pragma once

#include <vector>

#include <QByteArray>

#include "d1gfx.h"
#include "openasdialog.h"

#define MICRO_WIDTH 32
#define MICRO_HEIGHT 32

class D1CelTilesetFrame {
public:
    static bool load(D1GfxFrame &frame, D1CEL_FRAME_TYPE frameType, const QByteArray rawData, const OpenAsParam &params);

    static quint8 *writeFrameData(D1GfxFrame &frame, quint8 *pBuf);

    static void collectPixels(const D1GfxFrame *frame, D1CEL_FRAME_TYPE mask, std::vector<FramePixel> &pixels);
    static D1CEL_FRAME_TYPE altFrameType(const D1GfxFrame *frame, int *limit);
    static void selectFrameType(D1GfxFrame *frame);
    static void validate(const D1GfxFrame *frame, QString &error, QString &warning);

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

    static bool validSquare(const D1GfxFrame *frame, QString &msg, int *limit);
    static bool validLeftTriangle(const D1GfxFrame *frame, QString &msg, int *limit);
    static bool validRightTriangle(const D1GfxFrame *frame, QString &msg, int *limit);
    static bool validLeftTrapezoid(const D1GfxFrame *frame, QString &msg, int *limit);
    static bool validRightTrapezoid(const D1GfxFrame *frame, QString &msg, int *limit);
    static bool validEmpty(const D1GfxFrame *frame, QString &msg, int *limit);

    static void collectSquare(const D1GfxFrame *frame, std::vector<FramePixel> &pixels);
    static void collectLeftTriangle(const D1GfxFrame *frame, std::vector<FramePixel> &pixels);
    static void collectRightTriangle(const D1GfxFrame *frame, std::vector<FramePixel> &pixels);
    static void collectLeftTrapezoid(const D1GfxFrame *frame, std::vector<FramePixel> &pixels);
    static void collectRightTrapezoid(const D1GfxFrame *frame, std::vector<FramePixel> &pixels);
};
