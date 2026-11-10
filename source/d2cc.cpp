#include "d2cc.h"

#include <vector>

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>

#include "d2ccframe.h"
#include "progressdialog.h"

bool D2Cc::readHeader(QIODevice *device, QDataStream &in, CelHeaderInfo &hi, D1Gfx &gfx)
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

    hi.groupped = true; // directionsByte > 1;

    quint32 sizeDword;
    in >> sizeDword;

    // Trying to find file size in CEL header
    if (fileSize < sizeDword)
        dProgressWarn() << QApplication::tr("Invalid filesize %1 (expected %2).").arg(sizeDword).arg(fileSize);
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

            // Going through all frames of the CC
            for (unsigned int j = 1; j <= celFrameCount; j++) {
                hi.frameOffsets.push_back(
                    QPair<quint32, quint32>(celGroupStartOffset, celGroupEndOffset));
            }

            celGroupStartOffset = celGroupEndOffset;
        }
    }

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
    CelHeaderInfo hi;
    if (!readHeader(device, in, hi, gfx)) {
        return false;
    }
    gfx.type = hi.groupped ? D1CEL_TYPE::V1_COMPILATION : D1CEL_TYPE::V1_REGULAR;
    auto fileSize = device->size();

    // CEL FRAMES OFFSETS CALCULATION

    // BUILDING {CEL FRAMES}
    const quint32 framesPerDirectionDword = gfx.groupFrameIndices.empty() ? 0 : (gfx.groupFrameIndices[0].second - gfx.groupFrameIndices[0].first + 1);
    for (unsigned i = 0; i < hi.frameOffsets.size(); i++) {
        const auto &offset = hi.frameOffsets[i];
        D1GfxFrame *frame = new D1GfxFrame();
        if (fileSize >= offset.second && offset.second >= offset.first) {
            device->seek(offset.first);
            QByteArray celFrameRawData = device->read(offset.second - offset.first);

            if (!D2CcFrame::load(*frame, i % framesPerDirectionDword, celFrameRawData, params)) {
                dProgressErr() << QApplication::tr("Frame %1 is invalid.").arg(i + 1);
            }
        } else {
            dProgressErr() << QApplication::tr("Address of Frame %1 is invalid (%2-%3 of %4).").arg(i + 1).arg(offset.first).arg(offset.second).arg(fileSize);
        }
        gfx.frames.append(frame);
    }
    gfx.clipped = false;

    gfx.gfxFilePath = filePath;
    gfx.modified = false;
    return true;
}
