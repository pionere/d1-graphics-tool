#include "progressdialog.h"

#include <queue>

#include <QFontMetrics>
#include <QMessageBox>
#include <QScrollBar>
#include <QStyle>
#include <QTextBlock>

#include "mainwindow.h"
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
static bool taskAsync;

// task-properties - THREAD
// static QPromise<void> *taskPromise; // the promise object of the task
static DPromise *taskPromise = nullptr; // the promise object of the task - TODO: use map with QThread::currentThreadId(); ?
static int taskProgress;                // progression in the task's thread
static int taskTextVersion;
static QString taskTextLastLine;
static PROGRESS_TEXT_MODE taskTextMode;
static int taskErrorOnFail;

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
/*void DPromise::finish()
{
    emit this->finished();
}*/

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
    // taskErrorOnFail = 0;
    taskPromise = promise;
    connect(taskPromise, &DPromise::progressValueChanged, this, &ProgressThread::reportResults, Qt::QueuedConnection);
    // connect(taskPromise, &DPromise::finished, this, &ProgressThread::reportReady, Qt::QueuedConnection);

    this->callFunc();

    // taskPromise->finish();

    // this->sleep(20);
    // delete taskPromise;
    taskPromise->deleteLater();
    taskPromise = nullptr;
}

// MAIN (!)
void ProgressThread::cancel()
{
    // emit this->cancelTask();
    if (taskPromise != nullptr) // !this->isFinished()
        taskPromise->cancel();
}

// MAIN
void ProgressThread::reportResults()
{
    emit this->resultReady();
}

// MAIN
/*void ProgressThread::reportReady()
{
    emit this->taskReady();
}*/

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
            ProgressDialog::incValue_impl(msg.value);
            break;
        case TMSG_INCBAR:
            ProgressDialog::incBar_impl(msg.text, msg.value);
            break;
        case TMSG_DECBAR:
            ProgressDialog::decBar_impl();
            break;
        case TMSG_LOGMSG:
            ProgressDialog::addResult_impl(msg.textMode, msg.text, msg.replace);
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

    // disable context menu (of the scroll bars)
    this->ui->outputTextEdit->horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    this->ui->outputTextEdit->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::openDialog()
{
    theDialog->showNormal();
    theDialog->adjustSize();
}

bool ProgressDialog::isRunning()
{
    return theDialog->running;
}

void ProgressDialog::start(PROGRESS_DIALOG_STATE mode, const QString &label, int numBars, int flags)
{
    bool background = mode == PROGRESS_DIALOG_STATE::BACKGROUND;

    taskAsync = false;
    taskTextVersion = 0;
    taskErrorOnFail = 0;
    taskTextLastLine.clear();
    // taskTextMode = PROGRESS_TEXT_MODE::NORMAL;
    theDialog->running = true;
    theDialog->afterFlags = flags;
    theDialog->setWindowTitle(label);
    theDialog->ui->outputTextEdit->clear();
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

    theWidget->updateWidget(theDialog->status, background, label);
    if (background) {
        // theWidget->updateWidget(theDialog->status, true, label);
        theDialog->showMinimized();
        return;
    }
    // theWidget->updateWidget(theDialog->status, false, "");

    theDialog->showNormal();
}

void ProgressDialog::done()
{
    if (theDialog->afterFlags & PAF_UPDATE_WINDOW) {
        dMainWindow().updateWindow();
    }

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
        ProgressDialog::addResult_impl(PROGRESS_TEXT_MODE::NORMAL, QApplication::tr("Process cancelled."), false);
    }
    bool emptyResult = theDialog->ui->outputTextEdit->document()->isEmpty();
    bool openDialog = (theDialog->afterFlags & PAF_OPEN_DIALOG) != 0;
    if (emptyResult
     || (theDialog->status != PROGRESS_STATE::FAIL && (!detailsOpen || !theDialog->isVisible() || theDialog->isMinimized()) && !openDialog)) {
        theDialog->hide();
    } else {
        theDialog->showNormal();
        theDialog->adjustSize();
        theDialog->ui->closePushButton->setFocus();
    }

    theWidget->updateWidget(theDialog->status, !emptyResult, "");
    theDialog->running = false;

    if (emptyResult && openDialog) {
        QMessageBox::information(this, "", tr("Done"));
    }
}

