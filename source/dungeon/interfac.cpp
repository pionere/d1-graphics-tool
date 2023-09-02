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

#include "../levelcelview.h"
#include "../progressdialog.h"

int ViewX;
int ViewY;
bool IsMultiGame;
bool IsHellfireGame;
bool HasTileset;
bool PatchDunFiles;
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
	char tmsg[256];
	va_list va;

	va_start(va, msg);

	vsnprintf(tmsg, sizeof(tmsg), msg, va);

	va_end(va);

	// dProgressErr() << QString(tmsg);
	
	snprintf(tmp, sizeof(tmp), "f:\\logdebug%d.txt", 0);
	FILE* f0 = fopen(tmp, "a+");
	if (f0 == NULL)
		return;

	fputs(tmsg, f0);

	fputc('\n', f0);

	fclose(f0);
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
		nSpecTrapTable[n + 1] = bv << 6;
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
	extern int32_t sglGameSeed;
	int32_t gameSeed = sglGameSeed;

	IncProgress();
	InitLvlDungeon(); // load tiles + meta data, reset pWarps, pSetPieces
	IncProgress();

	InitLvlAutomap();

	//if (lvldir != ENTRY_LOAD) {
	//	InitLighting();
	//	InitVision();
	//}
	InitLvlMonsters(); // reset monsters
	InitLvlObjects();  // reset objects
	InitLvlThemes();   // reset themes
	InitLvlItems();    // reset items
	IncProgress();

	SetRndSeed(gameSeed); // restore seed after InitLevelMonsters
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
		InitThemes();     // select theme types
		IncProgress();
		InitObjectGFX();  // load object graphics
		IncProgress();
		HoldThemeRooms(); // protect themes with dFlags
		InitMonsters();   // place monsters
		IncProgress();
		InitObjects();      // place objects
		InitItems();        // place items
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

static void ResetGameLevel(D1Dun *dun, const DecorateDunParam &params)
{
    const DunMonsterType noMon = { 0, false };
    for (int y = 0; y < dun->getHeight(); y++) {
        for (int x = 0; x < dun->getWidth(); x++) {
            if (params.resetMonsters && !dun->getSubtileMonProtectionAt(x, y)) {
                dun->setMonsterAt(x, y, noMon);
            }
            if (params.resetObjects && !dun->getSubtileObjProtectionAt(x, y)) {
                dun->setObjectAt(x, y, 0);
            }
            if (params.resetItems) {
                dun->setItemAt(x, y, 0);
            }
            if (params.resetRooms) {
                dun->setRoomAt(x, y, 0);
            }
        }
    }
}

