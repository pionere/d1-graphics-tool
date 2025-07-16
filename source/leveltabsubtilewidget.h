#pragma once

#include <QPointer>
#include <QPushButton>
#include <QUndoCommand>
#include <QWidget>

class LevelCelView;
class D1Gfx;
class D1Min;
class D1Sla;
class D1Tileset;

enum class SLA_FIELD_TYPE {
    SOL_PROP,
    LIGHT_PROP,
    TRAP_PROP,
    SPEC_PROP,
    RENDER_PROP,
    MAP_PROP,
};

class EditMinCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditMinCommand(D1Min *min, int subtileIndex, int index, int frameRef);
    ~EditMinCommand() = default;

    void undo() override;
    void redo() override;

signals:
    void modified();

private:
    QPointer<D1Min> min;
    int subtileIndex;
    int index;
    int frameRef;
};

class EditSlaCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditSlaCommand(D1Sla *sla, int subtileIndex, SLA_FIELD_TYPE field, int value);
    ~EditSlaCommand() = default;

    void undo() override;
    void redo() override;

signals:
    void modified();

private:
    QPointer<D1Sla> sla;
    int subtileIndex;
    SLA_FIELD_TYPE field;
    int value;
};

namespace Ui {
class LevelTabSubtileWidget;
} // namespace Ui

class LevelTabSubtileWidget : public QWidget {
    Q_OBJECT

public:
    explicit LevelTabSubtileWidget(QWidget *parent);
    ~LevelTabSubtileWidget();

    void initialize(LevelCelView *v, QUndoStack *undoStack);
    void setTileset(D1Tileset *ts);
    void setGfx(D1Gfx *g);
    void updateFields();

    bool selectFrame(int index);

private slots:
    void on_clearPushButtonClicked();
    void on_deletePushButtonClicked();

    void on_sol0_clicked();
    void on_sol1_clicked();
    void on_sol2_clicked();

    void on_lightComboBox_activated(int index);
    void on_trapNoneRadioButton_clicked();
    void on_trapLeftRadioButton_clicked();
    void on_trapRightRadioButton_clicked();
    void on_specCelComboBox_returnPressed();
    void on_specCelComboBox_activated(int index);

    void on_tmi0_clicked();
    void on_tmi1_clicked();
    void on_tmi2_clicked();
    void on_tmi3_clicked();
    void on_tmi4_clicked();
    void on_tmi5_clicked();
    void on_tmi6_clicked();

    void on_smpTypeComboBox_activated(int index);
    void on_smp4_clicked();
    void on_smp5_clicked();
    void on_smp6_clicked();
    void on_smp7_clicked();

    void on_framesPrevButton_clicked();
    void on_framesComboBox_activated(int index);
    void on_framesComboBox_currentTextChanged(const QString &arg1);
    void on_framesNextButton_clicked();

private:
    void setFrameReference(int subtileIndex, int index, int frameRef);
    void updateFramesSelection(int index);
    void setSolProperty(quint8 flags);
    void updateSolProperty();
    void setLightRadius(quint8 radius);
    void setSpecProperty(int spec);
    void setTrapProperty(int trap);
    void setTmiProperty(quint8 flags);
    void updateTmiProperty();
    void setSmpProperty(quint8 flags);
    void updateSmpProperty();

    Ui::LevelTabSubtileWidget *ui;
    QPushButton *clearButton;
    QPushButton *deleteButton;
    LevelCelView *levelCelView;
    QUndoStack *undoStack;
    D1Gfx *gfx;
    D1Min *min;
    D1Sla *sla;

    bool onUpdate = false;
    int lastSubtileIndex = -1;
    int lastFrameEntryIndex = 0;
};
