#pragma once

#include <QByteArray>

#include "d1gfx.h"
#include "openasdialog.h"

// Class used only for CEL frame width calculation
class D1CelPixelGroup {
public:
    D1CelPixelGroup() = default;
    D1CelPixelGroup(bool transparent, unsigned pixelCount);

    bool isTransparent() const;
    unsigned getPixelCount() const;

private:
    bool transparent = false;
    unsigned pixelCount = 0;
};

class D1CelFrame {
public:
    static bool load(D1GfxFrame &frame, const QByteArray &rawData, const OpenAsParam &params);

private:
    static unsigned computeWidthFromHeader(const QByteArray &rawData);
    static unsigned computeWidthFromData(const QByteArray &rawData, bool clipped);
};
