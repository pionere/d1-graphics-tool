#pragma once

#include <QWidget>

#include "lineeditwidget.h"

class SidePanelWidget;
class D1Hero;

namespace Ui {
class SkillDetailsWidget;
} // namespace Ui

class SkillDetailsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SkillDetailsWidget(SidePanelWidget *parent);
    ~SkillDetailsWidget();

    void initialize(D1Hero *hero);
    void displayFrame();

private:
    void updateFields();

private slots:
    void on_submitButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::SkillDetailsWidget *ui;
    SidePanelWidget *view;

    D1Hero *hero;
    LineEditWidget *skillWidgets[64];
    int skills[64];
};
