#include "cppviewentrywidget.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>

#include "cppview.h"
#include "d1cpp.h"
#include "labelwidget.h"
#include "lineeditwidget.h"
#include "pushbuttonwidget.h"
#include "ui_cppviewentrywidget.h"

#include "dungeon/all.h"

static int entryHorizontalMargin = 0;

CppViewEntryWidget::CppViewEntryWidget(CppView *parent)
    : QWidget(parent)
    , ui(new Ui::CppViewEntryWidget())
    , view(parent)
{
    ui->setupUi(this);
}

CppViewEntryWidget::~CppViewEntryWidget()
{
    delete ui;
}

int CppViewEntryWidget::baseHorizontalMargin()
{
    if (entryHorizontalMargin == 0) {
        int result = 8;
        LineEditWidget w = LineEditWidget(nullptr);
        QMargins qm = w.textMargins();
        result += qm.left() + qm.right();
        qm = w.contentsMargins();
        result += qm.left() + qm.right();
        entryHorizontalMargin = result;
    }
    return entryHorizontalMargin;
}

void CppViewEntryWidget::initialize(D1CppTable *t, int r, int c, int width)
{
    this->table = t;
    this->rowNum = r;
    this->columnNum = c;
LogErrorF("Init Entry r:%d c:%d", r, c);
    // clear the layout
    QLayoutItem *child;
    while ((child = this->ui->entryHorizontalLayout->takeAt(0)) != nullptr) {
        delete child->widget(); // delete the widget
        delete child;           // delete the layout item
    }

    QWidget *w;
    if (r == 0) {
        // header
        if (c == 0) {
            w = PushButtonWidget::addButton(this, QStyle::SP_FileDialogInfoView, tr("Show info"), view, &CppView::on_toggleInfoButton_clicked);
        } else {
            // standard header
            QString text = t->getHeader(c - 1);
            QFontMetrics fm = this->fontMetrics();
            if (fm.horizontalAdvance(text) > width) {
                w = PushButtonWidget::addButton(this, QStyle::SP_DialogHelpButton, text, this, &CppViewEntryWidget::on_headerButton_clicked);
            } else {
                w = new QPushButton(text, this);
                ((QPushButton *)w)->setFixedWidth(width);
				((QPushButton *)w)->setFlat(true);
		        QObject::connect(w, SIGNAL(clicked()), this, SLOT(ShowHeaderContextMenu(const QPoint &)));
            }
        }
    } else if (c == 0) {
        // leader
        w = new QPushButton(this);
        ((QPushButton *)w)->setFixedWidth(width);
		((QPushButton *)w)->setFlat(true);
        QString tooltip = t->getRowText(r);
        /*QString tooltip = t->getRowText(r - 1);
        QString postText = t->getRowText(r);
        if (!postText.isEmpty()) {
            if (!tooltip.isEmpty()) {
                tooltip += "</p><p style='white-space:pre'>";
            }
            tooltip += postText;
        }*/
        QString text = t->getLeader(r - 1);
        if (!tooltip.isEmpty()) {
            tooltip.prepend("<html><head/><body><p style='white-space:pre'>");
            tooltip.append("</p></body></html>");
            w->setToolTip(tooltip);
            text.prepend("<html><head/><body><i>");
            text.append("</i></body></html>");
        }
        ((QPushButton *)w)->setText(text);
        QObject::connect(w, SIGNAL(clicked()), this, SLOT(ShowRowContextMenu(const QPoint &)));
    } else {
        // standard entry
LogErrorF("Init Entry row:%1", t->getRow(r - 1) != nullptr);
LogErrorF("Init Entry entry:%1", t->getRow(r - 1)->getEntry(c - 1) != nullptr);
LogErrorF("Init Entry content:%1", t->getRow(r - 1)->getEntry(c - 1)->getContent());
        w = new LineEditWidget(t->getRow(r - 1)->getEntry(c - 1)->getContent(), this);
        ((LineEditWidget *)w)->setMinimumWidth(width);
LogErrorF("Init Entry leader:%1", t->getLeader(r - 1));
LogErrorF("Init Entry header:%1", t->getHeader(c - 1));
        w->setToolTip(QString("%1/%2").arg(t->getLeader(r - 1)).arg(t->getHeader(c - 1)));
        QObject::connect(w, SIGNAL(returnPressed()), this, SLOT(on_entryLineEdit_returnPressed()));
        // connect esc events of LineEditWidgets
        QObject::connect(w, SIGNAL(cancel_signal()), this, SLOT(on_entryLineEdit_escPressed()));
    }
    this->widget = w;
    this->ui->entryHorizontalLayout->addWidget(w);

    if (r != 0 && c != 0) {
        // standard entry
        QString text = t->getRow(r - 1)->getEntryText(c - 1);
        w = PushButtonWidget::addButton(this, QStyle::SP_MessageBoxInformation, text, this, &CppViewEntryWidget::on_infoButton_clicked);
        w->setVisible(false);
        this->ui->entryHorizontalLayout->addWidget(w);
    }
LogErrorF("Init Entry done");
}

