/**
 * @file items.cpp
 *
 * Implementation of item functionality.
 */
#include "all.h"

#define ITEM_ANIM_DELAY 1

int itemactive[MAXITEMS];
/** Contains the items on ground in the current game. */
ItemStruct items[MAXITEMS + 1];
//BYTE* itemanims[NUM_IFILE];
int numitems;

/** Maps from direction to delta X-offset in an 3x3 area. */
static const int area3x3_x[NUM_DIRS + 1] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };
/** Maps from direction to delta Y-offset in an 3x3 area. */
static const int area3x3_y[NUM_DIRS + 1] = { 0, 1, 1, 1, 0, -1, -1, -1, 0 };

static void SetItemLoc(int ii, int x, int y)
{
	items[ii]._ix = x;
	items[ii]._iy = y;
	if (ii != MAXITEMS)
		dItem[x][y] = ii + 1;
}

/**
 * Check the location if an item can be placed there in the init phase.
 * Must not consider the player's position, since it is already initialized
 * and messes up the pseudo-random generated dungeon.
 */
static bool RandomItemPlace(int x, int y)
{
	return (dMonster[x][y] | /*dPlayer[x][y] |*/ dItem[x][y] | dObject[x][y]
	 | (dFlags[x][y] & BFLAG_OBJ_PROTECT) | nSolidTable[dPiece[x][y]]) == 0;
}

static void GetRandomItemSpace(int ii)
{
	int x, y;

	do {
		x = random_(12, DSIZEX) + DBORDERX;
		y = random_(12, DSIZEY) + DBORDERY;
	} while (!RandomItemPlace(x, y));
	SetItemLoc(ii, x, y);
}

static void GetRandomItemSpace(int randarea, int ii)
{
	int x, y, i, j, tries;
	constexpr int numTries = 1000;
	// assert(randarea > 0 && randarea < DBORDERX && randarea < DBORDERY);

	tries = numTries;
	while (true) {
		x = random_(0, DSIZEX) + DBORDERX;
		y = random_(0, DSIZEY) + DBORDERY;
		for (i = x; i < x + randarea; i++) {
			for (j = y; j < y + randarea; j++) {
				if (!RandomItemPlace(i, j))
					goto fail;
			}
		}
		break;
fail:
		tries--;
		if (tries < 0 && randarea > 1) {
			randarea--;
			tries = numTries;
		}
	}

	SetItemLoc(ii, x, y);
}

static inline unsigned items_get_currlevel()
{
	return currLvl._dLevel;
}

void InitItemGFX()
{
	/*int i;
	char filestr[DATA_ARCHIVE_MAX_PATH];

	for (i = 0; i < NUM_IFILE; i++) {
		snprintf(filestr, sizeof(filestr), "Items\\%s.CEL", itemfiledata[i].ifName);
		assert(itemanims[i] == NULL);
		itemanims[i] = LoadFileInMem(filestr);
	}*/
}

void FreeItemGFX()
{
	/*int i;

	for (i = 0; i < NUM_IFILE; i++) {
		MemFreeDbg(itemanims[i]);
	}*/
}

void InitLvlItems()
{
	int i;

	numitems = 0;

	//memset(items, 0, sizeof(items));
	for (i = 0; i < MAXITEMS; i++) {
		itemactive[i] = i;
	}
}

static void PlaceInitItems()
{
	int ii, i;
	int32_t seed;
	unsigned lvl;

	lvl = items_get_currlevel();

	for (i = RandRange(3, 5); i != 0; i--) {
		ii = itemactive[numitems];
		assert(ii == numitems);
		seed = NextRndSeed();
		SetItemData(ii, random_(12, 2) != 0 ? IDI_HEAL : IDI_MANA);
		items[ii]._iSeed = seed;
		items[ii]._iCreateInfo = lvl; // | CF_PREGEN;
		// assert(gbLvlLoad);
		RespawnItem(ii);

		GetRandomItemSpace(ii);

		numitems++;
	}
}

void InitItems()
{
	// if (!currLvl._dSetLvl) {
		if (QuestStatus(Q_ANVIL))
			CreateQuestItemAt(IDI_ANVIL, 2 * pSetPieces[0]._spx + DBORDERX + 11, 2 * pSetPieces[0]._spy + DBORDERY + 11);
		if (currLvl._dLevelIdx == questlist[Q_VEIL]._qdlvl + 1 && quests[Q_VEIL]._qactive != QUEST_NOTAVAIL)
			PlaceQuestItemInArea(IDI_GLDNELIX, 5);
		if (QuestStatus(Q_MUSHROOM))
			PlaceQuestItemInArea(IDI_FUNGALTM, 5);
#ifdef HELLFIRE
		if (quests[Q_JERSEY]._qactive != QUEST_NOTAVAIL) {
			if (currLvl._dLevelIdx == DLV_NEST4)
				PlaceQuestItemInArea(IDI_BROWNSUIT, 3);
			else if (currLvl._dLevelIdx == DLV_NEST3)
				PlaceQuestItemInArea(IDI_GRAYSUIT, 3);
		}
#endif
		if ((currLvl._dLevelIdx >= DLV_CATHEDRAL1 && currLvl._dLevelIdx <= DLV_CATHEDRAL4)
		 || (currLvl._dLevelIdx >= DLV_CATACOMBS1 && currLvl._dLevelIdx <= DLV_CATACOMBS4))
			PlaceInitItems();
#ifdef HELLFIRE
		if (currLvl._dLevelIdx >= DLV_CRYPT1 && currLvl._dLevelIdx <= DLV_CRYPT3) {
			static_assert(DLV_CRYPT1 + 1 == DLV_CRYPT2, "InitItems requires ordered DLV_CRYPT indices I.");
			static_assert(DLV_CRYPT2 + 1 == DLV_CRYPT3, "InitItems requires ordered DLV_CRYPT indices II.");
			static_assert(IDI_NOTE1 + 1 == IDI_NOTE2, "InitItems requires ordered IDI_NOTE indices I.");
			static_assert(IDI_NOTE2 + 1 == IDI_NOTE3, "InitItems requires ordered IDI_NOTE indices II.");
			PlaceQuestItemInArea(IDI_NOTE1 + (currLvl._dLevelIdx - DLV_CRYPT1), 1);
		}
#endif
	// }
}

