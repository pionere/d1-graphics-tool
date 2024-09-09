#pragma once

#include <QWidget>

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

private:
    void updateFields();

private slots:
    void on_invItemIndexComboBox_activated(int index);

    void on_addItemButton_clicked();

    void on_submitButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::ItemDetailsWidget *ui;
    SidePanelWidget *view;

    D1Hero *hero;
    int invIdx;
};
