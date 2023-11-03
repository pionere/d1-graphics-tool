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
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));

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

void CppView::setColumnName(int column, const QString &text)
{
    if (this->currentTable->setHeader(column - 1, text)) {
		this->cpp->setModified();

		QLayoutItem *item = this->ui->tableGrid->itemAtPosition(0, column);
		CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();
		w->initialize(this->currentTable, 0, column, this->columnWidths[column]);
		this->displayFrame();
    }
}

void CppView::renameColumn(int index)
{
	this->renameDialog.initialize(index);
	this->renameDialog.show();
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

	this->hide();

    for (int y = 0; y < table->getRowCount() + 1; y++) {
	    for (int x = table->getColumnCount(); x > index; x--) {
			QLayoutItem *prevItem = this->ui->tableGrid->itemAtPosition(y, x - 1);
			CppViewEntryWidget *w = (CppViewEntryWidget *)prevItem->widget();

			// this->ui->tableGrid->removeWidget(w);
			w->adjustColumnNum(1);
			this->ui->tableGrid->addWidget(w, y, x, Qt::AlignTop);
        }

        CppViewEntryWidget *w = new CppViewEntryWidget(this);
        this->ui->tableGrid->addWidget(w, y, index, Qt::AlignTop);
        w->initialize(table, y, index, this->columnWidths[index]);
    }

	this->show();

	// this->updateFields();
	dMainWindow().updateWindow();
}

void CppView::moveColumnLeft(int index)
{
	D1CppTable *table = this->currentTable;

	table->moveColumnLeft(index - 1, (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) != 0);
	this->cpp->setModified();

    // this->on_tablesComboBox_activated(this->ui->tablesComboBox->currentIndex());
	// this->displayFrame();
	if (this->currentColumnIndex == index) {
		this->currentColumnIndex--;
    }
	this->hide();
    for (int y = 0; y < table->getRowCount() + 1; y++) {
		QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, index);
		CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();

		QLayoutItem *prevItem = this->ui->tableGrid->itemAtPosition(y, index - 1);
		CppViewEntryWidget *pw = (CppViewEntryWidget *)prevItem->widget();

		w->adjustColumnNum(-1);
		pw->adjustColumnNum(+1);

		this->ui->tableGrid->addWidget(w, y, index - 1, Qt::AlignTop);
		this->ui->tableGrid->addWidget(pw, y, index, Qt::AlignTop);
    }
	this->show();
	// this->updateFields();
	dMainWindow().updateWindow();
}

void CppView::moveColumnRight(int index)
{
	D1CppTable *table = this->currentTable;

	table->moveColumnRight(index - 1, (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) != 0);
	this->cpp->setModified();

    // this->on_tablesComboBox_activated(this->ui->tablesComboBox->currentIndex());
	// this->displayFrame();
	if (this->currentColumnIndex == index) {
		this->currentColumnIndex++;
    }
	this->hide();
    for (int y = 0; y < table->getRowCount() + 1; y++) {
		QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, index);
		CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();

		QLayoutItem *nextItem = this->ui->tableGrid->itemAtPosition(y, index + 1);
		CppViewEntryWidget *nw = (CppViewEntryWidget *)nextItem->widget();

		w->adjustColumnNum(+1);
		nw->adjustColumnNum(-1);

		this->ui->tableGrid->addWidget(w, y, index + 1, Qt::AlignTop);
		this->ui->tableGrid->addWidget(nw, y, index, Qt::AlignTop);
    }
	this->show();
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
	if (this->currentColumnIndex > index || this->currentColumnIndex > this->gridColumnCount) {
		this->currentColumnIndex--;
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
			this->ui->tableGrid->addWidget(w, y, x, Qt::AlignTop);
        }
    }
	// this->updateFields();
	dMainWindow().updateWindow();
}

void CppView::showColumn(int index)
{
    for (int y = 0; y < this->currentTable->getRowCount() + 1; y++) {
        QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, index);
        // if (item != nullptr) {
            CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();
            w->setVisible(true);
        // }
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
	this->hide();
    for (int x = 0; x < table->getColumnCount() + 1; x++) {
        for (int y = table->getRowCount(); y > index; y--) {
			QLayoutItem *prevItem = this->ui->tableGrid->itemAtPosition(y - 1, x);
			CppViewEntryWidget *w = (CppViewEntryWidget *)prevItem->widget();

			// this->ui->tableGrid->removeWidget(w);
			w->adjustRowNum(1);
			this->ui->tableGrid->addWidget(w, y, x, Qt::AlignTop);
        }

        CppViewEntryWidget *w = new CppViewEntryWidget(this);
        this->ui->tableGrid->addWidget(w, index, x, Qt::AlignTop);
        w->initialize(table, index, x, this->columnWidths[x]);
    }
	this->show();
	// this->updateFields();
	dMainWindow().updateWindow();
}

