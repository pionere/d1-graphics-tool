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
/** Specifies the tiles (groups of four subtiles). */
uint16_t pTiles[MAXTILES + 1][4];
BYTE* pSolidTbl;
/**
 * List of light blocking dPieces
 */
bool nBlockTable[MAXSUBTILES + 1];
/**
 * List of path blocking dPieces
 */
bool nSolidTable[MAXSUBTILES + 1];
/**
 * Flags of subtiles to specify trap-sources (_piece_trap_type) and special cel-frames
 */
BYTE nSpecTrapTable[MAXSUBTILES + 1];
/**
 * List of missile blocking dPieces
 */
bool nMissileTable[MAXSUBTILES + 1];
/** The difficuly level of the current game (_difficulty) */
int gnDifficulty;
/** Contains the data of the active dungeon level. */
LevelStruct currLvl;
/** Specifies the number of transparency blocks on the map. */
BYTE numtrans;
/* Specifies whether the transvals should be re-processed. */
static bool gbDoTransVals;
/** Specifies the active transparency indices. */
bool TransList[256];
/** Contains the subtile IDs of each square on the map. */
int dPiece[MAXDUNX][MAXDUNY];
/** Specifies the transparency index of each square on the map. */
BYTE dTransVal[MAXDUNX][MAXDUNY];
/** Specifies the base darkness levels of each square on the map. */
// BYTE dPreLight[MAXDUNX][MAXDUNY];
/** Specifies the current darkness levels of each square on the map. */
// BYTE dLight[MAXDUNX][MAXDUNY];
/** Specifies the (runtime) flags of each square on the map (dflag) */
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

