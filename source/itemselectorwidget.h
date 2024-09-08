#pragma once

#include <QWidget>

class SidePanelWidget;
class D1Hero;
class ItemStruct;

namespace Ui {
class ItemSelectorWidget;
} // namespace Ui

class ItemSelectorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ItemSelectorWidget(SidePanelWidget *parent);
    ~ItemSelectorWidget();

    void initialize(D1Hero *hero, int invIdx); // inv_item

private slots:
    void on_itemTypeComboBox_activated(int index);
    void on_itemLocComboBox_activated(int index);

    void on_submitButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::ItemSelectorWidget *ui;
    SidePanelWidget *view;

    D1Hero *hero;
    int invIdx;
    ItemStruct is;
};