void SetItemData(int ii, int idata)
{
	SetItemSData(&items[ii], idata);
}

void SetItemSData(ItemStruct* is, int idata)
{
	const ItemData* ids;

	// zero-initialize struct
	memset(is, 0, sizeof(*is));

	is->_iIdx = idata;
	ids = &AllItemList[idata];
	is->_iCurs = ids->iCurs;
	is->_itype = ids->itype;
	is->_iMiscId = ids->iMiscId;
	is->_iSpell = ids->iSpell;
	is->_iClass = ids->iClass;
	is->_iLoc = ids->iLoc;
	is->_iDamType = ids->iDamType;
	is->_iMinDam = ids->iMinDam;
	is->_iMaxDam = ids->iMaxDam;
	is->_iBaseCrit = ids->iBaseCrit;
	is->_iMinStr = ids->iMinStr;
	is->_iMinMag = ids->iMinMag;
	is->_iMinDex = ids->iMinDex;
	is->_iUsable = ids->iUsable;
	is->_iAC = ids->iMinAC == ids->iMaxAC ? ids->iMinAC : RandRangeLow(ids->iMinAC, ids->iMaxAC);
	is->_iDurability = ids->iUsable ? 1 : ids->iDurability; // STACK
	is->_iMaxDur = ids->iDurability;
	is->_ivalue = ids->iValue;
	is->_iIvalue = ids->iValue;

	if (is->_itype == ITYPE_STAFF && is->_iSpell != SPL_NULL) {
		is->_iCharges = BASESTAFFCHARGES;
		is->_iMaxCharges = is->_iCharges;
	}

	static_assert(ITEM_QUALITY_NORMAL == 0, "Zero-fill expects ITEM_QUALITY_NORMAL == 0.");
	//is->_iMagical = ITEM_QUALITY_NORMAL;
	static_assert(SPL_NULL == 0, "Zero-fill expects SPL_NULL == 0.");
	//is->_iPLSkill = SPL_NULL;
}

/**
 * @brief Set a new unique seed value on the given item
 * @param is Item to update
 */
void GetItemSeed(ItemStruct* is)
{
	is->_iSeed = NextRndSeed();
}

void SetGoldItemValue(ItemStruct* is, int value)
{
	is->_ivalue = value;
	if (value >= GOLD_MEDIUM_LIMIT)
		is->_iCurs = ICURS_GOLD_LARGE;
	else if (value <= GOLD_SMALL_LIMIT)
		is->_iCurs = ICURS_GOLD_SMALL;
	else
		is->_iCurs = ICURS_GOLD_MEDIUM;
}

/**
 * Check the location if an item can be placed there while generating the dungeon.
 */
static bool CanPut(int x, int y)
{
	int oi; // , oi2;

	if (x < DBORDERX || x >= DBORDERX + DSIZEX || y < DBORDERY || y >= DBORDERY + DSIZEY)
		return false;

	if ((dItem[x][y] | nSolidTable[dPiece[x][y]]) != 0)
		return false;

	oi = dObject[x][y];
	if (oi != 0) {
		oi = oi >= 0 ? oi - 1 : -(oi + 1);
		if (objects[oi]._oSolidFlag)
			return false;
	}

	/*oi = dObject[x + 1][y + 1];
	if (oi != 0) {
		oi = oi >= 0 ? oi - 1 : -(oi + 1);
		if (objects[oi]._oSelFlag != 0)
			return false;
	}

	oi = dObject[x + 1][y];
	if (oi > 0) {
		oi2 = dObject[x][y + 1];
		if (oi2 > 0 && objects[oi - 1]._oSelFlag != 0 && objects[oi2 - 1]._oSelFlag != 0)
			return false;
	}*/

	if (currLvl._dType == DTYPE_TOWN)
		if ((dMonster[x][y] /*| dMonster[x + 1][y + 1]*/) != 0)
			return false;

	return true;
}

/**
 * Check the location if an item can be placed there in 'runtime'.
 */
