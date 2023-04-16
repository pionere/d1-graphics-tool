/**
 * @file interfac.cpp
 *
 * Implementation of load screens.
 */
#include "all.h"

#include <QApplication>
#include <QDateTime>
#include <QMessageBox>
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
char infostr[256];

typedef struct ObjStruct {
    int otype;
    int animFrame;
    QString name;

    bool operator==(const ObjStruct & oval) const {
        return otype == oval.otype && animFrame == oval.animFrame && name == oval.name;
    };
    bool operator!=(const ObjStruct & oval) const {
        return otype != oval.otype || animFrame != oval.animFrame || name != oval.name;
    };
} ObjStruct;

static void IncProgress()
{
}

void LogErrorF(const char* msg, ...)
{
	char tmp[256];
	//snprintf(tmp, sizeof(tmp), "f:\\logdebug%d_%d.txt", mypnum, SDL_ThreadID());
	/*snprintf(tmp, sizeof(tmp), "f:\\logdebug%d.txt", 0);
	FILE* f0 = fopen(tmp, "a+");
	if (f0 == NULL)
		return;*/

	va_list va;

	va_start(va, msg);

	vsnprintf(tmp, sizeof(tmp), msg, va);

	va_end(va);

	dProgressErr() << QString(tmp);
	/*fputs(tmp, f0);

	fputc('\n', f0);

	fclose(f0);*/
}

static void StoreProtections(D1Dun *dun)
{
	for (int y = 0; y < MAXDUNY; y += 2) {
		for (int x = 0; x < MAXDUNX; x += 2) {
			dun->setTileProtectionAt(x, y, Qt::Unchecked);
			dun->setSubtileMonProtectionAt(x, y, false);
			dun->setSubtileObjProtectionAt(x, y, false);
		}
	}
	for (int y = 0; y < DMAXY; y++) {
		for (int x = 0; x < DMAXX; x++) {
			dun->setTileProtectionAt(DBORDERX + x * 2, DBORDERX + y * 2, (drlgFlags[x][y] & DRLG_FROZEN) != 0 ? Qt::Checked : ((drlgFlags[x][y] & DRLG_PROTECTED) != 0 ? Qt::PartiallyChecked : Qt::Unchecked));
		}
	}
	for (int y = 0; y < MAXDUNY; y++) {
		for (int x = 0; x < MAXDUNX; x++) {
			dun->setSubtileMonProtectionAt(x, y, (dFlags[x][y] & BFLAG_MON_PROTECT) != 0);
			dun->setSubtileObjProtectionAt(x, y, (dFlags[x][y] & BFLAG_OBJ_PROTECT) != 0);
		}
	}
}

static void CreateLevel()
{
    switch (currLvl._dDunType) {
    case DTYPE_TOWN:
        // CreateTown();
        break;
    case DTYPE_CATHEDRAL:
        CreateL1Dungeon();
        break;
    case DTYPE_CATACOMBS:
        CreateL2Dungeon();
        break;
    case DTYPE_CAVES:
        CreateL3Dungeon();
        break;
    case DTYPE_HELL:
        CreateL4Dungeon();
        break;
    default:
        ASSUME_UNREACHABLE
        break;
    }
    InitTriggers();
    // LoadRndLvlPal();
    int rv = RandRange(1, 4);
}

