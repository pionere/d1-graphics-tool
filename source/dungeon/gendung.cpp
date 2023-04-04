/**
 * @file gendung.cpp
 *
 * Implementation of general dungeon generation code.
 */
#include "all.h"

#include <QString>

#include "../progressdialog.h"

DEVILUTION_BEGIN_NAMESPACE

/** Contains the mega tile IDs of the (mega-)map. */
BYTE dungeon[DMAXX][DMAXY];
/** Contains a backup of the mega tile IDs of the (mega-)map. */
BYTE pdungeon[DMAXX][DMAXY];
/** Flags of mega tiles during dungeon generation. */
BYTE drlgFlags[DMAXX][DMAXY];
/** Container for large, temporary entities during dungeon generation. */
DrlgMem drlg;
/**
 * Contains information about the set pieces of the map.
 * pData contains the content of the .DUN file.
 *  - First the post version, at the end of CreateLevel the pre version is loaded.
 *  - this is not available after the player enters the level.
 */
SetPieceStruct pSetPieces[4];
/** List of the warp-points on the current level */
WarpStruct pWarps[NUM_DWARP];
/** Specifies the mega tiles (groups of four tiles). */
uint16_t* pMegaTiles;
BYTE* pSolidTbl;
/**
 * List of light blocking dPieces
 */
bool nBlockTable[MAXTILES + 1];
/**
 * List of path blocking dPieces
 */
bool nSolidTable[MAXTILES + 1];
/**
 * List of trap-source dPieces (_piece_trap_type)
 */
BYTE nTrapTable[MAXTILES + 1];
/**
 * List of missile blocking dPieces
 */
bool nMissileTable[MAXTILES + 1];
/** The difficuly level of the current game (_difficulty) */
int gnDifficulty;
/** Contains the data of the active dungeon level. */
LevelStruct currLvl;
/** Specifies the number of transparency blocks on the map. */
BYTE numtrans;
/** Specifies the active transparency indices. */
bool TransList[256];
/** Contains the tile IDs of each square on the map. */
int dPiece[MAXDUNX][MAXDUNY];
/** Specifies the transparency index at each coordinate of the map. */
BYTE dTransVal[MAXDUNX][MAXDUNY];
/** Specifies the (runtime) flags of each tile on the map (dflag) */
BYTE dFlags[MAXDUNX][MAXDUNY];
/**
 * Contains the NPC numbers of the map. The NPC number represents a
 * towner number (towners array index) in Tristram and a monster number
 * (monsters array index) in the dungeon.
 *   mnum + 1 : the NPC is on the given location.
 * -(mnum + 1): reserved for a moving NPC
 */
int dMonster[MAXDUNX][MAXDUNY];
/**
 * Contains the object numbers (objects array indices) of the map.
 *   oi + 1 : the object is on the given location.
 * -(oi + 1): a large object protrudes from its base location.
 */
int8_t dObject[MAXDUNX][MAXDUNY];
static_assert(MAXOBJECTS <= CHAR_MAX, "Index of an object might not fit to dObject.");
/**
 * Contains the item numbers (items array indices) of the map.
 *   ii + 1 : the item is on the floor on the given location.
 */
BYTE dItem[MAXDUNX][MAXDUNY];
static_assert(MAXITEMS <= UCHAR_MAX, "Index of an item might not fit to dItem.");
/**
 * Contains the arch frame numbers of the map from the special tileset
 * (e.g. "levels/l1data/l1s.cel"). Note, the special tileset of Tristram (i.e.
 * "levels/towndata/towns.cel") contains trees rather than arches.
 */
BYTE dSpecial[MAXDUNX][MAXDUNY];

void DRLG_Init_Globals()
{
	memset(dFlags, 0, sizeof(dFlags));
	memset(dMonster, 0, sizeof(dMonster));
	memset(dObject, 0, sizeof(dObject));
	memset(dItem, 0, sizeof(dItem));
	memset(dSpecial, 0, sizeof(dSpecial));
}

