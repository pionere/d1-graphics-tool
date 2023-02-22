#pragma once

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPoint>
#include <QStringList>
#include <QTimer>
#include <QWidget>

#include "d1gfx.h"
#include "upscaledialog.h"

#define ZOOM_LIMIT 10
#define CEL_SCENE_SPACING 8

namespace Ui {
class CelView;
} // namespace Ui

enum class IMAGE_FILE_MODE;

class CelScene : public QGraphicsScene {
    Q_OBJECT

public:
    CelScene(QWidget *view);

    void zoomOut();
    void zoomIn();
    void setZoom(QString &zoom);
    QString zoomText() const;

private:
    static void parseZoomValue(QString &zoom, quint8 &zoomNumerator, quint8 &zoomDenominator);
    void updateQGraphicsView();

private slots:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

signals:
    void framePixelClicked(const QPoint &pos, unsigned counter);

private:
    QWidget *view;

    quint8 currentZoomNumerator = 1;
    quint8 currentZoomDenominator = 1;
    QPoint lastPos;
    unsigned lastCounter;
};

class CelView : public QWidget {
    Q_OBJECT

public:
    explicit CelView(QWidget *parent);
    ~CelView();

    void initialize(D1Pal *pal, D1Gfx *gfx);
    void setPal(D1Pal *pal);

    CelScene *getCelScene() const;
    int getCurrentFrameIndex() const;

    void framePixelClicked(const QPoint &pos, unsigned counter);
    void insertImageFiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);
    void addToCurrentFrame(const QString &imagefilePath);
    void replaceCurrentFrame(const QString &imagefilePath);
    void removeCurrentFrame();

    QImage copyCurrent() const;
    void pasteCurrent(const QImage &image);

    void upscale(const UpscaleParam &params);

    void update();
    void displayFrame();

private:
    void updateLabel();
    void insertFrame(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void setFrameIndex(int frameIndex);
    void updateGroupIndex();
    void setGroupIndex(int groupIndex);

signals:
    void frameRefreshed();
    void frameClicked(D1GfxFrame *frame, const QPoint &pos, unsigned counter);
    void palModified();

private slots:
    void on_framesGroupCheckBox_stateChanged(int state);
    void on_firstFrameButton_clicked();
    void on_previousFrameButton_clicked();
    void on_nextFrameButton_clicked();
    void on_lastFrameButton_clicked();
    void on_frameIndexEdit_returnPressed();
    void on_frameIndexEdit_escPressed();

    void on_firstGroupButton_clicked();
    void on_previousGroupButton_clicked();
    void on_groupIndexEdit_returnPressed();
    void on_groupIndexEdit_escPressed();
    void on_nextGroupButton_clicked();
    void on_lastGroupButton_clicked();

    void on_zoomOutButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomEdit_returnPressed();
    void on_zoomEdit_escPressed();

    void on_playDelayEdit_returnPressed();
    void on_playDelayEdit_escPressed();
    void on_playButton_clicked();
    void on_stopButton_clicked();
    void playGroup();

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void ShowContextMenu(const QPoint &pos);

private:
    Ui::CelView *ui;
    CelScene celScene = CelScene(this);

    D1Pal *pal;
    D1Gfx *gfx;
    int currentGroupIndex = 0;
    int currentFrameIndex = 0;
    quint16 currentPlayDelay = 50;

    QTimer playTimer;
};
