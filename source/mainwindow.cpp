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
#include "d1celtileset.h"
#include "d1cl2.h"
#include "d1pcx.h"
#include "ui_mainwindow.h"

static MainWindow *theMainWindow;

MainWindow::MainWindow()
    : QMainWindow(nullptr)
    , ui(new Ui::MainWindow())
{
    // QCoreApplication::setAttribute( Qt::AA_EnableHighDpiScaling, true );

    this->lastFilePath = Config::getLastFilePath();

    ui->setupUi(this);

    theMainWindow = this;

    this->setWindowTitle(D1_GRAPHICS_TOOL_TITLE);

    // initialize the progress widget
    this->ui->statusBar->insertWidget(0, &this->progressWidget);

    // initialize 'new' submenu of 'File'
    this->newMenu.addAction("CEL graphics", this, SLOT(on_actionNew_CEL_triggered()));
    this->newMenu.addAction("CL2 graphics", this, SLOT(on_actionNew_CL2_triggered()));
    this->newMenu.addAction("Tileset", this, SLOT(on_actionNew_Tileset_triggered()));
    QAction *firstFileAction = (QAction *)this->ui->menuFile->actions()[0];
    this->ui->menuFile->insertMenu(firstFileAction, &this->newMenu);

    // Initialize 'Undo/Redo' of 'Edit
    this->undoStack = new QUndoStack(this);
    this->undoAction = undoStack->createUndoAction(this, "Undo");
    this->undoAction->setShortcut(QKeySequence::Undo);
    this->redoAction = undoStack->createRedoAction(this, "Redo");
    this->redoAction->setShortcut(QKeySequence::Redo);
    this->ui->menuEdit->addAction(this->undoAction);
    this->ui->menuEdit->addAction(this->redoAction);
    this->ui->menuEdit->addSeparator();

    // Initialize 'Frame' submenu of 'Edit'
    this->frameMenu.setToolTipsVisible(true);
    this->frameMenu.addAction("Add Layer", this, SLOT(on_actionAddTo_Frame_triggered()));
    this->frameMenu.addAction("Insert", this, SLOT(on_actionInsert_Frame_triggered()));
    this->frameMenu.addAction("Append", this, SLOT(on_actionAppend_Frame_triggered()));
    this->frameMenu.addAction("Replace", this, SLOT(on_actionReplace_Frame_triggered()));
    this->frameMenu.addAction("Delete", this, SLOT(on_actionDel_Frame_triggered()));
    this->ui->menuEdit->addMenu(&this->frameMenu);

    // Initialize 'Subtile' submenu of 'Edit'
    this->subtileMenu.setToolTipsVisible(true);
    this->subtileMenu.addAction("Create", this, SLOT(on_actionCreate_Subtile_triggered()));
    this->subtileMenu.addAction("Insert", this, SLOT(on_actionInsert_Subtile_triggered()));
    this->subtileMenu.addAction("Append", this, SLOT(on_actionAppend_Subtile_triggered()));
    this->subtileMenu.addAction("Replace", this, SLOT(on_actionReplace_Subtile_triggered()));
    this->subtileMenu.addAction("Delete", this, SLOT(on_actionDel_Subtile_triggered()));
    this->ui->menuEdit->addMenu(&this->subtileMenu);

    // Initialize 'Tile' submenu of 'Edit'
    this->tileMenu.setToolTipsVisible(true);
    this->tileMenu.addAction("Create", this, SLOT(on_actionCreate_Tile_triggered()));
    this->tileMenu.addAction("Insert", this, SLOT(on_actionInsert_Tile_triggered()));
    this->tileMenu.addAction("Append", this, SLOT(on_actionAppend_Tile_triggered()));
    this->tileMenu.addAction("Replace", this, SLOT(on_actionReplace_Tile_triggered()));
    this->tileMenu.addAction("Delete", this, SLOT(on_actionDel_Tile_triggered()));
    this->ui->menuEdit->addMenu(&this->tileMenu);

    // Initialize upscale option of 'Edit'
    this->ui->menuEdit->addSeparator();
    this->upscaleAction = this->ui->menuEdit->addAction("Upscale", this, SLOT(on_actionUpscale_triggered()));

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
    delete this->exportDialog;
    delete this->patchTilesetDialog;
    delete this->upscaleDialog;
    delete this->upscaleTaskDialog;
}

MainWindow &dMainWindow()
{
    return *theMainWindow;
}

