#include "cpprowchangedialog.h"

#include "cppview.h"
#include "ui_cpprowchangedialog.h"

CppRowChangeDialog::CppRowChangeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CppRowChangeDialog())
{
    this->ui->setupUi(this);
}

CppRowChangeDialog::~CppRowChangeDialog()
{
    delete ui;
}

void CppRowChangeDialog::initialize(int idx)
{
    this->index = idx;

    CppView *view = qobject_cast<CppView *>(this->parentWidget());
    this->ui->textLineEdit->setText(view->getCurrentTable()->getLeader(idx - 1));
}

void CppRowChangeDialog::on_doneButton_clicked()
{
    QString text = this->ui->textLineEdit->text();

    this->close();

    CppView *view = qobject_cast<CppView *>(this->parentWidget());
    view->setRowLeader(this->index, text);
}

void CppRowChangeDialog::on_cancelButton_clicked()
{
    this->close();
}
