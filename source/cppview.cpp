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

#define BASE_COLUMN_WIDTH 2

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
    this->currentTable = nullptr;

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

D1CppTable *CppView::getCurrentTable() const
{
	return const_cast<D1CppTable *>(this->currentTable);
}

void CppView::setCurrent(int row, int column)
{
	this->currentRowIndex = row;
	this->currentColumnIndex = column;
}

void CppView::setTableContent(int row, int column, const QString &text)
{
    if (this->currentTable->getRow(row - 1)->getEntry(column - 1)->setContent(text)) {
		this->cpp->setModified();

		this->displayFrame();
    }
}

void CppView::insertColumn(int index)
{
	D1CppTable *table = this->currentTable;

	table->insertColumn(index - 1);
	this->cpp->setModified();

	// this->on_tablesComboBox_activated(this->ui->tablesComboBox->currentIndex());
	// this->displayFrame();
	this->gridColumnCount++;
	int cw = BASE_COLUMN_WIDTH + CppViewEntryWidget::baseHorizontalMargin();
	this->columnWidths.insert(this->columnWidths.begin() + index, cw);
	if (this->currentColumnIndex >= index) {
		this->currentColumnIndex++;
    }

    for (int y = 0; y < table->getRowCount() + 1; y++) {
	    for (int x = table->getColumnCount(); x > index; x--) {
			QLayoutItem *prevItem = this->ui->tableGrid->itemAtPosition(y, x - 1);
			CppViewEntryWidget *w = (CppViewEntryWidget *)prevItem->widget();

			// this->ui->tableGrid->removeWidget(w);
			w->adjustColumnNum(1);
			this->ui->tableGrid->addWidget(w, y, x);
        }

        CppViewEntryWidget *w = new CppViewEntryWidget(this);
        this->ui->tableGrid->addWidget(w, y, index);
        w->initialize(table, y, index, this->columnWidths[index]);
    }
	// this->updateFields();
	dMainWindow().updateWindow();
}

void CppView::delColumn(int index)
{
	D1CppTable *table = this->currentTable;

	table->delColumn(index - 1);
	this->cpp->setModified();

    // this->on_tablesComboBox_activated(this->ui->tablesComboBox->currentIndex());
	// this->displayFrame();
	this->gridColumnCount--;
	if (this->gridColumnCount > index) {
		this->gridColumnCount--;
    }
	this->columnWidths.erase(this->columnWidths.begin() + index);
    for (int y = 0; y < table->getRowCount() + 1; y++) {
	    for (int x = index; x < table->getColumnCount() + 1; x++) {
            QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, x);
			QLayoutItem *nextItem = this->ui->tableGrid->itemAtPosition(y, x + 1);
			CppViewEntryWidget *w = (CppViewEntryWidget *)nextItem->widget();
			if (x == index) {
				item->widget()->deleteLater();
				// this->ui->tableGrid->removeItem(item);
				// delete item;
            }
			// this->ui->tableGrid->removeItem(nextItem);
			// delete nextItem;
			this->ui->tableGrid->removeWidget(w);

			w->adjustColumnNum(-1);
			this->ui->tableGrid->addWidget(w, y, x);
        }
    }
	// this->updateFields();
	dMainWindow().updateWindow();
}

void CppView::hideColumn(int index)
{
    for (int y = 0; y < this->currentTable->getRowCount() + 1; y++) {
        QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, index);
        if (item != nullptr) {
            CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();
            w->setVisible(false);
        }
    }
}

void CppView::insertRow(int index)
{
	D1CppTable *table = this->currentTable;

	table->insertRow(index - 1);
	this->cpp->setModified();

    // this->on_tablesComboBox_activated(this->ui->tablesComboBox->currentIndex());
	// this->displayFrame();
	this->gridRowCount++;
	if (this->currentRowIndex >= index) {
		this->currentRowIndex++;
    }
    for (int x = 0; x < table->getColumnCount() + 1; x++) {
        for (int y = table->getRowCount(); y > index; y--) {
			QLayoutItem *prevItem = this->ui->tableGrid->itemAtPosition(y - 1, x);
			CppViewEntryWidget *w = (CppViewEntryWidget *)prevItem->widget();

			// this->ui->tableGrid->removeWidget(w);
			w->adjustRowNum(1);
			this->ui->tableGrid->addWidget(w, y, x);
        }

        CppViewEntryWidget *w = new CppViewEntryWidget(this);
        this->ui->tableGrid->addWidget(w, index, x);
        w->initialize(table, index, x, this->columnWidths[x]);
    }
	// this->updateFields();
	dMainWindow().updateWindow();
}

void CppView::delRow(int index)
{
	D1CppTable *table = this->currentTable;

	table->delRow(index - 1);
	this->cpp->setModified();

	// this->on_tablesComboBox_activated(this->ui->tablesComboBox->currentIndex());
	// this->displayFrame();
	this->gridRowCount--;
	if (this->currentRowIndex > index) {
		this->currentRowIndex--;
    }
    for (int x = 0; x < table->getColumnCount() + 1; x++) {
        for (int y = index; y < table->getRowCount() + 1; y++) {
            QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, x);
			QLayoutItem *nextItem = this->ui->tableGrid->itemAtPosition(y + 1, x);
			CppViewEntryWidget *w = (CppViewEntryWidget *)nextItem->widget();
			if (y == index) {
				item->widget()->deleteLater();
				// this->ui->tableGrid->removeItem(item);
				// delete item;
            }
			// this->ui->tableGrid->removeItem(nextItem);
			// delete nextItem;
			this->ui->tableGrid->removeWidget(w);

			w->adjustRowNum(-1);
			this->ui->tableGrid->addWidget(w, y, x);
        }
    }
	// this->updateFields();
	dMainWindow().updateWindow();

}

