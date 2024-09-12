#pragma once

#include <QComboBox>
#include <QDialog>

class D1Dun;
class D1Tileset;

class GenerateDunParam {
public:
    int levelIdx;   // dungeon_level / _setlevels
    int levelNum;   // index in AllLevels (dungeon_level / NUM_FIXLVLS)
    int levelType;   // dungeon_type
    int difficulty; // _difficulty
    int numPlayers;
    bool isMulti;
    bool isHellfire;
    bool useTileset;
    bool patchDunFiles;
    int32_t seed;
    int32_t seedQuest;
    int entryMode;
    int extraRounds;
    bool extraQuestRnd;
};

namespace Ui {
class DungeonGenerateDialog;
}

class DungeonGenerateDialog : public QDialog {
    Q_OBJECT

public:
    explicit DungeonGenerateDialog(QWidget *parent);
    ~DungeonGenerateDialog();

    void initialize(D1Dun *dun, D1Tileset *tileset);

private slots:
    void on_lvlComboBox_activated(int index); 
    void on_lvlTypeComboBox_activated(int index);
    void on_lvlLineEdit_returnPressed();
    void on_lvlLineEdit_escPressed();
    void on_seedLineEdit_returnPressed();
    void on_seedLineEdit_escPressed();
    void on_actionGenerateSeed_triggered();
    void on_questSeedLineEdit_returnPressed();
    void on_questSeedLineEdit_escPressed();
    void on_actionGenerateQuestSeed_triggered();

    void on_generateButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::DungeonGenerateDialog *ui;

    D1Dun *dun;
    D1Tileset *tileset;
    int dunSeed = 0;
    int questSeed = 0;
};
