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
#include "d1smk.h"
#include "d1trs.h"
#include "ui_mainwindow.h"

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

    /*for (const QAction *ac : this->findChildren<QAction*>()) {
           LogErrorF("Main ac:%s tt:%s", ac->text(), ac->toolTip());
        QList<QKeySequence> sc = ac->shortcuts();
        for (const QKeySequence &ks : sc) {
            LogErrorF("Main seq:%s", ks.toString(QKeySequence::PortableText));
            if (ks == QKeySequence::Cancel || ks == QKeySequence::New || ks == QKeySequence::Copy || ks == QKeySequence::Cut || ks == QKeySequence::Delete || ks == QKeySequence::Paste) {
                // qDebug() << tr("Conflicing shortcut in the main menu (%1).").arg(ks.toString());
                LogErrorF("Conflicing shortcut in the main menu (%s).", ks.toString(QKeySequence::PortableText));
            }
            for (int i = 0; i < ks.count(); i++) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                int kc = ks[i].toCombined();
#else
                int kc = ks[i];
#endif
                const int kcs[2] = { (Qt::CTRL | Qt::Key_E), (Qt::CTRL | Qt::Key_P) };
                for (int n = 0; n < 2; n++) {
                    if (kcs[n] == kc) {
                        LogErrorF("Conflicing shortcut in the main menu (%s).", ks.toString(QKeySequence::PortableText));
                        // qDebug() << tr("Conflicing shortcut in the main menu (%1).").arg(ks.toString());
                        i = INT_MAX;
                        break;
                    }
                }
            }
        }
    }*/
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
    delete this->mergeFramesDialog;
    delete this->patchDungeonDialog;
    delete this->patchGfxDialog;
    delete this->patchTilesetDialog;
    delete this->remapDialog;
    delete this->upscaleTaskDialog;
}

MainWindow &dMainWindow()
{
    return *theMainWindow;
}

void MainWindow::remapColors(const RemapParam &params)
{
    QList<QPair<D1GfxPixel, D1GfxPixel>> replacements;
    int index = params.colorTo.first;
    const int dc = params.colorTo.first == params.colorTo.second ? 0 : (params.colorTo.first < params.colorTo.second ? 1 : -1);
    for (int i = params.colorFrom.first; i <= params.colorFrom.second; i++, index += dc) {
        D1GfxPixel source = (i == D1PAL_COLORS) ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(i);
        D1GfxPixel replacement = (index == D1PAL_COLORS) ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(index);
        replacements.push_back(std::pair<D1GfxPixel, D1GfxPixel>(source, replacement));
    }

    this->changeColors(replacements, params);
}

