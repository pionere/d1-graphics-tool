#include "d1cl2.h"

#include <vector>

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>

#include "d1cel.h"
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
    CelHeaderInfo hi;
    if (!D1Cel::readHeader(device, in, hi, gfx)) {
        return false;
    }
    gfx.type = hi.groupped ? D1CEL_TYPE::V2_MULTIPLE_GROUPS : D1CEL_TYPE::V2_MONO_GROUP;
    auto fileSize = device->size();

    // BUILDING {CL2 FRAMES}
    int clipped = -1;
    for (unsigned i = 0; i < hi.frameOffsets.size(); i++) {
        const auto &offset = hi.frameOffsets[i];
        D1GfxFrame *frame = new D1GfxFrame();
        if (fileSize >= offset.second && offset.second >= offset.first) {
            device->seek(offset.first);
            QByteArray cl2FrameRawData = device->read(offset.second - offset.first);

            int res = D1Cl2Frame::load(*frame, cl2FrameRawData, params);
            if (res < 0) {
                if (res == -1)
                    dProgressErr() << QApplication::tr("Could not determine the width of Frame %1.").arg(i + 1);
                else
                    dProgressErr() << QApplication::tr("Frame %1 is invalid.").arg(i + 1);
            } else if (clipped != res) {
                if (clipped == -1)
                    clipped = res;
                else
                    dProgressErr() << QApplication::tr("Inconsistent clipping (Frame %1 is %2).").arg(i + 1).arg(D1Gfx::clippedtoStr(res != 0));
            }
        } else {
            dProgressErr() << QApplication::tr("Address of Frame %1 is invalid (%2-%3 of %4).").arg(i + 1).arg(offset.first).arg(offset.second).arg(fileSize);
        }
        gfx.frames.append(frame);
    }
    gfx.clipped = clipped != 0;

    gfx.gfxFilePath = filePath;
    gfx.modified = false;
    return true;
}

