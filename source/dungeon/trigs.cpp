/**
 * @file trigs.cpp
 *
 * Implementation of functionality for triggering events when the player enters an area.
 */
#include "all.h"

int numtrigs;
TriggerStruct trigs[MAXTRIGGERS];

#define PIECE dPiece[pcurspos.x][pcurspos.y]
/** Specifies the dungeon piece IDs which constitute stairways leading down to the cathedral from town. */
#define TOWN_L1_WARP     (PIECE == 716 || (PIECE >= 723 && PIECE <= 728))
/** Specifies the dungeon piece IDs which constitute stairways leading down to the catacombs from town. */
#define TOWN_L2_WARP     (PIECE >= 1175 && PIECE <= 1178)
/** Specifies the dungeon piece IDs which constitute stairways leading down to the caves from town. */
#define TOWN_L3_WARP     (PIECE >= 1199 && PIECE <= 1220)
/** Specifies the dungeon piece IDs which constitute stairways leading down to hell from town. */
#define TOWN_L4_WARP     (PIECE >= 1240 && PIECE <= 1255)
/** Specifies the dungeon piece IDs which constitute stairways leading down to the hive from town. */
#define TOWN_L5_WARP     (PIECE >= 1307 && PIECE <= 1310)
/** Specifies the dungeon piece IDs which constitute stairways leading down to the crypt from town. */
#define TOWN_L6_WARP     (PIECE >= 1331 && PIECE <= 1338)
/** Specifies the dungeon piece IDs which constitute stairways leading up from the cathedral. */
#define L1_UP_WARP       (PIECE >= 129 && PIECE <= 140 && PIECE != 134 && PIECE != 136)
/** Specifies the dungeon piece IDs which constitute stairways leading down from the cathedral. */
//							{ 106, 107, 108, 109, 110, /*111,*/ 112, /*113,*/ 114, 115, /*116, 117,*/ 118, }
#define L1_DOWN_WARP     ((PIECE >= 106 && PIECE <= 115 && PIECE != 111 && PIECE != 113) || PIECE == 118)
/** Specifies the dungeon piece IDs which constitute stairways leading up from the catacombs. */
#define L2_UP_WARP       (PIECE == 266 || PIECE == 267)
/** Specifies the dungeon piece IDs which constitute stairways leading down from the catacombs. */
#define L2_DOWN_WARP     (PIECE >= 269 && PIECE <= 272)
/** Specifies the dungeon piece IDs which constitute stairways leading up to town from the catacombs. */
#define L2_TOWN_WARP     (PIECE == 558 || PIECE == 559)
/** Specifies the dungeon piece IDs which constitute stairways leading up from the caves. */
#define L3_UP_WARP       (PIECE == 170 || PIECE == 171)
#define L3_UP_WARPx(x)   (x == 170 || x == 171)
/** Specifies the dungeon piece IDs which constitute stairways leading down from the caves. */
#define L3_DOWN_WARP     (PIECE == 168)
#define L3_DOWN_WARPx(x) (x == 168)
/** Specifies the dungeon piece IDs which constitute stairways leading up to town from the caves. */
//#define L3_TOWN_WARP     (PIECE == 548 || PIECE == 549 || PIECE == 559 || PIECE == 560)
#define L3_TOWN_WARP     (PIECE == 548 || PIECE == 549)
#define L3_TOWN_WARPx(x) (x == 548 || x == 549)
/** Specifies the dungeon piece IDs which constitute stairways leading up from hell. */
#define L4_UP_WARP       (PIECE == 82 || (PIECE >= 90 && PIECE <= 97 && PIECE != 91 && PIECE != 93))
/** Specifies the dungeon piece IDs which constitute stairways leading down from hell. */
#define L4_DOWN_WARP     ((PIECE >= 130 && PIECE <= 133) || PIECE == 120)
/** Specifies the dungeon piece IDs which constitute stairways leading up to town from hell. */
#define L4_TOWN_WARP     (PIECE == 421 || (PIECE >= 429 && PIECE <= 436 && PIECE != 430 && PIECE != 432))
/** Specifies the dungeon piece IDs which constitute stairways leading down to Diablo from hell. */
#define L4_PENTA_WARP    (PIECE >= 353 && PIECE <= 384)
#ifdef HELLFIRE
/** Specifies the dungeon piece IDs which constitute stairways leading up to town from crypt. */
//#define L5_TOWN_WARP		(PIECE >= 172 && PIECE <= 185 && (PIECE <= 179 || PIECE >= 184))
/** Specifies the dungeon piece IDs which constitute stairways leading up from crypt. */
#define L5_UP_WARP       (PIECE >= 149 && PIECE <= 159 && (PIECE <= 153 || PIECE >= 158))
/** Specifies the dungeon piece IDs which constitute stairways leading down from crypt. */
#define L5_DOWN_WARP     (PIECE >= 125 && PIECE <= 126)
#define L5_DOWN_WARPx(x) (x >= 125 && x <= 126)
/** Specifies the dungeon piece IDs which constitute stairways leading up to town from nest. */
//#define L6_TOWN_WARP     (PIECE >= 79 && PIECE <= 80)
//#define L6_TOWN_WARPx(x) (x >= 79 && x <= 80)
/** Specifies the dungeon piece IDs which constitute stairways leading up from nest. */
#define L6_UP_WARP       (PIECE >= 65 && PIECE <= 66)
#define L6_UP_WARPx(x)   (x >= 65 && x <= 66)
/** Specifies the dungeon piece IDs which constitute stairways leading down from nest. */
#define L6_DOWN_WARPx(x) (x == 61 || x == 63)
#endif

