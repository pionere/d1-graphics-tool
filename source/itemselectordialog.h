#pragma once

#include <QDialog>

class D1Hero;
struct ItemStruct;

namespace Ui {
class ItemSelectorDialog;
} // namespace Ui

class ItemSelectorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ItemSelectorDialog(QWidget *view);
    ~ItemSelectorDialog();

    void initialize(D1Hero *hero, int invIdx); // inv_item

private:
    void updateFields();
    bool recreateItem();

private slots:
    void on_itemTypeComboBox_activated(int index);
    void on_itemLocComboBox_activated(int index);
    void on_itemIdxComboBox_activated(int index);

    void on_itemSeedEdit_returnPressed();
    void on_itemSeedEdit_escPressed();
    void on_itemLevelEdit_returnPressed();
    void on_itemLevelEdit_escPressed();

    void on_itemSourceComboBox_activated(int index);
    void on_itemQualityComboBox_activated(int index);

    void on_submitButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::ItemSelectorDialog *ui;

    D1Hero *hero;
    int invIdx;
    ItemStruct *is = nullptr;
};