void MainWindow::changeColor(const std::vector<std::pair<D1GfxPixel, D1GfxPixel>> &replacements, bool all)
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 0, PAF_UPDATE_WINDOW);

    if (all || this->gfx->getFrameCount() == 0) {
        for (int i = 0; i < this->gfx->getFrameCount(); i++) {
            D1GfxFrame *frame = this->gfx->getFrame(i);
            frame->replacePixels(replacements);
        }
    } else {
        int currentFrameIndex = this->celView != nullptr ? this->celView->getCurrentFrameIndex() : this->levelCelView->getCurrentFrameIndex();
        D1GfxFrame *frame = this->gfx->getFrame(currentFrameIndex);
        frame->replacePixels(replacements);
    }
    this->gfx->setModified();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::setPal(const QString &path)
{
    this->pal = this->pals[path];
    this->trnUnique->setPalette(this->pal);
    this->trnUnique->refreshResultingPalette();
    this->trnBase->refreshResultingPalette();
    // update the widgets
    // - views
    if (this->celView != nullptr) {
        this->celView->setPal(this->pal);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->setPal(this->pal);
    }
    // - palWidget
    this->palWidget->updatePathComboBoxOptions(this->pals.keys(), path);
    this->palWidget->setPal(this->pal);
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

    this->gfx->setPalette(this->trnBase->getResultingPalette());
    this->paintDialog->setPalette(this->trnBase->getResultingPalette());

    // update trnBaseWidget
    this->trnBaseWidget->updatePathComboBoxOptions(this->baseTrns.keys(), path);
    this->trnBaseWidget->setTrn(this->trnBase);
}

