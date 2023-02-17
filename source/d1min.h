#pragma once

#include <map>
#include <vector>

#include <QImage>

#include "d1celtilesetframe.h"
#include "d1gfx.h"
#include "d1sol.h"
#include "saveasdialog.h"

class D1Min : public QObject {
    Q_OBJECT

    friend class Upscaler;

public:
    D1Min() = default;
    ~D1Min() = default;

    bool load(const QString &minFilePath, D1Gfx *gfx, D1Sol *sol, std::map<unsigned, D1CEL_FRAME_TYPE> &celFrameTypes, const OpenAsParam &params);
    bool save(const SaveAsParam &params);

    QImage getSubtileImage(int subtileIndex) const;
    std::vector<std::vector<D1GfxPixel>> getSubtilePixelImage(int subtileIndex) const;
    void getFrameUses(std::vector<bool> &frameUses) const;

    void removeFrame(int frameIndex, int replacement);
    void insertSubtile(int subtileIndex, const std::vector<unsigned> &frameReferencesList);
    void createSubtile();
    void removeSubtile(int subtileIndex);
    void remapSubtiles(const std::map<unsigned, unsigned> &remap);

    QString getFilePath() const;
    bool isModified() const;
    void setModified();
    int getSubtileCount() const;
    quint16 getSubtileWidth() const;
    void setSubtileWidth(int width);
    quint16 getSubtileHeight() const;
    void setSubtileHeight(int height);
    std::vector<unsigned> &getFrameReferences(int subtileIndex) const;
    bool setFrameReference(int subtileIndex, int index, unsigned frameRef);

private:
    QString minFilePath;
    bool modified;
    quint8 subtileWidth;
    quint8 subtileHeight;
    std::vector<std::vector<unsigned>> frameReferences;
    D1Gfx *gfx = nullptr;
};
