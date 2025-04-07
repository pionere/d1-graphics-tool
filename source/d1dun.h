#pragma once

#include <map>
#include <vector>

#include <QPainter>
#include <QString>

#include "dungeonresourcedialog.h"
#include "openasdialog.h"
#include "saveasdialog.h"

class D1Gfx;
class D1Min;
class D1Pal;
class D1Sla;
class D1Til;
class D1Trn;
struct LoadFileContent;

#define UNDEF_SUBTILE -1
#define UNDEF_TILE -1

typedef enum dun_file_index {
    DUN_SKELKING_ENTRY,      // entry of the "Skeleton King's Lair" setmap (SKngDO.DUN)
    DUN_SKELKING_PRE,        // setmap "Skeleton King's Lair" before pulling the levers or destroying the cruxs (SklKng2.DUN)
    DUN_SKELKING_AFT,        // setmap "Skeleton King's Lair" after pulling the levers or destroying the cruxs (SklKng1.DUN)
    DUN_BANNER_PRE,          // map tile for the "Ogden's Sign" quest before delivering the 'Tavern Sign' (Banner2.DUN)
    DUN_BANNER_AFT,          // map tile for the "Ogden's Sign" quest after delivering the 'Tavern Sign' (Banner1.DUN)
    DUN_BONECHAMB_ENTRY_PRE, // entry of the "Chamber of Bone" setmap before reading the book (Bonestr1.DUN)
    DUN_BONECHAMB_ENTRY_AFT, // entry of the "Chamber of Bone" setmap after reading the book (Bonestr2.DUN)
    DUN_BONECHAMB_PRE,       // setmap "Chamber of Bone" before pulling the levers (Bonecha1.DUN)
    DUN_BONECHAMB_AFT,       // setmap "Chamber of Bone" after pulling the levers (Bonecha2.DUN)
    DUN_BLIND_PRE,           // map tile for the "Halls of the Blind" quest before reading the book (Blind2.DUN)
    DUN_BLIND_AFT,           // map tile for the "Halls of the Blind" quest after reading the book (Blind1.DUN)
    DUN_BLOOD_PRE,           // map tile for the "Valor" quest before and after placing the stones (Blood2.DUN)
    DUN_BLOOD_AFT,           // map tile for the "Valor" quest 'after' placing the stones (Blood1.DUN)
    DUN_FOULWATR,            // setmap for the "Poisoned Water" quest (Foulwatr.DUN)
    DUN_VILE_PRE,            // setmap for the "Archbishop Lazarus" quest before reading the books (Vile2.DUN)
    DUN_VILE_AFT,            // setmap for the "Archbishop Lazarus" quest after reading the books (Vile1.DUN)
    DUN_WARLORD_PRE,         // map tile for the "Warlord" quest before reading the book (Warlord.DUN)
    DUN_WARLORD_AFT,         // map tile for the "Warlord" quest after reading the book (Warlord2.DUN)
    DUN_DIAB_1,              // map tile for the "Diablo" quest (Diab1.DUN)
    DUN_DIAB_2_PRE,          // map tile for the "Diablo" quest before pulling the levers (Diab2a.DUN)
    DUN_DIAB_2_AFT,          // map tile for the "Diablo" quest after pulling the levers (Diab2b.DUN)
    DUN_DIAB_3_PRE,          // map tile for the "Diablo" quest before pulling the levers (Diab3a.DUN)
    DUN_DIAB_3_AFT,          // map tile for the "Diablo" quest after pulling the levers (Diab3b.DUN)
    DUN_DIAB_4_PRE,          // map tile for the "Diablo" quest before pulling the levers (Diab4a.DUN)
    DUN_DIAB_4_AFT,          // map tile for the "Diablo" quest after pulling the levers (Diab4b.DUN)
    DUN_BETRAYER,            // map tile for the "Archbishop Lazarus" quest (Vile1.DUN)
} dun_file_index;

enum class D1DUN_TYPE {
    NORMAL,
    RAW,
};

typedef enum dun_overlay_type {
    DOT_NONE,
    DOT_MAP,
    DOT_ROOMS,
    DOT_TILE_PROTECTIONS,
    DOT_SUBTILE_PROTECTIONS,
    DOT_TILE_NUMBERS,
    DOT_SUBTILE_NUMBERS,
} dun_overlay_type;

typedef struct DunMonsterType {
    int monIndex;
    bool monUnique;

    bool operator==(const DunMonsterType &rhs) const {
        return monIndex == rhs.monIndex && monUnique == rhs.monUnique;
    }
    bool operator!=(const DunMonsterType &rhs) const {
        return monIndex != rhs.monIndex || monUnique != rhs.monUnique;
    }
} DunMonsterType;
Q_DECLARE_METATYPE(DunMonsterType);

typedef struct MapMonster {
    DunMonsterType type;
    int mox;
    int moy;
} MapMonster;

typedef struct DunObjectStruct {
    const char *name;
} DunObjectStruct;

typedef struct CustomObjectStruct {
    int type;
    int width;
    QString path;
    QString name;
    int frameNum;
} CustomObjectStruct;

