#pragma once

#include <QString>

#include "d1gfx.h"
#include "d1pal.h"
// #include "openasdialog.h"
// #include "saveasdialog.h"

class D1Pcx {
public:
    // static bool load(D1Gfx &gfx, QString pcxFilePath, const OpenAsParam &params);
    // static bool save(D1Gfx &gfx, const SaveAsParam &params);
    static bool load(D1GfxFrame &frame, QString pcxFilePath, bool clipped, D1Pal *pal);
};
