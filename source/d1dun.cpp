#include "d1dun.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QMessageBox>

#include "config.h"
#include "d1cel.h"
#include "d1cl2.h"
#include "d1pal.h"
#include "d1sol.h"
#include "d1til.h"
#include "d1tileset.h"
#include "d1tmi.h"
#include "d1trn.h"
#include "progressdialog.h"

#include "dungeon/all.h"

typedef struct SpecCell {
    int subtileRef;
    int dx;
    int dy;
    int specIndex;
} SpecCell;

typedef struct DunObjFileData {
    const char *path;
    int width;
    int numFrames;
} DunObjFileData;

typedef struct DunMonFileData {
    const char *path;
    int width;
} DunMonFileData;

typedef struct DungeonStruct {
    int defaultTile;
    const char *specPath;
    const SpecCell *specialCels;
    int numSpecCels;
} DungeonStruct;

static const SpecCell townSpecialCels[] = {
    // clang-format off
    { 360,  0,  0,  1 - 1 }, { 358,  0,  0,  2 - 1 }, { 129,  0,  0,  6 - 1 }, { 130,  0,  0,  7 - 1 }, { 128,  0,  0,  8 - 1 },
    { 117,  0,  0,  9 - 1 }, { 157,  0,  0, 10 - 1 }, { 158,  0,  0, 11 - 1 }, { 156,  0,  0, 12 - 1 }, { 162,  0,  0, 13 - 1 },
    { 160,  0,  0, 14 - 1 }, { 214,  0,  0, 15 - 1 }, { 212,  0,  0, 16 - 1 }, { 217,  0,  0, 17 - 1 }, { 216,  0,  0, 18 - 1 },
    // clang-format on
};

static const SpecCell l1SpecialCels[] = {
    // clang-format off
    {  12,  0,  0, 1 - 1 }, {  71,  0,  0, 1 - 1 }, { 211,  0,  0, 1 - 1 }, { 321,  0,  0, 1 - 1 }, { 341,  0,  0, 1 - 1 }, { 418,  0,  0, 1 - 1 },
    {  11,  0,  0, 2 - 1 }, { 249,  0,  0, 2 - 1 }, { 325,  0,  0, 2 - 1 }, { 331,  0,  0, 2 - 1 }, { 344,  0,  0, 2 - 1 }, { 421,  0,  0, 2 - 1 },
    { 253,  0,  0, 3 - 1 }, { 255,  0,  0, 4 - 1 }, { 259,  0,  0, 5 - 1 }, { 267,  0,  0, 6 - 1 }, { 417,  0,  1, 7 - 1 }, { 420,  1,  0, 8 - 1 },
    // clang-format on
};

static const SpecCell l2SpecialCels[] = {
    // clang-format off
    {  13,  0,  0, 5 - 1 }, { 178,  0,  0, 5 - 1 },/* { 541,  0,  0, 5 - 1 },*/ { 551,  0,  0, 5 - 1 }, {  17,  0,  0, 6 - 1 },/* { 542,  0,  0, 6 - 1 },*/ { 553,  0,  0, 6 - 1 },
    { 132,  0,  1, 2 - 1 }, { 132,  0,  2, 1 - 1 }, { 135,  1,  0, 3 - 1 }, { 135,  2,  0, 4 - 1 }, { 139,  1,  0, 3 - 1 }, { 139,  2,  0, 4 - 1 }
    // clang-format on
};

static const SpecCell l5SpecialCels[] = {
    // clang-format off
    { 77, 0, 0, 1 - 1 }, { 80, 0, 0, 2 - 1 }
    // clang-format on
};

const DungeonStruct dungeonTbl[NUM_DUNGEON_TYPES] = {
    // clang-format off
/* DTYPE_TOWN      */ { UNDEF_TILE, "Levels/TownData/Town", townSpecialCels, lengthof(townSpecialCels) },
/* DTYPE_CATHEDRAL */ { 13,         "Levels/L1Data/L1",     l1SpecialCels,   lengthof(l1SpecialCels) },
/* DTYPE_CATACOMBS */ { 3,          "Levels/L2Data/L2",     l2SpecialCels,   lengthof(l2SpecialCels) },
/* DTYPE_CAVES     */ { 7,          nullptr,                nullptr,         0 },
/* DTYPE_HELL      */ { 6,          nullptr,                nullptr,         0 },
/* DTYPE_CRYPT     */ { 13,         "NLevels/L5Data/L5",    l5SpecialCels,   lengthof(l5SpecialCels) },
/* DTYPE_NEST      */ { 7,          nullptr,                nullptr,         0 },
/* DTYPE_NONE      */ { UNDEF_TILE, nullptr,                nullptr,         0 },
    // clang-format on
};

const DunObjectStruct DunObjConvTbl[128] = {
    // clang-format off
                     { nullptr },
/*OBJ_LEVER*/        { "Lever" }, // Q_SKELKING
/*OBJ_CRUXM*/        { "Crucifix1" }, // Q_SKELKING
/*OBJ_CRUXR*/        { "Crucifix2" }, // Q_SKELKING
/*OBJ_CRUXL*/        { "Crucifix3" }, // Q_SKELKING
                     { nullptr }, //OBJ_ANGEL,
                     { nullptr }, //OBJ_BANNERL,
                     { nullptr }, //OBJ_BANNERM,
                     { nullptr }, //OBJ_BANNERR,
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
/*OBJ_BOOK2L*/       { "Bookstand" }, // Q_BCHAMB, Q_BETRAYER
                     { nullptr }, //OBJ_BOOK2R,
/*OBJ_TBCROSS*/      { "Burning cross" }, // Q_BCHAMB
                     { nullptr },
                     { nullptr }, //OBJ_CANDLE1,
/*OBJ_CANDLE2*/      { "Candle" }, // Q_BCHAMB
                     { nullptr }, //OBJ_CANDLEO,
                     { nullptr }, //OBJ_CAULDRON,
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr }, //OBJ_FLAMEHOLE,
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
/*OBJ_MCIRCLE1*/     { "Magic Circle 1" }, // Q_BETRAYER
/*OBJ_MCIRCLE2*/     { "Magic Circle 2" }, // Q_BETRAYER
                     { nullptr }, //OBJ_SKFIRE,
                     { nullptr }, //OBJ_SKPILE,
                     { nullptr }, //OBJ_SKSTICK1,
                     { nullptr }, //OBJ_SKSTICK2,
                     { nullptr }, //OBJ_SKSTICK3,
                     { nullptr }, //OBJ_SKSTICK4,
                     { nullptr }, //OBJ_SKSTICK5,
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
/*OBJ_SWITCHSKL*/    { "Switch" }, // Q_BCHAMB, Q_DIABLO
                     { nullptr },
                     { nullptr }, //OBJ_TRAPL,
                     { nullptr }, //OBJ_TRAPR,
/*OBJ_TORTUREL1*/    { "Tortured body 1" }, // Q_BUTCHER
/*OBJ_TORTUREL2*/    { "Tortured body 2" }, // Q_BUTCHER
/*OBJ_TORTUREL3*/    { "Tortured body 3" }, // Q_BUTCHER
/*OBJ_TORTUREL4*/    { "Tortured body 4" }, // Q_BUTCHER
/*OBJ_TORTUREL5*/    { "Tortured body 5" }, // Q_BUTCHER
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr }, //OBJ_NUDEW2R,
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
/*OBJ_TNUDEM*/       { "Tortured male 1" }, //1, Q_BUTCHER
/*OBJ_TNUDEM*/       { "Tortured male 2" }, //2, Q_BUTCHER
/*OBJ_TNUDEM*/       { "Tortured male 3" }, //3, Q_BUTCHER
/*OBJ_TNUDEM*/       { "Tortured male 4" }, //4, Q_BUTCHER
/*OBJ_TNUDEW*/       { "Tortured female 1" }, //1, Q_BUTCHER
/*OBJ_TNUDEW*/       { "Tortured female 2" }, //2, Q_BUTCHER
/*OBJ_TNUDEW*/       { "Tortured female 3" }, //3, Q_BUTCHER
                     { nullptr }, //OBJ_CHEST1,
/*OBJ_CHEST1*/       { "Chest 1" }, // Q_SKELKING
                     { nullptr }, //OBJ_CHEST1,
                     { nullptr }, //OBJ_CHEST2,
/*OBJ_CHEST2*/       { "Chest 2" }, // Q_SKELKING
                     { nullptr }, //OBJ_CHEST2,
                     { nullptr }, //OBJ_CHEST3,
/*OBJ_CHEST3*/       { "Chest 3" }, // Q_BCHAMB
                     { nullptr }, //OBJ_CHEST3,
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr }, //OBJ_PEDISTAL,
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
/*OBJ_ALTBOY*/       { "Altarboy" }, // Q_BETRAYER
                     { nullptr },
                     { nullptr },
/*OBJ_ARMORSTANDN*/  { "Armor stand" }, //OBJ_ARMORSTAND, // Q_WARLORD - changed to inactive versions to eliminate farming potential
/*OBJ_WEAPONRACKLN*/ { "Weapon stand" }, //OBJ_WEAPONRACKL, // Q_WARLORD
/*OBJ_TORCHR1*/      { "Torch 2" }, // Q_BLOOD
/*OBJ_TORCHL1*/      { "Torch 1" }, // Q_BLOOD
                     { nullptr }, //OBJ_MUSHPATCH,
                     { nullptr }, //OBJ_STAND,
                     { nullptr }, //OBJ_TORCHL2,
                     { nullptr }, //OBJ_TORCHR2,
                     { nullptr }, //OBJ_FLAMELVR,
                     { nullptr }, //OBJ_SARC,
                     { nullptr }, //OBJ_BARREL,
                     { nullptr }, //OBJ_BARRELEX,
                     { nullptr }, //OBJ_BOOKSHELF,
                     { nullptr }, //OBJ_BOOKCASEL,
                     { nullptr }, //OBJ_BOOKCASER,
                     { nullptr }, //OBJ_ARMORSTANDN,
                     { nullptr }, //OBJ_WEAPONRACKLN,
                     { nullptr }, //OBJ_BLOODFTN,
                     { nullptr }, //OBJ_PURIFYINGFTN,
                     { nullptr }, //OBJ_SHRINEL,
    // clang-format on
};

const DunMonsterStruct DunMonstConvTbl[128] = {
    // clang-format off
               { nullptr },
               { nullptr }, //MT_NZOMBIE,
               { nullptr }, //MT_BZOMBIE,
               { nullptr }, //MT_GZOMBIE,
               { nullptr }, //MT_YZOMBIE,
               { nullptr }, //MT_RFALLSP,
/*MT_DFALLSP*/ { "Carver" }, // Q_PWATER
               { nullptr }, //MT_YFALLSP,
               { nullptr }, // {   8, DMOFILE_FALLSP, "FalSpear\\Blue",  "Dark One" }, // Monster from banner2.dun,
               { nullptr }, //MT_WSKELAX,
               { nullptr }, //MT_TSKELAX,
/*MT_RSKELAX*/ { "Burning Dead" }, // Q_SKELKING
               { nullptr }, //MT_XSKELAX,
               { nullptr }, //MT_RFALLSD,
               { nullptr }, //MT_DFALLSD,
/*MT_YFALLSD*/ { "Devil Kin" }, // Q_PWATER
/*MT_BFALLSD*/ { "Dark One" },  // Q_BANNER
               { nullptr }, //MT_NSCAV,
               { nullptr }, //MT_BSCAV,
               { nullptr }, //MT_WSCAV,
               { nullptr }, //MT_YSCAV,
               { nullptr }, //MT_WSKELBW,
/*MT_TSKELBW*/ { "Corpse Bow" },   // Q_SKELKING
/*MT_RSKELBW*/ { "Burning Dead" }, // Q_SKELKING
/*MT_XSKELBW*/ { "Horror" },       // Q_SKELKING
               { nullptr }, //MT_WSKELSD,
               { nullptr }, //MT_TSKELSD,
/*MT_RSKELSD*/ { "Burning Dead Captain" }, // Q_SKELKING
/*MT_XSKELSD*/ { "Horror Captain" },       // Q_BCHAMB
               { nullptr }, //MT_NSNEAK,
               { nullptr }, //MT_RSNEAK,
/*MT_BSNEAK*/  { "Unseen" },            // Q_BCHAMB
/*MT_YSNEAK*/  { "Illusion Weaver" },   // Q_BLIND
/*MT_NGOATMC*/ { "Flesh Clan (Mace)" }, // Q_PWATER
               { nullptr }, //MT_BGOATMC,
               { nullptr }, //MT_RGOATMC,
               { nullptr }, //MT_GGOATMC,
               { nullptr }, //MT_RBAT,
               { nullptr }, //MT_GBAT,
               { nullptr }, //MT_NBAT,
               { nullptr }, //MT_XBAT,
/*MT_NGOATBW*/ { "Flesh Clan (Bow)" }, // Q_PWATER
               { nullptr }, //MT_BGOATBW,
               { nullptr }, //MT_RGOATBW,
/*MT_GGOATBW*/ { "Night Clan" }, // Q_ANVIL
               { nullptr }, //MT_NACID,
               { nullptr }, //MT_RACID,
               { nullptr }, //MT_BACID,
               { nullptr }, //MT_XACID,
               { nullptr }, //MT_SKING,
/*MT_NFAT*/    { "Overlord" }, // Q_BANNER
               { nullptr }, //MT_BFAT,
               { nullptr }, //MT_XFAT,
               { nullptr }, //MT_RFAT,
               { nullptr }, //MT_WYRM,
               { nullptr }, //MT_CAVSLUG,
               { nullptr }, //MT_DEVOUR,
               { nullptr }, //MT_DVLWYRM,
               { nullptr }, //MT_NMAGMA,
               { nullptr }, //MT_YMAGMA,
               { nullptr }, //MT_BMAGMA,
               { nullptr }, //MT_WMAGMA,
/*MT_NRHINO*/  { "Horned Demon" }, // Q_BLOOD, Q_BCHAMB
               { nullptr }, // MT_XRHINO, // Q_MAZE
               { nullptr }, //MT_BRHINO,
/*MT_DRHINO*/  { "Obsidian Lord" }, // Q_ANVIL
               { nullptr }, ///MT_BONEDMN,
               { nullptr }, ///MT_REDDTH,
               { nullptr }, ///MT_LTCHDMN,
               { nullptr }, ///MT_UDEDBLRG,
               { nullptr },
               { nullptr },
               { nullptr },
               { nullptr },
               { nullptr }, ///MT_INCIN,
               { nullptr }, ///MT_FLAMLRD,
               { nullptr }, ///MT_DOOMFIRE,
               { nullptr }, ///MT_HELLBURN,
               { nullptr },
               { nullptr },
               { nullptr },
               { nullptr },
               { nullptr }, //MT_NTHIN,
               { nullptr }, //MT_RTHIN,
               { nullptr }, //MT_XTHIN,
               { nullptr }, //MT_GTHIN,
               { nullptr }, //MT_NGARG,
               { nullptr }, //MT_XGARG,
               { nullptr }, //MT_DGARG,
               { nullptr }, //MT_BGARG,
               { nullptr }, //MT_NMEGA,
               { nullptr }, //MT_DMEGA,
               { nullptr }, //MT_BMEGA,
               { nullptr }, //MT_RMEGA,
               { nullptr }, //MT_NSNAKE,
               { nullptr }, //MT_RSNAKE,
               { nullptr }, //MT_GSNAKE,
               { nullptr }, //MT_BSNAKE,
/*MT_NBLACK*/  { "Black Knight" }, // Q_DIABLO
               { nullptr }, //MT_RBLACK,
/*MT_BBLACK*/  { "Steel Lord" },   // Q_WARLORD
/*MT_GBLACK*/  { "Blood Knight" }, // Q_DIABLO
               { nullptr }, ///MT_UNRAV,
               { nullptr }, ///MT_HOLOWONE,
               { nullptr }, ///MT_PAINMSTR,
               { nullptr }, ///MT_REALWEAV,
               { nullptr }, //MT_NSUCC,
               { nullptr }, //MT_GSUCC,
/*MT_RSUCC*/   { "Hell Spawn" }, // Q_BETRAYER
               { nullptr }, //MT_BSUCC,
               { nullptr }, //MT_NMAGE,
               { nullptr }, //MT_GMAGE,
               { nullptr }, //MT_XMAGE,
/*MT_BMAGE*/   { "Advocate" }, // Q_BETRAYER, Q_DIABLO
               { nullptr },
               { nullptr }, // { "The Dark Lord" }, // Diab4*.dun
               { nullptr },
               { nullptr }, //MT_GOLEM,
               { nullptr },
               { nullptr },
               { nullptr }, // Monster from blood1.dun and blood2.dun
               { nullptr },
               { nullptr },
               { nullptr },
               { nullptr }, // { "Snotspill" }, // Banner2.dun
               { nullptr },
               { nullptr },
               { nullptr }, ///MT_BIGFALL,
    // clang-format on
};

