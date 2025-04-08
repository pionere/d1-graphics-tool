#pragma once

#include <map>
#include <vector>

#include <QImage>

#include "d1celtilesetframe.h"
#include "saveasdialog.h"

class D1Gfx;
class D1Tileset;

class D1Min : public QObject {
    Q_OBJECT

    friend class Upscaler;

public:
    D1Min() = default;
    ~D1Min() = default;

    bool load(const QString &minFilePath, D1Tileset *tileset, std::map<unsigned, D1CEL_FRAME_TYPE> &celFrameTypes, const OpenAsParam &params);
    bool save(const SaveAsParam &params);
    void clear();

    void compareTo(const D1Min *min, const std::map<unsigned, D1CEL_FRAME_TYPE> &celFrameTypes) const;

    QImage getSubtileImage(int subtileIndex) const;
    QImage getSpecSubtileImage(int subtileIndex) const;
    std::vector<std::vector<D1GfxPixel>> getSubtilePixelImage(int subtileIndex) const;
    QImage getFloorImage(int subtileIndex) const;
    void getFrameUses(std::vector<bool> &frameUses) const;

    void insertSubtile(int subtileIndex, const std::vector<unsigned> &frameReferencesList);
    void removeSubtile(int subtileIndex);
    void remapSubtiles(const std::map<unsigned, unsigned> &remap);

    QString getFilePath() const;
    bool isModified() const;
    void setModified();
    int getSubtileCount() const;
    int getSubtileWidth() const;
    void setSubtileWidth(int width);
    int getSubtileHeight() const;
    void setSubtileHeight(int height);
    std::vector<unsigned> &getFrameReferences(int subtileIndex) const;
    bool setFrameReference(int subtileIndex, int index, unsigned frameRef);

private:
    QString minFilePath;
    bool modified;
    int subtileWidth;
    int subtileHeight;
    std::vector<std::vector<unsigned>> frameReferences;
    D1Tileset *tileset = nullptr;
};
