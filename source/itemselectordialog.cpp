#include "itemselectordialog.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>
#include <QWidgetAction>

#include "d1hro.h"
#include "mainwindow.h"
#include "sidepanelwidget.h"
#include "ui_itemselectordialog.h"

#include "dungeon/all.h"

ItemSelectorDialog::ItemSelectorDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ItemSelectorDialog())
{
    ui->setupUi(this);

    QHBoxLayout *layout = this->ui->seedWithRefreshButtonLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_BrowserReload, tr("Generate"), this, &ItemSelectorDialog::on_actionGenerateSeed_triggered);
    layout->addStretch();

    this->itemProps = new ItemPropertiesWidget(this);
    this->ui->itemProperties->addWidget(this->itemProps);

    QObject::connect(this->ui->itemSeedEdit, SIGNAL(cancel_signal()), this, SLOT(on_itemSeedEdit_escPressed()));
    QObject::connect(this->ui->itemLevelEdit, SIGNAL(cancel_signal()), this, SLOT(on_itemLevelEdit_escPressed()));
}

ItemSelectorDialog::~ItemSelectorDialog()
{
    delete ui;
    delete this->is;
}

void ItemSelectorDialog::initialize(D1Hero *h, int ii)
{
    this->hero = h;
    this->invIdx = ii;

    delete this->is;
    this->is = new ItemStruct();

    if (ii < NUM_INVLOC) {
        const ItemStruct* pi = h->item(ii);
        memcpy(this->is, pi, sizeof(ItemStruct));

        this->itemType = pi->_itype;
    } else {
        memset(this->is, 0, sizeof(ItemStruct));
        this->is->_iCreateInfo = 1 | (CFL_NONE << 8) | (CFDQ_NORMAL << 11);

        this->itemType = ITYPE_NONE;
    }

    QComboBox *locComboBox = this->ui->itemLocComboBox;
    locComboBox->clear();

    switch (ii) {
    case INVITEM_HEAD:
        // this->is->_iClass = ICLASS_ARMOR;
        locComboBox->addItem(tr("Helm"), QVariant::fromValue(ILOC_HELM));
        break;
    case INVITEM_RING_LEFT:
        // this->is->_iClass = ICLASS_MISC;
        locComboBox->addItem(tr("Ring"), QVariant::fromValue(ILOC_RING));
        break;
    case INVITEM_RING_RIGHT:
        // this->is->_iClass = ICLASS_MISC;
        locComboBox->addItem(tr("Ring"), QVariant::fromValue(ILOC_RING));
        break;
    case INVITEM_AMULET:
        // this->is->_iClass = ICLASS_MISC;
        locComboBox->addItem(tr("Amulet"), QVariant::fromValue(ILOC_AMULET));
        break;
    case INVITEM_HAND_LEFT:
    case INVITEM_HAND_RIGHT:
        // this->is->_iClass = ICLASS_WEAPON;
        locComboBox->addItem(tr("Weapon / Shield"), QVariant::fromValue(ILOC_UNEQUIPABLE));
        locComboBox->addItem(tr("One handed"), QVariant::fromValue(ILOC_ONEHAND));
        locComboBox->addItem(tr("Two handed"), QVariant::fromValue(ILOC_TWOHAND));
        break;
    case INVITEM_CHEST:
        // this->is->_iClass = ICLASS_ARMOR;
        locComboBox->addItem(tr("Armor"), QVariant::fromValue(ILOC_ARMOR));
        break;
    default:
        locComboBox->addItem(tr("Any"), QVariant::fromValue(ILOC_UNEQUIPABLE));
        locComboBox->addItem(tr("Helm"), QVariant::fromValue(ILOC_HELM));
        locComboBox->addItem(tr("Amulet"), QVariant::fromValue(ILOC_AMULET));
        locComboBox->addItem(tr("Armor"), QVariant::fromValue(ILOC_ARMOR));
        locComboBox->addItem(tr("One handed"), QVariant::fromValue(ILOC_ONEHAND));
        locComboBox->addItem(tr("Two handed"), QVariant::fromValue(ILOC_TWOHAND));
        locComboBox->addItem(tr("Ring"), QVariant::fromValue(ILOC_RING));
        break;
    }

    int idx = locComboBox->findData(QVariant::fromValue((item_equip_type)this->is->_iLoc));
    // QMessageBox::critical(this, "Error", tr("Loc %1 idx%2 ii %3 wth%4.").arg(this->is->_iLoc).arg(idx).arg(ii).arg(this->is->_iLoc == ILOC_ONEHAND));
    if (idx < 0) idx = 0;
    locComboBox->setCurrentIndex(idx);

    this->updateFilters();
    this->updateFields();
}

