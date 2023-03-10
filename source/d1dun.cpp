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

template <class T, int N>
constexpr int lengthof(T (&arr)[N])
{
    return N;
}

typedef struct SpecCell {
    int subtileRef;
    int dx;
    int dy;
    int specIndex;
} SpecCell;

typedef struct ObjFileData {
    const char *path;
    int width;
    int numFrames;
} ObjFileData;

typedef struct MonFileData {
    const char *path;
    int width;
} MonFileData;

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

const ObjectStruct ObjConvTbl[128] = {
    // clang-format off
    { 0 },
    {   1, OFILE_LEVER,    "Lever", 1 }, // Q_SKELKING
    {   2, OFILE_CRUXSK1,  "Crucifix1", 1 }, // Q_SKELKING
    {   3, OFILE_CRUXSK2,  "Crucifix2", 1 }, // Q_SKELKING
    {   4, OFILE_CRUXSK3,  "Crucifix2", 1 }, // Q_SKELKING
    { 0 }, //OBJ_ANGEL,
    { 0 }, //OBJ_BANNERL,
    { 0 }, //OBJ_BANNERM,
    { 0 }, //OBJ_BANNERR,
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    {  14, OFILE_BOOK2,    "Bookstand", 1 }, // Q_BCHAMB, Q_BETRAYER
    { 0 }, //OBJ_BOOK2R,
    {  16, OFILE_BURNCROS, "Burning cross", 0 }, // Q_BCHAMB
    { 0 },
    { 0 }, //OBJ_CANDLE1,
    {  19, OFILE_CANDLE2,  "Candle", 0 }, // Q_BCHAMB
    { 0 }, //OBJ_CANDLEO,
    { 0 }, //OBJ_CAULDRON,
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 }, //OBJ_FLAMEHOLE,
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    {  36, OFILE_MCIRL,    "Magic Circle 1", 1 }, // Q_BETRAYER
    {  37, OFILE_MCIRL,    "Magic Circle 2", 3 }, // Q_BETRAYER
    { 0 }, //OBJ_SKFIRE,
    { 0 }, //OBJ_SKPILE,
    { 0 }, //OBJ_SKSTICK1,
    { 0 }, //OBJ_SKSTICK2,
    { 0 }, //OBJ_SKSTICK3,
    { 0 }, //OBJ_SKSTICK4,
    { 0 }, //OBJ_SKSTICK5,
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    {  51, OFILE_SWITCH4,  "Switch", 1 }, // Q_BCHAMB, Q_DIABLO
    { 0 },
    { 0 }, //OBJ_TRAPL,
    { 0 }, //OBJ_TRAPR,
    {  55, OFILE_TSOUL,    "Tortured body 1", 1 }, // Q_BUTCHER
    {  56, OFILE_TSOUL,    "Tortured body 2", 2 }, // Q_BUTCHER
    {  57, OFILE_TSOUL,    "Tortured body 3", 3 }, // Q_BUTCHER
    {  58, OFILE_TSOUL,    "Tortured body 4", 4 }, // Q_BUTCHER
    {  59, OFILE_TSOUL,    "Tortured body 5", 5 }, // Q_BUTCHER
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 }, //OBJ_NUDEW2R,
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    {  70, OFILE_TNUDEM,   "Tortured male 1", 1 }, //1, Q_BUTCHER
    {  71, OFILE_TNUDEM,   "Tortured male 2", 2 }, //2, Q_BUTCHER
    {  72, OFILE_TNUDEM,   "Tortured male 3", 3 }, //3, Q_BUTCHER
    {  73, OFILE_TNUDEM,   "Tortured male 4", 4 }, //4, Q_BUTCHER
    {  74, OFILE_TNUDEW,   "Tortured female 1", 1 }, //1, Q_BUTCHER
    {  75, OFILE_TNUDEW,   "Tortured female 1", 2 }, //2, Q_BUTCHER
    {  76, OFILE_TNUDEW,   "Tortured female 1", 3 }, //3, Q_BUTCHER
    { 0 }, //OBJ_CHEST1,
    {  78, OFILE_CHEST1,   "Chest 1", 1 }, // Q_SKELKING
    { 0 }, //OBJ_CHEST1,
    { 0 }, //OBJ_CHEST2,
    {  81, OFILE_CHEST2,   "Chest 2", 1 }, // Q_SKELKING
    { 0 }, //OBJ_CHEST2,
    { 0 }, //OBJ_CHEST3,
    {  84, OFILE_CHEST3,   "Chest 3", 1 }, // Q_BCHAMB
    { 0 }, //OBJ_CHEST3,
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 }, //OBJ_PEDISTAL,
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 105, OFILE_ALTBOY,   "Altarboy", 1 }, // Q_BETRAYER
    { 0 },
    { 0 },
    { 108, OFILE_ARMSTAND, "Armor stand", 2 }, //OBJ_ARMORSTAND, // Q_WARLORD - changed to inactive versions to eliminate farming potential
    { 109, OFILE_WEAPSTND, "Weapon stand", 2 }, //OBJ_WEAPONRACKL, // Q_WARLORD
    { 110, OFILE_WTORCH2,  "Torch 2", 0 }, // Q_BLOOD
    { 111, OFILE_WTORCH1,  "Torch 1", 0 }, // Q_BLOOD
    { 0 }, //OBJ_MUSHPATCH,
    { 0 }, //OBJ_STAND,
    { 0 }, //OBJ_TORCHL2,
    { 0 }, //OBJ_TORCHR2,
    { 0 }, //OBJ_FLAMELVR,
    { 0 }, //OBJ_SARC,
    { 0 }, //OBJ_BARREL,
    { 0 }, //OBJ_BARRELEX,
    { 0 }, //OBJ_BOOKSHELF,
    { 0 }, //OBJ_BOOKCASEL,
    { 0 }, //OBJ_BOOKCASER,
    { 0 }, //OBJ_ARMORSTANDN,
    { 0 }, //OBJ_WEAPONRACKLN,
    { 0 }, //OBJ_BLOODFTN,
    { 0 }, //OBJ_PURIFYINGFTN,
    { 0 }, //OBJ_SHRINEL,
    // clang-format on
};

