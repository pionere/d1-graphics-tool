#include "d1image.h"

#include <climits>
#include <vector>

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QList>

#include "progressdialog.h"

static std::pair<quint8, int> getPalColor(const std::vector<PaletteColor> &colors, QColor color)
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

    return std::pair<quint8, int>(res, best);
}
#if 0
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
            return (r * 256 * 256 + g * 256 + b) * 8 + 3;
        } else {
            if (r >= b) {
                return (r * 256 * 256 + b * 256 + g) * 8 + 2;
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
    case 2: r = c / (256 * 256); g = c; b = c / 256; break;
    case 3: r = c / (256 * 256); g = c / 256; b = c; break;
    case 4: r = c; g = c / (256 * 256); b = c / 256; break;
    case 5: r = c / 256; g = c / (256 * 256); b = c; break;
    }
    return QColor(r % 256, g % 256, b % 256);
}
#else
static int colorWeight(const QColor color)
{
    int r = color.red(), g = color.green(), b = color.blue();
    return (r * 256 * 256 + g * 256 + b);
}
static QColor weightColor(unsigned weight)
{
    unsigned c = weight;
    unsigned r, g, b;
    r = c / (256 * 256); g = c / 256; b = c;
    return QColor(r % 256, g % 256, b % 256);
}
#endif
//static int frameDone = 0;
//static int frameLimit = 27;
bool D1ImageFrame::load(D1GfxFrame &frame, const QImage &image, const D1Pal *pal)
{
    frame.width = image.width();
    frame.height = image.height();

    frame.pixels.clear();
/*
    if (frameDone >= frameLimit) {
        frame.width = 0;
        frame.height = 0;
        // frame.setFramePal((D1Pal *)pal);
        return true;
    }
*/
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
                    if (wmap.count(w) == 0) {
                        dProgressWarn() << QApplication::tr("New color %1:%2:%3 w %4 at %5:%6").arg(color.red()).arg(color.green()).arg(color.blue()).arg(w).arg(x).arg(y);
                    }
                    wmap[w] += 1;
                }
                wLine.push_back(w);
            }
            weights.push_back(std::move(wLine));
        }

        unsigned pixels = 0;
        for (std::map<int, int>::const_iterator mi = wmap.cbegin(); mi != wmap.cend(); mi++)
            pixels += mi->second;
//        if (frameDone < 100)
//            dProgressWarn() << QApplication::tr("Starting %1 w %2:%3 groups: %4 colors %5 pixels %6").arg(frameDone).arg(frame.width).arg(frame.height).arg(pixels / D1PAL_COLORS).arg(wmap.size()).arg(pixels);

        std::map<int, int> cmap; // weight to palette-index
        int n, i = 0;
