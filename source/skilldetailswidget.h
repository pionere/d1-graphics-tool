#pragma once

#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

#include "lineeditwidget.h"

class D1Hero;

namespace Ui {
class SkillDetailsWidget;
} // namespace Ui

class SkillDetailsWidget;

class SkillPushButton : public QPushButton {
    Q_OBJECT

public:
    explicit SkillPushButton(int sn, SkillDetailsWidget *parent);
    ~SkillPushButton() = default;

private slots:
    void on_btn_clicked();

private:
    int sn;
    SkillDetailsWidget *sdw;
};

class SkillSpinBox : public QSpinBox {
    Q_OBJECT

public:
    explicit SkillSpinBox(int sn, SkillDetailsWidget *parent);
    ~SkillSpinBox() = default;

    void stepBy(int steps) override;
    void changeValue(int value);

private slots:
    void on_value_changed(int value);

private:
    int sn;
    SkillDetailsWidget *sdw;
};

class SkillDetailsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SkillDetailsWidget(QWidget *parent);
    ~SkillDetailsWidget();

    void initialize(D1Hero *hero);
    void displayFrame();

    void on_skill_clicked(int sn);
    void on_skill_changed(int sn, int value);

private:
    void updateFields();

private slots:
    void on_resetButton_clicked();
    void on_maxButton_clicked();

    void on_submitButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::SkillDetailsWidget *ui;

    D1Hero *hero;
    // LineEditWidget *skillWidgets[64];
    SkillSpinBox *skillWidgets[64];
    int skills[64];
    int currentSkill = -1;
};
