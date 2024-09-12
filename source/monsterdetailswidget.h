#pragma once

#include <QWidget>

class SidePanelWidget;
class D1Hero;

namespace Ui {
class MonsterDetailsWidget;
} // namespace Ui

class MonsterDetailsWidget : public QWidget {
    Q_OBJECT

public:
    explicit MonsterDetailsWidget(SidePanelWidget *parent);
    ~MonsterDetailsWidget();

    void initialize(D1Hero *hero);
    void displayFrame();

private:
    void updateFields();

private slots:
    void on_difficutlyComboBox_activated(int index);
    void on_plrCountSpinBox_valueChanged(int value);
    void on_dunSeedEdit_returnPressed();
    void on_dunSeedEdit_escPressed();
    void on_actionGenerateSeed_triggered();

    void on_dunTypeComboBox_activated(int index);

    void on_dunLevelEdit_returnPressed();
    void on_dunLevelEdit_escPressed();

    void on_dunLevelBonusEdit_returnPressed();
    void on_dunLevelBonusEdit_escPressed();

    void on_monTypeComboBox_activated(int index);

    void on_closeButton_clicked();

private:
    Ui::MonsterDetailsWidget *ui;

    D1Hero *hero;
    int dunSeed = 0;
    int dunLevel = 0;
    int dunLevelBonus = 0;
};