static void InitNoTriggers()
{
	numtrigs = 0;
}

static void InitL1Triggers()
{
	int i, j;

	numtrigs = 0;
	/*for (j = 0; j < MAXDUNY; j++) {
		for (i = 0; i < MAXDUNX; i++) {
			if (dPiece[i][j] == 129) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = currLvl._dLevelIdx == DLV_CATHEDRAL1 ? DVL_DWM_TWARPUP : DVL_DWM_PREVLVL;
				numtrigs++;
			} else if (dPiece[i][j] == 115) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
				numtrigs++;
			}
		}
	}*/
	// if (pWarps[DWARP_ENTRY]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_ENTRY]._wx + DBORDERX;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_ENTRY]._wy + DBORDERY;
		trigs[numtrigs]._tmsg = currLvl._dLevelIdx == DLV_CATHEDRAL1 ? DVL_DWM_TWARPUP : DVL_DWM_PREVLVL;
		numtrigs++;
	// }
	// if (pWarps[DWARP_EXIT]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_EXIT]._wx + DBORDERX + 1;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_EXIT]._wy + DBORDERY;
		trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
		numtrigs++;
	// }
	if (pWarps[DWARP_SIDE]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_SIDE]._wx + DBORDERX;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_SIDE]._wy + DBORDERY;
		if (currLvl._dLevelIdx == questlist[Q_SKELKING]._qdlvl) { // TODO: add qn to pWarps?
			trigs[numtrigs]._tlvl = questlist[Q_SKELKING]._qslvl;
			trigs[numtrigs]._tx += 1;
		} else {
			trigs[numtrigs]._tlvl = questlist[Q_PWATER]._qslvl;
			trigs[numtrigs]._ty += 1;
		}
		trigs[numtrigs]._tmsg = DVL_DWM_SETLVL;
		numtrigs++;
	}
}

