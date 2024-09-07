#pragma once

#include <QFile>
#include <QString>

#include "d1gfx.h"
#include "openasdialog.h"
#include "saveasdialog.h"

class D1Cel {
public:
    static bool load(D1Gfx &gfx, const QString &celFilePath, const OpenAsParam &params);
};
