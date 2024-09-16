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

static int GetBaseMissile(int mn)
{
    switch (mn) {
    case MIS_ARROW:
    case MIS_PBARROW:
    case MIS_ASARROW:
    case MIS_MLARROW:
    case MIS_PCARROW:
    case MIS_FIREBOLT:
    case MIS_FIREBALL:
    case MIS_HBOLT:
    case MIS_FLARE:
    case MIS_SNOWWICH:
    case MIS_HLSPWN:
    case MIS_SOLBRNR:
    case MIS_MAGE:
    case MIS_MAGMABALL:
    case MIS_ACID:
    case MIS_ACIDPUD:
    case MIS_EXACIDP:
    case MIS_EXFIRE:
    case MIS_EXFBALL:
    case MIS_EXLGHT:
    case MIS_EXMAGIC:
    case MIS_EXACID:
    case MIS_EXHOLY:
    case MIS_EXFLARE:
    case MIS_EXSNOWWICH:
    case MIS_EXHLSPWN:
    case MIS_EXSOLBRNR:
    case MIS_EXMAGE:
    case MIS_POISON:
    case MIS_WIND:
    case MIS_LIGHTBALL: break;
    case MIS_LIGHTNINGC: mn = MIS_LIGHTNING; break;
    case MIS_LIGHTNING: break;
    case MIS_LIGHTNINGC2: mn = MIS_LIGHTNING; break;
    case MIS_LIGHTNING2: break;
    case MIS_BLOODBOILC: mn = MIS_BLOODBOIL; break;
    case MIS_BLOODBOIL: break;
    case MIS_SWAMPC: mn = MIS_SWAMP; break;
    case MIS_SWAMP:
    case MIS_TOWN:
    case MIS_RPORTAL:
    case MIS_FLASH:
    case MIS_FLASH2:
    case MIS_CHAIN:
        //case MIS_BLODSTAR:	// TODO: Check beta
        //case MIS_BONE:		// TODO: Check beta
        //case MIS_METLHIT:	// TODO: Check beta
    case MIS_RHINO:
    case MIS_CHARGE:
    case MIS_TELEPORT:
    case MIS_RNDTELEPORT:
        //case MIS_FARROW:
        //case MIS_DOOMSERP:
    case MIS_STONE:
    case MIS_SHROUD: break;
        //case MIS_INVISIBL:
    case MIS_GUARDIAN: mn = MIS_FIREBOLT; break;
    case MIS_GOLEM:
        //case MIS_ETHEREALIZE:
    case MIS_BLEED: break;
        //case MIS_EXAPOCA:
    case MIS_FIREWALLC: mn = MIS_FIREWALL; break;
    case MIS_FIREWALL: break;
    case MIS_FIREWAVEC: mn = MIS_FIREWAVE; break;
    case MIS_FIREWAVE:
    case MIS_METEOR: break;
    case MIS_LIGHTNOVAC: mn = MIS_LIGHTBALL; break;
        //case MIS_APOCAC:
    case MIS_HEAL:
    case MIS_HEALOTHER:
    case MIS_RESURRECT:
    case MIS_ATTRACT:
    case MIS_TELEKINESIS:
        //case MIS_LARROW:
    case MIS_OPITEM:
    case MIS_REPAIR:
    case MIS_DISARM: break;
    case MIS_INFERNOC: mn = MIS_FIREWALL; break;
    case MIS_INFERNO:
        //case MIS_FIRETRAP:
    case MIS_BARRELEX: break;
        //case MIS_FIREMAN:	// TODO: Check beta
        //case MIS_KRULL:		// TODO: Check beta
    case MIS_CBOLTC: mn = MIS_FIREWALL; break;
    case MIS_CBOLT:
    case MIS_ELEMENTAL:
        //case MIS_BONESPIRIT:
    case MIS_APOCAC2:
    case MIS_EXAPOCA2:
    case MIS_MANASHIELD:
    case MIS_INFRA:
    case MIS_RAGE: break;
#ifdef HELLFIRE
        //case MIS_LIGHTWALLC:
        //case MIS_LIGHTWALL:
        //case MIS_FIRENOVAC:
        //case MIS_FIREBALL2:
        //case MIS_REFLECT:
    case MIS_FIRERING: mn = MIS_FIREWALL;  break;
        //case MIS_MANATRAP:
        //case MIS_LIGHTRING:
    case MIS_RUNEFIRE: mn = MIS_FIREEXP; break;
    case MIS_RUNELIGHT: mn = MIS_LIGHTNING; break;
    case MIS_RUNENOVA: mn = MIS_LIGHTBALL; break;
    case MIS_RUNEWAVE: mn = MIS_FIREWAVE; break;
    case MIS_RUNESTONE: mn = MIS_STONE; break;
    case MIS_FIREEXP:
    case MIS_HORKDMN:
    case MIS_PSYCHORB:
    case MIS_LICH:
    case MIS_BONEDEMON:
    case MIS_ARCHLICH:
    case MIS_NECROMORB:
    case MIS_EXPSYCHORB:
    case MIS_EXLICH:
    case MIS_EXBONEDEMON:
    case MIS_EXARCHLICH:
    case MIS_EXNECROMORB: break;
#endif
    }
    return mn;
}

static int GetMissileResist(int mn)
{
    return missiledata[GetBaseMissile(mn)].mResist;
}

SkillPushButton::SkillPushButton(int sn, QWidget *parent)
    : QPushButton(spelldata[sn].sNameText, parent)
    , sn(sn)
{
    QString style = "border: none;%1";

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
    this->setStyleSheet(style.arg(type));

    QObject::connect(this, SIGNAL(clicked_signal()), this, SLOT(on_btn_clicked()));
}

void SkillPushButton::on_btn_clicked()
{
    QObject *view = this->parent();
    SkillDetailsWidget *celView = qobject_cast<SkillDetailsWidget *>(view);
    if (celView != nullptr) {
        celView->on_skill_clicked(this->sn);
    }
}

SkillDetailsWidget::SkillDetailsWidget(SidePanelWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SkillDetailsWidget())
    , view(parent)
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
    // static_assert(lengthof(this->skills) >= NUM_SPELLS, "too many skills to fit to the array");
    for (int sn = 0; sn < NUM_SPELLS; sn++) {
        if (skillWidgets[sn] == nullptr)
            continue;
        int sm = this->skills[sn];
        // skillWidgets[sn]->setText(QString::number(sm));
        skillWidgets[sn]->setValue(sm);
        if (sm <= 0) {
            skillWidgets[sn]->setToolTip("");
        } else {
            sm += this->hero->getSkillLvl(sn) - this->hero->getSkillLvlBase(sn);
            GetSkillDesc(this->hero, sn, this->skills[sn] + sm);
            skillWidgets[sn]->setToolTip(infostr);
        }
    }
}

void SkillDetailsWidget::on_skill_clicked(int sn)
{
    this->ui->skillName->setText(spelldata[sn].sNameText);
    this->ui->skillType->setCurrentIndex(spelldata[sn].sType);

    this->ui->skillManaCost->setText(QString::number(this->hero->getSkillCost(sn)));

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
