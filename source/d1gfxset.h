#pragma once

#include <QList>
#include <QPair>
#include <QRect>

#include "d1gfx.h"
#include "openasdialog.h"
#include "remapdialog.h"
#include "saveasdialog.h"

#include "dungeon/defs.h"
#include "dungeon/enums.h"

enum class D1GFX_SET_TYPE {
    Missile,
    Monster,
    Player,
    Unknown = -1,
};

enum class D1GFX_SET_CLASS_TYPE {
    Warrior,
    Rogue,
    Mage,
    Monk,
    Unknown = -1,
};

enum class D1GFX_SET_ARMOR_TYPE {
    Light,
    Medium,
    Heavy,
    Unknown = -1,
};

enum class D1GFX_SET_WEAPON_TYPE {
    Unarmed,
    ShieldOnly,
    Sword,
    SwordShield,
    Bow,
    Axe,
    Blunt,
    BluntShield,
    Staff,
    Unknown = -1,
};

struct LoadFileContent;

class D1Gfxset {
public:
    D1Gfxset(D1Gfx *gfx);
    ~D1Gfxset();

    bool load(const QString &gfxFilePath, const OpenAsParam &params, bool optional = false);
    void save(const SaveAsParam &params);

    bool isClippedConstant() const;
    bool isGroupsConstant() const;
    void compareTo(const LoadFileContent *fileContent, bool patchData) const;
    QRect getBoundary() const;
    bool check(const D1Gfx *gfx, int assetMpl) const;
    void mask(int frameIndex, unsigned flags);

    D1GFX_SET_TYPE getType() const;
    D1GFX_SET_CLASS_TYPE getClassType() const;
    D1GFX_SET_ARMOR_TYPE getArmorType() const;
    D1GFX_SET_WEAPON_TYPE getWeaponType() const;
    int getGfxCount() const;
    void setGfx(D1Gfx *gfx); // set base gfx
    D1Gfx *getGfx(int i) const;
    D1Gfx *getBaseGfx() const;
    QList<D1Gfx *> &getGfxList() const;
    QString getGfxLabel(int) const;
    void frameModified(const D1GfxFrame *frame);
    void setPalette(D1Pal *pal);

private:
    D1GFX_SET_TYPE type = D1GFX_SET_TYPE::Unknown;
    D1GFX_SET_CLASS_TYPE ctype = D1GFX_SET_CLASS_TYPE::Unknown;
    D1GFX_SET_ARMOR_TYPE atype = D1GFX_SET_ARMOR_TYPE::Unknown;
    D1GFX_SET_WEAPON_TYPE wtype = D1GFX_SET_WEAPON_TYPE::Unknown;
    D1Gfx *baseGfx;
    QList<D1Gfx *> gfxList;
};
