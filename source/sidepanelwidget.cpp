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
    // clear the layout
    QVBoxLayout *layout = this->ui->panelVBoxLayout;
    /*QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        child->widget()->deleteLater(); // delete the widget
        delete child;           // delete the layout item
    }*/
    for (int i = layout->count() - 1; i >= 0; i--) {
        QWidget *w = layout->itemAt(i)->widget();
        if (w != nullptr) {
            w->deleteLater();
        }
    }
    QWidget *w;
    switch (this->mode) {
    case 0:
        // if (this->itemDetails == nullptr) {
            this->itemDetails = new ItemDetailsWidget(this);
        // }
        w = this->itemDetails;
        break;
    case 1:
        // if (this->itemDetails == nullptr) {
            this->skillDetails = new SkillDetailsWidget(this);
        // }
        w = this->skillDetails;
        break;
    }
    layout->addWidget(w, 0, Qt::AlignTop);
}

void SidePanelWidget::showHeroItem(D1Hero *h, int ii)
{
    // LogErrorF("SidePanelWidget::showHeroItem 0 %d", ii);
    this->initialize(h, 0);
    // LogErrorF("SidePanelWidget::showHeroItem 1 %d", ii);
    this->itemDetails->initialize(h, ii);
    // LogErrorF("SidePanelWidget::showHeroItem 2 %d", ii);
}

void SidePanelWidget::showHeroSkills(D1Hero *h)
{
    // LogErrorF("SidePanelWidget::showHeroSkills 0 %d", ii);
    this->initialize(h, 1);
    // LogErrorF("SidePanelWidget::showHeroSkills 1 %d", ii);
    this->skillDetails->initialize(h);
    // LogErrorF("SidePanelWidget::showHeroSkills 2 %d", ii);
}
