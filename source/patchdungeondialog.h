#pragma once

#include <QDialog>

#include "d1dun.h"

namespace Ui {
class PatchDungeonDialog;
}

class PatchDungeonDialog : public QDialog {
    Q_OBJECT

public:
    explicit PatchDungeonDialog(QWidget *parent);
    ~PatchDungeonDialog();

    void initialize(D1Dun *dun);

private slots:
    void on_runButton_clicked();
    void on_cancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::PatchDungeonDialog *ui;

    D1Dun *dun;
};
