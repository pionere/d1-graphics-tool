#pragma once

#include <QWidget>

#include "celview.h"
#include "d1tableset.h"

namespace Ui {
class TblView;
} // namespace Ui

enum class IMAGE_FILE_MODE;

class TblView : public QWidget {
    Q_OBJECT

public:
    explicit TblView(QWidget *parent);
    ~TblView();

    void initialize(D1Pal *pal, D1Tableset *tableset, bool bottomPanelHidden);
    void setPal(D1Pal *pal);

    void framePixelClicked(const QPoint &pos, bool first);
    void framePixelHovered(const QPoint &pos);

    void update();
    void displayFrame();
    void toggleBottomPanel();

private:
    void updateLabel();
    void setRadius(int nextRadius);
    void setOffset(int xoff, int yoff);

signals:
    void frameRefreshed();

public slots:
    void palColorsSelected(const std::vector<quint8> &indices);

private slots:
    void on_levelTypeComboBox_activated(int index);

    void on_radiusDecButton_clicked();
    void on_radiusIncButton_clicked();
    void on_radiusLineEdit_returnPressed();
    void on_radiusLineEdit_escPressed();

    void on_moveNWButton_clicked();
    void on_moveNButton_clicked();
    void on_moveNEButton_clicked();
    void on_moveWButton_clicked();
    void on_moveEButton_clicked();
    void on_moveSWButton_clicked();
    void on_moveSButton_clicked();
    void on_moveSEButton_clicked();
    void on_offsetXYLineEdit_returnPressed();
    void on_offsetXYLineEdit_escPressed();

    void on_zoomOutButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomEdit_returnPressed();
    void on_zoomEdit_escPressed();

    void on_playDelayEdit_returnPressed();
    void on_playDelayEdit_escPressed();
    void on_playStopButton_clicked();

    void timerEvent(QTimerEvent *event) override;

private:
    Ui::TblView *ui;
    CelScene tblScene = CelScene(this);

    D1Pal *pal;
    D1Tableset *tableset;
    int currentLightRadius = 0;
    int currentXOffset = 0;
    int currentYOffset = 0;
    quint8 currentColor = 0;
    quint16 currentPlayDelay = 50;
    int playTimer = 0;
};
