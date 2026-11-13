#include "d2cc.h"
/*
 * Dcc decoder based on the implementation from https://github.com/Lectem/Worldstone .
 */
#include <vector>

#include <QtAlgorithms>
#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>

#include "d2ccframe.h"
#include "progressdialog.h"
#include "resizedialog.h"

#include "dungeon/all.h"

#define D2PAL_COLORS 256

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

    // Trying to find file size in CEL header
    // if (fileSize < sizeDword)
    //    dProgressWarn() << QApplication::tr("Invalid filesize %1 (expected %2).").arg(sizeDword).arg(fileSize);
    //    return false;

    hi.framesPerDirection = framesPerDirectionDword;
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
    QByteArray *rawStreamData;
    quint64 remVal;
    int remBits;
    int cursor;
} D2CcBitReaderState;

static quint32 readBits(D2CcBitReaderState& hi, int numBits)  // quint32 &remVal, int &remBits, int &cursor)
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
        hi.remVal |= val << hi.remBits;
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
    bool flipped;
    // calculated values
    int XCells;
    int YCells;
    // RECT32 box;
    size_t firstPixelBufferEntry;
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

    quint8 PaletteEntries[256];
    /*
    Box                        *image.Rectangle
    Cells                      []*Cell
    PixelData                  []byte
    PixelBuffer                PixelBuffer
    */
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

