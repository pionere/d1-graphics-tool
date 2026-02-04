#include "d1wav.h"

#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QFile>

#include "config.h"
#include "d1smk.h"
#include "progressdialog.h"

typedef struct _WavHeader {
    quint32 RiffMarker;
    quint32 FileSize;
    quint32 WaveMarker;
    quint32 FmtMarker;
    quint32 HeaderLength;
    quint16 FormatType;
    quint16 ChannelCount;
    quint32 SampleRate;
    quint32 SampleRateBytesChannels;
    quint16 BytesChannels;
    quint16 BitsPerSample;
    quint32 DataMarker;
    quint32 DataSize;
} WAVHEADER;

bool D1Wav::load(WAVAudioData &audioData, const QString &filePath)
{
    // Opening WAV file
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        dProgressErr() << QApplication::tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    quint64 fileSize = file.size();
    WAVHEADER wavhdr;

    if (fileSize < sizeof(wavhdr)) {
        dProgressErr() << QApplication::tr("Invalid WAV file.");
        return false;
    }
    fileSize -= sizeof(wavhdr);

    // process the header
    file.read((char *)&wavhdr, sizeof(wavhdr));
    if (wavhdr.FmtMarker != SwapLE32(*((uint32_t*)"fmt "))) {
        dProgressErr() << QApplication::tr("Invalid WAV header.");
        return false;
    }

    unsigned channels = SwapLE16(wavhdr.ChannelCount);
    if (channels > D1SMK_CHANNELS) {
        dProgressErr() << QApplication::tr("Unsupported number of channels (%1. Max. 2).").arg(channels);
        return false;
    }

    unsigned bitDepth = SwapLE16(wavhdr.BitsPerSample);
    if (bitDepth != 8 && bitDepth != 16) {
        dProgressErr() << QApplication::tr("Unsupported number of channels (%1. Max. 2).").arg(channels);
        return false;
    }

    unsigned long bitRate = SwapLE32(wavhdr.SampleRate);

    uint32_t dataMarker = wavhdr.DataMarker;
    uint32_t dataSize = SwapLE32(wavhdr.DataSize);
    while (true) {
        if (dataSize < 8) {
            dProgressErr() << QApplication::tr("Invalid chunk length in the WAV file (%1. Min 8).").arg(dataSize);
            return false;
        }
        dataSize -= 8;
        if (dataSize > fileSize) {
            dProgressErr() << QApplication::tr("Invalid chunk in the WAV header.");
            return false;
        }
        fileSize -= dataSize;
        if (dataMarker == SwapLE32(*((uint32_t*)"data"))) {
            if (fileSize != 0) {
                dProgressWarn() << QApplication::tr("Unrecognized content in the WAV file (length: %1).").arg(fileSize);
            }
            break;
        }
        uint64_t chunkName = dataMarker;
        dProgressWarn() << QApplication::tr("Ignored chunk '%1' in the WAV header.").arg(chunkName);
        file.skip(dataSize);

        file.read((char *)&dataMarker, sizeof(dataMarker));
        file.read((char *)&dataSize, sizeof(dataSize));
        dataSize = SwapLE32(dataSize);
    }

    unsigned long len = dataSize;
    unsigned extraBits = len % (channels * bitDepth / 8);
    if (extraBits != 0) {
        dProgressWarn() << QApplication::tr("Mismatching content in the WAV file (length: %1).").arg(extraBits);
        len -= extraBits;
    }
    uint8_t *data = nullptr;
    if (len != 0) {
        data = (uint8_t *)malloc(len);
        if (data == nullptr) {
            dProgressErr() << QApplication::tr("Failed to allocate memory (%1).").arg(len);
            return false;
        }
        file.read((char *)data, len);
    }

    audioData.bitRate = bitRate;
    audioData.channels = channels;
    audioData.bitDepth = bitDepth;
    audioData.len = len;
    audioData.audio = data;
    return true;
}

