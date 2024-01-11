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
class AudioBuffer;

static bool audioSemaphore = false;
static QAudioOutput *audioOutput = nullptr;
// static QByteArray *audioBytes = nullptr;
// static QBuffer *audioBuffer = nullptr;
static AudioBuffer *smkAudioBuffer = nullptr;

class AudioBuffer : public QIODevice {

public:
    AudioBuffer();
    ~AudioBuffer() = default;

    bool	isOpen() const;
    bool	isReadable() const;
	QIODevice::OpenMode	openMode() const;
    qint64	peek(char *data, qint64 maxSize);
    // qint64	read(char *data, qint64 maxSize);
    bool	atEnd() const override;
    qint64	bytesAvailable() const override;
    qint64	bytesToWrite() const override;
    bool	canReadLine() const override;
    void	close() override;
    bool	isSequential() const override;
    bool	open(QIODevice::OpenMode mode) override;
    qint64	pos() const override;
    bool	reset() override;
    bool	seek(qint64 pos) override;
    qint64	size() const override;
    bool	waitForBytesWritten(int msecs) override;
    bool	waitForReadyRead(int msecs) override;
	qint64	readData(char *data, qint64 maxSize) override;
	qint64	writeData(char *data, qint64 maxSize) override;

    void	enqueue(uint8_t *audioData, unsigned long audioLen);

private:
    QList<QPair<uint8_t *, unsigned long>> audioQueue;
    qint64 currPos = 0;
    qint64 availableBytes = 0;
};

AudioBuffer::AudioBuffer()
    : QIODevice()
{
}

bool AudioBuffer::isOpen() const
{
    return true;
}

bool AudioBuffer::isReadable() const
{
    return availableBytes != 0;
}

QIODevice::OpenMode	AudioBuffer::openMode() const
{
    return QIODevice::ReadOnly;
}

bool AudioBuffer::atEnd() const
{
    return availableBytes == 0;
}

qint64 AudioBuffer::bytesAvailable() const
{
    return availableBytes;
}

qint64 AudioBuffer::bytesToWrite() const
{
    return 0;
}

bool AudioBuffer::canReadLine() const
{
    return false;
}

void AudioBuffer::close()
{
    audioQueue.clear();
    availableBytes = 0;
}

bool AudioBuffer::isSequential() const
{
    return true;
}

bool AudioBuffer::open(QIODevice::OpenMode mode)
{
    return true;
}

qint64 AudioBuffer::pos() const
{
    return 0;
}

bool AudioBuffer::reset()
{
    return false;
}

bool AudioBuffer::seek(qint64 pos)
{
    return false;
}

qint64 AudioBuffer::size() const
{
    return availableBytes + currPos;
}

bool AudioBuffer::waitForBytesWritten(int msecs)
{
    return false;
}

bool AudioBuffer::waitForReadyRead(int msecs)
{
    return currPos;
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

// qint64 AudioBuffer::read(char *data, qint64 maxSize)
qint64 AudioBuffer::readData(char *data, qint64 maxSize)
{
    qint64 result = peek(data, maxSize);

    if (result != 0) {
        availableBytes -= result;

        qint64 rem = result;
        int i = 0;
        for (; ; i++) {
            QPair<uint8_t *, unsigned long> &data = audioQueue[i];
            qint64 len = data.second;
            if (i == 0) {
                len -= currPos;
            }
            if (len > rem) {
                if (i == 0) {
                    currPos += rem;
                } else {
                    currPos = rem;
                }
                break;
            } else {
                rem -= len;
                if (rem == 0) {
                    currPos = 0;
                    i++;
                    break;
                }
            }
        }

        if (i != 0) {
            audioQueue.erase(audioQueue.begin(), audioQueue.begin() + i - 1);
        }
    }
    return result;
}

/*qint64	AudioBuffer::readData(char *data, qint64 maxSize)
{
    return read(data, maxSize);
}*/

qint64	AudioBuffer::writeData(char *data, qint64 maxSize)
{
    return 0;
}

void AudioBuffer::enqueue(uint8_t *audioData, unsigned long audioLen)
{
    audioQueue.push_back(QPair<uint8_t *, unsigned long>(audioData, audioLen));
    availableBytes += audioLen;
}

/*void	commitTransaction()
int	currentReadChannel() const
int	currentWriteChannel() const
QString	errorString() const
bool	getChar(char *c)
bool	isTextModeEnabled() const
bool	isTransactionStarted() const
bool	isWritable() const
QIODevice::OpenMode	openMode() const
QByteArray	peek(qint64 maxSize)
bool	putChar(char c)
QByteArray	read(qint64 maxSize)
QByteArray	readAll()
int	readChannelCount() const
qint64	readLine(char *data, qint64 maxSize)
QByteArray	readLine(qint64 maxSize = 0)
void	rollbackTransaction()
void	setCurrentReadChannel(int channel)
void	setCurrentWriteChannel(int channel)
void	setTextModeEnabled(bool enabled)
qint64	skip(qint64 maxSize)
void	startTransaction()
void	ungetChar(char c)
qint64	write(const char *data, qint64 maxSize)
qint64	write(const char *data)
qint64	write(const QByteArray &byteArray)
int	writeChannelCount() const*/


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
        // if (!audioQueue.isEmpty() && !audioSemaphore) {
        if (!smkAudioBuffer->atEnd() && !audioSemaphore) {
            audioSemaphore = true;
            /*QPair<uint8_t *, unsigned long> audioData = audioQueue[0];
            audioQueue.pop_front();
            audioBytes->setRawData((char *)audioData.first, audioData.second);
            audioBuffer->seek(0);
            audioOutput->start(audioBuffer);
            auto state = audioOutput->state();
            if (state != QAudio::ActiveState) {
                QMessageBox::critical(nullptr, "Error", QApplication::tr("playAudio failed-state %1").arg(state));
            }*/
            audioOutput->start(smkAudioBuffer);
            auto state = audioOutput->state();
            if (state != QAudio::ActiveState) {
                QMessageBox::critical(nullptr, "Error", QApplication::tr("playAudio failed-state %1").arg(state));
            }
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
        /*audioQueue.push_back(QPair<uint8_t *, unsigned long>(audioData, audioDataLen));

        if (audioOutput != nullptr) {
            QAudioFormat& m_audioFormat = audioOutput->format();
            if (m_audioFormat.sampleRate() != bitRate || m_audioFormat.sampleSize() != bitDepth || m_audioFormat.channelCount() != channels) {
                audioOutput->stop();
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
            });* /

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
        }*/

        if (audioOutput != nullptr) {
            QAudioFormat& m_audioFormat = audioOutput->format();
            if (m_audioFormat.sampleRate() != bitRate || m_audioFormat.sampleSize() != bitDepth || m_audioFormat.channelCount() != channels) {
                audioOutput->stop();
                delete audioOutput;
                audioOutput = nullptr;
                smkAudioBuffer->close();
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

            if (smkAudioBuffer == nullptr) {
                smkAudioBuffer = new AudioBuffer();
            }
        }

        smkAudioBuffer->enqueue(audioData, audioDataLen);

        QAudio::State state = audioOutput->state();
        if (state != QAudio::ActiveState) {
            audioCallback(QAudio::IdleState);
            if (state != QAudio::IdleState && state != QAudio::StoppedState) {
                QMessageBox::critical(nullptr, "Error", QApplication::tr("First state %1").arg(state));
            }
        }
    }
}

void D1Smk::stopAudio()
{
    if (audioOutput != nullptr) {
        audioOutput->stop();
    }
    if (smkAudioBuffer != nullptr) {
        smkAudioBuffer->close();
    }
}