static void InitL2Triggers()
{
	int i, j;

	numtrigs = 0;
	/*for (j = 0; j < MAXDUNY; j++) {
		for (i = 0; i < MAXDUNX; i++) {
			if (dPiece[i][j] == 267 && (quests[Q_BCHAMB]._qactive == QUEST_NOTAVAIL || abs(quests[Q_BCHAMB]._qtx - i) > 1 || abs(quests[Q_BCHAMB]._qty - j) > 1)) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_PREVLVL;
				numtrigs++;
			} else if (dPiece[i][j] == 559) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_TWARPUP;
				numtrigs++;
			} else if (dPiece[i][j] == 271) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
				numtrigs++;
			}
		}
	}*/
	// if (pWarps[DWARP_ENTRY]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_ENTRY]._wx + DBORDERX;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_ENTRY]._wy + DBORDERY + 1;
		trigs[numtrigs]._tmsg = DVL_DWM_PREVLVL;
		numtrigs++;
	// }
	// if (pWarps[DWARP_EXIT]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_EXIT]._wx + DBORDERX;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_EXIT]._wy + DBORDERY + 1;
		trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
		numtrigs++;
	// }
	if (pWarps[DWARP_TOWN]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_TOWN]._wx + DBORDERX;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_TOWN]._wy + DBORDERY + 1;
		trigs[numtrigs]._tmsg = DVL_DWM_TWARPUP;
		numtrigs++;
	}
	if (pWarps[DWARP_SIDE]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_SIDE]._wx + DBORDERX;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_SIDE]._wy + DBORDERY + 1;
		trigs[numtrigs]._tlvl = questlist[Q_BCHAMB]._qslvl;
		trigs[numtrigs]._tmsg = DVL_DWM_SETLVL;
		numtrigs++;
	}
}

static void InitL3Triggers()
{
	int i, j;

	numtrigs = 0;
	/*for (j = 0; j < MAXDUNY; j++) {
		for (i = 0; i < MAXDUNX; i++) {
			if (dPiece[i][j] == 171) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_PREVLVL;
				numtrigs++;
			} else if (dPiece[i][j] == 168) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
				numtrigs++;
			} else if (dPiece[i][j] == 549) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_TWARPUP;
				numtrigs++;
			}
		}
	}*/
	// if (pWarps[DWARP_ENTRY]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_ENTRY]._wx + DBORDERX + 1;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_ENTRY]._wy + DBORDERY;
		trigs[numtrigs]._tmsg = DVL_DWM_PREVLVL;
		numtrigs++;
	// }
	// if (pWarps[DWARP_EXIT]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_EXIT]._wx + DBORDERX;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_EXIT]._wy + DBORDERY + 1;
		trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
		numtrigs++;
	// }
	if (pWarps[DWARP_TOWN]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_TOWN]._wx + DBORDERX + 1;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_TOWN]._wy + DBORDERY;
		trigs[numtrigs]._tmsg = DVL_DWM_TWARPUP;
		numtrigs++;
	}
}

static void InitL4Triggers()
{
	int i, j;

	numtrigs = 0;
	/*for (j = 0; j < MAXDUNY; j++) {
		for (i = 0; i < MAXDUNX; i++) {
			if (dPiece[i][j] == 82) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_PREVLVL;
				numtrigs++;
			} else if (dPiece[i][j] == 421) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_TWARPUP;
				numtrigs++;
			} else if (dPiece[i][j] == 120) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
				numtrigs++;
			}
		}
	}

	for (j = 0; j < MAXDUNY; j++) {
		for (i = 0; i < MAXDUNX; i++) {
			if (dPiece[i][j] == 370 && quests[Q_BETRAYER]._qactive == QUEST_DONE) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
				numtrigs++;
			}
		}
	}*/
	// if (pWarps[DWARP_ENTRY]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_ENTRY]._wx + DBORDERX;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_ENTRY]._wy + DBORDERY;
		trigs[numtrigs]._tmsg = DVL_DWM_PREVLVL;
		numtrigs++;
	// }
	if (pWarps[DWARP_EXIT]._wx != 0) {
		if (currLvl._dLevelIdx != DLV_HELL3) {
			trigs[numtrigs]._tx = 2 * pWarps[DWARP_EXIT]._wx + DBORDERX;
			trigs[numtrigs]._ty = 2 * pWarps[DWARP_EXIT]._wy + DBORDERY;
			trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
			numtrigs++;
		} else if (quests[Q_BETRAYER]._qactive == QUEST_DONE) {
			trigs[numtrigs]._tx = 2 * pWarps[DWARP_EXIT]._wx + DBORDERX; // + 1;
			trigs[numtrigs]._ty = 2 * pWarps[DWARP_EXIT]._wy + DBORDERY; // + 1;
			trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
			numtrigs++;
		}
	}
	if (pWarps[DWARP_TOWN]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_TOWN]._wx + DBORDERX;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_TOWN]._wy + DBORDERY;
		trigs[numtrigs]._tmsg = DVL_DWM_TWARPUP;
		numtrigs++;
	}
}

