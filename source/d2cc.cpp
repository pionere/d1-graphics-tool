#include "d2cc.h"

#include <vector>

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>

#include "d2ccframe.h"
#include "progressdialog.h"

#include "dungeon/all.h"

#define D2PAL_COLORS 256

static const int cellSize = 4;

bool D2Cc::readHeader(QIODevice *device, QDataStream &in, D2CcHeaderInfo &hi, D1Gfx &gfx)
{
    auto fileSize = device->size();
    const int headerSize = 3 * 1 + 3 * 4;
    // CEL HEADER CHECKS
    // Read first DWORD
    if (fileSize < headerSize) {
        return false;
    }

    quint8 signatureByte;
    in >> signatureByte;

    if (signatureByte != 0x74) {
        return false;
    }

    quint8 versionByte;
    in >> versionByte;

    quint8 directionsByte;
    in >> directionsByte;

    quint32 framesPerDirectionDword;
    in >> framesPerDirectionDword;

    quint32 checkDword;
    in >> checkDword;

    if (checkDword != 1) {
        return false;
    }

    quint32 sizeDword;
    in >> sizeDword;

    // Trying to find file size in CEL header
    // if (fileSize < sizeDword)
    //    dProgressWarn() << QApplication::tr("Invalid filesize %1 (expected %2).").arg(sizeDword).arg(fileSize);
    //    return false;

    // CEL FRAMES OFFSETS CALCULATION
    if (directionsByte * framesPerDirectionDword != 0) {
        if (fileSize < headerSize + directionsByte * 4)
            return false;

        // Going through all groups
        quint32 celGroupStartOffset;
        in >> celGroupStartOffset;
        for (unsigned i = 0; i < directionsByte; i++) {
            quint32 celFrameCount = framesPerDirectionDword;

            gfx.groupFrameIndices.push_back(std::pair<int, int>(i * framesPerDirectionDword, (i + 1) * framesPerDirectionDword - 1));

            quint32 celGroupEndOffset;
            if (i != directionsByte - 1) {
                in >> celGroupEndOffset;
                if (fileSize < celGroupEndOffset) {
                    dProgressErr() << QApplication::tr("Invalid groupoffset %1 of %2 for group %3.").arg(celGroupEndOffset).arg(fileSize).arg(i + 1);
                    break;
                }
            } else {
                celGroupEndOffset = fileSize;
            }

            // add the address of the directionData
            hi.dirOffsets.push_back(
                QPair<quint32, quint32>(celGroupStartOffset, celGroupEndOffset));

            celGroupStartOffset = celGroupEndOffset;
        }
    }

    return true;
}

static int crazyLookup(unsigned idx)
{
    const int crazyBitTable[] = { 0, 1, 2, 4, 6, 8, 10, 12, 14, 16, 20, 24, 26, 28, 30, 32 };

    if (idx >= (unsigned)lengthof(crazyBitTable)) {
        dProgressErr() << QString("Invalid index (%1) to the lookup table (must be less than %2).").arg(idx).arg(lengthof(crazyBitTable));
        idx = 0;
    }
    return crazyBitTable[idx];
}

typedef struct D2CcBitReaderState
{
    // reader state
    quint64 remVal;
    int remBits;
    int cursor;
} D2CcBitReaderState;

static quint32 readBits(const QByteArray &rawData, int numBits, D2CcBitReaderState& hi)  // quint32 &remVal, int &remBits, int &cursor)
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

typedef struct D2CcFrameInfo
{
    int width;
    int height;
    int XOffset;
    int YOffset;
    bool flipped;
    // RECT32 box;
} D2CcFrameInfo;

