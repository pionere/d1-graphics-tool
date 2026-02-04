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
#include <QPainter>
#include <QString>

#include "config.h"
#include "celview.h"
#include "d1smk.h"
#include "d1wav.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_smkaudiowidget.h"

SmkAudioWidget::SmkAudioWidget(CelView *parent)
    : QDialog(parent)
    , ui(new Ui::SmkAudioWidget())
{
    this->setWindowFlags((this->windowFlags() & ~(Qt::WindowTitleHint | Qt::WindowCloseButtonHint)) | Qt::FramelessWindowHint);

    this->ui->setupUi(this);
    this->ui->audioGraphicsView->setScene(&this->audioScene);

    QLayout *layout = this->ui->leftButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_MediaPlay, tr("Play"), this, &SmkAudioWidget::on_playPushButtonClicked);
    PushButtonWidget::addButton(this, layout, QStyle::SP_TitleBarNormalButton, tr("Load chunk"), this, &SmkAudioWidget::on_loadChunkPushButtonClicked);
    PushButtonWidget::addButton(this, layout, QStyle::SP_TitleBarMaxButton, tr("Load track"), this, &SmkAudioWidget::on_loadTrackPushButtonClicked);
    layout = this->ui->centerButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogListView, tr("Move"), this, &SmkAudioWidget::on_movePushButtonClicked);
    layout = this->ui->rightButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogCancelButton, tr("Remove chunk"), this, &SmkAudioWidget::on_removeChunkPushButtonClicked);
    PushButtonWidget::addButton(this, layout, QStyle::SP_TrashIcon, tr("Remove track"), this, &SmkAudioWidget::on_removeTrackPushButtonClicked);
    this->muteBtn = PushButtonWidget::addButton(this, layout, QStyle::SP_MediaVolume, tr("Mute"), this, &SmkAudioWidget::on_mutePushButtonClicked);
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
    D1SmkAudioData *frameAudio;
    unsigned long audioDataLen;

    this->currentFrameIndex = frameIndex;

    if (frameIndex >= 0) {
        frameAudio = this->gfx->getFrame(frameIndex)->getFrameAudio();
        if (frameAudio != nullptr) {
            if (this->currentTrack == -1 && frameAudio->getAudio(0, &audioDataLen) != nullptr) {
                this->currentTrack = 0;
            }
        }
    } else {
        this->hide();
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

#define SMK_AUDIO_SCENE_MARGIN 0
#define SMK_AUDIO_HEIGHT 256

void SmkAudioWidget::frameModified()
{
    D1SmkAudioData *frameAudio;
    unsigned long audioDataLen, audioLen;
    uint8_t *audioData;
    unsigned bitWidth, channels, bitRate, width, height;
    int track;
    bool compress;

    if (this->isHidden())
        return;

    frameAudio = this->gfx->getFrame(this->currentFrameIndex)->getFrameAudio();
    track = this->currentTrack;
    if (frameAudio != nullptr) {
        if ((unsigned)track >= D1SMK_TRACKS) {
            track = 0;
        }
        channels = frameAudio->getChannels();
        bitWidth = frameAudio->getBitDepth() / 8;
        audioData = frameAudio->getAudio(track, &audioDataLen);
        compress = frameAudio->getCompress(track) != 0;
    } else {
        // track = -1;
        channels = 0;
        bitWidth = 1;
        audioData = nullptr;
        audioDataLen = 0;
        compress = false;
    }
    audioLen = (channels == 0 || bitWidth == 0) ? audioDataLen : (audioDataLen / (channels * bitWidth));

    // update fields
    bool hasAudio = frameAudio != nullptr;
    this->ui->trackComboBox->clear();
    this->ui->channelsLabel->setText("");
    this->ui->bitWidthLabel->setText("");
    this->ui->bitRateLabel->setText("");
    this->ui->audioLenLineEdit->setText("");
    this->ui->audioLenLineEdit->setToolTip("");
    this->ui->trackComboBox->setEnabled(hasAudio);
    this->ui->audioCompressCheckBox->setChecked(compress);
    this->ui->audioCompressCheckBox->setEnabled(hasAudio);
    if (hasAudio) {
        // - tracks
        for (int i = 0; i < D1SMK_TRACKS; i++) {
            unsigned long trackLen;
            frameAudio->getAudio(i, &trackLen);
            QString label = trackLen != 0 ? tr("Track %1") : tr("- Track %1 -");
            this->ui->trackComboBox->addItem(label.arg(i + 1), i);
        }
        this->ui->trackComboBox->setCurrentIndex(track);

        // - channels
        this->ui->channelsLabel->setText(QString::number(channels));

        // - bitWidth
        this->ui->bitWidthLabel->setText(tr("%1bit").arg(bitWidth * 8));

        // - bitRate
        bitRate = frameAudio->getBitRate();
        this->ui->bitRateLabel->setText(tr("%1Hz").arg(bitRate));

        // - audio length
        this->ui->audioLenLineEdit->setText(QString::number(audioLen));
        this->ui->audioLenLineEdit->setToolTip(bitRate == 0 ? tr("N/A") : tr("%1us").arg((uint64_t)audioLen * 1000000 / bitRate));
    }

    // update the scene
    this->audioScene.clear();
    this->audioScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    width = audioLen;
    height = SMK_AUDIO_HEIGHT * channels; // * bitWidth

    // Resize the scene rectangle to include some padding around the CEL frame
    this->audioScene.setSceneRect(0, 0,
        SMK_AUDIO_SCENE_MARGIN + width + SMK_AUDIO_SCENE_MARGIN,
        SMK_AUDIO_SCENE_MARGIN + height  + SMK_AUDIO_SCENE_MARGIN);

    // Building background of the width/height of the CEL frame
    QImage audioFrame = QImage(width, height, QImage::Format_ARGB32);
    audioFrame.fill(QColor(Config::getGraphicsTransparentColor()));

    if (audioData != nullptr && channels != 0 && bitWidth != 0) {
        QPainter audioPainter(&audioFrame);
        audioPainter.setPen(QColor(Config::getPaletteUndefinedColor())); // getPaletteSelectionBorderColor?

        for (unsigned long i = 0; i < audioDataLen / (bitWidth * channels); i++) {
            for (unsigned ch = 0; ch < channels; ch++) {
                int value;
                if (bitWidth == 1) {
                    value = (INT8_MAX + 1 + *(int8_t*)&audioData[0]) * height / (UINT8_MAX  + 1);
                } else {
                    value = (INT16_MAX + 1 + *(int16_t*)&audioData[0]) * height / (UINT16_MAX  + 1);
                }
                audioPainter.drawLine(i, height - value + ch * SMK_AUDIO_HEIGHT, i, height + ch * SMK_AUDIO_HEIGHT);

                audioData += bitWidth;
            }
        }
    }

    // Add the backgrond and CEL frame while aligning it in the center
    this->audioScene.addPixmap(QPixmap::fromImage(audioFrame))
        ->setPos(SMK_AUDIO_SCENE_MARGIN, SMK_AUDIO_SCENE_MARGIN);
}

void SmkAudioWidget::on_trackComboBox_activated(int index)
{
    this->currentTrack = index;

    this->frameModified();
}

void SmkAudioWidget::on_audioCompressCheckBox_clicked()
{
    int track = this->currentTrack;
    uint8_t compress = this->ui->audioCompressCheckBox->isChecked() ? 1 : 0;
    bool result = false;

    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        D1GfxFrame *frame = this->gfx->getFrame(i);
        D1SmkAudioData *frameAudio = frame->getFrameAudio();
        if (frameAudio != nullptr) {
            result |= frameAudio->setCompress(track, compress);
        }
    }
    if (result) {
        this->gfx->setModified();
        // update the window
        this->frameModified();
        // update the main view
        ((CelView *)this->parent())->displayFrame();
    }
}

