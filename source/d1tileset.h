#pragma once

#include <set>

#include "d1amp.h"
#include "d1gfx.h"
#include "d1min.h"
#include "d1sol.h"
#include "d1til.h"
#include "d1tmi.h"
#include "saveasdialog.h"

class D1Tileset {
public:
    D1Tileset(D1Gfx *gfx);
    ~D1Tileset();

    void save(const SaveAsParam &params);

    void createSubtile();
    void removeSubtile(int subtileIndex);
    bool reuseFrames(std::set<int> &removedIndices);
    bool reuseSubtiles(std::set<int> &removedIndices);

    D1Gfx *gfx;
    D1Min *min;
    D1Til *til;
    D1Sol *sol;
    D1Amp *amp;
    D1Tmi *tmi;
};
