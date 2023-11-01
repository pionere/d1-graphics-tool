#include "cppdataeditdialog.h"

#include "cppview.h"
#include "ui_cppdataeditdialog.h"

CppDataEditDialog::CppDataEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CppDataEditDialog())
{
    this->ui->setupUi(this);
}

CppDataEditDialog::~CppDataEditDialog()
{
    delete ui;
}

void CppDataEditDialog::initialize(CPP_EDIT_MODE t, int index)
{
    if (this->type != (int)t) {
        this->type = (int)t;
        // reset the fields
		if (index != -1) {
			this->ui->fromLineEdit->setText(Qstring::number(index));
			this->ui->toLineEdit->setText(Qstring::number(index));
        }
        // select window title
        QString title;
        switch (t) {
        case CPP_EDIT_MODE::COLUMN_HIDE:
            title = tr("Hide Columns");
            break;
        case CPP_EDIT_MODE::COLUMN_DEL:
            title = tr("Delete Columns");
            break;
        case CPP_EDIT_MODE::ROW_HIDE:
            title = tr("Hide Rows");
            break;
        case CPP_EDIT_MODE::ROW_DEL:
            title = tr("Delete Rows");
            break;
        }
        this->setWindowTitle(title);
    }
}

void CppDataEditDialog::on_startButton_clicked()
{
	CPP_EDIT_MODE t = (CPP_EDIT_MODE)this->type;

	int fromIndex = this->ui->fromLineEdit->text().toInt();
	int toIndex = this->ui->toLineEdit->text().toInt();

    this->close();

    CppView *view = qobject_cast<CppView *>(this->parentWidget());
	if (fromIndex <= 0) {
		fromIndex = 1;
    }
	int maxIndex = 0;
	D1CppTable *table = view->getCurrentTable();
    switch (t) {
    case CPP_EDIT_MODE::COLUMN_HIDE:
    case CPP_EDIT_MODE::COLUMN_DEL:
		maxIndex = table->getColumnCount();
        break;
    case CPP_EDIT_MODE::ROW_HIDE:
    case CPP_EDIT_MODE::ROW_DEL:
		maxIndex = table->getRowCount();
        break;
    }
	if (fromIndex > maxIndex) {
		return;
    }

	if (toIndex <= 0) {
		if (toIndex == 0) {
			toIndex = maxIndex;
        } else {
			return;
        }
    }
	if (toIndex > maxIndex) {
		toIndex = maxIndex;
    }

	for (int i = fromIndex; i <= toIndex; i++) {
		switch (t) {
		case CPP_EDIT_MODE::COLUMN_HIDE:
			view->hideColumn(fromIndex);
			break;
		case CPP_EDIT_MODE::COLUMN_DEL:
			view->delColumn(fromIndex);
			break;
		case CPP_EDIT_MODE::ROW_HIDE:
			view->hideRow(fromIndex);
			break;
		case CPP_EDIT_MODE::ROW_DEL:
			view->delRow(fromIndex);
			break;
		}
    }
}

void CppDataEditDialog::on_cancelButton_clicked()
{
    this->close();
}
