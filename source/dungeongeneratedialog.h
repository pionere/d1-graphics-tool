#pragma once

#include <QComboBox>
#include <QDialog>

class D1Dun;
class D1Tileset;

class GenerateDunParam {
public:
    int level;      // dungeon_level / _setlevels
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
    void on_actionGenerateSeed_triggered();
    void on_actionGenerateQuestSeed_triggered();

    void on_generateButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::DungeonGenerateDialog *ui;

    D1Dun *dun;
    D1Tileset *tileset;
};
