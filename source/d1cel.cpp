#include "d1cel.h"

#include <vector>

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>

#include "d1celframe.h"
#include "progressdialog.h"

bool D1Cel::load(D1Gfx &gfx, const QString &filePath, const OpenAsParam &params)
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
    auto fileSize = device->size();
    // CEL HEADER CHECKS
    // Read first DWORD
    if (fileSize < 4)
        return false;

    quint32 firstDword;
    in >> firstDword;

    // Trying to find file size in CEL header
    if (fileSize < (4 + firstDword * 4 + 4))
        return false;

    device->seek(4 + firstDword * 4);
    quint32 fileSizeDword;
    in >> fileSizeDword;

    // If the dword is not equal to the file size then
    // check if it's a CEL compilation
    D1CEL_TYPE type = fileSize == fileSizeDword ? D1CEL_TYPE::V1_REGULAR : D1CEL_TYPE::V1_COMPILATION;
    std::vector<std::pair<quint32, quint32>> frameOffsets;
    if (type == D1CEL_TYPE::V1_REGULAR) {
        // Going through all frames of the CEL
        if (firstDword > 0) {
            gfx.groupFrameIndices.push_back(std::pair<int, int>(0, firstDword - 1));
        }
        for (unsigned int i = 1; i <= firstDword; i++) {
            device->seek(i * 4);
            quint32 celFrameStartOffset;
            in >> celFrameStartOffset;
            quint32 celFrameEndOffset;
            in >> celFrameEndOffset;

            frameOffsets.push_back(std::pair<quint32, quint32>(celFrameStartOffset, celFrameEndOffset));
        }
    } else {
#if 0
        // Read offset of the last CEL of the CEL compilation
        device->seek(firstDword - 4);
        quint32 lastCelOffset;
        in >> lastCelOffset;

        // Go to last CEL of the CEL compilation
        // Read last CEL header
        if (fileSize < (lastCelOffset + 4))
            return false;

        device->seek(lastCelOffset);
        quint32 lastCelFrameCount;
        in >> lastCelFrameCount;

        // Read the last CEL size
        if (fileSize < (lastCelOffset + 4 + lastCelFrameCount * 4 + 4))
            return false;

        device->seek(lastCelOffset + 4 + lastCelFrameCount * 4);
        quint32 lastCelSize;
        in >> lastCelSize;

        // If the last CEL size plus the last CEL offset is equal to
        // the file size then it's a CEL compilation
        if (fileSize != (lastCelOffset + lastCelSize)) {
            return false;
        }
#endif
        // Going through all groups
        int cursor = 0;
        for (unsigned int i = 0; i * 4 < firstDword; i++) {
            device->seek(i * 4);
            quint32 celOffset;
            in >> celOffset;

            if (fileSize < (celOffset + 4))
                return false;

            device->seek(celOffset);
            quint32 celFrameCount;
            in >> celFrameCount;

            if (celFrameCount == 0) {
                continue;
            }
            if (fileSize < (celOffset + celFrameCount * 4 + 4 + 4))
                return false;

            gfx.groupFrameIndices.push_back(std::pair<int, int>(cursor, cursor + celFrameCount - 1));

            // Going through all frames of the CEL
            for (unsigned int j = 1; j <= celFrameCount; j++) {
                quint32 celFrameStartOffset;
                quint32 celFrameEndOffset;

                device->seek(celOffset + j * 4);
                in >> celFrameStartOffset;
                in >> celFrameEndOffset;

                frameOffsets.push_back(
                    std::pair<quint32, quint32>(celOffset + celFrameStartOffset,
                        celOffset + celFrameEndOffset));
            }
            cursor += celFrameCount;
        }

        if (!frameOffsets.empty() && frameOffsets.back().second != fileSize) {
            dProgressWarn() << QApplication::tr("Failed to read the complete CEL. Finished at %1 while filesize is %2.").arg(frameOffsets.back().second).arg(fileSize));
        } else {
dProgressErr() << "CEL compilation read";
        }
    }

    gfx.type = type;

    // CEL FRAMES OFFSETS CALCULATION

    // BUILDING {CEL FRAMES}
    // std::stack<quint16> invalidFrames;
    int clipped = -1;
    for (const auto &offset : frameOffsets) {
        device->seek(offset.first);
        QByteArray celFrameRawData = device->read(offset.second - offset.first);

        D1GfxFrame *frame = new D1GfxFrame();
        int res = D1CelFrame::load(*frame, celFrameRawData, params);
        quint16 frameIndex = gfx.frames.size();
        if (res < 0) {
            if (res == -1)
                dProgressErr() << QApplication::tr("Could not determine the width of Frame %1.").arg(frameIndex + 1);
            else
                dProgressErr() << QApplication::tr("Frame %1 is invalid.").arg(frameIndex + 1);
            // invalidFrames.push(frameIndex);
        } else if (clipped != res) {
            if (clipped == -1)
                clipped = res;
            else
                dProgressErr() << QApplication::tr("Inconsistent clipping (Frame %1 is %2).").arg(frameIndex + 1).arg(res == 0 ? QApplication::tr("not clipped") : QApplication::tr("clipped"));
        }
        gfx.frames.append(frame);
    }
    gfx.clipped = clipped == 1;

    gfx.gfxFilePath = filePath;
    gfx.modified = false;
    /*while (!invalidFrames.empty()) {
        quint16 frameIndex = invalidFrames.top();
        invalidFrames.pop();
        gfx.removeFrame(frameIndex);
    }*/
    return true;
}

