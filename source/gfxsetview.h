#pragma once

#include <vector>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QList>
#include <QPair>
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

class GfxComponentDialog;

class GfxsetView : public QWidget {
    Q_OBJECT

public:
    explicit GfxsetView(QWidget *parent);
    ~GfxsetView();

    void initialize(D1Pal *pal, D1Gfxset *gfxset, bool bottomPanelHidden);
    void setPal(D1Pal *pal);
    void setGfx(D1Gfx *gfx);
    void selectGfx(int gfxIndex);

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

    void coloredFrames(bool gfxOnly, const std::pair<int, int>& colors) const;
    void activeFrames(bool gfxOnly) const;
    void checkGraphics(bool gfxOnly) const;

    void resize(const ResizeParam &params);
    void upscale(const UpscaleParam &params);

    void displayFrame();
    void toggleBottomPanel();
    void changeColor(const QList<QPair<D1GfxPixel, D1GfxPixel>> &replacements, bool all);
    void zoomInOut(int dir);

private:
    void drawGrid(QImage &celFrame);
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

public slots:
    void palColorsSelected(const std::vector<quint8> &indices);

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
    void on_showOutlineCheckBox_clicked();    
    void on_assetMplEdit_returnPressed();
    void on_assetMplEdit_escPressed();

    void on_celFramesClippedCheckBox_clicked();

    void on_actionToggle_Mode_triggered();
    void on_metaTypeComboBox_activated(int index);
    void on_metaStoredCheckBox_clicked();

    void on_metaFrameWidthEdit_returnPressed();
    void on_metaFrameWidthEdit_escPressed();
    void on_metaFrameHeightEdit_returnPressed();
    void on_metaFrameHeightEdit_escPressed();
    void on_metaFrameDimensionsCheckBox_clicked();

    void on_animOrderEdit_textChanged();
    void on_formatAnimOrderButton_clicked();

    void on_animDelayEdit_returnPressed();
    void on_animDelayEdit_escPressed();
    void on_metaAnimDelayCheckBox_clicked();

    void on_animOffsetXEdit_returnPressed();
    void on_animOffsetXEdit_escPressed();
    void on_animOffsetYEdit_returnPressed();
    void on_animOffsetYEdit_escPressed();

    void on_actionFramesEdit_returnPressed();
    void on_actionFramesEdit_escPressed();
    void on_formatActionFramesButton_clicked();

    void on_zoomOutButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomEdit_returnPressed();
    void on_zoomEdit_escPressed();

    void on_playDelayEdit_returnPressed();
    void on_playDelayEdit_escPressed();
    void on_playStopButton_clicked();
    void on_playReverseCheckBox_clicked();

    void timerEvent(QTimerEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void ShowContextMenu(const QPoint &pos);

private:
    Ui::GfxsetView *ui;
    PushButtonWidget *loadGfxBtn;
    CelScene celScene = CelScene(this);
    GfxComponentDialog *gfxComponentDialog = nullptr;
    unsigned assetMpl = 1;

    D1Pal *pal;
    D1Gfx *gfx;
    D1Gfxset *gfxset;
    QPushButton *buttons[16];
    D1GFX_SET_TYPE currType = D1GFX_SET_TYPE::Unknown;
    int selectedColor = 0;
    int currentGroupIndex = 0;
    int currentFrameIndex = 0;
    QList<int> animOrder;
    int animAdd = 1;
    int animFrameIndex;
    int origFrameIndex = 0;
    unsigned currentPlayDelay = 50000; // microsec
    int playTimer = 0;
};
