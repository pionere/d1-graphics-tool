#pragma once

#include <QDialog>
#include <QGraphicsScene>
#include <QMouseEvent>

#include "herodetailswidget.h"
#include "itemdetailswidget.h"
#include "monsterdetailswidget.h"
#include "pvpdetailswidget.h"
#include "skilldetailswidget.h"

class D1Hero;

namespace Ui {
class SidePanelWidget;
}

class SidePanelWidget : public QDialog {
    Q_OBJECT

public:
    explicit SidePanelWidget(QWidget *parent);
    ~SidePanelWidget();

    void displayFrame();
    void showHero(D1Hero *hero);
    void showHeroItem(D1Hero *hero, int ii);
    void showHeroSkills(D1Hero *hero);
    void showMonsters(D1Hero *hero);
    void showPvP(D1Hero *hero);

private:
    void initialize(/*D1Hero *hero,*/ int mode);

private:
    Ui::SidePanelWidget *ui;

    int mode;
    // D1Hero *hero;
    HeroDetailsWidget *heroDetails = nullptr;
    ItemDetailsWidget *itemDetails = nullptr;
    SkillDetailsWidget *skillDetails = nullptr;
    MonsterDetailsWidget *monsterDetails = nullptr;
    PvPDetailsWidget *pvpDetails = nullptr;
};
