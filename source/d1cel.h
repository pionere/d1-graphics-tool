#pragma once

#include <QDataStream>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QString>

#include "d1gfx.h"
#include "openasdialog.h"
#include "saveasdialog.h"

typedef struct CelMetaInfo
{
    int dimensions;
    QList<int> animOrder;
    QList<int> actionFrames;
} CelMetaInfo;

class D1Cel {
    friend class D1Cl2;
public:
    static bool load(D1Gfx &gfx, const QString &celFilePath, const OpenAsParam &params);
    static bool save(D1Gfx &gfx, const SaveAsParam &params);

private:
    static bool readMeta(QIODevice *device, QDataStream &in, quint32 startOffset, quint32 endOffset, unsigned frameCount, D1Gfx &gfx);
    static int prepareCelMeta(const D1Gfx &gfx, CelMetaInfo &result);
    static bool parseFrameList(const QString &content, D1CEL_META_TYPE type, QList<int> &result);
    static quint8* writeDimensions(int dimensions, const D1Gfx &gfx, quint8 *dest);
    static quint8* writeFrameList(const QList<int> &frames, D1CEL_META_TYPE type, quint8 *dest);

private:
    static bool writeFileData(D1Gfx &gfx, QFile &outFile, const SaveAsParam &params);
    static bool writeCompFileData(D1Gfx &gfx, QFile &outFile, const SaveAsParam &params);
};
