#include "d1image.h"

#include <climits>

#include <QColor>
#include <QImage>
#include <QList>
#include <QVector>

static quint8 getPalColor(QVector<QColor> &colors, QColor color)
{
    int res = 0;
    int best = INT_MAX;

    for (int i = 0; i < colors.count(); i++) {
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

    QVector<QColor> colors;
    QColor undefColor = pal->getUndefinedColor();
    for (int i = 0; i < D1PAL_COLORS; i++) {
        QColor palColor = pal->getColor(i);
        if (palColor != undefColor) {
            colors.append(palColor);
        }
    }
    if (colors.isEmpty()) {
        colors.append(undefColor);
    }
    for (int y = 0; y < frame.height; y++) {
        frame.pixels.push_back(QList<D1GfxPixel>());
        QList<D1GfxPixel> &pixelLine = frame.pixels.back();
        for (int x = 0; x < frame.width; x++) {
            QColor color = image.pixelColor(x, y);
            // if (color == QColor(Qt::transparent)) {
            if (color.alpha() < COLOR_ALPHA_LIMIT) {
                pixelLine.append(D1GfxPixel::transparentPixel());
            } else {
                pixelLine.append(D1GfxPixel::colorPixel(getPalColor(colors, color)));
            }
        }
    }

    return true;
}