#ifdef HELLFIRE
static void InitL5Triggers()
{
	int i, j;

	numtrigs = 0;
	/*for (j = 0; j < MAXDUNY; j++) {
		for (i = 0; i < MAXDUNX; i++) {
			//if (dPiece[i][j] == 184) {
			//	trigs[numtrigs]._tx = i;
			//	trigs[numtrigs]._ty = j;
			//	trigs[numtrigs]._tmsg = DVL_DWM_TWARPUP;
			//	numtrigs++;
			//} else
            if (dPiece[i][j] == 158) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = currLvl._dLevelIdx == DLV_CRYPT1 ? DVL_DWM_TWARPUP : DVL_DWM_PREVLVL;
				numtrigs++;
			} else if (dPiece[i][j] == 126) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
				numtrigs++;
			}
		}
	}*/
	// if (pWarps[DWARP_ENTRY]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_ENTRY]._wx + DBORDERX;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_ENTRY]._wy + DBORDERY;
		trigs[numtrigs]._tmsg = currLvl._dLevelIdx == DLV_CRYPT1 ? DVL_DWM_TWARPUP : DVL_DWM_PREVLVL;
		numtrigs++;
	// }
	if (pWarps[DWARP_EXIT]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_EXIT]._wx + DBORDERX + 1;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_EXIT]._wy + DBORDERY;
		trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
		numtrigs++;
	}
}

static void InitL6Triggers()
{
	int i, j;

	numtrigs = 0;
	/*for (j = 0; j < MAXDUNY; j++) {
		for (i = 0; i < MAXDUNX; i++) {
			if (dPiece[i][j] == 66) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = currLvl._dLevelIdx == DLV_NEST1 ? DVL_DWM_TWARPUP : DVL_DWM_PREVLVL;
				numtrigs++;
			} else if (dPiece[i][j] == 63) {
				trigs[numtrigs]._tx = i;
				trigs[numtrigs]._ty = j;
				trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
				numtrigs++;
			} //else if (dPiece[i][j] == 80) {
			//	trigs[numtrigs]._tx = i;
			//	trigs[numtrigs]._ty = j;
			//	trigs[numtrigs]._tmsg = DVL_DWM_TWARPUP;
			//	numtrigs++;
			//}
		}
	}*/
	// if (pWarps[DWARP_ENTRY]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_ENTRY]._wx + DBORDERX + 1;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_ENTRY]._wy + DBORDERY;
		trigs[numtrigs]._tmsg = currLvl._dLevelIdx == DLV_NEST1 ? DVL_DWM_TWARPUP : DVL_DWM_PREVLVL;
		numtrigs++;
	// }
	if (pWarps[DWARP_EXIT]._wx != 0) {
		trigs[numtrigs]._tx = 2 * pWarps[DWARP_EXIT]._wx + DBORDERX;
		trigs[numtrigs]._ty = 2 * pWarps[DWARP_EXIT]._wy + DBORDERY + 1;
		trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
		numtrigs++;
	}
}
#endif

