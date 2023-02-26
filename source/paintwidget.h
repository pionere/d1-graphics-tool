#pragma once

#include <vector>

#include <QFrame>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>
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

signals:
    void modified();

private:
    QPointer<D1GfxFrame> frame;
    std::vector<FramePixel> modPixels;
};

namespace Ui {
class PaintWidget;
} // namespace Ui

class CelView;
class D1Tileset;
class LevelCelView;

class PaintWidget : public QFrame {
    Q_OBJECT

public:
    explicit PaintWidget(QWidget *parent, QUndoStack *undoStack, D1Gfx *gfx, CelView *celView, LevelCelView *levelCelView);
    ~PaintWidget();

    void setPalette(D1Pal *pal);

    void show(); // override;
    void hide(); // override;

private:
    D1GfxPixel getCurrentColor(unsigned counter) const;
    void stopMove();
    void collectPixels(int baseX, int baseY, int baseDist, std::vector<FramePixel> &pixels);
    void traceClick(const QPoint &startPos, const QPoint &destPos, std::vector<FramePixel> &pixels);

public slots:
    bool frameClicked(D1GfxFrame *frame, const QPoint &pos, unsigned counter);
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
    void on_gradientClearPushButton_clicked();
    void on_tilesetMaskPushButton_clicked();

private:
    Ui::PaintWidget *ui;
    QUndoStack *undoStack;
    D1Gfx *gfx;
    CelView *celView;
    LevelCelView *levelCelView;
    QGraphicsView *graphView;
    bool moving;
    bool moved;
    QPoint lastPos;
    int distance;
    D1Pal *pal;
    std::vector<quint8> selectedColors;
    unsigned brushWidth;
};