void CppView::moveRowUp(int index)
{
	D1CppTable *table = this->currentTable;

	table->moveRowUp(index - 1, (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) != 0);
	this->cpp->setModified();

    // this->on_tablesComboBox_activated(this->ui->tablesComboBox->currentIndex());
	// this->displayFrame();
	if (this->currentRowIndex == index) {
		this->currentRowIndex--;
    }
	this->hide();
    for (int x = 0; x < table->getColumnCount() + 1; x++) {
		QLayoutItem *item = this->ui->tableGrid->itemAtPosition(index, x);
		CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();

		QLayoutItem *prevItem = this->ui->tableGrid->itemAtPosition(index - 1, x);
		CppViewEntryWidget *pw = (CppViewEntryWidget *)prevItem->widget();

		w->adjustRowNum(-1);
		pw->adjustRowNum(+1);

		this->ui->tableGrid->addWidget(w, index - 1, x, Qt::AlignTop);
		this->ui->tableGrid->addWidget(pw, index, x, Qt::AlignTop);
    }
	this->show();
	// this->updateFields();
	dMainWindow().updateWindow();
}

void CppView::moveRowDown(int index)
{
	D1CppTable *table = this->currentTable;

	table->moveRowDown(index - 1, (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) != 0);
	this->cpp->setModified();

    // this->on_tablesComboBox_activated(this->ui->tablesComboBox->currentIndex());
	// this->displayFrame();
	if (this->currentRowIndex == index) {
		this->currentRowIndex++;
    }
	this->hide();
    for (int x = 0; x < table->getColumnCount() + 1; x++) {
		QLayoutItem *item = this->ui->tableGrid->itemAtPosition(index, x);
		CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();

		QLayoutItem *nextItem = this->ui->tableGrid->itemAtPosition(index + 1, x);
		CppViewEntryWidget *nw = (CppViewEntryWidget *)nextItem->widget();

		w->adjustRowNum(+1);
		nw->adjustRowNum(-1);

		this->ui->tableGrid->addWidget(w, index + 1, x, Qt::AlignTop);
		this->ui->tableGrid->addWidget(nw, index, x, Qt::AlignTop);
    }
	this->show();
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
	if (this->currentRowIndex > index || this->currentRowIndex > this->gridRowCount) {
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
			this->ui->tableGrid->addWidget(w, y, x, Qt::AlignTop);
        }
    }
	// this->updateFields();
	dMainWindow().updateWindow();

}

void CppView::showRow(int index)
{
    for (int x = 0; x < this->currentTable->getColumnCount() + 1; x++) {
        QLayoutItem *item = this->ui->tableGrid->itemAtPosition(index, x);
        if (item != nullptr) {
            CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();
            w->setVisible(true);
        }
    }
}

void CppView::delColumns(int fromIndex, int toIndex)
{
	this->hide();

	for (int i = fromIndex; i <= toIndex; i++) {
		this->delColumn(fromIndex);
    }

	this->show();
}

void CppView::hideColumns(int fromIndex, int toIndex)
{
	this->hide();

	// hide the given columns
	for (int i = fromIndex; i <= toIndex; i++) {
		for (int y = 0; y < this->currentTable->getRowCount() + 1; y++) {
			QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, i);
			// if (item != nullptr) {
				CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();
				w->setVisible(false);
			// }
		}
    }

	this->show();

	int index = this->currentColumnIndex;
	if (index >= fromIndex && index <= toIndex) {
		// find the next visible column
		while (true) {
			index++;

			QLayoutItem *item = this->ui->tableGrid->itemAtPosition(0, index);
			if (item == nullptr) {
				index = this->currentColumnIndex;
				break;
			}
			CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();
			if (w == nullptr) {
				index = this->currentColumnIndex;
				break;
            }
			if (w->isVisible()) {
				break;
            }
        }
		// find the previous visible column
		if (this->currentColumnIndex == index) {
			while (true) {
				index--;
				if (index <= 0) {
					index = -1;
					break;
				}
				QLayoutItem *item = this->ui->tableGrid->itemAtPosition(0, index);
				CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();
				if (w->isVisible()) {
					break;
				}
            }
        }
		this->currentColumnIndex = index;
    }
}