typedef struct DunMonsterStruct {
    const char *name;
} DunMonsterStruct;

typedef struct CustomMonsterStruct {
    DunMonsterType type;
    int width;
    int animGroup;
    QString path;
    QString baseTrnPath;
    QString uniqueTrnPath;
    QString name;
} CustomMonsterStruct;

typedef struct CustomItemStruct {
    int type;
    int width;
    QString path;
    QString name;
} CustomItemStruct;

typedef struct ObjectCacheEntry {
    int objectIndex;
    D1Gfx *objGfx;
    int frameNum;
} ObjectCacheEntry;

typedef struct MonsterCacheEntry {
    DunMonsterType monType;
    D1Gfx *monGfx;
    int monDir;
    D1Pal *monPal;
    D1Trn *monBaseTrn;
    D1Trn *monUniqueTrn;
} MonsterCacheEntry;

typedef struct ItemCacheEntry {
    int itemIndex;
    D1Gfx *itemGfx;
} ItemCacheEntry;

class DunDrawParam {
public:
    Qt::CheckState tileState;
    int overlayType; // dun_overlay_type
    bool showItems;
    bool showMonsters;
    bool showObjects;
    unsigned time;
};

class D1Dun : public QObject {
    Q_OBJECT

public:
    D1Dun() = default;
    ~D1Dun();

    bool load(const QString &dunFilePath, const OpenAsParam &params);
    void initialize(D1Pal *pal, D1Tileset *tileset);
    bool save(const SaveAsParam &params);
    void setTileset(D1Tileset *tileset);

    void compareTo(const LoadFileContent *fileContent) const;

    QImage getImage(const DunDrawParam &params);
    QImage getObjectImage(int objectIndex, unsigned time);
    QImage getMonsterImage(DunMonsterType monType, unsigned time);
    QImage getItemImage(int itemIndex);

    void setPal(D1Pal *pal);

    QString getFilePath() const;
    bool isModified() const;
    uint8_t getNumLayers() const;
    int getWidth() const;
    bool setWidth(int width, bool force);
    int getHeight() const;
    bool setHeight(int height, bool force);

    int getTileAt(int posx, int posy) const;
    bool setTileAt(int posx, int posy, int tileRef);
    int getSubtileAt(int posx, int posy) const;
    bool setSubtileAt(int posx, int posy, int subtileRef);
    int getItemAt(int posx, int posy) const;
    bool setItemAt(int posx, int posy, int itemIndex);
    MapMonster getMonsterAt(int posx, int posy) const;
    bool setMonsterAt(int posx, int posy, const DunMonsterType monType, int xoff, int yoff);
    int getObjectAt(int posx, int posy) const;
    bool setObjectAt(int posx, int posy, int objectIndex);
    int getRoomAt(int posx, int posy) const;
    bool setRoomAt(int posx, int posy, int roomIndex);
    Qt::CheckState getTileProtectionAt(int posx, int posy) const;
    bool setTileProtectionAt(int posx, int posy, Qt::CheckState protection);
    bool getSubtileObjProtectionAt(int posx, int posy) const;
    bool setSubtileObjProtectionAt(int posx, int posy, bool protection);
    bool getSubtileMonProtectionAt(int posx, int posy) const;
    bool setSubtileMonProtectionAt(int posx, int posy, bool protection);
    bool swapPositions(int mode, int posx0, int posy0, int posx1, int posy1);

    int getLevelType() const;
    bool setLevelType(int levelType);
    int getDefaultTile() const;
    bool setDefaultTile(int defaultTile);
    bool setAssetPath(QString path);
    QString getAssetPath() const;
    QString getItemName(int itemIndex) const;
    QString getMonsterName(const DunMonsterType &monType) const;
    QString getObjectName(int objectIndex) const;

    std::pair<int, int> collectSpace() const;
    void collectItems(std::vector<std::pair<int, int>> &items) const;
    void collectMonsters(std::vector<std::pair<DunMonsterType, int>> &monsters) const;
    void collectObjects(std::vector<std::pair<int, int>> &objects) const;
    void checkTiles() const;
    void checkProtections() const;
    void checkItems() const;
    void checkMonsters() const;
    void checkObjects() const;
    bool removeTiles();
    bool removeProtections();
    bool removeItems();
    bool removeMonsters();
    bool removeObjects();
    void loadTiles(const D1Dun *srcDun);
    void loadProtections(const D1Dun *srcDun);
    void loadItems(const D1Dun *srcDun);
    void loadMonsters(const D1Dun *srcDun);
    void loadObjects(const D1Dun *srcDun);

    bool resetTiles();
    bool resetSubtiles();
    void refreshSubtiles();
    void subtileInserted(unsigned subtileIndex);
    void tileInserted(unsigned tileIndex);
    void subtileRemoved(unsigned subtileIndex);
    void tileRemoved(unsigned tileIndex);
    void subtilesSwapped(unsigned subtileIndex0, unsigned subtileIndex1);
    void tilesSwapped(unsigned tileIndex0, unsigned tileIndex1);
    void subtilesRemapped(const std::map<unsigned, unsigned> &remap);
    void tilesRemapped(const std::map<unsigned, unsigned> &remap);
    bool maskTilesFrom(const D1Dun *srcDun);
    bool protectTiles();
    bool protectTilesFrom(const D1Dun *srcDun);
    bool protectSubtiles();

