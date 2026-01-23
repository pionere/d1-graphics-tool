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
#include "d1cel.h"
#include "d1cl2.h"
#include "d1pcx.h"
#include "d1smk.h"
#include "gfxcomponentdialog.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_celview.h"
#include "upscaler.h"

CelScene::CelScene(QWidget *v)
    : QGraphicsScene(v)
{
}

void CelScene::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control && !this->leftMousePressed) {
        this->panning = true;
        this->views()[0]->setDragMode(QGraphicsView::ScrollHandDrag);
        return;
    }
    QGraphicsScene::keyPressEvent(event);
}

void CelScene::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control && !this->leftMousePressed) {
        this->panning = false;
        this->views()[0]->setDragMode(QGraphicsView::NoDrag);
        return;
    }
    QGraphicsScene::keyReleaseEvent(event);
}

void CelScene::mouseEvent(QGraphicsSceneMouseEvent *event, int flags)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }

    if (this->panning) {
        if (!(flags & DOUBLE_CLICK)) {
            QGraphicsScene::mousePressEvent(event);
        }
        return;
    }

    this->leftMousePressed = true;

    QPointF scenePos = event->scenePos();
    QPoint currPos = QPoint(scenePos.x(), scenePos.y());
    // qDebug() << QStringLiteral("Mouse event at: %1:%2").arg(currPos.x()).arg(currPos.y());
    if (!(flags & FIRST_CLICK) && this->lastPos == currPos) {
        return;
    }
    this->lastPos = currPos;

    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        flags |= SHIFT_CLICK;
    }
    // emit this->framePixelClicked(this->lastPos, first);
    QObject *view = this->parent();
    CelView *celView = qobject_cast<CelView *>(view);
    if (celView != nullptr) {
        celView->framePixelClicked(currPos, flags);
        return;
    }
    LevelCelView *levelCelView = qobject_cast<LevelCelView *>(view);
    if (levelCelView != nullptr) {
        levelCelView->framePixelClicked(currPos, flags);
        return;
    }
    GfxsetView *setView = qobject_cast<GfxsetView *>(view);
    if (setView != nullptr) {
        setView->framePixelClicked(currPos, flags);
        return;
    }
    TblView *tblView = qobject_cast<TblView *>(view);
    if (tblView != nullptr) {
        tblView->framePixelClicked(currPos, flags);
        return;
    }
}

void CelScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    this->mouseEvent(event, FIRST_CLICK);
}

void CelScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->leftMousePressed = false;
        if (this->panning) {
            this->panning = (event->modifiers() & Qt::ControlModifier) != 0;
            this->views()[0]->setDragMode(this->panning ? QGraphicsView::ScrollHandDrag : QGraphicsView::NoDrag);
        }
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

void CelScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    this->mouseEvent(event, FIRST_CLICK | DOUBLE_CLICK);
}

void CelScene::mouseHoverEvent(QGraphicsSceneMouseEvent *event)
{
    // emit this->framePixelHovered(this->lastPos);
    QPointF scenePos = event->scenePos();
    QPoint currPos = QPoint(scenePos.x(), scenePos.y());
    QObject *view = this->parent();
    CelView *celView = qobject_cast<CelView *>(view);
    if (celView != nullptr) {
        celView->framePixelHovered(currPos);
        return;
    }
    LevelCelView *levelCelView = qobject_cast<LevelCelView *>(view);
    if (levelCelView != nullptr) {
        levelCelView->framePixelHovered(currPos);
        return;
    }
    GfxsetView *setView = qobject_cast<GfxsetView *>(view);
    if (setView != nullptr) {
        setView->framePixelHovered(currPos);
        return;
    }
    TblView *tblView = qobject_cast<TblView *>(view);
    if (tblView != nullptr) {
        tblView->framePixelHovered(currPos);
        return;
    }
}

void CelScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (this->panning) {
        QGraphicsScene::mouseMoveEvent(event);
        return;
    }
    Qt::MouseButtons buttons = event->buttons();
    // simulate left click during drag
    // if (buttons != Qt::NoButton) {
    if (buttons & Qt::LeftButton) {
        this->mouseEvent(event, 0);
    }
    this->mouseHoverEvent(event);
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
{
    this->ui->setupUi(this);
    this->ui->celGraphicsView->setScene(&this->celScene);
    this->ui->celGraphicsView->setMouseTracking(true);
    this->on_zoomEdit_escPressed();
    // this->on_playDelayEdit_escPressed();
    // this->on_assetMplEdit_escPressed();
    QLayout *compLayout = this->ui->componentsHorizontalLayout;
    PushButtonWidget::addButton(this, compLayout, QStyle::SP_FileDialogNewFolder, tr("New Component"), this, &CelView::on_newComponentPushButtonClicked);
    PushButtonWidget::addButton(this, compLayout, QStyle::SP_DialogOpenButton, tr("Edit Component"), this, &CelView::on_editComponentPushButtonClicked);
    PushButtonWidget::addButton(this, compLayout, QStyle::SP_BrowserReload, tr("Reload Component Graphics"), this, &CelView::on_reloadComponentPushButtonClicked);
    PushButtonWidget::addButton(this, compLayout, QStyle::SP_DialogCloseButton, tr("Remove Component"), this, &CelView::on_closeComponentPushButtonClicked);

    QLayout *layout = this->ui->paintbuttonHorizontalLayout;
    this->audioBtn = PushButtonWidget::addButton(this, layout, QStyle::SP_MediaVolume, tr("Show audio"), this, &CelView::showAudioInfo);
    layout->setAlignment(this->audioBtn, Qt::AlignLeft);
    this->audioBtn->setVisible(false);
    this->audioMuted = false;
    PushButtonWidget *btn = PushButtonWidget::addButton(this, layout, QStyle::SP_DialogResetButton, tr("Start drawing"), &dMainWindow(), &MainWindow::on_actionToggle_Painter_triggered);
    layout->setAlignment(btn, Qt::AlignRight);

    // If a pixel of the frame was clicked get pixel color index and notify the palette widgets
    // QObject::connect(&this->celScene, &CelScene::framePixelClicked, this, &CelView::framePixelClicked);
    // QObject::connect(&this->celScene, &CelScene::framePixelHovered, this, &CelView::framePixelHovered);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->frameIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_frameIndexEdit_escPressed()));
    QObject::connect(this->ui->groupIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_groupIndexEdit_escPressed()));
    QObject::connect(this->ui->zoomEdit, SIGNAL(cancel_signal()), this, SLOT(on_zoomEdit_escPressed()));
    QObject::connect(this->ui->playDelayEdit, SIGNAL(cancel_signal()), this, SLOT(on_playDelayEdit_escPressed()));
    QObject::connect(this->ui->assetMplEdit, SIGNAL(cancel_signal()), this, SLOT(on_assetMplEdit_escPressed()));

    // setup context menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));

    setAcceptDrops(true);
}

CelView::~CelView()
{
    delete gfxComponentDialog;
    delete smkAudioWidget;
    delete ui;
}

void CelView::initialize(D1Pal *p, D1Gfx *g, bool bottomPanelHidden)
{
    this->pal = p;
    this->gfx = g;

    this->ui->bottomPanel->setVisible(!bottomPanelHidden);
    bool smkGfx = g->getType() == D1CEL_TYPE::SMK;
    this->audioBtn->setVisible(smkGfx);
    if (smkGfx) {
        this->currentPlayDelay = g->getFrameLen();
    }

    this->updateFields();
}

void CelView::setPal(D1Pal *p)
{
    this->pal = p;

    if (this->gfx->getType() == D1CEL_TYPE::SMK) {
        for (int i = this->currentFrameIndex; i >= 0; i--) {
            QPointer<D1Pal> &fp = this->gfx->getFrame(i)->getFramePal();
            if (!fp.isNull()) {
                if (fp.data() != p) {
                    // update the palette of the current frame if it does not match
                    i = this->currentFrameIndex;
                    this->gfx->getFrame(i)->setFramePal(p);
                    // remove subsequent palette if it matches
                    while (++i < this->gfx->getFrameCount()) {
                        QPointer<D1Pal> &fp = this->gfx->getFrame(i)->getFramePal();
                        if (!fp.isNull()) {
                            if (fp.data() == p) {
                                this->gfx->getFrame(i)->setFramePal(nullptr);
                            }
                            break;
                        }
                    }
                    this->gfx->setModified();
                }
                break;
            }
        }
    }
}

void CelView::setGfx(D1Gfx *g)
{
    // stop playback
    if (this->playTimer != 0) {
        this->on_playStopButton_clicked();
    }

    this->gfx = g;
    if (g->getType() == D1CEL_TYPE::SMK) {
        this->currentPlayDelay = g->getFrameLen();
    }

    if (this->currentFrameIndex >= this->gfx->getFrameCount()) {
        this->currentFrameIndex = 0;
    }
    this->updateGroupIndex();

    // notify the audio-popup that the gfx changed
    if (this->smkAudioWidget != nullptr) {
        this->smkAudioWidget->setGfx(this->gfx);
    }
}

void CelView::setLabelContent(QLabel *label, const QString &filePath, bool modified)
{
    label->setToolTip(filePath);

    QFileInfo fileInfo(filePath);
    QString labelText = fileInfo.fileName();
    if (modified) {
        labelText += "*";
    }
    label->setText(labelText);
}

// Displaying CEL file path information
void CelView::updateLabel()
{
    CelView::setLabelContent(this->ui->celLabel, this->gfx->getFilePath(), this->gfx->isModified());
}

