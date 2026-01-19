/**
 * @file player.cpp
 *
 * Implementation of player functionality, leveling, actions, creation, loading, etc.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

int mypnum;
PlayerStruct players[MAX_PLRS];

/** Specifies the frame of attack and spell animation for which the action is triggered, for each player class. */
static const BYTE PlrActFrames[NUM_CLASSES][9] = {
	// clang-format off
	{  9,  9,  9,  9, 11, 10,  9,  9, 11 },
	{ 10, 10, 10, 10,  7, 13, 10, 10, 11 },
	{ 12,  9, 12, 12, 16, 16, 12, 12, 12 },
#ifdef HELLFIRE
	{  7,  7, 12, 12, 14, 14, 12, 12,  8 },
	{ 10, 10, 10, 10, 11, 13, 10, 10, 11 },
	{  9,  9,  9,  9, 11,  8,  8,  8, 11 },
#endif
	// clang-format on
};
static const BYTE PlrSplFrames[NUM_CLASSES] = {
	// clang-format off
	14,
	12,
	 8,
#ifdef HELLFIRE
	13,
	12,
	14
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
	pc = plr._pClass;
	gn = plr._pgfxnum & 0xF;

	plr._pAFNum = PlrActFrames[pc][gn];
	plr._pSFNum = PlrSplFrames[pc];
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

bool PlrDecHp(int pnum, int hp, int dmgtype)
{
    return false;
}

void PlrIncHp(int pnum, int hp)
{

}

DEVILUTION_END_NAMESPACE
