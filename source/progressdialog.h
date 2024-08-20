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

typedef enum progress_after_flags {
    PAF_NONE = 0,
    PAF_OPEN_DIALOG = 1 << 0,
    PAF_UPDATE_WINDOW = 1 << 1,
} progress_after_flags;

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
    explicit ProgressDialog(QWidget *parent);
    ~ProgressDialog();

    static void openDialog();
    static bool isRunning();

    static void start(PROGRESS_DIALOG_STATE mode, const QString &label, int numBars, int flags);
    static void done();

    static void startAsync(PROGRESS_DIALOG_STATE mode, const QString &label, int numBars, int flags, std::function<void()> &&callFunc);

    static void incBar(const QString &label, int maxValue);
    static void decBar();

    static bool wasCanceled();
    static bool incValue();
    static bool addValue(int amount);

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
    void on_message_ready();
    void on_task_finished();

protected:
    void closeEvent(QCloseEvent *e) override;
    void changeEvent(QEvent *e) override;

private:
    static void incBar_impl(const QString &label, int maxValue);
    static void decBar_impl();
    static void incValue_impl(int amount);
    static void addResult_impl(PROGRESS_TEXT_MODE textMode, const QString &line, bool replace);

    static void consumeMessages();

private:
    Ui::ProgressDialog *ui;

    QList<QProgressBar *> progressBars;
    int activeBars;
    PROGRESS_STATE status = PROGRESS_STATE::DONE;
    bool running = false;
    int afterFlags; // progress_after_flags
};

ProgressDialog &dProgress();
ProgressDialog &dProgressWarn();
ProgressDialog &dProgressErr();
ProgressDialog &dProgressFail();

class ProgressWidget : public QFrame {
    Q_OBJECT

    friend class ProgressDialog;

public:
    explicit ProgressWidget(QWidget *parent);
    ~ProgressWidget();

    void showMessage(const QString &text);

private:
    void updateWidget(PROGRESS_STATE status, bool active, const QString &text);

private slots:
    void on_openPushButton_clicked();

private:
    Ui::ProgressWidget *ui;
};
