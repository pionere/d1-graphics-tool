/**
 * @file trigs.cpp
 *
 * Implementation of functionality for triggering events when the player enters an area.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

int numtrigs;
TriggerStruct trigs[MAXTRIGGERS];

static void InitDunTriggers()
{
	numtrigs = 0;
	for (int i = lengthof(pWarps) - 1; i >= 0; i--) {
		if (pWarps[i]._wx == 0) {
			continue;
		}
		if (i == DWARP_EXIT && currLvl._dLevelIdx == DLV_HELL3 && quests[Q_BETRAYER]._qactive != QUEST_DONE) {
			continue;
		}
		int tmsg;
		switch (i) {
		case DWARP_EXIT:  tmsg = DVL_DWM_NEXTLVL; break;
		case DWARP_ENTRY: tmsg = DVL_DWM_PREVLVL; break;
		case DWARP_TOWN:  tmsg = DVL_DWM_TWARPUP; break;
		case DWARP_SIDE:  tmsg = DVL_DWM_SETLVL;  break;
		default: ASSUME_UNREACHABLE; break;
		}
		trigs[numtrigs]._tx = pWarps[i]._wx;
		trigs[numtrigs]._ty = pWarps[i]._wy;
		trigs[numtrigs]._ttype = pWarps[i]._wtype;
		trigs[numtrigs]._tlvl = pWarps[i]._wlvl;
		trigs[numtrigs]._tmsg = tmsg;
		numtrigs++;
	}
}

static void InitSetDunTriggers()
{
	numtrigs = 0;

	// TODO: set tlvl in drlg_*
	int tlvl;
	switch (currLvl._dLevelIdx) {
	case SL_SKELKING:
		tlvl = questlist[Q_SKELKING]._qdlvl;
		break;
	case SL_BONECHAMB:
		tlvl = questlist[Q_BCHAMB]._qdlvl;
		break;
	//case SL_MAZE:
	//	break;
	case SL_POISONWATER:
		tlvl = questlist[Q_PWATER]._qdlvl;
		break;
	case SL_VILEBETRAYER:
		/*if (quests[Q_BETRAYER]._qvar1 >= QV_BETRAYER_DEAD) {
			tlvl = questlist[Q_BETRAYER]._qdlvl;

			trigs[numtrigs]._tx = pWarps[DWARP_ENTRY]._wx;     // DBORDERX + 19
			trigs[numtrigs]._ty = pWarps[DWARP_ENTRY]._wy - 4; // DBORDERX + 16
			trigs[numtrigs]._tlvl = tlvl;
			trigs[numtrigs]._ttype = WRPT_RPORTAL;
			trigs[numtrigs]._tmsg = DVL_DWM_RTNLVL;
			numtrigs++;
			// TODO: set BFLAG_MON_PROTECT | BFLAG_OBJ_PROTECT? test if the missile exists?
			AddMissile(0, 0, trigs[0]._tx, trigs[0]._ty, 0, MIS_RPORTAL, MST_NA, -1, deltaload ? -1 : 0);
		}*/
		return;
	default:
		ASSUME_UNREACHABLE
		break;
	}

	trigs[numtrigs]._tx = pWarps[DWARP_ENTRY]._wx;
	trigs[numtrigs]._ty = pWarps[DWARP_ENTRY]._wy;
	trigs[numtrigs]._ttype = pWarps[DWARP_ENTRY]._wtype;
	trigs[numtrigs]._tlvl = tlvl;
	trigs[numtrigs]._tmsg = DVL_DWM_RTNLVL;
	numtrigs++;
}

