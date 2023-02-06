#pragma once

#include <QDialog>
#include <QFrame>
#include <QProgressBar>
#include <QtConcurrent>

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
    BACKGROUND,
};

enum class PROGRESS_TEXT_MODE {
    NORMAL,
    WARNING,
    ERROR,
    FAIL,
};

class DPromise : public QObject {
    Q_OBJECT

public:
    bool isCanceled();
    void cancel();
    // void finish();
    void setProgressValue(int value);

signals:
    void progressValueChanged();
    // void finished();

private:
    PROGRESS_STATE status = PROGRESS_STATE::RUNNING;
};

class ProgressThread : public QThread {
    Q_OBJECT

public:
    explicit ProgressThread(std::function<void()> &&callFunc);
    ~ProgressThread() = default;

    void run() override;

    void cancel();

private:
    void reportResults();
    // void reportReady();

signals:
    void resultReady();
    // void taskReady();
    void cancelTask();

private:
    std::function<void()> callFunc;
};

class ProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);
    ~ProgressDialog();

    static void openDialog();

    static void start(PROGRESS_DIALOG_STATE mode, const QString &label, int numBars, bool forceOpen);
    static void done();

    static void startAsync(PROGRESS_DIALOG_STATE mode, const QString &label, int numBars, bool forceOpen, std::function<void()> &&callFunc);

    static void incProgressBar(const QString &label, int maxValue);

    static void incBar(const QString &label, int maxValue);
    static void decBar();

    static bool wasCanceled();
    static bool incValue();
    static bool addValue(int amount);

    friend ProgressDialog &dProgress();
    friend ProgressDialog &dProgressProc();
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
    void on_message_ready();
    void on_task_finished();

protected:
    void closeEvent(QCloseEvent *e) override;
    void changeEvent(QEvent *e);

private:
    static void incBar_impl(const QString &label, int maxValue);
    static void decBar_impl();
    static void incValue_impl(int amount);

    bool incBarValue(int index, int amount);
    void appendLine(PROGRESS_TEXT_MODE textMode, const QString &line, bool replace);
    void removeLastLine();
    void consumeMessages();

private:
    Ui::ProgressDialog *ui;

    QList<QProgressBar *> progressBars;
    int activeBars;
    PROGRESS_STATE status = PROGRESS_STATE::DONE;
    bool forceOpen;
};

ProgressDialog &dProgress();
ProgressDialog &dProgressProc();
ProgressDialog &dProgressWarn();
ProgressDialog &dProgressErr();
ProgressDialog &dProgressFail();

class ProgressWidget : public QDialog {
    Q_OBJECT

    friend class ProgressDialog;

public:
    explicit ProgressWidget(QWidget *parent = nullptr);
    ~ProgressWidget();

private:
    // void setModality(bool modal);
    void updateWidget(PROGRESS_STATE status, bool active, const QString &text);

private slots:
    void on_openPushButton_clicked();

private:
    Ui::ProgressWidget *ui;
};