bool D1Wav::checkAudio(const D1SmkAudioData *audio, const WAVAudioData &wavAudioData, int track)
{
    for (int i = 0; i < D1SMK_TRACKS; i++) {
        if (i != track && audio->len[i] != 0) {
            if (audio->channels != wavAudioData.channels) {
                dProgressErr() << QApplication::tr("Mismatching audio channels (%1 vs %2).").arg(audio->channels).arg(wavAudioData.channels);
            } else if (audio->bitDepth != wavAudioData.bitDepth) {
                dProgressErr() << QApplication::tr("Mismatching audio bit-depth (%1 vs %2).").arg(audio->bitDepth).arg(wavAudioData.bitDepth);
            } else if (audio->bitRate != wavAudioData.bitRate) {
                dProgressErr() << QApplication::tr("Mismatching audio bit-rate (%1 vs %2).").arg(audio->bitRate).arg(wavAudioData.bitRate);
            } else {
                continue;
            }
            return false;
        }
    }
    return true;
}

bool D1Wav::load(D1Gfx &gfx, int track, const QString &filePath)
{
    WAVAudioData wavAudioData;
    if (!D1Wav::load(wavAudioData, filePath)) {
        return false;
    }
    const int frameCount = gfx.getFrameCount();
    // TODO: D1SMK::load?
    // validate existing audio
    for (int i = 0; i < frameCount; i++) {
        D1GfxFrame *gfxFrame = gfx.getFrame(i);
        D1SmkAudioData *audio = gfxFrame->frameAudio;
        if (audio != nullptr && !checkAudio(audio, wavAudioData, track)) {
            free(wavAudioData.audio);
            return false;
        }
    }
    // clear track
    for (int i = 0; i < frameCount; i++) {
        D1GfxFrame *gfxFrame = gfx.getFrame(i);
        D1SmkAudioData *audio = gfxFrame->frameAudio;
        if (audio != nullptr) {
            audio->channels = wavAudioData.channels;
            audio->bitDepth = wavAudioData.bitDepth;
            audio->bitRate = wavAudioData.bitRate;
        } else {
            audio = new D1SmkAudioData(wavAudioData.channels, wavAudioData.bitDepth, wavAudioData.bitRate);
            gfxFrame->frameAudio = audio;
        }
        audio->setAudio(track, nullptr, 0);
    }
    // add the new track
    unsigned long audioLen = wavAudioData.len;
    if (audioLen != 0) {
        unsigned sampleSize = wavAudioData.channels * wavAudioData.bitDepth / 8;
        unsigned long bitRate = wavAudioData.bitRate;
        // assert((audioLen % sampleSize) == 0);
        unsigned sampleCount = audioLen / sampleSize;
        unsigned frameLen = gfx.frameLen; // us
        if (frameLen == 0) {
            // assert(frameCount != 0);
            frameLen = ((uint64_t)sampleCount * 1000000 + frameCount * bitRate - 1) / (frameCount * bitRate);
            gfx.frameLen = frameLen;
        }
        uint64_t tmp = (uint64_t)bitRate * frameLen;
        unsigned samplePerFrame = tmp / 1000000;
        unsigned remPerFrame = tmp % 1000000;
        if (remPerFrame != 0) {
            samplePerFrame++;
            remPerFrame = 1000000 - remPerFrame;
        }
        // use 4-byte aligned samplePerFrame
        if ((samplePerFrame * sampleSize) % 4) {
            unsigned align = 1;
            if (sampleSize & 1) {
                align = 4 - (samplePerFrame & 3);
            }
            samplePerFrame += align;
            remPerFrame += 1000000 * align;
        }
        // add extra content to the first frame  TODO: make this configurable?
        unsigned leadingSamples = bitRate * 1;
        unsigned long cursor = 0;
        uint64_t remContent = 0;
        for (int i = 0; i < frameCount; i++) {
            unsigned frameSamples = samplePerFrame;
            frameSamples += leadingSamples;
            // limit the samples and update sampleCount
            if (frameSamples >= sampleCount) {
                frameSamples = sampleCount;
                sampleCount = 0;
            } else {
                // skip frames periodically to avoid accumulation due to rounding
                remContent += remPerFrame;
                if (remContent > tmp) {
                    remContent -= tmp;
                    continue;
                }
                sampleCount -= frameSamples;
                // split the content of last frame if it is too small
                if (sampleCount < frameSamples / 2 && i != frameCount - 1/* && i != 0*/) {
                    sampleCount += frameSamples;
                    frameSamples = (sampleCount + 1) / 2;
                    sampleCount -= frameSamples;
                }
            }
            if (frameSamples == 0) {
                break;
            }
            // copy the audio chunk to the frame
            unsigned frameAudioLen = frameSamples * sampleSize;
            D1GfxFrame *gfxFrame = gfx.getFrame(i);
            D1SmkAudioData *audio = gfxFrame->frameAudio;
            audio->audio[track] = (uint8_t *)malloc(frameAudioLen);
            if (audio->audio[track] != nullptr) {
                audio->len[track] = frameAudioLen;
                memcpy(audio->audio[track], &wavAudioData.audio[cursor], frameAudioLen);
                cursor += frameAudioLen;
            } else {
                dProgressErr() << QApplication::tr("Out of memory.");
            }
            // reset for subsequent frames
            leadingSamples = 0;
        }
    }
    free(wavAudioData.audio);
    return true;
}

