#pragma once

#include <vector>

#include <QString>

#include "openasdialog.h"
#include "saveasdialog.h"

class D1Til;
class D1Tmi;

class D1Dun : public QObject {
    Q_OBJECT

public:
    D1Dun() = default;
    ~D1Dun() = default;

    bool load(const QString &dunFilePath, D1Til *til, D1Tmi *tmi, const OpenAsParam &params);
    bool save(const SaveAsParam &params);

    QString getFilePath() const;
    bool isModified() const;
    int getWidth() const;
    void setWidth(int width);
    int getHeight() const;
    void setHeight(int height);

    int getTileAt(int posx, int posy) const;
    void setTileAt(int posx, int posy, int tileRef);
    int getItemAt(int posx, int posy) const;
    void setItemAt(int posx, int posy, int itemIndex);
    int getMonsterAt(int posx, int posy) const;
    void setMonsterAt(int posx, int posy, int monsterIndex);
    int getObjectAt(int posx, int posy) const;
    void setObjectAt(int posx, int posy, int objectIndex);
    int getTransvalAt(int posx, int posy) const;
    void setTransvalAt(int posx, int posy, int transval);

private:
    QString dunFilePath;
    D1Til *til;
    D1Tmi *tmi;
    bool modified;
    int width;
    int height;
    std::vector<std::vector<int>> tiles;
    std::vector<std::vector<int>> items;
    std::vector<std::vector<int>> monsters;
    std::vector<std::vector<int>> objects;
    std::vector<std::vector<int>> transvals;
};
