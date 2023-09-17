#include "cppview.h"

#include <algorithm>

#include <QAction>
#include <QDebug>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>

#include "celview.h"
#include "config.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "pushbuttonwidget.h"
#include "ui_cppview.h"

CppView::CppView(QWidget *parent, QUndoStack *us)
    : QWidget(parent)
    , ui(new Ui::CppView())
    , undoStack(us)
{
    this->ui->setupUi(this);

    // setup context menu
    // this->setContextMenuPolicy(Qt::CustomContextMenu);
    // QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));

    setAcceptDrops(true);
}

CppView::~CppView()
{
    delete ui;
}

void CppView::initialize(D1Cpp *c)
{
    this->cpp = c;
    this->cppTable = nullptr;

    int n = c->getTableCount();
    if (n != 0) {
        for (int i = 0; i < n; i++) {
            D1CppTable *table = c->getTable(i);
            this->ui->tablesComboBox->addItem(table->getName(), i);
        }
        this->ui->tablesComboBox->setCurrentIndex(0);
        this->cppTable = c->getTable(0);
    }

    this->updateFields();
}

void CppView::displayFrame()
{
    this->updateFields();
}

// Displaying CPP file path information
void CppView::updateLabel()
{
    CelView::setLabelContent(this->ui->cppLabel, this->cpp->getFilePath(), this->cpp->isModified());
}

void CppView::updateFields()
{
    this->updateLabel();
}

void CppView::ShowContextMenu(const QPoint &pos)
{
	if (this->cppTable == nullptr) {
		return;
    }

    MainWindow *mw = &dMainWindow();
    QAction actions[8];
    QMenu contextMenu(this);

    QMenu rowMenu(tr("Row"), this);
    rowMenu.setToolTipsVisible(true);

    /*int cursor = 0;
    actions[cursor].setText(tr("Add"));
    actions[cursor].setToolTip(tr("Add row to the end of the table"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAddRow_Table_triggered()));
    rowMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Insert"));
    actions[cursor].setToolTip(tr("Add new row before the current one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionInsertRow_Table_triggered()));
    rowMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Delete"));
    actions[cursor].setToolTip(tr("Delete the current row"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDelRow_Table_triggered()));
    actions[cursor].setEnabled(this->cppTable->getRowCount() != 0);
    rowMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Hide"));
    actions[cursor].setToolTip(tr("Hide the current row"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDelRow_Table_triggered()));
    actions[cursor].setEnabled(this->cppTable->getRowCount() != 0);
    rowMenu.addAction(&actions[cursor]);*/

    contextMenu.addMenu(&rowMenu);

    QMenu columnMenu(tr("Column"), this);
    columnMenu.setToolTipsVisible(true);

    /*cursor++;
    actions[cursor].setText(tr("Add"));
    actions[cursor].setToolTip(tr("Add column to the end of the table"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAddColumn_Table_triggered()));
    columnMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Insert"));
    actions[cursor].setToolTip(tr("Add new column before the current one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionInsertColumn_Table_triggered()));
    columnMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Delete"));
    actions[cursor].setToolTip(tr("Delete the current column"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDelColumn_Table_triggered()));
    actions[cursor].setEnabled(this->cppTable->getColumnCount() != 0);
    columnMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Hide"));
    actions[cursor].setToolTip(tr("Hide the current column"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDelColumn_Table_triggered()));
    actions[cursor].setEnabled(this->cppTable->getColumnCount() != 0);
    columnMenu.addAction(&actions[cursor]);*/

    contextMenu.addMenu(&columnMenu);

    contextMenu.exec(mapToGlobal(pos));
}

void CppView::dragEnterEvent(QDragEnterEvent *event)
{
    this->dragMoveEvent(event);
}

void CppView::dragMoveEvent(QDragMoveEvent *event)
{
    for (const QUrl &url : event->mimeData()->urls()) {
        QString filePath = url.toLocalFile();
        // add PCX support
        if (filePath.toLower().endsWith(".cpp")) {
            event->acceptProposedAction();
            return;
        }
    }
}

void CppView::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    QStringList filePaths;
    for (const QUrl &url : event->mimeData()->urls()) {
        filePaths.append(url.toLocalFile());
    }
    // try to insert as frames
    dMainWindow().openFiles(filePaths);
}
