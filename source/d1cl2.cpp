#include "d1cl2.h"

#include <vector>

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>

#include "d1cl2frame.h"
#include "progressdialog.h"

bool D1Cl2::load(D1Gfx &gfx, const QString &filePath, const OpenAsParam &params)
{
    gfx.clear();

    // Opening CL2 file and load it in RAM
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QByteArray fileData = file.readAll();

    // Read CL2 binary data
    QDataStream in(fileData);
    in.setByteOrder(QDataStream::LittleEndian);

    QIODevice *device = in.device();
    auto fileSize = device->size();
    // CL2 HEADER CHECKS
    if (fileSize < 4)
        return false;

    // Read first DWORD
    quint32 firstDword;
    in >> firstDword;

    // Trying to find file size in CL2 header
    if (fileSize < (4 + firstDword * 4 + 4))
        return false;

    device->seek(4 + firstDword * 4);
    quint32 fileSizeDword;
    in >> fileSizeDword;

    // If the dword is not equal to the file size then
    // check if it's a CL2 with multiple groups
    D1CEL_TYPE type = fileSize == fileSizeDword ? D1CEL_TYPE::V2_MONO_GROUP : D1CEL_TYPE::V2_MULTIPLE_GROUPS;
    if (type == D1CEL_TYPE::V2_MULTIPLE_GROUPS) {
        // Read offset of the last CL2 group header
        device->seek(firstDword - 4);
        quint32 lastCl2GroupHeaderOffset;
        in >> lastCl2GroupHeaderOffset;

        // Read the number of frames of the last CL2 group
        if (fileSize < (lastCl2GroupHeaderOffset + 4))
            return false;

        device->seek(lastCl2GroupHeaderOffset);
        quint32 lastCl2GroupFrameCount;
        in >> lastCl2GroupFrameCount;

        // Read the last frame offset corresponding to the file size
        if (fileSize < (lastCl2GroupHeaderOffset + lastCl2GroupFrameCount * 4 + 4 + 4))
            return false;

        device->seek(lastCl2GroupHeaderOffset + lastCl2GroupFrameCount * 4 + 4);
        in >> fileSizeDword;
        // The offset is from the beginning of the last group header
        // so we need to add the offset of the lasr group header
        // to have an offset from the beginning of the file
        fileSizeDword += lastCl2GroupHeaderOffset;

        if (fileSize != fileSizeDword) {
            return false;
        }
    }

    gfx.type = type;

    // CL2 FRAMES OFFSETS CALCULATION
    std::vector<std::pair<quint32, quint32>> frameOffsets;
    if (gfx.type == D1CEL_TYPE::V2_MONO_GROUP) {
        // Going through all frames of the only group
        if (firstDword > 0) {
            gfx.groupFrameIndices.push_back(std::pair<int, int>(0, firstDword - 1));
        }
        for (unsigned i = 1; i <= firstDword; i++) {
            device->seek(i * 4);
            quint32 cl2FrameStartOffset;
            in >> cl2FrameStartOffset;
            quint32 cl2FrameEndOffset;
            in >> cl2FrameEndOffset;

            frameOffsets.push_back(
                std::pair<quint32, quint32>(cl2FrameStartOffset, cl2FrameEndOffset));
        }
    } else {
        // Going through all groups
        int cursor = 0;
        for (unsigned i = 0; i * 4 < firstDword; i++) {
            device->seek(i * 4);
            quint32 cl2GroupOffset;
            in >> cl2GroupOffset;

            if (fileSize < (cl2GroupOffset + 4))
                return false;

            device->seek(cl2GroupOffset);
            quint32 cl2GroupFrameCount;
            in >> cl2GroupFrameCount;

            if (cl2GroupFrameCount == 0) {
                continue;
            }
            if (fileSize < (cl2GroupOffset + cl2GroupFrameCount * 4 + 4 + 4))
                return false;

            gfx.groupFrameIndices.push_back(std::pair<int, int>(cursor, cursor + cl2GroupFrameCount - 1));

            // Going through all frames of the group
            for (unsigned j = 1; j <= cl2GroupFrameCount; j++) {
                quint32 cl2FrameStartOffset;
                quint32 cl2FrameEndOffset;

                device->seek(cl2GroupOffset + j * 4);
                in >> cl2FrameStartOffset;
                in >> cl2FrameEndOffset;

                frameOffsets.push_back(
                    std::pair<quint32, quint32>(cl2GroupOffset + cl2FrameStartOffset,
                        cl2GroupOffset + cl2FrameEndOffset));
            }
            cursor += cl2GroupFrameCount;
        }
    }

    // BUILDING {CL2 FRAMES}
    // std::stack<quint16> invalidFrames;
    int clipped = -1;
    unsigned rle_len_min = 0;
    unsigned rle_len_max = UINT_MAX;
    for (const auto &offset : frameOffsets) {
        device->seek(offset.first);
        QByteArray cl2FrameRawData = device->read(offset.second - offset.first);

        D1GfxFrame *frame = new D1GfxFrame();
        unsigned rle_min = 0;
        unsigned rle_max = UINT_MAX;
        int res = D1Cl2Frame::load(*frame, cl2FrameRawData, params, &rle_min, &rle_max);
        quint16 frameIndex = gfx.frames.size();
        if (res < 0) {
            if (res == -1)
                dProgressErr() << QApplication::tr("Could not determine the width of Frame %1.").arg(frameIndex + 1);
            else
                dProgressErr() << QApplication::tr("Frame %1 is invalid.").arg(frameIndex + 1);
            // invalidFrames.push(frameIndex);
        } else {
            if (clipped != res) {
                if (clipped == -1)
                    clipped = res;
                else
                    dProgressErr() << QApplication::tr("Inconsistent clipping (Frame %1 is %2).").arg(frameIndex + 1).arg(D1Gfx::clippedtoStr(res != 0));
            }
            if (rle_min > rle_len_min) {
                if (rle_min <= rle_len_max) {
                    rle_len_min = rle_min;
                } else {
                    dProgressErr() << QApplication::tr("Inconsistent rle-encoding (Frame %1 is %2..%3 while the preceeding frames are %4..%5).").arg(frameIndex + 1)
                        .arg(rle_min).arg(rle_max).arg(rle_len_min).arg(rle_len_max);
                }
            }
            if (rle_max < rle_len_max) {
                if (rle_max >= rle_len_min) {
                    rle_len_max = rle_max;
                } else {
                    dProgressErr() << QApplication::tr("Inconsistent rle-encoding (Frame %1 is %2..%3 while the preceeding frames are %4..%5).").arg(frameIndex + 1)
                        .arg(rle_min).arg(rle_max).arg(rle_len_min).arg(rle_len_max);
                }
            }
        }
        gfx.frames.append(frame);
    }
    gfx.clipped = clipped != 0;
    unsigned rle_len = rle_len_min;
    if (rle_len == 0) {
        rle_len = rle_len_max;
        if (rle_len == UINT_MAX)
            rle_len = 0;
    }
    gfx.rle_len = rle_len;

    gfx.gfxFilePath = filePath;
    gfx.modified = false;
    /*while (!invalidFrames.empty()) {
        quint16 frameIndex = invalidFrames.top();
        invalidFrames.pop();
        gfx.removeFrame(frameIndex);
    }*/
    return true;
}

