#pragma once

#include <QDialog>
#include <QFrame>

namespace Ui {
class ProgressDialog;
class ProgressWidget;
} // namespace Ui

class enum PROGESS_STATE {
    DONE,
    RUNNING,
    CANCELLED,
};

class enum PROGESS_DIALOG_STATE {
    ACTIVE,
    OPEN,
    BACKGROUND,
};


class ProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);
    ~ProgressDialog();

    static void start(PROGESS_DIALOG_STATE mode, const QString &label, int maxValue);
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
    void on_closePushButton_clicked();

protected:
    void closeEvent(QCloseEvent *e) override;

private:
    void removeLastLine();

private:
    Ui::ProgressDialog *ui;

    int textVersion;
    PROGESS_STATE status = PROGESS_STATE::DONE;
};

ProgressDialog &dProgress();

class ProgressWidget : public QFrame {
    Q_OBJECT

    friend class ProgressDialog;

public:
    explicit ProgressWidget(QWidget *parent = nullptr);
    ~ProgressWidget();

    void update(PROGESS_STATE status, bool active);

private:
    void setLabelText(const QString &text);

private slots:
    void on_openPushButton_clicked();

private:
    Ui::ProgressWidget *ui;
};
