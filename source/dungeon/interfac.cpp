/**
 * @file interfac.cpp
 *
 * Implementation of load screens.
 */
#include "all.h"

#include <QApplication>
#include <QElapsedTimer>
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
int ddLevelPlrs;
int dnLevel;
int dnType;
QString assetPath;
char infostr[256];
unsigned baseMonsters;
bool stopgen;
bool dooDebug;
QElapsedTimer* timer;
quint64 dt[16];
int counter1, counter2, lc2;

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
	
	snprintf(tmp, sizeof(tmp), "c:\\logdebug%d.txt", 0);
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
	InitLvlDungeon(); // load tiles + meta data, reset pWarps, pSetPieces
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
    baseMonsters = nummonsters;
	CreateThemeRooms(); // populate theme rooms
	FreeSetPieces();
	IncProgress();
//	InitMissiles();  // reset missiles
//	SavePreLighting(); // fill dPreLight
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

static void ResetGameLevel(D1Dun *dun, const DecorateDunParam &params)
{
    const MapMonster noMon = { { 0, false }, 0, 0, 0, 0 };
    const MapObject noObj = { 0, 0 };
    const MapMissile noMis = { 0, 0, 0, 0, 0 };
    for (int y = 0; y < dun->getHeight(); y++) {
        for (int x = 0; x < dun->getWidth(); x++) {
            if (params.resetMonsters && !dun->getSubtileMonProtectionAt(x, y)) {
                dun->setMonsterAt(x, y, noMon);
            }
            if (params.resetObjects && !dun->getSubtileObjProtectionAt(x, y)) {
                dun->setObjectAt(x, y, noObj);
            }
            if (params.resetItems) {
                dun->setItemAt(x, y, 0);
            }
            if (params.resetRooms) {
                dun->setRoomAt(x, y, 0);
            }
            if (params.resetMissiles) {
                dun->setMissileAt(x, y, noMis);
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
            // TODO: dMonster[DBORDERX + x][DBORDERX + y] = monType.monIndex;
            dObject[DBORDERX + x][DBORDERX + y] = dun->getObjectAt(baseX + x, baseY + y).oType;
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
            // TODO: dun->setMonsterAt(baseX + x, baseY + y, dMonster[DBORDERX + x][DBORDERY + y], 0, 0);
            const MapObject obj = { dObject[DBORDERX + x][DBORDERY + y], 0 };
            dun->setObjectAt(baseX + x, baseY + y, obj);
            dun->setItemAt(baseX + x, baseY + y, dItem[DBORDERX + x][DBORDERY + y]);
        }
    }
}

void DecorateGameLevel(D1Dun *dun, D1Tileset *tileset, LevelCelView *view, const DecorateDunParam &params)
{
    ddLevelPlrs = params.numPlayers;
    dnLevel = params.levelNum;
    dnType = params.levelType;
    IsMultiGame = params.isMulti;
    IsHellfireGame = params.isHellfire;
    gnDifficulty = params.difficulty;
    assetPath = dun->getAssetPath();
    HasTileset = params.useTileset && tileset != nullptr;

    if (HasTileset) {
        LoadTileset(tileset);
    }

    ResetGameLevel(dun, params);

    EnterLevel(params.levelIdx, params.seed);

    int extraRounds = params.extraRounds;
    // SetRndSeed(params.seed);
    do {
        LoadDungeon(dun, extraRounds == params.extraRounds);

        DecorateDungeon(dun, params);
    } while (--extraRounds >= 0);

    StoreDungeon(dun);
}
static unsigned totalMonsters;
static unsigned themeMonsters;
static int maxMonsters;
static int minMonsters;
static unsigned totalThemes;
static int maxThemes;
static int minThemes;
static unsigned rounds;
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
    stopgen = false;
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

    extern int minars[4];
    extern int maxars[4];
    extern int avgars[4]; 
    extern int cntars[4];
    for (int i = 0; i < 4; i++)
        minars[i] = INT_MAX;
    memset(maxars, 0, sizeof(maxars));
    memset(avgars, 0, sizeof(avgars));
    memset(cntars, 0, sizeof(cntars));
    rounds = 0;
    counter1 = 0;
    counter2 = 0;
    lc2 = 0;
    totalThemes = 0;
    totalMonsters = 0;
    themeMonsters = 0;
    minMonsters = INT_MAX;
    maxMonsters = 0;
    minThemes = INT_MAX;
    maxThemes = 0;
    int32_t questSeed = params.seedQuest;
    int extraRounds = params.extraRounds;
    memset(dt, 0, sizeof(dt));
    QElapsedTimer tmr;
    timer = &tmr;
    tmr.start();
    // SetRndSeed(params.seed);
    while (!stopgen /*true*/) {
        //LogErrorF("Generating dungeon %d/%d with seed: %d / %d. Entry mode: %d", params.levelIdx, params.levelNum, lvlSeed, questSeed, params.entryMode);
        dProgress() << QApplication::tr("Generating dungeon %1: %2/%3 with seed: %4 / %5. Entry mode: %6").arg(rounds).arg(params.levelIdx).arg(params.levelNum).arg(lvlSeed).arg(questSeed).arg(params.entryMode);
        // dooDebug = lvlSeed == 952458269 && questSeed == 1654566178;
        LoadGameLevel(params.entryMode, dun);
        FreeLvlDungeon();
        extern int nRoomCnt;
        dProgress() << QApplication::tr("Done. The dungeon contains %1/%2 monsters (%3 types), %4 objects and %5 items %6 themes %7 rooms. (%8:%9)").arg(nummonsters - MAX_MINIONS).arg(nummonsters - baseMonsters).arg(nummtypes - 1).arg(numobjects).arg(numitems).arg(numthemes).arg(nRoomCnt).arg(counter1).arg(counter2).arg(counter2 - lc2);
        rounds++;
        lc2 = counter2;
        totalMonsters += (nummonsters - MAX_MINIONS);
        themeMonsters += nummonsters - baseMonsters;
        if (nummonsters > maxMonsters)
            maxMonsters = nummonsters;
        if (nummonsters < minMonsters)
            minMonsters = nummonsters;
        totalThemes += numthemes;
        if (numthemes > maxThemes)
            maxThemes = numthemes;
        if (numthemes < minThemes)
            minThemes = numthemes;
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
        // lvlSeed = (lvlSeed >> 8) | (lvlSeed << 24); // _rotr(lvlSeed, 8)
        EnterLevel(params.levelIdx, lvlSeed);
    }
    dProgress() << QApplication::tr("Generated %1 dungeon. Elapsed time: %2ms. Monsters avg:%3/%4 min:%5 max:%6. Themes: avg:%7 total:%8 min:%9 max:%10 Leveltype %11. times(dun%12, mon%13, obj%14, themes%15)").arg(params.extraRounds - extraRounds).arg(tmr.elapsed()).arg(totalMonsters / rounds).arg(themeMonsters / rounds).arg(minMonsters - MAX_MINIONS).arg(maxMonsters - MAX_MINIONS).arg(totalThemes / rounds).arg(totalThemes).arg(minThemes).arg(maxThemes).arg(currLvl._dType).arg(dt[0]).arg(dt[1]).arg(dt[2]).arg(dt[3]);
    dProgress() << QApplication::tr("minareas(%1, %2, %3) maxareas(%4, %5, %6) avgareas(%7, %8, %9) (%10, %11, %12) cnt%13:%14").arg(minars[1]).arg(minars[2]).arg(minars[3]).arg(maxars[1]).arg(maxars[2]).arg(maxars[3]).arg(cntars[1] == 0 ? 0 : (avgars[1] / cntars[1])).arg(cntars[2] == 0 ? 0 : (avgars[2] / cntars[2])).arg(cntars[3] == 0 ? 0 : (avgars[3] / cntars[3])).arg(cntars[1]).arg(cntars[2]).arg(cntars[3]).arg(counter1 / rounds).arg(counter2 / rounds);

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
            int monIdx = dMonster[x][y];
            MapMonster mon = { { 0, false, false }, 0, 0, 0, 0 };
            if (monIdx != 0) {
                MonsterStruct *ms = &monsters[monIdx - 1];
                mon.moType.monUnique = ms->_muniqtype != 0;
                if (!mon.moType.monUnique) {
                    monIdx = ms->_mMTidx;
                    monIdx += lengthof(DunMonstConvTbl);
                } else {
                    // monUniques.push_back(monIdx - 1);
                    // monIdx = nummtypes + monUniques.size() - 1;
                    monIdx = ms->_muniqtype;
                }
                // mon += lengthof(DunMonstConvTbl);
                mon.moType.monIndex = monIdx;
            }
            dun->setMonsterAt(x, y, mon);
            int objIdx = dObject[x][y];
            MapObject obj = { 0, 0, false };
            if (objIdx > 0) {
                GetObjectStr(objIdx - 1);
                ObjectStruct *os = &objects[objIdx - 1];
                obj.oPreFlag = os->_oPreFlag;
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
                obj.oType = lengthof(DunObjConvTbl) + std::distance(objectTypes.begin(), iter);
                if (iter == objectTypes.end()) {
                    objectTypes.push_back(on);
                }
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
    for (int i = 0; i < nummtypes; i++) {
        AddResourceParam monRes = AddResourceParam();
        monRes.type = DUN_ENTITY_TYPE::MONSTER;
        monRes.index = lengthof(DunMonstConvTbl) + i;
        monRes.name = mapMonTypes[i].cmName;
        monRes.path = monfiledata[mapMonTypes[i].cmFileNum].moGfxFile;
        monRes.path.replace("%c", "N");
        monRes.path = assetPath + "/" + monRes.path;
        // monRes.width = monfiledata[mapMonTypes[i].cmFileNum].moWidth;
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
