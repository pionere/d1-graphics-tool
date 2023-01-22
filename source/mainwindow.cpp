#include "mainwindow.h"

#include <QApplication>
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
#include "ui_mainwindow.h"

MainWindow::MainWindow()
    : QMainWindow(nullptr)
    , ui(new Ui::MainWindow())
{
    // QCoreApplication::setAttribute( Qt::AA_EnableHighDpiScaling, true );

    this->lastFilePath = Config::getLastFilePath();

    ui->setupUi(this);

    this->setWindowTitle(D1_GRAPHICS_TOOL_TITLE);

    // initialize 'new' submenu of 'File'
    this->newMenu.addAction("CEL gfx", this, SLOT(on_actionNew_CEL_triggered()));
    this->newMenu.addAction("CL2 gfx", this, SLOT(on_actionNew_CL2_triggered()));
    this->newMenu.addAction("Tileset", this, SLOT(on_actionNew_Tileset_triggered()));
    QAction *firstFileAction = (QAction *)this->ui->menuFile->actions()[0];
    this->ui->menuFile->insertMenu(firstFileAction, &this->newMenu);

    // Initialize 'Undo/Redo' of 'Edit
    this->undoStack = new QUndoStack(this);
    this->undoAction = undoStack->createUndoAction(this);
    this->undoAction->setShortcuts(QKeySequence::Undo);
    this->redoAction = undoStack->createRedoAction(this);
    this->redoAction->setShortcuts(QKeySequence::Redo);
    this->ui->menuEdit->addAction(this->undoAction);
    this->ui->menuEdit->addAction(this->redoAction);
    this->ui->menuEdit->addSeparator();

    // Initialize 'Frame' submenu of 'Edit'
    this->frameMenu.setToolTipsVisible(true);
    this->frameMenu.addAction("Insert", this, SLOT(on_actionInsert_Frame_triggered()));
    this->frameMenu.addAction("Add", this, SLOT(on_actionAdd_Frame_triggered()));
    this->frameMenu.addAction("Replace", this, SLOT(on_actionReplace_Frame_triggered()));
    this->frameMenu.addAction("Delete", this, SLOT(on_actionDel_Frame_triggered()));
    this->ui->menuEdit->addMenu(&this->frameMenu);

    // Initialize 'Subtile' submenu of 'Edit'
    this->subtileMenu.setToolTipsVisible(true);
    this->subtileMenu.addAction("Create", this, SLOT(on_actionCreate_Subtile_triggered()));
    this->subtileMenu.addAction("Insert", this, SLOT(on_actionInsert_Subtile_triggered()));
    this->subtileMenu.addAction("Add", this, SLOT(on_actionAdd_Subtile_triggered()));
    this->subtileMenu.addAction("Replace", this, SLOT(on_actionReplace_Subtile_triggered()));
    this->subtileMenu.addAction("Delete", this, SLOT(on_actionDel_Subtile_triggered()));
    this->ui->menuEdit->addMenu(&this->subtileMenu);

    // Initialize 'Tile' submenu of 'Edit'
    this->tileMenu.setToolTipsVisible(true);
    this->tileMenu.addAction("Create", this, SLOT(on_actionCreate_Tile_triggered()));
    this->tileMenu.addAction("Insert", this, SLOT(on_actionInsert_Tile_triggered()));
    this->tileMenu.addAction("Add", this, SLOT(on_actionAdd_Tile_triggered()));
    this->tileMenu.addAction("Replace", this, SLOT(on_actionReplace_Tile_triggered()));
    this->tileMenu.addAction("Delete", this, SLOT(on_actionDel_Tile_triggered()));
    this->ui->menuEdit->addMenu(&this->tileMenu);

    // add Upscale option to 'Edit'
    this->ui->menuEdit->addSeparator();
    this->ui->menuEdit->addAction("Upscale", this, SLOT(on_actionUpscale_triggered()));

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
    delete this->undoStack;
    delete this->undoAction;
    delete this->redoAction;
}

void MainWindow::setPal(QString path)
{
    this->pal = this->pals[path];
    this->trnUnique->setPalette(this->pal);
    this->trnUnique->refreshResultingPalette();
    this->trnBase->refreshResultingPalette();

    this->palWidget->setPal(this->pal);
}

void MainWindow::setUniqueTrn(QString path)
{
    this->trnUnique = this->uniqueTrns[path];
    this->trnUnique->setPalette(this->pal);
    this->trnUnique->refreshResultingPalette();
    this->trnBase->setPalette(this->trnUnique->getResultingPalette());
    this->trnBase->refreshResultingPalette();

    this->trnUniqueWidget->setTrn(this->trnUnique);
}

void MainWindow::setBaseTrn(QString path)
{
    this->trnBase = this->baseTrns[path];
    this->trnBase->setPalette(this->trnUnique->getResultingPalette());
    this->trnBase->refreshResultingPalette();

    this->gfx->setPalette(this->trnBase->getResultingPalette());

    this->trnBaseWidget->setTrn(this->trnBase);
}

