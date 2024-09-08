#pragma once

#include <QDialog>
#include <QGraphicsScene>
#include <QMouseEvent>

#include "itemselectorwidget.h"

class D1Hero;

namespace Ui {
class SidePanelWidget;
}

class SidePanelWidget : public QDialog {
    Q_OBJECT

public:
    explicit SidePanelWidget(QWidget *parent);
    ~SidePanelWidget();

    void initialize(D1Hero *hero);
    void setHeroItem(D1Hero *hero, int ii);

private:
    Ui::SidePanelWidget *ui;

    D1Hero *hero;
    ItemSelectorWidget *itemSelector;
};
