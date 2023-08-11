/**
 * @file automap.cpp
 *
 * Implementation of the in-game map overlay.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

/* Maps from tile_id to automap type (_automap_types + _automap_flags). */
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
		// assert((automaptype[i] & MAF_TYPE) < 13); required by DrawAutomapTile and SetAutomapView
		lm++;
	}

	mem_free_dbg(pAFile);
	// patch dAutomapData - L1.AMP
	if (currLvl._dType == DTYPE_CATHEDRAL) {
		// separate pillar tile
		automaptype[28] = MWT_PILLAR; // automaptype[15]
		// new shadows
		// - shadows created by fixCathedralShadows
		automaptype[145] = automaptype[11];
		automaptype[147] = automaptype[6];
		automaptype[149] = automaptype[12];
		automaptype[150] = automaptype[2];
		automaptype[151] = automaptype[12];
		automaptype[152] = automaptype[36];
		automaptype[153] = automaptype[36];
		automaptype[154] = automaptype[7];
		automaptype[155] = automaptype[2];
		automaptype[156] = automaptype[26];
		automaptype[157] = automaptype[35];
		automaptype[159] = automaptype[13];
		automaptype[160] = automaptype[14];
		automaptype[161] = automaptype[37];
		automaptype[164] = automaptype[13];
		automaptype[165] = automaptype[13];
		// - shadows for the banner setpiece
		automaptype[56] = automaptype[1];
		automaptype[55] = automaptype[1];
		automaptype[54] = automaptype[60];
		automaptype[53] = automaptype[58];
		// - shadows for the vile setmap
		automaptype[52] = automaptype[2];
		automaptype[51] = automaptype[2];
		automaptype[50] = automaptype[1];
		automaptype[49] = automaptype[17];
		automaptype[48] = automaptype[11];
		automaptype[47] = automaptype[2];
		automaptype[46] = automaptype[7];
	}
	// patch dAutomapData - L2.AMP
	if (currLvl._dType == DTYPE_CATACOMBS) {
		// fix automap type
		automaptype[42] &= ~MAF_EAST_ARCH; // not a horizontal arch
		automaptype[156] = MWT_NONE; // no door is placed
		automaptype[157] = MWT_NONE;
		// separate pillar tile
		automaptype[52] = MWT_PILLAR;
		// new shadows
		automaptype[17] = automaptype[5];
		automaptype[34] = automaptype[6];
		automaptype[35] = automaptype[7];
		automaptype[36] = automaptype[9];
		automaptype[37] = automaptype[9];
		automaptype[101] = MWT_PILLAR;
	}
	// patch dAutomapData - L3.AMP
	if (currLvl._dType == DTYPE_CAVES) {
		// new shadows
		automaptype[144] = automaptype[151];
		automaptype[145] = automaptype[152];
	}
	// patch dAutomapData - L4.AMP
	if (currLvl._dType == DTYPE_HELL) {
		// fix automap types
		automaptype[27] = MAF_EXTERN | MWT_NORTH_EAST_END;
		automaptype[28] = MAF_EXTERN | MWT_NORTH_WEST_END;
		automaptype[52] |= MAF_WEST_GRATE;
		automaptype[56] |= MAF_EAST_GRATE;
		automaptype[7] = MWT_NORTH_WEST_END;
		automaptype[8] = MWT_NORTH_EAST_END;
		automaptype[83] = MWT_NORTH_WEST_END;
		// new shadow-types
		automaptype[61] = automaptype[2];
		automaptype[62] = automaptype[2];
		automaptype[76] = automaptype[15];
		automaptype[129] = automaptype[15];
		automaptype[130] = automaptype[56];
		automaptype[131] = automaptype[56];
		automaptype[132] = automaptype[8];
		automaptype[133] = automaptype[8];
		automaptype[134] = automaptype[14];
		automaptype[135] = automaptype[14];
	}
#ifdef HELLFIRE
	// patch dAutomapData - L5.AMP
	if (currLvl._dType == DTYPE_CRYPT) {
		// fix automap types
		automaptype[20] = MAF_EXTERN | MWT_CORNER;
		automaptype[23] = MAF_EXTERN | MWT_NORTH_WEST_END;
		automaptype[24] = MAF_EXTERN | MWT_NORTH_EAST_END;
		// fix automap of the entrance
		automaptype[47] = MAF_STAIRS | MWT_NORTH_WEST;
		automaptype[50] = MWT_NORTH_WEST;
		automaptype[48] = MAF_STAIRS | MWT_NORTH;
		automaptype[51] = MWT_NORTH_WEST_END;
		automaptype[52] = MAF_EXTERN;
		automaptype[53] = MAF_STAIRS | MWT_NORTH;
		automaptype[54] = MAF_EXTERN;
		automaptype[56] = MWT_NONE;
		automaptype[58] = MAF_EXTERN | MWT_NORTH_WEST_END;
		// separate pillar tile
		automaptype[28] = MWT_PILLAR; // automaptype[15]
		// new shadows
		// - shadows created by fixCryptShadows
		automaptype[109] = MWT_NORTH_WEST;
		automaptype[110] = MWT_NORTH_WEST;
		automaptype[111] = MAF_WEST_ARCH | MWT_NORTH_WEST;
		automaptype[215] = MAF_WEST_GRATE | MWT_NORTH_WEST;
		// - 'add' new shadow-types with glow
		automaptype[216] = MAF_WEST_ARCH | MWT_NORTH_WEST;
		// - 'add' new shadow-types with horizontal arches
		automaptype[71] = MWT_NORTH_EAST;
		automaptype[80] = MWT_NORTH_EAST;
		automaptype[81] = MAF_EAST_ARCH | MWT_NORTH_EAST;
		automaptype[82] = MAF_EAST_ARCH | MWT_NORTH_EAST;
		automaptype[83] = MAF_EAST_GRATE | MWT_NORTH_EAST;
		automaptype[84] = MAF_EAST_GRATE | MWT_NORTH_EAST;
		automaptype[85] = MWT_NORTH_EAST;
		automaptype[86] = MWT_NORTH_EAST;
		automaptype[87] = MAF_EAST_DOOR | MWT_NORTH_EAST;
		automaptype[88] = MAF_EAST_DOOR | MWT_NORTH_EAST;
	}
#endif // HELLFIRE
}

DEVILUTION_END_NAMESPACE