const ObjFileData objfiledata[NUM_OFILE_TYPES] = {
    // clang-format off
/*OFILE_LEVER*/    { "Lever",     96, 1 }, // 2
/*OFILE_CRUXSK1*/  { "CruxSk1",   96, 1 }, // 15
/*OFILE_CRUXSK2*/  { "CruxSk2",   96, 1 }, // 15
/*OFILE_CRUXSK3*/  { "CruxSk3",   96, 1 }, // 15
/*OFILE_BOOK2*/    { "Book2",     96, 1 }, // 6
/*OFILE_BURNCROS*/ { "Burncros", 160, 1 }, // 10
/*OFILE_CANDLE2*/  { "Candle2",   96, 1 }, // 4
/*OFILE_MCIRL*/    { "Mcirl",     96, 3 }, // 4
/*OFILE_SWITCH4*/  { "Switch4",   96, 1 }, // 2
/*OFILE_TSOUL*/    { "TSoul",    128, 5 }, // 6
/*OFILE_TNUDEM*/   { "TNudeM",   128, 4 }, // 4
/*OFILE_TNUDEW*/   { "TNudeW",   128, 3 }, // 3
/*OFILE_CHEST1*/   { "Chest1",    96, 1 }, // 6
/*OFILE_CHEST2*/   { "Chest2",    96, 1 }, // 6
/*OFILE_CHEST3*/   { "Chest3",    96, 1 }, // 6
/*OFILE_ALTBOY*/   { "Altboy",   128, 1 }, // 1
/*OFILE_ARMSTAND*/ { "Armstand",  96, 2 }, // 2
/*OFILE_WEAPSTND*/ { "WeapStnd",  96, 2 }, // 4
/*OFILE_WTORCH2*/  { "WTorch2",   96, 1 }, // 9
/*OFILE_WTORCH1*/  { "WTorch1",   96, 1 }, // 9
    // clang-format on
};

