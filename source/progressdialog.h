#pragma once

#include <QProgressDialog>
#include <QWidget>

namespace Ui {
class ProgressDialog;
} // namespace Ui

class ProgressDialog : public QWidget {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);
    ~ProgressDialog();

    static void start(const QString &label, int maxValue);
    static void done();

    static bool wasCanceled();
    static void incValue();

private slots:
    void on_cancelPushButton_clicked();

private:
    bool wasCanceled_impl();
    void setValue_impl(int value);

private:
    Ui::ProgressDialog *ui;

    bool cancelled;
};