void MainWindow::changeColors(QList<QPair<D1GfxPixel, D1GfxPixel>> &replacements, const RemapParam &params)
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 0, PAF_UPDATE_WINDOW);

    if (this->gfxset != nullptr) {
        this->gfxset->replacePixels(replacements, params);
    } else {
        int rangeFrom = params.frames.first;
        if (rangeFrom != 0) {
            rangeFrom--;
        }
        int rangeTo = params.frames.second;
        if (rangeTo == 0 || rangeTo >= this->gfx->getFrameCount()) {
            rangeTo = this->gfx->getFrameCount();
        }
        rangeTo--;

        for (int i = rangeFrom; i <= rangeTo; i++) {
            D1GfxFrame *frame = this->gfx->getFrame(i);
            if (frame->replacePixels(replacements)) {
                this->gfx->setModified();
            }
        }
    }

    // Clear loading message from status bar
    ProgressDialog::done();
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
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->setPal(pal);
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
    if (this->gfxset != nullptr) {
        this->gfxset->setPalette(resPal);
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
    this->ui->actionDuplicate_Frame->setEnabled(hasFrame);
    this->ui->actionReplace_Frame->setEnabled(hasFrame);
    this->ui->actionDel_Frame->setEnabled(hasFrame);
    bool hasSubtile = this->tileset != nullptr && this->tileset->min->getSubtileCount() != 0;
    this->ui->actionDuplicate_Subtile->setEnabled(hasSubtile);
    this->ui->actionReplace_Subtile->setEnabled(hasSubtile);
    this->ui->actionDel_Subtile->setEnabled(hasSubtile);
    this->ui->actionCreate_Tile->setEnabled(hasSubtile);
    this->ui->actionInsert_Tile->setEnabled(hasSubtile);
    bool hasTile = this->tileset != nullptr && this->tileset->til->getTileCount() != 0;
    this->ui->actionDuplicate_Tile->setEnabled(hasTile);
    this->ui->actionReplace_Tile->setEnabled(hasTile);
    this->ui->actionDel_Tile->setEnabled(hasTile);
    bool hasColumn = this->cppView != nullptr && this->cppView->getCurrentTable() != nullptr && this->cppView->getCurrentTable()->getColumnCount() != 0;
    this->ui->actionDelColumn_Table->setEnabled(hasColumn);
    this->ui->actionHideColumn_Table->setEnabled(hasColumn);
    this->ui->actionMoveLeftColumn_Table->setEnabled(hasColumn);
    this->ui->actionMoveRightColumn_Table->setEnabled(hasColumn);
    this->ui->actionDelColumns_Table->setEnabled(hasColumn);
    this->ui->actionHideColumns_Table->setEnabled(hasColumn);
    bool hasRow = this->cppView != nullptr && this->cppView->getCurrentTable() != nullptr && this->cppView->getCurrentTable()->getRowCount() != 0;
    this->ui->actionDelRow_Table->setEnabled(hasRow);
    this->ui->actionHideRow_Table->setEnabled(hasRow);
    this->ui->actionMoveUpRow_Table->setEnabled(hasRow);
    this->ui->actionMoveDownRow_Table->setEnabled(hasRow);
    this->ui->actionDelRows_Table->setEnabled(hasRow);
    this->ui->actionHideRows_Table->setEnabled(hasRow);

    // update the view
    if (this->celView != nullptr) {
        // this->celView->updateFields();
        this->celView->displayFrame();
    }
    if (this->levelCelView != nullptr) {
        // this->levelCelView->updateFields();
        this->levelCelView->displayFrame();
    }
    if (this->gfxsetView != nullptr) {
        // this->gfxsetView->updateFields();
        this->gfxsetView->displayFrame();
    }
    if (this->tblView != nullptr) {
        // this->tblView->updateFields();
        this->tblView->displayFrame();
    }
    if (this->cppView != nullptr) {
        // this->cppView->updateFields();
        this->cppView->displayFrame();
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

void MainWindow::frameClicked(D1GfxFrame *frame, const QPoint &pos, int flags)
{
    if (this->paintWidget->frameClicked(frame, pos, flags)) {
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

void MainWindow::dunClicked(const QPoint &cell, int flags)
{
    if (this->builderWidget != nullptr && this->builderWidget->dunClicked(cell, flags)) {
        return;
    }
    // check if it is a valid position
    if (cell.x() < 0 || cell.x() >= this->dun->getWidth() || cell.y() < 0 || cell.y() >= this->dun->getHeight()) {
        // no target hit -> ignore
        return;
    }
    // Set dungeon location
    this->levelCelView->selectPos(cell, flags);
}

void MainWindow::dunHovered(const QPoint &cell)
{
    this->builderWidget->dunHovered(cell);
}

int MainWindow::getDunBuilderMode() const
{
    return this->builderWidget == nullptr ? -1 : this->builderWidget->getOverlayType();
}

void MainWindow::frameModified(D1GfxFrame *frame)
{
    if (this->gfxset != nullptr) {
        this->gfxset->frameModified(frame);
    } else {
        this->gfx->setModified();
    }

    this->updateWindow();
}

void MainWindow::colorModified()
{
    // update the view
    if (this->celView != nullptr) {
        this->celView->displayFrame();
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->displayFrame();
    }
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->displayFrame();
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
        if (change && iter.value() == this->pal) {
            currPalChanged = true;
        }
    }
    // refresh the palette widgets and the view
    if (currPalChanged && this->palWidget != nullptr) {
        this->palWidget->modify();
    }
}

void MainWindow::gfxChanged(D1Gfx *gfx)
{
    this->gfx = gfx;
    if (this->palHits != nullptr) {
        this->palHits->setGfx(gfx);
    }
    // if (this->tileset != nullptr) {
    //    this->tileset->gfx = gfx;
    //    this->tileset->min->setGfx(gfx);
    // }
    if (this->gfxset != nullptr) {
        this->gfxset->setGfx(gfx);
    }
    if (this->celView != nullptr) {
        this->celView->setGfx(gfx);
    }
    // if (this->levelCelView != nullptr) {
    //    this->levelCelView->setGfx(gfx);
    // }
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->setGfx(gfx);
    }
    if (this->paintWidget != nullptr) {
        this->paintWidget->setGfx(gfx);
    }
    this->updateWindow();
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
    this->openNew(OPEN_GFX_TYPE::BASIC, OPEN_CLIPPED_TYPE::FALSE, false);
}

void MainWindow::on_actionNew_CL2_triggered()
{
    this->openNew(OPEN_GFX_TYPE::BASIC, OPEN_CLIPPED_TYPE::TRUE, false);
}

void MainWindow::on_actionNew_Tileset_triggered()
{
    this->openNew(OPEN_GFX_TYPE::TILESET, OPEN_CLIPPED_TYPE::FALSE, false);
}

void MainWindow::on_actionNew_Dungeon_triggered()
{
    this->openNew(OPEN_GFX_TYPE::TILESET, OPEN_CLIPPED_TYPE::FALSE, true);
}

void MainWindow::openNew(OPEN_GFX_TYPE gfxType, OPEN_CLIPPED_TYPE clipped, bool createDun)
{
    OpenAsParam params = OpenAsParam();
    params.gfxType = gfxType;
    params.clipped = clipped;
    params.createDun = createDun;
    this->openFile(params);
}

void MainWindow::on_actionNew_Gfxset_triggered()
{
    QString openFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Open Graphics"), tr("CL2 Files (*.cl2 *.CL2)"));

    if (!openFilePath.isEmpty()) {
        OpenAsParam params = OpenAsParam();
        params.gfxType = OPEN_GFX_TYPE::GFXSET;
        params.celFilePath = openFilePath;
        this->openFile(params);
    }
}

void MainWindow::on_actionOpen_triggered()
{
    QString openFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Open Graphics"), tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2);;PCX Files (*.pcx *.PCX);;SMK Files (*.smk *.SMK);;DUN Files (*.dun *.DUN *.rdun *.RDUN);;TBL Files (*.tbl *.TBL);;CPP Files (*.cpp *.CPP *.c *.C)"));

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
    if (this->levelCelView != nullptr) {
        title = tr("Load Dungeon");
        filter = tr("DUN Files (*.dun *.DUN *.rdun *.RDUN)");
    } else {
        // assert(this->celView != nullptr);
        title = tr("Load Graphics");
        filter = tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2)");
    }

    QString gfxFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, title, filter);
    if (gfxFilePath.isEmpty()) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_NONE); // PAF_UPDATE_WINDOW

    QString fileLower = gfxFilePath.toLower();
    OpenAsParam params = OpenAsParam();
    if (fileLower.endsWith("dun")) { // .dun or .rdun
        params.dunFilePath = gfxFilePath;
    } else {
        params.celFilePath = gfxFilePath;
    }

    if (!params.dunFilePath.isEmpty()) {
        D1Dun *dun = new D1Dun();
        if (dun->load(params.dunFilePath, params)) {
            dun->initialize(this->pal, this->tileset);
            // TODO: this->dunChanged(dun)
            delete this->dun;
            this->dun = dun;
            this->levelCelView->setDungeon(dun);
            if (this->builderWidget == nullptr) {
                // TODO: copy-paste from openFile()...
                this->builderWidget = new BuilderWidget(this, this->undoStack, dun, this->levelCelView, this->tileset);
                // Refresh builder widget when the resource-options are changed
                QObject::connect(this->levelCelView, &LevelCelView::dunResourcesModified, this->builderWidget, &BuilderWidget::dunResourcesModified);
            } else {
                this->builderWidget->setDungeon(dun);
            }
            this->ui->menuDungeon->setEnabled(true);
            this->updateWindow();
        } else {
            delete dun;
            dProgressFail() << tr("Failed loading DUN file: %1.").arg(QDir::toNativeSeparators(gfxFilePath));
        }
    } else {
        D1Gfx *gfx = new D1Gfx();
        gfx->setPalette(this->trnBase->getResultingPalette());

        if ((fileLower.endsWith(".cel") && D1Cel::load(*gfx, gfxFilePath, params)) || D1Cl2::load(*gfx, gfxFilePath, params)) {
            delete this->gfx;
            // this->gfx = gfx;
            this->gfxChanged(gfx);
        } else /* if (fileLower.endsWith(".cl2"))*/ {
            delete gfx;
            dProgressFail() << tr("Failed loading GFX file: %1.").arg(QDir::toNativeSeparators(gfxFilePath));
        }
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
    bool unloaded = this->celView == nullptr && this->levelCelView == nullptr && this->gfxsetView == nullptr;

    for (const QUrl &url : event->mimeData()->urls()) {
        QString fileLower = url.toLocalFile().toLower();
        if (fileLower.endsWith(".cel") || fileLower.endsWith(".cl2") || fileLower.endsWith(".dun") || fileLower.endsWith(".rdun") || fileLower.endsWith(".tbl") || (unloaded && (fileLower.endsWith(".pcx") || fileLower.endsWith(".smk")))) {
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
        if (filePath.toLower().endsWith("dun")) { // .dun or .rdun
            params.dunFilePath = filePath;
        } else {
            params.celFilePath = filePath;
        }
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
    if (keyCombinationMatchesSequence(kc, QKeySequence::Cancel)) { // event->matches(QKeySequence::Cancel)) {
        if (this->paintWidget != nullptr && !this->paintWidget->isHidden()) {
            this->paintWidget->hide();
        }
        if (this->builderWidget != nullptr && !this->builderWidget->isHidden()) {
            this->builderWidget->hide();
        }
        return;
    }
    if (keyCombinationMatchesSequence(kc, QKeySequence::New)) { // event->matches(QKeySequence::New)) {
        this->ui->mainMenu->setActiveAction(this->ui->mainMenu->actions()[0]);
        this->ui->menuFile->setActiveAction(this->ui->menuFile->actions()[0]);
        this->ui->menuNew->setActiveAction(this->ui->menuNew->actions()[0]);
        return;
    }
    if (keyCombinationMatchesSequence(kc, QKeySequence::Copy)) { // event->matches(QKeySequence::Copy)) {
        QImage image;
        if (this->paintWidget != nullptr && !this->paintWidget->isHidden()) {
            image = this->paintWidget->copyCurrentImage();
        } else if (this->celView != nullptr) {
            image = this->celView->copyCurrentImage();
        } else if (this->levelCelView != nullptr) {
            image = this->levelCelView->copyCurrentImage();
        } else if (this->gfxsetView != nullptr) {
            image = this->gfxsetView->copyCurrentImage();
        }
        if (!image.isNull()) {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setImage(image);
        }
        return;
    }
    if (kc == (Qt::CTRL | Qt::Key_E) || kc == (Qt::CTRL | Qt::Key_E | Qt::ShiftModifier)) {
        bool shift = (kc & Qt::ShiftModifier) != 0;
        QString pixels;
        if (this->paintWidget != nullptr && !this->paintWidget->isHidden()) {
            pixels = this->paintWidget->copyCurrentPixels(shift);
        } else if (this->celView != nullptr) {
            pixels = this->celView->copyCurrentPixels(shift);
        } else if (this->levelCelView != nullptr) {
            pixels = this->levelCelView->copyCurrentPixels(shift);
        } else if (this->gfxsetView != nullptr) {
            pixels = this->gfxsetView->copyCurrentPixels(shift);
        }
        if (!pixels.isEmpty()) {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(pixels);
        }
        return;
    }
    if (keyCombinationMatchesSequence(kc, QKeySequence::Cut)) { // event->matches(QKeySequence::Cut)) {
        if (this->paintWidget != nullptr && !this->paintWidget->isHidden()) {
            QImage image = this->paintWidget->copyCurrentImage();
            if (!image.isNull()) {
                this->paintWidget->deleteCurrent();
                QClipboard *clipboard = QGuiApplication::clipboard();
                clipboard->setImage(image);
            }
        }
        return;
    }
    if (keyCombinationMatchesSequence(kc, QKeySequence::Delete)) { // event->matches(QKeySequence::Delete)) {
        if (this->paintWidget != nullptr && !this->paintWidget->isHidden()) {
            this->paintWidget->deleteCurrent();
        }
        return;
    }
    if (keyCombinationMatchesSequence(kc, QKeySequence::Paste)) { // event->matches(QKeySequence::Paste)) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QImage image = clipboard->image();
        if (!image.isNull()) {
            ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_UPDATE_WINDOW);

            if (this->paintWidget != nullptr && !this->paintWidget->isHidden()) {
                this->paintWidget->pasteCurrentImage(image);
            } else if (this->celView != nullptr) {
                this->celView->pasteCurrentImage(image);
            } else if (this->levelCelView != nullptr) {
                this->levelCelView->pasteCurrentImage(image);
            } else if (this->gfxsetView != nullptr) {
                this->gfxsetView->pasteCurrentImage(image);
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
    if (kc == (Qt::CTRL | Qt::Key_R)) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QString pixels = clipboard->text();
        if (!pixels.isEmpty()) {
            if (this->paintWidget != nullptr && !this->paintWidget->isHidden()) {
                this->paintWidget->pasteCurrentPixels(pixels);
            } else if (this->celView != nullptr) {
                this->celView->pasteCurrentPixels(pixels);
            } else if (this->levelCelView != nullptr) {
                this->levelCelView->pasteCurrentPixels(pixels);
            } else if (this->gfxsetView != nullptr) {
                this->gfxsetView->pasteCurrentPixels(pixels);
            }
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
        else if (fileLower.endsWith(".cpp"))
            fileType = 5;
        else if (fileLower.endsWith(".smk"))
            fileType = 6;
        else
            return;
    }

    this->on_actionClose_triggered();

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_NONE); // PAF_UPDATE_WINDOW

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
    QString slaFilePath = params.slaFilePath;
    QString tlaFilePath = params.tlaFilePath;
    QString dunFilePath = params.dunFilePath;
    QString tblFilePath = params.tblFilePath;

    QString baseDir;
    if (fileType == 4) {
        if (tblFilePath.isEmpty()) {
            QFileInfo tblFileInfo = QFileInfo(gfxFilePath);
            QString fileLower = tblFileInfo.completeBaseName().toLower();
            QDirIterator it(tblFileInfo.absolutePath(), QStringList("*.tbl"), QDir::Files | QDir::Readable);
            while (it.hasNext()) {
                QString sPath = it.next();

                if (sPath == gfxFilePath) {
                    continue;
                }
                tblFilePath = sPath;

                tblFileInfo = QFileInfo(sPath);
                QString sLower = tblFileInfo.completeBaseName().toLower();

                // try to find a matching pair for any *dark*.tbl
                int match = 0;
                while (true) {
                    match = fileLower.indexOf("dark", match);
                    if (match == -1) {
                        break;
                    }
                    QString test = fileLower;
                    test.replace(match, 4, "dist");
                    if (test == sLower) {
                        break;
                    }
                    match++;
                }
                if (match != -1) {
                    break;
                }
                // try to find a matching pair for any *dist*.tbl
                match = 0;
                while (true) {
                    match = fileLower.indexOf("dist", match);
                    if (match == -1) {
                        break;
                    }
                    QString test = fileLower;
                    test.replace(match, 4, "dark");
                    if (test == sLower) {
                        break;
                    }
                    match++;
                }
                if (match != -1) {
                    break;
                }
            }
            if (tblFilePath.isEmpty()) {
                this->failWithError(tr("Could not find the other table file for TBL file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
                return;
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
        if (slaFilePath.isEmpty()) {
            slaFilePath = basePath + ".sla";
        }
        if (tlaFilePath.isEmpty()) {
            tlaFilePath = basePath + ".tla";
        }
    } else if (!dunFilePath.isEmpty()) {
        QFileInfo dunFileInfo = QFileInfo(dunFilePath);

        baseDir = dunFileInfo.absolutePath();
        QString baseName;

        findFirstFile(baseDir, QStringLiteral("*.til"), tilFilePath, baseName);
        findFirstFile(baseDir, QStringLiteral("*.min"), minFilePath, baseName);
        findFirstFile(baseDir, QStringLiteral("*.sla"), slaFilePath, baseName);
        findFirstFile(baseDir, QStringLiteral("*.tla"), tlaFilePath, baseName);
        findFirstFile(baseDir, QStringLiteral("*.cel"), gfxFilePath, baseName);
        findFirstFile(baseDir, QStringLiteral("*s.cel"), clsFilePath, baseName);
    }

    // If SLA, MIN and TIL files exist then build a LevelCelView
    bool isTileset = params.gfxType == OPEN_GFX_TYPE::TILESET;
    if (params.gfxType == OPEN_GFX_TYPE::AUTODETECT) {
        isTileset = ((fileType == 1 || fileType == 0) && QFileInfo::exists(dunFilePath))
            || (fileType == 1 && QFileInfo::exists(tilFilePath) && QFileInfo::exists(minFilePath) && QFileInfo::exists(slaFilePath));
    }

    bool isGfxset = params.gfxType == OPEN_GFX_TYPE::GFXSET;

    this->gfx = new D1Gfx();
    this->gfx->setPalette(this->trnBase->getResultingPalette());
    if (isGfxset) {
        this->gfxset = new D1Gfxset(this->gfx);
        if (!this->gfxset->load(gfxFilePath, params)) {
            this->failWithError(tr("Failed loading GFX file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
    } else if (isTileset) {
        this->tileset = new D1Tileset(this->gfx);
        // Loading SLA
        if (!this->tileset->sla->load(slaFilePath)) {
            this->failWithError(tr("Failed loading SLA file: %1.").arg(QDir::toNativeSeparators(slaFilePath)));
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

        // Loading TLA
        if (!this->tileset->tla->load(tlaFilePath, this->tileset->til->getTileCount(), params)) {
            this->failWithError(tr("Failed loading TLA file: %1.").arg(QDir::toNativeSeparators(tlaFilePath)));
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
    } else if (fileType == 5) { // CPP
        this->cpp = new D1Cpp();
        if (!this->cpp->load(gfxFilePath)) {
            this->failWithError(tr("Failed loading CPP file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
    } else if (fileType == 6) { // SMK
        if (!D1Smk::load(*this->gfx, this->pals, gfxFilePath, params)) {
            this->failWithError(tr("Failed loading SMK file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
        // add paths in palWidget
        // this->palWidget->updatePathComboBoxOptions(this->pals.keys(), this->pal->getFilePath()); -- should be called later
    } else {
        // gfxFilePath.isEmpty()
        this->gfx->setType(params.clipped == OPEN_CLIPPED_TYPE::TRUE ? D1CEL_TYPE::V2_MONO_GROUP : D1CEL_TYPE::V1_REGULAR);
    }

    if (fileType != 5) {
    // Add palette widgets for PAL and TRNs
    this->palWidget = new PaletteWidget(this, this->undoStack, tr("Palette"));
    this->trnUniqueWidget = new PaletteWidget(this, this->undoStack, tr("Unique translation"));
    this->trnBaseWidget = new PaletteWidget(this, this->undoStack, tr("Base Translation"));
    QLayout *palLayout = this->ui->palFrameWidget->layout();
    palLayout->addWidget(this->palWidget);
    palLayout->addWidget(this->trnUniqueWidget);
    palLayout->addWidget(this->trnBaseWidget);
    }

    QWidget *view;
    if (isGfxset) {
        // build a GfxsetView
        this->gfxsetView = new GfxsetView(this);
        this->gfxsetView->initialize(this->pal, this->gfxset, this->bottomPanelHidden);

        // Refresh palette widgets when frame is changed
        QObject::connect(this->gfxsetView, &GfxsetView::frameRefreshed, this->palWidget, &PaletteWidget::refresh);

        // Refresh palette widgets when the palette is changed (loading a PCX file)
        QObject::connect(this->gfxsetView, &GfxsetView::palModified, this->palWidget, &PaletteWidget::refresh);

        view = this->gfxsetView;
    } else if (isTileset) {
        // build a LevelCelView
        this->levelCelView = new LevelCelView(this, this->undoStack);
        this->levelCelView->initialize(this->pal, this->tileset, this->dun, this->bottomPanelHidden);

        // Refresh palette widgets when frame, subtile or tile is changed
        QObject::connect(this->levelCelView, &LevelCelView::frameRefreshed, this->palWidget, &PaletteWidget::refresh);

        // Refresh palette widgets when the palette is changed (loading a PCX file)
        QObject::connect(this->levelCelView, &LevelCelView::palModified, this->palWidget, &PaletteWidget::refresh);

        view = this->levelCelView;
    } else if (fileType == 4) {
        this->tblView = new TblView(this, this->undoStack);
        this->tblView->initialize(this->pal, this->tableset, this->bottomPanelHidden);

        // Refresh palette widgets when frame is changed
        QObject::connect(this->tblView, &TblView::frameRefreshed, this->palWidget, &PaletteWidget::refresh);

        view = this->tblView;
    } else if (fileType == 5) {
        this->cppView = new CppView(this, this->undoStack);
        this->cppView->initialize(this->cpp);

        view = this->cppView;
    } else {
        // build a CelView
        this->celView = new CelView(this);
        this->celView->initialize(this->pal, this->gfx, this->bottomPanelHidden);

        // Refresh palette widgets when frame is changed
        QObject::connect(this->celView, &CelView::frameRefreshed, this->palWidget, &PaletteWidget::refresh);

        // Refresh palette widgets when the palette is changed (loading a PCX file)
        QObject::connect(this->celView, &CelView::palModified, this->palWidget, &PaletteWidget::refresh);

        view = this->celView;
    }
    // Add the view to the main frame
    this->ui->mainFrameLayout->addWidget(view);

    // prepare the paint dialog
    if (fileType != 4 && fileType != 5) {
        this->paintWidget = new PaintWidget(this, this->undoStack, this->gfx, this->celView, this->levelCelView, this->gfxsetView);
        this->paintWidget->setPalette(this->trnBase->getResultingPalette());
    }

    // prepare the builder dialog
    if (this->dun != nullptr) {
        this->builderWidget = new BuilderWidget(this, this->undoStack, this->dun, this->levelCelView, this->tileset);
        // Refresh builder widget when the resource-options are changed
        QObject::connect(this->levelCelView, &LevelCelView::dunResourcesModified, this->builderWidget, &BuilderWidget::dunResourcesModified);
    }

    if (fileType != 5) {
    // Initialize palette widgets
    this->palHits = new D1PalHits(this->gfx, this->tileset);
    this->palWidget->initialize(this->pal, this->celView, this->levelCelView, this->gfxsetView, this->palHits);
    this->trnUniqueWidget->initialize(this->trnUnique, this->celView, this->levelCelView, this->gfxsetView, this->palHits);
    this->trnBaseWidget->initialize(this->trnBase, this->celView, this->levelCelView, this->gfxsetView, this->palHits);

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
    QString firstPaletteFound;
    if (fileType == 3) {
        firstPaletteFound = D1Pal::DEFAULT_PATH;
    } else if (fileType == 6 && this->pals.size() > 1) {
        firstPaletteFound = (this->pals.begin() + 1).key();
    }
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
    } else {
        this->cppView->displayFrame();
    }
    // update available menu entries
    this->ui->menuEdit->setEnabled(fileType != 4);
    this->ui->menuView->setEnabled(fileType != 5);
    this->ui->menuColors->setEnabled(fileType != 5);
    this->ui->menuData->setEnabled(fileType == 5);
    this->ui->actionExport->setEnabled(fileType != 4 && fileType != 5);
    this->ui->actionLoad->setEnabled(this->celView != nullptr || this->levelCelView != nullptr);
    this->ui->actionSave->setEnabled(true);
    this->ui->actionSaveAs->setEnabled(true);
    this->ui->actionClose->setEnabled(true);

    this->ui->menuFrame->setEnabled(fileType != 4 && fileType != 5);
    this->ui->menuSubtile->setEnabled(isTileset);
    this->ui->menuTile->setEnabled(isTileset);
    this->ui->actionPatch->setEnabled(this->celView != nullptr);
    this->ui->actionResize->setEnabled(this->celView != nullptr || this->gfxsetView != nullptr);
    this->ui->actionUpscale->setEnabled(fileType != 4 && fileType != 5);
    this->ui->actionMerge->setEnabled(fileType != 4 && fileType != 5);

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
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->insertImageFiles(mode, filePaths, append);
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
    if (!filePath.isEmpty() && this->tableset == nullptr && this->gfxset == nullptr && this->cpp == nullptr) {
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
            } else if (fileLower.endsWith(".smk")) {
                D1Smk::save(*this->gfx, params);
            } else {
                // Clear loading message from status bar
                ProgressDialog::done();
                QMessageBox::critical(this, tr("Error"), tr("Not supported."));
                return;
            }
        }
    }

    if (this->tileset != nullptr) {
        this->tileset->save(params);
    }

    if (this->gfxset != nullptr) {
        this->gfxset->save(params);
    }

    if (this->dun != nullptr) {
        this->dun->save(params);
    }

    if (this->tableset != nullptr) {
        this->tableset->save(params);
    }

    if (this->cpp != nullptr) {
        this->cpp->save(params);
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::resize(const ResizeParam &params)
{
    // ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Resizing..."), 1, PAF_UPDATE_WINDOW);

    if (this->celView != nullptr) {
        this->celView->resize(params);
    }
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->resize(params);
    }

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
        if (this->gfxsetView != nullptr) {
            this->gfxsetView->upscale(params);
        }
    };
    ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::ACTIVE, tr("Upscaling..."), 1, PAF_UPDATE_WINDOW, std::move(func));
}

void MainWindow::mergeFrames(const MergeFramesParam &params)
{
    if (this->celView != nullptr) {
        this->celView->mergeFrames(params);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->mergeFrames(params);
    }
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->mergeFrames(params);
    }
    this->updateWindow();
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

void MainWindow::addFrames(bool append)
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        QString filter = imageNameFilter();
        QStringList files = this->filesDialog(tr("Select Image Files"), filter.toLatin1().data());

        this->openImageFiles(IMAGE_FILE_MODE::FRAME, files, append);
        return;
    }
    if (this->celView != nullptr) {
        this->celView->createFrame(append);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->createFrame(append);
    }
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->createFrame(append);
    }
    this->updateWindow();
}

void MainWindow::addSubtiles(bool append)
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        QString filter = imageNameFilter();
        QStringList files = this->filesDialog(tr("Select Image Files"), filter.toLatin1().data());

        this->openImageFiles(IMAGE_FILE_MODE::SUBTILE, files, append);
        return;
    }
    this->levelCelView->createSubtile(append);
    this->updateWindow();
}

void MainWindow::addTiles(bool append)
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        QString filter = imageNameFilter();
        QStringList files = this->filesDialog(tr("Select Image Files"), filter.toLatin1().data());

        this->openImageFiles(IMAGE_FILE_MODE::TILE, files, append);
        return;
    }

    this->levelCelView->createTile(append);
    this->updateWindow();
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
        && (this->tableset == nullptr || this->tableset->distTbl->getFilePath().isEmpty() || this->tableset->darkTbl->getFilePath().isEmpty())
        && (this->cpp == nullptr || this->cpp->getFilePath().isEmpty())) {
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
    this->saveAsDialog->initialize(this->gfx, this->tileset, this->gfxset, this->dun, this->tableset, this->cpp);
    this->saveAsDialog->show();
}

void MainWindow::on_actionClose_triggered()
{
    this->undoStack->clear();

    MemFree(this->paintWidget);
    MemFree(this->builderWidget);
    MemFree(this->celView);
    MemFree(this->levelCelView);
    MemFree(this->gfxsetView);
    MemFree(this->tblView);
    MemFree(this->cppView);
    MemFree(this->palWidget);
    MemFree(this->trnUniqueWidget);
    MemFree(this->trnBaseWidget);
    MemFree(this->gfx);
    MemFree(this->tileset);
    MemFree(this->gfxset);
    MemFree(this->dun);
    MemFree(this->tableset);
    MemFree(this->cpp);

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
    this->ui->menuData->setEnabled(false);
    this->ui->actionExport->setEnabled(false);
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

void MainWindow::on_actionMerge_Frame_triggered()
{
    if (this->mergeFramesDialog == nullptr) {
        this->mergeFramesDialog = new MergeFramesDialog(this);
    }
    this->mergeFramesDialog->initialize(this->gfx);
    this->mergeFramesDialog->show();
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
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->addToCurrentFrame(imgFilePath);
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionCreate_Frame_triggered()
{
    this->addFrames(true);
}

void MainWindow::on_actionInsert_Frame_triggered()
{
    this->addFrames(false);
}

void MainWindow::on_actionDuplicate_Frame_triggered()
{
    const bool wholeGroup = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->celView != nullptr) {
        this->celView->duplicateCurrentFrame(wholeGroup);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->duplicateCurrentFrame(wholeGroup);
    }
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->duplicateCurrentFrame(wholeGroup);
    }
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
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->replaceCurrentFrame(imgFilePath);
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionDel_Frame_triggered()
{
    const bool wholeGroup = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->celView != nullptr) {
        this->celView->removeCurrentFrame(wholeGroup);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->removeCurrentFrame(wholeGroup);
    }
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->removeCurrentFrame(wholeGroup);
    }
    this->updateWindow();
}

void MainWindow::on_actionCreate_Subtile_triggered()
{
    this->addSubtiles(true);
}

void MainWindow::on_actionInsert_Subtile_triggered()
{
    this->addSubtiles(false);
}

void MainWindow::on_actionDuplicate_Subtile_triggered()
{
    const bool deepCopy = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    this->levelCelView->duplicateCurrentSubtile(deepCopy);
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

void MainWindow::on_actionInsert_Tile_triggered()
{
    this->addTiles(false);
}

void MainWindow::on_actionDuplicate_Tile_triggered()
{
    const bool deepCopy = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    this->levelCelView->duplicateCurrentTile(deepCopy);
}

void MainWindow::on_actionCreate_Tile_triggered()
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
    if (this->celView != nullptr) {
        this->celView->toggleBottomPanel();
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->toggleBottomPanel();
    }
    if (this->gfxsetView != nullptr) {
        this->gfxsetView->toggleBottomPanel();
    }
    if (this->tblView != nullptr) {
        this->tblView->toggleBottomPanel();
    }
}

void MainWindow::on_actionPatch_triggered()
{
    if (this->patchGfxDialog == nullptr) {
        this->patchGfxDialog = new PatchGfxDialog(this);
    }
    this->patchGfxDialog->initialize(this->gfx, this->celView);
    this->patchGfxDialog->show();
}

void MainWindow::on_actionResize_triggered()
{
    if (this->resizeDialog == nullptr) {
        this->resizeDialog = new ResizeDialog(this);
    }
    this->resizeDialog->initialize(this->gfxset);
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

void MainWindow::on_actionMerge_triggered()
{
    QStringList gfxFilePaths = this->filesDialog(tr("Open Graphics"), tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2)"));

    D1Gfx *gfx = nullptr;
    for (const QString &filePath : gfxFilePaths) {
        // load the gfx
        OpenAsParam params = OpenAsParam();
        QString fileLower = filePath.toLower();
        delete gfx;
        gfx = new D1Gfx();
        gfx->setPalette(this->trnBase->getResultingPalette());
        if (fileLower.endsWith(".cel")) {
            if (!D1Cel::load(*gfx, filePath, params)) {
                dProgressErr() << tr("Failed loading CEL file: %1.").arg(QDir::toNativeSeparators(filePath));
                continue;
            }
        } else { // CL2 (?)
            if (!D1Cl2::load(*gfx, filePath, params)) {
                dProgressErr() << tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(filePath));
                continue;
            }
        }
        // merge with the current content
        this->gfx->addGfx(gfx);
    }
    delete gfx;
    this->updateWindow();
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
    this->patchTilesetDialog->initialize(this->tileset, this->dun, this->levelCelView);
    this->patchTilesetDialog->show();
}

void MainWindow::on_actionLightSubtiles_Tileset_triggered()
{
    this->levelCelView->lightSubtiles();
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

void MainWindow::on_actionRemoveTiles_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->removeTiles();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionLoadTiles_Dungeon_triggered()
{
    D1Dun *srcDun = this->loadDun(tr("Source of the tiles"));
    if (srcDun == nullptr) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->loadTiles(srcDun);
    delete srcDun;

    // Clear loading message from status bar
    ProgressDialog::done();
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

void MainWindow::on_actionDecorate_Dungeon_triggered()
{
    this->levelCelView->decorateDungeon();
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

void MainWindow::on_actionDisplay_Colors_triggered()
{
    if (this->paletteShowDialog == nullptr) {
        this->paletteShowDialog = new PaletteShowDialog(this);
    }
    this->paletteShowDialog->initialize(this->pal);
    this->paletteShowDialog->show();
}

void MainWindow::on_actionRemap_Colors_triggered()
{
    if (this->remapDialog == nullptr) {
        this->remapDialog = new RemapDialog(this);
    }
    this->remapDialog->initialize(this->palWidget);
    this->remapDialog->show();
}

void MainWindow::on_actionSmack_Colors_triggered()
{
    QList<quint8> colors;
    D1Smk::fixColors(this->pal, colors);

    if (!colors.isEmpty()) {
        QList<QPair<D1GfxPixel, D1GfxPixel>> replacements;
        for (quint8 colIdx : colors) {
            QColor col = this->pal->getColor(colIdx);
            for (int i = 0; i < D1PAL_COLORS; i++) {
                if (this->pal->getColor(i) == col && i != colIdx) {
                    replacements.push_back(QPair<D1GfxPixel, D1GfxPixel>(D1GfxPixel::colorPixel(colIdx), D1GfxPixel::colorPixel(i)));
                    break;
                }
            }
        }
        RemapParam params;
        params.frames.first = 0;
        params.frames.second = 0;
        this->changeColors(replacements, params);
    }
}

void MainWindow::on_actionGenTrns_Colors_triggered()
{
    if (this->trnGenerateDialog == nullptr) {
        this->trnGenerateDialog = new TrnGenerateDialog(this);
    }
    this->trnGenerateDialog->initialize(this->pal);
    this->trnGenerateDialog->show();
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

void MainWindow::on_actionLoadTrns_Colors_triggered()
{
    QString trsFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Load Translation-Set File"), tr("TRS Files (*.trs *.TRS)"));

    if (trsFilePath.isEmpty()) {
        return;
    }

    std::vector<D1Trn *> trns;
    if (!D1Trs::load(trsFilePath, this->pal, trns)) {
        return;
    }

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
    if (trns.empty()) {
        this->setUniqueTrn(D1Trn::IDENTITY_PATH);
        return;
    }
    // load the TRN files
    for (D1Trn *trn : trns) {
        this->uniqueTrns[trn->getFilePath()] = trn;
    }
    this->setUniqueTrn(trns[0]->getFilePath());
}

void MainWindow::on_actionSaveTrns_Colors_triggered()
{
    std::vector<D1Trn *> trns;
    for (auto it = this->uniqueTrns.cbegin(); it != this->uniqueTrns.cend(); it++) {
        if (MainWindow::isResourcePath(it.key())) {
            continue;
        }
        trns.push_back(it.value());
    }

    if (trns.empty()) {
        QMessageBox::critical(this, tr("Error"), tr("Built-in TRN files can not be saved to a translation set."));
        return;
    }

    QString trsFilePath = this->fileDialog(FILE_DIALOG_MODE::SAVE_CONF, tr("Save Translation-Set File as..."), tr("TRS Files (*.trs *.TRS)"));

    if (!trsFilePath.isEmpty()) {
        D1Trs::save(trsFilePath, trns);
    }
}

void MainWindow::on_actionAddColumn_Table_triggered()
{
    this->cppView->on_actionAddColumn_triggered();
    // this->updateWindow(); -- done by cppView
}

void MainWindow::on_actionInsertColumn_Table_triggered()
{
    this->cppView->on_actionInsertColumn_triggered();
}

void MainWindow::on_actionDelColumn_Table_triggered()
{
    this->cppView->on_actionDelColumn_triggered();
}

void MainWindow::on_actionHideColumn_Table_triggered()
{
    this->cppView->on_actionHideColumn_triggered();
}

void MainWindow::on_actionMoveLeftColumn_Table_triggered()
{
    this->cppView->on_actionMoveLeftColumn_triggered();
}

void MainWindow::on_actionMoveRightColumn_Table_triggered()
{
    this->cppView->on_actionMoveRightColumn_triggered();
}

void MainWindow::on_actionDelColumns_Table_triggered()
{
    this->cppView->on_actionDelColumns_triggered();
}

void MainWindow::on_actionHideColumns_Table_triggered()
{
    this->cppView->on_actionHideColumns_triggered();
}

void MainWindow::on_actionShowColumns_Table_triggered()
{
    this->cppView->on_actionShowColumns_triggered();
}

void MainWindow::on_actionAddRow_Table_triggered()
{
    this->cppView->on_actionAddRow_triggered();
}

void MainWindow::on_actionInsertRow_Table_triggered()
{
    this->cppView->on_actionInsertRow_triggered();
}

void MainWindow::on_actionDelRow_Table_triggered()
{
    this->cppView->on_actionDelRow_triggered();
}

void MainWindow::on_actionHideRow_Table_triggered()
{
    this->cppView->on_actionHideRow_triggered();
}

void MainWindow::on_actionMoveUpRow_Table_triggered()
{
    this->cppView->on_actionMoveUpRow_triggered();
}

void MainWindow::on_actionMoveDownRow_Table_triggered()
{
    this->cppView->on_actionMoveDownRow_triggered();
}

void MainWindow::on_actionDelRows_Table_triggered()
{
    this->cppView->on_actionDelRows_triggered();
}

void MainWindow::on_actionHideRows_Table_triggered()
{
    this->cppView->on_actionHideRows_triggered();
}

void MainWindow::on_actionShowRows_Table_triggered()
{
    this->cppView->on_actionShowRows_triggered();
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
