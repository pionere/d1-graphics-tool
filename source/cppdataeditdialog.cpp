#include "cppdataeditdialog.h"

#include <QMessageBox>

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
    QComboBox *fromComboBox = this->ui->fromComboBox;
    QComboBox *toComboBox = this->ui->toComboBox;
    // setup the options
    fromComboBox->clear();
    toComboBox->clear();
    D1CppTable *table = qobject_cast<CppView *>(this->parentWidget())->getCurrentTable();
    switch (t) {
    case CPP_EDIT_MODE::COLUMN_HIDE:
    case CPP_EDIT_MODE::COLUMN_SHOW:
    case CPP_EDIT_MODE::COLUMN_DEL:
        for (int i = 0; i < table->getColumnCount(); i++) {
            QString header = table->getHeader(i);
            if (header.isEmpty()) {
                header = tr("-- %1.column --").arg(i + 1);
            }
            fromComboBox->addItem(header);
            toComboBox->addItem(header);
        }
        break;
    case CPP_EDIT_MODE::ROW_HIDE:
    case CPP_EDIT_MODE::ROW_SHOW:
    case CPP_EDIT_MODE::ROW_DEL:
        for (int i = 0; i < table->getRowCount(); i++) {
            QString leader = table->getLeader(i);
            if (leader.isEmpty()) {
                leader = tr("-- %1.row --").arg(i + 1);
            }
            fromComboBox->addItem(leader);
            toComboBox->addItem(leader);
        }
        break;
    }

    if (index <= 0) {
        index = 1;
    }
    index--;
    fromComboBox->setCurrentIndex(index);
    toComboBox->setCurrentIndex(index);


    // if (this->type != (int)t) {
        this->type = (int)t;
        // select window title
        QString title;
        switch (t) {
        case CPP_EDIT_MODE::COLUMN_HIDE:
            title = tr("Hide Columns");
            break;
        case CPP_EDIT_MODE::COLUMN_SHOW:
            title = tr("Show Columns");
            break;
        case CPP_EDIT_MODE::COLUMN_DEL:
            title = tr("Delete Columns");
            break;
        case CPP_EDIT_MODE::ROW_HIDE:
            title = tr("Hide Rows");
            break;
        case CPP_EDIT_MODE::ROW_SHOW:
            title = tr("Show Rows");
            break;
        case CPP_EDIT_MODE::ROW_DEL:
            title = tr("Delete Rows");
            break;
        }
        this->setWindowTitle(title);
    // }
}

void CppDataEditDialog::on_fromComboBox_activated(int index)
{
    int toIndex = this->ui->toComboBox->currentIndex();
    if (toIndex < index) {
        this->ui->toComboBox->setCurrentIndex(index);
    }
}

void CppDataEditDialog::on_toComboBox_activated(int index)
{
    int fromIndex = this->ui->fromComboBox->currentIndex();
    if (fromIndex > index) {
        this->ui->fromComboBox->setCurrentIndex(index);
    }
}

void CppDataEditDialog::on_startButton_clicked()
{
    CPP_EDIT_MODE t = (CPP_EDIT_MODE)this->type;

    int fromIndex = this->ui->fromComboBox->currentIndex();
    int toIndex = this->ui->toComboBox->currentIndex();

    this->close();

    CppView *view = qobject_cast<CppView *>(this->parentWidget());
    fromIndex++;
    toIndex++;
    int maxIndex = 0;
    D1CppTable *table = view->getCurrentTable();
    switch (t) {
    case CPP_EDIT_MODE::COLUMN_HIDE:
    case CPP_EDIT_MODE::COLUMN_SHOW:
    case CPP_EDIT_MODE::COLUMN_DEL:
        maxIndex = table->getColumnCount();
        break;
    case CPP_EDIT_MODE::ROW_HIDE:
    case CPP_EDIT_MODE::ROW_SHOW:
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

    switch (t) {
    case CPP_EDIT_MODE::COLUMN_HIDE:
        view->hideColumns(fromIndex, toIndex);
        break;
    case CPP_EDIT_MODE::COLUMN_SHOW:
        view->showColumns(fromIndex, toIndex);
        break;
    case CPP_EDIT_MODE::COLUMN_DEL:
        view->delColumns(fromIndex, toIndex);
        break;
    case CPP_EDIT_MODE::ROW_HIDE:
        view->hideRows(fromIndex, toIndex);
        break;
    case CPP_EDIT_MODE::ROW_SHOW:
        view->showRows(fromIndex, toIndex);
        break;
    case CPP_EDIT_MODE::ROW_DEL:
        view->delRows(fromIndex, toIndex);
        break;
    }
}

void CppDataEditDialog::on_cancelButton_clicked()
{
    this->close();
}