void CppViewEntryWidget::adjustRowNum(int delta)
{
	this->rowNum += delta;
}

void CppViewEntryWidget::adjustColumnNum(int delta)
{
	this->columnNum += delta;
}

void CppViewEntryWidget::on_headerButton_clicked()
{

}

void CppViewEntryWidget::on_infoButton_clicked()
{

}

void CppViewEntryWidget::on_toggleInfoButton()
{
    auto layout = this->ui->entryHorizontalLayout;
    if (this->rowNum == 0) {
        // header
        if (this->columnNum == 0) {
            if (layout->count() == 0) {
                QMessageBox::critical(nullptr, "Error", tr("No item in %1:%2").arg(this->rowNum).arg(this->columnNum));
            }
            PushButtonWidget *w = qobject_cast<PushButtonWidget *>(layout->itemAt(0)->widget());
            if (w != nullptr) {
            QString showTooltip = tr("Show info");
            if (w->toolTip() == showTooltip) {
                showTooltip = tr("Hide info");
            }
            w->setToolTip(showTooltip);
            } else {
                QMessageBox::critical(nullptr, "Error", tr("No button in %1:%2").arg(this->rowNum).arg(this->columnNum));
            }
        }
    } else if (this->columnNum != 0) {
        // standard entry
        if (layout->count() == 2) {
            PushButtonWidget *w = qobject_cast<PushButtonWidget *>(layout->itemAt(1)->widget());
            if (w != nullptr) {
            w->setVisible(!w->isVisible());
            } else {
                QMessageBox::critical(nullptr, "Error", tr("No button in %1:%2").arg(this->rowNum).arg(this->columnNum));
            }
        }
    }
}

void CppViewEntryWidget::ShowHeaderContextMenu(const QPoint &pos)
{
    QAction actions[3];
    QMenu contextMenu(this);

    QMenu columnMenu(tr("Column"), this);
    columnMenu.setToolTipsVisible(true);

    int cursor = 0;
    actions[cursor].setText(tr("Insert"));
    actions[cursor].setToolTip(tr("Add new column before this one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionInsertColumn_triggered()));
    columnMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Delete"));
    actions[cursor].setToolTip(tr("Delete this column"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionDelColumn_triggered()));
    columnMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Hide"));
    actions[cursor].setToolTip(tr("Hide this column"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionHideColumn_triggered()));
    columnMenu.addAction(&actions[cursor]);

    contextMenu.addMenu(&columnMenu);

    contextMenu.exec(mapToGlobal(pos));
}

void CppViewEntryWidget::ShowRowContextMenu(const QPoint &pos)
{
    QAction actions[3];
    QMenu contextMenu(this);

    QMenu rowMenu(tr("Row"), this);
    rowMenu.setToolTipsVisible(true);

    int cursor = 0;
    actions[cursor].setText(tr("Insert"));
    actions[cursor].setToolTip(tr("Add new row before this one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionInsertRow_triggered()));
    rowMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Delete"));
    actions[cursor].setToolTip(tr("Delete this row"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionDelRow_triggered()));
    rowMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Hide"));
    actions[cursor].setToolTip(tr("Hide this row"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionHideRow_triggered()));
    rowMenu.addAction(&actions[cursor]);

    contextMenu.addMenu(&rowMenu);

    contextMenu.exec(mapToGlobal(pos));
}

void CppViewEntryWidget::on_actionInsertColumn_triggered()
{
    this->view->insertColumn(this->columnNum);
}

void CppViewEntryWidget::on_actionDelColumn_triggered()
{
    this->view->delColumn(this->columnNum);
}

void CppViewEntryWidget::on_actionHideColumn_triggered()
{
    this->view->hideColumn(this->columnNum);
}

void CppViewEntryWidget::on_actionInsertRow_triggered()
{
    this->view->insertRow(this->rowNum);
}

void CppViewEntryWidget::on_actionDelRow_triggered()
{
    this->view->delRow(this->rowNum);
}

void CppViewEntryWidget::on_actionHideRow_triggered()
{
    this->view->hideRow(this->rowNum);
}

void CppViewEntryWidget::on_entryLineEdit_returnPressed()
{
    QString text = ((LineEditWidget *)this->widget)->text();
    this->view->setTableContent(this->rowNum, this->columnNum, text);
    this->on_entryLineEdit_escPressed();
}

void CppViewEntryWidget::on_entryLineEdit_escPressed()
{
    QString text = this->table->getRow(this->rowNum - 1)->getEntry(this->columnNum - 1)->getContent();
    ((LineEditWidget *)this->widget)->setText(text);
    this->widget->clearFocus();
}