void InitLvlDungeon()
{
	uint16_t bv;
	size_t dwSubtiles;
	BYTE *pTmp;
	const LevelData* lds = &AllLevels[currLvl._dLevelIdx];
	const LevelFileData* lfd = &levelfiledata[lds->dfindex];

	static_assert((int)WRPT_NONE == 0, "InitLvlDungeon fills pWarps with 0 instead of WRPT_NONE values.");
	memset(pWarps, 0, sizeof(pWarps));
	static_assert((int)SPT_NONE == 0, "InitLvlDungeon fills pSetPieces with 0 instead of SPT_NONE values.");
	memset(pSetPieces, 0, sizeof(pSetPieces));

	if (HasTileset) {
		return;
	}

	memset(pTiles, 0, sizeof(pTiles));
	if (lfd->dMegaTiles != NULL) { 
		LoadFileWithMem(lfd->dMegaTiles, (BYTE*)&pTiles[1][0]); // .TIL
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		for (int i = 1; i < lengthof(pTiles); i++) {
			for (bv = 0; bv < lengthof(pTiles[0]); bv++) {
				pTiles[i][bv] = SwapLE16(pTiles[i][bv]);
			}
		}
#endif
		for (int i = 1; i < lengthof(pTiles); i++) {
			for (bv = 0; bv < lengthof(pTiles[0]); bv++) {
				pTiles[i][bv] = pTiles[i][bv] + 1;
			}
		}
	}
	LoadFileWithMem(lfd->dSpecFlags, &nSpecTrapTable[0]); // .SPT
	static_assert(false == 0, "InitLvlDungeon fills tables with 0 instead of false values.");
	memset(nBlockTable, 0, sizeof(nBlockTable));
	memset(nSolidTable, 0, sizeof(nSolidTable));
	memset(nMissileTable, 0, sizeof(nMissileTable));
	assert(pSolidTbl == NULL);
	pSolidTbl = LoadFileInMem(lfd->dSolidTable, &dwSubtiles); // .SOL
	if (pSolidTbl != NULL) {
		assert(dwSubtiles <= MAXSUBTILES);
		pTmp = pSolidTbl;

		// dpiece 0 is always black/void -> make it non-passable to reduce the necessary checks
		// no longer necessary, because dPiece is never zero
		//nSolidTable[0] = true;

		for (unsigned i = 1; i <= dwSubtiles; i++) {
			bv = *pTmp++;
			nSolidTable[i] = (bv & PFLAG_BLOCK_PATH) != 0;
			nBlockTable[i] = (bv & PFLAG_BLOCK_LIGHT) != 0;
			nMissileTable[i] = (bv & PFLAG_BLOCK_MISSILE) != 0;
		}
	}

	switch (currLvl._dType) {
	case DTYPE_TOWN:
		// patch dSolidTable - Town.SOL
		// nSolidTable[553] = false; // allow walking on the left side of the pot at Adria
		// nSolidTable[761] = true;  // make the tile of the southern window of the church non-walkable
		// nSolidTable[945] = true;  // make the eastern side of Griswold's house consistent (non-walkable)
		// patch dMicroCels - TOWN.CEL
		// - overwrite subtile 237 with subtile 402 to make the inner tile of Griswold's house non-walkable
		// pSubtiles[237][0] = pSubtiles[402][0];
		// pSubtiles[237][1] = pSubtiles[402][1];
		break;
	case DTYPE_CATHEDRAL:
		// patch dSolidTable - L1.SOL
		nMissileTable[8] = false; // the only column which was blocking missiles
		// adjust SOL after fixCathedralShadows
		nSolidTable[298] = true;
		nSolidTable[304] = true;
		nBlockTable[334] = false;
		nMissileTable[334] = false;
		// - special subtiles for the banner setpiece
		nBlockTable[336] = false;
		nMissileTable[336] = false;
		nBlockTable[337] = false;
		nMissileTable[337] = false;
		nBlockTable[338] = false;
		nMissileTable[338] = false;
		// - special subtile for the vile setmap
		nMissileTable[335] = false;
		// create the new shadows
		// - use the shadows created by fixCathedralShadows
		pTiles[131][0] = pTiles[140][0];
		pTiles[131][1] = pTiles[140][1];
		pTiles[131][2] = pTiles[140][2];
		pTiles[131][3] = pTiles[140][3];
		pTiles[132][0] = pTiles[139][0];
		pTiles[132][1] = pTiles[139][1];
		pTiles[132][2] = pTiles[139][2];
		pTiles[132][3] = pTiles[139][3];
		pTiles[145][0] = pTiles[147][0];
		pTiles[145][1] = pTiles[147][1];
		pTiles[145][2] = pTiles[147][2];
		pTiles[145][3] = pTiles[147][3];
		pTiles[147][0] = pTiles[6][0];
		pTiles[147][1] = pTiles[6][1];
		pTiles[147][2] = pTiles[6][2];
		pTiles[147][3] = pTiles[6][3];
		pTiles[150][0] = pTiles[148][0];
		pTiles[150][1] = pTiles[148][1];
		pTiles[150][2] = pTiles[148][2];
		pTiles[150][3] = pTiles[148][3];
		pTiles[151][0] = pTiles[149][0];
		pTiles[151][1] = pTiles[149][1];
		pTiles[151][2] = pTiles[149][2];
		pTiles[151][3] = pTiles[149][3];
		pTiles[152][0] = pTiles[36][0];
		pTiles[152][1] = pTiles[36][1];
		pTiles[152][2] = pTiles[36][2];
		pTiles[152][3] = pTiles[36][3];
		pTiles[153][0] = pTiles[36][0];
		pTiles[153][1] = pTiles[36][1];
		pTiles[153][2] = pTiles[36][2];
		pTiles[153][3] = pTiles[36][3];
		pTiles[154][0] = pTiles[7][0];
		pTiles[154][1] = pTiles[7][1];
		pTiles[154][2] = pTiles[7][2];
		pTiles[154][3] = pTiles[7][3];
		pTiles[155][0] = pTiles[7][0];
		pTiles[155][1] = pTiles[7][1];
		pTiles[155][2] = pTiles[7][2];
		pTiles[155][3] = pTiles[7][3];
		pTiles[156][0] = pTiles[26][0];
		pTiles[156][1] = pTiles[26][1];
		pTiles[156][2] = pTiles[26][2];
		pTiles[156][3] = pTiles[26][3];
		pTiles[157][0] = pTiles[35][0];
		pTiles[157][1] = pTiles[35][1];
		pTiles[157][2] = pTiles[35][2];
		pTiles[157][3] = pTiles[35][3];
		pTiles[158][0] = pTiles[4][0];
		pTiles[158][1] = pTiles[4][1];
		pTiles[158][2] = pTiles[4][2];
		pTiles[158][3] = pTiles[4][3];
		pTiles[159][0] = pTiles[144][0];
		pTiles[159][1] = pTiles[144][1];
		pTiles[159][2] = pTiles[144][2];
		pTiles[159][3] = pTiles[144][3];
		pTiles[160][0] = pTiles[14][0];
		pTiles[160][1] = pTiles[14][1];
		pTiles[160][2] = pTiles[14][2];
		pTiles[160][3] = pTiles[14][3];
		pTiles[161][0] = pTiles[37][0];
		pTiles[161][1] = pTiles[37][1];
		pTiles[161][2] = pTiles[37][2];
		pTiles[161][3] = pTiles[37][3];
		pTiles[164][0] = pTiles[144][0];
		pTiles[164][1] = pTiles[144][1];
		pTiles[164][2] = pTiles[144][2];
		pTiles[164][3] = pTiles[144][3];
		pTiles[165][0] = pTiles[139][0];
		pTiles[165][1] = pTiles[139][1];
		pTiles[165][2] = pTiles[139][2];
		pTiles[165][3] = pTiles[139][3];
		// - shadows for the banner setpiece
		pTiles[56][0] = pTiles[1][0];
		pTiles[56][1] = pTiles[1][1];
		pTiles[56][2] = pTiles[1][2];
		pTiles[56][3] = pTiles[1][3];
		pTiles[55][0] = pTiles[1][0];
		pTiles[55][1] = pTiles[1][1];
		pTiles[55][2] = pTiles[1][2];
		pTiles[55][3] = pTiles[1][3];
		pTiles[54][0] = pTiles[60][0];
		pTiles[54][1] = pTiles[60][1];
		pTiles[54][2] = pTiles[60][2];
		pTiles[54][3] = pTiles[60][3];
		pTiles[53][0] = pTiles[58][0];
		pTiles[53][1] = pTiles[58][1];
		pTiles[53][2] = pTiles[58][2];
		pTiles[53][3] = pTiles[58][3];
		// - shadows for the vile setmap
		pTiles[52][0] = pTiles[2][0];
		pTiles[52][1] = pTiles[2][1];
		pTiles[52][2] = pTiles[2][2];
		pTiles[52][3] = pTiles[2][3];
		pTiles[51][0] = pTiles[2][0];
		pTiles[51][1] = pTiles[2][1];
		pTiles[51][2] = pTiles[2][2];
		pTiles[51][3] = pTiles[2][3];
		pTiles[50][0] = pTiles[1][0];
		pTiles[50][1] = pTiles[1][1];
		pTiles[50][2] = pTiles[1][2];
		pTiles[50][3] = pTiles[1][3];
		pTiles[49][0] = pTiles[17][0];
		pTiles[49][1] = pTiles[17][1];
		pTiles[49][2] = pTiles[17][2];
		pTiles[49][3] = pTiles[17][3];
		pTiles[48][0] = pTiles[11][0];
		pTiles[48][1] = pTiles[11][1];
		pTiles[48][2] = pTiles[11][2];
		pTiles[48][3] = pTiles[11][3];
		pTiles[47][0] = pTiles[2][0];
		pTiles[47][1] = pTiles[2][1];
		pTiles[47][2] = pTiles[2][2];
		pTiles[47][3] = pTiles[2][3];
		pTiles[46][0] = pTiles[7][0];
		pTiles[46][1] = pTiles[7][1];
		pTiles[46][2] = pTiles[7][2];
		pTiles[46][3] = pTiles[7][3];
		break;
	case DTYPE_CATACOMBS:
		// patch dMegaTiles, dMiniTiles and dSolidTable - L2.TIL, L2.MIN, L2.SOL
		// create the new shadows
		// - horizontal door for a pillar
		pTiles[17][0] = 540; // TODO: use 17 and update DRLG_L2DoorSubs?
		pTiles[17][1] = 18;
		pTiles[17][2] = 155;
		pTiles[17][3] = 162;
		// - horizontal hallway for a pillar
		pTiles[18][0] = 553;
		pTiles[18][1] = 99;
		pTiles[18][2] = 155;
		pTiles[18][3] = 162;
		// - pillar tile for a pillar
		pTiles[34][0] = 21;
		pTiles[34][1] = 26;
		pTiles[34][2] = 148;
		pTiles[34][3] = 169;
		// - vertical wall end for a horizontal arch
		pTiles[35][0] = 25;
		pTiles[35][1] = 26;
		pTiles[35][2] = 512;
		pTiles[35][3] = 162;
		// - horizontal wall end for a pillar
		pTiles[36][0] = 33;
		pTiles[36][1] = 34;
		pTiles[36][2] = 148;
		pTiles[36][3] = 162;
		// - horizontal wall end for a horizontal arch
		pTiles[37][0] = 33; // 268
		pTiles[37][1] = 515;
		pTiles[37][2] = 148;
		pTiles[37][3] = 169;
		// - floor tile with vertical arch
		pTiles[44][0] = 150;
		pTiles[44][1] = 10;
		pTiles[44][2] = 153;
		pTiles[44][3] = 12;
		// - floor tile with shadow of a vertical arch + horizontal arch
		pTiles[46][0] = 9;   // 150
		pTiles[46][1] = 154;
		pTiles[46][2] = 161; // '151'
		pTiles[46][3] = 162; // 152
		// - floor tile with shadow of a pillar + vertical arch
		pTiles[47][0] = 9;   // 153
		// pTiles[47][1] = 154;
		// pTiles[47][2] = 155;
		pTiles[47][3] = 162; // 156
		// - floor tile with shadow of a pillar
		// pTiles[48][0] = 9;
		// pTiles[48][1] = 10;
		pTiles[48][2] = 155; // 157
		pTiles[48][3] = 162; // 158
		// - floor tile with shadow of a horizontal arch
		pTiles[49][0] = 9;   // 159
		pTiles[49][1] = 10;  // 160
		// pTiles[49][2] = 161;
		// pTiles[49][3] = 162;
		// - floor tile with shadow(45) with vertical arch
		pTiles[95][0] = 158;
		pTiles[95][1] = 165;
		pTiles[95][2] = 155;
		pTiles[95][3] = 162;
		// - floor tile with shadow(49) with vertical arch
		pTiles[96][0] = 150;
		pTiles[96][1] = 10;
		pTiles[96][2] = 156;
		pTiles[96][3] = 162;
		// - floor tile with shadow(51) with horizontal arch
		pTiles[100][0] = 158;
		pTiles[100][1] = 165;
		pTiles[100][2] = 155;
		pTiles[100][3] = 169;
		// fix the upstairs
		// - make the back of the stairs non-walkable
		pTiles[72][1] = 56;
		nSolidTable[252] = true;
		nBlockTable[252] = true;
		nMissileTable[252] = true;
		// - make the stair-floor non light-blocker
		nBlockTable[267] = false;
		nBlockTable[559] = false;
		break;
	case DTYPE_CAVES:
		// patch dSolidTable - L3.SOL
		nSolidTable[249] = false; // sync tile 68 and 69 by making subtile 249 of tile 68 walkable.
		nBlockTable[146] = false; // fix unreasonable light-blocker
		nBlockTable[150] = false; // fix unreasonable light-blocker
		// fix fence subtiles
		nSolidTable[474] = false;
		nSolidTable[479] = false;
		nSolidTable[487] = false; // unused after patch
		nSolidTable[488] = true;
		nSolidTable[540] = false; // unused in base game
		// create new fences
		pTiles[144][0] = 505;
		pTiles[144][1] = 25;
		pTiles[144][2] = 516;
		pTiles[144][3] = 520;
		pTiles[145][0] = 505;
		pTiles[145][1] = 519;
		pTiles[145][2] = 202;
		pTiles[145][3] = 547;
		break;
	case DTYPE_HELL:
		// patch dSolidTable - L4.SOL
		nMissileTable[141] = false; // fix missile-blocking tile of down-stairs.
		nMissileTable[137] = false; // fix missile-blocking tile of down-stairs.
		nSolidTable[137] = false;   // fix non-walkable tile of down-stairs. - causes a graphic glitch, but keep in sync with patch users
		nSolidTable[130] = true;    // make the inner tiles of the down-stairs non-walkable I.
		nSolidTable[132] = true;    // make the inner tiles of the down-stairs non-walkable II.
		nSolidTable[131] = true;    // make the inner tiles of the down-stairs non-walkable III.
		// fix all-blocking tile on the diablo-level
		nSolidTable[211] = false;
		nMissileTable[211] = false;
		nBlockTable[211] = false;
		// new shadow-types
		pTiles[76][0] = 5;
		pTiles[76][1] = 6;
		pTiles[76][2] = 238; // 35
		pTiles[76][3] = 239;
		pTiles[129][0] = 5;
		pTiles[129][1] = 6;
		pTiles[129][2] = 7;
		pTiles[129][3] = 176;
		pTiles[130][0] = 41;
		pTiles[130][1] = 31;
		pTiles[130][2] = 10; // 13
		pTiles[130][3] = 239;
		pTiles[62][0] = 41;
		pTiles[62][1] = 31;
		pTiles[62][2] = 10;
		pTiles[62][3] = 176;
		pTiles[131][0] = 177;
		pTiles[131][1] = 31;
		pTiles[131][2] = 179; // 37
		pTiles[131][3] = 239;
		pTiles[61][0] = 177;
		pTiles[61][1] = 31;
		pTiles[61][2] = 179;
		pTiles[61][3] = 176;
		pTiles[132][0] = 24;
		pTiles[132][1] = 25;
		pTiles[132][2] = 10; // 13
		pTiles[132][3] = 239;
		pTiles[133][0] = 24;
		pTiles[133][1] = 25;
		pTiles[133][2] = 10;
		pTiles[133][3] = 176;
		pTiles[134][0] = 38;
		pTiles[134][1] = 31;
		pTiles[134][2] = 16; // 26
		pTiles[134][3] = 239;
		pTiles[135][0] = 38;
		pTiles[135][1] = 31;
		pTiles[135][2] = 16;
		pTiles[135][3] = 176;
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
		// - adjust SOL after cleanupCrypt
		nSolidTable[238] = false;
		nMissileTable[238] = false;
		nBlockTable[238] = false;
		nMissileTable[178] = false;
		nBlockTable[178] = false;
		nSolidTable[242] = false;
		nMissileTable[242] = false;
		nBlockTable[242] = false;
		// - fix automap of the entrance I.
		nMissileTable[158] = false;
		nBlockTable[158] = false;
		nSolidTable[159] = false;
		nMissileTable[159] = false;
		nBlockTable[159] = false;
		nMissileTable[148] = true;
		nBlockTable[148] = true;
		nSolidTable[148] = true;
		// patch dMegaTiles - L5.TIL
		// - fix automap of the entrance II.
		pTiles[52][0] = pTiles[23][0];
		pTiles[52][1] = pTiles[23][1];
		pTiles[52][2] = pTiles[23][2];
		pTiles[52][3] = pTiles[23][3];
		pTiles[58][0] = pTiles[18][0];
		pTiles[58][1] = pTiles[18][1];
		pTiles[58][2] = pTiles[18][2];
		pTiles[58][3] = pTiles[18][3];
		// - adjust TIL after patchCryptMin
		pTiles[109][0] = pTiles[1][0];
		pTiles[109][1] = pTiles[1][1];
		pTiles[109][2] = pTiles[1][2];
		pTiles[109][3] = pTiles[1][3];
		pTiles[110][0] = pTiles[6][0];
		pTiles[110][1] = pTiles[6][1];
		pTiles[110][2] = pTiles[6][2];
		pTiles[110][3] = pTiles[6][3];
		pTiles[111][0] = pTiles[11][0];
		pTiles[111][1] = pTiles[11][1];
		pTiles[111][2] = pTiles[11][2];
		pTiles[111][3] = pTiles[11][3];

		pTiles[71][0] = pTiles[2][0];
		pTiles[71][1] = pTiles[2][1];
		pTiles[71][2] = pTiles[2][2];
		pTiles[71][3] = pTiles[2][3];
		pTiles[80][0] = pTiles[2][0];
		pTiles[80][1] = pTiles[2][1];
		pTiles[80][2] = pTiles[2][2];
		pTiles[80][3] = pTiles[2][3];
		pTiles[81][0] = pTiles[12][0];
		pTiles[81][1] = pTiles[12][1];
		pTiles[81][2] = pTiles[12][2];
		pTiles[81][3] = pTiles[12][3];
		pTiles[82][0] = pTiles[12][0];
		pTiles[82][1] = pTiles[12][1];
		pTiles[82][2] = pTiles[12][2];
		pTiles[82][3] = pTiles[12][3];
		pTiles[83][0] = pTiles[36][0];
		pTiles[83][1] = pTiles[36][1];
		pTiles[83][2] = pTiles[36][2];
		pTiles[83][3] = pTiles[36][3];
		pTiles[84][0] = pTiles[36][0];
		pTiles[84][1] = pTiles[36][1];
		pTiles[84][2] = pTiles[36][2];
		pTiles[84][3] = pTiles[36][3];
		pTiles[85][0] = pTiles[7][0];
		pTiles[85][1] = pTiles[7][1];
		pTiles[85][2] = pTiles[7][2];
		pTiles[85][3] = pTiles[7][3];
		pTiles[86][0] = pTiles[7][0];
		pTiles[86][1] = pTiles[7][1];
		pTiles[86][2] = pTiles[7][2];
		pTiles[86][3] = pTiles[7][3];
		pTiles[87][0] = pTiles[26][0];
		pTiles[87][1] = pTiles[26][1];
		pTiles[87][2] = pTiles[26][2];
		pTiles[87][3] = pTiles[26][3];
		pTiles[88][0] = pTiles[26][0];
		pTiles[88][1] = pTiles[26][1];
		pTiles[88][2] = pTiles[26][2];
		pTiles[88][3] = pTiles[26][3];
		pTiles[215][0] = pTiles[35][0];
		pTiles[215][1] = pTiles[35][1];
		pTiles[215][2] = pTiles[35][2];
		pTiles[215][3] = pTiles[35][3];
		pTiles[216][0] = pTiles[11][0];
		pTiles[216][1] = pTiles[11][1];
		pTiles[216][2] = pTiles[11][2];
		pTiles[216][3] = pTiles[11][3];
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
	uint16_t* pTile;

	/*int cursor = 0;
	char tmpstr[1024];
	long lvs[] = { 22, 56, 57, 58, 59, 60, 61 };
	for (i = 0; i < lengthof(lvs); i++) {
		lv = lvs[i];
		pTile = &pTiles[mt][0];
		v1 = pTile[0];
		v2 = pTile[1];
		v3 = pTile[2];
		v4 = pTile[3];
		cat_str(tmpstr, cursor, "- %d: %d, %d, %d, %d", lv, v1, v2, v3, v4);
	}
	app_fatal(tmpstr);*/

	pTile = &pTiles[mt][0];
	v1 = pTile[0];
	v2 = pTile[1];
	v3 = pTile[2];
	v4 = pTile[3];

	for (j = 0; j < MAXDUNY; j += 2) {
		for (i = 0; i < MAXDUNX; i += 2) {
			dPiece[i][j] = v1;
			dPiece[i + 1][j] = v2;
			dPiece[i][j + 1] = v3;
			dPiece[i + 1][j + 1] = v4;
		}
	}

    int et = -1;
    switch (currLvl._dType) {
    case DTYPE_CATHEDRAL: et = 74; break;
    case DTYPE_CATACOMBS: et = 2;  break;
    case DTYPE_CAVES:     et = 2;  break;
    case DTYPE_HELL:      et = 14;  break;
#ifdef  HELLFIRE
    case DTYPE_CRYPT:     et = 8;  break;
    case DTYPE_NEST:      et = 10; break;
#endif //  HELLFIRE
    }
	yy = DBORDERY;
	for (j = 0; j < DMAXY; j++) {
		xx = DBORDERX;
		for (i = 0; i < DMAXX; i++) {
			mt = dungeon[i][j];
			if (mt <= 0) {
				dProgressErr() << QString("Missing tile at %1:%2 .. %3:%4").arg(i).arg(j).arg(xx).arg(yy);
				xx += 2;
				continue;
			}
			assert(mt > 0);
			pTile = &pTiles[mt][0];
			v1 = pTile[0];
			v2 = pTile[1];
			v3 = pTile[2];
			v4 = pTile[3];
            if (v1 == et) {
                dProgressErr() << QString("Missing subtile at %1:%2 .. %3:%4").arg(i).arg(j).arg(xx).arg(yy);
            }
			dPiece[xx][yy] = v1;
			dPiece[xx + 1][yy] = v2;
			dPiece[xx][yy + 1] = v3;
			dPiece[xx + 1][yy + 1] = v4;
			xx += 2;
		}
		yy += 2;
	}
}

void DRLG_DrawMiniSet(const BYTE* miniset, int sx, int sy)
{
	int xx, yy, sh, sw, ii;

	sw = miniset[0];
	sh = miniset[1];

	ii = sw * sh + 2;

	for (yy = sy; yy < sy + sh; yy++) {
		for (xx = sx; xx < sx + sw; xx++) {
			if (miniset[ii] != 0) {
				dungeon[xx][yy] = miniset[ii];
			}
			ii++;
		}
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
	//memset(TransList, 0, sizeof(TransList)); - LoadGame() needs this preserved
	numtrans = 1;
	gbDoTransVals = false;
}

/*void DRLG_MRectTrans(int x1, int y1, int x2, int y2, int tv)
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
}*/

/*void DRLG_RectTrans(int x1, int y1, int x2, int y2)
{
	int i, j;

	for (i = x1; i <= x2; i++) {
		for (j = y1; j <= y2; j++) {
			dTransVal[i][j] = numtrans;
		}
	}
	numtrans++;
}*/

/*void DRLG_ListTrans(int num, const BYTE* List)
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
}*/

static void DRLG_FTVR(unsigned offset)
{
	BYTE *tvp = &dTransVal[0][0];
	if (tvp[offset] != 0) {
		return;
	}
	tvp[offset] = numtrans;

	BYTE *tdp = &drlg.transDirMap[0][0];
	if (tdp[offset] & (1 << 0)) { // DIR_SE
		DRLG_FTVR(offset + 1);
	}
	if (tdp[offset] & (1 << 1)) { // DIR_NW
		DRLG_FTVR(offset - 1);
	}
	if (tdp[offset] & (1 << 2)) { // DIR_N
		DRLG_FTVR(offset - 1 - DSIZEY);
	}
	if (tdp[offset] & (1 << 3)) { // DIR_NE
		DRLG_FTVR(offset - DSIZEY);
	}
	if (tdp[offset] & (1 << 4)) { // DIR_E
		DRLG_FTVR(offset + 1 - DSIZEY);
	}
	if (tdp[offset] & (1 << 5)) { // DIR_W
		DRLG_FTVR(offset - 1 + DSIZEY);
	}
	if (tdp[offset] & (1 << 6)) { // DIR_SW
		DRLG_FTVR(offset + DSIZEY);
	}
	if (tdp[offset] & (1 << 7)) { // DIR_S
		DRLG_FTVR(offset + DSIZEY + 1);
	}
}

void DRLG_FloodTVal(const BYTE *floorTypes)
{
	int i, j;
	BYTE *tdp = &drlg.transDirMap[0][0]; // Overlaps with transvalMap!
	BYTE *tvp = &dTransVal[0][0];

	DRLG_InitTrans();

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
			drlg.transDirMap[2 * i + 0][2 * j + 0] = tpm;
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
			drlg.transDirMap[2 * i + 0][2 * j + 1] = tpm;
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
			drlg.transDirMap[2 * i + 1][2 * j + 0] = tpm;
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
			drlg.transDirMap[2 * i + 1][2 * j + 1] = tpm;
		}
	}
	// create the rooms
	for (i = 0; i < DSIZEX * DSIZEY; i++) {
		if (tvp[i] != 0)
			continue;
		if (tdp[i] == 0)
			continue;
		DRLG_FTVR(i);
		numtrans++;
	}
	// move the values into position (add borders)
	static_assert(DBORDERY + DBORDERX * MAXDUNY > DSIZEY, "DRLG_FloodTVal requires large enough border(x) to use memcpy instead of memmove.");
	for (i = DSIZEX - 1; i >= 0; i--) {
		BYTE *tvpSrc = tvp + i * DSIZEY;
		BYTE *tvpDst = tvp + (i + DBORDERX) * MAXDUNY + DBORDERY;
		memcpy(tvpDst, tvpSrc, DSIZEY);
	}
	// clear the borders
	memset(tvp, 0, MAXDUNY * DBORDERX + DBORDERY);
	tvp += MAXDUNY * DBORDERX + DBORDERY + DSIZEY;
	while (tvp < (BYTE*)&dTransVal[0][0] + DSIZEX * DSIZEY) {
		static_assert(DBORDERX != 0, "DRLG_FloodTVal requires large enough border(x) to use merged memset.");
		memset(tvp, 0, 2 * DBORDERY);
		tvp += 2 * DBORDERY + DSIZEY;
	}
}

