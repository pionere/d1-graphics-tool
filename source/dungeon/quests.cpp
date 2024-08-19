/**
 * @file quests.cpp
 *
 * Implementation of functionality for handling quests.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

/** Contains the quests of the current game. */
QuestStruct quests[NUM_QUESTS];

/**
 * A quest group containing the three quests the Butcher,
 * Ogden's Sign and Gharbad the Weak, which ensures that exactly
 * two of these three quests appear in any game.
 */
static const int QuestGroup1[3] = { Q_BUTCHER, Q_BANNER, Q_GARBUD };
/**
 * A quest group containing the three quests Halls of the Blind,
 * the Magic Rock and Valor, which ensures that exactly two of
 * these three quests appear in any game.
 */
static const int QuestGroup2[3] = { Q_BLIND, Q_ROCK, Q_BLOOD };
/**
 * A quest group containing the three quests Black Mushroom,
 * Zhar the Mad and Anvil of Fury, which ensures that exactly
 * two of these three quests appear in any game.
 */
static const int QuestGroup3[3] = { Q_MUSHROOM, Q_ZHAR, Q_ANVIL };
/*
 * Other quest groups:
 * { Q_SKELKING, Q_PWATER }
 * { Q_VEIL, Q_WARLORD }
 * { Q_JERSEY, Q_FARMER }
 */

void InitQuests(int seed)
{
	QuestStruct* qs;
	const QuestData* qdata;
	int i;

	qs = quests;
	qdata = questlist;
	for (i = 0; i < NUM_QUESTS; i++, qs++, qdata++) {
		qs->_qactive = QUEST_INIT;
		qs->_qvar1 = QV_INIT;
		qs->_qvar2 = 0;
		qs->_qlog = FALSE;
		qs->_qmsg = qdata->_qdmsg;
	}

	SetRndSeed(seed);
	quests[Q_DIABLO]._qvar1 = random_(0, 3);
	quests[random_(0, 2) != 0 ? Q_SKELKING : Q_PWATER]._qactive = QUEST_NOTAVAIL;
	if (IsHellfireGame) {
	if (random_(0, 2) != 0)
		quests[Q_GIRL]._qactive = QUEST_NOTAVAIL;
	}

	quests[QuestGroup1[random_(0, lengthof(QuestGroup1))]]._qactive = QUEST_NOTAVAIL;
	quests[QuestGroup2[random_(0, lengthof(QuestGroup2))]]._qactive = QUEST_NOTAVAIL;
	quests[QuestGroup3[random_(0, lengthof(QuestGroup3))]]._qactive = QUEST_NOTAVAIL;
	quests[random_(0, 2) != 0 ? Q_VEIL : Q_WARLORD]._qactive = QUEST_NOTAVAIL;
	if (IsHellfireGame) {
	quests[random_(0, 2) != 0 ? Q_FARMER : Q_JERSEY]._qactive = QUEST_NOTAVAIL;
	}

	if (quests[Q_PWATER]._qactive == QUEST_NOTAVAIL)
		quests[Q_PWATER]._qvar1 = QV_PWATER_CLEAN;
}

bool QuestStatus(int qn)
{
	if (currLvl._dLevelIdx == questlist[qn]._qdlvl
	 && quests[qn]._qactive != QUEST_NOTAVAIL) {
		assert(!currLvl._dSetLvl);
		return true;
	}
	return false;
}

void ResyncBanner()
{
	// assert(currLvl._dLevelIdx == questlist[Q_BANNER]._qdlvl && quests[Q_BANNER]._qvar1 == QV_BANNER_ATTACK);
	int sx = pSetPieces[0]._spx + 3;
	int sy = pSetPieces[0]._spy + 3;
	// DRLG_ChangeMap(sx, sy, sx + 3, sy + 3/*, false*/);
}

void ResyncDiablo()
{
	// assert(currLvl._dLevelIdx == questlist[Q_BETRAYER]._qdlvl && quests[Q_BETRAYER]._qvar1 >= QV_BETRAYER_DEAD);
	int sx = (pWarps[DWARP_EXIT]._wx - (1 + 2 + DBORDERX)) >> 1;
	int sy = (pWarps[DWARP_EXIT]._wy - (1 + 2 + DBORDERY)) >> 1;
	// DRLG_ChangeMap(sx, sy, sx + 2, sy + 2);
}

DEVILUTION_END_NAMESPACE