void MainWindow::updateWindow()
{
    // rebuild palette hits
    if (this->palHits != nullptr) {
        this->palHits->update();
    }
    if (this->palWidget != nullptr) {
        this->palWidget->refresh();
    }
    // update menu options
    bool hasFrame = this->gfx != nullptr && this->gfx->getFrameCount() != 0;
    this->frameMenu.actions()[2]->setEnabled(hasFrame); // replace frame
    this->frameMenu.actions()[3]->setEnabled(hasFrame); // delete frame
    if (this->levelCelView != nullptr) {
        bool hasSubtile = this->tileset->min->getSubtileCount() != 0;
        this->subtileMenu.actions()[3]->setEnabled(hasSubtile); // replace subtile
        this->subtileMenu.actions()[4]->setEnabled(hasSubtile); // delete subtile
        this->tileMenu.actions()[0]->setEnabled(hasSubtile);    // create tile
        bool hasTile = this->tileset->til->getTileCount() != 0;
        this->tileMenu.actions()[3]->setEnabled(hasTile); // replace tile
        this->tileMenu.actions()[4]->setEnabled(hasTile); // delete tile
    }
    // update the view
    if (this->celView != nullptr) {
        // this->celView->update();
        this->celView->displayFrame();
    }
    if (this->levelCelView != nullptr) {
        // this->levelCelView->update();
        this->levelCelView->displayFrame();
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

void MainWindow::frameClicked(D1GfxFrame *frame, const QPoint &pos, unsigned counter)
{
    if (frame == nullptr || pos.x() < 0 || pos.x() >= frame->getWidth() || pos.y() < 0 || pos.y() >= frame->getHeight()) {
        // no target hit -> ignore
        return;
    }
    if (!this->paintDialog->isHidden()) {
        // drawing
        this->paintDialog->frameClicked(frame, pos, counter);
    } else {
        // picking
        const D1GfxPixel pixel = frame->getPixel(pos.x(), pos.y());
        this->paintDialog->selectColor(pixel);
        this->palWidget->selectColor(pixel);
        this->trnUniqueWidget->selectColor(pixel);
        this->trnBaseWidget->selectColor(pixel);
    }
}

void MainWindow::frameModified()
{
    this->gfx->setModified();

    this->updateWindow();
}

void MainWindow::colorModified()
{
    if (this->celView != nullptr) {
        this->celView->displayFrame();
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->displayFrame();
    }
    this->paintDialog->colorModified();
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
        if (iter.value() == this->pal) {
            currPalChanged = true;
        }
    }
    // refresh the palette widgets and the view
    if (currPalChanged) {
        this->palWidget->refresh();
        this->colorModified();
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
        QString extPatterns = filterBase.mid(filterBase.lastIndexOf('(') + 1, filterBase.lastIndexOf(')') - 1);
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

bool MainWindow::hasImageUrl(const QMimeData *mimeData)
{
    const QByteArrayList supportedMimeTypes = QImageReader::supportedMimeTypes();
    QMimeDatabase mimeDB;

    for (const QUrl &url : mimeData->urls()) {
        QString filePath = url.toLocalFile();
        // add PCX support
        if (filePath.toLower().endsWith(".pcx")) {
            return true;
        }
        QMimeType mimeType = mimeDB.mimeTypeForFile(filePath);
        for (const QByteArray &mimeTypeName : supportedMimeTypes) {
            if (mimeType.inherits(mimeTypeName)) {
                return true;
            }
        }
    }
    return false;
}

bool MainWindow::isResourcePath(const QString &path)
{
    return path.startsWith(':');
}

void MainWindow::on_actionNew_CEL_triggered()
{
    OpenAsParam params = OpenAsParam();
    params.isTileset = OPEN_TILESET_TYPE::FALSE;
    params.clipped = OPEN_CLIPPED_TYPE::FALSE;
    this->openFile(params);
}

void MainWindow::on_actionNew_CL2_triggered()
{
    OpenAsParam params = OpenAsParam();
    params.isTileset = OPEN_TILESET_TYPE::FALSE;
    params.clipped = OPEN_CLIPPED_TYPE::TRUE;
    this->openFile(params);
}

void MainWindow::on_actionNew_Tileset_triggered()
{
    OpenAsParam params = OpenAsParam();
    params.isTileset = OPEN_TILESET_TYPE::TRUE;
    this->openFile(params);
}

void MainWindow::on_actionOpen_triggered()
{
    QString openFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Open Graphics"), tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2);;PCX Files (*.pcx *.PCX)"));

    if (!openFilePath.isEmpty()) {
        OpenAsParam params = OpenAsParam();
        params.celFilePath = openFilePath;
        this->openFile(params);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    this->dragMoveEvent(event);
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    bool unloaded = this->celView == nullptr && this->levelCelView == nullptr;

    for (const QUrl &url : event->mimeData()->urls()) {
        QString fileLower = url.toLocalFile().toLower();
        if (fileLower.endsWith(".cel") || fileLower.endsWith(".cl2") || (unloaded && fileLower.endsWith(".pcx"))) {
            event->acceptProposedAction();
            return;
        }
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    OpenAsParam params = OpenAsParam();
    for (const QUrl &url : event->mimeData()->urls()) {
        params.celFilePath = url.toLocalFile();
        this->openFile(params);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (this->paintDialog != nullptr && !this->paintDialog->isHidden()) {
            this->paintDialog->hide();
        }
        return;
    }
    if (event->matches(QKeySequence::Copy)) {
        QImage image;
        if (this->celView != nullptr) {
            image = this->celView->copyCurrent();
        }
        if (this->levelCelView != nullptr) {
            image = this->levelCelView->copyCurrent();
        }
        if (!image.isNull()) {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setImage(image);
        }
        return;
    }
    if (event->matches(QKeySequence::Paste)) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QImage image = clipboard->image();
        if (!image.isNull()) {
            /*ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_UPDATE_WINDOW);

            if (this->celView != nullptr) {
                this->celView->pasteCurrent(image);
            }
            if (this->levelCelView != nullptr) {
                this->levelCelView->pasteCurrent(image);
            }

            // Clear loading message from status bar
            ProgressDialog::done();*/
            std::function<void()> func = [this, image]() {
                if (this->celView != nullptr) {
                    this->celView->pasteCurrent(image);
                }
                if (this->levelCelView != nullptr) {
                    this->levelCelView->pasteCurrent(image);
                }
            };
            ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_UPDATE_WINDOW, std::move(func));
        }
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
        { // (re)translate the 'new' menu
            this->newMenu.setTitle(tr("New"));
            QList<QAction *> menuActions = this->newMenu.actions();
            menuActions[0]->setText(tr("CEL graphics"));
            menuActions[1]->setText(tr("CL2 graphics"));
            menuActions[2]->setText(tr("Tileset"));
        }
        // (re)translate undoAction, redoAction
        this->undoAction->setText(tr("Undo"));
        this->redoAction->setText(tr("Redo"));
        { // (re)translate 'Frame' submenu of 'Edit'
            this->frameMenu.setTitle(tr("Frame"));
            QList<QAction *> menuActions = this->frameMenu.actions();
            menuActions[0]->setText(tr("Add Layer"));
            menuActions[0]->setToolTip(tr("Add the content of an image to the current frame"));
            menuActions[1]->setText(tr("Insert"));
            menuActions[1]->setToolTip(tr("Add new frames before the current one"));
            menuActions[2]->setText(tr("Append"));
            menuActions[2]->setToolTip(tr("Append new frames at the end"));
            menuActions[3]->setText(tr("Replace"));
            menuActions[3]->setToolTip(tr("Replace the current frame"));
            menuActions[4]->setText(tr("Delete"));
            menuActions[4]->setToolTip(tr("Delete the current frame"));
        }
        { // (re)translate 'Subtile' submenu of 'Edit'
            this->subtileMenu.setTitle(tr("Subtile"));
            QList<QAction *> menuActions = this->subtileMenu.actions();
            menuActions[0]->setText(tr("Create"));
            menuActions[0]->setToolTip(tr("Create a new subtile"));
            menuActions[1]->setText(tr("Insert"));
            menuActions[1]->setToolTip(tr("Add new subtiles before the current one"));
            menuActions[2]->setText(tr("Append"));
            menuActions[2]->setToolTip(tr("Append new subtiles at the end"));
            menuActions[3]->setText(tr("Replace"));
            menuActions[3]->setToolTip(tr("Replace the current subtile"));
            menuActions[4]->setText(tr("Delete"));
            menuActions[4]->setToolTip(tr("Delete the current subtile"));
        }
        { // (re)translate 'Tile' submenu of 'Edit'
            this->tileMenu.setTitle(tr("Tile"));
            QList<QAction *> menuActions = this->tileMenu.actions();
            menuActions[0]->setText(tr("Create"));
            menuActions[0]->setToolTip(tr("Create a new tile"));
            menuActions[1]->setText(tr("Insert"));
            menuActions[1]->setToolTip(tr("Add new tiles before the current one"));
            menuActions[2]->setText(tr("Append"));
            menuActions[2]->setToolTip(tr("Append new tiles at the end"));
            menuActions[3]->setText(tr("Replace"));
            menuActions[3]->setToolTip(tr("Replace the current tile"));
            menuActions[4]->setText(tr("Delete"));
            menuActions[4]->setToolTip(tr("Delete the current tile"));
        }
        // (re)translate the upscale option of 'Edit'
        this->upscaleAction->setText(tr("Upscale"));
        this->upscaleAction->setToolTip(tr("Upscale the current graphics"));
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::failWithError(const QString &error)
{
    dProgressFail() << error;

    this->on_actionClose_triggered();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::openFile(const OpenAsParam &params)
{
    QString openFilePath = params.celFilePath;

    // Check file extension
    int fileType = 0;
    if (!openFilePath.isEmpty()) {
        QString fileLower = openFilePath.toLower();
        if (fileLower.endsWith(".cel"))
            fileType = 1;
        else if (fileLower.endsWith(".cl2"))
            fileType = 2;
        else if (fileLower.endsWith(".pcx"))
            fileType = 3;
        else
            return;
    }

    this->on_actionClose_triggered();

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_UPDATE_WINDOW);

    // Loading default.pal
    D1Pal *newPal = new D1Pal();
    newPal->load(D1Pal::DEFAULT_PATH);
    this->pals[D1Pal::DEFAULT_PATH] = newPal;
    this->pal = newPal;

    // Loading default null.trn
    D1Trn *newTrn = new D1Trn();
    newTrn->load(D1Trn::IDENTITY_PATH, this->pal);
    this->uniqueTrns[D1Trn::IDENTITY_PATH] = newTrn;
    this->trnUnique = newTrn;
    newTrn = new D1Trn();
    newTrn->load(D1Trn::IDENTITY_PATH, this->trnUnique->getResultingPalette());
    this->baseTrns[D1Trn::IDENTITY_PATH] = newTrn;
    this->trnBase = newTrn;

    const QFileInfo celFileInfo = QFileInfo(openFilePath);

    // If a SOL, MIN and TIL files exists then build a LevelCelView
    const QString celDir = celFileInfo.absolutePath();
    const QString basePath = celDir + "/" + celFileInfo.completeBaseName();
    QString tilFilePath = params.tilFilePath;
    QString minFilePath = params.minFilePath;
    QString solFilePath = params.solFilePath;
    if (!openFilePath.isEmpty() && tilFilePath.isEmpty()) {
        tilFilePath = basePath + ".til";
    }
    if (!openFilePath.isEmpty() && minFilePath.isEmpty()) {
        minFilePath = basePath + ".min";
    }
    if (!openFilePath.isEmpty() && solFilePath.isEmpty()) {
        solFilePath = basePath + ".sol";
    }

    bool isTileset = params.isTileset == OPEN_TILESET_TYPE::TRUE;
    if (params.isTileset == OPEN_TILESET_TYPE::AUTODETECT) {
        isTileset = fileType == 1 && QFileInfo::exists(tilFilePath) && QFileInfo::exists(minFilePath) && QFileInfo::exists(solFilePath);
    }

    this->gfx = new D1Gfx();
    this->gfx->setPalette(this->trnBase->getResultingPalette());
    if (isTileset) {
        this->tileset = new D1Tileset(this->gfx);
        // Loading SOL
        if (!this->tileset->sol->load(solFilePath)) {
            this->failWithError(tr("Failed loading SOL file: %1.").arg(QDir::toNativeSeparators(solFilePath)));
            return;
        }

        // Loading MIN
        std::map<unsigned, D1CEL_FRAME_TYPE> celFrameTypes;
        if (!this->tileset->min->load(minFilePath, this->gfx, this->tileset->sol, celFrameTypes, params)) {
            this->failWithError(tr("Failed loading MIN file: %1.").arg(QDir::toNativeSeparators(minFilePath)));
            return;
        }

        // Loading TIL
        if (!this->tileset->til->load(tilFilePath, this->tileset->min)) {
            this->failWithError(tr("Failed loading TIL file: %1.").arg(QDir::toNativeSeparators(tilFilePath)));
            return;
        }

        // Loading AMP
        QString ampFilePath = params.ampFilePath;
        if (!openFilePath.isEmpty() && ampFilePath.isEmpty()) {
            ampFilePath = basePath + ".amp";
        }
        if (!this->tileset->amp->load(ampFilePath, this->tileset->til->getTileCount(), params)) {
            this->failWithError(tr("Failed loading AMP file: %1.").arg(QDir::toNativeSeparators(ampFilePath)));
            return;
        }

        // Loading TMI
        QString tmiFilePath = params.tmiFilePath;
        if (!openFilePath.isEmpty() && tmiFilePath.isEmpty()) {
            tmiFilePath = basePath + ".tmi";
        }
        if (!this->tileset->tmi->load(tmiFilePath, this->tileset->sol, params)) {
            this->failWithError(tr("Failed loading TMI file: %1.").arg(QDir::toNativeSeparators(tmiFilePath)));
            return;
        }

        // Loading CEL
        if (!D1CelTileset::load(*this->gfx, celFrameTypes, openFilePath, params)) {
            this->failWithError(tr("Failed loading Tileset-CEL file: %1.").arg(QDir::toNativeSeparators(openFilePath)));
            return;
        }
    } else if (fileType == 1) { // CEL
        if (!D1Cel::load(*this->gfx, openFilePath, params)) {
            this->failWithError(tr("Failed loading CEL file: %1.").arg(QDir::toNativeSeparators(openFilePath)));
            return;
        }
    } else if (fileType == 2) { // CL2
        if (!D1Cl2::load(*this->gfx, openFilePath, params)) {
            this->failWithError(tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(openFilePath)));
            return;
        }
    } else if (fileType == 3) { // PCX
        if (!D1Pcx::load(*this->gfx, this->pal, openFilePath, params)) {
            this->failWithError(tr("Failed loading PCX file: %1.").arg(QDir::toNativeSeparators(openFilePath)));
            return;
        }
    } else {
        // openFilePath.isEmpty()
        this->gfx->setType(params.clipped == OPEN_CLIPPED_TYPE::TRUE ? D1CEL_TYPE::V2_MONO_GROUP : D1CEL_TYPE::V1_REGULAR);
    }

    // Add palette widgets for PAL and TRNs
    this->palWidget = new PaletteWidget(this, this->undoStack, tr("Palette"));
    this->trnUniqueWidget = new PaletteWidget(this, this->undoStack, tr("Unique translation"));
    this->trnBaseWidget = new PaletteWidget(this, this->undoStack, tr("Base Translation"));
    this->ui->palFrame->layout()->addWidget(this->palWidget);
    this->ui->palFrame->layout()->addWidget(this->trnUniqueWidget);
    this->ui->palFrame->layout()->addWidget(this->trnBaseWidget);

    if (isTileset) {
        // build a LevelCelView
        this->levelCelView = new LevelCelView(this);
        this->levelCelView->initialize(this->pal, this->tileset);

        // Refresh palette widgets when frame, subtile of tile is changed
        QObject::connect(this->levelCelView, &LevelCelView::frameRefreshed, this->palWidget, &PaletteWidget::refresh);

        // Refresh palette widgets when the palette is changed (loading a PCX file)
        QObject::connect(this->levelCelView, &LevelCelView::palModified, this->palWidget, &PaletteWidget::refresh);
    } else {
        // build a CelView
        this->celView = new CelView(this);
        this->celView->initialize(this->pal, this->gfx);

        // Refresh palette widgets when frame is changed
        QObject::connect(this->celView, &CelView::frameRefreshed, this->palWidget, &PaletteWidget::refresh);

        // Refresh palette widgets when the palette is changed (loading a PCX file)
        QObject::connect(this->celView, &CelView::palModified, this->palWidget, &PaletteWidget::refresh);
    }
    // Add the (level)CelView to the main frame
    this->ui->mainFrame->layout()->addWidget(isTileset ? (QWidget *)this->levelCelView : this->celView);

    // prepare the paint dialog
    this->paintDialog = new PaintDialog(this, this->undoStack, this->celView, this->levelCelView);
    this->paintDialog->setPalette(this->trnBase->getResultingPalette());

    // Initialize palette widgets
    this->palHits = new D1PalHits(this->gfx, this->tileset);
    this->palWidget->initialize(this->pal, this->celView, this->levelCelView, this->palHits);
    this->trnUniqueWidget->initialize(this->trnUnique, this->celView, this->levelCelView, this->palHits);
    this->trnBaseWidget->initialize(this->trnBase, this->celView, this->levelCelView, this->palHits);

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
    QObject::connect(this->palWidget, &PaletteWidget::colorsSelected, this->trnUniqueWidget, &PaletteWidget::checkTranslationsSelection);
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::colorsSelected, this->trnBaseWidget, &PaletteWidget::checkTranslationsSelection);
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::colorPicking_started, this->palWidget, &PaletteWidget::startTrnColorPicking);     // start color picking
    QObject::connect(this->trnBaseWidget, &PaletteWidget::colorPicking_started, this->trnUniqueWidget, &PaletteWidget::startTrnColorPicking); // start color picking
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::colorPicking_stopped, this->palWidget, &PaletteWidget::stopTrnColorPicking);      // finish or cancel color picking
    QObject::connect(this->trnBaseWidget, &PaletteWidget::colorPicking_stopped, this->trnUniqueWidget, &PaletteWidget::stopTrnColorPicking);  // finish or cancel color picking
    QObject::connect(this->palWidget, &PaletteWidget::colorPicking_stopped, this->trnUniqueWidget, &PaletteWidget::stopTrnColorPicking);      // cancel color picking
    QObject::connect(this->palWidget, &PaletteWidget::colorPicking_stopped, this->trnBaseWidget, &PaletteWidget::stopTrnColorPicking);        // cancel color picking
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::colorPicking_stopped, this->trnBaseWidget, &PaletteWidget::stopTrnColorPicking);  // cancel color picking

    // Refresh the view if a PAL or TRN is modified
    QObject::connect(this->palWidget, &PaletteWidget::modified, this, &MainWindow::colorModified);
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::modified, this, &MainWindow::colorModified);
    QObject::connect(this->trnBaseWidget, &PaletteWidget::modified, this, &MainWindow::colorModified);

    // Refresh paint dialog when the selected color is changed
    QObject::connect(this->palWidget, &PaletteWidget::colorsSelected, this->paintDialog, &PaintDialog::palColorsSelected);

    // Look for all palettes in the same folder as the CEL/CL2 file
    QString firstPaletteFound = fileType == 3 ? D1Pal::DEFAULT_PATH : "";
    if (!celDir.isEmpty()) {
        QDirIterator it(celDir, QStringList("*.pal"), QDir::Files | QDir::Readable);
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
    this->ui->menuPalette->setEnabled(true);
    this->ui->actionExport->setEnabled(true);
    this->ui->actionSave->setEnabled(true);
    this->ui->actionSaveAs->setEnabled(true);
    this->ui->actionClose->setEnabled(true);

    this->subtileMenu.setEnabled(isTileset);
    this->tileMenu.setEnabled(isTileset);

    this->ui->menuTileset->setEnabled(isTileset);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::openImageFiles(IMAGE_FILE_MODE mode, QStringList filePaths, bool append)
{
    if (filePaths.isEmpty()) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Reading..."), 0, PAF_UPDATE_WINDOW);

    if (this->celView != nullptr) {
        this->celView->insertImageFiles(mode, filePaths, append);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->insertImageFiles(mode, filePaths, append);
    }

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

    QString filePath = params.celFilePath.isEmpty() ? this->gfx->getFilePath() : params.celFilePath;
    QString fileLower = filePath.toLower();
    if (this->gfx->getType() == D1CEL_TYPE::V1_LEVEL) {
        if (!fileLower.endsWith(".cel")) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to save as %1? Data conversion is not supported.").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                // Clear loading message from status bar
                ProgressDialog::done();
                return;
            }
        }
        D1CelTileset::save(*this->gfx, params);
    } else {
        if (fileLower.endsWith(".cel")) {
            D1Cel::save(*this->gfx, params);
        } else if (fileLower.endsWith(".cl2")) {
            D1Cl2::save(*this->gfx, params);
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Not supported."));
            // Clear loading message from status bar
            ProgressDialog::done();
            return;
        }
    }

    if (this->tileset != nullptr) {
        this->tileset->save(params);
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::upscale(const UpscaleParam &params)
{
    /*ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Upscaling..."), 1, PAF_UPDATE_WINDOW);

    if (this->celView != nullptr) {
        this->celView->upscale(params);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->upscale(params);
    }

    // Clear loading message from status bar
    ProgressDialog::done();*/
    std::function<void()> func = [this, params]() {
        if (this->celView != nullptr) {
            this->celView->upscale(params);
        }
        if (this->levelCelView != nullptr) {
            this->levelCelView->upscale(params);
        }
    };
    ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::ACTIVE, tr("Upscaling..."), 1, PAF_UPDATE_WINDOW, std::move(func));
}