void DRLG_LoadSP(int idx, BYTE bv)
{
	int rx1, ry1, rw, rh, i, j;
	BYTE* sp;
	SetPieceStruct* pSetPiece = &pSetPieces[idx];

	rx1 = pSetPiece->_spx;
	ry1 = pSetPiece->_spy;
	rw = SwapLE16(*(uint16_t*)&pSetPiece->_spData[0]);
	rh = SwapLE16(*(uint16_t*)&pSetPiece->_spData[2]);
	sp = &pSetPiece->_spData[4];
	// load tiles
	for (j = ry1; j < ry1 + rh; j++) {
		for (i = rx1; i < rx1 + rw; i++) {
			dungeon[i][j] = *sp != 0 ? *sp : bv;
			sp += 2;
		}
	}
	// load flags
	for (j = ry1; j < ry1 + rh; j++) {
		for (i = rx1; i < rx1 + rw; i++) {
			static_assert((int)DRLG_PROTECTED == 1 << 6, "DRLG_LoadSP sets the protection flags with a simple bit-shift I.");
			static_assert((int)DRLG_FROZEN == 1 << 7, "DRLG_LoadSP sets the protection flags with a simple bit-shift II.");
			drlgFlags[i][j] |= (*sp & 3) << 6;
			sp += 2;
		}
	}
}