void CppView::hideRow(int index)
{
    for (int x = 0; x < this->currentTable->getColumnCount() + 1; x++) {
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
    this->currentTable = table;
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
    this->columnWidths.clear();
    QFontMetrics fm = this->fontMetrics();
    int entryHorizontalMargin = CppViewEntryWidget::baseHorizontalMargin();
    for (int x = 0; x < table->getColumnCount() + 1; x++) {
        int maxWidth = BASE_COLUMN_WIDTH;
        for (int y = 1; y < table->getRowCount() + 1; y++) {
            /*if (x == 0 && y == 0) {
                continue;
            }*/
            int tw = getTableEntryLength(table, y, x, fm);
            if (tw > maxWidth) {
                maxWidth = tw;
            }
        }
        this->columnWidths.push_back(maxWidth + entryHorizontalMargin);
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
            } else {
				// restore visiblity
				w->setVisible(true);
            }
            w->initialize(table, y, x, this->columnWidths[x]);
			// focus on the first entry
			if (x == 1 && y == 1) {
				w->setFocus();
            }
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
    D1CppTable *table = this->currentTable;

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

void CppView::on_actionAddColumn_triggered()
{
	this->insertColumn(this->gridColumnCount); // this->currentTable->getColumnCount() + 1
}

void CppView::on_actionInsertColumn_triggered()
{
	if (this->currentColumnIndex > 0) {
		this->insertColumn(this->currentColumnIndex);
    }
}

void CppView::on_actionDelColumn_triggered()
{
	if (this->currentColumnIndex > 0) {
		this->delColumn(this->currentColumnIndex);
    }
}

void CppView::on_actionHideColumn_triggered()
{
	if (this->currentColumnIndex > 0) {
		this->hideColumn(this->currentColumnIndex);
    }
}

void CppView::on_actionAddRow_triggered()
{
	this->insertRow(this->gridRowCount); // this->currentTable->getRowCount() + 1
}

void CppView::on_actionInsertRow_triggered()
{
	if (this->currentRowIndex > 0) {
		this->insertRow(this->currentRowIndex);
    }
}

void CppView::on_actionDelRow_triggered()
{
	if (this->currentRowIndex > 0) {
		this->delRow(this->currentRowIndex);
    }
}

void CppView::on_actionHideRow_triggered()
{
	if (this->currentRowIndex > 0) {
		this->hideRow(this->currentRowIndex);
    }
}

void CppView::ShowContextMenu(const QPoint &pos)
{
    if (this->currentTable == nullptr) {
        return;
    }

    MainWindow *mw = &dMainWindow();
	QAction *action;
	QMenu *menu;
    QMenu contextMenu(this);

    menu = new QMenu(tr("Rows"), this);
    menu->setToolTipsVisible(true);

	action = new QAction(tr("Add"));
    action->setToolTip(tr("Add row to the end of the table"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionAddRow_Table_triggered()));
    menu->addAction(action);

	action = new QAction(tr("Insert"));
    action->setToolTip(tr("Add new row before the current one"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionInsertRow_Table_triggered()));
    menu->addAction(action);

	action = new QAction(tr("Delete"));
    action->setToolTip(tr("Delete the current row"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionDelRow_Table_triggered()));
    action->setEnabled(this->currentTable->getRowCount() != 0);
    menu->addAction(action);

	action = new QAction(tr("Hide"));
    action->setToolTip(tr("Hide the current row"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionHideRow_Table_triggered()));
    action->setEnabled(this->currentTable->getRowCount() != 0);
    menu->addAction(action);

	menu->addSeparator();

	action = new QAction(tr("Delete..."));
    action->setToolTip(tr("Delete rows"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionDelRows_Table_triggered()));
    action->setEnabled(this->currentTable->getRowCount() != 0);
    menu->addAction(action);

	action = new QAction(tr("Hide..."));
    action->setToolTip(tr("Hide rows"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionHideRows_Table_triggered()));
    action->setEnabled(this->currentTable->getRowCount() != 0);
    menu->addAction(action);

    contextMenu.addMenu(menu);

    menu = new QMenu(tr("Columns"), this);
    menu->setToolTipsVisible(true);

	action = new QAction(tr("Add"));
    action->setToolTip(tr("Add column to the end of the table"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionAddColumn_Table_triggered()));
    menu->addAction(action);

	action = new QAction(tr("Insert"));
    action->setToolTip(tr("Add new column before the current one"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionInsertColumn_Table_triggered()));
    menu->addAction(action);

	action = new QAction(tr("Delete"));
    action->setToolTip(tr("Delete columns"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionDelColumn_Table_triggered()));
    action->setEnabled(this->currentTable->getColumnCount() != 0);
    menu->addAction(action);

	action = new QAction(tr("Hide"));
    action->setToolTip(tr("Hide columns"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionHideColumn_Table_triggered()));
    action->setEnabled(this->currentTable->getColumnCount() != 0);
    menu->addAction(action);

	menu->addSeparator();

	action = new QAction(tr("Delete..."));
    action->setToolTip(tr("Delete columns"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionDelColumns_Table_triggered()));
    action->setEnabled(this->currentTable->getColumnCount() != 0);
    menu->addAction(action);

	action = new QAction(tr("Hide..."));
    action->setToolTip(tr("Hide columns"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionHideColumns_Table_triggered()));
    action->setEnabled(this->currentTable->getColumnCount() != 0);
    menu->addAction(action);

    contextMenu.addMenu(menu);

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
