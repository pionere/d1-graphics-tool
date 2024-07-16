#pragma once

#include <vector>

#include <QList>

class D1Gfx;
class D1GfxFrame;
class D1Min;
class D1Pal;
class UpscaleParam;
struct UpscalingParam;

class Upscaler {
public:
    static bool upscaleGfx(D1Gfx *gfx, const UpscaleParam &params, bool silent);
    static bool upscaleTileset(D1Gfx *gfx, D1Min *min, const UpscaleParam &params, bool silent);

private:
    static void upscaleFrame(D1GfxFrame *frame, const UpscalingParam &params);
    static void downscaleFrame(D1GfxFrame *frame, const UpscalingParam &params);
    static D1GfxFrame *createSubtileFrame(const D1Gfx *gfx, const D1Min *min, int subtileIndex);
    static int storeSubtileFrame(const D1GfxFrame *subtileFrame, std::vector<std::vector<unsigned>> &newFrameReferences, QList<D1GfxFrame *> &newframes);
};