bool ItemSpaceOk(int x, int y)
{
	return CanPut(x, y) && (dMonster[x][y] /*| dPlayer[x][y]*/) == 0;
#if 0
	int oi;

	if (x < DBORDERX || x >= DBORDERX + DSIZEX || y < DBORDERY || y >= DBORDERY + DSIZEY)
		return false;

	if ((dItem[x][y] | dMonster[x][y] | /*dPlayer[x][y] |*/ nSolidTable[dPiece[x][y]]) != 0)
		return false;

	oi = dObject[x][y];
	if (oi != 0) {
		oi = oi >= 0 ? oi - 1 : -(oi + 1);
		if (objects[oi]._oSolidFlag)
			return false;
	}

	/*oi = dObject[x + 1][y + 1];
	if (oi != 0) {
		oi = oi >= 0 ? oi - 1 : -(oi + 1);
		if (objects[oi]._oSelFlag != 0)
			return false;
	}

	oi = dObject[x + 1][y];
	if (oi > 0) {
		oi2 = dObject[x][y + 1];
		if (oi2 > 0 && objects[oi - 1]._oSelFlag != 0 && objects[oi2 - 1]._oSelFlag != 0)
			return false;
	}

	if (currLvl._dType == DTYPE_TOWN)
		if (dMonster[x + 1][y + 1] != 0)
			return false;
	*/
	return true;
#endif
}

static bool GetItemSpace(int x, int y, bool (func)(int x, int y), int ii)
{
	BYTE i, rs;
	BYTE slist[NUM_DIRS + 1];

	rs = 0;
	for (i = 0; i < lengthof(area3x3_x); i++) {
		if (func(x + area3x3_x[i], y + area3x3_y[i])) {
			slist[rs] = i;
			rs++;
		}
	}

	if (rs == 0)
		return false;

	rs = slist[random_low(13, rs)];

	SetItemLoc(ii, x + area3x3_x[rs], y + area3x3_y[rs]);
	return true;
}

static void GetSuperItemSpace(int x, int y, int ii)
{
	int xx, yy;
	int i, j;

	if (!GetItemSpace(x, y, ItemSpaceOk, ii) && !GetItemSpace(x, y, CanPut, ii)) {
		const int8_t* cr;
		bool ignoreLine = false;
restart:
		for (i = 2; i < lengthof(CrawlNum); i++) {
			cr = &CrawlTable[CrawlNum[i]];
			for (j = (BYTE)*cr; j > 0; j--) {
				xx = x + *++cr;
				yy = y + *++cr;
				if (CanPut(xx, yy) && (ignoreLine || LineClear(x, y, xx, yy))) {
					SetItemLoc(ii, xx, yy);
					return;
				}
			}
		}
		if (!ignoreLine) {
			ignoreLine = true;
			goto restart;
		}
	}
}

/*static void GetSuperItemLoc(int x, int y, int ii)
{
	int xx, yy;
	int i, j, k;

	for (k = 1; k < 50; k++) {
		for (j = -k; j <= k; j++) {
			yy = y + j;
			for (i = -k; i <= k; i++) {
				xx = i + x;
				if (ItemSpaceOk(xx, yy)) {
					SetItemLoc(ii, xx, yy);
					return;
				}
			}
		}
	}
}*/

static void GetBookSpell(int ii, unsigned lvl)
{
	const SpellData* sd;
	ItemStruct* is;
	static_assert((int)NUM_SPELLS < UCHAR_MAX, "GetBookSpell stores spell-ids in BYTEs.");
	BYTE ss[NUM_SPELLS];
	int bs, ns;

	if (lvl < BOOK_MIN)
		lvl = BOOK_MIN;

	ns = 0;
	for (bs = 0; bs < (IsHellfireGame ? NUM_SPELLS : NUM_SPELLS_DIABLO); bs++) {
		if (spelldata[bs].sBookLvl != SPELL_NA && lvl >= spelldata[bs].sBookLvl
		 && (IsMultiGame || bs != SPL_RESURRECT)) {
			ss[ns] = bs;
			ns++;
		}
	}
	// assert(ns > 0);
	bs = ss[random_low(14, ns)];

	is = &items[ii];
	is->_iSpell = bs;
	sd = &spelldata[bs];
	is->_iMinMag = sd->sMinInt;
	// assert(is->_ivalue == 0 && is->_iIvalue == 0);
	is->_ivalue = sd->sBookCost;
	is->_iIvalue = sd->sBookCost;
	switch (sd->sType) {
	case STYPE_FIRE:
		bs = ICURS_BOOK_RED;
		break;
	case STYPE_LIGHTNING:
		bs = ICURS_BOOK_BLUE;
		break;
	case STYPE_MAGIC:
	case STYPE_NONE:
		bs = ICURS_BOOK_GRAY;
		break;
	default:
		ASSUME_UNREACHABLE
		break;
	}
	is->_iCurs = bs;
}

static void GetScrollSpell(int ii, unsigned lvl)
{
	const SpellData* sd;
	ItemStruct* is;
	static_assert((int)NUM_SPELLS < UCHAR_MAX, "GetScrollSpell stores spell-ids in BYTEs.");
#ifdef HELLFIRE
	static_assert((int)SPL_RUNE_LAST + 1 == (int)NUM_SPELLS, "GetScrollSpell skips spells at the end of the enum.");
	BYTE ss[SPL_RUNE_FIRST];
#else
	BYTE ss[NUM_SPELLS];
#endif
	int bs, ns;

	if (lvl < SCRL_MIN)
		lvl = SCRL_MIN;

	ns = 0;
	for (bs = 0; bs < (IsHellfireGame ? SPL_RUNE_FIRST : NUM_SPELLS_DIABLO); bs++) {
		if (spelldata[bs].sScrollLvl != SPELL_NA && lvl >= spelldata[bs].sScrollLvl
		 && (IsMultiGame || bs != SPL_RESURRECT)) {
			ss[ns] = bs;
			ns++;
		}
	}
	// assert(ns > 0);
	bs = ss[random_low(14, ns)];

	is = &items[ii];
	is->_iSpell = bs;
	sd = &spelldata[bs];
	is->_iMinMag = sd->sMinInt > 20 ? sd->sMinInt - 20 : 0;
	// assert(is->_ivalue == 0 && is->_iIvalue == 0);
	is->_ivalue = sd->sStaffCost;
	is->_iIvalue = sd->sStaffCost;
}

