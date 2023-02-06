#pragma once

#include <QDialog>
#include <QFrame>
#include <QProgressBar>

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

class ProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);
    ~ProgressDialog();

    static void openDialog();
    static void start(PROGRESS_DIALOG_STATE mode, const QString &label, int numBars, int flags);
    static void done();

    static void incBar(const QString &label, int maxValue);
    static void decBar();

    static bool wasCanceled();
    static bool incValue();

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
    static void addResult_impl(PROGRESS_TEXT_MODE textMode, const QString &line, bool replace);

signals:
    void updateWindow();

private:
    Ui::ProgressDialog *ui;

    QList<QProgressBar *> progressBars;
    int activeBars;
    PROGRESS_STATE status = PROGRESS_STATE::DONE;
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
    explicit ProgressWidget(QWidget *parent = nullptr);
    ~ProgressWidget();

private:
    void updateWidget(PROGRESS_STATE status, bool active, const QString &text);

private slots:
    void on_openPushButton_clicked();

private:
    Ui::ProgressWidget *ui;
};