void ProgressDialog::startAsync(PROGRESS_DIALOG_STATE mode, const QString &label, int numBars, int flags, std::function<void()> &&callFunc)
{
    ProgressDialog::start(mode, label, numBars, flags);
    taskAsync = true;

    mainWatcher = new ProgressThread(std::move(callFunc));

    QObject::connect(mainWatcher, &ProgressThread::resultReady, theDialog, &ProgressDialog::on_message_ready);
    // QObject::connect(mainWatcher, &ProgressThread::taskReady, theDialog, &ProgressDialog::on_task_finished); // runs in the context of the MAIN...
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
    ProgressDialog::consumeMessages();
}

// MAIN
void ProgressDialog::on_task_finished()
{
    ProgressDialog::done();
}

void ProgressDialog::incBar(const QString &label, int maxValue)
{
    if (!taskAsync) {
        // MAIN
        ProgressDialog::incBar_impl(label, maxValue);
    } else {
        // THREAD
        TaskMessage msg;
        msg.type = TMSG_INCBAR;
        msg.text = label;
        msg.value = maxValue;
        sendMsg(msg);
    }

    taskErrorOnFail++;
}

// MAIN
void ProgressDialog::incBar_impl(const QString &label, int maxValue)
{
    if (!label.isEmpty()) {
        if (theDialog->ui->progressLabel->isVisible())
            theDialog->ui->progressLabel->setText(label);
        else
            theDialog->setWindowTitle(label);
    }
    if (theDialog->progressBars.count() <= theDialog->activeBars) {
        addResult_impl(PROGRESS_TEXT_MODE::ERROR, QApplication::tr("Too many progression bars."), false);
        return;
    }
    QProgressBar *newProgressBar = theDialog->progressBars[theDialog->activeBars];

    theDialog->activeBars++;
    newProgressBar->setRange(0, maxValue);
    newProgressBar->setValue(0);
    newProgressBar->setToolTip("0%");
    newProgressBar->setEnabled(true);

    theDialog->update();
}

void ProgressDialog::decBar()
{
    if (!taskAsync) {
        // MAIN
        ProgressDialog::decBar_impl();
    } else {
        // THREAD
        TaskMessage msg;
        msg.type = TMSG_DECBAR;
        sendMsg(msg);
    }

    taskErrorOnFail--;
}

// MAIN
void ProgressDialog::decBar_impl()
{
    if (theDialog->activeBars == 0) {
        addResult_impl(PROGRESS_TEXT_MODE::ERROR, QApplication::tr("Mismatching progression bars."), false);
        return;
    }
    theDialog->activeBars--;

    QProgressBar *oldProgressBar = theDialog->progressBars[theDialog->activeBars];

    oldProgressBar->setEnabled(false);
}

bool ProgressDialog::wasCanceled()
{
    if (!taskAsync) {
        // MAIN
        QCoreApplication::processEvents();

        return theDialog->status >= PROGRESS_STATE::CANCEL;
    } else {
        // THREAD
        return taskPromise->isCanceled();
    }
}

bool ProgressDialog::incValue()
{
    return ProgressDialog::addValue(1);
}

bool ProgressDialog::addValue(int amount)
{
    if (!taskAsync) {
        // MAIN
        theDialog->incValue_impl(amount);
        return !ProgressDialog::wasCanceled();
    } else {
        // THREAD
        TaskMessage msg;
        msg.type = TMSG_PROGRESS;
        msg.value = amount;
        sendMsg(msg);
        return !taskPromise->isCanceled();
    }
}

// MAIN
void ProgressDialog::incValue_impl(int amount)
{
    int index = theDialog->activeBars - 1;
    QProgressBar *progressBar = theDialog->progressBars[index];

    amount += progressBar->value();
    progressBar->setValue(amount);
    progressBar->setToolTip(QString("%1%").arg(amount * 100 / progressBar->maximum()));
}

ProgressDialog &dProgress()
{
    taskTextMode = PROGRESS_TEXT_MODE::NORMAL;
    return *theDialog;
}

ProgressDialog &dProgressWarn()
{
    taskTextMode = PROGRESS_TEXT_MODE::WARNING;
    return *theDialog;
}

ProgressDialog &dProgressErr()
{
    taskTextMode = PROGRESS_TEXT_MODE::ERROR;
    return *theDialog;
}

ProgressDialog &dProgressFail()
{
    taskTextMode = taskErrorOnFail ? PROGRESS_TEXT_MODE::ERROR : PROGRESS_TEXT_MODE::FAIL;
    return *theDialog;
}

