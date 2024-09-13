#include "monsterdetailswidget.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>
#include <QWidgetAction>

#include "d1hro.h"
#include "mainwindow.h"
#include "sidepanelwidget.h"
#include "ui_monsterdetailswidget.h"

#include "dungeon/all.h"

MonsterDetailsWidget::MonsterDetailsWidget(SidePanelWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MonsterDetailsWidget())
{
    ui->setupUi(this);

    QObject::connect(this->ui->dunLevelEdit, SIGNAL(cancel_signal()), this, SLOT(on_dunLevelEdit_escPressed()));
    QObject::connect(this->ui->dunLevelBonusEdit, SIGNAL(cancel_signal()), this, SLOT(on_dunLevelBonusEdit_escPressed()));
}

MonsterDetailsWidget::~MonsterDetailsWidget()
{
    delete ui;
}

void MonsterDetailsWidget::initialize(D1Hero *h)
{
    this->hero = h;

    this->updateFields();

    this->setVisible(true);
}

void MonsterDetailsWidget::displayFrame()
{
    this->updateFields();
}

static QString MonLeaderText(BYTE flag, BYTE packsize)
{
    QString result;
    switch (flag) {
    case MLEADER_NONE:
        result = "-";
        break;
    case MLEADER_PRESENT:
        result = QApplication::tr("Minion");
        break;
    case MLEADER_AWAY:
        result = QApplication::tr("Lost");
        break;
    case MLEADER_SELF:
        // result = QApplication::tr("Leader(%1)").arg(packsize);
        result = QApplication::tr("Leader");
        break;
    default:
        result = "???";
        break;
    }
    return result;
}

static void MonResistText(unsigned resist, unsigned idx, QProgressBar *label)
{
    int value = 0;
    QString tooltip;
    switch ((resist >> idx) & 3) {
    case MORT_NONE:
        value = 0;
        tooltip = "Vulnerable to %1 damage";
        break;
    case MORT_PROTECTED:
        value = 50 - 50 / 4;
        tooltip = "Protected against %1 damage";
        break;
    case MORT_RESIST:
        value = 75;
        tooltip = "Resists %1 damage";
        break;
    case MORT_IMMUNE:
        value = 100;
        tooltip = "Immune to %1 damage";
        break;
    default:
        tooltip = "???";
        break;
    }
    const char *type;
    switch (idx) {
    case MORS_IDX_SLASH:     type = "slash";     break;
    case MORS_IDX_BLUNT:     type = "blunt";     break;
    case MORS_IDX_PUNCTURE:  type = "puncture";  break;
    case MORS_IDX_FIRE:      type = "fire";      break;
    case MORS_IDX_LIGHTNING: type = "lightning"; break;
    case MORS_IDX_MAGIC:     type = "magic";     break;
    case MORS_IDX_ACID:      type = "acid";      break;
    }
    label->setValue(value);
    label->setToolTip(tooltip.arg(type));
}

static void displayDamage(QLabel *label, int minDam, int maxDam)
{
    if (maxDam != 0) {
        if (minDam != maxDam)
            label->setText(QString("%1-%2").arg(minDam).arg(maxDam));
        else
            label->setText(QString("%1").arg(minDam));
    } else {
        label->setText(QString("-"));
    }
}

static RANGE monLevelRange(int mtype, int dtype)
{
    RANGE result = { DLV_INVALID, DLV_INVALID };
    if (dtype != DTYPE_TOWN) {
        int n;
        for (n = 0; n < NUM_FIXLVLS; n++) {
            if (AllLevels[n].dType != dtype) continue;
            for (int m = 0; AllLevels[n].dMonTypes[m] != MT_INVALID; m++) {
                if (AllLevels[n].dMonTypes[m] == mtype) {
                    if (result.from == DLV_INVALID)
                        result.from = n;
                    result.to == n;
                    break;
                }
            }
        }
    } else {
        result = { 0, NUM_FIXLVLS - 1};
    }
    return result;
}

static int uniqMonLevel(const UniqMonData &mon)
{
    int ml = mon.muLevelIdx;
    switch (i) {
    case UMT_GARBUD:                          break;
    case UMT_SKELKING:   ml = DLV_CATHEDRAL3; break;
    case UMT_ZHAR:                            break;
    case UMT_SNOTSPIL:   ml = DLV_CATHEDRAL4; break;
    case UMT_LAZARUS:    ml = DLV_HELL3;      break;
    case UMT_RED_VEX:    ml = DLV_HELL3;      break;
    case UMT_BLACKJADE:  ml = DLV_HELL3;      break;
    case UMT_LACHDAN:                         break;
    case UMT_WARLORD:    ml = DLV_HELL1;      break;
    case UMT_BUTCHER:    ml = DLV_CATHEDRAL2; break;
    case UMT_DIABLO:     ml = DLV_HELL4;      break;
    case UMT_ZAMPHIR:                         break;
#ifdef HELLFIRE
    case UMT_HORKDMN:                         break;
    case UMT_DEFILER:                         break;
    case UMT_NAKRUL:     ml = DLV_CRYPT4;     break;
#endif
    }
    // assert(ml != 0);
    return ml;
}

