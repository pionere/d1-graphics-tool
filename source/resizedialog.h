#pragma once

#include <QDialog>

class D1Gfxset;

enum class RESIZE_PLACEMENT {
    TOP_LEFT,
    TOP,
    TOP_RIGHT,
    CENTER_LEFT,
    CENTER,
    CENTER_RIGHT,
    BOTTOM_LEFT,
    BOTTOM,
    BOTTOM_RIGHT,
};

class ResizeParam {
public:
    int width;
    int height;
    int backcolor;
    int rangeFrom;
    int rangeTo;
    bool resizeAll;
    RESIZE_PLACEMENT placement;
};

namespace Ui {
class ResizeDialog;
}

class ResizeDialog : public QDialog {
    Q_OBJECT

public:
    explicit ResizeDialog(QWidget *parent);
    ~ResizeDialog();

    void initialize(D1Gfxset *gfxset);

private slots:
    void on_resizeButton_clicked();
    void on_resizeCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::ResizeDialog *ui;
};
