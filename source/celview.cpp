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
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_celview.h"

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
}

void CelScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (this->panning) {
        QGraphicsScene::mouseMoveEvent(event);
        return;
    }
    if (event->buttons() != Qt::NoButton) {
        this->mouseEvent(event, false);
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
    // dMainWindow().openImageFiles(IMAGE_FILE_MODE::AUTO, filePaths, false);
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

    // If a pixel of the frame was clicked get pixel color index and notify the palette widgets
    // QObject::connect(&this->celScene, &CelScene::framePixelClicked, this, &CelView::framePixelClicked);
    // QObject::connect(&this->celScene, &CelScene::framePixelHovered, this, &CelView::framePixelHovered);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->groupIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_groupIndexEdit_escPressed()));

    // setup context menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));

    setAcceptDrops(true);
}

CelView::~CelView()
{
    delete ui;
}

void CelView::initialize(D1Pal *p, D1Hero *h, bool bottomPanelHidden)
{
    this->pal = p;
    this->hero = h;

    this->ui->bottomPanel->setVisible(!bottomPanelHidden);

    this->updateFields();
}

void CelView::setPal(D1Pal *p)
{
    this->pal = p;
}

void CelView::setHero(D1Hero *h)
{
    this->hero = h;
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
    CelView::setLabelContent(this->ui->celLabel, this->hero->getFilePath(), this->hero->isModified());
}

void CelView::updateFields()
{
    int count;

    this->updateLabel();
    // set play-delay text
    this->ui->playDelayEdit->setText(QString::number(this->currentPlayDelay));

    // Set current and maximum group text
    count = 0;
    this->ui->groupIndexEdit->setText(
        QString::number(count != 0 ? this->currentGroupIndex + 1 : 0));
    this->ui->groupNumberEdit->setText(QString::number(count));

    // Set current and maximum frame text
    int frameIndex = this->currentFrameIndex;
    this->ui->frameIndexEdit->setText(
        QString::number(count != 0 ? frameIndex + 1 : 0));
    this->ui->frameNumberEdit->setText(QString::number(count));
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
    /*QPoint p = pos;
    p -= QPoint(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    dMainWindow().frameClicked(frame, p, flags);*/
}

bool CelView::framePos(QPoint &pos) const
{
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

void CelView::displayFrame()
{
    this->updateFields();
    this->celScene.clear();

    // Getting the current frame to display
    QImage celFrame = QImage();

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
}

void CelView::toggleBottomPanel()
{
    this->ui->bottomPanel->setVisible(this->ui->bottomPanel->isHidden());
}

void CelView::setGroupIndex(int groupIndex)
{
}

void CelView::ShowContextMenu(const QPoint &pos)
{
    /*MainWindow *mw = &dMainWindow();
    QAction actions[8];

    QMenu contextMenu(this);
    contextMenu.setToolTipsVisible(true);

    contextMenu.exec(mapToGlobal(pos));*/
}

void CelView::on_framesGroupCheckBox_clicked()
{
    // update frameIndexEdit and frameNumberEdit
    this->updateFields();
}

void CelView::on_firstFrameButton_clicked()
{
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
    // dMainWindow().openImageFiles(IMAGE_FILE_MODE::AUTO, filePaths, false);
}