// assumes a zero filled pBuf-buffer
static quint8 *writeFrameData(D1GfxFrame *frame, quint8 *pBuf, int subHeaderSize, bool clipped)
{
    // add optional {CEL FRAME HEADER}
    quint8 *pHeader = pBuf;
    if (clipped) {
        *(quint16 *)&pBuf[0] = SwapLE16(subHeaderSize); // SUB_HEADER_SIZE
        pBuf += subHeaderSize;
    }
    // convert to cel
    quint8 *pHead;
    for (int i = 1; i <= frame->getHeight(); i++) {
        pHead = pBuf;
        pBuf++;
        bool alpha = false;
        if (clipped && (i % CEL_BLOCK_HEIGHT) == 1 /*&& (i / CEL_BLOCK_HEIGHT) * 2 < SUB_HEADER_SIZE*/) {
            *(quint16 *)(&pHeader[(i / CEL_BLOCK_HEIGHT) * 2]) = SwapLE16(pHead - pHeader); // pHead - buf - SUB_HEADER_SIZE;
        }
        for (int j = 0; j < frame->getWidth(); j++) {
            D1GfxPixel pixel = frame->getPixel(j, frame->getHeight() - i);
            if (!pixel.isTransparent()) {
                // add opaque pixel
                if (alpha || *pHead > 126) {
                    pHead = pBuf;
                    pBuf++;
                }
                ++*pHead;
                *pBuf = pixel.getPaletteIndex();
                pBuf++;
                alpha = false;
            } else {
                // add transparent pixel
                if (j != 0 && (!alpha || (char)*pHead == -128)) {
                    pHead = pBuf;
                    pBuf++;
                }
                --*pHead;
                alpha = true;
            }
        }
    }
    return pBuf;
}