const MonsterStruct MonstConvTbl[128] = {
    // clang-format off
    { 0 },
    { 0 }, //MT_NZOMBIE,
    { 0 }, //MT_BZOMBIE,
    { 0 }, //MT_GZOMBIE,
    { 0 }, //MT_YZOMBIE,
    { 0 }, //MT_RFALLSP,
    {   6, MOFILE_FALLSP, "FalSpear\\Dark",  "Carver" }, // Q_PWATER
    { 0 }, //MT_YFALLSP,
    { 0 }, // {   8, MOFILE_FALLSP, "FalSpear\\Blue",  "Dark One" }, // Monster from banner2.dun,
    { 0 }, //MT_WSKELAX,
    { 0 }, //MT_TSKELAX,
    {  11, MOFILE_SKELAX, nullptr,           "Burning Dead" }, // Q_SKELKING
    { 0 }, //MT_XSKELAX,
    { 0 }, //MT_RFALLSD,
    { 0 }, //MT_DFALLSD,
    {  15, MOFILE_FALLSD, nullptr,           "Devil Kin" }, // Q_PWATER
    {  16, MOFILE_FALLSD, "FalSpear\\Blue",  "Dark One" },  // Q_BANNER
    { 0 }, //MT_NSCAV,
    { 0 }, //MT_BSCAV,
    { 0 }, //MT_WSCAV,
    { 0 }, //MT_YSCAV,
    { 0 }, //MT_WSKELBW,
    {  22, MOFILE_SKELBW, "SkelSd\\Skelt",   "Corpse Bow" },   // Q_SKELKING
    {  23, MOFILE_SKELBW, nullptr,           "Burning Dead" }, // Q_SKELKING
    {  24, MOFILE_SKELBW, "SkelSd\\Black",   "Horror" },       // Q_SKELKING
    { 0 }, //MT_WSKELSD,
    { 0 }, //MT_TSKELSD,
    {  27, MOFILE_SKELSD, nullptr,           "Burning Dead Captain" }, // Q_SKELKING
    {  28, MOFILE_SKELSD, "SkelSd\\Black",   "Horror Captain" },       // Q_BCHAMB
    { 0 }, //MT_NSNEAK,
    { 0 }, //MT_RSNEAK,
    {  31, MOFILE_SNEAK,  "Sneak\\Sneakv3",  "Unseen" },            // Q_BCHAMB
    {  32, MOFILE_SNEAK,  "Sneak\\Sneakv1",  "Illusion Weaver" },   // Q_BLIND
    {  33, MOFILE_GOATMC, nullptr,           "Flesh Clan (Mace)" }, // Q_PWATER
    { 0 }, //MT_BGOATMC,
    { 0 }, //MT_RGOATMC,
    { 0 }, //MT_GGOATMC,
    { 0 }, //MT_RBAT,
    { 0 }, //MT_GBAT,
    { 0 }, //MT_NBAT,
    { 0 }, //MT_XBAT,
    {  41, MOFILE_GOATBW, nullptr,           "Flesh Clan (Bow)" }, // Q_PWATER
    { 0 }, //MT_BGOATBW,
    { 0 }, //MT_RGOATBW,
    {  44, MOFILE_GOATBW, "GoatMace\\Gray",  "Night Clan" }, // Q_ANVIL
    { 0 }, //MT_NACID,
    { 0 }, //MT_RACID,
    { 0 }, //MT_BACID,
    { 0 }, //MT_XACID,
    { 0 }, //MT_SKING,
    {  50, MOFILE_FAT,    nullptr,           "Overlord" }, // Q_BANNER
    { 0 }, //MT_BFAT,
    { 0 }, //MT_XFAT,
    { 0 }, //MT_RFAT,
    { 0 }, //MT_WYRM,
    { 0 }, //MT_CAVSLUG,
    { 0 }, //MT_DEVOUR,
    { 0 }, //MT_DVLWYRM,
    { 0 }, //MT_NMAGMA,
    { 0 }, //MT_YMAGMA,
    { 0 }, //MT_BMAGMA,
    { 0 }, //MT_WMAGMA,
    {  62, MOFILE_RHINO,  nullptr,           "Horned Demon" }, // Q_BLOOD, Q_BCHAMB
    { 0 }, // MT_XRHINO, // Q_MAZE
    { 0 }, //MT_BRHINO,
    {  65, MOFILE_RHINO,  "Rhino\\RhinoB",   "Obsidian Lord" }, // Q_ANVIL
    { 0 }, ///MT_BONEDMN,
    { 0 }, ///MT_REDDTH,
    { 0 }, ///MT_LTCHDMN,
    { 0 }, ///MT_UDEDBLRG,
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 }, ///MT_INCIN,
    { 0 }, ///MT_FLAMLRD,
    { 0 }, ///MT_DOOMFIRE,
    { 0 }, ///MT_HELLBURN,
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 }, //MT_NTHIN,
    { 0 }, //MT_RTHIN,
    { 0 }, //MT_XTHIN,
    { 0 }, //MT_GTHIN,
    { 0 }, //MT_NGARG,
    { 0 }, //MT_XGARG,
    { 0 }, //MT_DGARG,
    { 0 }, //MT_BGARG,
    { 0 }, //MT_NMEGA,
    { 0 }, //MT_DMEGA,
    { 0 }, //MT_BMEGA,
    { 0 }, //MT_RMEGA,
    { 0 }, //MT_NSNAKE,
    { 0 }, //MT_RSNAKE,
    { 0 }, //MT_GSNAKE,
    { 0 }, //MT_BSNAKE,
    {  98, MOFILE_BLACK,  nullptr,           "Black Knight" }, // Q_DIABLO
    { 0 }, //MT_RBLACK,
    { 100, MOFILE_BLACK,  "Black\\BlkKntBT", "Steel Lord" },   // Q_WARLORD
    { 101, MOFILE_BLACK,  "Black\\BlkKntBe", "Blood Knight" }, // Q_DIABLO
    { 0 }, ///MT_UNRAV,
    { 0 }, ///MT_HOLOWONE,
    { 0 }, ///MT_PAINMSTR,
    { 0 }, ///MT_REALWEAV,
    { 0 }, //MT_NSUCC,
    { 0 }, //MT_GSUCC,
    { 108, MOFILE_SUCC,   "Succ\\Succrw",    "Hell Spawn" }, // Q_BETRAYER
    { 0 }, //MT_BSUCC,
    { 0 }, //MT_NMAGE,
    { 0 }, //MT_GMAGE,
    { 0 }, //MT_XMAGE,
    { 113, MOFILE_MAGE,   "Mage\\Cnselbk",   "Advocate" }, // Q_BETRAYER, Q_DIABLO
    { 0 },
    { 115, MOFILE_DIABLO, nullptr,           "The Dark Lord" }, // Q_DIABLO
    { 0 },
    { 0 }, //MT_GOLEM,
    { 0 },
    { 0 },
    { 0 }, // Monster from blood1.dun and blood2.dun
    { 0 },
    { 0 },
    { 0 },
    { 0 }, // { 124, 128, "FalSpear\\Phall", "FalSpear\\Blue",    "Dark One" }, // Snotspill from banner2.dun
    { 0 },
    { 0 },
    { 0 }, ///MT_BIGFALL,
    // clang-format on
};

