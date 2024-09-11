#include "skilldetailswidget.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>
#include <QWidgetAction>

#include "d1hro.h"
#include "mainwindow.h"
#include "sidepanelwidget.h"
#include "ui_skilldetailswidget.h"

#include "dungeon/all.h"

SkillDetailsWidget::SkillDetailsWidget(SidePanelWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SkillDetailsWidget())
    , view(parent)
{
    ui->setupUi(this);

    // static_assert(lengthof(this->skills) >= NUM_SPELLS, "too many skills to fit to the array");
    int row = 0, column = 0;
    constexpr int COLUMNS = 2;
    for (int s = 0; s < NUM_SPELLS; s++) {
        QLabel *label = new QLabel(spelldata[s].sNameText);
        this->ui->heroSkillGridLayout->addWidget(label, row, 2 * column);
        skillWidgets[s] = new LineEditWidget(this);
        this->ui->heroSkillGridLayout->addWidget(skillWidgets[s], row, 2 * column + 1);

        if (++column == COLUMNS) {
            row++;
            column = 0;
        }
    }
}

SkillDetailsWidget::~SkillDetailsWidget()
{
    delete ui;
}

void SkillDetailsWidget::initialize(D1Hero *h)
{
    this->hero = h;

    // static_assert(lengthof(this->skills) >= NUM_SPELLS, "too many skills to fit to the array");
    for (int s = 0; s < NUM_SPELLS; s++) {
        this->skills[s] = this->hero->getSkillLvl(s);
    }

    // LogErrorF("SkillDetailsWidget init 5");
    this->updateFields();
    // LogErrorF("SkillDetailsWidget init 6");
    this->setVisible(true);
}

void SkillDetailsWidget::updateFields()
{
    // static_assert(lengthof(this->skills) >= NUM_SPELLS, "too many skills to fit to the array");
    for (int s = 0; s < NUM_SPELLS; s++) {
        skillWidgets[s]->setText(QString::number(this->skills[s]));
        GetSkillDesc(this->hero, this->skills[s]);
        skillWidgets[s]->setToolTip(infostr);
    }
}

void SkillDetailsWidget::on_submitButton_clicked()
{
    for (int s = 0; s < NUM_SPELLS; s++) {
        int lvl = skillWidgets[s]->text().toInt();
        if (lvl < 0)
            lvl = 0;
        if (lvl > MAXSPLLEVEL)
            lvl = MAXSPLLEVEL;
        this->hero->setSkillLvl(s, lvl);
    }

    this->setVisible(false);

    dMainWindow().updateWindow();
}

void SkillDetailsWidget::on_cancelButton_clicked()
{
    this->setVisible(false);
}
