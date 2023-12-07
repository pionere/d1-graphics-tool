#pragma once

#include <QDialog>

#include "d1gfx.h"

class MergeFramesParam {
public:
    int rangeFrom;
    int rangeTo;
};

namespace Ui {
class MergeFramesDialog;
}

class MergeFramesDialog : public QDialog {
    Q_OBJECT

public:
    explicit MergeFramesDialog(QWidget *parent);
    ~MergeFramesDialog();

    void initialize(D1Gfx *gfx);

private slots:
    void on_submitButton_clicked();
    void on_cancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::MergeFramesDialog *ui;

    D1Gfx *gfx;
};
