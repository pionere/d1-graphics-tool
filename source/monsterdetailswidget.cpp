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

void MonsterDetailsWidget::updateFields()
{
    QComboBox *typesComboBox = this->ui->monTypeComboBox;
    typesComboBox->clear();
    
    int loc = this->ui->monLocationComboBox->currentIndex();
    int mi = typesComboBox->currentData().value<int>();
    for (int i = 0; i < NUM_MTYPES; i++) {
        if (loc != DTYPE_TOWN) {
            int n;
            for (n = 0; n < NUM_FIXLVLS; n++) {
                if (AllLevels[n].dType != loc) continue;
                int m;
                for (m = 0; AllLevels[n].dMonTypes[m] != MT_INVALID; m++) {
                    if (m == i) break;
                }
                if (AllLevels[n].dMonTypes[m] != MT_INVALID) break;
            }
            if (n >= NUM_FIXLVLS) continue;
        }
        typesComboBox->addItem(monsterdata[i].mName, QVariant::fromValue(i));
    }
    for (int i = 0; ; i++) {
        const UniqMonData &mon = uniqMonData[i];
        if (mon.mtype == MT_INVALID)
            break;
        if (loc != DTYPE_TOWN) {
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
            switch (loc) {
            case DTYPE_CATHEDRAL: if (ml < DLV_CATHEDRAL1 || ml > DLV_CATHEDRAL4) continue; break;
            case DTYPE_CATACOMBS: if (ml < DLV_CATACOMBS1 || ml > DLV_CATACOMBS4) continue; break;
            case DTYPE_CAVES:     if (ml < DLV_CAVES1 || ml > DLV_CAVES4) continue; break;
            case DTYPE_HELL:      if (ml < DLV_HELL1 || ml > DLV_HELL4) continue; break;
            case DTYPE_CRYPT:     if (ml < DLV_CRYPT1 || ml > DLV_CRYPT4) continue; break;
            case DTYPE_NEST:      if (ml < DLV_NEST1 || ml > DLV_NEST4) continue; break;
            }
        }
        typesComboBox->addItem(QString("**%1**").arg(mon.mName), QVariant::fromValue(-(i + 1)));
    }
    // typesComboBox->adjustSize();
    mi = typesComboBox->findData(mi);
    if (mi < 0) mi = 0;
    typesComboBox->setCurrentIndex(mi);

    int type = typesComboBox->currentData().value<int>();
    int lvl = this->dunLevel;
    int numplrs = this->ui->plrCountSpinBox->value();
    int difficulty = this->ui->difficutlyComboBox->currentIndex();
    int lvlbonus = this->dunLevelBonus;

    this->ui->dunLevelEdit->setText(QString::number(lvl));
    this->ui->dunLevelBonusEdit->setText(QString::number(lvlbonus));

    if (type < 0)
        InitUniqMonster(-(type + 1), lvl, numplrs, difficulty, lvlbonus);
    else
        InitLvlMonster(type, lvl, numplrs, difficulty, lvlbonus);

    MonsterStruct *mon = monsters[0];

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
    this->ui->monsterDamage->setText(QString("%1 - %2").arg(mon->_mMinDamage).arg(mon->_mMaxDamage));
    this->ui->monsterHit2->setText(QString::number(mon->_mHit2));
    this->ui->monsterDamage2->setText(QString("%1 - %2").arg(mon->_mMinDamage2).arg(mon->_mMaxDamage2));
    this->ui->monsterMagic->setText(QString::number(mon->_mMagic));

    this->ui->monsterHp->setText(QString::number(mon->_mmaxhp >> 6));
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

    this->adjustSize(); // not sure why this is necessary...
}

void MonsterDetailsWidget::on_monLocationComboBox_activated(int index)
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
