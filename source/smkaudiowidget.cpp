#include "smkaudiowidget.h"

#include <QApplication>
#include <QCursor>
#include <QGraphicsView>
#include <QImage>
#include <QList>
#include <QMessageBox>
#include <QPainter>
#include <QString>

#include "config.h"
#include "celview.h"
#include "ui_smkaudiowidget.h"

SmkAudioWidget::SmkAudioWidget(CelView *parent)
    : QFrame(parent)
    , ui(new Ui::SmkAudioWidget())
{
    this->ui->setupUi(this);
    this->ui->audioGraphicsView->setScene(&this->audioScene);

    QLayout *layout = this->ui->centerButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogListView, tr("Move"), this, &SmkAudioWidget::on_movePushButtonClicked);
    layout = this->ui->rightButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogCloseButton, tr("Close"), this, &SmkAudioWidget::on_closePushButtonClicked);

    // cache the active graphics view
    QList<QGraphicsView *> views = parent->getCelScene()->views();
    this->graphView = views[0];
}

SmkAudioWidget::~SmkAudioWidget()
{
    delete ui;
}

void SmkAudioWidget::initialize(int frameIndex)
{
    this->currentFrameIndex = frameIndex;

    this->frameModified();
}

void SmkAudioWidget::setGfx(D1Gfx *g)
{
    this->gfx = g;

    if (g->getType() != D1CEL_TYPE::SMK) {
        this->hide();
    }
}

void SmkAudioWidget::show()
{
    this->resetPos();
    QFrame::show();

    this->setFocus(); // otherwise the widget does not receive keypress events...
    // update the view
    this->frameModified();
}

void SmkAudioWidget::hide()
{
    if (this->moving) {
        this->stopMove();
    }

    QFrame::hide();
}

void SmkAudioWidget::frameModified()
{
    D1SmkAudioData *frameAudio;
    unsigned long len;
    uint8_t *audioData;
    unsigned depth, width, height;

    if (this->isHidden())
        return;

    this->audioScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    frameAudio = this->gfx->getFrame(this->currentFrameIndex)->getFrameAudio();
    if (frameAudio != nullptr) {
        audioData = frameAudio->getAudio(this->currentTrack, &len) : nullptr;
        depth = frameAudio->getDepth();
        if (depth == 16)
            len /= 2;
    } else {
        len = 512;
        audioData = nullptr;
        depth = 8;
    }
    width = len;
    height = (256 * depth / 8);

    // Resize the scene rectangle to include some padding around the CEL frame
    this->audioScene.setSceneRect(0, 0,
        CEL_SCENE_MARGIN + width + CEL_SCENE_MARGIN,
        CEL_SCENE_MARGIN + height  + CEL_SCENE_MARGIN);

    // Building background of the width/height of the CEL frame
    QImage audioFrame = QImage(width, height, QImage::Format_ARGB32);

    if (audioData != nullptr) {
        QPainter audioPainter(&audioFrame);
        audioPainter.setPen(QColor(Config::getPaletteUndefinedColor())); // getPaletteSelectionBorderColor?

        for (unsigned long i = 0; i < width; i++) {
            audioPainter.drawLine(i, 256 - audioData[0], i, 256);
            audioData++;
            if (depth == 16) {
                audioPainter.drawLine(i, 256, i, 256 + audioData[0]);
                audioData++;
            }
        }
    }

    // Add the backgrond and CEL frame while aligning it in the center
    this->audioScene.addPixmap(QPixmap::fromImage(audioFrame))
        ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);

    this->adjustSize();
}

void SmkAudioWidget::on_trackComboBox_activated(int index)
{
    this->currentTrack = index;

    this->frameModified();
}

void SmkAudioWidget::on_closePushButtonClicked()
{
    this->hide();
}

void SmkAudioWidget::resetPos()
{
    if (!this->moved) {
        QSize viewSize = this->graphView->frameSize();
        QPoint viewBottomRight = this->graphView->mapToGlobal(QPoint(viewSize.width(), viewSize.height()));
        QSize mySize = this->frameSize();
        QPoint targetPos = viewBottomRight - QPoint(mySize.width(), mySize.height());
        QPoint relPos = this->mapFromGlobal(targetPos);
        QPoint destPos = relPos + this->pos();
        this->move(destPos);
    }
}

void SmkAudioWidget::stopMove()
{
    this->setMouseTracking(false);
    this->moving = false;
    this->moved = true;
    this->releaseMouse();
    // this->setCursor(Qt::BlankCursor);
}

void SmkAudioWidget::on_movePushButtonClicked()
{
    if (this->moving) { // this->hasMouseTracking()
        this->stopMove();
        return;
    }
    this->grabMouse(QCursor(Qt::ClosedHandCursor));
    this->moving = true;
    this->setMouseTracking(true);
    this->lastPos = QCursor::pos();
    this->setFocus();
    // this->setCursor(Qt::ClosedHandCursor);
}

void SmkAudioWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (this->moving) {
            this->stopMove();
        } else {
            this->hide();
        }
        return;
    }

    QFrame::keyPressEvent(event);
}

void SmkAudioWidget::mousePressEvent(QMouseEvent *event)
{
    if (this->moving) {
        this->stopMove();
        return;
    }

    QFrame::mouseMoveEvent(event);
}

void SmkAudioWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (this->moving) {
        QPoint currPos = QCursor::pos();
        QPoint wndPos = this->pos();
        wndPos += currPos - this->lastPos;
        this->lastPos = currPos;
        this->move(wndPos);
        // return;
    }

    QFrame::mouseMoveEvent(event);
}
