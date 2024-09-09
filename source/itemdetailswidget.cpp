#include "itemdetailswidget.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>
#include <QWidgetAction>

#include "d1hro.h"
#include "sidepanelwidget.h"
#include "ui_itemdetailswidget.h"

#include "dungeon/all.h"

ItemDetailsWidget::ItemDetailsWidget(SidePanelWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ItemDetailsWidget())
    , view(parent)
{
    ui->setupUi(this);
}

ItemDetailsWidget::~ItemDetailsWidget()
{
    delete ui;
}

void ItemDetailsWidget::initialize(D1Hero *h, int ii)
{
    this->hero = h;
    this->invIdx = ii;

    QComboBox *itemsComboBox = this->ui->invItemIndexComboBox;
    itemsComboBox->clear();
    itemsComboBox->addItem(tr("None"), QVariant::fromValue(-1));
    LogErrorF("ItemDetailsWidget init 1 %d", ii);
    const ItemStruct* pi = h->item(ii);
    if (pi->_itype != ITYPE_NONE) {
        LogErrorF("ItemDetailsWidget init 2 %s", ItemName(pi));
        itemsComboBox->addItem(ItemName(pi), QVariant::fromValue(-2));

        itemsComboBox->setCurrentIndex(1);
    } else {
        itemsComboBox->setCurrentIndex(0);
    }
    LogErrorF("ItemDetailsWidget init 3");
    for (int i = INVITEM_INV_FIRST; i < NUM_INVELEM; i++) {
        const ItemStruct* is = h->item(i);
        if (is->_itype == ITYPE_NONE || is->_itype == ITYPE_PLACEHOLDER) {
            continue;
        }
        switch (ii) {
        case INVITEM_HEAD:
            if (is->_iLoc != ILOC_HELM)
                continue;
            break;
        case INVITEM_RING_LEFT:
        case INVITEM_RING_RIGHT:
            if (is->_iLoc != ILOC_RING)
                continue;
            break;
        case INVITEM_AMULET:
            if (is->_iLoc != ILOC_AMULET)
                continue;
            break;
        case INVITEM_HAND_LEFT:
            if (is->_iLoc != ILOC_ONEHAND && is->_iLoc != ILOC_TWOHAND)
                continue;
            break;
        case INVITEM_HAND_RIGHT:
            if (is->_iLoc != ILOC_ONEHAND)
                continue;
            break;
        case INVITEM_CHEST:
            if (is->_iLoc != ILOC_ARMOR)
                continue;
            break;
        }
        LogErrorF("ItemDetailsWidget init 4 %s (%d) %d", ItemName(is), is->_itype, i);
        itemsComboBox->addItem(ItemName(is), QVariant::fromValue(i));
    }
    LogErrorF("ItemDetailsWidget init 5");
    this->updateFields();
    LogErrorF("ItemDetailsWidget init 6");
    this->setVisible(true);
}

