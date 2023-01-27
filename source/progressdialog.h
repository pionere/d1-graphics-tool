#pragma once

#include <QDialog>
#include <QFrame>

namespace Ui {
class ProgressDialog;
class ProgressWidget;
} // namespace Ui

enum class PROGRESS_STATE {
    DONE,
    RUNNING,
    WARN,
    ERROR,
    CANCEL,
    FAIL,
};

enum class PROGRESS_DIALOG_STATE {
    ACTIVE,
    OPEN,
    BACKGROUND,
};

enum class PROGRESS_TEXT_MODE {
    NORMAL,
    WARNING,
    ERROR,
};

class ProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);
    ~ProgressDialog();

    static void start(PROGRESS_DIALOG_STATE mode, const QString &label, int maxValue);
    static void done(bool forceOpen = false);

    static bool wasCanceled();
    static void incValue();

    friend ProgressDialog &dProgress();
    friend ProgressDialog &dProgressWarn();
    friend ProgressDialog &dProgressErr();
    friend ProgressDialog &dProgressFail();

    ProgressDialog &operator<<(const QString &text);
    ProgressDialog &operator<<(const QPair<QString, QString> &text);
    ProgressDialog &operator<<(QPair<int, QString> &tdxText);

private slots:
    void on_detailsPushButton_clicked();
    void on_cancelPushButton_clicked();
    void on_closePushButton_clicked();

protected:
    void closeEvent(QCloseEvent *e) override;
    void changeEvent(QEvent *e);

private:
    void removeLastLine();

private:
    Ui::ProgressDialog *ui;

    int textVersion;
    PROGRESS_STATE status = PROGRESS_STATE::DONE;
    PROGRESS_TEXT_MODE textMode = PROGRESS_TEXT_MODE::NORMAL;
};

ProgressDialog &dProgress();
ProgressDialog &dProgressWarn();
ProgressDialog &dProgressErr();
ProgressDialog &dProgressFail();

class ProgressWidget : public QFrame {
    Q_OBJECT

    friend class ProgressDialog;

public:
    explicit ProgressWidget(QWidget *parent = nullptr);
    ~ProgressWidget();

private:
    void update(PROGRESS_STATE status, bool active, const QString &text);

private slots:
    void on_openPushButton_clicked();

private:
    Ui::ProgressWidget *ui;
};