static void LoadDungeon(D1Dun *dun, bool first)
{
    if (first && (dun->getWidth() > TILE_WIDTH * DMAXX || dun->getHeight() > TILE_HEIGHT * DMAXY)) {
        dProgressWarn() << QApplication::tr("Decoration is limited to %1x%2 dungeon.").arg(TILE_WIDTH * DMAXX).arg(TILE_HEIGHT * DMAXY);
    }

    memset(drlgFlags, 0, sizeof(drlgFlags));
    int baseTile = GetBaseTile();
    memset(dungeon, baseTile, sizeof(dungeon));

    int defaultTile = dun->getDefaultTile();
    int baseX = dun->getWidth() >= MAXDUNX ? DBORDERX : 0;
    int baseY = dun->getHeight() >= MAXDUNY ? DBORDERY : 0;
    for (int y = 0; y < dun->getHeight() / TILE_HEIGHT && y < DMAXY; y++) {
        for (int x = 0; x < dun->getWidth() / TILE_WIDTH && x < DMAXX; x++) {
            // load tile protection
            Qt::CheckState tps = dun->getTileProtectionAt(baseX + x * TILE_WIDTH, baseY + y * TILE_HEIGHT);
            if (tps == Qt::PartiallyChecked) {
                drlgFlags[x][y] |= DRLG_FROZEN;
            }
            else if (tps == Qt::Checked) {
                drlgFlags[x][y] |= DRLG_FROZEN | DRLG_PROTECTED;
            }
            // load tile
            int tile = dun->getTileAt(baseX + x * TILE_WIDTH, baseY + y * TILE_HEIGHT);
            if (tile == UNDEF_TILE) {
                continue;
            }
            if (tile == 0) {
                tile = defaultTile;
            }
            dungeon[x][y] = tile;
        }
    }
    memset(dPiece, 0, sizeof(dPiece));
    DRLG_InitTrans();
    /*memset(dFlags, 0, sizeof(dFlags));
    memset(dMonster, 0, sizeof(dMonster));
    memset(dObject, 0, sizeof(dObject));
    memset(dItem, 0, sizeof(dItem));*/
    InitLvlMap();
    for (int y = 0; y < dun->getHeight() && y < TILE_HEIGHT * DMAXY; y++) {
        for (int x = 0; x < dun->getWidth() && y < TILE_WIDTH * DMAXX; x++) {
            // load mon/obj protection
            if (dun->getSubtileMonProtectionAt(x, y)) {
                dFlags[DBORDERX + x][DBORDERX + y] |= BFLAG_MON_PROTECT;
            }
            if (dun->getSubtileObjProtectionAt(x, y)) {
                dFlags[DBORDERX + x][DBORDERX + y] |= BFLAG_OBJ_PROTECT;
            }
            // load room
            int room = dun->getRoomAt(baseX + x, baseY + y);
            if (room != 0) {
                dTransVal[DBORDERX + x][DBORDERX + y] = room;
                if (numtrans <= room) {
                    numtrans = room + 1;
                }
            }
            // load entities
            // DunMonsterType monType = dun->getMonsterAt(baseX + x, baseY + y);
            // TODO: dMonster[DBORDERX + x][DBORDERX + y] = monType.first;
            dObject[DBORDERX + x][DBORDERX + y] = dun->getObjectAt(baseX + x, baseY + y);
            dItem[DBORDERX + x][DBORDERX + y] = dun->getItemAt(baseX + x, baseY + y);
            // load subtile
            int subtile = dun->getSubtileAt(baseX + x, baseY + y);
            if (subtile == UNDEF_SUBTILE) {
                continue;
            }
            dPiece[DBORDERX + x][DBORDERX + y] = subtile;
        }
    }
}

static void DecorateDungeon(D1Dun *dun, const DecorateDunParam &params)
{
    if (params.addTiles) {
        switch (currLvl._dType) {
        case DTYPE_TOWN:
            // CreateTown();
            break;
        case DTYPE_CATHEDRAL:
            DRLG_L1Subs();
            break;
        case DTYPE_CATACOMBS:
            DRLG_L2Subs();
            break;
        case DTYPE_CAVES:
            DRLG_L3Subs();
            break;
        case DTYPE_HELL:
            DRLG_L4Subs();
            break;
        case DTYPE_CRYPT:
            DRLG_L5Subs();
            break;
        case DTYPE_NEST:
            DRLG_L6Subs();
            break;
        default:
            ASSUME_UNREACHABLE
            break;
        }
    }
    if (params.addShadows) {
        switch (currLvl._dType) {
        case DTYPE_TOWN:
            // CreateTown();
            break;
        case DTYPE_CATHEDRAL:
            DRLG_L1Shadows();
            break;
        case DTYPE_CATACOMBS:
            DRLG_L2Shadows();
            break;
        case DTYPE_CAVES:
            DRLG_L3Shadows();
            break;
        case DTYPE_HELL:
            DRLG_L4Shadows();
            break;
        case DTYPE_CRYPT:
            DRLG_L5Shadows();
            break;
        case DTYPE_NEST:
            DRLG_L6Shadows();
            break;
        default:
            ASSUME_UNREACHABLE
            break;
        }
    }
    if (params.addRooms) {
        // TODO: preserve the original rooms?
        switch (currLvl._dDunType) {
        case DGT_TOWN:
            // CreateTown();
            break;
        case DGT_CATHEDRAL:
            DRLG_L1InitTransVals();
            break;
        case DGT_CATACOMBS:
            DRLG_L2InitTransVals();
            break;
        case DGT_CAVES:
            DRLG_L3InitTransVals();
            break;
        case DGT_HELL:
            DRLG_L4InitTransVals();
            break;
        default:
            ASSUME_UNREACHABLE
            break;
        }
    }
    int baseTile = GetBaseTile();
    DRLG_PlaceMegaTiles(baseTile);
    /*if (params.addMonsters) {

    }
    if (params.addObjects) {

    }
    if (params.addItems) {

    }*/
}

