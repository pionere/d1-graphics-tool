#include "sidepanelwidget.h"

#include <QMessageBox>

#include "d1hro.h"
#include "ui_sidepanelwidget.h"

#include "dungeon/interfac.h"

SidePanelWidget::SidePanelWidget(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SidePanelWidget())
{
    this->ui->setupUi(this);

    this->mode = -1;
}

SidePanelWidget::~SidePanelWidget()
{
    delete ui;
}

void SidePanelWidget::initialize(D1Hero *h, int m)
{
    this->hero = h;
    if (this->mode == m) {
        return;
    }
    this->mode = m;
    if (this->heroDetails != nullptr) {
        this->heroDetails->setVisible(false);
    }
    if (this->itemDetails != nullptr) {
        this->itemDetails->setVisible(false);
    }
    if (this->skillDetails != nullptr) {
        this->skillDetails->setVisible(false);
    }
    if (this->monsterDetails != nullptr) {
        this->monsterDetails->setVisible(false);
    }

    QWidget *w;
    switch (this->mode) {
    case 0:
        if (this->heroDetails == nullptr) {
            this->heroDetails = new HeroDetailsWidget(this);
        }
        w = this->heroDetails;
        break;
    case 1:
        if (this->itemDetails == nullptr) {
            this->itemDetails = new ItemDetailsWidget(this);
        }
        w = this->itemDetails;
        break;
    case 2:
        if (this->skillDetails == nullptr) {
            this->skillDetails = new SkillDetailsWidget(this);
        }
        w = this->skillDetails;
        break;
    case 3:
        if (this->monsterDetails == nullptr) {
            this->monsterDetails = new MonsterDetailsWidget(this);
        }
        w = this->monsterDetails;
        break;
    }
    QVBoxLayout *layout = this->ui->panelVBoxLayout;
    layout->addWidget(w, 0, Qt::AlignTop);
}

void SidePanelWidget::displayFrame()
{
    if (this->heroDetails != nullptr && this->heroDetails->isVisible())
        this->heroDetails->displayFrame();
    if (this->itemDetails != nullptr && this->itemDetails->isVisible())
        this->itemDetails->displayFrame();
    if (this->skillDetails != nullptr && this->skillDetails->isVisible())
        this->skillDetails->displayFrame();
    if (this->monsterDetails != nullptr && this->monsterDetails->isVisible())
        this->monsterDetails->displayFrame();
}

void SidePanelWidget::showHero(D1Hero *h)
{
    // LogErrorF("SidePanelWidget::showHero 0 %d", ii);
    this->initialize(h, 0);
    // LogErrorF("SidePanelWidget::showHero 1 %d", ii);
    this->heroDetails->initialize(h);
    // LogErrorF("SidePanelWidget::showHero 2 %d", ii);
}

void SidePanelWidget::showHeroItem(D1Hero *h, int ii)
{
    // LogErrorF("SidePanelWidget::showHeroItem 0 %d", ii);
    this->initialize(h, 1);
    // LogErrorF("SidePanelWidget::showHeroItem 1 %d", ii);
    this->itemDetails->initialize(h, ii);
    // LogErrorF("SidePanelWidget::showHeroItem 2 %d", ii);
}

void SidePanelWidget::showHeroSkills(D1Hero *h)
{
    // LogErrorF("SidePanelWidget::showHeroSkills 0 %d", ii);
    this->initialize(h, 2);
    // LogErrorF("SidePanelWidget::showHeroSkills 1 %d", ii);
    this->skillDetails->initialize(h);
    // LogErrorF("SidePanelWidget::showHeroSkills 2 %d", ii);
}

void SidePanelWidget::showMonsters(D1Hero *h)
{
    // LogErrorF("SidePanelWidget::showMonsters 0 %d", ii);
    this->initialize(h, 3);
    // LogErrorF("SidePanelWidget::showMonsters 1 %d", ii);
    this->monsterDetails->initialize(h);
    // LogErrorF("SidePanelWidget::showMonsters 2 %d", ii);
}
