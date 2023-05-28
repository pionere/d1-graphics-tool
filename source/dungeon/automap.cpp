/**
 * @file automap.cpp
 *
 * Implementation of the in-game map overlay.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

/**
 * Maps from tile_id to automap type.
 */
uint16_t automaptype[MAXTILES + 1];

/**
 * @brief Initializes the automap of a dungeon level.
 *  1. Loads the mapping between tile IDs and automap shapes.
 *  2. Resets the offsets.
 */
void InitLvlAutomap()
{
	size_t dwTiles, i;
	BYTE* pAFile;
	uint16_t* lm;
	const char* mapData;

	/* commented out because the flags are reset in gendung.cpp anyway
	static_assert(sizeof(dFlags) == MAXDUNX * MAXDUNY, "Linear traverse of dFlags does not work in InitAutomap.");
	pTmp = &dFlags[0][0];
	for (i = 0; i < MAXDUNX * MAXDUNY; i++, pTmp++) {
		assert((*pTmp & BFLAG_EXPLORED) == 0);
		*pTmp &= ~BFLAG_EXPLORED;
	}*/

	if (HasTileset) {
		return;
	}

	mapData = levelfiledata[AllLevels[currLvl._dLevelIdx].dfindex].dAutomapData;
	bool _gbAutomapData = mapData != NULL;
	if (!_gbAutomapData) {
		memset(automaptype, 0, sizeof(automaptype));
		return;
	}

	pAFile = LoadFileInMem(mapData, &dwTiles);

	dwTiles /= 2;
	assert(dwTiles < (size_t)lengthof(automaptype));

	lm = (uint16_t*)pAFile;
	for (i = 1; i <= dwTiles; i++) {
		automaptype[i] = SwapLE16(*lm);
		// assert((automaptype[i] & MAPFLAG_TYPE) < 13); required by DrawAutomapTile and SetAutomapView
		lm++;
	}

	mem_free_dbg(pAFile);
	// patch dAutomapData - L2.AMP
	if (currLvl._dType == DTYPE_CATACOMBS) {
		automaptype[42] &= ~MAPFLAG_HORZARCH;
		automaptype[156] &= ~(MAPFLAG_VERTDOOR | MAPFLAG_TYPE);
		automaptype[157] &= ~(MAPFLAG_HORZDOOR | MAPFLAG_TYPE);
	}
	// patch dAutomapData - L4.AMP
	if (currLvl._dType == DTYPE_HELL) {
		automaptype[52] |= MAPFLAG_VERTGRATE;
		automaptype[56] |= MAPFLAG_HORZGRATE;
	}
#ifdef HELLFIRE
	// patch dAutomapData - L5.AMP
	if (currLvl._dType == DTYPE_CRYPT) {
		// fix automap of the entrance
		automaptype[52] = MAPFLAG_DIRT;
		automaptype[53] = MAPFLAG_STAIRS | 4;
		automaptype[54] = MAPFLAG_DIRT;
		automaptype[56] = 0;
		automaptype[58] = MAPFLAG_DIRT | 5;
		// adjust AMP after cleanupCrypt
		// - use the shadows created by fixCryptShadows
		automaptype[109] = 2;
		automaptype[110] = 2;
		automaptype[111] = MAPFLAG_VERTARCH | 2;
		automaptype[215] = MAPFLAG_VERTGRATE | 2;
		// - 'add' new shadow-types with glow
		automaptype[216] = MAPFLAG_VERTARCH | 2;
		// - 'add' new shadow-types with horizontal arches
		automaptype[71] = 3;
		automaptype[80] = 3;
		automaptype[81] = MAPFLAG_HORZARCH | 3;
		automaptype[82] = MAPFLAG_HORZARCH | 3;
		automaptype[83] = MAPFLAG_HORZGRATE | 3;
		automaptype[84] = MAPFLAG_HORZGRATE | 3;
		automaptype[85] = 3;
		automaptype[86] = 3;
		automaptype[87] = MAPFLAG_HORZDOOR | 3;
		automaptype[88] = MAPFLAG_HORZDOOR | 3;
	}
#endif // HELLFIRE
}

DEVILUTION_END_NAMESPACE