void MonsterDetailsWidget::updateFields()
{
    QComboBox *typesComboBox = this->ui->monTypeComboBox;
    int mi = typesComboBox->currentData().value<int>();
    typesComboBox->clear();
    
    int dtype = this->ui->dunTypeComboBox->currentIndex();
    for (int i = 0; i < NUM_MTYPES; i++) {
        RANGE range = monLevelRange(i, dtype);
        if (range.from == DLV_INVALID)
            continue;
        /*if (loc != DTYPE_TOWN) {
            int n;
            for (n = 0; n < NUM_FIXLVLS; n++) {
                if (AllLevels[n].dType != loc) continue;
                int m;
                for (m = 0; AllLevels[n].dMonTypes[m] != MT_INVALID; m++) {
                    if (AllLevels[n].dMonTypes[m] == i) break;
                }
                if (AllLevels[n].dMonTypes[m] != MT_INVALID) break;
            }
            if (n >= NUM_FIXLVLS) continue;
        }*/
        typesComboBox->addItem(monsterdata[i].mName, QVariant::fromValue(i));
    }
    for (int i = 0; ; i++) {
        const UniqMonData &mon = uniqMonData[i];
        if (mon.mtype == MT_INVALID)
            break;
        if (dtype != DTYPE_TOWN) {
            int ml = uniqMonLevel(mon);
            switch (dtype) {
            case DTYPE_CATHEDRAL: if (ml < DLV_CATHEDRAL1 || ml > DLV_CATHEDRAL4) continue; break;
            case DTYPE_CATACOMBS: if (ml < DLV_CATACOMBS1 || ml > DLV_CATACOMBS4) continue; break;
            case DTYPE_CAVES:     if (ml < DLV_CAVES1 || ml > DLV_CAVES4)         continue; break;
            case DTYPE_HELL:      if (ml < DLV_HELL1 || ml > DLV_HELL4)           continue; break;
            case DTYPE_CRYPT:     if (ml < DLV_CRYPT1 || ml > DLV_CRYPT4)         continue; break;
            case DTYPE_NEST:      if (ml < DLV_NEST1 || ml > DLV_NEST4)           continue; break;
            }
        }
        typesComboBox->addItem(QString("**%1**").arg(mon.mName), QVariant::fromValue(-(i + 1)));
    }
    // typesComboBox->adjustSize();
    mi = typesComboBox->findData(mi);
    if (mi < 0) mi = 0;
    typesComboBox->setCurrentIndex(mi);

    int type = typesComboBox->currentData().value<int>();
    if (type < 0) {
        int lvl = uniqMonLevel(uniqMonData[-(type + 1)]);
        typesComboBox->setToolTip(tr("Dungeon Level %1").arg(lvl));
    } else {
        RANGE range = monLevelRange(type, dtype);
        typesComboBox->setToolTip(tr("Dungeon Level %1-%2").arg(range.from).arg(range.to));
    }

    bool minion = type < 0 && (uniqMonData[-(type + 1)].mUnqFlags & UMF_GROUP) != 0;
    this->ui->minionCheckBox->setVisible(minion);
    minion &= this->ui->minionCheckBox->isChecked();

    int lvl = this->dunLevel;
    int numplrs = this->ui->plrCountSpinBox->value();
    int difficulty = this->ui->difficutlyComboBox->currentIndex();
    int lvlbonus = this->dunLevelBonus;

    this->ui->dunLevelEdit->setText(QString::number(lvl));
    this->ui->dunLevelBonusEdit->setText(QString::number(lvlbonus));

    if (type < 0)
        InitUniqMonster(-(type + 1), lvl, numplrs, difficulty, lvlbonus, minion);
    else
        InitLvlMonster(type, lvl, numplrs, difficulty, lvlbonus);

    MonsterStruct *mon = &monsters[MAX_MINIONS];

    const char* color = "black";
    if (mon->_mNameColor == COL_BLUE)
        color = "blue";
    else if (mon->_mNameColor == COL_GOLD)
        color = "orange";
    this->ui->monsterName->setText(QString("<u><font color='%1'>%2</font></u>").arg(color).arg(mon->_mName));
    this->ui->monsterLevel->setText(tr("(Level %1)").arg(mon->_mLevel));
    this->ui->monsterExp->setText(QString::number(mon->_mExp));
    this->ui->monsterStatus->setText(MonLeaderText(mon->_mleaderflag, mon->_mpacksize));

    // MonsterAI _mAI;
    this->ui->monsterInt->setText(QString::number(mon->_mAI.aiInt));

    this->ui->monsterHit->setText(QString::number(mon->_mHit));
    displayDamage(this->ui->monsterDamage, mon->_mMinDamage, mon->_mMaxDamage);
    this->ui->monsterHit2->setText(QString::number(mon->_mHit2));
    displayDamage(this->ui->monsterDamage2, mon->_mMinDamage2, mon->_mMaxDamage2);
    this->ui->monsterMagic->setText(QString::number(mon->_mMagic));

    displayDamage(this->ui->monsterHp, mon->_mhitpoints, mon->_mmaxhp);
    this->ui->monsterArmorClass->setText(QString::number(mon->_mArmorClass));
    this->ui->monsterEvasion->setText(QString::number(mon->_mEvasion));

    unsigned res = mon->_mMagicRes;
    MonResistText(res, MORS_IDX_SLASH, this->ui->monsterResSlash);
    MonResistText(res, MORS_IDX_BLUNT, this->ui->monsterResBlunt);
    MonResistText(res, MORS_IDX_PUNCTURE, this->ui->monsterResPunct);
    MonResistText(res, MORS_IDX_FIRE, this->ui->monsterResFire);
    MonResistText(res, MORS_IDX_LIGHTNING, this->ui->monsterResLight);
    MonResistText(res, MORS_IDX_MAGIC, this->ui->monsterResMagic);
    MonResistText(res, MORS_IDX_ACID, this->ui->monsterResAcid);

    unsigned flags = mon->_mFlags;
    this->ui->monsterHiddenCheckBox->setChecked((flags & MFLAG_HIDDEN) != 0);
    this->ui->monsterGargCheckBox->setChecked((flags & MFLAG_GARG_STONE) != 0);
    this->ui->monsterStealCheckBox->setChecked((flags & MFLAG_LIFESTEAL) != 0);
    this->ui->monsterOpenCheckBox->setChecked((flags & MFLAG_CAN_OPEN_DOOR) != 0);
    this->ui->monsterSearchCheckBox->setChecked((flags & MFLAG_SEARCH) != 0);
    this->ui->monsterNoStoneCheckBox->setChecked((flags & MFLAG_NOSTONE) != 0);
    this->ui->monsterBleedCheckBox->setChecked((flags & MFLAG_CAN_BLEED) != 0);
    this->ui->monsterNoDropCheckBox->setChecked((flags & MFLAG_NODROP) != 0);
    this->ui->monsterKnockbackCheckBox->setChecked((flags & MFLAG_KNOCKBACK) != 0);

    // player vs. monster info
    int hper, mindam, maxdam;
    bool hth = false;
    bool special = false; // special (hit2/dam2 + afnum2)
    int missile = -1;
    bool ranged_special; // aiParam1 + afnum2
    switch (mon->_mAI.aiType) {
    case AI_LACHDAN:
        break;
    case AI_ZOMBIE:
    case AI_SKELSD:
    case AI_SCAV:
    case AI_RHINO: // + charge
    case AI_FALLEN:
    case AI_SKELKING:
    case AI_BAT:   // + MIS_LIGHTNING if MT_XBAT
    case AI_CLEAVER:
    case AI_SNEAK:
    //case AI_FIREMAN: // MIS_KRULL
    case AI_GOLUM:
    case AI_SNOTSPIL:
    case AI_SNAKE:
    case AI_WARLORD:
#ifdef HELLFIRE
    case AI_HORKDMN:
#endif
        hth = true;
        break;  // hth  --  MOFILE_MAGMA (hit + 10, dam - 2), MOFILE_THIN (hit - 20, dam + 4)
    case AI_FAT:
        hth = true;
        special = true;
        break;
    case AI_ROUND: // + special if aiParam1
    case AI_GARG:  // + AI_ROUND
    case AI_GARBUD: // + AI_ROUND
        hth = true;
        special = mon->_mAI.aiParam1;
        break;
    case AI_SKELBOW:
        missile = MIS_ARROW;
        break;
    case AI_RANGED: // special ranged / ranged if aiParam2
    case AI_LAZHELP: // AI_RANGED
        ranged_special = mon->_mAI.aiParam2;
        missile = mon->_mAI.aiParam1;
        break;
    case AI_ROUNDRANGED: // special ranged / hth
    case AI_ROUNDRANGED2: // AI_ROUNDRANGED
        hth = true;
        ranged_special = true;
        missile = mon->_mAI.aiParam1;
        break;
    case AI_ZHAR: // AI_COUNSLR
    case AI_COUNSLR: // ranged + MIS_FLASH
    case AI_LAZARUS: // AI_COUNSLR
        missile = mon->_mAI.aiParam1; // + MIS_FLASH
        break;
    case AI_MAGE:
        missile = MIS_MAGE; // + MIS_FLASH + param1
        break;
    }

    hper = this->hero->getHitChance() - mon->_mArmorClass;
    hper = CheckHit(hper);
    this->ui->plrHitChance->setText(QString("%1%").arg(hper));
    hper = 0;
    if (hth || (missile != -1 && (missiledata[missile].mdFlags & MIF_ARROW))) {
        hper = 30 + mon->_mHit + (2 * mon->_mLevel) - this->hero->getAC();
        hper = CheckHit(hper);
    } else if (missile != -1 && (missiledata[missile].mdFlags & MIF_AREA)) {
        hper = 40 + 2 * mon->_mLevel;
		hper -= 2 * plr._pLevel;
    } else if (missile != -1 && missile != MIS_SWAMPC) {
        hper = 50 + mon->_mMagic;
		hper -= plr._pIEvasion;
    }
    if (!special) {
        this->ui->monHitChance->setText(QString("%1%").arg(hper));
    } else {
        int hper2 = 30 + mon->_mHit + (2 * mon->_mLevel) - this->hero->getAC();
        hper2 = CheckHit(hper2);
        this->ui->monHitChance->setText(QString("%1% / %2%").arg(hper).arg(hper2));
    }

    displayDamage(this->ui->plrDamage, this->hero->getTotalMinDam(mon), this->hero->getTotalMaxDam(mon));
    mindam = 0;
    maxdam = 0;
    if (hth || (missile != -1 && missile != MIS_SWAMPC)) {
        mindam = mon->_mMinDamage;
        maxdam = mon->_mMaxDamage;
        if (missile == MIS_BLOODBOILC) {
            mindam = mon->_mLevel >> 1;
            maxdam = mon->_mLevel;
        } else if (missile == MIS_FLASH) {
            mindam = maxdam = mon->_mLevel << 1;
        } else if (missile == MIS_LIGHTNINGC || missile == MIS_LIGHTNINGC2) {
            // mindam = mon->_mMinDamage;
	        maxdam = mon->_mMaxDamage << 1;
        } else if (missile == MIS_CBOLTC) {
            mindam = maxdam = 15 << (6 + gnDifficulty); // FIXME
        } else if (missile == MIS_APOCAC2) {
            mindam = maxdam = 40 << (6 + gnDifficulty);
        }

        if (hth || !(missiledata[missile].mdFlags & MIF_DOT)) {
            mindam += this->hero->getGetHit();
            maxdam += this->hero->getGetHit();
        }
        if (mindam < 1)
            mindam = 1;
        if (maxdam < 1)
            maxdam = 1;
        // if (hth && (missile != -1 && missile != MIS_SWAMPC)) { FIXME
        // if (hth && MOFILE_MAGMA (hit + 10, dam - 2), MOFILE_THIN (hit - 20, dam + 4)) {
    }
    displayDamage(this->ui->monDamage, mindam, maxdam);

    hper = 0;
    this->ui->plrBlockChance->setText(QString("%1%").arg(hper));
    hper = 0;
    if (hth || (missile != -1 && !(missiledata[missile].mdFlags & MIF_NOBLOCK))) {
        hper = this->hero->getBlockChance() - (mon->_mLevel << 1);
    }
    this->ui->monBlockChance->setText(QString("%1%").arg(hper));

    this->adjustSize(); // not sure why this is necessary...
}


