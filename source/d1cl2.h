#pragma once

#include <QFile>
#include <QString>

#include "d1gfx.h"
#include "openasdialog.h"
#include "saveasdialog.h"

class D1Cl2 {
public:
    static bool load(D1Gfx &gfx, const QString &cl2FilePath, const OpenAsParam &params);
};
