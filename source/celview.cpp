#include "celview.h"

#include <algorithm>

#include <QAction>
#include <QDebug>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QImageReader>
#include <QMenu>
#include <QMimeData>

#include "config.h"
#include "d1pcx.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_celview.h"
#include "upscaler.h"

CelScene::CelScene(QWidget *v)
    : QGraphicsScene(v)
    , view(v)
{
    // this->setStickyFocus(true);
}

void CelScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    this->lastPos = event->scenePos().toPoint();
    this->lastCounter = 0;

    qDebug() << QStringLiteral("Clicked: %1:%2").arg(this->lastPos.x()).arg(this->lastPos.y());

    emit this->framePixelClicked(this->lastPos, this->lastCounter);
}

void CelScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }

    if (dMainWindow().cursor().shape() != Qt::CrossCursor) {
        return; // ignore if not drawing
    }

    QPoint currPos = event->scenePos().toPoint();

    if (this->lastPos == currPos) {
        return;
    }

    this->lastPos = currPos;
    this->lastCounter++;

    emit this->framePixelClicked(this->lastPos, this->lastCounter);
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
    dMainWindow().openImageFiles(IMAGE_FILE_MODE::AUTO, filePaths, false);
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

// Displaying CEL file path information
void CelView::updateLabel()
{
    QFileInfo gfxFileInfo(this->gfx->getFilePath());
    QString label = gfxFileInfo.fileName();
    if (this->gfx->isModified()) {
        label += "*";
    }
    ui->celLabel->setText(label);
}

void CelView::update()
{
    this->updateLabel();

    ui->groupNumberEdit->setText(
        QString::number(this->gfx->getGroupCount()));

    ui->frameNumberEdit->setText(
        QString::number(this->gfx->getFrameCount()));
}

void CelView::changeColor(quint8 startColorIndex, quint8 endColorIndex, D1GfxPixel pixel, bool all)
{
    if (all || this->gfx->getFrameCount() == 0) {
        for (int i = 0; i < this->gfx->getFrameCount(); i++) {
            D1GfxFrame *frame = this->gfx->getFrame(i);
            frame->replacePixels(startColorIndex, endColorIndex, pixel);
        }
    } else {
        D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
        frame->replacePixels(startColorIndex, endColorIndex, pixel);
    }
    this->gfx->setModified();
    // update the view - done by the caller
    // this->update();
    // this->displayFrame();
}

int CelView::getCurrentFrameIndex()
{
    return this->currentFrameIndex;
}

void CelView::framePixelClicked(const QPoint &pos, unsigned counter)
{
    D1GfxFrame *frame = nullptr;

    if (this->gfx->getFrameCount() != 0) {
        frame = this->gfx->getFrame(this->currentFrameIndex);
    }

    QPoint p = pos;
    p -= QPoint(CEL_SCENE_SPACING, CEL_SCENE_SPACING);
    emit this->frameClicked(frame, p, counter);
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
    // update the view - done by the caller
    // this->update();
    // this->displayFrame();
}

void CelView::insertFrame(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool wasModified = this->gfx->isModified();
        bool clipped, palMod;
        D1GfxFrame *frame = this->gfx->insertFrame(index, &clipped);
        if (!D1Pcx::load(*frame, imagefilePath, clipped, this->pal, this->gfx->getPalette(), &palMod)) {
            this->gfx->removeFrame(index);
            this->gfx->setModified(wasModified);
            QString msg = tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
            if (mode != IMAGE_FILE_MODE::AUTO) {
                dProgressFail() << msg;
            } else {
                dProgressErr() << msg;
            }
        } else if (palMod) {
            // update the palette
            emit this->palModified();
        }
        return;
    }
    QImageReader reader = QImageReader(imagefilePath);
    if (!reader.canRead()) {
        QString msg = tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << msg;
        } else {
            dProgressErr() << msg;
        }
        return;
    }
    while (true) {
        QImage image = reader.read();
        if (image.isNull()) {
            break;
        }
        this->gfx->insertFrame(index, image);
        index++;
    }
}

void CelView::addToCurrentFrame(const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool clipped = false, palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, clipped, &basePal, this->gfx->getPalette(), &palMod);
        if (!success) {
            dProgressFail() << tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
            return;
        }
        D1GfxFrame *resFrame = this->gfx->addToFrame(this->currentFrameIndex, frame);
        if (resFrame == nullptr) {
            return; // error set by gfx->addToFrame
        }
        if (palMod) {
            // update the palette
            this->pal->updateColors(basePal);
            emit this->palModified();
        }
        // update the view - done by the caller
        // this->update();
        // this->displayFrame();
        return;
    }

    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        dProgressFail() << tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        return;
    }

    D1GfxFrame *frame = this->gfx->addToFrame(this->currentFrameIndex, image);
    if (frame == nullptr) {
        return; // error set by gfx->addToFrame
    }

    // update the view - done by the caller
    // this->update();
    // this->displayFrame();
}

