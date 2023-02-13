#pragma once

#include <QPushButton>
#include <QStyle>
#include <QWidget>

namespace Ui {
class LevelTabTileWidget;
} // namespace Ui

class LevelCelView;
class D1Til;
class D1Min;
class D1Amp;

class LevelTabTileWidget : public QWidget {
    Q_OBJECT

public:
    explicit LevelTabTileWidget();
    ~LevelTabTileWidget();

    void initialize(LevelCelView *v, D1Til *t, D1Min *m, D1Amp *a);
    void update();

    void selectSubtile(int index);

private slots:
    void on_clearPushButtonClicked();
    void on_deletePushButtonClicked();

    void on_ampTypeComboBox_activated(int index);

    void on_amp0_clicked();
    void on_amp1_clicked();
    void on_amp2_clicked();
    void on_amp3_clicked();
    void on_amp4_clicked();
    void on_amp5_clicked();
    void on_amp6_clicked();
    void on_amp7_clicked();

    void on_subtilesPrevButton_clicked();
    void on_subtilesComboBox_activated(int index);
    void on_subtilesComboBox_currentTextChanged(const QString &arg1);
    void on_subtilesNextButton_clicked();

private:
    void updateSubtilesSelection(int index);
    void setAmpProperty(quint8 flags);
    void updateAmpProperty();

private:
    Ui::LevelTabTileWidget *ui;
    QPushButton *clearButton;
    QPushButton *deleteButton;
    LevelCelView *levelCelView;
    D1Til *til;
    D1Min *min;
    D1Amp *amp;

    bool onUpdate = false;
    int lastTileIndex = -1;
    int lastSubtileEntryIndex = 0;
};
