#pragma once

#include <map>

#include <QImage>
#include <QList>
#include <QMap>

#include "d1celtilesetframe.h"
#include "d1gfx.h"
#include "d1sol.h"
#include "saveasdialog.h"

class D1Min : public QObject {
    Q_OBJECT

public:
    D1Min() = default;
    ~D1Min() = default;

    bool load(QString minFilePath, D1Gfx *gfx, D1Sol *sol, std::map<unsigned, D1CEL_FRAME_TYPE> &celFrameTypes, const OpenAsParam &params);
    bool save(const SaveAsParam &params);

    QImage getSubtileImage(int subtileIndex);

    void insertSubtile(int subtileIndex, const QList<quint16> &frameReferencesList);
    void createSubtile();
    void removeSubtile(int subtileIndex);
    void remapSubtiles(const QMap<unsigned, unsigned> &remap);

    QString getFilePath();
    bool isModified() const;
    void setModified();
    int getSubtileCount();
    quint16 getSubtileWidth();
    void setSubtileWidth(int width);
    quint16 getSubtileHeight();
    void setSubtileHeight(int height);
    QList<quint16> &getFrameReferences(int subtileIndex);
    bool setFrameReference(int subtileIndex, int index, int frameRef);

private:
    QString minFilePath;
    bool modified;
    quint8 subtileWidth;
    quint8 subtileHeight;
    QList<QList<quint16>> frameReferences;
    D1Gfx *gfx = nullptr;
};
