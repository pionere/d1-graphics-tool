#include "d1dun.h"

#include <QApplication>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QMessageBox>

#include "builderwidget.h"
#include "config.h"
#include "d1cel.h"
#include "d1cl2.h"
#include "d1pal.h"
#include "d1sla.h"
#include "d1til.h"
#include "d1tileset.h"
#include "d1trn.h"
#include "mainwindow.h"
#include "progressdialog.h"

#include "dungeon/all.h"

typedef struct DungeonStruct {
    int defaultTile;
} DungeonStruct;

const DungeonStruct dungeonTbl[NUM_DUNGEON_TYPES] = {
    // clang-format off
/* DTYPE_TOWN      */ { UNDEF_TILE },
/* DTYPE_CATHEDRAL */ { 13,        },
/* DTYPE_CATACOMBS */ { 3,         },
/* DTYPE_CAVES     */ { 7,         },
/* DTYPE_HELL      */ { 6,         },
/* DTYPE_CRYPT     */ { 13,        },
/* DTYPE_NEST      */ { 7,         },
/* DTYPE_NONE      */ { UNDEF_TILE },
    // clang-format on
};

const DunObjectStruct DunObjConvTbl[128] = {
    // clang-format off
                     { nullptr },
/*OBJ_LEVER*/        { "Lever" },       // SklKng2.DUN
/*OBJ_CRUXM*/        { "Crucifix1" },   // SklKng2.DUN
/*OBJ_CRUXR*/        { "Crucifix2" },   // SklKng2.DUN
/*OBJ_CRUXL*/        { "Crucifix3" },   // SklKng2.DUN
/*OBJ_SARC*/         { "Sarcophagus" }, // SklKng2.DUN
                     { nullptr }, //OBJ_BANNERL,
                     { nullptr }, //OBJ_BANNERM,
                     { nullptr }, //OBJ_BANNERR,
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
/*OBJ_ANCIENTBOOK*/  { "Ancient Book" }, // Bonecha1.DUN
/*OBJ_BLOODBOOK*/    { "BloodBook" }, // Blood2.DUN (Q_BLOOD)
/*OBJ_TBCROSS*/      { "Burning cross" }, // Bonecha1.DUN
                     { nullptr },
                     { nullptr }, //OBJ_CANDLE1,
/*OBJ_CANDLE2*/      { "Candle" }, // Bonecha1.DUN, Viles.DUN (Q_BETRAYER)
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
/*OBJ_MCIRCLE1*/     { "Magic Circle 1" }, // Vile2.DUN
/*OBJ_MCIRCLE2*/     { "Magic Circle 2" }, // Vile2.DUN
                     { nullptr }, //OBJ_SKFIRE,
                     { nullptr }, //OBJ_SKPILE,
                     { nullptr }, //OBJ_SKSTICK1,
                     { nullptr }, //OBJ_SKSTICK2,
                     { nullptr }, //OBJ_SKSTICK3,
                     { nullptr }, //OBJ_SKSTICK4,
                     { nullptr }, //OBJ_SKSTICK5,
                     { nullptr },
                     { nullptr },
/*OBJ_VILEBOOK*/     { "Book of Vileness" }, // Vile2.DUN
                     { nullptr },
                     { nullptr },
                     { nullptr },
/*OBJ_SWITCHSKL*/    { "Switch" }, // Bonecha1.DUN, diab1.DUN, diab2a.DUN, diab3a.DUN
                     { nullptr },
                     { nullptr }, //OBJ_TRAPL,
                     { nullptr }, //OBJ_TRAPR,
/*OBJ_TORTUREL1*/    { "Tortured body 1" }, // Butcher.DUN (Q_BUTCHER)
/*OBJ_TORTUREL2*/    { "Tortured body 2" }, // Butcher.DUN (Q_BUTCHER)
/*OBJ_TORTUREL3*/    { "Tortured body 3" }, // Butcher.DUN (Q_BUTCHER)
/*OBJ_TORTUREL4*/    { "Tortured body 4" }, // Butcher.DUN (Q_BUTCHER)
/*OBJ_TORTUREL5*/    { "Tortured body 5" }, // Butcher.DUN (Q_BUTCHER)
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr },
                     { nullptr }, //OBJ_NUDEW2R,
                     { nullptr },
                     { nullptr },
                     { nullptr },
/*OBJ_LAZSTAND*/     { "Lazarus's stand" }, // Viles.DUN (Q_BETRAYER)
/*OBJ_TNUDEM*/       { "Tortured male" }, // Butcher.DUN (Q_BUTCHER)
                     { nullptr },
                     { nullptr },
                     { nullptr },
/*OBJ_TNUDEW*/       { "Tortured female" }, // Butcher.DUN (Q_BUTCHER)
                     { nullptr },
                     { nullptr },
                     { nullptr }, //OBJ_CHEST1,
/*OBJ_CHEST1*/       { "Chest 1" }, // SklKng2.DUN
                     { nullptr }, //OBJ_CHEST1,
                     { nullptr }, //OBJ_CHEST2,
/*OBJ_CHEST2*/       { "Chest 2" }, // SklKng2.DUN
                     { nullptr }, //OBJ_CHEST2,
                     { nullptr }, //OBJ_CHEST3,
/*OBJ_CHEST3*/       { "Chest 3" }, // Bonecha1.DUN
/*OBJ_NAKRULBOOK*/   { "A Spellbook" }, // Nakrul1.DUN (Q_NAKRUL)
/*OBJ_NAKRULLEVER*/  { "Nakrul's lever" }, // Nakrul1.DUN (Q_NAKRUL)
                     { nullptr },
                     { nullptr },
                     { nullptr },
/*OBJ_SIGNCHEST*/    { "Sign Chest" }, // Banner2.DUN (Q_BANNER)
/*OBJ_PEDESTAL*/     { "Pedestal" }, // Blood2.DUN (Q_BLOOD)
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
/*OBJ_ALTBOY*/       { "Altarboy" }, // L4Data/Vile1.DUN (Q_BETRAYER), L1Data/Vile2.DUN
                     { nullptr },
                     { nullptr },
/*OBJ_ARMORSTANDN*/  { "Armor stand" }, //OBJ_ARMORSTAND, // Warlord2.DUN (Q_WARLORD) - changed to inactive versions to eliminate farming potential
/*OBJ_WEAPONRACKLN*/ { "Weapon stand" }, //OBJ_WEAPONRACKL, // Warlord2.DUN (Q_WARLORD)
                     { nullptr }, //OBJ_TORCHR1  //Blood2.DUN (Q_BLOOD)
                     { nullptr }, //OBJ_TORCHL1  //Blood2.DUN (Q_BLOOD)
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
/*MT_RSKELAX*/ { "Burning Dead" }, // SklKng2.DUN
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
/*MT_TSKELBW*/ { "Corpse Bow" },   // SklKng2.DUN
/*MT_RSKELBW*/ { "Burning Dead" }, // SklKng2.DUN
/*MT_XSKELBW*/ { "Horror" },       // SklKng2.DUN
               { nullptr }, //MT_WSKELSD,
               { nullptr }, //MT_TSKELSD,
/*MT_RSKELSD*/ { "Burning Dead Captain" }, // SklKng2.DUN
/*MT_XSKELSD*/ { "Horror Captain" },       // Bonecha1.DUN
               { nullptr }, //MT_NSNEAK,
               { nullptr }, //MT_RSNEAK,
/*MT_BSNEAK*/  { "Unseen" },            // Bonecha1.DUN
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
/*MT_GGOATBW*/ { "Night Clan" }, // Anvil.DUN
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
/*MT_NRHINO*/  { "Horned Demon" }, // Blood2.DUN (Q_BLOOD), Bonecha1.DUN
               { nullptr }, // MT_XRHINO, // Q_MAZE
               { nullptr }, //MT_BRHINO,
/*MT_DRHINO*/  { "Obsidian Lord" }, // Anvil.DUN
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

const DunMissileStruct DunMissConvTbl[96] = {
               { nullptr }, // MIS_ARROW
               { "Point-blank Arrow" }, // MIS_PBARROW
               { "Aimed-shot Arrow" }, // MIS_ASARROW
               { "Multi-shot Arrow" }, // MIS_MLARROW
               { "Arrow" }, // MIS_PCARROW
               { "Firebolt" }, // MIS_FIREBOLT
               { "Fireball" }, // MIS_FIREBALL
               { "Holybolt" }, // MIS_HBOLT
               { "Flare" }, // MIS_FLARE
               { "Snow Witch" }, // MIS_SNOWWICH
               { "Hell Spawn" }, // MIS_HLSPWN
               { "Soul Burner" }, // MIS_SOLBRNR
               { "Mage" }, // MIS_MAGE
               { "Magmaball" }, // MIS_MAGMABALL
               { "Acid" }, // MIS_ACID
               { "Acidpud" }, // MIS_ACIDPUD
               { "Acidpud Explosion" }, // MIS_EXACIDP
               { "Fire Explosion" }, // MIS_EXFIRE
               { "Fireball Explosion" }, // MIS_EXFBALL
               { "Light Explosion" }, // MIS_EXLGHT
               { "Magic Explosion" }, // MIS_EXMAGIC
               { "Acid Explosion" }, // MIS_EXACID
               { "Holybolt Explosion" }, // MIS_EXHOLY
               { "Flare Explosion" }, // MIS_EXFLARE
               { "Snow Witch Explosion" }, // MIS_EXSNOWWICH
               { "Hell Spawn Explosion" }, // MIS_EXHLSPWN
               { "Soul Burner Explosion" }, // MIS_EXSOLBRNR
               { "Mage Explosion" }, // MIS_EXMAGE
               { "Poison" }, // MIS_POISON
               { "Wind" }, // MIS_WIND
               { "Lightball" }, // MIS_LIGHTBALL
               { "Lightning - big" }, // MIS_LIGHTNINGC
               { nullptr }, // MIS_LIGHTNING
               { "Lightning - small" }, // MIS_LIGHTNINGC2
               { nullptr }, // MIS_LIGHTNING2
               { "Bloodboil" }, // MIS_BLOODBOILC
               { nullptr }, // MIS_BLOODBOIL
               { "Swamp" }, // MIS_SWAMPC
               { nullptr }, // MIS_SWAMP
               { "Portal - blue" }, // MIS_TOWN
               { "Portal - red" }, // MIS_RPORTAL
               { "Flash" }, // MIS_FLASH
               { nullptr }, // MIS_FLASH2
               { "Chain lightning" }, // MIS_CHAIN
////           { nullptr }, // MIS_BLODSTAR
////           { nullptr }, // MIS_BONE
////           { nullptr }, // MIS_METLHIT
               { nullptr }, // MIS_RHINO
               { nullptr }, // MIS_CHARGE
               { nullptr }, // MIS_TELEPORT
               { nullptr }, // MIS_RNDTELEPORT
////           { nullptr }, // MIS_FARROW
////           { nullptr }, // MIS_DOOMSERP
               { nullptr }, // MIS_STONE
               { "Shroud" }, // MIS_SHROUD
////           { nullptr }, // MIS_INVISIBL
               { "Guardian" }, // MIS_GUARDIAN
               { nullptr }, // MIS_GOLEM
////           { nullptr }, // MIS_ETHEREALIZE
               { "Bleed" }, // MIS_BLEED
////           { nullptr }, // MIS_EXAPOCA
               { "Firewall" }, // MIS_FIREWALLC
               { nullptr }, // MIS_FIREWALL
               { "Firewave" }, // MIS_FIREWAVEC
               { nullptr }, // MIS_FIREWAVE
               { "Meteor" }, // MIS_METEOR
               { "Nova" }, // MIS_LIGHTNOVAC
////           { nullptr }, // MIS_APOCAC
               { nullptr }, // MIS_HEAL
               { nullptr }, // MIS_HEALOTHER
               { "Resurrect" }, // MIS_RESURRECT
               { "Attract" }, // MIS_ATTRACT
               { nullptr }, // MIS_TELEKINESIS
////           { nullptr }, // MIS_LARROW
               { nullptr }, // MIS_OPITEM
               { nullptr }, // MIS_REPAIR
               { nullptr }, // MIS_DISARM
               { "Inferno" }, // MIS_INFERNOC
               { nullptr }, // MIS_INFERNO
////           { nullptr }, // MIS_FIRETRAP
               { "Barrel Explosion" }, // MIS_BARRELEX
////           { nullptr }, // MIS_FIREMAN
////           { nullptr }, // MIS_KRULL
               { "Charged bolt" }, // MIS_CBOLTC
               { nullptr }, // MIS_CBOLT
               { "Elemental" }, // MIS_ELEMENTAL
////           { nullptr }, // MIS_BONESPIRIT
               { "Apocalypse" }, // MIS_APOCAC2
               { "Apocalypse Explosion" }, // MIS_EXAPOCA2
               { nullptr }, // MIS_MANASHIELD
               { nullptr }, // MIS_INFRA
               { nullptr }, // MIS_RAGE

////           { nullptr }, // MIS_LIGHTWALLC
////           { nullptr }, // MIS_LIGHTWALL
////           { nullptr }, // MIS_FIRENOVAC
////           { nullptr }, // MIS_FIREBALL2
////           { nullptr }, // MIS_REFLECT
               { "Fire Ring" }, // MIS_FIRERING
////           { nullptr }, // MIS_MANATRAP
////           { nullptr }, // MIS_LIGHTRING
               { "Rune" }, // MIS_RUNEFIRE
               { nullptr }, // MIS_RUNELIGHT
               { nullptr }, // MIS_RUNENOVA
               { nullptr }, // MIS_RUNEWAVE
               { nullptr }, // MIS_RUNESTONE
               { "Fire Explosion" }, // MIS_FIREEXP
               { "Hork Demon" }, // MIS_HORKDMN
               { "Psychorb" }, // MIS_PSYCHORB
               { "Lich" }, // MIS_LICH
               { "Bone Demon" }, // MIS_BONEDEMON
               { "Arch Lich" }, // MIS_ARCHLICH
               { "Necromorb" }, // MIS_NECROMORB
               { "Psychorb Explosion" }, // MIS_EXPSYCHORB
               { "Lich Explosion" }, // MIS_EXLICH
               { "Bone Demon Explosion" }, // MIS_EXBONEDEMON
               { "Arch Lich Explosion" }, // MIS_EXARCHLICH
               { "Necromorb Explosion" }, // MIS_EXNECROMORB,
    // clang-format on
};
	// clang-format on


D1Dun::~D1Dun()
{
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
    tileProtections.resize(dunHeight);
    for (int i = 0; i < dunHeight; i++) {
        tileProtections[i].resize(dunWidth);
    }
    subtileProtections.resize(dunHeight * TILE_HEIGHT);
    for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
        subtileProtections[i].resize(dunWidth * TILE_WIDTH);
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
    missiles.resize(dunHeight * TILE_HEIGHT);
    for (int i = 0; i < dunHeight * TILE_HEIGHT; i++) {
        missiles[i].resize(dunWidth * TILE_WIDTH);
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
                // read protections
                for (int y = 0; y < dunHeight; y++) {
                    for (int x = 0; x < dunWidth; x++) {
                        in >> readWord;
                        Qt::CheckState tps = (readWord & 3) == 3 ? Qt::Checked : ((readWord & 1) ? Qt::PartiallyChecked : Qt::Unchecked);
                        this->tileProtections[y][x] = tps;
                        readWord >>= 8;
                        this->subtileProtections[2 * y + 0][2 * x + 0] = readWord & 3;
                        readWord >>= 2;
                        this->subtileProtections[2 * y + 0][2 * x + 1] = readWord & 3;
                        readWord >>= 2;
                        this->subtileProtections[2 * y + 1][2 * x + 0] = readWord & 3;
                        readWord >>= 2;
                        this->subtileProtections[2 * y + 1][2 * x + 1] = readWord & 3;
                    }
                }
                for (int x = 0; x < 3 * dunWidth * dunHeight; x++) {
                    in >> readWord;
                }
                numLayers++;
            } else {
                dProgressWarn() << tr("Protections are not defined in the DUN file.");
            }

            if (dataSize >= dataUnitSize) {
                dataSize -= dataUnitSize;
                // read monsters
                for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                    for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                        in >> readWord;
                        this->monsters[y][x] = { { readWord & INT16_MAX, (readWord & ~INT16_MAX) != 0 }, 0, 0 };
                    }
                }
                numLayers++;
            } else {
                dProgress() << tr("Monsters are not defined in the DUN file.");
            }

            if (dataSize >= dataUnitSize) {
                dataSize -= dataUnitSize;
                // read objects
                for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                    for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                        in >> readWord;
                        this->objects[y][x] = { readWord, 0 };
                    }
                }
                numLayers++;
            } else {
                dProgress() << tr("Objects are not defined in the DUN file.");
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
                // numLayers++;
            } else {
                ; // dProgressWarn() << tr("Rooms are not defined in the DUN file.");
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
    return true;
}

void D1Dun::setTileset(D1Tileset *ts)
{
    this->tileset = ts;
    this->cls = ts->cls;
    this->til = ts->til;
    this->min = ts->min;
    this->sla = ts->sla;
}

void D1Dun::initialize(D1Pal *p, D1Tileset *ts)
{
    this->pal = p;

    setTileset(ts);

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
            if (this->tileProtections[y / TILE_WIDTH][x / TILE_HEIGHT] != Qt::Unchecked) {
                layers |= 1 << 0;
            }
            if (this->subtileProtections[y][x] != 0) {
                layers |= 1 << 0;
            }
            if (this->monsters[y][x].moType.monIndex != 0 || this->monsters[y][x].moType.monUnique) {
                layers |= 1 << 1;
            }
            if (this->objects[y][x].oType != 0) {
                layers |= 1 << 2;
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
                    dProgressFail() << tr("Undefined tiles (one at %1:%2) can not be saved in this format (DUN).").arg(x * TILE_WIDTH).arg(y * TILE_HEIGHT);
                    return false;
                }
            }
        }
        // report unsaved information
        for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
            for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                if (this->items[y][x] != 0) {
                    dProgressWarn() << tr("Defined item at %1:%2 is not saved.").arg(x).arg(y);
                }
                if (this->rooms[y][x] != 0) {
                    dProgressWarn() << tr("Defined room at %1:%2 is not saved.").arg(x).arg(y);
                }
                if (this->missiles[y][x].miType != 0) {
                    dProgressWarn() << tr("Defined missile at %1:%2 is not saved.").arg(x).arg(y);
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
        uint8_t layersNeeded = layers >= (1 << 2) ? 3 : (layers >= (1 << 1) ? 2 : (layers >= (1 << 0) ? 1 : 0));
        if (params.dunLayerNum != UINT8_MAX) {
            // user defined the number of layers -> report unsaved information
            if (params.dunLayerNum < layersNeeded) {
                if (params.dunLayerNum <= 0 && (layers & (1 << 0))) {
                    dProgressWarn() << tr("Defined protection is not saved.");
                }
                if (params.dunLayerNum <= 1 && (layers & (1 << 1))) {
                    dProgressWarn() << tr("Defined monster is not saved.");
                }
                if (params.dunLayerNum <= 2 && (layers & (1 << 2))) {
                    dProgressWarn() << tr("Defined object is not saved.");
                }
            }
            numLayers = params.dunLayerNum;
        } else {
            // user did not define the number of layers -> extend if necessary, otherwise preserve
            if (layersNeeded > numLayers) {
                numLayers = layersNeeded;
            }
        }
        // report if the requirement of the game is not meet
        if (numLayers < 1) {
            dProgressWarn() << tr("The DUN file has to have a layer for protections to be used in the game.");
        }
    } else {
        // rdun - subtiles must be defined
        int dunWidth = this->width;
        int dunHeight = this->height;
        if (dunWidth != dunHeight) {
            dProgressFail() << tr("Non-square dungeons (%1:%2) can not be saved in this format (RDUN).").arg(dunWidth).arg(dunHeight);
            return false;
        }
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
        for (int y = 0; y < dunHeight; y++) {
            for (int x = 0; x < dunWidth; x++) {
                if (this->items[y][x] != 0) {
                    dProgressWarn() << tr("Defined item at %1:%2 is not saved.").arg(x).arg(y);
                }
                if (this->rooms[y][x] != 0) {
                    dProgressWarn() << tr("Defined room at %1:%2 is not saved.").arg(x).arg(y);
                }
                if (this->missiles[y][x].miType != 0) {
                    dProgressWarn() << tr("Defined missile at %1:%2 is not saved.").arg(x).arg(y);
                }
            }
        }
        if (layers & (1 << 0)) {
            dProgressWarn() << tr("Defined protection is not saved in this format (RDUN).");
        }
        if (layers & (1 << 1)) {
            dProgressWarn() << tr("Defined monster is not saved in this format (RDUN).");
        }
        if (layers & (1 << 2)) {
            dProgressWarn() << tr("Defined object is not saved in this format (RDUN).");
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

        // write protections
        if (numLayers >= 1) {
            for (int y = 0; y < dunHeight; y++) {
                for (int x = 0; x < dunWidth; x++) {
                    Qt::CheckState tps = this->tileProtections[y][x];
                    writeWord = tps == Qt::Checked ? 3 : (tps == Qt::PartiallyChecked ? 1 : 0);
                    writeWord |= this->subtileProtections[2 * y + 0][2 * x + 0] << 8;
                    writeWord |= this->subtileProtections[2 * y + 0][2 * x + 1] << 10;
                    writeWord |= this->subtileProtections[2 * y + 1][2 * x + 0] << 12;
                    writeWord |= this->subtileProtections[2 * y + 1][2 * x + 1] << 14;
                    out << writeWord;
                }
            }
            writeWord = 0;
            for (int x = 0; x < dunWidth * dunHeight * 3; x++) {
                out << writeWord;
            }
        }

        // write monsters
        if (numLayers >= 2) {
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    writeWord = this->monsters[y][x].moType.monIndex | (this->monsters[y][x].moType.monUnique ? 1 << 15 : 0);
                    out << writeWord;
                }
            }
        }

        // write objects
        if (numLayers >= 3) {
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    writeWord = this->objects[y][x].oType;
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

static void reportDiff(const QString text, QString &header)
{
    if (!header.isEmpty()) {
        dProgress() << header;
        header.clear();
    }
    dProgress() << text;
}
void D1Dun::compareTo(const LoadFileContent *fileContent) const
{
    D1Dun *dunB = fileContent->dun;
    if (dunB != nullptr) {
        QString header = QApplication::tr("Dungeon:");
        if (dunB->width == this->width && dunB->height == this->height) {
            int dunWidth = this->width / TILE_WIDTH;
            int dunHeight = this->height / TILE_HEIGHT;

            for (int y = 0; y < dunHeight; y++) {
                for (int x = 0; x < dunWidth; x++) {
                    if (this->tiles[y][x] != dunB->tiles[y][x]) {
                        reportDiff(QApplication::tr("tile %1:%2 is %3 (was %4)").arg(x).arg(y)
                            .arg(this->tiles[y][x])
                            .arg(dunB->tiles[y][x]), header);
                    }

                }
            }
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    if (this->subtiles[y][x] != dunB->subtiles[y][x]) {
                        reportDiff(QApplication::tr("subtile %1:%2 is %3 (was %4)").arg(x).arg(y)
                            .arg(this->subtiles[y][x])
                            .arg(dunB->subtiles[y][x]), header);
                    }

                }
            }
            for (int y = 0; y < dunHeight; y++) {
                for (int x = 0; x < dunWidth; x++) {
                    if (this->tileProtections[y][x] != dunB->tileProtections[y][x]) {
                        reportDiff(QApplication::tr("tile-protection %1:%2 is %3 (was %4)").arg(x).arg(y)
                            .arg(this->tileProtections[y][x])
                            .arg(dunB->tileProtections[y][x]), header);
                    }

                }
            }
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    if (this->subtileProtections[y][x] != dunB->subtileProtections[y][x]) {
                        reportDiff(QApplication::tr("subtile-protection %1:%2 is %3 (was %4)").arg(x).arg(y)
                            .arg(this->subtileProtections[y][x])
                            .arg(dunB->subtileProtections[y][x]), header);
                    }

                }
            }
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    if (this->items[y][x] != dunB->items[y][x]) {
                        reportDiff(QApplication::tr("item %1:%2 is %3 (was %4)").arg(x).arg(y)
                            .arg(this->items[y][x] == 0 ? "-" : this->getItemName(this->items[y][x]))
                            .arg(dunB->items[y][x] == 0 ? "-" : this->getItemName(dunB->items[y][x])), header);
                    }

                }
            }
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    if (this->objects[y][x].oType != dunB->objects[y][x].oType) {
                        reportDiff(QApplication::tr("object %1:%2 is %3 (was %4)").arg(x).arg(y)
                            .arg(this->objects[y][x].oType == 0 ? "-" : this->getObjectName(this->objects[y][x].oType))
                            .arg(dunB->objects[y][x].oType == 0 ? "-" : this->getObjectName(dunB->objects[y][x].oType)), header);
                    }

                }
            }
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    const DunMonsterType &aMonster = this->monsters[y][x].moType;
                    const DunMonsterType &bMonster = dunB->monsters[y][x].moType;
                    if (this->monsters[y][x].moType != dunB->monsters[y][x].moType) {
                        QString aMonsterName = (aMonster.monIndex || aMonster.monUnique) ? this->getMonsterName(aMonster) : QString("-");
                        QString bMonsterName = (bMonster.monIndex || bMonster.monUnique) ? this->getMonsterName(bMonster) : QString("-");
                        reportDiff(QApplication::tr("monster %1:%2 is %3 (was %4)").arg(x).arg(y)
                            .arg(aMonsterName)
                            .arg(bMonsterName), header);
                    }

                }
            }
            for (int y = 0; y < dunHeight * TILE_HEIGHT; y++) {
                for (int x = 0; x < dunWidth * TILE_WIDTH; x++) {
                    if (this->rooms[y][x] != dunB->rooms[y][x]) {
                        reportDiff(QApplication::tr("room %1:%2 is %3 (was %4)").arg(x).arg(y)
                            .arg(this->rooms[y][x])
                            .arg(dunB->rooms[y][x]), header);
                    }

                }
            }
        } else {
                reportDiff(QApplication::tr("size is %1x%2 (was %3x%4)")
                    .arg(this->width).arg(this->height)
                    .arg(dunB->width).arg(dunB->height), header);
        }
    } else {
        dProgressErr() << tr("Not a dungeon file (%1)").arg(MainWindow::FileContentTypeToStr(fileContent->fileType));
    }
}