void InitLvlDungeon()
{
	uint16_t bv;
	size_t i, dwTiles;
	BYTE *pTmp;
	const LevelData* lds;
	lds = &AllLevels[currLvl._dLevelIdx];

	static_assert((int)WRPT_NONE == 0, "InitLvlDungeon fills pWarps with 0 instead of WRPT_NONE values.");
	memset(pWarps, 0, sizeof(pWarps));
	static_assert((int)SPT_NONE == 0, "InitLvlDungeon fills pSetPieces with 0 instead of SPT_NONE values.");
	memset(pSetPieces, 0, sizeof(pSetPieces));

	assert(pMegaTiles == NULL);
	pMegaTiles = (uint16_t*)LoadFileInMem(lds->dMegaTiles);
	if (pMegaTiles != NULL) {
	// patch mega tiles
	if (currLvl._dType == DTYPE_CATHEDRAL) {
		// patch L1.TIL
		// - remove lower layer of tile 203
		// assert(pMegaTiles[(203 - 1) * 4 + 0] == SwapLE16(447 - 1));
		// assert(pMegaTiles[(203 - 1) * 4 + 1] == SwapLE16(438 - 1));
		// assert(pMegaTiles[(203 - 1) * 4 + 2] == SwapLE16(443 - 1));
		// assert(pMegaTiles[(203 - 1) * 4 + 3] == SwapLE16(440 - 1));
		pMegaTiles[(203 - 1) * 4 + 0] = 40 - 1;
		pMegaTiles[(203 - 1) * 4 + 1] = 31 - 1;
		pMegaTiles[(203 - 1) * 4 + 2] = 36 - 1;
		pMegaTiles[(203 - 1) * 4 + 3] = 33 - 1;
		// - remove lower layer of tile 201
		// assert(pMegaTiles[(201 - 1) * 4 + 0] == SwapLE16(444 - 1));
		// assert(pMegaTiles[(201 - 1) * 4 + 1] == SwapLE16(438 - 1));
		// assert(pMegaTiles[(201 - 1) * 4 + 2] == SwapLE16(443 - 1));
		// assert(pMegaTiles[(201 - 1) * 4 + 3] == SwapLE16(440 - 1));
		pMegaTiles[(201 - 1) * 4 + 0] = 37 - 1;
		pMegaTiles[(201 - 1) * 4 + 1] = 31 - 1;
		pMegaTiles[(201 - 1) * 4 + 2] = 36 - 1;
		pMegaTiles[(201 - 1) * 4 + 3] = 33 - 1;
	}
	if (currLvl._dType == DTYPE_CATACOMBS) {
		// patch L2.TIL
		// - remove lower layer of tile 145
		// assert(pMegaTiles[(145 - 1) * 4 + 0] == SwapLE16(527 - 1));
		// assert(pMegaTiles[(145 - 1) * 4 + 1] == SwapLE16(521 - 1));
		// assert(pMegaTiles[(145 - 1) * 4 + 2] == SwapLE16(526 - 1));
		// assert(pMegaTiles[(145 - 1) * 4 + 3] == SwapLE16(523 - 1));
		pMegaTiles[(145 - 1) * 4 + 0] = 44 - 1;
		pMegaTiles[(145 - 1) * 4 + 1] = 38 - 1;
		pMegaTiles[(145 - 1) * 4 + 2] = 43 - 1;
		pMegaTiles[(145 - 1) * 4 + 3] = 40 - 1;
	}
#ifdef HELLFIRE
	if (currLvl._dType == DTYPE_CRYPT) {
		// patch L5.TIL
		// - remove lower layer of tile 86
		// assert(pMegaTiles[(86 - 1) * 4 + 0] == SwapLE16(245 - 1));
		// assert(pMegaTiles[(86 - 1) * 4 + 1] == SwapLE16(237 - 1));
		// assert(pMegaTiles[(86 - 1) * 4 + 2] == SwapLE16(242 - 1));
		// assert(pMegaTiles[(86 - 1) * 4 + 3] == SwapLE16(239 - 1));
		pMegaTiles[(86 - 1) * 4 + 0] = 72 - 1;
		pMegaTiles[(86 - 1) * 4 + 1] = 64 - 1;
		pMegaTiles[(86 - 1) * 4 + 2] = 69 - 1;
		pMegaTiles[(86 - 1) * 4 + 3] = 66 - 1;
	}
#endif
	}
	static_assert(false == 0, "InitLvlDungeon fills tables with 0 instead of false values.");
	memset(nSolidTable, 0, sizeof(nSolidTable));
	memset(nBlockTable, 0, sizeof(nBlockTable));
	memset(nMissileTable, 0, sizeof(nBlockTable));
	memset(nTrapTable, 0, sizeof(nTrapTable));
	assert(pSolidTbl == NULL);
	pSolidTbl = LoadFileInMem(lds->dSolidTable, &dwTiles);
	if (pSolidTbl == NULL) {
		return;
	}
	assert(dwTiles <= MAXTILES);
	pTmp = pSolidTbl;

	// dpiece 0 is always black/void -> make it non-passable to reduce the necessary checks
	// no longer necessary, because dPiece is never zero
	//nSolidTable[0] = true;

	for (i = 1; i <= dwTiles; i++) {
		bv = *pTmp++;
		nSolidTable[i] = (bv & PFLAG_BLOCK_PATH) != 0;
		nBlockTable[i] = (bv & PFLAG_BLOCK_LIGHT) != 0;
		nMissileTable[i] = (bv & PFLAG_BLOCK_MISSILE) != 0;
		nTrapTable[i] = (bv & PFLAG_TRAP_SOURCE) != 0 ? PTT_ANY : PTT_NONE;
	}

	switch (currLvl._dType) {
	case DTYPE_TOWN:
		// patch dSolidTable - Town.SOL
		nSolidTable[553] = false; // allow walking on the left side of the pot at Adria
		nSolidTable[761] = true;  // make the tile of the southern window of the church non-walkable
		nSolidTable[945] = true;  // make the eastern side of Griswold's house consistent (non-walkable)
		break;
	case DTYPE_CATHEDRAL:
		// patch dSolidTable - L1.SOL
		nMissileTable[8] = false; // the only column which was blocking missiles
		break;
	case DTYPE_CATACOMBS:
		// patch dSolidTable - L2.SOL
		// specify direction for torches
		nTrapTable[1] = PTT_LEFT;
		nTrapTable[3] = PTT_LEFT;
		nTrapTable[5] = PTT_RIGHT;
		nTrapTable[6] = PTT_RIGHT;
		nTrapTable[15] = PTT_LEFT;
		nTrapTable[18] = PTT_RIGHT;
		nTrapTable[27] = PTT_LEFT;
		nTrapTable[30] = PTT_RIGHT;
		nTrapTable[31] = PTT_LEFT;
		nTrapTable[34] = PTT_RIGHT;
		nTrapTable[57] = PTT_LEFT;  // added
		nTrapTable[59] = PTT_RIGHT; // added
		nTrapTable[60] = PTT_LEFT;
		nTrapTable[62] = PTT_LEFT;
		nTrapTable[64] = PTT_LEFT;
		nTrapTable[66] = PTT_LEFT;
		nTrapTable[68] = PTT_RIGHT;
		nTrapTable[69] = PTT_RIGHT;
		nTrapTable[72] = PTT_RIGHT;
		nTrapTable[73] = PTT_RIGHT;
		nTrapTable[78] = PTT_LEFT;
		nTrapTable[82] = PTT_LEFT;
		nTrapTable[85] = PTT_LEFT;
		nTrapTable[88] = PTT_LEFT;
		nTrapTable[92] = PTT_LEFT;
		nTrapTable[94] = PTT_LEFT;
		nTrapTable[96] = PTT_LEFT;
		nTrapTable[99] = PTT_RIGHT;
		nTrapTable[104] = PTT_RIGHT;
		nTrapTable[108] = PTT_RIGHT;
		nTrapTable[111] = PTT_RIGHT; // added
		nTrapTable[112] = PTT_RIGHT;
		nTrapTable[115] = PTT_LEFT; // added
		nTrapTable[117] = PTT_LEFT;
		nTrapTable[119] = PTT_LEFT;
		nTrapTable[120] = PTT_LEFT;
		nTrapTable[121] = PTT_RIGHT; // added
		nTrapTable[122] = PTT_RIGHT;
		nTrapTable[125] = PTT_RIGHT;
		nTrapTable[126] = PTT_RIGHT;
		nTrapTable[128] = PTT_RIGHT;
		nTrapTable[129] = PTT_LEFT;
		nTrapTable[144] = PTT_LEFT;
		//nTrapTable[170] = PTT_LEFT; // added
		//nTrapTable[172] = PTT_LEFT; // added
		//nTrapTable[174] = PTT_LEFT; // added
		//nTrapTable[176] = PTT_LEFT; // added
		//nTrapTable[180] = PTT_LEFT; // added
		//nTrapTable[183] = PTT_RIGHT; // added
		//nTrapTable[186] = PTT_RIGHT; // added
		//nTrapTable[187] = PTT_RIGHT; // added
		//nTrapTable[190] = PTT_RIGHT; // added
		//nTrapTable[191] = PTT_RIGHT; // added
		//nTrapTable[194] = PTT_RIGHT; // added
		//nTrapTable[195] = PTT_RIGHT; // added
		nTrapTable[234] = PTT_LEFT;
		nTrapTable[236] = PTT_LEFT;
		nTrapTable[238] = PTT_LEFT; // added
		nTrapTable[240] = PTT_LEFT;
		nTrapTable[242] = PTT_LEFT; // added
		nTrapTable[244] = PTT_LEFT;
		nTrapTable[253] = PTT_RIGHT; // added
		nTrapTable[254] = PTT_RIGHT;
		nTrapTable[257] = PTT_RIGHT;
		nTrapTable[258] = PTT_RIGHT; // added
		nTrapTable[261] = PTT_RIGHT; // added
		nTrapTable[262] = PTT_RIGHT;
		nTrapTable[277] = PTT_LEFT;
		nTrapTable[281] = PTT_LEFT;
		nTrapTable[285] = PTT_LEFT;
		nTrapTable[292] = PTT_RIGHT;
		nTrapTable[296] = PTT_RIGHT;
		nTrapTable[300] = PTT_RIGHT;
		nTrapTable[304] = PTT_RIGHT;
		nTrapTable[305] = PTT_LEFT;
		nTrapTable[446] = PTT_LEFT;
		nTrapTable[448] = PTT_LEFT;
		nTrapTable[450] = PTT_LEFT;
		nTrapTable[452] = PTT_LEFT;
		nTrapTable[454] = PTT_RIGHT;
		nTrapTable[455] = PTT_RIGHT;
		nTrapTable[458] = PTT_RIGHT;
		nTrapTable[459] = PTT_RIGHT;
		nTrapTable[480] = PTT_LEFT;
		nTrapTable[499] = PTT_RIGHT;
		nTrapTable[510] = PTT_LEFT;
		nTrapTable[512] = PTT_LEFT;
		nTrapTable[514] = PTT_RIGHT;
		nTrapTable[515] = PTT_RIGHT;
		nTrapTable[539] = PTT_LEFT;  // added
		nTrapTable[543] = PTT_LEFT;  // added
		nTrapTable[545] = PTT_LEFT;  // added
		nTrapTable[547] = PTT_RIGHT; // added
		nTrapTable[548] = PTT_RIGHT; // added
		nTrapTable[552] = PTT_LEFT;  // added
		// enable torches on (southern) walls
		// nTrapTable[37] = PTT_LEFT;
		// nTrapTable[39] = PTT_LEFT;
		// nTrapTable[41] = PTT_RIGHT;
		// nTrapTable[42] = PTT_RIGHT;
		// nTrapTable[46] = PTT_RIGHT;
		// nTrapTable[47] = PTT_LEFT;
		// nTrapTable[49] = PTT_LEFT;
		// nTrapTable[51] = PTT_RIGHT;
		nTrapTable[520] = PTT_LEFT;
		nTrapTable[522] = PTT_LEFT;
		nTrapTable[524] = PTT_RIGHT;
		nTrapTable[525] = PTT_RIGHT;
		nTrapTable[529] = PTT_RIGHT;
		nTrapTable[530] = PTT_LEFT;
		nTrapTable[532] = PTT_LEFT;
		nTrapTable[534] = PTT_RIGHT;
		break;
	case DTYPE_CAVES:
		break;
	case DTYPE_HELL:
		// patch dSolidTable - L4.SOL
		nMissileTable[141] = false; // fix missile-blocking tile of down-stairs.
		// nMissileTable[137] = false; // fix missile-blocking tile of down-stairs. - skip to keep in sync with the nSolidTable
		// nSolidTable[137] = false;   // fix non-walkable tile of down-stairs. - skip, because it causes a graphic glitch
		nSolidTable[130] = true;    // make the inner tiles of the down-stairs non-walkable I.
		nSolidTable[132] = true;    // make the inner tiles of the down-stairs non-walkable II.
		nSolidTable[131] = true;    // make the inner tiles of the down-stairs non-walkable III.
		nSolidTable[133] = true;    // make the inner tiles of the down-stairs non-walkable IV.
		// fix all-blocking tile on the diablo-level
		nSolidTable[211] = false;
		nMissileTable[211] = false;
		nBlockTable[211] = false;
		// enable hooked bodies on  walls
		nTrapTable[2] = PTT_LEFT;
		nTrapTable[189] = PTT_LEFT;
		nTrapTable[197] = PTT_LEFT;
		nTrapTable[205] = PTT_LEFT;
		nTrapTable[209] = PTT_LEFT;
		nTrapTable[5] = PTT_RIGHT;
		nTrapTable[192] = PTT_RIGHT;
		nTrapTable[212] = PTT_RIGHT;
		nTrapTable[216] = PTT_RIGHT;
		break;
#ifdef HELLFIRE
	case DTYPE_NEST:
		// patch dSolidTable - L6.SOL
		nSolidTable[390] = false; // make a pool tile walkable I.
		nSolidTable[413] = false; // make a pool tile walkable II.
		nSolidTable[416] = false; // make a pool tile walkable III.
		break;
	case DTYPE_CRYPT:
		// patch dSolidTable - L5.SOL
		nSolidTable[143] = false; // make right side of down-stairs consistent (walkable)
		nSolidTable[148] = false; // make the back of down-stairs consistent (walkable)
		// make collision-checks more reasonable
		//  - prevent non-crossable floor-tile configurations I.
		nSolidTable[461] = false;
		//  - set top right tile of an arch non-walkable (full of lava)
		//nSolidTable[471] = true;
		//  - set top right tile of a pillar walkable (just a small obstacle)
		nSolidTable[481] = false;
		//  - tile 491 is the same as tile 594 which is not solid
		//  - prevents non-crossable floor-tile configurations
		nSolidTable[491] = false;
		//  - set bottom left tile of a rock non-walkable (rather large obstacle, feet of the hero does not fit)
		//  - prevents non-crossable floor-tile configurations
		nSolidTable[523] = true;
		//  - set the top right tile of a floor mega walkable (similar to 594 which is not solid)
		nSolidTable[570] = false;
		//  - prevent non-crossable floor-tile configurations II.
		nSolidTable[598] = false;
		nSolidTable[600] = false;
		break;
#endif /* HELLFIRE */
	default:
		ASSUME_UNREACHABLE
		break;
	}
}