void ItemSelectorDialog::updateFilters()
{
    QComboBox *locComboBox = this->ui->itemLocComboBox;
    QComboBox *typeComboBox = this->ui->itemTypeComboBox;
    QComboBox *idxComboBox = this->ui->itemIdxComboBox;
    typeComboBox->clear();
    idxComboBox->clear();

    // QMessageBox::critical(this, "Error", tr("updateFilters loc %1.").arg(locComboBox->currentData().value<int>()));
    int iloc = locComboBox->currentData().value<int>();
    switch (iloc) {
    case ILOC_HELM:
        typeComboBox->addItem(tr("Helm"), QVariant::fromValue(ITYPE_HELM));
        break;
    case ILOC_AMULET:
        typeComboBox->addItem(tr("Amulet"), QVariant::fromValue(ITYPE_AMULET));
        break;
    case ILOC_ARMOR:
        typeComboBox->addItem(tr("Armor"), QVariant::fromValue(ITYPE_NONE));
        typeComboBox->addItem(tr("Light Armor"), QVariant::fromValue(ITYPE_LARMOR));
        typeComboBox->addItem(tr("Medium Armor"), QVariant::fromValue(ITYPE_MARMOR));
        typeComboBox->addItem(tr("Heavy Armor"), QVariant::fromValue(ITYPE_HARMOR));
        break;
    case ILOC_ONEHAND:
        typeComboBox->addItem(tr("All"), QVariant::fromValue(ITYPE_NONE));
        typeComboBox->addItem(tr("Sword"), QVariant::fromValue(ITYPE_SWORD));
        typeComboBox->addItem(tr("Axe"), QVariant::fromValue(ITYPE_AXE));
        typeComboBox->addItem(tr("Mace"), QVariant::fromValue(ITYPE_MACE));
        typeComboBox->addItem(tr("Shield"), QVariant::fromValue(ITYPE_SHIELD));
        break;
    case ILOC_TWOHAND:
        typeComboBox->addItem(tr("All"), QVariant::fromValue(ITYPE_NONE));
        typeComboBox->addItem(tr("Sword"), QVariant::fromValue(ITYPE_SWORD));
        typeComboBox->addItem(tr("Axe"), QVariant::fromValue(ITYPE_AXE));
        typeComboBox->addItem(tr("Bow"), QVariant::fromValue(ITYPE_BOW));
        typeComboBox->addItem(tr("Mace"), QVariant::fromValue(ITYPE_MACE));
        typeComboBox->addItem(tr("Staff"), QVariant::fromValue(ITYPE_STAFF));
        break;
    case ILOC_RING:
        typeComboBox->addItem(tr("Ring"), QVariant::fromValue(ITYPE_RING));
        break;
    default:
        typeComboBox->addItem(tr("All"), QVariant::fromValue(ITYPE_NONE));
        if (this->invIdx == INVITEM_NONE) {
            typeComboBox->addItem(tr("Helm"), QVariant::fromValue(ITYPE_HELM));
            typeComboBox->addItem(tr("Amulet"), QVariant::fromValue(ITYPE_AMULET));
            typeComboBox->addItem(tr("Ring"), QVariant::fromValue(ITYPE_RING));
            typeComboBox->addItem(tr("Light Armor"), QVariant::fromValue(ITYPE_LARMOR));
            typeComboBox->addItem(tr("Medium Armor"), QVariant::fromValue(ITYPE_MARMOR));
            typeComboBox->addItem(tr("Heavy Armor"), QVariant::fromValue(ITYPE_HARMOR));
        } else {
            // assert(this->invIdx == INVITEM_HAND_LEFT || this->invIdx == INVITEM_HAND_RIGHT);
        }
        typeComboBox->addItem(tr("Sword"), QVariant::fromValue(ITYPE_SWORD));
        typeComboBox->addItem(tr("Axe"), QVariant::fromValue(ITYPE_AXE));
        typeComboBox->addItem(tr("Bow"), QVariant::fromValue(ITYPE_BOW));
        typeComboBox->addItem(tr("Mace"), QVariant::fromValue(ITYPE_MACE));
        typeComboBox->addItem(tr("Staff"), QVariant::fromValue(ITYPE_STAFF));
        typeComboBox->addItem(tr("Shield"), QVariant::fromValue(ITYPE_SHIELD));
        break;
    }

    int idx = typeComboBox->findData(QVariant::fromValue((item_type)this->itemType));
    // QMessageBox::critical(this, "Error", tr("updateFilters type %1 val %2 - %3.").arg(this->itemType).arg(idx).arg(this->itemType == ITYPE_AMULET));
    // for (int i = 0; i < typeComboBox->count(); i++) {
    //     int tv = typeComboBox->currentData().value<int>();
    //     if (tv == this->itemType && idx != i) {
    //         QMessageBox::critical(this, "Error", tr("Fuck you!!!!!"));
    //     }
    // }
    if (idx < 0) idx = 0;
    typeComboBox->setCurrentIndex(idx);

    int itype = typeComboBox->currentData().value<int>();
    // QMessageBox::critical(this, "Error", tr("updateFilters filter idx by loc %1 type %2.").arg(iloc).arg(itype));
    for (int i = 0; i < NUM_IDI; ++i) {
        const ItemData &id = AllItemsList[i];
        //if (id.iClass != this->is->_iClass) {
        //    continue;
        //}
        if (itype != ITYPE_NONE && itype != id.itype) {
            continue;
        }
        if (iloc != ILOC_UNEQUIPABLE && iloc != id.iLoc) {
            continue;
        }
        idxComboBox->addItem(QString("%1 (%2)").arg(id.iName).arg(i), QVariant::fromValue(i));
    }

    idx = idxComboBox->findData(QVariant::fromValue(this->is->_iIdx));
    if (idx < 0) idx = 0;
    idxComboBox->setCurrentIndex(idx);
}

