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

bool D1ImageFrame::load(D1GfxFrame &frame, const QImage &image, bool forSmk, const D1Pal *pal)
{
    frame.width = image.width();
    frame.height = image.height();

    frame.pixels.clear();

    std::vector<PaletteColor> colors;
    pal->getValidColors(colors);

    if (colors.empty()) {
        D1Pal *framePal = new D1Pal();
        framePal->load(D1Pal::EMPTY_PATH);

        framePal->genColors(image, forSmk);

        frame.setFramePal(framePal);

        framePal->getValidColors(colors);
    }
    {
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
                    // smackColor(color, forSmk);
                    pixelLine.push_back(D1GfxPixel::colorPixel(getPalColor(colors, color).first));
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
