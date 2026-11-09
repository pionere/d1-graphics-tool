#pragma once

#include <QDataStream>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QString>

#include "d1cel.h"
#include "d1gfx.h"
#include "openasdialog.h"
#include "saveasdialog.h"

class D2C6 {
public:
    static bool load(D1Gfx &gfx, const QString &c6FilePath, const OpenAsParam &params);

private:
    static bool readHeader(QIODevice *device, QDataStream &in, CelHeaderInfo &headerInfo, D1Gfx &gfx);
};