void CppView::showColumns(int fromIndex, int toIndex)
{
	this->hide();

	for (int i = fromIndex; i <= toIndex; i++) {
		this->showColumn(i);
    }

	this->show();
}

void CppView::delRows(int fromIndex, int toIndex)
{
	this->hide();

	for (int i = fromIndex; i <= toIndex; i++) {
		this->delRow(fromIndex);
    }

	this->show();
}

void CppView::hideRows(int fromIndex, int toIndex)
{
	this->hide();

	// hide the given rows
	for (int i = fromIndex; i <= toIndex; i++) {
		for (int x = 0; x < this->currentTable->getColumnCount() + 1; x++) {
			QLayoutItem *item = this->ui->tableGrid->itemAtPosition(i, x);
			if (item != nullptr) {
				CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();
				w->setVisible(false);
			}
		}
    }

	this->show();

	int index = this->currentRowIndex;
	if (index >= fromIndex && index <= toIndex) {
		// find the next visible row
		while (true) {
			index++;

			QLayoutItem *item = this->ui->tableGrid->itemAtPosition(index, 0);
			if (item == nullptr) {
				index = this->currentRowIndex;
				break;
			}
			CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();
			if (w == nullptr) {
				index = this->currentRowIndex;
				break;
            }
			if (w->isVisible()) {
				break;
            }
        }
		// find the previous visible row
		if (this->currentRowIndex == index) {
			while (true) {
				index--;
				if (index <= 0) {
					index = -1;
					break;
				}
				QLayoutItem *item = this->ui->tableGrid->itemAtPosition(index, 0);
				CppViewEntryWidget *w = (CppViewEntryWidget *)item->widget();
				if (w->isVisible()) {
					break;
				}
            }
        }
		this->currentRowIndex = index;
    }
}

void CppView::showRows(int fromIndex, int toIndex)
{
	this->hide();

	for (int i = fromIndex; i <= toIndex; i++) {
		this->showRow(i);
    }

	this->show();
}