void CelView::updateFields()
{
    int count;

    this->updateLabel();
    // set play-delay text
    this->ui->playDelayEdit->setText(QString::number(this->currentPlayDelay));
    // update visiblity of the audio icon
    this->audioBtn->setVisible(this->gfx->getType() == D1CEL_TYPE::SMK && this->gfx->getFrameCount() != 0);

    // update the components
    QComboBox *comboBox = this->ui->componentsComboBox;
    int prevIndex = comboBox->currentIndex();
    comboBox->hide();
    comboBox->clear();
    comboBox->addItem("", 0);
    count = this->gfx->getComponentCount();
    if (prevIndex < 0) {
        prevIndex = 0;
    } else if (count < prevIndex) {
        prevIndex = count;
    }
    for (int i = 0; i < count; i++) {
        D1GfxComp *comp = this->gfx->getComponent(i);
        QString labelText = comp->getLabel();
        if (comp->getGFX()->isModified()) {
            labelText += "*";
        }
        comboBox->addItem(labelText, i + 1);
    }
    comboBox->show();
    comboBox->setCurrentIndex(prevIndex);

    // update the asset multiplier field
    this->ui->assetMplEdit->setText(QString::number(this->assetMpl));

    // Set current and maximum group text
    count = this->gfx->getGroupCount();
    this->ui->groupIndexEdit->setText(
        QString::number(count != 0 ? this->currentGroupIndex + 1 : 0));
    this->ui->groupNumberEdit->setText(QString::number(count));

    // Set current and maximum frame text
    int frameIndex = this->currentFrameIndex;
    if (this->ui->framesGroupCheckBox->isChecked() && count != 0) {
        std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);
        frameIndex -= groupFrameIndices.first;
        count = groupFrameIndices.second - groupFrameIndices.first + 1;
    } else {
        count = this->gfx->getFrameCount();
    }
    this->ui->frameIndexEdit->setText(
        QString::number(count != 0 ? frameIndex + 1 : 0));
    this->ui->frameNumberEdit->setText(QString::number(count));

    // update clipped checkbox
    this->ui->celFramesClippedCheckBox->setChecked(this->gfx->isClipped());
}

CelScene *CelView::getCelScene() const
{
    return const_cast<CelScene *>(&this->celScene);
}

int CelView::getCurrentFrameIndex() const
{
    return this->currentFrameIndex;
}

