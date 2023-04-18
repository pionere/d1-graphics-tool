#pragma once

#include <QWidget>

#include "celview.h"
#include "d1tbl.h"

namespace Ui {
class TblView;
} // namespace Ui

enum class IMAGE_FILE_MODE;

class TblView : public QWidget {
    Q_OBJECT

public:
    explicit TblView(QWidget *parent);
    ~TblView();

    void initialize(D1Pal *pal, D1Tbl *tbl, bool bottomPanelHidden);
    void setPal(D1Pal *pal);

    void framePixelClicked(const QPoint &pos, bool first);
    void framePixelHovered(const QPoint &pos);

    void update();
    void displayFrame();
    void toggleBottomPanel();

private:
    void updateLabel();
    void setRadius(int nextRadius);

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

    void on_zoomOutButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomEdit_returnPressed();
    void on_zoomEdit_escPressed();

    void on_playDelayEdit_returnPressed();
    void on_playDelayEdit_escPressed();
    void on_playButton_clicked();
    void on_stopButton_clicked();

    void timerEvent(QTimerEvent *event) override;

private:
    Ui::TblView *ui;
    CelScene tblScene = CelScene(this);

    D1Pal *pal;
    D1Tbl *tbl;
    int currentLightRadius = 0;
    quint8 currentColor = 0;
    quint16 currentPlayDelay = 50;
    int playTimer = 0;
};
