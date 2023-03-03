#pragma once

#include <vector>

#include <QPainter>
#include <QString>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Til;
class D1Tmi;

#define UNDEF_SUBTILE -1
#define UNDEF_TILE -1

typedef struct ObjectStruct {
    int type;
    int width;
    const char *path;
    const char *name;
    int frameNum;
} ObjectStruct;

typedef struct MonsterStruct
{
    int type,
    int width;
    const char *path;
    const char *trnPath;
    const char *name;
} MonsterStruct;

class DunDrawParam {
public:
    Qt::CheckState tileState;
    bool showItems;
    bool showMonsters;
    bool showObjects;
};

class D1Dun : public QObject {
    Q_OBJECT

public:
    D1Dun() = default;
    ~D1Dun();

    bool load(D1Pal *pal, const QString &dunFilePath, D1Til *til, D1Tmi *tmi, const OpenAsParam &params);
    bool save(const SaveAsParam &params);

    QImage getImage(const DunDrawParam &params) const;

    void setPal(D1Pal *pal);

    QString getFilePath() const;
    bool isModified() const;
    int getWidth() const;
    bool setWidth(int width);
    int getHeight() const;
    bool setHeight(int height);

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
    int getTransvalAt(int posx, int posy) const;
    bool setTransvalAt(int posx, int posy, int transval);

    bool setLevelType(int levelType);
    int getDefaultTile() const;
    bool setDefaultTile(int defaultTile);
    bool setAssetPath(QString path);
    QString getAssetPath() const;

private:
    void drawImage(QPainter &dungeon, QImage &backImage, int drawCursorX, int drawCursorY, int dunCursorX, int dunCursorY, const DunDrawParam &params) const;
    void initVectors(int width, int height);
    D1Gfx *loadObject(int objectIndex);
    D1Gfx *loadMonster(int monsterIndex);
    void clearAssets();
    void updateSubtiles(int tilePosX, int tilePosY, int tileRef);

private:
    QString dunFilePath;
    D1Pal *pal;
    D1Til *til;
    D1Tmi *tmi;
    bool modified;
    int width;
    int height;
    std::vector<std::vector<int>> tiles;
    std::vector<std::vector<int>> subtiles;
    std::vector<std::vector<int>> items;
    std::vector<std::vector<int>> monsters;
    std::vector<std::vector<int>> objects;
    std::vector<std::vector<int>> transvals;

    int defaultTile;
    QString assetPath;
    int levelType = DTYPE_NONE; // dungeon_type
    D1Gfx *specGfx;
    std::vector<std::pair<const ObjectStruct *, D1Gfx *>> objectCache;
    std::vector<std::pair<const MonsterStruct *, D1Gfx *>> monsterCache;
};

extern const ObjectStruct ObjConvTbl[128];
extern const MonsterStruct MonstConvTbl[128];
