#pragma once

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QUndoStack>
#include <QWidget>

#include "cppcolumnchangedialog.h"
#include "cppdataeditdialog.h"
#include "d1cpp.h"
#include "lineeditwidget.h"

namespace Ui {
class CppView;
} // namespace Ui

class CppView : public QWidget {
    Q_OBJECT

public:
    explicit CppView(QWidget *parent, QUndoStack *undoStack);
    ~CppView();

    void initialize(D1Cpp *cpp);

    void displayFrame();

    D1CppTable *getCurrentTable() const;
    void setCurrent(int row, int column);
    void setTableContent(int row, int column, const QString &text);
    void setColumnNameType(int column, const QString &text, D1CPP_ENTRY_TYPE type);
    void changeColumn(int column);
    void trimColumn(int column);
    void insertColumn(int column);
    void moveColumnLeft(int column);
    void moveColumnRight(int column);
    void insertRow(int row);
    void moveRowUp(int row);
    void moveRowDown(int row);
    void delColumns(int fromIndex, int toIndex);
    void hideColumns(int fromIndex, int toIndex);
    void showColumns(int fromIndex, int toIndex);
    void delRows(int fromIndex, int toIndex);
    void hideRows(int fromIndex, int toIndex);
    void showRows(int fromIndex, int toIndex);

public slots:
    void on_toggleInfoButton_clicked();

    void on_actionAddColumn_triggered();
    void on_actionInsertColumn_triggered();
    void on_actionDelColumn_triggered();
    void on_actionHideColumn_triggered();
    void on_actionMoveLeftColumn_triggered();
    void on_actionMoveRightColumn_triggered();
    void on_actionDelColumns_triggered();
    void on_actionHideColumns_triggered();
    void on_actionShowColumns_triggered();
    void on_actionAddRow_triggered();
    void on_actionInsertRow_triggered();
    void on_actionDelRow_triggered();
    void on_actionHideRow_triggered();
    void on_actionMoveUpRow_triggered();
    void on_actionMoveDownRow_triggered();
    void on_actionDelRows_triggered();
    void on_actionHideRows_triggered();
    void on_actionShowRows_triggered();

private:
    void updateFields();
    void updateLabel();

    void delRow(int row);
    void showRow(int row);
    void delColumn(int column);
    void showColumn(int column);

private slots:
    void on_tablesComboBox_activated(int index);

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void ShowContextMenu(const QPoint &pos);

private:
    Ui::CppView *ui;
    QUndoStack *undoStack;
    CppColumnChangeDialog changeDialog = CppColumnChangeDialog(this);
    CppDataEditDialog editDialog = CppDataEditDialog(this);

    D1Cpp *cpp;
    D1CppTable *currentTable;
    int currentColumnIndex;
    int currentRowIndex;

    QList<int> columnWidths;
    int gridRowCount; // because QGridLayout is lame...
    int gridColumnCount;
    bool infoVisible;
};