static void pushHead(quint8 **prevHead, quint8 **lastHead, quint8 *head)
{
    if (*lastHead != nullptr && *prevHead != nullptr && head != nullptr) {
        if (**lastHead == 0xBF - 3 && *head >= 0xBF && **prevHead >= 0xBF) {
            unsigned len = 3 + (256 - *head) + (256 - **prevHead);
            if (len <= (256 - 0xBF)) {
                **prevHead = 256 - len;
                quint8 col = *((*lastHead) + 1);
                **lastHead = col;
                *head = col;
                *lastHead = *prevHead;
                *prevHead = nullptr;
                return;
            }
        }
    }

    *prevHead = *lastHead;
    *lastHead = head;
}

static quint8 *writeFrameData(const D1GfxFrame *frame, quint8 *pBuf, int subHeaderSize, unsigned RLE_LEN, bool clipped)
{
    // convert one image to cl2-data
    quint8 *pHeader = pBuf;
    if (clipped) {
        // add CL2 FRAME HEADER
        *(quint16 *)&pBuf[0] = SwapLE16(subHeaderSize); // SUB_HEADER_SIZE
        //*(quint32 *)&pBuf[2] = 0;
        //*(quint32 *)&pBuf[6] = 0;
        pBuf += subHeaderSize;
    }

    quint8 *pHead = pBuf;
    quint8 col, lastCol;
    quint8 colMatches = 0; // does not matter
    //int mode = 0; // -1;
    bool alpha = false;
    bool first = false; // true; - does not matter
    quint8 *pPrevHead = nullptr;
    quint8 *pLastHead = nullptr;
    for (int i = 1; i <= frame->getHeight(); i++) {
        if (clipped && (i % CEL_BLOCK_HEIGHT) == 1 /*&& (i / CEL_BLOCK_HEIGHT) * 2 < SUB_HEADER_SIZE*/) {
            pushHead(&pPrevHead, &pLastHead, /*first ? nullptr :*/ pHead);
            pHead = pBuf;
            *(quint16 *)(&pHeader[(i / CEL_BLOCK_HEIGHT) * 2]) = SwapLE16(pHead - pHeader); // pHead - buf - SUB_HEADER_SIZE;

            //colMatches = 0;
            alpha = false;
            // first = true;
            // if (mode == 2)
            //mode = -1;
            pLastHead = nullptr;
        }
        first = true;
        //if (mode != 2) {
        //    mode = -1;
        //}
        //colMatches = 0;
        for (int j = 0; j < frame->getWidth(); j++) {
            D1GfxPixel pixel = frame->getPixel(j, frame->getHeight() - i);
            if (!pixel.isTransparent()) {
                // add opaque pixel
                col = pixel.getPaletteIndex();
                if (alpha || first || col != lastCol)
                //if ((mode != 0 && mode != 1) || col != lastCol)
                    colMatches = 1;
                else
                    colMatches++;
                if (colMatches < RLE_LEN || *pHead == 0x80u) {
                // if (colMatches < RLE_LEN || (/*mode == 1 && */*pHead == 0x80u)) {
                    // bmp encoding
                    if (/*alpha ||*/ *pHead <= 0xBFu || first) {
                    //if (mode != 0 || *pHead == 0xBF) {
                    //    mode = 0;
                        pushHead(&pPrevHead, &pLastHead, /*first ? nullptr :*/ pHead);
                        pHead = pBuf;
                        pBuf++;
                        colMatches = 1;
                    }
                    *pBuf = col;
                    pBuf++;
                } else {
                    // RLE encoding
                    if (colMatches == RLE_LEN) {
                    //if (mode != 1) {
                    //    mode = 1;
                        memset(pBuf - (RLE_LEN - 1), 0, RLE_LEN - 1);
                        *pHead += RLE_LEN - 1;
                        if (*pHead != 0) {
                            pushHead(&pPrevHead, &pLastHead, /*first ? nullptr : */pHead);
                            pHead = pBuf - (RLE_LEN - 1);
                        }
                        *pHead = 0xBFu - (RLE_LEN - 1);
                        pBuf = pHead + 1;
                        *pBuf = col;
                        pBuf++;
                    }
                }
                --*pHead;

                lastCol = col;
                alpha = false;
            } else {
                // add transparent pixel
                if (!alpha || *pHead == 0x7Fu) {
                // if (mode != 2 || *pHead == 0x7Fu) {
                //    mode = 2;
                    pushHead(&pPrevHead, &pLastHead, /*first ? nullptr :*/ pHead);
                    pHead = pBuf;
                    pBuf++;
                }
                ++*pHead;
                alpha = true;
            }
            first = false;
        }
    }
    pushHead(&pPrevHead, &pLastHead, pHead);
    return pBuf;
}

