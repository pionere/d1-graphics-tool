#include "itemselectorwidget.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>
#include <QWidgetAction>

#include "d1hro.h"
#include "sidepanelwidget.h"
#include "ui_itemselectorwidget.h"

#include "dungeon/all.h"

ItemSelectorWidget::ItemSelectorWidget(SidePanelWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ItemSelectorWidget())
    , view(parent)
{
    ui->setupUi(this);
}

ItemSelectorWidget::~ItemSelectorWidget()
{
    delete ui;
    delete this->is;
}

void ItemSelectorWidget::initialize(D1Hero *h, int ii)
{
    this->hero = h;
    this->invIdx = ii;

    const ItemStruct* pi = h->item(ii);

    delete this->is;
    is = new ItemStruct();
    memcpy(this->is, pi, sizeof(*pi));
    
    QComboBox *typeComboBox = this->ui->itemTypeComboBox;
    QComboBox *locComboBox = this->ui->itemLocComboBox;
    typeComboBox->clear();
    locComboBox->clear();
    
    typeComboBox->addItem(tr("None"), ITYPE_NONE);
    switch (ii) {
    case INVITEM_HEAD:
        this->is->_iClass = ICLASS_ARMOR;
        locComboBox->addItem(tr("Helm"), ILOC_HELM);
        typeComboBox->addItem(tr("Helm"), ITYPE_HELM);
        break;
    case INVITEM_RING_LEFT:
        this->is->_iClass = ICLASS_MISC;
        locComboBox->addItem(tr("Ring"), ILOC_RING);
        typeComboBox->addItem(tr("Ring"), ITYPE_RING);
        break;
    case INVITEM_RING_RIGHT:
        this->is->_iClass = ICLASS_MISC;
        locComboBox->addItem(tr("Ring"), ILOC_RING);
        typeComboBox->addItem(tr("Ring"), ITYPE_RING);
        break;
    case INVITEM_AMULET:
        this->is->_iClass = ICLASS_MISC;
        locComboBox->addItem(tr("Amulet"), ILOC_AMULET);
        typeComboBox->addItem(tr("Amulet"), ITYPE_AMULET);
        break;
    case INVITEM_HAND_LEFT:
        this->is->_iClass = ICLASS_WEAPON;
        locComboBox->addItem(tr("One handed"), ILOC_ONEHAND);
        locComboBox->addItem(tr("Two handed"), ILOC_TWOHAND);
        typeComboBox->addItem(tr("Sword"), ITYPE_SWORD);
        typeComboBox->addItem(tr("Axe"), ITYPE_AXE);
        typeComboBox->addItem(tr("Bow"), ITYPE_BOW);
        typeComboBox->addItem(tr("Mace"), ITYPE_MACE);
        typeComboBox->addItem(tr("Staff"), ITYPE_STAFF);
        break;
    case INVITEM_HAND_RIGHT:
        this->is->_iClass = ICLASS_WEAPON;
        locComboBox->addItem(tr("One handed"), ILOC_ONEHAND);
        typeComboBox->addItem(tr("Sword"), ITYPE_SWORD);
        typeComboBox->addItem(tr("Axe"), ITYPE_AXE);
        typeComboBox->addItem(tr("Bow"), ITYPE_BOW);
        typeComboBox->addItem(tr("Mace"), ITYPE_MACE);
        typeComboBox->addItem(tr("Staff"), ITYPE_STAFF);
        typeComboBox->addItem(tr("Shield"), ITYPE_SHIELD);
        break;
    case INVITEM_CHEST:
        this->is->_iClass = ICLASS_ARMOR;
        locComboBox->addItem(tr("Armor"), ILOC_ARMOR);
        typeComboBox->addItem(tr("Light"), ITYPE_LARMOR);
        typeComboBox->addItem(tr("Medium"), ITYPE_MARMOR);
        typeComboBox->addItem(tr("Heavy"), ITYPE_HARMOR);
        break;
    }

    if (pi->_itype != ITYPE_NONE) {
        locComboBox->setCurrentIndex(locComboBox->findData(pi->_iLoc));
        typeComboBox->setCurrentIndex(typeComboBox->findData(pi->_itype));
    }

    this->setVisible(true);
}

void ItemSelectorWidget::on_itemTypeComboBox_activated(int index)
{

}

void ItemSelectorWidget::on_itemLocComboBox_activated(int index)
{

}

void ItemSelectorWidget::on_submitButton_clicked()
{
    // ...

    this->setVisible(false);
}

void ItemSelectorWidget::on_cancelButton_clicked()
{
    this->setVisible(false);
}