static void readFrameHeader(D2CcBitReaderState &rs, const D2CcDirectionInfo &di, D2CcFrameInfo &fi)
{
    int Variable0, OptionalData, CodedBytes;
    Variable0 = readBits(rs, di.Variable0Bits);
    fi.width = readBits(rs, di.WidthBits);
    fi.height = readBits(rs, di.HeightBits);

    fi.XOffset = readBits(rs, di.XOffsetBits);
    fi.YOffset = readBits(rs, di.YOffsetBits);
    OptionalData = readBits(rs, di.OptionalDataBits);
    CodedBytes = readBits(rs, di.CodedBytesBits);

    fi.flipped = readBits(rs, 1);

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
    const bool decodeRaw = (dirInfo.encodingTypeBitStream.remBits != 0 || (dirInfo.encodingTypeBitStream.rawStreamData->size() > dirInfo.encodingTypeBitStream.cursor)) &&
                           readBits(dirInfo.encodingTypeBitStream, 1);
LogErrorF("decodePixelCodesStack %d , %d.", nbPixelsInMask, decodeRaw);
    uint8_t lastPixelCode = 0;
    int  curPixelIdx   = 0;

    for ( ; curPixelIdx < nbPixelsInMask; curPixelIdx++) {
        uint8_t nextPixelCode;
        if (decodeRaw) {
            // Read the value of the code directly from rawPixelCodesBitStream
            nextPixelCode = readBits(dirInfo.rawPixelCodesBitStream, 8);
        } else {
            // Read the value of the code incrementally from pixelCodesDisplacementBitStream
            nextPixelCode = lastPixelCode;
            uint8_t pixelDisplacement;
            do {
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

static void decodeFrameStage1(D2CcDirectionInfo& dirInfo, D2CcFrameInfo &frameInfo, QList<size_t>& pixelBuffer,
                       QList<PixelBufferEntry>& pbEntries)
{
    // Offset in terms of cells for this frame
    const size_t frameCellOffsetX = frameInfo.XOffset / cellSize;
    const size_t frameCellOffsetY = frameInfo.YOffset / cellSize;

    // For each cell of this frame (not the same number as the pixel buffer cells ! )
    for (size_t y = 0; y < frameInfo.YCells; y++) {
        const size_t curCellY = frameCellOffsetY + y;
        for (size_t x = 0; x < frameInfo.XCells; x++) {
            const size_t curCellX = frameCellOffsetX + x;

            const size_t curPbCellIndex    = curCellX + curCellY * dirInfo.HorizontalCellCount;

            size_t& lastPixelEntryIndexForCell = pixelBuffer[curPbCellIndex];

            bool    sameAsPreviousCell = false; // By default always decode the cell
            uint8_t pixelMask          = 0x0F;  // Default pixel mask

            // Check if this cell is equal to the previous one
            if (lastPixelEntryIndexForCell < (size_t)pbEntries.size()) {
                // Check if we have to reuse the previous values
                if (dirInfo.compressEqualCells) {
                    // If true, the cell is the same as the previous one or transparent.
                    // Which actually mean the same thing : skip the decoding of this cell
                    sameAsPreviousCell = readBits(dirInfo.equalCellBitStream, 1);
                }
                if (!sameAsPreviousCell) {
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
                if (lastPixelEntryIndexForCell < (size_t)pbEntries.size()) {
                    previousEntryForCell = pbEntries[lastPixelEntryIndexForCell];
                } else {
                    // No need for previousEntryForCell if it doesn't exist, as the mask is 0xF
                    assert(pixelMask == 0xF);
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

static int cellCountAt(int len, unsigned offset)
{
    if (len == 0)
        dProgressErr() << QString("Zero frame len.");
    int nc = 0;
    if (offset % cellSize != 0) {
        len -= offset % cellSize;
        //if (len < 0)
        //    len = 0;
        nc++;
    // } else if (len == 0) {
    //    len = 1;
    }
    nc += (len + cellSize - 1) / cellSize;

    return len;
}

static bool readDirHeader(D2CcBitReaderState &rs, const D2CcHeaderInfo &hi, D2CcDirectionInfo &di)
{
    int bitCursor = 0;

    di = { };

    // read direction-header
    const int directionHeaderBitSize = 32 + 1 + 1 + 4 + 4 + 4 + 4 + 4 + 4 + 4;
    bitCursor += directionHeaderBitSize;
    if (rs.rawStreamData->size() < (bitCursor + 8 - 1) / 8)
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
    if (rs.rawStreamData->size() < (bitCursor + 8 - 1) / 8)
        return false;

    di.box = { INT_MAX, INT_MAX, INT_MIN, INT_MIN };

    for (unsigned i = 0; i < hi.framesPerDirection; i++) {
        D2CcFrameInfo fi;
        readFrameHeader(rs, di, fi);

//    dProgressWarn() << QApplication::tr("frame %1 dimensions %2x%3 with offset %4x%5 (flipped %6).").arg(i).arg(fi.width).arg(fi.height).arg(fi.XOffset).arg(fi.YOffset).arg(fi.flipped);
LogErrorF("frame %d dimensions %dx%d with offset %dx%d (flipped %d).", i, fi.width, fi.height, fi.XOffset, fi.YOffset, fi.flipped);

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

// dProgressWarn() << QApplication::tr("dir box %1x%2 with offset %3x%4.").arg(di.box.w).arg(di.box.h).arg(di.box.x).arg(di.box.y);
LogErrorF("dir box %dx%d with offset %dx%d.", di.box.w, di.box.h, di.box.x, di.box.y);

    di.HorizontalCellCount = (di.box.w + cellSize - 1) / cellSize;
    di.VerticalCellCount = (di.box.h + cellSize - 1) / cellSize;

    for (unsigned i = 0; i < hi.framesPerDirection; i++) {
        D2CcFrameInfo &fi = di.frameInfos[i];
        fi.XOffset -= di.box.x;
        fi.YOffset -= di.box.y;

        fi.XCells = cellCountAt(fi.width, fi.XOffset);
        fi.YCells = cellCountAt(fi.height, fi.YOffset);
LogErrorF("frame offset %dx%d cells %dx%d.", fi.XOffset, fi.YOffset, fi.XCells, fi.YCells);
    }

    di.box.x = 0;
    di.box.y = 0;

    // read compression flags
    const int streamBitSize = 20;
    bitCursor += (di.compressEqualCells ? streamBitSize : 0) + streamBitSize + (di.hasRawPixelEncoding ? 2 * streamBitSize : 0);
    if (rs.rawStreamData->size() < (bitCursor + 8 - 1) / 8)
        return false;

    if (di.compressEqualCells) {
        di.EqualCellsBitstreamSize = readBits(rs, streamBitSize);
    }

    di.PixelMaskBitstreamSize = readBits(rs, streamBitSize);

    if (di.hasRawPixelEncoding) {
        di.EncodingTypeBitstreamSize = readBits(rs, streamBitSize);
        di.RawPixelCodesBitstreamSize = readBits(rs, streamBitSize);
    }

LogErrorF("streamsizes %d : %d : %d : %d.", di.EqualCellsBitstreamSize, di.PixelMaskBitstreamSize, di.EncodingTypeBitstreamSize, di.RawPixelCodesBitstreamSize);
    // read palette entries
    bitCursor += D2PAL_COLORS;
    if (rs.rawStreamData->size() < (bitCursor + 8 - 1) / 8)
        return false;

    int paletteEntryCount = 0;
    for (unsigned i = 0; i < D2PAL_COLORS; i++) {
        if (readBits(rs, 1)) {
            di.PaletteEntries[paletteEntryCount] = i;
            paletteEntryCount++;
        }
    }

    // prepare subviews
LogErrorF("preparing subviews.");
    di.equalCellBitStream = rs;
    di.pixelMaskBitStream = di.equalCellBitStream;
    readBits(di.pixelMaskBitStream, di.EqualCellsBitstreamSize);
LogErrorF("preparing subviews 1.");
    di.encodingTypeBitStream = di.pixelMaskBitStream;
    readBits(di.encodingTypeBitStream, di.PixelMaskBitstreamSize);
LogErrorF("preparing subviews 2.");
    di.rawPixelCodesBitStream = di.encodingTypeBitStream;
    readBits(di.rawPixelCodesBitStream, di.EncodingTypeBitstreamSize);
LogErrorF("preparing subviews 3.");

    di.pixelCodesDisplacementBitStream = di.rawPixelCodesBitStream;
    readBits(di.pixelCodesDisplacementBitStream, di.RawPixelCodesBitstreamSize);
LogErrorF("preparing subviews 4.");

    // fillPixelBuffer
    // For each cell store a PixelBufferEntry index that points to the last entry for this cell
    // This will be used to retrieve values from the previous frame
    constexpr size_t invalidIndex = SIZE_MAX;
    const size_t  pixelBufferNbCells = di.HorizontalCellCount * di.VerticalCellCount;
    QList<size_t> pixelBuffer(pixelBufferNbCells, invalidIndex);

    // 1st phase of decoding : fill the pixel buffer
    // We actually fill a buffer of entries as to avoid storing empty entries
    for (unsigned i = 0; i < hi.framesPerDirection; i++) {
        D2CcFrameInfo &fi = di.frameInfos[i];
        fi.firstPixelBufferEntry = di.pbEntries.size();
LogErrorF("decoding framebuffers %d of %d.", i + 1, hi.framesPerDirection);
        decodeFrameStage1(di, fi, pixelBuffer, di.pbEntries);
    }
LogErrorF("readDirHeader done.");
    return true;
}

bool D2Cc::decodeFrames(const D2CcHeaderInfo& hi, D2CcDirectionInfo &di, D1Gfx &gfx)
{
    // This is the reason why we need to stages, we don't have the offset of this bitstream
    // di.pixelCodesDisplacementBitStream;
LogErrorF("decodeFrames 0");
    const size_t pbWidth            = size_t(di.box.w);
    const size_t pbHeight           = size_t(di.box.h);
    // const size_t pbStride           = pbWidth;
    // const size_t nbPixelBufferCells = di.HorizontalCellCount * di.VerticalCellCount;
LogErrorF("decodeFrames area %d:%d.", (unsigned)pbWidth, (unsigned)pbHeight);
    // QList<Cell>       pixelBufferCells(nbPixelBufferCells, Cell{0xF, 0xF});
    // QList<uint8_t>    pixelBufferColors(pbStride * pbHeight);
    // ImageView<uint8_t> pBuffer{pixelBufferColors.data(), pbWidth, pbHeight, pbStride};
    D1GfxFrame *pBuffer = new D1GfxFrame();
    // pBuffer->resize(pbWidth, pbHeight, RESIZE_PLACEMENT::TOP_LEFT, D1GfxPixel::colorPixel(0));
    pBuffer->resize(pbWidth, pbHeight, RESIZE_PLACEMENT::TOP_LEFT, D1GfxPixel::transparentPixel());

    // 2nd phase of decoding : Finish using the pixel buffer entries
    for (size_t frameIndex = 0; frameIndex < hi.framesPerDirection; ++frameIndex) {
LogErrorF("decodeFrame %d", frameIndex);
        const D2CcFrameInfo &frameInfo = di.frameInfos[frameIndex];
        size_t           pbEntryIndex = frameInfo.firstPixelBufferEntry;

        size_t pbCellPosY = frameInfo.YOffset;
        for (size_t cellY = 0; cellY < frameInfo.YCells; cellY++) {
            size_t pbCellPosX = frameInfo.XOffset;

            int ch = cellSize;
            if (cellY == 0) {
                ch -= pbCellPosY % cellSize;
            }
            if (cellY == frameInfo.YCells - 1) {
                ch = (frameInfo.YOffset + frameInfo.height) % cellSize;
                if (ch == 0) {
                    ch = cellSize;
                }
            }

            for (size_t cellX = 0; cellX < frameInfo.XCells; cellX++)
            {
                const size_t frameCellIndex = cellX + cellY * frameInfo.XCells;
                // Cell         frameCell;

                int cw = cellSize;
                if (cellX == 0) {
                    cw -= pbCellPosX % cellSize;
                }
                if (cellX == frameInfo.XCells - 1) {
                    cw = (frameInfo.XOffset + frameInfo.width) % cellSize;
                    if (cw == 0) {
                        cw = cellSize;
                    }
                }

                // frameCell.width  = cw;
                // frameCell.height  = ch;
                // frameCell.width  = frameInfo.cellWidths[cellX];
                // frameCell.height = frameInfo.cellHeights[cellY];

                const size_t pbCellIndex =
                    (pbCellPosX / cellSize) +
                    (pbCellPosY / cellSize) * di.HorizontalCellCount;
//                Cell& pbCell = pixelBufferCells[pbCellIndex];

                if (frameInfo.cellSameAsPrevious[frameCellIndex]) {
/*
                    if ((cw != pbCell.width) || (ch != pbCell.height)) {
                        // Clear the cell to 0
                        // If we used the previous frame pixels instead of reusing the same
                        // buffer for all frames we would not need to clear this (it would be
                        // initialized to 0 before writing any value to the frame pixels) But we
                        // would then need to copy values if the size matched
//                        pBuffer.fillBytes(pbCellPosX, pbCellPosY, frameCell.width, frameCell.height,
//                                          0);
                    }
                    else
                    {
                        // Same size, copy from previous cell in the buffer
                        // Nothing to change !
                    }
*/
                } else {
                    const auto& pixelValues = di.pbEntries[pbEntryIndex++].values;

                    if (pixelValues[0] == pixelValues[1]) {
                        // This means we only got one pixel code, so fill the cell with it
                        for (int y = 0; y < ch; y++) {
                            for (int x = 0; x < cw; x++) {
                                pBuffer->setPixel(pbCellPosX + x, pbCellPosY + y, D1GfxPixel::colorPixel(pixelValues[0]));
                            }
                        }
                        // pBuffer.fillBytes(pbCellPosX, pbCellPosY, frameCell.width, frameCell.height,
                        //                  pixelValues[0]);
                    } else {
                        int nbBitsToRead = 0;
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
                        for (size_t y = 0; y < ch; y++) {
                            for (size_t x = 0; x < cw; x++) {
                                const uint8_t pixelCodeIndex =
                                    readBits(di.pixelCodesDisplacementBitStream, nbBitsToRead);
                                // Note: This actually means that a cell (4x4 block) can use at most
                                // 4 colors, a bit like DXT !
                                const uint8_t pixelValue = pixelValues[pixelCodeIndex];

                                // pBuffer(pbCellPosX + x, pbCellPosY + y) = pixelValue;
                                pBuffer->setPixel(pbCellPosX + x, pbCellPosY + y, D1GfxPixel::colorPixel(pixelValue));
                            }
                        }
                    }
                }

//                pbCell = frameCell;
                pbCellPosX += cw;
            }
            // pbCellPosY += frameInfo.cellHeights[cellY];
            pbCellPosY += ch;
        }
        // Done decoding this frame, now copy its content.
LogErrorF("decodeFrame %d size %dx%d done", frameIndex, frameInfo.width, frameInfo.height);
        D1GfxFrame *frame = new D1GfxFrame();
        frame->resize(frameInfo.width, frameInfo.height, RESIZE_PLACEMENT::TOP_LEFT, D1GfxPixel::transparentPixel());
        for (int x = 0; x < frameInfo.width; x++) {
            for (int y = 0; y < frameInfo.height; y++) {
                frame->setPixel(x, y, pBuffer->getPixel(x + frameInfo.XOffset, y + frameInfo.YOffset));
            }
        }
        gfx.frames.append(frame);
    }
    delete pBuffer;
LogErrorF("decodeFrames done");
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
LogErrorF("dcc directions %d.", hi.dirOffsets.size());
    // BUILDING {CEL FRAMES}
    for (unsigned d = 0; d < hi.dirOffsets.size(); d++) {
LogErrorF("decoding direction %d.", d + 1);
        const auto &offset = hi.dirOffsets[d];
        if (fileSize >= offset.second && offset.second >= offset.first) {
            device->seek(offset.first);
            QByteArray ccDirRawData = device->read(offset.second - offset.first);

            D2CcBitReaderState rs = { };
            rs.rawStreamData = &ccDirRawData;
            D2CcDirectionInfo cdi;
            if (readDirHeader(rs, hi, cdi) && decodeFrames(hi, cdi, gfx)) {
                /*for (unsigned i = 0; i < hi.framesPerDirection; i++) {
                    D1GfxFrame *frame = new D1GfxFrame();

                    if (!D2CcFrame::load(*frame, ccDirRawData, params)) {
                        dProgressErr() << QApplication::tr("Frame %1 of direction %2 is invalid.").arg(i + 1).arg(d + 1);
                    }
                    gfx.frames.append(frame);
                }*/


                continue;
            }
        } else {
            dProgressErr() << QApplication::tr("Address of Direction %1 is invalid (%2-%3 of %4).").arg(d + 1).arg(offset.first).arg(offset.second).arg(fileSize);
        }
        for (unsigned i = 0; i < hi.framesPerDirection; i++) {
            gfx.frames.append(new D1GfxFrame());
        }
    }
LogErrorF("decoding done.");
    gfx.clipped = false;

    gfx.gfxFilePath = filePath;
    gfx.modified = false;
    return true;
}