#define MAP_SCALE_MIN (128 * 2)
static unsigned AmLine64;
static unsigned AmLine32;
static unsigned AmLine16;
/** color for bright map lines (doors, stairs etc.) */
#define COLOR_BRIGHT PAL8_YELLOW
/** color for dim map lines/dots */
#define COLOR_DIM (PAL16_YELLOW + 8)

static QPainter *DunPainter = nullptr;
static D1Pal *DunPal = nullptr;

void D1Dun::DrawDiamond(QImage &image, unsigned sx, unsigned sy, unsigned width, const QColor &color)
{
    unsigned len = 0;
    unsigned y = 1;
    QRgb *destBits = reinterpret_cast<QRgb *>(image.scanLine(sy + y));
    destBits += sx;
    QRgb srcBit = color.rgba();
    for ( ; y <= width / 4; y++) {
        len += 2;
        for (unsigned x = width / 2 - len; x < width / 2 + len; x++) {
            // image.setPixelColor(sx + x, sy + y, color);
            destBits[x] = srcBit;
        }
        destBits += image.width();
    }
    for ( ; y < width / 2; y++) {
        len -= 2;
        for (unsigned x = width / 2 - len; x < width / 2 + len; x++) {
            // image.setPixelColor(sx + x, sy + y, color);
            destBits[x] = srcBit;
        }
        destBits += image.width();
    }
}

QImage D1Dun::getObjectImage(const MapObject &mapObj)
{
    const ObjectCacheEntry *objEntry = nullptr;
    for (const auto &obj : this->objectCache) {
        if (obj.objectIndex == mapObj.oType) {
            objEntry = &obj;
            break;
        }
    }
    if (objEntry == nullptr) {
        this->loadObject(mapObj.oType);
        objEntry = &this->objectCache.back();
    }
    QImage result = QImage();
    if (objEntry->objGfx != nullptr) {
        const int frameCount = objEntry->objGfx->getFrameCount();
        if (frameCount != 0) {
            int baseFrameNum = mapObj.baseFrameNum;
            if (baseFrameNum == 0)
                baseFrameNum = objEntry->baseFrameNum;
            if (baseFrameNum != 0) {
                result = objEntry->objGfx->getFrameImage(baseFrameNum - 1);
            }
            int animFrameNum = mapObj.animFrameNum;
            if (animFrameNum == 0)
                animFrameNum = objEntry->animFrameNum;
            if (animFrameNum >= 0) {
                if (animFrameNum == 0 || frameCount < animFrameNum)
                    animFrameNum = 1;
                QImage anim = objEntry->objGfx->getFrameImage(animFrameNum - 1);
                if (result.isNull()) {
                    result = anim;
                } else if (anim.width() == result.width() && anim.height() == result.height()) {
                    QRgb *srcBits = reinterpret_cast<QRgb *>(anim.bits());
                    QRgb *destBits = reinterpret_cast<QRgb *>(result.bits());
                    for (int y = 0; y < anim.height(); y++) {
                        for (int x = 0; x < anim.width(); x++, srcBits++, destBits++) {
                            if (qAlpha(*srcBits) == 0) continue;
                            *destBits = *srcBits;
                        }
                    }
                }
            }
        }
    }
    return result;
}

QImage D1Dun::getMonsterImage(const MapMonster &mapMon)
{
    const MonsterCacheEntry *monEntry = nullptr;
    for (const auto &mon : this->monsterCache) {
        if (mon.monType == mapMon.moType) {
            monEntry = &mon;
            break;
        }
    }
    if (monEntry == nullptr) {
        this->loadMonster(mapMon);
        monEntry = &this->monsterCache.back();
    }
    if (monEntry->monGfx != nullptr && monEntry->monGfx->getGroupCount() > mapMon.moDir) {
        const std::pair<int, int> frameIndices = monEntry->monGfx->getGroupFrameIndices(mapMon.moDir);
        // int frameIdx = frameIndices.first + (time % (frameIndices.second - frameIndices.first + 1));
        int frameIdx = frameIndices.first + ((unsigned)mapMon.frameNum % (unsigned)(frameIndices.second - frameIndices.first + 1));
        if (mapMon.moType.monDeadFlag)
            frameIdx = frameIndices.second;
        monEntry->monGfx->setPalette(monEntry->monPal);
        return monEntry->monGfx->getFrameImage(frameIdx);
    } else {
        return QImage();
    }
}

QImage D1Dun::getItemImage(int itemIndex)
{
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
        return itemEntry->itemGfx->getFrameImage(frameNum - 1);
    } else {
        return QImage();
    }
}

QImage D1Dun::getMissileImage(const MapMissile &mapMis)
{
    const MissileCacheEntry *misEntry = nullptr;
    for (const auto &mis : this->missileCache) {
        if (mis.misIndex == mapMis.miType) {
            misEntry = &mis;
            break;
        }
    }
    if (misEntry == nullptr) {
        this->loadMissile(mapMis.miType);
        misEntry = &this->missileCache.back();
    }
    if ((unsigned)mapMis.miDir < (unsigned)lengthof(misEntry->misGfx)) {
        D1Gfx *gfx = misEntry->misGfx[mapMis.miDir];
        if (gfx != nullptr) {
            const int frameCount = gfx->getFrameCount();
            if (frameCount != 0) {
                int frameNum = mapMis.frameNum;
                if (frameNum == 0 || frameCount < frameNum) {
                    frameNum = 1;
                }
                return gfx->getFrameImage(frameNum - 1);
            }
        }
    }
    return QImage();
}

typedef enum _draw_mask {
    DM_WALL    = 1 << 0, // WallMask
    DM_LFOL    = 1 << 1, // LeftFoliageMask
    DM_LTRI    = 2 << 1, // LeftTriangleMask
    DM_LMASK   = (DM_LTRI | DM_LFOL), // LeftMask

    DM_RFOL    = 1 << 3, // RightFoliageMask
    DM_RTRI    = 2 << 3, // RightTriangleMask
    DM_RMASK   = (DM_RTRI | DM_RFOL), // RightMask
} _draw_mask;

static void drawSubtile(QPainter &dungeon, QImage subtileImage, int drawCursorX, int drawCursorY, unsigned backWidth, unsigned backHeight, unsigned drawMask)
{
    QImage *destImage = (QImage *)dungeon.device();
    // assert(subtileImage.height() >= backHeight);
    // QRgb *srcBits = reinterpret_cast<QRgb *>(subtileImage.scanLine(subtileImage.height() - backHeight));
    QRgb *srcBits = reinterpret_cast<QRgb *>(subtileImage.bits());
    // assert(drawCursorY >= backHeight);
    QRgb *destBits;
    int line = drawCursorY;
    line -= subtileImage.height();
    destBits = reinterpret_cast<QRgb *>(destImage->scanLine(line));
    destBits += drawCursorX;
    if (drawMask & DM_WALL) {
        // draw the non-floor bits
        for (unsigned y = 0; y < subtileImage.height() - backHeight; y++) {
            // assert(subtileImage.width() == backWidth);
            for (unsigned x = 0; x < backWidth; x++, srcBits++, destBits++) {
                if (qAlpha(*srcBits) == 0) {
                    continue;
                }
                *destBits = *srcBits;
            }
            destBits += destImage->width() - backWidth;
        }
    } else {
        // assert(subtileImage.height() == backHeight);
    }
    // draw the floor
    if ((drawMask & ~DM_WALL) == 0) {
        return;
    }

    for (unsigned y = 0; y < backHeight; y++) {
        unsigned x = 0;
        if ((drawMask & DM_LMASK) == 0) {
            x += backWidth / 2;
            srcBits += backWidth / 2;
            destBits += backWidth / 2;
        }
        unsigned limit = backWidth;
        if ((drawMask & DM_RMASK) == 0) {
            limit = backWidth / 2;
        }
        // assert(subtileImage.width() == backWidth);
        for ( ; x < limit; x++, srcBits++, destBits++) {
            if (qAlpha(*srcBits) == 0) {
                continue;
            }
            if (x < backWidth / 2) {
                bool inLeftTriangle = y >= (backHeight / 2 - x / 2) && y <= (backHeight / 2 + x / 2);
                if (!(drawMask & DM_LFOL) && !inLeftTriangle) {
                    continue;
                }
                if (!(drawMask & DM_LTRI) && inLeftTriangle) {
                    continue;
                }
            } else {
                bool inRightTriangle = y >= (1 + (x - backWidth / 2) / 2) && y < (backHeight - (x - backWidth / 2) / 2);
                if (!(drawMask & DM_RFOL) && !inRightTriangle) {
                    continue;
                }
                if (!(drawMask & DM_RTRI) && inRightTriangle) {
                    continue;
                }
            }
            *destBits = *srcBits;
        }
        if ((drawMask & DM_RMASK) == 0) {
            srcBits += backWidth / 2;
            destBits += backWidth / 2;
        }
        destBits += destImage->width() - backWidth;
    }
}

void D1Dun::drawBack(QPainter &dungeon, unsigned cellWidth, int drawCursorX, int drawCursorY, const DunDrawParam &params)
{
    QColor backColor = QColor(Config::getGraphicsTransparentColor());
    QRgb srcBit = backColor.rgba();
    unsigned cellHeight = cellWidth / 2;
    unsigned sx = drawCursorX + cellWidth / 2;
    unsigned sy = drawCursorY - cellHeight + 1;
    QImage *destImage = (QImage *)dungeon.device();
    unsigned imgWidth = destImage->width();
    if (params.tileState != Qt::Unchecked) {
        unsigned len = 4;
        unsigned drawlines = (this->width + this->height) * cellHeight / 2;
        unsigned wilines = this->width * cellHeight / 2;
        unsigned helines = this->height * cellHeight / 2;
        QRgb *destBits = reinterpret_cast<QRgb *>(destImage->scanLine(0 + sy));
        destBits += sx - 2;
        for (unsigned n = 0; n < drawlines - 1; n++) {
            for (unsigned x = 0; x < len; x++) {
                destBits[x] = srcBit;
            }
            destBits += imgWidth;
            if (n < helines - 1) {
                len += 2;
                destBits -= 2;
            } else {
                len -= 2;
                destBits += 2;
            }
            if (n < wilines - 1) {
                len += 2;
            } else {
                len -= 2;
            }
        }
    } else {
        {
            unsigned drawlines = this->height;
            unsigned dx = this->width * cellWidth / 2;
            unsigned dy = this->width * cellHeight / 2;
            QRgb *db = reinterpret_cast<QRgb *>(destImage->scanLine(sy));
            db += sx;
            for (unsigned n = 0; n < drawlines; n++) {
                QRgb *destBits = db;
                for (unsigned n = 0; n < dx / 2; n++) {
                    destBits[0] = srcBit;
                    destBits[1] = srcBit;
                    destBits += imgWidth + 2;
                }
                db += imgWidth * (cellHeight / 2) - cellWidth / 2;
                /*destBits = db - imgWidth;
                for (unsigned n = 0; n < dx / 2; n++) {
                    destBits[0] = srcBit;
                    destBits[1] = srcBit;
                    destBits += imgWidth + 2;
                }*/
            }
            {
                QRgb *destBits = db - imgWidth;
                for (unsigned n = 0; n < dx / 2; n++) {
                    destBits[0] = srcBit;
                    destBits[1] = srcBit;
                    destBits += imgWidth + 2;
                }
            }
        }
        {
            unsigned drawlines = this->width;
            unsigned dx = this->height * cellWidth / 2;
            unsigned dy = this->height * cellHeight / 2;
            QRgb *db = reinterpret_cast<QRgb *>(destImage->scanLine(sy));
            db += sx - 2;
            for (unsigned n = 0; n < drawlines; n++) {
                QRgb *destBits = db;
                for (unsigned n = 0; n < dx / 2; n++) {
                    destBits[0] = srcBit;
                    destBits[1] = srcBit;
                    destBits += imgWidth - 2;
                }
                db += imgWidth * (cellHeight / 2) + cellWidth / 2;
                /*destBits = db - imgWidth;
                for (unsigned n = 0; n < dx / 2; n++) {
                    destBits[0] = srcBit;
                    destBits[1] = srcBit;
                    destBits += imgWidth - 2;
                }*/
            }
            {
                QRgb *destBits = db - imgWidth;
                for (unsigned n = 0; n < dx / 2; n++) {
                    destBits[0] = srcBit;
                    destBits[1] = srcBit;
                    destBits += imgWidth - 2;
                }
            }
        }
    }
}

void D1Dun::drawFloor(QPainter &dungeon, unsigned cellWidth, int drawCursorX, int drawCursorY, int dunCursorX, int dunCursorY, const DunDrawParam &params)
{
    const unsigned backWidth = cellWidth;
    const unsigned backHeight = cellWidth / 2;
    if (params.tileState != Qt::Unchecked) {
        // draw the subtile
        int tileRef = this->tiles[dunCursorY / TILE_HEIGHT][dunCursorX / TILE_WIDTH];
        int subtileRef = this->subtiles[dunCursorY][dunCursorX];
        if (subtileRef != 0) {
            if (subtileRef >= 0 && subtileRef <= this->min->getSubtileCount()) {
                unsigned drawMask = 0;
                if (params.tileState == Qt::Checked) {
                    quint8 rp = this->sla->getRenderProperties(subtileRef - 1);
                    if ((rp & (TMIF_LEFT_REDRAW | TMIF_LEFT_FOLIAGE)) != TMIF_LEFT_REDRAW) {
                        drawMask |= DM_LMASK;
                    }
                    if ((rp & (TMIF_RIGHT_REDRAW | TMIF_RIGHT_FOLIAGE)) != TMIF_RIGHT_REDRAW) {
                        drawMask |= DM_RMASK;
                    }
                } else {
                    drawMask = DM_LTRI | DM_RTRI;
                }
                if (drawMask != 0) {
                    QImage subtileImage = this->min->getFloorImage(subtileRef - 1);
                    drawSubtile(dungeon, subtileImage, drawCursorX, drawCursorY, backWidth, backHeight, drawMask);
                }
            }
        }
    }
}

