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

SkillPushButton::SkillPushButton(int sn, SkillDetailsWidget *parent)
    : QPushButton(parent)
    , sn(sn)
    , sdw(parent)
{
    GetSkillName(sn);
    this->setText(infostr);

    QString style = "border: none;%1";

    const char *color = GetElementColor(GetSkillElement(sn));

    this->setStyleSheet(style.arg(color));

    QObject::connect(this, SIGNAL(clicked()), this, SLOT(on_btn_clicked()));
}

void SkillPushButton::on_btn_clicked()
{
    this->sdw->on_skill_clicked(this->sn);
}

SkillSpinBox::SkillSpinBox(int sn, SkillDetailsWidget *parent)
    : QSpinBox(parent)
    , sn(sn)
    , sdw(parent)
{
    // this->setMinimum(0);
    this->setMaximum(MAXSPLLEVEL);
    this->setMaximumWidth(36);
    this->setKeyboardTracking(false);

    QObject::connect(this, SIGNAL(valueChanged(int)), this, SLOT(on_value_changed(int)));
}

void SkillSpinBox::stepBy(int steps)
{
    int current_value = this->value();
    int next_value;
    if (steps < -1 || steps > 1 || (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier))
        next_value = steps < 0 ? 0 : MAXSPLLEVEL;
    else
        next_value = current_value + steps;
    this->setValue(next_value);
}

void SkillSpinBox::changeValue(int value)
{
    this->blockSignals(true);
    this->setValue(value);
    this->blockSignals(false);
}

void SkillSpinBox::on_value_changed(int value)
{
    // bool userInput = sender() == nullptr;
    // QMessageBox::critical(this, "Error", tr("SkillSpinBox new value %1 sender %2.").arg(value).arg(userInput));
    // if (userInput)
        this->sdw->on_skill_changed(this->sn, value);
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
            this->skillWidgets[sn] = nullptr;
            continue;
        }

        SkillPushButton *label = new SkillPushButton(sn, this);
        this->ui->heroSkillGridLayout->addWidget(label, row, 2 * column);
        this->skillWidgets[sn] = new SkillSpinBox(sn, this);
        this->ui->heroSkillGridLayout->addWidget(this->skillWidgets[sn], row, 2 * column + 1);
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

static QString GetAnimTypeText(int type)
{
    QString result = QApplication::tr("N/A");
    switch (type) {
    case STYPE_FIRE:      result = QApplication::tr("Fire");      break;
    case STYPE_LIGHTNING: result = QApplication::tr("Lightning"); break;
    case STYPE_MAGIC:     result = QApplication::tr("Magic");     break;
    case STYPE_NONE:      result = QApplication::tr("None");      break;
    }
    return result;
}

void SkillDetailsWidget::updateFields()
{
    int sn;
    // static_assert(lengthof(this->skills) >= NUM_SPELLS, "too many skills to fit to the array");
    for (sn = 0; sn < NUM_SPELLS; sn++) {
        if (this->skillWidgets[sn] == nullptr)
            continue;
        this->skillWidgets[sn]->changeValue(this->skills[sn]);
        this->skillWidgets[sn]->setEnabled(spelldata[sn].sBookLvl != SPELL_NA && (this->hero->isHellfire() || sn < NUM_SPELLS_DIABLO));
    }

    sn = this->currentSkill;
    if ((unsigned)sn < NUM_SPELLS) {
        GetSkillName(sn);
        this->ui->skillName->setText(infostr);
        this->ui->skillAnimType->setText(GetAnimTypeText(spelldata[sn].sType));
        int lvl = -1;
        if ((this->hero->getFixedSkills() & SPELL_MASK(sn)) || this->skills[sn] != 0) {
            lvl = this->skills[sn] + this->hero->getSkillLvl(sn) - this->hero->getSkillLvlBase(sn);
            if (lvl < 0)
                lvl = 0;
        }
        this->ui->skillLevel->setText(lvl >= 0 ? QString::number(lvl) : QString());
        this->ui->skillManaCost->setText(lvl >= 0 ? QString::number(GetSkillCost(sn, lvl, this->hero->getLevel())) : QString());

        QString desc = tr("Not available");
        if (lvl >= 0) {
            GetSkillDesc(this->hero, sn, lvl);
            desc = infostr;
        }
        this->ui->skillDesc->setText(desc);
        int sources = this->hero->getSkillSources(sn);
        if (this->skills[sn] != 0)
            sources |= (1 << RSPLTYPE_SPELL);
        else
            sources &= ~(1 << RSPLTYPE_SPELL);
        this->ui->sourceAbilityCheckBox->setChecked((sources & (1 << RSPLTYPE_ABILITY)) != 0);
        this->ui->sourceMemCheckBox->setChecked((sources & (1 << RSPLTYPE_SPELL)) != 0);
        this->ui->sourceInvCheckBox->setChecked((sources & (1 << RSPLTYPE_INV)) != 0);
        this->ui->sourceItemCheckBox->setChecked((sources & (1 << RSPLTYPE_CHARGES)) != 0);

        int mn = -1;
        if (spelldata[sn].sType != STYPE_NONE || (spelldata[sn].sUseFlags & SFLAG_RANGED)) {
            mn = spelldata[sn].sMissile;
        }
        unsigned flags = mn >= 0 ? missiledata[mn].mdFlags : 0;
        this->ui->misAreaCheckBox->setChecked((flags & MIF_AREA) != 0);
        this->ui->misNoBlockCheckBox->setChecked((flags & MIF_NOBLOCK) != 0);
        this->ui->misDotCheckBox->setChecked((flags & MIF_DOT) != 0);
        this->ui->misLeadCheckBox->setChecked((flags & MIF_LEAD) != 0);
        this->ui->misShroudCheckBox->setChecked((flags & MIF_SHROUD) != 0);
        this->ui->misArrowCheckBox->setChecked((flags & MIF_ARROW) != 0);

        flags = mn >= 0 ? missiledata[GetBaseMissile(mn)].mdFlags : 0;
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

void SkillDetailsWidget::on_skill_changed(int sn, int value)
{
    this->skills[sn] = value;

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
        this->skills[sn] = ((this->hero->isHellfire() || sn < NUM_SPELLS_DIABLO) && spelldata[sn].sBookLvl != SPELL_NA) ? MAXSPLLEVEL : 0;
    }
    this->updateFields();
}

void SkillDetailsWidget::on_submitButton_clicked()
{
    for (int sn = 0; sn < NUM_SPELLS; sn++) {
        if (this->skillWidgets[sn] == nullptr)
            continue;
        // int lvl = this->skillWidgets[sn]->text().toInt();
        int lvl = this->skillWidgets[sn]->value();
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
