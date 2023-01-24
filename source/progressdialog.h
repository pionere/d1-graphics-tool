#pragma once

#include <QDialog>
#include <QFrame>

namespace Ui {
class ProgressDialog;
class ProgressWidget;
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

class ProgressWidget : public QFrame {
    Q_OBJECT

public:
    explicit ProgressWidget(QWidget *parent = nullptr);
    ~ProgressWidget();

    void update();

private:
    void setLabelText(const QString &text);

private slots:
    void on_openPushButton_clicked();

private:
    Ui::ProgressWidget *ui;
};
