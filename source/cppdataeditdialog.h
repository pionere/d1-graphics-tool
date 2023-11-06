#pragma once

#include <QDialog>

class D1Dun;

enum class CPP_EDIT_MODE {
    COLUMN_HIDE,
    COLUMN_SHOW,
    COLUMN_DEL,
    ROW_HIDE,
    ROW_SHOW,
    ROW_DEL,
};

namespace Ui {
class CppDataEditDialog;
}

class CppDataEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit CppDataEditDialog(QWidget *view);
    ~CppDataEditDialog();

    void initialize(CPP_EDIT_MODE type, int index);

private slots:
    void on_fromComboBox_activated(int index);
    void on_toComboBox_activated(int index);

    void on_startButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::CppDataEditDialog *ui;

    int type = -1;
};
