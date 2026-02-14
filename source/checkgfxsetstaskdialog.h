#pragma once

#include <QDialog>
#include <QString>

class CheckGfxsetsTaskParam {
public:
    QString folder;
    bool recursive;
};

namespace Ui {
class CheckGfxsetsTaskDialog;
}

class CheckGfxsetsTaskDialog : public QDialog {
    Q_OBJECT

public:
    explicit CheckGfxsetsTaskDialog(QWidget *parent);
    ~CheckGfxsetsTaskDialog();

private:
    static void runTask(const CheckGfxsetsTaskParam &params);

private slots:
    void on_assetsFolderBrowseButton_clicked();
    void on_checkRunButton_clicked();
    void on_checkCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::CheckGfxsetsTaskDialog *ui;
};
