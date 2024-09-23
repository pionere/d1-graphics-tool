#pragma once

#include <QWidget>
#include <QList>

class D1Hero;

namespace Ui {
class PvPDetailsWidget;
} // namespace Ui

class PvPDetailsWidget;

class PvPDetailsWidget : public QWidget {
    Q_OBJECT

public:
    explicit PvPDetailsWidget(QWidget *parent);
    ~PvPDetailsWidget();

    void initialize(D1Hero *hero);
    void displayFrame();

private:
    void updateFields();

private slots:
    void on_pvpHerosComboBox_activated(int index);

    void on_discardHeroButton_clicked();
    void on_addHeroButton_clicked();

    void on_offHeroSkillsComboBox_activated(int index);
    void on_plrDistSpinBox_valueChanged(int value);
    void on_defHeroSkillsComboBox_activated(int index);

    void on_closeButton_clicked();

private:
    Ui::PvPDetailsWidget *ui;

    D1Hero *hero;
    QList<D1Hero*> heros;
};
