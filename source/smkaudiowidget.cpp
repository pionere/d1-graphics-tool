#include "smkaudiowidget.h"

#include <QApplication>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QBuffer>
#include <QByteArray>
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

#include "dungeon/all.h"

SmkAudioWidget::SmkAudioWidget(CelView *parent)
    : QDialog(parent)
    , ui(new Ui::SmkAudioWidget())
{
    this->setWindowFlags((this->windowFlags() & ~(Qt::WindowTitleHint | Qt::WindowCloseButtonHint)) | Qt::FramelessWindowHint);

    this->ui->setupUi(this);
    this->ui->audioGraphicsView->setScene(&this->audioScene);

    QLayout *layout = this->ui->leftButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_MediaPlay, tr("Play"), this, &SmkAudioWidget::on_playPushButtonClicked);
    layout = this->ui->centerButtonsHorizontalLayout;
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
    D1SmkAudioData *frameAudio;
    unsigned long audioDataLen;

    this->currentFrameIndex = frameIndex;

    if (frameIndex >= 0) {
        frameAudio = this->gfx->getFrame(frameIndex)->getFrameAudio();
        if (frameAudio != nullptr) {
            if (this->currentTrack == -1 && frameAudio->getAudio(0, &audioDataLen) != nullptr) {
                this->currentTrack = 0;
            }
            if (this->currentChannel == -1 && frameAudio->getChannels() != 0) {
                this->currentChannel = 0;
            }
        }
    }

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
    QDialog::show();

    this->setFocus(); // otherwise the widget does not receive keypress events...
    // update the view
    this->frameModified();
}

void SmkAudioWidget::hide()
{
    if (this->moving) {
        this->stopMove();
    }

    QDialog::hide();
}

void SmkAudioWidget::frameModified()
{
    D1SmkAudioData *frameAudio;
    unsigned long audioDataLen, audioLen;
    uint8_t *audioData;
    unsigned bitWidth, channels, bitRate, width, height;
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
//		LogErrorF("frameModified getAudio %d", track);
        audioData = frameAudio->getAudio(track, &audioDataLen);
//		LogErrorF("frameModified gotAudio %d len %d", audioData != nullptr, audioDataLen);
    } else {
        // track = -1;
        // channel = -1;
        channels = 0;
        bitWidth = 1;
        audioData = nullptr;
        audioDataLen = 0;
    }
    audioLen = (channels == 0 || bitWidth == 0) ? audioDataLen : (audioDataLen / (channels * bitWidth));

    // update fields
    bool hasAudio = frameAudio != nullptr;
    this->ui->trackComboBox->clear();
    this->ui->channelComboBox->clear();
    this->ui->bitRateLineEdit->setText("");
    this->ui->audioLenLineEdit->setText("");
    this->ui->audioLenLabel->setText("");
    this->ui->trackComboBox->setEnabled(hasAudio);
    this->ui->channelComboBox->setEnabled(hasAudio);
    this->ui->bitRateLineEdit->setEnabled(hasAudio);
    if (hasAudio) {
//		LogErrorF("frameModified hasAudio 0");
        // - tracks
        for (int i = 0; i < D1SMK_TRACKS; i++) {
            unsigned long trackLen;
            frameAudio->getAudio(i, &trackLen);
            QString label = trackLen != 0 ? tr("Track %1") : tr("- Track %1 -");
            this->ui->trackComboBox->addItem(label.arg(i + 1), i);
        }
        this->ui->trackComboBox->setCurrentIndex(track);
//		LogErrorF("frameModified hasAudio 1 %d", track);

        // - channels
        for (unsigned i = 0; i < D1SMK_CHANNELS; i++) {
            QString label = channels > i ? tr("Channel %1") : tr("- Channel %1 -");
            this->ui->channelComboBox->addItem(label.arg(i + 1), i);
        }
        this->ui->channelComboBox->setCurrentIndex(channel);
//		LogErrorF("frameModified hasAudio 2 %d", channel);

        // - bitRate
        bitRate = frameAudio->getBitRate();
        this->ui->bitRateLineEdit->setText(QString::number(bitRate));

        // - audio length
        this->ui->audioLenLineEdit->setText(QString::number(audioLen));
        this->ui->audioLenLabel->setText(bitRate == 0 ? tr("N/A") : tr("%1us").arg((uint64_t)audioLen * 1000000 / bitRate));
    }

    // update the scene
    this->audioScene.clear();
    this->audioScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    width = audioLen;
    if (width < 512) {
        width = 512;
    }
    height = 256; // (256 * bitWidth);