static void InitQuestTriggers()
{
	/*int i;
	QuestStruct* qs;

	for (i = 0; i < NUM_QUESTS; i++) {
		qs = &quests[i];
		if (questlist[i]._qslvl != 0 && i != Q_BETRAYER
		 && currLvl._dLevelIdx == questlist[i]._qdlvl && qs->_qactive != QUEST_NOTAVAIL) {
			trigs[numtrigs]._tx = qs->_qtx;
			trigs[numtrigs]._ty = qs->_qty;
			trigs[numtrigs]._tmsg = DVL_DWM_SETLVL;
			trigs[numtrigs]._tlvl = questlist[i]._qslvl;
			numtrigs++;
		}
	}*/
}

static void InitSKingTriggers()
{
	numtrigs = 1;
	trigs[0]._tx = AllLevels[SL_SKELKING].dSetLvlDunX - 1; // DBORDERX + 66
	trigs[0]._ty = AllLevels[SL_SKELKING].dSetLvlDunY - 2; // DBORDERY + 26
	trigs[0]._tmsg = DVL_DWM_RTNLVL;
	trigs[0]._tlvl = questlist[Q_SKELKING]._qdlvl;
}

static void InitSChambTriggers()
{
	numtrigs = 1;
	trigs[0]._tx = AllLevels[SL_BONECHAMB].dSetLvlDunY + 1; // DBORDERX + 54
	trigs[0]._ty = AllLevels[SL_BONECHAMB].dSetLvlDunY - 0; // DBORDERY + 23
	trigs[0]._tmsg = DVL_DWM_RTNLVL;
	trigs[0]._tlvl = questlist[Q_BCHAMB]._qdlvl;
}

static void InitPWaterTriggers()
{
	numtrigs = 1;
	trigs[0]._tx = AllLevels[SL_POISONWATER].dSetLvlDunY - 1; // DBORDERX + 14
	trigs[0]._ty = AllLevels[SL_POISONWATER].dSetLvlDunY - 0; // DBORDERY + 67
	trigs[0]._tmsg = DVL_DWM_RTNLVL;
	trigs[0]._tlvl = questlist[Q_PWATER]._qdlvl;
}

static void Freeupstairs()
{
	int i, tx, ty, xx, yy;

	for (i = 0; i < numtrigs; i++) {
		tx = trigs[i]._tx;
		ty = trigs[i]._ty;

		for (yy = -2; yy <= 2; yy++) {
			for (xx = -2; xx <= 2; xx++) {
				dFlags[tx + xx][ty + yy] |= BFLAG_POPULATED;
			}
		}
	}
}

void InitTriggers()
{
	if (!currLvl._dSetLvl) {
		switch (currLvl._dType) {
		case DTYPE_TOWN:
			return;
		case DTYPE_CATHEDRAL:
			InitL1Triggers();
			break;
		case DTYPE_CATACOMBS:
			InitL2Triggers();
			break;
		case DTYPE_CAVES:
			InitL3Triggers();
			break;
		case DTYPE_HELL:
			InitL4Triggers();
			break;
#ifdef HELLFIRE
		case DTYPE_CRYPT:
			InitL5Triggers();
			break;
		case DTYPE_NEST:
			InitL6Triggers();
			break;
#endif
		default:
			ASSUME_UNREACHABLE
			break;
		}
		InitQuestTriggers();
		Freeupstairs();
	} else {
		switch (currLvl._dLevelIdx) {
		case SL_SKELKING:
			InitSKingTriggers();
			break;
		case SL_BONECHAMB:
			InitSChambTriggers();
			break;
		//case SL_MAZE:
		//	break;
		case SL_POISONWATER:
			InitPWaterTriggers();
			break;
		case SL_VILEBETRAYER:
			InitNoTriggers();
			break;
		default:
			ASSUME_UNREACHABLE
			break;
		}
	}
}