    void insertTileRow(int posy);
    void insertTileColumn(int posx);
    void removeTileRow(int posy);
    void removeTileColumn(int posx);

    void patch(int dunFileIndex); // dun_file_index

    bool reloadTileset(const QString &celFilePath);

    bool addResource(const AddResourceParam &params);
    const std::vector<CustomObjectStruct> &getCustomObjectTypes() const;
    const std::vector<CustomMonsterStruct> &getCustomMonsterTypes() const;
    const std::vector<CustomItemStruct> &getCustomItemTypes() const;
    void clearAssets();

private:
    static void DrawDiamond(QImage &image, unsigned sx, unsigned sy, unsigned width, const QColor &color);
    static void DrawPixel(int sx, int sy, uint8_t color);
    static void DrawLine(int x0, int y0, int x1, int y1, uint8_t color);
    static void DrawAutomapExtern(int sx, int sy);
    static void DrawAutomapStairs(int sx, int sy);
    static void DrawAutomapDoorDiamond(int dir, int sx, int sy);
    static void DrawMap(int drawCursorX, int drawCursorY, uint8_t automap_type);
    void drawBack(QPainter &dungeon, unsigned cellWidth, int drawCursorX, int drawCursorY, const DunDrawParam &params);
    void drawFloor(QPainter &dungeon, unsigned cellWidth, int drawCursorX, int drawCursorY, int dunCursorX, int dunCursorY, const DunDrawParam &params);
    void drawCell(QPainter &dungeon, unsigned cellWidth, int drawCursorX, int drawCursorY, int dunCursorX, int dunCursorY, const DunDrawParam &params);
    void drawMeta(QPainter &dungeon, unsigned cellWidth, int drawCursorX, int drawCursorY, int dunCursorX, int dunCursorY, const DunDrawParam &params);
    void drawLayer(QPainter &dungeon, const DunDrawParam &params, int layer);
    void initVectors(int width, int height);
    void loadObjectGfx(const QString &filePath, int width, ObjectCacheEntry &result);
    void loadMonsterGfx(const QString &filePath, int width, int dir, const QString &baseTrnFilePath, const QString &uniqueTrnFilePath, MonsterCacheEntry &result);
    void loadItemGfx(const QString &filePath, int width, ItemCacheEntry &result);
    void loadObject(int objectIndex);
    void loadMonster(const DunMonsterType &monType);
    void loadItem(int itemIndex);
    void updateSubtiles(int tilePosX, int tilePosY, int tileRef);
    bool changeTileAt(int tilePosX, int tilePosY, int tileRef);
    bool changeObjectAt(int posx, int posy, int objectIndex);
    bool changeMonsterAt(int posx, int posy, int monsterIndex, bool isUnique);
    bool changeItemAt(int posx, int posy, int itemIndex);
    bool changeTileProtectionAt(int tilePosX, int tilePosY, Qt::CheckState protection);
    bool changeSubtileProtectionAt(int posx, int posy, int protection);
    bool needsProtectionAt(int posx, int posy) const;
    bool hasContentAt(int posx, int posy) const;

private:
    D1DUN_TYPE type = D1DUN_TYPE::NORMAL;
    QString dunFilePath;
    D1Pal *pal;
    D1Tileset *tileset;
    D1Min *min;
    D1Til *til;
    D1Sla *sla;
    D1Gfx *cls;
    bool modified;
    uint8_t numLayers;
    int width;
    int height;
    std::vector<std::vector<int>> tiles;
    std::vector<std::vector<int>> subtiles;
    std::vector<std::vector<Qt::CheckState>> tileProtections;
    std::vector<std::vector<int>> subtileProtections;
    std::vector<std::vector<int>> items;
    std::vector<std::vector<MapMonster>> monsters;
    std::vector<std::vector<int>> objects;
    std::vector<std::vector<int>> rooms;

    int defaultTile;
    QString assetPath;
    int levelType; // dungeon_type
    std::vector<CustomObjectStruct> customObjectTypes;
    std::vector<CustomMonsterStruct> customMonsterTypes;
    std::vector<CustomItemStruct> customItemTypes;
    std::vector<ObjectCacheEntry> objectCache;
    std::vector<MonsterCacheEntry> monsterCache;
    std::vector<ItemCacheEntry> itemCache;
    std::vector<std::pair<D1Gfx *, unsigned>> objDataCache;
    std::vector<std::pair<D1Gfx *, unsigned>> monDataCache;
    std::vector<std::pair<D1Gfx *, unsigned>> itemDataCache;
};

extern const DunObjectStruct DunObjConvTbl[128];
extern const DunMonsterStruct DunMonstConvTbl[128];
