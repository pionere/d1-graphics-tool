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
#include "d1til.h"
#include "d1tmi.h"
#include "progressdialog.h"

template <class T, int N>
constexpr int lengthof(T (&arr)[N])
{
    return N;
}

typedef struct DungeonStruct {
    int defaultTile;
    const char *specPath;
    std::pair<int, int> *specialCels;
    int numSpecCels;
} DungeonStruct;

static const std::pair<int, int> townSpecialCels[] = {
    // clang-format off
    { 360,  1 }, { 358,  2 }, { 129,  6 }, { 130,  7 }, { 128,  8 }, { 117,  9 }, { 157, 10 }, { 158, 11 }, { 156, 12 }, { 162, 13 },
    { 160, 14 }, { 214, 15 }, { 212, 16 }, { 217, 17 }, { 216, 18 },
    // clang-format on
};

static const std::pair<int, int> l1SpecialCels[] = {
    // clang-format off
    { 12, 1 }, { 71, 1 }, { 211, 1 }, { 321, 1 }, { 341, 1 }, { 418, 1 },
    { 11, 2 }, { 249, 2 }, { 325, 2 }, { 331, 2 }, { 344, 2 }, { 421, 2 },
    { 253, 3 }, { 255, 4 }, { 259, 5 }, { 267, 6 } /*, { 417, '7' } */
    // clang-format on
};

static const std::pair<int, int> l2SpecialCels[] = {
    // clang-format off
    { 13, 5 }, { 178, 5 }, { 551, 5 }, { 17, 6 }, { 553, 6 } /*, { 132, '2'/'1' }, { 135, '3'/'4' }, { 139, '3'/'4' }*/
    // clang-format on
};

static const std::pair<int, int> l5SpecialCels[] = {
    // clang-format off
    { 77, 1 }, { 80, 2 }
    // clang-format on
};

const DungeonStruct dungeonTbl[NUM_DUNGEON_TYPES] = {
    // clang-format off
/* DTYPE_TOWN      */ { UNDEF_TILE, "Levels/TownData/Towns.cel", &townSpecialCels, lengthof(townSpecialCels) },
/* DTYPE_CATHEDRAL */ { 13,         "Levels/L1Data/L1s.cel",     &l1SpecialCels,   lengthof(l1SpecialCels) },
/* DTYPE_CATACOMBS */ { 3,          "Levels/L2Data/L2s.cel",     &l2SpecialCels,   lengthof(l2SpecialCels) },
/* DTYPE_CAVES     */ { 7,          nullptr,                     nullptr,          0 },
/* DTYPE_HELL      */ { 6,          nullptr,                     nullptr,          0 },
/* DTYPE_CRYPT     */ { 13,         "NLevels/L5Data/L5s.cel",    &l5SpecialCels,   lengthof(l5SpecialCels) },
/* DTYPE_NEST      */ { 7,          nullptr,                     nullptr,          0 },
/* DTYPE_NONE      */ { UNDEF_TILE, nullptr,                     nullptr,          0 },
    // clang-format on
};

