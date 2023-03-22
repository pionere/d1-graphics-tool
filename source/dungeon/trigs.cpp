/**
 * @file trigs.cpp
 *
 * Implementation of functionality for triggering events when the player enters an area.
 */
#include "all.h"

int numtrigs;
TriggerStruct trigs[MAXTRIGGERS];

static void InitNoTriggers()
{
	numtrigs = 0;
}

static void InitL1Triggers()
{
	// int i, j;

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
		trigs[numtrigs]._tx = pWarps[DWARP_ENTRY]._wx;
		trigs[numtrigs]._ty = pWarps[DWARP_ENTRY]._wy;
		trigs[numtrigs]._tmsg = currLvl._dLevelIdx == DLV_CATHEDRAL1 ? DVL_DWM_TWARPUP : DVL_DWM_PREVLVL;
		numtrigs++;
	// }
	// if (pWarps[DWARP_EXIT]._wx != 0) {
		trigs[numtrigs]._tx = pWarps[DWARP_EXIT]._wx + 1;
		trigs[numtrigs]._ty = pWarps[DWARP_EXIT]._wy;
		trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
		numtrigs++;
	// }
	if (pWarps[DWARP_SIDE]._wx != 0) {
		trigs[numtrigs]._tx = pWarps[DWARP_SIDE]._wx;
		trigs[numtrigs]._ty = pWarps[DWARP_SIDE]._wy;
		if (currLvl._dLevelIdx == questlist[Q_SKELKING]._qdlvl) { // TODO: add qn to pWarps?
			trigs[numtrigs]._tlvl = questlist[Q_SKELKING]._qslvl;
			trigs[numtrigs]._tx += 1;
		} else {
			trigs[numtrigs]._tlvl = questlist[Q_PWATER]._qslvl;
			trigs[numtrigs]._tx += 1;
		}
		trigs[numtrigs]._tmsg = DVL_DWM_SETLVL;
		numtrigs++;
	}
}

static void InitL2Triggers()
{
	// int i, j;

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
		trigs[numtrigs]._tx = pWarps[DWARP_ENTRY]._wx;
		trigs[numtrigs]._ty = pWarps[DWARP_ENTRY]._wy + 1;
		trigs[numtrigs]._tmsg = DVL_DWM_PREVLVL;
		numtrigs++;
	// }
	// if (pWarps[DWARP_EXIT]._wx != 0) {
		trigs[numtrigs]._tx = pWarps[DWARP_EXIT]._wx;
		trigs[numtrigs]._ty = pWarps[DWARP_EXIT]._wy + 1;
		trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
		numtrigs++;
	// }
	if (pWarps[DWARP_TOWN]._wx != 0) {
		trigs[numtrigs]._tx = pWarps[DWARP_TOWN]._wx;
		trigs[numtrigs]._ty = pWarps[DWARP_TOWN]._wy + 1;
		trigs[numtrigs]._tmsg = DVL_DWM_TWARPUP;
		numtrigs++;
	}
	if (pWarps[DWARP_SIDE]._wx != 0) {
		trigs[numtrigs]._tx = pWarps[DWARP_SIDE]._wx;
		trigs[numtrigs]._ty = pWarps[DWARP_SIDE]._wy + 1;
		trigs[numtrigs]._tlvl = questlist[Q_BCHAMB]._qslvl;
		trigs[numtrigs]._tmsg = DVL_DWM_SETLVL;
		numtrigs++;
	}
}

static void InitL3Triggers()
{
	// int i, j;

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
		trigs[numtrigs]._tx = pWarps[DWARP_ENTRY]._wx + 1;
		trigs[numtrigs]._ty = pWarps[DWARP_ENTRY]._wy;
		trigs[numtrigs]._tmsg = DVL_DWM_PREVLVL;
		numtrigs++;
	// }
	// if (pWarps[DWARP_EXIT]._wx != 0) {
		trigs[numtrigs]._tx = pWarps[DWARP_EXIT]._wx;
		trigs[numtrigs]._ty = pWarps[DWARP_EXIT]._wy + 1;
		trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
		numtrigs++;
	// }
	if (pWarps[DWARP_TOWN]._wx != 0) {
		trigs[numtrigs]._tx = pWarps[DWARP_TOWN]._wx + 1;
		trigs[numtrigs]._ty = pWarps[DWARP_TOWN]._wy;
		trigs[numtrigs]._tmsg = DVL_DWM_TWARPUP;
		numtrigs++;
	}
}

