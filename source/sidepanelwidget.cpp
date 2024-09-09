#include "sidepanelwidget.h"

#include <QMessageBox>

#include "d1hro.h"
#include "ui_sidepanelwidget.h"

#include "dungeon/interfac.h"

SidePanelWidget::SidePanelWidget(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SidePanelWidget())
{
    this->ui->setupUi(this);
}

SidePanelWidget::~SidePanelWidget()
{
    delete ui;
}

void SidePanelWidget::initialize(D1Hero *h)
{
    this->hero = h;
    if (this->itemDetails == nullptr) {
        this->itemDetails = new ItemDetailsWidget(this);

        // clear the layout
        QVBoxLayout *layout = this->ui->panelVBoxLayout;
        /*QLayoutItem *child;
        while ((child = layout->takeAt(0)) != nullptr) {
            child->widget()->deleteLater(); // delete the widget
            delete child;           // delete the layout item
        }*/
        for (int i = layout->count() - 1; i >= 0; i--) {
            QWidget *w = layout->itemAt(i)->widget();
            if (w != nullptr) {
                w->deleteLater();
            }
        }

        layout->addWidget(this->itemDetails, 0, Qt::AlignTop);
    }
}

void SidePanelWidget::setHeroItem(D1Hero *h, int ii)
{
    LogErrorF("SidePanelWidget::setHeroItem 0 %d", ii);
    this->initialize(h);
    LogErrorF("SidePanelWidget::setHeroItem 1 %d", ii);
    this->itemDetails->initialize(h, ii);
    LogErrorF("SidePanelWidget::setHeroItem 2 %d", ii);
}