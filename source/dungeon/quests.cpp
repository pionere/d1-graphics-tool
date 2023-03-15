/**
 * @file quests.cpp
 *
 * Implementation of functionality for handling quests.
 */
#include "all.h"

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
		qs->_qtx = 0;
		qs->_qty = 0;
		qs->_qmsg = qdata->_qdmsg;
	}

	SetRndSeed(seed);
	quests[Q_DIABLO]._qvar1 = random_(0, 3);

		quests[random_(0, 2) != 0 ? Q_SKELKING : Q_PWATER]._qactive = QUEST_NOTAVAIL;
#ifdef HELLFIRE
		if (random_(0, 2) != 0)
			quests[Q_GIRL]._qactive = QUEST_NOTAVAIL;
#endif

		quests[QuestGroup1[random_(0, lengthof(QuestGroup1))]]._qactive = QUEST_NOTAVAIL;
		quests[QuestGroup2[random_(0, lengthof(QuestGroup2))]]._qactive = QUEST_NOTAVAIL;
		quests[QuestGroup3[random_(0, lengthof(QuestGroup3))]]._qactive = QUEST_NOTAVAIL;
		quests[random_(0, 2) != 0 ? Q_VEIL : Q_WARLORD]._qactive = QUEST_NOTAVAIL;
#ifdef HELLFIRE
		quests[random_(0, 2) != 0 ? Q_FARMER : Q_JERSEY]._qactive = QUEST_NOTAVAIL;
#endif

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

static void DrawButcher()
{
	int x, y;

	assert(setpc_w == 6);
	assert(setpc_h == 6);
	x = 2 * setpc_x + DBORDERX;
	y = 2 * setpc_y + DBORDERY;
	// fix transVal on the bottom left corner of the room
	DRLG_CopyTrans(x, y + 9, x + 1, y + 9);
	DRLG_CopyTrans(x, y + 10, x + 1, y + 10);
	// set transVal in the room
	DRLG_RectTrans(x + 3, y + 3, x + 10, y + 10);
}

static void DrawSkelKing()
{
	int x, y;

	x = 2 * setpc_x + DBORDERX;
	y = 2 * setpc_y + DBORDERY;
	// fix transVal on the bottom left corner of the box
	DRLG_CopyTrans(x, y + 11, x + 1, y + 11);
	DRLG_CopyTrans(x, y + 12, x + 1, y + 12);
	// fix transVal at the entrance - commented out because it makes the wall transparent
	//DRLG_CopyTrans(x + 13, y + 7, x + 12, y + 7);
	//DRLG_CopyTrans(x + 13, y + 8, x + 12, y + 8);
	// patch dSolidTable - L1.SOL - commented out because 299 is used elsewhere
	//nSolidTable[299] = true;

	quests[Q_SKELKING]._qtx = x + 12;
	quests[Q_SKELKING]._qty = y + 7;
}

static void DrawMap(const char* name, BYTE bv)
{
	int x, y, rw, rh, i, j;
	BYTE* pMap;
	BYTE* sp;

	pMap = LoadFileInMem(name);
	rw = pMap[0];
	rh = pMap[2];

	sp = &pMap[4];
	assert(setpc_w == rw);
	assert(setpc_h == rh);
	x = setpc_x;
	y = setpc_y;
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
	mem_free_dbg(pMap);
}

static void DrawWarLord()
{
	DrawMap("Levels\\L4Data\\Warlord2.DUN", 6);
}

static void DrawSChamber()
{
	quests[Q_BCHAMB]._qtx = 2 * setpc_x + DBORDERX + 6;
	quests[Q_BCHAMB]._qty = 2 * setpc_y + DBORDERY + 7;

	DrawMap("Levels\\L2Data\\Bonestr1.DUN", 3);
	// patch the map - Bonestr1.DUN
	// shadow of the external-left column
	dungeon[setpc_x][setpc_y + 4] = 48;
	dungeon[setpc_x][setpc_y + 5] = 50;
}

