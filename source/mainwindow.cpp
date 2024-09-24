#include "mainwindow.h"

#include <QApplication>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMimeType>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QStringList>
#include <QTextStream>
#include <QTime>
#include <QUndoCommand>
#include <QUndoStack>

#include "config.h"
#include "d1cel.h"
#include "d1cl2.h"
#include "d1hro.h"
#include "ui_mainwindow.h"

#if (defined (_WIN32) || defined (_WIN64))
#include "Shlobj.h"
#endif

#include "dungeon/all.h"

static MainWindow *theMainWindow;

MainWindow::MainWindow()
    : QMainWindow(nullptr)
    , ui(new Ui::MainWindow())
{
    // QCoreApplication::setAttribute( Qt::AA_EnableHighDpiScaling, true );

    this->lastFilePath = Config::getLastFilePath();

    ui->setupUi(this);

    theMainWindow = this;

    this->setWindowTitle(D1_HERO_PLANNER_TITLE);

    // initialize the progress widget
    this->ui->statusBar->insertWidget(0, &this->progressWidget);

    // Initialize 'Undo/Redo' of 'Edit
    this->undoStack = new QUndoStack(this);
    this->undoAction = undoStack->createUndoAction(this, "Undo");
    this->undoAction->setShortcut(QKeySequence::Undo);
    this->redoAction = undoStack->createRedoAction(this, "Redo");
    this->redoAction->setShortcut(QKeySequence::Redo);
    QAction *firstEditAction = this->ui->menuEdit->actions()[0];
    this->ui->menuEdit->insertAction(firstEditAction, this->undoAction);
    this->ui->menuEdit->insertAction(firstEditAction, this->redoAction);

    this->on_actionClose_triggered();

    setAcceptDrops(true);

    // initialize the translators
    this->reloadConfig();
}

MainWindow::~MainWindow()
{
    // close modal windows
    this->on_actionClose_triggered();
    // store last path
    Config::setLastFilePath(this->lastFilePath);
    // cleanup memory
    delete ui;

    delete this->undoAction;
    delete this->redoAction;
    delete this->undoStack;

    delete this->openAsDialog;
    delete this->saveAsDialog;
    delete this->settingsDialog;
}

MainWindow &dMainWindow()
{
    return *theMainWindow;
}

void MainWindow::updatePalette(const D1Pal* pal)
{
    const QString &path = pal->getFilePath();
    if (this->pals.contains(path)) {
        this->setPal(path);
    }
}

void MainWindow::setPal(const QString &path)
{
    D1Pal *pal = this->pals[path];
    this->pal = pal;
    this->trnUnique->setPalette(this->pal);
    this->trnUnique->refreshResultingPalette();
    this->trnBase->refreshResultingPalette();
    // update entities
    // update the widgets
    // - views
    if (this->heroView != nullptr) {
        this->heroView->setPal(pal);
    }
    // - palWidget
    this->palWidget->updatePathComboBoxOptions(this->pals.keys(), path);
    this->palWidget->setPal(pal);
}

void MainWindow::setUniqueTrn(const QString &path)
{
    this->trnUnique = this->uniqueTrns[path];
    this->trnUnique->setPalette(this->pal);
    this->trnUnique->refreshResultingPalette();
    this->trnBase->setPalette(this->trnUnique->getResultingPalette());
    this->trnBase->refreshResultingPalette();

    // update trnUniqueWidget
    this->trnUniqueWidget->updatePathComboBoxOptions(this->uniqueTrns.keys(), path);
    this->trnUniqueWidget->setTrn(this->trnUnique);
}

void MainWindow::setBaseTrn(const QString &path)
{
    this->trnBase = this->baseTrns[path];
    this->trnBase->setPalette(this->trnUnique->getResultingPalette());
    this->trnBase->refreshResultingPalette();

    D1Pal *resPal = this->trnBase->getResultingPalette();
    // update entities
    this->hero->setPalette(resPal);
}

void MainWindow::updateWindow()
{
    // refresh the palette-colors - triggered by displayFrame
    // if (this->palWidget != nullptr) {
    //     this->palWidget->refresh();
    // }
    // update menu options

    // update the view
    this->sideView->displayFrame();
    if (this->heroView != nullptr) {
        // this->heroView->updateFields();
        this->heroView->displayFrame();
    }
}

bool MainWindow::loadPal(const QString &path)
{
    D1Pal *newPal = new D1Pal();
    if (!newPal->load(path)) {
        delete newPal;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading PAL file."));
        return false;
    }
    // replace entry in the pals map
    if (this->pals.contains(path)) {
        if (this->pal == this->pals[path]) {
            this->pal = newPal; // -> setPal must be called!
        }
        delete this->pals[path];
    }
    this->pals[path] = newPal;
    // add path in palWidget
    this->palWidget->updatePathComboBoxOptions(this->pals.keys(), this->pal->getFilePath());
    return true;
}

bool MainWindow::loadUniqueTrn(const QString &path)
{
    D1Trn *newTrn = new D1Trn();
    if (!newTrn->load(path, this->pal)) {
        delete newTrn;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading TRN file."));
        return false;
    }
    // replace entry in the uniqueTrns map
    if (this->uniqueTrns.contains(path)) {
        if (this->trnUnique == this->uniqueTrns[path]) {
            this->trnUnique = newTrn; // -> setUniqueTrn must be called!
        }
        delete this->uniqueTrns[path];
    }
    this->uniqueTrns[path] = newTrn;
    // add path in trnUniqueWidget
    this->trnUniqueWidget->updatePathComboBoxOptions(this->uniqueTrns.keys(), this->trnUnique->getFilePath());
    return true;
}

