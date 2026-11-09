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

class D2Cc {
public:
    static bool load(D1Gfx &gfx, const QString &ccFilePath, const OpenAsParam &params);

private:
    static bool readHeader(QIODevice *device, QDataStream &in, CelHeaderInfo &headerInfo, D1Gfx &gfx);
};
