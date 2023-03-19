#pragma once

#include <QComboBox>
#include <QDialog>

class D1Dun;

#ifndef __ALL_H__
typedef enum lvl_entry {
    ENTRY_MAIN,
    ENTRY_PREV,
    ENTRY_SETLVL,
    ENTRY_RTNLVL,
    ENTRY_LOAD,
    ENTRY_WARPLVL,
    ENTRY_TWARPDN,
    ENTRY_TWARPUP,
    ENTRY_RETOWN,
} lvl_entry;
#endif

class GenerateDunParam {
public:
    int level;      // dungeon_level / _setlevels
    int difficulty; // _difficulty
    bool isMulti;
    bool isHellfire;
    int32_t seed;
    int32_t seedQuest;
    int entryMode;
};

namespace Ui {
class DungeonGenerateDialog;
}

class DungeonGenerateDialog : public QDialog {
    Q_OBJECT

public:
    explicit DungeonGenerateDialog(QWidget *parent);
    ~DungeonGenerateDialog();

    void initialize(D1Dun *dun);

private slots:
    void on_generateButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::DungeonGenerateDialog *ui;

    D1Dun *dun;
};
