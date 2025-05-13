#pragma once

#include <QString>

#include "d1gfx.h"
#include "exportdialog.h"

class D1SmkAudioData;

typedef struct _AudioData {
    unsigned channels;     // 1/2 channel
    unsigned bitDepth;     // 8/16 bit
    unsigned long bitRate; // Hz
    uint8_t* audio;
    unsigned long len;     // length of the (audio) data
} WAVAudioData;

class D1Wav {
public:
    static bool load(D1Gfx &gfx, int track, const QString &wavFilePath);
    static bool load(D1GfxFrame &gfxFrame, int track, const QString &wavFilePath);
    static bool save(const D1SmkAudioData *audioData, int track, const QString &wavFilePath);

private:
    static bool checkAudio(const D1SmkAudioData *audio, const WAVAudioData &wavAudioData, int track);
    static bool load(WAVAudioData &audioData, const QString &wavFilePath);
};
