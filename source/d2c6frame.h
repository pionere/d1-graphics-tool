#pragma once

#include <QByteArray>

#include "d1gfx.h"
#include "openasdialog.h"

class D2C6Frame {
public:
    static bool load(D1GfxFrame &frame, const QByteArray &rawData, const OpenAsParam &params);
};