static int GetItemBonusFlags(int itype, int misc_id)
{
    int flgs = 0;
    switch (itype) {
    case ITYPE_MISC:
        if (misc_id != IMISC_MAP)
            break;
        flgs = PLT_MAP;
        break;
    case ITYPE_SWORD:
    case ITYPE_AXE:
    case ITYPE_MACE:
        flgs = PLT_MELEE;
        break;
    case ITYPE_BOW:
        flgs = PLT_BOW;
        break;
    case ITYPE_SHIELD:
        flgs = PLT_SHLD;
        break;
    case ITYPE_LARMOR:
        flgs = PLT_ARMO | PLT_LARMOR;
        break;
    case ITYPE_HELM:
        flgs = PLT_ARMO;
        break;
    case ITYPE_MARMOR:
        flgs = PLT_ARMO | PLT_MARMOR;
        break;
    case ITYPE_HARMOR:
        flgs = PLT_ARMO | PLT_HARMOR;
        break;
    case ITYPE_STAFF:
        flgs = PLT_STAFF | PLT_CHRG;
        break;
    case ITYPE_GOLD:
        break;
    case ITYPE_RING:
    case ITYPE_AMULET:
        flgs = PLT_MISC;
        break;
    }
    return flgs;
}

static QString AffixPowerName(int power)
{
    QString result = "";
    switch (power) {
    case IPL_TOHIT:          result = QApplication::tr("to hit");                break;
    case IPL_DAMP:           result = QApplication::tr("damage %");              break;
    case IPL_TOHIT_DAMP:     result = QApplication::tr("to hit + damage");       break;
    case IPL_ACP:            result = QApplication::tr("armor %");               break;
    case IPL_FIRERES:        result = QApplication::tr("fire res.");             break;
    case IPL_LIGHTRES:       result = QApplication::tr("light res.");            break;
    case IPL_MAGICRES:       result = QApplication::tr("magic res.");            break;
    case IPL_ACIDRES:        result = QApplication::tr("acid res.");             break;
    case IPL_ALLRES:         result = QApplication::tr("all res.");              break;
    case IPL_CRITP:          result = QApplication::tr("crit. %");               break;
    case IPL_SKILLLVL:       result = QApplication::tr("skill");                 break;
    case IPL_SKILLLEVELS:    result = QApplication::tr("skills");                break;
    case IPL_CHARGES:        result = QApplication::tr("charges");               break;
    case IPL_FIREDAM:        result = QApplication::tr("fire damage");           break;
    case IPL_LIGHTDAM:       result = QApplication::tr("lightning  damage");     break;
    case IPL_MAGICDAM:       result = QApplication::tr("magic damage");          break;
    case IPL_ACIDDAM:        result = QApplication::tr("acid damage");           break;
    case IPL_STR:            result = QApplication::tr("strength");              break;
    case IPL_MAG:            result = QApplication::tr("magic");                 break;
    case IPL_DEX:            result = QApplication::tr("dexterity");             break;
    case IPL_VIT:            result = QApplication::tr("vitality");              break;
    case IPL_ATTRIBS:        result = QApplication::tr("attributes");            break;
    case IPL_GETHIT:         result = QApplication::tr("get hit");               break;
    case IPL_LIFE:           result = QApplication::tr("life");                  break;
    case IPL_MANA:           result = QApplication::tr("mana");                  break;
    case IPL_DUR:            result = QApplication::tr("durability +");          break;
    case IPL_DUR_CURSE:      result = QApplication::tr("durability -");          break;
    case IPL_INDESTRUCTIBLE: result = QApplication::tr("indestructible");        break;
    case IPL_LIGHT:          result = QApplication::tr("light range");           break;
    //case IPL_INVCURS: result = QApplication::tr("xxx"); break;
    //case IPL_THORNS: result = QApplication::tr("xxx"); break;
    case IPL_NOMANA:         result = QApplication::tr("no mana");               break;
    case IPL_KNOCKBACK:      result = QApplication::tr("knockback");             break;
    case IPL_STUN:           result = QApplication::tr("stun");                  break;
    //case IPL_NOHEALMON: result = QApplication::tr("xxx"); break;
    case IPL_NO_BLEED:       result = QApplication::tr("no bleed");              break;
    case IPL_BLEED:          result = QApplication::tr("bleed");                 break;
    case IPL_STEALMANA:      result = QApplication::tr("steal mana");            break;
    case IPL_STEALLIFE:      result = QApplication::tr("steal life");            break;
    case IPL_PENETRATE_PHYS: result = QApplication::tr("penetrate phy.");        break;
    case IPL_FASTATTACK:     result = QApplication::tr("attack speed");          break;
    case IPL_FASTRECOVER:    result = QApplication::tr("recovery speed");        break;
    case IPL_FASTBLOCK:      result = QApplication::tr("block speed");           break;
    case IPL_DAMMOD:         result = QApplication::tr("damage +");              break;
    case IPL_SETDAM:         result = QApplication::tr("damage *");              break;
    case IPL_SETDUR:         result = QApplication::tr("durability *");          break;
    case IPL_NOMINSTR:       result = QApplication::tr("no min. strength");      break;
    case IPL_SPELL:          result = QApplication::tr("spell");                 break;
    case IPL_ONEHAND:        result = QApplication::tr("one handed");            break;
    case IPL_ALLRESZERO:     result = QApplication::tr("all res. zero");         break;
    case IPL_DRAINLIFE:      result = QApplication::tr("drain life");            break;
    //case IPL_INFRAVISION: result = QApplication::tr("xxx"); break;
    case IPL_SETAC:          result = QApplication::tr("armor *");               break;
    case IPL_ACMOD:          result = QApplication::tr("armor +");               break;
    case IPL_CRYSTALLINE:    result = QApplication::tr("damage % durability -"); break;
    case IPL_MANATOLIFE:     result = QApplication::tr("mana to life");          break;     /* only used in hellfire */
    case IPL_LIFETOMANA:     result = QApplication::tr("life to mana");          break;     /* only used in hellfire */
    case IPL_FASTCAST:       result = QApplication::tr("cast speed");            break;
    case IPL_FASTWALK:       result = QApplication::tr("walk speed");            break;
    }
    return result;
}