void CelView::replaceCurrentFrame(const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool clipped = this->gfx->getFrame(this->currentFrameIndex)->isClipped(), palMod;
        D1GfxFrame *frame = new D1GfxFrame();
        bool success = D1Pcx::load(*frame, imagefilePath, clipped, this->pal, this->gfx->getPalette(), &palMod);
        if (!success) {
            delete frame;
            dProgressFail() << tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
            return;
        }
        this->gfx->setFrame(this->currentFrameIndex, frame);

        if (palMod) {
            // update the palette
            emit this->palModified();
        }
        // update the view - done by the caller
        // this->update();
        // this->displayFrame();
        return;
    }

    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        dProgressFail() << tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        return;
    }

    this->gfx->replaceFrame(this->currentFrameIndex, image);

    // update the view - done by the caller
    // this->update();
    // this->displayFrame();
}

void CelView::removeCurrentFrame()
{
    // remove the frame
    this->gfx->removeFrame(this->currentFrameIndex);
    if (this->gfx->getFrameCount() == this->currentFrameIndex) {
        this->currentFrameIndex = std::max(0, this->currentFrameIndex - 1);
    }
    this->updateGroupIndex();
    // update the view - done by the caller
    // this->update();
    // this->displayFrame();
}

QImage CelView::copyCurrent()
{
    /*if (this->gfx->getFrameCount() == 0) {
        return QImage();
    }*/
    return this->gfx->getFrameImage(this->currentFrameIndex);
}

void CelView::pasteCurrent(const QImage &image)
{
    if (this->gfx->getFrameCount() != 0) {
        this->gfx->replaceFrame(this->currentFrameIndex, image);
    } else {
        this->gfx->insertFrame(this->currentFrameIndex, image);
    }
    // update the view - done by the caller
    // this->update();
    // this->displayFrame();
}

void CelView::upscale(const UpscaleParam &params)
{
    if (Upscaler::upscaleGfx(this->gfx, params, false)) {
        // update the view - done by the caller
        // this->update();
        // this->displayFrame();
    }
}

void CelView::displayFrame()
{
    this->celScene->clear();

    // Getting the current frame to display
    QImage celFrame = this->gfx->getFrameCount() != 0 ? this->gfx->getFrameImage(this->currentFrameIndex) : QImage();

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
    int i = 0;

    for (; i < this->gfx->getGroupCount(); i++) {
        std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(i);

        if (this->currentFrameIndex >= groupFrameIndices.first
            && this->currentFrameIndex <= groupFrameIndices.second) {
            break;
        }
    }
    this->currentGroupIndex = i;
}

void CelView::setFrameIndex(int frameIndex)
{
    const int frameCount = this->gfx->getFrameCount();
    if (frameCount == 0) {
        this->currentFrameIndex = 0;
        this->currentGroupIndex = 0;
        return;
    }
    if (frameIndex >= frameCount) {
        frameIndex = frameCount - 1;
    } else if (frameIndex < 0) {
        frameIndex = 0;
    }
    this->currentFrameIndex = frameIndex;
    this->updateGroupIndex();

    this->displayFrame();
}

void CelView::setGroupIndex(int groupIndex)
{
    const int groupCount = this->gfx->getGroupCount();
    if (groupCount == 0) {
        this->currentFrameIndex = 0;
        this->currentGroupIndex = 0;
        return;
    }
    if (groupIndex >= groupCount) {
        groupIndex = groupCount - 1;
    } else if (groupIndex < 0) {
        groupIndex = 0;
    }
    std::pair<int, int> prevGroupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);
    int frameIndex = this->currentFrameIndex - groupFrameIndices.first;
    std::pair<int, int> newGroupFrameIndices = this->gfx->getGroupFrameIndices(groupIndex);
    this->currentGroupIndex = groupIndex;
    this->currentFrameIndex = std::min(newGroupFrameIndices.first + frameIndex, newGroupFrameIndices.second);

    this->displayFrame();
}

