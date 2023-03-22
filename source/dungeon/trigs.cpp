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
	numtrigs = 0;
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
	numtrigs = 0;
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
	numtrigs = 0;
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
	numtrigs = 0;
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
	numtrigs = 0;
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
	numtrigs = 0;
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

	for (i = 0; i < NUM_DWARP; i++) {
		tx = pWarps[i]._wx;
		ty = pWarps[i]._wy;

		if (tx == 0) {
			continue;
		}
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