QString MainWindow::getLastFilePath()
{
    return this->lastFilePath;
}

void MainWindow::updateWindow()
{
    // rebuild palette hits
    this->palHits->update();
    this->palWidget->refresh();
    this->undoStack->clear();
    // update menu options
    bool hasFrame = this->gfx->getFrameCount() != 0;
    this->frameMenu.actions()[2]->setEnabled(hasFrame); // replace frame
    this->frameMenu.actions()[3]->setEnabled(hasFrame); // delete frame
    if (this->levelCelView != nullptr) {
        bool hasSubtile = this->min->getSubtileCount() != 0;
        this->subtileMenu.actions()[3]->setEnabled(hasSubtile); // replace subtile
        this->subtileMenu.actions()[4]->setEnabled(hasSubtile); // delete subtile
        this->tileMenu.actions()[0]->setEnabled(hasSubtile);    // create tile
        bool hasTile = this->til->getTileCount() != 0;
        this->tileMenu.actions()[3]->setEnabled(hasTile); // replace tile
        this->tileMenu.actions()[4]->setEnabled(hasTile); // delete tile
    }
}

bool MainWindow::loadPal(QString palFilePath)
{
    QFileInfo palFileInfo(palFilePath);
    // QString path = palFileInfo.absoluteFilePath();
    QString &path = palFilePath;
    QString name = palFileInfo.fileName();

    D1Pal *newPal = new D1Pal();
    if (!newPal->load(path)) {
        delete newPal;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading PAL file."));
        return false;
    }

    if (this->pals.contains(path))
        delete this->pals[path];
    this->pals[path] = newPal;
    this->palWidget->addPath(path, name);
    return true;
}

bool MainWindow::loadUniqueTrn(QString trnFilePath)
{
    QFileInfo trnFileInfo(trnFilePath);
    // QString path = trnFileInfo.absoluteFilePath();
    QString &path = trnFilePath;
    QString name = trnFileInfo.fileName();

    D1Trn *newTrn = new D1Trn(this->pal);
    if (!newTrn->load(path)) {
        delete newTrn;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading TRN file."));
        return false;
    }

    if (this->uniqueTrns.contains(path))
        delete this->uniqueTrns[path];
    this->uniqueTrns[path] = newTrn;
    this->trnUniqueWidget->addPath(path, name);
    return true;
}

bool MainWindow::loadBaseTrn(QString trnFilePath)
{
    QFileInfo trnFileInfo(trnFilePath);
    // QString path = trnFileInfo.absoluteFilePath();
    QString &path = trnFilePath;
    QString name = trnFileInfo.fileName();

    D1Trn *newTrn = new D1Trn(this->pal);
    if (!newTrn->load(path)) {
        delete newTrn;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading TRN file."));
        return false;
    }

    if (this->baseTrns.contains(path))
        delete this->baseTrns[path];
    this->baseTrns[path] = newTrn;
    this->trnBaseWidget->addPath(path, name);
    return true;
}

void MainWindow::pixelClicked(const D1GfxPixel &pixel)
{
    this->palWidget->selectColor(pixel);
    this->trnUniqueWidget->selectColor(pixel);
    this->trnBaseWidget->selectColor(pixel);
}

