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
    }

    gfx.type = type;

    // CEL FRAMES OFFSETS CALCULATION

    // BUILDING {CEL FRAMES}

    // gfx.frames.clear();
    // std::stack<quint16> invalidFrames;
    for (const auto &offset : frameOffsets) {
        device->seek(offset.first);
        QByteArray celFrameRawData = device->read(offset.second - offset.first);

        D1GfxFrame *frame = new D1GfxFrame();
        if (!D1CelFrame::load(*frame, celFrameRawData, params)) {
            quint16 frameIndex = gfx.frames.size();
            dProgressErr() << QApplication::tr("Frame %1 is invalid (size = %2. from %3 to %4)").arg(frameIndex + 1).arg(offset.second - offset.first).arg(offset.first).arg(offset.second);
            // dProgressErr() << QApplication::tr("Invalid frame %1 is eliminated.").arg(frameIndex + 1);
            // invalidFrames.push(frameIndex);
        }
        gfx.frames.append(frame);
    }

    gfx.gfxFilePath = filePath;
    gfx.modified = false;
    /*while (!invalidFrames.empty()) {
        quint16 frameIndex = invalidFrames.top();
        invalidFrames.pop();
        gfx.removeFrame(frameIndex);
    }*/
    return true;
}
