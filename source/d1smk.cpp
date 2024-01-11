#include "d1smk.h"

#include <QApplication>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QList>
#include <QMessageBox>
#include <QObject>

#include "config.h"
#include "progressdialog.h"

#include "dungeon/all.h"
//#include <smacker.h>
//#include "../3rdParty/libsmacker/smacker.h"
#include "libsmacker/smacker.h"

/*typedef struct SmkAudioPlayer {
    QAudioOutput *output;
    QBuffer audioBuffer;
    QByteArray audioData;
} SmkAudioPlayer;
static QList<SmkAudioPlayer *> audioPlayers;*/

static bool audioSemaphore = false;
static QAudioOutput *audioOutput = nullptr;
static QByteArray *audioBytes = nullptr;
static QBuffer *audioBuffer = nullptr;
static QList<QPair<uint8_t *, unsigned long>> audioQueue;

#define D1SMK_COLORS 256

D1SmkAudioData::D1SmkAudioData(unsigned ch, unsigned bd, unsigned long br)
    : channels(ch)
    , bitDepth(bd)
    , bitRate(br)
{
}

D1SmkAudioData::~D1SmkAudioData()
{
    for (int i = 0; i < D1SMK_TRACKS; i++) {
        mem_free_dbg(this->audio[i]);
    }
}

unsigned D1SmkAudioData::getChannels() const
{
    return this->channels;
}

unsigned D1SmkAudioData::getBitDepth() const
{
    return this->bitDepth;
}

bool D1SmkAudioData::setBitRate(unsigned br)
{
    if (this->bitRate == br) {
        return false;
    }
    this->bitRate = br;
    return true;
}

unsigned D1SmkAudioData::getBitRate() const
{
    return this->bitRate;
}

void D1SmkAudioData::setAudio(unsigned track, uint8_t* data, unsigned long len)
{
    this->audio[track] = data;
    this->len[track] = len;
}

uint8_t* D1SmkAudioData::getAudio(unsigned track, unsigned long *len)
{
    *len = this->len[track];
    return this->audio[track];
}

static D1Pal* LoadPalette(smk SVidSMK)
{
    D1Pal *pal = new D1Pal();

    const unsigned char *smkPal = smk_get_palette(SVidSMK);
    for (int i = 0; i < D1SMK_COLORS; i++) {
        pal->setColor(i, QColor(smkPal[i * 3 + 0], smkPal[i * 3 + 1], smkPal[i * 3 + 2]));
    }
    return pal;
}

static void RegisterPalette(D1Pal *pal, unsigned frameFrom, unsigned frameTo, QMap<QString, D1Pal *> &pals)
{
    QString palPath = QString("Frame%1-%2").arg(frameFrom, 4, 10, QChar('0')).arg((int)frameTo, 4, 10, QChar('0'));
    pal->setFilePath(palPath);
    pals[palPath] = pal;
}