static void StoreDungeon(D1Dun *dun)
{
    int baseX = dun->getWidth() >= MAXDUNX ? DBORDERX : 0;
    int baseY = dun->getHeight() >= MAXDUNY ? DBORDERY : 0;
    for (int y = 0; y < dun->getHeight() / TILE_HEIGHT && y < DMAXY; y++) {
        for (int x = 0; x < dun->getWidth() / TILE_WIDTH && x < DMAXX; x++) {
            dun->setTileAt(baseX + x * TILE_WIDTH, baseX + y * TILE_HEIGHT, dungeon[x][y]);
        }
    }

    for (int y = 0; y < dun->getHeight() && y < DMAXY * TILE_HEIGHT; y++) {
        for (int x = 0; x < dun->getWidth() && x < DMAXX * TILE_WIDTH; x++) {
            dun->setSubtileAt(baseX + x, baseY + y, dPiece[DBORDERX + x][DBORDERY + y]);
            dun->setRoomAt(baseX + x, baseY + y, dTransVal[DBORDERX + x][DBORDERY + y]);
            // TODO: dun->setMonsterAt(baseX + x, baseY + y, dMonster[DBORDERX + x][DBORDERY + y]);
            dun->setObjectAt(baseX + x, baseY + y, dObject[DBORDERX + x][DBORDERY + y]);
            dun->setItemAt(baseX + x, baseY + y, dItem[DBORDERX + x][DBORDERY + y]);
        }
    }
}

void DecorateGameLevel(D1Dun *dun, D1Tileset *tileset, LevelCelView *view, const DecorateDunParam &params)
{
    IsMultiGame = params.isMulti;
    IsHellfireGame = params.isHellfire;
    gnDifficulty = params.difficulty;
    assetPath = dun->getAssetPath();
    HasTileset = params.useTileset && tileset != nullptr;

    if (HasTileset) {
        LoadTileset(tileset);
    }

    ResetGameLevel(dun, params);

    EnterLevel(params.level);

    int extraRounds = params.extraRounds;
    SetRndSeed(params.seed);
    do {
        LoadDungeon(dun, extraRounds == params.extraRounds);

        DecorateDungeon(dun, params);
    } while (--extraRounds >= 0);

    StoreDungeon(dun);
}

void EnterGameLevel(D1Dun *dun, D1Tileset *tileset, LevelCelView *view, const GenerateDunParam &params)
{
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
    EnterLevel(params.level);
    IncProgress();

	int32_t questSeed = params.seedQuest;
	int extraRounds = params.extraRounds;
	quint64 started = QDateTime::currentMSecsSinceEpoch();
	SetRndSeed(params.seed);
	while (true) {
		extern int32_t sglGameSeed;
		//LogErrorF("Generating dungeon %d with seed: %d / %d. Entry mode: %d", params.level, sglGameSeed, params.seedQuest, params.entryMode);
		dProgress() << QApplication::tr("Generating dungeon %1 with seed: %2 / %3. Entry mode: %4").arg(params.level).arg(sglGameSeed).arg(questSeed).arg(params.entryMode);
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
	}
	quint64 now = QDateTime::currentMSecsSinceEpoch();
	dProgress() << QApplication::tr("Generated %1 dungeon. Elapsed time: %2ms.").arg(params.extraRounds + 1).arg(now - started);

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
}
