/**
 * @file interfac.cpp
 *
 * Implementation of load screens.
 */
#include "all.h"

#include <QApplication>
#include <QString>

#include "../levelcelview.h"
#include "../progressdialog.h"

int ViewX;
int ViewY;
bool IsMultiGame;
bool IsHellfireGame;
bool HasTileset;
bool PatchDunFiles;
int ddLevelPlrs;
int dnLevel;
int dnType;
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

static void LogErrorF(const char* msg, ...)
{
	char tmp[256];
	/*snprintf(tmp, sizeof(tmp), "c:\\logdebug%d.txt", 0);
	FILE* f0 = fopen(tmp, "a+");
	if (f0 == NULL)
		return;*/

	va_list va;

	va_start(va, msg);

	vsnprintf(tmp, sizeof(tmp), msg, va);

	va_end(va);

	dProgress() << QString(tmp);
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

static void LoadTileset(D1Tileset *tileset)
{
	int entries;

	// 'load' TLA
	memset(nTrnShadowTable, 0, sizeof(nTrnShadowTable));
	entries = std::min(lengthof(nTrnShadowTable) - 1, tileset->til->getTileCount());
	for (int n = 0; n < entries; n++) {
		nTrnShadowTable[n + 1] = tileset->tla->getTileProperties(n);
	}

	// 'load' tiles
	memset(pTiles, 0, sizeof(pTiles));
	entries = std::min(lengthof(pTiles) - 1, tileset->til->getTileCount());
	for (int n = 0; n < entries; n++) {
		std::vector<int> &tilSubtiles = tileset->til->getSubtileIndices(n);
		for (int i = 0; i < lengthof(pTiles[0]) && i < (int)tilSubtiles.size(); i++) {
			pTiles[n + 1][i] = tilSubtiles[i] + 1;
		}
	}

	// 'load' special properties
	memset(nSpecTrapTable, 0, sizeof(nSpecTrapTable));
	entries = std::min(lengthof(nSpecTrapTable) - 1, tileset->sla->getSubtileCount());
	for (int n = 0; n < entries; n++) {
		quint8 bv = tileset->sla->getTrapProperty(n);
		nSpecTrapTable[n + 1] = bv;
	}

	// 'load' collision properties
	memset(nCollLightTable, 0, sizeof(nCollLightTable));
	memset(nSolidTable, 0, sizeof(nSolidTable));
	memset(nBlockTable, 0, sizeof(nBlockTable));
	memset(nMissileTable, 0, sizeof(nMissileTable));
	entries = std::min(lengthof(nSolidTable) - 1, tileset->sla->getSubtileCount());
	bool lightEmpty = true;
	for (int n = 0; n < entries; n++) {
		quint8 lr = tileset->sla->getLightRadius(n);
		nCollLightTable[n + 1] = lr;
		lightEmpty &= lr == 0;
		quint8 bv = tileset->sla->getSubProperties(n);
		nSolidTable[n + 1] = (bv & PSF_BLOCK_PATH) != 0;
		nBlockTable[n + 1] = (bv & PSF_BLOCK_LIGHT) != 0;
		nMissileTable[n + 1] = (bv & PSF_BLOCK_MISSILE) != 0;
	}
	nCollLightTable[0] = lightEmpty ? 1 : 0;

	// 'load' map properties
	memset(automaptype, 0, sizeof(automaptype));
	entries = std::min(lengthof(automaptype) - 1, tileset->sla->getSubtileCount());
	bool mapEmpty = true;
	for (int n = 0; n < entries; n++) {
		quint8 maptype = tileset->sla->getMapType(n);
		maptype |= tileset->sla->getMapProperties(n);
		automaptype[n + 1] = maptype;
		mapEmpty &= maptype == 0;
	}
	automaptype[0] = mapEmpty ? 1 : 0;
}

static void CreateDungeon()
{
	switch (currLvl._dDunType) {
	case DGT_TOWN:
		// CreateTown();
		break;
	case DGT_CATHEDRAL:
		CreateL1Dungeon();
		break;
	case DGT_CATACOMBS:
		CreateL2Dungeon();
		break;
	case DGT_CAVES:
		CreateL3Dungeon();
		break;
	case DGT_HELL:
		CreateL4Dungeon();
		break;
	default:
		ASSUME_UNREACHABLE
		break;
	}
}

static int GetBaseTile()
{
    int baseTile = 0;
    switch (currLvl._dDunType) {
    case DGT_TOWN:
        break;
    case DGT_CATHEDRAL:
        baseTile = 22; // BASE_MEGATILE_L1
        break;
    case DGT_CATACOMBS:
        baseTile = 12; // BASE_MEGATILE_L2
        break;
    case DGT_CAVES:
        baseTile = 8; // BASE_MEGATILE_L3
        break;
    case DGT_HELL:
        baseTile = 30; // BASE_MEGATILE_L4
        break;
    default:
        ASSUME_UNREACHABLE
        break;
    }
    return baseTile;
}

static void LoadGameLevel(int lvldir, D1Dun *dun)
{
	// extern int32_t sglGameSeed;
	// int32_t gameSeed = sglGameSeed;

	IncProgress();
	InitLvlDungeon();
	IncProgress();

	InitLvlAutomap();

	//if (lvldir != ENTRY_LOAD) {
//		InitLvlLighting();
//		InitLvlVision();
	//}
	InitLvlMonsters(); // reset monsters
	InitLvlObjects();  // reset objects
	InitLvlThemes();   // reset themes
	InitLvlItems();    // reset items
	IncProgress();

	// SetRndSeed(gameSeed); // restore seed after InitLvlMonsters
	// fill pre: pSetPieces
	// fill in loop: dungeon, pWarps, uses drlgFlags, dungBlock
	// fill post: themeLoc, pdungeon, dPiece, dTransVal
	CreateDungeon();
	// LoadLvlPalette();
	int rv = RandRange(1, 4);
	InitLvlMap(); // reset: dMonster, dObject, dPlayer, dItem, dMissile, dFlags+, dLight+
	StoreProtections(dun);
	IncProgress();
	if (currLvl._dType != DTYPE_TOWN) {
		GetLevelMTypes(); // select monster types and load their fx
		InitThemes();     // protect themes with dFlags and select theme types
		IncProgress();
		InitMonsters();   // place monsters
	} else {
//		InitLvlStores();
		// TODO: might want to reset RndSeed, since InitLvlStores is player dependent, but it does not matter at the moment
		// SetRndSeed(seed);
		IncProgress();

//		InitTowners();
	}
	IncProgress();
	InitObjectGFX();    // load object graphics
	IncProgress();
	InitObjects();      // place objects
	InitItems();        // place items
	CreateThemeRooms(); // populate theme rooms
	FreeSetPieces();
	IncProgress();
//	InitMissiles();
//	SavePreLighting();
	InitView(lvldir);

	IncProgress();

//	music_start(AllLevels[currLvl._dLevelNum].dMusic);
}

static void EnterLevel(int lvl, int seed)
{
	int lvlBonus;

	SetRndSeed(seed);
	currLvl._dLevelPlyrs = IsMultiGame ? ddLevelPlrs : 1;
	currLvl._dLevelIdx = lvl;
	currLvl._dDynLvl = lvl >= NUM_FIXLVLS;
	if (currLvl._dDynLvl) {
		// select level
		unsigned baseLevel = dnLevel;
		unsigned leveltype = dnType;
		// assert(baseLevel + HELL_LEVEL_BONUS < CF_LEVEL);
		int availableLvls[NUM_FIXLVLS];
		int numLvls = 0;
		for (int i = DLV_CATHEDRAL1; i < NUM_STDLVLS; i++) {
			if (AllLevels[i].dLevel <= baseLevel && (leveltype == DLV_TOWN || leveltype == AllLevels[i].dType) /*&& AllLevels[i].dMonTypes[0] != MT_INVALID*/) {
				availableLvls[numLvls] = i;
				numLvls++;
			}
		}
		lvl = DLV_CATHEDRAL1;
		if (numLvls != 0) {
			lvl = availableLvls[random_low(141, numLvls)];
		} else {
			baseLevel = AllLevels[DLV_CATHEDRAL1].dLevel;
		}
		currLvl._dLevelNum = lvl;
		currLvl._dLevel = AllLevels[lvl].dLevel;
		currLvl._dSetLvl = false; // AllLevels[lvl].dSetLvl;
		currLvl._dType = AllLevels[lvl].dType;
		currLvl._dDunType = AllLevels[lvl].dDunType;
		lvlBonus = baseLevel - AllLevels[lvl].dLevel;
	} else {
		currLvl._dLevelNum = lvl;
		currLvl._dLevel = AllLevels[lvl].dLevel;
		currLvl._dSetLvl = AllLevels[lvl].dSetLvl;
		currLvl._dType = AllLevels[lvl].dType;
		currLvl._dDunType = AllLevels[lvl].dDunType;
		lvlBonus = 0;
	}
	if (gnDifficulty == DIFF_NIGHTMARE) {
		lvlBonus += NIGHTMARE_LEVEL_BONUS;
	} else if (gnDifficulty == DIFF_HELL) {
		lvlBonus += HELL_LEVEL_BONUS;
	}
	currLvl._dLevelBonus = lvlBonus;
	currLvl._dLevel += lvlBonus;
}

void EnterGameLevel(D1Dun *dun, D1Tileset *tileset, LevelCelView *view, const GenerateDunParam &params)
{
    ddLevelPlrs = params.numPlayers;
    dnLevel = params.levelNum;
    dnType = params.levelType;
    IsMultiGame = params.isMulti;
    IsHellfireGame = params.isHellfire;
    gnDifficulty = params.difficulty;
    assetPath = dun->getAssetPath();
    HasTileset = params.useTileset && tileset != nullptr;
    PatchDunFiles = params.patchDunFiles;

    if (HasTileset) {
        LoadTileset(tileset);
    }

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
    int32_t lvlSeed = params.seed;
    EnterLevel(params.levelIdx, lvlSeed);
    IncProgress();

    int32_t questSeed = params.seedQuest;
    int extraRounds = params.extraRounds;
    // SetRndSeed(params.seed);
    while (true) {
        LogErrorF("Generating dungeon %d/%d with seed: %d / %d. Entry mode: %d", params.levelIdx, params.levelNum, lvlSeed, questSeed, params.entryMode);
        LoadGameLevel(params.entryMode, dun);
        FreeLvlDungeon();
        if (--extraRounds < 0) {
            break;
        }
        if (params.extraQuestRnd) {
            // QRandomGenerator *gen = QRandomGenerator::global();
            // questSeed = (int)gen->generate();
            questSeed = NextRndSeed();
            InitQuests(questSeed);
        }
        lvlSeed = NextRndSeed();
        EnterLevel(params.levelIdx, lvlSeed);
    }

    dun->setLevelType(currLvl._dType);

    int baseTile = GetBaseTile();
    for (int y = 0; y < MAXDUNY; y += TILE_HEIGHT) {
        for (int x = 0; x < MAXDUNX; x += TILE_WIDTH) {
            dun->setTileAt(x, y, baseTile);
        }
    }
    for (int y = 0; y < DMAXY; y++) {
        for (int x = 0; x < DMAXX; x++) {
            dun->setTileAt(DBORDERX + x * TILE_WIDTH, DBORDERY + y * TILE_HEIGHT, dungeon[x][y]);
        }
    }
    std::vector<ObjStruct> objectTypes;
    std::set<int> itemTypes;
    // std::vector<int> monUniques;
    for (int y = 0; y < MAXDUNY; y++) {
        for (int x = 0; x < MAXDUNX; x++) {
            dun->setSubtileAt(x, y, dPiece[x][y]);
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
                monType.monUnique = ms->_muniqtype != 0;
                if (!monType.monUnique) {
                    mon = ms->_mMTidx;
                    mon += lengthof(DunMonstConvTbl);
                } else {
                    // monUniques.push_back(mon - 1);
                    // mon = nummtypes + monUniques.size() - 1;
                    mon = ms->_muniqtype;
                }
                // mon += lengthof(DunMonstConvTbl);
                monType.monIndex = mon;
            }
            dun->setMonsterAt(x, y, monType, 0, 0);
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
        itemRes.name = AllItemList[itype].iName; // TODO: more specific names?
        itemRes.path = assetPath + "/Items/" + itemfiledata[ItemCAnimTbl[AllItemList[itype].iCurs]].ifName + ".CEL";
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
}

MonsterStruct* GetMonsterAt(int x, int y)
{
    MonsterStruct *result = NULL;
    if ((unsigned)x < MAXDUNX && (unsigned)y < MAXDUNY) {
        int mnum = dMonster[x][y];
        if (mnum != 0) {
            result = &monsters[mnum - 1];
        }
    }
    return result;
}
