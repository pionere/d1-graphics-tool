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
    for (int sn = 0; sn < NUM_SPELLS; sn++) {
        if (spelldata[sn].sBookLvl == SPELL_NA && spelldata[sn].sStaffLvl == SPELL_NA && !SPELL_RUNE(sn)) {
            skillWidgets[sn] = nullptr;
            continue;
        }

        QLabel *label = new QLabel(spelldata[sn].sNameText);
        this->ui->heroSkillGridLayout->addWidget(label, row, 2 * column);
        skillWidgets[sn] = new LineEditWidget(this);
        skillWidgets[sn].setEnabled(spelldata[sn].sBookLvl != SPELL_NA);
        this->ui->heroSkillGridLayout->addWidget(skillWidgets[sn], row, 2 * column + 1);

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
    for (int sn = 0; sn < NUM_SPELLS; sn++) {
        this->skills[sn] = this->hero->getSkillLvlBase(sn);
    }

    // LogErrorF("SkillDetailsWidget init 5");
    this->updateFields();
    // LogErrorF("SkillDetailsWidget init 6");
    this->setVisible(true);
}

void SkillDetailsWidget::displayFrame()
{
    this->updateFields();
}

void SkillDetailsWidget::updateFields()
{
    // static_assert(lengthof(this->skills) >= NUM_SPELLS, "too many skills to fit to the array");
    for (int sn = 0; sn < NUM_SPELLS; sn++) {
        if (skillWidgets[sn] == nullptr)
            continue;
        int sm = this->skills[sn];
        skillWidgets[sn]->setText(QString::number(sm));
        if (sm <= 0) {
            skillWidgets[sn]->setToolTip("");
        } else {
            sm += this->hero->getSkillLvl(sn) - this->hero->getSkillLvlBase(sn);
            GetSkillDesc(this->hero, sn, this->skills[sn] + sm);
            skillWidgets[sn]->setToolTip(infostr);
        }
    }
}

void SkillDetailsWidget::on_submitButton_clicked()
{
    for (int sn = 0; sn < NUM_SPELLS; sn++) {
        if (skillWidgets[sn] == nullptr)
            continue;
        int lvl = skillWidgets[sn]->text().toInt();
        if (lvl < 0)
            lvl = 0;
        if (lvl > MAXSPLLEVEL)
            lvl = MAXSPLLEVEL;
        this->hero->setSkillLvlBase(sn, lvl);
    }

    this->setVisible(false);

    dMainWindow().updateWindow();
}

void SkillDetailsWidget::on_cancelButton_clicked()
{
    this->setVisible(false);
}
