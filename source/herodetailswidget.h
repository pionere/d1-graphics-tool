#pragma once

#include <QWidget>

class D1Hero;

namespace Ui {
class HeroDetailsWidget;
} // namespace Ui

class HeroDetailsWidget : public QWidget {
    Q_OBJECT

public:
    explicit HeroDetailsWidget(QWidget *parent);
    ~HeroDetailsWidget();

    void initialize(D1Hero *h);
    void setReadOnly();
    void displayFrame();

private:
    void updateFields();

private slots:
    //void on_gameHellfireCheckBox_clicked();
    //void on_gameMultiCheckBox_clicked();
    //void on_gameDifficultyComboBox_activated(int index);

    void on_heroNameEdit_returnPressed();
    void on_heroNameEdit_escPressed();
    void on_heroClassComboBox_activated(int index);
    void on_heroDecLevelButton_clicked();
    void on_heroIncLevelButton_clicked();
    void on_heroLevelEdit_returnPressed();
    void on_heroLevelEdit_escPressed();
    void on_heroRankEdit_returnPressed();
    void on_heroRankEdit_escPressed();

    // void on_heroSkillsButton_clicked();
    // void on_heroMonstersButton_clicked();

    void on_heroDecLifeButton_clicked();
    void on_heroRestoreLifeButton_clicked();
    void on_heroSubStrengthButton_clicked();
    void on_heroStrengthEdit_returnPressed();
    void on_heroStrengthEdit_escPressed();
    void on_heroAddStrengthButton_clicked();
    void on_heroSubDexterityButton_clicked();
    void on_heroDexterityEdit_returnPressed();
    void on_heroDexterityEdit_escPressed();
    void on_heroAddDexterityButton_clicked();
    void on_heroSubMagicButton_clicked();
    void on_heroMagicEdit_returnPressed();
    void on_heroMagicEdit_escPressed();
    void on_heroAddMagicButton_clicked();
    void on_heroSubVitalityButton_clicked();
    void on_heroVitalityEdit_returnPressed();
    void on_heroVitalityEdit_escPressed();
    void on_heroAddVitalityButton_clicked();

private:
    Ui::HeroDetailsWidget *ui;

    D1Hero *hero;
    bool readOnly = false;
};
