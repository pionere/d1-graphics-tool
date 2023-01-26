#include "celview.h"

#include <algorithm>

#include <QAction>
#include <QDebug>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QImageReader>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>

#include "config.h"
#include "d1pcx.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_celview.h"
#include "upscaler.h"

CelScene::CelScene(QWidget *v)
    : QGraphicsScene()
    , view(v)
{
}

void CelScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    int x = event->scenePos().x();
    int y = event->scenePos().y();

    qDebug() << QStringLiteral("Clicked: %1:%2").arg(x).arg(y);

    emit this->framePixelClicked(x, y);
}

void CelScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    this->dragMoveEvent(event);
}

void CelScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (MainWindow::hasImageUrl(event->mimeData())) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void CelScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    event->acceptProposedAction();

    QStringList filePaths;
    for (const QUrl &url : event->mimeData()->urls()) {
        filePaths.append(url.toLocalFile());
    }
    // try to insert as frames
    ((MainWindow *)this->view->window())->openImageFiles(IMAGE_FILE_MODE::AUTO, filePaths, false);
}

void CelScene::contextMenuEvent(QContextMenuEvent *event)
{
    emit this->showContextMenu(event->globalPos());
}

void CelScene::zoomOut()
{
    if (this->currentZoomNumerator > 1 || this->currentZoomDenominator < ZOOM_LIMIT) {
        if (this->currentZoomNumerator > 1) {
            this->currentZoomNumerator--;
        } else {
            this->currentZoomDenominator++;
        }
        this->updateQGraphicsView();
    }
}

void CelScene::zoomIn()
{
    if (this->currentZoomDenominator > 1 || this->currentZoomNumerator < ZOOM_LIMIT) {
        if (this->currentZoomDenominator > 1) {
            this->currentZoomDenominator--;
        } else {
            this->currentZoomNumerator++;
        }
        this->updateQGraphicsView();
    }
}

void CelScene::setZoom(QString &zoom)
{
    CelScene::parseZoomValue(zoom, this->currentZoomNumerator, this->currentZoomDenominator);

    this->updateQGraphicsView();
}

QString CelScene::zoomText() const
{
    return QString::number(this->currentZoomNumerator) + ":" + QString::number(this->currentZoomDenominator);
}

void CelScene::updateQGraphicsView()
{
    qreal zoomFactor = (qreal)this->currentZoomNumerator / this->currentZoomDenominator;
    for (QGraphicsView *view : this->views()) {
        view->resetTransform();
        view->scale(zoomFactor, zoomFactor);
        view->show();
    }
}

void CelScene::parseZoomValue(QString &zoom, quint8 &zoomNumerator, quint8 &zoomDenominator)
{
    int sepIdx = zoom.indexOf(":");

    if (sepIdx >= 0) {
        if (sepIdx == 0) {
            zoomNumerator = 1;
            zoomDenominator = zoom.mid(1).toUShort();
        } else if (sepIdx == zoom.length() - 1) {
            zoom.chop(1);
            zoomNumerator = zoom.toUShort();
            zoomDenominator = 1;
        } else {
            zoomNumerator = zoom.mid(0, sepIdx).toUShort();
            zoomDenominator = zoom.mid(sepIdx + 1).toUShort();
        }
    } else {
        zoomNumerator = zoom.toUShort();
        zoomDenominator = 1;
    }
    if (zoomNumerator == 0) {
        zoomNumerator = 1;
    }
    if (zoomNumerator > ZOOM_LIMIT) {
        zoomNumerator = ZOOM_LIMIT;
    }
    if (zoomDenominator == 0) {
        zoomDenominator = 1;
    }
    if (zoomDenominator > ZOOM_LIMIT) {
        zoomDenominator = ZOOM_LIMIT;
    }
}