bool D1Cl2::writeFileData(D1Gfx &gfx, QFile &outFile, const SaveAsParam &params)
{
    // calculate header size
    bool groupped = false;
    int numGroups = params.groupNum;
    int headerSize = 0;
    if (numGroups == 0) {
        numGroups = gfx.getGroupCount();
        groupped = numGroups > 1;
        for (int i = 0; i < numGroups; i++) {
            std::pair<int, int> gfi = gfx.getGroupFrameIndices(i);
            int ni = gfi.second - gfi.first + 1;
            headerSize += 4 + 4 * (ni + 1);
        }
    } else {
        // update group indices
        const int numFrames = gfx.frames.count();
        if (numFrames == 0 || (numFrames % numGroups) != 0) {
            dProgressFail() << QApplication::tr("Frames can not be split to equal groups.");
            return false;
        }
        groupped = true;
        gfx.groupFrameIndices.clear();
        for (int i = 0; i < numGroups; i++) {
            int ni = numFrames / numGroups;
            gfx.groupFrameIndices.push_back(std::pair<int, int>(i * ni, i * ni + ni - 1));
            headerSize += 4 + 4 * (ni + 1);
        }
    }
    if (groupped) {
        headerSize += sizeof(quint32) * numGroups;
    }
    // update type
    gfx.type = groupped ? D1CEL_TYPE::V2_MULTIPLE_GROUPS : D1CEL_TYPE::V2_MONO_GROUP;
    // update clipped info
    bool clipped = params.clipped == SAVE_CLIPPED_TYPE::TRUE || (params.clipped == SAVE_CLIPPED_TYPE::AUTODETECT && gfx.clipped);
    gfx.clipped = clipped;
    // update rle length
    unsigned rle_len = params.rle_len == 0 ? gfx.rle_len : params.rle_len;
    if (rle_len == 0) {
        rle_len = 3; // use 'default' rle
    } else if (rle_len == 1) {
        rle_len = UINT_MAX; // turn off rle
    }
    gfx.rle_len = rle_len;

    // calculate sub header size
    int subHeaderSize = SUB_HEADER_SIZE;
    for (D1GfxFrame *frame : gfx.frames) {
        if (clipped) {
            int hs = (frame->getHeight() - 1) / CEL_BLOCK_HEIGHT;
            hs = (hs + 1) * sizeof(quint16);
            subHeaderSize = std::max(subHeaderSize, hs);
        }
    }
    // estimate data size
    int maxSize = headerSize;
    for (D1GfxFrame *frame : gfx.frames) {
        if (clipped) {
            maxSize += subHeaderSize; // SUB_HEADER_SIZE
        }
        maxSize += frame->getHeight() * (2 * frame->getWidth());
    }

    QByteArray fileData;
    fileData.append(maxSize, 0);

    quint8 *buf = (quint8 *)fileData.data();
    quint8 *hdr = buf;
    if (groupped) {
        // add optional {CL2 GROUP HEADER}
        int offset = numGroups * 4;
        for (int i = 0; i < numGroups; i++, hdr += 4) {
            *(quint32 *)&hdr[0] = offset;
            std::pair<int, int> gfi = gfx.getGroupFrameIndices(i);
            int ni = gfi.second - gfi.first + 1;
            offset += 4 + 4 * (ni + 1);
        }
    }

    quint8 *pBuf = &buf[headerSize];
    int idx = 0;
    for (int ii = 0; ii < numGroups; ii++) {
        std::pair<int, int> gfi = gfx.getGroupFrameIndices(ii);
        int ni = gfi.second - gfi.first + 1;
        *(quint32 *)&hdr[0] = SwapLE32(ni);
        *(quint32 *)&hdr[4] = SwapLE32(pBuf - hdr);

        for (int n = 0; n < ni; n++, idx++) {
            D1GfxFrame *frame = gfx.getFrame(idx); // TODO: what if the groups are not continuous?
            pBuf = writeFrameData(frame, pBuf, subHeaderSize, rle_len, clipped);
            *(quint32 *)&hdr[4 + 4 * (n + 1)] = SwapLE32(pBuf - hdr);
        }
        hdr += 4 + 4 * (ni + 1);
    }

    // write to file
    QDataStream out(&outFile);
    out.writeRawData((char *)buf, pBuf - buf);

    return true;
}

bool D1Cl2::save(D1Gfx &gfx, const SaveAsParam &params)
{
    QString filePath = gfx.gfxFilePath;
    if (!params.celFilePath.isEmpty()) {
        filePath = params.celFilePath;
        if (!params.autoOverwrite && QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, QApplication::tr("Confirmation"), QApplication::tr("Are you sure you want to overwrite %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    } else if (!gfx.isModified()) {
        return false;
    }

    if (filePath.isEmpty()) {
        return false;
    }

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    bool result = D1Cl2::writeFileData(gfx, outFile, params);

    if (result) {
        gfx.gfxFilePath = filePath; // D1Cl2::load(gfx, filePath);
        gfx.modified = false;
    }
    return result;
}
