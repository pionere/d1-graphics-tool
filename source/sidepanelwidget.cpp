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
}

void SidePanelWidget::initialize(D1Hero *h)
{
    this->hero = h;
    if (this->itemSelector == nullptr) {
        this->itemSelector = new ItemSelectorWidget(this);

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

        layout->addWidget(this->itemSelector, 0, Qt::AlignTop);
    }
}

void SidePanelWidget::setHeroItem(D1Hero *h, int ii)
{
    this->initialize(h);
    this->itemSelector->initialize(h, ii);


}
