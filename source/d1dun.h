#pragma once

#include <vector>

#include <QPainter>
#include <QString>

#include "dungeonresourcedialog.h"
#include "openasdialog.h"
#include "saveasdialog.h"

class D1Gfx;
class D1Min;
class D1Pal;
class D1Sol;
class D1Til;
class D1Tmi;
class D1Trn;

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
    DUN_VILE_PRE,            // setmap for the "Archbishop Lazarus" quest before reading the books (Vile2.DUN)
    DUN_VILE_AFT,            // setmap for the "Archbishop Lazarus" quest after reading the books (Vile1.DUN)
    DUN_WARLORD_PRE,         // map tile for the "Warlord" quest before reading the book (Warlord.DUN)
    DUN_WARLORD_AFT,         // map tile for the "Warlord" quest after reading the book (Warlord2.DUN)
} dun_file_index;

enum class D1DUN_TYPE {
    NORMAL,
    RAW,
};

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
    int type;
    int width;
    QString path;
    QString trnPath;
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
    int monsterIndex;
    D1Gfx *monGfx;
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
    bool showRooms;
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

    QImage getImage(const DunDrawParam &params);

    void setPal(D1Pal *pal);

    QString getFilePath() const;
    bool isModified() const;
    quint8 getNumLayers() const;
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
    int getMonsterAt(int posx, int posy) const;
    bool setMonsterAt(int posx, int posy, int monsterIndex);
    int getObjectAt(int posx, int posy) const;
    bool setObjectAt(int posx, int posy, int objectIndex);
    int getRoomAt(int posx, int posy) const;
    bool setRoomAt(int posx, int posy, int roomIndex);

    int getLevelType() const;
    bool setLevelType(int levelType);
    int getDefaultTile() const;
    bool setDefaultTile(int defaultTile);
    bool setAssetPath(QString path);
    QString getAssetPath() const;
    const D1Gfx *getSpecGfx() const;
    QString getItemName(int itemIndex) const;
    QString getMonsterName(int monsterIndex) const;
    QString getObjectName(int objectIndex) const;

    void collectItems(std::vector<std::pair<int, int>> &items) const;
    void collectMonsters(std::vector<std::pair<int, int>> &monsters) const;
    void collectObjects(std::vector<std::pair<int, int>> &objects) const;
    void checkTiles() const;
    void checkItems(D1Sol *sol) const;
    void checkMonsters(D1Sol *sol) const;
    void checkObjects() const;
    bool removeItems();
    bool removeMonsters();
    bool removeObjects();
    bool removeRooms();
    void loadItems(D1Dun *srcDun);
    void loadMonsters(D1Dun *srcDun);
    void loadObjects(D1Dun *srcDun);
    void loadRooms(D1Dun *srcDun);

    bool resetTiles();
    bool resetSubtiles();

    void patch(int dunFileIndex); // dun_file_index

    void loadSpecCels();
    bool reloadTileset(const QString &celFilePath);

    bool addResource(const AddResourceParam &params);
    const std::vector<CustomObjectStruct> &getCustomObjectTypes() const;
    const std::vector<CustomMonsterStruct> &getCustomMonsterTypes() const;
    const std::vector<CustomItemStruct> &getCustomItemTypes() const;
    void clearAssets();

private:
    static void drawDiamond(QImage &image, unsigned sx, unsigned sy, unsigned width, unsigned height, const QColor &color);
    void drawImage(QPainter &dungeon, QImage &backImage, int drawCursorX, int drawCursorY, int dunCursorX, int dunCursorY, const DunDrawParam &params);
    void initVectors(int width, int height);
    void loadObjectGfx(const QString &filePath, int width, ObjectCacheEntry &result);
    void loadMonsterGfx(const QString &filePath, int width, const QString &baseTrnFilePath, const QString &uniqueTrnFilePath, MonsterCacheEntry &result);
    void loadItemGfx(const QString &filePath, int width, ItemCacheEntry &result);
    void loadObject(int objectIndex);
    void loadMonster(int monsterIndex);
    void loadItem(int itemIndex);
    void updateSubtiles(int tilePosX, int tilePosY, int tileRef);
    bool changeTileAt(int tilePosX, int tilePosY, int tileRef);
    bool changeObjectAt(int posx, int posy, int objectIndex);
    bool changeMonsterAt(int posx, int posy, int monsterIndex);
    bool changeItemAt(int posx, int posy, int itemIndex);

private:
    D1DUN_TYPE type = D1DUN_TYPE::NORMAL;
    QString dunFilePath;
    D1Pal *pal;
    D1Tileset *tileset;
    D1Min *min;
    D1Til *til;
    bool modified;
    quint8 numLayers;
    int width;
    int height;
    std::vector<std::vector<int>> tiles;
    std::vector<std::vector<int>> subtiles;
    std::vector<std::vector<int>> items;
    std::vector<std::vector<int>> monsters;
    std::vector<std::vector<int>> objects;
    std::vector<std::vector<int>> rooms;

    int defaultTile;
    QString assetPath;
    int levelType; // dungeon_type
    D1Gfx *specGfx = nullptr;
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
