#include "cppviewentrywidget.h"

#include <QApplication>
#include <QStyle>

#include "cppview.h"
#include "d1cpp.h"
#include "labelwidget.h"
#include "lineeditwidget.h"

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
        w = new LabelWidget(t->getHeader(c - 1));
		((LabelWidget *)w)->setCharWidth(width);
    } else if (c == 0) {
        // leader
        w = new LabelWidget(t->getLeader(r - 1));
		((LabelWidget *)w)->setCharWidth(width);
    } else {
        // standard entry
        w = new LineEditWidget(this);
		((LineEditWidget *)w)->setCharWidth(width);
        ((LineEditWidget *)w)->setText(t->getRow(r - 1)->getEntry(c - 1)->getContent());
    }
    this->ui->entryHorizontalLayout->addWidget(w);
}