void FreeSetPieces()
{
	for (int i = lengthof(pSetPieces) - 1; i >= 0; i--) {
		MemFreeDbg(pSetPieces[i]._spData);
	}
}

void FreeLvlDungeon()
{
	MemFreeDbg(pMegaTiles);
	MemFreeDbg(pSolidTbl);
}

void DRLG_PlaceRndTile(BYTE search, BYTE replace, BYTE rndper)
{
	int i, j, rv; // rv could be BYTE, but in that case VC generates a pointless movzx...

	rv = rndper * 128 / 100; // make the life of random_ easier

	for (i = 0; i < DMAXX; i++) {
		for (j = 0; j < DMAXY; j++) {
			if (dungeon[i][j] == search && drlgFlags[i][j] == 0 && random_(0, 128) < rv) {
				dungeon[i][j] = replace;
			}
		}
	}
}

POS32 DRLG_PlaceMiniSet(const BYTE* miniset)
{
	int sx, sy, sw, sh, xx, yy, ii, tries;
	bool done;

	sw = miniset[0];
	sh = miniset[1];
	// assert(sw < DMAXX && sh < DMAXY);
	tries = (DMAXX * DMAXY) & ~0xFF;
	while (TRUE) {
		if ((tries & 0xFF) == 0) {
			sx = random_low(0, DMAXX - sw);
			sy = random_low(0, DMAXY - sh);
		}
		if (--tries == 0)
			return { -1, 0 };
		ii = 2;
		done = true;
		for (yy = sy; yy < sy + sh && done; yy++) {
			for (xx = sx; xx < sx + sw && done; xx++) {
				if (miniset[ii] != 0 && dungeon[xx][yy] != miniset[ii]) {
					done = false;
				}
				if (drlgFlags[xx][yy]) {
					done = false;
				}
				ii++;
			}
		}
		if (done)
			break;
		if (++sx == DMAXX - sw) {
			sx = 0;
			if (++sy == DMAXY - sh) {
				sy = 0;
			}
		}
	}

	//assert(ii == sw * sh + 2);
	for (yy = sy; yy < sy + sh; yy++) {
		for (xx = sx; xx < sx + sw; xx++) {
			if (miniset[ii] != 0) {
				dungeon[xx][yy] = miniset[ii];
			}
			ii++;
		}
	}

	return { sx, sy };
}