#ifdef HELLFIRE
static void GetRuneSpell(int ii, unsigned lvl)
{
	const SpellData* sd;
	ItemStruct* is;
	static_assert((int)NUM_SPELLS < UCHAR_MAX, "GetRuneSpell stores spell-ids in BYTEs.");
	BYTE ss[SPL_RUNE_LAST - SPL_RUNE_FIRST + 1];
	int bs, ns;

	if (lvl < RUNE_MIN)
		lvl = RUNE_MIN;

	ns = 0;
	for (bs = SPL_RUNE_FIRST; bs <= SPL_RUNE_LAST; bs++) {
		if (/*spelldata[bs].sScrollLvl != SPELL_NA &&*/ lvl >= spelldata[bs].sScrollLvl
		 /*&& (IsMultiGame || bs != SPL_RESURRECT)*/) {
			ss[ns] = bs;
			ns++;
		}
	}
	// assert(ns > 0);
	bs = ss[random_low(14, ns)];

	is = &items[ii];
	is->_iSpell = bs;
	sd = &spelldata[bs];
	is->_iMinMag = sd->sMinInt;
	// assert(is->_ivalue == 0 && is->_iIvalue == 0);
	is->_ivalue = sd->sStaffCost;
	is->_iIvalue = sd->sStaffCost;
	switch (sd->sType) {
	case STYPE_FIRE:
		bs = ICURS_RUNE_OF_FIRE;
		break;
	case STYPE_LIGHTNING:
		bs = ICURS_RUNE_OF_LIGHTNING;
		break;
	case STYPE_MAGIC:
	// case STYPE_NONE:
		bs = ICURS_RUNE_OF_STONE;
		break;
	default:
		ASSUME_UNREACHABLE
		break;
	}
	is->_iCurs = bs;
}
#endif

static void GetStaffSpell(int ii, unsigned lvl)
{
	const SpellData* sd;
	ItemStruct* is;
	static_assert((int)NUM_SPELLS < UCHAR_MAX, "GetStaffSpell stores spell-ids in BYTEs.");
	BYTE ss[NUM_SPELLS];
	int bs, ns, v;

	if (lvl < STAFF_MIN)
		lvl = STAFF_MIN;

	ns = 0;
	for (bs = 0; bs < (IsHellfireGame ? NUM_SPELLS : NUM_SPELLS_DIABLO); bs++) {
		if (spelldata[bs].sStaffLvl != SPELL_NA && lvl >= spelldata[bs].sStaffLvl
		 && (IsMultiGame || bs != SPL_RESURRECT)) {
			ss[ns] = bs;
			ns++;
		}
	}
	// assert(ns > 0);
	bs = ss[random_low(18, ns)];

	is = &items[ii];
	sd = &spelldata[bs];

	is->_iSpell = bs;
	is->_iCharges = RandRangeLow(sd->sStaffMin, sd->sStaffMax);
	is->_iMaxCharges = is->_iCharges;

	is->_iMinMag = sd->sMinInt;
	v = is->_iCharges * sd->sStaffCost;
	is->_ivalue += v;
	is->_iIvalue += v;
}

static int GetItemSpell()
{
	int ns, bs;
	BYTE ss[NUM_SPELLS];

	ns = 0;
	for (bs = 0; bs < (IsHellfireGame ? NUM_SPELLS : NUM_SPELLS_DIABLO); bs++) {
		if (spelldata[bs].sManaCost != 0) { // TODO: use sSkillFlags ?
			// assert(!IsMultiGame || bs != SPL_RESURRECT);
			ss[ns] = bs;
			ns++;
		}
	}
	// assert(ns > 0);
	return ss[random_low(19, ns)];
}

static void GetItemAttrs(int ii, int idata, unsigned lvl)
{
	ItemStruct* is;
	int rndv;

	SetItemData(ii, idata);

	is = &items[ii];
	if (is->_iMiscId == IMISC_BOOK)
		GetBookSpell(ii, lvl);
	else if (is->_iMiscId == IMISC_SCROLL)
		GetScrollSpell(ii, lvl);
#ifdef HELLFIRE
	else if (is->_iMiscId == IMISC_RUNE)
		GetRuneSpell(ii, lvl);
#endif
	else if (is->_itype == ITYPE_GOLD) {
		lvl = items_get_currlevel();
		rndv = RandRangeLow(2 * lvl, 8 * lvl);
		if (rndv > GOLD_MAX_LIMIT)
			rndv = GOLD_MAX_LIMIT;

		SetGoldItemValue(is, rndv);
	}
}

static int PLVal(const AffixData* affix, int pv)
{
	int dp, dv, rv;
	int p1 = affix->PLParam1;
	int p2 = affix->PLParam2;
	int minv = affix->PLMinVal;
	int maxv = affix->PLMaxVal;

	rv = minv;
	dp = p2 - p1;
	if (dp != 0) {
		dv = maxv - minv;
		if (dv != 0)
			rv += dv * (pv - p1) / dp;
	}
	return rv;
}