bool MainWindow::loadBaseTrn(const QString &path)
{
    D1Trn *newTrn = new D1Trn();
    if (!newTrn->load(path, this->trnUnique->getResultingPalette())) {
        delete newTrn;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading TRN file."));
        return false;
    }
    // replace entry in the baseTrns map
    if (this->baseTrns.contains(path)) {
        if (this->trnBase == this->baseTrns[path]) {
            this->trnBase = newTrn; // -> setBaseTrn must be called!
        }
        delete this->baseTrns[path];
    }
    this->baseTrns[path] = newTrn;
    // add path in trnBaseWidget
    this->trnBaseWidget->updatePathComboBoxOptions(this->baseTrns.keys(), this->trnBase->getFilePath());
    return true;
}

void MainWindow::pointHovered(const QPoint &pos)
{
    QString msg;
    if (pos.x() == UINT_MAX) {
    } else {
        if (pos.y() == UINT_MAX) {
            msg = QString(":%1:").arg(pos.x());
        } else {
            msg = QString("%1:%2").arg(pos.x()).arg(pos.y());
        }
    }
    this->progressWidget.showMessage(msg);
}

void MainWindow::colorModified()
{
    // update the view
    if (this->heroView != nullptr) {
        this->heroView->displayFrame();
    }
}

void MainWindow::reloadConfig()
{
    // update locale
    QString lang = Config::getLocale();
    if (lang != this->currLang) {
        QLocale locale = QLocale(lang);
        QLocale::setDefault(locale);
        // remove the old translator
        qApp->removeTranslator(&this->translator);
        // load the new translator
        // QString path = QApplication::applicationDirPath() + "/lang_" + lang + ".qm";
        QString path = ":/lang_" + lang + ".qm";
        if (this->translator.load(path)) {
            qApp->installTranslator(&this->translator);
        }
        this->currLang = lang;
    }
    // reload palettes
    bool currPalChanged = false;
    for (auto iter = this->pals.cbegin(); iter != this->pals.cend(); ++iter) {
        bool change = iter.value()->reloadConfig();
        if (change && iter.value() == this->pal) {
            currPalChanged = true;
        }
    }
    // refresh the palette widgets and the view
    if (currPalChanged && this->palWidget != nullptr) {
        this->palWidget->modify();
    }
    // reload asset-dependent data
    InitLighting();
    InitCursorGFX(this->pal);
    InitInv(this->pal);
}

void MainWindow::heroChanged(D1Hero *hero)
{
    this->hero = hero;
    this->updateWindow();
}

void MainWindow::heroSkillsClicked()
{
    this->sideView->showHeroSkills(this->hero);
}

void MainWindow::heroMonstersClicked()
{
    this->sideView->showMonsters(this->hero);
}

void MainWindow::heroPvPClicked()
{
    this->sideView->showPvP(this->hero);
}

void MainWindow::heroItemClicked(int ii)
{
    this->sideView->showHeroItem(this->hero, ii);
}

void MainWindow::selectHeroItem(int ii)
{
    if (this->itemSelectorDialog == nullptr) {
        this->itemSelectorDialog = new ItemSelectorDialog(this);
    }
    this->itemSelectorDialog->initialize(this->hero, ii);
    this->itemSelectorDialog->show();
}

void MainWindow::addHeroItem(int ii, ItemStruct *is)
{
    if (this->hero->addInvItem(ii, is)) {
        if (this->itemSelectorDialog != nullptr) {
            this->itemSelectorDialog->initialize(this->hero, ii);
        }
        this->updateWindow();
    }
}

void MainWindow::paletteWidget_callback(PaletteWidget *widget, PWIDGET_CALLBACK_TYPE type)
{
    if (widget == this->palWidget) {
        switch (type) {
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_NEW:
            this->on_actionNew_PAL_triggered();
            break;
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_OPEN:
            this->on_actionOpen_PAL_triggered();
            break;
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_SAVE:
            this->on_actionSave_PAL_triggered();
            break;
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_SAVEAS:
            this->on_actionSave_PAL_as_triggered();
            break;
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_CLOSE:
            this->on_actionClose_PAL_triggered();
            break;
        }
    } else if (widget == this->trnUniqueWidget) {
        switch (type) {
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_NEW:
            this->on_actionNew_Translation_Unique_triggered();
            break;
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_OPEN:
            this->on_actionOpen_Translation_Unique_triggered();
            break;
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_SAVE:
            this->on_actionSave_Translation_Unique_triggered();
            break;
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_SAVEAS:
            this->on_actionSave_Translation_Unique_as_triggered();
            break;
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_CLOSE:
            this->on_actionClose_Translation_Unique_triggered();
            break;
        }
    } else if (widget == this->trnBaseWidget) {
        switch (type) {
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_NEW:
            this->on_actionNew_Translation_Base_triggered();
            break;
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_OPEN:
            this->on_actionOpen_Translation_Base_triggered();
            break;
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_SAVE:
            this->on_actionSave_Translation_Base_triggered();
            break;
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_SAVEAS:
            this->on_actionSave_Translation_Base_as_triggered();
            break;
        case PWIDGET_CALLBACK_TYPE::PWIDGET_CALLBACK_CLOSE:
            this->on_actionClose_Translation_Base_triggered();
            break;
        }
    }
}

void MainWindow::initPaletteCycle()
{
    for (int i = 0; i < 32; i++)
        this->origCyclePalette[i] = this->pal->getColor(i);
}

void MainWindow::resetPaletteCycle()
{
    for (int i = 0; i < 32; i++)
        this->pal->setColor(i, this->origCyclePalette[i]);

    this->palWidget->modify();
}

void MainWindow::nextPaletteCycle(D1PAL_CYCLE_TYPE type)
{
    this->pal->cycleColors(type);
    this->palWidget->modify();
}

