#pragma once

#include <QPointer>
#include <QPushButton>
#include <QUndoCommand>
#include <QWidget>

class LevelCelView;
class D1Gfx;
class D1GfxFrame;
class D1Tileset;
enum class D1CEL_FRAME_TYPE;

class EditFrameTypeCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditFrameTypeCommand(D1Gfx *gfx, int frameIndex, D1CEL_FRAME_TYPE type);
    ~EditFrameTypeCommand() = default;

    void undo() override;
    void redo() override;

signals:
    void modified();

private:
    QPointer<D1Gfx> gfx;
    int frameIndex;
    D1CEL_FRAME_TYPE type;
};

namespace Ui {
class LevelTabFrameWidget;
} // namespace Ui

class LevelTabFrameWidget : public QWidget {
    Q_OBJECT

public:
    explicit LevelTabFrameWidget(QWidget *parent);
    ~LevelTabFrameWidget();

    void initialize(LevelCelView *v, QUndoStack *undoStack);
    void setTileset(D1Tileset *ts);
    void setGfx(D1Gfx *g);
    void updateFields();

private slots:
    void on_deletePushButtonClicked();
    void on_frameTypeComboBox_activated(int index);

private:
    void updateTab();
    void validate();

private:
    Ui::LevelTabFrameWidget *ui;
    QPushButton *deleteButton;
    LevelCelView *levelCelView;
    QUndoStack *undoStack;
    D1Gfx *gfx;
};
