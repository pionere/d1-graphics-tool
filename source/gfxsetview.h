#pragma once

#include <vector>

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QLabel>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>

#include "celview.h"
#include "d1gfxset.h"
#include "resizedialog.h"
#include "upscaledialog.h"

#define ZOOM_LIMIT 10
#define CEL_SCENE_SPACING 8
#define CEL_SCENE_MARGIN 0

namespace Ui {
class GfxsetView;
} // namespace Ui

enum class IMAGE_FILE_MODE;

class GfxsetView : public QWidget {
    Q_OBJECT

public:
    explicit GfxsetView(QWidget *parent);
    ~GfxsetView();

    void initialize(D1Pal *pal, D1Gfxset *gfxset, bool bottomPanelHidden);
    void setPal(D1Pal *pal);

    CelScene *getCelScene() const;
    int getCurrentFrameIndex() const;

    void framePixelClicked(const QPoint &pos, bool first);
    void framePixelHovered(const QPoint &pos);
    void insertImageFiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);
    void addToCurrentFrame(const QString &imagefilePath);
    void replaceCurrentFrame(const QString &imagefilePath);
    void removeCurrentFrame();

    QImage copyCurrent() const;
    void pasteCurrent(const QImage &image);

    void resize(const ResizeParam &params);
    void upscale(const UpscaleParam &params);

    void update();
    void displayFrame();
    void toggleBottomPanel();
    void changeColor(const std::vector<std::pair<D1GfxPixel, D1GfxPixel>> &replacements, bool all);

private:
    void updateLabel();
    void insertFrame(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void setFrameIndex(int frameIndex);
    void updateGroupIndex();
    void setGroupIndex(int groupIndex);

signals:
    void frameRefreshed();
    void palModified();

private slots:
    void on_framesGroupCheckBox_clicked();
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
    void on_playStopButton_clicked();

    void timerEvent(QTimerEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void ShowContextMenu(const QPoint &pos);

private:
    Ui::GfxsetView *ui;
    CelScene celScene = CelScene(this);

    D1Pal *pal;
    D1Gfx *gfx;
    D1Gfxset *gfxset;
    int currentGroupIndex = 0;
    int currentFrameIndex = 0;
    int origFrameIndex = 0;
    quint16 currentPlayDelay = 50;
    int playTimer = 0;
};
