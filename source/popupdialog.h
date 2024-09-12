#pragma once

#include <QDialog>
#include <QString>

namespace Ui {
class PopupDialog;
}

class PopupDialog : public QDialog {
    Q_OBJECT

public:
    explicit PopupDialog(QWidget *parent);
    ~PopupDialog();

    void initialize(const QString &title, QString content, std::function<void(QString)> &&callFunc);

private slots:
    void on_textLineEdit_returnPressed();
    void on_textLineEdit_escPressed();

private:
    Ui::PopupDialog *ui;

    std::function<void(QString)> callFunc;
};