static int SaveItemPower(int ii, int power, int param1, int param2)
{
	ItemStruct* is;
	ItemAffixStruct* ias;
	int r2;

	is = &items[ii];
	ias = &is->_iAffixes[is->_iNumAffixes];
	is->_iNumAffixes++;
	ias->asPower = power;

	const int r = param1 == param2 ? param1 : RandRangeLow(param1, param2);
	ias->asValue0 = r;

	switch (power) {
	case IPL_TOHIT:
		is->_iPLToHit = r;
		break;
	case IPL_DAMP:
		is->_iPLDam = r;
		break;
	case IPL_TOHIT_DAMP:
		is->_iPLDam = r;
		r2 = RandRangeLow(param1 >> 2, param2 >> 2);
		is->_iPLToHit = r2;
		break;
	case IPL_ACP:
	case IPL_TOBLOCK:
	case IPL_FIRERES:
	case IPL_LIGHTRES:
	case IPL_MAGICRES:
	case IPL_ACIDRES:
	case IPL_ALLRES:
	case IPL_CRITP:
		break;
	case IPL_SKILLLVL:
		ias->asValue1 = GetItemSpell();
		break;
	case IPL_SKILLLEVELS:
		break;
	case IPL_CHARGES:
		is->_iCharges *= r;
		is->_iMaxCharges = is->_iCharges;
		break;
	case IPL_FIREDAM:
	case IPL_LIGHTDAM:
	case IPL_MAGICDAM:
	case IPL_ACIDDAM:
		ias->asFrom = param1;
		ias->asTo = param2;
		break;
	case IPL_STR:
		is->_iPLStr = r;
		break;
	case IPL_MAG:
		is->_iPLMag = r;
		break;
	case IPL_DEX:
		is->_iPLDex = r;
		break;
	case IPL_VIT:
		is->_iPLVit = r;
		break;
	case IPL_ATTRIBS:
		is->_iPLStr = r;
		is->_iPLMag = r;
		is->_iPLDex = r;
		is->_iPLVit = r;
		break;
	case IPL_ABS_ANYHIT:
	case IPL_ABS_PHYHIT:
	case IPL_LIFE:
	case IPL_MANA:
		break;
	case IPL_DUR:
		r2 = r * is->_iMaxDur / 100;
		is->_iDurability = is->_iMaxDur = is->_iMaxDur + r2;
		break;
	case IPL_INDESTRUCTIBLE:
		is->_iDurability = is->_iMaxDur = DUR_INDESTRUCTIBLE;
		break;
	case IPL_LIGHT:
		break;
	// case IPL_INVCURS:
	//	is->_iCurs = param1;
	//	break;
	//case IPL_THORNS:
	//	break;
	case IPL_NOMANA:
	case IPL_KNOCKBACK:
	case IPL_STUN:
	case IPL_NO_BLEED:
	case IPL_BLEED:
	//case IPL_NOHEALMON:
	case IPL_STEALMANA:
	case IPL_STEALLIFE:
	case IPL_PENETRATE_PHYS:
	case IPL_FASTATTACK:
	case IPL_FASTRECOVER:
	case IPL_FASTBLOCK:
	case IPL_DAMMOD:
		break;
	case IPL_SETDAM:
		is->_iMinDam = param1;
		is->_iMaxDam = param2;
		break;
	case IPL_SETDUR:
		is->_iDurability = is->_iMaxDur = r;
		break;
	case IPL_REQSTR:
		is->_iMinStr += r;
		break;
	case IPL_SPELL:
		is->_iSpell = param1;
		is->_iCharges = param2;
		is->_iMaxCharges = param2;
		is->_iMinMag = spelldata[param1].sMinInt;
		break;
	case IPL_ONEHAND:
		is->_iLoc = ILOC_ONEHAND;
		break;
	case IPL_ALLRESZERO:
	case IPL_DRAINLIFE:
	//case IPL_INFRAVISION:
		break;
	case IPL_SETAC:
		is->_iAC = r;
		break;
	case IPL_ACMOD:
		is->_iAC += r;
		break;
	case IPL_CRYSTALLINE:
		is->_iPLDam = r * 2;
		is->_iDurability = is->_iMaxDur = r < 100 ? (is->_iMaxDur - r * is->_iMaxDur / 100) : 1;
		break;
	case IPL_MANATOLIFE:
	case IPL_LIFETOMANA:
	case IPL_FASTCAST:
	case IPL_FASTWALK:
		break;
	default:
		ASSUME_UNREACHABLE
	}
	return r;
}