D1Dun::~D1Dun()
{
    // TODO: MemFree?
    delete this->specGfx;
    this->specGfx = nullptr;

    this->clearAssets();
}

void D1Dun::initVectors(int dunWidth, int dunHeight)
{
    tiles.resize(dunHeight);
    for (int i = 0; i < dunHeight; i++) {
        tiles[i].resize(dunWidth);
    }
    subtiles.resize(dunHeight * TILE_HEIGHT);
    for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
        subtiles[i].resize(dunWidth * TILE_WIDTH);
    }
    items.resize(dunHeight * TILE_HEIGHT);
    for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
        items[i].resize(dunWidth * TILE_WIDTH);
    }
    objects.resize(dunHeight * TILE_HEIGHT);
    for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
        objects[i].resize(dunWidth * TILE_WIDTH);
    }
    monsters.resize(dunHeight * TILE_HEIGHT);
    for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
        monsters[i].resize(dunWidth * TILE_WIDTH);
    }
    rooms.resize(dunHeight * TILE_HEIGHT);
    for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
        rooms[i].resize(dunWidth * TILE_WIDTH);
    }
}

bool D1Dun::load(const QString &filePath, const OpenAsParam &params)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.dunFilePath.isEmpty()) {
    //    filePath = params.dunFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly) && !params.dunFilePath.isEmpty()) {
            return false; // report read-error only if the file was explicitly requested
        }
    }

    const QByteArray fileData = file.readAll();

    unsigned fileSize = fileData.size();
    int dunWidth = 0;
    int dunHeight = 0;
    bool changed = fileSize == 0; // !file.isOpen();
    uint8_t numLayers = 0;
    D1DUN_TYPE type = filePath.toLower().endsWith(".dun") ? D1DUN_TYPE::NORMAL : D1DUN_TYPE::RAW;
    if (fileSize != 0) {
        if (type == D1DUN_TYPE::NORMAL) {
            // File size check
            if (fileSize < 2 * 2) {
                dProgressErr() << tr("Invalid DUN file.");
                return false;
            }

            // Read DUN binary data
            QDataStream in(fileData);
            in.setByteOrder(QDataStream::LittleEndian);

            quint16 readWord;
            in >> readWord;
            dunWidth = readWord;
            in >> readWord;
            dunHeight = readWord;

            unsigned tileCount = dunWidth * dunHeight;
            if (fileSize < 2 * 2 + tileCount * 2 || ((dunWidth != 0) != (dunHeight != 0))) {
                dProgressErr() << tr("Invalid DUN header.");
                return false;
            }
            unsigned dataSize = fileSize - (2 * 2 + tileCount * 2);

            // prepare the vectors
            this->initVectors(dunWidth, dunHeight);

            // read tiles
            for (int y = 0; y < dunHeight; y++) {
                for (int x = 0; x < dunWidth; x++) {
                    in >> readWord;
                    this->tiles[y][x] = readWord;
                }
            }

            const unsigned subtileCount = tileCount * TILE_WIDTH * TILE_HEIGHT;
            const unsigned dataUnitSize = subtileCount * sizeof(readWord);
            if (dataSize >= dataUnitSize) {
                dataSize -= dataUnitSize;
                // read items
                for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                    for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                        in >> readWord;
                        this->items[y][x] = readWord;
                    }
                }
                numLayers++;
            } else {
                dProgressWarn() << tr("Items are not defined in the DUN file.");
            }

            if (dataSize >= dataUnitSize) {
                dataSize -= dataUnitSize;
                // read monsters
                for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                    for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                        in >> readWord;
                        this->monsters[y][x] = readWord;
                    }
                }
                numLayers++;
            } else {
                dProgressWarn() << tr("Monsters are not defined in the DUN file.");
            }

            if (dataSize >= dataUnitSize) {
                dataSize -= dataUnitSize;
                // read objects
                for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                    for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                        in >> readWord;
                        this->objects[y][x] = readWord;
                    }
                }
                numLayers++;
            } else {
                dProgressWarn() << tr("Objects are not defined in the DUN file.");
            }

            if (dataSize >= dataUnitSize) {
                dataSize -= dataUnitSize;
                // read rooms
                for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                    for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                        in >> readWord;
                        this->rooms[y][x] = readWord;
                    }
                }
                numLayers++;
            } else {
                dProgressWarn() << tr("Rooms are not defined in the DUN file.");
            }

            if (dataSize > 0) {
                dProgressWarn() << tr("Unrecognizable data is ignored at the end of the DUN file.");
                changed = true;
            }
        } else {
            // rdun
            dunWidth = dunHeight = sqrt(fileSize / (TILE_WIDTH * TILE_HEIGHT * 4));
            if (fileSize != dunWidth * dunHeight * (TILE_WIDTH * TILE_HEIGHT * 4)) {
                dProgressErr() << tr("Invalid RDUN file.");
                return false;
            }

            // prepare the vectors
            this->initVectors(dunWidth, dunHeight);

            // Read RDUN binary data
            QDataStream in(fileData);
            in.setByteOrder(QDataStream::LittleEndian);

            quint32 readDword;
            // read subtiles
            for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                    in >> readDword;
                    this->subtiles[y][x] = readDword;
                }
            }
        }
    }

    this->type = type;
    this->width = dunWidth * TILE_WIDTH;
    this->height = dunHeight * TILE_HEIGHT;
    this->dunFilePath = filePath;
    this->modified = changed;
    this->numLayers = numLayers;
    this->defaultTile = UNDEF_TILE;
    this->specGfx = nullptr;
    return true;
}

void D1Dun::initialize(D1Pal *p, D1Tileset *ts)
{
    this->pal = p;
    this->tileset = ts;
    this->til = ts->til;
    this->min = ts->min;

    if (this->type == D1DUN_TYPE::NORMAL) {
        // prepare subtiles
        for (int y = 0; y < this->height / TILE_HEIGHT; y++) {
            for (int x = 0; x < this->width / TILE_WIDTH; x++) {
                this->updateSubtiles(x, y, this->tiles[y][x]);
            }
        }
    } else {
        // prepare tiles
        for (int y = 0; y < this->height / TILE_HEIGHT; y++) {
            for (int x = 0; x < this->width / TILE_WIDTH; x++) {
                this->tiles[y][x] = UNDEF_TILE;
            }
        }
    }

    // ensure there is at least one valid tile
    // assert((this->height == 0) == (this->width == 0));
    if (this->width == 0) {
        this->width = TILE_WIDTH;
        this->height = TILE_HEIGHT;

        this->modified = true;

        // prepare the vectors
        this->initVectors(1, 1);

        // prepare tiles
        this->tiles[0][0] = UNDEF_TILE;

        // prepare subtiles
        this->updateSubtiles(0, 0, UNDEF_TILE);
    }
    // calculate meta info
    QString tilPath = this->til->getFilePath();
    QFileInfo fileInfo = QFileInfo(tilPath);
    // initialize levelType based on the fileName of D1Til
    QString baseName = fileInfo.completeBaseName().toLower();
    int dungeonType = DTYPE_TOWN;
    if (baseName.length() == 2 && baseName[0] == 'l') { // TODO: merge with PatchTilesetDialog::initialize?
        switch (baseName[1].digitValue()) {
        case 1:
            dungeonType = DTYPE_CATHEDRAL;
            break;
        case 2:
            dungeonType = DTYPE_CATACOMBS;
            break;
        case 3:
            dungeonType = DTYPE_CAVES;
            break;
        case 4:
            dungeonType = DTYPE_HELL;
            break;
        case 5:
            dungeonType = DTYPE_CRYPT;
            break;
        case 6:
            dungeonType = DTYPE_NEST;
            break;
        }
    }
    this->levelType = DTYPE_NONE; // ensure change is triggered
    this->setLevelType(dungeonType);
    // select asset path
    QDir tilDir = fileInfo.dir();
    QString assetDir = tilDir.absolutePath();
    if (!tilDir.isRoot()) {
        // check if assetDir is ../*levels/../, use root if it matches
        QFileInfo dirInfo = QFileInfo(assetDir);
        QDir parentDir = dirInfo.dir();
        QString parentPath = parentDir.absolutePath().toLower();
        if (parentPath.endsWith("levels")) {
            QFileInfo parentInfo = QFileInfo(parentPath);
            parentDir = parentInfo.dir();
            assetDir = parentDir.absolutePath();
        }
    }
    this->setAssetPath(assetDir);
    this->loadSpecCels();
}

