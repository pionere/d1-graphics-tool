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

    void setTableContent(int row, int colum, const QString &text);

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

    D1Cpp *cpp;
    D1CppTable *cppTable;

    int gridRowCount; // because QGridLayout is lame...
    int gridColumnCount;
};
