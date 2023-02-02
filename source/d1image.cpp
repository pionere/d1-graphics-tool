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
    std::vector<std::pair<QColor, quint8>> colorMap;
    constexpr int CACHE_SIZE = 512;
    for (int y = 0; y < frame.height; y++) {
        std::vector<D1GfxPixel> pixelLine;
        for (int x = 0; x < frame.width; x++) {
            QColor color = image.pixelColor(x, y);
            // if (color == QColor(Qt::transparent)) {
            if (color.alpha() < COLOR_ALPHA_LIMIT) {
                pixelLine.push_back(D1GfxPixel::transparentPixel());
            } else {
                auto iter = std::lower_bound(colorMap.begin(), colorMap.end(), color,
                    [](const std::pair<QColor, quint8> &colPix, const QColor &col) {
                        if (colPix.first.red() < col.red())
                            return true;
                        if (colPix.first.red() != col.red())
                            return false;
                        if (colPix.first.green() < col.green())
                            return true;
                        if (colPix.first.green() != col.green())
                            return false;
                        return colPix.first.blue() < col.blue();
                    });
                quint8 index;
                if (iter == colorMap.end() || iter->first != color) {
                    index = getPalColor(colors, color);
                    if (colorMap.size() < CACHE_SIZE) {
                        colorMap.insert(iter, std::pair<QColor, quint8>(color, index));
                    }
                } else {
                    index = iter->second;
                }
                pixelLine.push_back(D1GfxPixel::colorPixel(index));
            }
        }
        frame.pixels.push_back(std::move(pixelLine));
    }

    return true;
}
