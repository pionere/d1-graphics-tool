#pragma once

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QLabel>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>

#include "d1gfx.h"
#include "pushbuttonwidget.h"

#define ZOOM_LIMIT 10
#define CEL_SCENE_SPACING 8
#define CEL_SCENE_MARGIN 0

namespace Ui {
class CelView;
} // namespace Ui

enum class IMAGE_FILE_MODE;

typedef enum _mouse_click_flags {
    FIRST_CLICK  = 1 << 0,
    DOUBLE_CLICK = 1 << 1,
    SHIFT_CLICK  = 1 << 2,
} _mouse_click_flags;

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
    void mouseEvent(QGraphicsSceneMouseEvent *event, int flags);
    void mouseHoverEvent(QGraphicsSceneMouseEvent *event);

private slots:
    void keyPressEvent(QKeyEvent *keyEvent) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

signals:
    // void framePixelClicked(const QPoint &pos, int flags);
    // void framePixelHovered(const QPoint &pos);

private:
    quint8 currentZoomNumerator = 1;
    quint8 currentZoomDenominator = 1;
    bool panning = false;
    bool leftMousePressed = false;
    QPoint lastPos;
};

class CelView : public QWidget {
    Q_OBJECT

public:
    explicit CelView(QWidget *parent);
    ~CelView();

    void initialize(D1Pal *pal, D1Gfx *gfx, bool bottomPanelHidden);
    void setPal(D1Pal *pal);
    void setGfx(D1Gfx *gfx);

    CelScene *getCelScene() const;
    int getCurrentFrameIndex() const;

    void framePixelClicked(const QPoint &pos, int flags);
    void framePixelHovered(const QPoint &pos) const;
    void createFrame(bool append);
    void insertImageFiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);
    void addToCurrentFrame(const QString &imagefilePath);
    void duplicateCurrentFrame(bool wholeGroup);
    void replaceCurrentFrame(const QString &imagefilePath);
    void removeCurrentFrame(bool wholeGroup);

    QString copyCurrentPixels(bool values) const;
    void pasteCurrentPixels(const QString &pixels);
    QImage copyCurrentImage() const;
    void pasteCurrentImage(const QImage &image);

    void displayFrame();
    void toggleBottomPanel();

    static void setLabelContent(QLabel *label, const QString &filePath, bool modified);

private:
    void updateFields();
    void updateLabel();
    void insertFrame(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void setFrameIndex(int frameIndex);
    void updateGroupIndex();
    void setGroupIndex(int groupIndex);
    bool framePos(QPoint &pos) const;

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
    Ui::CelView *ui;
    CelScene celScene = CelScene(this);

    D1Pal *pal;
    D1Gfx *gfx;
    int currentGroupIndex = 0;
    int currentFrameIndex = 0;
    int origFrameIndex = 0;
    QPointer<D1Pal> origPal;
    unsigned currentPlayDelay = 50000; // microsec
    int playTimer = 0;
};