static QString prepareFilePath(QString filePath, const QString &filter, QString &selectedFilter)
{
    // filter file-name unless it matches the filter
    QFileInfo fi(filePath);
    if (fi.isDir()) {
        return filePath;
    }
    QStringList filterList = filter.split(";;", Qt::SkipEmptyParts);
    for (const QString &filterBase : filterList) {
        int firstIndex = filterBase.lastIndexOf('(') + 1;
        int lastIndex = filterBase.lastIndexOf(')') - 1;
        QString extPatterns = filterBase.mid(firstIndex, lastIndex - firstIndex + 1);
        QStringList extPatternList = extPatterns.split(QRegularExpression(" "), Qt::SkipEmptyParts);
        for (QString &extPattern : extPatternList) {
            // convert filter to regular expression
            for (int n = 0; n < extPattern.size(); n++) {
                if (extPattern[n] == '*') {
                    // convert * to .*
                    extPattern.insert(n, '.');
                    n++;
                } else if (extPattern[n] == '.') {
                    // convert . to \.
                    extPattern.insert(n, '\\');
                    n++;
                }
            }
            QRegularExpression re(QRegularExpression::anchoredPattern(extPattern));
            QRegularExpressionMatch qmatch = re.match(filePath);
            if (qmatch.hasMatch()) {
                selectedFilter = filterBase;
                return filePath;
            }
        }
    }
    // !match -> cut the file-name
    filePath = fi.absolutePath();
    return filePath;
}

QString MainWindow::fileDialog(FILE_DIALOG_MODE mode, const QString &title, const QString &filter)
{
    QString selectedFilter;
    QString filePath = prepareFilePath(this->lastFilePath, filter, selectedFilter);

    if (mode == FILE_DIALOG_MODE::OPEN) {
        filePath = QFileDialog::getOpenFileName(this, title, filePath, filter, &selectedFilter);
    } else {
        filePath = QFileDialog::getSaveFileName(this, title, filePath, filter, &selectedFilter, mode == FILE_DIALOG_MODE::SAVE_NO_CONF ? QFileDialog::DontConfirmOverwrite : QFileDialog::Options());
    }

    if (!filePath.isEmpty()) {
        this->lastFilePath = filePath;
    }
    return filePath;
}

QStringList MainWindow::filesDialog(const QString &title, const QString &filter)
{
    QString selectedFilter;
    QString filePath = prepareFilePath(this->lastFilePath, filter, selectedFilter);

    QStringList filePaths = QFileDialog::getOpenFileNames(this, title, filePath, filter, &selectedFilter);

    if (!filePaths.isEmpty()) {
        this->lastFilePath = filePaths[0];
    }
    return filePaths;
}

QString MainWindow::folderDialog(const QString &title)
{
    QFileInfo fi(this->lastFilePath);
    QString dirPath = fi.absolutePath();

    dirPath = QFileDialog::getExistingDirectory(this, title, dirPath);

    if (!dirPath.isEmpty()) {
        this->lastFilePath = dirPath;
    }
    return dirPath;
}

bool MainWindow::hasHeroUrl(const QMimeData *mimeData)
{
    for (const QUrl &url : mimeData->urls()) {
        QString filePath = url.toLocalFile();
        if (filePath.toLower().endsWith(".hro")) {
            return true;
        }
    }
    return false;
}

bool MainWindow::isResourcePath(const QString &path)
{
    return path.startsWith(':');
}

void MainWindow::on_actionNew_Warrior_triggered()
{
    this->openNew(OPEN_HERO_CLASS::WARRIOR);
}

void MainWindow::on_actionNew_Rogue_triggered()
{
    this->openNew(OPEN_HERO_CLASS::ROGUE);
}

void MainWindow::on_actionNew_Sorcerer_triggered()
{
    this->openNew(OPEN_HERO_CLASS::SORCERER);
}

void MainWindow::on_actionNew_Monk_triggered()
{
    this->openNew(OPEN_HERO_CLASS::MONK);
}

void MainWindow::on_actionNew_Bard_triggered()
{
    this->openNew(OPEN_HERO_CLASS::BARD);
}

void MainWindow::on_actionNew_Barbarian_triggered()
{
    this->openNew(OPEN_HERO_CLASS::BARBARIAN);
}

void MainWindow::on_actionToggle_View_triggered()
{
}

void MainWindow::openNew(OPEN_HERO_CLASS heroClass)
{
    OpenAsParam params = OpenAsParam();
    params.heroType = OPEN_HERO_TYPE::AUTODETECT;
    params.heroClass = heroClass;
    this->openFile(params);
}

void MainWindow::on_actionOpen_triggered()
{
    QString openFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Hero"), tr("HRO Files (*.hro *.HRO)"));

    if (!openFilePath.isEmpty()) {
        QStringList filePaths;
        filePaths.append(openFilePath);
        this->openFiles(filePaths);
    }
}

void MainWindow::on_actionLoad_triggered()
{
    QString title;
    QString filter;
    // assert(this->heroView != nullptr);
    title = tr("Load Hero");
    filter = tr("HRO Files (*.hro *.HRO)");

    QString filePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, title, filter);
    if (filePath.isEmpty()) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_NONE); // PAF_UPDATE_WINDOW

    QString fileLower = filePath.toLower();
    OpenAsParam params = OpenAsParam();
    params.filePath = filePath;

        D1Hero *hero = D1Hero::instance();
        hero->setPalette(this->trnBase->getResultingPalette());

        if (hero->load(filePath, params)) {
            delete this->hero;
            // this->hero = hero;
            this->heroChanged(hero);
        } else {
            delete hero;
            dProgressFail() << tr("Failed loading HRO file: %1.").arg(QDir::toNativeSeparators(filePath));
        }


    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    this->dragMoveEvent(event);
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (hasHeroUrl(event->mimeData())) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    QStringList filePaths;
    for (const QUrl &url : event->mimeData()->urls()) {
        filePaths.append(url.toLocalFile());
    }
    this->openFiles(filePaths);
}

void MainWindow::openArgFile(const char *arg)
{
    QStringList filePaths;
    QString filePath = arg;
    if (filePath.isEmpty()) {
        return;
    }
    this->lastFilePath = filePath;
    filePaths.append(filePath);
    this->openFiles(filePaths);
}

