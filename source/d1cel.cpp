#include "d1cel.h"

#include <vector>

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>

#include "d1celframe.h"
#include "progressdialog.h"

bool D1Cel::readMeta(QIODevice *device, QDataStream &in, quint32 startOffset, quint32 endOffset, unsigned frameCount, D1Gfx &gfx)
{
    device->seek(startOffset);
    while (startOffset < endOffset) {
        startOffset++;

        quint8 type;
        in >> type;

        D1GfxMeta *meta = gfx.getMeta(type);
        switch (type) {
        case CELMETA_DIMENSIONS: {
            if (endOffset < startOffset + 8) {
                dProgressErr() << QApplication::tr("Not enough dimensions in the meta info.");
                return false;
            }
            startOffset += 8;

            quint32 width;
            quint32 height;
            in >> width;
            in >> height;

            meta->setWidth(width);
            meta->setHeight(height);
        } break;
        case CELMETA_DIMENSIONS_PER_FRAME: {
            for (unsigned idx = 0; idx < frameCount; idx++) {
                if (endOffset < startOffset + 8) {
                    dProgressErr() << QApplication::tr("Not enough dimensions in the meta info.");
                    return false;
                }
                startOffset += 8;

                quint32 width;
                quint32 height;
                in >> width;
                in >> height;

                // meta->setContent("x");
            }
        } break;
        case CELMETA_ANIMDELAY: {
            startOffset++;

            quint8 delay;
            in >> delay;

            meta->setContent(QString::number(delay));
        } break;
        case CELMETA_ANIMORDER:
        case CELMETA_ACTIONFRAMES: {
            QString content;
            while (true) {
                if (endOffset < startOffset + 1) {
                    dProgressErr() << QApplication::tr("Open frame list in meta type %1.").arg(D1GfxMeta::metaTypeToStr(type));
                    return false;
                }
                startOffset++;

                quint8 idx;
                in >> idx;
                if (idx == 0) {
                    break;
                }
                content.append(QString::number(idx) + ", ");
            }
            content.chop(2);
            meta->setContent(content);
        } break;
        default:
            dProgressErr() << QApplication::tr("Invalid meta type %1.").arg(type);
            return false;
        }
        meta->setStored(true);
    }

    return startOffset == endOffset;
}

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
    // try to read it as a CEL compilation
    D1CEL_TYPE type = (firstDword == 0 || fileSize == fileSizeDword) ? D1CEL_TYPE::V1_REGULAR : D1CEL_TYPE::V1_COMPILATION;
    gfx.type = type;

    // CEL FRAMES OFFSETS CALCULATION
    std::vector<std::pair<quint32, quint32>> frameOffsets;
    quint32 metaOffset, contentOffset = fileSize;
    if (type == D1CEL_TYPE::V1_REGULAR) {
        // Going through all frames of the CEL
        if (firstDword > 0) {
            gfx.groupFrameIndices.push_back(std::pair<int, int>(0, firstDword - 1));
        }
        unsigned i = 1;
        for ( ; i <= firstDword; i++) {
            device->seek(i * 4);
            quint32 celFrameStartOffset;
            in >> celFrameStartOffset;
            quint32 celFrameEndOffset;
            in >> celFrameEndOffset;

            frameOffsets.push_back(std::pair<quint32, quint32>(celFrameStartOffset, celFrameEndOffset));
        }

        metaOffset = 4 + i * 4;
        if (frameOffsets.size() != 0)
            contentOffset = frameOffsets[0].first;
    } else {
        // Going through all groups
        int cursor = 0;
        unsigned i = 0;
        while (i * 4 < firstDword) {
            device->seek(i * 4);
            quint32 celGroupOffset;
            in >> celGroupOffset;

            if (fileSize < (celGroupOffset + 4)) {
                dProgressErr() << QApplication::tr("Invalid frameoffset %1 of %2 for group %3.").arg(celGroupOffset).arg(fileSize).arg(i);
                break;
            }

            device->seek(celGroupOffset);
            quint32 celFrameCount;
            in >> celFrameCount;

            if (celFrameCount == 0) {
                i++;
                continue;
            }
            if (fileSize < (celGroupOffset + celFrameCount * 4 + 4 + 4)) {
                dProgressErr() << QApplication::tr("Not enough space for %1 frameoffsets at %2 in %3 for group %4.").arg(celFrameCount).arg(celGroupOffset).arg(fileSize).arg(i);
                break;
            }

            if (celGroupOffset < contentOffset) {
                contentOffset = celGroupOffset;
            }
            gfx.groupFrameIndices.push_back(std::pair<int, int>(cursor, cursor + celFrameCount - 1));

            // Going through all frames of the CEL
            for (unsigned int j = 1; j <= celFrameCount; j++) {
                quint32 celFrameStartOffset;
                quint32 celFrameEndOffset;

                device->seek(celGroupOffset + j * 4);
                in >> celFrameStartOffset;
                in >> celFrameEndOffset;

                frameOffsets.push_back(
                    std::pair<quint32, quint32>(celGroupOffset + celFrameStartOffset,
                        celGroupOffset + celFrameEndOffset));
            }
            cursor += celFrameCount;

            i++;
            if (frameOffsets.back().second == fileSize) {
                break;
            }
        }

        metaOffset = i * 4;
    }

    readMeta(device, in, metaOffset, contentOffset, frameOffsets.size(), gfx);

    // CEL FRAMES OFFSETS CALCULATION

    // BUILDING {CEL FRAMES}
    int clipped = -1;
    for (unsigned i = 0; i < frameOffsets.size(); i++) {
        const auto &offset = frameOffsets[i];
        D1GfxFrame *frame = new D1GfxFrame();
        if (fileSize >= offset.second && offset.second >= offset.first) {
            device->seek(offset.first);
            QByteArray celFrameRawData = device->read(offset.second - offset.first);

            int res = D1CelFrame::load(*frame, celFrameRawData, params);
            if (res < 0) {
                if (res == -1)
                    dProgressErr() << QApplication::tr("Could not determine the width of Frame %1.").arg(i + 1);
                else
                    dProgressErr() << QApplication::tr("Frame %1 is invalid.").arg(i + 1);
            } else if (clipped != res) {
                if (clipped == -1)
                    clipped = res;
                else
                    dProgressErr() << QApplication::tr("Inconsistent clipping (Frame %1 is %2).").arg(i + 1).arg(res == 0 ? QApplication::tr("not clipped") : QApplication::tr("clipped"));
            }
        } else {
            dProgressErr() << QApplication::tr("Address of Frame %1 is invalid (%2-%3 of %4).").arg(i + 1).arg(offset.first).arg(offset.second).arg(fileSize);
        }
        gfx.frames.append(frame);
    }
    gfx.clipped = clipped == 1;

    gfx.gfxFilePath = filePath;
    gfx.modified = false;
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

