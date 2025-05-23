#pragma once

#include <vector>

#include <QString>

#include "d1gfx.h"
#include "d1pal.h"
#include "exportdialog.h"
// #include "openasdialog.h"

struct LoadFileContent;

class D1Pcx {
public:
    static bool load(D1Gfx &gfx, D1Pal *pal, const QString &pcxFilePath, const OpenAsParam &params);
    static bool save(const std::vector<std::vector<D1GfxPixel>> &pixels, const D1Pal *pal, const QString &pcxFilePath, const ExportParam &params);

    static bool load(D1GfxFrame &frame, const QString &pcxFilePath, D1Pal *basePal, D1Pal *resPal, bool *palMod);

    static void compare(D1Gfx &gfx, D1Pal *pal, const LoadFileContent *fileContent, bool patchData);
};
