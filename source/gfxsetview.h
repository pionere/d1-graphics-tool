#pragma once

#include <vector>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QPoint>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>

#include "celview.h"
#include "d1gfxset.h"
#include "pushbuttonwidget.h"

#define SET_SCENE_MARGIN 0

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
    void setGfx(D1Gfx *gfx);

    CelScene *getCelScene() const;
    int getCurrentFrameIndex() const;

    void framePixelClicked(const QPoint &pos, int flags);
    void framePixelHovered(const QPoint &pos);
    void createFrame(bool append);
    void insertImageFiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append);
    void addToCurrentFrame(const QString &imagefilePath);
    void duplicateCurrentFrame(bool wholeGroup);
    void replaceCurrentFrame(const QString &imagefilePath);
    void removeCurrentFrame(bool wholeGroup);
    void mergeFrames(const MergeFramesParam &params);

    QString copyCurrentPixels() const;
    QImage copyCurrentImage() const;
    void pasteCurrentImage(const QImage &image);

    void resize(const ResizeParam &params);
    void upscale(const UpscaleParam &params);

    void displayFrame();
    void toggleBottomPanel();
    void changeColor(const std::vector<std::pair<D1GfxPixel, D1GfxPixel>> &replacements, bool all);

private:
    void updateFields();
    void updateLabel();
    void insertFrame(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath);
    void setFrameIndex(int frameIndex);
    void updateGroupIndex();
    void setGroupIndex(int groupIndex);
    void selectGfx(int gfxIndex);

signals:
    void frameRefreshed();
    void palModified();

private slots:
    void on_misNWButton_clicked();
    void on_misNNWButton_clicked();
    void on_misNButton_clicked();
    void on_misNNEButton_clicked();
    void on_misNEButton_clicked();
    void on_misWNWButton_clicked();
    void on_misENEButton_clicked();
    void on_misWButton_clicked();
    void on_misEButton_clicked();
    void on_misWSWButton_clicked();
    void on_misESEButton_clicked();
    void on_misSWButton_clicked();
    void on_misSSWButton_clicked();
    void on_misSButton_clicked();
    void on_misSSEButton_clicked();
    void on_misSEButton_clicked();
    void on_monStandButton_clicked();
    void on_monAttackButton_clicked();
    void on_monWalkButton_clicked();
    void on_monSpecButton_clicked();
    void on_monHitButton_clicked();
    void on_monDeathButton_clicked();
    void on_plrStandTownButton_clicked();
    void on_plrStandDunButton_clicked();
    void on_plrWalkTownButton_clicked();
    void on_plrWalkDunButton_clicked();
    void on_plrAttackButton_clicked();
    void on_plrBlockButton_clicked();
    void on_plrFireButton_clicked();
    void on_plrMagicButton_clicked();
    void on_plrLightButton_clicked();
    void on_plrHitButton_clicked();
    void on_plrDeathButton_clicked();

    void on_loadGfxPushButtonClicked();

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
    PushButtonWidget *loadGfxBtn;
    CelScene celScene = CelScene(this);

    D1Pal *pal;
    D1Gfx *gfx;
    D1Gfxset *gfxset;
    QPushButton *buttons[16];
    D1GFX_SET_TYPE currType = D1GFX_SET_TYPE::Unknown;
    int currentGroupIndex = 0;
    int currentFrameIndex = 0;
    int origFrameIndex = 0;
    quint16 currentPlayDelay = 50;
    int playTimer = 0;
};