void CelView::framePixelClicked(const QPoint &pos, int flags)
{
    if (this->gfx->getFrameCount() == 0) {
        return;
    }
    QRect rect = this->gfx->getFrameRect(this->currentFrameIndex, true);

    QPoint p = pos;
    p -= QPoint(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    p += QPoint(rect.x(), rect.y());

    int compIdx = this->ui->componentsComboBox->currentIndex();
    if (compIdx > 0) {
        D1GfxComp *comp = this->gfx->getComponent(compIdx - 1);
        D1GfxCompFrame *cFrame = comp->getCompFrame(this->currentFrameIndex);
        unsigned frameRef = cFrame->cfFrameRef;
        D1Gfx* cGfx = comp->getGFX();
        if (frameRef != 0 && frameRef <= (unsigned)cGfx->getFrameCount()) {
            p -= QPoint(cFrame->cfOffsetX, cFrame->cfOffsetY);

            D1GfxFrame *frame = cGfx->getFrame(frameRef - 1);
            dMainWindow().frameClicked(frame, p, flags);
        }
        return;
    }

    D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
    dMainWindow().frameClicked(frame, p, flags);
}

bool CelView::framePos(QPoint &pos) const
{
    if (this->gfx->getFrameCount() != 0) {
        D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
        pos -= QPoint(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
        return pos.x() >= 0 && pos.x() < frame->getWidth() && pos.y() >= 0 && pos.y() < frame->getHeight();
    }

    return false;
}

void CelView::framePixelHovered(const QPoint &pos) const
{
    QPoint tpos = pos;
    if (this->framePos(tpos)) {
        dMainWindow().pointHovered(tpos);
        return;
    }

    tpos.setX(UINT_MAX);
    tpos.setY(UINT_MAX);
    dMainWindow().pointHovered(tpos);
}

void CelView::createFrame(bool append)
{
    int width, height;
    int frameCount = this->gfx->getFrameCount();
    if (frameCount != 0) {
        D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
        width = frame->getWidth();
        height = frame->getHeight();
    } else {
        // TODO: request from the user?
        width = 64;
        height = 128;
    }
    int newFrameIndex = append ? this->gfx->getFrameCount() : this->currentFrameIndex;
    this->gfx->insertFrame(newFrameIndex, width, height);
    // jump to the new frame
    this->currentFrameIndex = newFrameIndex;
    // update the view - done by the caller
    // this->displayFrame();
}

void CelView::insertImageFiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append)
{
    const int numFiles = imagefilePaths.count();
    ProgressDialog::incBar(QApplication::tr("Inserting graphics..."), numFiles + 1);

    int prevFrameCount = this->gfx->getFrameCount();
    if (append) {
        // append the frame(s)
        for (int i = 0; i < numFiles; i++) {
            this->insertFrame(mode, this->gfx->getFrameCount(), imagefilePaths[i]);
            if (!ProgressDialog::incValue()) {
                break;
            }
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
        for (int i = numFiles - 1; i >= 0; i--) {
            this->insertFrame(mode, this->currentFrameIndex, imagefilePaths[i]);
            if (!ProgressDialog::incValue()) {
                break;
            }
        }
        int deltaFrameCount = this->gfx->getFrameCount() - prevFrameCount;
        if (deltaFrameCount == 0) {
            return; // no new frame -> done
        }
    }
    // update the view - done by the caller
    // this->displayFrame();

    ProgressDialog::decBar();
}

void CelView::insertFrame(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool wasModified = this->gfx->isModified();
        bool palMod;
        D1GfxFrame *frame = this->gfx->insertFrame(index);
        if (!D1Pcx::load(*frame, imagefilePath, this->pal, this->gfx->getPalette(), &palMod)) {
            this->gfx->removeFrame(index, false);
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
    ProgressDialog::incBar(QApplication::tr("Inserting frames..."), reader.imageCount() + 1);
    QPair<int, QString> progress;
    progress.first = -1;
    int first = index;
    while (true) {
        QImage image = reader.read();
        if (image.isNull()) {
            break;
        }
        progress.second = QString(QApplication::tr("Processing frame %1.")).arg(index + 1);
        dProgress() << progress;
        this->gfx->insertFrame(index, image);
        index++;
        if (!ProgressDialog::incValue()) {
            break;
        }
    }
    progress.second = QApplication::tr("Added %n frame(s).", "", index - first);
    dProgress() << progress;
    ProgressDialog::decBar();
}

void CelView::addToCurrentFrame(const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, &basePal, this->gfx->getPalette(), &palMod);
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
    // this->displayFrame();
}

void CelView::duplicateCurrentFrame(bool wholeGroup)
{
    this->currentFrameIndex = this->gfx->duplicateFrame(this->currentFrameIndex, wholeGroup);

    this->updateGroupIndex();
    // update the view
    this->updateFields();
}

void CelView::replaceCurrentFrame(const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool palMod;
        D1GfxFrame *frame = new D1GfxFrame();
        bool success = D1Pcx::load(*frame, imagefilePath, this->pal, this->gfx->getPalette(), &palMod);
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
    // this->displayFrame();
}

void CelView::removeCurrentFrame(bool wholeGroup)
{
    // remove the frame
    this->gfx->removeFrame(this->currentFrameIndex, wholeGroup);
    int numFrame = this->gfx->getFrameCount();
    if (numFrame <= this->currentFrameIndex) {
        this->currentFrameIndex = std::max(0, numFrame - 1);
    }
    this->updateGroupIndex();
    // update the view - done by the caller
    // this->displayFrame();
}

void CelView::flipHorizontalCurrentFrame(bool wholeGroup)
{
    // flip the frame
    this->gfx->flipHorizontalFrame(this->currentFrameIndex, wholeGroup);

    // update the view - done by the caller
    // this->displayFrame();
}

void CelView::flipVerticalCurrentFrame(bool wholeGroup)
{
    // flip the frame
    this->gfx->flipVerticalFrame(this->currentFrameIndex, wholeGroup);

    // update the view - done by the caller
    // this->displayFrame();
}

void CelView::mergeFrames(const MergeFramesParam &params)
{
    int firstFrameIdx = params.rangeFrom;
    int lastFrameIdx = params.rangeTo;
    // assert(firstFrameIdx >= 0 && firstFrameIdx < lastFrameIdx && lastFrameIdx < this->gfx->getFrameCount());
    this->gfx->mergeFrames(firstFrameIdx, lastFrameIdx);
    if (this->currentFrameIndex > firstFrameIdx) {
        if (this->currentFrameIndex <= lastFrameIdx) {
            this->currentFrameIndex = firstFrameIdx;
        } else {
            this->currentFrameIndex -= lastFrameIdx - firstFrameIdx;
        }
    }
    this->updateGroupIndex();
    // update the view - done by the caller
    // this->displayFrame();
}

QString CelView::copyCurrentPixels(bool values) const
{
    if (this->gfx->getFrameCount() == 0) {
        return QString();
    }
    return this->gfx->getFramePixels(this->currentFrameIndex, values);
}

void CelView::pasteCurrentPixels(const QString &pixels)
{
    if (this->gfx->getFrameCount() != 0) {
        this->gfx->replaceFrame(this->currentFrameIndex, pixels);
    } else {
        this->gfx->insertFrame(this->currentFrameIndex, pixels);
    }
    // update the view - done by the caller
    // this->displayFrame();
}

QImage CelView::copyCurrentImage() const
{
    if (this->gfx->getFrameCount() == 0) {
        return QImage();
    }
    return this->gfx->getFrameImage(this->currentFrameIndex);
}

void CelView::pasteCurrentImage(const QImage &image)
{
    if (this->gfx->getFrameCount() != 0) {
        this->gfx->replaceFrame(this->currentFrameIndex, image);
    } else {
        this->gfx->insertFrame(this->currentFrameIndex, image);
    }
    // update the view - done by the caller
    // this->displayFrame();
}

void CelView::coloredFrames(const std::pair<int, int>& colors) const
{
    ProgressDialog::incBar(tr("Checking frames..."), 1);
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    if ((unsigned)colors.first >= D1PAL_COLORS) {
        progress.second = tr("Frames with transparent pixels:");
    } else {
        progress.second = tr("Frames with pixels in the [%1..%2] color range:").arg(colors.first).arg(colors.second);
    }

    dProgress() << progress;
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        const std::vector<std::vector<D1GfxPixel>> pixelImage = this->gfx->getFramePixelImage(i);
        int numPixels = D1GfxPixel::countAffectedPixels(pixelImage, colors);
        if (numPixels != 0) {
            dProgress() << tr("Frame %1 has %n affected pixels.", "", numPixels).arg(i + 1);
            result = true;
        }
    }

    if (!result) {
        if ((unsigned)colors.first >= D1PAL_COLORS) {
            progress.second = tr("None of the frames have transparent pixel.");
        } else {
            progress.second = tr("None of the frames are using the colors [%1..%2].").arg(colors.first).arg(colors.second);
        }
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

void CelView::activeFrames() const
{
    ProgressDialog::incBar(tr("Checking frames..."), 1);
    QComboBox *cycleBox = this->ui->playComboBox;
    QString cycleTypeTxt = cycleBox->currentText();
    int cycleType = cycleBox->currentIndex();
    if (cycleType != 0) {
        int cycleColors = D1Pal::getCycleColors((D1PAL_CYCLE_TYPE)(cycleType - 1));
        const std::pair<int, int> colors = { 1, cycleColors - 1 };
        bool result = false;

        QPair<int, QString> progress;
        progress.first = -1;
        progress.second = tr("Active frames (using '%1' playback mode):").arg(cycleTypeTxt);

        dProgress() << progress;
        for (int i = 0; i < this->gfx->getFrameCount(); i++) {
            const std::vector<std::vector<D1GfxPixel>> pixelImage = this->gfx->getFramePixelImage(i);
            int numPixels = D1GfxPixel::countAffectedPixels(pixelImage, colors);
            if (numPixels != 0) {
                dProgress() << tr("Frame %1 has %n affected pixels.", "", numPixels).arg(i + 1);
                result = true;
            }
        }

        if (!result) {
            progress.second = tr("None of the frames are active in '%1' playback mode.").arg(cycleTypeTxt);
            dProgress() << progress;
        }
    } else {
        dProgress() << tr("Colors are not affected if the playback mode is '%1'.").arg(cycleTypeTxt);
    }

    ProgressDialog::decBar();
}

void CelView::checkGraphics() const
{
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    progress.second = tr("Inconsistencies in the graphics:");

    dProgress() << progress;
    result = this->gfx->check();

    if (!result) {
        progress.second = tr("No inconsistency detected in the current gfx.");
        dProgress() << progress;
    }
}

void CelView::resize(const ResizeParam &params)
{
    int frameWithPixelLost = this->gfx->testResize(params);
    if (frameWithPixelLost != -1) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Pixels with non-background colors are going to be eliminated (At least Frame %1 is affected). Are you sure you want to proceed?").arg(frameWithPixelLost + 1), QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Resizing..."), 1, PAF_NONE);

    bool change = this->gfx->resize(params);

    ProgressDialog::done();

    if (change) {
        this->gfx->setModified();
        // update the view
        this->displayFrame();
    }
}

void CelView::upscale(const UpscaleParam &params)
{
    if (Upscaler::upscaleGfx(this->gfx, params, false)) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void CelView::drawGrid(QImage &celFrame)
{
    int width = celFrame.width();
    int height = celFrame.height();
    QColor color = this->pal->getUndefinedColor();

    unsigned microHeight = MICRO_HEIGHT * this->assetMpl;
    for (int i = (height + microHeight) / microHeight - 1; i >= 0; i--) {
        for (int x = 0; x < width; x++) {
            int y0 = height - microHeight * (i + 1) + ( 0 + (x - width / 2) / 2) % microHeight;
            if (y0 >= 0 && y0 < height)
                celFrame.setPixelColor(x, y0, color);

            int y1 = height - microHeight * (i + 1) + (microHeight - 1 - (x - width / 2) / 2) % microHeight;
            if (y1 >= 0 && y1 < height)
                celFrame.setPixelColor(x, y1, color);
        }
    }
}

void CelView::displayFrame()
{
    this->updateFields();
    this->celScene.clear();

    // Getting the current frame to display
    Qt::CheckState compType = this->ui->showComponentsCheckBox->checkState();
    int component = compType == Qt::Unchecked ? 0 : (compType == Qt::PartiallyChecked ? this->ui->componentsComboBox->currentIndex() : -1);
    QImage celFrame = this->gfx->getFrameCount() != 0 ? this->gfx->getFrameImage(this->currentFrameIndex, component) : QImage();

    // add grid if requested
    if (this->ui->showGridCheckBox->isChecked()) {
        this->drawGrid(celFrame);
    }

    this->celScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    // Building background of the width/height of the CEL frame
    QImage celFrameBackground = QImage(celFrame.width(), celFrame.height(), QImage::Format_ARGB32);
    celFrameBackground.fill(QColor(Config::getGraphicsTransparentColor()));

    // Resize the scene rectangle to include some padding around the CEL frame
    this->celScene.setSceneRect(0, 0,
        CEL_SCENE_MARGIN + celFrame.width() + CEL_SCENE_MARGIN,
        CEL_SCENE_MARGIN + celFrame.height() + CEL_SCENE_MARGIN);
    // ui->celGraphicsView->adjustSize();

    // Add the backgrond and CEL frame while aligning it in the center
    this->celScene.addPixmap(QPixmap::fromImage(celFrameBackground))
        ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    this->celScene.addPixmap(QPixmap::fromImage(celFrame))
        ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);

    // Set current frame width and height
    this->ui->celFrameWidthEdit->setText(QString::number(celFrame.width()) + " px");
    this->ui->celFrameHeightEdit->setText(QString::number(celFrame.height()) + " px");

    // Notify PalView that the frame changed (used to refresh palette widget)
    emit this->frameRefreshed();

    // notify the audio-popup that the frame changed
    if (this->smkAudioWidget != nullptr) {
        this->smkAudioWidget->initialize(this->currentFrameIndex);
    }
}

void CelView::toggleBottomPanel()
{
    this->ui->bottomPanel->setVisible(this->ui->bottomPanel->isHidden());
}

bool CelView::toggleMute()
{
    this->audioMuted = !this->audioMuted;
    this->audioBtn->setIcon(QApplication::style()->standardIcon(this->audioMuted ? QStyle::SP_MediaVolumeMuted : QStyle::SP_MediaVolume));
    return this->audioMuted;
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
        // this->currentFrameIndex = 0;
        // this->currentGroupIndex = 0;
        // this->displayFrame();
        return;
    }
    if (frameIndex >= frameCount) {
        frameIndex = frameCount - 1;
    } else if (frameIndex < 0) {
        frameIndex = 0;
    }
    this->currentFrameIndex = frameIndex;
    this->updateGroupIndex();

    const bool switchPal = QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier;
    if (switchPal && frameIndex >= 0) {
        int palIndex = frameIndex;
        while (palIndex >= 0 && this->gfx->getFrame(palIndex)->getFramePal().isNull()) {
            palIndex--;
        }
        if (palIndex >= 0) {
            dMainWindow().updatePalette(this->gfx->getFrame(palIndex)->getFramePal().data());
        }
    }
    
    this->displayFrame();
}

