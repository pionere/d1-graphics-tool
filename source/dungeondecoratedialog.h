#pragma once

#include <QComboBox>
#include <QDialog>

class D1Dun;
class D1Tileset;

class DecorateDunParam {
public:
    int levelIdx;   // dungeon_level / _setlevels
    int levelNum;   // index in AllLevels (dungeon_level / NUM_FIXLVLS)
    int levelType;   // dungeon_type
    int difficulty; // _difficulty
    int numPlayers;
    bool isMulti;
    bool isHellfire;
    bool useTileset;
    bool resetMonsters;
    bool resetObjects;
    bool resetItems;
    bool resetRooms;
    bool addShadows;
    bool addTiles;
    bool addMonsters;
    bool addObjects;
    bool addItems;
    bool addRooms;
    int32_t seed;
    int extraRounds;
};

namespace Ui {
class DungeonDecorateDialog;
}

class DungeonDecorateDialog : public QDialog {
    Q_OBJECT

public:
    explicit DungeonDecorateDialog(QWidget *parent);
    ~DungeonDecorateDialog();

    void initialize(D1Dun *dun, D1Tileset *tileset);

private slots:
    void on_levelComboBox_activated(int index);
    void on_actionGenerateSeed_triggered();

    void on_decorateButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::DungeonDecorateDialog *ui;

    D1Dun *dun;
    D1Tileset *tileset;
};
