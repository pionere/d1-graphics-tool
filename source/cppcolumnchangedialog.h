#pragma once

#include <QDialog>

namespace Ui {
class CppColumnChangeDialog;
}

class CppColumnChangeDialog : public QDialog {
    Q_OBJECT

public:
    explicit CppColumnChangeDialog(QWidget *view);
    ~CppColumnChangeDialog();

    void initialize(int index);

private slots:
    void on_doneButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::CppColumnChangeDialog *ui;

    int index;
};
