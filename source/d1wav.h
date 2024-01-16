#pragma once

#include <QString>

#include "d1gfx.h"
#include "exportdialog.h"

class D1SmkAudioData;

class D1Wav {
public:
    static bool save(const D1SmkAudioData *audioData, int track, const QString &wavFilePath);
};
