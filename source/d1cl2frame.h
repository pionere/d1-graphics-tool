#pragma once

#include <QByteArray>

#include "d1gfx.h"
#include "openasdialog.h"

class D1Cl2Frame {
public:
    static int load(D1GfxFrame &frame, const QByteArray rawFrameData, const OpenAsParam &params, unsigned *rle_len);

private:
    static unsigned computeWidthFromHeader(const QByteArray &rawFrameData);
};
