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
#include "cppviewentrywidget.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "pushbuttonwidget.h"
#include "ui_cppview.h"

#include "dungeon/all.h"

CppView::CppView(QWidget *parent, QUndoStack *us)
    : QWidget(parent)
    , ui(new Ui::CppView())
    , undoStack(us)
    , gridRowCount(0)
    , gridColumnCount(0)
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
        this->on_tablesComboBox_activated(0);
    }

    // this->updateFields();
}

static int getTableEntryLength(D1CppTable *table, int row, int column, QFontMetrics &fm)
{
	/*int result;
	// assert(row != 0);
    if (column == 0) {
        // leader
        result = fm.horizontalAdvance(table->getLeader(row - 1));
    } else {
		// standard entry
		D1CppRowEntry *entry = table->getRow(row - 1)->getEntry(column - 1);
		result = fm.horizontalAdvance(entry->getContent());
   		if (entry->isComplexFirst()) {
			result += fm.horizontalAdvance("{");
		}
   		if (entry->isComplexLast()) {
			result += fm.horizontalAdvance("}");
		}
    }
	return result;*/
	int result;
	// assert(row != 0);
    if (column == 0) {
        // leader
        result = fm.horizontalAdvance(table->getLeader(row - 1));
    } else {
		// standard entry
		D1CppRowEntry *entry = table->getRow(row - 1)->getEntry(column - 1);
		result = fm.horizontalAdvance(entry->getContent());
	}
	return result;
}

void CppView::setTableContent(int row, int column, const QString &text)
{
    this->cppTable->getRow(row - 1)->getEntry(column - 1)->setContent(text);
    this->cpp->setModified();

    this->displayFrame();
}

void CppView::insertColumn(int index)
{
	this->cppTable->insertColumn(index);

	this->on_tablesComboBox_activated(this->ui->tablesComboBox->currentIndex());
}

void CppView::delColumn(int index)
{
    /*for (int y = 0; y < this->cppTable->getRowCount() + 1; y++) {
	    for (int x = index; x < this->cppTable->getColumnCount(); x++) {
            QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, x);
			QLayoutItem *nextItem = this->ui->tableGrid->itemAtPosition(y, x + 1);
			CppViewEntryWidget *w = (CppViewEntryWidget *)nextItem->widget();
			if (x == index) {
				this->ui->tableGrid->removeItem(item);
				item->widget()->deleteLater();
				delete item;
            }
			this->ui->tableGrid->removeItem(nextItem);
			delete nextItem;

			w->adjustColumnNum(-1);
			this->ui->tableGrid->addWidget(w, y, x);
        }
    }*/
	this->cppTable->delColumn(index);

	this->on_tablesComboBox_activated(this->ui->tablesComboBox->currentIndex());
}

void CppView::hideColumn(int index)
{
    for (int y = 0; y < this->cppTable->getRowCount() + 1; y++) {
        QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, index);
        if (item != nullptr) {
            CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();
            w->setVisible(false);
        }
    }
}

void CppView::insertRow(int index)
{
	this->cppTable->insertRow(index);

    this->on_tablesComboBox_activated(this->ui->tablesComboBox->currentIndex());
}

void CppView::delRow(int index)
{
    /*for (int x = 0; x < this->cppTable->getColumnCount() + 1; x++) {
        for (int y = index; y < this->cppTable->getRowCount(); y++) {
            QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, x);
			QLayoutItem *nextItem = this->ui->tableGrid->itemAtPosition(y + 1, x);
			CppViewEntryWidget *w = (CppViewEntryWidget *)nextItem->widget();
			if (y == index) {
				this->ui->tableGrid->removeItem(item);
				delete item;
            }
			this->ui->tableGrid->removeItem(nextItem);
			delete nextItem;

			w->adjustRowNum(-1);
			this->ui->tableGrid->addWidget(w, y, x);
        }
    }*/
	this->cppTable->delRow(index);

	this->on_tablesComboBox_activated(this->ui->tablesComboBox->currentIndex());
}

void CppView::hideRow(int index)
{
    for (int x = 0; x < this->cppTable->getColumnCount() + 1; x++) {
        QLayoutItem *item = this->ui->tableGrid->itemAtPosition(index, x);
        if (item != nullptr) {
            CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();
            w->setVisible(false);
        }
    }
}

void CppView::on_tablesComboBox_activated(int index)
{
    D1CppTable *table = this->cpp->getTable(index);
    this->cppTable = table;

    // eliminate obsolete content
    for (int y = this->gridRowCount - 1 + 1; y > table->getRowCount() - 1 + 1; y--) {
        for (int x = this->ui->tableGrid->columnCount() - 1 + 1; x >= 0; x--) {
            QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, x);
            if (item != nullptr) {
				QWidget *w = item->widget();
                this->ui->tableGrid->removeWidget(w);
				w->deleteLater();
            }
        }
    }
    this->gridRowCount = table->getRowCount() + 1; // std::min(this->gridRowCount, table->getRowCount() + 1);
    for (int y = 0; y < table->getRowCount() + 1; y++) {
        for (int x = this->gridColumnCount - 1 + 1; x > table->getColumnCount() - 1 + 1; x--) {
            QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, x);
            if (item != nullptr) {
				QWidget *w = item->widget();
                this->ui->tableGrid->removeWidget(w);
				w->deleteLater();
            }
        }
    }
    this->gridColumnCount = table->getColumnCount() + 1; // std::min(this->gridColumnCount, table->getColumnCount() + 1);
    // calculate the column-widths
    QList<int> columnWidths;
    QFontMetrics fm = this->fontMetrics();
    int entryHorizontalMargin = CppViewEntryWidget::baseHorizontalMargin();
    for (int x = 0; x < table->getColumnCount() + 1; x++) {
        int maxWidth = 2;
        for (int y = 1; y < table->getRowCount() + 1; y++) {
            /*if (x == 0 && y == 0) {
                continue;
            }*/
            int tw = getTableEntryLength(table, y, x, fm);
            if (tw > maxWidth) {
                maxWidth = tw;
            }
        }
        columnWidths.push_back(maxWidth + entryHorizontalMargin);
    }
    // add new items
    for (int x = 0; x < table->getColumnCount() + 1; x++) {
        for (int y = 0; y < table->getRowCount() + 1; y++) {
            /*if (x == 0 && y == 0) {
                continue;
            }*/
            QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, x);
            CppViewEntryWidget *w = nullptr;
			if (item != nullptr) {
				w = (CppViewEntryWidget *)item->widget();
            }
            if (w == nullptr) {
                w = new CppViewEntryWidget(this);
                this->ui->tableGrid->addWidget(w, y, x);
            }
            w->initialize(table, y, x, columnWidths[x]);
        }
    }

    // this->gridRowCount = table->getRowCount() + 1;
    // this->gridColumnCount = table->getColumnCount() + 1;

    // if (table->getRowCount() != 0) {
    //    this->ui->tableGrid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), table->getRowCount() - 1, table->getColumnCount());
    // }
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

void CppView::on_toggleInfoButton_clicked()
{
    D1CppTable *table = this->cppTable;

    for (int x = 0; x < table->getColumnCount() + 1; x++) {
        for (int y = 0; y < table->getRowCount() + 1; y++) {
            /*if (x == 0 && y == 0) {
                continue;
            }*/
            QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, x);
            CppViewEntryWidget *w;
            if (item != nullptr) {
                w = (CppViewEntryWidget *)item->widget();
                w->on_toggleInfoButton();
            }
        }
    }
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