bool D1Cel::writeFileData(D1Gfx &gfx, QFile &outFile, const SaveAsParam &params)
{
    const int numFrames = gfx.frames.count();

    // update type
    gfx.type = D1CEL_TYPE::V1_REGULAR;
    // update clipped info
    bool clipped = params.clipped == SAVE_CLIPPED_TYPE::TRUE || (params.clipped == SAVE_CLIPPED_TYPE::AUTODETECT && gfx.clipped);
    gfx.clipped = clipped;

    // calculate header size
    int HEADER_SIZE = 4 + 4 + numFrames * 4;
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
    int maxSize = HEADER_SIZE;
    for (D1GfxFrame *frame : gfx.frames) {
        if (clipped) {
            maxSize += subHeaderSize; // SUB_HEADER_SIZE
        }
        maxSize += frame->getHeight() * (2 * frame->getWidth());
    }

    QByteArray fileData;
    fileData.append(maxSize, 0);

    quint8 *buf = (quint8 *)fileData.data();
    *(quint32 *)&buf[0] = SwapLE32(numFrames);
    *(quint32 *)&buf[4] = SwapLE32(HEADER_SIZE);
    quint8 *pBuf = &buf[HEADER_SIZE];
    for (int n = 0; n < numFrames; n++) {
        D1GfxFrame *frame = gfx.getFrame(n);
        pBuf = writeFrameData(frame, pBuf, subHeaderSize, clipped);
        *(quint32 *)&buf[4 + 4 * (n + 1)] = SwapLE32(pBuf - buf);
    }

    // write to file
    QDataStream out(&outFile);
    out.writeRawData((char *)buf, pBuf - buf);

    return true;
}

bool D1Cel::writeCompFileData(D1Gfx &gfx, QFile &outFile, const SaveAsParam &params)
{
    // calculate header size
    int numGroups = params.groupNum;
    int headerSize = 0;
    if (numGroups == 0) {
        numGroups = gfx.getGroupCount();
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
        gfx.groupFrameIndices.clear();
        for (int i = 0; i < numGroups; i++) {
            int ni = numFrames / numGroups;
            gfx.groupFrameIndices.push_back(std::pair<int, int>(i * ni, i * ni + ni - 1));
            headerSize += 4 + 4 * (ni + 1);
        }
    }

    // if (numGroups > 1) {
    headerSize += sizeof(quint32) * numGroups;
    // }

    // update type
    gfx.type = D1CEL_TYPE::V1_COMPILATION;
    // update clipped info
    bool clipped = params.clipped == SAVE_CLIPPED_TYPE::TRUE || (params.clipped == SAVE_CLIPPED_TYPE::AUTODETECT && gfx.clipped);
    gfx.clipped = clipped;

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
    quint8 *pBuf = &buf[sizeof(quint32) * numGroups];
    int idx = 0;
    for (int ii = 0; ii < numGroups; ii++) {
        std::pair<int, int> gfi = gfx.getGroupFrameIndices(ii);
        int ni = gfi.second - gfi.first + 1;
        *(quint32 *)&buf[ii * sizeof(quint32)] = SwapLE32(pBuf - buf);

        quint8 *hdr = pBuf;
        *(quint32 *)&hdr[0] = SwapLE32(ni);
        *(quint32 *)&hdr[4] = SwapLE32(4 + 4 * (ni + 1));

        pBuf += 4 + 4 * (ni + 1);
        for (int n = 0; n < ni; n++, idx++) {
            D1GfxFrame *frame = gfx.getFrame(idx); // TODO: what if the groups are not continuous?
            pBuf = writeFrameData(frame, pBuf, subHeaderSize, clipped);
            *(quint32 *)&hdr[4 + 4 * (n + 1)] = SwapLE32(pBuf - hdr);
        }
    }
    // write to file
    QDataStream out(&outFile);
    out.writeRawData((char *)buf, pBuf - buf);

    return true;
}

bool D1Cel::save(D1Gfx &gfx, const SaveAsParam &params)
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

    D1CEL_TYPE type;
    if (params.groupNum == 0) {
        type = gfx.type;
    } else {
        type = params.groupNum > 1 ? D1CEL_TYPE::V1_COMPILATION : D1CEL_TYPE::V1_REGULAR;
    }

    bool result;
    if (type == D1CEL_TYPE::V1_COMPILATION) {
        result = D1Cel::writeCompFileData(gfx, outFile, params);
    } else {
        result = D1Cel::writeFileData(gfx, outFile, params);
    }

    if (result) {
        gfx.gfxFilePath = filePath; //  D1Cel::load(gfx, filePath);
        gfx.modified = false;
    }
    return result;
}