void MainWindow::openFiles(const QStringList &filePaths)
{
    for (const QString &filePath : filePaths) {
        OpenAsParam params = OpenAsParam();
        params.filePath = filePath;
        this->openFile(params);
    }
}

static bool keyCombinationMatchesSequence(int kc, const QKeySequence &ks)
{
    for (int i = 0; i < ks.count(); i++) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (ks[i].toCombined() == kc) {
            return true;
        }
#else
        if (ks[i] == kc) {
            return true;
        }
#endif
    }
    return false;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    const int kc = event->key() | event->modifiers();
    if (keyCombinationMatchesSequence(kc, QKeySequence::New)) { // event->matches(QKeySequence::New)) {
        this->ui->mainMenu->setActiveAction(this->ui->mainMenu->actions()[0]);
        this->ui->menuFile->setActiveAction(this->ui->menuFile->actions()[0]);
        this->ui->menuNew->setActiveAction(this->ui->menuNew->actions()[0]);
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
        // (re)translate undoAction, redoAction
        this->undoAction->setText(tr("Undo"));
        this->redoAction->setText(tr("Redo"));
    }
    QMainWindow::changeEvent(event);
}

static void closeFileContent(LoadFileContent *result)
{
    result->fileType = FILE_CONTENT::UNKNOWN;

    MemFree(result->trnUnique);
    MemFree(result->trnBase);
    MemFree(result->hero);

    qDeleteAll(result->pals);
    result->pals.clear();
}

void MainWindow::failWithError(MainWindow *instance, LoadFileContent *result, const QString &error)
{
    dProgressFail() << error;

    if (instance != nullptr) {
        instance->on_actionClose_triggered();
    }

    closeFileContent(result);

    // Clear loading message from status bar
    ProgressDialog::done();
}

static void findFirstFile(const QString &dir, const QString &filter, QString &filePath, QString &baseName)
{
    if (filePath.isEmpty()) {
        if (!baseName.isEmpty()) {
            QDirIterator it(dir, QStringList(baseName + filter), QDir::Files | QDir::Readable);
            if (it.hasNext()) {
                filePath = it.next();
                return;
            }
        }
        QDirIterator it(dir, QStringList(filter), QDir::Files | QDir::Readable);
        if (it.hasNext()) {
            filePath = it.next();
            if (baseName.isEmpty()) {
                QFileInfo fileInfo = QFileInfo(filePath);
                baseName = fileInfo.completeBaseName();
            }
        }
    }
}

void MainWindow::loadFile(const OpenAsParam &params, MainWindow *instance, LoadFileContent *result)
{
    QString filePath = params.filePath;

    // Check file extension
    FILE_CONTENT fileType = FILE_CONTENT::EMPTY;
    if (!filePath.isEmpty()) {
        QString fileLower = filePath.toLower();
        if (fileLower.endsWith(".hro"))
            fileType = FILE_CONTENT::HRO;
        else
            fileType = FILE_CONTENT::UNKNOWN;
    }
    result->fileType = fileType;
    if (fileType == FILE_CONTENT::UNKNOWN)
        return;

    if (instance != nullptr) {
        instance->on_actionClose_triggered();
    }
    // result->pal = nullptr;
    // result->trnUnique = nullptr;
    // result->trnBase = nullptr;
    result->hero = nullptr;

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_NONE); // PAF_UPDATE_WINDOW

    // Loading default.pal
    D1Pal *newPal = new D1Pal();
    newPal->load(D1Pal::DEFAULT_PATH);
    result->pals[D1Pal::DEFAULT_PATH] = newPal;
    result->pal = newPal;

    // Loading default null.trn
    D1Trn *newUniqueTrn = new D1Trn();
    newUniqueTrn->load(D1Trn::IDENTITY_PATH, newPal);
    result->trnUnique = newUniqueTrn;
    D1Trn *newBaseTrn = new D1Trn();
    newBaseTrn->load(D1Trn::IDENTITY_PATH, newUniqueTrn->getResultingPalette());
    result->trnBase = newBaseTrn;

    QString baseDir;
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo = QFileInfo(filePath);

        baseDir = fileInfo.absolutePath();
    }

    result->baseDir = baseDir;

    result->hero = D1Hero::instance();
    // assert(result->hero != nullptr);
    result->hero->setPalette(result->trnBase->getResultingPalette());
    if (fileType == FILE_CONTENT::HRO) {
        if (!result->hero->load(filePath, params)) {
            MainWindow::failWithError(instance, result, tr("Failed loading HRO file: %1.").arg(QDir::toNativeSeparators(filePath)));
            return;
        }
    } else {
        // filePath.isEmpty()
        result->hero->create(params.heroClass == OPEN_HERO_CLASS::AUTODETECT ? 0 : ((int)params.heroClass - 1));
    }
}