static void DRLG_InitFlags()
{
	BYTE c;

	c = currLvl._dType == DTYPE_TOWN ? BFLAG_VISIBLE : 0;
	memset(dFlags, c, sizeof(dFlags));

	if (!currLvl._dSetLvl) {
		for (int i = lengthof(pWarps) - 1; i >= 0; i--) {
			int tx = pWarps[i]._wx;
			int ty = pWarps[i]._wy;

			if (tx == 0) {
				continue;
			}
			int r = pWarps[i]._wtype == WRPT_L4_PENTA ? 4 : 2;
			tx -= r;
			ty -= r;
			r = 2 * r + 1;
			for (int xx = 0; xx < r; xx++) {
				for (int yy = 0; yy < r; yy++) {
					dFlags[tx + xx][ty + yy] |= BFLAG_MON_PROTECT | BFLAG_OBJ_PROTECT;
				}
			}
		}
	}

	for (int n = lengthof(pSetPieces) - 1; n >= 0; n--) {
		if (pSetPieces[n]._spData != NULL) { // pSetPieces[n]._sptype != SPT_NONE
			int x = pSetPieces[n]._spx;
			int y = pSetPieces[n]._spy;
			int w = SwapLE16(*(uint16_t*)&pSetPieces[n]._spData[0]);
			int h = SwapLE16(*(uint16_t*)&pSetPieces[n]._spData[2]);

			x = 2 * x + DBORDERX;
			y = 2 * y + DBORDERY;

			BYTE* sp = &pSetPieces[n]._spData[4];
			sp += 2 * w * h; // skip tiles

			sp++;
			for (int j = 0; j < h; j++) {
				for (int i = 0; i < w; i++) {
					BYTE flags = *sp;
					static_assert((1 << (BFLAG_MON_PROTECT_SHL + 1)) == (int)BFLAG_OBJ_PROTECT, "DRLG_SetPC uses bitshift to populate dFlags");
					dFlags[x + 2 * i][y + 2 * j] |= (flags & 3) << BFLAG_MON_PROTECT_SHL;
					flags >>= 2;
					dFlags[x + 2 * i + 1][y + 2 * j] |= (flags & 3) << BFLAG_MON_PROTECT_SHL;
					flags >>= 2;
					dFlags[x + 2 * i][y + 2 * j  + 1] |= (flags & 3) << BFLAG_MON_PROTECT_SHL;
					flags >>= 2;
					dFlags[x + 2 * i + 1][y + 2 * j + 1] |= (flags & 3) << BFLAG_MON_PROTECT_SHL;
					sp += 2;
				}
			}
		}
	}
}

