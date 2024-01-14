#pragma once

#include <vector>

#include <QImage>
#include <QString>

#include "d1gfx.h"
#include "d1pal.h"

// alpha value under which the color is considered as transparent
#define COLOR_ALPHA_LIMIT 128

class D1ImageFrame {
public:
    static bool load(D1GfxFrame &frame, const QImage &image, bool clipped, const D1Pal *pal);
    static bool load(D1GfxFrame &frame, const QString &pixels, bool clipped, const D1Pal *pal);
};

class D1PixelImage {
public:
    static QSize getImageSize(const std::vector<std::vector<D1GfxPixel>> &pixels);
    static void createImage(std::vector<std::vector<D1GfxPixel>> &pixels, int width, int height);
    static void drawImage(std::vector<std::vector<D1GfxPixel>> &outPixels, int dx, int dy, const std::vector<std::vector<D1GfxPixel>> &srcPixels);
};
