/**
 * @file interfac.cpp
 *
 * Implementation of load screens.
 */
#include "all.h"

#include <QApplication>
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

static void IncProgress()
{
}

static void LogErrorF(const char* type, const char* msg, ...)
{
    char tmp[256];
    va_list va;

    va_start(va, msg);

    vsnprintf(tmp, sizeof(tmp), msg, va);

    va_end(va);

    dProgress() << QString(tmp);
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

static void LoadGameLevel(int lvldir, int seed)
{
    //SetRndSeed(seed);

    IncProgress();
    InitLvlDungeon(); // load tiles + meta data
//    MakeLightTable();
    IncProgress();

//    InitLvlAutomap();

    //if (lvldir != ENTRY_LOAD) {
//        InitLighting();
//        InitVision();
    //}

    InitLevelMonsters(); // reset monster types
    InitLevelObjects();  // reset objects
    IncProgress();

    SetRndSeed(seed);

    if (!currLvl._dSetLvl) {
        CreateLevel(); // fill dungeon, pdungeon, dPiece, dSpecial, dTransVal, dPreLight, dLight, pWarps, pSetPieces, themeLoc, dFlags, reset dMonster, dObject, dPlayer, dItem, dMissile, uses drlgFlags
QMessageBox::critical(nullptr, "Error", "CreateLevel done");
        if (pMegaTiles == NULL || pSolidTbl == NULL) {
            return;
        }
        IncProgress();
        if (currLvl._dType != DTYPE_TOWN) {
            GetLevelMTypes(); // load monster types
QMessageBox::critical(nullptr, "Error", "MTypes done");
            InitThemes(); // select theme types
QMessageBox::critical(nullptr, "Error", "InitThemes done");
            IncProgress();
            InitObjectGFX(); // load object graphics
QMessageBox::critical(nullptr, "Error", "InitObjectGFX done");
        } else {
//            InitLvlStores();
            // TODO: might want to reset RndSeed, since InitLvlStores is player dependent, but it does not matter at the moment
            // SetRndSeed(seed);
            IncProgress();
        }
        IncProgress();

        if (currLvl._dType != DTYPE_TOWN) {
QMessageBox::critical(nullptr, "Error", "HoldThemeRooms start");
            HoldThemeRooms(); // protect themes
QMessageBox::critical(nullptr, "Error", "HoldThemeRooms done");
            InitMonsters();   // place monsters
QMessageBox::critical(nullptr, "Error", "InitMonsters done");
            IncProgress();
//            if (IsMultiGame || lvldir == ENTRY_LOAD || !IsLvlVisited(currLvl._dLevelIdx)) {
QMessageBox::critical(nullptr, "Error", "InitObjects start");
                InitObjects(); // place objects
QMessageBox::critical(nullptr, "Error", "InitObjects done");
                InitItems();   // place items
QMessageBox::critical(nullptr, "Error", "InitItems done");
                CreateThemeRooms(); // populate theme rooms
QMessageBox::critical(nullptr, "Error", "CreateThemeRooms done");
//            }
        } else {
//            InitTowners();
            IncProgress();
//            InitItems();
        }
QMessageBox::critical(nullptr, "Error", "FreeSetPieces start");
		FreeSetPieces();
    } else {
        LoadSetMap();
        IncProgress();
        // GetLevelMTypes();
        IncProgress();
        IncProgress();
        // InitMonsters();
        IncProgress();

        InitItems();
    }
QMessageBox::critical(nullptr, "Error", "Main done");
    IncProgress();
//    InitMissiles();
//    SavePreLighting();
    InitView(lvldir);
QMessageBox::critical(nullptr, "Error", "InitView done");

    IncProgress();

//    if (!IsMultiGame) {
        ResyncQuests();
//        if (lvldir != ENTRY_LOAD && IsLvlVisited(currLvl._dLevelIdx)) {
//            LoadLevel();
//        }
        //SyncPortals();
//    }
    IncProgress();
//    InitSync();
//    PlayDungMsgs();

//    guLvlVisited |= LEVEL_MASK(currLvl._dLevelIdx);

//    music_start(AllLevels[currLvl._dLevelIdx].dMusic);
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
//    if (IsMultiGame) {
//        DeltaSaveLevel();
//    } else {
//        SaveLevel();
//    }
//    IncProgress();
//    FreeLevelMem();
    EnterLevel(params.level);
    IncProgress();
    LoadGameLevel(params.entryMode, params.seed);

    bool hasSubtiles = pMegaTiles != NULL;
    FreeLvlDungeon();

    dun->setWidth(MAXDUNX, true);
    dun->setHeight(MAXDUNY, true);
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
    std::vector<std::pair<int, int>> objectTypes;
    std::set<int> itemTypes;
    // std::vector<int> monUniques;
    for (int y = 0; y < MAXDUNY; y++) {
        for (int x = 0; x < MAXDUNY; x++) {
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
                ObjectStruct *os = &objects[obj - 1];
                int otype = os->_otype;
                int animFrame = os->_oAnimFlag == OAM_LOOP ? 0 : os->_oAnimFrame;
                auto iter = objectTypes.begin();
                for (; iter != objectTypes.end(); iter++) {
                    if (iter->first == otype && iter->second == animFrame) {
                        break;
                    }
                }
                if (iter == objectTypes.end()) {
                    obj = objectTypes.size();
                    objectTypes.push_back(std::pair<int, int>(otype, animFrame));
                } else {
                    obj = std::distance(objectTypes.begin(), iter);
                }
                obj += lengthof(DunObjConvTbl);
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
        const std::pair<int, int> &objType = objectTypes[i];
        int otype = objType.first;
        AddResourceParam objRes = AddResourceParam();
        objRes.type = DUN_ENTITY_TYPE::OBJECT;
        objRes.index = lengthof(DunObjConvTbl) + i;
        objRes.name = objfiledata[objectdata[otype].ofindex].ofName; // TODO: object labels?
        objRes.path = assetPath + "/Objects/" + objfiledata[objectdata[otype].ofindex].ofName + ".CEL";
        objRes.width = objfiledata[objectdata[otype].ofindex].oAnimWidth;
        objRes.frame = objType.second;
        dun->addResource(objRes);
    }
    view->updateEntityOptions();

    view->scrollTo(ViewX, ViewY);

    return true;
}
