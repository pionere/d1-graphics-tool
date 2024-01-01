#pragma once

#include <QString>

#include "d1gfx.h"
#include "d1pal.h"
// #include "exportdialog.h"
#include "openasdialog.h"

class D1Smk {
public:
    static bool load(D1Gfx &gfx, D1Pal *pal, const QString &smkFilePath, const OpenAsParam &params);
    // static bool save(const std::vector<std::vector<D1GfxPixel>> &pixels, const D1Pal *pal, const QString &smkFilePath, const ExportParam &params);
};
