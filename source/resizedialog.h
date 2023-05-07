#pragma once

#include <QDialog>

class D1Gfx;

class ResizeParam {
public:
    int width;
    int height;
    int backcolor;
    int rangeFrom;
    int rangeTo;
    bool center;
};

namespace Ui {
class ResizeDialog;
}

class ResizeDialog : public QDialog {
    Q_OBJECT

public:
    explicit ResizeDialog(QWidget *parent);
    ~ResizeDialog();

    void initialize(D1Gfx *gfx);

private slots:
    void on_resizeButton_clicked();
    void on_resizeCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::ResizeDialog *ui;
};
