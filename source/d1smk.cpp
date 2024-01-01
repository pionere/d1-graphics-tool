#include "d1smk.h"

#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QList>

#include "config.h"
#include "progressdialog.h"

#include "dungeon/all.h"
//#include <smacker.h>
//#include "../3rdParty/libsmacker/smacker.h"
#include "libsmacker/smacker.h"

#define D1SMK_COLORS 256

bool D1Smk::load(D1Gfx &gfx, D1Pal *pal, const QString &filePath, const OpenAsParam &params)
{
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        dProgressErr() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    const quint64 fileSize = file.size();
    unsigned char *SVidBuffer = (unsigned char *)malloc(fileSize);
    if (file.read(SVidBuffer, fileSize) != fileSize) {
        MemFreeDbg(SVidBuffer);
        dProgressErr() << QApplication::tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    smk SVidSMK = smk_open_memory(SVidBuffer, fileSize);
    if (SVidSMK == NULL) {
        MemFreeDbg(SVidBuffer);
        dProgressErr() << QApplication::tr("Invalid SMK file.");
        return false;
    }

    unsigned long SVidWidth, SVidHeight;
    smk_info_video(SVidSMK, &SVidWidth, &SVidHeight, NULL);
    smk_enable_video(SVidSMK, true);
    // Decode first frame
    char result = smk_first(SVidSMK);
    if (SMK_ERR(result)) {
        MemFreeDbg(SVidBuffer);
        dProgressErr() << QApplication::tr("Empty SMK file.");
        return false;
    }
    // load the first palette
    const unsigned char *smkPal = smk_get_palette(SVidSMK);
    for (int i = 0; i < D1SMK_COLORS; i++) {
        pal->setColor(i, QColor(smkPal[i * 3 + 0], smkPal[i * 3 + 1], smkPal[i * 3 + 2]));
    }
    // load the frames
    // gfx.frames.clear();
    if (params.celWidth != 0) {
        dProgressWarn() << QApplication::tr("Width setting is ignored when a SMK file is loaded.");
    }
    const bool clipped = params.clipped == OPEN_CLIPPED_TYPE::TRUE;
    unsigned frameNum = 0;
    const unsigned char *smkFrame = smk_get_video(SVidSMK);
    do {
        if (smk_palette_updated(SVidSMK) && frameNum != 0)
            dProgressWarn() << QApplication::tr("Palette changed in the %1.frame.").arg(frameNum + 1);
        // create a new frame
        D1GfxFrame *frame = new D1GfxFrame();
        frame->setWidth(SVidWidth);
        frame->setHeight(SVidHeight);
        frame->clipped = clipped;
        const unsigned char *smkFrameCursor = smkFrame;
        for (unsigned y = 0; y < SVidHeight; y++) {
            for (unsigned x = 0; x < SVidWidth; x++, smkFrameCursor++) {
                frame->setPixel(x, y, D1GfxPixel::colorPixel(*smkFrameCursor));
            }
        }

        gfx.frames.append(frame);
        frameNum++;
    } while ((result = smk_next(SVidSMK)) == SMK_MORE);

    if (SMK_ERR(result)) {
        dProgressErr() << QApplication::tr("SMK not fully loaded.");
    }

    gfx.groupFrameIndices.clear();
    gfx.groupFrameIndices.push_back(std::pair<int, int>(0, frameNum - 1));

    QString celPath = filePath;
    // assert(filePath.toLower().endsWith(".smk"));
    celPath.chop(4);
    celPath += ".cel";

    gfx.type = D1CEL_TYPE::V1_REGULAR;

    gfx.gfxFilePath = celPath;
    gfx.modified = true;
    return true;
}