void CelView::playGroup()
{
    if (this->gfx->getGroupCount() == 0) {
        return;
    }
    std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);

    if (this->currentFrameIndex < groupFrameIndices.second)
        this->currentFrameIndex++;
    else
        this->currentFrameIndex = groupFrameIndices.first;

    int cycleType = this->ui->playComboBox->currentIndex();
    if (cycleType == 0) {
        // normal playback
        this->displayFrame();
    } else {
        dMainWindow().nextPaletteCycle((D1PAL_CYCLE_TYPE)(cycleType - 1));
        // this->displayFrame();
    }
}

void CelView::ShowContextMenu(const QPoint &pos)
{
    MainWindow *mw = &dMainWindow();
    QAction actions[7];

    QMenu contextMenu(this);
    contextMenu.setToolTipsVisible(true);

    // 'Frame' submenu of 'Edit'
    int cursor = 0;
    actions[cursor].setText(tr("Add Layer"));
    actions[cursor].setToolTip(tr("Add the content of an image to the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAddTo_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Insert Frame"));
    actions[cursor].setToolTip(tr("Add new frames before the current one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionInsert_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Append Frame"));
    actions[cursor].setToolTip(tr("Append new frames at the end"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAdd_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Replace Frame"));
    actions[cursor].setToolTip(tr("Replace the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionReplace_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Del Frame"));
    actions[cursor].setToolTip(tr("Delete the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDel_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&actions[cursor]);

    contextMenu.addSeparator();

    // drawing options of 'Edit'
    cursor++;
    actions[cursor].setText(tr("Start drawing"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionStart_Draw_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Stop drawing"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionStop_Draw_triggered()));
    contextMenu.addAction(&actions[cursor]);

    contextMenu.exec(mapToGlobal(pos));
}

void CelView::on_firstFrameButton_clicked()
{
    int nextFrameIndex = 0;
    if (this->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).first;
    }
    this->setFrameIndex(nextFrameIndex);
}

void CelView::on_previousFrameButton_clicked()
{
    int nextFrameIndex = this->currentFrameIndex - 1;
    if (this->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = std::max(nextFrameIndex, this->gfx->getGroupFrameIndices(this->currentGroupIndex).first);
    }
    this->setFrameIndex(nextFrameIndex);
}

void CelView::on_nextFrameButton_clicked()
{
    int nextFrameIndex = this->currentFrameIndex + 1;
    if (this->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = std::min(nextFrameIndex, this->gfx->getGroupFrameIndices(this->currentGroupIndex).second);
    }
    this->setFrameIndex(nextFrameIndex);
}

void CelView::on_lastFrameButton_clicked()
{
    int nextFrameIndex = this->gfx->getFrameCount() - 1;
    if (this->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).second;
    }
    this->setFrameIndex(nextFrameIndex);
}

void CelView::on_frameIndexEdit_returnPressed()
{
    int frameIndex = this->ui->frameIndexEdit->text().toInt() - 1;

    if (this->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);
        nextFrameIndex = std::max(nextFrameIndex, groupFrameIndices.first);
        nextFrameIndex = std::min(nextFrameIndex, groupFrameIndices.second);
    }
    this->setFrameIndex(frameIndex);

    this->on_frameIndexEdit_escPressed();
}

void CelView::on_frameIndexEdit_escPressed()
{
    this->ui->frameIndexEdit->setText(QString::number(this->gfx->getFrameCount() != 0 ? this->currentFrameIndex + 1 : 0));
    this->ui->frameIndexEdit->clearFocus();
}

void CelView::on_firstGroupButton_clicked()
{
    this->setGroupIndex(0);
}

void CelView::on_previousGroupButton_clicked()
{
    this->setGroupIndex(this->currentGroupIndex - 1);
}

void CelView::on_groupIndexEdit_returnPressed()
{
    int groupIndex = this->ui->groupIndexEdit->text().toInt() - 1;

    this->setGroupIndex(groupIndex);

    this->on_groupIndexEdit_escPressed();
}

void CelView::on_groupIndexEdit_escPressed()
{
    this->ui->groupIndexEdit->setText(QString::number(this->gfx->getGroupCount() != 0 ? this->currentGroupIndex + 1 : 0));
    this->ui->groupIndexEdit->clearFocus();
}

void CelView::on_nextGroupButton_clicked()
{
    this->setGroupIndex(this->currentGroupIndex + 1);
}

void CelView::on_lastGroupButton_clicked()
{
    this->setGroupIndex(this->gfx->getGroupCount() - 1);
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
    dMainWindow().initPaletteCycle();

    this->playTimer.start(this->currentPlayDelay);
}

void CelView::on_stopButton_clicked()
{
    this->playTimer.stop();

    // restore palette
    dMainWindow().resetPaletteCycle();
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
    dMainWindow().openImageFiles(IMAGE_FILE_MODE::AUTO, filePaths, false);
}