//	LogErrorF("frameModified updatescene 2 %d", channel);

    // Resize the scene rectangle to include some padding around the CEL frame
    this->audioScene.setSceneRect(0, 0,
        CEL_SCENE_MARGIN + width + CEL_SCENE_MARGIN,
        CEL_SCENE_MARGIN + height  + CEL_SCENE_MARGIN);

    // Building background of the width/height of the CEL frame
    QImage audioFrame = QImage(width, height, QImage::Format_ARGB32);
    audioFrame.fill(QColor(Config::getGraphicsTransparentColor()));

    if (audioData != nullptr) {
        QPainter audioPainter(&audioFrame);
        audioPainter.setPen(QColor(Config::getPaletteUndefinedColor())); // getPaletteSelectionBorderColor?

        for (unsigned long i = 0; i < audioDataLen / bitWidth; i++) {
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

    // this->adjustSize();
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

void SmkAudioWidget::on_playPushButtonClicked()
{
    D1SmkAudioData *frameAudio;
    unsigned long audioDataLen, audioLen;
    uint8_t *audioData;
    unsigned bitDepth, channels, bitRate, width, height;
    int frame, track, channel;

    frame = this->currentFrameIndex;
    if (frame >= 0) {
        frameAudio = this->gfx->getFrame(frame)->getFrameAudio();
        track = this->currentTrack;
        channel = this->currentChannel;
        if (frameAudio != nullptr && (unsigned)track < D1SMK_TRACKS && (unsigned)channel < D1SMK_CHANNELS) {
            channels = frameAudio->getChannels();
            bitDepth = frameAudio->getBitDepth();
            bitRate = frameAudio->getBitRate();

            audioData = frameAudio->getAudio(track, &audioDataLen);
            QByteArray* arr = new QByteArray((char *)audioData, audioDataLen);
            QBuffer *input = new QBuffer(arr);
            input->setBuffer(arr);
            input->open(QIODevice::ReadOnly);
			// QMessageBox::critical(this, "Error", tr("Playing audio rate%1 cha%2 ss%3").arg(bitRate).arg(channels).arg(bitDepth));
            QAudioFormat m_audioFormat = QAudioFormat();
            m_audioFormat.setSampleRate(bitRate);
            m_audioFormat.setChannelCount(channels);
            m_audioFormat.setSampleSize(bitDepth);
            m_audioFormat.setCodec("audio/pcm");
            m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
            m_audioFormat.setSampleType(QAudioFormat::SignedInt);

            QAudioOutput *audio = new QAudioOutput(m_audioFormat, this);

            // connect up signal stateChanged to a lambda to get feedback
            connect(audio, &QAudioOutput::stateChanged, [audio, input, arr](QAudio::State newState)
            {
                if (newState == QAudio::IdleState) {   // finished playing (i.e., no more data)
                    // qWarning() << "finished playing sound";
                    audio->stop();
                    delete audio;
                    delete input;
                    delete arr;
                }
				QMessageBox::critical(nullptr, "Error", tr("Play state %1 idle%2 active%3 ss%4 sus%5").arg(newState).arg(newState == QAudio::IdleState).arg(newState == QAudio::ActiveState).arg(newState == QAudio::StoppedState).arg(newState == QAudio::SuspendedState));
            });

            // start the audio (i.e., play sound from the QAudioOutput object that we just created)
            audio->start(input);
        }
    }
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

    QDialog::keyPressEvent(event);
}

void SmkAudioWidget::mousePressEvent(QMouseEvent *event)
{
    if (this->moving) {
        this->stopMove();
        return;
    }

    QDialog::mouseMoveEvent(event);
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

    QDialog::mouseMoveEvent(event);
}