const MonFileData monfiledata[NUM_MOFILE_TYPES] = {
    // clang-format off
/*MOFILE_FALLSP*/  { "FalSpear\\Phall", 128 },
/*MOFILE_SKELAX*/  { "SkelAxe\\SklAx",  128 },
/*MOFILE_FALLSD*/  { "FalSword\\Fall",  128 },
/*MOFILE_SKELBW*/  { "SkelBow\\SklBw",  128 },
/*MOFILE_SKELSD*/  { "SkelSd\\SklSr",   128 },
/*MOFILE_SNEAK*/   { "Sneak\\Sneak",    128 },
/*MOFILE_GOATMC*/  { "GoatMace\\Goat",  128 },
/*MOFILE_GOATBW*/  { "GoatBow\\GoatB",  128 },
/*MOFILE_FAT*/     { "Fat\\Fat",        128 },
/*MOFILE_RHINO*/   { "Rhino\\Rhino",    160 },
/*MOFILE_BLACK*/   { "Black\\Black",    160 },
/*MOFILE_SUCC*/    { "Succ\\Scbs",      128 },
/*MOFILE_MAGE*/    { "Mage\\Mage",      128 },
/*MOFILE_DIABLO*/  { "Diablo\\Diablo",  160 },
    // clang-format on
};

