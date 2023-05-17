#pragma once

#include <QPushButton>
#include <QUndoCommand>
#include <QWidget>

class LevelCelView;
class D1Til;
class D1Min;
class D1Amp;

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

class EditAmpCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditAmpCommand(D1Amp *amp, int tileIndex, int value, bool type);
    ~EditAmpCommand() = default;

    void undo() override;
    void redo() override;

signals:
    void modified();

private:
    QPointer<D1Amp> amp;
    int tileIndex;
    int value;
    bool type;
};

namespace Ui {
class LevelTabTileWidget;
} // namespace Ui

class LevelTabTileWidget : public QWidget {
    Q_OBJECT

public:
    explicit LevelTabTileWidget(QWidget *parent);
    ~LevelTabTileWidget();

    void initialize(LevelCelView *v, QUndoStack *undoStack, D1Til *t, D1Min *m, D1Amp *a);
    void updateFields();

    void selectSubtile(int index);

private slots:
    void on_clearPushButtonClicked();
    void on_deletePushButtonClicked();

    void on_ampTypeComboBox_activated(int index);

    void on_amp0_clicked();
    void on_amp1_clicked();
    void on_amp2_clicked();
    void on_amp3_clicked();
    void on_amp4_clicked();
    void on_amp5_clicked();
    void on_amp6_clicked();
    void on_amp7_clicked();

    void on_subtilesPrevButton_clicked();
    void on_subtilesComboBox_activated(int index);
    void on_subtilesComboBox_currentTextChanged(const QString &arg1);
    void on_subtilesNextButton_clicked();

private:
    void setSubtileIndex(int tileIndex, int index, int subtileIndex);
    void updateSubtilesSelection(int index);
    void setAmpProperty(quint8 flags);
    void updateAmpProperty();

private:
    Ui::LevelTabTileWidget *ui;
    QPushButton *clearButton;
    QPushButton *deleteButton;
    LevelCelView *levelCelView;
    QUndoStack *undoStack;
    D1Til *til;
    D1Min *min;
    D1Amp *amp;

    bool onUpdate = false;
    int lastTileIndex = -1;
    int lastSubtileEntryIndex = 0;
};
