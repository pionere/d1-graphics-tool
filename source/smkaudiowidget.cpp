#include "smkaudiowidget.h"

#include <QApplication>
#include <QCursor>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QImage>
#include <QList>
#include <QMessageBox>
#include <QPainter>
#include <QString>

#include "config.h"
#include "celview.h"
#include "d1smk.h"
#include "pushbuttonwidget.h"
#include "ui_smkaudiowidget.h"

SmkAudioWidget::SmkAudioWidget(CelView *parent)
    : QDialog(parent)
    , ui(new Ui::SmkAudioWidget())
{
    this->ui->setupUi(this);
    this->ui->audioGraphicsView->setScene(&this->audioScene);

    QLayout *layout = this->ui->centerButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogListView, tr("Move"), this, &SmkAudioWidget::on_movePushButtonClicked);
    layout = this->ui->rightButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogCloseButton, tr("Close"), this, &SmkAudioWidget::on_closePushButtonClicked);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->bitRateLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_bitRateLineEdit_escPressed()));

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
    unsigned long audioLen;
    uint8_t *audioData;
    unsigned bitWidth, channels, width, height;
    int track, channel;

    if (this->isHidden())
        return;

    frameAudio = this->gfx->getFrame(this->currentFrameIndex)->getFrameAudio();
    track = this->currentTrack;
    channel = this->currentChannel;
    if (frameAudio != nullptr) {
        if ((unsigned)track >= D1SMK_TRACKS) {
            track = 0;
        }
        if ((unsigned)channel >= D1SMK_CHANNELS) {
            channel = 0;
        }
        channels = frameAudio->getChannels();
        bitWidth = frameAudio->getBitDepth() / 8;
        audioData = frameAudio->getAudio(track, &audioLen);
        audioLen /= bitWidth;
    } else {
        // track = -1;
        // channel = -1;
        channels = 1;
        bitWidth = 1;
        audioData = nullptr;
        audioLen = 0;
    }

    // update fields
    bool hasAudio = audioData != nullptr;
    this->ui->trackComboBox->clear();
    this->ui->channelComboBox->clear();
    this->ui->bitRateLineEdit->setText("");
    this->ui->trackComboBox->setEnabled(hasAudio);
    this->ui->channelComboBox->setEnabled(hasAudio);
    this->ui->bitRateLineEdit->setEnabled(hasAudio);
    if (hasAudio) {
        // - tracks
        for (int i = 0; i < D1SMK_TRACKS; i++) {
            unsigned long trackLen;
            frameAudio->getAudio(i, &trackLen);
            QString label = trackLen != 0 ? tr("Track %1") : tr("<i>Track %1</i>");
            this->ui->trackComboBox->addItem(label.arg(i + 1), i);
        }
        this->ui->trackComboBox->setCurrentIndex(track);

        // - channels
        for (unsigned i = 0; i < D1SMK_CHANNELS; i++) {
            QString label = channels > i ? tr("Channel %1") : tr("<i>Channel %1</i>");
            this->ui->channelComboBox->addItem(label.arg(i + 1), i);
        }
        this->ui->channelComboBox->setCurrentIndex(channel);

        // - bitRate
        this->ui->bitRateLineEdit->setText(QString::number(frameAudio->getBitRate()));
    }

    // update the scene
    this->audioScene.clear();
    this->audioScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    width = audioLen / channels;
    if (width < 512) {
        width = 512;
    }
    height = 256; // (256 * bitWidth);

    // Resize the scene rectangle to include some padding around the CEL frame
    this->audioScene.setSceneRect(0, 0,
        CEL_SCENE_MARGIN + width + CEL_SCENE_MARGIN,
        CEL_SCENE_MARGIN + height  + CEL_SCENE_MARGIN);

    // Building background of the width/height of the CEL frame
    QImage audioFrame = QImage(width, height, QImage::Format_ARGB32);

    if (audioData != nullptr) {
        QPainter audioPainter(&audioFrame);
        audioPainter.setPen(QColor(Config::getPaletteUndefinedColor())); // getPaletteSelectionBorderColor?

        for (unsigned long i = 0; i < audioLen; i++) {
            if ((i % channels) == channel) {
                int value;
                if (bitWidth == 1) {
                    value = (INT8_MAX + 1 + *(int8_t*)&audioData[0]) * height / (UINT8_MAX  + 1);
                } else {
                    value = (INT16_MAX + 1 + *(int16_t*)&audioData[0]) * height / (UINT16_MAX  + 1);
                }
                audioPainter.drawLine(i, height - value, i, height);
            }
            audioData += bitWidth;
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

void SmkAudioWidget::on_channelComboBox_activated(int index)
{
    this->currentChannel = index;

    this->frameModified();
}

void SmkAudioWidget::on_bitRateLineEdit_returnPressed()
{
    unsigned bitRate = this->ui->bitRateLineEdit->text().toUInt();

    D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
    D1SmkAudioData *frameAudio = frame->getFrameAudio();

    if (frameAudio != nullptr && frameAudio->setBitRate(bitRate)) {
        this->gfx->setModified();
    }

    this->on_bitRateLineEdit_escPressed();
}

void SmkAudioWidget::on_bitRateLineEdit_escPressed()
{
    // update frameIndexEdit
    this->frameModified();
    this->ui->bitRateLineEdit->clearFocus();
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
