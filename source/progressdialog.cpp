#include "progressdialog.h"

#include <queue>

#include <QFontMetrics>
#include <QMessageBox>
#include <QScrollBar>
#include <QStyle>
#include <QTextBlock>

#include "ui_progressdialog.h"
#include "ui_progresswidget.h"

static ProgressDialog *theDialog;
static ProgressWidget *theWidget;

typedef enum task_message_type {
    TMSG_PROGRESS,
    TMSG_INCBAR,
    TMSG_DECBAR,
    TMSG_LOGMSG,
} task_message_type;
typedef struct TaskMessage {
    task_message_type type;
    union {
        int value;
        struct {
            PROGRESS_TEXT_MODE textMode;
            bool replace;
        };
    };
    QString text;
} TaskMessage;

// task-properties - shared
static QMutex sharedMutex;
static std::queue<TaskMessage> sharedQueue;

// task-properties - MAIN
// static QFutureWatcher<void> *mainWatcher;
static ProgressThread *mainWatcher = nullptr;

// task-properties - THREAD
// static QPromise<void> *taskPromise; // the promise object of the task
static DPromise *taskPromise = nullptr; // the promise object of the task - TODO: use map with QThread::currentThreadId(); ?
static int taskProgress;                // progression in the task's thread
static int taskTextVersion;
static QString taskTextLastLine;
static PROGRESS_TEXT_MODE taskTextMode = PROGRESS_TEXT_MODE::NORMAL;
static bool taskErrorOnFail;

// THREAD
bool DPromise::isCanceled()
{
    return this->status >= PROGRESS_STATE::CANCEL;
}

// MAIN (!)
void DPromise::cancel()
{
    if (this->status < PROGRESS_STATE::CANCEL)
        this->status = PROGRESS_STATE::CANCEL;
}

// THREAD
void DPromise::setProgressValue(int value)
{
    (void)value;

    emit this->progressValueChanged();
}

// MAIN
ProgressThread::ProgressThread(std::function<void()> &&cf)
    : QThread()
    , callFunc(cf)
{
}

// THREAD
void ProgressThread::run()
{
    DPromise *promise = new DPromise();

    // connect(this, &ProgressThread::cancelTask, promise, &DPromise::cancel);

    taskProgress = 0;
    // taskTextMode = PROGRESS_TEXT_MODE::NORMAL;
    taskErrorOnFail = false;
    taskPromise = promise;
    connect(taskPromise, &DPromise::progressValueChanged, this, &ProgressThread::reportResults, Qt::QueuedConnection);

    this->callFunc();

    delete taskPromise;
    taskPromise = nullptr;
}

// MAIN (!)
void ProgressThread::cancel()
{
    // emit this->cancelTask();
    taskPromise->cancel();
}

// MAIN
void ProgressThread::reportResults()
{
    emit this->resultReady();
}

// THREAD
static void sendMsg(TaskMessage &msg)
{
    sharedMutex.lock();

    sharedQueue.push(msg);

    sharedMutex.unlock();

    taskProgress++;
    taskPromise->setProgressValue(taskProgress);
}

// MAIN
void ProgressDialog::consumeMessages()
{
    while (true) {
        sharedMutex.lock();

        if (sharedQueue.empty()) {
            sharedMutex.unlock();
            break;
        }

        TaskMessage msg = sharedQueue.front();
        sharedQueue.pop();

        sharedMutex.unlock();

        switch (msg.type) {
        case TMSG_PROGRESS:
            ProgressDialog::incValue();
            break;
        case TMSG_INCBAR:
            ProgressDialog::incBar(msg.text, msg.value);
            break;
        case TMSG_DECBAR:
            ProgressDialog::decBar();
            break;
        case TMSG_LOGMSG:
            /*switch (msg.textMode) {
            theDialog->status
            }*/
            theDialog->appendLine(msg.textMode, msg.text, msg.replace);
            break;
        }
    }
}