void ItemDetailsWidget::updateFields()
{
    QComboBox *itemsComboBox = this->ui->invItemIndexComboBox;

    int ii = itemsComboBox->currentData().value<int>();
    LogErrorF("updateFields 0 %d", ii);
    const ItemStruct* pi = ii == -1 ? nullptr : (ii == -2 ? this->hero->item(this->invIdx) : this->hero->item(ii));

    if (pi != nullptr && pi->_itype != ITYPE_NONE) {
        QString text;
        LogErrorF("updateFields 1 %d", pi->_itype);
        text.clear();
        switch (pi->_itype) {
        case ITYPE_SWORD:       text = tr("Sword");        break;
        case ITYPE_AXE:         text = tr("Axe");          break;
        case ITYPE_BOW:         text = tr("Bow");          break;
        case ITYPE_MACE:        text = tr("Mace");         break;
        case ITYPE_STAFF:       text = tr("Staff");        break;
        case ITYPE_SHIELD:      text = tr("Shield");       break;
        case ITYPE_HELM:        text = tr("Helm");         break;
        case ITYPE_LARMOR:      text = tr("Light Armor");  break;
        case ITYPE_MARMOR:      text = tr("Medium Armor"); break;
        case ITYPE_HARMOR:      text = tr("Heavy Armor");  break;
        case ITYPE_MISC:        text = tr("Misc");         break;
        case ITYPE_GOLD:        text = tr("Gold");         break;
        case ITYPE_RING:        text = tr("Ring");         break;
        case ITYPE_AMULET:      text = tr("Amulet");       break;
        case ITYPE_PLACEHOLDER: text = tr("Placeholder");  break;
        }
        this->ui->itemTypeEdit->setText(text);
        LogErrorF("updateFields 2 %d", pi->_iClass);
        text.clear();
        switch (pi->_iClass) {
        case ICLASS_NONE:   text = tr("None");   break;
        case ICLASS_WEAPON: text = tr("Weapon"); break;
        case ICLASS_ARMOR:  text = tr("Armor");  break;
        case ICLASS_MISC:   text = tr("Misc");   break;
        case ICLASS_GOLD:   text = tr("Gold");   break;
        case ICLASS_QUEST:  text = tr("Quest");  break;
        }
        this->ui->itemClassEdit->setText(text);
        LogErrorF("updateFields 3 %d", pi->_iLoc);
        text.clear();
        switch (pi->_iLoc) {
        case ILOC_UNEQUIPABLE: text = tr("Unequipable"); break;
        case ILOC_ONEHAND:     text = tr("One handed");  break;
        case ILOC_TWOHAND:     text = tr("Two handed");  break;
        case ILOC_ARMOR:       text = tr("Armor");       break;
        case ILOC_HELM:        text = tr("Helm");        break;
        case ILOC_RING:        text = tr("Ring");        break;
        case ILOC_AMULET:      text = tr("Amulet");      break;
        case ILOC_BELT:        text = tr("Belt");        break;
        }
        this->ui->itemLocEdit->setText(text);
        LogErrorF("updateFields 4 %d", pi->_iSeed);
        this->ui->itemSeedEdit->setText(QString::number(pi->_iSeed));
        LogErrorF("updateFields 5 %d", pi->_iIdx);
        this->ui->itemIdxEdit->setText(QString("%1 (%2)").arg(pi->_iIdx < NUM_IDI ? AllItemsList[pi->_iIdx].iName : "").arg(pi->_iIdx));
        LogErrorF("updateFields 6 %d", pi->_iCreateInfo & CF_LEVEL);
        this->ui->itemLevelEdit->setText(QString::number(pi->_iCreateInfo & CF_LEVEL));
        LogErrorF("updateFields 7 %d", (pi->_iCreateInfo & CF_TOWN) >> 8);
        text.clear();
        switch ((pi->_iCreateInfo & CF_TOWN) >> 8) {
        case CFL_NONE:         text = tr("Drop");          break;
        case CFL_SMITH:        text = tr("Smith Normal");  break;
        case CFL_SMITHPREMIUM: text = tr("Smith Premium"); break;
        case CFL_BOY:          text = tr("Wirt");          break;
        case CFL_WITCH:        text = tr("Adria");         break;
        case CFL_HEALER:       text = tr("Pepin");         break;
        case CFL_CRAFTED:      text = tr("Crafted");       break;
        }
        this->ui->itemSourceEdit->setText(text);
        LogErrorF("updateFields 7 %d", (pi->_iCreateInfo & CF_DROP_QUALITY) >> 11);
        text.clear();
        switch ((pi->_iCreateInfo & CF_DROP_QUALITY) >> 11) {
        case CFDQ_NONE:   text = tr("None");   break;
        case CFDQ_NORMAL: text = tr("Normal"); break;
        case CFDQ_GOOD:   text = tr("Good");   break;
        case CFDQ_UNIQUE: text = tr("Unique"); break;
        }
        this->ui->itemQualityEdit->setText(text);
        LogErrorF("updateFields 8");
/*
	union {
		int _ix;
		int _iPHolder; // parent index of a placeholder entry in InvList
	};
	int _iy;
	int _iCurs;   // item_cursor_graphic
	int _iMiscId; // item_misc_id
	int _iSpell;  // spell_id
	BYTE _iDamType; // item_damage_type
	BYTE _iMinDam;
	BYTE _iMaxDam;
	BYTE _iBaseCrit;
	BYTE _iMinStr;
	BYTE _iMinMag;
	BYTE _iMinDex;
	BOOLEAN _iUsable; // can be placed in belt, can be consumed/used or stacked (if max durability is not 1)
	BYTE _iPrePower; // item_effect_type
	BYTE _iSufPower; // item_effect_type
	BYTE _iMagical;	// item_quality
	BYTE _iSelFlag;
	BOOLEAN _iFloorFlag;
	BOOL _iIdentified;
	int _ivalue;
	int _iIvalue;
	int _iAC;
	int _iFlags;	// item_special_effect
	int _iCharges;
	int _iMaxCharges;
	int _iDurability;
	int _iMaxDur;
	int _iPLDam;
	int _iPLToHit;
	int _iPLAC;
	int _iPLStr;
	int _iPLMag;
	int _iPLDex;
	int _iPLVit;
	int _iPLFR;
	int _iPLLR;
	int _iPLMR;
	int _iPLAR;
	int _iPLMana;
	int _iPLHP;
	int _iPLDamMod;
	int _iPLGetHit;
	int8_t _iPLLight;
	int8_t _iPLSkillLevels;
	BYTE _iPLSkill;
	int8_t _iPLSkillLvl;
	BYTE _iPLManaSteal;
	BYTE _iPLLifeSteal;
	BYTE _iPLCrit;
	BOOLEAN _iStatFlag;
	int _iUid; // unique_item_indexes
	BYTE _iPLFMinDam;
	BYTE _iPLFMaxDam;
	BYTE _iPLLMinDam;
	BYTE _iPLLMaxDam;
	BYTE _iPLMMinDam;
	BYTE _iPLMMaxDam;
	BYTE _iPLAMinDam;
	BYTE _iPLAMaxDam;
	int _iVAdd;
	int _iVMult;

*/
    } else {
        this->ui->itemTypeEdit->setVisible(false);
        this->ui->itemClassEdit->setVisible(false);        
        this->ui->itemLocEdit->setVisible(false);
        this->ui->itemSeedEdit->setVisible(false);
        this->ui->itemIdxEdit->setVisible(false);
        this->ui->itemLevelEdit->setVisible(false);
        this->ui->itemSourceEdit->setVisible(false);
        this->ui->itemQualityEdit->setVisible(false);
    }
}

void ItemDetailsWidget::on_invItemIndexComboBox_activated(int index)
{
    this->updateFields();
}

void ItemDetailsWidget::on_submitButton_clicked()
{
    QComboBox *itemsComboBox = this->ui->invItemIndexComboBox;

    int ii = itemsComboBox->currentData().value<int>();

    if (ii == -2) {
        ii = INVITEM_NONE;
    } else if (ii == -1) {
        ii = this->invIdx;
    }

    this->hero->swapItem(this->invIdx, ii);

    this->setVisible(false);
}

void ItemDetailsWidget::on_cancelButton_clicked()
{
    this->setVisible(false);
}
