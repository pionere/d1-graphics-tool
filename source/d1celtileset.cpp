#include "d1celtileset.h"

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>

#include "d1celtilesetframe.h"
#include "progressdialog.h"

D1CEL_FRAME_TYPE guessFrameType(const QByteArray &rawFrameData)
{
    if (rawFrameData.size() == 544 || rawFrameData.size() == 800) {
        const int leftZeros[16] = {
            0, 8, 24, 48, 80, 120, 168, 224,
            288, 348, 400, 444, 480, 508, 528, 540
        };

        for (int i = 0; i < 16; i++) {
            if (rawFrameData[leftZeros[i]] != 0 || rawFrameData[leftZeros[i] + 1] != 0)
                break;
            if (i == 7 && rawFrameData.size() == 800)
                return D1CEL_FRAME_TYPE::LeftTrapezoid;
            if (i == 15 && rawFrameData.size() == 544)
                return D1CEL_FRAME_TYPE::LeftTriangle;
        }

        const int rightZeros[16] = {
            2, 14, 34, 62, 98, 142, 194, 254,
            318, 374, 422, 462, 494, 518, 534, 542
        };

        for (int i = 0; i < 16; i++) {
            if (rawFrameData[rightZeros[i]] != 0 || rawFrameData[rightZeros[i] + 1] != 0)
                break;
            if (i == 7 && rawFrameData.size() == 800)
                return D1CEL_FRAME_TYPE::RightTrapezoid;
            if (i == 15 && rawFrameData.size() == 544)
                return D1CEL_FRAME_TYPE::RightTriangle;
        }
    }

    if (rawFrameData.size() == 1024) {
        return D1CEL_FRAME_TYPE::Square;
    }

    return D1CEL_FRAME_TYPE::TransparentSquare;
}

bool D1CelTileset::load(D1Gfx &gfx, std::map<unsigned, D1CEL_FRAME_TYPE> &celFrameTypes, const QString &filePath, const OpenAsParam &params)
{
    gfx.clear();

    // Opening Tileset-CEL file and load it in RAM
    QFile file;
    // done by the caller
    // if (!params.celFilePath.isEmpty()) {
    //    filePath = params.celFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
    }

    const QByteArray fileData = file.readAll();

    // Read CEL binary data
    QDataStream in(fileData);
    in.setByteOrder(QDataStream::LittleEndian);

    QIODevice *device = in.device();

    // File size check
    int numFrames = 0;
    auto fileSize = device->size();
    if (fileSize != 0) {
        // CEL HEADER CHECKS
        if (fileSize < 4)
            return false;

        // Read first DWORD
        quint32 readDword;
        in >> readDword;

        numFrames = readDword;

        // Trying to find file size in CEL header
        if (fileSize < (4 + numFrames * 4 + 4))
            return false;

        device->seek(4 + numFrames * 4);
        quint32 fileSizeDword;
        in >> fileSizeDword;

        if (fileSize != fileSizeDword)
            return false;
    }

    if (numFrames > 0) {
        gfx.groupFrameIndices.push_back(std::pair<int, int>(0, numFrames - 1));
    }

    gfx.type = D1CEL_TYPE::V1_LEVEL;

    // CEL FRAMES OFFSETS CALCULATION
    std::vector<std::pair<quint32, quint32>> frameOffsets;
    for (int i = 1; i <= numFrames; i++) {
        device->seek(i * 4);
        quint32 celFrameStartOffset;
        in >> celFrameStartOffset;
        quint32 celFrameEndOffset;
        in >> celFrameEndOffset;

        frameOffsets.push_back(std::pair<quint32, quint32>(celFrameStartOffset, celFrameEndOffset));
    }

    // BUILDING {CEL FRAMES}
    // std::stack<quint16> invalidFrames;
    gfx.patched = false;
    for (unsigned i = 0; i < frameOffsets.size(); i++) {
        const auto &offset = frameOffsets[i];
        device->seek(offset.first);
        QByteArray celFrameRawData = device->read(offset.second - offset.first);
        D1CEL_FRAME_TYPE frameType;
        if (gfx.upscaled && !celFrameRawData.isEmpty()) {
            frameType = (D1CEL_FRAME_TYPE)celFrameRawData.at(0);
            celFrameRawData.remove(0, 1);
        } else {
            auto iter = celFrameTypes.find(i + 1);
            if (iter != celFrameTypes.end()) {
                frameType = iter->second;
            } else {
                dProgressWarn() << QApplication::tr("Unknown frame type for frame %1.").arg(i + 1);
                frameType = guessFrameType(celFrameRawData);
            }
        }
        D1GfxFrame *frame = new D1GfxFrame();
        if (!D1CelTilesetFrame::load(*frame, frameType, celFrameRawData, &gfx.patched)) {
            quint16 frameIndex = gfx.frames.size();
            dProgressErr() << QApplication::tr("Frame %1 is invalid (type %2 offset:%3-%4 (%5)).").arg(frameIndex + 1).arg(frameType).arg(offset.first).arg(offset.second).arg(offset.second - offset.first);
            // dProgressErr() << QApplication::tr("Invalid frame %1 is eliminated.").arg(frameIndex + 1);
            // invalidFrames.push(frameIndex);
        }
        gfx.frames.append(frame);
    }
    gfx.gfxFilePath = filePath;
    gfx.modified = !file.isOpen();
    /*while (!invalidFrames.empty()) {
        quint16 frameIndex = invalidFrames.top();
        invalidFrames.pop();
        gfx.removeFrame(frameIndex);
    }*/
    return true;
}