static void InitL4Triggers()
{
	// int i, j;

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
		trigs[numtrigs]._tx = pWarps[DWARP_ENTRY]._wx + 1;
		trigs[numtrigs]._ty = pWarps[DWARP_ENTRY]._wy + 1;
		trigs[numtrigs]._tmsg = DVL_DWM_PREVLVL;
		numtrigs++;
	// }
	if (pWarps[DWARP_EXIT]._wx != 0) {
		if (currLvl._dLevelIdx != DLV_HELL3) {
			trigs[numtrigs]._tx = pWarps[DWARP_EXIT]._wx;
			trigs[numtrigs]._ty = pWarps[DWARP_EXIT]._wy;
			trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
			numtrigs++;
		} else if (quests[Q_BETRAYER]._qactive == QUEST_DONE) {
			trigs[numtrigs]._tx = pWarps[DWARP_EXIT]._wx + 1;
			trigs[numtrigs]._ty = pWarps[DWARP_EXIT]._wy + 1;
			trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
			numtrigs++;
		}
	}
	if (pWarps[DWARP_TOWN]._wx != 0) {
		trigs[numtrigs]._tx = pWarps[DWARP_TOWN]._wx + 1;
		trigs[numtrigs]._ty = pWarps[DWARP_TOWN]._wy + 1;
		trigs[numtrigs]._tmsg = DVL_DWM_TWARPUP;
		numtrigs++;
	}
}

#ifdef HELLFIRE
static void InitL5Triggers()
{
	// int i, j;

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
		trigs[numtrigs]._tx = pWarps[DWARP_ENTRY]._wx;
		trigs[numtrigs]._ty = pWarps[DWARP_ENTRY]._wy;
		trigs[numtrigs]._tmsg = currLvl._dLevelIdx == DLV_CRYPT1 ? DVL_DWM_TWARPUP : DVL_DWM_PREVLVL;
		numtrigs++;
	// }
	if (pWarps[DWARP_EXIT]._wx != 0) {
		trigs[numtrigs]._tx = pWarps[DWARP_EXIT]._wx + 1;
		trigs[numtrigs]._ty = pWarps[DWARP_EXIT]._wy;
		trigs[numtrigs]._tmsg = DVL_DWM_NEXTLVL;
		numtrigs++;
	}
}

static void InitL6Triggers()
{
	// int i, j;

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
		trigs[numtrigs]._tx = pWarps[DWARP_ENTRY]._wx + 1;
		trigs[numtrigs]._ty = pWarps[DWARP_ENTRY]._wy;
		trigs[numtrigs]._tmsg = currLvl._dLevelIdx == DLV_NEST1 ? DVL_DWM_TWARPUP : DVL_DWM_PREVLVL;
		numtrigs++;
	// }
	if (pWarps[DWARP_EXIT]._wx != 0) {
		trigs[numtrigs]._tx = pWarps[DWARP_EXIT]._wx;
		trigs[numtrigs]._ty = pWarps[DWARP_EXIT]._wy + 1;
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
	trigs[0]._tx = AllLevels[SL_BONECHAMB].dSetLvlDunX + 1; // DBORDERX + 54
	trigs[0]._ty = AllLevels[SL_BONECHAMB].dSetLvlDunY - 0; // DBORDERY + 23
	trigs[0]._tmsg = DVL_DWM_RTNLVL;
	trigs[0]._tlvl = questlist[Q_BCHAMB]._qdlvl;
}

static void InitPWaterTriggers()
{
	numtrigs = 1;
	trigs[0]._tx = AllLevels[SL_POISONWATER].dSetLvlDunX - 1; // DBORDERX + 14
	trigs[0]._ty = AllLevels[SL_POISONWATER].dSetLvlDunY - 0; // DBORDERY + 67
	trigs[0]._tmsg = DVL_DWM_RTNLVL;
	trigs[0]._tlvl = questlist[Q_PWATER]._qdlvl;
}

static void Freeupstairs()
{
	int i, tx, ty, xx, yy;

	/*for (i = 0; i < numtrigs; i++) {
		tx = trigs[i]._tx;
		ty = trigs[i]._ty;

		for (yy = -2; yy <= 2; yy++) {
			for (xx = -2; xx <= 2; xx++) {
				dFlags[tx + xx][ty + yy] |= BFLAG_POPULATED;
			}
		}
	}*/
	for (i = 0; i < NUM_DWARP; i++) {
		tx = pWarps[i]._wx;
		ty = pWarps[i]._wy;

		if (tx == 0) {
			continue;
		}
		// tx = 2 * tx + DBORDERX;
		// ty = 2 * ty + DBORDERY;
		/*for (xx = -2; xx <= 2; xx++) {
			for (yy = -2; yy <= 2; yy++) {
				dFlags[tx + xx][ty + yy] |= BFLAG_POPULATED;
			}
		}*/
		int r = (currLvl._dLevelIdx == DLV_HELL3 && i == DWARP_EXIT) ? 4 : 2;
		tx -= r;
		ty -= r;
		r = 2 * r + 1;
		for (xx = 0; xx < r; xx++) {
			for (yy = 0; yy < r; yy++) {
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