static QString imageNameFilter()
{
    // get supported image file types
    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = QImageReader::supportedMimeTypes();
    for (const QByteArray &mimeTypeName : supportedMimeTypes) {
        mimeTypeFilters.append(mimeTypeName);
    }

    // compose filter for all supported types
    QMimeDatabase mimeDB;
    QStringList allSupportedFormats;
    for (const QString &mimeTypeFilter : mimeTypeFilters) {
        QMimeType mimeType = mimeDB.mimeTypeForName(mimeTypeFilter);
        if (mimeType.isValid()) {
            QStringList mimePatterns = mimeType.globPatterns();
            for (int i = 0; i < mimePatterns.count(); i++) {
                allSupportedFormats.append(mimePatterns[i]);
                allSupportedFormats.append(mimePatterns[i].toUpper());
            }
        }
    }
    // add PCX support
    allSupportedFormats.append("*.pcx");
    allSupportedFormats.append("*.PCX");

    QString allSupportedFormatsFilter = QApplication::tr("Image files (%1)").arg(allSupportedFormats.join(' '));
    return allSupportedFormatsFilter;
}

void MainWindow::addFrames(bool append)
{
    QString filter = imageNameFilter();
    QStringList files = this->filesDialog(tr("Select Image Files"), filter.toLatin1().data());

    this->openImageFiles(IMAGE_FILE_MODE::FRAME, files, append);
}

