#pragma once

#include <vector>

#include <QMap>

#include "d1gfx.h"
#include "d1tileset.h"

enum class D1PALHITS_MODE {
    ALL_COLORS,
    ALL_FRAMES,
    CURRENT_TILE,
    CURRENT_SUBTILE,
    CURRENT_FRAME
};

class D1PalHits : public QObject {
    Q_OBJECT

public:
    D1PalHits(D1Gfx *g, D1Tileset *ts);

    void update();

    void setGfx(D1Gfx *gfx);
    D1PALHITS_MODE getMode() const;
    void setMode(D1PALHITS_MODE mode);

    // Returns the number of hits for a specific index
    int getIndexHits(quint8 colorIndex, unsigned itemIndex);

private:
    void buildFramePalHits();
    void buildSubtilePalHits();
    void buildTilePalHits();

private:
    D1PALHITS_MODE mode = D1PALHITS_MODE::ALL_COLORS;

    D1Gfx *gfx;
    D1Tileset *tileset;

    // Palette hits are stored with a palette index key and a hit count value
    QMap<quint8, int> allFramesPalHits;
    std::vector<QMap<quint8, int>> framePalHits;
    std::vector<QMap<quint8, int>> tilePalHits;
    std::vector<QMap<quint8, int>> subtilePalHits;
    bool framePalHitsBuilt;
    bool subtilePalHitsBuilt;
    bool tilePalHitsBuilt;
};
