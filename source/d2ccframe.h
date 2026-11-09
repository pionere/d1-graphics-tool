#pragma once

#include <QByteArray>

#include "d1gfx.h"
#include "openasdialog.h"

class D2CcFrame {
public:
    static int load(D1GfxFrame &frame, int idx, const QByteArray &rawData, const OpenAsParam &params);
};
