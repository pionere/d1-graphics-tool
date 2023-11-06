#pragma once

#include <QDialog>

namespace Ui {
class CppRowChangeDialog;
}

class CppRowChangeDialog : public QDialog {
    Q_OBJECT

public:
    explicit CppRowChangeDialog(QWidget *view);
    ~CppRowChangeDialog();

    void initialize(int index);

private slots:
    void on_doneButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::CppRowChangeDialog *ui;

    int index;
};
