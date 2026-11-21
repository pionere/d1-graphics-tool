#include "d2cc.h"
/*
 * Dcc decoder based on the implementation from https://github.com/Lectem/Worldstone .
 */

#include <QtAlgorithms>
#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>
#include <QVector>

#include "progressdialog.h"
#include "resizedialog.h"

#include "dungeon/all.h"

#define D2PAL_COLORS 256

#define DEBUGDCC 0
#if DEBUGDCC
#define LogDebug(exp, ...) LogErrorF(exp, __VA_ARGS__)
#else
#define LogDebug(exp, ...) do { (void) sizeof(exp); } while(0)
#endif

static const int cellSize = 4;

typedef struct D2CcHeaderInfo
{
    QList<QPair<quint32, quint32>> dirOffsets;
    quint32 framesPerDirection;
} D2CcHeaderInfo;

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

    hi.framesPerDirection = framesPerDirectionDword;
    // DCC FRAMES OFFSETS CALCULATION
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
        dProgressErr() << QApplication::tr("Invalid index (%1) to the lookup table (must be less than %2).").arg(idx).arg(lengthof(crazyBitTable));
        idx = 0;
    }
    return crazyBitTable[idx];
}

typedef struct D2CcBitReaderState
{
    // reader state
    QByteArray *rawStreamData;
    quint64 remVal;
    unsigned remBits;
    int cursor;
} D2CcBitReaderState;

static quint32 readBits(D2CcBitReaderState& hi, unsigned numBits)  // quint32 &remVal, int &remBits, int &cursor)
{
    while (numBits > hi.remBits) {
        quint8 val = (*hi.rawStreamData)[hi.cursor];
        hi.cursor++;
        if (hi.remBits >= 64 - 8) {
            // discard bits in case of a long read assuming it is for skipping
            hi.remVal >>= 32;
            hi.remBits -= 32;
            numBits -= 32;
        }
        hi.remVal |= (quint64)val << hi.remBits;
        hi.remBits += 8;
    }

    quint32 res = hi.remVal;
    if (numBits < 64) {
        res &= ((1ll << numBits) - 1);
        hi.remVal >>= numBits;
        hi.remBits -= numBits;
    } else {
        hi.remVal = 0;
        hi.remBits = 0;
    }
    return res;
}

static qint32 readSgnBits(D2CcBitReaderState& hi, unsigned numBits)
{
    if (numBits > 32) {
        LogDebug("oob readSgnBits does not support %d", numBits);
        dProgressErr() << QApplication::tr("oob readSgnBits does not support %1").arg(numBits);
    }
    quint32 res = readBits(hi, numBits);
    quint32 sgn = (UINT32_MAX >> (numBits - 1)) << (numBits - 1);
    if (res & sgn) {
        res |= sgn;
    }
    return res;
}

typedef struct PixelBufferEntry
{
    static constexpr size_t nbValues = 4;
    uint8_t                 values[nbValues];
} PixelBufferEntry;

typedef struct D2CcFrameInfo
{
    // values from file
    int width;
    int height;
    int XOffset;
    int YOffset;
    unsigned OptionalData;
    bool flipped;
    // calculated values
    unsigned XCells;
    unsigned YCells;
    QList<int> cellWidths;
    QList<int> cellHeights;
    // RECT32 box;
    unsigned firstPixelBufferEntry;
    QList<bool> cellSameAsPrevious;
} D2CcFrameInfo;

typedef struct D2CcDirectionInfo
{
    int OutSizeCoded;
    bool hasRawPixelEncoding;
    bool compressEqualCells;
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
    unsigned pixelCodesDisplacementBitStreamSize;

    unsigned paletteEntryCount;
    quint8 PaletteEntries[256];

    unsigned HorizontalCellCount;
    unsigned VerticalCellCount;

    QList<PixelBufferEntry> pbEntries;
    QList<D2CcFrameInfo> frameInfos;
    RECT32 box;

    D2CcBitReaderState equalCellBitStream;
    D2CcBitReaderState pixelMaskBitStream;
    D2CcBitReaderState encodingTypeBitStream;
    D2CcBitReaderState rawPixelCodesBitStream;
    D2CcBitReaderState pixelCodesDisplacementBitStream;
} D2CcDirectionInfo;

