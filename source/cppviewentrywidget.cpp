#include "cppviewentrywidget.h"

#include <QApplication>
#include <QStyle>

#include "cppview.h"
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

void CppViewEntryWidget::initialize(D1CppTable *t, int r, int c)
{
    this->table = t;
    this->rowNum = r;
    this->columnNum = c;

	this->ui->entryHorizontalLayout->clear();

	QWidget *w;
	if (r == 0) {
		// header
		w = QLabel(t->getHeader(c - 1));
    } else if (c == 0) {
		// leader
		w = QLabel(t->getLeader(r - 1));
    } else {
		// standard entry
		w = new LineEditWidget(this);
        ((LineEditWidget *)w)->setText(t->getRow(r - 1)->getEntry(c - 1)->getContent());
    }
	this->ui->entryHorizontalLayout->addWidget(w);
}
