#pragma once

#include <QPointer>
#include <QPushButton>
#include <QUndoCommand>
#include <QWidget>

class LevelCelView;
class D1Gfx;
class D1Til;
class D1Min;
class D1Tla;
class D1Tileset;

class EditTileCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditTileCommand(D1Til *til, int tileIndex, int index, int subtileIndex);
    ~EditTileCommand() = default;

    void undo() override;
    void redo() override;

signals:
    void modified();

private:
    QPointer<D1Til> til;
    int tileIndex;
    int index;
    int subtileIndex;
};

class EditTlaCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditTlaCommand(D1Tla *tla, int tileIndex, int value);
    ~EditTlaCommand() = default;

    void undo() override;
    void redo() override;

signals:
    void modified();

private:
    QPointer<D1Tla> tla;
    int tileIndex;
    int value;
};

namespace Ui {
class LevelTabTileWidget;
} // namespace Ui

class LevelTabTileWidget : public QWidget {
    Q_OBJECT

public:
    explicit LevelTabTileWidget(QWidget *parent);
    ~LevelTabTileWidget();

    void initialize(LevelCelView *v, QUndoStack *undoStack);
    void setTileset(D1Tileset *ts);
    void setGfx(D1Gfx *g);
    void updateFields();

    bool selectSubtile(int index);

private slots:
    void on_clearPushButtonClicked();
    void on_deletePushButtonClicked();

    void on_tlaF00_clicked();
    void on_tlaF01_clicked();
    void on_tlaF10_clicked();
    void on_tlaF11_clicked();
    void on_tlaS00_clicked();
    void on_tlaS01_clicked();
    void on_tlaS02_clicked();
    void on_tlaS03_clicked();

    void on_subtilesPrevButton_clicked();
    void on_subtilesComboBox_activated(int index);
    void on_subtilesComboBox_currentTextChanged(const QString &arg1);
    void on_subtilesNextButton_clicked();

private:
    void setSubtileIndex(int tileIndex, int index, int subtileIndex);
    void updateSubtilesSelection(int index);
    void setTlaProperty(quint8 flags);
    void updateTlaProperty();

private:
    Ui::LevelTabTileWidget *ui;
    QPushButton *clearButton;
    QPushButton *deleteButton;
    LevelCelView *levelCelView;
    QUndoStack *undoStack;
    D1Til *til;
    D1Min *min;
    D1Tla *tla;

    bool onUpdate = false;
    int lastTileIndex = -1;
    int lastSubtileEntryIndex = 0;
};