bool D1Dun::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.dunFilePath.isEmpty()) {
        filePath = params.dunFilePath;
        if (!params.autoOverwrite && QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to overwrite %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    } else if (!this->isModified()) {
        return false;
    }

    if (filePath.isEmpty()) {
        return false;
    }

    D1DUN_TYPE type = filePath.toLower().endsWith(".dun") ? D1DUN_TYPE::NORMAL : D1DUN_TYPE::RAW;
    // validate data
    // - check the active layers
    uint8_t layers = 0;
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            if (this->items[y][x] != 0) {
                layers |= 1 << 0;
            }
            if (this->monsters[y][x] != 0) {
                layers |= 1 << 1;
            }
            if (this->objects[y][x] != 0) {
                layers |= 1 << 2;
            }
            if (this->rooms[y][x] != 0) {
                layers |= 1 << 3;
            }
        }
    }
    uint8_t numLayers = this->numLayers;
    if (type == D1DUN_TYPE::NORMAL) {
        // dun - tiles must be defined
        int dunWidth = this->width / TILE_WIDTH;
        int dunHeight = this->height / TILE_HEIGHT;
        for (int y = 0; y < dunHeight; y++) {
            for (int x = 0; x < dunWidth; x++) {
                if (this->tiles[y][x] == UNDEF_TILE) {
                    dProgressFail() << tr("Undefined tiles (one at %1:%2) can not be saved in this format (DUN).").arg(x).arg(y);
                    return false;
                }
            }
        }
        // report inconsistent data (subtile vs tile)
        for (int y = 0; y < dunHeight; y++) {
            for (int x = 0; x < dunWidth; x++) {
                std::vector<int> subs = std::vector<int>(TILE_WIDTH * TILE_HEIGHT);
                int tileRef = this->tiles[y][x];
                if (tileRef == 0 && this->defaultTile != UNDEF_TILE) {
                    tileRef = this->defaultTile;
                }
                if (tileRef != 0) {
                    if (this->til->getTileCount() >= tileRef) {
                        subs = this->til->getSubtileIndices(tileRef - 1);
                        for (int &sub : subs) {
                            sub += 1;
                        }
                    } else {
                        continue; // skip check if there is no info
                    }
                }
                int posx = x * TILE_WIDTH;
                int posy = y * TILE_HEIGHT;
                for (int dy = 0; dy < TILE_HEIGHT; dy++) {
                    for (int dx = 0; dx < TILE_WIDTH; dx++) {
                        int dunx = posx + dx;
                        int duny = posy + dy;
                        if (this->subtiles[duny][dunx] != subs[dy * TILE_WIDTH + dx]) {
                            dProgressWarn() << tr("Subtile value at %1:%2 inconsistent with tile.").arg(dunx).arg(duny);
                        }
                    }
                }
            }
        }
        // calculate the number of layers
        uint8_t layersNeeded = layers >= (1 << 3) ? 4 : (layers >= (1 << 2) ? 3 : (layers >= (1 << 1) ? 2 : (layers >= (1 << 0) ? 1 : 0)));
        if (params.dunLayerNum != UINT8_MAX) {
            // user defined the number of layers -> report unsaved information
            if (params.dunLayerNum < layersNeeded) {
                if (params.dunLayerNum <= 0 && (layers & (1 << 0))) {
                    dProgressWarn() << tr("Defined item is not saved.");
                }
                if (params.dunLayerNum <= 1 && (layers & (1 << 1))) {
                    dProgressWarn() << tr("Defined monster is not saved.");
                }
                if (params.dunLayerNum <= 2 && (layers & (1 << 2))) {
                    dProgressWarn() << tr("Defined object is not saved.");
                }
                if (params.dunLayerNum <= 3 && (layers & (1 << 3))) {
                    dProgressWarn() << tr("Defined room is not saved.");
                }
            }
            numLayers = params.dunLayerNum;
        } else {
            // user did not define the number of layers -> extend if necessary, otherwise preserve
            if (layersNeeded > numLayers) {
                numLayers = layersNeeded;
            }
        }
    } else {
        // rdun - subtiles must be defined
        int dunWidth = this->width;
        int dunHeight = this->height;
        for (int y = 0; y < dunHeight; y++) {
            for (int x = 0; x < dunWidth; x++) {
                if (this->subtiles[y][x] == UNDEF_SUBTILE) {
                    dProgressFail() << tr("Undefined subtiles (one at %1:%2) can not be saved in this format (RDUN).").arg(x).arg(y);
                    return false;
                }
            }
        }
        // check if the user defined the number of layers
        if (params.dunLayerNum != UINT8_MAX && params.dunLayerNum != 0) {
            dProgressFail() << tr("Only the subtiles are saved in this format (RDUN).");
            return false;
        }
        // report unsaved information
        for (int y = 0; y < dunHeight / TILE_HEIGHT; y++) {
            for (int x = 0; x < dunWidth / TILE_WIDTH; x++) {
                if (this->tiles[y][x] != UNDEF_TILE && this->tiles[y][x] != 0) {
                    dProgressWarn() << tr("Defined tile at %1:%2 is not saved in this format (RDUN).").arg(x).arg(y);
                }
            }
        }
        if (layers & (1 << 0)) {
            dProgressWarn() << tr("Defined item is not saved in this format (RDUN).");
        }
        if (layers & (1 << 1)) {
            dProgressWarn() << tr("Defined monster is not saved in this format (RDUN).");
        }
        if (layers & (1 << 2)) {
            dProgressWarn() << tr("Defined object is not saved in this format (RDUN).");
        }
        if (layers & (1 << 3)) {
            dProgressWarn() << tr("Defined room is not saved in this format (RDUN).");
        }
        numLayers = 0;
    }

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    // write to file
    QDataStream out(&outFile);
    out.setByteOrder(QDataStream::LittleEndian);

    if (type == D1DUN_TYPE::NORMAL) {
        // dun
        int dunWidth = this->width / TILE_WIDTH;
        int dunHeight = this->height / TILE_HEIGHT;

        quint16 writeWord;
        writeWord = dunWidth;
        out << writeWord;
        writeWord = dunHeight;
        out << writeWord;

        // write tiles
        for (int y = 0; y < dunHeight; y++) {
            for (int x = 0; x < dunWidth; x++) {
                writeWord = this->tiles[y][x];
                out << writeWord;
            }
        }

        // write items
        if (numLayers >= 1) {
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    writeWord = this->items[y][x];
                    out << writeWord;
                }
            }
        }

        // write monsters
        if (numLayers >= 2) {
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    writeWord = this->monsters[y][x];
                    out << writeWord;
                }
            }
        }

        // write objects
        if (numLayers >= 3) {
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    writeWord = this->objects[y][x];
                    out << writeWord;
                }
            }
        }

        // write rooms
        if (numLayers >= 4) {
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    writeWord = this->rooms[y][x];
                    out << writeWord;
                }
            }
        }
    } else {
        // rdun
        int dunWidth = this->width;
        int dunHeight = this->height;

        quint32 writeDword;
        // write subtiles
        for (int x = 0; x < dunWidth; x++) {
            for (int y = 0; y < dunHeight; y++) {
                writeDword = this->subtiles[y][x];
                out << writeDword;
            }
        }
    }

    this->numLayers = numLayers;
    this->type = type;
    this->dunFilePath = filePath; // this->load(filePath, allocate);
    this->modified = false;
    return true;
}

#define CELL_BORDER 0

void D1Dun::drawDiamond(QImage &image, unsigned sx, unsigned sy, unsigned width, unsigned height, const QColor &color)
{
    unsigned len = 0;
    unsigned y = 1;
    QRgb *destBits = reinterpret_cast<QRgb *>(image.scanLine(sy + y));
    destBits += sx;
    QRgb srcBit = color.rgba();
    for (; y <= height / 2; y++) {
        len += 2;
        for (unsigned x = width / 2 - len - CELL_BORDER; x < width / 2 + len + CELL_BORDER; x++) {
            // image.setPixelColor(sx + x, sy + y, color);
            destBits[x] = srcBit;
        }
        destBits += image.width();
    }
    for (; y < height; y++) {
        len -= 2;
        for (unsigned x = width / 2 - len - CELL_BORDER; x < width / 2 + len + CELL_BORDER; x++) {
            // image.setPixelColor(sx + x, sy + y, color);
            destBits[x] = srcBit;
        }
        destBits += image.width();
    }
}

