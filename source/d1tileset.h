#pragma once

#include <set>

#include "d1amp.h"
#include "d1gfx.h"
#include "d1min.h"
#include "d1sol.h"
#include "d1til.h"
#include "d1tmi.h"
#include "openasdialog.h"
#include "saveasdialog.h"

typedef enum dungeon_type {
    DTYPE_TOWN,
    DTYPE_CATHEDRAL,
    DTYPE_CATACOMBS,
    DTYPE_CAVES,
    DTYPE_HELL,
    DTYPE_CRYPT,
    DTYPE_NEST,
    DTYPE_NONE,
    NUM_DUNGEON_TYPES,
} dungeon_type;

class D1Tileset {
public:
    D1Tileset(D1Gfx *gfx);
    ~D1Tileset();

    bool load(const OpenAsParam &params);
    void save(const SaveAsParam &params);

    void createSubtile();
    void removeSubtile(int subtileIndex, int replacement);
    bool reuseFrames(std::set<int> &removedIndices, bool silent);
    bool reuseSubtiles(std::set<int> &removedIndices);

    void patch(int dunType, bool silent);

    D1Gfx *gfx;
    D1Min *min;
    D1Til *til;
    D1Sol *sol;
    D1Amp *amp;
    D1Tmi *tmi;
};
