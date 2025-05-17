/**
 * @file player.cpp
 *
 * Implementation of player functionality, leveling, actions, creation, loading, etc.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

PlayerStruct players[MAX_PLRS];

/**
 * Specifies the number of frames of each animation for each player class.
   STAND, WALK, ATTACK, SPELL, BLOCK, GOTHIT, DEATH
 */
const BYTE PlrGFXAnimLens[NUM_CLASSES][NUM_PLR_ANIMS] = {
	// clang-format off
	{ 10, 8, 16, 20, 2, 6, 20 },
	{  8, 8, 18, 16, 4, 7, 20 },
	{  8, 8, 16, 12, 6, 8, 20 },
#ifdef HELLFIRE
	{  8, 8, 16, 18, 3, 6, 20 },
	{  8, 8, 18, 16, 4, 7, 20 },
	{ 10, 8, 16, 20, 2, 6, 20 },
#endif
	// clang-format on
};
/** Specifies the frame of attack and spell animation for which the action is triggered, for each player class. */
const BYTE PlrGFXAnimActFrames[NUM_CLASSES][2] = {
	// clang-format off
	{  9, 14 },
	{ 10, 12 },
	{ 12,  9 },
#ifdef HELLFIRE
	{ 12, 13 },
	{ 10, 12 },
	{  9, 14 },
#endif
	// clang-format on
};

