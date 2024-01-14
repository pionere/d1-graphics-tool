#include "d1smk.h"

#include <QApplication>
#include <QAudioFormat>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioSink>
typedef QAudioSink SmkAudioOutput;
#else
#include <QAudioOutput>
typedef QAudioOutput SmkAudioOutput;
#endif
#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QList>
#include <QObject>

#include "config.h"
#include "progressdialog.h"

#include "dungeon/all.h"
#include "libsmacker/smacker.h"

class AudioBuffer;

static bool audioSemaphore[D1SMK_TRACKS] = { false };
static SmkAudioOutput *audioOutput[D1SMK_TRACKS] = { nullptr };
static AudioBuffer *smkAudioBuffer[D1SMK_TRACKS] = { nullptr };

class AudioBuffer : public QIODevice {

public:
    AudioBuffer();
    ~AudioBuffer() = default;

    qint64 peek(char *data, qint64 maxSize);
    qint64 bytesAvailable() const override;
    bool   isSequential() const override;
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

    void clear();

private:
    QList<QPair<uint8_t *, unsigned long>> audioQueue;
    qint64 currPos = 0;
    qint64 availableBytes = 0;
};

AudioBuffer::AudioBuffer()
    : QIODevice()
{
}

qint64 AudioBuffer::bytesAvailable() const
{
    return availableBytes;
}

bool AudioBuffer::isSequential() const
{
    return true;
}

qint64 AudioBuffer::peek(char *data, qint64 maxSize)
{
    qint64 result = availableBytes;
    if (maxSize < result) {
        result = maxSize;
    }
    qint64 rem = result;
    for (int i = 0; i < audioQueue.count(); i++) {
        QPair<uint8_t *, unsigned long> &entry = audioQueue[i];
        uint8_t *entryData = entry.first;
        qint64 len = entry.second;
        if (i == 0) {
            entryData += currPos;
            len -= currPos;
        }
        if (len > rem) {
            len = rem;
            rem = 0;
        } else {
            rem -= len;
        }
        memcpy(data, entryData, len);
        data += len;
        if (rem == 0)
            break;
    }

    return result;
}

qint64 AudioBuffer::readData(char *data, qint64 maxSize)
{
    qint64 result = peek(data, maxSize);

    if (result != 0) {
        availableBytes -= result;

        qint64 rem = result;
        qint64 cp = currPos;
        int i = 0;
        for (; ; i++) {
            QPair<uint8_t *, unsigned long> &data = audioQueue[i];
            qint64 len = data.second;
            len -= cp;
            if (len > rem) {
                cp += rem;
                break;
            } else {
                rem -= len;
                cp = 0;
                if (rem == 0) {
                    i++;
                    break;
                }
            }
        }
        currPos = cp;
        if (i != 0) {
            audioQueue.erase(audioQueue.begin(), audioQueue.begin() + i);
        }
    }
    return result;
}

qint64 AudioBuffer::writeData(const char *data, qint64 maxSize)
{
    audioQueue.push_back(QPair<uint8_t *, unsigned long>((uint8_t *)data, (unsigned long)maxSize));
    availableBytes += maxSize;
    return maxSize;
}

void AudioBuffer::clear()
{
    audioQueue.clear();
    availableBytes = 0;
}

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
    return true;
}

bool D1Smk::save(D1Gfx &gfx, const SaveAsParam &params)
{
    dProgressErr() << QApplication::tr("Not supported.");
    return false;
}

static void audioCallback(int track, QAudio::State newState)
{
    if (newState == QAudio::IdleState) {   // finished playing (i.e., no more data)
        if (!smkAudioBuffer[track]->atEnd() && !audioSemaphore[track]) {
            audioSemaphore[track] = true;
            audioOutput[track]->start(smkAudioBuffer[track]);
            audioSemaphore[track] = false;
        }
    }
}

