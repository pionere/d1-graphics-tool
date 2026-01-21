#include "d1image.h"

#include <climits>
#include <vector>

#include <QColor>
#include <QImage>
#include <QList>

static quint8 getPalColor(const std::vector<PaletteColor> &colors, QColor color)
{
    unsigned res = 0;
    int best = INT_MAX;

    for (const PaletteColor &palColor : colors) {
        int currR = color.red() - palColor.red();
        int currG = color.green() - palColor.green();
        int currB = color.blue() - palColor.blue();
        int curr = currR * currR + currG * currG + currB * currB;
        if (curr < best) {
            best = curr;
            res = palColor.index();
        }
    }

    return res;
}

static int colorWeight(const QColor color)
{
    int r = color.red(), g = color.green(), b = color.blue();
    if (g >= r) {
        if (g >= b) {
            if (r >= b)
                return (g * 256 * 256 + r * 256 + b) * 8 + 5;
            else
                return (g * 256 * 256 + b * 256 + r) * 8 + 4;
        } else {
            return (b * 256 * 256 + g * 256 + r) * 8 + 1;
        }
    } else {
        if (g >= b) {
            return (r * 256 * 256 + g * 256 + b) * 8 + 2;
        } else {
            if (r >= b) {
                return (r * 256 * 256 + b * 256 + g) * 8 + 3;
            } else {
                return (b * 256 * 256 + r * 256 + g) * 8 + 0;
            }
        }
    }
}

static QColor weightColor(unsigned weight)
{
    unsigned c = weight / 8;
    unsigned r, g, b;
    switch (weight % 8) {
    case 0: r = c / 256; g = c; b = c / (256 * 256); break;
    case 1: r = c; g = c / 256; b = c / (256 * 256); break;
    case 2: r = c / (256 * 256); g = c / 256; b = c; break;
    case 3: r = c / (256 * 256); g = c; b = c / 256; break;
    case 4: r = c; g = c / (256 * 256); b = c / 256; break;
    case 5: r = c / 256; g = c / (256 * 256); b = c; break;
    }
    return QColor(r % 256, g % 256, b % 256);
}

bool D1ImageFrame::load(D1GfxFrame &frame, const QImage &image, const D1Pal *pal)
{
    frame.width = image.width();
    frame.height = image.height();

    frame.pixels.clear();

    std::vector<PaletteColor> colors;
    pal->getValidColors(colors);

    if (colors.empty()) {
        const QRgb *srcBits = reinterpret_cast<const QRgb *>(image.bits());
        std::vector<std::vector<int>> weights;
        std::map<int, int> wmap;
        for (int y = 0; y < frame.height; y++) {
            std::vector<int> wLine;
            std::vector<QColor> colorLine;
            for (int x = 0; x < frame.width; x++, srcBits++) {
                // QColor color = image.pixelColor(x, y);
                QColor color = QColor::fromRgba(*srcBits);
                int w;
                // if (color == QColor(Qt::transparent)) {
                if (color.alpha() < COLOR_ALPHA_LIMIT) {
                    w = -1;
                } else {
                    w = colorWeight(color);
                    wmap[w] += 1;
                }
                wLine.push_back(w);
            }
            weights.push_back(std::move(wLine));
        }

        D1Pal *pal = new D1Pal();
        // RegisterPalette
        unsigned n = frame.width * frame.height;
        n = (n + D1PAL_COLORS - 1) / D1PAL_COLORS;

        std::map<int, int> cmap; // weight to palette-index
        int i = 0;
        while (true) {
            std::map<int, int>::iterator mi = wmap.begin();
            if (mi == wmap.end() || mi->second < n) {
                break;
            }
            QColor c = weightColor(mi->first);
            pal->setColor(i, c);
            cmap[mi->first] = i;
            wmap.erase(mi->first);
            i++;
        }
        int si = i;
        while (true) {
            std::map<int, int>::reverse_iterator mi = wmap.rbegin();
            if (mi == wmap.rend() || mi->second < n) {
                break;
            }
            QColor c = weightColor(mi->first);
            pal->setColor(i, c);
            cmap[mi->first] = i;
            wmap.erase(mi->first);
            i++;
        }
        int ei = i;
        std::map<int, int>::reverse_iterator mi = wmap.begin();
        std::vector<int> lastmap;
        for ( ; i < D1PAL_COLORS; i++) {
            std::vector<int> currmap;
            int cc = 0, sum = 0;
            while (mi != wmap.end() && cc < n) {
                cc += mi->second;
                sum += mi->first *  mi->second;
                currmap.push_back(mi->first);
                mi++;
            }
            if (cc == 0) {
                break;
            }
            sum /= cc;
            int best = INT_MAX;
            int res = 0;
            for (const int ce : currmap) {
                int diff = abs(ce - sum);
                if (diff < best) {
                    best = diff;
                    res = ce;
                }
            }

            QColor c = weightColor(res);
            pal->setColor(i, c);

            std::vector<PaletteColor> colors;
            colors.push_back(PaletteColor(c, i));
            if (!lastmap.empty()) {
                colors.push_back(PaletteColor(pal->getColor(i - 1), i - 1));

                for (const int ce : lastmap) {
                    QColor wc = weightColor(ce);
                    cmap[ce] = getPalColor(colors, wc);
                }
            } else if (si != 0) {
                colors.push_back(PaletteColor(pal->getColor(si - 1), si - 1));
            }
            if (i >= D1PAL_COLORS - 2 && si != ei) {
                colors.push_back(PaletteColor(pal->getColor(si), si));
            }
            lastmap.clear();
            for (const int ce : currmap) {
                QColor wc = weightColor(ce);
                int pc = getPalColor(colors, wc);
                cmap[ce] = pc;
                if (pc == i) {
                    lastmap.push_back(ce);
                }
            }
        }

        frame.setFramePal(pal);

        for (const std::vector<int> wline : weights) {
            std::vector<D1GfxPixel> pixelLine;
            for (int w : wline) {
                D1GfxPixel pixel = w == -1 ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(cmap[w]);
                pixelLine.push_back(pixel);
            }
            frame.pixels.push_back(std::move(pixelLine));
        }
    } else {
        const QRgb *srcBits = reinterpret_cast<const QRgb *>(image.bits());
        for (int y = 0; y < frame.height; y++) {
            std::vector<D1GfxPixel> pixelLine;
            for (int x = 0; x < frame.width; x++, srcBits++) {
                // QColor color = image.pixelColor(x, y);
                QColor color = QColor::fromRgba(*srcBits);
                // if (color == QColor(Qt::transparent)) {
                if (color.alpha() < COLOR_ALPHA_LIMIT) {
                    pixelLine.push_back(D1GfxPixel::transparentPixel());
                } else {
                    pixelLine.push_back(D1GfxPixel::colorPixel(getPalColor(colors, color)));
                }
            }
            frame.pixels.push_back(std::move(pixelLine));
        }
    }
    return true;
}