void SmkAudioWidget::on_playPushButtonClicked()
{
    int frame, track;

    frame = this->currentFrameIndex;
    track = this->currentTrack;
    if (frame >= 0 && track != -1) {
        D1Smk::playAudio(*this->gfx->getFrame(frame), track);
    }
}

void SmkAudioWidget::on_loadChunkPushButtonClicked()
{
    int frame, track;

    frame = this->currentFrameIndex;
    if (frame < 0) {
        frame = 0;
    }
    track = this->currentTrack;
    if (track < 0) {
        track = 0;
    }
    int frameCount = this->gfx->getFrameCount();
    if (frameCount > 0) {
        QStringList filePaths = dMainWindow().filesDialog(tr("Select Audio"), tr("WAV Files (*.wav *.WAV)"));
        ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_NONE);
        int fileCount = filePaths.count();
        bool wavLoaded = false;
        for (int i = 0; i < fileCount; i++) {
            if (frame + i >= frameCount) {
                dProgressWarn() << QApplication::tr("Not enough frames for the audio files. The last %n audio file(s) are ignored.", "", fileCount - i);
                break;
            }
            wavLoaded |= D1Wav::load(*this->gfx->getFrame(frame + i), track, filePaths[i]);
        }
        ProgressDialog::done();
        if (wavLoaded) {
            this->currentTrack = track;
            this->gfx->setModified();
            // update the window
            this->frameModified();
            // update the main view
            ((CelView *)this->parent())->displayFrame();
        }
    }
}

