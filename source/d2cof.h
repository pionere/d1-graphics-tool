#pragma once

#include <QByteArray>

#include "d1gfx.h"
#include "openasdialog.h"

class D2Cof {
public:
    static bool load(D1Gfx &gfx, const QString &cofFilePath, const OpenAsParam &params);

private:
    static bool readContent(QIODevice *device, QDataStream &in, D1Gfx &gfx);
};