typedef struct CellInfo
{
    int width;
    int height;
    int xoffset;
    int yoffset;
} CellInfo;

static void readFrameHeader(D2CcBitReaderState &rs, const D2CcDirectionInfo &di, D2CcFrameInfo &fi)
{
    int Variable0, CodedBytes;
    Variable0 = readBits(rs, di.Variable0Bits);
    fi.width = readBits(rs, di.WidthBits);
    fi.height = readBits(rs, di.HeightBits);

    fi.XOffset = readSgnBits(rs, di.XOffsetBits);
    fi.YOffset = readSgnBits(rs, di.YOffsetBits);
    fi.OptionalData = readBits(rs, di.OptionalDataBits);
    CodedBytes = readBits(rs, di.CodedBytesBits);

    fi.flipped = readBits(rs, 1);

    if (!fi.flipped)
        fi.YOffset = fi.YOffset - fi.height + 1;
    // fi.box = { fi.XOffset, fi.YOffset - fi.Height + 1, fi.width, fi.height};
}

/**
 * @return the number of pixels codes decoded from the stream
 */
static int decodePixelCodesStack(D2CcDirectionInfo& dirInfo, uint8_t pixelMask, uint8_t (&pixelCodesStack)[4])
{
    if (!pixelMask) return 0; // Reuse the previous cell values, but still decode the cell in stage2
    const int nbPixelsInMask = qPopulationCount(pixelMask);

    // Is the cell encoded in the raw stream ?
    bool decodeRaw = false;
    if (dirInfo.EncodingTypeBitstreamSize != 0) {
        dirInfo.EncodingTypeBitstreamSize = dirInfo.EncodingTypeBitstreamSize - 1;
        decodeRaw = readBits(dirInfo.encodingTypeBitStream, 1);
    }
    uint8_t lastPixelCode = 0;
    int curPixelIdx = 0;
    for ( ; curPixelIdx < nbPixelsInMask; curPixelIdx++) {
        uint8_t nextPixelCode;
        if (decodeRaw) {
            // Read the value of the code directly from rawPixelCodesBitStream
            if (dirInfo.RawPixelCodesBitstreamSize < 8) {
                LogDebug("oob in rawPixelCodesBitStream %d", dirInfo.RawPixelCodesBitstreamSize);
                dProgressErr() << QApplication::tr("oob in rawPixelCodesBitStream %1").arg(dirInfo.RawPixelCodesBitstreamSize);
            }
            dirInfo.RawPixelCodesBitstreamSize -= 8;
            nextPixelCode = readBits(dirInfo.rawPixelCodesBitStream, 8);
        } else {
            // Read the value of the code incrementally from pixelCodesDisplacementBitStream
            nextPixelCode = lastPixelCode;
            uint8_t pixelDisplacement;
            do {
                if (dirInfo.pixelCodesDisplacementBitStreamSize < 4) {
                    LogDebug("oob in pixelCodesDisplacementBitStream %d", dirInfo.pixelCodesDisplacementBitStreamSize);
                    dProgressErr() << QApplication::tr("oob in pixelCodesDisplacementBitStream %1").arg(dirInfo.pixelCodesDisplacementBitStreamSize);
                }
                dirInfo.pixelCodesDisplacementBitStreamSize -= 4;
                pixelDisplacement = readBits(dirInfo.pixelCodesDisplacementBitStream, 4);
                nextPixelCode += pixelDisplacement;
            } while (pixelDisplacement == 0xF);
        }
        // Stop decoding if we encounter twice the same pixel code.
        // It also means that this pixel code is discarded.
        if (nextPixelCode == lastPixelCode) {
            // Note : We discard the pixel by putting a 0 but it doesn't matter anyway since we only
            // use nbPixelsDecoded values later when popping the stack
            pixelCodesStack[curPixelIdx] = 0;
            break;
        } else {
            pixelCodesStack[curPixelIdx] = nextPixelCode;
            lastPixelCode = nextPixelCode;
        }
    }
    return curPixelIdx;
}

