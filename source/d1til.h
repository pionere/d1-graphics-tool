#pragma once

#include <vector>

#include <QImage>

#include "d1min.h"
#include "saveasdialog.h"

#define TILE_WIDTH 2
#define TILE_HEIGHT 2

class D1Til : public QObject {
    Q_OBJECT

public:
    D1Til() = default;
    ~D1Til() = default;

    bool load(const QString &tilFilePath, D1Min *min);
    bool save(const SaveAsParam &params);
    void clear();

    void compareTo(const D1Til *til) const;

    QImage getTileImage(int tileIndex) const;
    QImage getFlatTileImage(int tileIndex) const;
    QImage getSpecTileImage(int tileIndex) const;
    std::vector<std::vector<D1GfxPixel>> getTilePixelImage(int tileIndex) const;
    std::vector<std::vector<D1GfxPixel>> getFlatTilePixelImage(int tileIndex) const;

    void insertTile(int tileIndex, const std::vector<int> &subtileIndices);
    void removeTile(int tileIndex);

    QString getFilePath() const;
    const D1Min *getMin() const;
    bool isModified() const;
    void setModified();
    int getTileCount() const;
    std::vector<int> &getSubtileIndices(int tileIndex) const;
    bool setSubtileIndex(int tileIndex, int index, int subtileIndex);

private:
    QString tilFilePath;
    bool modified;
    std::vector<std::vector<int>> subtileIndices;
    D1Min *min = nullptr;
};
