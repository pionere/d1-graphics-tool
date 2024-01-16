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