static QString AffixName(const AffixData *affix)
{
    return AffixPowerName(affix->PLPower);
}

static void addUniqueOption(int power, int paramA, int paramB, int idx, QComboBox *preComboBox, QComboBox *sufComboBox)
{
    if (paramA == paramB)
        return;
    if (preComboBox->count() == 0) {
        preComboBox->addItem(QString("%1 (%2-%3)").arg(AffixPowerName(power)).arg(paramA).arg(paramB), QVariant::fromValue(idx));
    } else {
        sufComboBox->addItem(QString("%1 (%2-%3)").arg(AffixPowerName(power)).arg(paramA).arg(paramB), QVariant::fromValue(idx));
    }
}

void ItemSelectorDialog::updateFields()
{
    // QComboBox *typeComboBox = this->ui->itemTypeComboBox;
    // QComboBox *locComboBox = this->ui->itemLocComboBox;
    QComboBox *idxComboBox = this->ui->itemIdxComboBox;

    // locComboBox->setCurrentIndex(locComboBox->findData(this->is->_iLoc));
    // typeComboBox->setCurrentIndex(typeComboBox->findData(this->is->_itype));
    // idxComboBox->setCurrentIndex(idxComboBox->findData(this->is->_iIdx));

    int idx = idxComboBox->currentData().value<int>();
    bool drop = AllItemsList[idx].iRnd != 0;
    if (idx != this->is->_iIdx) {
        this->is->_iIdx = idx;
        this->is->_itype = ITYPE_NONE;
        if (!drop)
            this->is->_iCreateInfo &= CF_LEVEL;
    }
    int ci = this->is->_iCreateInfo;
    this->ui->itemSeedEdit->setText(QString::number(this->is->_iSeed));
    this->ui->itemLevelEdit->setText(QString::number(ci & CF_LEVEL));
    static_assert(((int)CF_TOWN & ((1 << 8) - 1)) == 0, "ItemSelectorDialog hardcoded CF_TOWN must be adjusted I.");
    static_assert((((int)CF_TOWN >> 8) & ((((int)CF_TOWN >> 8) + 1))) == 0, "ItemSelectorDialog hardcoded CF_TOWN must be adjusted II.");
    this->ui->itemSourceComboBox->setCurrentIndex((ci & CF_TOWN) >> 8);
    this->ui->itemSourceComboBox->setEnabled(drop);
    static_assert(((int)CF_DROP_QUALITY & ((1 << 11) - 1)) == 0, "ItemSelectorDialog hardcoded CF_DROP_QUALITY must be adjusted I.");
    static_assert((((int)CF_DROP_QUALITY >> 11) & ((((int)CF_DROP_QUALITY >> 11) + 1))) == 0, "ItemSelectorDialog hardcoded CF_DROP_QUALITY must be adjusted II.");
    this->ui->itemQualityComboBox->setCurrentIndex((ci & CF_DROP_QUALITY) >> 11);
    this->ui->itemQualityComboBox->setEnabled(drop);

    this->itemProps->initialize(this->is);
    this->itemProps->adjustSize();
    this->ui->itemPropertiesBox->adjustSize();
    this->itemProps->setVisible(this->is->_itype != ITYPE_NONE);

    // update whish-lists
    int flgs = GetItemBonusFlags(AllItemsList[idx].itype /* this->is->_itype*/, IMISC_NONE/* this->is->_iMiscId*/);
    int source = (ci & CF_TOWN) >> 8;
    int range = source == CFL_NONE ? IAR_DROP : (source == CFL_CRAFTED ? IAR_CRAFT : IAR_SHOP);
    int lvl = ci & CF_LEVEL;
    int si;

    // update possible uniques
    QComboBox *uniqComboBox = this->ui->itemUniquesComboBox;
    uniqComboBox->clear();

    uniqComboBox->addItem(tr("Any"), QVariant::fromValue(-1));

    if ((ci & ~CF_LEVEL) != 0) {
        for (int i = 0; i < NUM_UITEM; i++) {
            const UniqItemData &uid = UniqueItemList[i];
            if (uid.UIUniqType == AllItemsList[idx].iUniqType && uid.UIMinLvl <= lvl) {
                uniqComboBox->addItem(uid.UIName, QVariant::fromValue(i));
            }
        }
    }

    uniqComboBox->addItem(tr("None"), QVariant::fromValue(-2));
    uniqComboBox->setEnabled(drop);
    uniqComboBox->adjustSize();
    si = uniqComboBox->findData(QVariant::fromValue(this->wishUniq));
    if (si < 0) si = 0;
    uniqComboBox->setCurrentIndex(si);

    // update possible affixes
    QComboBox *preComboBox = this->ui->itemPrefixComboBox;
    QComboBox *sufComboBox = this->ui->itemSuffixComboBox;
    preComboBox->clear();
    sufComboBox->clear();

    int uniqIdx = uniqComboBox->currentData().value<int>();
    if (uniqIdx < 0) {
        preComboBox->addItem(tr("Any"), QVariant::fromValue(-1));
        sufComboBox->addItem(tr("Any"), QVariant::fromValue(-1));

        if ((ci & ~CF_LEVEL) != 0) {
            si = 0;
            for (const AffixData *pres = PL_Prefix; pres->PLPower != IPL_INVALID; pres++, si++) {
                if ((flgs & pres->PLIType)
                    && pres->PLRanges[range].from <= lvl && pres->PLRanges[range].to >= lvl) {
                    preComboBox->addItem(QString("%1 (%2-%3)").arg(AffixName(pres)).arg(pres->PLParam1).arg(pres->PLParam2), QVariant::fromValue(si));
                }
            }
            si = 0;
            for (const AffixData *sufs = PL_Suffix; sufs->PLPower != IPL_INVALID; sufs++, si++) {
                if ((flgs & sufs->PLIType)
                    && sufs->PLRanges[range].from <= lvl && sufs->PLRanges[range].to >= lvl) {
                    sufComboBox->addItem(QString("%1 (%2-%3)").arg(AffixName(sufs)).arg(sufs->PLParam1).arg(sufs->PLParam2), QVariant::fromValue(si));
                }
            }
        }
        preComboBox->addItem(tr("None"), QVariant::fromValue(-2));
        sufComboBox->addItem(tr("None"), QVariant::fromValue(-2));
    } else {
        // if ((ci & ~CF_LEVEL) != 0) {
            const UniqItemData* ui = &UniqueItemList[uniqIdx];
            addUniqueOption(ui->UIPower1, ui->UIParam1a, ui->UIParam1b, 0, preComboBox, sufComboBox);
            if (ui->UIPower2 != IPL_INVALID) {
                addUniqueOption(ui->UIPower2, ui->UIParam2a, ui->UIParam2b, 1, preComboBox, sufComboBox);
            if (ui->UIPower3 != IPL_INVALID) {
                addUniqueOption(ui->UIPower3, ui->UIParam3a, ui->UIParam3b, 2, preComboBox, sufComboBox);
            if (ui->UIPower4 != IPL_INVALID) {
                addUniqueOption(ui->UIPower4, ui->UIParam4a, ui->UIParam4b, 3, preComboBox, sufComboBox);
            if (ui->UIPower5 != IPL_INVALID) {
                addUniqueOption(ui->UIPower5, ui->UIParam5a, ui->UIParam5b, 4, preComboBox, sufComboBox);
            if (ui->UIPower6 != IPL_INVALID) {
                addUniqueOption(ui->UIPower6, ui->UIParam6a, ui->UIParam6b, 5, preComboBox, sufComboBox);
            }}}}}
        // }
        if (preComboBox->count() == 0)
            preComboBox->addItem(tr("Any"), QVariant::fromValue(-1));
        if (sufComboBox->count() == 0)
            sufComboBox->addItem(tr("Any"), QVariant::fromValue(-1));
    }
    preComboBox->setEnabled(drop);
    sufComboBox->setEnabled(drop);

    preComboBox->adjustSize();
    sufComboBox->adjustSize();
    si = preComboBox->findData(QVariant::fromValue(this->wishPre));
    if (si < 0) si = 0;
    preComboBox->setCurrentIndex(si);
    si = sufComboBox->findData(QVariant::fromValue(this->wishSuf));
    if (si < 0) si = 0;
    sufComboBox->setCurrentIndex(si);

    si = preComboBox->currentData().value<int>();
    this->ui->itemPrefixLimitedCheckBox->setVisible(si >= 0);
    Qt::CheckState cs = this->ui->itemPrefixLimitedCheckBox->checkState();
    this->ui->itemPrefixLimitedCheckBox->setToolTip(cs == Qt::Unchecked ? tr("unrestricted") : (cs == Qt::PartiallyChecked ? tr("lower limited to:") : tr("upper limited to:")));
    this->ui->itemPrefixLimitSlider->setVisible(cs != Qt::Unchecked && si >= 0 && (uniqIdx >= 0 || (PL_Prefix[si].PLParam1 != PL_Prefix[si].PLParam2)));
    if (si >= 0 && (uniqIdx >= 0 || (PL_Prefix[si].PLParam1 != PL_Prefix[si].PLParam2))) {
        int minval, maxval;
        if (uniqIdx >= 0) {
            switch (si) {
            case 0: minval = ui->UIParam1a; maxval = ui->UIParam1b; break;
            case 1: minval = ui->UIParam2a; maxval = ui->UIParam2b; break;
            case 2: minval = ui->UIParam3a; maxval = ui->UIParam3b; break;
            case 3: minval = ui->UIParam4a; maxval = ui->UIParam4b; break;
            case 4: minval = ui->UIParam5a; maxval = ui->UIParam5b; break;
            case 5: minval = ui->UIParam6a; maxval = ui->UIParam6b; break;
            }
        } else {
            minval = PL_Prefix[si].PLParam1;
            maxval = PL_Prefix[si].PLParam2;
        }
        this->ui->itemPrefixLimitSlider->setMinimum(minval);
        this->ui->itemPrefixLimitSlider->setMaximum(maxval);
        if (this->resetSlider & 1) {
            this->resetSlider &= ~1;
            this->ui->itemPrefixLimitSlider->setValue(cs == Qt::Checked ? maxval : minval);
        }
        this->ui->itemPrefixLimitSlider->setToolTip(QString::number(this->ui->itemPrefixLimitSlider->value()));
    }

    si = sufComboBox->currentData().value<int>();
    this->ui->itemSuffixLimitedCheckBox->setVisible(si >= 0);
    cs = this->ui->itemSuffixLimitedCheckBox->checkState();
    this->ui->itemSuffixLimitedCheckBox->setToolTip(cs == Qt::Unchecked ? tr("unrestricted") : (cs == Qt::PartiallyChecked ? tr("lower limited to:") : tr("upper limited to:")));
    this->ui->itemSuffixLimitSlider->setVisible(cs != Qt::Unchecked && si >= 0 && (uniqIdx >= 0 || (PL_Suffix[si].PLParam1 != PL_Suffix[si].PLParam2)));
    if (si >= 0 && (uniqIdx >= 0 || (PL_Suffix[si].PLParam1 != PL_Suffix[si].PLParam2))) {
        if (uniqIdx >= 0) {
            switch (si) {
            case 0: minval = ui->UIParam1a; maxval = ui->UIParam1b; break;
            case 1: minval = ui->UIParam2a; maxval = ui->UIParam2b; break;
            case 2: minval = ui->UIParam3a; maxval = ui->UIParam3b; break;
            case 3: minval = ui->UIParam4a; maxval = ui->UIParam4b; break;
            case 4: minval = ui->UIParam5a; maxval = ui->UIParam5b; break;
            case 5: minval = ui->UIParam6a; maxval = ui->UIParam6b; break;
            }
        } else {
            minval = PL_Suffix[si].PLParam1;
            maxval = PL_Suffix[si].PLParam2;
        }
        this->ui->itemSuffixLimitSlider->setMinimum(minval);
        this->ui->itemSuffixLimitSlider->setMaximum(maxval);
        if (this->resetSlider & 2) {
            this->resetSlider &= ~2;
            this->ui->itemSuffixLimitSlider->setValue(cs == Qt::Checked ? maxval : minval);
        }
        this->ui->itemSuffixLimitSlider->setToolTip(QString::number(this->ui->itemSuffixLimitSlider->value()));
    }
}

