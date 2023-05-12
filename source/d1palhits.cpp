#include "d1palhits.h"

D1PalHits::D1PalHits(D1Gfx *g, D1Tileset *ts, D1Gfxset *gs)
    : gfx(g)
    , tileset(ts)
    , gfxset(gs)
{
    this->update();
}

void D1PalHits::update()
{
    this->framePalHitsBuilt = false;
    this->tilePalHitsBuilt = false;
    this->subtilePalHitsBuilt = false;
}

D1PALHITS_MODE D1PalHits::getMode() const
{
    return this->mode;
}

void D1PalHits::setMode(D1PALHITS_MODE m)
{
    this->mode = m;
}

void D1PalHits::buildFramePalHits()
{
    this->framePalHitsBuilt = true;

    // Go through all frames
    this->allFramesPalHits.clear();
    this->framePalHits.resize(this->gfx->getFrameCount());
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        QMap<quint8, int> &frameHits = this->framePalHits[i];
        frameHits.clear();

        // Get frame pointer
        D1GfxFrame *frame = this->gfx->getFrame(i);

        // Go through every pixels of the frame
        for (int x = 0; x < frame->getWidth(); x++) {
            for (int y = 0; y < frame->getHeight(); y++) {
                // Retrieve the color of the pixel
                D1GfxPixel pixel = frame->getPixel(x, y);
                if (pixel.isTransparent())
                    continue;
                quint8 paletteIndex = pixel.getPaletteIndex();

                // Add one hit to the frameHits and allFramesPalHits maps
                frameHits.insert(paletteIndex, frameHits.value(paletteIndex) + 1);

                this->allFramesPalHits.insert(paletteIndex, frameHits.value(paletteIndex) + 1);
            }
        }
    }
}

void D1PalHits::buildSubtilePalHits()
{
    if (!this->framePalHitsBuilt) {
        this->buildFramePalHits();
    }

    // assert(this->tileset != nullptr);
    this->subtilePalHitsBuilt = true;

    this->subtilePalHits.resize(this->tileset->min->getSubtileCount());
    // Go through all sub-tiles
    for (int i = 0; i < this->tileset->min->getSubtileCount(); i++) {
        QMap<quint8, int> &subtileHits = this->subtilePalHits[i];
        subtileHits.clear();

        // Retrieve the CEL frame references of the current sub-tile
        std::vector<unsigned> &frameReferences = this->tileset->min->getFrameReferences(i);

        // Go through the CEL frames
        for (unsigned frameRef : frameReferences) {
            if (frameRef == 0)
                continue;
            // Go through the hits of the CEL frame and add them to the subtile hits
            QMapIterator<quint8, int> it2(this->framePalHits[frameRef - 1]);
            while (it2.hasNext()) {
                it2.next();
                subtileHits.insert(it2.key(), it2.value());
            }
        }
    }
}

void D1PalHits::buildTilePalHits()
{
    if (!this->subtilePalHitsBuilt) {
        this->buildSubtilePalHits();
    }
    // assert(this->tileset != nullptr);
    this->tilePalHitsBuilt = true;

    this->tilePalHits.resize(this->tileset->til->getTileCount());
    // Go through all tiles
    for (int i = 0; i < this->tileset->til->getTileCount(); i++) {
        QMap<quint8, int> &tileHits = this->tilePalHits[i];
        tileHits.clear();

        // Retrieve the subtile indices of the current tile
        std::vector<int> &subtileIndices = this->tileset->til->getSubtileIndices(i);

        // Go through the subtiles
        for (int subtileIndex : subtileIndices) {
            // Go through the hits of the subtile and add them to the tile hits
            QMapIterator<quint8, int> it2(this->subtilePalHits[subtileIndex]);
            while (it2.hasNext()) {
                it2.next();
                tileHits.insert(it2.key(), it2.value());
            }
        }
    }
}

int D1PalHits::getIndexHits(quint8 colorIndex, unsigned itemIndex) const
{
    switch (this->mode) {
    case D1PALHITS_MODE::ALL_COLORS:
        return 1;
    case D1PALHITS_MODE::ALL_FRAMES:
        if (!this->framePalHitsBuilt) {
            this->buildFramePalHits();
        }
        if (this->allFramesPalHits.contains(colorIndex))
            return this->allFramesPalHits[colorIndex];
        break;
    case D1PALHITS_MODE::CURRENT_TILE:
        if (!this->tilePalHitsBuilt) {
            this->buildTilePalHits();
        }
        if (this->tilePalHits.size() > itemIndex && this->tilePalHits[itemIndex].contains(colorIndex))
            return this->tilePalHits[itemIndex][colorIndex];
        break;
    case D1PALHITS_MODE::CURRENT_SUBTILE:
        if (!this->subtilePalHitsBuilt) {
            this->buildSubtilePalHits();
        }
        if (this->subtilePalHits.size() > itemIndex && this->subtilePalHits[itemIndex].contains(colorIndex))
            return this->subtilePalHits[itemIndex][colorIndex];
        break;
    case D1PALHITS_MODE::CURRENT_FRAME:
        if (!this->framePalHitsBuilt) {
            this->buildFramePalHits();
        }
        if (this->framePalHits.size() > itemIndex && this->framePalHits[itemIndex].contains(colorIndex))
            return this->framePalHits[itemIndex][colorIndex];
        break;
    }

    return 0;
}
