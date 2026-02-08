#pragma once

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QLabel>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>

#include "d1gfx.h"
#include "mergeframesdialog.h"
#include "pushbuttonwidget.h"
#include "resizedialog.h"
#include "smkaudiowidget.h"
#include "upscaledialog.h"

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

class GfxComponentDialog;

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
    void flipHorizontalCurrentFrame(bool wholeGroup);
    void flipVerticalCurrentFrame(bool wholeGroup);
    void mergeFrames(const MergeFramesParam &params);

    QString copyCurrentPixels(bool values) const;
    void pasteCurrentPixels(const QString &pixels);
    QImage copyCurrentImage() const;
    void pasteCurrentImage(const QImage &image);

    void coloredFrames(const std::pair<int, int>& colors) const;
    void activeFrames() const;
    void checkGraphics() const;

    void resize(const ResizeParam &params);
    void upscale(const UpscaleParam &params);

    void displayFrame();
    void toggleBottomPanel();
    bool toggleMute();

    static void setLabelContent(QLabel *label, const QString &filePath, bool modified);

private:
    void drawGrid(QImage &celFrame);
    void updateFields();
    void updateLabel();
    void insertFrame(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void setFrameIndex(int frameIndex);
    void updateGroupIndex();
    void setGroupIndex(int groupIndex);
    void showAudioInfo();
    bool framePos(QPoint &pos) const;

signals:
    void frameRefreshed();
    void palModified();

private slots:
    void on_newComponentPushButtonClicked();
    void on_editComponentPushButtonClicked();
    void on_reloadComponentPushButtonClicked();
    void on_closeComponentPushButtonClicked();
    void on_componentsComboBox_activated(int index);

    void on_showComponentsCheckBox_clicked();
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

    void on_showGridCheckBox_clicked();
    void on_assetMplEdit_returnPressed();
    void on_assetMplEdit_escPressed();

    void on_celFramesClippedCheckBox_clicked();

    void on_zoomOutButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomEdit_returnPressed();
    void on_zoomEdit_escPressed();

    void on_playDelayEdit_returnPressed();
    void on_playDelayEdit_escPressed();
    void on_playStopButton_clicked();

    void on_actionToggle_Mode_triggered();
    void on_metaTypeComboBox_activated(int index);
    void on_metaStoredCheckBox_clicked();
    void on_formatAnimOrderButton_clicked();
    void on_formatActionFramesButton_clicked();

    void timerEvent(QTimerEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void ShowContextMenu(const QPoint &pos);

private:
    Ui::CelView *ui;
    CelScene celScene = CelScene(this);
    GfxComponentDialog *gfxComponentDialog = nullptr;
    SmkAudioWidget *smkAudioWidget = nullptr;
    PushButtonWidget *audioBtn;
    bool audioMuted;
    unsigned assetMpl = 1;

    D1Pal *pal;
    D1Gfx *gfx;
    int currentGroupIndex = 0;
    int currentFrameIndex = 0;
    int origFrameIndex = 0;
    QPointer<D1Pal> origPal;
    unsigned currentPlayDelay = 50000; // microsec
    int playTimer = 0;
};