void ItemSelectorDialog::on_itemTypeComboBox_activated(int index)
{
    this->itemType = this->ui->itemTypeComboBox->currentData().value<int>();
    this->updateFilters();
}

void ItemSelectorDialog::on_itemLocComboBox_activated(int index)
{
    this->updateFilters();
}

void ItemSelectorDialog::on_itemIdxComboBox_activated(int index)
{
    // this->is->_iIdx = this->ui->itemIdxComboBox->currentData().value<int>();
    this->updateFields();
}

void ItemSelectorDialog::on_itemSeedEdit_returnPressed()
{
    bool ok;
    QString seedTxt = this->ui->itemSeedEdit->text();
    int seed = seedTxt.toInt(&ok);
    if (ok || seedTxt.isEmpty()) {
        this->is->_iSeed = seed;
    } else {
        QMessageBox::critical(this, "Error", "Failed to parse the seed to a 32-bit integer.");
    }

    this->on_itemSeedEdit_escPressed();
}

void ItemSelectorDialog::on_itemSeedEdit_escPressed()
{
    this->updateFields();
    this->ui->itemSeedEdit->clearFocus();
}

void ItemSelectorDialog::on_actionGenerateSeed_triggered()
{
    QRandomGenerator *gen = QRandomGenerator::global();
    this->is->_iSeed = (int)gen->generate();
    this->updateFields();
}

