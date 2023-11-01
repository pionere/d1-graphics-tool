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

#include "cppcolumnrenamedialog.h"
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
	void setColumnName(int column, const QString &text);
	void renameColumn(int column);
    void insertColumn(int column);
    void delColumn(int column);
    void hideColumn(int column);
    void insertRow(int row);
    void delRow(int row);
    void hideRow(int row);

public slots:
    void on_toggleInfoButton_clicked();

    void on_actionAddColumn_triggered();
    void on_actionInsertColumn_triggered();
    void on_actionDelColumn_triggered();
    void on_actionHideColumn_triggered();
    void on_actionDelColumns_triggered();
    void on_actionHideColumns_triggered();
    void on_actionAddRow_triggered();
    void on_actionInsertRow_triggered();
    void on_actionDelRow_triggered();
    void on_actionHideRow_triggered();
    void on_actionDelRows_triggered();
    void on_actionHideRows_triggered();

private:
    void updateFields();
    void updateLabel();

private slots:
    void on_tablesComboBox_activated(int index);

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void ShowContextMenu(const QPoint &pos);

private:
    Ui::CppView *ui;
    QUndoStack *undoStack;
	CppColumnRenameDialog renameDialog(this);
	CppDataEditDialog editDialog(this);

    D1Cpp *cpp;
    D1CppTable *currentTable;
	int currentColumnIndex;
	int currentRowIndex;

	QList<int> columnWidths;
    int gridRowCount; // because QGridLayout is lame...
    int gridColumnCount;
};
