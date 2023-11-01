#include "cppcolumnrenamedialog.h"

#include "cppview.h"
#include "ui_cppcolumnrenamedialog.h"

CppColumnRenameDialog::CppColumnRenameDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CppColumnRenameDialog())
{
    this->ui->setupUi(this);
}

CppColumnRenameDialog::~CppColumnRenameDialog()
{
    delete ui;
}

void CppColumnRenameDialog::initialize(int idx)
{
	this->index = idx;

	CppView *view = qobject_cast<CppView *>(this->parentWidget());
    this->ui->nameLineEdit->setText(view->getCurrentTable()->getHeader(idx - 1));
}

void CppColumnRenameDialog::on_doneButton_clicked()
{
    QString name = this->ui->nameLineEdit->text();

    this->close();

    CppView *view = qobject_cast<CppView *>(this->parentWidget());
	view->setColumnName(this->index, name);
}

void CppColumnRenameDialog::on_cancelButton_clicked()
{
    this->close();
}