static void decodeFrameStage1(D2CcDirectionInfo& dirInfo, D2CcFrameInfo &frameInfo, QVector<unsigned>& pixelBuffer,
                       QList<PixelBufferEntry>& pbEntries)
{
    // Offset in terms of cells for this frame
    const unsigned frameCellOffsetX = (unsigned)frameInfo.XOffset / cellSize;
    const unsigned frameCellOffsetY = (unsigned)frameInfo.YOffset / cellSize;

    LogDebug("decodeFrameStage1 offset %d:%d cells %d:%d", frameInfo.XOffset, frameInfo.YOffset, frameInfo.XCells, frameInfo.YCells);
    // For each cell of this frame (not the same number as the pixel buffer cells ! )
    for (unsigned y = 0; y < frameInfo.YCells; y++) {
        const unsigned curCellY = frameCellOffsetY + y;
        for (unsigned x = 0; x < frameInfo.XCells; x++) {
            const unsigned curCellX = frameCellOffsetX + x;

            const unsigned curPbCellIndex    = curCellX + curCellY * dirInfo.HorizontalCellCount;
            if (curPbCellIndex >= (unsigned)pixelBuffer.size()) {
                LogDebug("oob in decodeFrameStage1 pbuf %d of %d cell [%d:%d] area [%d:%d]", curPbCellIndex, pixelBuffer.size(), curCellX, curCellY, dirInfo.HorizontalCellCount, dirInfo.VerticalCellCount);
                dProgressErr() << QApplication::tr("oob in decodeFrameStage1 pbuf %1 of %2 cell [%3:%4] area [%5:%6]").arg(curPbCellIndex).arg(pixelBuffer.size()).arg(curCellX).arg(curCellY).arg(dirInfo.HorizontalCellCount).arg(dirInfo.VerticalCellCount);
            }
            unsigned& lastPixelEntryIndexForCell = pixelBuffer[curPbCellIndex];

            bool    sameAsPreviousCell = false; // By default always decode the cell
            uint8_t pixelMask          = 0x0F;  // Default pixel mask

            // Check if this cell is equal to the previous one
            if (lastPixelEntryIndexForCell < (unsigned)pbEntries.size()) {
                // Check if we have to reuse the previous values
                if (dirInfo.compressEqualCells) {
                    // If true, the cell is the same as the previous one or transparent.
                    // Which actually mean the same thing : skip the decoding of this cell
                    if (dirInfo.EqualCellsBitstreamSize < 1) {
                        LogDebug("oob in decodeFrameStage1 cell %d", dirInfo.EqualCellsBitstreamSize);
                        dProgressErr() << QApplication::tr("oob in decodeFrameStage1 cell %1").arg(dirInfo.EqualCellsBitstreamSize);
                    }
                    dirInfo.EqualCellsBitstreamSize -= 1;
                    sameAsPreviousCell = readBits(dirInfo.equalCellBitStream, 1);
                }
                if (!sameAsPreviousCell) {
                    if (dirInfo.PixelMaskBitstreamSize < 4) {
                        LogDebug("oob in decodeFrameStage1 mask %d", dirInfo.PixelMaskBitstreamSize);
                        dProgressErr() << QApplication::tr("oob in decodeFrameStage1 mask %1").arg(dirInfo.PixelMaskBitstreamSize);
                    }
                    dirInfo.PixelMaskBitstreamSize -= 4;
                    pixelMask = readBits(dirInfo.pixelMaskBitStream, 4);
                }
            }

            // Store the fact that we skipped this cell for the 2nd phase
            frameInfo.cellSameAsPrevious.push_back(sameAsPreviousCell);

            if (!sameAsPreviousCell) {
                // Pixel buffer entries are encoded as a stack in the stream which means the
                // last value decoded is actually the 1st value with a bit in the mask.
                uint8_t pixelCodesStack[PixelBufferEntry::nbValues] = { };
                int nbPixelsDecoded = decodePixelCodesStack(dirInfo, pixelMask, pixelCodesStack);

                PixelBufferEntry previousEntryForCell;
                if (lastPixelEntryIndexForCell < (unsigned)pbEntries.size()) {
                    previousEntryForCell = pbEntries[lastPixelEntryIndexForCell];
                } else {
                    // No need for previousEntryForCell if it doesn't exist, as the mask is 0xF
                    if (pixelMask != 0xF) {
                        LogDebug("previousEntryForCell is not set with mask %d", pixelMask);
                        dProgressErr() << QApplication::tr("previousEntryForCell is not set with mask %1").arg(pixelMask);
                    }
                }

                // Finalize the decoding of the pixel buffer entry
                PixelBufferEntry newEntry;

                int curIndex = nbPixelsDecoded - 1;
                for (size_t i = 0; i < PixelBufferEntry::nbValues; i++) {
                    uint8_t pixel;
                    if (pixelMask & (1u << i)) {
                        // Pop a value if bit set in the mask
                        if (curIndex >= 0) {
                            pixel = pixelCodesStack[size_t(curIndex--)];
                        } else {
                            pixel = 0;
                        }
                        // Store the actual values instead of the codes
                        if (dirInfo.paletteEntryCount <= pixel) {
                            LogDebug("PaletteEntries is not set for pixel %d (entries: %d)", pixel, dirInfo.paletteEntryCount);
                            dProgressErr() << QApplication::tr("PaletteEntries is not set for pixel %1 (entries: %2)").arg(pixel).arg(dirInfo.paletteEntryCount);
                        }
                        pixel = dirInfo.PaletteEntries[pixel];
                    } else {
                        // If not set, use the previous value for this entry
                        pixel = previousEntryForCell.values[i];
                    }
                    newEntry.values[i] = pixel;
                }
                // Update the pixel buffer cell information
                lastPixelEntryIndexForCell = pbEntries.size();
                // Add the new entry for use in the 2nd stage
                pbEntries.push_back(newEntry);
            }
        }
    }
}