void ItemSelectorDialog::on_itemLevelEdit_returnPressed()
{
    this->is->_iCreateInfo = (this->is->_iCreateInfo & ~CF_LEVEL) | (this->ui->itemLevelEdit->text().toShort() & CF_LEVEL);

    this->on_itemLevelEdit_escPressed();
}

void ItemSelectorDialog::on_itemLevelEdit_escPressed()
{
    this->updateFields();
    this->ui->itemLevelEdit->clearFocus();
}

void ItemSelectorDialog::on_itemSourceComboBox_activated(int index)
{
    this->is->_iCreateInfo = (this->is->_iCreateInfo & ~CF_TOWN) | (index << 8);
    this->updateFields();
}

void ItemSelectorDialog::on_itemQualityComboBox_activated(int index)
{
    this->is->_iCreateInfo = (this->is->_iCreateInfo & ~CF_DROP_QUALITY) | (index << 11);
    this->updateFields();
}

void ItemSelectorDialog::on_itemPrefixComboBox_activated(int index)
{
    this->wishPre = this->ui->itemPrefixComboBox->itemData(index).value<int>();
    this->resetSlider |= 1;
    this->updateFields();
}

void ItemSelectorDialog::on_itemSuffixComboBox_activated(int index)
{
    this->wishSuf = this->ui->itemSuffixComboBox->itemData(index).value<int>();
    this->resetSlider |= 2;
    this->updateFields();
}

