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


static int GetMissileResist(int mn)
{
    return missiledata[GetBaseMissile(mn)].mResist;
}

SkillPushButton::SkillPushButton(int sn, SkillDetailsWidget *parent)
    : QPushButton(spelldata[sn].sNameText, parent)
    , sn(sn)
    , sdw(parent)
{
    /*QString style = "border: none;%1";

    const char *type = "";
    if (spelldata[sn].sType != STYPE_NONE || (spelldata[sn].sUseFlags & SFLAG_RANGED)) {
        int rs = GetMissileResist(spelldata[sn].sMissile);
        if (rs == MISR_FIRE) {
            type = "color:red;";
        }
        else if (rs == MISR_MAGIC) {
            type = "color:blue;";
        }
        else if (rs == MISR_LIGHTNING) {
            type = "color:#FFBF00;"; // amber
        }
        else if (rs == MISR_ACID) {
            type = "color:green;";
        }
        else if (rs == MISR_PUNCTURE) {
            type = "color:olive;";
        }
        else if (rs == MISR_BLUNT) {
            type = "color:maroon;";
        }
    }
    this->setStyleSheet(style.arg(type));*/

    QObject::connect(this, SIGNAL(clicked()), this, SLOT(on_btn_clicked()));
}

void SkillPushButton::on_btn_clicked()
{
    this->sdw->on_skill_clicked(this->sn);
}

SkillDetailsWidget::SkillDetailsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SkillDetailsWidget())
{
    ui->setupUi(this);

    // static_assert(lengthof(this->skills) >= NUM_SPELLS, "too many skills to fit to the array");
    int row = 0, column = 0;
    constexpr int COLUMNS = 3;
    for (int sn = 0; sn < NUM_SPELLS; sn++) {
        if (spelldata[sn].sBookLvl == SPELL_NA && spelldata[sn].sStaffLvl == SPELL_NA && !SPELL_RUNE(sn)) {
            skillWidgets[sn] = nullptr;
            continue;
        }

        /*QLabel *label = new QLabel(spelldata[sn].sNameText);
        int rs = GetMissileResist(spelldata[sn].sMissile);
        if (rs == MISR_FIRE) {
            label->setStyleSheet("color:red;");
        } else if (rs == MISR_MAGIC) {
            label->setStyleSheet("color:blue;");
        } else if (rs == MISR_LIGHTNING) {
            label->setStyleSheet("color:#FFBF00;"); // amber
        } else if (rs == MISR_ACID) {
            label->setStyleSheet("color:green;");
        } else if (rs == MISR_PUNCTURE) {
            label->setStyleSheet("color:olive;");
        } else if (rs == MISR_BLUNT) {
            label->setStyleSheet("color:maroon;");
        }*/
        SkillPushButton *label = new SkillPushButton(sn, this);
        this->ui->heroSkillGridLayout->addWidget(label, row, 2 * column);
        // skillWidgets[sn] = new LineEditWidget(this);
        // skillWidgets[sn]->setCharWidth(2);
        skillWidgets[sn] = new QSpinBox(this);
        // skillWidgets[sn]->setMinimum(0);
        skillWidgets[sn]->setMaximum(MAXSPLLEVEL);
        skillWidgets[sn]->setMaximumWidth(36);
        skillWidgets[sn]->setEnabled(spelldata[sn].sBookLvl != SPELL_NA);
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
    int sn;
    // static_assert(lengthof(this->skills) >= NUM_SPELLS, "too many skills to fit to the array");
    for (sn = 0; sn < NUM_SPELLS; sn++) {
        if (skillWidgets[sn] == nullptr)
            continue;
        skillWidgets[sn]->setValue(this->skills[sn]);
    }

    sn = this->currentSkill;
    if ((unsigned)sn < NUM_SPELLS) {
        this->ui->skillName->setText(spelldata[sn].sNameText);
        this->ui->skillType->setCurrentIndex(spelldata[sn].sType);
        int lvl = this->hero->getSkillLvl(sn);
        this->ui->skillLevel->setText(QString::number(lvl));
        this->ui->skillManaCost->setText(QString::number(this->hero->getSkillCost(sn)));

        GetSkillDesc(this->hero, sn, lvl);
        this->ui->skillDesc->setText(infostr);

        int mn = spelldata[sn].sMissile;
        unsigned flags = missiledata[mn].mdFlags;
        this->ui->misAreaCheckBox->setChecked((flags & MIF_AREA) != 0);
        this->ui->misNoBlockCheckBox->setChecked((flags & MIF_NOBLOCK) != 0);
        this->ui->misDotCheckBox->setChecked((flags & MIF_DOT) != 0);
        this->ui->misLeadCheckBox->setChecked((flags & MIF_LEAD) != 0);
        this->ui->misShroudCheckBox->setChecked((flags & MIF_SHROUD) != 0);
        this->ui->misArrowCheckBox->setChecked((flags & MIF_ARROW) != 0);

        int bmn = GetBaseMissile(mn);
        flags = missiledata[bmn].mdFlags;
        this->ui->misBaseAreaCheckBox->setChecked((flags & MIF_AREA) != 0);
        this->ui->misBaseNoBlockCheckBox->setChecked((flags & MIF_NOBLOCK) != 0);
        this->ui->misBaseDotCheckBox->setChecked((flags & MIF_DOT) != 0);
        this->ui->misBaseLeadCheckBox->setChecked((flags & MIF_LEAD) != 0);
        this->ui->misBaseShroudCheckBox->setChecked((flags & MIF_SHROUD) != 0);
        this->ui->misBaseArrowCheckBox->setChecked((flags & MIF_ARROW) != 0);
    }
}

void SkillDetailsWidget::on_skill_clicked(int sn)
{
    this->currentSkill = sn;

    this->updateFields();
}

void SkillDetailsWidget::on_resetButton_clicked()
{
    for (int sn = 0; sn < NUM_SPELLS; sn++) {
        this->skills[sn] = 0;
    }
    this->updateFields();
}

void SkillDetailsWidget::on_maxButton_clicked()
{
    for (int sn = 0; sn < NUM_SPELLS; sn++) {
        this->skills[sn] = spelldata[sn].sBookLvl == SPELL_NA ? 0 : MAXSPLLEVEL;
    }
    this->updateFields();
}

void SkillDetailsWidget::on_submitButton_clicked()
{
    for (int sn = 0; sn < NUM_SPELLS; sn++) {
        if (skillWidgets[sn] == nullptr)
            continue;
        // int lvl = skillWidgets[sn]->text().toInt();
        int lvl = skillWidgets[sn]->value();
        // if (lvl < 0)
        //     lvl = 0;
        // if (lvl > MAXSPLLEVEL)
        //     lvl = MAXSPLLEVEL;
        this->hero->setSkillLvlBase(sn, lvl);
    }

    this->setVisible(false);

    dMainWindow().updateWindow();
}

void SkillDetailsWidget::on_cancelButton_clicked()
{
    this->setVisible(false);
}