int D1Cel::prepareCelMeta(const D1Gfx &gfx, CelMetaInfo &result)
{
    int META_SIZE = 0;
    result.dimensions = 0;
    result.animDelay = 0;
    for (int i = 0; i < NUM_CELMETA; i++) {
        const D1GfxMeta *meta = gfx.getMeta(i);
        if (!meta->isStored()) continue;
        META_SIZE++;
        switch (i) {
        case CELMETA_DIMENSIONS:
            result.dimensions |= 1;
            META_SIZE += 8;
            break;
        case CELMETA_DIMENSIONS_PER_FRAME:
            result.dimensions |= 2;
            META_SIZE += gfx.getFrameCount() * 8;
            break;
        case CELMETA_ANIMDELAY:
            result.animDelay = meta->getContent().toInt();
            META_SIZE++;
            break;
        case CELMETA_ANIMORDER:
        case CELMETA_ACTIONFRAMES: {
            QList<int> *dest = i == CELMETA_ANIMORDER ? &result.animOrder : &result.actionFrames;
            int num = parseFrameList(meta->getContent(), *dest);
            if (num != dest->count()) {
                dProgressWarn() << QApplication::tr("Not all frames are stored of the meta type %1 (%2 instead of %3).").arg(D1GfxMeta::metaTypeToStr(i)).arg(dest->count()).arg(num);
            }
            dest->append(0);
            META_SIZE += dest->count();
        } break;
        }
    }
    return META_SIZE;
}