static void DrawPostMap(const char* name)
{
	int x, y, rw, rh, i, j;
	BYTE* pMap;
	BYTE* sp;

	pMap = LoadFileInMem(name);
	rw = pMap[0];
	rh = pMap[2];

	sp = &pMap[4];
	assert(setpc_w == rw);
	assert(setpc_h == rh);
	x = setpc_x;
	y = setpc_y;
	rw += x;
	rh += y;
	for (j = y; j < rh; j++) {
		for (i = x; i < rw; i++) {
			if (*sp != 0) {
				pdungeon[i][j] = *sp;
			}
			sp += 2;
		}
	}
	mem_free_dbg(pMap);
}

static void DrawLTBanner()
{
	DrawMap("Levels\\L1Data\\Banner2.DUN");
	// patch the map - Banner2.DUN
	// replace the wall with door
	dungeon[setpc_x + 7][setpc_y + 6] = 193;
}

static void DrawBlind()
{
	DrawMap("Levels\\L2Data\\Blind2.DUN", 3);
	// patch the map - Blind2.DUN
	// replace the door with wall
	dungeon[setpc_x + 4][setpc_y + 3] = 25;
}

static void DrawBlood()
{
	DrawMap("Levels\\L2Data\\Blood2.DUN", 3);
	// patch the map - Blood2.DUN
	// place pieces with closed doors
	dungeon[setpc_x + 4][setpc_y + 10] = 151;
	dungeon[setpc_x + 4][setpc_y + 15] = 151;
	dungeon[setpc_x + 5][setpc_y + 15] = 151;
	// shadow of the external-left column -- do not place to prevent overwriting large decorations
	//dungeon[setpc_x - 1][setpc_y + 7] = 48;
	//dungeon[setpc_x - 1][setpc_y + 8] = 50;
	// shadow of the bottom-left column(s) -- one is missing
	dungeon[setpc_x + 1][setpc_y + 13] = 48;
	dungeon[setpc_x + 1][setpc_y + 14] = 50;
	// shadow of the internal column next to the pedistal
	dungeon[setpc_x + 5][setpc_y + 7] = 142;
	dungeon[setpc_x + 5][setpc_y + 8] = 50;
}

static void DrawNakrul()
{
	DrawMap("NLevels\\L5Data\\Nakrul1.DUN");
}

void DRLG_CheckQuests()
{
	int i;

	for (i = 0; i < NUM_QUESTS; i++) {
		if (QuestStatus(i)) {
			switch (i) {
			case Q_BUTCHER:
				DrawButcher();
				break;
			case Q_BANNER:
				DrawLTBanner();
				break;
			case Q_BLIND:
				DrawBlind();
				break;
			case Q_BLOOD:
				DrawBlood();
				break;
			case Q_WARLORD:
				DrawWarLord();
				break;
			case Q_SKELKING:
				DrawSkelKing();
				break;
			case Q_BCHAMB:
				DrawSChamber();
				break;
			case Q_NAKRUL:
				DrawNakrul();
				break;
			}
		}
	}
}

static void ResyncBanner()
{
	if (quests[Q_BANNER]._qvar1 == QV_BANNER_ATTACK) {
		ObjChangeMap(setpc_x, setpc_y, setpc_x + setpc_w, setpc_y + setpc_h/*, false*/);
		//for (i = 0; i < numobjects; i++)
		//	SyncObjectAnim(objectactive[i]);
		BYTE tv = dTransVal[2 * setpc_x + 1 + DBORDERX][2 * (setpc_y + 6) + 1 + DBORDERY];
		DRLG_MRectTrans(setpc_x, setpc_y + 3, setpc_x + setpc_w - 1, setpc_y + setpc_h - 1, tv);
	}
}

void ResyncQuests()
{
	if (QuestStatus(Q_BANNER)) {
		ResyncBanner();
	}
}
