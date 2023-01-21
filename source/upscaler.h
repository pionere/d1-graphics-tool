#pragma once

#include <QList>

class D1Gfx;
class D1GfxFrame;
class D1Min;
class D1Pal;
class UpscaleParam;

class Upscaler {
public:
    static bool upscaleGfx(D1Gfx *gfx, const UpscaleParam &params);
    static bool upscaleTileset(D1Gfx *gfx, D1Min *min, const UpscaleParam &params);

private:
    static void upscaleFrame(D1GfxFrame *frame, D1Pal *palette, const UpscaleParam &params);
    static D1GfxFrame *createSubtileFrame(const D1Gfx *gfx, const D1Min *min, int subtileIndex);
    static void storeSubtileFrame(const D1GfxFrame *subtileFrame, QList<QList<quint16>> &newFrameReferences, QList<D1GfxFrame> &newframes);
};