void MainWindow::openFile(const OpenAsParam &params)
{
    LoadFileContent fileContent;
    MainWindow::loadFile(params, this, &fileContent);
    if (fileContent.fileType == FILE_CONTENT::UNKNOWN)
        return;

#if (defined (_WIN32) || defined (_WIN64))
    // SHAddToRecentDocs(SHARD_PATHA, params.filePath.toLatin1().constData());
    SHAddToRecentDocs(SHARD_PATHA, params.filePath.toStdWString().data());
#endif
    const FILE_CONTENT fileType = fileContent.fileType;
    const QString &baseDir = fileContent.baseDir;
    this->pal = fileContent.pal;
    this->trnUnique = fileContent.trnUnique;
    this->trnBase = fileContent.trnBase;
    this->hero = fileContent.hero;

    this->pals = fileContent.pals;

    this->uniqueTrns[D1Trn::IDENTITY_PATH] = this->trnUnique;
    this->baseTrns[D1Trn::IDENTITY_PATH] = this->trnBase;

    // Add palette widgets for PAL and TRNs
    this->palWidget = new PaletteWidget(this, this->undoStack, tr("Palette"));
    this->trnUniqueWidget = new PaletteWidget(this, this->undoStack, tr("Unique translation"));
    this->trnBaseWidget = new PaletteWidget(this, this->undoStack, tr("Base Translation"));
    QLayout *palLayout = this->ui->palFrameWidget->layout();
    palLayout->addWidget(this->palWidget);
    palLayout->addWidget(this->trnUniqueWidget);
    palLayout->addWidget(this->trnBaseWidget);

    // initialize context
    IsHellfireGame = this->hero->isHellfire();
    gnDifficulty = this->hero->getRank();

    QWidget *view;
        // build a HeroView
        this->heroView = new HeroView(this);
        this->heroView->initialize(this->pal, this->hero, this->bottomPanelHidden);

        // Refresh palette widgets when frame is changed
        // QObject::connect(this->heroView, &HeroView::frameRefreshed, this->palWidget, &PaletteWidget::refresh);

        // Refresh palette widgets when the palette is changed (loading a PCX file)
        // QObject::connect(this->heroView, &HeroView::palModified, this->palWidget, &PaletteWidget::refresh);

        view = this->heroView;

    // Add the view to the main frame
    this->ui->mainFrameLayout->addWidget(view);

    // Initialize the side panel
    this->sideView = new SidePanelWidget(this);
    this->ui->sideFrameLayout->addWidget(this->sideView);

    // Initialize palette widgets
    this->palWidget->initialize(this->pal);
    this->trnUniqueWidget->initialize(this->trnUnique);
    this->trnBaseWidget->initialize(this->trnBase);

    // setup default options in the palette widgets
    // this->palWidget->updatePathComboBoxOptions(this->pals.keys(), this->pal->getFilePath());
    this->trnUniqueWidget->updatePathComboBoxOptions(this->uniqueTrns.keys(), this->trnUnique->getFilePath());
    this->trnBaseWidget->updatePathComboBoxOptions(this->baseTrns.keys(), this->trnBase->getFilePath());

    // Palette and translation file selection
    // When a .pal or .trn file is selected in the PaletteWidget update the pal or trn
    QObject::connect(this->palWidget, &PaletteWidget::pathSelected, this, &MainWindow::setPal);
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::pathSelected, this, &MainWindow::setUniqueTrn);
    QObject::connect(this->trnBaseWidget, &PaletteWidget::pathSelected, this, &MainWindow::setBaseTrn);

    // Refresh PAL/TRN view chain
    QObject::connect(this->palWidget, &PaletteWidget::refreshed, this->trnUniqueWidget, &PaletteWidget::refresh);
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::refreshed, this->trnBaseWidget, &PaletteWidget::refresh);

    // Translation color selection
    QObject::connect(this->palWidget, &PaletteWidget::colorsPicked, this->trnUniqueWidget, &PaletteWidget::checkTranslationsSelection);
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::colorsPicked, this->trnBaseWidget, &PaletteWidget::checkTranslationsSelection);
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::colorPicking_started, this->palWidget, &PaletteWidget::startTrnColorPicking);     // start color picking
    QObject::connect(this->trnBaseWidget, &PaletteWidget::colorPicking_started, this->trnUniqueWidget, &PaletteWidget::startTrnColorPicking); // start color picking
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::colorPicking_stopped, this->palWidget, &PaletteWidget::stopTrnColorPicking);      // finish or cancel color picking
    QObject::connect(this->trnBaseWidget, &PaletteWidget::colorPicking_stopped, this->trnUniqueWidget, &PaletteWidget::stopTrnColorPicking);  // finish or cancel color picking
    QObject::connect(this->palWidget, &PaletteWidget::colorPicking_stopped, this->trnUniqueWidget, &PaletteWidget::stopTrnColorPicking);      // cancel color picking
    QObject::connect(this->palWidget, &PaletteWidget::colorPicking_stopped, this->trnBaseWidget, &PaletteWidget::stopTrnColorPicking);        // cancel color picking
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::colorPicking_stopped, this->trnBaseWidget, &PaletteWidget::stopTrnColorPicking);  // cancel color picking

    // Look for all palettes in the same folder as the CEL/CL2 file
    QString firstPaletteFound;
    if (!baseDir.isEmpty()) {
        QDirIterator it(baseDir, QStringList("*.pal"), QDir::Files | QDir::Readable);
        while (it.hasNext()) {
            QString sPath = it.next();

            if (this->loadPal(sPath) && firstPaletteFound.isEmpty()) {
                firstPaletteFound = sPath;
            }
        }
    }
    if (firstPaletteFound.isEmpty()) {
        firstPaletteFound = D1Pal::DEFAULT_PATH;
    }
    this->setPal(firstPaletteFound); // should trigger view->displayFrame()

    // update available menu entries
    this->ui->menuEdit->setEnabled(true);
    this->ui->menuView->setEnabled(true);
    this->ui->menuColors->setEnabled(true);
    this->ui->actionDiff->setEnabled(true);
    this->ui->actionLoad->setEnabled(true);
    this->ui->actionSave->setEnabled(true);
    this->ui->actionSaveAs->setEnabled(true);
    this->ui->actionClose->setEnabled(true);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::openPalFiles(const QStringList &filePaths, PaletteWidget *widget)
{
    QString firstFound;

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Reading..."), 0, PAF_NONE);

    if (widget == this->palWidget) {
        for (const QString &path : filePaths) {
            if (this->loadPal(path) && firstFound.isEmpty()) {
                firstFound = path;
            }
        }
        if (!firstFound.isEmpty()) {
            this->setPal(firstFound);
        }
    } else if (widget == this->trnUniqueWidget) {
        for (const QString &path : filePaths) {
            if (this->loadUniqueTrn(path) && firstFound.isEmpty()) {
                firstFound = path;
            }
        }
        if (!firstFound.isEmpty()) {
            this->setUniqueTrn(firstFound);
        }
    } else if (widget == this->trnBaseWidget) {
        for (const QString &path : filePaths) {
            if (this->loadBaseTrn(path) && firstFound.isEmpty()) {
                firstFound = path;
            }
        }
        if (!firstFound.isEmpty()) {
            this->setBaseTrn(firstFound);
        }
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::saveFile(const SaveAsParam &params)
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Saving..."), 0, PAF_UPDATE_WINDOW);

    QString filePath = params.filePath.isEmpty() ? this->hero->getFilePath() : params.filePath;
    if (!filePath.isEmpty()) {
        QString fileLower = filePath.toLower();
            if (fileLower.endsWith(".hro")) {
                this->hero->save(params);
            } else {
                // Clear loading message from status bar
                ProgressDialog::done();
                QMessageBox::critical(this, tr("Error"), tr("Not supported."));
                return;
            }
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::supportedImageFormats(QStringList &allSupportedImageFormats)
{
    // get supported image file types
    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = QImageReader::supportedMimeTypes();
    for (const QByteArray &mimeTypeName : supportedMimeTypes) {
        mimeTypeFilters.append(mimeTypeName);
    }

    // compose filter for all supported types
    QMimeDatabase mimeDB;
    for (const QString &mimeTypeFilter : mimeTypeFilters) {
        QMimeType mimeType = mimeDB.mimeTypeForName(mimeTypeFilter);
        if (mimeType.isValid()) {
            QStringList mimePatterns = mimeType.globPatterns();
            for (int i = 0; i < mimePatterns.count(); i++) {
                allSupportedImageFormats.append(mimePatterns[i]);
                allSupportedImageFormats.append(mimePatterns[i].toUpper());
            }
        }
    }
}

static QString imageNameFilter()
{
    // get supported image file types
    QStringList allSupportedFormats;
    MainWindow::supportedImageFormats(allSupportedFormats);
    // add PCX support
    allSupportedFormats.append("*.pcx");
    allSupportedFormats.append("*.PCX");

    QString allSupportedFormatsFilter = QApplication::tr("Image files (%1)").arg(allSupportedFormats.join(' '));
    return allSupportedFormatsFilter;
}

void MainWindow::on_actionOpenAs_triggered()
{
    if (this->openAsDialog == nullptr) {
        this->openAsDialog = new OpenAsDialog(this);
    }
    this->openAsDialog->initialize();
    this->openAsDialog->show();
}

void MainWindow::on_actionSave_triggered()
{
    if (this->hero->getFilePath().isEmpty()) {
        this->on_actionSaveAs_triggered();
        return;
    }
    SaveAsParam params = SaveAsParam();
    this->saveFile(params);
}

void MainWindow::on_actionSaveAs_triggered()
{
    if (this->saveAsDialog == nullptr) {
        this->saveAsDialog = new SaveAsDialog(this);
    }
    this->saveAsDialog->initialize(this->hero);
    this->saveAsDialog->show();
}

void MainWindow::on_actionClose_triggered()
{
    this->undoStack->clear();

    MemFree(this->sideView);
    MemFree(this->heroView);
    MemFree(this->palWidget);
    MemFree(this->trnUniqueWidget);
    MemFree(this->trnBaseWidget);
    MemFree(this->hero);

    qDeleteAll(this->pals);
    this->pals.clear();

    qDeleteAll(this->uniqueTrns);
    this->uniqueTrns.clear();

    qDeleteAll(this->baseTrns);
    this->baseTrns.clear();

    // update available menu entries
    this->ui->menuEdit->setEnabled(false);
    this->ui->menuView->setEnabled(false);
    this->ui->menuColors->setEnabled(false);
    this->ui->actionDiff->setEnabled(false);
    this->ui->actionLoad->setEnabled(false);
    this->ui->actionSave->setEnabled(false);
    this->ui->actionSaveAs->setEnabled(false);
    this->ui->actionClose->setEnabled(false);
}

void MainWindow::on_actionSettings_triggered()
{
    if (this->settingsDialog == nullptr) {
        this->settingsDialog = new SettingsDialog(this);
    }
    this->settingsDialog->initialize();
    this->settingsDialog->show();
}

QString MainWindow::FileContentTypeTxt(FILE_CONTENT fileType)
{
    QString result;
    switch (fileType) {
    case FILE_CONTENT::EMPTY: result = QApplication::tr("empty"); break;
    case FILE_CONTENT::CEL:   result = "CEL"; break;
    case FILE_CONTENT::CL2:   result = "CL2"; break;
    case FILE_CONTENT::PCX:   result = "PCX"; break;
    case FILE_CONTENT::TBL:   result = "TBL"; break;
    case FILE_CONTENT::CPP:   result = "CPP"; break;
    case FILE_CONTENT::SMK:   result = "SMK"; break;
    case FILE_CONTENT::DUN:   result = "DUN"; break;
    default: result = "???"; break;
    }
    return result;
}

void MainWindow::on_actionDiff_triggered()
{
    QString openFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Hero"), "HRO Files (*.hro *.HRO)");
    if (openFilePath.isEmpty()) {
        return;
    }

    OpenAsParam params = OpenAsParam();
    params.filePath = openFilePath;

    LoadFileContent fileContent;
    MainWindow::loadFile(params, nullptr, &fileContent);
    if (fileContent.fileType == FILE_CONTENT::UNKNOWN)
        return;

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Comparing..."), 0, PAF_OPEN_DIALOG);

    {
        QString header;
        switch (fileContent.fileType) {
        case FILE_CONTENT::EMPTY: dProgressErr() << tr("File is empty."); break;
        case FILE_CONTENT::HRO: this->hero->compareTo(fileContent.hero, header); break;
        default: dProgressErr() << tr("Not supported."); break;
        }
    }

    closeFileContent(&fileContent);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionQuit_triggered()
{
    qApp->quit();
}

void MainWindow::on_actionTogglePalTrn_triggered()
{
    this->ui->palFrameWidget->setVisible(this->ui->palFrameWidget->isHidden());
}

void MainWindow::on_actionToggleBottomPanel_triggered()
{
    this->bottomPanelHidden = !this->bottomPanelHidden;
    if (this->heroView != nullptr) {
        this->heroView->toggleBottomPanel();
    }
}

void MainWindow::on_actionNew_PAL_triggered()
{
    QString palFilePath = this->fileDialog(FILE_DIALOG_MODE::SAVE_CONF, tr("New Palette File"), tr("PAL Files (*.pal *.PAL)"));

    if (palFilePath.isEmpty()) {
        return;
    }

    QFileInfo palFileInfo(palFilePath);
    const QString &path = palFilePath; // palFileInfo.absoluteFilePath();
    const QString name = palFileInfo.fileName();

    D1Pal *newPal = new D1Pal();
    if (!newPal->load(D1Pal::DEFAULT_PATH)) {
        delete newPal;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading PAL file."));
        return;
    }
    newPal->setFilePath(path);
    // replace entry in the pals map
    if (this->pals.contains(path))
        delete this->pals[path];
    this->pals[path] = newPal;
    // add path in palWidget
    // this->palWidget->updatePathComboBoxOptions(this->pals.keys(), path);
    // select the new palette
    this->setPal(path);
}

void MainWindow::on_actionOpen_PAL_triggered()
{
    QStringList palFilePaths = this->filesDialog(tr("Select Palette Files"), tr("PAL Files (*.pal *.PAL)"));

    this->openPalFiles(palFilePaths, this->palWidget);
}

void MainWindow::on_actionSave_PAL_triggered()
{
    QString filePath = this->pal->getFilePath();
    if (MainWindow::isResourcePath(filePath)) {
        this->on_actionSave_PAL_as_triggered();
    } else {
        this->pal->save(filePath);
    }
}

void MainWindow::on_actionSave_PAL_as_triggered()
{
    QString palFilePath = this->fileDialog(FILE_DIALOG_MODE::SAVE_CONF, tr("Save Palette File as..."), tr("PAL Files (*.pal *.PAL)"));

    if (palFilePath.isEmpty()) {
        return;
    }

    if (!this->pal->save(palFilePath)) {
        return;
    }
    if (!this->loadPal(palFilePath)) {
        return;
    }
    // select the 'new' palette
    this->setPal(palFilePath); // path
}

void MainWindow::on_actionClose_PAL_triggered()
{
    QString filePath = this->pal->getFilePath();
    if (MainWindow::isResourcePath(filePath)) {
        this->pal->load(filePath);
        this->setPal(filePath);
        return;
    }
    // remove entry from the pals map
    D1Pal *pal = this->pals.take(filePath);
    MemFree(pal);
    // remove path in palWidget
    // this->palWidget->updatePathComboBoxOptions(this->pals.keys(), D1Pal::DEFAULT_PATH);
    // select the default palette
    this->setPal(D1Pal::DEFAULT_PATH);
}

void MainWindow::on_actionNew_Translation_Unique_triggered()
{
    QString trnFilePath = this->fileDialog(FILE_DIALOG_MODE::SAVE_CONF, tr("New Translation File"), tr("TRN Files (*.trn *.TRN)"));

    if (trnFilePath.isEmpty()) {
        return;
    }

    QFileInfo trnFileInfo(trnFilePath);
    const QString &path = trnFilePath; // trnFileInfo.absoluteFilePath();
    const QString name = trnFileInfo.fileName();

    D1Trn *newTrn = new D1Trn();
    if (!newTrn->load(D1Trn::IDENTITY_PATH, this->pal)) {
        delete newTrn;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading TRN file."));
        return;
    }
    newTrn->setFilePath(path);
    // replace entry in the uniqueTrns map
    if (this->uniqueTrns.contains(path))
        delete this->uniqueTrns[path];
    this->uniqueTrns[path] = newTrn;
    // add path in trnUniqueWidget
    // this->trnUniqueWidget->updatePathComboBoxOptions(this->pals.keys(), path);
    // select the new trn file
    this->setUniqueTrn(path);
}

void MainWindow::on_actionOpen_Translation_Unique_triggered()
{
    QStringList trnFilePaths = this->filesDialog(tr("Select Unique Translation Files"), tr("TRN Files (*.trn *.TRN)"));

    this->openPalFiles(trnFilePaths, this->trnUniqueWidget);
}

void MainWindow::on_actionSave_Translation_Unique_triggered()
{
    QString filePath = this->trnUnique->getFilePath();
    if (MainWindow::isResourcePath(filePath)) {
        this->on_actionSave_Translation_Unique_as_triggered();
    } else {
        this->trnUnique->save(filePath);
    }
}

void MainWindow::on_actionSave_Translation_Unique_as_triggered()
{
    QString trnFilePath = this->fileDialog(FILE_DIALOG_MODE::SAVE_CONF, tr("Save Translation File as..."), tr("TRN Files (*.trn *.TRN)"));

    if (trnFilePath.isEmpty()) {
        return;
    }

    if (!this->trnUnique->save(trnFilePath)) {
        return;
    }
    if (!this->loadUniqueTrn(trnFilePath)) {
        return;
    }
    // select the 'new' trn file
    this->setUniqueTrn(trnFilePath); // path
}

void MainWindow::on_actionClose_Translation_Unique_triggered()
{
    QString filePath = this->trnUnique->getFilePath();
    if (MainWindow::isResourcePath(filePath)) {
        this->trnUnique->load(filePath, this->pal);
        this->setUniqueTrn(filePath);
        return;
    }
    // remove entry from the uniqueTrns map
    D1Trn *trn = this->uniqueTrns.take(filePath);
    MemFree(trn);
    // remove path in trnUniqueWidget
    // this->trnUniqueWidget->updatePathComboBoxOptions(this->uniqueTrns.keys(), D1Trn::IDENTITY_PATH);
    // select the default trn
    this->setUniqueTrn(D1Trn::IDENTITY_PATH);
}

void MainWindow::on_actionPatch_Translation_Unique_triggered()
{
    this->trnUniqueWidget->patchTrn();
}

void MainWindow::on_actionNew_Translation_Base_triggered()
{
    QString trnFilePath = this->fileDialog(FILE_DIALOG_MODE::SAVE_CONF, tr("New Translation File"), tr("TRN Files (*.trn *.TRN)"));

    if (trnFilePath.isEmpty()) {
        return;
    }

    QFileInfo trnFileInfo(trnFilePath);
    const QString &path = trnFilePath; // trnFileInfo.absoluteFilePath();
    const QString name = trnFileInfo.fileName();

    D1Trn *newTrn = new D1Trn();
    if (!newTrn->load(D1Trn::IDENTITY_PATH, this->trnUnique->getResultingPalette())) {
        delete newTrn;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading TRN file."));
        return;
    }
    newTrn->setFilePath(path);
    // replace entry in the baseTrns map
    if (this->baseTrns.contains(path))
        delete this->baseTrns[path];
    this->baseTrns[path] = newTrn;
    // add path in trnBaseWidget
    // this->trnBaseWidget->updatePathComboBoxOptions(this->baseTrns.keys(), path);
    // select the 'new' trn file
    this->setBaseTrn(path);
}

void MainWindow::on_actionOpen_Translation_Base_triggered()
{
    QStringList trnFilePaths = this->filesDialog(tr("Select Base Translation Files"), tr("TRN Files (*.trn *.TRN)"));

    this->openPalFiles(trnFilePaths, this->trnBaseWidget);
}

void MainWindow::on_actionSave_Translation_Base_triggered()
{
    QString filePath = this->trnBase->getFilePath();
    if (MainWindow::isResourcePath(filePath)) {
        this->on_actionSave_Translation_Base_as_triggered();
    } else {
        this->trnBase->save(filePath);
    }
}

void MainWindow::on_actionSave_Translation_Base_as_triggered()
{
    QString trnFilePath = this->fileDialog(FILE_DIALOG_MODE::SAVE_CONF, tr("Save Translation File as..."), tr("TRN Files (*.trn *.TRN)"));

    if (trnFilePath.isEmpty()) {
        return;
    }

    if (!this->trnBase->save(trnFilePath)) {
        return;
    }
    if (!this->loadBaseTrn(trnFilePath)) {
        return;
    }
    // select the 'new' trn file
    this->setBaseTrn(trnFilePath); // path
}

void MainWindow::on_actionClose_Translation_Base_triggered()
{
    QString filePath = this->trnBase->getFilePath();
    if (MainWindow::isResourcePath(filePath)) {
        this->trnBase->load(filePath, this->trnUnique->getResultingPalette());
        this->setBaseTrn(filePath);
        return;
    }
    // remove entry from the baseTrns map
    D1Trn *trn = this->baseTrns.take(filePath);
    MemFree(trn);
    // remove path in trnBaseWidget
    // this->trnBaseWidget->updatePathComboBoxOptions(this->baseTrns.keys(), D1Trn::IDENTITY_PATH);
    // select the default trn
    this->setBaseTrn(D1Trn::IDENTITY_PATH);
}

void MainWindow::on_actionPatch_Translation_Base_triggered()
{
    this->trnBaseWidget->patchTrn();
}

void MainWindow::updateTrns(const std::vector<D1Trn *> &newTrns)
{
    // reset unique translations
    this->on_actionClose_Translation_Unique_triggered();
    for (auto it = this->uniqueTrns.begin(); it != this->uniqueTrns.end(); ) {
        if (MainWindow::isResourcePath(it.key())) {
            it++;
            continue;
        }
        MemFree(it.value());
        it = this->uniqueTrns.erase(it);
    }
    // reset base translations
    this->on_actionClose_Translation_Base_triggered();
    for (auto it = this->baseTrns.begin(); it != this->baseTrns.end(); ) {
        if (MainWindow::isResourcePath(it.key())) {
            it++;
            continue;
        }
        MemFree(it.value());
        it = this->baseTrns.erase(it);
    }
    if (newTrns.empty()) {
        return;
    }
    // load the TRN files
    for (D1Trn *trn : newTrns) {
        // trn->setPalette(this->pal);
        // trn->refreshResultingPalette();
        this->uniqueTrns[trn->getFilePath()] = trn;
    }
    this->setUniqueTrn(newTrns[0]->getFilePath());
}

#if defined(Q_OS_WIN)
#define OS_TYPE "Windows"
#elif defined(Q_OS_QNX)
#define OS_TYPE "qnx"
#elif defined(Q_OS_ANDROID)
#define OS_TYPE "android"
#elif defined(Q_OS_IOS)
#define OS_TYPE "iOS"
#elif defined(Q_OS_TVOS)
#define OS_TYPE "tvOS"
#elif defined(Q_OS_WATCHOS)
#define OS_TYPE "watchOS"
#elif defined(Q_OS_MACOS)
#define OS_TYPE "macOS"
#elif defined(Q_OS_DARWIN)
#define OS_TYPE "darwin"
#elif defined(Q_OS_WASM)
#define OS_TYPE "wasm"
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#define OS_TYPE "linux"
#else
#define OS_TYPE QApplication::tr("Unknown")
#endif

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About"), QStringLiteral("%1 %2 (%3) (%4-bit)").arg(D1_HERO_PLANNER_TITLE).arg(D1_HERO_PLANNER_VERSION).arg(OS_TYPE).arg(sizeof(void *) == 8 ? "64" : "32"));
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}