int D1Cel::parseFrameList(const QString &content, QList<int> &result)
{
    QStringList frames = content.split(",");
    for (const QString &frame : frames) {
        int idx = frame.toInt();
        if (idx <= 0 || idx > UINT8_MAX) {
            break;
        }
        result.append(idx);
    }
    return frames.count();
}

void D1Cel::formatFrameList(QString &text)
{
    while (true) {
        QString tx = text;
        text.replace(QRegularExpression("[\\s]+"), " ");
        text.replace(QRegularExpression("[^0-9 ,]*"), "");
        text.replace(QRegularExpression("([0-9]) ([0-9])"), "\\1, \\2");
        text.replace(QRegularExpression(",([0-9])"), ", \\1");
        if (tx == text)
            break;
    }
}

static quint8* writeDimensions(cel_meta_type type, const D1Gfx &gfx, quint8 *dest)
{
    *dest = type;
    dest++;
    if (type == CELMETA_DIMENSIONS) {
        QSize fs = gfx.getFrameSize();
        if (!fs.isValid()) {
            dProgressErr() << QApplication::tr("Framesize is not constant");
        }
        *(quint32 *)&dest[0] = SwapLE32(fs.width());
        *(quint32 *)&dest[4] = SwapLE32(fs.height());
        dest += 8;
    } else { // CELMETA_DIMENSIONS_PER_FRAME
        for (int n = 0; n < gfx.getFrameCount(); n++) {
            D1GfxFrame *frame = gfx.getFrame(n);
            *(quint32 *)&dest[0] = SwapLE32(frame->getWidth());
            *(quint32 *)&dest[4] = SwapLE32(frame->getHeight());
            dest += 8;
        }
    }
    return dest;
}

static quint8* writeFrameList(const QList<int> &frames, cel_meta_type type, quint8 *dest)
{
    if (!frames.isEmpty()) {
        *dest = type;
        dest++;

        for (int idx : frames) {
            *dest = idx;
            dest++;
        }
    }
    return dest;
}

quint8* D1Cel::writeCelMeta(const CelMetaInfo &meta, const D1Gfx &gfx, quint8 *dest)
{
    if (meta.dimensions & 1) {
        dest = writeDimensions(CELMETA_DIMENSIONS, gfx, dest);
    }
    if (meta.dimensions & 2) {
        dest = writeDimensions(CELMETA_DIMENSIONS_PER_FRAME, gfx, dest);
    }
    if (meta.animDelay != 0) {
        *dest = CELMETA_ANIMDELAY;
        dest++;
        *dest = meta.animDelay;
        dest++;
    }
    dest = writeFrameList(meta.animOrder, CELMETA_ANIMORDER, dest);
    dest = writeFrameList(meta.actionFrames, CELMETA_ACTIONFRAMES, dest);
    return dest;
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
    // calculate the meta info size
    CelMetaInfo meta;
    int META_SIZE = prepareCelMeta(gfx, meta);
    // estimate data size
    int maxSize = HEADER_SIZE + META_SIZE;
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
    // setup the header
    *(quint32 *)&buf[0] = SwapLE32(numFrames);
    *(quint32 *)&buf[4] = SwapLE32(HEADER_SIZE + META_SIZE);
    { // write the metadata
    pBuf = &buf[HEADER_SIZE];
    pBuf = writeCelMeta(meta , gfx, pBuf);
    }
    { // write the content
    // pBuf = &buf[HEADER_SIZE + META_SIZE];
    for (int n = 0; n < numFrames; n++) {
        D1GfxFrame *frame = gfx.getFrame(n);
        pBuf = writeFrameData(frame, pBuf, subHeaderSize, clipped);
        // update the offset
        *(quint32 *)&buf[4 + 4 * (n + 1)] = SwapLE32(pBuf - buf);
    }
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

    // calculate the meta info size
    CelMetaInfo meta;
    int metaSize = prepareCelMeta(gfx, meta);

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

    headerSize = sizeof(quint32) * numGroups;

    quint8 *buf = (quint8 *)fileData.data();
    quint8 *pBuf;
    { // write the metadata
    pBuf = &buf[headerSize];
    pBuf = writeCelMeta(meta , gfx, pBuf);
    }
    { // write the content
    // pBuf = &buf[headerSize + metaSize];
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
