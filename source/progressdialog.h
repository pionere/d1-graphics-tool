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

    ProgressDialog &operator<<(const QString &text);
    ProgressDialog &operator<<(const QPair<QString, QString> &text);
    ProgressDialog &operator<<(QPair<int, QString> &tdxText);

private slots:
    void on_detailsPushButton_clicked();
    void on_cancelPushButton_clicked();

protected:
    void closeEvent(QCloseEvent *e) override;

private:
    void removeLastLine();

private:
    Ui::ProgressDialog *ui;

    int textVersion;
    bool cancelled;
};

ProgressDialog &dProgress();
