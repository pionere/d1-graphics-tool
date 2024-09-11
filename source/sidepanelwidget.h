#pragma once

#include <QDialog>
#include <QGraphicsScene>
#include <QMouseEvent>

#include "itemdetailswidget.h"
#include "monsterdetailswidget.h"
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

    void initialize(D1Hero *hero, int mode);
    void displayFrame();
    void showHeroItem(D1Hero *hero, int ii);
    void showHeroSkills(D1Hero *hero);
    void showMonsters(D1Hero *hero);

private:
    Ui::SidePanelWidget *ui;

    int mode;
    D1Hero *hero;
    ItemDetailsWidget *itemDetails = nullptr;
    SkillDetailsWidget *skillDetails = nullptr;
    MonsterDetailsWidget *monsterDetails = nullptr;
};
