#pragma once

#include <QString>

#include "d1gfx.h"
#include "exportdialog.h"
// #include "openasdialog.h"

class D1SmkAudioData;

class D1Wav {
public:
    // static bool load(D1Gfx &gfx, D1Pal *pal, const QString &pcxFilePath, const OpenAsParam &params);
    static bool save(const D1SmkAudioData *audioData, int track, const QString &wavFilePath, const ExportParam &params);

    // static bool load(D1GfxFrame &frame, const QString &pcxFilePath, bool clipped, D1Pal *basePal, D1Pal *resPal, bool *palMod);
};
