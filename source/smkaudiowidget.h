#pragma once

#include <QDialog>
#include <QFrame>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>

namespace Ui {
class SmkAudioWidget;
} // namespace Ui

class D1Gfx;
class CelView;

class SmkAudioWidget : public QDialog {
    Q_OBJECT

public:
    explicit SmkAudioWidget(CelView *parent);
    ~SmkAudioWidget();

    void initialize(int frameIndex);
    void setGfx(D1Gfx *gfx);

    void show(); // override;
    void hide(); // override;

private:
    void resetPos();
    void stopMove();
    void frameModified();

private slots:
    void on_trackComboBox_activated(int index);
    void on_channelComboBox_activated(int index);
    void on_bitRateLineEdit_returnPressed();
    void on_bitRateLineEdit_escPressed();

    void on_playPushButtonClicked();

    void on_closePushButtonClicked();
    void on_movePushButtonClicked();
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    Ui::SmkAudioWidget *ui;
    QGraphicsScene audioScene = QGraphicsScene(this);
    QGraphicsView *graphView;
    bool moving = false;
    bool moved = false;
    QPoint lastPos;
    D1Gfx *gfx;
    int currentFrameIndex;
    int currentTrack = -1;
    int currentChannel = -1;
};
