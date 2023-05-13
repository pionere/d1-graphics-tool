#pragma once

#include <QPushButton>
#include <QStyle>
#include <QWidget>

namespace Ui {
class LevelTabFrameWidget;
} // namespace Ui

class LevelCelView;
class D1Gfx;
class D1GfxFrame;
enum class D1CEL_FRAME_TYPE;

class LevelTabFrameWidget : public QWidget {
    Q_OBJECT

public:
    explicit LevelTabFrameWidget(QWidget *parent);
    ~LevelTabFrameWidget();

    void initialize(LevelCelView *v, D1Gfx *g);
    void updateFields();

private slots:
    void on_deletePushButtonClicked();
    void on_frameTypeComboBox_activated(int index);

private:
    void validate();

private:
    Ui::LevelTabFrameWidget *ui;
    QPushButton *deleteButton;
    LevelCelView *levelCelView;
    D1Gfx *gfx;
};