void DRLG_PlaceMegaTiles(int mt)
{
	int i, j, xx, yy;
	int v1, v2, v3, v4;
	uint16_t* Tiles;

	/*int cursor = 0;
	char tmpstr[1024];
	long lvs[] = { 22, 56, 57, 58, 59, 60, 61 };
	for (i = 0; i < lengthof(lvs); i++) {
		lv = lvs[i];
		Tiles = &pMegaTiles[mt * 4];
		v1 = SwapLE16(Tiles[0]) + 1;
		v2 = SwapLE16(Tiles[1]) + 1;
		v3 = SwapLE16(Tiles[2]) + 1;
		v4 = SwapLE16(Tiles[3]) + 1;
		cat_str(tmpstr, cursor, "- %d: %d, %d, %d, %d", lv, v1, v2, v3, v4);
	}
	app_fatal(tmpstr);*/

	if (pMegaTiles == NULL) {
		return;
	}
	Tiles = &pMegaTiles[mt * 4];
	v1 = SwapLE16(Tiles[0]) + 1;
	v2 = SwapLE16(Tiles[1]) + 1;
	v3 = SwapLE16(Tiles[2]) + 1;
	v4 = SwapLE16(Tiles[3]) + 1;

	for (j = 0; j < MAXDUNY; j += 2) {
		for (i = 0; i < MAXDUNX; i += 2) {
			dPiece[i][j] = v1;
			dPiece[i + 1][j] = v2;
			dPiece[i][j + 1] = v3;
			dPiece[i + 1][j + 1] = v4;
		}
	}

	yy = DBORDERY;
	for (j = 0; j < DMAXY; j++) {
		xx = DBORDERX;
		for (i = 0; i < DMAXX; i++) {
			mt = dungeon[i][j] - 1;
			assert(mt >= 0);
			Tiles = &pMegaTiles[mt * 4];
			v1 = SwapLE16(Tiles[0]) + 1;
			v2 = SwapLE16(Tiles[1]) + 1;
			v3 = SwapLE16(Tiles[2]) + 1;
			v4 = SwapLE16(Tiles[3]) + 1;
			dPiece[xx][yy] = v1;
			dPiece[xx + 1][yy] = v2;
			dPiece[xx][yy + 1] = v3;
			dPiece[xx + 1][yy + 1] = v4;
			xx += 2;
		}
		yy += 2;
	}
}

