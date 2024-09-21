#pragma once

#include <QWidget>

#include "itempropertieswidget.h"
#include "popupdialog.h"

class SidePanelWidget;
class D1Hero;

namespace Ui {
class ItemDetailsWidget;
} // namespace Ui

class ItemDetailsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ItemDetailsWidget(SidePanelWidget *parent);
    ~ItemDetailsWidget();

    void initialize(D1Hero *hero, int invIdx); // inv_item
    void displayFrame();

private:
    void updateFields();

private slots:
    void on_invItemIndexComboBox_activated(int index);

    void on_editNameButton_clicked();
    void on_discardItemButton_clicked();
    void on_addItemButton_clicked();

    void on_selectButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::ItemDetailsWidget *ui;
    SidePanelWidget *view;
    ItemPropertiesWidget *itemProps;

    PopupDialog *namePopupDialog = nullptr;

    D1Hero *hero;
    int invIdx;
    int currentItem;
};