D1Dun::~D1Dun()
{
    // TODO: MemFree?
    delete this->specGfx;

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

bool D1Dun::load(const QString &filePath, D1Til *t, const OpenAsParam &params)
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

    this->til = t;
    this->min = t->getMin();

    const QByteArray fileData = file.readAll();

    unsigned fileSize = fileData.size();
    int dunWidth = 0;
    int dunHeight = 0;
    bool changed = fileSize == 0; // !file.isOpen();
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
            } else {
                dProgressWarn() << tr("Items are not defined in the DUN file.");
                changed = true;
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
            } else {
                dProgressWarn() << tr("Monsters are not defined in the DUN file.");
                changed = true;
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
            } else {
                dProgressWarn() << tr("Objects are not defined in the DUN file.");
                changed = true;
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
            } else {
                dProgressWarn() << tr("Rooms are not defined in the DUN file.");
                changed = true;
            }

            if (dataSize > 0) {
                dProgressWarn() << tr("Unrecognizable data is ignored at the end of the DUN file.");
                changed = true;
            }

            // prepare subtiles
            for (int y = 0; y < dunHeight; y++) {
                for (int x = 0; x < dunWidth; x++) {
                    this->updateSubtiles(x, y, this->tiles[y][x]);
                }
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

            // prepare tiles
            for (int y = 0; y < dunHeight; y++) {
                for (int x = 0; x < dunWidth; x++) {
                    this->tiles[y][x] = UNDEF_TILE;
                }
            }
        }
    }

    this->type = type;
    this->width = dunWidth * TILE_WIDTH;
    this->height = dunHeight * TILE_HEIGHT;
    this->dunFilePath = filePath;
    this->modified = changed;
    this->defaultTile = UNDEF_TILE;
    this->specGfx = nullptr;
    return true;
}