typedef struct D2CcDirectionInfo
{
    int OutSizeCoded;
    unsigned CompressionFlags;
    unsigned Variable0Bits;
    unsigned WidthBits;
    unsigned HeightBits;
    unsigned XOffsetBits;
    unsigned YOffsetBits;
    unsigned OptionalDataBits;
    unsigned CodedBytesBits;

    unsigned EqualCellsBitstreamSize;
    unsigned PixelMaskBitstreamSize;
    unsigned EncodingTypeBitstreamSize;
    unsigned RawPixelCodesBitstreamSize;

    quint8 PaletteEntries[256];
    /*
    Box                        *image.Rectangle
    Cells                      []*Cell
    PixelData                  []byte
    PixelBuffer                PixelBuffer
    */
    unsigned HorizontalCellCount;
    unsigned VerticalCellCount;

    QList<D2CcFrameInfo> frameInfos;
    RECT32 box;
} D2CcDirectionInfo;

static void readFrameHeader(const QByteArray &rawData, const D2CcDirectionInfo &di, D2CcFrameInfo &fi, D2CcBitReaderState &rs)
{
    int Variable0, OptionalData, CodedBytes;
    Variable0 = readBits(rawData, di.Variable0Bits, rs);
    fi.width = readBits(rawData, di.WidthBits, rs);
    fi.height = readBits(rawData, di.HeightBits, rs);

    fi.XOffset = readBits(rawData, di.XOffsetBits, rs);
    fi.YOffset = readBits(rawData, di.YOffsetBits, rs);
    OptionalData = readBits(rawData, di.OptionalDataBits, rs);
    CodedBytes = readBits(rawData, di.CodedBytesBits, rs);

    fi.flipped = readBits(rawData, 1, rs);

    fi.YOffset = fi.YOffset - fi.height + 1;
    // fi.box = { fi.XOffset, fi.YOffset - fi.Height + 1, fi.width, fi.height};
}

static bool readDirHeader(const QByteArray &rawData, const D2CcHeaderInfo &hi, D2CcDirectionInfo &di, D2CcBitReaderState &rs)
{
    int bitCursor = 0;

    di = { };

    // read direction-header
    const int directionHeaderBitSize = 32 + 2 + 4 + 4 + 4 + 4 + 4 + 4 + 4;
    bitCursor += directionHeaderBitSize;
    if (rawData.size() < (bitCursor + 8 - 1) / 8)
        return false;

    di.OutSizeCoded = readBits(rawData, 32, rs);
    di.CompressionFlags = readBits(rawData, 2, rs);
    di.Variable0Bits = crazyLookup(readBits(rawData, 4, rs));
    di.WidthBits = crazyLookup(readBits(rawData, 4, rs));
    di.HeightBits = crazyLookup(readBits(rawData, 4, rs));
    di.XOffsetBits = crazyLookup(readBits(rawData, 4, rs));
    di.YOffsetBits = crazyLookup(readBits(rawData, 4, rs));
    di.OptionalDataBits = crazyLookup(readBits(rawData, 4, rs));
    di.CodedBytesBits = crazyLookup(readBits(rawData, 4, rs));

    // read frame headers
    const int frameHeaderBitSize = di.Variable0Bits + di.WidthBits + di.HeightBits + di.XOffsetBits + di.YOffsetBits + di.OptionalDataBits + di.CodedBytesBits + 1;
    bitCursor += hi.framesPerDirection * frameHeaderBitSize;
    if (rawData.size() < (bitCursor + 8 - 1) / 8)
        return false;

    di.box = { INT_MAX, INT_MAX, INT_MIN, INT_MIN };

    for (unsigned i = 0; i < hi.framesPerDirection; i++) {
        D2CcFrameInfo fi;
        readFrameHeader(rawData, di, fi, rs);

    dProgressWarn() << QApplication::tr("frame dimensions %1x%2 with offset %3x%4 (flipped %5).").arg(fi.width).arg(fi.height).arg(fi.XOffset).arg(fi.YOffset).arg(fi.flipped);

        di.frameInfos.push_back(fi);

        if (di.box.x > fi.XOffset)
            di.box.x = fi.XOffset;
        if (di.box.y > fi.YOffset)
            di.box.y = fi.YOffset;
        if (di.box.w < fi.XOffset + fi.width)
            di.box.w = fi.XOffset + fi.width;
        if (di.box.h < fi.YOffset + fi.height)
            di.box.h = fi.YOffset + fi.height;
    }

    di.box.w -= di.box.x;
    di.box.h -= di.box.y;

dProgressWarn() << QApplication::tr("dir box %1x%2 with offset %3x%4.").arg(di.box.w).arg(di.box.h).arg(di.box.x).arg(di.box.y);

    di.HorizontalCellCount = (di.box.w + cellSize - 1) / cellSize;
    di.VerticalCellCount = (di.box.h + cellSize - 1) / cellSize;

    // read compression flags
    const int streamBitSize = 20;
    bitCursor += ((di.CompressionFlags & 2) ? streamBitSize : 0) + streamBitSize + ((di.CompressionFlags & 1) ? 2 * streamBitSize : 0);
    if (rawData.size() < (bitCursor + 8 - 1) / 8)
        return false;

    if (di.CompressionFlags & 2) {
        di.EqualCellsBitstreamSize = readBits(rawData, streamBitSize, rs);
    }

    di.PixelMaskBitstreamSize = readBits(rawData, streamBitSize, rs);

    if (di.CompressionFlags & 1) {
        di.EncodingTypeBitstreamSize = readBits(rawData, streamBitSize, rs);
        di.RawPixelCodesBitstreamSize = readBits(rawData, streamBitSize, rs);
    }

    // read palette entries
    bitCursor += D2PAL_COLORS;
    if (rawData.size() < (bitCursor + 8 - 1) / 8)
        return false;

    int paletteEntryCount = 0;
    for (unsigned i = 0; i < D2PAL_COLORS; i++) {
        if (readBits(rawData, 1, rs)) {
            di.PaletteEntries[paletteEntryCount] = i;
            paletteEntryCount++;
        }
    }

    // fillPixelBuffer

    return true;
}