void DRLG_DrawMap(int idx)
{
	int x, y, rw, rh, i, j;
	BYTE* pMap;
	BYTE* sp;

	pMap = pSetPieces[idx]._spData;
	if (pMap == NULL) {
		return;
	}
	rw = SwapLE16(*(uint16_t*)&pMap[0]);
	rh = SwapLE16(*(uint16_t*)&pMap[2]);

	sp = &pMap[4];
	x = pSetPieces[idx]._spx;
	y = pSetPieces[idx]._spy;
	rw += x;
	rh += y;
	for (j = y; j < rh; j++) {
		for (i = x; i < rw; i++) {
			// dungeon[i][j] = *sp != 0 ? *sp : bv;
			if (*sp != 0) {
				dungeon[i][j] = *sp;
			}
			sp += 2;
		}
	}
}

void DRLG_InitTrans()
{
	memset(dTransVal, 0, sizeof(dTransVal));
	//memset(TransList, 0, sizeof(TransList));
	numtrans = 1;
}

void DRLG_MRectTrans(int x1, int y1, int x2, int y2, int tv)
{
	int i, j;

	x1 = 2 * x1 + DBORDERX + 1;
	y1 = 2 * y1 + DBORDERY + 1;
	x2 = 2 * x2 + DBORDERX;
	y2 = 2 * y2 + DBORDERY;

	for (i = x1; i <= x2; i++) {
		for (j = y1; j <= y2; j++) {
			dTransVal[i][j] = tv;
		}
	}
}

void DRLG_RectTrans(int x1, int y1, int x2, int y2)
{
	int i, j;

	for (i = x1; i <= x2; i++) {
		for (j = y1; j <= y2; j++) {
			dTransVal[i][j] = numtrans;
		}
	}
	numtrans++;
}

void DRLG_ListTrans(int num, const BYTE* List)
{
	int i;
	BYTE x1, y1, x2, y2;

	for (i = 0; i < num; i++) {
		x1 = *List++;
		y1 = *List++;
		x2 = *List++;
		y2 = *List++;
		DRLG_RectTrans(x1, y1, x2, y2);
	}
}

void DRLG_AreaTrans(int num, const BYTE* List)
{
	int i;
	BYTE x1, y1, x2, y2;

	for (i = 0; i < num; i++) {
		x1 = *List++;
		y1 = *List++;
		x2 = *List++;
		y2 = *List++;
		DRLG_RectTrans(x1, y1, x2, y2);
		numtrans--;
	}
	numtrans++;
}

/*#if defined(__3DS__)
#pragma GCC push_options
#pragma GCC optimize("O0")
#endif
static void DRLG_FTVR(int i, int j, int x, int y, int dir)
{
	assert(i >= 0 && i < DMAXX && j >= 0 && j < DMAXY);
	if (!drlg.transvalMap[i][j]) {
		switch (dir) {
		case 0:
			dTransVal[x][y] = numtrans;
			dTransVal[x][y + 1] = numtrans;
			break;
		case 1:
			dTransVal[x + 1][y] = numtrans;
			dTransVal[x + 1][y + 1] = numtrans;
			break;
		case 2:
			dTransVal[x][y] = numtrans;
			dTransVal[x + 1][y] = numtrans;
			break;
		case 3:
			dTransVal[x][y + 1] = numtrans;
			dTransVal[x + 1][y + 1] = numtrans;
			break;
		case 4:
			dTransVal[x + 1][y + 1] = numtrans;
			break;
		case 5:
			dTransVal[x][y + 1] = numtrans;
			break;
		case 6:
			dTransVal[x + 1][y] = numtrans;
			break;
		case 7:
			dTransVal[x][y] = numtrans;
			break;
		default:
			ASSUME_UNREACHABLE
			break;
		}
	} else {
		if (dTransVal[x][y] != 0) {
			// assert(dTransVal[x][y] == TransVal);
			return;
		}
		dTransVal[x][y] = numtrans;
		dTransVal[x + 1][y] = numtrans;
		dTransVal[x][y + 1] = numtrans;
		dTransVal[x + 1][y + 1] = numtrans;
		DRLG_FTVR(i + 1, j, x + 2, y, 0);
		DRLG_FTVR(i - 1, j, x - 2, y, 1);
		DRLG_FTVR(i, j + 1, x, y + 2, 2);
		DRLG_FTVR(i, j - 1, x, y - 2, 3);
		DRLG_FTVR(i - 1, j - 1, x - 2, y - 2, 4);
		DRLG_FTVR(i + 1, j - 1, x + 2, y - 2, 5);
		DRLG_FTVR(i - 1, j + 1, x - 2, y + 2, 6);
		DRLG_FTVR(i + 1, j + 1, x + 2, y + 2, 7);
	}
}

void DRLG_FloodTVal(const bool *floorTypes)
{
	int xx, yy, i, j;

	// prepare transvalMap
	for (i = 0; i < DMAXX; i++) {
		for (j = 0; j < DMAXY; j++) {
			drlg.transvalMap[i][j] = floorTypes[drlg.transvalMap[i][j]];
		}
	}

	xx = DBORDERX;

	for (i = 0; i < DMAXX; i++) {
		yy = DBORDERY;

		for (j = 0; j < DMAXY; j++) {
			if (drlg.transvalMap[i][j] && dTransVal[xx][yy] == 0) {
				DRLG_FTVR(i, j, xx, yy, 0);
				numtrans++;
			}
			yy += 2;
		}
		xx += 2;
	}
}
#if defined(__3DS__)
#pragma GCC pop_options
#endif*/

