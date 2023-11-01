#pragma once

#include <QDialog>

namespace Ui {
class CppColumnRenameDialog;
}

class CppColumnRenameDialog : public QDialog {
    Q_OBJECT

public:
    explicit CppColumnRenameDialog(QWidget *view);
    ~CppColumnRenameDialog();

    void initialize(int index);

private slots:
    void on_doneButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::CppColumnRenameDialog *ui;

    int index;
};