CelView::CelView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CelView())
    , celScene(new CelScene(this))
{
    this->ui->setupUi(this);
    this->ui->celGraphicsView->setScene(this->celScene);
    this->on_zoomEdit_escPressed();
    this->on_playDelayEdit_escPressed();
    this->ui->stopButton->setEnabled(false);
    this->playTimer.connect(&this->playTimer, SIGNAL(timeout()), this, SLOT(playGroup()));

    // If a pixel of the frame was clicked get pixel color index and notify the palette widgets
    QObject::connect(this->celScene, &CelScene::framePixelClicked, this, &CelView::framePixelClicked);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->frameIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_frameIndexEdit_escPressed()));
    QObject::connect(this->ui->groupIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_groupIndexEdit_escPressed()));
    QObject::connect(this->ui->zoomEdit, SIGNAL(cancel_signal()), this, SLOT(on_zoomEdit_escPressed()));
    QObject::connect(this->ui->playDelayEdit, SIGNAL(cancel_signal()), this, SLOT(on_playDelayEdit_escPressed()));

    // setup context menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));
    QObject::connect(this->celScene, &CelScene::showContextMenu, this, &CelView::ShowContextMenu);

    setAcceptDrops(true);
}

CelView::~CelView()
{
    delete ui;
    delete celScene;
}

void CelView::initialize(D1Pal *p, D1Gfx *g)
{
    this->pal = p;
    this->gfx = g;

    this->update();
}

void CelView::setPal(D1Pal *p)
{
    this->pal = p;
}

void CelView::update()
{
    // Displaying CEL file path information
    QFileInfo gfxFileInfo(this->gfx->getFilePath());
    QString label = gfxFileInfo.fileName();
    if (this->gfx->isModified()) {
        label += "*";
    }
    ui->celLabel->setText(label);

    ui->groupNumberEdit->setText(
        QString::number(this->gfx->getGroupCount()));

    ui->frameNumberEdit->setText(
        QString::number(this->gfx->getFrameCount()));
}

int CelView::getCurrentFrameIndex()
{
    return this->currentFrameIndex;
}

void CelView::framePixelClicked(unsigned x, unsigned y)
{
    D1GfxPixel pixel = D1GfxPixel::transparentPixel();

    if (this->gfx->getFrameCount() != 0) {
        D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);

        pixel = frame->getPixel(x - CEL_SCENE_SPACING, y - CEL_SCENE_SPACING);
    }

    emit this->pixelClicked(pixel);
}

void CelView::insertImageFiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append)
{
    int prevFrameCount = this->gfx->getFrameCount();

    if (append) {
        // append the frame(s)
        for (int i = 0; i < imagefilePaths.count(); i++) {
            this->insertFrame(mode, this->gfx->getFrameCount(), imagefilePaths[i]);
        }
        int deltaFrameCount = this->gfx->getFrameCount() - prevFrameCount;
        if (deltaFrameCount == 0) {
            return; // no new frame -> done
        }
        // jump to the first appended frame
        this->currentFrameIndex = prevFrameCount;
        this->updateGroupIndex();
    } else {
        // insert the frame(s)
        for (int i = imagefilePaths.count() - 1; i >= 0; i--) {
            this->insertFrame(mode, this->currentFrameIndex, imagefilePaths[i]);
        }
        int deltaFrameCount = this->gfx->getFrameCount() - prevFrameCount;
        if (deltaFrameCount == 0) {
            return; // no new frame -> done
        }
    }
    // update the view
    this->update();
    this->displayFrame();
}

void CelView::insertFrame(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool wasModified = this->gfx->isModified();
        bool clipped, palModified;
        D1GfxFrame *frame = this->gfx->insertFrame(index, &clipped);
        if (!D1Pcx::load(*frame, imagefilePath, clipped, this->pal, this->gfx->getPalette(), &palModified)) {
            this->gfx->removeFrame(index);
            this->gfx->setModified(wasModified);
        } else if (palModified) {
            // update the palette
            emit this->palModified();
        }
        return;
    }
    QImageReader reader = QImageReader(imagefilePath);
    int numImages = 0;

    while (true) {
        QImage image = reader.read();
        if (image.isNull()) {
            break;
        }
        this->gfx->insertFrame(index + numImages, image);
        numImages++;
    }

    if (mode != IMAGE_FILE_MODE::AUTO && numImages == 0) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to read file: %1.").arg(imagefilePath));
    }
}