static void DRLG_FTVR(unsigned offset)
{
	BYTE *tvp = &dTransVal[0][0];
	if (tvp[offset] != 0) {
		return;
	}
	tvp[offset] = numtrans;
	BYTE *tp = (BYTE*)&dPiece[0][0];
	//if (numtrans == 1 || numtrans == 2 || numtrans == 6) {
	//	dProgress() << QString("%1:%2 set to %3 on %4 with flags:%5 tpos%6:%7 off%8:%9").arg(offset / DSIZEY).arg(offset % DSIZEY).arg(numtrans).arg(drlg.transvalMap[(offset / DSIZEY) /2][(offset % DSIZEY) / 2]).arg(tp[offset]).arg((offset / DSIZEY) /2).arg(((offset % DSIZEY) / 2) & 1).arg().arg(((offset % DSIZEY) / 2) & 1);
	//}
	//if (drlg.transvalMap[(offset / DSIZEY) /2][(offset % DSIZEY) / 2] == 2) {
	//	dProgress() << QString("%1:%2 has %3 flags").arg(offset / DSIZEY).arg(offset % DSIZEY).arg(tp[offset]);
    //}
	if (tp[offset] & (1 << 0)) { // DIR_SE
		DRLG_FTVR(offset + 1);
	}
	if (tp[offset] & (1 << 1)) { // DIR_NW
		DRLG_FTVR(offset - 1);
	}
	if (tp[offset] & (1 << 2)) { // DIR_N
		DRLG_FTVR(offset - 1 - DSIZEY);
	}
	if (tp[offset] & (1 << 3)) { // DIR_NE
		DRLG_FTVR(offset - DSIZEY);
	}
	if (tp[offset] & (1 << 4)) { // DIR_E
		DRLG_FTVR(offset + 1 - DSIZEY);
	}
	if (tp[offset] & (1 << 5)) { // DIR_W
		DRLG_FTVR(offset - 1 + DSIZEY);
	}
	if (tp[offset] & (1 << 6)) { // DIR_SW
		DRLG_FTVR(offset + DSIZEY);
	}
	if (tp[offset] & (1 << 7)) { // DIR_S
		DRLG_FTVR(offset + DSIZEY + 1);
	}
}

void DRLG_FloodTVal(const BYTE *floorTypes)
{
	int i, j;
	BYTE *tp = (BYTE*)&drlg.transDirMap[0][0];
	BYTE *tm = &drlg.transvalMap[0][0];
	BYTE *tvp = &dTransVal[0][0];

	// prepare the propagation-directions
	for (i = DMAXX - 1; i >= 0; i--) {
		for (j = DMAXY - 1; j >= 0; j--) {
			BYTE tvm = floorTypes[drlg.transvalMap[i][j]];
			BYTE tpm;
			// 1. subtile
			if (tvm & (1 << 0)) {
				tpm = (1 << 1) | (1 << 2) | (1 << 3); // DIR_NW, DIR_N, DIR_NE
				if (tvm & (1 << 2)) // 3. subtile
					tpm |= (1 << 0); // DIR_SE
				if (tvm & (1 << 1)) // 2. subtile
					tpm |= (1 << 6); // DIR_SW
			} else {
				tpm = 0;
			}
			tp[2 * i * DSIZEY + 2 * j] = tpm;
			// 3. subtile
			if (tvm & (1 << 2)) {
				tpm = (1 << 3) | (1 << 4) | (1 << 0); // DIR_NE, DIR_E, DIR_SE
				if (tvm & (1 << 0)) // 1. subtile
					tpm |= (1 << 1); // DIR_NW
				if (tvm & (1 << 3)) // 4. subtile
					tpm |= (1 << 6); // DIR_SW
			} else {
				tpm = 0;
			}
			tp[2 * i * DSIZEY + 2 * j + 1] = tpm;
			// 2. subtile
			if (tvm & (1 << 1)) {
				tpm = (1 << 6) | (1 << 5) | (1 << 1); // DIR_SW, DIR_W, DIR_NW
				if (tvm & (1 << 0)) // 1. subtile
					tpm |= (1 << 3); // DIR_NE
				if (tvm & (1 << 3)) // 4. subtile
					tpm |= (1 << 0); // DIR_SE
			} else {
				tpm = 0;
			}
			tp[(2 * i + 1) * DSIZEY + 2 * j] = tpm;
			// 4. subtile
			if (tvm & (1 << 3)) {
				tpm = (1 << 0) | (1 << 7) | (1 << 6); // DIR_SE, DIR_S, DIR_SW
				if (tvm & (1 << 2)) // 3. subtile
					tpm |= (1 << 3); // DIR_NE
				if (tvm & (1 << 1)) // 2. subtile
					tpm |= (1 << 1); // DIR_NW
			} else {
				tpm = 0;
			}
			tp[(2 * i + 1) * DSIZEY + 2 * j + 1] = tpm;
		}
	}

	for (i = 0; i < DSIZEX * DSIZEY; i++) {
		if (tvp[i] != 0)
			continue;
		if (tp[i] == 0)
			continue;
		DRLG_FTVR(i);
		numtrans++;
	}

	static_assert(DBORDERY + DBORDERX * MAXDUNY > DSIZEY, "DRLG_FloodTVal requires large enough border(x) to use memcpy instead of memmove.");
	for (i = DSIZEX - 1; (int)i >= 0; i--) {
		BYTE *tvpSrc = tvp + i * DSIZEY;
		BYTE *tvpDst = tvp + (i + DBORDERX) * MAXDUNY + DBORDERY;
		memcpy(tvpDst, tvpSrc, DSIZEY);
	}

	memset(tvp, 0, MAXDUNY * DBORDERX + DBORDERY);
	tvp += MAXDUNY * DBORDERX + DBORDERY + DSIZEY;
	while (tvp < (BYTE*)&dTransVal[0][0] + DSIZEX * DSIZEY) {
		memset(tvp, 0, 2 * DBORDERY);
		tvp += 2 * DBORDERY + DSIZEY;
	}
}