void MonsterDetailsWidget::on_dunTypeComboBox_activated(int index)
{
    this->updateFields();
}

void MonsterDetailsWidget::on_monTypeComboBox_activated(int index)
{
    this->updateFields();
}

void MonsterDetailsWidget::on_dunLevelEdit_returnPressed()
{
    this->dunLevel = this->ui->dunLevelEdit->text().toShort();

    this->on_dunLevelEdit_escPressed();
}

void MonsterDetailsWidget::on_dunLevelEdit_escPressed()
{
    // update dunLevelEdit
    this->updateFields();
    this->ui->dunLevelEdit->clearFocus();
}

void MonsterDetailsWidget::on_dunLevelBonusEdit_returnPressed()
{
    this->dunLevelBonus = this->ui->dunLevelBonusEdit->text().toShort();

    this->on_dunLevelBonusEdit_escPressed();
}

void MonsterDetailsWidget::on_dunLevelBonusEdit_escPressed()
{
    // update dunLevelBonusEdit
    this->updateFields();
    this->ui->dunLevelBonusEdit->clearFocus();
}

void MonsterDetailsWidget::on_plrCountSpinBox_valueChanged(int value)
{
    this->updateFields();
}

void MonsterDetailsWidget::on_difficutlyComboBox_activated(int index)
{
    this->updateFields();
}

void MonsterDetailsWidget::on_closeButton_clicked()
{
    this->setVisible(false);
}
