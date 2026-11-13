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

typedef struct D2CcHeaderInfo
{
    QList<QPair<quint32, quint32>> dirOffsets;
    quint32 framesPerDirection;
} D2CcHeaderInfo;

struct D2CcDirectionInfo;

class D2Cc {
public:
    static bool load(D1Gfx &gfx, const QString &ccFilePath, const OpenAsParam &params);

private:
    static bool readHeader(QIODevice *device, QDataStream &in, D2CcHeaderInfo &headerInfo, D1Gfx &gfx);
    static bool decodeFrames(const D2CcHeaderInfo& hi, D2CcDirectionInfo &cdi, D1Gfx &gfx);
};