static void DRLG_LightSubtiles()
{
	/*BYTE c;
	int i, j, pn;

	c = currLvl._dType == DTYPE_TOWN ? 0 : MAXDARKNESS;
#if DEBUG_MODE
	if (lightflag)
		c = 0;
#endif
	memset(dLight, c, sizeof(dLight));

	assert(LightList[MAXLIGHTS]._lxoff == 0);
	assert(LightList[MAXLIGHTS]._lyoff == 0);
	if (currLvl._dType == DTYPE_CAVES) {
		LightList[MAXLIGHTS]._lradius = 7;
		for (i = 0; i < MAXDUNX; i++) {
			for (j = 0; j < MAXDUNY; j++) {
				pn = dPiece[i][j];
				if (pn >= 56 && pn <= 161
				 && (pn <= 147 || pn >= 154 || pn == 150 || pn == 152)) {
					LightList[MAXLIGHTS]._lx = i;
					LightList[MAXLIGHTS]._ly = j;
					DoLighting(MAXLIGHTS);
				}
			}
		}
#ifdef HELLFIRE
	} else if (currLvl._dType == DTYPE_NEST) {
		LightList[MAXLIGHTS]._lradius = 6; // 9
		for (i = 0; i < MAXDUNX; i++) {
			for (j = 0; j < MAXDUNY; j++) {
				pn = dPiece[i][j];
				if ((pn >= 386 && pn <= 496) || (pn >= 534 && pn <= 537)) {
					LightList[MAXLIGHTS]._lx = i;
					LightList[MAXLIGHTS]._ly = j;
					DoLighting(MAXLIGHTS);
				}
			}
		}
#endif
	}*/
}

