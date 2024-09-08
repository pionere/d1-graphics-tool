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

#include "dungeon/defs.h"

#include "dungeon/interfac.h"

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

/*void CelScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    this->dragMoveEvent(event);
}

void CelScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (MainWindow::hasHeroUrl(event->mimeData())) {
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
    dMainWindow().openFiles(filePaths);
}*/

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
    QObject::connect(this->ui->heroNameEdit, SIGNAL(cancel_signal()), this, SLOT(on_heroNameEdit_escPressed()));
    QObject::connect(this->ui->heroLevelEdit, SIGNAL(cancel_signal()), this, SLOT(on_heroLevelEdit_escPressed()));

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
    CelView::setLabelContent(this->ui->heroLabel, this->hero->getFilePath(), this->hero->isModified());
}

void CelView::updateFields()
{
    QLabel *label;
    this->updateLabel();

    // set texts
    this->ui->heroNameEdit->setText(this->hero->getName());

    this->ui->heroClassComboBox->setCurrentIndex(this->hero->getClass());

    this->ui->heroLevelEdit->setText(QString::number(this->hero->getLevel()));

    int statPts = this->hero->getStatPoints();
    this->ui->heroStatPtsLabel->setText(QString::number(statPts));
    this->ui->heroAddStrengthButton->setEnabled(statPts > 0);
    this->ui->heroAddDexterityButton->setEnabled(statPts > 0);
    this->ui->heroAddMagicButton->setEnabled(statPts > 0);
    this->ui->heroAddVitalityButton->setEnabled(statPts > 0);

    label = this->ui->heroStrengthLabel;
    label->setText(QString::number(this->hero->getStrength()));
    label->setToolTip(QString::number(this->hero->getBaseStrength()));
    label = this->ui->heroDexterityLabel;
    label->setText(QString::number(this->hero->getDexterity()));
    label->setToolTip(QString::number(this->hero->getBaseDexterity()));
    label = this->ui->heroMagicLabel;
    label->setText(QString::number(this->hero->getMagic()));
    label->setToolTip(QString::number(this->hero->getBaseMagic()));
    label = this->ui->heroVitalityLabel;
    label->setText(QString::number(this->hero->getVitality()));
    label->setToolTip(QString::number(this->hero->getBaseVitality()));

    label = this->ui->heroLifeLabel;
    label->setText(QString::number(this->hero->getLife()));
    label->setToolTip(QString::number(this->hero->getBaseLife()));
    label = this->ui->heroManaLabel;
    label->setText(QString::number(this->hero->getMana()));
    label->setToolTip(QString::number(this->hero->getBaseMana()));

    this->ui->heroMagicResist->setText(QString("%1%").arg(this->hero->getMagicResist()));
    this->ui->heroFireResist->setText(QString("%1%").arg(this->hero->getFireResist()));
    this->ui->heroLightningResist->setText(QString("%1%").arg(this->hero->getLightningResist()));
    this->ui->heroAcidResist->setText(QString("%1%").arg(this->hero->getAcidResist()));
}

CelScene *CelView::getCelScene() const
{
    return const_cast<CelScene *>(&this->celScene);
}

void CelView::framePixelClicked(const QPoint &pos, int flags)
{
    QMessageBox::critical(nullptr, "Error", QStringLiteral("Clicked %1:%2").arg(pos.x()).arg(pos.y()));
    /*QPoint p = pos;
    p -= QPoint(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    dMainWindow().frameClicked(frame, p, flags);*/
}

bool CelView::framePos(QPoint &pos) const
{
    return true;
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
    QImage celFrame = this->hero->getEquipmentImage();

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

    // Notify PalView that the frame changed (used to refresh palette widget)
    emit this->frameRefreshed();
}

void CelView::toggleBottomPanel()
{
    this->ui->bottomPanel->setVisible(this->ui->bottomPanel->isHidden());
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

void CelView::on_heroNameEdit_returnPressed()
{
    QString name = this->ui->heroNameEdit->text();

    this->hero->setName(name);
    this->ui->heroNameEdit->setText(this->hero->getName());

    this->on_heroNameEdit_escPressed();
}

void CelView::on_heroNameEdit_escPressed()
{
    // update heroNameEdit
    this->updateFields();
    this->ui->heroNameEdit->clearFocus();
}

void CelView::on_heroLevelEdit_returnPressed()
{
    int level = this->ui->heroLevelEdit->text().toShort();

    if (level >= 1 && level < MAXCHARLEVEL) {
        this->hero->setLevel(level);
    }

    this->on_heroLevelEdit_escPressed();
}

void CelView::on_heroLevelEdit_escPressed()
{
    // update heroLevelEdit
    this->updateFields();
    this->ui->heroLevelEdit->clearFocus();
}

void CelView::on_heroDecLifeButton_clicked()
{
    this->hero->decLife();
    this->updateFields();
}

void CelView::on_heroRestoreLifeButton_clicked()
{
    this->hero->restoreLife();
    this->updateFields();
}

void CelView::on_heroAddStrengthButton_clicked()
{
    this->hero->addStrength();
    this->updateFields();
}

void CelView::on_heroAddDexterityButton_clicked()
{
    this->hero->addDexterity();
    this->updateFields();
}

void CelView::on_heroAddMagicButton_clicked()
{
    this->hero->addMagic();
    this->updateFields();
}

void CelView::on_heroAddVitalityButton_clicked()
{
    this->hero->addVitality();
    this->updateFields();
}

/*void CelView::dragEnterEvent(QDragEnterEvent *event)
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
}*/