// MAIN
void ProgressDialog::addResult_impl(PROGRESS_TEXT_MODE mode, const QString &line, bool replace)
{
    QPlainTextEdit *textEdit = theDialog->ui->outputTextEdit;
    QTextCursor cursor = textEdit->textCursor();
    int scrollValue = textEdit->verticalScrollBar()->value();
    // The user has selected text or scrolled away from the bottom: maintain position.
    const bool active = cursor.hasSelection() || scrollValue != textEdit->verticalScrollBar()->maximum();

    // remove last line if replacing the text
    if (replace) {
        // remove last line
        QTextCursor lastCursor = QTextCursor(textEdit->document()->lastBlock());
        lastCursor.select(QTextCursor::BlockUnderCursor);
        lastCursor.removeSelectedText();
    }
    // Append the text at the end of the document.
    if (mode == PROGRESS_TEXT_MODE::NORMAL) {
        // using appendHtml instead of appendPlainText because Qt can not handle mixed appends...
        // (the new block inherits the properties of previous/selected block which might be colored due to the code below)
        textEdit->appendHtml(line);
    } else { // Using <pre> tag to allow multiple spaces
        QString htmlText = QString("<p style=\"color:%1;white-space:pre\">%2</p>").arg(mode == PROGRESS_TEXT_MODE::WARNING ? "orange" : "red").arg(line);
        textEdit->appendHtml(htmlText);
    }
    // update status
    PROGRESS_STATE refStatus;
    switch (mode) {
    case PROGRESS_TEXT_MODE::NORMAL:
        refStatus = PROGRESS_STATE::RUNNING;
        break;
    case PROGRESS_TEXT_MODE::WARNING:
        refStatus = PROGRESS_STATE::WARN;
        break;
    case PROGRESS_TEXT_MODE::ERROR:
        refStatus = PROGRESS_STATE::ERROR;
        break;
    case PROGRESS_TEXT_MODE::FAIL:
        refStatus = PROGRESS_STATE::FAIL;
        break;
    }
    if (theDialog->status < refStatus) {
        theDialog->status = refStatus;
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
    if (!taskAsync) {
        // MAIN
        ProgressDialog::addResult_impl(taskTextMode, text, false);
    } else {
        // THREAD
        TaskMessage msg;
        msg.type = TMSG_LOGMSG;
        msg.text = text;
        msg.textMode = taskTextMode;
        msg.replace = false;
        sendMsg(msg);
    }
    taskTextLastLine = text;
    taskTextVersion++;
    return *this;
}

ProgressDialog &ProgressDialog::operator<<(const QPair<QString, QString> &text)
{
    bool replace = taskTextLastLine == text.first;
    if (!taskAsync) {
        // MAIN
        ProgressDialog::addResult_impl(taskTextMode, text.second, replace);
    } else {
        // THREAD
        TaskMessage msg;
        msg.type = TMSG_LOGMSG;
        msg.text = text.second;
        msg.textMode = taskTextMode;
        msg.replace = replace;
        sendMsg(msg);
    }
    taskTextVersion++;
    taskTextLastLine = text.second;
    return *this;
}

ProgressDialog &ProgressDialog::operator<<(QPair<int, QString> &idxText)
{
    bool replace = taskTextVersion == idxText.first;
    if (!taskAsync) {
        // MAIN
        ProgressDialog::addResult_impl(taskTextMode, idxText.second, replace);
    } else {
        // THREAD
        TaskMessage msg;
        msg.type = TMSG_LOGMSG;
        msg.text = idxText.second;
        msg.textMode = taskTextMode;
        msg.replace = replace;
        sendMsg(msg);
    }
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
        while (mainWatcher != nullptr) {
            QThread::msleep(200);
        }
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

        this->hide();
    }*/
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    return QDialog::changeEvent(event);
}

ProgressWidget::ProgressWidget(QWidget *parent)
    : QFrame(parent)
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

void ProgressWidget::showMessage(const QString &text)
{
    if (!theDialog->isRunning() && this->ui->messageLabel->text() != text) {
        this->ui->messageLabel->setText(text);
        this->adjustSize();
        this->repaint();
    }
}

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
    this->repaint();
    // QFrame::update();
}

void ProgressWidget::on_openPushButton_clicked()
{
    ProgressDialog::openDialog();
}
