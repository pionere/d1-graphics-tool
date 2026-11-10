#include "d2ccframe.h"

#include <QDataStream>

#include "progressdialog.h"

#include "dungeon/all.h"

typedef struct D2CcHeaderInfo
{
    // reader state
    quint64 remVal;
    int remBits;
    int cursor;

    // read values



    // - direction-info
    int OutSizeCoded;
    unsigned CompressionFlags;
    unsigned Variable0Bits;
    unsigned WidthBits;
    unsigned HeightBits;
    unsigned XOffsetBits;
    unsigned YOffsetBits;
    unsigned OptionalDataBits;
    unsigned CodedBytesBits;

} D2CcHeaderInfo;

static int crazyLookup(unsigned idx)
{
    const int crazyBitTable[] = { 0, 1, 2, 4, 6, 8, 10, 12, 14, 16, 20, 24, 26, 28, 30, 32 };

    if (idx >= lengthof(crazyBitTable)) {
        dProgressErr() << QString("Invalid index (%1) to the lookup table (must be less than %2).").arg(idx).arg(lengthof(crazyBitTable));
        idx = 0;
    }
    return crazyBitTable[idx];
}

static quint32 readBits(const QByteArray &rawData, int numBits, D2CcHeaderInfo& hi)  // quint32 &remVal, int &remBits, int &cursor)
{
    while (numBits > hi.remBits) {
        quint8 val = rawData[hi.cursor];
        hi.cursor++;
        hi.remVal |= val << hi.remBits;
        hi.remBits += 8;
    }

    quint32 res = hi.remVal & ((1ll << numBits) - 1);
    hi.remVal >>= numBits;
    hi.remBits -= numBits;
    return res;
}

bool D2CcFrame::load(D1GfxFrame &frame, int idx, const QByteArray &rawData, const OpenAsParam &params)
{
    const int directionHeaderBitSize = 32 + 2 + 4 + 4 + 4 + 4 + 4 + 4 + 4;
    if (rawData.size() < (directionHeaderBitSize + 8 - 1) / 8)
        return false;

    D2CcHeaderInfo hi = { };

    hi.OutSizeCoded = readBits(rawData, 32, hi);
    hi.CompressionFlags = readBits(rawData, 2, hi);
    hi.Variable0Bits = crazyLookup(readBits(rawData, 4, hi));
    hi.WidthBits = crazyLookup(readBits(rawData, 4, hi));
    hi.HeightBits = crazyLookup(readBits(rawData, 4, hi));
    hi.XOffsetBits = crazyLookup(readBits(rawData, 4, hi));
    hi.YOffsetBits = crazyLookup(readBits(rawData, 4, hi));
    hi.OptionalDataBits = crazyLookup(readBits(rawData, 4, hi));
    hi.CodedBytesBits = crazyLookup(readBits(rawData, 4, hi));

    const int frameHeaderBitSize = hi.Variable0Bits + hi.WidthBits + hi.HeightBits + hi.XOffsetBits + hi.YOffsetBits + hi.OptionalDataBits + hi.Bits + 1;
    if (rawData.size() < (directionHeaderBitSize + idx * frameHeaderBitSize + 8 - 1) / 8)
        return false;

    int Variable0, OptionalData, CodedBytes;
    int width, height, XOffset, YOffset;
    bool flipped;
    for (int i = 0; i <= idx; i++) {
        Variable0 = readBits(rawData, hi.Variable0Bits, hi);
        width = readBits(rawData, hi.WidthBits, hi);
        height = readBits(rawData, hi.HeightBits, hi);

        XOffset = readBits(rawData, hi.XOffsetBits, hi);
        YOffset = readBits(rawData, hi.YOffsetBits, hi);
        OptionalData = readBits(rawData, hi.OptionalDataBits, hi);
        CodedBytes = readBits(rawData, hi.CodedBytesBits, hi);

        flipped = readBits(rawData, 1, hi);
    }

    // min := image.Point{X: f.XOffset, Y: f.YOffset - f.Height + 1}
    // max := image.Point{X: f.XOffset + f.Width, Y: f.YOffset + 1}
    // f.Box = image.Rectangle{Min: min, Max: max}


    dProgressWarn() << QApplication::tr("frame dimensions %1x%2 with offset %3x%4 (flipped %5).").arg(width).arg(height).arg(XOffset).arg(YOffset).arg(flipped);

    // frame.width = width;
    // frame.height = height;
    return true;
}
