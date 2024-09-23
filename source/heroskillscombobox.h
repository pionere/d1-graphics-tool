#pragma once

#include <QComboBox>

class D1Hero;

class HeroSkillsComboBox : public QComboBox {
    Q_OBJECT

public:
    HeroSkillsComboBox(QWidget *parent = nullptr);
    ~HeroSkillsComboBox() = default;

    void setHero(D1Hero *hero);
    int update();

private:
    D1Hero *hero;
};