void SmkAudioWidget::on_loadTrackPushButtonClicked()
{
    int track;

    track = this->currentTrack;
    if (track < 0) {
        track = 0;
    }
    int frameCount = this->gfx->getFrameCount();
    if (frameCount > 0) {
        QStringList filePaths = dMainWindow().filesDialog(tr("Select Audio"), tr("WAV Files (*.wav *.WAV)"));
        ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 0, PAF_NONE);
        int fileCount = filePaths.count();
        bool wavLoaded = false;
        for (int i = 0; i < fileCount; i++) {
            if (track + i >= D1SMK_TRACKS) {
                dProgressWarn() << QApplication::tr("Not enough tracks for the audio files. The last %n audio file(s) are ignored.", "", fileCount - i);
                break;
            }
            wavLoaded |= D1Wav::load(*this->gfx, track + i, filePaths[i]);
        }
        ProgressDialog::done();
        if (wavLoaded) {
            this->currentTrack = track;
            this->gfx->setModified();
            // update the window
            this->frameModified();
            // update the main view
            ((CelView *)this->parent())->displayFrame();
        }
    }
}

void SmkAudioWidget::on_removeChunkPushButtonClicked()
{
    int frameIdx, track;

    frameIdx = this->currentFrameIndex;
    if (frameIdx < 0) {
        frameIdx = 0;
    }
    track = this->currentTrack;

    int frameCount = this->gfx->getFrameCount();
    if (track >= 0 && frameCount > frameIdx) {
        D1GfxFrame *frame = this->gfx->getFrame(frameIdx);
        D1SmkAudioData *frameAudio = frame->getFrameAudio();
        bool result = false;
        if (frameAudio != nullptr) {
            result |= frameAudio->setAudio(track, nullptr, 0);
        }
        if (result) {
            this->gfx->setModified();
            // update the window
            this->frameModified();
            // update the main view
            ((CelView *)this->parent())->displayFrame();
        }
    }
}

void SmkAudioWidget::on_removeTrackPushButtonClicked()
{
    int track = this->currentTrack;
    bool result = false;

    if (track >= 0) {
        for (int i = 0; i < this->gfx->getFrameCount(); i++) {
            D1GfxFrame *frame = this->gfx->getFrame(i);
            D1SmkAudioData *frameAudio = frame->getFrameAudio();
            if (frameAudio != nullptr) {
                result |= frameAudio->setAudio(track, nullptr, 0);
            }
        }
    }
    if (result) {
        this->gfx->setModified();
        // update the window
        this->frameModified();
        // update the main view
        ((CelView *)this->parent())->displayFrame();
    }
}

void SmkAudioWidget::on_mutePushButtonClicked()
{
    bool muted = ((CelView *)this->parent())->toggleMute();
    this->muteBtn->setIcon(QApplication::style()->standardIcon(muted ? QStyle::SP_MediaVolumeMuted : QStyle::SP_MediaVolume));
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
    if (event->matches(QKeySequence::Cancel)) {
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