bool D1Smk::load(D1Gfx &gfx, QMap<QString, D1Pal *> &pals, const QString &filePath, const OpenAsParam &params)
{
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        dProgressErr() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    const quint64 fileSize = file.size();
    unsigned char *SVidBuffer = (unsigned char *)malloc(fileSize);
    if (file.read((char *)SVidBuffer, fileSize) != fileSize) {
        MemFreeDbg(SVidBuffer);
        dProgressErr() << QApplication::tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    smk SVidSMK = smk_open_memory(SVidBuffer, fileSize);
    if (SVidSMK == NULL) {
        MemFreeDbg(SVidBuffer);
        dProgressErr() << QApplication::tr("Invalid SMK file.");
        return false;
    }

    unsigned long SVidWidth, SVidHeight;
    double SVidFrameLength;
    smk_info_all(SVidSMK, &SVidWidth, &SVidHeight, &SVidFrameLength);
    unsigned char channels, depth;
    unsigned long rate;
    smk_info_audio(SVidSMK, &channels, &depth, &rate);
    smk_enable_video(SVidSMK, true);
    for (int i = 0; i < D1SMK_TRACKS; i++) {
        smk_enable_audio(SVidSMK, i, true);
    }
    // Decode first frame
    char result = smk_first(SVidSMK);
    if (SMK_ERR(result)) {
        MemFreeDbg(SVidBuffer);
        dProgressErr() << QApplication::tr("Empty SMK file.");
        return false;
    }
    // load the first palette
    D1Pal *pal = LoadPalette(SVidSMK);
    // load the frames
    // gfx.frames.clear();
    if (params.celWidth != 0) {
        dProgressWarn() << QApplication::tr("Width setting is ignored when a SMK file is loaded.");
    }
    const bool clipped = params.clipped == OPEN_CLIPPED_TYPE::TRUE;
    unsigned frameNum = 0;
    unsigned prevPalFrame = 0;
    const unsigned char *smkFrame = smk_get_video(SVidSMK);
    do {
//    LogErrorF("Smk load %d", frameNum);
        bool palUpdate = smk_palette_updated(SVidSMK);
        if (palUpdate && frameNum != 0) {
            RegisterPalette(pal, prevPalFrame, frameNum, pals);
            prevPalFrame = frameNum;
            // load the new palette
            pal = LoadPalette(SVidSMK);
        }
        // create a new frame
        D1GfxFrame *frame = new D1GfxFrame();
        frame->clipped = clipped;
        frame->framePal = palUpdate ? pal : nullptr;
        const unsigned char *smkFrameCursor = smkFrame;
        for (unsigned y = 0; y < SVidHeight; y++) {
            std::vector<D1GfxPixel> pixelLine;
            for (unsigned x = 0; x < SVidWidth; x++, smkFrameCursor++) {
                pixelLine.push_back(D1GfxPixel::colorPixel(*smkFrameCursor));
            }
            frame->addPixelLine(std::move(pixelLine));
        }

        D1SmkAudioData *audio = new D1SmkAudioData(channels, depth, rate);
        for (unsigned i = 0; i < D1SMK_TRACKS; i++) {
            unsigned long len = smk_get_audio_size(SVidSMK, i);
            unsigned char* ct = nullptr;
            if (len != 0) {
                unsigned char* track = smk_get_audio(SVidSMK, i);
                ct = (unsigned char *)malloc(len);
                memcpy(ct, track, len);
            }
            audio->setAudio(i, ct, len);
        }
        frame->frameAudio = audio;

        gfx.frames.append(frame);
        frameNum++;
//    LogErrorF("Smk load %d..", frameNum);
    } while ((result = smk_next(SVidSMK)) == SMK_MORE);

    if (SMK_ERR(result)) {
        dProgressErr() << QApplication::tr("SMK not fully loaded.");
    }
    RegisterPalette(pal, prevPalFrame, frameNum, pals);

    smk_close(SVidSMK);
    MemFreeDbg(SVidBuffer);

    gfx.groupFrameIndices.clear();
    gfx.groupFrameIndices.push_back(std::pair<int, int>(0, frameNum - 1));

    gfx.type = D1CEL_TYPE::SMK;
    gfx.frameLen = SVidFrameLength;

    gfx.gfxFilePath = filePath;
    gfx.modified = false;
//    LogErrorF("Smk load done");
    return true;
}

bool D1Smk::save(D1Gfx &gfx, const SaveAsParam &params)
{
    dProgressErr() << QApplication::tr("Not supported.");
    return false;
}

static void audioCallback(QAudio::State newState)
{
    if (newState == QAudio::IdleState) {   // finished playing (i.e., no more data)
        // qWarning() << "finished playing sound";
        if (!audioQueue.isEmpty() && !audioSemaphore) {
            audioSemaphore = true;
            QPair<uint8_t *, unsigned long> audioData = audioQueue[0];
            audioQueue.pop_front();
            audioBytes->setRawData(audioData.first, audioData.second);
            audioOutput->start(audioBuffer);
            audioSemaphore = false;
        }
    }
}

void D1Smk::playAudio(D1GfxFrame &gfxFrame, int track, int channel)
{
    D1SmkAudioData *frameAudio;
    unsigned long audioDataLen;
    uint8_t *audioData;
    unsigned bitDepth, channels, bitRate;

    frameAudio = gfxFrame.getFrameAudio();
    if (track == -1)
        track = 0;
    if (channel == -1)
        channel = 0;
    if (frameAudio != nullptr && (unsigned)track < D1SMK_TRACKS && (unsigned)channel < D1SMK_CHANNELS) {
        channels = frameAudio->getChannels();
        bitDepth = frameAudio->getBitDepth();
        bitRate = frameAudio->getBitRate();

        audioData = frameAudio->getAudio(track, &audioDataLen);
        /*QByteArray* arr = new QByteArray((char *)audioData, audioDataLen);
        QBuffer *input = new QBuffer(arr);
        input->setBuffer(arr);
        input->open(QIODevice::ReadOnly);
        QAudioFormat m_audioFormat = QAudioFormat();
        m_audioFormat.setSampleRate(bitRate);
        m_audioFormat.setChannelCount(channels);
        m_audioFormat.setSampleSize(bitDepth);
        m_audioFormat.setCodec("audio/pcm");
        m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
        m_audioFormat.setSampleType(QAudioFormat::SignedInt);

        QAudioOutput *audio = new QAudioOutput(m_audioFormat); // , this);

        // connect up signal stateChanged to a lambda to get feedback
        QObject::connect(audio, &QAudioOutput::stateChanged, [audio, input, arr](QAudio::State newState)
        {
            if (newState == QAudio::IdleState) {   // finished playing (i.e., no more data)
                // qWarning() << "finished playing sound";
                audio->stop();
                delete audio;
                delete input;
                delete arr;
            }
        });

        // start the audio (i.e., play sound from the QAudioOutput object that we just created)
        audio->start(input);*/
        /*auto ait = audioPlayers.begin();
        for ( ; ait != audioPlayers.end(); ait++) {
            if ((*ait)->output->state() != QAudio::IdleState) {
                continue;
            }
            QAudioFormat& m_audioFormat = (*ait)->output->format();
            if (m_audioFormat.sampleRate() != bitRate || m_audioFormat.sampleSize() != bitDepth || m_audioFormat.channelCount() != channels) {
                continue;
            }
            break;
        }
        if (ait == audioPlayers.end()) {
            ait = audioPlayers.insert(ait, new SmkAudioPlayer());

            QAudioFormat m_audioFormat = QAudioFormat();
            m_audioFormat.setSampleRate(bitRate);
            m_audioFormat.setChannelCount(channels);
            m_audioFormat.setSampleSize(bitDepth);
            m_audioFormat.setCodec("audio/pcm");
            m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
            m_audioFormat.setSampleType(QAudioFormat::SignedInt);

            (*ait)->output = new QAudioOutput(m_audioFormat); // , this);
        } else {
            (*ait)->audioBuffer.close();
        }

        (*ait)->audioData.setRawData((char *)audioData, audioDataLen);
        (*ait)->audioBuffer.setBuffer(&(*ait)->audioData);
        if (!(*ait)->audioBuffer.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(nullptr, "Error", "Failed to open buffer");
        }

        (*ait)->output->start(&(*ait)->audioBuffer);
        auto state = (*ait)->output->state();
        if (state != QAudio::ActiveState) {
            QMessageBox::critical(nullptr, "Error", QApplication::tr("playAudio failed-state %1").arg(state));
        }*/
        audioQueue.push_back(QPair<uint8_t *, unsigned long>(audioData, audioDataLen));

        if (audioOutput != nullptr) {
            QAudioFormat& m_audioFormat = audioOutput->format();
            if (m_audioFormat.sampleRate() != bitRate || m_audioFormat.sampleSize() != bitDepth || m_audioFormat.channelCount() != channels) {
                audioOutput->close();
                delete audioOutput;
                audioOutput = nullptr;
            }
        }
        if (audioOutput == nullptr) {
            QAudioFormat m_audioFormat = QAudioFormat();
            m_audioFormat.setSampleRate(bitRate);
            m_audioFormat.setChannelCount(channels);
            m_audioFormat.setSampleSize(bitDepth);
            m_audioFormat.setCodec("audio/pcm");
            m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
            m_audioFormat.setSampleType(QAudioFormat::SignedInt);

            audioOutput = new QAudioOutput(m_audioFormat); // , this);
            // connect up signal stateChanged to a lambda to get feedback
            QObject::connect(audioOutput, &QAudioOutput::stateChanged, &audioCallback);
            /*QObject::connect(audioOutput, &QAudioOutput::stateChanged, [audio, input, arr](QAudio::State newState)
            {
                if (newState == QAudio::IdleState) {   // finished playing (i.e., no more data)
                    // qWarning() << "finished playing sound";
                    audio->stop();
                    delete audio;
                    delete input;
                    delete arr;
                }
            });*/

            if (audioBuffer == nullptr) {
                audioBytes = new QByteArray((char *)audioData, audioDataLen);
                audioBuffer = new QBuffer(audioBytes);
                if (!audioBuffer->open(QIODevice::ReadOnly)) {
                    QMessageBox::critical(nullptr, "Error", "Failed to open buffer");
                }
            }
        }
        QAudio::State state = audioOutput->state();
        if (state != QAudio::ActiveState) {
            audioCallback(QAudio::IdleState);
            if (state != QAudio::IdleState && state != QAudio::StoppedState) {
                QMessageBox::critical(nullptr, "Error", QApplication::tr("First state %1").arg(state));
            }
        }
    }
}