void D1Dun::drawCell(QPainter &dungeon, unsigned cellWidth, int drawCursorX, int drawCursorY, int dunCursorX, int dunCursorY, const DunDrawParam &params)
{
    const unsigned backWidth = cellWidth;
    const unsigned backHeight = cellWidth / 2;
    unsigned cellCenterX = drawCursorX + backWidth / 2;
    unsigned cellCenterY = drawCursorY - backHeight / 2;
    bool middleText = false;
    bool bottomText = false;
    if (params.tileState != Qt::Unchecked) {
        // draw the subtile
        int tileRef = this->tiles[dunCursorY / TILE_HEIGHT][dunCursorX / TILE_WIDTH];
        int subtileRef = this->subtiles[dunCursorY][dunCursorX];
        if (subtileRef != 0) {
            if (subtileRef >= 0 && subtileRef <= this->min->getSubtileCount()) {
                if (params.tileState == Qt::Checked) {
                    QImage subtileImage = this->min->getSubtileImage(subtileRef - 1);
                    quint8 rp = this->sla->getRenderProperties(subtileRef - 1);
                    unsigned drawMask = DM_WALL;
                    if (rp & TMIF_LEFT_REDRAW) {
                        drawMask |= (rp & TMIF_LEFT_FOLIAGE) ? DM_LFOL : DM_LMASK;
                    }
                    if (rp & TMIF_RIGHT_REDRAW) {
                        drawMask |= (rp & TMIF_RIGHT_FOLIAGE) ? DM_RFOL : DM_RMASK;
                    }
                    drawSubtile(dungeon, subtileImage, drawCursorX, drawCursorY, backWidth, backHeight, drawMask);
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
    if (params.showMissiles) {
        // draw the missile
        const MapMissile &mis = this->missiles[dunCursorY][dunCursorX];
        if (mis.miType != 0 && mis.miPreFlag) {
            QImage misImage = this->getMissileImage(mis);
            if (!misImage.isNull()) {
                int xo = mis.mix;
                int yo = mis.miy;
                dungeon.drawImage(drawCursorX + ((int)backWidth - misImage.width()) / 2 + xo, drawCursorY - misImage.height() + yo, misImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            } else {
                QString text = this->getMissileName(mis.miType);
                QFontMetrics fm(dungeon.font());
                unsigned textWidth = fm.horizontalAdvance(text);
                dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY - (3 * fm.height() / 2), text);
            }
        }
    }
    if (params.showMonsters) {
        // draw the monster
        const MapMonster &mon = this->monsters[dunCursorY][dunCursorX];
        if ((mon.moType.monIndex != 0 || mon.moType.monUnique) && mon.moType.monDeadFlag) {
            QImage monImage = this->getMonsterImage(mon);
            if (!monImage.isNull()) {
                int xo = mon.mox;
                int yo = mon.moy;
                dungeon.drawImage(drawCursorX + ((int)backWidth - monImage.width()) / 2 + xo, drawCursorY - monImage.height() + yo, monImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            } else {
                QString text = this->getMonsterName(mon.moType);
                QFontMetrics fm(dungeon.font());
                unsigned textWidth = fm.horizontalAdvance(text);
                dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY - (3 * fm.height() / 2), text);
            }
        }
    }
    if (params.showObjects) {
        // draw the object
        const MapObject &obj = this->objects[dunCursorY][dunCursorX];
        if (obj.oType != 0 && obj.oPreFlag) {
            QImage objectImage = this->getObjectImage(obj);
            if (!objectImage.isNull()) {
                dungeon.drawImage(drawCursorX + ((int)backWidth - objectImage.width()) / 2, drawCursorY - objectImage.height(), objectImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            } else {
                QString text = this->getObjectName(obj.oType);
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
            QImage itemImage = this->getItemImage(itemIndex);
            if (!itemImage.isNull()) {
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
    // params.showPlayers && pDeadFlag
    // params.showPlayers && !pDeadFlag
    if (params.showMonsters) {
        // draw the monster
        const MapMonster &mon = this->monsters[dunCursorY][dunCursorX];
        if ((mon.moType.monIndex != 0 || mon.moType.monUnique) && !mon.moType.monDeadFlag) {
            QImage monImage = this->getMonsterImage(mon);
            if (!monImage.isNull()) {
                int xo = mon.mox;
                int yo = mon.moy;
                dungeon.drawImage(drawCursorX + ((int)backWidth - monImage.width()) / 2 + xo, drawCursorY - monImage.height() + yo, monImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            } else {
                QString text = this->getMonsterName(mon.moType);
                QFontMetrics fm(dungeon.font());
                unsigned textWidth = fm.horizontalAdvance(text);
                dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY - (3 * fm.height() / 2), text);
            }
        }
    }
    if (params.showMissiles) {
        // draw the missile
        const MapMissile &mis = this->missiles[dunCursorY][dunCursorX];
        if (mis.miType != 0 && !mis.miPreFlag) {
            QImage misImage = this->getMissileImage(mis);
            if (!misImage.isNull()) {
                int xo = mis.mix;
                int yo = mis.miy;
                dungeon.drawImage(drawCursorX + ((int)backWidth - misImage.width()) / 2 + xo, drawCursorY - misImage.height() + yo, misImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            } else {
                QString text = this->getMissileName(mis.miType);
                QFontMetrics fm(dungeon.font());
                unsigned textWidth = fm.horizontalAdvance(text);
                dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY - (3 * fm.height() / 2), text);
            }
        }
    }
    if (params.showObjects) {
        // draw the object
        const MapObject &obj = this->objects[dunCursorY][dunCursorX];
        if (obj.oType != 0 && !obj.oPreFlag) {
            QImage objectImage = this->getObjectImage(obj);
            if (!objectImage.isNull()) {
                dungeon.drawImage(drawCursorX + ((int)backWidth - objectImage.width()) / 2, drawCursorY - objectImage.height(), objectImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            } else {
                QString text = this->getObjectName(obj.oType);
                QFontMetrics fm(dungeon.font());
                unsigned textWidth = fm.horizontalAdvance(text);
                dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY + (middleText ? 1 : -1) * fm.height() / 2, text);
                bottomText = middleText;
            }
        }
    }
    if (params.tileState == Qt::Checked) {
        // draw special cel
        int subtileRef = this->subtiles[dunCursorY][dunCursorX];
        if (subtileRef > 0 && subtileRef <= this->min->getSubtileCount()) {
            int specRef = this->sla->getSpecProperty(subtileRef - 1);
            if (specRef != 0 && this->cls->getFrameCount() >= specRef) {
                QImage specImage = this->cls->getFrameImage(specRef - 1);
                dungeon.drawImage(drawCursorX, drawCursorY - specImage.height(), specImage, 0, 0, -1, -1, Qt::NoFormatConversion | Qt::NoOpaqueDetection);
            }
        }
    }
}

static void InitAutomapScale(int subtileWidth)
{
    unsigned AutoMapScale = MAP_SCALE_MIN;
    AmLine64 = (AutoMapScale * subtileWidth) / 128;
    AmLine32 = AmLine64 >> 1;
    AmLine16 = AmLine32 >> 1;
}

void D1Dun::DrawPixel(int sx, int sy, uint8_t color)
{
    QColor col = QColor(DunPal->getColor(color));
    QImage *destImage = (QImage *)DunPainter->device();
    QRgb *destBits = reinterpret_cast<QRgb *>(destImage->scanLine(sy));
    destBits[sx] = col.rgba();
}

void D1Dun::DrawLine(int x0, int y0, int x1, int y1, uint8_t color)
{
    QColor col = QColor(DunPal->getColor(color));
    QPen basePen = DunPainter->pen();
    DunPainter->setPen(col);
    DunPainter->drawLine(x0, y0, x1, y1);
    DunPainter->setPen(basePen);
}

void D1Dun::DrawAutomapExtern(int sx, int sy)
{
    unsigned d32 = AmLine32;
    unsigned d16 = (d32 >> 1), d8 = (d32 >> 2), d4 = (d32 >> 3);

    DrawPixel(sx, sy - d8, COLOR_DIM);
    return;

    /*    02
       01    03
          00
    */
    DrawPixel(sx, sy, COLOR_DIM);                 // 00
    DrawPixel(sx - d8, sy - d4, COLOR_DIM);       // 01
    DrawPixel(sx + d8, sy - d4, COLOR_DIM);       // 03
    DrawPixel(sx, sy - d8, COLOR_DIM);            // 02
}

void D1Dun::DrawAutomapStairs(int sx, int sy)
{
    unsigned d32 = AmLine32;
    unsigned d16 = (d32 >> 1), d8 = (d32 >> 2), d4 = (d32 >> 3);

    DrawLine(sx - d16 + d8, sy - d16 + d4, sx + d8, sy - d16 + d4 + d8, COLOR_BRIGHT);
    DrawLine(sx - d16, sy - d8, sx, sy, COLOR_BRIGHT);
}

void D1Dun::DrawAutomapDoorDiamond(int dir, int sx, int sy)
{
    unsigned d32 = AmLine32;
    unsigned d16 = (d32 >> 1), d8 = (d32 >> 2), d4 = (d32 >> 3), d2 = (d32 >> 4);

    /*switch (dir) {
    case 0: sx -= d8; sy -= d4; break; // NW
    case 1: sx += d8; sy -= d4; break; // NE
    case 2: sx -= d8; sy += d4; break; // SW
    case 3: sx += d8; sy += d4; break; // SE
    }*/
    if (dir == 0) { // WEST
        sx -= d8;
        sy -= d4;
    } else {        // EAST
        sx += d8;
        sy -= d4;
    }

    DrawLine(sx - d16 + 2, sy - d8, sx - 1, sy - d16 + 2, COLOR_BRIGHT); // top left
    DrawLine(sx, sy - d16 + 2, sx + d16 - 3, sy - d8, COLOR_BRIGHT);     // top right
    DrawLine(sx - d16 + 2, sy - d8 + 1, sx - 1, sy - 1, COLOR_BRIGHT);   // bottom left
    DrawLine(sx, sy - 1, sx + d16 - 3, sy - d8 + 1, COLOR_BRIGHT);       // bottom right
}

void D1Dun::DrawMap(int sx, int sy, uint8_t automap_type)
{
    switch (automap_type & MAT_TYPE) {
    case MAT_NONE:
        break;
    case MAT_EXTERN:
        D1Dun::DrawAutomapExtern(sx, sy);
        break;
    case MAT_STAIRS:
        D1Dun::DrawAutomapStairs(sx, sy);
        break;
    case MAT_DOOR_WEST:
        D1Dun::DrawAutomapDoorDiamond(0, sx, sy);
        break;
    case MAT_DOOR_EAST:
        D1Dun::DrawAutomapDoorDiamond(1, sx, sy);
        break;
    }

    unsigned d32 = AmLine32;
    unsigned d16 = (d32 >> 1);
    unsigned d8 = (d32 >> 2);
    if (automap_type & MAT_WALL_NW) {
        D1Dun::DrawLine(sx - d16, sy - d8, sx - 1, sy - d16 + 1, COLOR_DIM);
    }
    if (automap_type & MAT_WALL_NE) {
        D1Dun::DrawLine(sx, sy - d16 + 1, sx + d16 - 1, sy - d8, COLOR_DIM);
    }
    if (automap_type & MAT_WALL_SW) {
        D1Dun::DrawLine(sx - d16, sy - d8 + 1, sx - 1, sy, COLOR_DIM);
    }
    if (automap_type & MAT_WALL_SE) {
        D1Dun::DrawLine(sx, sy, sx + d16 - 1, sy - d8 + 1, COLOR_DIM);
    }
}

void D1Dun::drawMeta(QPainter &dungeon, unsigned cellWidth, int drawCursorX, int drawCursorY, int dunCursorX, int dunCursorY, const DunDrawParam &params)
{
    const unsigned backWidth = cellWidth;
    const unsigned backHeight = cellWidth / 2;

    static_assert(TILE_WIDTH == 2 && TILE_HEIGHT == 2, "D1Dun::drawMeta skips boundary checks.");
    switch (params.overlayType) {
    case DOT_MAP: {
        int subtileRef = this->subtiles[dunCursorY][dunCursorX];
        if (subtileRef > 0 && subtileRef <= this->min->getSubtileCount()) { // !0 || !UNDEF_SUBTILE
            quint8 mapType = this->sla->getMapType(subtileRef - 1);
            quint8 mapProp = this->sla->getMapProperties(subtileRef - 1);

            D1Dun::DrawMap(drawCursorX + backWidth / 2, drawCursorY - 1, mapType | mapProp);
        }
    } break;
    case DOT_ROOMS: {
        // draw the room meta info
        unsigned roomIndex = this->rooms[dunCursorY][dunCursorX];
        if (roomIndex != 0) {
            if (params.tileState == Qt::Unchecked) {
                QColor color = this->pal->getColor(roomIndex % D1PAL_COLORS);
                QImage *destImage = (QImage *)dungeon.device();
                D1Dun::DrawDiamond(*destImage, drawCursorX, drawCursorY - backHeight, backWidth, color);
            } else {
                QColor color = this->pal->getColor(((unsigned)(D1PAL_COLORS - 1 - roomIndex)) % D1PAL_COLORS);
                const QPen prevPen = dungeon.pen();
                dungeon.setPen(color);
                dungeon.drawRect(drawCursorX + backWidth / 4, drawCursorY - 3 * backHeight / 4, backWidth / 2, backHeight / 2);
                dungeon.setPen(prevPen);
            }
        }
    } break;
    case DOT_TILE_PROTECTIONS: {
        // draw X if the tile-flag is set
        Qt::CheckState tps = this->tileProtections[dunCursorY / TILE_HEIGHT][dunCursorX / TILE_WIDTH];
        if (tps == Qt::PartiallyChecked) {
            dungeon.drawLine(drawCursorX + backWidth / 4, drawCursorY - backHeight / 4, drawCursorX + 3 * backWidth / 4, drawCursorY - 3 * backHeight / 4);
            dungeon.drawLine(drawCursorX + backWidth / 4, drawCursorY - 3 * backHeight / 4, drawCursorX + 3 * backWidth / 4, drawCursorY - backHeight / 4);
        } else if (tps == Qt::Checked) {
            constexpr int SHIFT = 3;
            dungeon.drawLine(drawCursorX + backWidth / 4 - SHIFT, drawCursorY - backHeight / 4 - SHIFT, drawCursorX + 3 * backWidth / 4 - SHIFT, drawCursorY - 3 * backHeight / 4 - SHIFT);
            dungeon.drawLine(drawCursorX + backWidth / 4 + SHIFT, drawCursorY - backHeight / 4 + SHIFT, drawCursorX + 3 * backWidth / 4 + SHIFT, drawCursorY - 3 * backHeight / 4 + SHIFT);
            dungeon.drawLine(drawCursorX + backWidth / 4 - SHIFT, drawCursorY - 3 * backHeight / 4 + SHIFT, drawCursorX + 3 * backWidth / 4 - SHIFT, drawCursorY - backHeight / 4 + SHIFT);
            dungeon.drawLine(drawCursorX + backWidth / 4 + SHIFT, drawCursorY - 3 * backHeight / 4 - SHIFT, drawCursorX + 3 * backWidth / 4 + SHIFT, drawCursorY - backHeight / 4 - SHIFT);
        }
    } break;
    case DOT_SUBTILE_PROTECTIONS: {
        // draw X if the subtile-flag is set
        int sps = this->subtileProtections[dunCursorY][dunCursorX];
        if (sps & 1) {
            dungeon.drawLine(drawCursorX + backWidth / 4, drawCursorY - backHeight / 4, drawCursorX + 3 * backWidth / 4, drawCursorY - 3 * backHeight / 4);
        }
        if (sps & 2) {
            dungeon.drawLine(drawCursorX + backWidth / 4, drawCursorY - 3 * backHeight / 4, drawCursorX + 3 * backWidth / 4, drawCursorY - backHeight / 4);
        }
    } break;
    case DOT_TILE_NUMBERS: {
        static_assert(TILE_WIDTH == 2 && TILE_HEIGHT == 2, "D1Dun::drawMeta skips boundary checks.");
        if ((dunCursorX & 1) && (dunCursorY & 1)) {
            int tileRef = this->tiles[dunCursorY / TILE_HEIGHT][dunCursorX / TILE_WIDTH];
            // unsigned cellCenterX = drawCursorX + backWidth / 2;
            QString text = tileRef == UNDEF_TILE ? QString("???") : QString::number(tileRef);
            /*QFontMetrics fm(dungeon.font());
            unsigned textWidth = fm.horizontalAdvance(text);
            dungeon.drawText(cellCenterX - textWidth / 2, drawCursorY - backHeight + fm.height() / 2, text);*/
            QRect rect = QRect(drawCursorX, drawCursorY - 2 * backHeight, backWidth, 2 * backHeight);
            dungeon.drawText(rect, Qt::AlignCenter, text);
        }
    } break;
    case DOT_SUBTILE_NUMBERS: {
        // unsigned cellCenterX = drawCursorX + backWidth / 2;
        // unsigned cellCenterY = drawCursorY - backHeight / 2;
        int subtileRef = this->subtiles[dunCursorY][dunCursorX];
        QString text = subtileRef == UNDEF_SUBTILE ? QString("???") : QString::number(subtileRef);
        /*QFontMetrics fm(dungeon.font());
        unsigned textWidth = fm.horizontalAdvance(text);
        dungeon.drawText(cellCenterX - textWidth / 2, cellCenterY + fm.height() / 2, text);*/
        QRect rect = QRect(drawCursorX, drawCursorY - backHeight, backWidth, backHeight);
        dungeon.drawText(rect, Qt::AlignCenter, text);
    } break;
    }
}

void D1Dun::drawLayer(QPainter &dunPainter, const DunDrawParam &params, int layer)
{
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    unsigned cellWidth = subtileWidth;
    unsigned cellHeight = cellWidth / 2;

    int maxDunSize = this->width;
    int minDunSize = this->height;
    if (maxDunSize < minDunSize) {
        std::swap(maxDunSize, minDunSize);
    }
    // int drawCursorX = ((maxDunSize - 1) * cellWidth) / 2 - (this->width - this->height) * (cellWidth / 2);
    int drawCursorX = (maxDunSize - 1 - (this->width - this->height)) * cellWidth / 2;
    int drawCursorY = subtileHeight;
    int dunCursorX;
    int dunCursorY = 0;

    if (layer == 0) {
        this->drawBack(dunPainter, cellWidth, drawCursorX, drawCursorY, params);
        return;
    }

    // draw top triangle
    for (int i = 0; i < minDunSize; i++) {
        dunCursorX = 0;
        dunCursorY = i;
        while (dunCursorY >= 0) {
            if (layer == 1) {
                this->drawFloor(dunPainter, cellWidth, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
            } else if (layer == 2) {
                this->drawCell(dunPainter, cellWidth, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
            } else { // if (layer == 3) {
                this->drawMeta(dunPainter, cellWidth, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
            }
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
                if (layer == 1) {
                    this->drawFloor(dunPainter, cellWidth, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
                } else if (layer == 2) {
                    this->drawCell(dunPainter, cellWidth, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
                } else { // if (layer == 3) {
                    this->drawMeta(dunPainter, cellWidth, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
                }
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
                if (layer == 1) {
                    this->drawFloor(dunPainter, cellWidth, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
                } else if (layer == 2) {
                    this->drawCell(dunPainter, cellWidth, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
                } else { // if (layer == 3) {
                    this->drawMeta(dunPainter, cellWidth, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
                }
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
            if (layer == 1) {
                this->drawFloor(dunPainter, cellWidth, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
            } else if (layer == 2) {
                this->drawCell(dunPainter, cellWidth, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
            } else { // if (layer == 3) {
                this->drawMeta(dunPainter, cellWidth, drawCursorX, drawCursorY, dunCursorX, dunCursorY, params);
            }
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
}

QImage D1Dun::getImage(const DunDrawParam &params)
{
    int maxDunSize = std::max(this->width, this->height);

    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    unsigned cellWidth = subtileWidth;
    unsigned cellHeight = cellWidth / 2;

    QImage dungeon = QImage(maxDunSize * cellWidth,
        (maxDunSize - 1) * cellHeight + subtileHeight, QImage::Format_ARGB32_Premultiplied); // QImage::Format_ARGB32
    dungeon.fill(Qt::transparent);

    InitAutomapScale(subtileWidth);

    QPainter dunPainter(&dungeon);
    dunPainter.setPen(QColor(Config::getPaletteUndefinedColor()));

    DunPainter = &dunPainter;
    DunPal = this->pal;

    this->drawLayer(dunPainter, params, 0);
    this->drawLayer(dunPainter, params, 1);
    this->drawLayer(dunPainter, params, 2);
    if (params.overlayType != DOT_NONE) {
        this->drawLayer(dunPainter, params, 3);
    }

    // dunPainter.end();
    return dungeon;
}

void D1Dun::setPal(D1Pal *pal)
{
    this->pal = pal;
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
    for (auto &entry : this->missileCache) {
        D1Trn *misTrn = entry.misTrn;
        if (misTrn != nullptr) {
            misTrn->setPalette(pal);
            misTrn->refreshResultingPalette();
        } else {
            entry.misPal = pal;
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

bool D1Dun::hasContentAt(int x, int y) const
{
    bool hasContent = false;
    hasContent |= this->tiles[y / TILE_HEIGHT][x / TILE_WIDTH] > 0; // !0 && !UNDEF_TILE
    hasContent |= this->subtiles[y][x] > 0;                         // !0 && !UNDEF_SUBTILE
    hasContent |= this->items[y][x] != 0;
    hasContent |= this->tileProtections[y / TILE_HEIGHT][x / TILE_WIDTH] != Qt::Unchecked;
    hasContent |= this->subtileProtections[y][x] != 0;
    hasContent |= this->monsters[y][x].moType.monIndex != 0 || this->monsters[y][x].moType.monUnique;
    hasContent |= this->objects[y][x].oType != 0;
    hasContent |= this->rooms[y][x] != 0;
    hasContent |= this->missiles[y][x].miType != 0;
    return hasContent;
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
                hasContent |= this->hasContentAt(x, y);
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
    for (std::vector<Qt::CheckState> &tileProtectionsRow : this->tileProtections) {
        tileProtectionsRow.resize(newWidth / TILE_WIDTH);
    }
    for (std::vector<int> &subtileProtectionsRow : this->subtileProtections) {
        subtileProtectionsRow.resize(newWidth);
    }
    for (std::vector<int> &itemsRow : this->items) {
        itemsRow.resize(newWidth);
    }
    for (std::vector<MapMonster> &monsRow : this->monsters) {
        monsRow.resize(newWidth);
    }
    for (std::vector<MapObject> &objsRow : this->objects) {
        objsRow.resize(newWidth);
    }
    for (std::vector<int> &roomsRow : this->rooms) {
        roomsRow.resize(newWidth);
    }
    for (std::vector<MapMissile> &missilesRow : this->missiles) {
        missilesRow.resize(newWidth);
    }
    // update subtiles to match the defaultTile - TODO: better solution?
    int prevDefaultTile = this->defaultTile;
    this->defaultTile = UNDEF_TILE;
    this->setDefaultTile(prevDefaultTile);

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
                hasContent |= this->hasContentAt(x, y);
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
    this->tileProtections.resize(newHeight / TILE_HEIGHT);
    this->subtileProtections.resize(newHeight);
    this->items.resize(newHeight);
    this->monsters.resize(newHeight);
    this->objects.resize(newHeight);
    this->rooms.resize(newHeight);
    this->missiles.resize(newHeight);
    for (int y = prevHeight; y < newHeight; y++) {
        this->tiles[y / TILE_HEIGHT].resize(width / TILE_WIDTH);
        this->subtiles[y].resize(width);
        this->tileProtections[y / TILE_HEIGHT].resize(width / TILE_WIDTH);
        this->subtileProtections[y].resize(width);
        this->items[y].resize(width);
        this->monsters[y].resize(width);
        this->objects[y].resize(width);
        this->rooms[y].resize(width);
        this->missiles[y].resize(width);
    }
    // update subtiles to match the defaultTile - TODO: better solution?
    int prevDefaultTile = this->defaultTile;
    this->defaultTile = UNDEF_TILE;
    this->setDefaultTile(prevDefaultTile);

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

MapMonster D1Dun::getMonsterAt(int posx, int posy) const
{
    return this->monsters[posy][posx];
}

bool D1Dun::setMonsterAt(int posx, int posy, const MapMonster &mon)
{
    if (this->monsters[posy][posx] == mon) {
        return false;
    }
    this->monsters[posy][posx] = mon;
    this->modified = true;
    return true;
}

MapObject D1Dun::getObjectAt(int posx, int posy) const
{
    return this->objects[posy][posx];
}

bool D1Dun::setObjectAt(int posx, int posy, const MapObject &obj)
{
    if (this->objects[posy][posx] == obj) {
        return false;
    }
    this->objects[posy][posx] = obj;
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

MapMissile D1Dun::getMissileAt(int posx, int posy) const
{
    return this->missiles[posy][posx];
}

bool D1Dun::setMissileAt(int posx, int posy, const MapMissile &mis)
{
    if (this->missiles[posy][posx] == mis) {
        return false;
    }
    this->missiles[posy][posx] = mis;
    this->modified = true;
    return true;
}

Qt::CheckState D1Dun::getTileProtectionAt(int posx, int posy) const
{
    return this->tileProtections[posy / TILE_WIDTH][posx / TILE_HEIGHT];
}

bool D1Dun::setTileProtectionAt(int posx, int posy, Qt::CheckState protection)
{
    int tilePosX = posx / TILE_WIDTH;
    int tilePosY = posy / TILE_HEIGHT;
    if (this->tileProtections[tilePosY][tilePosX] == protection) {
        return false;
    }
    this->tileProtections[tilePosY][tilePosX] = protection;
    this->modified = true;
    return true;
}

bool D1Dun::getSubtileMonProtectionAt(int posx, int posy) const
{
    return (this->subtileProtections[posy][posx] & 1) != 0;
}

bool D1Dun::setSubtileMonProtectionAt(int posx, int posy, bool protection)
{
    if (((this->subtileProtections[posy][posx] & 1) != 0) == protection) {
        return false;
    }
    this->subtileProtections[posy][posx] = (this->subtileProtections[posy][posx] & ~1) | (protection ? 1 : 0);
    this->modified = true;
    return true;
}

bool D1Dun::getSubtileObjProtectionAt(int posx, int posy) const
{
    return (this->subtileProtections[posy][posx] & 2) != 0;
}

bool D1Dun::setSubtileObjProtectionAt(int posx, int posy, bool protection)
{
    if (((this->subtileProtections[posy][posx] & 2) != 0) == protection) {
        return false;
    }
    this->subtileProtections[posy][posx] = (this->subtileProtections[posy][posx] & ~2) | (protection ? 2 : 0);
    this->modified = true;
    return true;
}

void D1Dun::subtileInserted(unsigned subtileIndex)
{
    int subtileRef = subtileIndex + 1;
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            int currSubtileRef = this->subtiles[y][x];
            if (currSubtileRef >= subtileRef) {
                this->subtiles[y][x] = currSubtileRef + 1;
            } else {
                continue;
            }
            if (this->type == D1DUN_TYPE::RAW) {
                this->modified = true;
            }
        }
    }
}

void D1Dun::tileInserted(unsigned tileIndex)
{
    int tileRef = tileIndex + 1;
    for (int y = 0; y < this->height / TILE_HEIGHT; y++) {
        for (int x = 0; x < this->width / TILE_WIDTH; x++) {
            int currTileRef = this->tiles[y][x];
            if (currTileRef >= tileRef) {
                this->tiles[y][x] = currTileRef + 1;
            } else {
                continue;
            }
            if (this->type == D1DUN_TYPE::NORMAL) {
                this->modified = true;
            }
        }
    }
}

void D1Dun::subtileRemoved(unsigned subtileIndex)
{
    int subtileRef = subtileIndex + 1;
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            int currSubtileRef = this->subtiles[y][x];
            if (currSubtileRef == subtileRef) {
                dProgressWarn() << QApplication::tr("Subtile %1 was used at %2:%3.").arg(currSubtileRef).arg(x).arg(y);
                this->subtiles[y][x] = UNDEF_SUBTILE;
            } else if (currSubtileRef > subtileRef) {
                this->subtiles[y][x] = currSubtileRef - 1;
            } else {
                continue;
            }
            if (this->type == D1DUN_TYPE::RAW) {
                this->modified = true;
            }
        }
    }
}

void D1Dun::tileRemoved(unsigned tileIndex)
{
    int tileRef = tileIndex + 1;
    for (int y = 0; y < this->height / TILE_HEIGHT; y++) {
        for (int x = 0; x < this->width / TILE_WIDTH; x++) {
            int currTileRef = this->tiles[y][x];
            if (currTileRef == tileRef) {
                dProgressWarn() << QApplication::tr("Tile %1 was used at %2:%3.").arg(currTileRef).arg(x * TILE_WIDTH).arg(y * TILE_HEIGHT);
                this->setTileAt(x * TILE_WIDTH, y * TILE_HEIGHT, UNDEF_TILE);
            } else if (currTileRef > tileRef) {
                this->tiles[y][x] = currTileRef - 1;
            } else {
                continue;
            }
            if (this->type == D1DUN_TYPE::NORMAL) {
                this->modified = true;
            }
        }
    }
}

void D1Dun::subtilesSwapped(unsigned subtileIndex0, unsigned subtileIndex1)
{
    // TODO: keep in sync with D1Tileset::swapSubtiles
    const unsigned numSubtiles = this->tileset->sla->getSubtileCount();
    int subtileRef0, subtileRef1;
    if (subtileIndex0 >= numSubtiles) {
        // move subtileIndex1 to the front
        subtileRef0 = 1;
        subtileRef1 = subtileIndex1 + 1;
    } else if (subtileIndex1 >= numSubtiles) {
        // move subtileIndex0 to the end
        subtileRef0 = subtileIndex0 + 1;
        subtileRef1 = numSubtiles;
    } else {
        // swap subtileIndex0 and subtileIndex1
        subtileRef0 = subtileIndex0 + 1;
        subtileRef1 = subtileIndex1 + 1;
    }

    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            int subtileRef = this->subtiles[y][x];
            if (subtileRef == subtileRef0) {
                this->subtiles[y][x] = subtileRef1;
            } else if (subtileRef == subtileRef1) {
                this->subtiles[y][x] = subtileRef0;
            }
        }
    }
}

void D1Dun::tilesSwapped(unsigned tileIndex0, unsigned tileIndex1)
{
    // TODO: keep in sync with D1Tileset::swapTiles
    const unsigned numTiles = this->tileset->til->getTileCount();
    int tileRef0, tileRef1;
    if (tileIndex0 >= numTiles) {
        // move tileIndex1 to the front
        tileRef0 = 1;
        tileRef1 = tileIndex1 + 1;
    } else if (tileIndex1 >= numTiles) {
        // move tileIndex0 to the end
        tileRef0 = tileIndex0 + 1;
        tileRef1 = numTiles;
    } else {
        // swap tileIndex0 and tileIndex1
        tileRef0 = tileIndex0 + 1;
        tileRef1 = tileIndex1 + 1;
    }
    for (int y = 0; y < this->height / TILE_HEIGHT; y++) {
        for (int x = 0; x < this->width / TILE_WIDTH; x++) {
            int tileRef = this->tiles[y][x];
            if (tileRef == tileRef0) {
                this->tiles[y][x] = tileRef1;
            } else if (tileRef == tileRef1) {
                this->tiles[y][x] = tileRef0;
            }
        }
    }
}

void D1Dun::subtilesRemapped(const std::map<unsigned, unsigned> &remap)
{
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            int subtileRef = this->subtiles[y][x];
            if (subtileRef <= 0) { // 0 || UNDEF_SUBTILE
                continue;
            }
            unsigned subtileIndex = subtileRef - 1;
            auto it = remap.find(subtileIndex);
            if (it == remap.end() || it->second == subtileIndex) {
                continue;
            }
            this->subtiles[y][x] = it->second;
            if (this->type == D1DUN_TYPE::RAW) {
                this->modified = true;
            }
        }
    }
}

void D1Dun::tilesRemapped(const std::map<unsigned, unsigned> &remap)
{
    for (int y = 0; y < this->height / TILE_HEIGHT; y++) {
        for (int x = 0; x < this->width / TILE_WIDTH; x++) {
            int tileRef = this->tiles[y][x];
            if (tileRef <= 0) { // 0 || UNDEF_TILE
                continue;
            }
            unsigned tileIndex = tileRef - 1;
            auto it = remap.find(tileIndex);
            if (it == remap.end() || it->second == tileIndex) {
                continue;
            }
            this->tiles[y][x] = it->second;
            if (this->type == D1DUN_TYPE::NORMAL) {
                this->modified = true;
            }
        }
    }
}

bool D1Dun::swapPositions(int mode, int posx0, int posy0, int posx1, int posy1)
{
    if (posx0 < 0 || posx0 >= this->width) {
        return false;
    }
    if (posx1 < 0 || posx1 >= this->width) {
        return false;
    }
    if (posy0 < 0 || posy0 >= this->height) {
        return false;
    }
    if (posy1 < 0 || posy1 >= this->height) {
        return false;
    }
    if (mode == -1) {
        posx0 &= ~1;
        posy0 &= ~1;
        posx1 &= ~1;
        posy1 &= ~1;
    }
    if (posx0 == posx1 && posy0 == posy1) {
        return false;
    }
    bool change = false;
    if (mode == -1 || mode == BEM_TILE) {
        int tile0 = this->getTileAt(posx0, posy0);
        int tile1 = this->getTileAt(posx1, posy1);
        change |= this->setTileAt(posx0, posy0, tile1);
        change |= this->setTileAt(posx1, posy1, tile0);
    }
    if (mode == -1 || mode == BEM_TILE_PROTECTION) {
        Qt::CheckState tile0 = this->getTileProtectionAt(posx0, posy0);
        Qt::CheckState tile1 = this->getTileProtectionAt(posx1, posy1);
        change |= this->setTileProtectionAt(posx0, posy0, tile1);
        change |= this->setTileProtectionAt(posx1, posy1, tile0);
    }
    if (/*mode == -1 ||*/ mode == BEM_SUBTILE) {
        int subtile0 = this->getSubtileAt(posx0, posy0);
        int subtile1 = this->getSubtileAt(posx1, posy1);
        change |= this->setSubtileAt(posx0, posy0, subtile1);
        change |= this->setSubtileAt(posx1, posy1, subtile0);
    }
    if (mode == -1 || mode == BEM_SUBTILE_PROTECTION) {
        for (int dy = 0; dy < (mode == -1 ? TILE_HEIGHT : 1); dy++) {
            for (int dx = 0; dx < (mode == -1 ? TILE_WIDTH : 1); dx++) {
                bool objProtection0 = this->getSubtileObjProtectionAt(posx0 + dx, posy0 + dy);
                bool objProtection1 = this->getSubtileObjProtectionAt(posx1 + dx, posy1 + dy);
                change |= this->setSubtileObjProtectionAt(posx0 + dx, posy0 + dy, objProtection1);
                change |= this->setSubtileObjProtectionAt(posx1 + dx, posy1 + dy, objProtection0);
                bool monProtection0 = this->getSubtileMonProtectionAt(posx0 + dx, posy0 + dy);
                bool monProtection1 = this->getSubtileMonProtectionAt(posx1 + dx, posy1 + dy);
                change |= this->setSubtileMonProtectionAt(posx0 + dx, posy0 + dy, monProtection1);
                change |= this->setSubtileMonProtectionAt(posx1 + dx, posy1 + dy, monProtection0);
            }
        }
    }
    if (mode == -1 || mode == BEM_MONSTER) {
        for (int dy = 0; dy < (mode == -1 ? TILE_HEIGHT : 1); dy++) {
            for (int dx = 0; dx < (mode == -1 ? TILE_WIDTH : 1); dx++) {
                MapMonster mon0 = this->getMonsterAt(posx0 + dx, posy0 + dy);
                MapMonster mon1 = this->getMonsterAt(posx1 + dx, posy1 + dy);
                change |= this->setMonsterAt(posx0 + dx, posy0 + dy, mon1);
                change |= this->setMonsterAt(posx1 + dx, posy1 + dy, mon0);
            }
        }
    }
    if (mode == -1 || mode == BEM_OBJECT) {
        for (int dy = 0; dy < (mode == -1 ? TILE_HEIGHT : 1); dy++) {
            for (int dx = 0; dx < (mode == -1 ? TILE_WIDTH : 1); dx++) {
                MapObject obj0 = this->getObjectAt(posx0 + dx, posy0 + dy);
                MapObject obj1 = this->getObjectAt(posx1 + dx, posy1 + dy);
                change |= this->setObjectAt(posx0 + dx, posy0 + dy, obj1);
                change |= this->setObjectAt(posx1 + dx, posy1 + dy, obj0);
            }
        }
    }
    if (mode == -1) {
        for (int dy = 0; dy < (mode == -1 ? TILE_HEIGHT : 1); dy++) {
            for (int dx = 0; dx < (mode == -1 ? TILE_WIDTH : 1); dx++) {
                MapMissile mis0 = this->getMissileAt(posx0 + dx, posy0 + dy);
                MapMissile mis1 = this->getMissileAt(posx1 + dx, posy1 + dy);
                change |= this->setMissileAt(posx0 + dx, posy0 + dy, mis1);
                change |= this->setMissileAt(posx1 + dx, posy1 + dy, mis0);
            }
        }
    }
    if (mode == -1) {
        for (int dy = 0; dy < (mode == -1 ? TILE_HEIGHT : 1); dy++) {
            for (int dx = 0; dx < (mode == -1 ? TILE_WIDTH : 1); dx++) {
                int itemIndex0 = this->getItemAt(posx0 + dx, posy0 + dy);
                int itemIndex1 = this->getItemAt(posx1 + dx, posy1 + dy);
                change |= this->setItemAt(posx0 + dx, posy0 + dy, itemIndex1);
                change |= this->setItemAt(posx1 + dx, posy1 + dy, itemIndex0);
            }
        }
    }
    if (mode == -1) {
        for (int dy = 0; dy < (mode == -1 ? TILE_HEIGHT : 1); dy++) {
            for (int dx = 0; dx < (mode == -1 ? TILE_WIDTH : 1); dx++) {
                int roomIndex0 = this->getRoomAt(posx0 + dx, posy0 + dy);
                int roomIndex1 = this->getRoomAt(posx1 + dx, posy1 + dy);
                change |= this->setRoomAt(posx0 + dx, posy0 + dy, roomIndex1);
                change |= this->setRoomAt(posx1 + dx, posy1 + dy, roomIndex0);
            }
        }
    }
    return change;
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
    // keep defaultTile and levelType in sync
    if (dungeonTbl[this->levelType].defaultTile != defaultTile) {
        this->levelType = DTYPE_NONE;
    }
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

QString D1Dun::getMonsterName(const DunMonsterType &monType) const
{
    // check if it is a custom monster
    for (const CustomMonsterStruct &customMonster : customMonsterTypes) {
        if (customMonster.type == monType) {
            return customMonster.name;
        }
    }
    // check if it is built-in monster
    // - normal monster
    unsigned monBaseType = monType.monIndex;
    if (!monType.monUnique && monBaseType < (unsigned)lengthof(DunMonstConvTbl) && DunMonstConvTbl[monBaseType].name != nullptr) {
        return DunMonstConvTbl[monBaseType].name;
    }
    // - unique monster
    unsigned monUniqType = monType.monIndex - 1;
    if (monType.monUnique && monUniqType < (unsigned)lengthof(uniqMonData) && uniqMonData[monUniqType].mName != nullptr) {
        return uniqMonData[monUniqType].mName;
    }
    // out of options -> generic name
    QString result = monType.monUnique ? tr("UniqMonster%1") : tr("Monster%1");
    return result.arg(monType.monIndex);
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

QString D1Dun::getMissileName(int misIndex) const
{
    // check if it is a custom monster
    for (const CustomMissileStruct &customMissile : customMissileTypes) {
        if (customMissile.type == misIndex) {
            return customMissile.name;
        }
    }
    // check if it is built-in missile
    // - normal missile
    if ((unsigned)misIndex < (unsigned)lengthof(DunMissConvTbl) && DunMissConvTbl[misIndex].name != nullptr) {
        return DunMissConvTbl[misIndex].name;
    }
    // out of options -> generic name
    QString result = tr("Missile%1");
    return result.arg(misIndex);
}

void D1Dun::loadObjectGfx(const QString &filePath, ObjectCacheEntry &result)
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
    ObjectCacheEntry result = { objectIndex, nullptr, 0, 0 };
    unsigned i = 0;
    for ( ; i < this->customObjectTypes.size(); i++) {
        const CustomObjectStruct &customObject = this->customObjectTypes[i];
        if (customObject.type == objectIndex) {
            result.baseFrameNum = customObject.frameNum;
            result.animFrameNum = -1;
            QString celFilePath = customObject.path;
            this->loadObjectGfx(celFilePath, result);
            break;
        }
    }
    const BYTE *objType = &ObjConvTbl[objectIndex];
    if (i >= this->customObjectTypes.size() && (unsigned)objectIndex < (unsigned)lengthof(ObjConvTbl) && *objType != 0 && !this->assetPath.isEmpty()) {
        const ObjectData &od = objectdata[*objType];
        const ObjFileData &ofd = objfiledata[od.ofindex];
        result.baseFrameNum = od.oBaseFrame;
        if (result.baseFrameNum == 0 && ofd.oAnimFlag != OAM_LOOP) {
            result.baseFrameNum = 1;
        }
        result.animFrameNum = ofd.oAnimFlag == OAM_LOOP ? 0 : -1;
        QString celFilePath = this->assetPath + "/Objects/" + ofd.ofName + ".CEL";
        this->loadObjectGfx(celFilePath, result);
    }
    this->objectCache.push_back(result);
}

void D1Dun::loadMonsterGfx(const QString &filePath, const QString &baseTrnFilePath, const QString &uniqueTrnFilePath, MonsterCacheEntry &result)
{
    // check for existing entry
    unsigned i = 0;
    for ( ; i < this->monDataCache.size(); i++) {
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

void D1Dun::loadMonster(const MapMonster &mapMon)
{
    MonsterCacheEntry result = { mapMon.moType, nullptr, this->pal, nullptr, nullptr };
    // load a custom monster
    unsigned i = 0;
    for ( ; i < this->customMonsterTypes.size(); i++) {
        const CustomMonsterStruct &customMonster = this->customMonsterTypes[i];
        if (customMonster.type == mapMon.moType) {
            QString cl2FilePath = customMonster.path;
            this->loadMonsterGfx(cl2FilePath, customMonster.baseTrnPath, customMonster.uniqueTrnPath, result);
            break;
        }
    }
    if (i >= this->customMonsterTypes.size() && !this->assetPath.isEmpty()) {
        // load normal monster
        unsigned monBaseType = mapMon.moType.monIndex;
        if (!mapMon.moType.monUnique && monBaseType < (unsigned)lengthof(MonstConvTbl) && MonstConvTbl[monBaseType] != 0) {
            const MonsterData &md = monsterdata[MonstConvTbl[monBaseType]];
            QString cl2FilePath = monfiledata[md.moFileNum].moGfxFile;
            cl2FilePath.replace("%c", mapMon.moType.monDeadFlag ? "d" : "n");
            cl2FilePath = this->assetPath + "/" + cl2FilePath;
            QString baseTrnFilePath;
            if (md.mTransFile != nullptr) {
                baseTrnFilePath = this->assetPath + "/" + md.mTransFile;
            }
            QString uniqueTrnFilePath;
            this->loadMonsterGfx(cl2FilePath, baseTrnFilePath, uniqueTrnFilePath, result);
        }
        // load unique monster
        unsigned monUniqueType = mapMon.moType.monIndex - 1;
        if (mapMon.moType.monUnique && monUniqueType < (unsigned)lengthof(uniqMonData) && uniqMonData[monUniqueType].mtype != MT_INVALID) {
            const MonsterData &md = monsterdata[uniqMonData[monUniqueType].mtype];
            QString cl2FilePath = monfiledata[md.moFileNum].moGfxFile;
            cl2FilePath.replace("%c", mapMon.moType.monDeadFlag ? "d" : "n");
            cl2FilePath = this->assetPath + "/" + cl2FilePath;
            QString baseTrnFilePath;
            if (md.mTransFile != nullptr) {
                baseTrnFilePath = this->assetPath + "/" + md.mTransFile;
            }
            QString uniqueTrnFilePath;
            if (uniqMonData[monUniqueType].mTrnName != nullptr) {
                uniqueTrnFilePath = this->assetPath + "/Monsters/Monsters/" + uniqMonData[monUniqueType].mTrnName + ".TRN";
            }
            this->loadMonsterGfx(cl2FilePath, baseTrnFilePath, uniqueTrnFilePath, result);
        }
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

void D1Dun::loadMissileGfx(const QString &filePath, const QString &trnFilePath, MissileCacheEntry &result)
{
    // check for existing entry
    unsigned i = 0;
    for ( ; i < this->missileDataCache.size(); i++) {
        auto &dataEntry = this->missileDataCache[i];
        if (dataEntry.midPath == filePath) {
            dataEntry.numrefs++;
            memcpy(&result.misGfx[0], &dataEntry.midGfx[0], sizeof(dataEntry.midGfx));
            break;
        }
    }
    if (i >= this->missileDataCache.size()) {
        OpenAsParam params = OpenAsParam();
        MissileDataEntry mde;
        mde.numrefs = 1;
        mde.midPath = filePath;
        memset(&mde.midGfx[0], 0, sizeof(mde.midGfx));
        if (filePath.toLower().endsWith(".cl2")) {
            // create new entry
            D1Gfx *gfx = new D1Gfx();
            // gfx->setPalette(this->pal);
            D1Cl2::load(*gfx, filePath, params);
            if (gfx->getFrameCount() != 0) {
                mde.midGfx[0] = gfx;
                mde.numgfxs = 1;
            } else {
                // TODO: suppress errors?
                delete gfx;
                return;
            }
        } else {
            for (i = 0; i < (unsigned)lengthof(mde.midGfx); i++) {
                // create new entry
                D1Gfx *gfx = new D1Gfx();
                // gfx->setPalette(this->pal);
                QString fp = QString("%1%2.CL2").arg(filePath).arg(i + 1);
                D1Cl2::load(*gfx, fp, params);
                if (gfx->getFrameCount() != 0) {
                    mde.midGfx[i] = gfx;
                    mde.numgfxs++;
                } else {
                    // TODO: suppress errors?
                    delete gfx;
                    break;
                }
            }
            if (i == 0)
                return;
        }
        memcpy(&result.misGfx[0], &mde.midGfx[0], sizeof(mde.midGfx));
        this->missileDataCache.push_back(mde);
    }
    if (!trnFilePath.isEmpty()) {
        D1Trn *trn = new D1Trn();
        if (trn->load(trnFilePath, result.misPal)) {
            // set palette
            result.misPal = trn->getResultingPalette();
        } else {
            // TODO: suppress errors? MemFree?
            delete trn;
            trn = nullptr;
        }
        result.misTrn = trn;
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

void D1Dun::loadMissile(int misIndex)
{
    MissileCacheEntry result = { misIndex, { nullptr, nullptr, }, this->pal, nullptr };
    // load a custom monster
    unsigned i = 0;
    for ( ; i < this->customMissileTypes.size(); i++) {
        const CustomMissileStruct &customMissile = this->customMissileTypes[i];
        if (customMissile.type == misIndex) {
            QString cl2FilePath = customMissile.path;
            this->loadMissileGfx(cl2FilePath, customMissile.trnPath, result);
            break;
        }
    }
    if (i >= this->customMissileTypes.size() && !this->assetPath.isEmpty()) {
        // load normal missile
        if ((unsigned)misIndex < (unsigned)lengthof(DunMissConvTbl) && DunMissConvTbl[misIndex].name != nullptr) {
            const MissileData &md = missiledata[misIndex];
            const MisFileData &mfd = misfiledata[md.mFileNum];
            QString cl2FilePath = mfd.mfName;
            cl2FilePath = this->assetPath + "/Missiles/" + cl2FilePath;
            if (mfd.mfAnimFAmt == 1)
                cl2FilePath += ".CL2";
            QString trnFilePath;
            if (mfd.mfAnimTrans != nullptr) {
                trnFilePath = this->assetPath + "/" + mfd.mfAnimTrans;
            }
            this->loadMissileGfx(cl2FilePath, trnFilePath, result);
        }
    }
    this->missileCache.push_back(result);

}

void D1Dun::clearAssets()
{
    this->customObjectTypes.clear();
    this->customMonsterTypes.clear();
    this->customItemTypes.clear();
    this->customMissileTypes.clear();
    this->objectCache.clear();
    for (auto &entry : this->monsterCache) {
        delete entry.monBaseTrn;
        delete entry.monUniqueTrn;
    }
    this->monsterCache.clear();
    this->itemCache.clear();
    for (auto &entry : this->missileCache) {
        delete entry.misTrn;
    }
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
    for (auto &dataEntry : this->missileDataCache) {
        for (int i = 0; i < lengthof(dataEntry.midGfx); i++)
            delete dataEntry.midGfx[i];
    }
    this->missileDataCache.clear();
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

std::pair<int, int> D1Dun::collectSpace() const
{
    int spaceMonster = 0, spaceObject = 0;
    for (int posy = 0; posy < this->height; posy++) {
        for (int posx = 0; posx < this->width; posx++) {
            int subtileRef = this->subtiles[posy][posx];
            if (subtileRef <= 0) // UNDEF_SUBTILE || 0
                continue;
            quint8 solFlags = this->sla->getSubProperties(subtileRef - 1);
            if (solFlags & (1 << 0))
                continue; // subtile is non-passable
            if ((this->subtileProtections[posy][posx] & 1) == 0 && (this->monsters[posy][posx].moType.monIndex == 0 && !this->monsters[posy][posx].moType.monUnique)) {
                spaceMonster++;
            }
            if ((this->subtileProtections[posy][posx] & 2) == 0 && (this->monsters[posy][posx].moType.monIndex == 0 && !this->monsters[posy][posx].moType.monUnique) && this->objects[posy][posx].oType == 0) {
                spaceObject++;
            }
        }
    }
    return std::pair<int, int>(spaceMonster, spaceObject);
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

void D1Dun::collectMonsters(std::vector<std::pair<DunMonsterType, int>> &foundMonsters) const
{
    for (const std::vector<MapMonster> &monstersRow : this->monsters) {
        for (const MapMonster &mon : monstersRow) {
            const DunMonsterType &monType = mon.moType;
            if (monType.monIndex == 0 && !monType.monUnique)
                continue;
            bool added = false;
            for (std::pair<DunMonsterType, int> &monsterEntry : foundMonsters) {
                if (monsterEntry.first == monType) {
                    monsterEntry.second++;
                    added = true;
                    break;
                }
            }
            if (!added) {
                foundMonsters.push_back(std::pair<DunMonsterType, int>(monType, 1));
            }
        }
    }
}

void D1Dun::collectObjects(std::vector<std::pair<int, int>> &foundObjects) const
{
    for (const std::vector<MapObject> &objectsRow : this->objects) {
        for (const MapObject obj : objectsRow) {
            if (obj.oType == 0) {
                continue;
            }
            bool added = false;
            for (std::pair<int, int> &objectEntry : foundObjects) {
                if (objectEntry.first == obj.oType) {
                    objectEntry.second++;
                    added = true;
                }
            }
            if (!added) {
                foundObjects.push_back(std::pair<int, int>(obj.oType, 1));
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
                    dProgressWarn() << tr("Tile-value at %1:%2 is not supported by the game (Diablo 1/DevilutionX).").arg(x).arg(y);
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

void D1Dun::checkProtections() const
{
    ProgressDialog::incBar(tr("Checking Protections..."), 1);
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    progress.second = tr("Protection inconsistencies:");
    dProgress() << progress;
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            bool protection = (this->subtileProtections[y][x] & 1) != 0 && this->tileProtections[y / TILE_HEIGHT][x / TILE_WIDTH] != Qt::Unchecked;
            if (!protection) {
                if (this->objects[y][x].oType != 0) {
                    dProgressWarn() << tr("Subtile with an object is not protected at %1:%2.").arg(x).arg(y);
                    result = true;
                }
                if (this->items[y][x] != 0) {
                    dProgressWarn() << tr("Subtile with an item is not protected at %1:%2.").arg(x).arg(y);
                    result = true;
                }
            }
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected with the Protections.");
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

void D1Dun::checkItems() const
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
                quint8 solFlags = this->sla->getSubProperties(subtileRef - 1);
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

void D1Dun::checkMonsters() const
{
    ProgressDialog::incBar(tr("Checking Monsters..."), 1);
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    progress.second = tr("Monster inconsistencies:");
    dProgress() << progress;
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            const DunMonsterType &monType = this->monsters[y][x].moType;
            if (monType.monIndex == 0) {
                if (monType.monUnique) {
                    dProgressWarn() << tr("An unique monster is indicated at %1:%2, but its index is zero which is not supported by the game (Diablo 1/DevilutionX).").arg(x).arg(y);
                    result = true;
                }
                continue;
            }
            int subtileRef = this->subtiles[y][x];
            QString monsterName = this->getMonsterName(monType);
            if (subtileRef == UNDEF_SUBTILE) {
                dProgressWarn() << tr("'%1' at %2:%3 is on an undefined subtile.").arg(monsterName).arg(x).arg(y);
                result = true;
            } else if (subtileRef == 0) {
                dProgressWarn() << tr("'%1' at %2:%3 is on an empty subtile.").arg(monsterName).arg(x).arg(y);
                result = true;
            } else {
                quint8 solFlags = this->sla->getSubProperties(subtileRef - 1);
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
            int objectIndex = this->objects[y][x].oType;
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
            if (this->monsters[y][x].moType.monIndex != 0 || this->monsters[y][x].moType.monUnique) {
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

bool D1Dun::removeTiles()
{
    bool result = false;
    for (int y = 0; y < this->height; y += TILE_HEIGHT) {
        for (int x = 0; x < this->width; x += TILE_WIDTH) {
            result |= this->setTileAt(x, y, 0);
        }
    }
    return result;
}

bool D1Dun::removeProtections()
{
    bool result = false;
    for (std::vector<Qt::CheckState> &tileProtectionsRow : this->tileProtections) {
        for (Qt::CheckState &protection : tileProtectionsRow) {
            if (protection != Qt::Unchecked) {
                protection = Qt::Unchecked;
                result = true;
                this->modified = true;
            }
        }
    }
    for (std::vector<int> &subtileProtectionsRow : this->subtileProtections) {
        for (int &protection : subtileProtectionsRow) {
            if (protection != 0) {
                protection = 0;
                result = true;
                this->modified = true;
            }
        }
    }
    return result;
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
    for (std::vector<MapMonster> &monstersRow : this->monsters) {
        for (MapMonster &mon : monstersRow) {
            result |= D1Dun::setMapMonster(mon, 0, false, false);
        }
    }
    this->modified |= result;
    return result;
}

bool D1Dun::removeObjects()
{
    bool result = false;
    for (std::vector<MapObject> &objectsRow : this->objects) {
        for (MapObject &obj : objectsRow) {
            result |= D1Dun::setMapObject(obj, 0);
        }
    }
    this->modified |= result;
    return result;
}

bool D1Dun::removeMissiles()
{
    bool result = false;
    for (std::vector<MapMissile> &missilesRow : this->missiles) {
        for (MapMissile &mis : missilesRow) {
            result |= D1Dun::setMapMissile(mis, 0);
        }
    }
    this->modified |= result;
    return result;
}

static QString protectionString(Qt::CheckState protectionState)
{
    if (protectionState == Qt::Unchecked) {
        return QApplication::tr("No Protection");
    }
    if (protectionState == Qt::PartiallyChecked) {
        return QApplication::tr("Partial Protection");
    }
    return QApplication::tr("Complete Protection");
}

void D1Dun::loadTiles(const D1Dun *srcDun)
{
    if (this->height != srcDun->height || this->width != srcDun->width) {
        dProgressWarn() << tr("Size of the dungeon does not match. (%1x%2 vs. %3x%4)").arg(this->width).arg(this->height).arg(srcDun->width).arg(srcDun->height);
    }
    for (int y = 0; y < std::min(this->height, srcDun->height) / TILE_HEIGHT; y++) {
        for (int x = 0; x < std::min(this->width, srcDun->width) / TILE_WIDTH; x++) {
            int newTile = srcDun->tiles[y][x];
            int currTile = this->tiles[y][x];
            if (newTile != 0 && currTile != newTile) {
                int posx = x * TILE_WIDTH;
                int posy = y * TILE_HEIGHT;
                this->setTileAt(posx, posy, newTile);
                QString msg = tr("Tile '%1' at %2:%3 was replaced by '%4'.").arg(currTile).arg(posx).arg(posy).arg(newTile);
                Qt::CheckState currProtections = this->tileProtections[y][x];
                if (currTile == 0 || currProtections == Qt::Checked) {
                    dProgress() << msg;
                } else {
                    dProgressWarn() << msg;
                }
            }
        }
    }
}

void D1Dun::loadProtections(const D1Dun *srcDun)
{
    if (this->height != srcDun->height || this->width != srcDun->width) {
        dProgressWarn() << tr("Size of the dungeon does not match. (%1x%2 vs. %3x%4)").arg(this->width).arg(this->height).arg(srcDun->width).arg(srcDun->height);
    }
    for (int y = 0; y < std::min(this->height, srcDun->height) / TILE_HEIGHT; y++) {
        for (int x = 0; x < std::min(this->width, srcDun->width) / TILE_WIDTH; x++) {
            Qt::CheckState newProtections = srcDun->tileProtections[y][x];
            Qt::CheckState currProtections = this->tileProtections[y][x];
            if (newProtections != Qt::Unchecked && currProtections != newProtections) {
                if (currProtections != Qt::Unchecked) {
                    dProgressWarn() << tr("'%1' at %2:%3 was replaced by '%4'.").arg(protectionString(currProtections)).arg(x).arg(y).arg(protectionString(newProtections));
                }
                this->tileProtections[y][x] = newProtections;
                this->modified = true;
            }
        }
    }
    for (int y = 0; y < std::min(this->height, srcDun->height); y++) {
        for (int x = 0; x < std::min(this->width, srcDun->width); x++) {
            int newProtections = srcDun->subtileProtections[y][x];
            int currProtections = this->subtileProtections[y][x];
            if (newProtections && currProtections != newProtections) {
                this->subtileProtections[y][x] = newProtections;
                this->modified = true;
            }
        }
    }
}

void D1Dun::loadItems(const D1Dun *srcDun)
{
    if (this->height != srcDun->height || this->width != srcDun->width) {
        dProgressWarn() << tr("Size of the dungeon does not match. (%1x%2 vs. %3x%4)").arg(this->width).arg(this->height).arg(srcDun->width).arg(srcDun->height);
    }
    for (int y = 0; y < std::min(this->height, srcDun->height); y++) {
        for (int x = 0; x < std::min(this->width, srcDun->width); x++) {
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

void D1Dun::loadMonsters(const D1Dun *srcDun)
{
    if (this->height != srcDun->height || this->width != srcDun->width) {
        dProgressWarn() << tr("Size of the dungeon does not match. (%1x%2 vs. %3x%4)").arg(this->width).arg(this->height).arg(srcDun->width).arg(srcDun->height);
    }
    for (int y = 0; y < std::min(this->height, srcDun->height); y++) {
        for (int x = 0; x < std::min(this->width, srcDun->width); x++) {
            const DunMonsterType &newMonster = srcDun->monsters[y][x].moType;
            const DunMonsterType currMonster = this->monsters[y][x].moType;
            if ((newMonster.monIndex != 0 || newMonster.monUnique) && D1Dun::setMapMonster(this->monsters[y][x], newMonster.monIndex, newMonster.monUnique, newMonster.monDeadFlag)) {
                if (currMonster.monIndex != 0 || currMonster.monUnique) {
                    QString currMonsterName = this->getMonsterName(currMonster);
                    QString newMonsterName = this->getMonsterName(newMonster);
                    dProgressWarn() << tr("'%1'(%2) %3monster at %4:%5 was replaced by '%6'(%7)%8.").arg(currMonsterName).arg(currMonster.monIndex).arg(currMonster.monUnique ? "unique " : "").arg(x).arg(y).arg(newMonsterName).arg(newMonster.monIndex).arg(newMonster.monUnique ? " unique monster" : "");
                }
                this->modified = true;
            }
        }
    }
}

void D1Dun::loadObjects(const D1Dun *srcDun)
{
    if (this->height != srcDun->height || this->width != srcDun->width) {
        dProgressWarn() << tr("Size of the dungeon does not match. (%1x%2 vs. %3x%4)").arg(this->width).arg(this->height).arg(srcDun->width).arg(srcDun->height);
    }
    for (int y = 0; y < std::min(this->height, srcDun->height); y++) {
        for (int x = 0; x < std::min(this->width, srcDun->width); x++) {
            int newObjectIndex = srcDun->objects[y][x].oType;
            int currObjectIndex = this->objects[y][x].oType;
            if (newObjectIndex != 0 && D1Dun::setMapObject(this->objects[y][x], newObjectIndex)) {
                if (currObjectIndex != 0) {
                    QString currObjectName = this->getObjectName(currObjectIndex);
                    QString newObjectName = this->getObjectName(newObjectIndex);
                    dProgressWarn() << tr("'%1'(%2) object at %3:%4 was replaced by '%5'(%6).").arg(currObjectName).arg(currObjectIndex).arg(x).arg(y).arg(newObjectName).arg(newObjectIndex);
                }
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

/*
 * Update subtiles which have a valid corresponding tile. TODO: silent resetSubtiles?
 */
void D1Dun::refreshSubtiles()
{
    if (this->type == D1DUN_TYPE::RAW) {
        return;
    }
    for (int tilePosY = 0; tilePosY < this->height / TILE_HEIGHT; tilePosY++) {
        for (int tilePosX = 0; tilePosX < this->width / TILE_WIDTH; tilePosX++) {
            int tileRef = this->tiles[tilePosY][tilePosX];
            // this->updateSubtiles(tilePosX, tilePosY, tileRef);
            std::vector<int> subs = std::vector<int>(TILE_WIDTH * TILE_HEIGHT);
            if (tileRef == 0 && this->defaultTile != UNDEF_TILE) {
                tileRef = this->defaultTile;
            }
            if (tileRef <= 0 || this->til->getTileCount() < tileRef) {
                continue;
            }
            subs = this->til->getSubtileIndices(tileRef - 1);
            for (int &sub : subs) {
                sub += 1;
            }
            int posx = tilePosX * TILE_WIDTH;
            int posy = tilePosY * TILE_HEIGHT;
            for (int y = 0; y < TILE_HEIGHT; y++) {
                for (int x = 0; x < TILE_WIDTH; x++) {
                    int dunx = posx + x;
                    int duny = posy + y;
                    this->subtiles[duny][dunx] = subs[y * TILE_WIDTH + x];
                }
            }
        }
    }
}

bool D1Dun::maskTilesFrom(const D1Dun *srcDun)
{
    ProgressDialog::incBar(tr("Checking tiles..."), 1);
    bool result = false;
    if (this->height != srcDun->height || this->width != srcDun->width) {
        dProgressWarn() << tr("Size of the dungeon does not match. (%1x%2 vs. %3x%4)").arg(this->width).arg(this->height).arg(srcDun->width).arg(srcDun->height);
    }
    for (int tilePosY = 0; tilePosY < std::min(this->height, srcDun->height) / TILE_HEIGHT; tilePosY++) {
        for (int tilePosX = 0; tilePosX < std::min(this->width, srcDun->width) / TILE_WIDTH; tilePosX++) {
            if (this->tiles[tilePosY][tilePosX] != 0 && srcDun->tiles[tilePosY][tilePosX] == this->tiles[tilePosY][tilePosX]) {
                this->changeTileAt(tilePosX, tilePosY, 0);
                result = true;
            }
        }
    }
    if (!result) {
        dProgress() << tr("No change was necessary.");
    }

    ProgressDialog::decBar();
    return result;
}

bool D1Dun::needsProtectionAt(int posx, int posy) const
{
    if (this->objects[posy][posx].oType == 0 && (this->monsters[posy][posx].moType.monIndex == 0 || this->monsters[posy][posx].moType.monUnique)) {
        return false;
    }
    int subtileRef = this->subtiles[posy][posx];
    if ((subtileRef > 0) // UNDEF_SUBTILE || 0
        && (this->sla->getSubProperties(subtileRef - 1) & PSF_BLOCK_PATH)
        && (this->sla->getTrapProperty(subtileRef - 1) == PST_NONE)) {
        return false;
    }
    return true;
}

bool D1Dun::protectTiles()
{
    ProgressDialog::incBar(tr("Checking tiles..."), 1);
    bool result = false;
    for (int tilePosY = 0; tilePosY < this->height / TILE_HEIGHT; tilePosY++) {
        for (int tilePosX = 0; tilePosX < this->width / TILE_WIDTH; tilePosX++) {
            bool needsProtection = this->tiles[tilePosY][tilePosX] > 0; // !0 && !UNDEF_TILE
            for (int duny = tilePosY * TILE_HEIGHT; duny < (tilePosY + 1) * TILE_HEIGHT; duny++) {
                for (int dunx = tilePosX * TILE_WIDTH; duny < (tilePosX + 1) * TILE_WIDTH; dunx++) {
                    needsProtection |= this->needsProtectionAt(dunx, duny);
                }
            }
            int dunx = tilePosX * TILE_WIDTH;
            int duny = tilePosY * TILE_HEIGHT;
            if (needsProtection && this->setTileProtectionAt(dunx, duny, Qt::PartiallyChecked)) {
                dProgress() << tr("Tile at %1:%2 is now '%3'.").arg(dunx).arg(duny).arg(protectionString(Qt::PartiallyChecked));
                result = true;
            }
        }
    }
    if (!result) {
        dProgress() << tr("No change was necessary.");
    } else {
        if (this->type != D1DUN_TYPE::RAW) {
            this->modified = true;
        }
    }

    ProgressDialog::decBar();
    return result;
}

bool D1Dun::protectTilesFrom(const D1Dun *srcDun)
{
    ProgressDialog::incBar(tr("Checking tiles..."), 1);
    bool result = false;
    if (this->height != srcDun->height || this->width != srcDun->width) {
        dProgressWarn() << tr("Size of the dungeon does not match. (%1x%2 vs. %3x%4)").arg(this->width).arg(this->height).arg(srcDun->width).arg(srcDun->height);
    }
    for (int tilePosY = 0; tilePosY < std::min(this->height, srcDun->height) / TILE_HEIGHT; tilePosY++) {
        for (int tilePosX = 0; tilePosX < std::min(this->width, srcDun->width) / TILE_WIDTH; tilePosX++) {
            bool needsProtection = srcDun->tiles[tilePosY][tilePosX] > 0; // !0 && !UNDEF_TILE
            int dunx = tilePosX * TILE_WIDTH;
            int duny = tilePosY * TILE_HEIGHT;
            if (needsProtection && this->setTileProtectionAt(dunx, duny, Qt::Checked)) {
                dProgress() << tr("Tile at %1:%2 is now '%3'.").arg(dunx).arg(duny).arg(protectionString(Qt::Checked));
                result = true;
            }
        }
    }
    if (!result) {
        dProgress() << tr("No change was necessary.");
    } else {
        if (this->type != D1DUN_TYPE::RAW) {
            this->modified = true;
        }
    }

    ProgressDialog::decBar();
    return result;
}

bool D1Dun::protectSubtiles()
{
    ProgressDialog::incBar(tr("Checking subtiles..."), 1);
    bool result = false;
    for (int duny = 0; duny < this->height; duny++) {
        for (int dunx = 0; dunx < this->width; dunx++) {
            bool needsProtection = this->needsProtectionAt(dunx, duny);
            if (needsProtection && this->changeSubtileProtectionAt(dunx, duny, 3)) {
                dProgress() << tr("Subtile at %1:%2 is now protected.").arg(dunx).arg(duny);
                result = true;
            }
        }
    }
    if (!result) {
        dProgress() << tr("No change was necessary.");
    } else {
        if (this->type != D1DUN_TYPE::RAW) {
            this->modified = true;
        }
    }

    ProgressDialog::decBar();
    return result;
}

void D1Dun::insertTileRow(int posy)
{
    int tilePosY = posy / TILE_HEIGHT;

    // resize the dungeon
    this->tiles.insert(this->tiles.begin() + tilePosY, std::vector<int>(this->width / TILE_WIDTH));
    this->tileProtections.insert(this->tileProtections.begin() + tilePosY, std::vector<Qt::CheckState>(this->width / TILE_WIDTH));
    for (int i = 0; i < TILE_HEIGHT; i++) {
        this->subtileProtections.insert(this->subtileProtections.begin() + TILE_HEIGHT * tilePosY, std::vector<int>(this->width));
        this->subtiles.insert(this->subtiles.begin() + TILE_HEIGHT * tilePosY, std::vector<int>(this->width));
        this->items.insert(this->items.begin() + TILE_HEIGHT * tilePosY, std::vector<int>(this->width));
        this->monsters.insert(this->monsters.begin() + TILE_HEIGHT * tilePosY, std::vector<MapMonster>(this->width));
        this->objects.insert(this->objects.begin() + TILE_HEIGHT * tilePosY, std::vector<MapObject>(this->width));
        this->rooms.insert(this->rooms.begin() + TILE_HEIGHT * tilePosY, std::vector<int>(this->width));
        this->missiles.insert(this->missiles.begin() + TILE_HEIGHT * tilePosY, std::vector<MapMissile>(this->width));
    }
    // update subtiles to match the defaultTile - TODO: better solution?
    int prevDefaultTile = this->defaultTile;
    this->defaultTile = UNDEF_TILE;
    this->setDefaultTile(prevDefaultTile);

    this->height += TILE_HEIGHT;
    this->modified = true;
}

void D1Dun::insertTileColumn(int posx)
{
    int tilePosX = posx / TILE_WIDTH;

    // resize the dungeon
    for (std::vector<int> &tilesRow : this->tiles) {
        tilesRow.insert(tilesRow.begin() + tilePosX, 0);
    }
    for (std::vector<Qt::CheckState> &tileProtectionsRow : this->tileProtections) {
        tileProtectionsRow.insert(tileProtectionsRow.begin() + tilePosX, Qt::Unchecked);
    }
    for (int i = 0; i < TILE_WIDTH; i++) {
        for (std::vector<int> &subtilesRow : this->subtiles) {
            subtilesRow.insert(subtilesRow.begin() + TILE_WIDTH * tilePosX, 0);
        }
        for (std::vector<int> &subtileProtectionsRow : this->subtileProtections) {
            subtileProtectionsRow.insert(subtileProtectionsRow.begin() + TILE_WIDTH * tilePosX, 0);
        }
        for (std::vector<int> &itemsRow : this->items) {
            itemsRow.insert(itemsRow.begin() + TILE_WIDTH * tilePosX, 0);
        }
        for (std::vector<MapMonster> &monsRow : this->monsters) {
            monsRow.insert(monsRow.begin() + TILE_WIDTH * tilePosX, MapMonster());
        }
        for (std::vector<MapObject> &objsRow : this->objects) {
            objsRow.insert(objsRow.begin() + TILE_WIDTH * tilePosX, MapObject());
        }
        for (std::vector<int> &roomsRow : this->rooms) {
            roomsRow.insert(roomsRow.begin() + TILE_WIDTH * tilePosX, 0);
        }
        for (std::vector<MapMissile> &missilesRow : this->missiles) {
            missilesRow.insert(missilesRow.begin() + TILE_WIDTH * tilePosX, MapMissile());
        }
    }
    // update subtiles to match the defaultTile - TODO: better solution?
    int prevDefaultTile = this->defaultTile;
    this->defaultTile = UNDEF_TILE;
    this->setDefaultTile(prevDefaultTile);

    this->width += TILE_WIDTH;
    this->modified = true;
}

void D1Dun::removeTileRow(int posy)
{
    int tilePosY = posy / TILE_HEIGHT;

    // resize the dungeon
    this->tiles.erase(this->tiles.begin() + tilePosY);
    this->tileProtections.erase(this->tileProtections.begin() + tilePosY);
    for (int i = 0; i < TILE_HEIGHT; i++) {
        this->subtileProtections.erase(this->subtileProtections.begin() + TILE_WIDTH * tilePosY);
        this->subtiles.erase(this->subtiles.begin() + TILE_WIDTH * tilePosY);
        this->items.erase(this->items.begin() + TILE_WIDTH * tilePosY);
        this->monsters.erase(this->monsters.begin() + TILE_WIDTH * tilePosY);
        this->objects.erase(this->objects.begin() + TILE_WIDTH * tilePosY);
        this->rooms.erase(this->rooms.begin() + TILE_WIDTH * tilePosY);
        this->missiles.erase(this->missiles.begin() + TILE_WIDTH * tilePosY);
    }

    this->height -= TILE_HEIGHT;
    this->modified = true;
}

void D1Dun::removeTileColumn(int posx)
{
    int tilePosX = posx / TILE_WIDTH;

    // resize the dungeon
    for (std::vector<int> &tilesRow : this->tiles) {
        tilesRow.erase(tilesRow.begin() + tilePosX);
    }
    for (std::vector<Qt::CheckState> &tileProtectionsRow : this->tileProtections) {
        tileProtectionsRow.erase(tileProtectionsRow.begin() + tilePosX);
    }
    for (int i = 0; i < TILE_WIDTH; i++) {
        for (std::vector<int> &subtilesRow : this->subtiles) {
            subtilesRow.erase(subtilesRow.begin() + TILE_WIDTH * tilePosX);
        }
        for (std::vector<int> &subtileProtectionsRow : this->subtileProtections) {
            subtileProtectionsRow.erase(subtileProtectionsRow.begin() + TILE_WIDTH * tilePosX);
        }
        for (std::vector<int> &itemsRow : this->items) {
            itemsRow.erase(itemsRow.begin() + TILE_WIDTH * tilePosX);
        }
        for (std::vector<MapMonster> &monsRow : this->monsters) {
            monsRow.erase(monsRow.begin() + TILE_WIDTH * tilePosX);
        }
        for (std::vector<MapObject> &objsRow : this->objects) {
            objsRow.erase(objsRow.begin() + TILE_WIDTH * tilePosX);
        }
        for (std::vector<int> &roomsRow : this->rooms) {
            roomsRow.erase(roomsRow.begin() + TILE_WIDTH * tilePosX);
        }
        for (std::vector<MapMissile> &missilesRow : this->missiles) {
            missilesRow.erase(missilesRow.begin() + TILE_WIDTH * tilePosX);
        }
    }

    this->width -= TILE_WIDTH;
    this->modified = true;
}

bool D1Dun::setMapMonster(MapMonster &dstMon, int monsterIndex, bool isUnique, bool isDead)
{
    if (dstMon.moType.monIndex == monsterIndex && dstMon.moType.monUnique == isUnique) {
        return false;
    }
    dstMon.moType = { monsterIndex, isUnique, isDead };
    dstMon.moDir = DIR_S;
    dstMon.frameNum = 0;
    dstMon.mox = 0;
    dstMon.moy = 0;
    return true;
}

bool D1Dun::setMapObject(MapObject &dstObj, int objectIndex)
{
    if (dstObj.oType == objectIndex) {
        return false;
    }
    dstObj.oType = objectIndex;
    dstObj.baseFrameNum = 0;
    dstObj.animFrameNum = -1;
    dstObj.oPreFlag = false;
    return true;
}

bool D1Dun::setMapMissile(MapMissile &dstMis, int misType)
{
    if (dstMis.miType == misType) {
        return false;
    }
    dstMis.miType = misType;
    dstMis.miDir = DIR_S;
    dstMis.frameNum = 0;
    dstMis.mix = 0;
    dstMis.miy = 0;
    dstMis.miPreFlag = false;
    return true;
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
    MapObject &mapObj = this->objects[posy][posx];
    int prevObject = mapObj.oType;
    if (!D1Dun::setMapObject(mapObj, objectIndex)) {
        return false;
    }
    if (prevObject == objectIndex) {
        return false;
    }
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

bool D1Dun::changeMonsterAt(int posx, int posy, int monsterIndex, bool isUnique)
{
    MapMonster &mapMon = this->monsters[posy][posx];
    DunMonsterType prevMon = mapMon.moType;
    if (!D1Dun::setMapMonster(mapMon, monsterIndex, isUnique, false)) {
        return false;
    }
    if (monsterIndex == 0) {
        dProgress() << tr("Removed %1Monster '%2' from %3:%4.").arg(prevMon.monUnique ? "unique " : "").arg(prevMon.monIndex).arg(posx).arg(posy);
    } else if (prevMon.monIndex == 0) {
        dProgress() << tr("Added %1Monster '%2' to %3:%4.").arg(isUnique ? "unique " : "").arg(monsterIndex).arg(posx).arg(posy);
    } else {
        dProgress() << tr("Changed Monster at %1:%2 from '%3'%4 to '%5'%6.").arg(posx).arg(posy).arg(prevMon.monIndex).arg(prevMon.monUnique ? " unique monster" : "").arg(monsterIndex).arg(isUnique ? " unique monster" : "");
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

bool D1Dun::changeTileProtectionAt(int tilePosX, int tilePosY, Qt::CheckState protection)
{
    Qt::CheckState prevProtection = this->tileProtections[tilePosY][tilePosX];
    if (prevProtection == protection) {
        return false;
    }
    this->tileProtections[tilePosY][tilePosX] = protection;
    dProgress() << tr("'%1' at %2:%3 was replaced by '%4'.").arg(protectionString(prevProtection)).arg(tilePosX * TILE_WIDTH).arg(tilePosY * TILE_HEIGHT).arg(protectionString(protection));
    this->modified = true;
    return true;
}

bool D1Dun::changeSubtileProtectionAt(int posx, int posy, int protection)
{
    int prevProtection = this->subtileProtections[posy][posx];
    if (prevProtection == protection) {
        return false;
    }
    this->subtileProtections[posy][posx] = protection;
    if ((protection & 1) != (prevProtection & 1)) {
        if (protection & 1) {
            dProgress() << tr("Added Monster Protection to %1:%2.").arg(posx).arg(posy);
        } else {
            dProgress() << tr("Removed Monster Protection from %1:%2.").arg(posx).arg(posy);
        }
    }
    if ((protection & 2) != (prevProtection & 2)) {
        if (protection & 2) {
            dProgress() << tr("Added Object Protection to %1:%2.").arg(posx).arg(posy);
        } else {
            dProgress() << tr("Removed Object Protection from %1:%2.").arg(posx).arg(posy);
        }
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
/* DUN_FOULWATR*/            { 38, 74 }, // Foulwatr.DUN
/* DUN_VILE_PRE*/            { 42, 46 }, // Vile2.DUN
/* DUN_VILE_AFT*/            { 42, 46 }, // Vile1.DUN
/* DUN_WARLORD_PRE*/         { 16, 14 }, // Warlord2.DUN
/* DUN_WARLORD_AFT*/         { 16, 14 }, // Warlord.DUN
/* DUN_DIAB_1*/              { 12, 12 }, // Diab1.DUN
/* DUN_DIAB_2_PRE*/          { 22, 24 }, // Diab2a.DUN
/* DUN_DIAB_2_AFT*/          { 22, 24 }, // Diab2b.DUN
/* DUN_DIAB_3_PRE*/          { 22, 22 }, // Diab3a.DUN
/* DUN_DIAB_3_AFT*/          { 22, 22 }, // Diab3b.DUN
/* DUN_DIAB_4_PRE*/          { 18, 18 }, // Diab4a.DUN
/* DUN_DIAB_4_AFT*/          { 18, 18 }, // Diab4b.DUN
/* DUN_BETRAYER*/            { 14, 14 }, // Vile1.DUN
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
        // change |= this->changeTileAt(5, 3, 203 - 181);
        change |= this->changeTileAt(5, 4, 203 - 181);
        // patch set-piece to use common tiles and make the inner tile at the entrance non-walkable
        change |= this->changeTileAt(5, 2, 203 - 181);
        // let the game generate the shadow
        change |= this->changeTileAt(0, 5, 0);
        change |= this->changeTileAt(0, 6, 0);
        // useless tiles
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 7; x++) {
                if (this->tiles[y][x] == 13) {
                    change |= this->changeTileAt(x, y, 0);
                }
            }
        }
        // protect the main structure
        for (int y = 1; y < 7; y++) {
            for (int x = 1; x < 7; x++) {
                change |= this->changeTileProtectionAt(x, y, Qt::Checked);
            }
        }
        break;
    case DUN_BONECHAMB_ENTRY_PRE: // Bonestr1.DUN
        // useless tiles
        change |= this->changeTileAt(0, 0, 0);
        change |= this->changeTileAt(0, 4, 0);
        change |= this->changeTileAt(0, 5, 0);
        change |= this->changeTileAt(0, 6, 0);
        change |= this->changeTileAt(6, 6, 0);
        change |= this->changeTileAt(6, 0, 0);
        change |= this->changeTileAt(2, 3, 0);
        change |= this->changeTileAt(3, 3, 0);
        // + eliminate obsolete stair-tile
        change |= this->changeTileAt(2, 4, 0);
        // shadow of the external-left column
        change |= this->changeTileAt(0, 4, 48);
        change |= this->changeTileAt(0, 5, 50);
        // protect inner tiles from spawning additional monsters/objects
        for (int y = 1; y < 6; y++) {
            for (int x = 1; x < 6; x++) {
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 1, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 1, 3);
            }
        }
        break;
    case DUN_BONECHAMB_ENTRY_AFT: // Bonestr2.DUN
        // useless tiles
        change |= this->changeTileAt(0, 0, 0);
        change |= this->changeTileAt(0, 6, 0);
        change |= this->changeTileAt(6, 6, 0);
        change |= this->changeTileAt(6, 0, 0);
        // add the separate pillar tile
        change |= this->changeTileAt(5, 5, 52);
        // add tiles with subtiles for arches
        change |= this->changeTileAt(2, 1, 45);
        change |= this->changeTileAt(4, 1, 45);
        change |= this->changeTileAt(2, 5, 45);
        change |= this->changeTileAt(4, 5, 45);
        change |= this->changeTileAt(1, 2, 44);
        change |= this->changeTileAt(1, 4, 44);
        change |= this->changeTileAt(5, 2, 44);
        change |= this->changeTileAt(5, 4, 44);
        // - remove tile to leave space for shadow
        change |= this->changeTileAt(2, 4, 0);
        /*// place shadows
        // NE-wall
        change |= this->changeTileAt(1, 0, 49);
        change |= this->changeTileAt(2, 0, 49);
        change |= this->changeTileAt(3, 0, 49);
        change |= this->changeTileAt(4, 0, 49);
        // SW-wall
        // change |= this->changeTileAt(1, 4, 49);
        change |= this->changeTileAt(2, 4, 49);
        change |= this->changeTileAt(3, 4, 49);
        change |= this->changeTileAt(4, 4, 49);
        // NW-wall
        change |= this->changeTileAt(0, 0, 48);
        change |= this->changeTileAt(0, 1, 51);
        change |= this->changeTileAt(0, 2, 47);
        change |= this->changeTileAt(0, 3, 51);
        change |= this->changeTileAt(0, 4, 47);
        change |= this->changeTileAt(0, 5, 50);
        // SE-wall
        // change |= this->changeTileAt(4, 1, 100);
        change |= this->changeTileAt(4, 2, 47);
        change |= this->changeTileAt(4, 3, 51);
        change |= this->changeTileAt(4, 4, 46);*/
        // protect the main structure
        for (int y = 1; y < 6; y++) {
            for (int x = 1; x < 6; x++) {
                change |= this->changeTileProtectionAt(x, y, Qt::Checked);
            }
        }
        // adjust the number of layers
        this->numLayers = 1;
        break;
    case DUN_BONECHAMB_PRE: // Bonecha1.DUN
        // external tiles
        change |= this->changeTileAt(20, 4, 12);
        change |= this->changeTileAt(21, 4, 12);
        // useless tiles
        for (int y = 0; y < 18; y++) {
            for (int x = 0; x < 32; x++) {
                if (x >= 13 && x <= 21 && y >= 1 && y <= 4) {
                    continue;
                }
                if (x == 18 && y == 5) {
                    continue;
                }
                if (x == 14 && y == 5) {
                    continue;
                }
                change |= this->changeTileAt(x, y, 0);
            }
        }
        // prevent placement of torches in the central room
        for (int y = 9; y < 13; y++) {
            for (int x = 13; x < 17; x++) {
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 1, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 1, 3);
            }
        }
        // prevent placement of torches on the changing tiles
        change |= this->changeSubtileProtectionAt(28 + 0, 10 + 0, 3);
        change |= this->changeSubtileProtectionAt(28 + 1, 10 + 0, 3);
        change |= this->changeSubtileProtectionAt(28 + 0, 10 + 1, 3);
        change |= this->changeSubtileProtectionAt(28 + 1, 10 + 1, 3);
        change |= this->changeSubtileProtectionAt(36 + 0, 10 + 0, 3);
        change |= this->changeSubtileProtectionAt(36 + 1, 10 + 0, 3);
        change |= this->changeSubtileProtectionAt(36 + 0, 10 + 1, 3);
        change |= this->changeSubtileProtectionAt(36 + 1, 10 + 1, 3);
        break;
    case DUN_BONECHAMB_AFT: // Bonecha2.DUN
        // reduce pointless bone-chamber complexity
        change |= this->changeTileAt(16, 9, 57);
        change |= this->changeTileAt(16, 10, 62);
        change |= this->changeTileAt(16, 11, 62);
        change |= this->changeTileAt(16, 12, 62);
        change |= this->changeTileAt(13, 12, 53);
        change |= this->changeTileAt(14, 12, 62);
        change |= this->changeTileAt(15, 12, 62);
        // external tiles
        change |= this->changeTileAt(2, 15, 11);
        change |= this->changeTileAt(3, 15, 11);
        change |= this->changeTileAt(4, 15, 11);
        change |= this->changeTileAt(5, 15, 11);
        change |= this->changeTileAt(6, 15, 11);
        change |= this->changeTileAt(7, 15, 11);
        change |= this->changeTileAt(8, 15, 11);

        change |= this->changeTileAt(10, 17, 11);
        change |= this->changeTileAt(11, 17, 11);
        change |= this->changeTileAt(12, 17, 11);
        change |= this->changeTileAt(13, 17, 15);
        change |= this->changeTileAt(14, 17, 11);
        change |= this->changeTileAt(15, 17, 11);
        change |= this->changeTileAt(16, 17, 11);
        change |= this->changeTileAt(17, 17, 15);
        change |= this->changeTileAt(18, 17, 11);
        change |= this->changeTileAt(19, 17, 11);
        change |= this->changeTileAt(20, 17, 11);
        change |= this->changeTileAt(21, 17, 16);
        change |= this->changeTileAt(21, 16, 10);
        change |= this->changeTileAt(21, 15, 10);
        change |= this->changeTileAt(21, 14, 10);

        change |= this->changeTileAt(20, 0, 12);
        change |= this->changeTileAt(21, 0, 12);
        change |= this->changeTileAt(21, 1, 14);
        change |= this->changeTileAt(21, 2, 10);
        change |= this->changeTileAt(21, 3, 10);
        change |= this->changeTileAt(21, 4, 10);
        change |= this->changeTileAt(21, 5, 14);
        change |= this->changeTileAt(21, 6, 10);
        change |= this->changeTileAt(21, 7, 10);
        change |= this->changeTileAt(21, 8, 10);

        change |= this->changeTileAt(31, 8, 10);
        change |= this->changeTileAt(31, 9, 10);
        change |= this->changeTileAt(31, 10, 10);
        change |= this->changeTileAt(31, 11, 10);
        change |= this->changeTileAt(31, 12, 10);
        change |= this->changeTileAt(31, 13, 10);
        change |= this->changeTileAt(31, 14, 10);
        change |= this->changeTileAt(31, 15, 16);
        change |= this->changeTileAt(24, 15, 11);
        change |= this->changeTileAt(25, 15, 11);
        change |= this->changeTileAt(26, 15, 11);
        change |= this->changeTileAt(27, 15, 11);
        change |= this->changeTileAt(28, 15, 11);
        change |= this->changeTileAt(29, 15, 11);
        change |= this->changeTileAt(30, 15, 11);

        change |= this->changeTileAt(21, 13, 13);
        change |= this->changeTileAt(22, 13, 11);

        change |= this->changeTileAt(8, 15, 11);
        change |= this->changeTileAt(8, 16, 12);
        change |= this->changeTileAt(8, 17, 12);
        change |= this->changeTileAt(9, 17, 15);

        // add tiles with subtiles for arches
        change |= this->changeTileAt(13, 6, 44);
        change |= this->changeTileAt(13, 8, 44);
        change |= this->changeTileAt(17, 6, 44);
        change |= this->changeTileAt(17, 8, 96);

        change |= this->changeTileAt(13, 14, 44);
        change |= this->changeTileAt(13, 16, 44);
        change |= this->changeTileAt(17, 14, 44);
        change |= this->changeTileAt(17, 16, 44);

        change |= this->changeTileAt(18, 9, 45);
        change |= this->changeTileAt(20, 9, 45);
        change |= this->changeTileAt(18, 13, 45);
        change |= this->changeTileAt(20, 13, 45);
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
        // change |= this->changeTileAt(17, 8, 96);
        change |= this->changeTileAt(18, 8, 49);
        change |= this->changeTileAt(19, 8, 49);
        change |= this->changeTileAt(20, 8, 49);
        // - central room (bottom)
        change |= this->changeTileAt(18, 12, 46);
        // change |= this->changeTileAt(19, 12, 49); -- ugly with the candle
        // - left corridor
        change |= this->changeTileAt(12, 14, 47);
        change |= this->changeTileAt(12, 15, 51);
        change |= this->changeTileAt(16, 14, 47);
        change |= this->changeTileAt(16, 15, 51);
        // remove objects, monsters
        for (int y = 0; y < 18 * 2; y++) {
            for (int x = 0; x < 32 * 2; x++) {
                change |= this->changeObjectAt(x, y, 0);
                change |= this->changeMonsterAt(x, y, 0, false);
            }
        }
        // adjust the number of layers
        this->numLayers = 1;
        break;
    case DUN_BLIND_PRE: // Blind2.DUN
        // external tiles
        change |= this->changeTileAt(2, 2, 13);
        change |= this->changeTileAt(2, 3, 10);
        change |= this->changeTileAt(3, 2, 11);
        change |= this->changeTileAt(3, 3, 12);

        change |= this->changeTileAt(6, 6, 13);
        change |= this->changeTileAt(6, 7, 10);
        change |= this->changeTileAt(7, 6, 11);
        change |= this->changeTileAt(7, 7, 12);
        // useless tiles
        for (int y = 0; y < 11; y++) {
            for (int x = 0; x < 11; x++) {
                // keep the boxes
                if (x >= 2 && y >= 2 && x < 4 && y < 4) {
                    continue;
                }
                if (x >= 6 && y >= 6 && x < 8 && y < 8) {
                    continue;
                }
                // keep the doors
                if (x == 0 && y == 1/* || x == 4 && y == 3*/ || x == 10 && y == 8) {
                    continue;
                }
                change |= this->changeTileAt(x, y, 0);
            }
        }
        // replace the door with wall
        change |= this->changeTileAt(4, 3, 25);
        // remove items
        //change |= this->changeItemAt(5, 5, 0);
        // remove obsolete 'protection' (item)
        change |= this->changeTileProtectionAt(5, 10, Qt::Unchecked);
        // protect inner tiles from spawning additional monsters/objects
        for (int y = 0; y < 6; y++) {
            for (int x = 0; x < 6; x++) {
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 1, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 1, 3);
            }
        }
        for (int y = 4; y < 11; y++) {
            for (int x = 4; x < 11; x++) {
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 1, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 1, 3);
            }
        }
        break;
    case DUN_BLIND_AFT: // Blind1.DUN
        // place pieces with closed doors
        change |= this->changeTileAt(4, 3, 150);
        change |= this->changeTileAt(6, 7, 150);
        /*// add monsters from Blind2.DUN
        change |= this->changeMonsterAt(1, 6, 32, false);
        change |= this->changeMonsterAt(4, 1, 32, false);
        change |= this->changeMonsterAt(6, 3, 32, false);
        change |= this->changeMonsterAt(6, 9, 32, false);
        change |= this->changeMonsterAt(5, 6, 32, false);
        change |= this->changeMonsterAt(7, 5, 32, false);
        change |= this->changeMonsterAt(7, 7, 32, false);
        change |= this->changeMonsterAt(13, 14, 32, false);
        change |= this->changeMonsterAt(14, 17, 32, false);
        change |= this->changeMonsterAt(14, 11, 32, false);
        change |= this->changeMonsterAt(15, 13, 32, false);*/
        // remove obsolete 'protection' (item)
        // change |= this->changeTileProtectionAt(5, 10, Qt::Unchecked);
        // protect the main structure
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 7; x++) {
                change |= this->changeTileProtectionAt(x, y, Qt::Checked);
            }
        }
        for (int y = 4; y < 11; y++) {
            for (int x = 4; x < 11; x++) {
                change |= this->changeTileProtectionAt(x, y, Qt::Checked);
            }
        }
        // remove monsters
        for (int y = 0; y < 11 * 2; y++) {
            for (int x = 0; x < 11 * 2; x++) {
                change |= this->changeMonsterAt(x, y, 0, false);
            }
        }
        // adjust the number of layers
        this->numLayers = 1;
        break;
    case DUN_BLOOD_PRE: // Blood2.DUN
        // external tiles
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 10; x++) {
                if (this->tiles[y][x] >= 143 && this->tiles[y][x] <= 149) {
                    change |= this->changeTileAt(x, y, this->tiles[y][x] - 133);
                }
            }
        }
        // useless tiles
        for (int y = 8; y < 16; y++) {
            for (int x = 0; x < 10; x++) {
                if (x != 5 || y != 8)
                    change |= this->changeTileAt(x, y, 0);
            }
        }
        // place pieces with closed doors
        // change |= this->changeTileAt(4, 10, 151);
        // change |= this->changeTileAt(4, 15, 151);
        // change |= this->changeTileAt(5, 15, 151);
        // shadow of the external-left column -- do not place to prevent overwriting large decorations
        // this->tiles[7][-1] = 48;
        // this->tiles[8][-1] = 50;
        // shadow of the bottom-left column(s) -- one is missing
        // change |= this->changeTileAt(1, 13, 48);
        // change |= this->changeTileAt(1, 14, 50);
        // shadow of the internal column next to the pedistal
        change |= this->changeTileAt(5, 7, 142);
        change |= this->changeTileAt(5, 8, 50);
        // remove items
        // change |= this->changeItemAt(9, 2, 0);
        // adjust objects
        // - book and the pedistal
        change |= this->changeObjectAt(9, 24, 15);
        change |= this->changeObjectAt(9, 16, 91);
        // - remove torches
        change |= this->changeObjectAt(11, 8, 0);
        change |= this->changeObjectAt(11, 10, 0);
        change |= this->changeObjectAt(11, 12, 0);
        change |= this->changeObjectAt(6, 8, 0);
        change |= this->changeObjectAt(6, 10, 0);
        change |= this->changeObjectAt(6, 12, 0);
        // protect inner tiles from spawning additional monsters/objects
        for (int y = 7; y < 15; y++) {
            for (int x = 2; x <= 6; x++) {
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 1, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 1, 3);
            }
        }
        change |= this->changeSubtileProtectionAt(2 * 2 + 1, 2 * 3 + 0, 3);
        change |= this->changeSubtileProtectionAt(2 * 3 + 0, 2 * 3 + 0, 3);
        change |= this->changeSubtileProtectionAt(2 * 3 + 0, 2 * 3 + 1, 3);
        change |= this->changeSubtileProtectionAt(2 * 6 + 0, 2 * 3 + 0, 3);
        change |= this->changeSubtileProtectionAt(2 * 6 + 1, 2 * 3 + 0, 3);
        change |= this->changeSubtileProtectionAt(2 * 6 + 0, 2 * 3 + 1, 3);
        for (int y = 4; y < 7; y++) {
            change |= this->changeSubtileProtectionAt(2 * 3 + 0, 2 * y + 0, 3);
            change |= this->changeSubtileProtectionAt(2 * 3 + 0, 2 * y + 1, 3);
            change |= this->changeSubtileProtectionAt(2 * 6 + 0, 2 * y + 0, 3);
            change |= this->changeSubtileProtectionAt(2 * 6 + 0, 2 * y + 1, 3);
        }
        break;
    case DUN_BLOOD_AFT: // Blood1.DUN
        // eliminate invisible 'fancy' tile to leave space for shadow
        change |= this->changeTileAt(3,  9, 0);
        // place pieces with closed doors
        change |= this->changeTileAt(4, 10, 151);
        change |= this->changeTileAt(4, 15, 151);
        change |= this->changeTileAt(5, 15, 151);
        /*// replace torches
        change |= this->changeObjectAt(11, 8, 110);
        change |= this->changeObjectAt(11, 10, 110);
        change |= this->changeObjectAt(11, 12, 110);
        change |= this->changeObjectAt(6, 8, 111);
        change |= this->changeObjectAt(7, 8, 0);
        change |= this->changeObjectAt(6, 10, 111);
        change |= this->changeObjectAt(7, 10, 0);
        change |= this->changeObjectAt(6, 12, 111);
        change |= this->changeObjectAt(7, 12, 0);
        // move book
        change |= this->changeObjectAt(8, 24, 0);
        change |= this->changeObjectAt(9, 24, 15);
        // replace monsters
        // - corridor
        change |= this->changeMonsterAt(8, 3, 62, false);
        change |= this->changeMonsterAt(9, 3, 62, false);
        change |= this->changeMonsterAt(10, 3, 62, false);
        change |= this->changeMonsterAt(5, 4, 62, false);
        change |= this->changeMonsterAt(12, 4, 62, false);
        // - left room
        change |= this->changeMonsterAt(3, 8, 62, false);
        change |= this->changeMonsterAt(3, 12, 62, false);
        // - right room
        change |= this->changeMonsterAt(14, 8, 62, false);
        change |= this->changeMonsterAt(14, 12, 62, false);
        // - front room
        change |= this->changeMonsterAt(9, 22, 62, false);
        change |= this->changeMonsterAt(6, 25, 62, false);
        change |= this->changeMonsterAt(12, 25, 62, false);
        // remove items
        change |= this->changeItemAt(9, 2, 0);*/
        // protect the main structure
        for (int y = 0; y <= 15; y++) {
            for (int x = 2; x <= 7; x++) {
                change |= this->changeTileProtectionAt(x, y, Qt::Checked);
            }
        }
        for (int y = 3; y <= 8; y++) {
            for (int x = 0; x <= 9; x++) {
                change |= this->changeTileProtectionAt(x, y, Qt::Checked);
            }
        }
        // remove objects, monsters, items
        for (int y = 0; y < 16 * 2; y++) {
            for (int x = 0; x < 10 * 2; x++) {
                change |= this->changeObjectAt(x, y, 0);
                change |= this->changeMonsterAt(x, y, 0, false);
                // change |= this->changeItemAt(x, y, 0);
            }
        }
        // adjust the number of layers
        this->numLayers = 1;
        break;
    case DUN_FOULWATR: // Foulwatr.DUN
        // - separate subtiles for the automap
        change |= this->changeTileAt(6, 33, 111);
        // protect island tiles from spawning additional monsters
        for (int y = 1; y < 7; y++) {
            for (int x = 7; x < 14; x++) {
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 1, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 1, 3);
            }
        }
        // remove most of the monsters
        for (int y = 13; y < 61; y++) {
            for (int x = 4; x < 30; x++) {
                if (x == 6 && y == 33) {
                    continue;
                }
                change |= this->changeMonsterAt(x, y, 0, false);
            }
        }
        break;
    case DUN_VILE_PRE: // Vile2.DUN
        // useless tiles
        for (int y = 0; y < 23; y++) {
            for (int x = 0; x < 21; x++) {
                // room on the left side
                if (x >= 4 && y >= 5 && x <= 6 && y <= 7) {
                    continue;
                }
                if (x >= 4 && y >= 8 && x <= 7 && y <= 10) {
                    continue;
                }
                // room on the right side
                if (x == 12 && y == 5) {
                    continue;
                }
                if (x >= 13 && y >= 5 && x <= 15 && y <= 7) {
                    continue;
                }
                if (x >= 12 && y >= 8 && x <= 14 && y <= 10) {
                    continue;
                }
                // main room
                if (x >= 7 && y >= 13 && x <= 13 && y <= 17) {
                    continue;
                }
                if (x >= 7 && y >= 18 && x <= 12 && y <= 18) {
                    continue;
                }
                if (x >= 8 && y >= 20 && x <= 11 && y <= 22) {
                    continue;
                }
                change |= this->changeTileAt(x, y, 0);
            }
        }
        // replace default tiles with external piece I.
        // - central room
        change |= this->changeTileAt(8, 16, 203 - 181);
        // use the new shadows
        change |= this->changeTileAt(14, 5, 152);
        // add the unique monsters
        change |= this->changeMonsterAt(16, 30, UMT_LAZARUS + 1, true);
        change |= this->changeMonsterAt(24, 29, UMT_RED_VEX + 1, true);
        change |= this->changeMonsterAt(22, 33, UMT_BLACKJADE + 1, true);
        // replace books
        change |= this->changeObjectAt(10, 29, 47);
        change |= this->changeObjectAt(29, 30, 47);
        break;
    case DUN_VILE_AFT: // Vile1.DUN
        // external tiles
        for (int y = 0; y < 23; y++) {
            for (int x = 0; x < 21; x++) {
                if (this->tiles[y][x] >= 181 + 18 && this->tiles[y][x] <= 181 + 24) {
                    change |= this->changeTileAt(x, y, this->tiles[y][x] - 181);
                }
            }
        }
        // replace default tiles with external piece
        // - SW in the middle
        change |= this->changeTileAt(12, 22, 203 - 181);
        change |= this->changeTileAt(13, 22, 203 - 181);
        change |= this->changeTileAt(14, 22, 203 - 181);
        // - SE
        for (int i = 1; i < 23; i++) {
            change |= this->changeTileAt(20, i, 203 - 181);
        }
        // use common tiles
        change |= this->changeTileAt(11, 3, 18);
        // use the new shadows
        // - hallway (left)
        change |= this->changeTileAt(3, 4, 52);
        change |= this->changeTileAt(4, 4, 51);
        change |= this->changeTileAt(5, 4, 51);
        change |= this->changeTileAt(3, 5, 139);
        change |= this->changeTileAt(3, 6, 139);
        change |= this->changeTileAt(3, 7, 165);
        // - room (left)
        change |= this->changeTileAt(4, 7, 48);
        change |= this->changeTileAt(5, 7, 164);
        change |= this->changeTileAt(4, 8, 27);
        change |= this->changeTileAt(5, 8, 49);
        // - hallway (right)
        change |= this->changeTileAt(11, 4, 46);
        change |= this->changeTileAt(12, 4, 51);
        change |= this->changeTileAt(13, 4, 51);
        change |= this->changeTileAt(14, 4, 47);
        // - room (right)
        change |= this->changeTileAt(14, 5, 149);
        change |= this->changeTileAt(14, 6, 139);
        change |= this->changeTileAt(14, 7, 139);
        change |= this->changeTileAt(12, 7, 50);
        change |= this->changeTileAt(13, 7, 164);
        change |= this->changeTileAt(13, 8, 49);
        // - hallway (center)
        change |= this->changeTileAt(9, 4, 140);
        // - corners
        change |= this->changeTileAt(4, 17, 159);
        change |= this->changeTileAt(16, 10, 159);
        // remove objects, monsters
        for (int y = 0; y < 23 * 2; y++) {
            for (int x = 0; x < 21 * 2; x++) {
                change |= this->changeObjectAt(x, y, 0);
                change |= this->changeMonsterAt(x, y, 0, false);
            }
        }
        // ensure the changing tiles are reserved
        change |= this->changeTileProtectionAt( 4,  5, Qt::Checked);
        change |= this->changeTileProtectionAt( 4,  6, Qt::Checked);
        change |= this->changeTileProtectionAt( 4,  7, Qt::Checked);
        change |= this->changeTileProtectionAt( 5,  5, Qt::Checked);
        change |= this->changeTileProtectionAt( 6,  5, Qt::Checked);
        change |= this->changeTileProtectionAt(12,  5, Qt::Checked);
        change |= this->changeTileProtectionAt(13,  5, Qt::Checked);
        change |= this->changeTileProtectionAt(14,  5, Qt::Checked);
        change |= this->changeTileProtectionAt(15,  5, Qt::Checked);
        change |= this->changeTileProtectionAt(15,  6, Qt::Checked);
        change |= this->changeTileProtectionAt(15,  7, Qt::Checked);
        change |= this->changeTileProtectionAt( 7, 18, Qt::Checked);
        change |= this->changeTileProtectionAt( 8, 18, Qt::Checked);
        change |= this->changeTileProtectionAt( 9, 18, Qt::Checked);
        change |= this->changeTileProtectionAt(10, 18, Qt::Checked);
        change |= this->changeTileProtectionAt(11, 18, Qt::Checked);
        change |= this->changeTileProtectionAt(12, 18, Qt::Checked);
        change |= this->changeTileProtectionAt(14, 15, Qt::Checked);
        // adjust the number of layers
        this->numLayers = 1;
        break;
    case DUN_WARLORD_PRE: // Warlord2.DUN
        // useless tiles
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 8; x++) {
                if (x >= 7 && y >= 1 && x <= 7 && y <= 5) {
                    continue;
                }
                change |= this->changeTileAt(x, y, 0);
            }
        }
        // replace monsters from Warlord.DUN
        change |= this->changeMonsterAt(2, 2, 100, false);
        change |= this->changeMonsterAt(2, 10, 100, false);
        change |= this->changeMonsterAt(13, 4, 100, false);
        change |= this->changeMonsterAt(13, 9, 100, false);
        change |= this->changeMonsterAt(10, 2, 100, false);
        change |= this->changeMonsterAt(10, 10, 100, false);
        // add monsters from Warlord.DUN
        change |= this->changeMonsterAt(6, 2, 100, false);
        change |= this->changeMonsterAt(6, 10, 100, false);
        change |= this->changeMonsterAt(11, 2, 100, false);
        change |= this->changeMonsterAt(11, 10, 100, false);
        // - add the Warlord
        change |= this->changeMonsterAt(6, 7, UMT_WARLORD + 1, true);
        // add objects from Warlord.DUN
        change |= this->changeObjectAt(2, 3, 108);
        change |= this->changeObjectAt(2, 9, 108);
        change |= this->changeObjectAt(5, 2, 109);
        change |= this->changeObjectAt(8, 2, 109);
        change |= this->changeObjectAt(5, 10, 109);
        change |= this->changeObjectAt(8, 10, 109);
        // remove items
        change |= this->changeItemAt(2, 3, 0);
        change |= this->changeItemAt(2, 9, 0);
        change |= this->changeItemAt(5, 2, 0);
        change |= this->changeItemAt(8, 2, 0);
        change |= this->changeItemAt(5, 10, 0);
        change |= this->changeItemAt(8, 10, 0);
        // protect inner tiles from spawning additional monsters/objects
        for (int y = 0; y <= 5; y++) {
            for (int x = 0; x <= 7; x++) {
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 1, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 1, 3);
            }
        }
        break;
    case DUN_WARLORD_AFT: // Warlord.DUN
        // fix corner
        change |= this->changeTileAt(6, 1, 10);
        change |= this->changeTileAt(6, 5, 10);
        // separate subtiles for the automap
        change |= this->changeTileAt(1, 2, 136);
        // use base tiles and decorate the walls randomly
        change |= this->changeTileAt(0, 0, 9);
        change |= this->changeTileAt(0, 6, 15);
        change |= this->changeTileAt(1, 0, 2);
        change |= this->changeTileAt(2, 0, 2);
        change |= this->changeTileAt(3, 0, 2);
        change |= this->changeTileAt(4, 0, 2);
        change |= this->changeTileAt(5, 0, 2);
        change |= this->changeTileAt(1, 6, 2);
        change |= this->changeTileAt(2, 6, 2);
        change |= this->changeTileAt(3, 6, 2);
        change |= this->changeTileAt(4, 6, 2);
        change |= this->changeTileAt(5, 6, 2);
        // change |= this->changeTileAt(6, 3, 50);
        // ensure the changing tiles are protected + protect inner tiles from decoration
        for (int y = 1; y <= 5; y++) {
            for (int x = 1; x <= 7; x++) {
                change |= this->changeTileProtectionAt(x, y, Qt::Checked);
            }
        }
        // remove monsters, objects
        for (int y = 0; y < 14; y++) {
            for (int x = 0; x < 16; x++) {
                change |= this->changeMonsterAt(x, y, 0, false);
                change |= this->changeObjectAt(x, y, 0);
            }
        }
        // adjust the number of layers
        this->numLayers = 1;
        break;
    case DUN_BANNER_PRE: // Banner2.DUN
        // useless tiles
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (x >= 3 && y >= 3 && x <= 3 && y <= 6) {
                    continue;
                }
                if (x >= 3 && y >= 5 && x <= 6 && y <= 5) {
                    continue;
                }
                change |= this->changeTileAt(x, y, 0);
            }
        }
        // protect subtiles from spawning monsters/objects
        // - protect objects from monsters/objects
        change |= this->changeSubtileProtectionAt(10 + 0,  2 + 1, 3);
        // - protect monsters from monsters/objects
        change |= this->changeSubtileProtectionAt( 0 + 1,  0 + 1, 3);
        change |= this->changeSubtileProtectionAt( 2 + 1,  2 + 0, 3);
        change |= this->changeSubtileProtectionAt( 8 + 0,  0 + 1, 3);
        change |= this->changeSubtileProtectionAt( 8 + 1,  4 + 0, 3);
        change |= this->changeSubtileProtectionAt(10 + 1,  2 + 0, 3);
        // - protect area from monsters/objects
        for (int y = 7; y <= 13; y++) {
            for (int x = 1; x <= 13; x++) {
                change |= this->changeSubtileProtectionAt(x, y, 3);
            }
        }
        // replace monsters
        for (int y = 7; y <= 9; y++) {
            for (int x = 7; x <= 13; x++) {
                change |= this->changeMonsterAt(x, y, 16, false);
            }
        }
        // - remove monsters
        change |= this->changeMonsterAt(1, 4, 0, false);
        change |= this->changeMonsterAt(13, 5, 0, false);
        change |= this->changeMonsterAt(7, 12, 0, false);
        // - add unique
        change |= this->changeMonsterAt(8, 12, UMT_SNOTSPIL + 1, true);
        // - add sign-chest
        change |= this->changeObjectAt(10, 3, 90);
        break;
    case DUN_BANNER_AFT: // Banner1.DUN
        // useless tiles
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (this->tiles[y][x] == 13) {
                    change |= this->changeTileAt(x, y, 0);
                }
            }
        }
        // fix the shadows
        change |= this->changeTileAt(0, 1, 11);
        change |= this->changeTileAt(3, 1, 11);
        // use the new shadows
        change |= this->changeTileAt(0, 4, 56);
        change |= this->changeTileAt(0, 5, 55);
        // change |= this->changeTileAt(2, 3, 148);
        change |= this->changeTileAt(2, 4, 53);
        change |= this->changeTileAt(2, 5, 54);
        change |= this->changeTileAt(6, 6, 139);
        // protect the main structure
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (x >= 1 && y >= 3 && x <= 6 && y <= 5) {
                    bool skip = true;
                    if (x == 1 && y == 3) {
                        skip = false;
                    }
                    if (x == 2 && y == 3) {
                        skip = false;
                    }
                    if (x >= 4 && y == 4) {
                        skip = false;
                    }
                    if (skip) {
                        continue;
                    }
                }
                if (x == 3 && y == 6) {
                    continue;
                }
                change |= this->changeTileProtectionAt(x, y, Qt::PartiallyChecked);
            }
        }
        // ensure the changing tiles are reserved
        change |= this->changeTileProtectionAt(3, 6, Qt::Checked);
        for (int y = 3; y <= 5; y++) {
            for (int x = 1; x <= 6; x++) {
                if (x == 1 && y == 3) {
                    continue;
                }
                if (x == 2 && y == 3) {
                    continue;
                }
                if (x >= 4 && y == 4) {
                    continue;
                }
                change |= this->changeTileProtectionAt(x, y, Qt::Checked);
            }
        }
        // remove monsters
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                change |= this->changeMonsterAt(x, y, 0, false);
            }
        }
        // adjust the number of layers
        this->numLayers = 1;
        break;
    case DUN_SKELKING_PRE: // SklKng2.DUN
        // external tiles
        for (int y = 0; y < 25; y++) {
            for (int x = 0; x < 37; x++) {
                if (this->tiles[y][x] >= 181 + 18 && this->tiles[y][x] <= 181 + 24) {
                    change |= this->changeTileAt(x, y, this->tiles[y][x] - 181);
                }
            }
        }
        // useless tiles
        for (int y = 0; y < 25; y++) {
            for (int x = 0; x < 37; x++) {
                // large hidden room
                if (x >= 8 && y >= 1 && x <= 15 && y <= 6) {
                    continue;
                }
                if (x >= 10 && y >= 7 && x <= 13 && y <= 10) {
                    continue;
                }
                if (x == 11 && y == 11) {
                    continue;
                }
                // small hidden room
                if (x >= 20 && y >= 7 && x <= 22 && y <= 10) {
                    continue;
                }
                if (x == 23 && y == 8) {
                    continue;
                }
                // grate
                if (x >= 20 && y >= 14 && x <= 21 && y <= 16) {
                    continue;
                }
                change |= this->changeTileAt(x, y, 0);
            }
        }
        // add sarcophagi
        change |= this->changeObjectAt(14, 23, 5);
        change |= this->changeObjectAt(14, 41, 5);
        change |= this->changeObjectAt(32, 23, 5);
        change |= this->changeObjectAt(32, 41, 5);
        change |= this->changeObjectAt(48, 45, 5);
        change |= this->changeObjectAt(48, 17, 5);
        // add the skeleton king
        change |= this->changeMonsterAt(19, 31, UMT_SKELKING + 1, true);
        // remove monsters
        for (int y = 21; y <= 41; y++) {
            for (int x = 13; x <= 39; x++) {
                if (x >= 18 && y >= 30 && x <= 20 && y <= 32) {
                    continue;
                }
                change |= this->changeMonsterAt(x, y, 0, false);
            }
        }
        for (int y = 21; y <= 36; y++) {
            for (int x = 43; x <= 59; x++) {
                change |= this->changeMonsterAt(x, y, 0, false);
            }
        }
        // protect subtiles from spawning monsters/objects
        // - protect objects from monsters
        change |= this->changeSubtileProtectionAt(10 + 1, 20 + 1, 3);
        change |= this->changeSubtileProtectionAt(12 + 0, 20 + 0, 3);
        change |= this->changeSubtileProtectionAt(12 + 0, 40 + 1, 3);
        change |= this->changeSubtileProtectionAt(10 + 1, 36 + 1, 3);
        change |= this->changeSubtileProtectionAt(30 + 0, 18 + 1, 3);
        change |= this->changeSubtileProtectionAt(32 + 1, 36 + 1, 3);
        change |= this->changeSubtileProtectionAt(14 + 0, 22 + 0, 3);
        change |= this->changeSubtileProtectionAt(14 + 0, 22 + 1, 3);
        change |= this->changeSubtileProtectionAt(14 + 0, 40 + 0, 3);
        change |= this->changeSubtileProtectionAt(14 + 0, 40 + 1, 3);
        change |= this->changeSubtileProtectionAt(18 + 0, 30 + 0, 3);
        change |= this->changeSubtileProtectionAt(18 + 1, 30 + 1, 3);
        change |= this->changeSubtileProtectionAt(18 + 0, 32 + 0, 3);
        change |= this->changeSubtileProtectionAt(20 + 0, 30 + 0, 3);
        change |= this->changeSubtileProtectionAt(20 + 0, 32 + 0, 3);
        change |= this->changeSubtileProtectionAt(32 + 0, 22 + 0, 3);
        change |= this->changeSubtileProtectionAt(32 + 0, 22 + 1, 3);
        change |= this->changeSubtileProtectionAt(32 + 0, 40 + 0, 3);
        change |= this->changeSubtileProtectionAt(32 + 0, 40 + 1, 3);
        change |= this->changeSubtileProtectionAt(34 + 0, 18 + 1, 3);
        change |= this->changeSubtileProtectionAt(46 + 1, 38 + 1, 3);
        change |= this->changeSubtileProtectionAt(48 + 0, 38 + 1, 3);
        change |= this->changeSubtileProtectionAt(48 + 1, 38 + 1, 3);
        change |= this->changeSubtileProtectionAt(48 + 0, 40 + 0, 3);
        change |= this->changeSubtileProtectionAt(48 + 1, 40 + 0, 3);
        change |= this->changeSubtileProtectionAt(50 + 0, 38 + 1, 3);
        change |= this->changeSubtileProtectionAt(48 + 0, 14 + 1, 3);
        change |= this->changeSubtileProtectionAt(48 + 0, 16 + 0, 3);
        change |= this->changeSubtileProtectionAt(48 + 0, 16 + 1, 3);
        change |= this->changeSubtileProtectionAt(48 + 0, 18 + 0, 3);
        change |= this->changeSubtileProtectionAt(48 + 0, 42 + 1, 3);
        change |= this->changeSubtileProtectionAt(48 + 0, 44 + 0, 3);
        change |= this->changeSubtileProtectionAt(48 + 0, 44 + 1, 3);
        // - protect the back-room from additional monsters
        for (int y = 31; y <= 37; y++) {
            for (int x = 3; x <= 9; x++) {
                change |= this->changeSubtileProtectionAt(x, y, 3);
            }
        }
        for (int y = 33; y <= 35; y++) {
            for (int x = 10; x <= 13; x++) {
                change |= this->changeSubtileProtectionAt(x, y, 3);
            }
        }
        // - add empty space after the grate
        for (int y = 29; y <= 33; y++) {
            for (int x = 32; x <= 41; x++) {
                if (x == 32 && (y == 30 || y == 32)) {
                    continue;
                }
                change |= this->changeSubtileProtectionAt(x, y, 3);
            }
        }
        // protect the changing tiles from trap placement
        change |= this->changeSubtileProtectionAt(22 + 0, 22 + 0, 3);
        change |= this->changeSubtileProtectionAt(22 + 1, 22 + 0, 3);
        change |= this->changeSubtileProtectionAt(46 + 0, 16 + 0, 3);
        change |= this->changeSubtileProtectionAt(46 + 0, 16 + 1, 3);
        break;
    case DUN_SKELKING_AFT: // SklKng1.DUN
        // external tiles
        for (int y = 0; y < 25; y++) {
            for (int x = 0; x < 37; x++) {
                if (this->tiles[y][x] >= 181 + 18 && this->tiles[y][x] <= 181 + 24) {
                    change |= this->changeTileAt(x, y, this->tiles[y][x] - 181);
                }
            }
        }
        // useless tiles
        change |= this->changeTileAt(15, 12, 0);
        change |= this->changeTileAt(15, 16, 0);
        change |= this->changeTileAt(25, 11, 0);
        change |= this->changeTileAt(24, 23, 0);
        // fix the shadows
        change |= this->changeTileAt( 9,  2, 143);
        change |= this->changeTileAt(12,  2, 143);
        change |= this->changeTileAt(10,  5, 157);
        change |= this->changeTileAt(24, 18, 140);
        // - remove obsolete shadows
        change |= this->changeTileAt(10, 12, 0);
        change |= this->changeTileAt(10, 13, 0);
        change |= this->changeTileAt(10, 17, 0);
        change |= this->changeTileAt(10, 18, 0);
        change |= this->changeTileAt(22, 13, 0);
        change |= this->changeTileAt(22, 14, 0);
        change |= this->changeTileAt(22, 16, 0);
        change |= this->changeTileAt(22, 17, 0);
        change |= this->changeTileAt(25, 13, 0);
        change |= this->changeTileAt(25, 14, 0);
        change |= this->changeTileAt(25, 16, 0);
        change |= this->changeTileAt(25, 17, 0);
        // use common tiles
        change |= this->changeTileAt(7, 14, 84);
        // use the new shadows
        change |= this->changeTileAt( 9,  3, 139);
        change |= this->changeTileAt( 9,  4, 139);
        change |= this->changeTileAt( 9,  5, 126);
        change |= this->changeTileAt(12,  3, 139);
        change |= this->changeTileAt(12,  4, 139);
        change |= this->changeTileAt(12,  5, 127);
        change |= this->changeTileAt( 4, 15, 150);
        change |= this->changeTileAt( 6, 16, 150);
        change |= this->changeTileAt(15, 17, 159);
        change |= this->changeTileAt(15, 13, 159);
        change |= this->changeTileAt(27, 13, 159);
        change |= this->changeTileAt( 8, 10, 159);
        change |= this->changeTileAt( 8, 12, 144);
        change |= this->changeTileAt(13, 12, 144);
        change |= this->changeTileAt( 8, 17, 144);
        change |= this->changeTileAt(13, 17, 144);
        // remove fix decorations
        change |= this->changeTileAt( 3, 15,   2);
        change |= this->changeTileAt( 5, 20,   1);
        change |= this->changeTileAt( 6,  9,   2);
        change |= this->changeTileAt(10,  1,   2);
        change |= this->changeTileAt(13,  1,   2);
        change |= this->changeTileAt(15,  9,   2);
        change |= this->changeTileAt(17, 14,   2);
        change |= this->changeTileAt(22, 12,   2);
        change |= this->changeTileAt(20,  8,   1);
        change |= this->changeTileAt(21,  7,   2);
        change |= this->changeTileAt(24,  7,   2);
        change |= this->changeTileAt(25, 12,   2);
        change |= this->changeTileAt(26, 12,   2);
        change |= this->changeTileAt(29, 14,   2);
        change |= this->changeTileAt(16, 15,  11);
        change |= this->changeTileAt( 8,  3,   1);
        change |= this->changeTileAt( 8,  4,   1);
        change |= this->changeTileAt(10,  7,   1);
        change |= this->changeTileAt(10,  9,   1);
        change |= this->changeTileAt(21, 13,   1);
        change |= this->changeTileAt(21, 17,   1);
        change |= this->changeTileAt(23,  9,   1);
        change |= this->changeTileAt(23, 10,   1);
        change |= this->changeTileAt(23, 20,   1);
        change |= this->changeTileAt(23, 21,   1);
        change |= this->changeTileAt(31, 15,  11);
        change |= this->changeTileAt(31, 16,  11);
        change |= this->changeTileAt(32, 16,   0);
        change |= this->changeTileAt(33, 16,   0);
        // remove objects, monsters, items
        // - remove sarcophagi tiles
        change |= this->changeTileAt( 6, 11,   0);
        change |= this->changeTileAt( 7, 11,   0);
        change |= this->changeTileAt(15, 11,   0);
        change |= this->changeTileAt(16, 11,   0);
        change |= this->changeTileAt( 6, 20,   0);
        change |= this->changeTileAt( 7, 20,   0);
        change |= this->changeTileAt(15, 20,   0);
        change |= this->changeTileAt(16, 20,   0);
        change |= this->changeTileAt(24,  8,   0);
        change |= this->changeTileAt(24, 22,   0);
        for (int y = 0; y < 25 * 2; y++) {
            for (int x = 0; x < 37 * 2; x++) {
                change |= this->changeObjectAt(x, y, 0);
                change |= this->changeMonsterAt(x, y, 0, false);
                // change |= this->changeItemAt(x, y, 0);
            }
        }
        // ensure the changing tiles are reserved
        change |= this->changeTileProtectionAt(10, 11, Qt::Checked);
        change |= this->changeTileProtectionAt(11, 11, Qt::Checked);
        change |= this->changeTileProtectionAt(12, 11, Qt::Checked);
        change |= this->changeTileProtectionAt(20, 14, Qt::Checked);
        change |= this->changeTileProtectionAt(20, 15, Qt::Checked);
        change |= this->changeTileProtectionAt(20, 16, Qt::Checked);
        change |= this->changeTileProtectionAt(21, 14, Qt::Checked);
        change |= this->changeTileProtectionAt(21, 15, Qt::Checked);
        change |= this->changeTileProtectionAt(21, 16, Qt::Checked);
        change |= this->changeTileProtectionAt(23,  8, Qt::Checked);
        // adjust the number of layers
        this->numLayers = 1;
        break;
    case DUN_BETRAYER: // Vile1.DUN
        // fix corner
        change |= this->changeTileAt(5, 0, 16);
        change |= this->changeTileAt(6, 1, 16);
        change |= this->changeTileAt(5, 1, 15);
        // use base tiles and decorate the walls randomly
        change |= this->changeTileAt(0, 0, 9);
        change |= this->changeTileAt(0, 6, 15);
        change |= this->changeTileAt(1, 0, 2);
        change |= this->changeTileAt(2, 0, 2);
        change |= this->changeTileAt(3, 0, 2);
        change |= this->changeTileAt(4, 0, 2);
        change |= this->changeTileAt(1, 6, 2);
        change |= this->changeTileAt(2, 6, 2);
        change |= this->changeTileAt(4, 6, 2);
        // change |= this->changeTileAt(6, 3, 50);
        // - add the unique monsters
        change |= this->changeMonsterAt(3, 6, UMT_LAZARUS + 1, true);
        change |= this->changeMonsterAt(5, 3, UMT_RED_VEX + 1, true);
        change |= this->changeMonsterAt(5, 9, UMT_BLACKJADE + 1, true);
        // protect inner tiles from spawning additional monsters/objects
        for (int y = 0; y <= 5; y++) {
            for (int x = 0; x <= 5; x++) {
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 0, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 0, 2 * y + 1, 3);
                change |= this->changeSubtileProtectionAt(2 * x + 1, 2 * y + 1, 3);
            }
        }
        // ensure the box is not connected to the rest of the dungeon + protect inner tiles from decoration
        for (int y = 0; y <= 6; y++) {
            for (int x = 0; x <= 6; x++) {
                if (x == 6 && (y == 0 || y == 6)) {
                    continue;
                }
                if (x == 0 || y == 0 || x == 6 || y == 6) {
                    change |= this->changeTileProtectionAt(x, y, Qt::PartiallyChecked);
                } else {
                    change |= this->changeTileProtectionAt(x, y, Qt::Checked);
                }
            }
        }
        break;
    case DUN_DIAB_1: // Diab1.DUN
        // - fix shadow of the left corner
        change |= this->changeTileAt(0, 4, 75);
        change |= this->changeTileAt(0, 5, 74);
        // - fix shadow of the right corner
        change |= this->changeTileAt(4, 1, 131);
        // protect tiles with monsters/objects from spawning additional monsters/objects
        change |= this->changeSubtileProtectionAt(3, 3, 3);
        change |= this->changeSubtileProtectionAt(3, 9, 3);
        change |= this->changeSubtileProtectionAt(4, 7, 3);
        change |= this->changeSubtileProtectionAt(7, 7, 3);
        change |= this->changeSubtileProtectionAt(7, 4, 3);
        change |= this->changeSubtileProtectionAt(9, 3, 3);
        change |= this->changeSubtileProtectionAt(9, 9, 3);
        change |= this->changeSubtileProtectionAt(5, 5, 3);
        // protect tiles with monsters/objects from decoration
        change |= this->changeTileProtectionAt(1, 4, Qt::Checked);
        change |= this->changeTileProtectionAt(2, 3, Qt::Checked);
        change |= this->changeTileProtectionAt(1, 1, Qt::Checked);
        change |= this->changeTileProtectionAt(3, 2, Qt::Checked);
        change |= this->changeTileProtectionAt(3, 3, Qt::Checked);
        change |= this->changeTileProtectionAt(4, 4, Qt::Checked);
        change |= this->changeTileProtectionAt(4, 1, Qt::Checked);
        change |= this->changeTileProtectionAt(2, 2, Qt::Checked);
        break;
    case DUN_DIAB_2_PRE: // Diab2a.DUN
        // external tiles
        for (int y = 1; y <= 3; y++) {
            for (int x = 9; x <= 10; x++) {
                if (this->tiles[y][x] >= 98 + 18 && this->tiles[y][x] <= 98 + 30) {
                    int newTile = this->tiles[y][x] - 98;
                    // if (newTile == 20) {
                    //    newTile = 30;
                    // }
                    change |= this->changeTileAt(x, y, newTile);
                }
            }
        }
        // useless tiles
        for (int y = 0; y < 12; y++) {
            for (int x = 0; x < 11; x++) {
                if (x >= 9 && x <= 10 && y >= 1 && y <= 3) {
                    continue;
                }
                change |= this->changeTileAt(x, y, 0);
            }
        }
        // protect changing tiles from objects
        change |= this->changeSubtileProtectionAt(21,  2, 3);
        change |= this->changeSubtileProtectionAt(21,  3, 3);
        change |= this->changeSubtileProtectionAt(21,  4, 3);
        change |= this->changeSubtileProtectionAt(21,  5, 3);
        change |= this->changeSubtileProtectionAt(21,  6, 3);
        change |= this->changeSubtileProtectionAt(21,  7, 3);
        // protect tiles with monsters/objects from spawning additional monsters/objects
        change |= this->changeSubtileProtectionAt( 5,  3, 3);
        change |= this->changeSubtileProtectionAt( 5, 19, 3);
        change |= this->changeSubtileProtectionAt(11,  9, 3);
        change |= this->changeSubtileProtectionAt(11, 13, 3);
        change |= this->changeSubtileProtectionAt(13, 12, 3);
        change |= this->changeSubtileProtectionAt(14, 10, 3);
        change |= this->changeSubtileProtectionAt(17,  9, 3);
        change |= this->changeSubtileProtectionAt(17, 13, 3);
        change |= this->changeSubtileProtectionAt(15, 17, 3);
        change |= this->changeSubtileProtectionAt(16, 3, 3);
        change |= this->changeSubtileProtectionAt(16, 5, 3);
        change |= this->changeSubtileProtectionAt(13, 10, 3);
        break;
    case DUN_DIAB_2_AFT: // Diab2b.DUN
        // external tiles
        for (int y = 0; y < 12; y++) {
            for (int x = 0; x < 11; x++) {
                if (this->tiles[y][x] >= 98 + 18 && this->tiles[y][x] <= 98 + 30) {
                    int newTile = this->tiles[y][x] - 98;
                    // if (newTile == 20) {
                    //    newTile = 30;
                    // }
                    change |= this->changeTileAt(x, y, newTile);
                }
            }
        }
        // use base tiles and let the engine decorate the walls
        for (int y = 0; y < 12; y++) {
            for (int x = 0; x < 11; x++) {
                if (y == 1 && (x == 8 || x == 9)) {
                    continue; // skip protected tiles
                }
                int wv = this->tiles[y][x];
                if (wv >= 63 && wv <= 70) {
                    if (wv == 63 || wv == 65 || wv == 67 || wv == 68) {
                        wv = 1;
                    } else {
                        wv = 2;
                    }
                    change |= this->changeTileAt(x, y, wv);
                }
            }
        }
        // remove shadow to enable optional connection
        change |= this->changeTileAt(0, 10, 0);
        change |= this->changeTileAt(0, 11, 0);
        // ensure the changing tiles are reserved
        change |= this->changeTileProtectionAt( 9,  1, Qt::Checked);
        change |= this->changeTileProtectionAt( 9,  2, Qt::Checked);
        change |= this->changeTileProtectionAt( 9,  3, Qt::Checked);
        change |= this->changeTileProtectionAt(10,  1, Qt::Checked);
        change |= this->changeTileProtectionAt(10,  2, Qt::Checked);
        change |= this->changeTileProtectionAt(10,  3, Qt::Checked);
        // protect tiles with monsters/objects from decoration
        change |= this->changeTileProtectionAt( 2,  1, Qt::Checked);
        change |= this->changeTileProtectionAt( 2,  9, Qt::Checked);
        change |= this->changeTileProtectionAt( 5,  4, Qt::Checked);
        change |= this->changeTileProtectionAt( 5,  6, Qt::Checked);
        change |= this->changeTileProtectionAt( 6,  6, Qt::Checked);
        change |= this->changeTileProtectionAt( 7,  5, Qt::Checked);
        change |= this->changeTileProtectionAt( 8,  4, Qt::Checked);
        change |= this->changeTileProtectionAt( 8,  6, Qt::Checked);
        change |= this->changeTileProtectionAt( 7,  8, Qt::Checked);
        change |= this->changeTileProtectionAt( 8,  1, Qt::Checked);
        change |= this->changeTileProtectionAt( 8,  2, Qt::Checked);
        change |= this->changeTileProtectionAt( 6,  5, Qt::Checked);
        // remove monsters
        for (int y = 0; y < 24; y++) {
            for (int x = 0; x < 22; x++) {
                change |= this->changeMonsterAt(x, y, 0, false);
            }
        }
        // remove objects
        change |= this->changeObjectAt(13, 10, 0);
        // adjust the number of layers
        this->numLayers = 1;
        break;
    case DUN_DIAB_3_PRE: // Diab3a.DUN
        // useless tiles
        for (int y = 0; y < 11; y++) {
            for (int x = 0; x < 11; x++) {
                if (x >= 4 && x <= 6 && y >= 10 && y <= 10) {
                    continue; // SW-wall
                }
                if (x >= 0 && x <= 0 && y >= 4 && y <= 6) {
                    continue; // NW-wall
                }
                if (x >= 4 && x <= 6 && y >= 0 && y <= 0) {
                    continue; // NE-wall
                }
                if (x >= 10 && x <= 10 && y >= 4 && y <= 6) {
                    continue; // SE-wall
                }
                change |= this->changeTileAt(x, y, 0);
            }
        }
        // protect changing tiles from objects
        // - SW-wall
        change |= this->changeSubtileProtectionAt( 8, 21, 3);
        change |= this->changeSubtileProtectionAt( 9, 21, 3);
        change |= this->changeSubtileProtectionAt(10, 21, 3);
        change |= this->changeSubtileProtectionAt(11, 21, 3);
        change |= this->changeSubtileProtectionAt(12, 21, 3);
        change |= this->changeSubtileProtectionAt(13, 21, 3);
        // - NE-wall
        change |= this->changeSubtileProtectionAt( 8,  1, 3);
        change |= this->changeSubtileProtectionAt( 9,  1, 3);
        change |= this->changeSubtileProtectionAt(10,  1, 3);
        change |= this->changeSubtileProtectionAt(11,  1, 3);
        change |= this->changeSubtileProtectionAt(12,  1, 3);
        change |= this->changeSubtileProtectionAt(13,  1, 3);
        // - NW-wall
        change |= this->changeSubtileProtectionAt( 1,  8, 3);
        change |= this->changeSubtileProtectionAt( 1,  9, 3);
        change |= this->changeSubtileProtectionAt( 1, 10, 3);
        change |= this->changeSubtileProtectionAt( 1, 11, 3);
        change |= this->changeSubtileProtectionAt( 1, 12, 3);
        change |= this->changeSubtileProtectionAt( 1, 13, 3);
        // - SE-wall
        change |= this->changeSubtileProtectionAt(21,  8, 3);
        change |= this->changeSubtileProtectionAt(21,  9, 3);
        change |= this->changeSubtileProtectionAt(21, 10, 3);
        change |= this->changeSubtileProtectionAt(21, 11, 3);
        change |= this->changeSubtileProtectionAt(21, 12, 3);
        change |= this->changeSubtileProtectionAt(21, 13, 3);
        // protect tiles with monsters/objects from spawning additional monsters/objects
        change |= this->changeSubtileProtectionAt( 1,  5, 3);
        change |= this->changeSubtileProtectionAt( 1, 15, 3);
        change |= this->changeSubtileProtectionAt( 5,  1, 3);
        change |= this->changeSubtileProtectionAt( 5,  8, 3);
        change |= this->changeSubtileProtectionAt( 5, 12, 3);
        change |= this->changeSubtileProtectionAt( 5, 19, 3);
        change |= this->changeSubtileProtectionAt( 7,  7, 3);
        change |= this->changeSubtileProtectionAt( 7, 13, 3);
        change |= this->changeSubtileProtectionAt( 8,  2, 3);
        change |= this->changeSubtileProtectionAt( 8,  5, 3);
        change |= this->changeSubtileProtectionAt( 8, 14, 3);
        change |= this->changeSubtileProtectionAt( 8, 15, 3);
        change |= this->changeSubtileProtectionAt(12,  5, 3);
        change |= this->changeSubtileProtectionAt(12, 15, 3);
        change |= this->changeSubtileProtectionAt(13,  7, 3);
        change |= this->changeSubtileProtectionAt(13, 13, 3);
        change |= this->changeSubtileProtectionAt(15,  1, 3);
        change |= this->changeSubtileProtectionAt(15,  8, 3);
        change |= this->changeSubtileProtectionAt(15, 12, 3);
        change |= this->changeSubtileProtectionAt(15, 19, 3);
        change |= this->changeSubtileProtectionAt(19,  5, 3);
        change |= this->changeSubtileProtectionAt(19, 15, 3);
        break;
    case DUN_DIAB_3_AFT: // Diab3b.DUN
        // external tiles
        change |= this->changeTileAt(4, 4, 21);
        change |= this->changeTileAt(4, 5, 18);
        change |= this->changeTileAt(5, 4, 19);
        change |= this->changeTileAt(5, 5, 30);
        // remove partial shadow
        change |= this->changeTileAt(5, 0, 50);
        // remove shadow to enable optional connection
        change |= this->changeTileAt(1,  9, 0);
        change |= this->changeTileAt(1, 10, 0);
        // use base tiles and let the engine decorate the walls
        change |= this->changeTileAt(3, 10, 2);
        change |= this->changeTileAt(9, 8, 2);
        change |= this->changeTileAt(8, 9, 1);
        change |= this->changeTileAt(6, 5, 1);
        change |= this->changeTileAt(5, 6, 2);
        change |= this->changeTileAt(10, 7, 1);
        change |= this->changeTileAt(2, 1, 1);
        change |= this->changeTileAt(1, 2, 2);
        change |= this->changeTileAt(0, 3, 1);
        change |= this->changeTileAt(10, 3, 1);
        // ensure the changing tiles are reserved
        // - SW-wall
        change |= this->changeTileProtectionAt( 4, 10, Qt::Checked);
        change |= this->changeTileProtectionAt( 5, 10, Qt::Checked);
        change |= this->changeTileProtectionAt( 6, 10, Qt::Checked);
        // - NE-wall
        change |= this->changeTileProtectionAt( 4,  0, Qt::Checked);
        change |= this->changeTileProtectionAt( 5,  0, Qt::Checked);
        change |= this->changeTileProtectionAt( 6,  0, Qt::Checked);
        // - NW-wall
        change |= this->changeTileProtectionAt( 0,  4, Qt::Checked);
        change |= this->changeTileProtectionAt( 0,  5, Qt::Checked);
        change |= this->changeTileProtectionAt( 0,  6, Qt::Checked);
        // - SE-wall
        change |= this->changeTileProtectionAt(10,  4, Qt::Checked);
        change |= this->changeTileProtectionAt(10,  5, Qt::Checked);
        change |= this->changeTileProtectionAt(10,  6, Qt::Checked);
        // protect tiles with monsters/objects from decoration
        change |= this->changeTileProtectionAt( 0,  2, Qt::Checked);
        change |= this->changeTileProtectionAt( 0,  7, Qt::Checked);
        change |= this->changeTileProtectionAt( 2,  0, Qt::Checked);
        change |= this->changeTileProtectionAt( 2,  4, Qt::Checked);
        change |= this->changeTileProtectionAt( 2,  6, Qt::Checked);
        change |= this->changeTileProtectionAt( 2,  9, Qt::Checked);
        change |= this->changeTileProtectionAt( 3,  3, Qt::Checked);
        change |= this->changeTileProtectionAt( 3,  6, Qt::Checked);
        change |= this->changeTileProtectionAt( 4,  1, Qt::Checked);
        change |= this->changeTileProtectionAt( 4,  2, Qt::Checked);
        change |= this->changeTileProtectionAt( 4,  7, Qt::Checked);
        change |= this->changeTileProtectionAt( 4,  7, Qt::Checked);
        change |= this->changeTileProtectionAt( 6,  2, Qt::Checked);
        change |= this->changeTileProtectionAt( 6,  7, Qt::Checked);
        change |= this->changeTileProtectionAt( 6,  3, Qt::Checked);
        change |= this->changeTileProtectionAt( 6,  6, Qt::Checked);
        change |= this->changeTileProtectionAt( 7,  0, Qt::Checked);
        change |= this->changeTileProtectionAt( 7,  4, Qt::Checked);
        change |= this->changeTileProtectionAt( 7,  6, Qt::Checked);
        change |= this->changeTileProtectionAt( 7,  9, Qt::Checked);
        change |= this->changeTileProtectionAt( 9,  2, Qt::Checked);
        change |= this->changeTileProtectionAt( 9,  7, Qt::Checked);
        // remove monsters
        for (int y = 0; y < 22; y++) {
            for (int x = 0; x < 22; x++) {
                change |= this->changeMonsterAt(x, y, 0, false);
            }
        }
        // remove objects
        change |= this->changeObjectAt(8, 14, 0);
        change |= this->changeObjectAt(8, 2, 0);
        // adjust the number of layers
        this->numLayers = 1;
        break;
    case DUN_DIAB_4_PRE: // Diab4a.DUN
        for (int y = 0; y < 9; y++) {
            for (int x = 0; x < 9; x++) {
                // external tiles
                if (this->tiles[y][x] >= 98 + 18 && this->tiles[y][x] <= 98 + 30) {
                    int newTile = this->tiles[y][x] - 98;
                    // if (newTile == 20) {
                    //    newTile = 30;
                    // }
                    change |= this->changeTileAt(x, y, newTile);
                }
                // useless tiles
                if (x >= 2 && x <= 6 && y >= 7 && y <= 8) {
                    continue; // SW-wall
                }
                if (x >= 0 && x <= 1 && y >= 2 && y <= 6) {
                    continue; // NW-wall
                }
                if (x >= 2 && x <= 6 && y >= 0 && y <= 1) {
                    continue; // NE-wall
                }
                if (x >= 7 && x <= 8 && y >= 2 && y <= 6) {
                    continue; // SE-wall
                }
                change |= this->changeTileAt(x, y, 0);
            }
        }
        // make diablo unique
        change |= this->changeMonsterAt(8, 8, UMT_DIABLO + 1, true);
        // replace the only black knight
        change |= this->changeMonsterAt(4, 6, 101, false);
        // protect changing tiles from objects
        // - SW-wall
        for (int y = 4; y < 14; y++) {
            change |= this->changeSubtileProtectionAt(17, y, 3);
        }
        // - NE-wall
        for (int x = 4; x < 14; x++) {
            change |= this->changeSubtileProtectionAt(x, 3, 3);
        }
        // - NW-wall
        for (int y = 4; y < 14; y++) {
            change |= this->changeSubtileProtectionAt(3, y, 3);
        }
        // - SE-wall
        for (int x = 4; x < 14; x++) {
            change |= this->changeSubtileProtectionAt(x, 17, 3);
        }
        // protect tiles with monsters/objects from spawning additional monsters/objects
        change |= this->changeSubtileProtectionAt( 4,  4, 3);
        change |= this->changeSubtileProtectionAt( 4,  6, 3);
        change |= this->changeSubtileProtectionAt( 4,  8, 3);
        change |= this->changeSubtileProtectionAt( 4, 12, 3);
        change |= this->changeSubtileProtectionAt( 6,  6, 3);
        change |= this->changeSubtileProtectionAt( 6,  8, 3);
        change |= this->changeSubtileProtectionAt( 6, 10, 3);
        change |= this->changeSubtileProtectionAt( 6, 12, 3);
        change |= this->changeSubtileProtectionAt( 8,  4, 3);
        change |= this->changeSubtileProtectionAt( 8,  6, 3);
        change |= this->changeSubtileProtectionAt( 8,  8, 3);
        change |= this->changeSubtileProtectionAt( 8, 10, 3);
        change |= this->changeSubtileProtectionAt( 8, 12, 3);
        change |= this->changeSubtileProtectionAt(10,  4, 3);
        change |= this->changeSubtileProtectionAt(10,  6, 3);
        change |= this->changeSubtileProtectionAt(10,  8, 3);
        change |= this->changeSubtileProtectionAt(10, 10, 3);
        change |= this->changeSubtileProtectionAt(12,  4, 3);
        change |= this->changeSubtileProtectionAt(12,  8, 3);
        change |= this->changeSubtileProtectionAt(12, 10, 3);
        change |= this->changeSubtileProtectionAt(12, 12, 3);
        break;
    case DUN_DIAB_4_AFT: // Diab4b.DUN
        // external tiles
        for (int y = 0; y < 9; y++) {
            for (int x = 0; x < 9; x++) {
                if (this->tiles[y][x] >= 98 + 18 && this->tiles[y][x] <= 98 + 30) {
                    int newTile = this->tiles[y][x] - 98;
                    // if (newTile == 20) {
                    //    newTile = 30;
                    // }
                    change |= this->changeTileAt(x, y, newTile);
                }
            }
        }
        // ensure the box is not connected to the rest of the dungeon and the changing tiles are reserved + protect inner tiles from decoration
        for (int y = 0; y < 9; y++) {
            for (int x = 0; x < 9; x++) {
                change |= this->changeTileProtectionAt(x, y, Qt::Checked);
            }
        }
        // remove monsters
        for (int y = 0; y < 18; y++) {
            for (int x = 0; x < 18; x++) {
                change |= this->changeMonsterAt(x, y, 0, false);
            }
        }
        // adjust the number of layers
        this->numLayers = 1;
        break;
    }
    if (!change) {
        dProgress() << tr("No change was necessary.");
    }
}

bool D1Dun::reloadTileset(const QString &celFilePath)
{
    OpenAsParam params = OpenAsParam();
    params.celFilePath = celFilePath;
    return this->tileset->load(params);
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
                // customObject.width = params.width;
                customObject.coPreFlag = params.preFlag;
                return true;
            }
        }
        // add new entry
        CustomObjectStruct customObject;
        customObject.type = params.index;
        customObject.name = params.name;
        customObject.path = params.path;
        customObject.frameNum = params.frame;
        // customObject.width = params.width;
        customObject.coPreFlag = params.preFlag;
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
            if (this->monsterCache[i].monType.monIndex == params.index && this->monsterCache[i].monType.monUnique == params.uniqueMon && this->monsterCache[i].monType.monDeadFlag == params.preFlag) {
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
        DunMonsterType monType = { params.index, params.uniqueMon, params.preFlag };
        for (unsigned i = 0; i < this->customMonsterTypes.size(); i++) {
            CustomMonsterStruct &customMonster = this->customMonsterTypes[i];
            if (customMonster.type == monType) {
                customMonster.name = params.name;
                customMonster.path = params.path;
                customMonster.baseTrnPath = params.baseTrnPath;
                customMonster.uniqueTrnPath = params.uniqueTrnPath;
                // customMonster.width = params.width;
                return true;
            }
        }
        // add new entry
        CustomMonsterStruct customMonster;
        customMonster.type = monType;
        customMonster.name = params.name;
        customMonster.path = params.path;
        customMonster.baseTrnPath = params.baseTrnPath;
        customMonster.uniqueTrnPath = params.uniqueTrnPath;
        // customMonster.width = params.width;
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
    case DUN_ENTITY_TYPE::MISSILE: {
        /* test if it overwrites an existing entry?
        for (const CustomMissileStruct customMissile : this->customMissileTypes) {
            if (customMissile.type == params.index) {
                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to replace %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
                if (reply != QMessageBox::Yes) {
                    return false;
                }
            }
        }*/
        // check if the gfx can be loaded - TODO: merge with loadMissile?
        D1Gfx *misGfx = new D1Gfx();
        // misGfx->setPalette(this->pal);
        QString cl2FilePath = params.path;
        OpenAsParam openParams = OpenAsParam();
        openParams.celWidth = params.width;
        D1Cl2::load(*misGfx, cl2FilePath, openParams);
        bool result = misGfx->getFrameCount() != 0;
        delete misGfx;
        if (!result) {
            dProgressFail() << tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(cl2FilePath));
            return false;
        }
        // check if the TRNs can be loaded
        if (!params.baseTrnPath.isEmpty()) {
            D1Trn *misTrn = new D1Trn();
            result = misTrn->load(params.baseTrnPath, this->pal);
            delete misTrn;
            if (!result) {
                dProgressFail() << tr("Failed loading TRN file: %1.").arg(QDir::toNativeSeparators(params.baseTrnPath));
                return false;
            }
        }
        // remove cache entry
        for (unsigned i = 0; i < this->missileCache.size(); i++) {
            if (this->missileCache[i].misIndex == params.index) {
                D1Gfx *gfx = this->missileCache[i].misGfx[0];
                this->missileCache.erase(this->missileCache.begin() + i);
                if (gfx == nullptr) {
                    break; // previous entry without gfx -> done
                }
                for (i = 0; i < this->missileDataCache.size(); i++) {
                    auto &dataEntry = this->missileDataCache[i];
                    if (dataEntry.midGfx[0] == gfx) {
                        dataEntry.numrefs--;
                        if (dataEntry.numrefs == 0) {
                            for (unsigned g = 0; g < dataEntry.numgfxs; g++) {
                                delete dataEntry.midGfx[g];
                            }
                            this->missileDataCache.erase(this->missileDataCache.begin() + i);
                        }
                        break;
                    }
                }
                break;
            }
        }
        // replace previous entry
        for (unsigned i = 0; i < this->customMissileTypes.size(); i++) {
            CustomMissileStruct &customMissile = this->customMissileTypes[i];
            if (customMissile.type == params.index) {
                customMissile.name = params.name;
                customMissile.path = params.path;
                customMissile.trnPath = params.baseTrnPath;
                // customMissile.width = params.width;
                return true;
            }
        }
        // add new entry
        CustomMissileStruct customMissile;
        customMissile.type = params.index;
        customMissile.name = params.name;
        customMissile.path = params.path;
        customMissile.trnPath = params.baseTrnPath;
        // customMissile.width = params.width;
        customMissile.cmiPreFlag = params.preFlag;
        this->customMissileTypes.push_back(customMissile);
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

const std::vector<CustomMissileStruct> &D1Dun::getCustomMissileTypes() const
{
    return this->customMissileTypes;
}

void D1Dun::game_logic()
{
    // process monsters
    for (unsigned y = 0; y < this->monsters.size(); y++) {
        std::vector<MapMonster> &monstersRow = this->monsters[y];
        for (unsigned x = 0; x < monstersRow.size(); x++) {
            if (monstersRow[x].moType.monIndex != 0 || monstersRow[x].moType.monUnique) {
                monstersRow[x].frameNum++;
            }
        }
    }
}
