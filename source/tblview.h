#pragma once

#include <vector>

#include <QPointer>
#include <QUndoCommand>
#include <QWidget>

#include "celview.h"
#include "d1tableset.h"

typedef struct TableValue {
    TableValue(int tblX, int tblY, int value);

    int tblX;
    int tblY;
    int value;
} TableValue;

class EditTableCommand : public QObject, public QUndoCommand {
    Q_OBJECT

public:
    explicit EditTableCommand(D1Tbl *table, const std::vector<TableValue> &modValues);
    ~EditTableCommand() = default;

    void undo() override;
    void redo() override;

signals:
    void modified();

private:
    QPointer<D1Tbl> table;
    std::vector<TableValue> modValues;
};

namespace Ui {
class TblView;
} // namespace Ui

enum class IMAGE_FILE_MODE;

class TblView : public QWidget {
    Q_OBJECT

public:
    explicit TblView(QWidget *parent, QUndoStack *undoStack);
    ~TblView();

    void initialize(D1Pal *pal, D1Tableset *tableset, bool bottomPanelHidden);
    void setPal(D1Pal *pal);

    void framePixelClicked(const QPoint &pos, bool first);
    void framePixelHovered(const QPoint &pos);

    void displayFrame();
    void toggleBottomPanel();

private:
    void updateFields();
    void updateLabel();
    void setRadius(int nextRadius);
    void setOffset(int xoff, int yoff);
    void selectTrsPath(QString path);

signals:
    void frameRefreshed();

public slots:
    void palColorsSelected(const std::vector<quint8> &indices);

private slots:
    void on_levelTypeComboBox_activated(int index);
    void on_trsLoadPushButton_clicked();
    void on_trsClearPushButton_clicked();

    void on_radiusDecButton_clicked();
    void on_radiusIncButton_clicked();
    void on_radiusLineEdit_returnPressed();
    void on_radiusLineEdit_escPressed();

    void on_moveNWButton_clicked();
    void on_moveNButton_clicked();
    void on_moveNEButton_clicked();
    void on_moveWButton_clicked();
    void on_moveEButton_clicked();
    void on_moveSWButton_clicked();
    void on_moveSButton_clicked();
    void on_moveSEButton_clicked();
    void on_offsetXYLineEdit_returnPressed();
    void on_offsetXYLineEdit_escPressed();

    void on_zoomOutButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomEdit_returnPressed();
    void on_zoomEdit_escPressed();

    void on_playDelayEdit_returnPressed();
    void on_playDelayEdit_escPressed();
    void on_playStopButton_clicked();

    void timerEvent(QTimerEvent *event) override;

private:
    Ui::TblView *ui;
    QUndoStack *undoStack;
    CelScene tblScene = CelScene(this);

    D1Pal *pal;
    D1Tableset *tableset;
    int currentLightRadius = 0;
    int currentXOffset = 0;
    int currentYOffset = 0;
    quint8 currentColor = 0;
    quint16 currentPlayDelay = 50;
    int playTimer = 0;
    QPoint lastPos;
    QPoint firstPos;
    QString trsPath;
};