void MainWindow::colorModified()
{
    if (this->levelCelView != nullptr) {
        this->levelCelView->displayFrame();
    } else {
        this->celView->displayFrame();
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

static QString prepareFilePath(QString filePath, const QString &filter)
{
    if (!filePath.isEmpty()) {
        // filter file-name unless it matches the filter
        QString pattern = QString(filter);
        pattern = pattern.mid(pattern.lastIndexOf('(', pattern.length() - 1) + 1, -1);
        pattern.chop(1);
        QStringList patterns = pattern.split(QRegularExpression(" "), Qt::SkipEmptyParts);
        bool match = false;
        for (int i = 0; i < patterns.size(); i++) {
            pattern = patterns.at(i);
            // convert filter to regular expression
            for (int n = 0; n < pattern.size(); n++) {
                if (pattern[n] == '*') {
                    // convert * to .*
                    pattern.insert(n, '.');
                    n++;
                } else if (pattern[n] == '.') {
                    // convert . to \.
                    pattern.insert(n, '\\');
                    n++;
                }
            }
            QRegularExpression re(pattern);
            QRegularExpressionMatch qmatch = re.match(filePath);
            match |= qmatch.hasMatch();
        }
        if (!match) {
            QFileInfo fi(filePath);
            filePath = fi.absolutePath();
        }
    }
    return filePath;
}

QString MainWindow::fileDialog(FILE_DIALOG_MODE mode, const QString &title, const QString &filter)
{
    QString filePath = prepareFilePath(this->lastFilePath, filter);

    if (mode == FILE_DIALOG_MODE::OPEN) {
        filePath = QFileDialog::getOpenFileName(this, title, filePath, filter);
    } else {
        filePath = QFileDialog::getSaveFileName(this, title, filePath, filter, nullptr, mode == FILE_DIALOG_MODE::SAVE_NO_CONF ? QFileDialog::DontConfirmOverwrite : QFileDialog::Options());
    }

    if (!filePath.isEmpty()) {
        this->lastFilePath = filePath;
    }
    return filePath;
}

QStringList MainWindow::filesDialog(const QString &title, const QString &filter)
{
    QString filePath = prepareFilePath(this->lastFilePath, filter);

    QStringList filePaths = QFileDialog::getOpenFileNames(this, title, filePath, filter);

    if (!filePaths.isEmpty()) {
        this->lastFilePath = filePaths[0];
    }
    return filePaths;
}

bool MainWindow::hasImageUrl(const QMimeData *mimeData)
{
    const QByteArrayList supportedMimeTypes = QImageReader::supportedMimeTypes();
    QMimeDatabase mimeDB;

    for (const QUrl &url : mimeData->urls()) {
        QMimeType mimeType = mimeDB.mimeTypeForFile(url.toLocalFile());
        for (const QByteArray &mimeTypeName : supportedMimeTypes) {
            if (mimeType.inherits(mimeTypeName)) {
                return true;
            }
        }
    }
    return false;
}

void MainWindow::on_actionNew_CEL_triggered()
{
    OpenAsParam params;
    params.isTileset = OPEN_TILESET_TYPE::FALSE;
    params.clipped = OPEN_CLIPPED_TYPE::FALSE;
    this->openFile(params);
}

void MainWindow::on_actionNew_CL2_triggered()
{
    OpenAsParam params;
    params.isTileset = OPEN_TILESET_TYPE::FALSE;
    params.clipped = OPEN_CLIPPED_TYPE::TRUE;
    this->openFile(params);
}

void MainWindow::on_actionNew_Tileset_triggered()
{
    OpenAsParam params;
    params.isTileset = OPEN_TILESET_TYPE::TRUE;
    this->openFile(params);
}

void MainWindow::on_actionOpen_triggered()
{
    QString openFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Open Graphics"), tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2)"));

    if (!openFilePath.isEmpty()) {
        OpenAsParam params;
        params.celFilePath = openFilePath;
        this->openFile(params);
    }

    // QMessageBox::information( this, "Debug", celFilePath );
    // QMessageBox::information( this, "Debug", QString::number(cel->getFrameCount()));

    // QTime timer = QTime();
    // timer.start();
    // QMessageBox::information( this, "time", QString::number(timer.elapsed()) );
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    this->dragMoveEvent(event);
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    for (const QUrl &url : event->mimeData()->urls()) {
        QString path = url.toLocalFile().toLower();
        if (path.endsWith(".cel") || path.endsWith(".cl2")) {
            event->acceptProposedAction();
            return;
        }
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    OpenAsParam params;
    for (const QUrl &url : event->mimeData()->urls()) {
        params.celFilePath = url.toLocalFile();
        this->openFile(params);
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
        { // (re)translate the 'new' menu
            this->newMenu.setTitle(tr("New"));
            QList<QAction *> menuActions = this->newMenu.actions();
            menuActions[0]->setText(tr("CEL gfx"));
            menuActions[1]->setText(tr("CL2 gfx"));
            menuActions[2]->setText(tr("Tileset"));
        }
        // (re)translate undoAction, redoAction
        this->undoAction->setText(tr("Undo"));
        this->redoAction->setText(tr("Redo"));
        { // (re)translate 'Frame' submenu of 'Edit'
            this->frameMenu.setTitle(tr("Frame"));
            QList<QAction *> menuActions = this->frameMenu.actions();
            menuActions[0]->setText(tr("Insert"));
            menuActions[0]->setToolTip(tr("Add new frames before the current one"));
            menuActions[1]->setText(tr("Add"));
            menuActions[1]->setToolTip(tr("Add new frames at the end"));
            menuActions[2]->setText(tr("Replace"));
            menuActions[2]->setToolTip(tr("Replace the current frame"));
            menuActions[3]->setText(tr("Delete"));
            menuActions[3]->setToolTip(tr("Delete the current frame"));
        }
        { // (re)translate 'Subtile' submenu of 'Edit'
            this->subtileMenu.setTitle(tr("Subtile"));
            QList<QAction *> menuActions = this->subtileMenu.actions();
            menuActions[0]->setText(tr("Create"));
            menuActions[0]->setToolTip(tr("Create a new subtile"));
            menuActions[1]->setText(tr("Insert"));
            menuActions[1]->setToolTip(tr("Add new subtiles before the current one"));
            menuActions[2]->setText(tr("Add"));
            menuActions[2]->setToolTip(tr("Add new subtiles at the end"));
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
            menuActions[2]->setText(tr("Add"));
            menuActions[2]->setToolTip(tr("Add new tiles at the end"));
            menuActions[3]->setText(tr("Replace"));
            menuActions[3]->setToolTip(tr("Replace the current tile"));
            menuActions[4]->setText(tr("Delete"));
            menuActions[4]->setToolTip(tr("Delete the current tile"));
        }
        { // (re)translate Upscale option of 'Edit'
            QList<QAction *> menuActions = this->ui->menuEdit->actions();
            menuActions.back()->setText(tr("Upscale"));
            menuActions.back()->setToolTip(tr("Upscale the current graphics"));
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::openFile(const OpenAsParam &params)
{
    QString openFilePath = params.celFilePath;

    // Check file extension
    if (!openFilePath.isEmpty()
        && !openFilePath.toLower().endsWith(".cel")
        && !openFilePath.toLower().endsWith(".cl2")) {
        return;
    }

    this->on_actionClose_triggered();

    this->ui->statusBar->showMessage(tr("Loading..."));
    this->ui->statusBar->repaint();

    // Loading default.pal
    D1Pal *newPal = new D1Pal();
    newPal->load(D1Pal::DEFAULT_PATH);
    this->pals[D1Pal::DEFAULT_PATH] = newPal;
    this->pal = newPal;

    // Loading default null.trn
    D1Trn *newTrn = new D1Trn(this->pal);
    newTrn->load(D1Trn::IDENTITY_PATH);
    this->uniqueTrns[D1Trn::IDENTITY_PATH] = newTrn;
    this->trnUnique = newTrn;
    newTrn = new D1Trn(this->trnUnique->getResultingPalette());
    newTrn->load(D1Trn::IDENTITY_PATH);
    this->baseTrns[D1Trn::IDENTITY_PATH] = newTrn;
    this->trnBase = newTrn;

    QFileInfo celFileInfo = QFileInfo(openFilePath);

    // If a SOL, MIN and TIL files exists then build a LevelCelView
    QString basePath = celFileInfo.absolutePath() + "/" + celFileInfo.completeBaseName();
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
        isTileset = openFilePath.toLower().endsWith(".cel") && QFileInfo::exists(tilFilePath) && QFileInfo::exists(minFilePath) && QFileInfo::exists(solFilePath);
    }

    this->gfx = new D1Gfx();
    this->gfx->setPalette(this->trnBase->getResultingPalette());
    if (isTileset) {
        // Loading SOL
        this->sol = new D1Sol();
        if (!this->sol->load(solFilePath)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed loading SOL file: %1.").arg(minFilePath));
            return;
        }

        // Loading MIN
        this->min = new D1Min();
        std::map<unsigned, D1CEL_FRAME_TYPE> celFrameTypes;
        if (!this->min->load(minFilePath, this->gfx, this->sol, celFrameTypes, params)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed loading MIN file: %1.").arg(minFilePath));
            return;
        }

        // Loading TIL
        this->til = new D1Til();
        if (!this->til->load(tilFilePath, this->min)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed loading TIL file: %1.").arg(tilFilePath));
            return;
        }

        // Loading AMP
        this->amp = new D1Amp();
        QString ampFilePath = params.ampFilePath;
        if (!openFilePath.isEmpty() && ampFilePath.isEmpty()) {
            ampFilePath = basePath + ".amp";
        }
        if (!this->amp->load(ampFilePath, this->til->getTileCount(), params)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed loading AMP file: %1.").arg(ampFilePath));
            return;
        }

        // Loading TMI
        this->tmi = new D1Tmi();
        QString tmiFilePath = params.tmiFilePath;
        if (!openFilePath.isEmpty() && tmiFilePath.isEmpty()) {
            tmiFilePath = basePath + ".tmi";
        }
        if (!this->tmi->load(tmiFilePath, this->sol, params)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed loading TMI file: %1.").arg(tmiFilePath));
            return;
        }

        // Loading CEL
        if (!D1CelTileset::load(*this->gfx, celFrameTypes, openFilePath, params)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed loading Tileset-CEL file: %1.").arg(openFilePath));
            return;
        }
    } else if (openFilePath.toLower().endsWith(".cel")) {
        if (!D1Cel::load(*this->gfx, openFilePath, params)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed loading CEL file: %1.").arg(openFilePath));
            return;
        }
    } else if (openFilePath.toLower().endsWith(".cl2")) {
        if (!D1Cl2::load(*this->gfx, openFilePath, params)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed loading CL2 file: %1.").arg(openFilePath));
            return;
        }
    } else {
        // openFilePath.isEmpty()
        this->gfx->setType(params.clipped == OPEN_CLIPPED_TYPE::TRUE ? D1CEL_TYPE::V2_MONO_GROUP : D1CEL_TYPE::V1_REGULAR);
    }

    // Add palette widgets for PAL and TRNs
    this->palWidget = new PaletteWidget(this->undoStack, tr("Palette"));
    this->trnUniqueWidget = new PaletteWidget(this->undoStack, tr("Unique translation"));
    this->trnBaseWidget = new PaletteWidget(this->undoStack, tr("Base Translation"));
    this->ui->palFrame->layout()->addWidget(this->palWidget);
    this->ui->palFrame->layout()->addWidget(this->trnUniqueWidget);
    this->ui->palFrame->layout()->addWidget(this->trnBaseWidget);

    // Configuration update triggers refresh of the translator and the palette widgets
    QObject::connect(&this->settingsDialog, &SettingsDialog::configurationSaved, this, &MainWindow::reloadConfig);

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

    if (isTileset) {
        // build a LevelCelView
        this->levelCelView = new LevelCelView();
        this->levelCelView->initialize(this->gfx, this->min, this->til, this->sol, this->amp, this->tmi);

        // Select color when level CEL view clicked
        QObject::connect(this->levelCelView, &LevelCelView::pixelClicked, this, &MainWindow::pixelClicked);

        // Refresh palette widgets when frame, subtile of tile is changed
        QObject::connect(this->levelCelView, &LevelCelView::frameRefreshed, this->palWidget, &PaletteWidget::refresh);
    } else {
        // build a CelView
        this->celView = new CelView();
        this->celView->initialize(this->gfx);

        // Select color when CEL view clicked
        QObject::connect(this->celView, &CelView::pixelClicked, this, &MainWindow::pixelClicked);

        // Refresh palette widgets when frame
        QObject::connect(this->celView, &CelView::frameRefreshed, this->palWidget, &PaletteWidget::refresh);
    }

    // Initialize palette widgets
    this->palHits = new D1PalHits(this->gfx, this->min, this->til);
    this->palWidget->initialize(this->pal, this->celView, this->levelCelView, this->palHits);
    this->trnUniqueWidget->initialize(this->pal, this->trnUnique, this->celView, this->levelCelView, this->palHits);
    this->trnBaseWidget->initialize(this->trnUnique->getResultingPalette(), this->trnBase, this->celView, this->levelCelView, this->palHits);

    // Refresh the view if a PAL or TRN is modified
    QObject::connect(this->palWidget, &PaletteWidget::modified, this, &MainWindow::colorModified);
    QObject::connect(this->trnUniqueWidget, &PaletteWidget::modified, this, &MainWindow::colorModified);
    QObject::connect(this->trnBaseWidget, &PaletteWidget::modified, this, &MainWindow::colorModified);

    // Look for all palettes in the same folder as the CEL/CL2 file
    QDirIterator it(celFileInfo.absolutePath(), QStringList("*.pal"), QDir::Files);
    QString firstPaletteFound = QString();
    while (it.hasNext()) {
        QString sPath = it.next();

        if (this->loadPal(sPath) && firstPaletteFound.isEmpty()) {
            firstPaletteFound = sPath;
        }
    }
    if (firstPaletteFound.isEmpty()) {
        firstPaletteFound = D1Pal::DEFAULT_PATH;
    }
    this->palWidget->selectPath(firstPaletteFound); // should trigger view->displayFrame()

    // Adding the CelView to the main frame
    this->ui->mainFrame->layout()->addWidget(isTileset ? (QWidget *)this->levelCelView : this->celView);

    // Adding the PalView to the pal frame
    // this->ui->palFrame->layout()->addWidget( this->palView );

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

    this->updateWindow();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::openImageFiles(IMAGE_FILE_MODE mode, QStringList filePaths, bool append)
{
    if (filePaths.isEmpty()) {
        return;
    }

    this->ui->statusBar->showMessage(tr("Reading..."));
    this->ui->statusBar->repaint();

    if (this->celView != nullptr) {
        this->celView->insertImageFiles(mode, filePaths, append);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->insertImageFiles(mode, filePaths, append);
    }
    this->updateWindow();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::openPalFiles(QStringList filePaths, PaletteWidget *widget)
{
    QString firstFound;

    this->ui->statusBar->showMessage(tr("Reading..."));
    this->ui->statusBar->repaint();

    if (widget == this->palWidget) {
        for (QString path : filePaths) {
            if (this->loadPal(path) && firstFound.isEmpty()) {
                firstFound = path;
            }
        }
        if (!firstFound.isEmpty()) {
            this->palWidget->selectPath(firstFound);
        }
    } else if (widget == this->trnUniqueWidget) {
        for (QString path : filePaths) {
            if (this->loadUniqueTrn(path) && firstFound.isEmpty()) {
                firstFound = path;
            }
        }
        if (!firstFound.isEmpty()) {
            this->trnUniqueWidget->selectPath(firstFound);
        }
    } else if (widget == this->trnBaseWidget) {
        for (QString path : filePaths) {
            if (this->loadBaseTrn(path) && firstFound.isEmpty()) {
                firstFound = path;
            }
        }
        if (!firstFound.isEmpty()) {
            this->trnBaseWidget->selectPath(firstFound);
        }
    }

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::saveFile(const SaveAsParam &params)
{
    this->ui->statusBar->showMessage(tr("Saving..."));
    this->ui->statusBar->repaint();

    bool change = false;
    QString filePath = params.celFilePath.isEmpty() ? this->gfx->getFilePath() : params.celFilePath;
    if (this->gfx->getType() == D1CEL_TYPE::V1_LEVEL) {
        if (!filePath.toLower().endsWith("cel")) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to save as %1? Data conversion is not supported.").arg(filePath), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                // Clear loading message from status bar
                this->ui->statusBar->clearMessage();
                return;
            }
        }
        change = D1CelTileset::save(*this->gfx, params);
    } else {
        if (filePath.toLower().endsWith("cel")) {
            change = D1Cel::save(*this->gfx, params);
        } else if (filePath.toLower().endsWith("cl2")) {
            change = D1Cl2::save(*this->gfx, params);
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Not supported."));
            // Clear loading message from status bar
            this->ui->statusBar->clearMessage();
            return;
        }
    }

    if (this->min != nullptr) {
        change |= this->min->save(params);
    }
    if (this->til != nullptr) {
        change |= this->til->save(params);
    }
    if (this->sol != nullptr) {
        change |= this->sol->save(params);
    }
    if (this->amp != nullptr) {
        change |= this->amp->save(params);
    }
    if (this->tmi != nullptr) {
        change |= this->tmi->save(params);
    }

    if (change) {
        // update view
        if (this->celView != nullptr) {
            this->celView->initialize(this->gfx);
        }
        if (this->levelCelView != nullptr) {
            this->levelCelView->initialize(this->gfx, this->min, this->til, this->sol, this->amp, this->tmi);
        }
    }

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::upscale(const UpscaleParam &params)
{
    this->ui->statusBar->showMessage(tr("Upscaling..."));
    this->ui->statusBar->repaint();

    if (this->celView != nullptr) {
        this->celView->upscale(params);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->upscale(params);
    }

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
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
    this->openAsDialog.initialize();
    this->openAsDialog.show();
}

void MainWindow::on_actionSave_triggered()
{
    if (this->gfx->getFilePath().isEmpty()) {
        this->on_actionSaveAs_triggered();
        return;
    }
    SaveAsParam params;
    this->saveFile(params);
}

void MainWindow::on_actionSaveAs_triggered()
{
    this->saveAsDialog.initialize(this->gfx, this->min, this->til, this->sol, this->amp, this->tmi);
    this->saveAsDialog.show();
}

void MainWindow::on_actionClose_triggered()
{
    this->undoStack->clear();

    delete this->celView;
    delete this->levelCelView;
    delete this->palWidget;
    delete this->trnUniqueWidget;
    delete this->trnBaseWidget;
    delete this->gfx;

    qDeleteAll(this->pals);
    this->pals.clear();

    qDeleteAll(this->uniqueTrns);
    this->uniqueTrns.clear();

    qDeleteAll(this->baseTrns);
    this->baseTrns.clear();

    delete this->min;
    delete this->til;
    delete this->sol;
    delete this->amp;
    delete this->tmi;
    delete this->palHits;

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
    this->settingsDialog.initialize();
    this->settingsDialog.show();
}

void MainWindow::on_actionExport_triggered()
{
    this->exportDialog.initialize(this->gfx, this->min, this->til, this->sol, this->amp);
    this->exportDialog.show();
}

void MainWindow::on_actionQuit_triggered()
{
    qApp->quit();
}

void MainWindow::on_actionInsert_Frame_triggered()
{
    this->addFrames(false);
}

void MainWindow::on_actionAdd_Frame_triggered()
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

    this->ui->statusBar->showMessage(tr("Reading..."));
    this->ui->statusBar->repaint();

    if (this->celView != nullptr) {
        this->celView->replaceCurrentFrame(imgFilePath);
    }
    if (this->levelCelView != nullptr) {
        this->levelCelView->replaceCurrentFrame(imgFilePath);
    }
    this->updateWindow();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
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

void MainWindow::on_actionAdd_Subtile_triggered()
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

    this->ui->statusBar->showMessage(tr("Reading..."));
    this->ui->statusBar->repaint();

    this->levelCelView->replaceCurrentSubtile(imgFilePath);

    this->updateWindow();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
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

void MainWindow::on_actionAdd_Tile_triggered()
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

    this->ui->statusBar->showMessage(tr("Reading..."));
    this->ui->statusBar->repaint();

    this->levelCelView->replaceCurrentTile(imgFilePath);

    this->updateWindow();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::on_actionDel_Tile_triggered()
{
    this->levelCelView->removeCurrentTile();
    this->updateWindow();
}

void MainWindow::on_actionUpscale_triggered()
{
    this->upscaleDialog.initialize(this->gfx);
    this->upscaleDialog.show();
}

void MainWindow::on_actionReportUse_Tileset_triggered()
{
    this->levelCelView->reportUsage();
}

void MainWindow::on_actionResetFrameTypes_Tileset_triggered()
{
    this->ui->statusBar->showMessage(tr("Processing..."));
    this->ui->statusBar->repaint();

    this->levelCelView->resetFrameTypes();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::on_actionInefficientFrames_Tileset_triggered()
{
    this->ui->statusBar->showMessage(tr("Processing..."));
    this->ui->statusBar->repaint();

    this->levelCelView->inefficientFrames();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::on_actionCleanupFrames_Tileset_triggered()
{
    this->ui->statusBar->showMessage(tr("Processing..."));
    this->ui->statusBar->repaint();

    this->levelCelView->cleanupFrames();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::on_actionCleanupSubtiles_Tileset_triggered()
{
    this->ui->statusBar->showMessage(tr("Processing..."));
    this->ui->statusBar->repaint();

    this->levelCelView->cleanupSubtiles();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::on_actionCleanupTileset_Tileset_triggered()
{
    this->ui->statusBar->showMessage(tr("Processing..."));
    this->ui->statusBar->repaint();

    this->levelCelView->cleanupTileset();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::on_actionCompressSubtiles_Tileset_triggered()
{
    this->ui->statusBar->showMessage(tr("Processing..."));
    this->ui->statusBar->repaint();

    this->levelCelView->compressSubtiles();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::on_actionCompressTiles_Tileset_triggered()
{
    this->ui->statusBar->showMessage(tr("Processing..."));
    this->ui->statusBar->repaint();

    this->levelCelView->compressTiles();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::on_actionCompressTileset_Tileset_triggered()
{
    this->ui->statusBar->showMessage(tr("Processing..."));
    this->ui->statusBar->repaint();

    this->levelCelView->compressTileset();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::on_actionSortFrames_Tileset_triggered()
{
    this->ui->statusBar->showMessage(tr("Processing..."));
    this->ui->statusBar->repaint();

    this->levelCelView->sortFrames();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::on_actionSortSubtiles_Tileset_triggered()
{
    this->ui->statusBar->showMessage(tr("Processing..."));
    this->ui->statusBar->repaint();

    this->levelCelView->sortSubtiles();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::on_actionSortTileset_Tileset_triggered()
{
    this->ui->statusBar->showMessage(tr("Processing..."));
    this->ui->statusBar->repaint();

    this->levelCelView->sortTileset();

    // Clear loading message from status bar
    this->ui->statusBar->clearMessage();
}

void MainWindow::on_actionNew_PAL_triggered()
{
    QString palFilePath = this->fileDialog(FILE_DIALOG_MODE::SAVE_CONF, tr("New Palette File"), tr("PAL Files (*.pal *.PAL)"));

    if (palFilePath.isEmpty()) {
        return;
    }

    QFileInfo palFileInfo(palFilePath);
    QString path = palFileInfo.absoluteFilePath();
    QString name = palFileInfo.fileName();

    D1Pal *newPal = new D1Pal();
    if (!newPal->load(D1Pal::DEFAULT_PATH)) {
        delete newPal;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading PAL file."));
        return;
    }
    if (!newPal->save(path)) {
        delete newPal;
        return;
    }

    if (this->pals.contains(path))
        delete this->pals[path];
    this->pals[path] = newPal;
    this->palWidget->addPath(path, name);
    this->palWidget->selectPath(path);
}

void MainWindow::on_actionOpen_PAL_triggered()
{
    QString palFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Load Palette File"), tr("PAL Files (*.pal *.PAL)"));

    if (!palFilePath.isEmpty() && this->loadPal(palFilePath)) {
        this->palWidget->selectPath(palFilePath);
    }
}

void MainWindow::on_actionSave_PAL_triggered()
{
    QString selectedPath = this->palWidget->getSelectedPath();
    if (selectedPath == D1Pal::DEFAULT_PATH) {
        this->on_actionSave_PAL_as_triggered();
    } else {
        this->pal->save(selectedPath);
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

    QFileInfo palFileInfo(palFilePath);
    QString path = palFileInfo.absoluteFilePath();
    QString name = palFileInfo.fileName();

    D1Pal *newPal = new D1Pal();
    if (!newPal->load(path)) {
        delete newPal;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading PAL file."));
        return;
    }

    if (this->pals.contains(path))
        delete this->pals[path];
    this->pals[path] = newPal;
    this->palWidget->addPath(path, name);
    this->palWidget->selectPath(path);
}

void MainWindow::on_actionClose_PAL_triggered()
{
    QString selectedPath = this->palWidget->getSelectedPath();
    if (selectedPath == D1Pal::DEFAULT_PATH)
        return;

    if (this->pals.contains(selectedPath)) {
        delete this->pals[selectedPath];
        this->pals.remove(selectedPath);
    }

    this->palWidget->removePath(selectedPath);
    this->palWidget->selectPath(D1Pal::DEFAULT_PATH);
}

void MainWindow::on_actionNew_Translation_Unique_triggered()
{
    QString trnFilePath = this->fileDialog(FILE_DIALOG_MODE::SAVE_CONF, tr("New Translation File"), tr("TRN Files (*.trn *.TRN)"));

    if (trnFilePath.isEmpty()) {
        return;
    }

    QFileInfo trnFileInfo(trnFilePath);
    QString path = trnFileInfo.absoluteFilePath();
    QString name = trnFileInfo.fileName();

    D1Trn *newTrn = new D1Trn(this->pal);
    if (!newTrn->load(D1Trn::IDENTITY_PATH)) {
        delete newTrn;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading TRN file."));
        return;
    }
    if (!newTrn->save(path)) {
        delete newTrn;
        return;
    }

    if (this->uniqueTrns.contains(path))
        delete this->uniqueTrns[path];
    this->uniqueTrns[path] = newTrn;
    this->trnUniqueWidget->addPath(path, name);
    this->trnUniqueWidget->selectPath(path);
}

void MainWindow::on_actionOpen_Translation_Unique_triggered()
{
    QString trnFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Load Translation File"), tr("TRN Files (*.trn *.TRN)"));

    if (!trnFilePath.isEmpty() && this->loadUniqueTrn(trnFilePath)) {
        this->trnUniqueWidget->selectPath(trnFilePath);
    }
}

void MainWindow::on_actionSave_Translation_Unique_triggered()
{
    QString selectedPath = this->trnUniqueWidget->getSelectedPath();
    if (selectedPath == D1Trn::IDENTITY_PATH) {
        this->on_actionSave_Translation_Unique_as_triggered();
    } else {
        if (!this->trnUnique->save(selectedPath)) {
            return;
        }
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

    QFileInfo trnFileInfo(trnFilePath);
    QString path = trnFileInfo.absoluteFilePath();
    QString name = trnFileInfo.fileName();

    D1Trn *newTrn = new D1Trn(this->pal);
    if (!newTrn->load(path)) {
        delete newTrn;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading TRN file."));
        return;
    }

    if (this->uniqueTrns.contains(path))
        delete this->uniqueTrns[path];
    this->uniqueTrns[path] = newTrn;
    this->trnUniqueWidget->addPath(path, name);
    this->trnUniqueWidget->selectPath(path);
}

void MainWindow::on_actionClose_Translation_Unique_triggered()
{
    QString selectedPath = this->trnUniqueWidget->getSelectedPath();
    if (selectedPath == D1Trn::IDENTITY_PATH)
        return;

    if (this->uniqueTrns.contains(selectedPath)) {
        delete this->uniqueTrns[selectedPath];
        this->uniqueTrns.remove(selectedPath);
    }

    this->trnUniqueWidget->removePath(selectedPath);
    this->trnUniqueWidget->selectPath(D1Trn::IDENTITY_PATH);
}

void MainWindow::on_actionNew_Translation_Base_triggered()
{
    QString trnFilePath = this->fileDialog(FILE_DIALOG_MODE::SAVE_CONF, tr("New Translation File"), tr("TRN Files (*.trn *.TRN)"));

    if (trnFilePath.isEmpty()) {
        return;
    }

    QFileInfo trnFileInfo(trnFilePath);
    QString path = trnFileInfo.absoluteFilePath();
    QString name = trnFileInfo.fileName();

    D1Trn *newTrn = new D1Trn(this->trnUnique->getResultingPalette());
    if (!newTrn->load(D1Trn::IDENTITY_PATH)) {
        delete newTrn;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading TRN file."));
        return;
    }
    if (!newTrn->save(path)) {
        delete newTrn;
        return;
    }

    if (this->baseTrns.contains(path))
        delete this->baseTrns[path];
    this->baseTrns[path] = newTrn;
    this->trnBaseWidget->addPath(path, name);
    this->trnBaseWidget->selectPath(path);
}

void MainWindow::on_actionOpen_Translation_Base_triggered()
{
    QString trnFilePath = this->fileDialog(FILE_DIALOG_MODE::OPEN, tr("Load Translation File"), tr("TRN Files (*.trn *.TRN)"));

    if (!trnFilePath.isEmpty() && this->loadBaseTrn(trnFilePath)) {
        this->trnBaseWidget->selectPath(trnFilePath);
    }
}

void MainWindow::on_actionSave_Translation_Base_triggered()
{
    QString selectedPath = this->trnBaseWidget->getSelectedPath();
    if (selectedPath == D1Trn::IDENTITY_PATH) {
        this->on_actionSave_Translation_Base_as_triggered();
    } else {
        if (!this->trnBase->save(selectedPath)) {
            return;
        }
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

    QFileInfo trnFileInfo(trnFilePath);
    QString path = trnFileInfo.absoluteFilePath();
    QString name = trnFileInfo.fileName();

    D1Trn *newTrn = new D1Trn(this->trnUnique->getResultingPalette());
    if (!newTrn->load(path)) {
        delete newTrn;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading TRN file."));
        return;
    }

    if (this->baseTrns.contains(path))
        delete this->baseTrns[path];
    this->baseTrns[path] = newTrn;
    this->trnBaseWidget->addPath(path, name);
    this->trnBaseWidget->selectPath(path);
}

void MainWindow::on_actionClose_Translation_Base_triggered()
{
    QString selectedPath = this->trnBaseWidget->getSelectedPath();
    if (selectedPath == D1Trn::IDENTITY_PATH)
        return;

    if (this->baseTrns.contains(selectedPath)) {
        delete this->baseTrns[selectedPath];
        this->baseTrns.remove(selectedPath);
    }

    this->trnBaseWidget->removePath(selectedPath);
    this->trnBaseWidget->selectPath(D1Trn::IDENTITY_PATH);
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
