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
    : QWidget(parent)
    , ui(new Ui::ItemSelectorDialog())
{
    ui->setupUi(this);

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
    } else {
        memset(this->is, 0, sizeof(ItemStruct));
    } 
    
    QComboBox *typeComboBox = this->ui->itemTypeComboBox;
    QComboBox *locComboBox = this->ui->itemLocComboBox;
    QComboBox *idxComboBox = this->ui->itemIdxComboBox;
    typeComboBox->clear();
    locComboBox->clear();
    idxComboBox->clear();
    
    typeComboBox->addItem(tr("None"), ITYPE_NONE);
    switch (ii) {
    case INVITEM_HEAD:
        // this->is->_iClass = ICLASS_ARMOR;
        locComboBox->addItem(tr("Helm"), QVariant::fromValue(ILOC_HELM));
        typeComboBox->addItem(tr("Helm"), QVariant::fromValue(ITYPE_HELM));
        break;
    case INVITEM_RING_LEFT:
        // this->is->_iClass = ICLASS_MISC;
        locComboBox->addItem(tr("Ring"), QVariant::fromValue(ILOC_RING));
        typeComboBox->addItem(tr("Ring"), QVariant::fromValue(ITYPE_RING));
        break;
    case INVITEM_RING_RIGHT:
        // this->is->_iClass = ICLASS_MISC;
        locComboBox->addItem(tr("Ring"), QVariant::fromValue(ILOC_RING)));
        typeComboBox->addItem(tr("Ring"), QVariant::fromValue(ITYPE_RING)));
        break;
    case INVITEM_AMULET:
        this->is->_iClass = ICLASS_MISC;
        locComboBox->addItem(tr("Amulet"), QVariant::fromValue(ILOC_AMULET)));
        typeComboBox->addItem(tr("Amulet"), QVariant::fromValue(ITYPE_AMULET)));
        break;
    case INVITEM_HAND_LEFT:
        // this->is->_iClass = ICLASS_WEAPON;
        locComboBox->addItem(tr("One handed"), QVariant::fromValue(ILOC_ONEHAND));
        locComboBox->addItem(tr("Two handed"), QVariant::fromValue(ILOC_TWOHAND));
        typeComboBox->addItem(tr("Sword"), QVariant::fromValue(ITYPE_SWORD));
        typeComboBox->addItem(tr("Axe"), QVariant::fromValue(ITYPE_AXE));
        typeComboBox->addItem(tr("Bow"), QVariant::fromValue(ITYPE_BOW));
        typeComboBox->addItem(tr("Mace"), QVariant::fromValue(ITYPE_MACE));
        typeComboBox->addItem(tr("Staff"), QVariant::fromValue(ITYPE_STAFF));
        break;
    case INVITEM_HAND_RIGHT:
        // this->is->_iClass = ICLASS_WEAPON;
        locComboBox->addItem(tr("One handed"), QVariant::fromValue(ILOC_ONEHAND));
        typeComboBox->addItem(tr("Sword"), QVariant::fromValue(ITYPE_SWORD));
        typeComboBox->addItem(tr("Axe"), QVariant::fromValue(ITYPE_AXE));
        typeComboBox->addItem(tr("Bow"), QVariant::fromValue(ITYPE_BOW));
        typeComboBox->addItem(tr("Mace"), QVariant::fromValue(ITYPE_MACE));
        typeComboBox->addItem(tr("Staff"), QVariant::fromValue(ITYPE_STAFF));
        typeComboBox->addItem(tr("Shield"), QVariant::fromValue(ITYPE_SHIELD));
        break;
    case INVITEM_CHEST:
        // this->is->_iClass = ICLASS_ARMOR;
        locComboBox->addItem(tr("Armor"), QVariant::fromValue(ILOC_ARMOR));
        typeComboBox->addItem(tr("Light"), QVariant::fromValue(ITYPE_LARMOR));
        typeComboBox->addItem(tr("Medium"), QVariant::fromValue(ITYPE_MARMOR));
        typeComboBox->addItem(tr("Heavy"), QVariant::fromValue(ITYPE_HARMOR));
        break;
    }

    for (int i = 0; i < NUM_IDI; ++i) {
        const ItemData &id = AllItemsList[i];
        //if (id.iClass != this->is->_iClass) {
        //    continue;
        //}
        if (typeComboBox->findData(id.itype) < 0) {
            continue;
        }
        idxComboBox->addItem(QString("%1 (%2)").arg(id.iName).arg(i), QVariant::fromValue(i));
    }

    if (this->is->_itype == ITYPE_NONE) {
        // this->is->_iSeed = 0;
        this->is->_iLoc = locComboBox->itemData(0).value<int>();
        this->is->_itype = typeComboBox->itemData(0).value<int>();
        this->is->_iIdx = idxComboBox->itemData(0).value<int>();
        this->is->_iCreateInfo = 1 | (CFL_NONE << 8) | (CFDQ_NORMAL << 11);
    }

    this->updateFields();
}