void InitLvlMap()
{
	DRLG_LightSubtiles();
	DRLG_InitFlags();

	// memset(dPlayer, 0, sizeof(dPlayer));
	memset(dMonster, 0, sizeof(dMonster));
	// memset(dDead, 0, sizeof(dDead));
	memset(dObject, 0, sizeof(dObject));
	memset(dItem, 0, sizeof(dItem));
	// memset(dMissile, 0, sizeof(dMissile));
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
static POS32 DRLG_FitThemeRoom(BYTE floor, int x, int y, int minSize, int maxSize)
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

static void DRLG_CreateThemeRoom(int themeIndex, const BYTE (&themeTiles)[NUM_DRT_TYPES])
{
	int xx, yy;
	const int x1 = themes[themeIndex]._tsx1;
	const int y1 = themes[themeIndex]._tsy1;
	const int x2 = themes[themeIndex]._tsx2;
	const int y2 = themes[themeIndex]._tsy2;
	BYTE v;

	// left/right side
	v = themeTiles[DRT_WALL_VERT];
	for (yy = y1; yy <= y2; yy++) {
		dungeon[x1][yy] = v;
		dungeon[x2][yy] = v;
	}
	// top/bottom line
	v = themeTiles[DRT_WALL_HORIZ];
	for (xx = x1 + 1; xx < x2; xx++) {
		dungeon[xx][y1] = v;
		dungeon[xx][y2] = v;
	}
	// inner tiles
	v = themeTiles[DRT_FLOOR];
	for (xx = x1 + 1; xx < x2; xx++) {
		for (yy = y1 + 1; yy < y2; yy++) {
			dungeon[xx][yy] = v;
		}
	}
	// corners
	dungeon[x1][y1] = themeTiles[DRT_TOP_LEFT];
	dungeon[x2][y1] = themeTiles[DRT_TOP_RIGHT];
	dungeon[x1][y2] = themeTiles[DRT_BOTTOM_LEFT];
	dungeon[x2][y2] = themeTiles[DRT_BOTTOM_RIGHT];

	// exits
	if (random_(0, 2) == 0) {
		dungeon[x2][(y1 + y2 + 1) / 2] = themeTiles[DRT_DOOR_VERT];
	} else {
		dungeon[(x1 + x2 + 1) / 2][y2] = themeTiles[DRT_DOOR_HORIZ];
	}
}

void DRLG_PlaceThemeRooms(int minSize, int maxSize, const BYTE (&themeTiles)[NUM_DRT_TYPES], int rndSkip, bool rndSize)
{
	int i, j;
	int min;

	for (i = 0; i < DMAXX; i++) {
		for (j = 0; j < DMAXY; j++) {
			// always start from a floor tile
			if (dungeon[i][j] != themeTiles[DRT_FLOOR]) {
				continue;
			}
			if (random_(0, 128) < rndSkip) {
				continue;
			}
			// check if there is enough space
			POS32 tArea = DRLG_FitThemeRoom(themeTiles[DRT_FLOOR], i, j, minSize, maxSize);
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
			if (!InThemeRoom(i + 1, j + 1)) {
				// create the room
				themes[numthemes]._tsx1 = i + 1;
				themes[numthemes]._tsy1 = j + 1;
				themes[numthemes]._tsx2 = i + 1 + tArea.x - 1;
				themes[numthemes]._tsy2 = j + 1 + tArea.y - 1;
				DRLG_CreateThemeRoom(numthemes, themeTiles);
				numthemes++;
				if (numthemes == lengthof(themes))
					return;
			}

			j += tArea.x + 2;
		}
	}
}

bool InThemeRoom(int x, int y)
{
	int i;

	for (i = numthemes - 1; i >= 0; i--) {
		if (x > themes[i]._tsx1 && x < themes[i]._tsx2
		 && y > themes[i]._tsy1 && y < themes[i]._tsy2)
			return true;
	}

	return false;
}

DEVILUTION_END_NAMESPACE
