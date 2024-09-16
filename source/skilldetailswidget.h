#pragma once

#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

#include "lineeditwidget.h"

class SidePanelWidget;
class D1Hero;

namespace Ui {
class SkillDetailsWidget;
} // namespace Ui


class SkillPushButton : public QPushButton {
    Q_OBJECT

public:
    explicit SkillPushButton(int sn, QWidget *parent);
    ~SkillPushButton();

private slots:
    void on_btn_clicked();

private:
    int sn;
};

class SkillDetailsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SkillDetailsWidget(SidePanelWidget *parent);
    ~SkillDetailsWidget();

    void initialize(D1Hero *hero);
    void displayFrame();

    void on_skill_clicked(int sn);

private:
    void updateFields();

private slots:
    void on_resetButton_clicked();
    void on_maxButton_clicked();

    void on_submitButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::SkillDetailsWidget *ui;
    SidePanelWidget *view;

    D1Hero *hero;
    // LineEditWidget *skillWidgets[64];
    QSpinBox *skillWidgets[64];
    int skills[64];
};