void ItemSelectorDialog::updateFields()
{
    QComboBox *typeComboBox = this->ui->itemTypeComboBox;
    QComboBox *locComboBox = this->ui->itemLocComboBox;
    QComboBox *idxComboBox = this->ui->itemIdxComboBox;

    locComboBox->setCurrentIndex(locComboBox->findData(this->is->_iLoc));
    typeComboBox->setCurrentIndex(typeComboBox->findData(this->is->_itype));
    idxComboBox->setCurrentIndex(idxComboBox->findData(this->is->_iIdx));

    this->ui->itemSeedEdit->setText(QString::number(this->is->_iSeed));
    this->ui->itemLevelEdit->setText(QString::number(this->is->_iCreateInfo & CF_LEVEL));
    static_assert(((int)CF_TOWN & ((1 << (8 + 1)) - 1)) == 0, "ItemSelectorDialog hardcoded CF_TOWN must be adjusted I.");
    static_assert(((((int)CF_TOWN >> 8) & ((((int)CF_TOWN >> 8) + 1))) == 0, "ItemSelectorDialog hardcoded CF_TOWN must be adjusted II.");
    this->ui->itemSourceComboBox->setCurrentIndex((this->is->_iCreateInfo & CF_TOWN) >> 8);
    static_assert(((int)CF_DROP_QUALITY & ((1 << (11 + 1)) - 1)) == 0, "ItemSelectorDialog hardcoded CF_DROP_QUALITY must be adjusted I.");
    static_assert(((((int)CF_DROP_QUALITY >> 11) & ((((int)CF_DROP_QUALITY >> 11) + 1))) == 0, "ItemSelectorDialog hardcoded CF_DROP_QUALITY must be adjusted II.");
    this->ui->itemQualityComboBox->setCurrentIndex((this->is->_iCreateInfo & CF_DROP_QUALITY) >> 11);
}

void ItemSelectorDialog::on_itemTypeComboBox_activated(int index)
{

}

void ItemSelectorDialog::on_itemLocComboBox_activated(int index)
{

}

void ItemSelectorDialog::on_itemIdxComboBox_activated(int index)
{
    this->is->_iIdx = this->ui->itemIdxComboBox->currentData().value<int>();
    // this->updateFields();
}

void ItemSelectorDialog::on_itemSeedEdit_returnPressed()
{
    this->is->_iSeed = this->ui->itemSeedEdit->text().toInt();

    this->on_itemSeedEdit_escPressed();
}

void ItemSelectorDialog::on_itemSeedEdit_escPressed()
{
    this->updateFields();
    this->ui->itemSeedEdit->clearFocus();
}

void ItemSelectorDialog::on_itemLevelEdit_returnPressed()
{
    this->is->_iCreateInfo = (this->is->_iCreateInfo & ~CF_LEVEL) | (this->ui->itemLevelEdit->toShort() & CF_LEVEL);

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
    // this->updateFields();
}

void ItemSelectorDialog::on_itemQualityComboBox_activated(int index)
{
    this->is->_iCreateInfo = (this->is->_iCreateInfo & ~CF_DROP_QUALITY) | (index << 11);
    // this->updateFields();
}

bool ItemSelectorDialog::recreateItem()
{
    bool ok;
    QString seedTxt = this->ui->itemSeedEdit->text();
    int seed = seedTxt.toInt(&ok);
    if (!ok && !seedTxt.isEmpty()) {
        QMessageBox::critical(this, "Error", "Failed to parse the seed to a 32-bit integer.");
        return false;
    }
    /*int wCI = this->ui->itemLevelEdit->toShort();
    wCI &= CF_LEVEL;
    wCI |= this->ui->itemSourceComboBox->currentIndex() << 8;
    wCI |= this->ui->itemQualityComboBox->currentIndex() << 11;

    int wIdx = this->ui->itemIdxComboBox->currentData().value<int>();*/
    int wCI = this->is->_iCreateInfo;
    int wIdx = this->is->_iIdx;

    RecreateItem(seed, wIdx, wCI);

    memcpy(this->is, &items[MAXITEMS], sizeof(ItemStruct));
    return true;
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