void ItemSelectorDialog::on_itemPrefixLimitedCheckBox_clicked()
{
    this->updateFields();
}

void ItemSelectorDialog::on_itemSuffixLimitedCheckBox_clicked()
{
    this->updateFields();
}

void ItemSelectorDialog::on_itemPrefixLimitSlider_sliderMoved(int value)
{
    this->ui->itemPrefixLimitSlider->setToolTip(QString::number(value));
}

void ItemSelectorDialog::on_itemSuffixLimitSlider_sliderMoved(int value)
{
    this->ui->itemSuffixLimitSlider->setToolTip(QString::number(value));
}

bool ItemSelectorDialog::recreateItem()
{
    /*bool ok;
    QString seedTxt = this->ui->itemSeedEdit->text();
    int seed = seedTxt.toInt(&ok);
    if (!ok && !seedTxt.isEmpty()) {
        QMessageBox::critical(this, "Error", "Failed to parse the seed to a 32-bit integer.");
        return false;
    }
    int wCI = this->ui->itemLevelEdit->text().toShort();
    wCI &= CF_LEVEL;
    wCI |= this->ui->itemSourceComboBox->currentIndex() << 8;
    wCI |= this->ui->itemQualityComboBox->currentIndex() << 11;

    int wIdx = this->ui->itemIdxComboBox->currentData().value<int>();*/
    int seed = this->is->_iSeed;
    int wCI = this->is->_iCreateInfo;
    int wIdx = this->is->_iIdx;

    int uniqIdx = this->ui->itemUniquesComboBox->currentData().value<int>();
    int preIdx = this->ui->itemPrefixComboBox->currentData().value<int>();
    int sufIdx = this->ui->itemSuffixComboBox->currentData().value<int>();

    typedef struct UIAffixData {
        bool active;
        BYTE power;
        int param1;
        int param2;
    } UIAffixData;
    UIAffixData prefix = { false, 0 };
    UIAffixData suffix = { false, 0 };
    prefix.active = preIdx != -1;
    if (prefix.active) {
        if (preIdx >= 0) {
            if (uniqIdx >= 0) {
                const UniqItemData* ui = &UniqueItemList[uniqIdx];
                switch (preIdx) {
                case 0: prefix.power = ui->UIPower1; prefix.param1 = ui->UIParam1a; prefix.param2 = ui->UIParam1b; break;
                case 1: prefix.power = ui->UIPower2; prefix.param1 = ui->UIParam2a; prefix.param2 = ui->UIParam2b; break;
                case 2: prefix.power = ui->UIPower3; prefix.param1 = ui->UIParam3a; prefix.param2 = ui->UIParam3b; break;
                case 3: prefix.power = ui->UIPower4; prefix.param1 = ui->UIParam4a; prefix.param2 = ui->UIParam4b; break;
                case 4: prefix.power = ui->UIPower5; prefix.param1 = ui->UIParam5a; prefix.param2 = ui->UIParam5b; break;
                case 5: prefix.power = ui->UIPower6; prefix.param1 = ui->UIParam6a; prefix.param2 = ui->UIParam6b; break;
                }
            } else {
                const AffixData *affix = &PL_Prefix[preIdx];
                prefix.power = affix->PLPower;
                prefix.param1 = affix->PLParam1;
                prefix.param2 = affix->PLParam2;
            }
            if (this->ui->itemPrefixLimitSlider->isVisible()) {
                int val = this->ui->itemPrefixLimitSlider->value();
                if (this->ui->itemPrefixLimitedCheckBox->checkState() == Qt::PartiallyChecked) {
                    prefix.param1 = val;
                } else {
                    prefix.param2 = val;
                }
            }
        } else {
            prefix.power = IPL_INVALID;
        }
    }
    suffix.active = sufIdx != -1;
    if (suffix.active) {
        if (sufIdx >= 0) {
            if (uniqIdx >= 0) {
                const UniqItemData* ui = &UniqueItemList[uniqIdx];
                switch (sufIdx) {
                case 0: suffix.power = ui->UIPower1; suffix.param1 = ui->UIParam1a; suffix.param2 = ui->UIParam1b; break;
                case 1: suffix.power = ui->UIPower2; suffix.param1 = ui->UIParam2a; suffix.param2 = ui->UIParam2b; break;
                case 2: suffix.power = ui->UIPower3; suffix.param1 = ui->UIParam3a; suffix.param2 = ui->UIParam3b; break;
                case 3: suffix.power = ui->UIPower4; suffix.param1 = ui->UIParam4a; suffix.param2 = ui->UIParam4b; break;
                case 4: suffix.power = ui->UIPower5; suffix.param1 = ui->UIParam5a; suffix.param2 = ui->UIParam5b; break;
                case 5: suffix.power = ui->UIPower6; suffix.param1 = ui->UIParam6a; suffix.param2 = ui->UIParam6b; break;
                }
            } else {
                const AffixData *affix = &PL_Suffix[sufIdx];
                suffix.power = affix->PLPower;
                suffix.param1 = affix->PLParam1;
                suffix.param2 = affix->PLParam2;
            }
            if (this->ui->itemSuffixLimitSlider->isVisible()) {
                int val = this->ui->itemSuffixLimitSlider->value();
                if (this->ui->itemSuffixLimitedCheckBox->checkState() == Qt::PartiallyChecked) {
                    suffix.param1 = val;
                } else {
                    suffix.param2 = val;
                }
            }
        } else {
            suffix.power = IPL_INVALID;
        }
    }

    int counter = 0;
start:
    RecreateItem(seed, wIdx, wCI);

    if (uniqIdx == -1) {
        if (prefix.active) {
            if (items[MAXITEMS]._iPrePower != prefix.power) {
                // LogErrorF("missed prefix %d vs %d (%d) seed%d", items[MAXITEMS]._iPrePower, affix->PLPower, preIdx, seed);
                goto restart;
            }
            if (prefix.power != IPL_INVALID && (affix_rnd[0] < prefix.param1 || affix_rnd[0] > prefix.param2)) {
                // LogErrorF("missed preval %d vs [%d:%d]", affix_rnd[0], affix->PLParam1, affix->PLParam2);
                goto restart;
            }
        }
        if (suffix.active) {
            if (items[MAXITEMS]._iSufPower != suffix.power) {
                // LogErrorF("missed prefix %d vs %d (%d) seed%d", items[MAXITEMS]._iPrePower, affix->PLPower, sufIdx);
                goto restart;
            }
            if (suffix.power != IPL_INVALID && (affix_rnd[1] < suffix.param1 || affix_rnd[1] > suffix.param2)) {
                // LogErrorF("missed sufval %d vs [%d:%d]", affix_rnd[1], affix->PLParam1, affix->PLParam2);
                goto restart;
            }
        }
    } else {
        if (uniqIdx >= 0) {
            if (items[MAXITEMS]._iMagical != ITEM_QUALITY_UNIQUE || items[MAXITEMS]._iUid != uniqIdx) {
                goto restart;
            }
            if (prefix.active) {
                if (affix_rnd[preIdx] < prefix.param1 || affix_rnd[preIdx] > prefix.param2) goto restart;
            }
            if (suffix.active) {
                if (affix_rnd[sufIdx] < suffix.param1 || affix_rnd[sufIdx] > suffix.param2) goto restart;
            }
        } else {
            if (items[MAXITEMS]._iMagical == ITEM_QUALITY_UNIQUE) {
                goto restart;
            }
        }
    }
    goto done;
restart:
    counter++;
    seed = NextRndSeed();
    goto start;
done:

    items[MAXITEMS]._iIdentified = TRUE;
    memcpy(this->is, &items[MAXITEMS], sizeof(ItemStruct));
    if (counter != 0) {
        dProgress() << tr("Succeeded after %1 iterations.").arg(counter + 1);
    }
    return true;
}


void ItemSelectorDialog::on_generateButton_clicked()
{
    if (this->recreateItem()) {
        this->updateFields();
    }
}

void ItemSelectorDialog::on_submitButton_clicked()
{
    if (this->recreateItem()) {
        dMainWindow().addHeroItem(this->invIdx, this->is);

        this->close();
    }
}

void ItemSelectorDialog::on_cancelButton_clicked()
{
    this->close();
}