void DRLG_LoadSP(int idx, BYTE bv)
{
	int rx1, ry1, rw, rh, i, j;
	BYTE* sp;

	rx1 = pSetPieces[idx]._spx;
	ry1 = pSetPieces[idx]._spy;
	rw = SwapLE16(*(uint16_t*)&pSetPieces[idx]._spData[0]);
	rh = SwapLE16(*(uint16_t*)&pSetPieces[idx]._spData[2]);
	sp = &pSetPieces[idx]._spData[4];
	// load tiles
	for (j = ry1; j < ry1 + rh; j++) {
		for (i = rx1; i < rx1 + rw; i++) {
			dungeon[i][j] = *sp != 0 ? *sp : bv;
			// drlgFlags[i][j] = *sp != 0 ? TRUE : FALSE; // |= DLRG_PROTECTED;
			sp += 2;
		}
	}
	// load flags
	for (j = ry1; j < ry1 + rh; j++) {
		for (i = rx1; i < rx1 + rw; i++) {
			drlgFlags[i][j] = (*sp & 1) != 0 ? DLRG_PROTECTED : 0; // |= DLRG_PROTECTED;
			sp += 2;
		}
	}
}

void DRLG_SetPC()
{
	// int x, y, w, h, i, j, x0, x1, y0, y1;

	for (int n = lengthof(pSetPieces) - 1; n >= 0; n--) {
		if (pSetPieces[n]._spData != NULL) { // pSetPieces[n]._sptype != SPT_NONE
			int x = pSetPieces[n]._spx;
			int y = pSetPieces[n]._spy;
			int w = SwapLE16(*(uint16_t*)&pSetPieces[n]._spData[0]);
			int h = SwapLE16(*(uint16_t*)&pSetPieces[n]._spData[2]);

			/*x0 = 2 * x + DBORDERX;
			y0 = 2 * y + DBORDERY;
			x1 = 2 * w + x0;
			y1 = 2 * h + y0;

			for (j = y0; j < y1; j++) {
				for (i = x0; i < x1; i++) {
					dFlags[i][j] |= BFLAG_POPULATED;
				}
			}*/
			x = 2 * x + DBORDERX;
			y = 2 * y + DBORDERY;

			BYTE* sp = &pSetPieces[n]._spData[4];
			sp += 2 * w * h; // skip tiles

			for (int j = 0; j < h; j++) {
				for (int i = 0; i < w; i++) {
					BYTE flags = *sp;
					if (flags & (1 << 1)) {
						dFlags[x + 2 * i][y + 2 * j] |= BFLAG_POPULATED;
					}
					if (flags & (1 << 2)) {
						dFlags[x + 2 * i + 1][y + 2 * j] |= BFLAG_POPULATED;
					}
					if (flags & (1 << 3)) {
						dFlags[x + 2 * i][y + 2 * j  + 1] |= BFLAG_POPULATED;
					}
					if (flags & (1 << 4)) {
						dFlags[x + 2 * i + 1][y + 2 * j + 1] |= BFLAG_POPULATED;
					}
					sp += 2;
				}
			}
		}
	}
}

/**
 * Find the largest available room (rectangle) starting from (x;y) using floor.
 * 
 * @param floor the id of the floor tile in dungeon
 * @param x the x-coordinate of the starting position
 * @param y the y-coordinate of the starting position
 * @param minSize the minimum size of the room (must be less than 20)
 * @param maxSize the maximum size of the room (must be less than 20)
 * @return the size of the room
 */
static POS32 DRLG_FitThemeRoom(int floor, int x, int y, int minSize, int maxSize)
{
	int xmax, ymax, i, j, smallest;
	int xArray[20], yArray[20];
	int size, bestSize, w, h;

	// assert(maxSize < 20);

	xmax = std::min(maxSize, DMAXX - x);
	ymax = std::min(maxSize, DMAXY - y);
	// BUGFIX: change '&&' to '||' (fixed)
	if (xmax < minSize || ymax < minSize)
		return { 0, 0 };

	//if (NearThemeRoom(x, y)) {
	//	return false;
	//}

	memset(xArray, 0, sizeof(xArray));
	memset(yArray, 0, sizeof(yArray));

	// find horizontal(x) limits
	smallest = xmax;
	for (i = 0; i < ymax; ) {
		for (j = 0; j < smallest; j++) {
			if (dungeon[x + j][y + i] != floor || drlgFlags[x + j][y + i]) {
				smallest = j;
				break;
			}
		}
		if (smallest < minSize)
			break;
		xArray[++i] = smallest;
	}
	if (i < minSize)
		return { 0, 0 };

	// find vertical(y) limits
	smallest = ymax;
	for (i = 0; i < xmax; ) {
		for (j = 0; j < smallest; j++) {
			if (dungeon[x + i][y + j] != floor || drlgFlags[x + i][y + j]) {
				smallest = j;
				break;
			}
		}
		if (smallest < minSize)
			break;
		yArray[++i] = smallest;
	}
	if (i < minSize)
		return { 0, 0 };

	// select the best option
	xmax = std::max(xmax, ymax);
	bestSize = 0;
	for (i = minSize; i <= xmax; i++) {
		size = xArray[i] * i;
		if (size > bestSize) {
			bestSize = size;
			w = xArray[i];
			h = i;
		}
		size = yArray[i] * i;
		if (size > bestSize) {
			bestSize = size;
			w = i;
			h = yArray[i];
		}
	}
	assert(bestSize != 0);
	return { w - 2, h - 2 };
}

