#pragma once

#include <QPushButton>
#include <QStyle>
#include <QWidget>

namespace Ui {
class LevelTabSubtileWidget;
} // namespace Ui

class LevelCelView;
class D1Gfx;
class D1Min;
class D1Sol;
class D1Spt;
class D1Tmi;

class LevelTabSubtileWidget : public QWidget {
    Q_OBJECT

public:
    explicit LevelTabSubtileWidget(QWidget *parent);
    ~LevelTabSubtileWidget();

    void initialize(LevelCelView *v, D1Gfx *gfx, D1Min *min, D1Sol *sol, D1Spt *spt, D1Tmi *tmi);
    void updateFields();

    void selectFrame(int index);

private slots:
    void on_clearPushButtonClicked();
    void on_deletePushButtonClicked();

    void on_sol0_clicked();
    void on_sol1_clicked();
    void on_sol2_clicked();

    void on_trapNoneRadioButton_clicked();
    void on_trapLeftRadioButton_clicked();
    void on_trapRightRadioButton_clicked();
    void on_specCelLineEdit_returnPressed();
    void on_specCelLineEdit_escPressed();

    void on_tmi0_clicked();
    void on_tmi1_clicked();
    void on_tmi2_clicked();
    void on_tmi3_clicked();
    void on_tmi4_clicked();
    void on_tmi5_clicked();
    void on_tmi6_clicked();

    void on_framesPrevButton_clicked();
    void on_framesComboBox_activated(int index);
    void on_framesComboBox_currentTextChanged(const QString &arg1);
    void on_framesNextButton_clicked();

private:
    void updateFramesSelection(int index);
    void setSolProperty(quint8 flags);
    void updateSolProperty();
    void setTmiProperty(quint8 flags);
    void updateTmiProperty();
    void setTrapProperty(int trap);

    Ui::LevelTabSubtileWidget *ui;
    QPushButton *clearButton;
    QPushButton *deleteButton;
    LevelCelView *levelCelView;
    D1Gfx *gfx;
    D1Min *min;
    D1Sol *sol;
    D1Spt *spt;
    D1Tmi *tmi;

    bool onUpdate = false;
    int lastSubtileIndex = -1;
    int lastFrameEntryIndex = 0;
};