void MainWindow::addSubtiles(bool append)
{
    QString filter = imageNameFilter();
    QStringList files = this->filesDialog(tr("Select Image Files"), filter.toLatin1().data());

    this->openImageFiles(IMAGE_FILE_MODE::SUBTILE, files, append);
}

void MainWindow::addTiles(bool append)
{
    QString filter = imageNameFilter();
    QStringList files = this->filesDialog(tr("Select Image Files"), filter.toLatin1().data());

    this->openImageFiles(IMAGE_FILE_MODE::TILE, files, append);
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
    if (this->gfx->getFilePath().isEmpty()) {
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
    this->saveAsDialog->initialize(this->gfx, this->tileset);
    this->saveAsDialog->show();
}

void MainWindow::on_actionClose_triggered()
{
    // this->on_actionStop_Draw_triggered();

    this->undoStack->clear();

    MemFree(this->paintDialog);
    MemFree(this->celView);
    MemFree(this->levelCelView);
    MemFree(this->palWidget);
    MemFree(this->trnUniqueWidget);
    MemFree(this->trnBaseWidget);
    MemFree(this->gfx);
    MemFree(this->tileset);

    qDeleteAll(this->pals);
    this->pals.clear();

    qDeleteAll(this->uniqueTrns);
    this->uniqueTrns.clear();

    qDeleteAll(this->baseTrns);
    this->baseTrns.clear();

    MemFree(this->palHits);

    // update available menu entries
    this->ui->menuEdit->setEnabled(false);
    this->ui->menuTileset->setEnabled(false);
    this->ui->menuPalette->setEnabled(false);
    this->ui->actionExport->setEnabled(false);
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

void MainWindow::on_actionExport_triggered()
{
    if (this->exportDialog == nullptr) {
        this->exportDialog = new ExportDialog(this);
    }
    this->exportDialog->initialize(this->gfx, this->tileset);
    this->exportDialog->show();
}

void MainWindow::on_actionQuit_triggered()
{
    qApp->quit();
}

void MainWindow::on_actionAddTo_Frame_triggered()
{
    QString filter = imageNameFilter();
    QString imgFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Composite Image File"), filter.toLatin1().data());

    if (imgFilePath.isEmpty()) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Reading..."), 0, PAF_UPDATE_WINDOW);

    if (this->celView != nullptr) {
        this->celView->addToCurrentFrame(imgFilePath);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->addToCurrentFrame(imgFilePath);
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionInsert_Frame_triggered()
{
    this->addFrames(false);
}

void MainWindow::on_actionAppend_Frame_triggered()
{
    this->addFrames(true);
}

void MainWindow::on_actionReplace_Frame_triggered()
{
    QString filter = imageNameFilter();
    QString imgFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Replacement Image File"), filter.toLatin1().data());

    if (imgFilePath.isEmpty()) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Reading..."), 0, PAF_UPDATE_WINDOW);

    if (this->celView != nullptr) {
        this->celView->replaceCurrentFrame(imgFilePath);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->replaceCurrentFrame(imgFilePath);
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionDel_Frame_triggered()
{
    if (this->celView != nullptr) {
        this->celView->removeCurrentFrame();
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->removeCurrentFrame();
    }
    this->updateWindow();
}

void MainWindow::on_actionCreate_Subtile_triggered()
{
    this->levelCelView->createSubtile();
    this->updateWindow();
}

void MainWindow::on_actionInsert_Subtile_triggered()
{
    this->addSubtiles(false);
}

void MainWindow::on_actionAppend_Subtile_triggered()
{
    this->addSubtiles(true);
}

void MainWindow::on_actionReplace_Subtile_triggered()
{
    QString filter = imageNameFilter();
    QString imgFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Replacement Image File"), filter.toLatin1().data());

    if (imgFilePath.isEmpty()) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Reading..."), 0, PAF_UPDATE_WINDOW);

    this->levelCelView->replaceCurrentSubtile(imgFilePath);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionDel_Subtile_triggered()
{
    this->levelCelView->removeCurrentSubtile();
    this->updateWindow();
}

void MainWindow::on_actionCreate_Tile_triggered()
{
    this->levelCelView->createTile();
    this->updateWindow();
}

void MainWindow::on_actionInsert_Tile_triggered()
{
    this->addTiles(false);
}

void MainWindow::on_actionAppend_Tile_triggered()
{
    this->addTiles(true);
}

void MainWindow::on_actionReplace_Tile_triggered()
{
    QString filter = imageNameFilter();
    QString imgFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Replacement Image File"), filter.toLatin1().data());

    if (imgFilePath.isEmpty()) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Reading..."), 0, PAF_UPDATE_WINDOW);

    this->levelCelView->replaceCurrentTile(imgFilePath);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionDel_Tile_triggered()
{
    this->levelCelView->removeCurrentTile();
    this->updateWindow();
}

void MainWindow::on_actionStart_Draw_triggered()
{
    this->paintDialog->show();
}

/*void MainWindow::on_actionStop_Draw_triggered()
{
    this->unsetCursor();
}*/

void MainWindow::on_actionUpscale_triggered()
{
    if (this->upscaleDialog == nullptr) {
        this->upscaleDialog = new UpscaleDialog(this);
    }
    this->upscaleDialog->initialize(this->gfx);
    this->upscaleDialog->show();
}

void MainWindow::on_actionReportUse_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->reportUsage();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionInefficientFrames_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->inefficientFrames();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionResetFrameTypes_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->resetFrameTypes();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionPatchTileset_Tileset_triggered()
{
    if (this->patchTilesetDialog == nullptr) {
        this->patchTilesetDialog = new PatchTilesetDialog(this);
    }
    this->patchTilesetDialog->initialize(this->tileset);
    this->patchTilesetDialog->show();
}

void MainWindow::on_actionCheckSubtileFlags_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->checkSubtileFlags();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCheckTileFlags_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->checkTileFlags();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCheckTilesetFlags_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->checkTilesetFlags();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCleanupFrames_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->levelCelView->cleanupFrames();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCleanupSubtiles_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->levelCelView->cleanupSubtiles();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCleanupTileset_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 2, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->levelCelView->cleanupTileset();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCompressSubtiles_Tileset_triggered()
{
    /*ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->levelCelView->compressSubtiles();

    // Clear loading message from status bar
    ProgressDialog::done();*/
    std::function<void()> func = [this]() {
        this->levelCelView->compressSubtiles();
    };
    ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW, std::move(func));
}

void MainWindow::on_actionCompressTiles_Tileset_triggered()
{
    /*ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->levelCelView->compressTiles();

    // Clear loading message from status bar
    ProgressDialog::done();*/
    std::function<void()> func = [this]() {
        this->levelCelView->compressTiles();
    };
    ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW, std::move(func));
}

void MainWindow::on_actionCompressTileset_Tileset_triggered()
{
    /*ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 2, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->levelCelView->compressTileset();

    // Clear loading message from status bar
    ProgressDialog::done();*/
    std::function<void()> func = [this]() {
        this->levelCelView->compressTileset();
    };
    ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 2, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW, std::move(func));
}

void MainWindow::on_actionSortFrames_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 0, PAF_UPDATE_WINDOW);

    this->levelCelView->sortFrames();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionSortSubtiles_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 0, PAF_UPDATE_WINDOW);

    this->levelCelView->sortSubtiles();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionSortTileset_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 0, PAF_UPDATE_WINDOW);

    this->levelCelView->sortTileset();

    // Clear loading message from status bar
    ProgressDialog::done();
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

void MainWindow::on_actionUpscaleTask_triggered()
{
    if (this->upscaleTaskDialog == nullptr) {
        this->upscaleTaskDialog = new UpscaleTaskDialog(this);
    }
    this->upscaleTaskDialog->show();
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
    QMessageBox::about(this, tr("About"), QStringLiteral("%1 %2 (%3) (%4-bit)").arg(D1_GRAPHICS_TOOL_TITLE).arg(D1_GRAPHICS_TOOL_VERSION).arg(OS_TYPE).arg(sizeof(void *) == 8 ? "64" : "32"));
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}
