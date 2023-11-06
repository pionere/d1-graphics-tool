#include "cppcolumnchangedialog.h"

#include "cppview.h"
#include "ui_cppcolumnchangedialog.h"

CppColumnChangeDialog::CppColumnChangeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CppColumnChangeDialog())
{
    this->ui->setupUi(this);
}

CppColumnChangeDialog::~CppColumnChangeDialog()
{
    delete ui;
}

void CppColumnChangeDialog::initialize(int idx)
{
    this->index = idx;

    CppView *view = qobject_cast<CppView *>(this->parentWidget());
    this->ui->nameLineEdit->setText(view->getCurrentTable()->getHeader(idx - 1));
    this->ui->typeComboBox->setCurrentIndex((int)view->getCurrentTable()->getColumnType(idx - 1));
}

void CppColumnChangeDialog::on_doneButton_clicked()
{
    QString name = this->ui->nameLineEdit->text();
    D1CPP_ENTRY_TYPE type = (D1CPP_ENTRY_TYPE)this->ui->typeComboBox->currentIndex();

    this->close();

    CppView *view = qobject_cast<CppView *>(this->parentWidget());
    view->setColumnNameType(this->index, name, type);
}

void CppColumnChangeDialog::on_cancelButton_clicked()
{
    this->close();
}
