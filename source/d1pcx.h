#pragma once

#include <QString>

#include "d1gfx.h"
#include "d1pal.h"
// #include "openasdialog.h"
// #include "saveasdialog.h"

class D1Pcx {
public:
    static bool load(D1Gfx &gfx, D1Pal *pal, const QString &pcxFilePath, const OpenAsParam &params);

    static bool load(D1GfxFrame &frame, const QString &pcxFilePath, bool clipped, D1Pal *basePal, D1Pal *resPal, bool *palMod);
};