static void pushHead(quint8 **prevHead, quint8 **lastHead, quint8 *head)
{
    if (*lastHead != nullptr && *prevHead != nullptr && head != nullptr) {
        // check for [len0 col0 .. coln] [rle3 col] [len1 col00 .. colnn] -> [(len0 + 3 + len1) col0 .. coln col col col col00 .. colnn]
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

static quint8 *writeFrameData(const D1GfxFrame *frame, quint8 *pBuf, int subHeaderSize, bool clipped)
{
    const int RLE_LEN = 3; // number of matching colors to switch from bmp encoding to RLE

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
    bool alpha = false;
    bool first = false; // true; - does not matter
    quint8 *pPrevHead = nullptr;
    quint8 *pLastHead = nullptr;
    const int width = frame->getWidth();
    const int height = frame->getHeight();
    for (int i = 1; i <= height; i++) {
        if (clipped && (i % CEL_BLOCK_HEIGHT) == 1 /*&& (i / CEL_BLOCK_HEIGHT) * 2 < SUB_HEADER_SIZE*/) {
            pushHead(&pPrevHead, &pLastHead, pHead);
            //if (first) {
                pLastHead = nullptr;
            //}

            pHead = pBuf;
            *(quint16 *)(&pHeader[(i / CEL_BLOCK_HEIGHT) * 2]) = SwapLE16(pHead - pHeader); // pHead - buf - SUB_HEADER_SIZE;

            //colMatches = 0;
            alpha = false;
            // first = true;
        }
        first = true;
        for (int j = 0; j < width; j++) {
            const D1GfxPixel pixel = frame->getPixel(j, height - i);
            if (!pixel.isTransparent()) {
                // add opaque pixel
                col = pixel.getPaletteIndex();
                if (alpha || first || col != lastCol)
                    colMatches = 1;
                else
                    colMatches++;
                if (colMatches < RLE_LEN || *pHead == 0x81u) {
                    // bmp encoding
                    if (/*alpha ||*/ *pHead <= 0xBFu || first) {
                        pushHead(&pPrevHead, &pLastHead, pHead);
                        if (first) {
                            pLastHead = nullptr;
                        }
                        pHead = pBuf;
                        pBuf++;
                        colMatches = 1;
                    }
                    *pBuf = col;
                    pBuf++;
                } else {
                    // RLE encoding
                    if (colMatches == RLE_LEN) {
                        memset(pBuf - (RLE_LEN - 1), 0, RLE_LEN - 1);
                        *pHead += RLE_LEN - 1;
                        if (*pHead != 0) {
                            pushHead(&pPrevHead, &pLastHead, pHead);
                            //if (first) {
                            //    pLastHead = nullptr;
                            //}
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
                    pushHead(&pPrevHead, &pLastHead, pHead);
                    //if (first) {
                    //    pLastHead = nullptr;
                    //}
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
    // add an extra header entry to ensure the width of the frame can be determined using the header
    if (clipped && height == CEL_BLOCK_HEIGHT) {
        int i = height + 1;
        pHead = pBuf;
        *(quint16 *)(&pHeader[(i / CEL_BLOCK_HEIGHT) * 2]) = SwapLE16(pHead - pHeader); // pHead - buf - SUB_HEADER_SIZE;
    }
    return pBuf;
}

bool D1Cl2::writeFileData(D1Gfx &gfx, QFile &outFile, const SaveAsParam &params)
{
    // calculate header size
    int numGroups = params.groupNum;
    int headerSize = 0;
    if (numGroups == 0) {
        numGroups = gfx.getGroupCount();
        for (int i = 0; i < numGroups; i++) {
            std::pair<int, int> gfi = gfx.groupFrameIndices[i];
            int ni = gfi.second - gfi.first + 1;
            headerSize += 4 + 4 * (ni + 1);
        }
    } else {
        // update group indices
        const int numFrames = gfx.frames.count();
        if ((numFrames % numGroups) != 0) {
            dProgressFail() << QApplication::tr("Frames can not be split to equal groups.");
            return false;
        }
        if (numFrames == 0) {
            if (numGroups != 1) {
                dProgressFail() << QApplication::tr("Frames can not be split to equal groups.");
                return false;
            }
            numGroups = 0;
        }
        gfx.groupFrameIndices.clear();
        for (int i = 0; i < numGroups; i++) {
            int ni = numFrames / numGroups;
            gfx.groupFrameIndices.push_back(std::pair<int, int>(i * ni, i * ni + ni - 1));
            headerSize += 4 + 4 * (ni + 1);
        }
    }
    if (numGroups == 0) {
        headerSize = 4 + 4;
    }
    // if (numGroups > 1) {
        headerSize += sizeof(quint32) * numGroups;
    // }
    // update type
    gfx.type = numGroups > 1 ? D1CEL_TYPE::V2_MULTIPLE_GROUPS : D1CEL_TYPE::V2_MONO_GROUP;
    // update clipped info
    bool clipped = params.clipped == SAVE_CLIPPED_TYPE::TRUE || (params.clipped == SAVE_CLIPPED_TYPE::AUTODETECT && gfx.clipped);
    gfx.clipped = clipped;

    // calculate the meta info size
    CelMetaInfo meta;
    int metaSize = D1Cel::prepareCelMeta(gfx, meta);

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
    int maxSize = headerSize + metaSize;
    for (D1GfxFrame *frame : gfx.frames) {
        if (clipped) {
            maxSize += subHeaderSize; // SUB_HEADER_SIZE
        }
        maxSize += frame->getHeight() * (2 * frame->getWidth());
    }

    QByteArray fileData;
    fileData.append(maxSize, 0);

    quint8 *buf = (quint8 *)fileData.data();
    quint8 *pBuf;
    // write meta
    { // write the metadata
    pBuf = &buf[numGroups > 1 ? numGroups * sizeof(quint32) : headerSize];
    pBuf = D1Cel::writeCelMeta(meta, gfx, pBuf);
    }
    quint8 *hdr = buf;
    if (numGroups > 1) {
        // add optional {CL2 GROUP HEADER}
        int offset = numGroups * sizeof(quint32) + metaSize;
        for (int i = 0; i < numGroups; i++, hdr += 4) {
            *(quint32 *)&hdr[0] = offset;
            std::pair<int, int> gfi = gfx.groupFrameIndices[i];
            int ni = gfi.second - gfi.first + 1;
            offset += 4 + 4 * (ni + 1);
        }
        hdr += metaSize;
    }

    pBuf = &buf[headerSize + metaSize];
    int idx = 0;
    for (int ii = 0; ii < numGroups; ii++) {
        std::pair<int, int> gfi = gfx.groupFrameIndices[ii];
        int ni = gfi.second - gfi.first + 1;
        *(quint32 *)&hdr[0] = SwapLE32(ni);
        *(quint32 *)&hdr[4] = SwapLE32(pBuf - hdr);

        for (int n = 0; n < ni; n++, idx++) {
            D1GfxFrame *frame = gfx.getFrame(idx); // TODO: what if the groups are not continuous?
            pBuf = writeFrameData(frame, pBuf, subHeaderSize, clipped);
            *(quint32 *)&hdr[4 + 4 * (n + 1)] = SwapLE32(pBuf - hdr);
        }
        hdr += 4 + 4 * (ni + 1);
    }
    if (numGroups == 0) {
        // *(quint32 *)&buf[0] = SwapLE32(0);
        *(quint32 *)&buf[4] = SwapLE32(pBuf - buf);
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
