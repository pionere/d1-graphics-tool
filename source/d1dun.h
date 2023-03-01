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

class D1Dun : public QObject {
    Q_OBJECT

public:
    D1Dun() = default;
    ~D1Dun() = default;

    bool load(const QString &dunFilePath, D1Til *til, D1Tmi *tmi, const OpenAsParam &params);
    bool save(const SaveAsParam &params);

    QImage getImage(Qt::CheckState tileState, bool showItems, bool showMonsters, bool showObjects) const;

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

private:
    void drawImage(QPainter &dungeon, QImage &backImage, int drawCursorX, int drawCursorY, int dunCursorX, int dunCursorY, Qt::CheckState tileState, bool showItems, bool showMonsters, bool showObjects) const;
    void updateSubtiles(int posx, int posy, int tileRef);

private:
    QString dunFilePath;
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
};
