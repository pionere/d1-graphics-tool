/**
 * @file interfac.cpp
 *
 * Implementation of load screens.
 */
//#include "all.h"

#include <QString>

#include "../d1dun.h"
#include "../dungeongeneratedialog.h"

#include "all.h"

int ViewX;
int ViewY;
bool IsMultiGame;
QString assetPath;

static void IncProgress()
{
}

/**
 * @param lvldir method of entry
 */
static void CreateLevel(int lvldir)
{
	switch (currLvl._dDunType) {
	case DTYPE_TOWN:
		// CreateTown(lvldir);
		break;
	case DTYPE_CATHEDRAL:
		CreateL1Dungeon(lvldir);
		break;
	case DTYPE_CATACOMBS:
		CreateL2Dungeon(lvldir);
		break;
	case DTYPE_CAVES:
		CreateL3Dungeon(lvldir);
		break;
	case DTYPE_HELL:
		CreateL4Dungeon(lvldir);
		break;
	default:
		ASSUME_UNREACHABLE
		break;
	}
	InitTriggers();
	// LoadRndLvlPal();
}

static void LoadGameLevel(int lvldir, int seed)
{
	//SetRndSeed(seed);

	IncProgress();
	InitLvlDungeon();
//	MakeLightTable();
	IncProgress();

//	InitLvlAutomap();

	//if (lvldir != ENTRY_LOAD) {
//		InitLighting();
//		InitVision();
	//}

	InitLevelMonsters();
	InitLevelObjects();
	IncProgress();

	SetRndSeed(seed);

	if (!currLvl._dSetLvl) {
		CreateLevel(lvldir);
		if (pMegaTiles == NULL || pSolidTbl == NULL) {
			return;
        }
		IncProgress();
		if (currLvl._dType != DTYPE_TOWN) {
			GetLevelMTypes();
			InitThemes();
			IncProgress();
			InitObjectGFX();
		} else {
//			InitLvlStores();
			// TODO: might want to reset RndSeed, since InitLvlStores is player dependent, but it does not matter at the moment
			// SetRndSeed(seed);
			IncProgress();
		}
		IncProgress();
		IncProgress();

//		if (lvldir == ENTRY_RTNLVL)
//			GetReturnLvlPos();
//		if (lvldir == ENTRY_WARPLVL)
//			GetPortalLvlPos();

		if (currLvl._dType != DTYPE_TOWN) {
			HoldThemeRooms();
			InitMonsters();
			IncProgress();
//			if (IsMultiGame || lvldir == ENTRY_LOAD || !IsLvlVisited(currLvl._dLevelIdx)) {
				InitObjects();
				InitItems();
				CreateThemeRooms();
//			}
		} else {
//			InitTowners();
			IncProgress();
//			InitItems();
		}
	} else {
		LoadSetMap();
		IncProgress();
		// GetLevelMTypes();
		IncProgress();
		InitMonsters();
		IncProgress();
		IncProgress();
		IncProgress();

//		if (lvldir == ENTRY_WARPLVL)
//			GetPortalLvlPos();

		InitItems();
	}
	IncProgress();
//	InitMissiles();
//	SavePreLighting();

	IncProgress();

//	if (!IsMultiGame) {
		ResyncQuests();
//		if (lvldir != ENTRY_LOAD && IsLvlVisited(currLvl._dLevelIdx)) {
//			LoadLevel();
//		}
		//SyncPortals();
//	}
	IncProgress();
//	InitSync();
//	PlayDungMsgs();

//	guLvlVisited |= LEVEL_MASK(currLvl._dLevelIdx);

//	music_start(AllLevels[currLvl._dLevelIdx].dMusic);
}

static void EnterLevel(int lvl)
{
	currLvl._dLevelIdx = lvl;
	currLvl._dLevel = AllLevels[lvl].dLevel;
	currLvl._dSetLvl = AllLevels[lvl].dSetLvl;
	currLvl._dType = AllLevels[lvl].dType;
	currLvl._dDunType = AllLevels[lvl].dDunType;
	if (gnDifficulty == DIFF_NIGHTMARE)
		currLvl._dLevel += NIGHTMARE_LEVEL_BONUS;
	else if (gnDifficulty == DIFF_HELL)
		currLvl._dLevel += HELL_LEVEL_BONUS;
}

bool EnterGameLevel(D1Dun *dun, const GenerateDunParam &params)
{
	if (params.level == DLV_TOWN || params.level >= NUM_LEVELS) {
		return false;
    }
	IsMultiGame = params.isMulti;
	gnDifficulty = params.difficulty;
	assetPath = dun->getAssetPath();
	InitQuests(params.seedQuest);
//	if (IsMultiGame) {
//		DeltaSaveLevel();
//	} else {
//		SaveLevel();
//	}
//	IncProgress();
//	FreeLevelMem();
	EnterLevel(params.level);
	IncProgress();
	LoadGameLevel(ENTRY_MAIN, params.seed);

	bool hasSubtiles = pMegaTiles != NULL;
	FreeLvlDungeon();

	dun->setWidth(MAXDUNX);
    dun->setHeight(MAXDUNY);
    dun->setLevelType(currLvl._dType);

	for (int y = 0; y < MAXDUNY; y += 2) {
		for (int x = 0; x < MAXDUNY; x += 2) {
			dun->setTileAt(x, y, 0);
		}
	}
	for (int y = 0; y < DMAXY; y++) {
		for (int x = 0; x < DMAXX; x++) {
			dun->setTileAt(DBORDERX + x * 2, DBORDERY + y * 2, dungeon[x][y]);
		}
	}
	for (int y = 0; y < MAXDUNY; y++) {
		for (int x = 0; x < MAXDUNY; x++) {
			if (hasSubtiles) {
				dun->setSubtileAt(x, y, dPiece[x][y]);
            }
			int item = dItem[x][y];
			if (item != 0) {
				item = items[item - 1]._itype + 1;
            }
			dun->setItemAt(x, y, item);
			int mon = dMonster[x][y];
			if (mon != 0) {
				monsters[mon - 1]._mType + 1;
            }
			dun->setMonsterAt(x, y, mon);
			int obj = dObject[x][y];
			if (obj > 0) {
				objects[obj - 1]._otype + 1;
            }
			dun->setObjectAt(x, y, obj);
			dun->setRoomAt(x, y, dTransVal[x][y]);
		}
    }

	// bool addResource(const AddResourceParam &params);

	return true;
}
