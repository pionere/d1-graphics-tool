#include "itempropertieswidget.h"

#include <QApplication>
#include <QMessageBox>
#include <QString>

#include "ui_itempropertieswidget.h"

#include "dungeon/all.h"

ItemPropertiesWidget::ItemPropertiesWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ItemPropertiesWidget())
{
    ui->setupUi(this);
}

ItemPropertiesWidget::~ItemPropertiesWidget()
{
    delete ui;
}

void ItemPropertiesWidget::initialize(const ItemStruct *is)
{
    QString label;
    QString text;
    QString tooltip;
    int active;

    active = 0;
	if (is->_iClass == ICLASS_WEAPON) {
        label = tr("Damage:");
		if (is->_iMinDam == is->_iMaxDam)
			text = QString::number(is->_iMinDam);
		else
            text = QString("%1-%2").arg(is->_iMinDam).arg(is->_iMaxDam);
        switch (is->_iDamType) {
        case IDAM_NONE:     tooltip = tr("-");           break;
        case IDAM_SLASH:    tooltip = tr("Slash");       break;
        case IDAM_BLUNT:    tooltip = tr("Blunt");       break;
        case IDAM_SB_MIX:   tooltip = tr("Slash/Blunt"); break;
        case IDAM_PUNCTURE: tooltip = tr("Puncture");    break;
        }
        tooltip = tr("Type: %1. Base crit. chance: %2").arg(tooltip).arg(is->_iBaseCrit);
        active = 1;
	} else if (is->_iClass == ICLASS_ARMOR) {
        label = tr("Armor:");
        text = QString::number(is->_iAC);
        tooltip = "";
        active = 1;
    }
    this->ui->itemDAMLabel->setVisible(active != 0);
    this->ui->itemDAMLabel->setText(label);
    this->ui->itemDAMText->setVisible(active != 0);
    this->ui->itemDAMText->setText(text);
    this->ui->itemDAMText->setToolTip(tooltip);

    active = 0;
    if (is->_iPrePower != IPL_INVALID) {
        PrintItemPower(is->_iPrePower, is);
        this->ui->itemPrePowerText->setText(tempstr);
        active = 1;
    }
    this->ui->itemPrePowerLabel->setVisible(active != 0);
    this->ui->itemPrePowerText->setVisible(active != 0);

    active = 0;
    if (is->_iSufPower != IPL_INVALID) {
        PrintItemPower(is->_iSufPower, is);
        this->ui->itemSufPowerText->setText(tempstr);
        active = 1;
    }
        this->ui->itemSufPowerLabel->setVisible(active != 0);
        this->ui->itemSufPowerText->setVisible(active != 0);

    active = 0;
    if (is->_iMagical == ITEM_QUALITY_UNIQUE && (unsigned)is->_iUid < NUM_UITEM) {
        // DrawUniqueInfo(is, x, y);
        const UniqItemData* uis;
        uis = &UniqueItemList[is->_iUid];
        PrintItemPower(uis->UIPower1, is);
        this->ui->itemUniquePower1Text->setText(tempstr);
        active++;
        if (uis->UIPower2 != IPL_INVALID) {
            PrintItemPower(uis->UIPower2, is);
            this->ui->itemUniquePower2Text->setText(tempstr);
            active++;
            if (uis->UIPower3 != IPL_INVALID) {
                PrintItemPower(uis->UIPower3, is);
                this->ui->itemUniquePower3Text->setText(tempstr);
                active++;
                if (uis->UIPower4 != IPL_INVALID) {
                    PrintItemPower(uis->UIPower4, is);
                    this->ui->itemUniquePower4Text->setText(tempstr);
                    active++;
                    if (uis->UIPower5 != IPL_INVALID) {
                        PrintItemPower(uis->UIPower5, is);
                        this->ui->itemUniquePower5Text->setText(tempstr);
                        active++;
                        if (uis->UIPower6 != IPL_INVALID) {
                            PrintItemPower(uis->UIPower6, is);
                            this->ui->itemUniquePower6Text->setText(tempstr);
                            active++;
                        }
                    }
                }
            }
        }
    }
    this->ui->itemUniquePower1Label->setVisible(active > 0);
    this->ui->itemUniquePower1Text->setVisible(active > 0);
    this->ui->itemUniquePower2Label->setVisible(active > 1);
    this->ui->itemUniquePower2Text->setVisible(active > 1);
    this->ui->itemUniquePower3Label->setVisible(active > 2);
    this->ui->itemUniquePower3Text->setVisible(active > 2);
    this->ui->itemUniquePower4Label->setVisible(active > 3);
    this->ui->itemUniquePower4Text->setVisible(active > 3);
    this->ui->itemUniquePower5Label->setVisible(active > 4);
    this->ui->itemUniquePower5Text->setVisible(active > 4);
    this->ui->itemUniquePower6Label->setVisible(active > 5);
    this->ui->itemUniquePower6Text->setVisible(active > 5);

    active = 0;
    if ((is->_iMinStr | is->_iMinMag | is->_iMinDex) != 0) {
        text.clear();
        if (is->_iMinStr)
            text.append(tr("%1 Str  ").arg(is->_iMinStr));
        if (is->_iMinMag)
            text.append(tr("%1 Mag  ").arg(is->_iMinMag));
        if (is->_iMinDex)
            text.append(tr("%1 Dex  ").arg(is->_iMinDex));
        this->ui->itemRequirementsText->setText(text);
        active = 1;
    }
    this->ui->itemRequirementsLabel->setVisible(active != 0);
    this->ui->itemRequirementsText->setVisible(active != 0);

/*
	union {
		int _ix;
		int _iPHolder; // parent index of a placeholder entry in InvList
	};
	int _iy;
	int _iCurs;   // item_cursor_graphic
	int _iMiscId; // item_misc_id
	int _iSpell;  // spell_id
	BYTE _iMagical;	// item_quality
	BYTE _iSelFlag;
	BOOLEAN _iFloorFlag;
	BOOL _iIdentified;
	int _ivalue;
	int _iIvalue;
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

}