const ObjectStruct ObjConvTbl[128] = {
	// clang-format off
    { 0 },
    {   1,  96, "Lever",    "Lever", 1 }, // Q_SKELKING
    {   2,  96, "CruxSk1",  "Crucifix1", 1 }, // Q_SKELKING
    {   3,  96, "CruxSk2",  "Crucifix2", 1 }, // Q_SKELKING
    {   4,  96, "CruxSk3",  "Crucifix2", 1 }, // Q_SKELKING
    { 0 }, //OBJ_ANGEL,
    { 0 }, //OBJ_BANNERL,
    { 0 }, //OBJ_BANNERM,
    { 0 }, //OBJ_BANNERR,
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
    {  14,  96, "Book2",    "Bookshelf", 1 }, // Q_BCHAMB, Q_BETRAYER
	{ 0 }, //OBJ_BOOK2R,
    {  16, 160, "Burncros", "Burning cross", 0 }, // Q_BCHAMB
	{ 0 },
	{ 0 }, //OBJ_CANDLE1,
    {  19,  96, "Candle2",  "Candle", 0 }, // Q_BCHAMB
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
    {  36,  96, "Mcirl",    "Magic Circle 1", 1 }, // Q_BETRAYER
    {  37,  96, "Mcirl",    "Magic Circle 2", 3 }, // Q_BETRAYER
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
    {  51,  96, "Switch4",  "Switch", 1 }, // Q_BCHAMB, Q_DIABLO
	{ 0 },
	{ 0 }, //OBJ_TRAPL,
	{ 0 }, //OBJ_TRAPR,
    {  55, 128, "TSoul",    "Tortured body 1", 1 }, // Q_BUTCHER
    {  56, 128, "TSoul",    "Tortured body 2", 2 }, // Q_BUTCHER
	{  57, 128, "TSoul",    "Tortured body 3", 3 }, // Q_BUTCHER
	{  58, 128, "TSoul",    "Tortured body 4", 4 }, // Q_BUTCHER
	{  59, 128, "TSoul",    "Tortured body 5", 5 }, // Q_BUTCHER
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
    {  70, 128, "TNudeM",   "Tortured male 1", 1 }, //1, Q_BUTCHER
    {  71, 128, "TNudeM",   "Tortured male 2", 2 }, //2, Q_BUTCHER
    {  72, 128, "TNudeM",   "Tortured male 3", 3 }, //3, Q_BUTCHER
    {  73, 128, "TNudeM",   "Tortured male 4", 4 }, //4, Q_BUTCHER
    {  74, 128, "TNudeW",   "Tortured female 1", 1 }, //1, Q_BUTCHER
	{  75, 128, "TNudeW",   "Tortured female 1", 2 }, //2, Q_BUTCHER
	{  76, 128, "TNudeW",   "Tortured female 1", 3 }, //3, Q_BUTCHER
	{ 0 }, //OBJ_CHEST1,
    {  78,  96, "Chest1",   "Chest 1", 1 }, // Q_SKELKING
	{ 0 }, //OBJ_CHEST1,
	{ 0 }, //OBJ_CHEST2,
    {  81,  96, "Chest2",   "Chest 2", 1 }, // Q_SKELKING
	{ 0 }, //OBJ_CHEST2,
	{ 0 }, //OBJ_CHEST3,
    {  84,  96, "Chest3",   "Chest 3", 1 }, // Q_BCHAMB
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
    { 105, 128, "Altboy",   "Altarboy", 1 }, // Q_BETRAYER
	{ 0 },
	{ 0 },
    { 108,  96, "Armstand", "Armor stand", 2 }, //OBJ_ARMORSTAND, // Q_WARLORD - changed to inactive versions to eliminate farming potential
    { 109,  96, "WeapStnd", "Weapon stand", 2 }, //OBJ_WEAPONRACKL, // Q_WARLORD
    { 110,  96, "WTorch2",  "Torch 1", 0 }, // Q_BLOOD
    { 111,  96, "WTorch1",  "Torch 2", 0 }, // Q_BLOOD
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

const MonsterStruct MonstConvTbl[128] = {
	// clang-format off
	{ 0 },
	{ 0 }, //MT_NZOMBIE,
	{ 0 }, //MT_BZOMBIE,
	{ 0 }, //MT_GZOMBIE,
	{ 0 }, //MT_YZOMBIE,
	{ 0 }, //MT_RFALLSP,
    {   6, 128, "FalSpear\\PhallN.CL2", "FalSpear\\Dark.TRN",  "Carver", // Q_PWATER
	{ 0 }, //MT_YFALLSP,
	{ 0 }, //MT_BFALLSP,
	{ 0 }, //MT_WSKELAX,
	{ 0 }, //MT_TSKELAX,
    {  11, 128, "SkelAxe\\SklAxN.CL2",  nullptr,               "Burning Dead", // Q_SKELKING
	{ 0 }, //MT_XSKELAX,
	{ 0 }, //MT_RFALLSD,
	{ 0 }, //MT_DFALLSD,
    {  15, 128, "FalSword\\FallN.CL2",  nullptr,               "Devil Kin", // Q_PWATER
    {  16, 128, "FalSword\\FallN.CL2",  "FalSpear\\Blue.TRN",  "Dark One", // Q_BANNER
	{ 0 }, //MT_NSCAV,
	{ 0 }, //MT_BSCAV,
	{ 0 }, //MT_WSCAV,
	{ 0 }, //MT_YSCAV,
	{ 0 }, //MT_WSKELBW,
    {  22, 128, "SkelBow\\SklBwN.CL2",  "SkelSd\\Skelt.TRN",   "Corpse Bow", // Q_SKELKING
	{  23, 128, "SkelBow\\SklBwN.CL2",  nullptr,                           "Burning Dead", // Q_SKELKING
	{  24, 128, "SkelBow\\SklBwN.CL2",  "SkelSd\\Black.TRN",   "Horror", // Q_SKELKING
	{ 0 }, //MT_WSKELSD,
	{ 0 }, //MT_TSKELSD,
	{  27, 128, "SkelSd\\SklSrN.CL2",   nullptr,               "Burning Dead Captain", // Q_SKELKING
	{  28, 128, "SkelSd\\SklSrN.CL2",   "SkelSd\\Black.TRN",   "Horror Captain", // Q_BCHAMB
	{ 0 }, //MT_NSNEAK,
	{ 0 }, //MT_RSNEAK,
	{  31, 128, "Sneak\\SneakN.CL2",    "Sneak\\Sneakv3.TRN",  "Unseen", // Q_BCHAMB
	{  32, 128, "Sneak\\SneakN.CL2",    "Sneak\\Sneakv1.TRN",  "Illusion Weaver", // Q_BLIND
	{  33, 128, "GoatMace\\GoatN.CL2",  nullptr,               "Flesh Clan", // Q_PWATER
	{ 0 }, //MT_BGOATMC,
	{ 0 }, //MT_RGOATMC,
	{ 0 }, //MT_GGOATMC,
	{ 0 }, //MT_RBAT,
	{ 0 }, //MT_GBAT,
	{ 0 }, //MT_NBAT,
	{ 0 }, //MT_XBAT,
	{  41, 128, "GoatBow\\GoatBN.CL2",  nullptr,               "Flesh Clan", // Q_PWATER
	{ 0 }, //MT_BGOATBW,
	{ 0 }, //MT_RGOATBW,
	{  44, 128, "GoatBow\\GoatBN.CL2",  "GoatMace\\Gray.TRN",  "Night Clan", // Q_ANVIL
	{ 0 }, //MT_NACID,
	{ 0 }, //MT_RACID,
	{ 0 }, //MT_BACID,
	{ 0 }, //MT_XACID,
	{ 0 }, //MT_SKING,
	{  50, 128, "Fat\\FatN.CL2",        nullptr,               "Overlord", // Q_BANNER
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
	{  62, 160, "Rhino\\RhinoN.CL2",    nullptr,               "Horned Demon", // Q_BLOOD, Q_BCHAMB
	{ 0 }, // MT_XRHINO, // Q_MAZE
	{ 0 }, //MT_BRHINO,
	{  65, 160, "Rhino\\RhinoN.CL2",    "Rhino\\RhinoB.TRN",   "Obsidian Lord", // Q_ANVIL
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
	{  98, 160, "Black\\BlackN.CL2",    nullptr,               "Black Knight", // Q_DIABLO
	{ 0 }, //MT_RBLACK,
	{ 100, 160, "Black\\BlackN.CL2",    "Black\\BlkKntBT.TRN", "Steel Lord", // Q_WARLORD
	{ 101, 160, "Black\\BlackN.CL2",    "Black\\BlkKntBe.TRN", "Blood Knight", // Q_DIABLO
	{ 0 }, ///MT_UNRAV,
	{ 0 }, ///MT_HOLOWONE,
	{ 0 }, ///MT_PAINMSTR,
	{ 0 }, ///MT_REALWEAV,
	{ 0 }, //MT_NSUCC,
	{ 0 }, //MT_GSUCC,
	{ 108, 128, "Succ\\ScbsN.CL2",      "Succ\\Succrw.TRN",    "Hell Spawn", // Q_BETRAYER
	{ 0 }, //MT_BSUCC,
	{ 0 }, //MT_NMAGE,
	{ 0 }, //MT_GMAGE,
	{ 0 }, //MT_XMAGE,
	{ 113, 128, "Mage\\MageN.CL2",      "Mage\\Cnselbk.TRN",   "Advocate", // Q_BETRAYER, Q_DIABLO
	{ 0 },
    { 115, 160, "Diablo\\DiabloN.CL2",  nullptr,               "The Dark Lord", // Q_DIABLO
	{ 0 },
	{ 0 }, //MT_GOLEM,
	{ 0 },
	{ 0 },
	{ 0 }, // Monster from blood1.dun and blood2.dun
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }, // Snotspill from banner2.dun
	{ 0 },
	{ 0 },
	{ 0 }, ///MT_BIGFALL,
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
    transvals.resize(dunHeight * TILE_HEIGHT);
    for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
        transvals[i].resize(dunWidth * TILE_WIDTH);
    }
}

bool D1Dun::load(D1Pal *p, const QString &filePath, D1Til *t, D1Tmi *m, const OpenAsParam &params)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.dunFilePath.isEmpty()) {
    //    filePath = params.dunFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly) && !params.ampFilePath.isEmpty()) {
            return false; // report read-error only if the file was explicitly requested
        }
    }

    this->pal = p;
    this->til = t;
    this->tmi = m;

    const QByteArray fileData = file.readAll();

    unsigned fileSize = fileData.size();
    int dunWidth = 0;
    int dunHeight = 0;
    bool changed = fileSize == 0; // !file.isOpen();
    if (fileSize != 0) {
        if (filePath.toLower().endsWith(".dun")) {
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
            if (fileSize < 2 * 2 + tileCount * 2) {
                dProgressErr() << tr("Invalid DUN header.");
                return false;
            }

            // prepare the vectors
            this->initVectors(dunWidth, dunHeight);

            // read tiles
            for (int y = 0; y < dunHeight; y++) {
                for (int x = 0; x < dunWidth; x++) {
                    in >> readWord;
                    this->tiles[y][x] = readWord;
                }
            }

            if (fileSize > 2 * 2 + tileCount * 2) {
                unsigned subtileCount = tileCount * TILE_WIDTH * TILE_HEIGHT;
                if (fileSize != 2 * 2 + tileCount * 2 + subtileCount * 2 * 4) {
                    dProgressWarn() << tr("Invalid DUN content.");
                    changed = true;
                }
                if (fileSize >= 2 * 2 + tileCount * 2 + subtileCount * 2 * 4) {
                    // read items
                    for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                        for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                            in >> readWord;
                            this->items[y][x] = readWord;
                        }
                    }

                    // read monsters
                    for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                        for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                            in >> readWord;
                            this->monsters[y][x] = readWord;
                        }
                    }

                    // read objects
                    for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                        for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                            in >> readWord;
                            this->objects[y][x] = readWord;
                        }
                    }

                    // read transval
                    for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                        for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                            in >> readWord;
                            this->transvals[y][x] = readWord;
                        }
                    }
                }
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
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    in >> readDword;
                    this->subtiles[y][x] = readDword + 1;
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

    this->width = dunWidth * TILE_WIDTH;
    this->height = dunHeight * TILE_HEIGHT;
    this->dunFilePath = filePath;
    this->modified = changed;
    // calculate meta info
    this->defaultTile = UNDEF_TILE;
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
    if (baseName.length() >= 2 && baseName[0] == "l") {
        switch (baseName[1].toInt()) {
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
    this->setLevelType(dungeonType);
    // find special cells
    QString specFilePath;
    if (!this->assetPath.isEmpty() && dungeonTbl[dungeonType].specPath != nullptr) {
        specFilePath = this->assetPath + "/" + dungeonTbl[dungeonType].specPath;
    } else {
        specFilePath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "s.cel";
    }
    D1Gfx *specGfx = nullptr;
    if (QFileInfo::exists(specFilePath)) {
        specGfx = new D1Gfx();
        specGfx->setPalette(this->pal);
        OpenAsParam params = OpenAsParam();
        params.celWidth = this->til->getMin()->getSubtileWidth() * MICRO_WIDTH;
        if (!D1Cel::load(*specGfx, specFilePath, params)) {
            dProgressErr() << tr("Failed to load special CEL file %1.").arg(QDir::toNativeSeparators(filePath));
            // TODO: MemFree?
            delete specGfx;
            specGfx = nullptr;
        } else {
            dProgress() << tr("Loaded special CEL file %1.").arg(QDir::toNativeSeparators(filePath));
        }
    }
    this->specGfx = specGfx;
    return true;
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

    bool baseDun = filePath.toLower().endsWith(".dun");
    // validate data
    if (baseDun) {
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
                if (this->subtiles[y][x] == UNDEF_SUBTILE || this->subtiles[y][x] == 0) {
                    dProgressFail() << tr("Undefined or zero subtiles (one at %1:%2) can not be saved in this format (RDUN).").arg(x).arg(y);
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
                if (this->transvals[y][x] != 0) {
                    dProgressWarn() << tr("Defined transval at %1:%2 is not saved in this format (RDUN).").arg(x).arg(y);
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

    if (baseDun) {
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

        // write transval
        for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
            for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                writeWord = this->transvals[y][x];
                out << writeWord;
            }
        }
    } else {
        // rdun
        int dunWidth = this->width;
        int dunHeight = this->height;

        quint32 writeDword;
        // write subtiles
        for (int y = 0; y < dunHeight; y++) {
            for (int x = 0; x < dunWidth; x++) {
                writeDword = this->subtiles[y][x] - 1;
                out << writeDword;
            }
        }
    }

    this->dunFilePath = filePath; // this->load(filePath, allocate);
    this->modified = false;

    return true;
}

#define CELL_BORDER 0

void D1Dun::drawImage(QPainter &dungeon, QImage &backImage, int drawCursorX, int drawCursorY, int dunCursorX, int dunCursorY, const DunDrawParam &params) const
{
    const unsigned backWidth = backImage.width() - 2 * CELL_BORDER;
    const unsigned backHeight = backImage.height() - 2 * CELL_BORDER;
    // draw the background
    dungeon.drawImage(drawCursorX - CELL_BORDER, drawCursorY - backHeight - CELL_BORDER, backImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
    unsigned cellCenterX = drawCursorX + backWidth / 2;
    unsigned cellCenterY = drawCursorY - backHeight / 2;
    bool subtileText = false;
    if (params.tileState != Qt::Unchecked) {
        // draw the subtile
        int subtileRef = this->subtiles[dunCursorY][dunCursorX];
        if (subtileRef != 0) {
            if (subtileRef == UNDEF_SUBTILE || subtileRef > this->til->getMin()->getSubtileCount()) {
                QString text = tr("Subtile%1");
                if (subtileRef == UNDEF_SUBTILE) {
                    text = text.arg("???");
                } else {
                    text = text.arg(subtileRef - 1);
                }
                QFontMetrics fm(dungeon.font());
                unsigned textWidth = fm.horizontalAdvance(text);
                dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY - fm.height() / 2, text);
                subtileText = true;
            } else {
                QImage subtileImage = this->til->getMin()->getSubtileImage(subtileRef - 1);
                QImage *destImage = (QImage *)dungeon.device();
                if (params.tileState == Qt::Checked) {
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
                    for (unsigned y = CELL_BORDER; y < backHeight - CELL_BORDER; y++) {
                        for (unsigned x = CELL_BORDER; x < backWidth - CELL_BORDER; x++) {
                            if (backImage.pixelColor(x, y).alpha() == 0) {
                                continue;
                            }
                            QColor color = subtileImage.pixelColor(x - CELL_BORDER, subtileImage.height() - backHeight + y);
                            if (/*color.isNull() ||*/ color.alpha() == 0) {
                                continue;
                            }
                            destImage->setPixelColor(drawCursorX + x - CELL_BORDER, drawCursorY - backHeight + y, color);
                        }
                    }
                }
            }
        }
        // draw tile text
        static_assert(TILE_WIDTH == 2 && TILE_HEIGHT == 2, "D1Dun::drawImage skips boundary checks.");
        int tileRef = this->tiles[dunCursorY / TILE_HEIGHT][dunCursorX / TILE_WIDTH];
        if (tileRef != 0 && (dunCursorX & 1) && (dunCursorY & 1)) {
            bool skipTile = true;
            for (int dy = TILE_HEIGHT - 1; dy >= 0; dy--) {
                for (int dx = TILE_WIDTH - 1; dx >= 0; dx--) {
                    int subtileRef = this->subtiles[dunCursorY - dy][dunCursorX - dx];
                    if (subtileRef == UNDEF_SUBTILE || subtileRef > this->til->getMin()->getSubtileCount()) {
                        skipTile = false;
                    }
                }
            }
            if (!skipTile) {
                QString text = tr("Tile%1");
                if (tileRef == UNDEF_TILE) {
                    text = text.arg("???");
                } else {
                    text = text.arg(tileRef - 1);
                }
                QFontMetrics fm(dungeon.font());
                unsigned textWidth = fm.horizontalAdvance(text);
                dungeon.drawText(cellCenterX - textWidth / 2, drawCursorY - backHeight - fm.height() / 2, text);
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
            dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY + fm.height() * (subtileText ? 2 : 1), text);
        }
    }
    if (params.showObjects) {
        // draw the object
        int objectIndex = this->objects[dunCursorY][dunCursorX];
        if (objectIndex != 0) {
            const ObjectStruct *objStr = &ObjConvTbl[objectIndex];
            D1Gfx *objGfx = nullptr;
            bool found = false;
            for (const std::pair<const ObjectStruct *, D1Gfx *> &obj : this->objectCache) {
                if (obj.first == objStr) {
                    objGfx = obj.second;
                    found = true;
                    break;
                }
            }
            if (!found) {
                objGfx = this->loadObject(objectIndex);
            }
            if (objGfx != nullptr) {
                int frameNum = objStr->frameNum;
                if (frameNum == 0) {
                    frameNum = 1;
                }
                QImage objectImage = objGfx->getFrameImage(frameNum - 1);
                dungeon.drawImage(drawCursorX + ((int)backWidth - objStr->width) / 2, drawCursorY - objectImage.height(), objectImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            } else {
                QString text = tr("Object%1").arg(objectIndex);
                QFontMetrics fm(dungeon.font());
                unsigned textWidth = fm.horizontalAdvance(text);
                dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY + fm.height() * (subtileText ? 1 : 0), text);
            }
        }
    }
    if (params.showMonsters) {
        // draw the monster
        int monsterIndex = this->monsters[dunCursorY][dunCursorX];
        if (monsterIndex != 0) {
            const MonsterStruct *monStr = &MonstConvTbl[monsterIndex];
            D1Gfx *monGfx = nullptr;
            bool found = false;
            for (const std::pair<const MonsterStruct *, D1Gfx *> &mon : this->monsterCache) {
                if (mon.first == monStr) {
                    monGfx = mon.second;
                    found = true;
                    break;
                }
            }
            if (!found) {
                monGfx = this->loadMonster(monsterIndex);
            }
            if (monGfx != nullptr) {
                int frameNum = 1;
                QImage monImage = monGfx->getFrameImage(frameNum - 1);
                dungeon.drawImage(drawCursorX + ((int)backWidth - monStr->width) / 2, drawCursorY - monImage.height(), monImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
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
        int subtileRef = this->subtiles[dunCursorY][dunCursorX];
        if (subtileRef != 0) {
            int specRef = 0;
            const DungeonStruct &ds = dungeonTbl[this->levelType];
            for (int i = 0; i < ds.numSpecCels; i++) {
                if (ds.specialCels[i].first == subtileRef) {
                    specRef = ds.specialCels[i].second;
                }
            }
            if (specRef != 0) {
                QImage subtileImage = specGfx->getFrameImage(specRef - 1);
                dungeon.drawImage(drawCursorX, drawCursorY - subtileImage.height(), subtileImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            }
        }
    }
}

QImage D1Dun::getImage(const DunDrawParam &params) const
{
    int maxDunSize = std::max(this->width, this->height);
    int minDunSize = std::min(this->width, this->height);
    int maxTilSize = std::max(TILE_WIDTH, TILE_HEIGHT);

    unsigned subtileWidth = this->til->getMin()->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->til->getMin()->getSubtileHeight() * MICRO_HEIGHT;

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
        unsigned len = 0;
        unsigned y = 1;
        for (; y <= cellHeight / 2; y++) {
            len += 2;
            for (unsigned x = cellWidth / 2 - len - CELL_BORDER; x < cellWidth / 2 + len + CELL_BORDER; x++) {
                backImage.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, backColor);
            }
        }
        for (; y < cellHeight; y++) {
            len -= 2;
            for (unsigned x = cellWidth / 2 - len - CELL_BORDER; x < cellWidth / 2 + len + CELL_BORDER; x++) {
                backImage.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, backColor);
            }
        }
    } else {
        unsigned len = 0;
        unsigned y = 1;
        for (; y <= cellHeight / 2; y++) {
            len += 2;
            for (unsigned x = cellWidth / 2 - len - CELL_BORDER; x <= cellWidth / 2 - len; x++) {
                backImage.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, backColor);
            }
            for (unsigned x = cellWidth / 2 + len - 1; x <= cellWidth / 2 + len + CELL_BORDER; x++) {
                backImage.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, backColor);
            }
        }
        for (; y < cellHeight; y++) {
            len -= 2;
            for (unsigned x = cellWidth / 2 - len - CELL_BORDER; x <= cellWidth / 2 - len; x++) {
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
        entry.second->setPalette(pal);
    }
    for (auto &entry : this->monsterCache) {
        // TODO: apply trnPath
        entry.second->setPalette(pal);
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
                hasContent |= this->transvals[y][x] != 0;
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
    for (std::vector<int> &transRow : this->transvals) {
        transRow.resize(newWidth);
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
                hasContent |= this->transvals[y][x] != 0;
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
    this->transvals.resize(newHeight);
    for (int y = prevHeight; y < newHeight; y++) {
        this->tiles[y / TILE_HEIGHT].resize(width / TILE_WIDTH);
        this->subtiles[y].resize(width);
        this->items[y].resize(width);
        this->monsters[y].resize(width);
        this->objects[y].resize(width);
        this->transvals[y].resize(width);
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
    int tileRef = 0;
    for (int y = 0; y < TILE_HEIGHT; y++) {
        for (int x = 0; x < TILE_WIDTH; x++) {
            if (this->subtiles[posy + y][posx + x] != 0) {
                tileRef = UNDEF_TILE;
            }
        }
    }

    this->tiles[posy / TILE_HEIGHT][posx / TILE_WIDTH] = tileRef;
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

int D1Dun::getTransvalAt(int posx, int posy) const
{
    return this->transvals[posy][posx];
}

bool D1Dun::setTransvalAt(int posx, int posy, int transval)
{
    if (this->transvals[posy][posx] == transval) {
        return false;
    }
    this->transvals[posy][posx] = transval;
    this->modified = true;
    return true;
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

D1Gfx *D1Dun::loadObject(int objectIndex)
{
    std::pair<const ObjectStruct *, D1Gfx *> result = { &ObjConvTbl[objectIndex], nullptr };
    if (!this->assetPath.isEmpty() && objectIndex < lengthof(ObjConvTbl)) {
        D1Gfx *gfx = new D1Gfx();
        gfx->setPalette(this->pal);
        QString celFilePath = this->assetPath + "/Objects/" + result.first->path;
        OpenAsParam params = OpenAsParam();
        params.celWidth = result.first->width;
        if (!D1Cel::load(*gfx, celFilePath, params) {
            // TODO: suppress errors? MemFree?
            delete gfx;
            gfx = nullptr;
        }
        result.second = gfx;
    }
    this->objectCache.push_back(result);
    return result.second;
}

D1Gfx *D1Dun::loadMonster(int monsterIndex)
{
    std::pair<const MonsterStruct *, D1Gfx *> result = { &MonstConvTbl[monsterIndex], nullptr };
    if (!this->assetPath.isEmpty() && monsterIndex < lengthof(MonstConvTbl)) {
        D1Gfx *gfx = new D1Gfx();
        gfx->setPalette(this->pal);
        QString cl2FilePath = this->assetPath + "/Monsters/" + result.first->path;
        OpenAsParam params = OpenAsParam();
        params.celWidth = result.first->width;
        if (!D1Cl2::load(*gfx, cl2FilePath, params) {
            // TODO: suppress errors? MemFree?
            delete gfx;
            gfx = nullptr;
        } else {
            // TODO: apply result.first->trnPath
        }
        result.second = gfx;
    }
    this->monsterCache.push_back(result);
    return result.second;
}

void D1Dun::clearAssets()
{
    for (auto &entry : this->objectCache) {
        delete entry.second;
    }
    this->objectCache.clear();
    for (auto &entry : this->monsterCache) {
        delete entry.second;
    }
    this->monsterCache.clear();
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
            // FIXME: set modified if type == RDUN and not in init phase
        }
    }
}