bool D1ImageFrame::load(D1GfxFrame &frame, const QString &pixels, const D1Pal *pal)
{
    QStringList rows = pixels.split('\n');
    int width = 0, height = 0;
    QList<QStringList> pixValues;
    for (const QString &row : rows) {
        QStringList colors = row.split(';');
        pixValues.push_back(colors);
        if (colors.count() > width) {
            width = colors.count();
        }
        height++;
    }

    frame.width = width;
    frame.height = height;

    frame.pixels.clear();

    std::vector<PaletteColor> colors;
    pal->getValidColors(colors);

    for (const QStringList &row : pixValues) {
        std::vector<D1GfxPixel> pixelLine;
        for (QString pixel : row) {
            pixel = pixel.trimmed();
            if (pixel.isEmpty()) {
                pixelLine.push_back(D1GfxPixel::transparentPixel());
            } else {
                bool valid;
                quint8 color = pixel.toInt(&valid);
                if (!valid) {
                    color = getPalColor(colors, QColor(pixel.right(7)));
                }
                pixelLine.push_back(D1GfxPixel::colorPixel(color));
            }
        }
        for (int i = pixelLine.size() - width; i > 0; i--) {
            pixelLine.push_back(D1GfxPixel::transparentPixel());
        }
        frame.pixels.push_back(std::move(pixelLine));
    }

    return true;
}

QSize D1PixelImage::getImageSize(const std::vector<std::vector<D1GfxPixel>> &pixels)
{
    int width = 0;
    int height = pixels.size();
    if (height != 0) {
        width = pixels[0].size();
    }
    return QSize(width, height);
}

void D1PixelImage::createImage(std::vector<std::vector<D1GfxPixel>> &pixels, int width, int height)
{
    for (int y = 0; y < height; y++) {
        std::vector<D1GfxPixel> pixelLine;
        for (int x = 0; x < width; x++) {
            pixelLine.push_back(D1GfxPixel::transparentPixel());
        }
        pixels.push_back(pixelLine);
    }
}

void D1PixelImage::drawImage(std::vector<std::vector<D1GfxPixel>> &outPixels, int dx, int dy, const std::vector<std::vector<D1GfxPixel>> &srcPixels)
{
    const QSize size = D1PixelImage::getImageSize(srcPixels);

    for (int y = 0; y < size.height(); y++) {
        for (int x = 0; x < size.width(); x++) {
            if (!srcPixels[y][x].isTransparent())
                outPixels[dy + y][dx + x] = srcPixels[y][x];
        }
    }
}