void CelView::setGroupIndex(int groupIndex)
{
    const int groupCount = this->gfx->getGroupCount();
    if (groupCount == 0) {
        // this->currentFrameIndex = 0;
        // this->currentGroupIndex = 0;
        // this->displayFrame();
        return;
    }
    if (groupIndex >= groupCount) {
        groupIndex = groupCount - 1;
    } else if (groupIndex < 0) {
        groupIndex = 0;
    }
    std::pair<int, int> prevGroupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);
    int frameIndex = this->currentFrameIndex - prevGroupFrameIndices.first;
    std::pair<int, int> newGroupFrameIndices = this->gfx->getGroupFrameIndices(groupIndex);
    this->currentGroupIndex = groupIndex;
    this->currentFrameIndex = std::min(newGroupFrameIndices.first + frameIndex, newGroupFrameIndices.second);

    this->displayFrame();
}

void CelView::showAudioInfo()
{
    if (this->smkAudioWidget == nullptr) {
        this->smkAudioWidget = new SmkAudioWidget(this);
        this->smkAudioWidget->setGfx(this->gfx);
    }
    this->smkAudioWidget->initialize(this->currentFrameIndex);
    this->smkAudioWidget->show();
}

void CelView::ShowContextMenu(const QPoint &pos)
{
    MainWindow *mw = &dMainWindow();
    QAction actions[9];

    QMenu contextMenu(this);
    contextMenu.setToolTipsVisible(true);

    // 'Frame' submenu of 'Edit'
    int cursor = 0;
    actions[cursor].setText(tr("Merge"));
    actions[cursor].setToolTip(tr("Merge frames of the current graphics"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionMerge_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Add Layer"));
    actions[cursor].setToolTip(tr("Add the content of an image to the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAddTo_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    contextMenu.addSeparator();

    cursor++;
    actions[cursor].setText(tr("Create"));
    actions[cursor].setToolTip(tr("Create a new frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionCreate_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Insert"));
    actions[cursor].setToolTip(tr("Add a new frame before the current one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionInsert_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Duplicate"));
    actions[cursor].setToolTip(tr("Duplicate the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDuplicate_Frame_triggered()));
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Replace"));
    actions[cursor].setToolTip(tr("Replace the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionReplace_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Del"));
    actions[cursor].setToolTip(tr("Delete the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDel_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&actions[cursor]);

    contextMenu.addSeparator();

    cursor++;
    actions[cursor].setText(tr("Horizontal Flip"));
    actions[cursor].setToolTip(tr("Flip the current frame horizontally"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionFlipHorizontal_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Vertical Flip"));
    actions[cursor].setToolTip(tr("Flip the current frame vertically"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionFlipVertical_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    contextMenu.addAction(&actions[cursor]);

    contextMenu.exec(mapToGlobal(pos));
}

void CelView::on_newComponentPushButtonClicked()
{
    QString gfxFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Graphics"), tr("CEL/CL2 Files (*.cel *.CEL *.cl2 *.CL2)"));
    if (gfxFilePath.isEmpty()) {
        return;
    }

    D1Gfx* gfx = this->gfx->loadComponentGFX(gfxFilePath);
    if (gfx == nullptr) {
        return;
    }

    int compIdx = this->ui->componentsComboBox->currentIndex();
    
    this->gfx->insertComponent(compIdx, gfx);

    dMainWindow().updateWindow();
}

void CelView::on_editComponentPushButtonClicked()
{
    int compIdx = this->ui->componentsComboBox->currentIndex();
    if (compIdx != 0 && this->gfx->getFrameCount() != 0) {
        if (this->gfxComponentDialog == nullptr) {
            this->gfxComponentDialog = new GfxComponentDialog(this);
        }
        this->gfxComponentDialog->initialize(this->gfx, this->gfx->getComponent(compIdx - 1));
        this->gfxComponentDialog->show();
    }
}


void CelView::on_reloadComponentPushButtonClicked()
{
    int compIdx = this->ui->componentsComboBox->currentIndex();
    if (compIdx != 0) {
        D1GfxComp *gfxComp = this->gfx->getComponent(compIdx - 1);
        D1Gfx* cGfx = this->gfx->loadComponentGFX(gfxComp->getGFX()->getFilePath());
        if (cGfx != nullptr) {
            gfxComp->setGFX(cGfx);

            this->displayFrame();
        }
    }
}

void CelView::on_closeComponentPushButtonClicked()
{
    int compIdx = this->ui->componentsComboBox->currentIndex();
    if (compIdx != 0) {
        this->gfx->removeComponent(compIdx - 1);

        dMainWindow().updateWindow();
    }
}

void CelView::on_componentsComboBox_activated(int index)
{
    // redraw the frame
    this->displayFrame();
}

void CelView::on_showComponentsCheckBox_clicked()
{
    // redraw the frame
    this->displayFrame();
}

void CelView::on_framesGroupCheckBox_clicked()
{
    // update frameIndexEdit and frameNumberEdit
    this->updateFields();
}

void CelView::on_firstFrameButton_clicked()
{
    int nextFrameIndex = 0;
    const bool moveFrame = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).first;
        if (moveFrame) {
            for (int i = this->currentFrameIndex; i > nextFrameIndex; i--) {
                this->gfx->swapFrames(i, i - 1);
            }
        }
    } else {
        if (moveFrame) {
            this->gfx->swapFrames(UINT_MAX, this->currentFrameIndex);
        }
    }
    this->setFrameIndex(nextFrameIndex);
}

void CelView::on_previousFrameButton_clicked()
{
    int nextFrameIndex = this->currentFrameIndex - 1;
    const bool moveFrame = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = std::max(nextFrameIndex, this->gfx->getGroupFrameIndices(this->currentGroupIndex).first);
    }
    if (moveFrame) {
        this->gfx->swapFrames(nextFrameIndex, this->currentFrameIndex);
    }
    this->setFrameIndex(nextFrameIndex);
}

void CelView::on_nextFrameButton_clicked()
{
    int nextFrameIndex = this->currentFrameIndex + 1;
    const bool moveFrame = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = std::min(nextFrameIndex, this->gfx->getGroupFrameIndices(this->currentGroupIndex).second);
    }
    if (moveFrame) {
        this->gfx->swapFrames(this->currentFrameIndex, nextFrameIndex);
    }
    this->setFrameIndex(nextFrameIndex);
}

void CelView::on_lastFrameButton_clicked()
{
    int nextFrameIndex = this->gfx->getFrameCount() - 1;
    const bool moveFrame = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).second;
        if (moveFrame) {
            for (int i = this->currentFrameIndex; i < nextFrameIndex; i++) {
                this->gfx->swapFrames(i, i + 1);
            }
        }
    } else {
        if (moveFrame) {
            this->gfx->swapFrames(this->currentFrameIndex, UINT_MAX);
        }
    }
    this->setFrameIndex(nextFrameIndex);
}

void CelView::on_frameIndexEdit_returnPressed()
{
    int nextFrameIndex = this->ui->frameIndexEdit->text().toInt() - 1;

    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);
        nextFrameIndex += groupFrameIndices.first;
        nextFrameIndex = std::max(nextFrameIndex, groupFrameIndices.first);
        nextFrameIndex = std::min(nextFrameIndex, groupFrameIndices.second);
    }
    this->setFrameIndex(nextFrameIndex);

    this->on_frameIndexEdit_escPressed();
}

void CelView::on_frameIndexEdit_escPressed()
{
    // update frameIndexEdit
    this->updateFields();
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
    // update groupIndexEdit
    this->updateFields();
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

void CelView::on_showGridCheckBox_clicked()
{
    this->displayFrame();
}

void CelView::on_assetMplEdit_returnPressed()
{
    unsigned ampl = this->ui->assetMplEdit->text().toUShort();
    if (ampl == 0)
        ampl = 1;
    if (this->assetMpl != ampl) {
        this->assetMpl = ampl;
        this->displayFrame();
    }

    this->on_assetMplEdit_escPressed();
}

void CelView::on_assetMplEdit_escPressed()
{
    this->updateFields();
    this->ui->assetMplEdit->clearFocus();
}

void CelView::on_celFramesClippedCheckBox_clicked()
{
    this->gfx->setClipped(this->ui->celFramesClippedCheckBox->isChecked());
    this->updateFields();
}

void CelView::on_zoomOutButton_clicked()
{
    this->celScene.zoomOut();
    this->on_zoomEdit_escPressed();
}

void CelView::on_zoomInButton_clicked()
{
    this->celScene.zoomIn();
    this->on_zoomEdit_escPressed();
}

void CelView::on_zoomEdit_returnPressed()
{
    QString zoom = this->ui->zoomEdit->text();

    this->celScene.setZoom(zoom);

    this->on_zoomEdit_escPressed();
}

void CelView::on_zoomEdit_escPressed()
{
    this->ui->zoomEdit->setText(this->celScene.zoomText());
    this->ui->zoomEdit->clearFocus();
}

void CelView::on_playDelayEdit_returnPressed()
{
    unsigned playDelay = this->ui->playDelayEdit->text().toUInt();

    if (playDelay != 0)
        this->currentPlayDelay = playDelay;

    this->on_playDelayEdit_escPressed();
}

void CelView::on_playDelayEdit_escPressed()
{
    // update playDelayEdit
    this->updateFields();
    this->ui->playDelayEdit->clearFocus();
}

void CelView::on_playStopButton_clicked()
{
    if (this->playTimer != 0) {
        this->killTimer(this->playTimer);
        this->playTimer = 0;
        D1Smk::stopAudio();

        // restore the currentFrameIndex
        this->currentFrameIndex = this->origFrameIndex;
        // update group-index because the user might have changed it in the meantime
        this->updateGroupIndex();
        // restore palette
        if (!this->origPal.isNull()) {
            dMainWindow().updatePalette(this->origPal.data());
        }
        dMainWindow().resetPaletteCycle();
        // change the label of the button
        this->ui->playStopButton->setText(tr("Play"));
        // enable the related fields
        this->ui->playDelayEdit->setReadOnly(false);
        this->ui->playComboBox->setEnabled(true);
        this->ui->playFrameCheckBox->setEnabled(true);
        return;
    }
    // disable the related fields
    this->ui->playDelayEdit->setReadOnly(true);
    this->ui->playComboBox->setEnabled(false);
    this->ui->playFrameCheckBox->setEnabled(false);
    // change the label of the button
    this->ui->playStopButton->setText(tr("Stop"));
    // preserve the currentFrameIndex
    this->origFrameIndex = this->currentFrameIndex;
    // preserve the palette
    this->origPal = this->pal;
    dMainWindow().initPaletteCycle();

    this->playTimer = this->startTimer(this->currentPlayDelay / 1000, Qt::PreciseTimer);
/*
    this->timer.start();
    qint64 nextTickNS = timer.nsecsElapsed() + this->currentPlayDelay * 1000;
    QTimer::singleShot(this->currentPlayDelay / 1000, Qt::PreciseTimer, this, &CelView::timerEvent);
*/
}

void CelView::timerEvent(QTimerEvent *event)
{
    if (this->gfx->getGroupCount() == 0) {
        return;
    }
    std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);

    int nextFrameIndex = this->currentFrameIndex + 1;
    Qt::CheckState playType = this->ui->playFrameCheckBox->checkState();
    if (playType == Qt::Unchecked) {
        // normal playback
        if (nextFrameIndex > groupFrameIndices.second)
            nextFrameIndex = groupFrameIndices.first;
    } else if (playType == Qt::PartiallyChecked) {
        // playback till the original frame
        if (nextFrameIndex > this->origFrameIndex)
            nextFrameIndex = groupFrameIndices.first;
    } else {
        // playback from the original frame
        if (nextFrameIndex > groupFrameIndices.second)
            nextFrameIndex = this->origFrameIndex;
    }
    this->currentFrameIndex = nextFrameIndex;
    if (this->gfx->getType() == D1CEL_TYPE::SMK) {
        D1GfxFrame *frame = this->gfx->getFrame(nextFrameIndex);
        if (!this->audioMuted) {
            D1Smk::playAudio(*frame);
        }
        QPointer<D1Pal>& pal = frame->getFramePal();
        if (!pal.isNull()) {
            dMainWindow().updatePalette(pal.data());
        }
    }
    int cycleType = this->ui->playComboBox->currentIndex();
    if (cycleType == 0) {
        // normal playback
        this->displayFrame();
    } else {
        dMainWindow().nextPaletteCycle((D1PAL_CYCLE_TYPE)(cycleType - 1));
        // this->displayFrame();
    }
/*
    nextTickNS += this->currentPlayDelay * 1000;
    qint64 now = timer.nsecsElapsed();
    int delta = (nextTick - now) / (1000 * 1000);
    if (delta > 0) {
        QTimer::singleShot(delta, Qt::PreciseTimer, this, &CelView::timerEvent);
    } else {
        this->timerEvent();
    }
*/
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