bool D2Cc::load(D1Gfx &gfx, const QString &filePath, const OpenAsParam &params)
{
    gfx.clear();

    // Opening CEL file and load it in RAM
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QByteArray fileData = file.readAll();

    // Read CEL binary data
    QDataStream in(fileData);
    in.setByteOrder(QDataStream::LittleEndian);

    QIODevice *device = in.device();
    D2CcHeaderInfo hi;
    if (!readHeader(device, in, hi, gfx)) {
        return false;
    }
    // gfx.type = hi.groupped ? D1CEL_TYPE::V1_COMPILATION : D1CEL_TYPE::V1_REGULAR;
    gfx.type = D1CEL_TYPE::V1_COMPILATION;
    auto fileSize = device->size();

    // CEL FRAMES OFFSETS CALCULATION

    // BUILDING {CEL FRAMES}
    for (unsigned d = 0; d < hi.dirOffsets.size(); d++) {
        const auto &offset = hi.dirOffsets[d];
        if (fileSize >= offset.second && offset.second >= offset.first) {
            device->seek(offset.first);
            QByteArray ccDirRawData = device->read(offset.second - offset.first);

            D2CcBitReaderState rs = { };
            D2CcDirectionInfo cdi;
            if (readDirHeader(ccDirRawData, hi, cdi, rs)) {
                for (unsigned i = 0; i < hi.framesPerDirection; i++) {
                    D1GfxFrame *frame = new D1GfxFrame();

                    if (!D2CcFrame::load(*frame, ccDirRawData, params)) {
                        dProgressErr() << QApplication::tr("Frame %1 of direction %2 is invalid.").arg(i + 1).arg(d + 1);
                    }
                    gfx.frames.append(frame);
                }
                continue;
            }
        } else {
            dProgressErr() << QApplication::tr("Address of Direction %1 is invalid (%2-%3 of %4).").arg(d + 1).arg(offset.first).arg(offset.second).arg(fileSize);
        }
        for (unsigned i = 0; i < hi.framesPerDirection; i++) {
            gfx.frames.append(new D1GfxFrame());
        }
    }
    gfx.clipped = false;

    gfx.gfxFilePath = filePath;
    gfx.modified = false;
    return true;
}