void D1Dun::drawImage(QPainter &dungeon, QImage &backImage, int drawCursorX, int drawCursorY, int dunCursorX, int dunCursorY, const DunDrawParam &params)
{
    const unsigned backWidth = backImage.width() - 2 * CELL_BORDER;
    const unsigned backHeight = backImage.height() - 2 * CELL_BORDER;
    unsigned cellCenterX = drawCursorX + backWidth / 2;
    unsigned cellCenterY = drawCursorY - backHeight / 2;
    bool middleText = false;
    bool bottomText = false;
    // draw the background
    dungeon.drawImage(drawCursorX - CELL_BORDER, drawCursorY - backHeight - CELL_BORDER, backImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
    if (params.showRooms) {
        unsigned roomIndex = this->rooms[dunCursorY][dunCursorX];
        if (roomIndex != 0) {
            QColor color = this->pal->getColor(roomIndex % D1PAL_COLORS);
            QImage *destImage = (QImage *)dungeon.device();
            D1Dun::drawDiamond(*destImage, drawCursorX, drawCursorY - backHeight, backWidth, backHeight, color);
        }
    }
    if (params.tileState != Qt::Unchecked) {
        // draw the subtile
        int tileRef = this->tiles[dunCursorY / TILE_HEIGHT][dunCursorX / TILE_WIDTH];
        int subtileRef = this->subtiles[dunCursorY][dunCursorX];
        if (subtileRef != 0) {
            if (subtileRef >= 0 && subtileRef <= this->min->getSubtileCount()) {
                if (params.tileState == Qt::Checked) {
                    QImage subtileImage = this->min->getSubtileImage(subtileRef - 1);
                    dungeon.drawImage(drawCursorX, drawCursorY - subtileImage.height(), subtileImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
                    /*for (int y = 0; y < subtileImage.height(); y++) {
                        for (int x = 0; x < subtileImage.width(); x++) {
                            QColor color = subtileImage.pixelColor(x, y);
                            if (color.alpha() == 0) {
                                continue;
                            }
                            destImage->setPixelColor(drawCursorX + x, drawCursorY - subtileImage.height() + y, color);
                        }
                    }*/
                } else {
                    // mask the image with backImage
                    QImage subtileImage = this->min->getFloorImage(subtileRef - 1);
                    QImage *destImage = (QImage *)dungeon.device();
                    QRgb *backBits = reinterpret_cast<QRgb *>(backImage.bits());
                    backBits += CELL_BORDER * backWidth;
                    // assert(subtileImage.height() >= backHeight);
                    QRgb *srcBits = reinterpret_cast<QRgb *>(subtileImage.scanLine(subtileImage.height() - backHeight));
                    // assert(drawCursorY >= backHeight);
                    QRgb *destBits = reinterpret_cast<QRgb *>(destImage->scanLine(drawCursorY - backHeight));
                    destBits += drawCursorX;
                    // assert(subtileImage.width() == backWidth);
                    for (unsigned y = CELL_BORDER; y < backHeight - CELL_BORDER; y++) {
                        for (unsigned x = CELL_BORDER; x < backWidth - CELL_BORDER; x++, backBits++, srcBits++, destBits++) {
                            // if (backImage.pixelColor(x, y).alpha() == 0) {
                            if (qAlpha(*backBits) == 0) {
                                continue;
                            }
                            // QColor color = subtileImage.pixelColor(x - CELL_BORDER, subtileImage.height() - y);
                            // if (/*color.isNull() ||*/ color.alpha() == 0) {
                            if (qAlpha(*srcBits) == 0) {
                                continue;
                            }
                            // destImage->setPixelColor(drawCursorX + x - CELL_BORDER, drawCursorY - (y + 1), color);
                            *destBits = *srcBits;
                        }
                        backBits += 2 * CELL_BORDER;
                        destBits += destImage->width() - backWidth;
                    }
                }
            } else {
                middleText = subtileRef != UNDEF_SUBTILE || tileRef == UNDEF_TILE;
                if (middleText) {
                    QString text = tr("Subtile%1");
                    if (subtileRef == UNDEF_SUBTILE) {
                        text = text.arg("???");
                    } else {
                        text = text.arg(subtileRef);
                    }
                    QFontMetrics fm(dungeon.font());
                    unsigned textWidth = fm.horizontalAdvance(text);
                    dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY - fm.height() / 2, text);
                }
            }
        }
        // draw tile text
        static_assert(TILE_WIDTH == 2 && TILE_HEIGHT == 2, "D1Dun::drawImage skips boundary checks.");
        if (tileRef > 0 && (dunCursorX & 1) && (dunCursorY & 1)) { // !0 || !UNDEF_TILE
            int undefSubtiles = 0;
            for (int dy = TILE_HEIGHT - 1; dy >= 0; dy--) {
                for (int dx = TILE_WIDTH - 1; dx >= 0; dx--) {
                    int subtileRef = this->subtiles[dunCursorY - dy][dunCursorX - dx];
                    if (subtileRef == UNDEF_SUBTILE) {
                        undefSubtiles++;
                    }
                }
            }
            if (undefSubtiles != 0) {
                QString text = tr("Tile%1").arg(tileRef);
                QFontMetrics fm(dungeon.font());
                unsigned textWidth = fm.horizontalAdvance(text);
                dungeon.drawText(cellCenterX - textWidth / 2, drawCursorY - backHeight - fm.height() / 2, text);
            }
        }
    }
    if (params.showObjects) {
        // draw the object
        int objectIndex = this->objects[dunCursorY][dunCursorX];
        if (objectIndex != 0) {
            const ObjectCacheEntry *objEntry = nullptr;
            for (const auto &obj : this->objectCache) {
                if (obj.objectIndex == objectIndex) {
                    objEntry = &obj;
                    break;
                }
            }
            if (objEntry == nullptr) {
                this->loadObject(objectIndex);
                objEntry = &this->objectCache.back();
            }
            if (objEntry->objGfx != nullptr) {
                int frameNum = objEntry->frameNum;
                if (frameNum == 0) {
                    frameNum = 1 + (params.time % objEntry->objGfx->getFrameCount());
                } else if (objEntry->objGfx->getFrameCount() < frameNum) {
                    frameNum = 1;
                }
                QImage objectImage = objEntry->objGfx->getFrameImage(frameNum - 1);
                dungeon.drawImage(drawCursorX + ((int)backWidth - objectImage.width()) / 2, drawCursorY - objectImage.height(), objectImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            } else {
                QString text = this->getObjectName(objectIndex);
                QFontMetrics fm(dungeon.font());
                unsigned textWidth = fm.horizontalAdvance(text);
                dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY + (middleText ? 1 : -1) * fm.height() / 2, text);
                bottomText = middleText;
            }
        }
    }
    if (params.showItems) {
        // draw the item
        int itemIndex = this->items[dunCursorY][dunCursorX];
        if (itemIndex != 0) {
            const ItemCacheEntry *itemEntry = nullptr;
            for (const auto &item : this->itemCache) {
                if (item.itemIndex == itemIndex) {
                    itemEntry = &item;
                    break;
                }
            }
            if (itemEntry == nullptr) {
                this->loadItem(itemIndex);
                itemEntry = &this->itemCache.back();
            }
            if (itemEntry->itemGfx != nullptr) {
                int frameNum = itemEntry->itemGfx->getFrameCount();
                QImage itemImage = itemEntry->itemGfx->getFrameImage(frameNum - 1);
                dungeon.drawImage(drawCursorX + ((int)backWidth - itemImage.width()) / 2, drawCursorY - itemImage.height(), itemImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            } else {
                QString text = this->getItemName(itemIndex);
                // dungeon.setFont(font);
                // dungeon.setPen(font);
                QFontMetrics fm(dungeon.font());
                unsigned textWidth = fm.horizontalAdvance(text);
                dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY + (bottomText ? 3 : 1) * fm.height() / 2, text);
            }
        }
    }
    if (params.showMonsters) {
        // draw the monster
        int monsterIndex = this->monsters[dunCursorY][dunCursorX];
        if (monsterIndex != 0) {
            const MonsterCacheEntry *monEntry = nullptr;
            for (const auto &mon : this->monsterCache) {
                if (mon.monsterIndex == monsterIndex) {
                    monEntry = &mon;
                    break;
                }
            }
            if (monEntry == nullptr) {
                this->loadMonster(monsterIndex);
                monEntry = &this->monsterCache.back();
            }
            if (monEntry->monGfx != nullptr) {
                std::pair<int, int> frameIndices = monEntry->monGfx->getGroupFrameIndices(0);
                int frameNum = 1 + (params.time % (frameIndices.second /*- frameIndices.first*/ + 1));
                monEntry->monGfx->setPalette(monEntry->monPal);
                QImage monImage = monEntry->monGfx->getFrameImage(frameNum - 1);
                dungeon.drawImage(drawCursorX + ((int)backWidth - monImage.width()) / 2, drawCursorY - monImage.height(), monImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            } else {
                QString text = this->getMonsterName(monsterIndex);
                QFontMetrics fm(dungeon.font());
                unsigned textWidth = fm.horizontalAdvance(text);
                dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY - (3 * fm.height() / 2), text);
            }
        }
    }
    if (params.tileState == Qt::Checked && specGfx != nullptr) {
        // draw special cel
        const DungeonStruct &ds = dungeonTbl[this->levelType];
        /*for (int dy = -2; dy <= 0; dy++) {
            int y = dunCursorY + dy;
            if (y < 0 || y >= this->height) {
                continue;
            }
            for (int dx = -2; dx <= 0; dx++) {
                int x = dunCursorX + dx;
                if (x < 0 || x >= this->width) {
                    continue;
                }
                int subtileRef = this->subtiles[y][x];
                if (subtileRef != 0) {
                    for (int i = 0; i < ds.numSpecCels; i++) {
                        const SpecCell &specCel = ds.specialCels[i];
                        if (specCel.subtileRef == subtileRef && x + specCel.dx == dunCursorX && y + specCel.dy == dunCursorY) {
                            QImage subtileImage = specGfx->getFrameImage(specCel.specIndex);
                            dungeon.drawImage(drawCursorX, drawCursorY - subtileImage.height(), subtileImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
                        }
                    }
                }
            }
        }*/
        for (int i = 0; i < ds.numSpecCels; i++) {
            const SpecCell &specCel = ds.specialCels[i];
            int x = dunCursorX - specCel.dx;
            if (x < 0 /*|| x >= this->width*/) {
                continue;
            }
            int y = dunCursorY - specCel.dy;
            if (y < 0 /*|| y >= this->height*/) {
                continue;
            }
            int subtileRef = this->subtiles[y][x];
            if (specCel.subtileRef == subtileRef && specGfx->getFrameCount() > specCel.specIndex) {
                QImage subtileImage = specGfx->getFrameImage(specCel.specIndex);
                dungeon.drawImage(drawCursorX, drawCursorY - subtileImage.height(), subtileImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            }
        }
    }
}

QImage D1Dun::getImage(const DunDrawParam &params)
{
    int maxDunSize = std::max(this->width, this->height);
    int minDunSize = std::min(this->width, this->height);
    int maxTilSize = std::max(TILE_WIDTH, TILE_HEIGHT);

    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    unsigned cellWidth = subtileWidth;
    unsigned cellHeight = cellWidth / 2;

    QImage dungeon = QImage(maxDunSize * cellWidth,
        (maxDunSize - 1) * cellHeight + subtileHeight, QImage::Format_ARGB32);
    dungeon.fill(Qt::transparent);

    // create template of the background image
    QImage backImage = QImage(cellWidth + 2 * CELL_BORDER, cellHeight + 2 * CELL_BORDER, QImage::Format_ARGB32);
    backImage.fill(Qt::transparent);
    QColor backColor = QColor(Config::getGraphicsTransparentColor());
    if (params.tileState != Qt::Unchecked) {
        D1Dun::drawDiamond(backImage, 0 + CELL_BORDER, 0 + CELL_BORDER, cellWidth, cellHeight, backColor);
    } else {
        unsigned len = 0;
        unsigned y = 1;
        QRgb *destBits = reinterpret_cast<QRgb *>(backImage.scanLine(0 + CELL_BORDER + y));
        destBits += 0;
        QRgb srcBit = backColor.rgba();
        for (; y <= cellHeight / 2; y++) {
            len += 2;
            for (unsigned x = cellWidth / 2 - len - CELL_BORDER - 1; x <= cellWidth / 2 - len; x++) {
                // backImage.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, backColor);
                destBits[x + CELL_BORDER] = srcBit;
            }
            for (unsigned x = cellWidth / 2 + len - 1; x <= cellWidth / 2 + len + CELL_BORDER; x++) {
                // backImage.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, backColor);
                destBits[x + CELL_BORDER] = srcBit;
            }
            destBits += cellWidth + 2 * CELL_BORDER; // backImage.width();
        }
        for (; y < cellHeight; y++) {
            len -= 2;
            for (unsigned x = cellWidth / 2 - len - CELL_BORDER - 1; x <= cellWidth / 2 - len; x++) {
                // backImage.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, backColor);
                destBits[x + CELL_BORDER] = srcBit;
            }
            for (unsigned x = cellWidth / 2 + len - 1; x <= cellWidth / 2 + len + CELL_BORDER; x++) {
                // backImage.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, backColor);
                destBits[x + CELL_BORDER] = srcBit;
            }
            destBits += cellWidth + 2 * CELL_BORDER; // backImage.width();
        }
    }

    QPainter dunPainter(&dungeon);
    dunPainter.setPen(QColor(Config::getPaletteUndefinedColor()));

    int drawCursorX = ((maxDunSize - 1) * cellWidth) / 2 - (this->width - this->height) * (cellWidth / 2);
    int drawCursorY = subtileHeight;
    int dunCursorX;
    int dunCursorY = 0;

    // draw top triangle
    for (int i = 0; i < minDunSize; i++) {
        dunCursorX = 0;
        dunCursorY = i;
        while (dunCursorY >= 0) {
            this->drawImage(dunPainter, backImage, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
            dunCursorY--;
            dunCursorX++;

            drawCursorX += cellWidth;
        }
        // move back to start
        drawCursorX -= cellWidth * (i + 1);
        // move down one row (+ half left)
        drawCursorX -= cellWidth / 2;
        drawCursorY += cellHeight / 2;
    }
    // draw middle 'square'
    if (this->width > this->height) {
        drawCursorX += cellWidth;
        for (int i = 0; i < this->width - this->height; i++) {
            dunCursorX = i + 1;
            dunCursorY = this->height - 1;
            while (dunCursorY >= 0) {
                this->drawImage(dunPainter, backImage, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
                dunCursorY--;
                dunCursorX++;

                drawCursorX += cellWidth;
            }
            // move back to start
            drawCursorX -= cellWidth * this->height;
            // move down one row (+ half right)
            drawCursorX += cellWidth / 2;
            drawCursorY += cellHeight / 2;
        }
        // sync drawCursorX with the other branches
        drawCursorX -= cellWidth;
    } else if (this->width < this->height) {
        for (int i = 0; i < this->height - this->width; i++) {
            dunCursorX = 0;
            dunCursorY = this->width + i;
            while (dunCursorX < this->width) {
                this->drawImage(dunPainter, backImage, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
                dunCursorY--;
                dunCursorX++;

                drawCursorX += cellWidth;
            }
            // move back to start
            drawCursorX -= cellWidth * this->width;
            // move down one row (+ half left)
            drawCursorX -= cellWidth / 2;
            drawCursorY += cellHeight / 2;
        }
    }
    // draw bottom triangle
    drawCursorX += cellWidth;
    for (int i = minDunSize - 1; i > 0; i--) {
        dunCursorX = this->width - i;
        dunCursorY = this->height - 1;
        while (dunCursorX < this->width) {
            this->drawImage(dunPainter, backImage, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
            dunCursorY--;
            dunCursorX++;

            drawCursorX += cellWidth;
        }
        // move back to start
        drawCursorX -= cellWidth * i;
        // move down one row (+ half right)
        drawCursorX += cellWidth / 2;
        drawCursorY += cellHeight / 2;
    }

    // dunPainter.end();
    return dungeon;
}

void D1Dun::setPal(D1Pal *pal)
{
    this->pal = pal;
    if (this->specGfx != nullptr) {
        this->specGfx->setPalette(pal);
    }
    for (auto &entry : this->objectCache) {
        entry.objGfx->setPalette(pal);
    }
    for (auto &entry : this->monsterCache) {
        D1Trn *monBaseTrn = entry.monBaseTrn;
        D1Trn *monUniqueTrn = entry.monUniqueTrn;
        if (monUniqueTrn != nullptr) {
            monUniqueTrn->setPalette(pal);
            monUniqueTrn->refreshResultingPalette();
            if (monBaseTrn != nullptr) {
                monBaseTrn->refreshResultingPalette();
            }
        } else if (monBaseTrn != nullptr) {
            monBaseTrn->setPalette(pal);
            monBaseTrn->refreshResultingPalette();
        } else {
            entry.monPal = pal;
        }
    }
}

QString D1Dun::getFilePath() const
{
    return this->dunFilePath;
}

bool D1Dun::isModified() const
{
    return this->modified;
}

uint8_t D1Dun::getNumLayers() const
{
    return this->numLayers;
}

int D1Dun::getWidth() const
{
    return this->width;
}

bool D1Dun::setWidth(int newWidth, bool force)
{
    if (newWidth % TILE_WIDTH != 0) {
        return false;
    }
    if (newWidth <= 0) { // TODO: check overflow
        return false;
    }
    int height = this->height;
    int prevWidth = this->width;
    int diff = newWidth - prevWidth;
    if (diff == 0) {
        return false;
    }
    if (diff < 0 && !force) {
        // check if there are non-zero values
        bool hasContent = false;
        for (int y = 0; y < height; y++) {
            for (int x = newWidth; x < prevWidth; x++) {
                hasContent |= this->tiles[y / TILE_HEIGHT][x / TILE_WIDTH] > 0; // !0 && !UNDEF_TILE
                hasContent |= this->subtiles[y][x] > 0;                         // !0 && !UNDEF_SUBTILE
                hasContent |= this->items[y][x] != 0;
                hasContent |= this->monsters[y][x] != 0;
                hasContent |= this->objects[y][x] != 0;
                hasContent |= this->rooms[y][x] != 0;
            }
        }

        if (hasContent) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Some content are going to be eliminited. Are you sure you want to proceed?"), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    }

    // resize the dungeon
    for (std::vector<int> &tilesRow : this->tiles) {
        tilesRow.resize(newWidth / TILE_WIDTH);
    }
    for (std::vector<int> &subtilesRow : this->subtiles) {
        subtilesRow.resize(newWidth);
    }
    for (std::vector<int> &itemsRow : this->items) {
        itemsRow.resize(newWidth);
    }
    for (std::vector<int> &monsRow : this->monsters) {
        monsRow.resize(newWidth);
    }
    for (std::vector<int> &objsRow : this->objects) {
        objsRow.resize(newWidth);
    }
    for (std::vector<int> &roomsRow : this->rooms) {
        roomsRow.resize(newWidth);
    }

    this->width = newWidth;
    this->modified = true;
    return true;
}

int D1Dun::getHeight() const
{
    return this->height;
}

bool D1Dun::setHeight(int newHeight, bool force)
{
    if (newHeight % TILE_HEIGHT != 0) {
        return false;
    }
    if (newHeight <= 0) { // TODO: check overflow
        return false;
    }
    int width = this->width;
    int prevHeight = this->height;
    int diff = newHeight - prevHeight;
    if (diff == 0) {
        return false;
    }
    if (diff < 0 && !force) {
        // check if there are non-zero values
        bool hasContent = false;
        for (int y = newHeight; y < prevHeight; y++) {
            for (int x = 0; x < width; x++) {
                hasContent |= this->tiles[y / TILE_HEIGHT][x / TILE_WIDTH] > 0; // !0 && !UNDEF_TILE
                hasContent |= this->subtiles[y][x] > 0;                         // !0 && !UNDEF_SUBTILE
                hasContent |= this->items[y][x] != 0;
                hasContent |= this->monsters[y][x] != 0;
                hasContent |= this->objects[y][x] != 0;
                hasContent |= this->rooms[y][x] != 0;
            }
        }

        if (hasContent) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Some content are going to be eliminited. Are you sure you want to proceed?"), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    }

    // resize the dungeon
    this->tiles.resize(newHeight / TILE_HEIGHT);
    this->subtiles.resize(newHeight);
    this->items.resize(newHeight);
    this->monsters.resize(newHeight);
    this->objects.resize(newHeight);
    this->rooms.resize(newHeight);
    for (int y = prevHeight; y < newHeight; y++) {
        this->tiles[y / TILE_HEIGHT].resize(width / TILE_WIDTH);
        this->subtiles[y].resize(width);
        this->items[y].resize(width);
        this->monsters[y].resize(width);
        this->objects[y].resize(width);
        this->rooms[y].resize(width);
    }

    this->height = newHeight;
    this->modified = true;
    return true;
}

int D1Dun::getTileAt(int posx, int posy) const
{
    return this->tiles[posy / TILE_HEIGHT][posx / TILE_WIDTH];
}

bool D1Dun::setTileAt(int posx, int posy, int tileRef)
{
    if (tileRef < 0) {
        tileRef = UNDEF_TILE;
    }
    int tilePosX = posx / TILE_WIDTH;
    int tilePosY = posy / TILE_HEIGHT;
    if (this->tiles[tilePosY][tilePosX] == tileRef) {
        return false;
    }
    this->tiles[tilePosY][tilePosX] = tileRef;
    // update subtile values
    this->updateSubtiles(tilePosX, tilePosY, tileRef);
    this->modified = true;
    return true;
}

int D1Dun::getSubtileAt(int posx, int posy) const
{
    return this->subtiles[posy][posx];
}

bool D1Dun::setSubtileAt(int posx, int posy, int subtileRef)
{
    if (subtileRef < 0) {
        subtileRef = UNDEF_SUBTILE;
    }
    if (this->subtiles[posy][posx] == subtileRef) {
        return false;
    }
    this->subtiles[posy][posx] = subtileRef;
    // update tile value
    int tilePosX = posx / TILE_WIDTH;
    int tilePosY = posy / TILE_HEIGHT;
    int tileRef = 0;
    for (int y = 0; y < TILE_HEIGHT; y++) {
        for (int x = 0; x < TILE_WIDTH; x++) {
            if (this->subtiles[tilePosY * TILE_HEIGHT + y][tilePosX * TILE_WIDTH + x] != 0) {
                tileRef = UNDEF_TILE;
            }
        }
    }

    this->tiles[tilePosY][tilePosX] = tileRef;
    this->modified = true;
    return true;
}

int D1Dun::getItemAt(int posx, int posy) const
{
    return this->items[posy][posx];
}

bool D1Dun::setItemAt(int posx, int posy, int itemIndex)
{
    if (this->items[posy][posx] == itemIndex) {
        return false;
    }
    this->items[posy][posx] = itemIndex;
    this->modified = true;
    return true;
}

int D1Dun::getMonsterAt(int posx, int posy) const
{
    return this->monsters[posy][posx];
}

bool D1Dun::setMonsterAt(int posx, int posy, int monsterIndex)
{
    if (this->monsters[posy][posx] == monsterIndex) {
        return false;
    }
    this->monsters[posy][posx] = monsterIndex;
    this->modified = true;
    return true;
}

int D1Dun::getObjectAt(int posx, int posy) const
{
    return this->objects[posy][posx];
}

bool D1Dun::setObjectAt(int posx, int posy, int objectIndex)
{
    if (this->objects[posy][posx] == objectIndex) {
        return false;
    }
    this->objects[posy][posx] = objectIndex;
    this->modified = true;
    return true;
}

int D1Dun::getRoomAt(int posx, int posy) const
{
    return this->rooms[posy][posx];
}

bool D1Dun::setRoomAt(int posx, int posy, int roomIndex)
{
    if (this->rooms[posy][posx] == roomIndex) {
        return false;
    }
    this->rooms[posy][posx] = roomIndex;
    this->modified = true;
    return true;
}

int D1Dun::getLevelType() const
{
    return this->levelType;
}

bool D1Dun::setLevelType(int levelType)
{
    if (this->levelType == levelType) {
        return false;
    }
    this->levelType = levelType;
    this->setDefaultTile(dungeonTbl[levelType].defaultTile);
    return true;
}

int D1Dun::getDefaultTile() const
{
    return this->defaultTile;
}

bool D1Dun::setDefaultTile(int defaultTile)
{
    if (defaultTile < 0) {
        defaultTile = UNDEF_TILE;
    }
    if (this->defaultTile == defaultTile) {
        return false;
    }
    this->defaultTile = defaultTile;
    // update subtiles
    for (unsigned y = 0; y < this->tiles.size(); y++) {
        std::vector<int> &tilesRow = this->tiles[y];
        for (unsigned x = 0; x < tilesRow.size(); x++) {
            if (tilesRow[x] == 0) {
                this->updateSubtiles(x, y, 0);
            }
        }
    }
    return true;
}

QString D1Dun::getAssetPath() const
{
    return this->assetPath;
}

bool D1Dun::setAssetPath(QString path)
{
    if (this->assetPath == path) {
        return false;
    }
    this->assetPath = path;
    this->clearAssets();
    return true;
}

const D1Gfx *D1Dun::getSpecGfx() const
{
    return this->specGfx;
}

QString D1Dun::getItemName(int itemIndex) const
{
    // check if it is a custom item
    for (const CustomItemStruct &customItem : customItemTypes) {
        if (customItem.type == itemIndex) {
            return customItem.name;
        }
    }
    // out of options -> generic name
    return tr("Item%1").arg(itemIndex);
}

QString D1Dun::getMonsterName(int monsterIndex) const
{
    // check if it is a custom monster
    for (const CustomMonsterStruct &customMonster : customMonsterTypes) {
        if (customMonster.type == monsterIndex) {
            return customMonster.name;
        }
    }
    // check if it is built-in monster
    if ((unsigned)monsterIndex < (unsigned)lengthof(DunMonstConvTbl) && DunMonstConvTbl[monsterIndex].name != nullptr) {
        return DunMonstConvTbl[monsterIndex].name;
    }
    // out of options -> generic name
    return tr("Monster%1").arg(monsterIndex);
}

QString D1Dun::getObjectName(int objectIndex) const
{
    // check if it is a custom object
    for (const CustomObjectStruct &customObject : customObjectTypes) {
        if (customObject.type == objectIndex) {
            return customObject.name;
        }
    }
    // check if it is built-in object
    if ((unsigned)objectIndex < (unsigned)lengthof(DunObjConvTbl) && DunObjConvTbl[objectIndex].name != nullptr) {
        return DunObjConvTbl[objectIndex].name;
    }
    // out of options -> generic name
    return tr("Object%1").arg(objectIndex);
}

void D1Dun::loadObjectGfx(const QString &filePath, int width, ObjectCacheEntry &result)
{
    // check for existing entry
    for (auto &dataEntry : this->objDataCache) {
        if (dataEntry.first->getFilePath() == filePath) {
            result.objGfx = dataEntry.first;
            dataEntry.second++;
            return;
        }
    }
    // create new entry
    result.objGfx = new D1Gfx();
    result.objGfx->setPalette(this->pal);
    OpenAsParam params = OpenAsParam();
    params.celWidth = width;
    D1Cel::load(*result.objGfx, filePath, params);
    if (result.objGfx->getFrameCount() != 0) {
        this->objDataCache.push_back(std::pair<D1Gfx *, unsigned>(result.objGfx, 1));
    } else {
        // TODO: suppress errors? MemFree?
        delete result.objGfx;
        result.objGfx = nullptr;
    }
}

void D1Dun::loadObject(int objectIndex)
{
    ObjectCacheEntry result = { objectIndex, nullptr, 0 };
    unsigned i = 0;
    for (; i < this->customObjectTypes.size(); i++) {
        const CustomObjectStruct &customObject = this->customObjectTypes[i];
        if (customObject.type == objectIndex) {
            result.frameNum = customObject.frameNum;
            QString celFilePath = customObject.path;
            this->loadObjectGfx(celFilePath, customObject.width, result);
            break;
        }
    }
    const BYTE *objType = &ObjConvTbl[objectIndex];
    if (i >= this->customObjectTypes.size() && (unsigned)objectIndex < (unsigned)lengthof(ObjConvTbl) && *objType != 0 && !this->assetPath.isEmpty()) {
        const ObjectData &od = objectdata[*objType];
        const ObjFileData &ofd = objfiledata[od.ofindex];
        result.frameNum = od.oAnimBaseFrame;
        if (result.frameNum == 0 && ofd.oAnimFlag != OAM_LOOP) {
            result.frameNum = 1;
        }
        QString celFilePath = this->assetPath + "/Objects/" + ofd.ofName + ".CEL";
        this->loadObjectGfx(celFilePath, ofd.oAnimWidth, result);
    }
    this->objectCache.push_back(result);
}

void D1Dun::loadMonsterGfx(const QString &filePath, int width, const QString &baseTrnFilePath, const QString &uniqueTrnFilePath, MonsterCacheEntry &result)
{
    // check for existing entry
    unsigned i = 0;
    for (; i < this->monDataCache.size(); i++) {
        auto &dataEntry = this->monDataCache[i];
        if (dataEntry.first->getFilePath() == filePath) {
            result.monGfx = dataEntry.first;
            dataEntry.second++;
            break;
        }
    }
    if (i >= this->monDataCache.size()) {
        // create new entry
        result.monGfx = new D1Gfx();
        // result.monGfx->setPalette(this->pal);
        OpenAsParam params = OpenAsParam();
        params.celWidth = width;
        D1Cl2::load(*result.monGfx, filePath, params);
        if (result.monGfx->getFrameCount() != 0) {
            this->monDataCache.push_back(std::pair<D1Gfx *, unsigned>(result.monGfx, 1));
        } else {
            // TODO: suppress errors? MemFree?
            delete result.monGfx;
            result.monGfx = nullptr;
            return;
        }
    }
    if (!uniqueTrnFilePath.isEmpty()) {
        D1Trn *trn = new D1Trn();
        if (trn->load(uniqueTrnFilePath, result.monPal)) {
            // apply Monster TRN
            /*for (int i = 0; i < D1PAL_COLORS; i++) {
                if (trn->getTranslation(i) == 0xFF) {
                    trn->setTranslation(i, 0);
                }
            }*/
            trn->refreshResultingPalette();
            // set palette
            result.monPal = trn->getResultingPalette();
        } else {
            // TODO: suppress errors? MemFree?
            delete trn;
            trn = nullptr;
        }
        result.monUniqueTrn = trn;
    }
    if (!baseTrnFilePath.isEmpty()) {
        D1Trn *trn = new D1Trn();
        if (trn->load(baseTrnFilePath, result.monPal)) {
            // apply Monster TRN
            for (int i = 0; i < D1PAL_COLORS; i++) {
                if (trn->getTranslation(i) == 0xFF) {
                    trn->setTranslation(i, 0);
                }
            }
            trn->refreshResultingPalette();
            // set palette
            result.monPal = trn->getResultingPalette();
        } else {
            // TODO: suppress errors? MemFree?
            delete trn;
            trn = nullptr;
        }
        result.monBaseTrn = trn;
    }
}

void D1Dun::loadMonster(int monsterIndex)
{
    MonsterCacheEntry result = { monsterIndex, nullptr, this->pal, nullptr, nullptr };
    unsigned i = 0;
    for (; i < this->customMonsterTypes.size(); i++) {
        const CustomMonsterStruct &customMonster = this->customMonsterTypes[i];
        if (customMonster.type == monsterIndex) {
            QString cl2FilePath = customMonster.path;
            this->loadMonsterGfx(cl2FilePath, customMonster.width, customMonster.baseTrnPath, customMonster.uniqueTrnPath, result);
            break;
        }
    }
    const BYTE *monType = &MonstConvTbl[monsterIndex];
    if (i >= this->customMonsterTypes.size() && (unsigned)monsterIndex < (unsigned)lengthof(MonstConvTbl) && *monType != 0 && !this->assetPath.isEmpty()) {
        const MonsterData &md = monsterdata[*monType];
        QString cl2FilePath = monfiledata[md.moFileNum].moGfxFile;
        cl2FilePath.replace("%c", "N");
        cl2FilePath = this->assetPath + "/" + cl2FilePath;
        QString baseTrnFilePath;
        if (md.mTransFile != nullptr) {
            baseTrnFilePath = this->assetPath + "/" + md.mTransFile;
        }
        QString uniqueTrnFilePath;
        this->loadMonsterGfx(cl2FilePath, monfiledata[md.moFileNum].moWidth, baseTrnFilePath, uniqueTrnFilePath, result);
    }
    this->monsterCache.push_back(result);
}

void D1Dun::loadItemGfx(const QString &filePath, int width, ItemCacheEntry &result)
{
    // check for existing entry
    for (auto &dataEntry : this->itemDataCache) {
        if (dataEntry.first->getFilePath() == filePath) {
            result.itemGfx = dataEntry.first;
            dataEntry.second++;
            return;
        }
    }
    // create new entry
    result.itemGfx = new D1Gfx();
    result.itemGfx->setPalette(this->pal);
    OpenAsParam params = OpenAsParam();
    params.celWidth = width;
    D1Cel::load(*result.itemGfx, filePath, params);
    if (result.itemGfx->getFrameCount() != 0) {
        this->itemDataCache.push_back(std::pair<D1Gfx *, unsigned>(result.itemGfx, 1));
    } else {
        // TODO: suppress errors? MemFree?
        delete result.itemGfx;
        result.itemGfx = nullptr;
    }
}

void D1Dun::loadItem(int itemIndex)
{
    ItemCacheEntry result = { itemIndex, nullptr };
    for (const CustomItemStruct &customItem : this->customItemTypes) {
        if (customItem.type == itemIndex) {
            QString celFilePath = customItem.path;
            this->loadItemGfx(celFilePath, customItem.width, result);
            break;
        }
    }
    this->itemCache.push_back(result);
}

void D1Dun::clearAssets()
{
    this->customObjectTypes.clear();
    this->customMonsterTypes.clear();
    this->customItemTypes.clear();
    this->objectCache.clear();
    for (auto &entry : this->monsterCache) {
        delete entry.monBaseTrn;
        delete entry.monUniqueTrn;
    }
    this->monsterCache.clear();
    this->itemCache.clear();
    for (auto &dataEntry : this->objDataCache) {
        delete dataEntry.first;
    }
    this->objDataCache.clear();
    for (auto &dataEntry : this->monDataCache) {
        delete dataEntry.first;
    }
    this->monDataCache.clear();
    for (auto &dataEntry : this->itemDataCache) {
        delete dataEntry.first;
    }
    this->itemDataCache.clear();
}

void D1Dun::updateSubtiles(int tilePosX, int tilePosY, int tileRef)
{
    std::vector<int> subs = std::vector<int>(TILE_WIDTH * TILE_HEIGHT);
    if (tileRef == 0 && this->defaultTile != UNDEF_TILE) {
        tileRef = this->defaultTile;
    }
    if (tileRef != 0) {
        if (tileRef != UNDEF_TILE && this->til->getTileCount() >= tileRef) {
            subs = this->til->getSubtileIndices(tileRef - 1);
            for (int &sub : subs) {
                sub += 1;
            }
        } else {
            for (int &sub : subs) {
                sub = UNDEF_SUBTILE;
            }
        }
    }
    int posx = tilePosX * TILE_WIDTH;
    int posy = tilePosY * TILE_HEIGHT;
    for (int y = 0; y < TILE_HEIGHT; y++) {
        for (int x = 0; x < TILE_WIDTH; x++) {
            int dunx = posx + x;
            int duny = posy + y;
            this->subtiles[duny][dunx] = subs[y * TILE_WIDTH + x];
            if (this->type == D1DUN_TYPE::RAW) {
                this->modified = true;
            }
        }
    }
}

void D1Dun::collectItems(std::vector<std::pair<int, int>> &foundItems) const
{
    for (const std::vector<int> &itemsRow : this->items) {
        for (int itemIndex : itemsRow) {
            if (itemIndex == 0) {
                continue;
            }
            for (std::pair<int, int> &itemEntry : foundItems) {
                if (itemEntry.first == itemIndex) {
                    itemEntry.second++;
                    itemIndex = 0;
                }
            }
            if (itemIndex != 0) {
                foundItems.push_back(std::pair<int, int>(itemIndex, 1));
            }
        }
    }
}

void D1Dun::collectMonsters(std::vector<std::pair<int, int>> &foundMonsters) const
{
    for (const std::vector<int> &monstersRow : this->monsters) {
        for (int monsterIndex : monstersRow) {
            if (monsterIndex == 0) {
                continue;
            }
            for (std::pair<int, int> &monsterEntry : foundMonsters) {
                if (monsterEntry.first == monsterIndex) {
                    monsterEntry.second++;
                    monsterIndex = 0;
                }
            }
            if (monsterIndex != 0) {
                foundMonsters.push_back(std::pair<int, int>(monsterIndex, 1));
            }
        }
    }
}

void D1Dun::collectObjects(std::vector<std::pair<int, int>> &foundObjects) const
{
    for (const std::vector<int> &objectsRow : this->objects) {
        for (int objectIndex : objectsRow) {
            if (objectIndex == 0) {
                continue;
            }
            for (std::pair<int, int> &objectEntry : foundObjects) {
                if (objectEntry.first == objectIndex) {
                    objectEntry.second++;
                    objectIndex = 0;
                }
            }
            if (objectIndex != 0) {
                foundObjects.push_back(std::pair<int, int>(objectIndex, 1));
            }
        }
    }
}

void D1Dun::checkTiles() const
{
    ProgressDialog::incBar(tr("Checking Tiles..."), 1);
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    progress.second = tr("Tile inconsistencies:");
    dProgress() << progress;
    for (int y = 0; y < this->height / TILE_HEIGHT; y++) {
        for (int x = 0; x < this->width / TILE_WIDTH; x++) {
            int tileRef = this->tiles[y][x];
            if (tileRef == 0) {
                continue;
            }
            if (tileRef == UNDEF_TILE) {
                if (this->type == D1DUN_TYPE::NORMAL) {
                    dProgressWarn() << tr("Tile at %1:%2 is undefined.").arg(x).arg(y);
                    result = true;
                }
            } else {
                if ((unsigned)tileRef > UCHAR_MAX) {
                    dProgressWarn() << tr("Tile-value at %1:%2 is not supported by the game.").arg(x).arg(y);
                    result = true;
                }
            }
        }
    }
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            int subtileRef = this->subtiles[y][x];
            if (subtileRef == 0) {
                continue;
            }
            if (subtileRef == UNDEF_SUBTILE) {
                if (this->type == D1DUN_TYPE::RAW) {
                    dProgressWarn() << tr("Subtile at %1:%2 is undefined.").arg(x).arg(y);
                    result = true;
                }
            }
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected with the Tiles.");
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

void D1Dun::checkItems(D1Sol *sol) const
{
    ProgressDialog::incBar(tr("Checking Items..."), 1);
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    progress.second = tr("Item inconsistencies:");
    dProgress() << progress;
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            int itemIndex = this->items[y][x];
            if (itemIndex == 0) {
                continue;
            }
            int subtileRef = this->subtiles[y][x];
            QString itemName = this->getItemName(itemIndex);
            if (subtileRef == UNDEF_SUBTILE) {
                dProgressWarn() << tr("'%1' at %2:%3 is on an undefined subtile.").arg(itemName).arg(x).arg(y);
                result = true;
            } else if (subtileRef == 0) {
                dProgressWarn() << tr("'%1' at %2:%3 is on an empty subtile.").arg(itemName).arg(x).arg(y);
                result = true;
            } else {
                quint8 solFlags = sol->getSubtileProperties(subtileRef - 1);
                if (solFlags & ((1 << 0) | (1 << 2))) {
                    dProgressErr() << tr("'%1' at %2:%3 is on a subtile which is not accessible (solid or missile blocker).").arg(itemName).arg(x).arg(y);
                    result = true;
                }
            }
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected with the Items.");
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

void D1Dun::checkMonsters(D1Sol *sol) const
{
    ProgressDialog::incBar(tr("Checking Monsters..."), 1);
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    progress.second = tr("Monster inconsistencies:");
    dProgress() << progress;
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            int monsterIndex = this->monsters[y][x];
            if (monsterIndex == 0) {
                continue;
            }
            int subtileRef = this->subtiles[y][x];
            QString monsterName = this->getMonsterName(monsterIndex);
            if (subtileRef == UNDEF_SUBTILE) {
                dProgressWarn() << tr("'%1' at %2:%3 is on an undefined subtile.").arg(monsterName).arg(x).arg(y);
                result = true;
            } else if (subtileRef == 0) {
                dProgressWarn() << tr("'%1' at %2:%3 is on an empty subtile.").arg(monsterName).arg(x).arg(y);
                result = true;
            } else {
                quint8 solFlags = sol->getSubtileProperties(subtileRef - 1);
                if (solFlags & ((1 << 0) | (1 << 2))) {
                    dProgressErr() << tr("'%1' at %2:%3 is on a subtile which is not accessible (solid or missile blocker).").arg(monsterName).arg(x).arg(y);
                    result = true;
                }
            }
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected with the Monsters.");
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

void D1Dun::checkObjects() const
{
    ProgressDialog::incBar(tr("Checking Objects..."), 1);
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    progress.second = tr("Object inconsistencies:");
    dProgress() << progress;
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            int objectIndex = this->objects[y][x];
            if (objectIndex == 0) {
                continue;
            }
            int subtileRef = this->subtiles[y][x];
            QString objectName = this->getObjectName(objectIndex);
            if (subtileRef == UNDEF_SUBTILE) {
                dProgressWarn() << tr("'%1' at %2:%3 is on an undefined subtile.").arg(objectName).arg(x).arg(y);
                result = true;
            } else if (subtileRef == 0) {
                dProgressWarn() << tr("'%1' at %2:%3 is on an empty subtile.").arg(objectName).arg(x).arg(y);
                result = true;
            }
            if (this->monsters[y][x] != 0) {
                dProgressErr() << tr("'%1' at %2:%3 is sharing a subtile with a monster.").arg(objectName).arg(x).arg(y);
                result = true;
            }
            if (this->items[y][x] != 0) {
                dProgressErr() << tr("'%1' at %2:%3 is sharing a subtile with an item.").arg(objectName).arg(x).arg(y);
                result = true;
            }
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected with the Objects.");
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

bool D1Dun::removeItems()
{
    bool result = false;
    for (std::vector<int> &itemsRow : this->items) {
        for (int &itemIndex : itemsRow) {
            if (itemIndex != 0) {
                itemIndex = 0;
                result = true;
                this->modified = true;
            }
        }
    }
    return result;
}

bool D1Dun::removeMonsters()
{
    bool result = false;
    for (std::vector<int> &monstersRow : this->monsters) {
        for (int &monsterIndex : monstersRow) {
            if (monsterIndex != 0) {
                monsterIndex = 0;
                result = true;
                this->modified = true;
            }
        }
    }
    return result;
}

bool D1Dun::removeObjects()
{
    bool result = false;
    for (std::vector<int> &objectsRow : this->objects) {
        for (int &objectIndex : objectsRow) {
            if (objectIndex != 0) {
                objectIndex = 0;
                result = true;
                this->modified = true;
            }
        }
    }
    return result;
}

bool D1Dun::removeRooms()
{
    bool result = false;
    for (std::vector<int> &roomRow : this->rooms) {
        for (int &roomIndex : roomRow) {
            if (roomIndex != 0) {
                roomIndex = 0;
                result = true;
                this->modified = true;
            }
        }
    }
    return result;
}

void D1Dun::loadItems(D1Dun *srcDun)
{
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            int newItemIndex = srcDun->items[y][x];
            int currItemIndex = this->items[y][x];
            if (newItemIndex != 0 && currItemIndex != newItemIndex) {
                if (currItemIndex != 0) {
                    QString currItemName = this->getItemName(currItemIndex);
                    QString newItemName = this->getItemName(newItemIndex);
                    dProgressWarn() << tr("'%1'(%2) item at %3:%4 was replaced by '%5'(%6).").arg(currItemName).arg(currItemIndex).arg(x).arg(y).arg(newItemName).arg(newItemIndex);
                }
                this->items[y][x] = newItemIndex;
                this->modified = true;
            }
        }
    }
}

void D1Dun::loadMonsters(D1Dun *srcDun)
{
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            int newMonsterIndex = srcDun->monsters[y][x];
            int currMonsterIndex = this->monsters[y][x];
            if (newMonsterIndex != 0 && currMonsterIndex != newMonsterIndex) {
                if (currMonsterIndex != 0) {
                    QString currMonsterName = this->getMonsterName(currMonsterIndex);
                    QString newMonsterName = this->getMonsterName(newMonsterIndex);
                    dProgressWarn() << tr("'%1'(%2) monster at %3:%4 was replaced by '%5'(%6).").arg(currMonsterName).arg(currMonsterIndex).arg(x).arg(y).arg(newMonsterName).arg(newMonsterIndex);
                }
                this->monsters[y][x] = newMonsterIndex;
                this->modified = true;
            }
        }
    }
}

void D1Dun::loadObjects(D1Dun *srcDun)
{
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            int newObjectIndex = srcDun->objects[y][x];
            int currObjectIndex = this->objects[y][x];
            if (newObjectIndex != 0 && currObjectIndex != newObjectIndex) {
                if (currObjectIndex != 0) {
                    QString currObjectName = this->getObjectName(currObjectIndex);
                    QString newObjectName = this->getObjectName(newObjectIndex);
                    dProgressWarn() << tr("'%1'(%2) object at %3:%4 was replaced by '%5'(%6).").arg(currObjectName).arg(currObjectIndex).arg(x).arg(y).arg(newObjectName).arg(newObjectIndex);
                }
                this->objects[y][x] = newObjectIndex;
                this->modified = true;
            }
        }
    }
}

void D1Dun::loadRooms(D1Dun *srcDun)
{
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            if (this->rooms[y][x] != srcDun->rooms[y][x]) {
                this->rooms[y][x] = srcDun->rooms[y][x];
                this->modified = true;
            }
        }
    }
}

bool D1Dun::resetTiles()
{
    ProgressDialog::incBar(tr("Checking tiles..."), 1);
    bool result = false;
    for (int tilePosY = 0; tilePosY < this->height / TILE_HEIGHT; tilePosY++) {
        for (int tilePosX = 0; tilePosX < this->width / TILE_WIDTH; tilePosX++) {
            int newTileRef = UNDEF_TILE;
            for (int i = 0; i < this->til->getTileCount(); i++) {
                std::vector<int> &subs = this->til->getSubtileIndices(i);
                int posx = tilePosX * TILE_WIDTH;
                int posy = tilePosY * TILE_HEIGHT;
                int y;
                for (y = 0; y < TILE_HEIGHT; y++) {
                    int x;
                    for (x = 0; x < TILE_WIDTH; x++) {
                        int dunx = posx + x;
                        int duny = posy + y;
                        if (this->subtiles[duny][dunx] != subs[y * TILE_WIDTH + x] + 1) {
                            break;
                        }
                    }
                    if (x < TILE_WIDTH) {
                        break;
                    }
                }
                if (y < TILE_HEIGHT) {
                    continue;
                }
                newTileRef = i + 1;
                break;
            }
            if (newTileRef == this->defaultTile) {
                newTileRef = 0;
            }
            int currTileRef = this->tiles[tilePosY][tilePosX];
            if (currTileRef != newTileRef) {
                if (newTileRef == UNDEF_TILE) {
                    dProgressWarn() << tr("Tile at %1:%2 is set to undefined, because no matching entry was found.").arg(tilePosX * TILE_WIDTH).arg(tilePosY * TILE_HEIGHT);
                } else {
                    dProgress() << tr("Tile%1 at %2:%3 was replaced with %4.").arg(currTileRef).arg(tilePosX * TILE_WIDTH).arg(tilePosY * TILE_HEIGHT).arg(newTileRef);
                }
                this->tiles[tilePosY][tilePosX] = newTileRef;
                result = true;
            }
        }
    }
    if (!result) {
        dProgress() << tr("No change was necessary.");
    } else {
        if (this->type == D1DUN_TYPE::NORMAL) {
            this->modified = true;
        }
    }

    ProgressDialog::decBar();
    return result;
}

bool D1Dun::resetSubtiles()
{
    ProgressDialog::incBar(tr("Checking subtiles..."), 1);
    bool result = false;
    for (int tilePosY = 0; tilePosY < this->height / TILE_HEIGHT; tilePosY++) {
        for (int tilePosX = 0; tilePosX < this->width / TILE_WIDTH; tilePosX++) {
            int tileRef = this->tiles[tilePosY][tilePosX];
            // this->updateSubtiles(tilePosX, tilePosY, tileRef);
            std::vector<int> subs = std::vector<int>(TILE_WIDTH * TILE_HEIGHT);
            if (tileRef == 0 && this->defaultTile != UNDEF_TILE) {
                tileRef = this->defaultTile;
            }
            if (tileRef != 0) {
                if (tileRef != UNDEF_TILE && this->til->getTileCount() >= tileRef) {
                    subs = this->til->getSubtileIndices(tileRef - 1);
                    for (int &sub : subs) {
                        sub += 1;
                    }
                } else {
                    for (int &sub : subs) {
                        sub = UNDEF_SUBTILE;
                    }
                }
            }
            int posx = tilePosX * TILE_WIDTH;
            int posy = tilePosY * TILE_HEIGHT;
            for (int y = 0; y < TILE_HEIGHT; y++) {
                for (int x = 0; x < TILE_WIDTH; x++) {
                    int dunx = posx + x;
                    int duny = posy + y;
                    int currSubtileRef = this->subtiles[duny][dunx];
                    int newSubtileRef = subs[y * TILE_WIDTH + x];
                    if (currSubtileRef != newSubtileRef) {
                        if (newSubtileRef == UNDEF_TILE) {
                            if (tileRef == UNDEF_TILE) {
                                dProgressWarn() << tr("Subtile at %1:%2 is set to undefined, because the corresponding tile is undefined.").arg(dunx).arg(duny);
                            } else {
                                dProgressWarn() << tr("Subtile at %1:%2 is set to undefined, because there was no tile-info for the corresponding tile (%3).").arg(dunx).arg(duny).arg(tileRef);
                            }
                        } else {
                            dProgress() << tr("Subtile%1 at %2:%3 was replaced with %4.").arg(currSubtileRef).arg(dunx).arg(duny).arg(newSubtileRef);
                        }
                        this->subtiles[duny][dunx] = newSubtileRef;
                        result = true;
                    }
                }
            }
        }
    }
    if (!result) {
        dProgress() << tr("No change was necessary.");
    } else {
        if (this->type == D1DUN_TYPE::RAW) {
            this->modified = true;
        }
    }

    ProgressDialog::decBar();
    return result;
}

bool D1Dun::changeTileAt(int tilePosX, int tilePosY, int tileRef)
{
    int prevTile = this->tiles[tilePosY][tilePosX];

    int posx = tilePosX * TILE_WIDTH;
    int posy = tilePosY * TILE_HEIGHT;
    if (this->setTileAt(posx, posy, tileRef)) {
        dProgress() << tr("Changed Tile at %1:%2 from '%3' to '%4'.").arg(posx).arg(posy).arg(prevTile).arg(tileRef);
        return true;
    }
    return false;
}

bool D1Dun::changeObjectAt(int posx, int posy, int objectIndex)
{
    int prevObject = this->objects[posy][posx];
    if (prevObject == objectIndex) {
        return false;
    }
    this->objects[posy][posx] = objectIndex;
    if (objectIndex == 0) {
        dProgress() << tr("Removed Object '%1' from %2:%3.").arg(prevObject).arg(posx).arg(posy);
    } else if (prevObject == 0) {
        dProgress() << tr("Added Object '%1' to %2:%3.").arg(objectIndex).arg(posx).arg(posy);
    } else {
        dProgress() << tr("Changed Object at %1:%2 from '%3' to '%4'.").arg(posx).arg(posy).arg(prevObject).arg(objectIndex);
    }
    this->modified = true;
    return true;
}

bool D1Dun::changeMonsterAt(int posx, int posy, int monsterIndex)
{
    int prevMonster = this->monsters[posy][posx];
    if (prevMonster == monsterIndex) {
        return false;
    }
    this->monsters[posy][posx] = monsterIndex;
    if (monsterIndex == 0) {
        dProgress() << tr("Removed Monster '%1' from %2:%3.").arg(prevMonster).arg(posx).arg(posy);
    } else if (prevMonster == 0) {
        dProgress() << tr("Added Monster '%1' to %2:%3.").arg(monsterIndex).arg(posx).arg(posy);
    } else {
        dProgress() << tr("Changed Monster at %1:%2 from '%3' to '%4'.").arg(posx).arg(posy).arg(prevMonster).arg(monsterIndex);
    }
    this->modified = true;
    return true;
}

bool D1Dun::changeItemAt(int posx, int posy, int itemIndex)
{
    int prevItem = this->items[posy][posx];
    if (prevItem == itemIndex) {
        return false;
    }
    this->items[posy][posx] = itemIndex;
    if (itemIndex == 0) {
        dProgress() << tr("Removed Item '%1' from %2:%3.").arg(prevItem).arg(posx).arg(posy);
    } else if (prevItem == 0) {
        dProgress() << tr("Added Item '%1' to %2:%3.").arg(itemIndex).arg(posx).arg(posy);
    } else {
        dProgress() << tr("Changed Item at %1:%2 from '%3' to '%4'.").arg(posx).arg(posy).arg(prevItem).arg(itemIndex);
    }
    this->modified = true;
    return true;
}

void D1Dun::patch(int dunFileIndex)
{
    const quint8 dunSizes[][2] {
        // clang-format off
/* DUN_SKELKING_ENTRY*/      { 14, 14 }, // SKngDO.DUN
/* DUN_SKELKING_PRE*/        { 74, 50 }, // SklKng2.DUN
/* DUN_SKELKING_AFT*/        { 74, 50 }, // SklKng1.DUN
/* DUN_BANNER_PRE*/          { 16, 16 }, // Banner2.DUN
/* DUN_BANNER_AFT*/          { 16, 16 }, // Banner1.DUN
/* DUN_BONECHAMB_ENTRY_PRE*/ { 14, 14 }, // Bonestr1.DUN
/* DUN_BONECHAMB_ENTRY_AFT*/ { 14, 14 }, // Bonestr2.DUN
/* DUN_BONECHAMB_PRE*/       { 64, 36 }, // Bonecha1.DUN
/* DUN_BONECHAMB_AFT*/       { 64, 36 }, // Bonecha2.DUN
/* DUN_BLIND_PRE*/           { 22, 22 }, // Blind2.DUN
/* DUN_BLIND_AFT*/           { 22, 22 }, // Blind1.DUN
/* DUN_BLOOD_PRE*/           { 20, 32 }, // Blood2.DUN
/* DUN_BLOOD_AFT*/           { 20, 32 }, // Blood1.DUN
/* DUN_VILE_PRE*/            { 42, 46 }, // Vile2.DUN
/* DUN_VILE_AFT*/            { 42, 46 }, // Vile1.DUN
/* DUN_WARLORD_PRE*/         { 16, 14 }, // Warlord.DUN
/* DUN_WARLORD_AFT*/         { 16, 14 }, // Warlord2.DUN
/* DUN_DIAB_2_AFT*/          { 22, 24 }, // Diab2b.DUN
/* DUN_DIAB_3_AFT*/          { 22, 22 }, // Diab3b.DUN
/* DUN_DIAB_4_AFT*/          { 18, 18 }, // Diab4b.DUN
        // clang-format on
    };
    if (this->width != dunSizes[dunFileIndex][0] || this->height != dunSizes[dunFileIndex][1]) {
        dProgressErr() << tr("Size of the dungeon does not match.");
        return;
    }
    bool change = false;
    switch (dunFileIndex) {
    case DUN_SKELKING_ENTRY: // SKngDO.DUN
        // patch set-piece to use common tiles
        change |= this->changeTileAt(5, 3, 203);
        change |= this->changeTileAt(5, 4, 22);
        // patch set-piece to use common tiles and make the inner tile at the entrance non-walkable
        change |= this->changeTileAt(5, 2, 203);
        break;
    case DUN_BONECHAMB_ENTRY_PRE: // Bonestr1.DUN
        // shadow of the external-left column
        change |= this->changeTileAt(0, 4, 48);
        change |= this->changeTileAt(0, 5, 50);
        break;
    case DUN_BONECHAMB_ENTRY_AFT: // Bonestr2.DUN
        // place shadows
        // NE-wall
        change |= this->changeTileAt(1, 0, 49);
        change |= this->changeTileAt(2, 0, 46);
        change |= this->changeTileAt(3, 0, 49);
        change |= this->changeTileAt(4, 0, 46);
        // SW-wall
        change |= this->changeTileAt(1, 4, 49);
        change |= this->changeTileAt(2, 4, 46);
        change |= this->changeTileAt(3, 4, 49);
        change |= this->changeTileAt(4, 4, 46);
        // NW-wall
        change |= this->changeTileAt(0, 0, 48);
        change |= this->changeTileAt(0, 1, 51);
        change |= this->changeTileAt(0, 2, 47);
        change |= this->changeTileAt(0, 3, 51);
        change |= this->changeTileAt(0, 4, 47);
        change |= this->changeTileAt(0, 5, 50);
        // SE-wall
        change |= this->changeTileAt(4, 1, 51);
        change |= this->changeTileAt(4, 2, 47);
        change |= this->changeTileAt(4, 3, 50); // 51
        // ensure the changing tiles are reserved
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 7; x++) {
                if (this->tiles[y][x] == 0) {
                    this->changeTileAt(x, y, 3);
                }
            }
        }
        break;
    case DUN_BONECHAMB_PRE: // Bonecha1.DUN
    case DUN_BONECHAMB_AFT: // Bonecha2.DUN
        // place pieces with closed doors
        change |= this->changeTileAt(17, 11, 150);
        // place shadows
        // - right corridor
        change |= this->changeTileAt(12, 6, 47);
        change |= this->changeTileAt(12, 7, 51);
        change |= this->changeTileAt(16, 6, 47);
        change |= this->changeTileAt(16, 7, 51);
        change |= this->changeTileAt(16, 8, 47);
        // - central room (top)
        change |= this->changeTileAt(17, 8, 49);
        change |= this->changeTileAt(18, 8, 46);
        change |= this->changeTileAt(19, 8, 49);
        change |= this->changeTileAt(20, 8, 46);
        // - central room (bottom)
        change |= this->changeTileAt(18, 12, 46);
        change |= this->changeTileAt(19, 12, 49);
        // - left corridor
        change |= this->changeTileAt(12, 14, 47);
        change |= this->changeTileAt(12, 15, 51);
        change |= this->changeTileAt(16, 14, 47);
        change |= this->changeTileAt(16, 15, 51);
        break;
    case DUN_BLIND_PRE: // Blind2.DUN
        // replace the door with wall
        change |= this->changeTileAt(4, 3, 25);
        // remove items
        change |= this->changeItemAt(5, 5, 0);
        break;
    case DUN_BLIND_AFT: // Blind1.DUN
        // place pieces with closed doors
        change |= this->changeTileAt(4, 3, 150);
        change |= this->changeTileAt(6, 7, 150);
        // ensure the changing tiles are reserved
        for (int y = 0; y < 11; y++) {
            for (int x = 0; x < 11; x++) {
                if (this->tiles[y][x] == 0) {
                    this->changeTileAt(x, y, 3);
                }
            }
        }
        // add monsters from Blind2.DUN
        change |= this->changeMonsterAt(1, 6, 32);
        change |= this->changeMonsterAt(4, 1, 32);
        change |= this->changeMonsterAt(6, 3, 32);
        change |= this->changeMonsterAt(6, 9, 32);
        change |= this->changeMonsterAt(5, 6, 32);
        change |= this->changeMonsterAt(7, 5, 32);
        change |= this->changeMonsterAt(7, 7, 32);
        change |= this->changeMonsterAt(13, 14, 32);
        change |= this->changeMonsterAt(14, 17, 32);
        change |= this->changeMonsterAt(14, 11, 32);
        change |= this->changeMonsterAt(15, 13, 32);
        // remove items
        change |= this->changeItemAt(5, 5, 0);
        break;
    case DUN_BLOOD_PRE: // Blood2.DUN
        // place pieces with closed doors
        change |= this->changeTileAt(4, 10, 151);
        change |= this->changeTileAt(4, 15, 151);
        change |= this->changeTileAt(5, 15, 151);
        // shadow of the external-left column -- do not place to prevent overwriting large decorations
        // this->tiles[7][-1] = 48;
        // this->tiles[8][-1] = 50;
        // shadow of the bottom-left column(s) -- one is missing
        change |= this->changeTileAt(1, 13, 48);
        change |= this->changeTileAt(1, 14, 50);
        // shadow of the internal column next to the pedistal
        change |= this->changeTileAt(5, 7, 142);
        change |= this->changeTileAt(5, 8, 50);
        // remove items
        change |= this->changeItemAt(9, 2, 0);
        break;
    case DUN_BLOOD_AFT: // Blood1.DUN
        // ensure the inner tiles are reserved
        change |= this->changeTileAt(5, 12, 3);
        // replace torches
        change |= this->changeObjectAt(11, 8, 110);
        change |= this->changeObjectAt(11, 10, 110);
        change |= this->changeObjectAt(11, 12, 110);
        change |= this->changeObjectAt(6, 8, 111);
        change |= this->changeObjectAt(7, 8, 0);
        change |= this->changeObjectAt(6, 10, 111);
        change |= this->changeObjectAt(7, 10, 0);
        change |= this->changeObjectAt(6, 12, 111);
        change |= this->changeObjectAt(7, 12, 0);
        // remove book and the pedistal
        change |= this->changeObjectAt(8, 24, 0);
        change |= this->changeObjectAt(9, 16, 0);
        // replace monsters
        // - corridor
        change |= this->changeMonsterAt(8, 3, 62);
        change |= this->changeMonsterAt(9, 3, 62);
        change |= this->changeMonsterAt(10, 3, 62);
        change |= this->changeMonsterAt(5, 4, 62);
        change |= this->changeMonsterAt(12, 4, 62);
        // - left room
        change |= this->changeMonsterAt(3, 8, 62);
        change |= this->changeMonsterAt(3, 12, 62);
        // - right room
        change |= this->changeMonsterAt(14, 8, 62);
        change |= this->changeMonsterAt(14, 12, 62);
        // - front room
        change |= this->changeMonsterAt(9, 22, 62);
        change |= this->changeMonsterAt(6, 25, 62);
        change |= this->changeMonsterAt(12, 25, 62);
        // remove items
        change |= this->changeItemAt(9, 2, 0);
        break;
    case DUN_VILE_PRE: // Vile2.DUN
        // replace default tiles with external piece I.
        // - central room
        change |= this->changeTileAt(8, 16, 203);
        /* fall-through */
    case DUN_VILE_AFT: // Vile1.DUN
        // replace default tiles with external piece
        // - SW in the middle
        change |= this->changeTileAt(12, 22, 203);
        change |= this->changeTileAt(13, 22, 203);
        change |= this->changeTileAt(14, 22, 203);
        // - SE
        for (int i = 1; i < 23; i++) {
            change |= this->changeTileAt(20, i, 22);
        }
        break;
    case DUN_WARLORD_PRE: // Warlord.DUN
        // ensure the changing tiles are reserved
        change |= this->changeTileAt(7, 2, 6);
        change |= this->changeTileAt(7, 3, 6);
        change |= this->changeTileAt(7, 4, 6);
        break;
    case DUN_WARLORD_AFT: // Warlord2.DUN
        // replace monsters from Warlord.DUN
        change |= this->changeMonsterAt(2, 2, 100);
        change |= this->changeMonsterAt(2, 10, 100);
        change |= this->changeMonsterAt(13, 4, 100);
        change |= this->changeMonsterAt(13, 9, 100);
        change |= this->changeMonsterAt(10, 2, 100);
        change |= this->changeMonsterAt(10, 10, 100);
        // add monsters from Warlord.DUN
        change |= this->changeMonsterAt(6, 2, 100);
        change |= this->changeMonsterAt(6, 10, 100);
        change |= this->changeMonsterAt(11, 2, 100);
        change |= this->changeMonsterAt(11, 10, 100);
        break;
    case DUN_BANNER_PRE: // Banner2.DUN
        // replace entry tile
        change |= this->changeTileAt(7, 6, 193);
        // replace monsters from Banner1.DUN
        for (int y = 7; y <= 9; y++) {
            for (int x = 7; x <= 13; x++) {
                change |= this->changeMonsterAt(x, y, 16);
            }
        }
        // remove monsters
        change |= this->changeMonsterAt(1, 4, 0);
        change |= this->changeMonsterAt(13, 5, 0);
        change |= this->changeMonsterAt(7, 12, 0);
        break;
    case DUN_BANNER_AFT: // Banner1.DUN
        break;
    case DUN_SKELKING_PRE: // SklKng2.DUN
        break;
    case DUN_SKELKING_AFT: // SklKng1.DUN
        // remove items
        // - room via crux
        change |= this->changeItemAt(17, 5, 0);
        change |= this->changeItemAt(17, 7, 0);
        change |= this->changeItemAt(17, 9, 0);
        change |= this->changeItemAt(29, 5, 0);
        change |= this->changeItemAt(29, 7, 0);
        change |= this->changeItemAt(29, 9, 0);
        // - room via lever
        change |= this->changeItemAt(41, 15, 0);
        change |= this->changeItemAt(41, 19, 0);
        // add chests
        // - room via crux
        change |= this->changeObjectAt(17, 10, 78);
        change |= this->changeObjectAt(29, 10, 78);
        // - back room
        change |= this->changeObjectAt(3, 31, 78);
        change |= this->changeObjectAt(3, 36, 78);
        change |= this->changeObjectAt(3, 34, 81);
        // - room via lever
        change |= this->changeObjectAt(41, 17, 81);
        // replace monsters from SklKng2.DUN
        // - back room
        change |= this->changeMonsterAt(6, 32, 27);
        change |= this->changeMonsterAt(6, 33, 22);
        change |= this->changeMonsterAt(6, 34, 22);
        change |= this->changeMonsterAt(6, 35, 27);
        change |= this->changeMonsterAt(8, 32, 27);
        change |= this->changeMonsterAt(8, 33, 27);
        change |= this->changeMonsterAt(8, 34, 27);
        change |= this->changeMonsterAt(8, 35, 27);
        change |= this->changeMonsterAt(10, 33, 11);
        change |= this->changeMonsterAt(10, 34, 11);
        change |= this->changeMonsterAt(12, 33, 11);
        change |= this->changeMonsterAt(12, 34, 11);
        // - room via crux
        change |= this->changeMonsterAt(19, 9, 24);
        // - central room
        change |= this->changeMonsterAt(12, 20, 24);
        change |= this->changeMonsterAt(18, 25, 23);
        change |= this->changeMonsterAt(18, 30, 27);
        change |= this->changeMonsterAt(18, 32, 27);
        change |= this->changeMonsterAt(33, 41, 23);
        change |= this->changeMonsterAt(39, 31, 11);
        break;
    case DUN_DIAB_2_AFT:
        // replace monsters from Diab2a.DUN
        change |= this->changeMonsterAt(11, 9, 101);
        change |= this->changeMonsterAt(11, 13, 101);
        change |= this->changeMonsterAt(16, 3, 101);
        change |= this->changeMonsterAt(16, 5, 101);
        change |= this->changeMonsterAt(17, 9, 101);
        change |= this->changeMonsterAt(17, 13, 101);
        change |= this->changeMonsterAt(14, 10, 101);
        change |= this->changeMonsterAt(13, 12, 101);
        break;
    case DUN_DIAB_3_AFT:
        // replace monsters from Diab3a.DUN
        change |= this->changeMonsterAt(1, 5, 101);
        change |= this->changeMonsterAt(1, 15, 101);
        change |= this->changeMonsterAt(5, 1, 101);
        change |= this->changeMonsterAt(5, 19, 101);
        change |= this->changeMonsterAt(7, 7, 101);
        change |= this->changeMonsterAt(7, 13, 101);
        change |= this->changeMonsterAt(13, 7, 101);
        change |= this->changeMonsterAt(13, 13, 101);
        change |= this->changeMonsterAt(15, 1, 101);
        change |= this->changeMonsterAt(15, 19, 101);
        change |= this->changeMonsterAt(19, 5, 101);
        change |= this->changeMonsterAt(19, 15, 101);
        // replace objects from Diab3a.DUN
        change |= this->changeObjectAt(8, 2, 51);
        break;
    case DUN_DIAB_4_AFT:
        // replace monsters from Diab4a.DUN
        change |= this->changeMonsterAt(4, 4, 101);
        change |= this->changeMonsterAt(4, 8, 101);
        change |= this->changeMonsterAt(4, 12, 101);
        change |= this->changeMonsterAt(8, 4, 101);
        change |= this->changeMonsterAt(8, 12, 101);
        change |= this->changeMonsterAt(12, 4, 101);
        change |= this->changeMonsterAt(12, 8, 101);
        change |= this->changeMonsterAt(12, 12, 101);
        // add monsters from Diab4a.DUN
        change |= this->changeMonsterAt(6, 12, 101);
        change |= this->changeMonsterAt(12, 10, 101);
        change |= this->changeMonsterAt(10, 4, 101);
        change |= this->changeMonsterAt(4, 6, 98);
        break;
    }
    if (!change) {
        dProgress() << tr("No change was necessary.");
    }
}

void D1Dun::loadSpecCels()
{
    // TODO: MemFree?
    delete this->specGfx;
    this->specGfx = nullptr;

    if (dungeonTbl[this->levelType].specPath != nullptr) {
        QString specFilePath = this->assetPath + "/" + dungeonTbl[this->levelType].specPath + "s.cel";
        if (!QFileInfo::exists(specFilePath)) {
            QString tilPath = this->til->getFilePath();
            QFileInfo fileInfo = QFileInfo(tilPath);
            QString specFilePath2 = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "s.cel";
            if (!QFileInfo::exists(specFilePath2)) {
                dProgressErr() << tr("Missing special-CEL. (Tried %1 and %2).").arg(QDir::toNativeSeparators(specFilePath)).arg(QDir::toNativeSeparators(specFilePath2));
                return;
            }
            specFilePath = specFilePath2;
        }
        D1Gfx *specGfx = new D1Gfx();
        specGfx->setPalette(this->pal);
        OpenAsParam params = OpenAsParam();
        params.celWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
        if (!D1Cel::load(*specGfx, specFilePath, params)) {
            dProgressErr() << tr("Failed loading special-CEL file: %1.").arg(QDir::toNativeSeparators(specFilePath));
            delete specGfx;
        } else {
            this->specGfx = specGfx;
            dProgress() << tr("Loaded special CEL file %1.").arg(QDir::toNativeSeparators(specFilePath));
        }
    }
}

bool D1Dun::reloadTileset(const QString &celFilePath)
{
    OpenAsParam params = OpenAsParam();
    params.celFilePath = celFilePath;
    if (!this->tileset->load(params)) {
        return false;
    }
    this->loadSpecCels();
    return true;
}

bool D1Dun::addResource(const AddResourceParam &params)
{
    switch (params.type) {
    case DUN_ENTITY_TYPE::OBJECT: {
        /* test if it overwrites an existing entry?
        for (const CustomObjectStruct customObject : this->customObjectTypes) {
            if (customObject.type == params.index) {
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to replace %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
                if (reply != QMessageBox::Yes) {
                    return false;
                }
            }
        }*/
        // check if the gfx can be loaded - TODO: merge with loadObject?
        D1Gfx *objGfx = new D1Gfx();
        // objGfx->setPalette(this->pal);
        QString celFilePath = params.path;
        OpenAsParam openParams = OpenAsParam();
        openParams.celWidth = params.width;
        D1Cel::load(*objGfx, celFilePath, openParams);
        bool result = objGfx->getFrameCount() >= (params.frame == 0 ? 1 : params.frame);
        delete objGfx;
        if (!result) {
            dProgressFail() << tr("Failed loading CEL file: %1.").arg(QDir::toNativeSeparators(celFilePath));
            return false;
        }
        // remove cache entry
        for (unsigned i = 0; i < this->objectCache.size(); i++) {
            if (this->objectCache[i].objectIndex == params.index) {
                D1Gfx *gfx = this->objectCache[i].objGfx;
                this->objectCache.erase(this->objectCache.begin() + i);
                if (gfx == nullptr) {
                    break; // previous entry without gfx -> done
                }
                for (i = 0; i < this->objDataCache.size(); i++) {
                    auto &dataEntry = this->objDataCache[i];
                    if (dataEntry.first == gfx) {
                        dataEntry.second--;
                        if (dataEntry.second == 0) {
                            this->objDataCache.erase(this->objDataCache.begin() + i);
                            delete gfx;
                        }
                        break;
                    }
                }
                break;
            }
        }
        // replace previous entry
        for (unsigned i = 0; i < this->customObjectTypes.size(); i++) {
            CustomObjectStruct &customObject = this->customObjectTypes[i];
            if (customObject.type == params.index) {
                customObject.name = params.name;
                customObject.path = params.path;
                customObject.frameNum = params.frame;
                customObject.width = params.width;
                return true;
            }
        }
        // add new entry
        CustomObjectStruct customObject;
        customObject.type = params.index;
        customObject.name = params.name;
        customObject.path = params.path;
        customObject.frameNum = params.frame;
        customObject.width = params.width;
        this->customObjectTypes.push_back(customObject);
    } break;
    case DUN_ENTITY_TYPE::MONSTER: {
        /* test if it overwrites an existing entry?
        for (const CustomMonsterStruct customMonster : this->customMonsterTypes) {
            if (customMonster.type == params.index) {
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to replace %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
                if (reply != QMessageBox::Yes) {
                    return false;
                }
            }
        }*/
        // check if the gfx can be loaded - TODO: merge with loadMonster?
        D1Gfx *monGfx = new D1Gfx();
        // monGfx->setPalette(this->pal);
        QString cl2FilePath = params.path;
        OpenAsParam openParams = OpenAsParam();
        openParams.celWidth = params.width;
        D1Cl2::load(*monGfx, cl2FilePath, openParams);
        bool result = monGfx->getFrameCount() != 0;
        delete monGfx;
        if (!result) {
            dProgressFail() << tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(cl2FilePath));
            return false;
        }
        // check if the TRNs can be loaded
        if (!params.baseTrnPath.isEmpty()) {
            D1Trn *monTrn = new D1Trn();
            result = monTrn->load(params.baseTrnPath, this->pal);
            delete monTrn;
            if (!result) {
                dProgressFail() << tr("Failed loading TRN file: %1.").arg(QDir::toNativeSeparators(params.baseTrnPath));
                return false;
            }
        }
        if (!params.uniqueTrnPath.isEmpty()) {
            D1Trn *monTrn = new D1Trn();
            result = monTrn->load(params.uniqueTrnPath, this->pal);
            delete monTrn;
            if (!result) {
                dProgressFail() << tr("Failed loading TRN file: %1.").arg(QDir::toNativeSeparators(params.uniqueTrnPath));
                return false;
            }
        }
        // remove cache entry
        for (unsigned i = 0; i < this->monsterCache.size(); i++) {
            if (this->monsterCache[i].monsterIndex == params.index) {
                D1Gfx *gfx = this->monsterCache[i].monGfx;
                this->monsterCache.erase(this->monsterCache.begin() + i);
                if (gfx == nullptr) {
                    break; // previous entry without gfx -> done
                }
                for (i = 0; i < this->monDataCache.size(); i++) {
                    auto &dataEntry = this->monDataCache[i];
                    if (dataEntry.first == gfx) {
                        dataEntry.second--;
                        if (dataEntry.second == 0) {
                            this->monDataCache.erase(this->monDataCache.begin() + i);
                            delete gfx;
                        }
                        break;
                    }
                }
                break;
            }
        }
        // replace previous entry
        for (unsigned i = 0; i < this->customMonsterTypes.size(); i++) {
            CustomMonsterStruct &customMonster = this->customMonsterTypes[i];
            if (customMonster.type == params.index) {
                customMonster.name = params.name;
                customMonster.path = params.path;
                customMonster.baseTrnPath = params.baseTrnPath;
                customMonster.uniqueTrnPath = params.uniqueTrnPath;
                customMonster.width = params.width;
                return true;
            }
        }
        // add new entry
        CustomMonsterStruct customMonster;
        customMonster.type = params.index;
        customMonster.name = params.name;
        customMonster.path = params.path;
        customMonster.baseTrnPath = params.baseTrnPath;
        customMonster.uniqueTrnPath = params.uniqueTrnPath;
        customMonster.width = params.width;
        this->customMonsterTypes.push_back(customMonster);
    } break;
    case DUN_ENTITY_TYPE::ITEM: {
        /* test if it overwrites an existing entry?
        for (const CustomItemStruct customItem : this->customItemTypes) {
            if (customItem.type == params.index) {
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to replace %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
                if (reply != QMessageBox::Yes) {
                    return false;
                }
            }
        }*/
        // check if the gfx can be loaded - TODO: merge with loadItem?
        D1Gfx *itemGfx = new D1Gfx();
        // itemGfx->setPalette(this->pal);
        QString celFilePath = params.path;
        OpenAsParam openParams = OpenAsParam();
        openParams.celWidth = params.width;
        D1Cel::load(*itemGfx, celFilePath, openParams);
        bool result = itemGfx->getFrameCount() != 0;
        delete itemGfx;
        if (!result) {
            dProgressFail() << tr("Failed loading CEL file: %1.").arg(QDir::toNativeSeparators(celFilePath));
            return false;
        }
        // remove cache entry
        for (unsigned i = 0; i < this->itemCache.size(); i++) {
            if (this->itemCache[i].itemIndex == params.index) {
                D1Gfx *gfx = this->itemCache[i].itemGfx;
                this->itemCache.erase(this->itemCache.begin() + i);
                if (gfx == nullptr) {
                    break; // previous entry without gfx -> done
                }
                for (i = 0; i < this->itemDataCache.size(); i++) {
                    auto &dataEntry = this->itemDataCache[i];
                    if (dataEntry.first == gfx) {
                        dataEntry.second--;
                        if (dataEntry.second == 0) {
                            this->itemDataCache.erase(this->itemDataCache.begin() + i);
                            delete gfx;
                        }
                        break;
                    }
                }
                break;
            }
        }
        // replace previous entry
        for (unsigned i = 0; i < this->customItemTypes.size(); i++) {
            CustomItemStruct &customItem = this->customItemTypes[i];
            if (customItem.type == params.index) {
                customItem.name = params.name;
                customItem.path = params.path;
                customItem.width = params.width;
                return true;
            }
        }
        // add new entry
        CustomItemStruct customItem;
        customItem.type = params.index;
        customItem.name = params.name;
        customItem.path = params.path;
        customItem.width = params.width;
        this->customItemTypes.push_back(customItem);
    } break;
    }

    return true;
}

const std::vector<CustomObjectStruct> &D1Dun::getCustomObjectTypes() const
{
    return this->customObjectTypes;
}

const std::vector<CustomMonsterStruct> &D1Dun::getCustomMonsterTypes() const
{
    return this->customMonsterTypes;
}

const std::vector<CustomItemStruct> &D1Dun::getCustomItemTypes() const
{
    return this->customItemTypes;
}