void InitView(int entry)
{
	int type;

	if (entry == ENTRY_WARPLVL) {
		// GetPortalLvlPos();
		return;
	}

	if (currLvl._dLevelIdx == DLV_TOWN) {
		/*if (entry == ENTRY_MAIN) {
			// New game
			ViewX = 65 + DBORDERX;
			ViewY = 58 + DBORDERY;
		//} else if (entry == ENTRY_PREV) { // Cathedral
		//	ViewX = 15 + DBORDERX;
		//	ViewY = 21 + DBORDERY;
		} else if (entry == ENTRY_TWARPUP) {
			switch (gbTWarpFrom) {
			case TWARP_CATHEDRAL:
				ViewX = 15 + DBORDERX;
				ViewY = 21 + DBORDERY;
				break;
			case TWARP_CATACOMB:
				ViewX = 39 + DBORDERX;
				ViewY = 12 + DBORDERY;
				break;
			case TWARP_CAVES:
				ViewX = 8 + DBORDERX;
				ViewY = 59 + DBORDERY;
				break;
			case TWARP_HELL:
				ViewX = 30 + DBORDERX;
				ViewY = 70 + DBORDERY;
				break;
#ifdef HELLFIRE
			case TWARP_CRYPT:
				ViewX = 26 + DBORDERX;
				ViewY = 15 + DBORDERY;
				break;
			case TWARP_NEST:
				ViewX = 69 + DBORDERX;
				ViewY = 52 + DBORDERY;
				break;
#endif
			default:
				ASSUME_UNREACHABLE
				break;
			}
		} else if (entry == ENTRY_RETOWN) {
			// Restart in Town
			ViewX = 63 + DBORDERX;
			ViewY = 70 + DBORDERY;
		}*/
		return;
	}

	switch (entry) {
	case ENTRY_MAIN:
		type = DWARP_ENTRY;
		break;
	case ENTRY_PREV:
		type = DWARP_EXIT;
		break;
	case ENTRY_SETLVL:
		type = DWARP_ENTRY;
		break;
	case ENTRY_RTNLVL:
		type = DWARP_SIDE;
		if (pWarps[type]._wtype == WRPT_NONE) {
			// return from the betrayer side-map - TODO: better solution?
			assert(currLvl._dLevelIdx == DLV_HELL3);
			type = DWARP_EXIT;
			ViewX = pWarps[type]._wx;
			ViewY = pWarps[type]._wy;
			assert(pWarps[type]._wtype == WRPT_L4_PENTA);
			ViewX += -2;
			ViewY += -2;
			return;
		}
		break;
	case ENTRY_LOAD:    // set from the save file
	case ENTRY_WARPLVL: // should not happen
		return;
	case ENTRY_TWARPDN:
		type = DWARP_TOWN;
		// if (pWarps[type]._wtype == WRPT_NONE)
		//	type = DWARP_ENTRY; // MAIN vs TWARPDN from town
		break;
	case ENTRY_TWARPUP: // should not happen
	case ENTRY_RETOWN:  // should not happen
		return;
	}

	ViewX = pWarps[type]._wx;
	ViewY = pWarps[type]._wy;
	type = pWarps[type]._wtype;
	switch (type) {
	case WRPT_NONE:
		break; // should not happen
	case WRPT_L1_UP:
		ViewX += 1;
		ViewY += 2;
		break;
	case WRPT_L1_DOWN:
		ViewX += 0;
		ViewY += 1;
		break;
	case WRPT_L1_SKING:
		ViewX += 1;
		ViewY += 0;
		break;
	case WRPT_L1_PWATER:
		ViewX += 0;
		ViewY += 1;
		break;
	case WRPT_L2_UP:
		ViewX += 1;
		ViewY += 0;
		break;
	case WRPT_L2_DOWN:
		ViewX += -1;
		ViewY += 0;
		break;
	case WRPT_L3_UP:
		ViewX += 0;
		ViewY += 1;
		break;
	case WRPT_L3_DOWN:
		ViewX += 1;
		ViewY += 0;
		break;
	case WRPT_L4_UP:
		ViewX += 0;
		ViewY += 1;
		break;
	case WRPT_L4_DOWN:
		ViewX += 1;
		ViewY += 0;
		break;
	case WRPT_L4_PENTA:
		ViewX += 0;
		ViewY += 1;
		break;
	case WRPT_CIRCLE:
		break;
	default:
		ASSUME_UNREACHABLE
		break;
	}
}

void InitTriggers()
{
	if (!currLvl._dSetLvl) {
		if (currLvl._dType == DTYPE_TOWN)
			return; // InitTownTriggers();
		else
			InitDunTriggers();
	} else {
		InitSetDunTriggers();
	}
}

DEVILUTION_END_NAMESPACE