static void GetItemPower(int ii, unsigned lvl, BYTE range, int flgs, bool onlygood)
{
	int nl, v;
	int va = 0, vm = 0;
	const AffixData *pres, *sufs;
	const AffixData* l[ITEM_RNDAFFIX_MAX];
	BYTE affix;
	BOOLEAN good;

	// assert(items[ii]._iMagical == ITEM_QUALITY_NORMAL);
	if (flgs != PLT_MISC) // items[ii]._itype != ITYPE_RING && items[ii]._itype != ITYPE_AMULET)
		lvl = lvl > AllItemList[items[ii]._iIdx].iMinMLvl ? lvl - AllItemList[items[ii]._iIdx].iMinMLvl : 0;

	// select affixes (3: both, 2: prefix, 1: suffix)
	v = random_(23, 128);
	affix = v < 21 ? 3 : (v < 48 ? 2 : 1);
	static_assert(TRUE > FALSE, "GetItemPower assumes TRUE is greater than FALSE.");
	good = (onlygood || random_(0, 3) != 0) ? TRUE : FALSE;
	if (affix >= 2) {
		nl = 0;
		for (pres = PL_Prefix; pres->PLPower != IPL_INVALID; pres++) {
			if ((flgs & pres->PLIType)
			 && pres->PLRanges[range].from <= lvl && pres->PLRanges[range].to >= lvl
			// && (!onlygood || pres->PLOk)) {
			 && (good <= pres->PLOk)) {
				l[nl] = pres;
				nl++;
				if (pres->PLDouble) {
					l[nl] = pres;
					nl++;
				}
			}
		}
		if (nl != 0) {
			// assert(nl <= 0x7FFF);
			pres = l[random_low(23, nl)];
			items[ii]._iMagical = ITEM_QUALITY_MAGIC;
			v = SaveItemPower(
			    ii,
			    pres->PLPower,
			    pres->PLParam1,
			    pres->PLParam2);
			va += PLVal(pres, v);
			vm += pres->PLMultVal;
		}
	}
	if (affix & 1) {
		nl = 0;
		for (sufs = PL_Suffix; sufs->PLPower != IPL_INVALID; sufs++) {
			if ((sufs->PLIType & flgs)
			    && sufs->PLRanges[range].from <= lvl && sufs->PLRanges[range].to >= lvl
			   // && (!onlygood || sufs->PLOk)) {
			    && (good <= sufs->PLOk)) {
				l[nl] = sufs;
				nl++;
			}
		}
		if (nl != 0) {
			// assert(nl <= 0x7FFF);
			sufs = l[random_low(23, nl)];
			items[ii]._iMagical = ITEM_QUALITY_MAGIC;
			v = SaveItemPower(
			    ii,
			    sufs->PLPower,
			    sufs->PLParam1,
			    sufs->PLParam2);
			va += PLVal(sufs, v);
			vm += sufs->PLMultVal;
		}
	}
	// prefix or suffix added -> recalculate the value of the item
	if (items[ii]._iMagical == ITEM_QUALITY_MAGIC) {
		if (items[ii]._iMiscId != IMISC_MAP) {
			v = vm;
			if (v >= 0) {
				v *= items[ii]._ivalue;
			} else {
				v = items[ii]._ivalue / -v;
			}
			v += va;
			if (v <= 0) {
				v = 1;
			}
		} else {
			v = 6;
			for (unsigned i = 0; i < items[ii]._iNumAffixes; i++) {
				const ItemAffixStruct* ias = &items[ii]._iAffixes[i];
				if (ias->asPower == IMP_AREAMOD) {
					v -= ias->asValue0;
				}
			}
			v = ((1 << MAXCAMPAIGNSIZE) - 1) >> v;
			items[ii]._ivalue = v;
		}
		items[ii]._iIvalue = v;
	}
}

static void GetItemBonus(int ii, unsigned lvl, BYTE range, bool onlygood, bool allowspells)
{
	int flgs;

	switch (items[ii]._itype) {
	case ITYPE_MISC:
		if (items[ii]._iMiscId != IMISC_MAP)
			return;
		flgs = PLT_MAP;
		break;
	case ITYPE_SWORD:
	case ITYPE_AXE:
	case ITYPE_MACE:
		flgs = PLT_MELEE;
		break;
	case ITYPE_BOW:
		flgs = PLT_BOW;
		break;
	case ITYPE_SHIELD:
		flgs = PLT_SHLD;
		break;
	case ITYPE_LARMOR:
		flgs = PLT_LARMOR;
		break;
	case ITYPE_HELM:
		flgs = PLT_HELM;
		break;
	case ITYPE_MARMOR:
		flgs = PLT_MARMOR;
		break;
	case ITYPE_HARMOR:
		flgs = PLT_HARMOR;
		break;
	case ITYPE_STAFF:
		flgs = PLT_STAFF;
		if (allowspells && random_(17, 4) != 0) {
			GetStaffSpell(ii, lvl);
			if (random_(51, 2) != 0)
				return;
			flgs |= PLT_CHRG;
		}
		break;
	case ITYPE_GOLD:
		return;
	case ITYPE_RING:
	case ITYPE_AMULET:
		flgs = PLT_MISC;
		break;
	default:
		ASSUME_UNREACHABLE
		return;
	}

	GetItemPower(ii, lvl, range, flgs, onlygood);
}

static int RndDropItem(bool func(const ItemData& item, void* arg), void* arg, unsigned lvl)
{
#if UNOPTIMIZED_RNDITEMS
	int i, j, ri;
	int ril[ITEM_RNDDROP_MAX];

	ri = 0;
	for (i = IDI_RNDDROP_FIRST; i < NUM_IDI; i++) {
		if (!func(AllItemList[i], arg) || lvl < AllItemList[i].iMinMLvl)
			continue;
		for (j = AllItemList[i].iRnd; j > 0; j--) {
			ril[ri] = i;
			ri++;
		}
	}
	assert(ri != 0);
	return ril[random_(50, ri)];
#else
	int i, ri;
	int ril[NUM_IDI - IDI_RNDDROP_FIRST];

	for (i = IDI_RNDDROP_FIRST; i < (IsHellfireGame ? NUM_IDI : NUM_IDI_DIABLO); i++) {
		ril[i - IDI_RNDDROP_FIRST] = (!func(AllItemList[i], arg) || lvl < AllItemList[i].iMinMLvl) ? 0 : AllItemList[i].iRnd;
	}
	ri = 0;
	for (i = 0; i < (NUM_IDI - IDI_RNDDROP_FIRST); i++)
		ri += ril[i];
	// assert(ri != 0 && ri <= 0x7FFF);
	ri = random_low(50, ri);
	for (i = 0; ; i++) {
		ri -= ril[i];
		if (ri < 0)
			break;
	}
	return i + IDI_RNDDROP_FIRST;
#endif
}

