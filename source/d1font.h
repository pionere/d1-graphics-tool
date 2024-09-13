#pragma once

#include <QString>

#include "d1gfx.h"
#include "importdialog.h"

class D1Font {
public:
    static bool load(D1Gfx &gfx, const QString &filePath, const ImportParam &params);
};