static void DRLG_CreateThemeRoom(int themeIndex)
{
	int xx, yy;
	const int lx = themes[themeIndex]._tsx;
	const int ly = themes[themeIndex]._tsy;
	const int hx = lx + themes[themeIndex]._tsWidth;
	const int hy = ly + themes[themeIndex]._tsHeight;
	BYTE v;

	// left/right side
	v = currLvl._dDunType == DTYPE_CAVES ? 137 : 1;
	for (yy = ly; yy < hy; yy++) {
		dungeon[lx][yy] = v;
		dungeon[hx - 1][yy] = v;
	}
	// top/bottom line
	v = currLvl._dDunType == DTYPE_CAVES ? 134 : 2;
	for (xx = lx; xx < hx; xx++) {
		dungeon[xx][ly] = v;
		dungeon[xx][hy - 1] = v;
	}
	// inner tiles
	v = currLvl._dDunType == DTYPE_CATACOMBS ? 3 : (currLvl._dDunType == DTYPE_CAVES ? 7 : 6);
	for (yy = ly + 1; yy < hy - 1; yy++) {
		for (xx = lx + 1; xx < hx - 1; xx++) {
			dungeon[xx][yy] = v;
		}
	}
	// corners
	if (currLvl._dDunType == DTYPE_CATACOMBS) {
		dungeon[lx][ly] = 8;
		dungeon[hx - 1][ly] = 7;
		dungeon[lx][hy - 1] = 9;
		dungeon[hx - 1][hy - 1] = 6;
	}
	if (currLvl._dDunType == DTYPE_CAVES) {
		dungeon[lx][ly] = 150;
		dungeon[hx - 1][ly] = 151;
		dungeon[lx][hy - 1] = 152;
		dungeon[hx - 1][hy - 1] = 138;
	}
	if (currLvl._dDunType == DTYPE_HELL) {
		dungeon[lx][ly] = 9;
		dungeon[hx - 1][ly] = 16;
		dungeon[lx][hy - 1] = 15;
		dungeon[hx - 1][hy - 1] = 12;
	}

	// exits
	if (currLvl._dDunType == DTYPE_CATACOMBS) {
		if (random_(0, 2) == 0) {
			dungeon[hx - 1][(ly + hy) / 2] = 4;
		} else {
			dungeon[(lx + hx) / 2][hy - 1] = 5;
		}
	}
	if (currLvl._dDunType == DTYPE_CAVES) {
		if (random_(0, 2) == 0) {
			dungeon[hx - 1][(ly + hy) / 2] = 147;
		} else {
			dungeon[(lx + hx) / 2][hy - 1] = 146;
		}
	}
	if (currLvl._dDunType == DTYPE_HELL) {
		if (random_(0, 2) == 0) {
			yy = (ly + hy) / 2;
			dungeon[hx - 1][yy - 1] = 53;
			dungeon[hx - 1][yy] = 6;
			dungeon[hx - 1][yy + 1] = 52;
			//dungeon[hx - 2][yy - 1] = 54;
		} else {
			xx = (lx + hx) / 2;
			dungeon[xx - 1][hy - 1] = 57;
			dungeon[xx][hy - 1] = 6;
			dungeon[xx + 1][hy - 1] = 56;
			//dungeon[xx][hy - 2] = 59;
			//dungeon[xx - 1][hy - 2] = 58;
		}
	}
}

void DRLG_PlaceThemeRooms(int minSize, int maxSize, int floor, int freq, bool rndSize)
{
	int i, j;
	int min;

	for (i = 0; i < DMAXX; i++) {
		for (j = 0; j < DMAXY; j++) {
			// always start from a floor tile
			if (dungeon[i][j] != floor) {
				continue;
			}
			if (freq != 0 && random_low(0, freq) != 0) {
				continue;
			}
			// check if there is enough space
			POS32 tArea = DRLG_FitThemeRoom(floor, i, j, minSize, maxSize);
			if (tArea.x <= 0) {
				continue;
			}
			// randomize the size
			if (rndSize) {
				// assert(minSize > 2);
				min = minSize - 2;
				static_assert(DMAXX /* - minSize */ + 2 < 0x7FFF, "DRLG_PlaceThemeRooms uses RandRangeLow to set themeW.");
				static_assert(DMAXY /* - minSize */ + 2 < 0x7FFF, "DRLG_PlaceThemeRooms uses RandRangeLow to set themeH.");
				tArea.x = RandRangeLow(min, tArea.x);
				tArea.y = RandRangeLow(min, tArea.y);
			}
			// ensure there is no overlapping with previous themes
			int n = numthemes - 1;
			for ( ; n >= 0; n--) {
				if (themes[n]._tsx <= i + tArea.x && themes[n]._tsx + themes[n]._tsWidth >= i) {
					break;
				}
				if (themes[n]._tsy <= j + tArea.y && themes[n]._tsy + themes[n]._tsHeight >= j) {
					break;
				}
			}
			if (n >= 0) {
				continue;
			}
			// create the room
			themes[numthemes]._tsx = i + 1;
			themes[numthemes]._tsy = j + 1;
			themes[numthemes]._tsWidth = tArea.x;
			themes[numthemes]._tsHeight = tArea.y;
			DRLG_CreateThemeRoom(numthemes);
			numthemes++;
		}
	}
}

bool NearThemeRoom(int x, int y)
{
	int i;

	for (i = 0; i < numthemes; i++) {
		if (x >= themes[i]._tsx - 2 && x < themes[i]._tsx + themes[i]._tsWidth + 2
		 && y >= themes[i]._tsy - 2 && y < themes[i]._tsy + themes[i]._tsHeight + 2)
			return true;
	}

	return false;
}

DEVILUTION_END_NAMESPACE
