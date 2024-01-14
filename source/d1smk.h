#pragma once

#include <QMap>
#include <QString>

#include "d1gfx.h"
#include "d1pal.h"
#include "openasdialog.h"
#include "saveasdialog.h"

#define D1SMK_TRACKS 7
#define D1SMK_CHANNELS 2

class D1SmkAudioData : public QObject {
    Q_OBJECT

    friend class D1Smk;

public:
    D1SmkAudioData(unsigned channels, unsigned bitDepth, unsigned long bitRate);
    ~D1SmkAudioData();

    unsigned getChannels() const;
    unsigned getBitDepth() const;
    bool setBitRate(unsigned bitRate);
    unsigned getBitRate() const;
    void setAudio(unsigned track, uint8_t* audio, unsigned long len);
    uint8_t* getAudio(unsigned track, unsigned long *len);

private:
    unsigned channels;
    unsigned bitDepth;
    unsigned long bitRate;
    uint8_t* audio[D1SMK_TRACKS] = { nullptr };
    unsigned long len[D1SMK_TRACKS] = { 0 };
};

class D1Smk {
public:
    static bool load(D1Gfx &gfx, QMap<QString, D1Pal *> &pals, const QString &smkFilePath, const OpenAsParam &params);
    static bool save(D1Gfx &gfx, const SaveAsParam &params);

    static void playAudio(D1GfxFrame &gfxFrame, int track = -1);
    static void stopAudio();
};