static void audioCallback0(QAudio::State newState)
{
    audioCallback(0, newState);
}
static void audioCallback1(QAudio::State newState)
{
    audioCallback(1, newState);
}
static void audioCallback2(QAudio::State newState)
{
    audioCallback(2, newState);
}
static void audioCallback3(QAudio::State newState)
{
    audioCallback(3, newState);
}
static void audioCallback4(QAudio::State newState)
{
    audioCallback(4, newState);
}
static void audioCallback5(QAudio::State newState)
{
    audioCallback(5, newState);
}
static void audioCallback6(QAudio::State newState)
{
    audioCallback(6, newState);
}
static void (*cbfunc[D1SMK_TRACKS])(QAudio::State) = { &audioCallback0, &audioCallback1, &audioCallback2, &audioCallback3, &audioCallback4, &audioCallback5, &audioCallback6 };

void D1Smk::playAudio(D1GfxFrame &gfxFrame, int trackIdx)
{
    D1SmkAudioData *frameAudio;
    unsigned long audioDataLen;
    uint8_t *audioData;
    unsigned channels, bitRate, bitDepth;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QAudioFormat::SampleFormat sampleFormat;
#endif
    frameAudio = gfxFrame.getFrameAudio();
    if (frameAudio != nullptr) {
        channels = frameAudio->getChannels();
        bitDepth = frameAudio->getBitDepth();
        bitRate = frameAudio->getBitRate();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        sampleFormat = bitDepth == 16 ? QAudioFormat::Int16 : QAudioFormat::UInt8;
#endif
        for (int track = 0; track < D1SMK_TRACKS; track++) {
            if (trackIdx >= 0 && track != trackIdx) {
                continue;
            }

            audioData = frameAudio->getAudio(track, &audioDataLen);
            if (audioDataLen == 0) {
                continue;
            }

            if (audioOutput[track] != nullptr) {
                QAudioFormat m_audioFormat = audioOutput[track]->format();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                if (m_audioFormat.sampleRate() != bitRate || m_audioFormat.sampleFormat() != sampleFormat || m_audioFormat.channelCount() != channels) {
#else
                if (m_audioFormat.sampleRate() != bitRate || m_audioFormat.sampleSize() != bitDepth || m_audioFormat.channelCount() != channels) {
#endif
                    audioOutput[track]->stop();
                    delete audioOutput[track];
                    audioOutput[track] = nullptr;
                    smkAudioBuffer[track]->clear();
                }
            }
            if (audioOutput[track] == nullptr) {
                QAudioFormat m_audioFormat = QAudioFormat();
                m_audioFormat.setSampleRate(bitRate);
                m_audioFormat.setChannelCount(channels);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                m_audioFormat.setSampleFormat(sampleFormat);
#else
                m_audioFormat.setSampleSize(bitDepth);
                m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
                m_audioFormat.setSampleType(QAudioFormat::SignedInt);
                m_audioFormat.setCodec("audio/pcm");
#endif

                audioOutput[track] = new SmkAudioOutput(m_audioFormat); // , this);
                // connect up signal stateChanged to a lambda to get feedback
                QObject::connect(audioOutput[track], &SmkAudioOutput::stateChanged, cbfunc[track]);

                if (smkAudioBuffer[track] == nullptr) {
                    smkAudioBuffer[track] = new AudioBuffer();
                    smkAudioBuffer[track]->open(QIODevice::ReadWrite | QIODevice::Unbuffered);
                }
            }
            smkAudioBuffer[track]->write((char *)audioData, audioDataLen);

            QAudio::State state = audioOutput[track]->state();
            if (state != QAudio::ActiveState) {
                audioCallback(track, QAudio::IdleState);
            }
        }
    }
}

void D1Smk::stopAudio()
{
    for (int track = 0; track < D1SMK_TRACKS; track++) {
        if (audioOutput[track] != nullptr) {
            audioOutput[track]->stop();
        }
        if (smkAudioBuffer[track] != nullptr) {
            smkAudioBuffer[track]->clear();
        }
    }
}