bool D1CelTileset::writeFileData(D1Gfx &gfx, QFile &outFile, const SaveAsParam &params)
{
    const int numFrames = gfx.getFrameCount();

    // update upscaled info
    bool upscaled = gfx.upscaled;
    if (params.upscaled != SAVE_UPSCALED_TYPE::AUTODETECT) {
        upscaled = params.upscaled == SAVE_UPSCALED_TYPE::TRUE;
        gfx.upscaled = upscaled; // setUpscaled
    }

    // update patched info
    bool patched = gfx.patched;
    if (params.patched != SAVE_PATCHED_TYPE::AUTODETECT) {
        patched = params.patched == SAVE_PATCHED_TYPE::TRUE;
        gfx.patched = patched; // setPatched
    }

    // calculate header size
    int headerSize = 4 + numFrames * 4 + 4;

    // estimate data size
    int maxSize = headerSize;
    maxSize += MICRO_WIDTH * MICRO_HEIGHT * numFrames;

    QByteArray fileData;
    fileData.append(maxSize, 0);

    quint8 *buf = (quint8 *)fileData.data();
    *(quint32 *)&buf[0] = SwapLE32(numFrames);

    quint8 *pBuf = &buf[headerSize];
    for (int ii = 0; ii < numFrames; ii++) {
        *(quint32 *)&buf[(ii + 1) * sizeof(quint32)] = SwapLE32(pBuf - buf);
        D1GfxFrame *frame = gfx.getFrame(ii);
        if (gfx.upscaled) {
            *pBuf = (quint8)frame->getFrameType();
            pBuf++;
        }
        pBuf = D1CelTilesetFrame::writeFrameData(*frame, pBuf, gfx.patched);
    }

    *(quint32 *)&buf[(numFrames + 1) * sizeof(quint32)] = SwapLE32(pBuf - buf);

    // write to file
    QDataStream out(&outFile);
    out.writeRawData((char *)buf, pBuf - buf);

    return true;
}

bool D1CelTileset::save(D1Gfx &gfx, const SaveAsParam &params)
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

    bool result = D1CelTileset::writeFileData(gfx, outFile, params);

    if (result) {
        gfx.gfxFilePath = filePath; // D1CelTileset::load(gfx, filePath);
        gfx.modified = false;
    }
    return result;
}