static bool RndUItemOk(const ItemData& item, void* arg)
{
	// assert(item.itype != ITYPE_GOLD);
	return item.itype != ITYPE_MISC || item.iMiscId == IMISC_BOOK || item.iMiscId == IMISC_MAP;
}

static int RndUItem(unsigned lvl)
{
	return RndDropItem(RndUItemOk, NULL, lvl);
}

static bool RndItemOk(const ItemData& item, void* arg)
{
	return true;
}

static int RndAnyItem(unsigned lvl)
{
	if (random_(26, 128) > 32)
		return IDI_GOLD;

	return RndDropItem(RndItemOk, NULL, lvl);
}

typedef struct RndTypeItemParam {
	int itype;
	int imid;
} RndTypeItemParam;

static bool RndTypeItemOk(const ItemData& item, void* arg)
{
	RndTypeItemParam* param = (RndTypeItemParam*)arg;
	return item.itype == param->itype && /*imid == IMISC_INVALID ||*/ item.iMiscId == param->imid;
}

static int RndTypeItem(int itype, int imid, unsigned lvl)
{
	RndTypeItemParam param = { itype, imid };
	// assert(itype != ITYPE_GOLD);
	return RndDropItem(RndTypeItemOk, &param, lvl);
}

static int CheckUnique(int ii, unsigned lvl, unsigned quality)
{
	int i, ui;
	BYTE uok[NUM_UITEM];
	const ItemData &item = AllItemList[items[ii]._iIdx];
	const BYTE uid = item.iUniqType;
	if (uid == UITYPE_NONE || (item.iRnd != 0 && random_(28, 100) > (quality == CFDQ_UNIQUE ? 15 : 1)))
		return -1;

	static_assert(NUM_UITEM <= UCHAR_MAX, "Unique index must fit to a BYTE in CheckUnique.");

	ui = 0;
	for (i = 0; i < (IsHellfireGame ? NUM_UITEM : NUM_UITEM_DIABLO); i++) {
		if (UniqueItemList[i].UIUniqType == uid
		 && lvl >= UniqueItemList[i].UIMinLvl) {
			uok[ui] = i;
			ui++;
		}
	}

	if (ui == 0)
		return -1;

	return uok[random_low(29, ui)];
}

static void GetUniqueItem(int ii, int uid)
{
	const UniqItemData* ui;

	ui = &UniqueItemList[uid];
	SaveItemPower(ii, ui->UIPower1, ui->UIParam1a, ui->UIParam1b);

	if (ui->UIPower2 != IPL_INVALID) {
		SaveItemPower(ii, ui->UIPower2, ui->UIParam2a, ui->UIParam2b);
	if (ui->UIPower3 != IPL_INVALID) {
		SaveItemPower(ii, ui->UIPower3, ui->UIParam3a, ui->UIParam3b);
	if (ui->UIPower4 != IPL_INVALID) {
		SaveItemPower(ii, ui->UIPower4, ui->UIParam4a, ui->UIParam4b);
	if (ui->UIPower5 != IPL_INVALID) {
		SaveItemPower(ii, ui->UIPower5, ui->UIParam5a, ui->UIParam5b);
	if (ui->UIPower6 != IPL_INVALID) {
		SaveItemPower(ii, ui->UIPower6, ui->UIParam6a, ui->UIParam6b);
	}}}}}

	items[ii]._iCurs = ui->UICurs;
	items[ii]._iIvalue = ui->UIValue;

	items[ii]._iUid = uid;
	items[ii]._iMagical = ITEM_QUALITY_UNIQUE;
	// items[ii]._iCreateInfo |= CF_UNIQUE;
}

static void ItemRndDur(int ii)
{
	// skip STACKable and non-durable items
	if (!items[ii]._iUsable && items[ii]._iMaxDur > 1 && items[ii]._iMaxDur != DUR_INDESTRUCTIBLE) {
		// assert((items[ii]._iMaxDur >> 1) <= 0x7FFF);
		items[ii]._iDurability = random_low(0, items[ii]._iMaxDur >> 1) + (items[ii]._iMaxDur >> 2) + 1;
	}
}

static void SetupItem(int ii, int idx, int32_t iseed, unsigned lvl, unsigned quality)
{
	int uid;

	SetRndSeed(iseed);
	GetItemAttrs(ii, idx, lvl);
	items[ii]._iSeed = iseed;
	items[ii]._iCreateInfo = lvl;

	items[ii]._iCreateInfo |= quality << 11;

		if (quality >= CFDQ_GOOD
		 || items[ii]._itype == ITYPE_STAFF
		 || items[ii]._itype == ITYPE_RING
		 || items[ii]._itype == ITYPE_AMULET
		 || random_(32, 128) < 14 || (unsigned)random_(33, 128) <= lvl) {
			uid = CheckUnique(ii, lvl, quality);
			if (uid < 0) {
				GetItemBonus(ii, lvl, IAR_DROP, quality >= CFDQ_GOOD, true);
			} else {
				GetUniqueItem(ii, uid);
				return;
			}
		}
		// if (items[ii]._iMagical != ITEM_QUALITY_UNIQUE)
			ItemRndDur(ii);
}