void CelView::replaceCurrentFrame(const QString &imagefilePath)
{
    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        return;
    }

    this->gfx->replaceFrame(this->currentFrameIndex, image);

    // update the view
    this->update();
    this->displayFrame();
}

void CelView::removeCurrentFrame()
{
    // remove the frame
    this->gfx->removeFrame(this->currentFrameIndex);
    if (this->gfx->getFrameCount() == this->currentFrameIndex) {
        this->currentFrameIndex = std::max(0, this->currentFrameIndex - 1);
    }
    this->updateGroupIndex();
    // update the view
    this->update();
    this->displayFrame();
}

void CelView::upscale(const UpscaleParam &params)
{
    int amount = this->gfx->getFrameCount();

    ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Upscaling..."), amount + 1);

    if (Upscaler::upscaleGfx(this->gfx, params)) {
        // update the view
        this->update();
        this->displayFrame();
    }

    ProgressDialog::done();
}

void CelView::displayFrame()
{
    this->celScene->clear();

    // Getting the current frame to display
    QImage celFrame = this->gfx->getFrameImage(this->currentFrameIndex);

    this->celScene->setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    // Building background of the width/height of the CEL frame
    QImage celFrameBackground = QImage(celFrame.width(), celFrame.height(), QImage::Format_ARGB32);
    celFrameBackground.fill(QColor(Config::getGraphicsTransparentColor()));

    // Resize the scene rectangle to include some padding around the CEL frame
    this->celScene->setSceneRect(0, 0,
        celFrame.width() + CEL_SCENE_SPACING * 2,
        celFrame.height() + CEL_SCENE_SPACING * 2);
    // ui->celGraphicsView->adjustSize();
    // ui->celFrameWidget->adjustSize();

    // Add the backgrond and CEL frame while aligning it in the center
    this->celScene->addPixmap(QPixmap::fromImage(celFrameBackground))
        ->setPos(CEL_SCENE_SPACING, CEL_SCENE_SPACING);
    this->celScene->addPixmap(QPixmap::fromImage(celFrame))
        ->setPos(CEL_SCENE_SPACING, CEL_SCENE_SPACING);

    // Set current frame width and height
    this->ui->celFrameWidthEdit->setText(QString::number(celFrame.width()) + " px");
    this->ui->celFrameHeightEdit->setText(QString::number(celFrame.height()) + " px");

    // Set current group text
    this->ui->groupIndexEdit->setText(
        QString::number(this->gfx->getGroupCount() != 0 ? this->currentGroupIndex + 1 : 0));

    // Set current frame text
    this->ui->frameIndexEdit->setText(
        QString::number(this->gfx->getFrameCount() != 0 ? this->currentFrameIndex + 1 : 0));

    // Notify PalView that the frame changed (used to refresh palette hits)
    emit this->frameRefreshed();
}

void CelView::updateGroupIndex()
{
    QPair<quint16, quint16> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);

    if (this->currentFrameIndex < groupFrameIndices.first || this->currentFrameIndex > groupFrameIndices.second) {
        this->setGroupIndex();
    }
}

void CelView::setGroupIndex()
{
    QPair<quint16, quint16> groupFrameIndices;
    int i = 0;

    for (; i < this->gfx->getGroupCount(); i++) {
        groupFrameIndices = this->gfx->getGroupFrameIndices(i);

        if (this->currentFrameIndex >= groupFrameIndices.first
            && this->currentFrameIndex <= groupFrameIndices.second) {
            break;
        }
    }
    this->currentGroupIndex = i;
}

