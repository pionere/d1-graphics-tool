#pragma once

#include <vector>

#include <QFrame>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>
#include <QRubberBand>
#include <QUndoCommand>

#include "d1gfx.h"

class EditFrameCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditFrameCommand(D1GfxFrame *frame, const QPoint &pos, D1GfxPixel newPixel);
    explicit EditFrameCommand(D1GfxFrame *frame, const std::vector<FramePixel> &modPixels);
    ~EditFrameCommand() = default;

    void undo() override;
    void redo() override;

private:
    QPointer<D1GfxFrame> frame;
    std::vector<FramePixel> modPixels;
};

namespace Ui {
class PaintWidget;
} // namespace Ui

class CelView;
class LevelCelView;
class GfxsetView;

class PaintWidget : public QFrame {
    Q_OBJECT

public:
    explicit PaintWidget(QWidget *parent, QUndoStack *undoStack, D1Gfx *gfx, CelView *celView, LevelCelView *levelCelView, GfxsetView *gfxsetView);
    ~PaintWidget();

    void setPalette(D1Pal *pal);
    void setGfx(D1Gfx *gfx);

    void show(); // override;
    void hide(); // override;

    QString copyCurrentPixels(bool values) const;
    void pasteCurrentPixels(const QString &pixels);
    QImage copyCurrentImage() const;
    void pasteCurrentImage(const QImage &image);
    void deleteCurrent();
    void toggleMode();
    void adjustBrush(int dir);
    void selectArea(const QRect &area);

private:
    D1GfxPixel getCurrentColor(unsigned counter) const;
    D1GfxFrame *getCurrentFrame() const;
    QRect getSelectArea(const D1GfxFrame *frame)  const;
    void pasteCurrentFrame(const D1GfxFrame &srcFrame);
    void stopMove();
    void collectPixels(const D1GfxFrame *frame, const QPoint &startPos, std::vector<FramePixel> &pixels);
    void collectPixelsSquare(int baseX, int baseY, int baseDist, std::vector<FramePixel> &pixels);
    void collectPixelsRound(int baseX, int baseY, int baseDist, std::vector<FramePixel> &pixels);
    void traceClick(const QPoint &startPos, const QPoint &destPos, std::vector<FramePixel> &pixels, void (PaintWidget::*collectorFunc)(int, int, int, std::vector<FramePixel> &));

public slots:
    bool frameClicked(D1GfxFrame *frame, const QPoint &pos, int flags);
    void selectColor(const D1GfxPixel &pixel);
    void palColorsSelected(const std::vector<quint8> &indices);
    void colorModified();

private slots:
    void on_closePushButtonClicked();
    void on_movePushButtonClicked();
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    void on_brushWidthDecButton_clicked();
    void on_brushWidthIncButton_clicked();
    void on_brushWidthLineEdit_returnPressed();
    void on_brushWidthLineEdit_escPressed();
    void on_brushLengthDecButton_clicked();
    void on_brushLengthIncButton_clicked();
    void on_brushLengthLineEdit_returnPressed();
    void on_brushLengthLineEdit_escPressed();
    void on_gradientClearPushButton_clicked();
    void on_tilesetMaskPushButton_clicked();

private:
    Ui::PaintWidget *ui;
    QUndoStack *undoStack;
    D1Gfx *gfx;
    CelView *celView;
    LevelCelView *levelCelView;
    GfxsetView *gfxsetView;
    QGraphicsView *graphView;
    QRubberBand *rubberBand;
    bool moving;
    bool moved;
    int8_t selectionMoveMode;
    EditFrameCommand *lastMoveCmd;
    QPoint movePos;
    QPoint lastDelta;
    QPoint currPos;
    QPoint lastPos;
    int distance;
    D1Pal *pal;
    std::vector<quint8> selectedColors;
    int prevmode = 0;
    unsigned brushWidth = 1;
    unsigned brushLength = 1;
};
