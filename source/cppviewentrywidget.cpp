#include "cppviewentrywidget.h"

#include <QApplication>
#include <QStyle>

#include "cppview.h"
#include "d1cpp.h"
#include "labelwidget.h"
#include "lineeditwidget.h"
#include "pushbuttonwidget.h"

#include "ui_cppviewentrywidget.h"

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

void CppViewEntryWidget::initialize(D1CppTable *t, int r, int c, int width)
{
    this->table = t;
    this->rowNum = r;
    this->columnNum = c;

    // clear the layout
    QLayoutItem *child;
    while ((child = this->ui->entryHorizontalLayout->takeAt(0)) != nullptr) {
        delete child->widget(); // delete the widget
        delete child;           // delete the layout item
    }

    QWidget *w;
    if (r == 0) {
        // header
        QString text = t->getHeader(c - 1);
        QFontMetrics fm = this->fontMetrics();
        if (fm.horizontalAdvance(text) > width) {
            w = PushButtonWidget::addButton(this, QStyle::SP_DialogHelpButton, text, this, &CppViewEntryWidget::on_headerButton_clicked);
        } else {
            w = new LabelWidget(text, this);
            ((LabelWidget *)w)->setFixedWidth(width);
        }
    } else if (c == 0) {
        // leader
        w = new LabelWidget(t->getLeader(r - 1),this);
        ((LabelWidget *)w)->setFixedWidth(width);
    } else {
        // standard entry
        w = new LineEditWidget(t->getRow(r - 1)->getEntry(c - 1)->getContent(), this);
        ((LineEditWidget *)w)->setMinimumWidth(width);
        w->setToolTip(QString("%1/%2").arg(t->getLeader(r - 1)).arg(t->getHeader(c - 1)));
        QObject::connect(w, SIGNAL(returnPressed()), this, SLOT(on_entryLineEdit_returnPressed()));
        // connect esc events of LineEditWidgets
        QObject::connect(w, SIGNAL(cancel_signal()), this, SLOT(on_entryLineEdit_escPressed()));
    }
    this->widget = w;
    this->ui->entryHorizontalLayout->addWidget(w);
}

void CppViewEntryWidget::on_headerButton_clicked()
{

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
