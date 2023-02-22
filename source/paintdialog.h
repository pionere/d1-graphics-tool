#pragma once

#include <QDialog>
#include <QList>
#include <QMouseEvent>
#include <QPoint>
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
class PaintDialog;
} // namespace Ui

class CelView;
class D1Tileset;
class LevelCelView;

class PaintDialog : public QDialog {
    Q_OBJECT

public:
    explicit PaintDialog(QWidget *parent, CelView *celView, LevelCelView *levelCelView);
    ~PaintDialog();

    void setPalette(D1Pal *pal);

    void show(); // override;
    void hide(); // override;

private:
    D1GfxPixel getCurrentColor(unsigned counter) const;

public slots:
    void frameClicked(D1GfxFrame *frame, const QPoint &pos, unsigned counter);
    void selectColor(const D1GfxPixel &pixel);
    void palColorsSelected(const QList<quint8> &indices);
    void colorModified();

private slots:
    void on_closePushButtonClicked();
    void on_movePushButtonPressed();
    void on_movePushButtonReleased();
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    Ui::PaintDialog *ui;
    QUndoStack *undoStack;
    CelView *celView;
    LevelCelView *levelCelView;
    bool moving;
    QPoint lastPos;
    D1Pal *pal;
    QList<quint8> selectedColors;
};