void CelView::playGroup()
{
    QPair<quint16, quint16> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);

    if (this->currentFrameIndex < groupFrameIndices.second)
        this->currentFrameIndex++;
    else
        this->currentFrameIndex = groupFrameIndices.first;

    MainWindow *mw = (MainWindow *)this->window();

    int cycleType = this->ui->playComboBox->currentIndex();
    if (cycleType == 0) {
        // normal playback
        this->displayFrame();
    } else {
        mw->nextPaletteCycle((D1PAL_CYCLE_TYPE)(cycleType - 1));
        // this->displayFrame();
    }
}

void CelView::ShowContextMenu(const QPoint &pos)
{
    MainWindow *mw = (MainWindow *)this->window();

    QMenu contextMenu(this);
    contextMenu.setToolTipsVisible(true);

    QAction action0(tr("Insert Frame"), this);
    action0.setToolTip(tr("Add new frames before the current one"));
    QObject::connect(&action0, SIGNAL(triggered()), mw, SLOT(on_actionInsert_Frame_triggered()));
    contextMenu.addAction(&action0);

    QAction action1(tr("Add Frame"), this);
    action1.setToolTip(tr("Add new frames at the end"));
    QObject::connect(&action1, SIGNAL(triggered()), mw, SLOT(on_actionAdd_Frame_triggered()));
    contextMenu.addAction(&action1);

    QAction action2(tr("Replace Frame"), this);
    action2.setToolTip(tr("Replace the current frame"));
    QObject::connect(&action2, SIGNAL(triggered()), mw, SLOT(on_actionReplace_Frame_triggered()));
    action2.setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&action2);

    QAction action3(tr("Del Frame"), this);
    action3.setToolTip(tr("Delete the current frame"));
    QObject::connect(&action3, SIGNAL(triggered()), mw, SLOT(on_actionDel_Frame_triggered()));
    action3.setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&action3);

    contextMenu.exec(mapToGlobal(pos));
}

void CelView::on_firstFrameButton_clicked()
{
    this->currentGroupIndex = 0;
    this->currentFrameIndex = 0;
    this->displayFrame();
}

void CelView::on_previousFrameButton_clicked()
{
    if (this->currentFrameIndex >= 1)
        this->currentFrameIndex--;
    else
        this->currentFrameIndex = std::max(0, this->gfx->getFrameCount() - 1);

    this->updateGroupIndex();
    this->displayFrame();
}

void CelView::on_nextFrameButton_clicked()
{
    if (this->currentFrameIndex < (this->gfx->getFrameCount() - 1))
        this->currentFrameIndex++;
    else
        this->currentFrameIndex = 0;

    this->updateGroupIndex();
    this->displayFrame();
}

void CelView::on_lastFrameButton_clicked()
{
    this->currentGroupIndex = std::max(0, this->gfx->getGroupCount() - 1);
    this->currentFrameIndex = std::max(0, this->gfx->getFrameCount() - 1);
    this->displayFrame();
}

void CelView::on_frameIndexEdit_returnPressed()
{
    int frameIndex = this->ui->frameIndexEdit->text().toInt() - 1;

    if (frameIndex >= 0 && frameIndex < this->gfx->getFrameCount()) {
        this->currentFrameIndex = frameIndex;
        this->updateGroupIndex();
        this->displayFrame();
    }
    this->on_frameIndexEdit_escPressed();
}

void CelView::on_frameIndexEdit_escPressed()
{
    this->ui->frameIndexEdit->setText(QString::number(this->gfx->getFrameCount() != 0 ? this->currentFrameIndex + 1 : 0));
    this->ui->frameIndexEdit->clearFocus();
}

void CelView::on_firstGroupButton_clicked()
{
    this->on_firstFrameButton_clicked();
}

void CelView::on_previousGroupButton_clicked()
{
    if (this->currentGroupIndex >= 1)
        this->currentGroupIndex--;
    else
        this->currentGroupIndex = std::max(0, this->gfx->getGroupCount() - 1);

    this->currentFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).first;
    this->displayFrame();
}

