#include "d2c6.h"

#include <vector>

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>

#include "d2c6frame.h"
#include "progressdialog.h"

bool D2C6::readHeader(QIODevice *device, QDataStream &in, CelHeaderInfo &hi, D1Gfx &gfx)
{
    auto fileSize = device->size();
    // CEL HEADER CHECKS
    const int headerSize = (4 + 2) * 4;
    // Read first DWORD
    if (fileSize < headerSize)
        return false;

    qint32 versionDword;
    in >> versionDword;

    quint32 flagsDword;
    in >> flagsDword;

    quint32 encodingDword;
    in >> encodingDword;

    quint32 terminationDword;
    in >> terminationDword;

    quint32 directionsDword;
    in >> directionsDword;

    quint32 framesPerDirectionDword;
    in >> framesPerDirectionDword;

    hi.groupped = false; // directionsDword > 1;

    // C6 FRAMES OFFSETS CALCULATION
    if (directionsDword * framesPerDirectionDword != 0) {
        if (fileSize < headerSize + (directionsDword * framesPerDirectionDword) * 4)
            return false;

        quint32 celFrameStartOffset;
        quint32 celFrameEndOffset;

        in >> celFrameStartOffset;

        for (unsigned i = 0; i < directionsDword; i++) {
            gfx.groupFrameIndices.push_back(std::pair<int, int>(i * framesPerDirectionDword, (i + 1) * framesPerDirectionDword - 1));

            // Going through all frames of the CEL
            for (unsigned int j = 1; j <= framesPerDirectionDword; j++) {
                if (i != directionsDword - 1 || j != framesPerDirectionDword) {
                    in >> celFrameEndOffset;
                    if (fileSize < celFrameEndOffset) {
                        dProgressErr() << QApplication::tr("Invalid frameoffset %1 of %2 for group %3.").arg(celFrameEndOffset).arg(fileSize).arg(i);
                        break;
                    }
                } else {
                    celFrameEndOffset = fileSize;
                }

                hi.frameOffsets.push_back(
                    QPair<quint32, quint32>(celFrameStartOffset, celFrameEndOffset));

                celFrameStartOffset = celFrameEndOffset;
            }
        }
    }
    return true;
}

bool D2C6::load(D1Gfx &gfx, const QString &filePath, const OpenAsParam &params)
{
    gfx.clear();

    // Opening C6 file and load it in RAM
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

    // BUILDING {CEL FRAMES}
    for (unsigned i = 0; i < hi.frameOffsets.size(); i++) {
        const auto &offset = hi.frameOffsets[i];
        D1GfxFrame *frame = new D1GfxFrame();
        if (fileSize >= offset.second && offset.second >= offset.first) {
            device->seek(offset.first);
            QByteArray c6FrameRawData = device->read(offset.second - offset.first);

            if (!D2C6Frame::load(*frame, c6FrameRawData, params)) {
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
