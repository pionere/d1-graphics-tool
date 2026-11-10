#pragma once

#include <QDataStream>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QString>

#include "d1gfx.h"
#include "openasdialog.h"
#include "saveasdialog.h"

typedef struct D2C6HeaderInfo
{
    QList<QPair<quint32, quint32>> frameOffsets;
} D2C6HeaderInfo;

class D2C6 {
public:
    static bool load(D1Gfx &gfx, const QString &c6FilePath, const OpenAsParam &params);

private:
    static bool readHeader(QIODevice *device, QDataStream &in, D2C6HeaderInfo &headerInfo, D1Gfx &gfx);
};
