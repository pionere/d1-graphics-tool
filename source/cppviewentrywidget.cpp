#include "cppviewentrywidget.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>
#include <QWidgetAction>

#include "cppview.h"
#include "d1cpp.h"
#include "lineeditwidget.h"
#include "pushbuttonwidget.h"
#include "ui_cppviewentrywidget.h"

#include "dungeon/all.h"

static int entryHorizontalMargin = 0;

CppViewEntryWidget::CppViewEntryWidget(CppView *parent)
    : QWidget(parent)
    , ui(new Ui::CppViewEntryWidget())
    , view(parent)
    , widget(nullptr)
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

    // clear the layout
    QHBoxLayout *layout = this->ui->entryHorizontalLayout;
    for (int i = layout->count() - 1; i >= 0; i--) {
        QWidget *w = layout->itemAt(i)->widget();
        if (w != nullptr) {
            w->deleteLater();
        }
    }

    QWidget *w;

    bool complexFirst = false;
    bool complexLast = false;
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
                //((QPushButton *)w)->setFlat(true);
                ((QPushButton *)w)->setStyleSheet("border: 0;");
                QObject::connect(w, SIGNAL(clicked()), this, SLOT(ShowHeaderContextMenu()));
            }

            if (t->getRowCount() != 0) {
                D1CppRowEntry *entry = t->getRow(0)->getEntry(c - 1);
                complexFirst = entry->isComplexFirst();
                complexLast = entry->isComplexLast();
            } else {
                // FIXME: store complexFirst/Last in the header
            }
        }
    } else if (c == 0) {
        // leader
        w = new QPushButton(this);
        ((QPushButton *)w)->setFixedWidth(width);
        //((QPushButton *)w)->setFlat(true);
        QString tooltip = t->getRowText(r);
        QString text = t->getLeader(r - 1);
        QString style = "border: 0;";
        if (!tooltip.isEmpty()) {
            tooltip.prepend("<html><head/><body><p style='white-space:pre'>");
            tooltip.append("</p></body></html>");
            w->setToolTip(tooltip);
            style.append("font: italic;");
        }
        ((QPushButton *)w)->setText(text);
        ((QPushButton *)w)->setStyleSheet(style);
        QObject::connect(w, SIGNAL(clicked()), this, SLOT(ShowRowContextMenu()));
    } else {
        // standard entry
        D1CppRowEntry *entry = t->getRow(r - 1)->getEntry(c - 1);
        w = new LineEditWidget(entry->getContent(), this);
        ((LineEditWidget *)w)->setMinimumWidth(width);
        w->setToolTip(QString("%1/%2").arg(t->getLeader(r - 1)).arg(t->getHeader(c - 1)));
        QObject::connect(w, SIGNAL(returnPressed()), this, SLOT(on_entryLineEdit_returnPressed()));
        // connect esc events of LineEditWidgets
        QObject::connect(w, SIGNAL(cancel_signal()), this, SLOT(on_entryLineEdit_escPressed()));
        QObject::connect(w, SIGNAL(focus_gain_signal()), this, SLOT(on_entryLineEdit_focusGain()));
        // QObject::connect(w, SIGNAL(focus_lost_signal()), this, SLOT(on_entryLineEdit_focusLost()));
        complexFirst = entry->isComplexFirst();
        complexLast = entry->isComplexLast();
    }
    this->widget = w;

    layout->addWidget(w);

    if (r != 0 && c != 0) {
        // standard entry
        QString text = t->getRow(r - 1)->getEntryText(c - 1);
        w = PushButtonWidget::addButton(this, QStyle::SP_MessageBoxInformation, text, this, &CppViewEntryWidget::on_infoButton_clicked);
        w->setVisible(false);
        layout->addWidget(w);
    }

    if (complexFirst || complexLast) {
        QFontMetrics fm = this->fontMetrics();
        int width = fm.horizontalAdvance("{");
        if (complexFirst) {
            QLabel *label = new QLabel("{");
            label->setFixedWidth(width);
            layout->insertWidget(0, label);
        }
        if (complexLast) {
            QLabel *label = new QLabel("}");
            label->setFixedWidth(width);
            layout->addWidget(label);
        }
    }
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
    int c = this->columnNum;
    D1CppTable *t = this->table;

    QString text = t->getHeader(c - 1);
    QFontMetrics fm = this->fontMetrics();
    int width = fm.horizontalAdvance(text);
    QWidget *w = new QPushButton(text, this);
    ((QPushButton *)w)->setFixedWidth(width);
    //((QPushButton *)w)->setFlat(true);
    ((QPushButton *)w)->setStyleSheet("border: 0;");
    QObject::connect(w, SIGNAL(clicked()), this, SLOT(ShowHeaderContextMenu()));

    this->ui->entryHorizontalLayout->replaceWidget(this->widget, w);
    this->widget->deleteLater();
    this->widget = w;
}