void CppView::on_tablesComboBox_activated(int index)
{
	this->hide();

    D1CppTable *table = this->cpp->getTable(index);
    this->currentTable = table;
	this->currentColumnIndex = -1;
	this->currentRowIndex = -1;
    // eliminate obsolete content
    for (int y = this->gridRowCount - 1 + 1; y > table->getRowCount() - 1 + 1; y--) {
        for (int x = this->ui->tableGrid->columnCount() - 1 + 1; x >= 0; x--) {
            QLayoutItem *item = this->ui->tableGrid->itemAtPosition(y, x);
            if (item != nullptr) {
				QWidget *w = item->widget();
                // this->ui->tableGrid->removeWidget(w);
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
                // this->ui->tableGrid->removeWidget(w);
				w->deleteLater();
            }
        }
    }
    this->gridColumnCount = table->getColumnCount() + 1; // std::min(this->gridColumnCount, table->getColumnCount() + 1);
    // calculate the column-widths
    this->columnWidths.clear();
    QFontMetrics fm = this->fontMetrics();
    int entryHorizontalMargin = CppViewEntryWidget::baseHorizontalMargin();
	int entryComplexWidth = fm.horizontalAdvance("{");
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
		maxWidth += entryHorizontalMargin;
        this->columnWidths.push_back(maxWidth);
		// try to minimize the required calculations in the QGridLayout
        /*if (x != 0 && table->getRowCount() != 0) {
            D1CppRowEntry *entry = table->getRow(0)->getEntry(x - 1);
            if (entry->isComplexFirst()) {
                maxWidth += entryComplexWidth;
            }
            if (entry->isComplexLast()) {
                maxWidth += entryComplexWidth;
            }
        } else {
            // FIXME: store complexFirst/Last in the header
        }
		this->ui->tableGrid->setColumnMinimumWidth(x, maxWidth);*/
    }
    // add new items
    // for (int x = 0; x < table->getColumnCount() + 1; x++) {
    //    for (int y = 0; y < table->getRowCount() + 1; y++) {
    for (int x = table->getColumnCount(); x >= 0; x--) {
        for (int y = table->getRowCount(); y >= 0; y--) {
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
                this->ui->tableGrid->addWidget(w, y, x, Qt::AlignTop);
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
	this->ui->tableGrid->parentWidget()->adjustSize();
	// this->adjustSize();
	this->show();

    // this->gridRowCount = table->getRowCount() + 1;
    // this->gridColumnCount = table->getColumnCount() + 1;
	dMainWindow().updateWindow();
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
	int index = this->currentColumnIndex;

	if (index > 0) {
		this->insertColumn(index);
    }
}

void CppView::on_actionDelColumn_triggered()
{
	int index = this->currentColumnIndex;

	if (index > 0) {
		this->delColumns(index, index);
    }
}

void CppView::on_actionHideColumn_triggered()
{
	int index = this->currentColumnIndex;

	if (index > 0) {
		this->hideColumns(index, index);
    }
}

void CppView::on_actionMoveLeftColumn_triggered()
{
	int index = this->currentColumnIndex;

	if (index > 1) {
		this->moveColumnLeft(index);
    }
}

void CppView::on_actionMoveRightColumn_triggered()
{
	int index = this->currentColumnIndex;

	if (index > 0 && index < this->gridColumnCount - 1) {
		this->moveColumnRight(index);
    }
}

void CppView::on_actionMoveUpRow_triggered()
{
	int index = this->currentRowIndex;

	if (index > 1) {
		this->moveRowUp(index);
    }
}

void CppView::on_actionMoveDownRow_triggered()
{
	int index = this->currentRowIndex;

	if (index > 0 && index < this->gridRowCount - 1) {
		this->moveRowDown(index);
    }
}

void CppView::on_actionDelColumns_triggered()
{
	editDialog.initialize(CPP_EDIT_MODE::COLUMN_DEL, this->currentColumnIndex);
	editDialog.show();
}

void CppView::on_actionHideColumns_triggered()
{
	editDialog.initialize(CPP_EDIT_MODE::COLUMN_HIDE, this->currentColumnIndex);
	editDialog.show();
}

void CppView::on_actionShowColumns_triggered()
{
	editDialog.initialize(CPP_EDIT_MODE::COLUMN_SHOW, 0);
	editDialog.show();
}

void CppView::on_actionAddRow_triggered()
{
	this->insertRow(this->gridRowCount); // this->currentTable->getRowCount() + 1
}

void CppView::on_actionInsertRow_triggered()
{
	int index = this->currentRowIndex;

	if (index > 0) {
		this->insertRow(index);
    }
}

void CppView::on_actionDelRow_triggered()
{
	int index = this->currentRowIndex;

	if (index > 0) {
		this->delRows(index, index);
    }
}

void CppView::on_actionHideRow_triggered()
{
	int index = this->currentRowIndex;

	if (index > 0) {
		this->hideRows(index, index);
    }
}

void CppView::on_actionDelRows_triggered()
{
	editDialog.initialize(CPP_EDIT_MODE::ROW_DEL, this->currentRowIndex);
	editDialog.show();
}

void CppView::on_actionHideRows_triggered()
{
	editDialog.initialize(CPP_EDIT_MODE::ROW_HIDE, this->currentRowIndex);
	editDialog.show();
}

void CppView::on_actionShowRows_triggered()
{
	editDialog.initialize(CPP_EDIT_MODE::ROW_SHOW, 0);
	editDialog.show();
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

	action = new QAction("<<--");
    action->setToolTip(tr("Move the current column to the left"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionMoveLeftColumn_Table_triggered()));
    action->setEnabled(this->currentTable->getColumnCount() != 0);
    menu->addAction(action);

	action = new QAction("-->>");
    action->setToolTip(tr("Move the current column to the right"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionMoveRightColumn_Table_triggered()));
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

	action = new QAction(tr("Show..."));
    action->setToolTip(tr("Show columns"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionShowColumns_Table_triggered()));
    action->setEnabled(this->currentTable->getColumnCount() != 0);
    menu->addAction(action);

    contextMenu.addMenu(menu);

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

	action = new QAction("^^ ^^");
    action->setToolTip(tr("Move the current row up"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionMoveUpRow_Table_triggered()));
    action->setEnabled(this->currentTable->getRowCount() != 0);
    menu->addAction(action);

	action = new QAction("vv vv");
    action->setToolTip(tr("Move the current row down"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionMoveDownRow_Table_triggered()));
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

	action = new QAction(tr("Show..."));
    action->setToolTip(tr("Show rows"));
    QObject::connect(action, SIGNAL(triggered()), mw, SLOT(on_actionShowRows_Table_triggered()));
    action->setEnabled(this->currentTable->getRowCount() != 0);
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
