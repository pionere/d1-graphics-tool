#pragma once

#include <QFrame>
#include <QList>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>
#include <QUndoCommand>

#include "d1gfx.h"

typedef struct FramePixel {
    FramePixel(const QPoint &p, D1GfxPixel px);

    QPoint pos;
    D1GfxPixel pixel;
} FramePixel;

class EditFrameCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditFrameCommand(D1GfxFrame *frame, const QPoint &pos, D1GfxPixel newPixel);
    ~EditFrameCommand() = default;

    void undo() override;
    void redo() override;

signals:
    void modified();

private:
    QPointer<D1GfxFrame> frame;
    QList<FramePixel> modPixels;
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
    explicit PaintWidget(QWidget *parent, QUndoStack *undoStack, CelView *celView, LevelCelView *levelCelView);
    ~PaintWidget();

    void setPalette(D1Pal *pal);

    void show(); // override;
    void hide(); // override;

private:
    D1GfxPixel getCurrentColor(unsigned counter) const;
    void stopMove();

public slots:
    void frameClicked(D1GfxFrame *frame, const QPoint &pos, unsigned counter);
    void selectColor(const D1GfxPixel &pixel);
    void palColorsSelected(const QList<quint8> &indices);
    void colorModified();

private slots:
    void on_closePushButtonClicked();
    void on_movePushButtonClicked();
    void mouseMoveEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    Ui::PaintWidget *ui;
    QUndoStack *undoStack;
    CelView *celView;
    LevelCelView *levelCelView;
    bool moving;
    bool moved;
    QPoint lastPos;
    D1Pal *pal;
    QList<quint8> selectedColors;
};