void CelView::on_groupIndexEdit_returnPressed()
{
    int groupIndex = this->ui->groupIndexEdit->text().toInt() - 1;

    if (groupIndex >= 0 && groupIndex < this->gfx->getGroupCount()) {
        this->currentGroupIndex = groupIndex;
        this->currentFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).first;
        this->displayFrame();
    }
    this->on_groupIndexEdit_escPressed();
}

void CelView::on_groupIndexEdit_escPressed()
{
    this->ui->groupIndexEdit->setText(QString::number(this->gfx->getGroupCount() != 0 ? this->currentGroupIndex + 1 : 0));
    this->ui->groupIndexEdit->clearFocus();
}

void CelView::on_nextGroupButton_clicked()
{
    if (this->currentGroupIndex < (this->gfx->getGroupCount() - 1))
        this->currentGroupIndex++;
    else
        this->currentGroupIndex = 0;

    this->currentFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).first;
    this->displayFrame();
}

void CelView::on_lastGroupButton_clicked()
{
    this->currentGroupIndex = std::max(0, this->gfx->getGroupCount() - 1);
    this->currentFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).first;
    this->displayFrame();
}

void CelView::on_zoomOutButton_clicked()
{
    this->celScene->zoomOut();
    this->on_zoomEdit_escPressed();
}

void CelView::on_zoomInButton_clicked()
{
    this->celScene->zoomIn();
    this->on_zoomEdit_escPressed();
}

void CelView::on_zoomEdit_returnPressed()
{
    QString zoom = this->ui->zoomEdit->text();

    this->celScene->setZoom(zoom);

    this->on_zoomEdit_escPressed();
}

void CelView::on_zoomEdit_escPressed()
{
    this->ui->zoomEdit->setText(this->celScene->zoomText());
    this->ui->zoomEdit->clearFocus();
}

void CelView::on_playDelayEdit_returnPressed()
{
    quint16 playDelay = this->ui->playDelayEdit->text().toUInt();

    if (playDelay != 0)
        this->currentPlayDelay = playDelay;

    this->on_playDelayEdit_escPressed();
}

void CelView::on_playDelayEdit_escPressed()
{
    this->ui->playDelayEdit->setText(QString::number(this->currentPlayDelay));
    this->ui->playDelayEdit->clearFocus();
}

void CelView::on_playButton_clicked()
{
    // disable the related fields
    this->ui->playButton->setEnabled(false);
    this->ui->playDelayEdit->setReadOnly(false);
    this->ui->playComboBox->setEnabled(false);
    // enable the stop button
    this->ui->stopButton->setEnabled(true);
    // preserve the palette
    ((MainWindow *)this->window())->initPaletteCycle();

    this->playTimer.start(this->currentPlayDelay);
}

void CelView::on_stopButton_clicked()
{
    this->playTimer.stop();

    // restore palette
    ((MainWindow *)this->window())->resetPaletteCycle();
    // disable the stop button
    this->ui->stopButton->setEnabled(false);
    // enable the related fields
    this->ui->playButton->setEnabled(true);
    this->ui->playDelayEdit->setReadOnly(true);
    this->ui->playComboBox->setEnabled(true);
}

void CelView::dragEnterEvent(QDragEnterEvent *event)
{
    this->dragMoveEvent(event);
}

void CelView::dragMoveEvent(QDragMoveEvent *event)
{
    if (MainWindow::hasImageUrl(event->mimeData())) {
        event->acceptProposedAction();
    }
}

void CelView::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    QStringList filePaths;
    for (const QUrl &url : event->mimeData()->urls()) {
        filePaths.append(url.toLocalFile());
    }
    // try to insert as frames
    ((MainWindow *)this->window())->openImageFiles(IMAGE_FILE_MODE::AUTO, filePaths, false);
}
