#pragma once

#include <QByteArray>

#include "d1gfx.h"
#include "openasdialog.h"

class D1Cl2Frame {
    friend class D1Cl2;

public:
    static bool load(D1GfxFrame &frame, QByteArray rawFrameData, const OpenAsParam &params);

private:
    static quint16 computeWidthFromHeader(QByteArray &rawFrameData);
};