void SetPlrAnims(int pnum)
{
	int pc, gn;
// LogErrorF("SetPlrAnims %d class%d", pnum, plr._pClass);
	if ((unsigned)pnum >= MAX_PLRS) {
		dev_fatal("SetPlrAnims: illegal player %d", pnum);
	}
	plr._pAnims[PGX_STAND].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_WALK].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_ATTACK].paAnimWidth = 128 * ASSET_MPL;
	plr._pAnims[PGX_FIRE].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_LIGHTNING].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_MAGIC].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_BLOCK].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_GOTHIT].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_DEATH].paAnimWidth = 128 * ASSET_MPL;

	pc = plr._pClass;
	plr._pAFNum = PlrGFXAnimActFrames[pc][0];
	plr._pSFNum = PlrGFXAnimActFrames[pc][1];

	plr._pAnims[PGX_STAND].paFrames = PlrGFXAnimLens[pc][PA_STAND];
	plr._pAnims[PGX_WALK].paFrames = PlrGFXAnimLens[pc][PA_WALK];
	plr._pAnims[PGX_ATTACK].paFrames = PlrGFXAnimLens[pc][PA_ATTACK];
	plr._pAnims[PGX_FIRE].paFrames = PlrGFXAnimLens[pc][PA_SPELL];
	plr._pAnims[PGX_LIGHTNING].paFrames = PlrGFXAnimLens[pc][PA_SPELL];
	plr._pAnims[PGX_MAGIC].paFrames = PlrGFXAnimLens[pc][PA_SPELL];
	plr._pAnims[PGX_BLOCK].paFrames = PlrGFXAnimLens[pc][PA_BLOCK];
	plr._pAnims[PGX_GOTHIT].paFrames = PlrGFXAnimLens[pc][PA_GOTHIT];
	plr._pAnims[PGX_DEATH].paFrames = PlrGFXAnimLens[pc][PA_DEATH];

	gn = plr._pgfxnum & 0xF;
	switch (pc) {
	case PC_WARRIOR:
		if (gn == ANIM_ID_BOW) {
			plr._pAnims[PGX_STAND].paFrames = 8;
			plr._pAnims[PGX_ATTACK].paAnimWidth = 96 * ASSET_MPL;
			// plr._pAnims[PGX_ATTACK].paFrames = 16;
			plr._pAFNum = 11;
		} else if (gn == ANIM_ID_AXE) {
			plr._pAnims[PGX_ATTACK].paFrames = 20;
			plr._pAFNum = 10;
		} else if (gn == ANIM_ID_STAFF) {
			// plr._pAnims[PGX_ATTACK].paFrames = 16;
			plr._pAFNum = 11;
		}
		break;
	case PC_ROGUE:
		if (gn == ANIM_ID_AXE) {
			plr._pAnims[PGX_ATTACK].paFrames = 22;
			plr._pAFNum = 13;
		} else if (gn == ANIM_ID_BOW) {
			plr._pAnims[PGX_ATTACK].paFrames = 12;
			plr._pAFNum = 7;
		} else if (gn == ANIM_ID_STAFF) {
			plr._pAnims[PGX_ATTACK].paFrames = 16;
			plr._pAFNum = 11;
		}
		break;
	case PC_SORCERER:
		plr._pAnims[PGX_FIRE].paAnimWidth = 128 * ASSET_MPL;
		plr._pAnims[PGX_LIGHTNING].paAnimWidth = 128 * ASSET_MPL;
		plr._pAnims[PGX_MAGIC].paAnimWidth = 128 * ASSET_MPL;
		if (gn == ANIM_ID_UNARMED) {
			plr._pAnims[PGX_ATTACK].paFrames = 20;
		} else if (gn == ANIM_ID_UNARMED_SHIELD) {
			// plr._pAnims[PGX_ATTACK].paFrames = 16;
			plr._pAFNum = 9;
		} else if (gn == ANIM_ID_BOW) {
			plr._pAnims[PGX_ATTACK].paFrames = 20;
			plr._pAFNum = 16;
		} else if (gn == ANIM_ID_AXE) {
			plr._pAnims[PGX_ATTACK].paFrames = 24;
			plr._pAFNum = 16;
		}
		break;
#ifdef HELLFIRE
	case PC_MONK:
		plr._pAnims[PGX_STAND].paAnimWidth = 112 * ASSET_MPL;
		plr._pAnims[PGX_WALK].paAnimWidth = 112 * ASSET_MPL;
		plr._pAnims[PGX_ATTACK].paAnimWidth = 130 * ASSET_MPL;
		plr._pAnims[PGX_FIRE].paAnimWidth = 114 * ASSET_MPL;
		plr._pAnims[PGX_LIGHTNING].paAnimWidth = 114 * ASSET_MPL;
		plr._pAnims[PGX_MAGIC].paAnimWidth = 114 * ASSET_MPL;
		plr._pAnims[PGX_BLOCK].paAnimWidth = 98 * ASSET_MPL;
		plr._pAnims[PGX_GOTHIT].paAnimWidth = 98 * ASSET_MPL;
		plr._pAnims[PGX_DEATH].paAnimWidth = 160 * ASSET_MPL;

		switch (gn) {
		case ANIM_ID_UNARMED:
		case ANIM_ID_UNARMED_SHIELD:
			plr._pAnims[PGX_ATTACK].paFrames = 12;
			plr._pAFNum = 7;
			break;
		case ANIM_ID_BOW:
			plr._pAnims[PGX_ATTACK].paFrames = 20;
			plr._pAFNum = 14;
			break;
		case ANIM_ID_AXE:
			plr._pAnims[PGX_ATTACK].paFrames = 23;
			plr._pAFNum = 14;
			break;
		case ANIM_ID_STAFF:
			plr._pAnims[PGX_ATTACK].paFrames = 13;
			plr._pAFNum = 8;
			break;
		}
		break;
	case PC_BARD:
		if (gn == ANIM_ID_AXE) {
			plr._pAnims[PGX_ATTACK].paFrames = 22;
			plr._pAFNum = 13;
		} else if (gn == ANIM_ID_BOW) {
			plr._pAnims[PGX_ATTACK].paFrames = 12;
			plr._pAFNum = 11;
		} else if (gn == ANIM_ID_STAFF) {
			plr._pAnims[PGX_ATTACK].paFrames = 16;
			plr._pAFNum = 11;
		} else if (gn == ANIM_ID_SWORD_SHIELD || gn == ANIM_ID_SWORD) {
			plr._pAnims[PGX_ATTACK].paFrames = 10; // TODO: check for onehanded swords or daggers?
		}
		break;
	case PC_BARBARIAN:
		if (gn == ANIM_ID_AXE) {
			plr._pAnims[PGX_ATTACK].paFrames = 20;
			plr._pAFNum = 8;
		} else if (gn == ANIM_ID_BOW) {
			plr._pAnims[PGX_STAND].paFrames = 8;
			plr._pAnims[PGX_ATTACK].paAnimWidth = 96 * ASSET_MPL;
			plr._pAFNum = 11;
		} else if (gn == ANIM_ID_STAFF) {
			// plr._pAnims[PGX_ATTACK].paFrames = 16;
			plr._pAFNum = 11;
		} else if (gn == ANIM_ID_MACE || gn == ANIM_ID_MACE_SHIELD) {
			// plr._pAnims[PGX_ATTACK].paFrames = 16;
			plr._pAFNum = 8;
		}
		break;
#endif
	default:
		ASSUME_UNREACHABLE
		break;
	}
	if (currLvl._dType == DTYPE_TOWN) {
		plr._pAnims[PGX_STAND].paFrames = 20;
		// plr._pAnims[PGX_WALK].paFrames = 8;
	}
}

bool PosOkActor(int x, int y)
{
	int oi;

	if ((nSolidTable[dPiece[x][y]] | /*dPlayer[x][y] |*/ dMonster[x][y]) != 0)
		return false;

	oi = dObject[x][y];
	if (oi != 0) {
		oi = oi >= 0 ? oi - 1 : -(oi + 1);
		if (objects[oi]._oSolidFlag)
			return false;
	}

	return true;
}

DEVILUTION_END_NAMESPACE
