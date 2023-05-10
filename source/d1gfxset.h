#pragma once

#include <vector>

#include "d1gfx.h"
#include "openasdialog.h"
#include "saveasdialog.h"

#include "dungeon/defs.h"
#include "dungeon/enums.h"

enum class D1GFX_SET_TYPE {
    Missile,
    Monster,
    Player,
    Unknown = -1,
};

class D1Gfxset {
public:
    D1Gfxset(D1Gfx *gfx);
    ~D1Gfxset();

    bool load(const QString &gfxFilePath, const OpenAsParam &params);
    void save(const SaveAsParam &params);

    int getGfxCount() const;
    D1Gfx *getGfx(int i) const;
    void frameModified(D1GfxFrame *frame);
    void setPalette(D1Pal *pal);

private:
    D1GFX_SET_TYPE type = D1GFX_SET_TYPE::Unknown;
    D1Gfx *baseGfx;
    std::vector<D1Gfx *> gfxList;
};