void CppViewEntryWidget::on_infoButton_clicked()
{

}

void CppViewEntryWidget::on_toggleInfoButton(bool visible)
{
    PushButtonWidget *w = this->ui->entryHorizontalLayout->parentWidget()->findChild<PushButtonWidget *>();
    if (this->rowNum == 0) {
        // header
        if (this->columnNum == 0) {
            /*QString showTooltip = tr("Show info");
            if (w->toolTip() == showTooltip) {
                showTooltip = tr("Hide info");
            }*/
            QString showTooltip = visible ? tr("Show info") : tr("Hide info");
            w->setToolTip(showTooltip);
        }
    } else if (this->columnNum != 0) {
        // standard entry
        // if (layout->count() == 2) {
        if (w != nullptr) {
            // w->setVisible(!w->isVisible());
            w->setVisible(visible);
        }
    }
}

void CppViewEntryWidget::setFocus()
{
    QWidget::setFocus();

    if (this->columnNum != 0 && this->rowNum != 0) {
        this->widget->setFocus();
    }
}

void CppViewEntryWidget::ShowHeaderContextMenu()
{
    QAction *action;
    QLabel *label;
    QMenu contextMenu(this);
    QMenu *menu = &contextMenu;

    menu->setToolTipsVisible(true);
    QString header = ((QPushButton *)this->widget)->text();
    //if (!header.isEmpty()) {
        label = new QLabel(header, this);
        label->setAlignment(Qt::AlignCenter);
        action = new QWidgetAction(menu);
        ((QWidgetAction *)action)->setDefaultWidget(label);
        menu->addAction(action);
    //}
    QString type;
    switch (this->table->getColumnType(this->columnNum - 1)) {
    case D1CPP_ENTRY_TYPE::String:       type = tr("string");  break;
    case D1CPP_ENTRY_TYPE::QuotedString: type = tr("text");    break;
    case D1CPP_ENTRY_TYPE::Integer:      type = tr("integer"); break;
    case D1CPP_ENTRY_TYPE::Real:         type = tr("real");    break;
    };
    label = new QLabel(QString("<u><i>(%1)</i></u>").arg(type), this);
    label->setAlignment(Qt::AlignCenter);
    action = new QWidgetAction(menu);
    ((QWidgetAction *)action)->setDefaultWidget(label);
    menu->addAction(action);

    action = new QAction(tr("Change..."));
    action->setToolTip(tr("Change the name or the type of this column"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionChangeColumn_triggered()));
    menu->addAction(action);

    action = new QAction(tr("Trim"));
    action->setToolTip(tr("Trim the content in this column"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionTrimColumn_triggered()));
    menu->addAction(action);

    action = new QAction(tr("Insert"));
    action->setToolTip(tr("Add a new column before this one"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionInsertColumn_triggered()));
    menu->addAction(action);

    action = new QAction(tr("Duplicate"));
    action->setToolTip(tr("Duplicate this column"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionDuplicateColumn_triggered()));
    menu->addAction(action);

    action = new QAction(tr("Delete"));
    action->setToolTip(tr("Delete this column"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionDelColumn_triggered()));
    menu->addAction(action);

    action = new QAction(tr("Hide"));
    action->setToolTip(tr("Hide this column"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionHideColumn_triggered()));
    menu->addAction(action);

    action = new QAction("<<--");
    action->setToolTip(tr("Move this column to the left"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionMoveLeftColumn_triggered()));
    action->setEnabled(this->columnNum > 1);
    menu->addAction(action);

    action = new QAction("-->>");
    action->setToolTip(tr("Move this column to the right"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionMoveRightColumn_triggered()));
    action->setEnabled(this->columnNum < this->table->getColumnCount());
    menu->addAction(action);

    QPoint pos = QCursor::pos();
    menu->exec(pos);
}

