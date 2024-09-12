#include "popupdialog.h"

#include "ui_popupdialog.h"

PopupDialog::PopupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PopupDialog())
{
    this->ui->setupUi(this);
}

PopupDialog::~PopupDialog()
{
    delete ui;
}

void PopupDialog::initialize(const QString &title, QString content, std::function<void(QString)> &&cf)
{
    this->setWindowTitle(title);
    this->ui->textLineEdit->setText(content);
    this->callFunc = cf;
}

void PopupDialog::on_textLineEdit_returnPressed()
{
    QString text = this->ui->textLineEdit->text();

    this->callFunc(text);

    this->close();
}

void PopupDialog::on_textLineEdit_escPressed()
{
    this->close();
}
