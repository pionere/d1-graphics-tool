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
    for (const auto &offset : frameOffsets) {
        device->seek(offset.first);
        QByteArray cl2FrameRawData = device->read(offset.second - offset.first);

        D1GfxFrame *frame = new D1GfxFrame();
        if (!D1Cl2Frame::load(*frame, cl2FrameRawData, params)) {
            quint16 frameIndex = gfx.frames.size();
            dProgressErr() << QApplication::tr("Frame %1 is invalid.").arg(frameIndex + 1);
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