bool D1Wav::load(D1GfxFrame &gfxFrame, int track, const QString &filePath)
{
    WAVAudioData wavAudioData;
    if (!D1Wav::load(wavAudioData, filePath)) {
        return false;
    }
    // TODO: D1SMK::load?
    // validate existing audio
    D1SmkAudioData *audio = gfxFrame.frameAudio;
    if (audio != nullptr && !checkAudio(audio, wavAudioData, D1SMK_TRACKS)) {
        free(wavAudioData.audio);
        return false;
    }
    // clear track(-chunk)
    if (audio != nullptr) {
        audio->channels = wavAudioData.channels;
        audio->bitDepth = wavAudioData.bitDepth;
        audio->bitRate = wavAudioData.bitRate;
    } else {
        audio = new D1SmkAudioData(wavAudioData.channels, wavAudioData.bitDepth, wavAudioData.bitRate);
        gfxFrame.frameAudio = audio;
    }
    if (audio->audio[track] != nullptr) {
        free(audio->audio[track]);
    }
    // add the new track(-chunk)
    audio->audio[track] = wavAudioData.audio;
    audio->len[track] = wavAudioData.len;
    return true;
}

bool D1Wav::save(const D1SmkAudioData *audioData, int track, const QString &filePath)
{
    unsigned long len;
    uint8_t* data = audioData->getAudio(track, &len);

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    // write to file
    QDataStream out(&outFile);
    // out.setByteOrder(QDataStream::LittleEndian);

    // - header
    WAVHEADER header = { 0 };
    header.RiffMarker = SwapLE32(*((uint32_t*)"RIFF"));
    header.FileSize = SwapLE32(len + sizeof(header));
    header.WaveMarker = SwapLE32(*((uint32_t*)"WAVE"));
    header.FmtMarker = SwapLE32(*((uint32_t*)"fmt "));
    header.HeaderLength = SwapLE32(16);
    header.FormatType = SwapLE16(1);
    header.ChannelCount = SwapLE16(audioData->getChannels());
    header.SampleRate = SwapLE32(audioData->getBitRate());
    header.SampleRateBytesChannels = SwapLE32(audioData->getBitRate() * audioData->getChannels() * audioData->getBitDepth() / 8);
    header.BytesChannels = SwapLE16(audioData->getChannels() * audioData->getBitDepth() / 8);
    header.BitsPerSample = SwapLE16(audioData->getBitDepth());
    header.DataMarker = SwapLE32(*((uint32_t*)"data"));
    header.DataSize = SwapLE32(len  + 8);

    out.writeRawData((char *)&header, sizeof(header));

    // - data
    out.writeRawData((char *)data, len);

    return true;
}
