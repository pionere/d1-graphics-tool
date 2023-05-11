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
    delete this->exportDialog;
    delete this->resizeDialog;
    delete this->upscaleDialog;
    delete this->patchTilesetDialog;
    delete this->patchDungeonDialog;
    delete this->remapDialog;
    delete this->upscaleTaskDialog;
}

MainWindow &dMainWindow()
{
    return *theMainWindow;
}

void MainWindow::changeColors(const RemapParam &params)
{
    std::vector<std::pair<D1GfxPixel, D1GfxPixel>> replacements;
    int index = params.colorTo.first;
    const int dc = params.colorTo.first == params.colorTo.second ? 0 : (params.colorTo.first < params.colorTo.second ? 1 : -1);
    for (int i = params.colorFrom.first; i <= params.colorFrom.second; i++, index += dc) {
        D1GfxPixel source = (i == D1PAL_COLORS) ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(i);
        D1GfxPixel replacement = (index == D1PAL_COLORS) ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(index);
        replacements.push_back(std::pair<D1GfxPixel, D1GfxPixel>(source, replacement));
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 0, PAF_UPDATE_WINDOW);

    int rangeFrom = params.frames.first;
    if (rangeFrom != 0) {
        rangeFrom--;
    }
    int rangeTo = params.frames.second;
    if (rangeTo == 0 || rangeTo > this->gfx->getFrameCount()) {
        rangeTo = this->gfx->getFrameCount();
    }
    rangeTo--;

    for (int i = rangeFrom; i <= rangeTo; i++) {
        D1GfxFrame *frame = this->gfx->getFrame(i);
        frame->replacePixels(replacements);
    }
    this->gfx->setModified();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::setPal(const QString &path)
{
    D1Pal *pal = this->pals[path];
    this->pal = pal;
    this->trnUnique->setPalette(this->pal);
    this->trnUnique->refreshResultingPalette();
    this->trnBase->refreshResultingPalette();
    // update entities
    if (this->dun != nullptr) {
        this->dun->setPal(pal);
    }
    // update the widgets
    // - views
    if (this->celView != nullptr) {
        this->celView->setPal(pal);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->setPal(pal);
    }
    if (this->tblView != nullptr) {
        this->tblView->setPal(pal);
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
    this->gfx->setPalette(resPal);
    if (this->tileset != nullptr) {
        this->tileset->cls->setPalette(resPal);
    }
    // update the widgets
    // - paint widget
    if (this->paintWidget != nullptr) {
        this->paintWidget->setPalette(resPal);
    }

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
    // refresh the palette-colors - triggered by displayFrame
    // if (this->palWidget != nullptr) {
    //     this->palWidget->refresh();
    // }
    // update menu options
    bool hasFrame = this->gfx != nullptr && this->gfx->getFrameCount() != 0;
    this->ui->actionReplace_Frame->setEnabled(hasFrame);
    this->ui->actionDel_Frame->setEnabled(hasFrame);
    bool hasSubtile = this->tileset != nullptr && this->tileset->min->getSubtileCount() != 0;
    this->ui->actionReplace_Subtile->setEnabled(hasSubtile);
    this->ui->actionDel_Subtile->setEnabled(hasSubtile);
    this->ui->actionCreate_Tile->setEnabled(hasSubtile);
    this->ui->actionInsert_Tile->setEnabled(hasSubtile);
    this->ui->actionAppend_Tile->setEnabled(hasSubtile);
    bool hasTile = this->tileset != nullptr && this->tileset->til->getTileCount() != 0;
    this->ui->actionReplace_Tile->setEnabled(hasTile);
    this->ui->actionDel_Tile->setEnabled(hasTile);

    // update the view
    if (this->celView != nullptr) {
        // this->celView->update();
        this->celView->displayFrame();
    }
    if (this->levelCelView != nullptr) {
        // this->levelCelView->update();
        this->levelCelView->displayFrame();
    }
    if (this->tblView != nullptr) {
        // this->tblView->update();
        this->tblView->displayFrame();
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

void MainWindow::frameClicked(D1GfxFrame *frame, const QPoint &pos, bool first)
{
    if (this->paintWidget->frameClicked(frame, pos, first)) {
        return;
    }
    // picking
    if (pos.x() < 0 || pos.x() >= frame->getWidth() || pos.y() < 0 || pos.y() >= frame->getHeight()) {
        // no target hit -> ignore
        return;
    }
    const D1GfxPixel pixel = frame->getPixel(pos.x(), pos.y());
    this->paintWidget->selectColor(pixel);
    this->palWidget->selectColor(pixel);
    this->trnUniqueWidget->selectColor(pixel);
    this->trnBaseWidget->selectColor(pixel);
}

void MainWindow::dunClicked(const QPoint &cell, bool first)
{
    // check if it is a valid position
    if (cell.x() < 0 || cell.x() >= this->dun->getWidth() || cell.y() < 0 || cell.y() >= this->dun->getHeight()) {
        // no target hit -> ignore
        return;
    }
    if (this->builderWidget != nullptr && this->builderWidget->dunClicked(cell, first)) {
        return;
    }
    // Set dungeon location
    this->levelCelView->selectPos(cell);
}

void MainWindow::dunHovered(const QPoint &cell)
{
    this->builderWidget->dunHovered(cell);
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
    if (this->tblView != nullptr) {
        this->tblView->displayFrame();
    }
    if (this->paintWidget != nullptr) {
        this->paintWidget->colorModified();
    }
    if (this->builderWidget != nullptr) {
        this->builderWidget->colorModified();
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
        if (iter.value() == this->pal) {
            currPalChanged = true;
        }
    }
    // refresh the palette widgets and the view
    if (currPalChanged) {
        this->palWidget->modify();
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
    this->openNew(OPEN_TILESET_TYPE::FALSE, OPEN_CLIPPED_TYPE::FALSE, false);
}

void MainWindow::on_actionNew_CL2_triggered()
{
    this->openNew(OPEN_TILESET_TYPE::FALSE, OPEN_CLIPPED_TYPE::TRUE, false);
}

void MainWindow::on_actionNew_Tileset_triggered()
{
    this->openNew(OPEN_TILESET_TYPE::TRUE, OPEN_CLIPPED_TYPE::FALSE, false);
}

void MainWindow::on_actionNew_Dungeon_triggered()
{
    this->openNew(OPEN_TILESET_TYPE::TRUE, OPEN_CLIPPED_TYPE::FALSE, true);
}

void MainWindow::openNew(OPEN_TILESET_TYPE tileset, OPEN_CLIPPED_TYPE clipped, bool createDun)
{
    OpenAsParam params = OpenAsParam();
    params.isTileset = tileset;
    params.clipped = clipped;
    params.createDun = createDun;
    this->openFile(params);
}

void MainWindow::on_actionOpen_triggered()
{
    QString openFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Open Graphics"), tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2);;PCX Files (*.pcx *.PCX);;DUN Files (*.dun *.DUN *.rdun *.RDUN);;TBL Files (*.tbl *.TBL)"));

    if (!openFilePath.isEmpty()) {
        QStringList filePaths;
        filePaths.append(openFilePath);
        this->openFiles(filePaths);
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
        if (fileLower.endsWith(".cel") || fileLower.endsWith(".cl2") || fileLower.endsWith(".dun") || fileLower.endsWith(".rdun") || fileLower.endsWith(".tbl") || (unloaded && fileLower.endsWith(".pcx"))) {
            event->acceptProposedAction();
            return;
        }
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

void MainWindow::openFiles(const QStringList &filePaths)
{
    for (const QString &filePath : filePaths) {
        OpenAsParam params = OpenAsParam();
        if (filePath.toLower().endsWith("dun")) { // .dun or .rdun
            params.dunFilePath = filePath;
        } else {
            params.celFilePath = filePath;
        }
        this->openFile(params);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (this->paintWidget != nullptr && !this->paintWidget->isHidden()) {
            this->paintWidget->hide();
        }
        if (this->builderWidget != nullptr && !this->builderWidget->isHidden()) {
            this->builderWidget->hide();
        }
        return;
    }
    if (event->matches(QKeySequence::New)) {
        this->ui->mainMenu->setActiveAction(this->ui->mainMenu->actions()[0]);
        this->ui->menuFile->setActiveAction(this->ui->menuFile->actions()[0]);
        this->ui->menuNew->setActiveAction(this->ui->menuNew->actions()[0]);
        return;
    }
    if (event->matches(QKeySequence::Copy)) {
        QImage image;
        if (this->paintWidget != nullptr && !this->paintWidget->isHidden()) {
            image = this->paintWidget->copyCurrent();
        } else if (this->celView != nullptr) {
            image = this->celView->copyCurrent();
        } else if (this->levelCelView != nullptr) {
            image = this->levelCelView->copyCurrent();
        }
        if (!image.isNull()) {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setImage(image);
        }
        return;
    }
    if (event->matches(QKeySequence::Cut)) {
        if (this->paintWidget != nullptr) {
            QImage image = this->paintWidget->copyCurrent();
            if (!image.isNull()) {
                this->paintWidget->deleteCurrent();
                QClipboard *clipboard = QGuiApplication::clipboard();
                clipboard->setImage(image);
            }
        }
        return;
    }
    if (event->matches(QKeySequence::Delete)) {
        if (this->paintWidget != nullptr) {
            this->paintWidget->deleteCurrent();
        }
        return;
    }
    if (event->matches(QKeySequence::Paste)) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QImage image = clipboard->image();
        if (!image.isNull()) {
            ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_UPDATE_WINDOW);

            if (this->paintWidget != nullptr && !this->paintWidget->isHidden()) {
                this->paintWidget->pasteCurrent(image);
            } else if (this->celView != nullptr) {
                this->celView->pasteCurrent(image);
            } else if (this->levelCelView != nullptr) {
                this->levelCelView->pasteCurrent(image);
            }

            // Clear loading message from status bar
            ProgressDialog::done();
            /*std::function<void()> func = [this, image]() {
                if (this->paintWidget != nullptr && !this->paintWidget->isHidden()) {
                    this->paintWidget->pasteCurrent(image);
                } else if (this->celView != nullptr) {
                    this->celView->pasteCurrent(image);
                } else if (this->levelCelView != nullptr) {
                    this->levelCelView->pasteCurrent(image);
                }
            };
            ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_UPDATE_WINDOW, std::move(func));*/
        }
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

void MainWindow::failWithError(const QString &error)
{
    dProgressFail() << error;

    this->on_actionClose_triggered();

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

void MainWindow::openFile(const OpenAsParam &params)
{
    QString gfxFilePath = params.celFilePath;

    // Check file extension
    int fileType = 0;
    if (!gfxFilePath.isEmpty()) {
        QString fileLower = gfxFilePath.toLower();
        if (fileLower.endsWith(".cel"))
            fileType = 1;
        else if (fileLower.endsWith(".cl2"))
            fileType = 2;
        else if (fileLower.endsWith(".pcx"))
            fileType = 3;
        else if (fileLower.endsWith(".tbl"))
            fileType = 4;
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

    QString clsFilePath = params.clsFilePath;
    QString tilFilePath = params.tilFilePath;
    QString minFilePath = params.minFilePath;
    QString solFilePath = params.solFilePath;
    QString ampFilePath = params.ampFilePath;
    QString sptFilePath = params.sptFilePath;
    QString tmiFilePath = params.tmiFilePath;
    QString dunFilePath = params.dunFilePath;
    QString tblFilePath = params.tblFilePath;

    QString baseDir;
    if (fileType == 4) {
        if (tblFilePath.isEmpty()) {
            tblFilePath = gfxFilePath;
            if (gfxFilePath.toLower().endsWith("dark.tbl")) {
                tblFilePath.replace(tblFilePath.length() - (sizeof("dark.tbl") - 2), 3, "ist");
            } else if (gfxFilePath.toLower().endsWith("dist.tbl")) {
                tblFilePath.replace(tblFilePath.length() - (sizeof("dist.tbl") - 2), 3, "ark");
            }
        }
    } else if (!gfxFilePath.isEmpty()) {
        QFileInfo celFileInfo = QFileInfo(gfxFilePath);

        baseDir = celFileInfo.absolutePath();
        QString basePath = baseDir + "/" + celFileInfo.completeBaseName();

        if (clsFilePath.isEmpty()) {
            clsFilePath = basePath + "s.cel";
        }
        if (tilFilePath.isEmpty()) {
            tilFilePath = basePath + ".til";
        }
        if (minFilePath.isEmpty()) {
            minFilePath = basePath + ".min";
        }
        if (solFilePath.isEmpty()) {
            solFilePath = basePath + ".sol";
        }
        if (ampFilePath.isEmpty()) {
            ampFilePath = basePath + ".amp";
        }
        if (sptFilePath.isEmpty()) {
            sptFilePath = basePath + ".spt";
        }
        if (tmiFilePath.isEmpty()) {
            tmiFilePath = basePath + ".tmi";
        }
    } else if (!dunFilePath.isEmpty()) {
        QFileInfo dunFileInfo = QFileInfo(dunFilePath);

        baseDir = dunFileInfo.absolutePath();
        QString baseName;

        findFirstFile(baseDir, QStringLiteral("*.til"), tilFilePath, baseName);
        findFirstFile(baseDir, QStringLiteral("*.min"), minFilePath, baseName);
        findFirstFile(baseDir, QStringLiteral("*.sol"), solFilePath, baseName);
        findFirstFile(baseDir, QStringLiteral("*.amp"), ampFilePath, baseName);
        findFirstFile(baseDir, QStringLiteral("*.spt"), sptFilePath, baseName);
        findFirstFile(baseDir, QStringLiteral("*.tmi"), tmiFilePath, baseName);
        findFirstFile(baseDir, QStringLiteral("*.cel"), gfxFilePath, baseName);
        findFirstFile(baseDir, QStringLiteral("*s.cel"), clsFilePath, baseName);
    }

    // If SOL, MIN and TIL files exist then build a LevelCelView
    bool isTileset = params.isTileset == OPEN_TILESET_TYPE::TRUE;
    if (params.isTileset == OPEN_TILESET_TYPE::AUTODETECT) {
        isTileset = ((fileType == 1 || fileType == 0) && QFileInfo::exists(dunFilePath))
            || (fileType == 1 && QFileInfo::exists(tilFilePath) && QFileInfo::exists(minFilePath) && QFileInfo::exists(solFilePath));
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
        if (!this->tileset->min->load(minFilePath, this->tileset, celFrameTypes, params)) {
            this->failWithError(tr("Failed loading MIN file: %1.").arg(QDir::toNativeSeparators(minFilePath)));
            return;
        }

        // Loading TIL
        if (!this->tileset->til->load(tilFilePath, this->tileset->min)) {
            this->failWithError(tr("Failed loading TIL file: %1.").arg(QDir::toNativeSeparators(tilFilePath)));
            return;
        }

        // Loading AMP
        if (!this->tileset->amp->load(ampFilePath, this->tileset->til->getTileCount(), params)) {
            this->failWithError(tr("Failed loading AMP file: %1.").arg(QDir::toNativeSeparators(ampFilePath)));
            return;
        }

        // Loading SPT
        if (!this->tileset->spt->load(sptFilePath, this->tileset->sol, params)) {
            this->failWithError(tr("Failed loading SPT file: %1.").arg(QDir::toNativeSeparators(sptFilePath)));
            return;
        }

        // Loading TMI
        if (!this->tileset->tmi->load(tmiFilePath, this->tileset->sol, params)) {
            this->failWithError(tr("Failed loading TMI file: %1.").arg(QDir::toNativeSeparators(tmiFilePath)));
            return;
        }

        // Loading CEL
        if (!D1CelTileset::load(*this->gfx, celFrameTypes, gfxFilePath, params)) {
            this->failWithError(tr("Failed loading Tileset-CEL file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }

        // Loading sCEL
        if (!this->tileset->loadCls(clsFilePath, params)) {
            this->failWithError(tr("Failed loading Special-CEL file: %1.").arg(QDir::toNativeSeparators(clsFilePath)));
            return;
        }

        // Loading DUN
        if (!dunFilePath.isEmpty() || params.createDun) {
            this->dun = new D1Dun();
            if (!this->dun->load(dunFilePath, params)) {
                this->failWithError(tr("Failed loading DUN file: %1.").arg(QDir::toNativeSeparators(dunFilePath)));
                return;
            }
            this->dun->initialize(this->pal, this->tileset);
        }
    } else if (fileType == 1) { // CEL
        if (!D1Cel::load(*this->gfx, gfxFilePath, params)) {
            this->failWithError(tr("Failed loading CEL file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
    } else if (fileType == 2) { // CL2
        if (!D1Cl2::load(*this->gfx, gfxFilePath, params)) {
            this->failWithError(tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
    } else if (fileType == 3) { // PCX
        if (!D1Pcx::load(*this->gfx, this->pal, gfxFilePath, params)) {
            this->failWithError(tr("Failed loading PCX file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
    } else if (fileType == 4) { // TBL
        this->tableset = new D1Tableset();
        if (!this->tableset->load(gfxFilePath, tblFilePath, params)) {
            this->failWithError(tr("Failed loading TBL file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
    } else {
        // gfxFilePath.isEmpty()
        this->gfx->setType(params.clipped == OPEN_CLIPPED_TYPE::TRUE ? D1CEL_TYPE::V2_MONO_GROUP : D1CEL_TYPE::V1_REGULAR);
    }

    // Add palette widgets for PAL and TRNs
    this->palWidget = new PaletteWidget(this, this->undoStack, tr("Palette"));
    this->trnUniqueWidget = new PaletteWidget(this, this->undoStack, tr("Unique translation"));
    this->trnBaseWidget = new PaletteWidget(this, this->undoStack, tr("Base Translation"));
    QLayout *palLayout = this->ui->palFrameWidget->layout();
    palLayout->addWidget(this->palWidget);
    palLayout->addWidget(this->trnUniqueWidget);
    palLayout->addWidget(this->trnBaseWidget);

    QWidget *view;
    if (isTileset) {
        // build a LevelCelView
        this->levelCelView = new LevelCelView(this);
        this->levelCelView->initialize(this->pal, this->tileset, this->dun, this->bottomPanelHidden);

        // Refresh palette widgets when frame, subtile of tile is changed
        QObject::connect(this->levelCelView, &LevelCelView::frameRefreshed, this->palWidget, &PaletteWidget::update);

        // Refresh palette widgets when the palette is changed (loading a PCX file)
        QObject::connect(this->levelCelView, &LevelCelView::palModified, this->palWidget, &PaletteWidget::refresh);

        view = this->levelCelView;
    } else if (fileType != 4) {
        // build a CelView
        this->celView = new CelView(this);
        this->celView->initialize(this->pal, this->gfx, this->bottomPanelHidden);

        // Refresh palette widgets when frame is changed
        QObject::connect(this->celView, &CelView::frameRefreshed, this->palWidget, &PaletteWidget::update);

        // Refresh palette widgets when the palette is changed (loading a PCX file)
        QObject::connect(this->celView, &CelView::palModified, this->palWidget, &PaletteWidget::refresh);

        view = this->celView;
    } else {
        this->tblView = new TblView(this, this->undoStack);
        this->tblView->initialize(this->pal, this->tableset, this->bottomPanelHidden);

        // Refresh palette widgets when frame is changed
        QObject::connect(this->tblView, &TblView::frameRefreshed, this->palWidget, &PaletteWidget::update);

        view = this->tblView;
    }
    // Add the view to the main frame
    this->ui->mainFrameLayout->addWidget(view);

    // prepare the paint dialog
    if (fileType != 4) {
        this->paintWidget = new PaintWidget(this, this->undoStack, this->gfx, this->celView, this->levelCelView);
        this->paintWidget->setPalette(this->trnBase->getResultingPalette());
    }

    // prepare the builder dialog
    if (this->dun != nullptr) {
        this->builderWidget = new BuilderWidget(this, this->undoStack, this->dun, this->levelCelView, this->tileset);
        // Refresh builder widget when the resource-options are changed
        QObject::connect(this->levelCelView, &LevelCelView::dunResourcesModified, this->builderWidget, &BuilderWidget::dunResourcesModified);
    }

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
    QObject::connect(this->palWidget, &PaletteWidget::colorsPicked, this->trnUniqueWidget, &PaletteWidget::checkTranslationsSelection);
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::colorsPicked, this->trnBaseWidget, &PaletteWidget::checkTranslationsSelection);
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::colorPicking_started, this->palWidget, &PaletteWidget::startTrnColorPicking);     // start color picking
    QObject::connect(this->trnBaseWidget, &PaletteWidget::colorPicking_started, this->trnUniqueWidget, &PaletteWidget::startTrnColorPicking); // start color picking
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::colorPicking_stopped, this->palWidget, &PaletteWidget::stopTrnColorPicking);      // finish or cancel color picking
    QObject::connect(this->trnBaseWidget, &PaletteWidget::colorPicking_stopped, this->trnUniqueWidget, &PaletteWidget::stopTrnColorPicking);  // finish or cancel color picking
    QObject::connect(this->palWidget, &PaletteWidget::colorPicking_stopped, this->trnUniqueWidget, &PaletteWidget::stopTrnColorPicking);      // cancel color picking
    QObject::connect(this->palWidget, &PaletteWidget::colorPicking_stopped, this->trnBaseWidget, &PaletteWidget::stopTrnColorPicking);        // cancel color picking
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::colorPicking_stopped, this->trnBaseWidget, &PaletteWidget::stopTrnColorPicking);  // cancel color picking

    // Refresh paint dialog when the selected color is changed
    if (this->paintWidget != nullptr) {
        QObject::connect(this->palWidget, &PaletteWidget::colorsSelected, this->paintWidget, &PaintWidget::palColorsSelected);
    }
    // Refresh tbl-view when the selected color is changed
    if (this->tblView != nullptr) {
        QObject::connect(this->palWidget, &PaletteWidget::colorsSelected, this->tblView, &TblView::palColorsSelected);
    }

    // Look for all palettes in the same folder as the CEL/CL2 file
    QString firstPaletteFound = fileType == 3 ? D1Pal::DEFAULT_PATH : "";
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
    this->ui->menuEdit->setEnabled(fileType != 4);
    this->ui->menuView->setEnabled(true);
    this->ui->menuColors->setEnabled(true);
    this->ui->actionExport->setEnabled(fileType != 4);
    this->ui->actionSave->setEnabled(true);
    this->ui->actionSaveAs->setEnabled(true);
    this->ui->actionClose->setEnabled(true);

    this->ui->menuSubtile->setEnabled(isTileset);
    this->ui->menuTile->setEnabled(isTileset);
    this->ui->actionResize->setEnabled(this->celView != nullptr);

    this->ui->menuTileset->setEnabled(isTileset);
    this->ui->menuDungeon->setEnabled(this->dun != nullptr);

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
    if (!filePath.isEmpty() && this->tableset == nullptr) {
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
    }

    if (this->tileset != nullptr) {
        this->tileset->save(params);
    }

    if (this->dun != nullptr) {
        this->dun->save(params);
    }

    if (this->tableset != nullptr) {
        this->tableset->save(params);
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::resize(const ResizeParam &params)
{
    // ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Resizing..."), 1, PAF_UPDATE_WINDOW);

    this->celView->resize(params);

    // Clear loading message from status bar
    // ProgressDialog::done();
    /*std::function<void()> func = [this, params]() {
        this->celView->resize(params);
    };
    ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::ACTIVE, tr("Resizing..."), 1, PAF_UPDATE_WINDOW, std::move(func));*/
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
    if (this->gfx->getFilePath().isEmpty()
        && (this->dun == nullptr || this->dun->getFilePath().isEmpty())
        && (this->tableset == nullptr || this->tableset->distTbl->getFilePath().isEmpty() || this->tableset->darkTbl->getFilePath().isEmpty())) {
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
    this->saveAsDialog->initialize(this->gfx, this->tileset, this->dun, this->tableset);
    this->saveAsDialog->show();
}

void MainWindow::on_actionClose_triggered()
{
    this->undoStack->clear();

    MemFree(this->paintWidget);
    MemFree(this->builderWidget);
    MemFree(this->celView);
    MemFree(this->levelCelView);
    MemFree(this->tblView);
    MemFree(this->palWidget);
    MemFree(this->trnUniqueWidget);
    MemFree(this->trnBaseWidget);
    MemFree(this->gfx);
    MemFree(this->tileset);
    MemFree(this->dun);
    MemFree(this->tableset);

    qDeleteAll(this->pals);
    this->pals.clear();

    qDeleteAll(this->uniqueTrns);
    this->uniqueTrns.clear();

    qDeleteAll(this->baseTrns);
    this->baseTrns.clear();

    MemFree(this->palHits);

    // update available menu entries
    this->ui->menuEdit->setEnabled(false);
    this->ui->menuView->setEnabled(false);
    this->ui->menuTileset->setEnabled(false);
    this->ui->menuDungeon->setEnabled(false);
    this->ui->menuColors->setEnabled(false);
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

void MainWindow::on_actionToggle_View_triggered()
{
    this->paintWidget->hide();
    if (this->builderWidget != nullptr) {
        this->builderWidget->hide();
    }
    this->levelCelView->on_actionToggle_View_triggered();
}

void MainWindow::on_actionToggle_Painter_triggered()
{
    if (this->paintWidget->isHidden()) {
        this->paintWidget->show();
    } else {
        this->paintWidget->hide();
    }
}

void MainWindow::on_actionToggle_Builder_triggered()
{
    if (this->builderWidget->isHidden()) {
        this->builderWidget->show();
    } else {
        this->builderWidget->hide();
    }
}

void MainWindow::on_actionTogglePalTrn_triggered()
{
    this->ui->palFrameWidget->setVisible(this->ui->palFrameWidget->isHidden());
}

void MainWindow::on_actionToggleBottomPanel_triggered()
{
    this->bottomPanelHidden = !this->bottomPanelHidden;
    if (this->levelCelView != nullptr) {
        this->levelCelView->toggleBottomPanel();
    }
    if (this->celView != nullptr) {
        this->celView->toggleBottomPanel();
    }
    if (this->tblView != nullptr) {
        this->tblView->toggleBottomPanel();
    }
}

void MainWindow::on_actionResize_triggered()
{
    if (this->resizeDialog == nullptr) {
        this->resizeDialog = new ResizeDialog(this);
    }
    this->resizeDialog->initialize();
    this->resizeDialog->show();
}

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

void MainWindow::on_actionReportActiveSubtiles_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->activeSubtiles();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionReportActiveTiles_Tileset_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->activeTiles();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionReportInefficientFrames_Tileset_triggered()
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

void MainWindow::on_actionReportUse_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->reportDungeonUsage();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionPatchDungeon_Dungeon_triggered()
{
    if (this->patchDungeonDialog == nullptr) {
        this->patchDungeonDialog = new PatchDungeonDialog(this);
    }
    this->patchDungeonDialog->initialize(this->dun);
    this->patchDungeonDialog->show();
}

void MainWindow::on_actionResetTiles_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->levelCelView->resetDungeonTiles();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionResetSubtiles_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->levelCelView->resetDungeonSubtiles();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionMaskTiles_Dungeon_triggered()
{
    D1Dun *srcDun = this->loadDun(tr("Dungeon map file"));
    if (srcDun == nullptr) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG | PAF_UPDATE_WINDOW);

    this->levelCelView->maskDungeonTiles(srcDun);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionProtectTiles_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->protectDungeonTiles();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionProtectTilesFrom_Dungeon_triggered()
{
    D1Dun *srcDun = this->loadDun(tr("Pre-Dungeon map file"));
    if (srcDun == nullptr) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->protectDungeonTilesFrom(srcDun);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionProtectSubtiles_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->protectDungeonSubtiles();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCheckTiles_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->checkTiles();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCheckProtections_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->checkProtections();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCheckItems_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->checkItems();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCheckMonsters_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->checkMonsters();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCheckObjects_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->checkObjects();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCheckEntities_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->checkEntities();

    // Clear loading message from status bar
    ProgressDialog::done();
}

D1Dun *MainWindow::loadDun(const QString &title)
{
    QString dunFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, title, "DUN Files (*.dun *.DUN)");

    if (dunFilePath.isEmpty()) {
        return nullptr;
    }
    D1Dun *srcDun = new D1Dun();
    OpenAsParam params = OpenAsParam();
    params.dunFilePath = dunFilePath;
    if (srcDun->load(dunFilePath, params)) {
        return srcDun;
    }
    delete srcDun;
    QMessageBox::critical(this, tr("Error"), tr("Failed loading DUN file."));
    return nullptr;
}

void MainWindow::on_actionRemoveProtections_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->removeProtections();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionLoadProtections_Dungeon_triggered()
{
    D1Dun *srcDun = this->loadDun(tr("Source of the flags"));
    if (srcDun == nullptr) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->loadProtections(srcDun);
    delete srcDun;

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionRemoveItems_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->removeItems();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionLoadItems_Dungeon_triggered()
{
    D1Dun *srcDun = this->loadDun(tr("Source of the items"));
    if (srcDun == nullptr) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->loadItems(srcDun);
    delete srcDun;

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionRemoveMonsters_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->removeMonsters();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionLoadMonsters_Dungeon_triggered()
{
    D1Dun *srcDun = this->loadDun(tr("Source of the monsters"));
    if (srcDun == nullptr) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->loadMonsters(srcDun);
    delete srcDun;

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionRemoveObjects_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->removeObjects();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionLoadObjects_Dungeon_triggered()
{
    D1Dun *srcDun = this->loadDun(tr("Source of the objects"));
    if (srcDun == nullptr) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->loadObjects(srcDun);
    delete srcDun;

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionGenerate_Dungeon_triggered()
{
    this->levelCelView->generateDungeon();
}

void MainWindow::on_actionSearch_Dungeon_triggered()
{
    this->levelCelView->searchDungeon();
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

void MainWindow::on_actionRemap_Colors_triggered()
{
    if (this->remapDialog == nullptr) {
        this->remapDialog = new RemapDialog(this);
    }
    this->remapDialog->initialize(this->palWidget);
    this->remapDialog->show();
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