void D1Dun::initialize(D1Pal *p, D1Tmi *m)
{
    this->pal = p;
    this->tmi = m;
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
    this->assetPath = assetDir;
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
    // find special cells
    QString specFilePath;
    if (!this->assetPath.isEmpty() && dungeonTbl[dungeonType].specPath != nullptr) {
        specFilePath = this->assetPath + "/" + dungeonTbl[dungeonType].specPath + "s.cel";
    } else {
        specFilePath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "s.cel";
    }
    if (QFileInfo::exists(specFilePath)) {
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
    }

    if (filePath.isEmpty()) {
        return false;
    }

    D1DUN_TYPE type = filePath.toLower().endsWith(".dun") ? D1DUN_TYPE::NORMAL : D1DUN_TYPE::RAW;
    // validate data
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
        // report unsaved information
        for (int y = 0; y < dunHeight / TILE_HEIGHT; y++) {
            for (int x = 0; x < dunWidth / TILE_WIDTH; x++) {
                if (this->tiles[y][x] != UNDEF_TILE && this->tiles[y][x] != 0) {
                    dProgressWarn() << tr("Defined tile at %1:%2 is not saved in this format (RDUN).").arg(x).arg(y);
                }
            }
        }
        for (int y = 0; y < dunHeight; y++) {
            for (int x = 0; x < dunWidth; x++) {
                if (this->items[y][x] != 0) {
                    dProgressWarn() << tr("Defined item at %1:%2 is not saved in this format (RDUN).").arg(x).arg(y);
                }
                if (this->monsters[y][x] != 0) {
                    dProgressWarn() << tr("Defined monster at %1:%2 is not saved in this format (RDUN).").arg(x).arg(y);
                }
                if (this->objects[y][x] != 0) {
                    dProgressWarn() << tr("Defined object at %1:%2 is not saved in this format (RDUN).").arg(x).arg(y);
                }
                if (this->rooms[y][x] != 0) {
                    dProgressWarn() << tr("Defined room at %1:%2 is not saved in this format (RDUN).").arg(x).arg(y);
                }
            }
        }
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
        for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
            for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                writeWord = this->items[y][x];
                out << writeWord;
            }
        }

        // write monsters
        for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
            for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                writeWord = this->monsters[y][x];
                out << writeWord;
            }
        }

        // write objects
        for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
            for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                writeWord = this->objects[y][x];
                out << writeWord;
            }
        }

        // write rooms
        for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
            for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                writeWord = this->rooms[y][x];
                out << writeWord;
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
    for (; y <= height / 2; y++) {
        len += 2;
        for (unsigned x = width / 2 - len - CELL_BORDER; x < width / 2 + len + CELL_BORDER; x++) {
            image.setPixelColor(sx + x, sy + y, color);
        }
    }
    for (; y < height; y++) {
        len -= 2;
        for (unsigned x = width / 2 - len - CELL_BORDER; x < width / 2 + len + CELL_BORDER; x++) {
            image.setPixelColor(sx + x, sy + y, color);
        }
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
            /*for (int y = backHeight - CELL_BORDER - 1; y >= 0; y--) {
                for (unsigned x = CELL_BORDER; x < backWidth - CELL_BORDER; x++) {
                    if (backImage.pixelColor(x, y).alpha() == 0) {
                        continue;
                    }
                    destImage->setPixelColor(drawCursorX + x - CELL_BORDER, drawCursorY - (y + 1), color);
                }
            }*/
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
                    for (int y = backHeight - CELL_BORDER - 1; y >= 0; y--) {
                        for (unsigned x = CELL_BORDER; x < backWidth - CELL_BORDER; x++) {
                            if (backImage.pixelColor(x, y).alpha() == 0) {
                                continue;
                            }
                            QColor color = subtileImage.pixelColor(x - CELL_BORDER, subtileImage.height() - y);
                            if (/*color.isNull() ||*/ color.alpha() == 0) {
                                continue;
                            }
                            destImage->setPixelColor(drawCursorX + x - CELL_BORDER, drawCursorY - (y + 1), color);
                        }
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
            const ObjectStruct *objStr = &ObjConvTbl[objectIndex];
            const ObjectCacheEntry *objEntry = nullptr;
            for (const auto &obj : this->objectCache) {
                if (obj.objStr == objStr) {
                    objEntry = &obj;
                    break;
                }
            }
            if (objEntry == nullptr) {
                this->loadObject(objectIndex);
                objEntry = &this->objectCache.back();
            }
            if (objEntry->objGfx != nullptr) {
                int frameNum = objEntry->objStr->frameNum;
                if (frameNum == 0) {
                    frameNum = 1 + (params.time % objEntry->objGfx->getFrameCount());
                }
                QImage objectImage = objEntry->objGfx->getFrameImage(frameNum - 1);
                dungeon.drawImage(drawCursorX + ((int)backWidth - objectImage.width()) / 2, drawCursorY - objectImage.height(), objectImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            } else {
                QString text = tr("Object%1").arg(objectIndex);
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
            QString text = tr("Item%1").arg(itemIndex);
            // dungeon.setFont(font);
            // dungeon.setPen(font);
            QFontMetrics fm(dungeon.font());
            unsigned textWidth = fm.horizontalAdvance(text);
            dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY + (bottomText ? 3 : 1) * fm.height() / 2, text);
        }
    }
    if (params.showMonsters) {
        // draw the monster
        int monsterIndex = this->monsters[dunCursorY][dunCursorX];
        if (monsterIndex != 0) {
            const MonsterStruct *monStr = &MonstConvTbl[monsterIndex];
            const MonsterCacheEntry *monEntry = nullptr;
            for (const auto &mon : this->monsterCache) {
                if (mon.monStr == monStr) {
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
                QString text = tr("Monster%1").arg(monsterIndex);
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
        (maxDunSize - 1) * cellHeight + subtileHeight, QImage::Format_ARGB32_Premultiplied); // QImage::Format_ARGB32
    dungeon.fill(Qt::transparent);

    // create template of the background image
    QImage backImage = QImage(cellWidth + 2 * CELL_BORDER, cellHeight + 2 * CELL_BORDER, QImage::Format_ARGB32_Premultiplied); // QImage::Format_ARGB32
    backImage.fill(Qt::transparent);
    QColor backColor = QColor(Config::getGraphicsTransparentColor());
    if (params.tileState != Qt::Unchecked) {
        D1Dun::drawDiamond(backImage, 0 + CELL_BORDER, 0 + CELL_BORDER, cellWidth, cellHeight, backColor);
    } else {
        unsigned len = 0;
        unsigned y = 1;
        for (; y <= cellHeight / 2; y++) {
            len += 2;
            for (unsigned x = cellWidth / 2 - len - CELL_BORDER - 1; x <= cellWidth / 2 - len; x++) {
                backImage.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, backColor);
            }
            for (unsigned x = cellWidth / 2 + len - 1; x <= cellWidth / 2 + len + CELL_BORDER; x++) {
                backImage.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, backColor);
            }
        }
        for (; y < cellHeight; y++) {
            len -= 2;
            for (unsigned x = cellWidth / 2 - len - CELL_BORDER - 1; x <= cellWidth / 2 - len; x++) {
                backImage.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, backColor);
            }
            for (unsigned x = cellWidth / 2 + len - 1; x <= cellWidth / 2 + len + CELL_BORDER; x++) {
                backImage.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, backColor);
            }
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
        D1Trn *monTrn = entry.monTrn;
        if (monTrn == nullptr) {
            entry.monPal = pal;
        } else {
            monTrn->setPalette(pal);
            monTrn->refreshResultingPalette();
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

int D1Dun::getWidth() const
{
    return this->width;
}

bool D1Dun::setWidth(int newWidth)
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
    if (diff < 0) {
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

bool D1Dun::setHeight(int newHeight)
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
    if (diff < 0) {
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

void D1Dun::loadObject(int objectIndex)
{
    ObjectCacheEntry result = { &ObjConvTbl[objectIndex], nullptr };
    if (objectIndex < lengthof(ObjConvTbl) && result.objStr->type != 0) {
        int objDataIndex = result.objStr->animType;
        result.objGfx = this->objDataCache[objDataIndex];
        if (result.objGfx == nullptr && !this->assetPath.isEmpty()) {
            result.objGfx = new D1Gfx();
            result.objGfx->setPalette(this->pal);
            QString celFilePath = this->assetPath + "/Objects/" + objfiledata[objDataIndex].path + ".CEL";
            OpenAsParam params = OpenAsParam();
            params.celWidth = objfiledata[objDataIndex].width;
            D1Cel::load(*result.objGfx, celFilePath, params);
            if (result.objGfx->getFrameCount() < objfiledata[objDataIndex].numFrames) {
                // TODO: suppress errors? MemFree?
                delete result.objGfx;
                result.objGfx = nullptr;
            } else {
                this->objDataCache[objDataIndex] = result.objGfx;
            }
        }
    }
    this->objectCache.push_back(result);
}

void D1Dun::loadMonster(int monsterIndex)
{
    MonsterCacheEntry result = { &MonstConvTbl[monsterIndex], nullptr, this->pal, nullptr };
    if (monsterIndex < lengthof(MonstConvTbl) && result.monStr->type != 0) {
        int monDataIndex = result.monStr->animType;
        result.monGfx = this->monDataCache[monDataIndex];
        if (result.monGfx == nullptr && !this->assetPath.isEmpty()) {
            result.monGfx = new D1Gfx();
            // result.monGfx->setPalette(result.monPal);
            QString cl2FilePath = this->assetPath + "/Monsters/" + monfiledata[monDataIndex].path + "N.CL2";
            OpenAsParam params = OpenAsParam();
            params.celWidth = monfiledata[monDataIndex].width;
            D1Cl2::load(*result.monGfx, cl2FilePath, params);
            if (result.monGfx->getFrameCount() == 0) {
                // TODO: suppress errors? MemFree?
                delete result.monGfx;
                result.monGfx = nullptr;
            } else {
                this->monDataCache[monDataIndex] = result.monGfx;
            }
        }
        if (result.monGfx != nullptr && result.monStr->trnPath != nullptr && !this->assetPath.isEmpty()) {
            result.monTrn = new D1Trn();
            QString trnFilePath = this->assetPath + "/Monsters/" + result.monStr->trnPath + ".TRN";
            if (!result.monTrn->load(trnFilePath, result.monPal)) {
                // TODO: suppress errors? MemFree?
                delete result.monTrn;
                result.monTrn = nullptr;
            } else {
                // apply Monster TRN
                for (int i = 0; i < D1PAL_COLORS; i++) {
                    if (result.monTrn->getTranslation(i) == 0xFF) {
                        result.monTrn->setTranslation(i, 0);
                    }
                }
                // set palette
                result.monPal = result.monTrn->getResultingPalette();
            }
        }
    }
    this->monsterCache.push_back(result);
}

void D1Dun::clearAssets()
{
    this->objectCache.clear();
    for (auto &entry : this->monsterCache) {
        delete entry.monTrn;
    }
    this->monsterCache.clear();
    for (int i = 0; i < lengthof(objDataCache); i++) {
        // TODO: MemFree?
        delete objDataCache[i];
        objDataCache[i] = nullptr;
    }
    for (int i = 0; i < lengthof(monDataCache); i++) {
        // TODO: MemFree?
        delete monDataCache[i];
        monDataCache[i] = nullptr;
    }
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
            if (subtileRef == UNDEF_SUBTILE) {
                dProgressWarn() << tr("Item%1 at %2:%3 is on an undefined subtile.").arg(itemIndex).arg(x).arg(y);
                result = true;
            } else if (subtileRef == 0) {
                dProgressWarn() << tr("Item%1 at %2:%3 is on an empty subtile.").arg(itemIndex).arg(x).arg(y);
                result = true;
            } else {
                quint8 solFlags = sol->getSubtileProperties(subtileRef - 1);
                if (solFlags & ((1 << 0) | (1 << 2))) {
                    dProgressErr() << tr("Item%1 at %2:%3 is on a subtile which is not accessible (solid or missile blocker).").arg(itemIndex).arg(x).arg(y);
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
            if (subtileRef == UNDEF_SUBTILE) {
                dProgressWarn() << tr("'%1' monster at %2:%3 is on an undefined subtile.").arg(MonstConvTbl[monsterIndex].name).arg(x).arg(y);
                result = true;
            } else if (subtileRef == 0) {
                dProgressWarn() << tr("'%1' monster at %2:%3 is on an empty subtile.").arg(MonstConvTbl[monsterIndex].name).arg(x).arg(y);
                result = true;
            } else {
                quint8 solFlags = sol->getSubtileProperties(subtileRef - 1);
                if (solFlags & ((1 << 0) | (1 << 2))) {
                    dProgressErr() << tr("'%1' monster at %2:%3 is on a subtile which is not accessible (solid or missile blocker).").arg(MonstConvTbl[monsterIndex].name).arg(x).arg(y);
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
            if (subtileRef == UNDEF_SUBTILE) {
                dProgressWarn() << tr("'%1' object at %2:%3 is on an undefined subtile.").arg(ObjConvTbl[objectIndex].name).arg(x).arg(y);
                result = true;
            } else if (subtileRef == 0) {
                dProgressWarn() << tr("'%1' object at %2:%3 is on an empty subtile.").arg(ObjConvTbl[objectIndex].name).arg(x).arg(y);
                result = true;
            }
            if (this->monsters[y][x] != 0) {
                dProgressErr() << tr("'%1' object at %2:%3 is sharing a subtile with a monster.").arg(ObjConvTbl[objectIndex].name).arg(x).arg(y);
                result = true;
            }
            if (this->items[y][x] != 0) {
                dProgressErr() << tr("'%1' object at %2:%3 is sharing a subtile with an item.").arg(ObjConvTbl[objectIndex].name).arg(x).arg(y);
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
                    dProgressWarn() << tr("Item%1 at %2:%3 was replaced by %4.").arg(currItemIndex).arg(x).arg(y).arg(newItemIndex);
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
                    dProgressWarn() << tr("'%1' monster at %2:%3 was replaced by '%4'.").arg(MonstConvTbl[currMonsterIndex].name).arg(x).arg(y).arg(MonstConvTbl[newMonsterIndex].name);
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
                    dProgressWarn() << tr("'%1' object at %2:%3 was replaced by '%4'.").arg(ObjConvTbl[currObjectIndex].name).arg(x).arg(y).arg(ObjConvTbl[newObjectIndex].name);
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
    dProgress() << tr("Changed Object at %1:%2 from '%3' to '%4'.").arg(posx).arg(posy).arg(prevObject).arg(objectIndex);
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
    dProgress() << tr("Changed Monster at %1:%2 from '%3' to '%4'.").arg(posx).arg(posy).arg(prevMonster).arg(monsterIndex);
    this->modified = true;
    return true;
}

void D1Dun::patch(int dunFileIndex)
{
    const quint8 dunSizes[][2] {
        // clang-format off
/* DUN_SKELKING_ENTRY*/      { 14, 14 }, // SKngDO.DUN
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
        break;
    case DUN_BLIND_AFT: // Blind1.DUN
        // place pieces with closed doors
        change |= this->changeTileAt(4, 3, 150);
        change |= this->changeTileAt(6, 7, 150);
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
        break;
    case DUN_BLOOD_AFT: // Blood1.DUN
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
    }
    if (!change) {
        dProgress() << tr("No change was necessary.");
    }
}
