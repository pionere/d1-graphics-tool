#include "d1image.h"

#include <climits>
#include <vector>

#include <QColor>
#include <QImage>
#include <QList>

static quint8 getPalColor(std::vector<QColor> &colors, QColor color)
{
    unsigned res = 0;
    int best = INT_MAX;

    for (unsigned i = 0; i < colors.size(); i++) {
        QColor palColor = colors[i];
        int currR = color.red() - palColor.red();
        int currG = color.green() - palColor.green();
        int currB = color.blue() - palColor.blue();
        int curr = currR * currR + currG * currG + currB * currB;
        if (curr < best) {
            best = curr;
            res = i;
        }
    }

    return res;
}

bool D1ImageFrame::load(D1GfxFrame &frame, const QImage &image, bool clipped, D1Pal *pal)
{
    frame.clipped = clipped;
    frame.width = image.width();
    frame.height = image.height();

    frame.pixels.clear();

    std::vector<QColor> colors;
    QColor undefColor = pal->getUndefinedColor();
    for (int i = 0; i < D1PAL_COLORS; i++) {
        QColor palColor = pal->getColor(i);
        if (palColor != undefColor) {
            colors.push_back(palColor);
        }
    }
    if (colors.empty()) {
        colors.push_back(undefColor);
    }
    for (int y = 0; y < frame.height; y++) {
        std::vector<D1GfxPixel> pixelLine;
        for (int x = 0; x < frame.width; x++) {
            QColor color = image.pixelColor(x, y);
            // if (color == QColor(Qt::transparent)) {
            if (color.alpha() < COLOR_ALPHA_LIMIT) {
                pixelLine.push_back(D1GfxPixel::transparentPixel());
            } else {
                pixelLine.push_back(D1GfxPixel::colorPixel(getPalColor(colors, color)));
            }
        }
        frame.pixels.push_back(std::move(pixelLine));
    }

    return true;
}
