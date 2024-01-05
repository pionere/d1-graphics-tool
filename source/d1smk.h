#pragma once

#include <QMap>
#include <QString>

#include "d1gfx.h"
#include "d1pal.h"
#include "openasdialog.h"
#include "saveasdialog.h"

class D1Smk {
public:
    static bool load(D1Gfx &gfx, QMap<QString, D1Pal *> &pals, const QString &smkFilePath, const OpenAsParam &params);
    static bool save(D1Gfx &gfx, const SaveAsParam &params);
};
