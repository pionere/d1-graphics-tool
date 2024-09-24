#include "itemdetailswidget.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>
#include <QWidgetAction>

#include "d1hro.h"
#include "mainwindow.h"
#include "sidepanelwidget.h"
#include "ui_itemdetailswidget.h"

#include "dungeon/all.h"

ItemDetailsWidget::ItemDetailsWidget(SidePanelWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ItemDetailsWidget())
    , view(parent)
{
    ui->setupUi(this);

    this->itemProps = new ItemPropertiesWidget(this);
    this->ui->itemProperties->addWidget(this->itemProps);
}

ItemDetailsWidget::~ItemDetailsWidget()
{
    delete ui;
    delete this->namePopupDialog;
}

void ItemDetailsWidget::initialize(D1Hero *h, int ii)
{
    this->hero = h;
    this->invIdx = ii;
    this->currentItem = ii;

    // LogErrorF("ItemDetailsWidget init 5");
    this->updateFields();
    // LogErrorF("ItemDetailsWidget init 6");
    this->setVisible(true);
}

void ItemDetailsWidget::displayFrame()
{
    this->updateFields();
}

void ItemDetailsWidget::updateFields()
{
    QComboBox *itemsComboBox = this->ui->invItemIndexComboBox;

    int ii = this->currentItem;
    itemsComboBox->clear();
    itemsComboBox->addItem(tr("None"), QVariant::fromValue(INVITEM_NONE));
    // LogErrorF("ItemDetailsWidget init 1 %d", ii);
    const ItemStruct* pi = this->hero->item(this->invIdx);
    if (pi->_itype != ITYPE_NONE) {
        // LogErrorF("ItemDetailsWidget init 2 %s", ItemName(pi));
        itemsComboBox->addItem(ItemName(pi), QVariant::fromValue(this->invIdx));
    }
    // LogErrorF("ItemDetailsWidget init 3");
    for (int i = INVITEM_INV_FIRST; i < NUM_INVELEM; i++) {
        const ItemStruct* is = this->hero->item(i);
        if (is->_itype == ITYPE_NONE || is->_itype == ITYPE_PLACEHOLDER) {
            continue;
        }
        switch (this->invIdx) {
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
        // LogErrorF("ItemDetailsWidget init 4 %s (%d) %d", ItemName(is), is->_itype, i);
        itemsComboBox->addItem(ItemName(is), QVariant::fromValue(i));
    }
    ii = itemsComboBox->findData(ii);
    if (ii < 0) ii = 0;
    itemsComboBox->setCurrentIndex(ii);
    itemsComboBox->adjustSize();
    ii = itemsComboBox->currentData().value<int>();

    // this->ui->discardItemButton->setEnabled(ii >= INVITEM_INV_FIRST && ii < NUM_INVELEM);
    this->ui->discardItemButton->setEnabled(ii != INVITEM_NONE);
    // LogErrorF("updateFields 0 %d", ii);
    pi = ii == INVITEM_NONE ? nullptr : this->hero->item(ii);
    if (pi != nullptr && pi->_itype != ITYPE_NONE) {
        this->ui->editNameButton->setVisible(true);
        QString text;
        // LogErrorF("updateFields 1 %d", pi->_itype);
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
        // LogErrorF("updateFields 2 %d", pi->_iClass);
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
        // LogErrorF("updateFields 3 %d", pi->_iLoc);
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
        // LogErrorF("updateFields 4 %d", pi->_iSeed);
        this->ui->itemSeedEdit->setText(QString::number(pi->_iSeed));
        // LogErrorF("updateFields 5 %d", pi->_iIdx);
        this->ui->itemIdxEdit->setText(QString("%1 (%2)").arg(pi->_iIdx < NUM_IDI ? AllItemsList[pi->_iIdx].iName : "").arg(pi->_iIdx));
        // LogErrorF("updateFields 6 %d", pi->_iCreateInfo & CF_LEVEL);
        this->ui->itemLevelEdit->setText(QString::number(pi->_iCreateInfo & CF_LEVEL));
        // LogErrorF("updateFields 7 %d", (pi->_iCreateInfo & CF_TOWN) >> 8);
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
        // LogErrorF("updateFields 7 %d", (pi->_iCreateInfo & CF_DROP_QUALITY) >> 11);
        text.clear();
        switch ((pi->_iCreateInfo & CF_DROP_QUALITY) >> 11) {
        case CFDQ_NONE:   text = tr("None");   break;
        case CFDQ_NORMAL: text = tr("Normal"); break;
        case CFDQ_GOOD:   text = tr("Good");   break;
        case CFDQ_UNIQUE: text = tr("Unique"); break;
        }
        this->ui->itemQualityEdit->setText(text);
        // LogErrorF("updateFields 8");
        this->ui->itemDetailsGroupBox->setVisible(true);

        this->itemProps->initialize(pi);
        this->itemProps->adjustSize();
        this->ui->itemPropertiesBox->adjustSize();
        this->itemProps->setVisible(true);
    } else {
        this->ui->editNameButton->setVisible(false);
        this->ui->itemDetailsGroupBox->setVisible(false);
        this->itemProps->setVisible(false);
    }
}

void ItemDetailsWidget::on_invItemIndexComboBox_activated(int index)
{
    this->currentItem = this->ui->invItemIndexComboBox->itemData(index).value<int>();

    this->updateFields();
}

void ItemDetailsWidget::on_editNameButton_clicked()
{
    if (this->namePopupDialog == nullptr) {
        this->namePopupDialog = new PopupDialog(this);
    }
    std::function<void(QString)> func = [this](QString text) {
        QComboBox *itemsComboBox = this->ui->invItemIndexComboBox;
        int ii = itemsComboBox->currentData().value<int>();
        if (ii != INVITEM_NONE) {
            this->hero->renameItem(ii, text);
            const ItemStruct* is = this->hero->item(ii);
            itemsComboBox->setItemText(itemsComboBox->currentIndex(), ItemName(is));
            // itemsComboBox->adjustSize();
            dMainWindow().updateWindow();
        }
    };
    this->namePopupDialog->initialize(tr("Name"), this->ui->invItemIndexComboBox->currentText(), std::move(func));
    this->namePopupDialog->show();
}

void ItemDetailsWidget::on_discardItemButton_clicked()
{
    QComboBox *itemsComboBox = this->ui->invItemIndexComboBox;

    int ii = itemsComboBox->currentData().value<int>();

    this->hero->swapInvItem(ii, INVITEM_NONE);

    // updateComboBox ...
    this->initialize(this->hero, this->invIdx);
    // dMainWindow().updateWindow();
}

void ItemDetailsWidget::on_addItemButton_clicked()
{
    dMainWindow().selectHeroItem(this->invIdx);
}

void ItemDetailsWidget::on_selectButton_clicked()
{
    QComboBox *itemsComboBox = this->ui->invItemIndexComboBox;

    int ii = itemsComboBox->currentData().value<int>();
    //LogErrorF("ItemDetailsWidget swap %d:%d", this->invIdx, ii);
    this->hero->swapInvItem(this->invIdx, ii);

    this->setVisible(false);
    //LogErrorF("ItemDetailsWidget swap 2");
    dMainWindow().updateWindow();
    //LogErrorF("ItemDetailsWidget swap 3");
}

void ItemDetailsWidget::on_cancelButton_clicked()
{
    this->setVisible(false);
}
