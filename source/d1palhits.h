#pragma once

#include <vector>

#include <QMap>

#include "d1gfx.h"
#include "d1gfxset.h"
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
    D1PalHits(D1Gfx *g, D1Tileset *ts, D1Gfxset *gs);

    void update();

    D1PALHITS_MODE getMode() const;
    void setMode(D1PALHITS_MODE m);

    // Returns the number of hits for a specific index
    int getIndexHits(quint8 colorIndex, unsigned itemIndex) const;

private:
    void buildPalHits();
    void buildSubtilePalHits();
    void buildTilePalHits();

private:
    D1PALHITS_MODE mode = D1PALHITS_MODE::ALL_COLORS;

    D1Gfx *gfx;
    D1Tileset *tileset;
    D1Gfxset *gfxset;

    // Palette hits are stored with a palette index key and a hit count value
    QMap<quint8, int> allFramesPalHits;
    std::vector<QMap<quint8, int>> framePalHits;
    std::vector<QMap<quint8, int>> tilePalHits;
    std::vector<QMap<quint8, int>> subtilePalHits;
};