static void LoadGameLevel(int lvldir, D1Dun *dun)
{
	extern int32_t sglGameSeed;
	int32_t gameSeed = sglGameSeed;

	IncProgress();
	InitLvlDungeon(); // load tiles + meta data, reset pWarps, pSetPieces
//	MakeLightTable();
	IncProgress();

//	InitLvlAutomap();

	//if (lvldir != ENTRY_LOAD) {
	//	InitLighting();
	//	InitVision();
	//}
	InitLevelMonsters(); // reset monsters
	InitLevelObjects();  // reset objects
	InitLvlThemes();     // reset themes
	InitLvlItems();      // reset items
	IncProgress();

	SetRndSeed(gameSeed); // restore seed after InitLevelMonsters
	// fill in loop: dungeon, dTransVal, pWarps, pSetPieces, uses drlgFlags, dungBlock
	// fill post: pdungeon, dPiece, dSpecial, themeLoc, dFlags
	// reset: dMonster, dObject, dPlayer, dItem, dMissile, dLight+
	CreateLevel();
	StoreProtections(dun);
	if (pMegaTiles == NULL || pSolidTbl == NULL) {
		return;
	}
	IncProgress();
	if (currLvl._dType != DTYPE_TOWN) {
		GetLevelMTypes(); // select monster types and load their fx
		InitThemes(); // select theme types
		IncProgress();
		InitObjectGFX(); // load object graphics
		IncProgress();
		HoldThemeRooms(); // protect themes with dFlags
		InitMonsters();   // place monsters
		IncProgress();
		InitObjects(); // place objects
		InitItems();   // place items
		CreateThemeRooms(); // populate theme rooms
	} else {
//		InitLvlStores();
		// TODO: might want to reset RndSeed, since InitLvlStores is player dependent, but it does not matter at the moment
		// SetRndSeed(seed);
		IncProgress();
		IncProgress();

//		InitTowners();
		IncProgress();
//		InitItems();
	}
	FreeSetPieces();
	IncProgress();
//	InitMissiles();  // reset missiles
//	SavePreLighting(); // fill dPreLight
	InitView(lvldir);

	IncProgress();

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

	dun->setWidth(MAXDUNX, true);
	dun->setHeight(MAXDUNY, true);

    InitQuests(params.seedQuest);
    ViewX = 0;
    ViewY = 0;
//    if (IsMultiGame) {
//        DeltaSaveLevel();
//    } else {
//        SaveLevel();
//    }
//    IncProgress();
//    FreeLevelMem();
    EnterLevel(params.level);
    IncProgress();

	bool hasSubtiles;
	int extraRounds = params.extraRounds;
	quint64 started = QDateTime::currentMSecsSinceEpoch();
	SetRndSeed(params.seed);
	do {
		extern int32_t sglGameSeed;
		//LogErrorF("Generating dungeon %d with seed: %d / %d. Entry mode: %d", params.level, sglGameSeed, params.seedQuest, params.entryMode);
		dProgress() << QApplication::tr("Generating dungeon %1 with seed: %2 / %3. Entry mode: %4").arg(params.level).arg(sglGameSeed).arg(params.seedQuest).arg(params.entryMode);
		LoadGameLevel(params.entryMode, dun);
		hasSubtiles = pMegaTiles != NULL;
		FreeLvlDungeon();
	} while (--extraRounds >= 0);
	quint64 now = QDateTime::currentMSecsSinceEpoch();
	dProgress() << QApplication::tr("Generated %1 dungeon. Elapsed time: %2ms.").arg(params.extraRounds + 1).arg(now - started);

    dun->setLevelType(currLvl._dType);

    for (int y = 0; y < MAXDUNY; y += 2) {
        for (int x = 0; x < MAXDUNX; x += 2) {
            dun->setTileAt(x, y, 0);
        }
    }
    for (int y = 0; y < DMAXY; y++) {
        for (int x = 0; x < DMAXX; x++) {
            dun->setTileAt(DBORDERX + x * 2, DBORDERY + y * 2, dungeon[x][y]);
        }
    }
    std::vector<ObjStruct> objectTypes;
    std::set<int> itemTypes;
    // std::vector<int> monUniques;
    for (int y = 0; y < MAXDUNY; y++) {
        for (int x = 0; x < MAXDUNX; x++) {
            if (hasSubtiles) {
                dun->setSubtileAt(x, y, dPiece[x][y]);
            }
            int item = dItem[x][y];
            if (item != 0) {
                item = items[item - 1]._iIdx + 1;
                itemTypes.insert(item - 1);
            }
            dun->setItemAt(x, y, item);
            int mon = dMonster[x][y];
            DunMonsterType monType = { 0, false };
            if (mon != 0) {
                MonsterStruct *ms = &monsters[mon - 1];
                monType.second = ms->_muniqtype != 0;
                if (!monType.second) {
                    mon = ms->_mMTidx;
                    mon += lengthof(DunMonstConvTbl);
                } else {
                    // monUniques.push_back(mon - 1);
                    // mon = nummtypes + monUniques.size() - 1;
                    mon = ms->_muniqtype;
                }
                // mon += lengthof(DunMonstConvTbl);
                monType.first = mon;
            }
            dun->setMonsterAt(x, y, monType);
            int obj = dObject[x][y];
            if (obj > 0) {
                GetObjectStr(obj - 1);
                ObjectStruct *os = &objects[obj - 1];
                ObjStruct on;
                on.otype = os->_otype;
                on.animFrame = os->_oAnimFlag == OAM_LOOP ? 0 : os->_oAnimFrame;
                on.name = infostr;
                auto iter = objectTypes.begin();
                for (; iter != objectTypes.end(); iter++) {
                    if (*iter == on) {
                        break;
                    }
                }
                obj = lengthof(DunObjConvTbl) + std::distance(objectTypes.begin(), iter);
                if (iter == objectTypes.end()) {
                    objectTypes.push_back(on);
                }
            } else {
                obj = 0;
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
    // - normal
    for (int i = 1; i < nummtypes; i++) {
        AddResourceParam monRes = AddResourceParam();
        monRes.type = DUN_ENTITY_TYPE::MONSTER;
        monRes.index = lengthof(DunMonstConvTbl) + i;
        monRes.name = mapMonTypes[i].cmName;
        monRes.path = monfiledata[mapMonTypes[i].cmFileNum].moGfxFile;
        monRes.path.replace("%c", "N");
        monRes.path = assetPath + "/" + monRes.path;
        monRes.width = monfiledata[mapMonTypes[i].cmFileNum].moWidth;
        if (monsterdata[mapMonTypes[i].cmType].mTransFile != NULL) {
            monRes.baseTrnPath = assetPath + "/" + monsterdata[mapMonTypes[i].cmType].mTransFile;
        }
        monRes.uniqueMon = false;
        dun->addResource(monRes);
    }
    // - unique
    /*for (unsigned i = 0; i < monUniques.size(); i++) {
        int mon = monUniques[i];
        MonsterStruct *ms = &monsters[mon];
        const MapMonData &md = mapMonTypes[ms->_mMTidx];
        AddResourceParam monRes = AddResourceParam();
        monRes.type = DUN_ENTITY_TYPE::MONSTER;
        monRes.index = lengthof(DunMonstConvTbl) + nummtypes + i;
        monRes.name = ms->_mName;
        monRes.path = monfiledata[md.cmFileNum].moGfxFile;
        monRes.path.replace("%c", "N");
        monRes.path = assetPath + "/" + monRes.path;
        monRes.width = monfiledata[md.cmFileNum].moWidth;
        if (monsterdata[md.cmType].mTransFile != NULL) {
            monRes.baseTrnPath = assetPath + "/" + monsterdata[md.cmType].mTransFile;
        }
        if (uniqMonData[ms->_muniqtype - 1].mTrnName != NULL) {
            monRes.uniqueTrnPath = assetPath + "/Monsters/Monsters/" + uniqMonData[ms->_muniqtype - 1].mTrnName + ".TRN";
        }
        monRes.uniqueMon = true;
        dun->addResource(monRes);
    }*/
    // add objects
    for (unsigned i = 0; i < objectTypes.size(); i++) {
        const ObjStruct &objType = objectTypes[i];
        int otype = objType.otype;
        AddResourceParam objRes = AddResourceParam();
        objRes.type = DUN_ENTITY_TYPE::OBJECT;
        objRes.index = lengthof(DunObjConvTbl) + i;
        objRes.name = objType.name.isEmpty() ? objfiledata[objectdata[otype].ofindex].ofName : objType.name;
        objRes.path = assetPath + "/Objects/" + objfiledata[objectdata[otype].ofindex].ofName + ".CEL";
        objRes.width = objfiledata[objectdata[otype].ofindex].oAnimWidth;
        objRes.frame = objType.animFrame;
        dun->addResource(objRes);
    }
    view->updateEntityOptions();

    view->scrollTo(ViewX, ViewY);

    return true;
}