static int calculateCells(int len, int offset, QList<int> &values)
{
    // add partial starting cell if necessary
    if (offset % cellSize != 0) {
        int widthFirstColumn = 4 - offset % cellSize;
        len -= widthFirstColumn;
        if (len < 0) {
            widthFirstColumn += len;
            len = 0;
        }
        values.push_back(widthFirstColumn);
    }
    // add standard cells
    int nc = (unsigned)len / cellSize;
    for (int i = 0; i < nc; i++) {
        values.push_back(cellSize);
    }
    len -= nc * cellSize;
    // add an extra cell if it is larger than 1 pixel, otherwise extend the last cell
    if (len > 0) {
        if (len > 1) {
            values.push_back(len);
        } else {
            values.last()++;
        }
    }
    return values.size();
}

static bool readDirHeader(D2CcBitReaderState &rs, const D2CcHeaderInfo &hi, D2CcDirectionInfo &di)
{
    int bitCursor = 0;

    // read direction-header
    const int directionHeaderBitSize = 32 + 1 + 1 + 4 + 4 + 4 + 4 + 4 + 4 + 4;
    bitCursor += directionHeaderBitSize;
    if (rs.rawStreamData->size() < (bitCursor + CHAR_BIT - 1) / CHAR_BIT)
        return false;

    di.OutSizeCoded = readBits(rs, 32);
    di.hasRawPixelEncoding = readBits(rs, 1);
    di.compressEqualCells = readBits(rs, 1);
    di.Variable0Bits = crazyLookup(readBits(rs, 4));
    di.WidthBits = crazyLookup(readBits(rs, 4));
    di.HeightBits = crazyLookup(readBits(rs, 4));
    di.XOffsetBits = crazyLookup(readBits(rs, 4));
    di.YOffsetBits = crazyLookup(readBits(rs, 4));
    di.OptionalDataBits = crazyLookup(readBits(rs, 4));
    di.CodedBytesBits = crazyLookup(readBits(rs, 4));

    // read frame headers
    const int frameHeaderBitSize = di.Variable0Bits + di.WidthBits + di.HeightBits + di.XOffsetBits + di.YOffsetBits + di.OptionalDataBits + di.CodedBytesBits + 1;
    bitCursor += hi.framesPerDirection * frameHeaderBitSize;
    if (rs.rawStreamData->size() < (bitCursor + CHAR_BIT - 1) / CHAR_BIT)
        return false;

    di.box = { INT_MAX, INT_MAX, INT_MIN, INT_MIN };

    for (unsigned i = 0; i < hi.framesPerDirection; i++) {
        D2CcFrameInfo fi;
        readFrameHeader(rs, di, fi);

        LogDebug("frame %d dimensions %dx%d with offset %dx%d (flipped %d).", i, fi.width, fi.height, fi.XOffset, fi.YOffset, fi.flipped);

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

    di.HorizontalCellCount = (di.box.w + cellSize - 1) / cellSize;
    di.VerticalCellCount = (di.box.h + cellSize - 1) / cellSize;

    for (unsigned i = 0; i < hi.framesPerDirection; i++) {
        D2CcFrameInfo &fi = di.frameInfos[i];
        readBits(rs, fi.OptionalData);

        fi.XOffset -= di.box.x;
        fi.YOffset -= di.box.y;

        fi.XCells = calculateCells(fi.width, fi.XOffset, fi.cellWidths);
        fi.YCells = calculateCells(fi.height, fi.YOffset, fi.cellHeights);
    }

//    di.box.x = 0;
//    di.box.y = 0;

    // read compression flags
    const int streamBitSize = 20;
    bitCursor += (di.compressEqualCells ? streamBitSize : 0) + streamBitSize + (di.hasRawPixelEncoding ? 2 * streamBitSize : 0);
    if (rs.rawStreamData->size() < (bitCursor + CHAR_BIT - 1) / CHAR_BIT)
        return false;

    di.EqualCellsBitstreamSize = 0;
    if (di.compressEqualCells) {
        di.EqualCellsBitstreamSize = readBits(rs, streamBitSize);
    }

    di.PixelMaskBitstreamSize = readBits(rs, streamBitSize);

    di.EncodingTypeBitstreamSize = 0;
    di.RawPixelCodesBitstreamSize = 0;
    if (di.hasRawPixelEncoding) {
        di.EncodingTypeBitstreamSize = readBits(rs, streamBitSize);
        di.RawPixelCodesBitstreamSize = readBits(rs, streamBitSize);
    }

    // read palette entries
    bitCursor += D2PAL_COLORS;
    if (rs.rawStreamData->size() < (bitCursor + CHAR_BIT - 1) / CHAR_BIT)
        return false;

    di.paletteEntryCount = 0;
    for (unsigned i = 0; i < D2PAL_COLORS; i++) {
        if (readBits(rs, 1)) {
            di.PaletteEntries[di.paletteEntryCount] = i;
            di.paletteEntryCount++;
        }
    }

    // prepare subviews
    bitCursor += di.EqualCellsBitstreamSize + di.PixelMaskBitstreamSize + di.EncodingTypeBitstreamSize + di.RawPixelCodesBitstreamSize;
    if (rs.rawStreamData->size() < (bitCursor + CHAR_BIT - 1) / CHAR_BIT)
        return false;

    di.equalCellBitStream = rs;
    di.pixelMaskBitStream = di.equalCellBitStream;
    readBits(di.pixelMaskBitStream, di.EqualCellsBitstreamSize);
    di.encodingTypeBitStream = di.pixelMaskBitStream;
    readBits(di.encodingTypeBitStream, di.PixelMaskBitstreamSize);
    di.rawPixelCodesBitStream = di.encodingTypeBitStream;
    readBits(di.rawPixelCodesBitStream, di.EncodingTypeBitstreamSize);

    di.pixelCodesDisplacementBitStream = di.rawPixelCodesBitStream;
    readBits(di.pixelCodesDisplacementBitStream, di.RawPixelCodesBitstreamSize);

    di.pixelCodesDisplacementBitStreamSize = di.pixelCodesDisplacementBitStream.remBits + (di.pixelCodesDisplacementBitStream.rawStreamData->size() - di.pixelCodesDisplacementBitStream.cursor) * CHAR_BIT;

    // fillPixelBuffer
    // For each cell store a PixelBufferEntry index that points to the last entry for this cell
    // This will be used to retrieve values from the previous frame
    constexpr unsigned invalidIndex = UINT_MAX;
    const size_t  pixelBufferNbCells = di.HorizontalCellCount * di.VerticalCellCount;
    QVector<unsigned> pixelBuffer(pixelBufferNbCells, invalidIndex);

    // 1st phase of decoding : fill the pixel buffer
    // We actually fill a buffer of entries as to avoid storing empty entries
    for (unsigned i = 0; i < hi.framesPerDirection; i++) {
        D2CcFrameInfo &fi = di.frameInfos[i];
        fi.firstPixelBufferEntry = di.pbEntries.size();
        decodeFrameStage1(di, fi, pixelBuffer, di.pbEntries);
    }
    return true;
}

bool D2Cc::decodeFrames(const D2CcHeaderInfo& hi, D2CcDirectionInfo &di, D1Gfx &gfx)
{
    QVector<CellInfo> pixelBufferCells(di.HorizontalCellCount * di.VerticalCellCount);
    D1GfxFrame *pBuffer = new D1GfxFrame();
    pBuffer->resize(di.box.w, di.box.h, RESIZE_PLACEMENT::TOP_LEFT, D1GfxPixel::colorPixel(0));
    // 2nd phase of decoding : Finish using the pixel buffer entries
    for (unsigned frameIndex = 0; frameIndex < hi.framesPerDirection; ++frameIndex) {
        const D2CcFrameInfo &frameInfo = di.frameInfos[frameIndex];
        size_t           pbEntryIndex = frameInfo.firstPixelBufferEntry;

        size_t pbCellPosY = frameInfo.YOffset;
        for (size_t cellY = 0; cellY < frameInfo.YCells; cellY++) {
            size_t pbCellPosX = frameInfo.XOffset;
            const int ch = frameInfo.cellHeights[cellY];
            const int yo = pbCellPosY % cellSize;

            for (size_t cellX = 0; cellX < frameInfo.XCells; cellX++) {
                const size_t frameCellIndex = cellX + cellY * frameInfo.XCells;
                const int cw = frameInfo.cellWidths[cellX];
                const int xo = pbCellPosX % cellSize;
                const size_t pbCellIndex =
                    (pbCellPosX / cellSize) +
                    (pbCellPosY / cellSize) * di.HorizontalCellCount;
                CellInfo& pbCell = pixelBufferCells[pbCellIndex];

                if (frameInfo.cellSameAsPrevious[frameCellIndex]) {
                    if (pbCell.width != cw || pbCell.height != ch || pbCell.xoffset != xo || pbCell.yoffset != yo) {
                        if (pbCell.width == cw && pbCell.height == ch)
                            dProgressWarn() << QApplication::tr("mismatching cell offsets (%1:%2 vs %3:%4) in frame %5 of direction %6 at %4:%5 (area %6x%7)").arg(xo).arg(yo).arg(pbCell.xoffset).arg(pbCell.yoffset).arg(frameIndex + 1).arg(gfx.frames.size() / hi.framesPerDirection + 1).arg(pbCellPosX).arg(pbCellPosY).arg(cw).arg(ch);
                        for (int y = 0; y < ch; y++) {
                            for (int x = 0; x < cw; x++) {
                                pBuffer->setPixel(pbCellPosX + x, pbCellPosY + y, D1GfxPixel::colorPixel(0));
                            }
                        }
                    }
                } else {
                    const auto& pixelValues = di.pbEntries[pbEntryIndex++].values;

                    if (pixelValues[0] == pixelValues[1]) {
                        // This means we only got one pixel code, so fill the cell with it
                        const D1GfxPixel pixel = D1GfxPixel::colorPixel(pixelValues[0]);
                        for (int y = 0; y < ch; y++) {
                            for (int x = 0; x < cw; x++) {
                                pBuffer->setPixel(pbCellPosX + x, pbCellPosY + y, pixel);
                            }
                        }
                    } else {
                        unsigned nbBitsToRead;
                        if (pixelValues[1] == pixelValues[2]) {
                            // Stopped decoding after the 2nd value, only pixelValues[0] and
                            // pixelValues[1] are different, so only 1bit needs to be read to choose
                            // from those values
                            nbBitsToRead = 1;
                        } else {
                            // We need 2 bits to index 3-4 values
                            nbBitsToRead = 2;
                        }

                        // fill FRAME cell with pixels
                        for (int y = 0; y < ch; y++) {
                            for (int x = 0; x < cw; x++) {
                                if (di.pixelCodesDisplacementBitStreamSize < nbBitsToRead) {
                                    LogDebug("oob in pixelCodesDisplacementBitStream %d", di.pixelCodesDisplacementBitStreamSize);
                                    dProgressErr() << QApplication::tr("oob in pixelCodesDisplacementBitStream %1").arg(di.pixelCodesDisplacementBitStreamSize);
                                }
                                di.pixelCodesDisplacementBitStreamSize -= nbBitsToRead;
                                const uint8_t pixelCodeIndex =
                                    readBits(di.pixelCodesDisplacementBitStream, nbBitsToRead);
                                // Note: This actually means that a cell (4x4 block) can use at most
                                // 4 colors, a bit like DXT !
                                const uint8_t pixelValue = pixelValues[pixelCodeIndex];
                                pBuffer->setPixel(pbCellPosX + x, pbCellPosY + y, D1GfxPixel::colorPixel(pixelValue));
                            }
                        }
                    }
                }

                pbCell.width = cw;
                pbCell.height = ch;
                pbCell.xoffset = xo;
                pbCell.yoffset = yo;

                pbCellPosX += cw;
            }
            pbCellPosY += ch;
        }
        // Done decoding this frame, now copy its content.
        D1GfxFrame *frame = new D1GfxFrame();
        frame->resize(frameInfo.width, frameInfo.height, RESIZE_PLACEMENT::TOP_LEFT, D1GfxPixel::transparentPixel());
        for (int x = 0; x < frameInfo.width; x++) {
            for (int y = 0; y < frameInfo.height; y++) {
                const D1GfxPixel pixel = pBuffer->getPixel(x + frameInfo.XOffset, y + frameInfo.YOffset);
                frame->setPixel(x, y, pixel.getPaletteIndex() == 0 ? D1GfxPixel::transparentPixel() : pixel);
            }
        }
        frame->flipped = frameInfo.flipped;
        frame->xoffset = frameInfo.XOffset + di.box.x;
        frame->yoffset = frameInfo.YOffset + di.box.y;
        if (!frame->flipped)
            frame->yoffset += frameInfo.height - 1;
        gfx.frames.append(frame);
    }
    delete pBuffer;
    return true;
}

bool D2Cc::load(D1Gfx &gfx, const QString &filePath, const OpenAsParam &params)
{
    gfx.clear();

    // Opening DCC file and load it in RAM
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

    // BUILDING {DCC FRAMES}
    for (unsigned d = 0; d < (unsigned)hi.dirOffsets.size(); d++) {
        const auto &offset = hi.dirOffsets[d];
        if (fileSize >= offset.second && offset.second >= offset.first) {
            device->seek(offset.first);
            QByteArray ccDirRawData = device->read(offset.second - offset.first);

            D2CcBitReaderState rs = { };
            rs.rawStreamData = &ccDirRawData;
            D2CcDirectionInfo cdi;
            if (readDirHeader(rs, hi, cdi)) {
                if (decodeFrames(hi, cdi, gfx)) {
                    if (cdi.EqualCellsBitstreamSize != 0) {
                        dProgressWarn() << QApplication::tr("Direction %1 has %2 unused bits in the equal cells stream.").arg(d + 1).arg(cdi.EqualCellsBitstreamSize);
                    }
                    if (cdi.PixelMaskBitstreamSize != 0) {
                        dProgressWarn() << QApplication::tr("Direction %1 has %2 unused bits in the pixel masks stream.").arg(d + 1).arg(cdi.PixelMaskBitstreamSize);
                    }
                    if (cdi.EncodingTypeBitstreamSize != 0) {
                        dProgressWarn() << QApplication::tr("Direction %1 has %2 unused bits in the encodings stream.").arg(d + 1).arg(cdi.EncodingTypeBitstreamSize);
                    }
                    if (cdi.pixelCodesDisplacementBitStreamSize > cdi.pixelCodesDisplacementBitStream.remBits) {
                        dProgressWarn() << QApplication::tr("Direction %1 has %2 unused bits in the pixel codes stream.").arg(d + 1).arg(cdi.pixelCodesDisplacementBitStreamSize);
                    }
                    continue;
                }
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