void CppViewEntryWidget::ShowRowContextMenu()
{
    QAction *action;
    QLabel *label;
    QMenu contextMenu(this);
    QMenu *menu = &contextMenu;

    menu->setToolTipsVisible(true);
    QString leader = ((QPushButton *)this->widget)->text();
    //if (!leader.isEmpty()) {
        label = new QLabel(QStringLiteral("<u>%1</u>").arg(leader), this);
        label->setAlignment(Qt::AlignCenter);
        action = new QWidgetAction(menu);
        ((QWidgetAction *)action)->setDefaultWidget(label);
        menu->addAction(action);
    //}

    action = new QAction(tr("Change..."));
    action->setToolTip(tr("Change the leader text of this row"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionChangeRow_triggered()));
    menu->addAction(action);

    action = new QAction(tr("Insert"));
    action->setToolTip(tr("Add a new row before this one"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionInsertRow_triggered()));
    menu->addAction(action);

    action = new QAction(tr("Duplicate"));
    action->setToolTip(tr("Duplicate this row"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionDuplicateRow_triggered()));
    menu->addAction(action);

    action = new QAction(tr("Delete"));
    action->setToolTip(tr("Delete this row"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionDelRow_triggered()));
    menu->addAction(action);

    action = new QAction(tr("Hide"));
    action->setToolTip(tr("Hide this row"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionHideRow_triggered()));
    menu->addAction(action);

    action = new QAction(QStringLiteral("^^ ^^"));
    action->setToolTip(tr("Move this row up"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionMoveUpRow_triggered()));
    action->setEnabled(this->rowNum > 1);
    menu->addAction(action);

    action = new QAction(QStringLiteral("vv vv"));
    action->setToolTip(tr("Move this row down"));
    QObject::connect(action, SIGNAL(triggered()), this, SLOT(on_actionMoveDownRow_triggered()));
    action->setEnabled(this->rowNum < this->table->getRowCount());
    menu->addAction(action);

    QPoint pos = QCursor::pos();
    menu->exec(pos);
}

void CppViewEntryWidget::on_actionChangeColumn_triggered()
{
    this->view->changeColumn(this->columnNum);
}

void CppViewEntryWidget::on_actionTrimColumn_triggered()
{
    this->view->trimColumn(this->columnNum);
}

void CppViewEntryWidget::on_actionInsertColumn_triggered()
{
    this->view->insertColumn(this->columnNum);
}

void CppViewEntryWidget::on_actionDuplicateColumn_triggered()
{
    this->view->duplicateColumn(this->columnNum);
}

void CppViewEntryWidget::on_actionDelColumn_triggered()
{
    this->view->delColumns(this->columnNum, this->columnNum);
}

void CppViewEntryWidget::on_actionHideColumn_triggered()
{
    this->view->hideColumns(this->columnNum, this->columnNum);
}

void CppViewEntryWidget::on_actionMoveLeftColumn_triggered()
{
    this->view->moveColumnLeft(this->columnNum);
}

void CppViewEntryWidget::on_actionMoveRightColumn_triggered()
{
    this->view->moveColumnRight(this->columnNum);
}

void CppViewEntryWidget::on_actionChangeRow_triggered()
{
    this->view->changeRow(this->rowNum);
}

void CppViewEntryWidget::on_actionInsertRow_triggered()
{
    this->view->insertRow(this->rowNum);
}

void CppViewEntryWidget::on_actionDuplicateRow_triggered()
{
    this->view->duplicateRow(this->rowNum);
}

void CppViewEntryWidget::on_actionDelRow_triggered()
{
    this->view->delRows(this->rowNum, this->rowNum);
}

void CppViewEntryWidget::on_actionHideRow_triggered()
{
    this->view->hideRows(this->rowNum, this->rowNum);
}

void CppViewEntryWidget::on_actionMoveUpRow_triggered()
{
    this->view->moveRowUp(this->rowNum);
}

void CppViewEntryWidget::on_actionMoveDownRow_triggered()
{
    this->view->moveRowDown(this->rowNum);
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

void CppViewEntryWidget::on_entryLineEdit_focusGain()
{
    this->view->setCurrent(this->rowNum, this->columnNum);
}

void CppViewEntryWidget::on_entryLineEdit_focusLost()
{
    this->view->setCurrent(-1, -1);
}
