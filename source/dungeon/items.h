/**
 * @file items.h
 *
 * Interface of item functionality.
 */
#ifndef __ITEMS_H__
#define __ITEMS_H__

DEVILUTION_BEGIN_NAMESPACE

static_assert((int)ITYPE_SWORD + 1 == (int)ITYPE_AXE, "ITYPE_DURABLE check requires a specific ITYPE order I.");
static_assert((int)ITYPE_AXE + 1 == (int)ITYPE_BOW, "ITYPE_DURABLE check requires a specific ITYPE order II.");
static_assert((int)ITYPE_BOW + 1 == (int)ITYPE_MACE, "ITYPE_DURABLE check requires a specific ITYPE order III.");
static_assert((int)ITYPE_MACE + 1 == (int)ITYPE_STAFF, "ITYPE_DURABLE check requires a specific ITYPE order IV.");
static_assert((int)ITYPE_STAFF + 1 == (int)ITYPE_SHIELD, "ITYPE_DURABLE check requires a specific ITYPE order V.");
static_assert((int)ITYPE_SHIELD + 1 == (int)ITYPE_HELM, "ITYPE_DURABLE check requires a specific ITYPE order VI.");
static_assert((int)ITYPE_HELM + 1 == (int)ITYPE_LARMOR, "ITYPE_DURABLE check requires a specific ITYPE order VII.");
static_assert((int)ITYPE_LARMOR + 1 == (int)ITYPE_MARMOR, "ITYPE_DURABLE check requires a specific ITYPE order VIII.");
static_assert((int)ITYPE_MARMOR + 1 == (int)ITYPE_HARMOR, "ITYPE_DURABLE check requires a specific ITYPE order IX.");
#define ITYPE_DURABLE(itype) (itype >= ITYPE_SWORD && itype <= ITYPE_HARMOR)

#define ITEM_ANIM_WIDTH (96 * ASSET_MPL)

extern int itemactive[MAXITEMS];
extern ItemStruct items[MAXITEMS + 1];
extern int numitems;

void InitItemGFX();
void FreeItemGFX();
void InitLvlItems();
void InitItems();
void GetItemSeed(ItemStruct* is);
void SetGoldItemValue(ItemStruct* is, int value);
bool ItemSpaceOk(int x, int y);
void SetItemData(int ii, int idata);
void SetItemSData(ItemStruct* is, int idata);
void CreateRndItem(int x, int y, unsigned quality);
void CreateTypeItem(int x, int y, unsigned quality, int itype, int imisc);
void PlaceQuestItemInArea(int idx, int areasize);
void CreateQuestItemAt(int idx, int x, int y);
void RespawnItem(int ii);

#endif /* __ITEMS_H__ */
