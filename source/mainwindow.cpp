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
#include "d1font.h"
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
    delete this->importDialog;
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
        D1GfxPixel source = ((unsigned)i >= D1PAL_COLORS) ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(i);
        D1GfxPixel replacement = ((unsigned)index >= D1PAL_COLORS) ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(index);
        replacements.push_back(QPair<D1GfxPixel, D1GfxPixel>(source, replacement));
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 0, PAF_UPDATE_WINDOW);

    if (this->gfxset != nullptr) {
        QList<D1Gfx *> &gfxs = this->gfxset->getGfxList();
        for (D1Gfx *gfx : gfxs) {
            gfx->replacePixels(replacements, params, 0);
        }
    } else {
        this->gfx->replacePixels(replacements, params, 0);
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
    // - Edit
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
    // - Data
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
    this->pointHovered(cell);
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
    if (this->tileset != nullptr) {
        this->tileset->gfx = gfx;
    }
    if (this->palHits != nullptr) {
        this->palHits->setGfx(gfx);
    }
    if (this->gfxset != nullptr) {
        this->gfxset->setGfx(gfx);
    }
    if (this->celView != nullptr) {
        this->celView->setGfx(gfx);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->setGfx(gfx);
    }
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

void MainWindow::on_actionImport_triggered()
{
    if (this->importDialog == nullptr) {
        this->importDialog = new ImportDialog(this);
    }
    this->importDialog->initialize(this->levelCelView != nullptr, this->palWidget);
    this->importDialog->show();
}

IMPORT_FILE_TYPE MainWindow::guessFileType(const QString& filePath, bool dunMode)
{
    QString fileLower = filePath.toLower();
    IMPORT_FILE_TYPE fileType;
    if (dunMode) {
        if (fileLower.endsWith("s.cel"))
            fileType = IMPORT_FILE_TYPE::SCEL;
        else if (fileLower.endsWith(".cel"))
            fileType = IMPORT_FILE_TYPE::CEL;
        else if (fileLower.endsWith(".min"))
            fileType = IMPORT_FILE_TYPE::MIN;
        else if (fileLower.endsWith(".til"))
            fileType = IMPORT_FILE_TYPE::TIL;
        else if (fileLower.endsWith(".sla"))
            fileType = IMPORT_FILE_TYPE::SLA;
        else if (fileLower.endsWith(".tla"))
            fileType = IMPORT_FILE_TYPE::TLA;
        else // if (fileLower.endsWith("dun")) // .dun or .rdun
            fileType = IMPORT_FILE_TYPE::DUNGEON;
    } else {
        if (fileLower.endsWith(".cl2"))
            fileType = IMPORT_FILE_TYPE::CL2;
        else if (fileLower.endsWith("tf"))    // .ttf or .otf
            fileType = IMPORT_FILE_TYPE::FONT;
        else if (fileLower.endsWith(".smk"))
            fileType = IMPORT_FILE_TYPE::SMK;
        else if (fileLower.endsWith(".pcx"))
            fileType = IMPORT_FILE_TYPE::PCX;
        else if (fileLower.endsWith(".tbl"))
            fileType = IMPORT_FILE_TYPE::TBL;
        else if (fileLower.endsWith(".cpp"))
            fileType = IMPORT_FILE_TYPE::CPP;
        else
            fileType = IMPORT_FILE_TYPE::CEL;
    }
    return fileType;
}

void MainWindow::importFile(const ImportParam &params)
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Importing..."), 0, PAF_NONE); // PAF_UPDATE_WINDOW

    QString gfxFilePath = params.filePath;

    IMPORT_FILE_TYPE fileType = params.fileType;
    if (fileType == IMPORT_FILE_TYPE::AUTODETECT) {
        fileType = MainWindow::guessFileType(gfxFilePath, this->levelCelView != nullptr);
    }

    OpenAsParam openParams = OpenAsParam();
    //if (fileType == IMPORT_FILE_TYPE::DUNGEON) {
        openParams.dunFilePath = gfxFilePath;
    //} else {
        openParams.celFilePath = gfxFilePath;
    //}

    if (this->levelCelView != nullptr) {
        if (fileType == IMPORT_FILE_TYPE::DUNGEON) {
            D1Dun *dun = new D1Dun();
            if (dun->load(openParams.dunFilePath, openParams)) {
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
                dProgressFail() << tr("Failed loading DUN file: %1.").arg(QDir::toNativeSeparators(openParams.celFilePath));
            }
        } else if (fileType == IMPORT_FILE_TYPE::MIN) {
            // Loading MIN
            std::map<unsigned, D1CEL_FRAME_TYPE> celFrameTypes;
            D1Min *min = new D1Min();
            if (min->load(openParams.celFilePath, this->tileset, celFrameTypes, openParams)) {
                delete this->tileset->min;
                this->tileset->min = min;
                if (this->dun != nullptr) {
                    this->dun->setTileset(this->tileset);
                }
                this->levelCelView->setTileset(this->tileset);
                this->updateWindow();
            } else {
                delete min;
                dProgressFail() << tr("Failed loading MIN file: %1.").arg(QDir::toNativeSeparators(openParams.celFilePath));
            }
        } else if (fileType == IMPORT_FILE_TYPE::TIL) {
            // Loading TIL
            D1Til *til = new D1Til();
            if (til->load(openParams.celFilePath, this->tileset->min)) {
                delete this->tileset->til;
                this->tileset->til = til;
                if (this->dun != nullptr) {
                    this->dun->setTileset(this->tileset);
                }
                this->levelCelView->setTileset(this->tileset);
                this->updateWindow();
            } else {
                delete til;
                dProgressFail() << tr("Failed loading TIL file: %1.").arg(QDir::toNativeSeparators(openParams.celFilePath));
            }
        } else if (fileType == IMPORT_FILE_TYPE::SLA) {
            // Loading SLA
            D1Sla *sla = new D1Sla();
            if (sla->load(openParams.celFilePath)) {
                delete this->tileset->sla;
                this->tileset->sla = sla;
                if (this->dun != nullptr) {
                    this->dun->setTileset(this->tileset);
                }
                this->levelCelView->setTileset(this->tileset);
                this->updateWindow();
            } else {
                delete sla;
                dProgressFail() << tr("Failed loading SLA file: %1.").arg(QDir::toNativeSeparators(openParams.celFilePath));
            }
        } else if (fileType == IMPORT_FILE_TYPE::TLA) {
            // Loading TLA
            D1Tla *tla = new D1Tla();
            if (tla->load(openParams.celFilePath, this->tileset->til->getTileCount(), openParams)) {
                delete this->tileset->tla;
                this->tileset->tla = tla;
                if (this->dun != nullptr) {
                    this->dun->setTileset(this->tileset);
                }
                this->levelCelView->setTileset(this->tileset);
                this->updateWindow();
            } else {
                delete tla;
                dProgressFail() << tr("Failed loading TLA file: %1.").arg(QDir::toNativeSeparators(openParams.celFilePath));
            }
        } else if (fileType == IMPORT_FILE_TYPE::SCEL) {
            // Loading sCEL
            D1Gfx *cls = new D1Gfx();
            cls->setPalette(this->trnBase->getResultingPalette());
            if (D1Cel::load(*cls, openParams.celFilePath, openParams)) { // this->tileset->loadCls
                delete this->tileset->cls;
                this->tileset->cls = cls;
                if (this->dun != nullptr) {
                    this->dun->setTileset(this->tileset);
                }
                this->levelCelView->setTileset(this->tileset);
                this->updateWindow();
            } else {
                delete cls;
                dProgressFail() << tr("Failed loading Special-CEL file: %1.").arg(QDir::toNativeSeparators(openParams.celFilePath));
            }
        } else if (fileType == IMPORT_FILE_TYPE::CEL) {
            // Loading (MIN and) CEL
            QString minFilePath = this->tileset->min->getFilePath();
            D1Min min;
            std::map<unsigned, D1CEL_FRAME_TYPE> celFrameTypes;
            if (min.load(minFilePath, this->tileset, celFrameTypes, openParams)) {
                D1Gfx* gfx = new D1Gfx();
                gfx->setPalette(this->trnBase->getResultingPalette());
                if (D1CelTileset::load(*gfx, celFrameTypes, openParams.celFilePath, openParams)) {
                    delete this->gfx;
                    this->gfxChanged(gfx);
                } else {
                    delete gfx;
                    dProgressFail() << tr("Failed loading Tileset-CEL file: %1.").arg(QDir::toNativeSeparators(openParams.celFilePath));
                }
            } else {
                dProgressFail() << tr("Failed loading MIN file: %1.").arg(QDir::toNativeSeparators(minFilePath));
            }
        }
    } else {
        D1Gfx *gfx = new D1Gfx();
        gfx->setPalette(this->trnBase->getResultingPalette());

        if ((fileType == IMPORT_FILE_TYPE::CEL && D1Cel::load(*gfx, openParams.celFilePath, openParams))
         || (fileType == IMPORT_FILE_TYPE::FONT && D1Font::load(*gfx, openParams.celFilePath, params))
         || D1Cl2::load(*gfx, openParams.celFilePath, openParams)) {
            delete this->gfx;
            this->gfxChanged(gfx);
        } else {
            delete gfx;
            dProgressFail() << tr("Failed loading GFX file: %1.").arg(QDir::toNativeSeparators(openParams.celFilePath));
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
        QMessageBox::critical(this, tr("Error"), tr("main cancel"));
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

static void closeFileContent(LoadFileContent *result)
{
    result->fileType = FILE_CONTENT::UNKNOWN;

    MemFree(result->trnUnique);
    MemFree(result->trnBase);
    MemFree(result->gfx);
    MemFree(result->tileset);
    MemFree(result->gfxset);
    MemFree(result->dun);
    MemFree(result->tableset);
    MemFree(result->cpp);

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
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_NONE); // PAF_UPDATE_WINDOW

    QString gfxFilePath = params.celFilePath;

    // Check file extension
    FILE_CONTENT fileType = FILE_CONTENT::EMPTY;
    if (!gfxFilePath.isEmpty()) {
        QString fileLower = gfxFilePath.toLower();
        if (fileLower.endsWith(".cel"))
            fileType = FILE_CONTENT::CEL;
        else if (fileLower.endsWith(".cl2"))
            fileType = FILE_CONTENT::CL2;
        else if (fileLower.endsWith(".pcx"))
            fileType = FILE_CONTENT::PCX;
        else if (fileLower.endsWith(".tbl"))
            fileType = FILE_CONTENT::TBL;
        else if (fileLower.endsWith(".cpp"))
            fileType = FILE_CONTENT::CPP;
        else if (fileLower.endsWith(".smk"))
            fileType = FILE_CONTENT::SMK;
        else
            fileType = FILE_CONTENT::UNKNOWN;
    }
    result->fileType = fileType;
    // result->fileType = fileType;
    // result->pal = nullptr;
    // result->trnUnique = nullptr;
    // result->trnBase = nullptr;
    result->gfx = nullptr;
    result->tileset = nullptr;
    result->gfxset = nullptr;
    result->dun = nullptr;
    result->tableset = nullptr;
    result->cpp = nullptr;
    if (fileType == FILE_CONTENT::UNKNOWN) {
        MainWindow::failWithError(nullptr, result, tr("Could not recognize file-type based on its extension."));
        return;
    }

    if (instance != nullptr) {
        instance->on_actionClose_triggered();
    }

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

    QString clsFilePath = params.clsFilePath;
    QString tilFilePath = params.tilFilePath;
    QString minFilePath = params.minFilePath;
    QString slaFilePath = params.slaFilePath;
    QString tlaFilePath = params.tlaFilePath;
    QString dunFilePath = params.dunFilePath;
    QString tblFilePath = params.tblFilePath;

    QString baseDir;
    if (fileType == FILE_CONTENT::TBL) {
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
                MainWindow::failWithError(instance, result, tr("Could not find the other table file for TBL file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
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
        if (!dunFilePath.isEmpty() && QFileInfo::exists(dunFilePath) && (fileType == FILE_CONTENT::CEL || fileType == FILE_CONTENT::EMPTY)) {
            fileType = FILE_CONTENT::DUN;
            isTileset = true;
        } else if (!isTileset && QFileInfo::exists(tilFilePath) && QFileInfo::exists(minFilePath) && fileType == FILE_CONTENT::CEL) {
            if (QFileInfo::exists(slaFilePath)) {
                isTileset = true;
            } else {
                dProgressWarn() << tr("Opening as standard CEL file because the SLA file (%1) is missing.").arg(QDir::toNativeSeparators(slaFilePath));
            }
        }
    }

    bool isGfxset = params.gfxType == OPEN_GFX_TYPE::GFXSET;
    result->isTileset = isTileset;
    result->isGfxset = isGfxset;
    result->baseDir = baseDir;

    result->gfx = new D1Gfx();
    result->gfx->setPalette(result->trnBase->getResultingPalette());
    if (isGfxset) {
        result->gfxset = new D1Gfxset(result->gfx);
        if (!result->gfxset->load(gfxFilePath, params)) {
            MainWindow::failWithError(instance, result, tr("Failed loading GFX file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
    } else if (isTileset) {
        result->tileset = new D1Tileset(result->gfx);
        // Loading SLA
        if (!result->tileset->sla->load(slaFilePath)) {
            MainWindow::failWithError(instance, result, tr("Failed loading SLA file: %1.").arg(QDir::toNativeSeparators(slaFilePath)));
            return;
        }

        // Loading MIN
        std::map<unsigned, D1CEL_FRAME_TYPE> celFrameTypes;
        if (!result->tileset->min->load(minFilePath, result->tileset, celFrameTypes, params)) {
            MainWindow::failWithError(instance, result, tr("Failed loading MIN file: %1.").arg(QDir::toNativeSeparators(minFilePath)));
            return;
        }

        // Loading TIL
        if (!result->tileset->til->load(tilFilePath, result->tileset->min)) {
            MainWindow::failWithError(instance, result, tr("Failed loading TIL file: %1.").arg(QDir::toNativeSeparators(tilFilePath)));
            return;
        }

        // Loading TLA
        if (!result->tileset->tla->load(tlaFilePath, result->tileset->til->getTileCount(), params)) {
            MainWindow::failWithError(instance, result, tr("Failed loading TLA file: %1.").arg(QDir::toNativeSeparators(tlaFilePath)));
            return;
        }

        // Loading CEL
        if (!D1CelTileset::load(*result->gfx, celFrameTypes, gfxFilePath, params)) {
            MainWindow::failWithError(instance, result, tr("Failed loading Tileset-CEL file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }

        // Loading sCEL
        if (!result->tileset->loadCls(clsFilePath, params)) {
            MainWindow::failWithError(instance, result, tr("Failed loading Special-CEL file: %1.").arg(QDir::toNativeSeparators(clsFilePath)));
            return;
        }

        // Loading DUN
        if (!dunFilePath.isEmpty() || params.createDun) {
            result->dun = new D1Dun();
            if (!result->dun->load(dunFilePath, params)) {
                MainWindow::failWithError(instance, result, tr("Failed loading DUN file: %1.").arg(QDir::toNativeSeparators(dunFilePath)));
                return;
            }
            result->dun->initialize(result->pal, result->tileset);
        }
    } else if (fileType == FILE_CONTENT::CEL) {
        if (!D1Cel::load(*result->gfx, gfxFilePath, params)) {
            MainWindow::failWithError(instance, result, tr("Failed loading CEL file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
    } else if (fileType == FILE_CONTENT::CL2) {
        if (!D1Cl2::load(*result->gfx, gfxFilePath, params)) {
            MainWindow::failWithError(instance, result, tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
    } else if (fileType == FILE_CONTENT::PCX) {
        if (!D1Pcx::load(*result->gfx, result->pal, gfxFilePath, params)) {
            MainWindow::failWithError(instance, result, tr("Failed loading PCX file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
    } else if (fileType == FILE_CONTENT::TBL) {
        result->tableset = new D1Tableset();
        if (!result->tableset->load(gfxFilePath, tblFilePath, params)) {
            MainWindow::failWithError(instance, result, tr("Failed loading TBL file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
    } else if (fileType == FILE_CONTENT::CPP) {
        result->cpp = new D1Cpp();
        if (!result->cpp->load(gfxFilePath)) {
            MainWindow::failWithError(instance, result, tr("Failed loading CPP file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
    } else if (fileType == FILE_CONTENT::SMK) {
        if (!D1Smk::load(*result->gfx, result->pals, gfxFilePath, params)) {
            MainWindow::failWithError(instance, result, tr("Failed loading SMK file: %1.").arg(QDir::toNativeSeparators(gfxFilePath)));
            return;
        }
        // add paths in palWidget
        // result->palWidget->updatePathComboBoxOptions(result->pals.keys(), result->pal->getFilePath()); -- should be called later
    } else {
        // gfxFilePath.isEmpty()
        result->gfx->setType(params.clipped == OPEN_CLIPPED_TYPE::TRUE ? D1CEL_TYPE::V2_MONO_GROUP : D1CEL_TYPE::V1_REGULAR);
    }
}

void MainWindow::openFile(const OpenAsParam &params)
{
    LoadFileContent fileContent;
    MainWindow::loadFile(params, this, &fileContent);
    if (fileContent.fileType == FILE_CONTENT::UNKNOWN)
        return;
    const FILE_CONTENT fileType = fileContent.fileType;
    const bool isTileset = fileContent.isTileset;
    const bool isGfxset = fileContent.isGfxset;
    const QString &baseDir = fileContent.baseDir;
    this->pal = fileContent.pal;
    this->trnUnique = fileContent.trnUnique;
    this->trnBase = fileContent.trnBase;
    this->gfx = fileContent.gfx;
    this->tileset = fileContent.tileset;
    this->gfxset = fileContent.gfxset;
    this->dun = fileContent.dun;
    this->tableset = fileContent.tableset;
    this->cpp = fileContent.cpp;

    this->pals = fileContent.pals;

    this->uniqueTrns[D1Trn::IDENTITY_PATH] = this->trnUnique;
    this->baseTrns[D1Trn::IDENTITY_PATH] = this->trnBase;

    if (fileType != FILE_CONTENT::CPP) {
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
    } else if (fileType == FILE_CONTENT::TBL) {
        this->tblView = new TblView(this, this->undoStack);
        this->tblView->initialize(this->pal, this->tableset, this->bottomPanelHidden);

        // Refresh palette widgets when frame is changed
        QObject::connect(this->tblView, &TblView::frameRefreshed, this->palWidget, &PaletteWidget::refresh);

        view = this->tblView;
    } else if (fileType == FILE_CONTENT::CPP) {
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
    if (fileType != FILE_CONTENT::TBL && fileType != FILE_CONTENT::CPP) {
        this->paintWidget = new PaintWidget(this, this->undoStack, this->gfx, this->celView, this->levelCelView, this->gfxsetView);
        this->paintWidget->setPalette(this->trnBase->getResultingPalette());
    }

    // prepare the builder dialog
    if (this->dun != nullptr) {
        this->builderWidget = new BuilderWidget(this, this->undoStack, this->dun, this->levelCelView, this->tileset);
        // Refresh builder widget when the resource-options are changed
        QObject::connect(this->levelCelView, &LevelCelView::dunResourcesModified, this->builderWidget, &BuilderWidget::dunResourcesModified);
    }

    if (fileType != FILE_CONTENT::CPP) {
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
    if (fileType == FILE_CONTENT::PCX) {
        firstPaletteFound = D1Pal::DEFAULT_PATH;
    } else if (fileType == FILE_CONTENT::SMK && this->gfx->getFrameCount() > 0) {
        D1GfxFrame *frame = this->gfx->getFrame(0);
        QPointer<D1Pal>& pal = frame->getFramePal();
        // assert(!pal.isNull());
        firstPaletteFound = pal->getFilePath();
        // assert(this->pals.contains(firstPaletteFound));
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
    this->ui->menuEdit->setEnabled(fileType != FILE_CONTENT::TBL);
    this->ui->menuView->setEnabled(fileType != FILE_CONTENT::CPP);
    this->ui->menuReports->setEnabled(fileType != FILE_CONTENT::TBL && fileType != FILE_CONTENT::CPP);
    this->ui->menuTileset->setEnabled(isTileset);
    this->ui->menuDungeon->setEnabled(this->dun != nullptr);
    this->ui->menuColors->setEnabled(fileType != FILE_CONTENT::CPP);
    this->ui->menuData->setEnabled(fileType == FILE_CONTENT::CPP);
    // - File
    this->ui->actionExport->setEnabled(fileType != FILE_CONTENT::TBL && fileType != FILE_CONTENT::CPP);
    this->ui->actionDiff->setEnabled(fileType != FILE_CONTENT::TBL && fileType != FILE_CONTENT::CPP);
    this->ui->actionImport->setEnabled(this->celView != nullptr || this->levelCelView != nullptr);
    this->ui->actionSave->setEnabled(true);
    this->ui->actionSaveAs->setEnabled(true);
    this->ui->actionClose->setEnabled(true);
    // - Edit
    this->ui->menuFrame->setEnabled(fileType != FILE_CONTENT::TBL && fileType != FILE_CONTENT::CPP);
    this->ui->menuSubtile->setEnabled(isTileset);
    this->ui->menuTile->setEnabled(isTileset);
    this->ui->actionPatch->setEnabled(this->celView != nullptr);
    this->ui->actionResize->setEnabled(this->celView != nullptr || this->gfxsetView != nullptr);
    this->ui->actionUpscale->setEnabled(this->gfx != nullptr);
    this->ui->actionMerge->setEnabled(this->gfx != nullptr);
    this->ui->actionMask->setEnabled(this->gfx != nullptr);
    this->ui->actionOptimize->setEnabled(this->celView != nullptr);
    // - Reports
    this->ui->actionReportBoundary->setEnabled(this->gfx != nullptr);
    this->ui->actionReportColoredFrames->setEnabled(this->gfx != nullptr);
    this->ui->actionReportColoredSubtiles->setEnabled(isTileset);
    this->ui->actionReportColoredTiles->setEnabled(isTileset);
    this->ui->actionReportActiveFrames->setEnabled(this->gfx != nullptr);
    this->ui->actionReportActiveSubtiles->setEnabled(isTileset);
    this->ui->actionReportActiveTiles->setEnabled(isTileset);
    this->ui->actionReportTilesetUse->setEnabled(isTileset);
    this->ui->actionReportTilesetInefficientFrames->setEnabled(isTileset);


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
    this->ui->menuReports->setEnabled(false);
    this->ui->menuDungeon->setEnabled(false);
    this->ui->menuColors->setEnabled(false);
    this->ui->menuData->setEnabled(false);
    this->ui->actionExport->setEnabled(false);
    this->ui->actionDiff->setEnabled(false);
    this->ui->actionImport->setEnabled(false);
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

QString MainWindow::FileContentTypeToStr(FILE_CONTENT fileType)
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
    QString filter;

    QString title = tr("Select Graphics");
    if (this->gfxset != nullptr) {
        filter = tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2)");
    } else if (this->tileset != nullptr) {
        filter = tr("CEL Files (*.cel *.CEL);;MIN Files (*.min *.MIN);;TIL Files (*.til *.TIL);;SLA Files (*.sla *.SLA);;TLA Files (*.tla *.TLA)");
        if (this->dun != nullptr) {
            title = tr("Select Dungeon or Graphics");
            filter = tr("DUN Files (*.dun *.DUN *.rdun *.RDUN)") + QString(";;") + filter;
        }
    // } else if (this->tableset != nullptr) {
    //    filter = tr("TBL Files (*.tbl *.TBL)");
    //} else if (this->cpp != nullptr) {
    //    filter = tr("CPP Files (*.cpp *.CPP *.c *.C)");
    } else {
        QString filePath = this->gfx->getFilePath();
        QString fileLower = filePath.toLower();
        if (fileLower.endsWith(".cel")) {
            filter = tr("CEL Files (*.cel *.CEL)");
        } else if (fileLower.endsWith(".cl2")) {
            filter = tr("CL2 Files (*.cl2 *.CL2)");
        } else if (fileLower.endsWith(".pcx")) {
            filter = tr("PCX Files (*.pcx *.PCX)");
        } else if (fileLower.endsWith(".smk")) {
            filter = tr("SMK Files (*.smk *.SMK)");
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Not supported."));
            return;
        }
    }

    QString filePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, title, filter);
    if (filePath.isEmpty()) {
        return;
    }

    IMPORT_FILE_TYPE fileType = MainWindow::guessFileType(filePath, this->tileset != nullptr);

    QString fileLower = filePath.toLower();
    bool main;
    switch (fileType) {
    case IMPORT_FILE_TYPE::MIN:
    case IMPORT_FILE_TYPE::TIL:
    case IMPORT_FILE_TYPE::SLA:
    case IMPORT_FILE_TYPE::TLA:
    case IMPORT_FILE_TYPE::SCEL: main = false; break;
    case IMPORT_FILE_TYPE::DUNGEON:
    case IMPORT_FILE_TYPE::CEL:
    case IMPORT_FILE_TYPE::CL2:
    case IMPORT_FILE_TYPE::FONT:
    case IMPORT_FILE_TYPE::SMK:
    case IMPORT_FILE_TYPE::PCX:
    case IMPORT_FILE_TYPE::TBL:
    case IMPORT_FILE_TYPE::CPP:
    default:                    main = true; break;
    }

    OpenAsParam params = OpenAsParam();
    if (fileType == IMPORT_FILE_TYPE::DUNGEON) {
        params.dunFilePath = filePath;
    } else {
        params.celFilePath = filePath;
    }
    LoadFileContent fileContent;
    if (main) {
        // first attempt with all-auto
        MainWindow::loadFile(params, nullptr, &fileContent);
        if (fileContent.fileType == FILE_CONTENT::UNKNOWN) {
            // second attempt using the current content
            if (this->tileset != nullptr) {
                params.minWidth = this->tileset->min->getSubtileWidth();
                params.minHeight = this->tileset->min->getSubtileHeight();
            } else if (this->gfx != nullptr && this->gfx->getFrameCount() != 0) {
                params.celWidth = this->gfx->getFrameWidth(0);
            } else {
                return;
            }
            MainWindow::loadFile(params, nullptr, &fileContent);
            if (fileContent.fileType == FILE_CONTENT::UNKNOWN)
                return;
        }
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Comparing..."), 0, PAF_OPEN_DIALOG);

    if (main) {
    if (this->gfxset != nullptr) {
        this->gfxset->compareTo(&fileContent);
    } else if (this->dun != nullptr && fileContent.fileType == FILE_CONTENT::DUN) {
        this->dun->compareTo(&fileContent);
    //} else if (this->tableset != nullptr) {
    //    this->tableset->compareTo(&fileContent);
    //} else if (this->cpp != nullptr) {
    //    this->cpp->compareTo(&fileContent);
    } else {
        QString header;
        switch (fileContent.fileType) {
        case FILE_CONTENT::EMPTY: dProgressErr() << tr("File is empty."); break;
        case FILE_CONTENT::CEL:
        case FILE_CONTENT::CL2: this->gfx->compareTo(fileContent.gfx, header); break;
        case FILE_CONTENT::PCX: D1Pcx::compare(*this->gfx, this->pal, &fileContent); break;
        case FILE_CONTENT::TBL: dProgressErr() << tr("Not a graphics file (%1)").arg(FileContentTypeToStr(FILE_CONTENT::TBL)); break;
        case FILE_CONTENT::CPP: dProgressErr() << tr("Not a graphics file (%1)").arg(FileContentTypeToStr(FILE_CONTENT::CPP)); break;
        case FILE_CONTENT::SMK: D1Smk::compare(*this->gfx, this->pals, &fileContent); break;
        case FILE_CONTENT::DUN: dProgressErr() << tr("Not a graphics file (%1)").arg(FileContentTypeToStr(FILE_CONTENT::DUN)); break;
        default: dProgressErr() << tr("Not supported."); break;
        }
    }

    closeFileContent(&fileContent);
    } else {
        switch (fileType) {
        case IMPORT_FILE_TYPE::MIN: {
            // Loading MIN
            D1Tileset tileset = D1Tileset(this->gfx);
            std::map<unsigned, D1CEL_FRAME_TYPE> celFrameTypes;
            if (tileset.min->load(params.celFilePath, this->tileset, celFrameTypes, params)) {
                this->tileset->min->compareTo(tileset.min, celFrameTypes);
            } else {
                dProgressFail() << tr("Failed loading MIN file: %1.").arg(QDir::toNativeSeparators(params.celFilePath));
            }
        } break;
        case IMPORT_FILE_TYPE::TIL: {
            // Loading TIL
            D1Til til = D1Til();
            D1Min min = D1Min();
            if (til.load(params.celFilePath, &min)) {
                this->tileset->til->compareTo(&til);
            } else {
                dProgressFail() << tr("Failed loading TIL file: %1.").arg(QDir::toNativeSeparators(params.celFilePath));
            }
        } break;
        case IMPORT_FILE_TYPE::SLA: {
            // Loading SLA
            D1Sla sla = D1Sla();
            if (sla.load(params.celFilePath)) {
                this->tileset->sla->compareTo(&sla);
            } else {
                dProgressFail() << tr("Failed loading SLA file: %1.").arg(QDir::toNativeSeparators(params.celFilePath));
            }
        } break;
        case IMPORT_FILE_TYPE::TLA: {
            // Loading TLA
            D1Tla tla = D1Tla();
            if (tla.load(params.celFilePath, -1, params)) {
                this->tileset->tla->compareTo(&tla);
            } else {
                dProgressFail() << tr("Failed loading TLA file: %1.").arg(QDir::toNativeSeparators(params.celFilePath));
            }
        } break;
        case IMPORT_FILE_TYPE::SCEL: {
            // Loading sCEL
            D1Gfx cls = D1Gfx();
            cls.setPalette(this->trnBase->getResultingPalette());
            if (D1Cel::load(cls, params.celFilePath, params)) { // tileset.loadCls
                QString header;
                this->tileset->cls->compareTo(&cls, header);
            } else {
                dProgressFail() << tr("Failed loading Special-CEL file: %1.").arg(QDir::toNativeSeparators(params.celFilePath));
            }
        } break;
        }
    }
    // Clear loading message from status bar
    ProgressDialog::done();
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

    if (gfxFilePaths.isEmpty()) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

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

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionMask_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 0, PAF_UPDATE_WINDOW);

    if (this->gfxset != nullptr)
        this->gfxset->mask();
    else
        this->gfx->mask();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionOptimize_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 0, PAF_UPDATE_WINDOW);

    this->gfx->optimize();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionReportBoundary_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    QRect rect = QRect();
    if (this->gfxset != nullptr)
        rect = this->gfxset->getBoundary();
    else
        rect = this->gfx->getBoundary();

    QString msg;
    if (!rect.isNull()) {
        msg = tr("The upper left of the bounding rectangle is %1:%2, the lower right corner is %3:%4. (width %5, height %6)")
            .arg(rect.x()).arg(rect.y()).arg(rect.x() + rect.width() - 1).arg(rect.y() + rect.height() - 1).arg(rect.width()).arg(rect.height());
    } else if (this->gfxset != nullptr) {
        msg = tr("The graphics is completely transparent.");
    } else {
        msg = tr("The graphics-set is completely transparent.");
    }
    dProgress() << msg;

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionReportColoredFrames_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    std::pair<int, int> colors = this->palWidget->getCurrentSelection();

    if (this->levelCelView != nullptr)
        this->levelCelView->coloredFrames(colors);

    if (this->celView != nullptr)
        this->celView->coloredFrames(colors);

    if (this->gfxsetView != nullptr)
        this->gfxsetView->coloredFrames(colors);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionReportColoredSubtiles_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    std::pair<int, int> colors = this->palWidget->getCurrentSelection();

    this->levelCelView->coloredSubtiles(colors);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionReportColoredTiles_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    std::pair<int, int> colors = this->palWidget->getCurrentSelection();

    this->levelCelView->coloredTiles(colors);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionReportActiveFrames_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    if (this->levelCelView != nullptr)
        this->levelCelView->activeFrames();

    if (this->celView != nullptr)
        this->celView->activeFrames();

    if (this->gfxsetView != nullptr)
        this->gfxsetView->activeFrames();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionReportActiveSubtiles_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->activeSubtiles();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionReportActiveTiles_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->activeTiles();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionReportTilesetInefficientFrames_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->inefficientFrames();

    // Clear loading message from status bar
    ProgressDialog::done();
}

void MainWindow::on_actionReportTilesetUse_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_OPEN_DIALOG);

    this->levelCelView->reportUsage();

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

void MainWindow::on_actionRemoveMissiles_Dungeon_triggered()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    this->levelCelView->removeMissiles();

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

void MainWindow::on_actionGen_PAL_triggered()
{
    QString filter = imageNameFilter();
    QString imgFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Image File"), filter.toLatin1().data());

    if (imgFilePath.isEmpty()) {
        return;
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Reading..."), 0, PAF_UPDATE_WINDOW);

    if (this->pal->genColors(imgFilePath)) {
        // updatePalette(this->pal);
        this->updateWindow();
    }

    // Clear loading message from status bar
    ProgressDialog::done();
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
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 0, PAF_UPDATE_WINDOW);

    QList<D1SmkColorFix> frameColorMods;
    D1Smk::fixColors(this->gfxset, this->gfx, this->pal, frameColorMods);

    if (!frameColorMods.isEmpty()) {
        // find possible replacement for the modified colors
        for (D1SmkColorFix &frameColorMod : frameColorMods) {
            QList<QPair<D1GfxPixel, D1GfxPixel>> replacements;
            D1Pal *pal = frameColorMod.pal;
            for (quint8 colIdx : frameColorMod.colors) {
                QColor col = pal->getColor(colIdx);
                unsigned i = 0; unsigned n = D1PAL_COLORS;
                for ( ; i < D1PAL_COLORS; i++) {
                    if (pal->getColor(i) != col/* || i == colIdx*/) {
                        continue;
                    }
                    auto it = frameColorMod.colors.begin();
                    for (; it != frameColorMod.colors.end(); it++) {
                        if (*it == i) {
                            break;
                        }
                    }
                    if (it == frameColorMod.colors.end()) {
                        break;
                    } else if (i < n) {
                        n = i;
                    }
                }
                if (i == D1PAL_COLORS) {
                    /*if (n == D1PAL_COLORS) {
                        // the color is still unique
                        continue;
                    }*/
                    // multiple colors were changed to the same new one -> use the lowest
                    if (n == colIdx) {
                        // no change -> skip
                        continue;
                    }
                    i = n;
                }
                replacements.push_back(QPair<D1GfxPixel, D1GfxPixel>(D1GfxPixel::colorPixel(colIdx), D1GfxPixel::colorPixel(i)));
            }
            if (!replacements.isEmpty()) {
                RemapParam params;
                params.frames.first = frameColorMod.frameFrom;
                params.frames.second = frameColorMod.frameTo;
                frameColorMod.gfx->replacePixels(replacements, params, this->gfxset != nullptr ? 2 : 1);
            }
        }
    }

    // Clear loading message from status bar
    ProgressDialog::done();
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

void MainWindow::on_actionDuplicateColumn_Table_triggered()
{
    this->cppView->on_actionDuplicateColumn_triggered();
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

void MainWindow::on_actionDuplicateRow_Table_triggered()
{
    this->cppView->on_actionDuplicateRow_triggered();
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
