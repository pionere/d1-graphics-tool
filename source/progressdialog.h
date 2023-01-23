#pragma once

#include <QDialog>

namespace Ui {
class ProgressDialog;
} // namespace Ui

class ProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);
    ~ProgressDialog();

    static void start(const QString &label, int maxValue);
    static void done();

    static bool wasCanceled();
    static void incValue();

    friend ProgressDialog &dProgress();

    ProgressDialog &operator<<(QString text);

private slots:
    void on_detailsPushButton_clicked();
    void on_cancelPushButton_clicked();

protected:
    void closeEvent(QCloseEvent *e) override;

private:
    Ui::ProgressDialog *ui;

    bool cancelled;
};