ProgressDialog::ProgressDialog(QWidget *parent)
    : QDialog(parent, Qt::WindowStaysOnTopHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint)
    , ui(new Ui::ProgressDialog())
{
    this->ui->setupUi(this);

    theDialog = this;
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::openDialog()
{
    // theWidget->updateWidget(theDialog->status, false, "");
    // theWidget->setModality(false);

    theDialog->showNormal();
    theDialog->adjustSize();
}

void ProgressDialog::start(PROGRESS_DIALOG_STATE mode, const QString &label, int numBars)
{
    bool background = mode == PROGRESS_DIALOG_STATE::BACKGROUND;

    taskTextVersion = 0;
    taskTextLastLine.clear();
    // taskTextMode = PROGRESS_TEXT_MODE::NORMAL;

    theDialog->setWindowTitle(label);
    theDialog->ui->outputTextEdit->document()->clear();
    theDialog->activeBars = 0;
    theDialog->status = PROGRESS_STATE::RUNNING;
    theDialog->ui->progressLabel->setVisible(!background);
    theDialog->ui->progressLabel->setText("");
    theDialog->ui->detailsGroupBox->setVisible(background);
    theDialog->ui->detailsPushButton->setText(background ? tr("Hide details") : tr("Show details"));
    theDialog->ui->cancelPushButton->setEnabled(true);
    theDialog->ui->progressButtonsWidget_1->setVisible(!background);
    theDialog->ui->progressButtonsWidget_2->setVisible(false);
    for (int i = 0; i < numBars; i++) {
        QProgressBar *progressBar = new QProgressBar();
        progressBar->setEnabled(false);
        progressBar->setTextVisible(false);
        theDialog->ui->verticalLayout->insertWidget(1 + i, progressBar);
        theDialog->progressBars.append(progressBar);
    }
    theDialog->adjustSize();
    theDialog->setWindowModality(background ? Qt::NonModal : Qt::ApplicationModal);

    theWidget->updateWidget(theDialog->status, background, label);
    if (background) {
        // theWidget->updateWidget(theDialog->status, true, label);
        // theWidget->setModality(true);
        // theWidget->setFocus();
        theDialog->showMinimized();
        return;
    }
    // theWidget->updateWidget(theDialog->status, false, "");

    theDialog->showNormal();
}

void ProgressDialog::done(bool forceOpen)
{
    theDialog->setWindowTitle(" ");
    theDialog->ui->progressLabel->setVisible(false);
    for (QProgressBar *progressBar : theDialog->progressBars) {
        theDialog->ui->verticalLayout->removeWidget(progressBar);
    }
    qDeleteAll(theDialog->progressBars);
    theDialog->progressBars.clear();
    // theDialog->activeBars = 0;
    theDialog->ui->progressButtonsWidget_1->setVisible(false);
    bool detailsOpen = theDialog->ui->detailsGroupBox->isVisible();
    theDialog->ui->detailsGroupBox->setVisible(true);
    theDialog->ui->progressButtonsWidget_2->setVisible(true);
    if (theDialog->status == PROGRESS_STATE::RUNNING) {
        theDialog->status = PROGRESS_STATE::DONE;
    } else if (theDialog->status == PROGRESS_STATE::CANCEL) {
        // dProgress() << QApplication::tr("Process cancelled.");
        theDialog->appendLine(PROGRESS_TEXT_MODE::WARNING, QApplication::tr("Process cancelled."), false);
    }
    if (theDialog->status != PROGRESS_STATE::FAIL && (!detailsOpen || !theDialog->isVisible() || theDialog->isMinimized()) && !forceOpen) {
        theDialog->hide();
    } else {
        theDialog->showNormal();
        theDialog->adjustSize();
        theDialog->ui->closePushButton->setFocus();
    }

    theWidget->updateWidget(theDialog->status, !theDialog->ui->outputTextEdit->document()->isEmpty(), "");
}

/*void ProgressDialog::setupAsync(QFuture<void> &&future, bool forceOpen = false)
{
    mainWatcher = new QFutureWatcher<void>();
    QObject::connect(mainWatcher, &QFutureWatcher<void>::progressValueChanged, [](int progress) {
        ProgressDialog::consumeMessages();
    });
    QObject::connect(mainWatcher, &QFutureWatcher<void>::finished, [forceOpen]() {
        ProgressDialog::done(forceOpen);
        mainWatcher->deleteLater();
        mainWatcher = nullptr;
    });
    mainWatcher->setFuture(future);
}

void ProgressDialog::setupThread(QPromise<void> *promise)
{
    taskPromise = promise;
    taskProgress = 0;
}*/
void ProgressDialog::startAsync(PROGRESS_DIALOG_STATE mode, const QString &label, int numBars, std::function<void()> &&callFunc, bool forceOpen)
{
    theDialog->forceOpen = forceOpen;
    ProgressDialog::start(mode, label, numBars);

    mainWatcher = new ProgressThread(std::move(callFunc));

    QObject::connect(mainWatcher, &ProgressThread::resultReady, theDialog, &ProgressDialog::on_message_ready);
    QObject::connect(mainWatcher, &ProgressThread::finished, theDialog, &ProgressDialog::on_task_finished); // runs in the context of the MAIN...
    QObject::connect(mainWatcher, &ProgressThread::finished, []() {
        // runs in the context of the THREAD
        mainWatcher->deleteLater();
        mainWatcher = nullptr;
    });

    mainWatcher->start();
}

// MAIN
void ProgressDialog::on_message_ready()
{
    this->consumeMessages();
}

// MAIN
void ProgressDialog::on_task_finished()
{
    this->done(this->forceOpen);
}

// THREAD
bool ProgressDialog::progressCanceled()
{
    return taskPromise->isCanceled();
}

// THREAD
void ProgressDialog::incProgressBar(const QString &label, int maxValue)
{
    TaskMessage msg;
    msg.type = TMSG_INCBAR;
    msg.text = label;
    msg.value = maxValue;
    sendMsg(msg);

    taskErrorOnFail = true;
}

// THREAD
void ProgressDialog::decProgressBar()
{
    TaskMessage msg;
    msg.type = TMSG_DECBAR;
    sendMsg(msg);

    taskErrorOnFail = false;
}

// THREAD
bool ProgressDialog::incProgress()
{
    TaskMessage msg;
    msg.type = TMSG_PROGRESS;
    sendMsg(msg);
    return !taskPromise->isCanceled();
}

// MAIN
void ProgressDialog::incBar(const QString &label, int maxValue)
{
    if (!label.isEmpty()) {
        theDialog->ui->progressLabel->setText(label);
    }
    QProgressBar *newProgressBar = theDialog->progressBars[theDialog->activeBars];

    theDialog->activeBars++;
    newProgressBar->setRange(0, maxValue);
    newProgressBar->setValue(0);
    newProgressBar->setToolTip("0%");
    newProgressBar->setEnabled(true);

    theDialog->update();
}

// MAIN
void ProgressDialog::decBar()
{
    theDialog->activeBars--;

    QProgressBar *oldProgressBar = theDialog->progressBars[theDialog->activeBars];

    oldProgressBar->setEnabled(false);
}

// MAIN
bool ProgressDialog::wasCanceled()
{
    QCoreApplication::processEvents();

    return theDialog->status >= PROGRESS_STATE::CANCEL;
}

// MAIN
bool ProgressDialog::incBarValue(int index, int amount)
{
    QProgressBar *progressBar = this->progressBars[index];

    amount += progressBar->value();
    progressBar->setValue(amount);
    progressBar->setToolTip(QString("%1%").arg(amount * 100 / progressBar->maximum()));
    return !ProgressDialog::wasCanceled();
}

// MAIN
bool ProgressDialog::incValue()
{
    return theDialog->incBarValue(theDialog->activeBars - 1, 1);
}

bool ProgressDialog::incMainValue(int amount)
{
    return theDialog->incBarValue(0, amount);
}

ProgressDialog &dProgress()
{
    taskTextMode = PROGRESS_TEXT_MODE::NORMAL;
    return *theDialog;
}

ProgressDialog &dProgressProc()
{
    taskTextMode = PROGRESS_TEXT_MODE::PROC;
    return *theDialog;
}

ProgressDialog &dProgressWarn()
{
    if (theDialog->status < PROGRESS_STATE::WARN) {
        theDialog->status = PROGRESS_STATE::WARN;
    }
    taskTextMode = PROGRESS_TEXT_MODE::WARNING;
    return *theDialog;
}

ProgressDialog &dProgressErr()
{
    if (theDialog->status < PROGRESS_STATE::ERROR) {
        theDialog->status = PROGRESS_STATE::ERROR;
    }
    taskTextMode = PROGRESS_TEXT_MODE::ERROR;
    return *theDialog;
}

ProgressDialog &dProgressFail()
{
    /*if (taskErrorOnFail) {
        return dProgressErr();
    }*/
    if (theDialog->status < PROGRESS_STATE::FAIL) {
        theDialog->status = PROGRESS_STATE::FAIL;
    }
    taskTextMode = PROGRESS_TEXT_MODE::ERROR;
    return *theDialog;
}

void ProgressDialog::appendLine(PROGRESS_TEXT_MODE mode, const QString &line, bool replace)
{
    QPlainTextEdit *textEdit = this->ui->outputTextEdit;
    QTextCursor cursor = textEdit->textCursor();
    int scrollValue = textEdit->verticalScrollBar()->value();
    // The user has selected text or scrolled away from the bottom: maintain position.
    const bool active = cursor.hasSelection() || scrollValue != textEdit->verticalScrollBar()->maximum();

    // remove last line if replacing the text
    if (replace) {
        this->removeLastLine();
    }
    // Append the text at the end of the document.
    if (mode == PROGRESS_TEXT_MODE::NORMAL) {
        // using appendHtml instead of appendPlainText because Qt can not handle mixed appends...
        // (the new block inherits the properties of previous/selected block which might be colored due to the code below)
        textEdit->appendHtml(line);
    } else { // Using <pre> tag to allow multiple spaces
        QString htmlText = QString("<p style=\"color:%1;white-space:pre\">%2</p>").arg(mode == PROGRESS_TEXT_MODE::ERROR ? "red" : "orange").arg(line);
        textEdit->appendHtml(htmlText);
    }

    if (!active) {
        // The user hasn't selected any text and the scrollbar is at the bottom: scroll to the bottom.
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        scrollValue = textEdit->verticalScrollBar()->maximum();
    }
    textEdit->setTextCursor(cursor);
    textEdit->verticalScrollBar()->setValue(scrollValue);
}

ProgressDialog &ProgressDialog::operator<<(const QString &text)
{
    if (taskTextMode == PROGRESS_TEXT_MODE::PROC) {
        TaskMessage msg;
        msg.type = TMSG_LOGMSG;
        msg.text = text;
        msg.textMode = PROGRESS_TEXT_MODE::NORMAL;
        msg.replace = false;
        sendMsg(msg);
    } else {
        this->appendLine(taskTextMode, text, false);
    }
    taskTextLastLine = text;
    taskTextVersion++;
    return *this;
}

void ProgressDialog::removeLastLine()
{
    QTextCursor cursor = QTextCursor(this->ui->outputTextEdit->document()->lastBlock());
    cursor.select(QTextCursor::BlockUnderCursor);
    cursor.removeSelectedText();
}

ProgressDialog &ProgressDialog::operator<<(const QPair<QString, QString> &text)
{
    this->appendLine(taskTextMode, text.second, taskTextLastLine == text.first);
    taskTextVersion++;
    taskTextLastLine = text.second;
    return *this;
}

ProgressDialog &ProgressDialog::operator<<(QPair<int, QString> &idxText)
{
    this->appendLine(taskTextMode, idxText.second, taskTextVersion == idxText.first);
    taskTextVersion++;
    taskTextLastLine = idxText.second;
    idxText.first = taskTextVersion;
    return *this;
}

void ProgressDialog::on_detailsPushButton_clicked()
{
    bool currVisible = this->ui->detailsGroupBox->isVisible();
    this->ui->detailsGroupBox->setVisible(!currVisible);
    this->ui->detailsPushButton->setText(currVisible ? tr("Show details") : tr("Hide details"));
    this->adjustSize();
}

void ProgressDialog::on_cancelPushButton_clicked()
{
    if (this->status < PROGRESS_STATE::CANCEL) {
        this->status = PROGRESS_STATE::CANCEL;
    }
    this->ui->cancelPushButton->setEnabled(false);
    if (mainWatcher != nullptr) {
        mainWatcher->cancel();
        // wait for the task to finish
        /*QDialog *blocker = new QDialog(this->parentWidget(), Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        blocker->show();
        QTimer *timer = new QTimer();
        QObject::connect(timer, &QTimer::timeout, [this, timer, blocker]() {
            if (mainWatcher == nullptr) {
                timer->deleteLater();
                blocker->deleteLater();
            }
        });
        timer->start(200);*/
        /*while (mainWatcher != nullptr) {
            QThread::msleep(200);
        }*/
    }
}

void ProgressDialog::on_closePushButton_clicked()
{
    this->hide();
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
    this->on_cancelPushButton_clicked();
    // QDialog::closeEvent(event);
}

void ProgressDialog::changeEvent(QEvent *event)
{
    /*if (event->type() == QEvent::WindowStateChange && this->isMinimized()) {
        theWidget->updateWidget(theDialog->status, true, this->windowTitle());
        theWidget->setModality(true);
        theWidget->setFocus();

        this->hide();
    }*/
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    return QDialog::changeEvent(event);
}

ProgressWidget::ProgressWidget(QWidget *parent)
    : QDialog(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
    , ui(new Ui::ProgressWidget())
{
    this->ui->setupUi(this);
    int fontHeight = this->fontMetrics().height() + 2;
    this->setMinimumHeight(fontHeight);
    this->setMaximumHeight(fontHeight);
    this->ui->openPushButton->setMinimumSize(fontHeight, fontHeight);
    this->ui->openPushButton->setMaximumSize(fontHeight, fontHeight);
    this->ui->openPushButton->setIconSize(QSize(fontHeight, fontHeight));
    this->updateWidget(PROGRESS_STATE::DONE, false, "");

    theWidget = this;
}

ProgressWidget::~ProgressWidget()
{
    delete ui;
}

/*void ProgressWidget::setModality(bool modal)
{
    this->hide();
    this->setWindowModality(modal ? Qt::ApplicationModal : Qt::NonModal);
    this->show();
}*/

void ProgressWidget::updateWidget(PROGRESS_STATE status, bool active, const QString &label)
{
    this->ui->openPushButton->setEnabled(active);
    QStyle::StandardPixmap type;
    switch (status) {
    case PROGRESS_STATE::DONE:
        type = active ? QStyle::SP_ArrowUp : QStyle::SP_DialogApplyButton;
        break;
    case PROGRESS_STATE::RUNNING:
        type = QStyle::SP_BrowserReload;
        break;
    case PROGRESS_STATE::WARN:
        type = QStyle::SP_MessageBoxWarning;
        break;
    case PROGRESS_STATE::ERROR:
        type = QStyle::SP_MessageBoxCritical;
        break;
    case PROGRESS_STATE::CANCEL:
        type = QStyle::SP_TitleBarCloseButton; // QStyle::SP_DialogCancelButton;
        break;
    case PROGRESS_STATE::FAIL:
        type = QStyle::SP_BrowserStop;
        break;
    }
    this->ui->openPushButton->setIcon(this->style()->standardIcon(type));
    this->ui->messageLabel->setText(label);
    this->adjustSize();
    // this->repaint();
    this->update();
    // QFrame::update();
}

void ProgressWidget::on_openPushButton_clicked()
{
    ProgressDialog::openDialog();
}