#if 0
        while (true) {
            n = (pixels + D1PAL_COLORS - 1) / D1PAL_COLORS;
            std::map<int, int>::iterator mi = wmap.begin();
            if (mi == wmap.end() || mi->second < n) {
                if (mi != wmap.end())
                dProgressWarn() << QApplication::tr("Not enough front color w %1 refs %2 gs: %3").arg(mi->first).arg(mi->second).arg(n);
                break;
            }
            QColor c = weightColor(mi->first);
            if (frameDone < 100)
            dProgressWarn() << QApplication::tr("Keeping front color w %1 in %2 with %3 refs (rgb=%4:%5:%6").arg(mi->first).arg(i).arg(mi->second).arg(c.red()).arg(c.green()).arg(c.blue());
            pixels -= mi->second;
            framePal->setColor(i, c);
            cmap[mi->first] = i;
            wmap.erase(mi->first);
            i++;
        }
        int si = i;
        while (true) {
            n = (pixels + D1PAL_COLORS - 1) / D1PAL_COLORS;
            std::map<int, int>::reverse_iterator mi = wmap.rbegin();
            if (mi == wmap.rend() || mi->second < n) {
                if (mi != wmap.rend())
                dProgressWarn() << QApplication::tr("Not enough back color w %1 refs %2 gs: %3").arg(mi->first).arg(mi->second).arg(n);
                break;
            }
            QColor c = weightColor(mi->first);
            if (frameDone < 100)
            dProgressWarn() << QApplication::tr("Keeping back color w %1 in %2 with %3 refs (rgb=%4:%5:%6").arg(mi->first).arg(i).arg(mi->second).arg(c.red()).arg(c.green()).arg(c.blue());
            pixels -= mi->second;
            framePal->setColor(i, c);
            cmap[mi->first] = i;
            wmap.erase(mi->first);
            i++;
        }
        std::vector<int> lastmap;

        std::map<int, int>::iterator mi = wmap.begin();
        int nl = 0; 
        for ( ; i < D1PAL_COLORS; i++) {
            nl += n;
            std::vector<int> currmap;
            int cc = 0;
            uint64_t sum = 0;
            while (mi != wmap.end()) {
                int cw = mi->first;
                int nc = mi->second;
                cc += nc;
                sum += (uint64_t)cw *  nc;
                currmap.push_back(cw);
                mi++;
                nl -= nc;
                if (nl <= 0) {
                    break;
                }
            }
            if (cc == 0) {
                break;
            }
            sum /= cc;
            int best = INT_MAX;
            int res = 0;
            for (const int ce : currmap) {
                int diff = abs(ce - (int)sum);
                if (diff < best) {
                    best = diff;
                    res = ce;
                }
            }

            QColor c = weightColor(res);
            framePal->setColor(i, c);
            if (frameDone < 100)
            dProgressWarn() << QApplication::tr("Using color w %1 in %2 with %3 refs (rgb=%4:%5:%6) cc:%7 in %8").arg(res).arg(i).arg(sum).arg(c.red()).arg(c.green()).arg(c.blue()).arg(cc).arg(currmap.size());
            std::vector<PaletteColor> colors;
            colors.push_back(PaletteColor(c, i));
            if (!lastmap.empty()) {
                colors.push_back(PaletteColor(framePal->getColor(i - 1), i - 1));

                for (const int ce : lastmap) {
                    QColor wc = weightColor(ce);
                    cmap[ce] = getPalColor(colors, wc);
                }
            } else if (si != 0) {
                colors.push_back(PaletteColor(framePal->getColor(si - 1), si - 1));
            }
            if (i >= D1PAL_COLORS - 2 && si != ei) {
                colors.push_back(PaletteColor(framePal->getColor(si), si));
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
#else
        n = (pixels + D1PAL_COLORS - 1) / D1PAL_COLORS;
        for (std::map<int, int>::iterator mi = wmap.begin(); mi != wmap.end(); ) {
            if (mi->second < n) {
                mi++;
                continue;
            }
            QColor c = weightColor(mi->first);
//            if (frameDone < 100)
//            dProgressWarn() << QApplication::tr("Keeping back color w %1 in %2 with %3 refs (rgb=%4:%5:%6").arg(mi->first).arg(i).arg(mi->second).arg(c.red()).arg(c.green()).arg(c.blue());
            pixels -= mi->second;
            // framePal->setColor(i, c);
            colors.push_back(PaletteColor(c, i));
            cmap[mi->first] = i;
            wmap.erase(mi);
            i++;
            if (i == D1PAL_COLORS)
                break;
            n = (pixels + (D1PAL_COLORS - i) - 1) / (D1PAL_COLORS - i);
            mi = wmap.begin();
        }

        for ( ; i < D1PAL_COLORS; i++) {
#if 0
            uint64_t worst = 0;
            int res = -1;
            for (std::map<int, int>::iterator mi = wmap.begin(); mi != wmap.end(); mi++) {
                QColor wc = weightColor(mi->first);
                std::pair<quint8, int> pc = getPalColor(colors, wc);
                uint64_t curr = (uint64_t)mi->second * pc.second; 
                if (curr > worst || res < 0) {
                    res = mi->first;
                    worst = curr;
                }
            }
            if (res < 0) {
                break;
            }
#else
            uint64_t r = 0, g = 0, b = 0; uint64_t tw = 0;
            for (std::map<int, int>::iterator mi = wmap.begin(); mi != wmap.end(); mi++) {
                QColor wc = weightColor(mi->first);
                std::pair<quint8, int> pc = getPalColor(colors, wc);
                uint64_t cw = (uint64_t)mi->second * pc.second;
                r += cw * wc.red();
                g += cw * wc.green();
                b += cw * wc.blue();
                tw += cw;
            }
            if (tw == 0) {
                break;
            }

            QColor color = QColor(r / tw, g / tw, b / tw);

            unsigned res = 0;
            int best = INT_MAX;

            for (std::map<int, int>::iterator mi = wmap.begin(); mi != wmap.end(); mi++) {
                QColor palColor = weightColor(mi->first);
                int currR = color.red() - palColor.red();
                int currG = color.green() - palColor.green();
                int currB = color.blue() - palColor.blue();
                int curr = currR * currR + currG * currG + currB * currB;
                if (curr < best) {
                    best = curr;
                    res = mi->first;
                }
            }
#endif
            QColor c = weightColor(res);
//            if (frameDone < 100)
//            dProgressWarn() << QApplication::tr("Keeping back color w %1 in %2 with %3 refs (rgb=%4:%5:%6").arg(res).arg(i).arg(wmap[res]).arg(c.red()).arg(c.green()).arg(c.blue());
            colors.push_back(PaletteColor(c, i));
            cmap[res] = i;
            wmap.erase(res);
        }
        // RegisterPalette
        D1Pal *framePal = new D1Pal();
        framePal->load(D1Pal::EMPTY_PATH);
        for (const PaletteColor pc : colors) {
            framePal->setColor(pc.index(), QColor(pc.red(), pc.green(), pc.blue()));
        }
        for (std::map<int, int>::iterator mi = wmap.begin(); mi != wmap.end(); mi++) {
            QColor wc = weightColor(mi->first);
            std::pair<quint8, int> pc = getPalColor(colors, wc);
//            if (frameDone < 100) {
//                QColor c = framePal->getColor(pc.first);
//                dProgressWarn() << QApplication::tr("Replacing color w %1 from %2 with %3 refs (rgb=%4:%5:%6 -> %7:%8:%9)").arg(mi->first).arg(pc.first).arg(mi->second).arg(wc.red()).arg(wc.green()).arg(wc.blue()).arg(c.red()).arg(c.green()).arg(c.blue());
//            }
            cmap[mi->first] = pc.first;
        }
#endif
        frame.setFramePal(framePal);

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
                    pixelLine.push_back(D1GfxPixel::colorPixel(getPalColor(colors, color).first));
                }
            }
            frame.pixels.push_back(std::move(pixelLine));
        }
    }
//    frameDone++;
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
                    color = getPalColor(colors, QColor(pixel.right(7))).first;
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
