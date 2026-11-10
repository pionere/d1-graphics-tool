#pragma once

#include <QByteArray>

#include "d1gfx.h"
#include "openasdialog.h"

class D2CcFrame {
public:
    static bool load(D1GfxFrame &frame, const QByteArray &rawData, const OpenAsParam &params);
};