static void SpawnRndItem(unsigned quality, unsigned lvl, int x, int y)
{
	int idx, ii;

	if (quality >= CFDQ_GOOD)
		idx = RndUItem(lvl);
	else
		idx = RndAnyItem(lvl);

	if (numitems >= MAXITEMS)
		return; // should never be the case
	ii = itemactive[numitems];
	numitems++;

	SetupItem(ii, idx, NextRndSeed(), lvl, quality);

	GetSuperItemSpace(x, y, ii);

	RespawnItem(ii);
}

void CreateRndItem(int x, int y, unsigned quality)
{
	unsigned lvl;

	lvl = items_get_currlevel();

	SpawnRndItem(quality, lvl, x, y);
}

static void SetupUsefulItem(int ii, int32_t iseed, unsigned lvl)
{
	int idx;

	SetRndSeed(iseed);

	idx = random_(34, lvl > 1 ? 5 : 4);
	switch (idx) {
	case 0:
	case 1:
		idx = IDI_HEAL;
		break;
	case 2:
	case 3:
		idx = IDI_MANA;
		break;
	case 4:
		idx = IDI_PORTAL;
		break;
	default:
		ASSUME_UNREACHABLE
		break;
	}

	SetItemData(ii, idx);
	items[ii]._iSeed = iseed;
	items[ii]._iCreateInfo = lvl; // | CF_USEFUL;
}

void CreateTypeItem(int x, int y, unsigned quality, int itype, int imisc)
{
	int idx, ii;
	unsigned lvl;

	lvl = items_get_currlevel();

	if (itype != ITYPE_GOLD)
		idx = RndTypeItem(itype, imisc, lvl);
	else
		idx = IDI_GOLD;
	if (numitems >= MAXITEMS)
		return; // should never be the case
	ii = itemactive[numitems];
	numitems++;

	SetupItem(ii, idx, NextRndSeed(), lvl, quality);

	GetSuperItemSpace(x, y, ii);

	RespawnItem(ii);
}

/**
 * Place a fixed item to the given location.
 * 
 * @param idx: the index of the item(item_indexes enum)
 * @param x tile-coordinate of the target location
 * @param y tile-coordinate of the target location
 * @param mode icreate_mode except for ICM_SEND_FLIP
 */
void CreateQuestItemAt(int idx, int x, int y)
{
	int ii;

		if (numitems >= MAXITEMS)
			return; // should never be the case
		ii = itemactive[numitems];
		numitems++;

	SetItemData(ii, idx);
	items[ii]._iCreateInfo = items_get_currlevel();
	// set Seed for the bloodstones, otherwise quick successive pickup and use
	// will be prevented by the ItemRecord logic
	items[ii]._iSeed = NextRndSeed();

	SetItemLoc(ii, x, y);

		RespawnItem(ii);
}

/**
 * Place a fixed item to a random location where the space is large enough.
 * 
 * @param idx: the index of the item(item_indexes enum)
 * @param areasize: the require size of the space (will be lowered if no matching place is found)
 */
void PlaceQuestItemInArea(int idx, int areasize)
{
	int ii;

	if (numitems >= MAXITEMS)
		return; // should never be the case

	ii = itemactive[numitems];
	numitems++;
	// assert(_iMiscId != IMISC_BOOK && _iMiscId != IMISC_SCROLL && _itype != ITYPE_GOLD);
	SetItemData(ii, idx);
	items[ii]._iCreateInfo = items_get_currlevel(); // | CF_PREGEN;
	items[ii]._iSeed = NextRndSeed();               // make sure it is unique

	GetRandomItemSpace(areasize, ii);

	RespawnItem(ii);
}

void RespawnItem(int ii)
{
	ItemStruct* is;
	int it;

	is = &items[ii];
	it = ItemCAnimTbl[is->_iCurs];
//	is->_iAnimData = itemanims[it];
	is->_iAnimLen = itemfiledata[it].iAnimLen;
	//is->_iAnimFrameLen = ITEM_ANIM_DELAY;
	//is->_iAnimWidth = ITEM_ANIM_WIDTH;
	//is->_iAnimXOffset = (ITEM_ANIM_WIDTH - TILE_WIDTH) / 2;
	//is->_iPostDraw = FALSE;
		is->_iAnimFrame = is->_iAnimLen;
		is->_iAnimFlag = is->_iCurs == ICURS_MAGIC_ROCK;
		is->_iSelFlag = 1;

	/*if (is->_iCurs == ICURS_MAGIC_ROCK) {
		is->_iSelFlag = 1;
		PlaySfxLoc(itemfiledata[ItemCAnimTbl[ICURS_MAGIC_ROCK]].idSFX, is->_ix, is->_iy);
	} else if (is->_iCurs == ICURS_TAVERN_SIGN || is->_iCurs == ICURS_ANVIL_OF_FURY)
		is->_iSelFlag = 1;*/
}
