/**
 * @file interfac.cpp
 *
 * Implementation of load screens.
 */
#include "all.h"

#include <QApplication>
#include <QString>

// #include "../d1dun.h"
// #include "../dungeongeneratedialog.h"
#include "../levelcelview.h"
#include "../progressdialog.h"

int ViewX;
int ViewY;
bool IsMultiGame;
bool IsHellfireGame;
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
	int rv = RandRange(1, 4);
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
extern int32_t sglGameSeed;
dProgress() << QApplication::tr("LoadGameLevel 0: %1").arg(sglGameSeed);

	if (!currLvl._dSetLvl) {
		CreateLevel(lvldir);
dProgress() << QApplication::tr("LoadGameLevel 1: %1").arg(sglGameSeed);
		if (pMegaTiles == NULL || pSolidTbl == NULL) {
			return;
        }
		IncProgress();
		if (currLvl._dType != DTYPE_TOWN) {
			GetLevelMTypes();
dProgress() << QApplication::tr("LoadGameLevel 2: %1").arg(sglGameSeed);
			InitThemes();
dProgress() << QApplication::tr("LoadGameLevel 3: %1").arg(sglGameSeed);
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
dProgress() << QApplication::tr("LoadGameLevel 4: %1").arg(sglGameSeed);
			InitMonsters();
dProgress() << QApplication::tr("LoadGameLevel 5: %1").arg(sglGameSeed);
			IncProgress();
//			if (IsMultiGame || lvldir == ENTRY_LOAD || !IsLvlVisited(currLvl._dLevelIdx)) {
				InitObjects();
dProgress() << QApplication::tr("LoadGameLevel 6: %1").arg(sglGameSeed);
				InitItems();
dProgress() << QApplication::tr("LoadGameLevel 7: %1").arg(sglGameSeed);
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
dProgress() << QApplication::tr("LoadGameLevel 8: %1").arg(sglGameSeed);
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
dProgress() << QApplication::tr("LoadGameLevel final: %1").arg(sglGameSeed);
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

bool EnterGameLevel(D1Dun *dun, LevelCelView *view, const GenerateDunParam &params)
{
	IsMultiGame = params.isMulti;
	IsHellfireGame = params.isHellfire;
	gnDifficulty = params.difficulty;
	assetPath = dun->getAssetPath();
	InitQuests(params.seedQuest);
	ViewX = 0;
	ViewY = 0;
//	if (IsMultiGame) {
//		DeltaSaveLevel();
//	} else {
//		SaveLevel();
//	}
//	IncProgress();
//	FreeLevelMem();
	EnterLevel(params.level);
	IncProgress();
	LoadGameLevel(params.entryMode, params.seed);

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
	std::set<int> objectTypes;
	std::set<int> itemTypes;
	for (int y = 0; y < MAXDUNY; y++) {
		for (int x = 0; x < MAXDUNY; x++) {
			if (hasSubtiles) {
				dun->setSubtileAt(x, y, dPiece[x][y]);
            }
			int item = dItem[x][y];
			if (item != 0) {
				item = items[item - 1]._itype + 1;
				itemTypes.insert(item - 1);
            }
			dun->setItemAt(x, y, item);
			int mon = dMonster[x][y];
			if (mon != 0) {
				mon = monsters[mon - 1]._mMTidx + lengthof(DunMonstConvTbl);
            }
			dun->setMonsterAt(x, y, mon);
			int obj = dObject[x][y];
			if (obj > 0) {
				obj = objects[obj - 1]._otype + lengthof(DunObjConvTbl);
				objectTypes.insert(obj - lengthof(DunObjConvTbl));
            }
			dun->setObjectAt(x, y, obj);
			dun->setRoomAt(x, y, dTransVal[x][y]);
		}
    }

	dun->clearAssets();
	// add items
	for (int itype : itemTypes) {
		AddResourceParam itemRes = AddResourceParam();
		itemRes.type = DUN_ENTITY_TYPE::ITEM;
		itemRes.index = 1 + itype;
		itemRes.name = AllItemsList[itype].iName; // TODO: more specific names?
		itemRes.path = assetPath + "/Items/" + itemfiledata[ItemCAnimTbl[AllItemsList[itype].iCurs]].ifName + ".CEL";
		// itemRes.width = ITEM_ANIM_WIDTH;
		dun->addResource(itemRes);
    }
	// add monsters
	for (int i = 1; i < nummtypes; i++) {
		AddResourceParam monRes = AddResourceParam();
		monRes.type = DUN_ENTITY_TYPE::MONSTER;
		monRes.index = lengthof(DunMonstConvTbl) + i;
		monRes.name = mapMonTypes[i].cmName;
		monRes.path = assetPath + "/" + monfiledata[mapMonTypes[i].cmFileNum].moGfxFile;
		monRes.path.replace("%c", "N");
		monRes.width = monfiledata[mapMonTypes[i].cmFileNum].moWidth;
		if (monsterdata[mapMonTypes[i].cmType].mTransFile != NULL) {
			monRes.trnPath = assetPath + "/" + monsterdata[mapMonTypes[i].cmType].mTransFile;
        }
		dun->addResource(monRes);
    }
	// add objects
	for (int otype : objectTypes) {
		AddResourceParam objRes = AddResourceParam();
		objRes.type = DUN_ENTITY_TYPE::OBJECT;
		objRes.index = lengthof(DunObjConvTbl) + otype;
		objRes.name = objfiledata[objectdata[otype].ofindex].ofName; // FIXME: object labels?
		objRes.path = assetPath + "/Objects/" + objfiledata[objectdata[otype].ofindex].ofName + ".CEL";
		objRes.width = objfiledata[objectdata[otype].ofindex].oAnimWidth;
		objRes.frame = objectdata[otype].oAnimBaseFrame; // TODO: books / chests?
		dun->addResource(objRes);
    }
	view->updateEntityOptions();

	view->setPositionX(ViewX);
	view->setPositionY(ViewY);

	return true;
}
