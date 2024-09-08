#include "sidepanelwidget.h"

#include <QMessageBox>

#include "d1hro.h"
#include "ui_sidepanelwidget.h"

SidePanelWidget::SidePanelWidget(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SidePanelWidget())
{
    this->ui->setupUi(this);
}

SidePanelWidget::~SidePanelWidget()
{
    delete ui;
    delete this->itemSelector;
}

void SidePanelWidget::initialize(D1Hero *h)
{
    this->itemSelector = new ItemSelectorWidget(this);

    this->ui->palettesVBoxLayout->addWidget(this->itemSelector, 0, Qt::AlignTop);
}

void SidePanelWidget::setHeroItem(D1Hero *h, int ii)
{
    // this->hero = h;
    this->itemSelector->initialize(h, ii);


}
