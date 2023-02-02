#include "upscaler.h"

#include <QApplication>
#include <QColor>
#include <QMessageBox>
#include <QVector>

#include "d1gfx.h"
#include "d1min.h"
#include "d1pal.h"
#include "leveltabframewidget.h"
#include "progressdialog.h"
#include "upscaledialog.h"

template <class T, int N>
constexpr int lengthof(T (&arr)[N])
{
    return N;
}

class UpscalingParam {
public:
    int multiplier;
    D1Pal *pal;
    QVector<QColor> dynColors;
    int firstfixcolor;
    int lastfixcolor;
};

static bool isPixelFixed(const D1GfxPixel *pixel, const UpscalingParam &params)
{
    return pixel->getPaletteIndex() >= params.firstfixcolor && pixel->getPaletteIndex() <= params.lastfixcolor;
}

// TODO: merge with getPalColor in d1image.cpp ?
static D1GfxPixel getPalColor(const UpscalingParam &params, QColor color)
{
    int res = 0;
    int best = INT_MAX;

    for (int i = 0; i < params.dynColors.count(); i++) {
        QColor palColor = params.dynColors[i];
        int currR = color.red() - palColor.red();
        int currG = color.green() - palColor.green();
        int currB = color.blue() - palColor.blue();
        int curr = currR * currR + currG * currG + currB * currB;
        if (curr < best) {
            best = curr;
            res = i;
        }
    }

    return D1GfxPixel::colorPixel(res);
}

static void BilinearInterpolateColors(const QColor &c0, const QColor &cR, int dx, const QColor &cD, int dy, const QColor &cDR, int len, QColor &res)
{
    res.setAlpha(255);
    res.setRed((c0.red() * (len - dx) * (len - dy) + cR.red() * dx * (len - dy) + cD.red() * (len - dx) * dy + cDR.red() * dx * dy) / (len * len));
    res.setGreen((c0.green() * (len - dx) * (len - dy) + cR.green() * dx * (len - dy) + cD.green() * (len - dx) * dy + cDR.green() * dx * dy) / (len * len));
    res.setBlue((c0.blue() * (len - dx) * (len - dy) + cR.blue() * dx * (len - dy) + cD.blue() * (len - dx) * dy + cDR.blue() * dx * dy) / (len * len));
}

static void BilinearInterpolate2Pixels(const D1GfxPixel *p0, const D1GfxPixel *p1, const UpscalingParam &params, D1GfxPixel &res)
{
    QColor c0 = params.pal->getColor(p0->getPaletteIndex());
    QColor c1 = params.pal->getColor(p1->getPaletteIndex());

    QColor cRes;
    cRes.setRgb((c0.red() + c1.red()) >> 1, (c0.green() + c1.green()) >> 1, (c0.blue() + c1.blue()) >> 1);

    res = getPalColor(params, cRes);
}

static D1GfxPixel BilinearInterpolate(D1GfxPixel *p0, D1GfxPixel *pR, int dx, D1GfxPixel *pD, int dy, D1GfxPixel *pDR, int len, const UpscalingParam &params)
{
    QColor res = QColor(0, 0, 0);

    QColor c0 = params.pal->getColor(p0->getPaletteIndex());
    QColor cR = params.pal->getColor(pR->getPaletteIndex());
    QColor cD = params.pal->getColor(pD->getPaletteIndex());
    QColor cDR = params.pal->getColor(pDR->getPaletteIndex());
    if (pR->isTransparent()) {
        if (pD->isTransparent()) {
            return *p0; // preserve if pixels on the right and down are transparent
        }
        if (pDR->isTransparent()) {
            // interpolate down
            res.setRed((c0.red() * (len - dy) + cD.red() * dy) / len);
            res.setGreen((c0.green() * (len - dy) + cD.green() * dy) / len);
            res.setBlue((c0.blue() * (len - dy) + cD.blue() * dy) / len);
        } else {
            // interpolate down and down-right
            QColor cR_;
            cR_.setRed((c0.red() + cDR.red()) / 2);
            cR_.setGreen((c0.green() + cDR.green()) / 2);
            cR_.setBlue((c0.blue() + cDR.blue()) / 2);
            BilinearInterpolateColors(c0, cR_, dx, cD, dy, cDR, len, res);
        }
    } else if (pD->isTransparent()) {
        if (pDR->isTransparent()) {
            // interpolate right
            res.setRed((c0.red() * (len - dx) + cR.red() * dx) / len);
            res.setGreen((c0.green() * (len - dx) + cR.green() * dx) / len);
            res.setBlue((c0.blue() * (len - dx) + cR.blue() * dx) / len);
        } else {
            // interpolate right and down-right
            QColor cD_;
            cD_.setRed((c0.red() + cDR.red()) / 2);
            cD_.setGreen((c0.green() + cDR.green()) / 2);
            cD_.setBlue((c0.blue() + cDR.blue()) / 2);
            BilinearInterpolateColors(c0, cR, dx, cD_, dy, cDR, len, res);
        }
    } else {
        if (pDR->isTransparent()) {
            // interpolate down and right
            QColor cDR_;
            cDR_.setRed((cR.red() + cD.red()) / 2);
            cDR_.setGreen((cR.green() + cD.green()) / 2);
            cDR_.setBlue((cR.blue() + cD.blue()) / 2);
            BilinearInterpolateColors(c0, cR, dx, cD, dy, cDR_, len, res);
        } else {
            // full bilinear interpolation
            BilinearInterpolateColors(c0, cR, dx, cD, dy, cDR, len, res);
        }
    }

    return getPalColor(params, res);
}

static void useLeftOrBottomAt(int sx, int sy, int dx, int dy, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    D1GfxPixel *pDest = &newPixels[dy][dx];
    bool leftFixed = isPixelFixed(&newPixels[dy][dx - multiplier], params);
    bool bottomFixed = isPixelFixed(&newPixels[dy + multiplier][dx], params);

    D1GfxPixel *pBottom = &origPixels[sy + 1][sx];
    D1GfxPixel *pLeft = &origPixels[sy][sx - 1];
    if (leftFixed || bottomFixed) {
        if (!bottomFixed) {
            // only the left is fixed -> use the bottom
            *pDest = *pBottom;
        } else if (!leftFixed) {
            // only the bottom is fixed -> use the left
            *pDest = *pLeft;
        } else {
            // both colors are fixed -> use one
            *pDest = *pLeft;
        }
    } else {
        // interpolate
        BilinearInterpolate2Pixels(pLeft, pBottom, params, *pDest);
    }
}

static void useRightOrBottomAt(int sx, int sy, int dx, int dy, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    D1GfxPixel *pDest = &newPixels[dy][dx];
    bool bottomFixed = isPixelFixed(&newPixels[dy + multiplier][dx], params);
    bool rightFixed = isPixelFixed(&newPixels[dy][dx + multiplier], params);

    D1GfxPixel *pBottom = &origPixels[sy + 1][sx];
    D1GfxPixel *pRight = &origPixels[sy][sx + 1];
    if (rightFixed || bottomFixed) {
        if (!bottomFixed) {
            // only the right is fixed -> use the bottom
            *pDest = *pBottom;
        } else if (!rightFixed) {
            // only the bottom is fixed -> use the right
            *pDest = *pRight;
        } else {
            // both colors are fixed -> use one
            *pDest = *pRight;
        }
    } else {
        // interpolate
        BilinearInterpolate2Pixels(pRight, pBottom, params, *pDest);
    }
}

typedef enum pattern_pixel {
    PTN_ALPHA,
    PTN_COLOR,
    PTN_DNC,
} pattern_pixel;

typedef quint8 BYTE;

typedef struct UpscalePatterns {
    const BYTE *pattern;
    bool (*fnc)(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params);
} UpscalePatterns;
/*
  c c c   cc cc cc    cc  cc  cc
  a c c X cc cc cc -> cc  cc  cc
  a a c   aa cc cc    aa [cc] cc
          aa cc cc    aa [Ac] cc
          aa aa cc    aa  aa  cc
          aa aa cc    aa  aa  cc
 */
static const BYTE patternLineDownRight[] = {
    // clang-format off
    3, 3,
    PTN_COLOR, PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_ALPHA, PTN_COLOR,
    // clang-format on
};
static bool LineDownRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    // int sx = 0, sy = y + 1;
    int dx = 0, dy = (y + 1) * multiplier;

    // sx += x;
    dx += x * multiplier;
    // move to [1; 0]
    // sx += 1;
    dx += multiplier;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy > xx) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

/*
  c c c   cc cc cc    cc  cc  cc
  c c a X cc cc cc -> cc  cc  cc
  c a a   cc cc aa    cc [cc] aa
          cc cc aa    cc [cA] aa
          cc aa aa    cc  aa  aa
          cc aa aa    cc  aa  aa
 */
static const BYTE patternLineDownLeft[] = {
    // clang-format off
    3, 3,
    PTN_COLOR, PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR, PTN_ALPHA,
    PTN_COLOR, PTN_ALPHA, PTN_ALPHA,
    // clang-format on
};
static bool LineDownLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    // int sx = 0, sy = y + 1;
    int dx = 0, dy = (y + 1) * multiplier;

    // sx += x;
    dx += x * multiplier;

    // move to [1; 0]
    // sx += 1;
    dx += multiplier;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= multiplier - xx) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

/*
  a a c   aa aa cc     aa [aa] cc
  a c c X aa aa cc ->  aa [aC] cc
  c c c   aa cc cc    [aa] cc  cc
          aa cc cc    [aC] cc  cc
          cc cc cc     cc  cc  cc
          cc cc cc     cc  cc  cc
 */
static const BYTE patternLineUpRight[] = {
    // clang-format off
    3, 3,
    PTN_ALPHA, PTN_ALPHA, PTN_COLOR,
    PTN_ALPHA, PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool LineUpRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    // move to [1; 0]
    sx += 1;
    dx += multiplier;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= multiplier - xx) {
                useRightOrBottomAt(sx, sy, dx + xx, dy + yy, origPixels, newPixels, params);
            }
        }
    }

    // move to [0; 1]
    dx -= multiplier;
    dy += multiplier;
    sx -= 1;
    sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= multiplier - xx) {
                useRightOrBottomAt(sx, sy, dx + xx, dy + yy, origPixels, newPixels, params);
            }
        }
    }
    return true;
}

/*
  c a a   cc aa aa    cc [aa] aa
  c c a X cc aa aa -> cc [Ca] aa
  c c c   cc cc aa    cc  cc [aa]
          cc cc aa    cc  cc [Ca]
          cc cc cc    cc  cc  cc
          cc cc cc    cc  cc  cc
 */
static const BYTE patternLineUpLeft[] = {
    // clang-format off
    3, 3,
    PTN_COLOR, PTN_ALPHA, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool LineUpLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    // move to [1; 0]
    sx += 1;
    dx += multiplier;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy > xx) {
                useLeftOrBottomAt(sx, sy, dx + xx, dy + yy, origPixels, newPixels, params);
            }
        }
    }

    // move to [2; 1]
    dx += multiplier;
    dy += multiplier;
    sx += 1;
    sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy > xx) {
                useLeftOrBottomAt(sx, sy, dx + xx, dy + yy, origPixels, newPixels, params);
            }
        }
    }
    return true;
}

/*
  c c c c   cc cc cc cc    cc  cc cc  cc
  a c c c X cc cc cc cc -> cc  cc cc  cc
  a a a c   aa cc cc cc    aa [cc cc] cc
            aa cc cc cc    aa [AA cc] cc
            aa aa aa cc    aa  aa aa  cc
            aa aa aa cc    aa  aa aa  cc
 */
static const BYTE patternSlowDownRight[] = {
    // clang-format off
    4, 3,
    PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_COLOR, PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_ALPHA, PTN_ALPHA, PTN_COLOR,
    // clang-format on
};
static bool SlowDownRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    // int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    // sx += x;
    dx += x * multiplier;

    // on the left border or [-1; 0] is alpha or [-1; 1] is not alpha
    /*if (x == 0 || origPixels[sy][sx - 1].isTransparent() || !origPixels[sy + 1][sx - 1].isTransparent()) {
        // on the bottom border or right border or [3; 3] is not alpha or [4; 3] is not alpha
        int origHeight = origPixels.count();
        int origWidth = origPixels[0].count();
        if (y == origHeight - 3 || x == origWidth - 4 || !origPixels[sy + 3][sx + 3].isTransparent() || !origPixels[sy + 3][sx + 4].isTransparent()) {
            return false;
        }
    }*/

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    // sx += 1;
    // sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < 2 * multiplier; xx++) {
            if (yy > xx / 2) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

/*
  c c c c   cc cc cc cc    cc  cc cc  cc
  c c c a X cc cc cc cc -> cc  cc cc  cc
  c a a a   cc cc cc aa    cc [cc cc] cc
            cc cc cc aa    cc [cc AA] aa
            cc aa aa aa    cc  aa aa  aa
            cc aa aa aa    cc  aa aa  aa
 */
static const BYTE patternSlowDownLeft[] = {
    // clang-format off
    4, 3,
    PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_ALPHA,
    PTN_COLOR, PTN_ALPHA, PTN_ALPHA, PTN_ALPHA,
    // clang-format on
};
static bool SlowDownLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    // int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    // sx += x;
    dx += x * multiplier;

    // on the right border or [4; 0] is alpha or [4; 1] is not alpha
    /*if (x == orimg_data.width - 4 || (src + 4)->a != 255 || (src + orimg_data.width + 4)->a == 255) {
        // on the bottom border or left border or [0; 3] is not alpha or [-1; 3] is not alpha
        if (y == orimg_data.height - 3 || x == 0 || (src + orimg_data.width * 3 + 0)->a == 255 || (src + orimg_data.width * 3 - 1)->a == 255) {
            return false;
        }
    }*/

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    // sx += 1;
    // sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < 2 * multiplier; xx++) {
            if (yy >= multiplier - xx / 2) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

/*
  a a a c   aa aa aa cc    aa [aa aa] cc
  a c c c X aa aa aa cc -> aa [aa CC] cc
  c c c c   aa cc cc cc    aa  cc cc  cc
            aa cc cc cc    aa  cc cc  cc
            cc cc cc cc    cc  cc cc  cc
            cc cc cc cc    cc  cc cc  cc
 */
static const BYTE patternSlowUpRight[] = {
    // clang-format off
    4, 3,
    PTN_ALPHA, PTN_ALPHA, PTN_ALPHA, PTN_COLOR,
    PTN_ALPHA, PTN_COLOR, PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool SlowUpRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    // on the left border or [-1; 2] is alpha or [-1; 1] is not alpha
    /*if (x == 0 || (src + orimg_data.width * 2 - 1)->a != 255 || (src + orimg_data.width - 1)->a == 255) {
        // on the top border or right border or [3; -1] is not alpha or [4; -1] is not alpha
        if (y == 0 || x == orimg_data.width - 4 || (src - orimg_data.width + 3)->a == 255 || (src - orimg_data.width + 4)->a == 255) {
            return false;
        }
    }*/

    // move to [1; 0]
    dx += multiplier;
    sx += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < 2 * multiplier; xx++) {
            if (yy >= multiplier - xx / 2) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + multiplier][dx + xx], params);
                bool rightFixed = isPixelFixed(&newPixels[dy][dx + 2 * multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + 1][sx + xx / multiplier];
                D1GfxPixel *pRight = &origPixels[sy][sx + 2];
                if (rightFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the right is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!rightFixed) {
                        // only the bottom is fixed -> use the right
                        *pDest = *pRight;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pRight;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pRight, pBottom, params, *pDest);
                }
            }
        }
    }
    return true;
}

/*
  c a a a   cc aa aa aa    cc [aa aa] aa
  c c c a X cc aa aa aa -> cc [CC aa] aa
  c c c c   cc cc cc aa    cc  cc cc  aa
            cc cc cc aa    cc  cc cc  aa
            cc cc cc cc    cc  cc cc  cc
            cc cc cc cc    cc  cc cc  cc
 */
static const BYTE patternSlowUpLeft[] = {
    // clang-format off
    4, 3,
    PTN_COLOR, PTN_ALPHA, PTN_ALPHA, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool SlowUpLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    // on the right border or [4; 2] is alpha or [4; 1] is not alpha
    /*if (x == orimg_data.width - 4 || (src + orimg_data.width * 2 + 4)->a != 255 || (src + orimg_data.width + 4)->a == 255) {
        // on the top border or left border or [0; -1] is not alpha or [-1; -1] is not alpha
        if (y == 0 || x == 0 || (src - orimg_data.width + 0)->a == 255 || (src - orimg_data.width - 1)->a == 255) {
            return false;
        }
    }*/

    // move to [1; 0]
    dx += multiplier;
    sx += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < 2 * multiplier; xx++) {
            if (yy > xx / 2) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + multiplier][dx + xx], params);
                bool leftFixed = isPixelFixed(&newPixels[dy][dx - multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + 1][sx + xx / multiplier];
                D1GfxPixel *pLeft = &origPixels[sy][sx - 1];
                if (leftFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the left is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!leftFixed) {
                        // only the bottom is fixed -> use the left
                        *pDest = *pLeft;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pLeft;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pLeft, pBottom, params, *pDest);
                }
            }
        }
    }
    return true;
}

/*
  c c c   cc cc cc    cc  cc  cc
  a c c X cc cc cc -> cc  cc  cc
  a c c   aa cc cc    aa [cc] cc
  a a c   aa cc cc    aa [cc] cc
          aa cc cc    aa [Ac] cc
          aa cc cc    aa [Ac] cc
          aa aa cc    aa  aa  aa
          aa aa cc    aa  aa  aa
 */
static const BYTE patternFastDownRight[] = {
    // clang-format off
    3, 4,
    PTN_COLOR, PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_ALPHA, PTN_COLOR,
    // clang-format on
};
static bool FastDownRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    // int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    // sx += x;
    dx += x * multiplier;

    // on the bottom border or [2; 4] is alpha or [1; 4] is not alpha
    /*if (y == orimg_data.height - 4 || (src + orimg_data.width * 4 + 2)->a != 255 || (src + orimg_data.width * 4 + 1)->a == 255) {
        // on the top border or left border or [-1; 0] is not alpha or [-1; -1] is not alpha
        if (y == 0 || x == 0 || (src + orimg_data.width * 0 - 1)->a == 255 || (src - orimg_data.width - 1)->a == 255) {
            return false;
        }
    }*/

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    // sx += 1;
    // sy += 1;

    for (int yy = 0; yy < 2 * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= (xx + 1) * 2) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

/*
  c c c   cc cc cc    cc  cc  cc
  c c a X cc cc cc -> cc  cc  cc
  c c a   cc cc aa    cc [cc] aa
  c a a   cc cc aa    cc [cc] aa
          cc cc aa    cc [cA] aa
          cc cc aa    cc [cA] aa
          cc aa aa    cc  aa  aa
          cc aa aa    cc  aa  aa
 */
static const BYTE patternFastDownLeft[] = {
    // clang-format off
    3, 4,
    PTN_COLOR, PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR, PTN_ALPHA,
    PTN_COLOR, PTN_ALPHA, PTN_ALPHA,
    // clang-format on
};
static bool FastDownLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    // int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    // sx += x;
    dx += x * multiplier;

    // on the bottom border or [0; 4] is alpha or [1; 4] is not alpha
    /*if (y == orimg_data.height - 4 || (src + orimg_data.width * 4 + 0)->a != 255 || (src + orimg_data.width * 4 + 1)->a == 255) {
        // on the top border or right border or [3; 0] is not alpha or [3; -1] is not alpha
        if (y == 0 || x == orimg_data.width - 3 || (src + orimg_data.width * 0 + 3)->a == 255 || (src - orimg_data.width + 3)->a == 255) {
            return false;
        }
    }*/

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    // sx += 1;
    // sy += 1;

    for (int yy = 0; yy < 2 * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= 2 * multiplier - 2 * xx) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

/*
  a a c   aa aa cc     aa  aa cc
  a c c X aa aa cc ->  aa  aa cc
  a c c   aa cc cc    [aa] cc cc
  c c c   aa cc cc    [aa] cc cc
          aa cc cc    [aC] cc cc
          aa cc cc    [aC] cc cc
          cc cc cc     cc  cc cc
          cc cc cc     cc  cc cc
 */
static const BYTE patternFastUpRight[] = {
    // clang-format off
    3, 4,
    PTN_ALPHA, PTN_ALPHA, PTN_COLOR,
    PTN_ALPHA, PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool FastUpRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    // on the top border or [2; -1] is alpha or [1; -1] is not alpha
    /*if (y == 0 || (src - orimg_data.width + 2)->a != 255 || (src - orimg_data.width + 1)->a == 255) {
        // on the bottom border or left border or [-1; 3] is not alpha or [-1; 4] is not alpha
        if (y == imagedata.height - 4 || x == 0 || (src + orimg_data.width * 3 - 1)->a == 255 || (src + orimg_data.width * 4 - 1)->a == 255) {
            return false;
        }
    }*/

    // move to [0; 1]
    dy += multiplier;
    sy += 1;

    for (int yy = 0; yy < 2 * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= 2 * multiplier - 2 * xx) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + 2 * multiplier][dx], params);
                bool rightFixed = isPixelFixed(&newPixels[dy + yy][dx + multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + 2][sx];
                D1GfxPixel *pRight = &origPixels[sy + yy / multiplier][sx + 1];
                if (rightFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the right is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!rightFixed) {
                        // only the bottom is fixed -> use the right
                        *pDest = *pRight;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pRight;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pRight, pBottom, params, *pDest);
                }
            }
        }
    }
    return true;
}

/*
  c a a   cc aa aa    cc aa  aa
  c c a X cc aa aa -> cc aa  aa
  c c a   cc cc aa    cc cc [aa]
  c c c   cc cc aa    cc cc [aa]
          cc cc aa    cc cc [Ca]
          cc cc aa    cc cc [Ca]
          cc cc cc    cc cc  cc
          cc cc cc    cc cc  cc
 */
static const BYTE patternFastUpLeft[] = {
    // clang-format off
    3, 4,
    PTN_COLOR, PTN_ALPHA, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool FastUpLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    // on the top border or [0; -1] is alpha or [1; -1] is not alpha
    /*if (y == 0 || (src - orimg_data.width + 0)->a != 255 || (src - orimg_data.width + 1)->a == 255) {
        // on the bottom border or right border or [3; 3] is not alpha or [3; 4] is not alpha
        if (y == imagedata.height - 4 || x == imagedata.width - 3 || (src + orimg_data.width * 3 + 3)->a == 255 || (src + orimg_data.width * 4 + 3)->a == 255) {
            return false;
        }
    }*/

    // move to [2; 1]
    dx += 2 * multiplier;
    dy += multiplier;
    sx += 2;
    sy += 1;

    for (int yy = 0; yy < 2 * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= 2 * (xx + 1)) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + 2 * multiplier][dx], params);
                bool leftFixed = isPixelFixed(&newPixels[dy + yy][dx + xx - multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + 2][sx];
                D1GfxPixel *pLeft = &origPixels[sy + yy / multiplier][sx - 1];
                if (leftFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the left is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!leftFixed) {
                        // only the bottom is fixed -> use the left
                        *pDest = *pLeft;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pLeft;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pLeft, pBottom, params, *pDest);
                }
            }
        }
    }
    return true;
}
#define PATLEN 0
/*
  [c c] c c   cc cc cc cc    cc  cc cc  cc
  [a c] c c X cc cc cc cc -> cc  cc cc  cc
  [a a] a c   aa cc cc cc    aa [cc cc] cc
              aa cc cc cc    aa [AA cc] cc
              aa aa aa cc    aa  aa aa  cc
              aa aa aa cc    aa  aa aa  cc
 */
static const BYTE patternAnySlowDownRight[] = {
    // clang-format off
    2, 3,
    PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_COLOR,
    // clang-format on
};
static bool AnySlowDownRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;

    sx += x;

    int len = 1;
    while (sx > len && !origPixels[sy][sx - len].isTransparent() && !origPixels[sy + 1][sx - len].isTransparent() && origPixels[sy + 2][sx - len].isTransparent()) {
        len++;
    }

    if (sx <= len || origPixels[sy][sx - len].isTransparent() || !origPixels[sy + 1][sx - len].isTransparent() || !origPixels[sy + 2][sx - len].isTransparent())
        return false;

    if (len <= PATLEN)
        return false;

    sx -= len;

    int dx = 0, dy = sy * multiplier;
    dx += sx * multiplier;

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    // sx += 1;
    // sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < len * multiplier; xx++) {
            if (yy > xx / len) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

/*
  [c c c]   cc cc cc    cc  cc  cc
  [a c c] X cc cc cc -> cc  cc  cc
   a c c    aa cc cc    aa [cc] cc
   a a c    aa cc cc    aa [cc] cc
            aa cc cc    aa [Ac] cc
            aa cc cc    aa [Ac] cc
            aa aa cc    aa  aa  cc
            aa aa cc    aa  aa  cc
 */
static const BYTE patternAnyFastDownRight[] = {
    // clang-format off
    3, 2,
    PTN_ALPHA, PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_ALPHA, PTN_COLOR,
    // clang-format on
};
static bool AnyFastDownRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;

    sx += x;

    int len = 1;
    while (sy > len && origPixels[sy - len][sx].isTransparent() && !origPixels[sy - len][sx + 1].isTransparent() && !origPixels[sy - len][sx + 2].isTransparent()) {
        len++;
    }

    if (sy <= len || origPixels[sy - len][sx].isTransparent() || origPixels[sy - len][sx + 1].isTransparent() || origPixels[sy - len][sx + 2].isTransparent())
        return false;

    if (len <= PATLEN)
        return false;

    sy -= len;

    int dx = 0, dy = sy * multiplier;
    dx += sx * multiplier;

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    // sx += 1;
    // sy += 1;

    for (int yy = 0; yy < len * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= (xx + 1) * len) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

/*
  [c c] c c   cc cc cc cc    cc  cc cc  cc
  [c c] c a X cc cc cc cc -> cc  cc cc  cc
  [c a] a a   cc cc cc aa    cc [cc cc] cc
              cc cc cc aa    cc [cc AA] aa
              cc aa aa aa    cc  aa aa  aa
              cc aa aa aa    cc  aa aa  aa
 */
static const BYTE patternAnySlowDownLeft[] = {
    // clang-format off
    2, 3,
    PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_ALPHA,
    PTN_ALPHA, PTN_ALPHA,
    // clang-format on
};
static bool AnySlowDownLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;

    sx += x;

    int len = 1;
    while (sx > len && !origPixels[sy][sx - len].isTransparent() && !origPixels[sy + 1][sx - len].isTransparent() && origPixels[sy + 2][sx - len].isTransparent()) {
        len++;
    }

    if (sx <= len || origPixels[sy][sx - len].isTransparent() || origPixels[sy + 1][sx - len].isTransparent() || origPixels[sy + 2][sx - len].isTransparent())
        return false;

    if (len <= PATLEN)
        return false;

    sx -= len;

    int dx = 0, dy = sy * multiplier;
    dx += sx * multiplier;

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    // sx += 1;
    // sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < len * multiplier; xx++) {
            if (yy >= multiplier - xx / len) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

/*
  [c c c]   cc cc cc    cc  cc  cc
  [c c a] X cc cc cc -> cc  cc  cc
   c c a    cc cc aa    cc [cc] aa
   c a a    cc cc aa    cc [cc] aa
            cc cc aa    cc [cA] aa
            cc cc aa    cc [cA] aa
            cc aa aa    cc  aa  aa
            cc aa aa    cc  aa  aa
 */
static const BYTE patternAnyFastDownLeft[] = {
    // clang-format off
    3, 2,
    PTN_COLOR, PTN_COLOR, PTN_ALPHA,
    PTN_COLOR, PTN_ALPHA, PTN_ALPHA,
    // clang-format on
};
static bool AnyFastDownLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;

    sx += x;

    int len = 1;
    while (sy > len && !origPixels[sy - len][sx].isTransparent() && !origPixels[sy - len][sx + 1].isTransparent() && origPixels[sy - len][sx + 2].isTransparent()) {
        len++;
    }

    if (sy <= len || origPixels[sy - len][sx].isTransparent() || origPixels[sy - len][sx + 1].isTransparent() || origPixels[sy - len][sx + 2].isTransparent())
        return false;

    if (len <= PATLEN)
        return false;

    sy -= len;

    int dx = 0, dy = sy * multiplier;
    dx += sx * multiplier;

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    // sx += 1;
    // sy += 1;

    for (int yy = 0; yy < len * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= len * (multiplier - xx)) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

/*
  [a a] a c   aa aa aa cc    aa [aa aa] cc
  [a c] c c X aa aa aa cc -> aa [aa CC] cc
  [c c] c c   aa cc cc cc    aa  cc cc  cc
              aa cc cc cc    aa  cc cc  cc
              cc cc cc cc    cc  cc cc  cc
              cc cc cc cc    cc  cc cc  cc
 */
static const BYTE patternAnySlowUpRight[] = {
    // clang-format off
    2, 3,
    PTN_ALPHA, PTN_COLOR,
    PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool AnySlowUpRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;

    sx += x;

    int len = 1;
    while (sx > len && origPixels[sy][sx - len].isTransparent() && !origPixels[sy + 1][sx - len].isTransparent() && !origPixels[sy + 2][sx - len].isTransparent()) {
        len++;
    }

    if (sx <= len || !origPixels[sy][sx - len].isTransparent() || !origPixels[sy + 1][sx - len].isTransparent() || origPixels[sy + 2][sx - len].isTransparent())
        return false;

    if (len <= PATLEN)
        return false;

    sx -= len;

    int dx = 0, dy = sy * multiplier;
    dx += sx * multiplier;

    // move to [1; 0]
    dx += multiplier;
    sx += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < len * multiplier; xx++) {
            if (yy >= multiplier - xx / len) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + multiplier][dx + xx], params);
                bool rightFixed = isPixelFixed(&newPixels[dy][dx + len * multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + 1][sx + xx / multiplier];
                D1GfxPixel *pRight = &origPixels[sy][sx + len];
                if (rightFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the right is fixed -> use the bottom
                        *pDest = origPixels[sy + 1][sx + xx / multiplier];
                    } else if (!rightFixed) {
                        // only the bottom is fixed -> use the right
                        *pDest = *pRight;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pRight;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pRight, pBottom, params, *pDest);
                }
            }
        }
    }
    return true;
}

/*
  [a a c]   aa aa cc     aa  aa cc
  [a c c] X aa aa cc ->  aa  aa cc
   a c c    aa cc cc    [aa] cc cc
   c c c    aa cc cc    [aa] cc cc
            aa cc cc    [aC] cc cc
            aa cc cc    [aC] cc cc
            cc cc cc     cc  cc cc
            cc cc cc     cc  cc cc
 */
static const BYTE patternAnyFastUpRight[] = {
    // clang-format off
    3, 2,
    PTN_ALPHA, PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool AnyFastUpRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;

    sx += x;

    int len = 1;
    while (sy > len && origPixels[sy - len][sx].isTransparent() && !origPixels[sy - len][sx + 1].isTransparent() && !origPixels[sy - len][sx + 2].isTransparent()) {
        len++;
    }

    if (sy <= len || !origPixels[sy - len][sx].isTransparent() || !origPixels[sy - len][sx + 1].isTransparent() || origPixels[sy - len][sx + 2].isTransparent())
        return false;

    if (len <= PATLEN)
        return false;

    sy -= len;

    int dx = 0, dy = sy * multiplier;
    dx += sx * multiplier;

    // move to [0; 1]
    dy += multiplier;
    sy += 1;

    for (int yy = 0; yy < len * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= len * (multiplier - xx)) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + len * multiplier][dx + xx], params);
                bool rightFixed = isPixelFixed(&newPixels[dy + yy][dx + multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + len][sx];
                D1GfxPixel *pRight = &origPixels[sy + yy / multiplier][sx + 1];
                if (rightFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the right is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!rightFixed) {
                        // only the bottom is fixed -> use the right
                        *pDest = *pRight;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pRight;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pRight, pBottom, params, *pDest);
                }
            }
        }
    }
    return true;
}

/*
  [c a] a a   cc aa aa aa    cc [aa aa] aa
  [c c] c a X cc aa aa aa -> cc [CC aa] aa
  [c c] c c   cc cc cc aa    cc  cc cc  aa
              cc cc cc aa    cc  cc cc  aa
              cc cc cc cc    cc  cc cc  cc
              cc cc cc cc    cc  cc cc  cc
 */
static const BYTE patternAnySlowUpLeft[] = {
    // clang-format off
    2, 3,
    PTN_ALPHA, PTN_ALPHA,
    PTN_COLOR, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool AnySlowUpLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;

    sx += x;

    int len = 1;
    while (sx > len && origPixels[sy][sx - len].isTransparent() && !origPixels[sy + 1][sx - len].isTransparent() && !origPixels[sy + 2][sx - len].isTransparent()) {
        len++;
    }

    if (sx <= len || origPixels[sy][sx - len].isTransparent() || origPixels[sy + 1][sx - len].isTransparent() || origPixels[sy + 2][sx - len].isTransparent())
        return false;

    if (len <= PATLEN)
        return false;

    sx -= len;

    int dx = 0, dy = sy * multiplier;
    dx += sx * multiplier;

    // move to [1; 0]
    dx += multiplier;
    sx += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < len * multiplier; xx++) {
            if (yy > xx / len) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + multiplier][dx + xx], params);
                bool leftFixed = isPixelFixed(&newPixels[dy][dx - 1], params);

                D1GfxPixel *pBottom = &origPixels[sy + 1][sx + xx / multiplier];
                D1GfxPixel *pLeft = &origPixels[sy][sx - 1];
                if (leftFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the left is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!leftFixed) {
                        // only the bottom is fixed -> use the left
                        *pDest = *pLeft;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pLeft;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pLeft, pBottom, params, *pDest);
                }
            }
        }
    }
    return true;
}

/*
  [c a a]   cc aa aa    cc aa  aa
  [c c a] X cc aa aa -> cc aa  aa
   c c a    cc cc aa    cc cc [aa]
   c c c    cc cc aa    cc cc [aa]
            cc cc aa    cc cc [Ca]
            cc cc aa    cc cc [Ca]
            cc cc cc    cc cc  cc
            cc cc cc    cc cc  cc
 */
static const BYTE patternAnyFastUpLeft[] = {
    // clang-format off
    3, 2,
    PTN_COLOR, PTN_COLOR, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool AnyFastUpLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;

    sx += x;

    int len = 1;
    while (sy > len && !origPixels[sy - len][sx].isTransparent() && !origPixels[sy - len][sx + 1].isTransparent() && origPixels[sy - len][sx + 2].isTransparent()) {
        len++;
    }

    if (sy <= len || origPixels[sy - len][sx].isTransparent() || !origPixels[sy - len][sx + 1].isTransparent() || !origPixels[sy - len][sx + 2].isTransparent())
        return false;

    if (len <= PATLEN)
        return false;

    sy -= len;

    int dx = 0, dy = sy * multiplier;
    dx += sx * multiplier;

    // move to [2; 1]
    dx += 2 * multiplier;
    dy += multiplier;
    sx += 2;
    sy += 1;

    for (int yy = 0; yy < len * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= len * (xx + 1)) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + len * multiplier][dx + xx], params);
                bool leftFixed = isPixelFixed(&newPixels[dy + yy][dx - 1], params);

                D1GfxPixel *pBottom = &origPixels[sy + len][sx];
                D1GfxPixel *pLeft = &origPixels[sy + yy / multiplier][sx - 1];
                if (leftFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the left is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!leftFixed) {
                        // only the bottom is fixed -> use the left
                        *pDest = *pLeft;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pLeft;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pLeft, pBottom, params, *pDest);
                }
            }
        }
    }
    return true;
}

/*
  [c a] a     cc aa aa       cc [aa aa]
  [a c] c a X cc aa aa    -> cc [CC aa]
  [  a] a c   aa cc cc aa    aa [cc cc] aa
              aa cc cc aa    aa [AA cc] aa
                 aa aa cc        aa aa  cc
                 aa aa cc        aa aa  cc
 */
static const BYTE patternAnySlowDownNarrow[] = {
    // clang-format off
    2, 3,
    PTN_ALPHA, PTN_DNC,
    PTN_COLOR, PTN_ALPHA,
    PTN_ALPHA, PTN_COLOR,
    // clang-format on
};
static bool AnySlowDownNarrow(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;

    sx += x;

    int len = 1;
    while (sx > len && origPixels[sy][sx - len].isTransparent() && !origPixels[sy + 1][sx - len].isTransparent() && origPixels[sy + 2][sx - len].isTransparent()) {
        len++;
    }

    if (sx <= len || origPixels[sy][sx - len].isTransparent() || !origPixels[sy + 1][sx - len].isTransparent())
        return false;

    if (len <= PATLEN)
        return false;

    sx -= len;

    int dx = 0, dy = sy * multiplier;
    dx += sx * multiplier;

    // move to [1; 0]
    dx += multiplier;
    sx += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < len * multiplier; xx++) {
            if (yy > xx / len) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + multiplier][dx + xx], params);
                bool leftFixed = isPixelFixed(&newPixels[dy][dx - 1], params);

                D1GfxPixel *pBottom = &origPixels[sy + 1][sx + xx / multiplier];
                D1GfxPixel *pLeft = &origPixels[sy][sx - 1];
                if (leftFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the left is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!leftFixed) {
                        // only the bottom is fixed -> use the left
                        *pDest = *pLeft;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pLeft;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pLeft, pBottom, params, *pDest);
                }
            }
        }
    }

    // move to [1; 1]
    dy += multiplier;
    // sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < len * multiplier; xx++) {
            if (yy > xx / len) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }

    return true;
}

/*
  [c a  ]   cc aa       cc  aa
  [a c a] X cc aa    -> cc  aa
   a c a    aa cc aa    aa [cc aa]
     a c    aa cc aa    aa [cc aa]
            aa cc aa    aa [Ac Ca]
            aa cc aa    aa [Ac Ca]
               aa cc        aa  cc
               aa cc        aa  cc
 */
static const BYTE patternAnyFastDownNarrow[] = {
    // clang-format off
    3, 2,
    PTN_ALPHA, PTN_COLOR, PTN_ALPHA,
    PTN_DNC,   PTN_ALPHA, PTN_COLOR,
    // clang-format on
};
static bool AnyFastDownNarrow(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;

    sx += x;

    int len = 1;
    while (sy > len && origPixels[sy - len][sx].isTransparent() && !origPixels[sy - len][sx + 1].isTransparent() && origPixels[sy - len][sx + 2].isTransparent()) {
        len++;
    }

    if (sy <= len || origPixels[sy - len][sx].isTransparent() || !origPixels[sy - len][sx + 1].isTransparent())
        return false;

    if (len <= PATLEN)
        return false;

    sy -= len;

    int dx = 0, dy = sy * multiplier;
    dx += sx * multiplier;

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    sx += 1;
    sy += 1;

    for (int yy = 0; yy < len * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= (xx + 1) * len) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }

    // move to [2; 1]
    dx += multiplier;
    sx += 1;

    for (int yy = 0; yy < len * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= len * (xx + 1)) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + len * multiplier][dx + xx], params);
                bool leftFixed = isPixelFixed(&newPixels[dy + yy][dx - 1], params);

                D1GfxPixel *pBottom = &origPixels[sy + len][sx];
                D1GfxPixel *pLeft = &origPixels[sy + yy / multiplier][sx - 1];
                if (leftFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the left is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!leftFixed) {
                        // only the bottom is fixed -> use the left
                        *pDest = *pLeft;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pLeft;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pLeft, pBottom, params, *pDest);
                }
            }
        }
    }
    return true;
}

/*
  [  a] a c      aa aa cc       [aa aa] cc
  [a c] c a X    aa aa cc ->    [aa CC] cc
  [c a] a     aa cc cc aa    aa [cc cc] aa
              aa cc cc aa    aa [cc AA] aa
              cc aa aa       cc  aa aa
              cc aa aa       cc  aa aa
 */
static const BYTE patternAnySlowUpNarrow[] = {
    // clang-format off
    2, 3,
    PTN_ALPHA, PTN_COLOR,
    PTN_COLOR, PTN_ALPHA,
    PTN_ALPHA, PTN_DNC,
    // clang-format on
};
static bool AnySlowUpNarrow(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;

    sx += x;

    int len = 1;
    while (sx > len && origPixels[sy][sx - len].isTransparent() && !origPixels[sy + 1][sx - len].isTransparent() && origPixels[sy + 2][sx - len].isTransparent()) {
        len++;
    }

    if (sx <= len || !origPixels[sy + 1][sx - len].isTransparent() || origPixels[sy + 2][sx - len].isTransparent())
        return false;

    if (len <= PATLEN)
        return false;

    sx -= len;

    int dx = 0, dy = sy * multiplier;
    dx += sx * multiplier;

    // move to [1; 0]
    dx += multiplier;
    sx += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < len * multiplier; xx++) {
            if (yy >= multiplier - xx / len) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + multiplier][dx + xx], params);
                bool rightFixed = isPixelFixed(&newPixels[dy][dx + len * multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + 1][sx + xx / multiplier];
                D1GfxPixel *pRight = &origPixels[sy][sx + len];
                if (rightFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the right is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!rightFixed) {
                        // only the bottom is fixed -> use the right
                        *pDest = *pRight;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pRight;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pRight, pBottom, params, *pDest);
                }
            }
        }
    }

    // move to [1; 1]
    dy += multiplier;
    // sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < len * multiplier; xx++) {
            if (yy >= multiplier - xx / len) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

/*
  [  a c]      aa cc          aa  cc
  [a c a] X    aa cc ->       aa  cc
   a c a    aa cc aa    [aa] [cc] aa
   c a      aa cc aa    [aa] [cc] aa
            aa cc aa    [aC] [cA] aa
            aa cc aa    [aC] [cA] aa
            cc aa        cc   aa
            cc aa        cc   aa
 */
static const BYTE patternAnyFastUpNarrow[] = {
    // clang-format off
    3, 2,
    PTN_ALPHA, PTN_COLOR, PTN_ALPHA,
    PTN_COLOR, PTN_ALPHA, PTN_DNC,
    // clang-format on
};
static bool AnyFastUpNarrow(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;

    sx += x;

    int len = 1;
    while (sy > len && origPixels[sy - len][sx].isTransparent() && !origPixels[sy - len][sx + 1].isTransparent() && origPixels[sy - len][sx + 2].isTransparent()) {
        len++;
    }

    if (sy <= len || !origPixels[sy - len][sx + 1].isTransparent() || origPixels[sy - len][sx + 2].isTransparent())
        return false;

    if (len <= PATLEN)
        return false;

    sy -= len;

    int dx = 0, dy = sy * multiplier;
    dx += sx * multiplier;

    // move to [0; 1]
    dy += multiplier;
    sy += 1;

    for (int yy = 0; yy < len * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= len * (multiplier - xx)) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + len * multiplier][dx + xx], params);
                bool rightFixed = isPixelFixed(&newPixels[dy + yy][dx + multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + len][sx];
                D1GfxPixel *pRight = &origPixels[sy + yy / multiplier][sx + 1];
                if (rightFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the right is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!rightFixed) {
                        // only the bottom is fixed -> use the right
                        *pDest = *pRight;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pRight;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pRight, pBottom, params, *pDest);
                }
            }
        }
    }

    // move to [1; 1]
    dx += multiplier;
    sx += 1;

    for (int yy = 0; yy < len * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= len * (multiplier - xx)) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

static D1GfxPixel *FixColorCheck(int x, int y, QList<QList<D1GfxPixel>> &origPixels, const UpscalingParam &params, const BYTE *pattern)
{
    int w = pattern[0];
    int h = pattern[1];

    D1GfxPixel *fP = NULL;
    const BYTE *ptnCol = &pattern[2];

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++, ptnCol++) {
            if (*ptnCol != PTN_COLOR)
                continue;
            D1GfxPixel *cP = &origPixels[y + j][x + i];
            if (fP == NULL) {
                if (!isPixelFixed(cP, params))
                    return NULL;
                fP = cP;
                continue;
            }
            if (*fP != *cP)
                return NULL;
        }
    }

    return fP;
}

/*
  [f1 f1] f1 f1   f1f1 f1f1 f1f1 f1f1    f1f1  f1f1 f1f1  f1f1
  [   f1] f1 f1 X f1f1 f1f1 f1f1 f1f1 -> f1f1  f1f1 f1f1  f1f1
  [     ]    f1        f1f1 f1f1 f1f1         [f1f1 f1f1] f1f1
                       f1f1 f1f1 f1f1         [???? f1f1] f1f1
                                 f1f1                     f1f1
                                 f1f1                     f1f1
 */
static const BYTE patternFixSlowDownRight[] = {
    // clang-format off
    2, 3,
    PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR,
    PTN_DNC,   PTN_COLOR,
    // clang-format on
};
static bool FixSlowDownRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    D1GfxPixel *fP = FixColorCheck(x, y, origPixels, params, patternFixSlowDownRight);
    if (fP == NULL)
        return false;

    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    if (newPixels[dy + 2 * multiplier][dx] == *fP)
        return false;

    int len = 1;
    while (sx > len
        && newPixels[dy][dx - len * multiplier] == *fP && newPixels[dy + multiplier][dx - len * multiplier] == *fP && newPixels[dy + 2 * multiplier][dx - len * multiplier] != *fP) {
        len++;
    }

    if (sx <= len
        || newPixels[dy][dx - len * multiplier] != *fP || newPixels[dy + multiplier][dx - len * multiplier] == *fP || newPixels[dy + 2 * multiplier][dx - len * multiplier] == *fP)
        return false;

    if (len <= PATLEN)
        return false;

    sx -= len;
    dx -= len * multiplier;

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    sx += 1;
    sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < len * multiplier; xx++) {
            if (yy > xx / len) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + yy + multiplier][dx + xx], params);
                bool leftFixed = isPixelFixed(&newPixels[dy + yy][dx + xx - multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + 1][sx + xx / multiplier];
                D1GfxPixel *pLeft = &origPixels[sy][sx + xx / multiplier - 1];
                if (leftFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the left is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!leftFixed) {
                        // only the bottom is fixed -> use the left
                        *pDest = *pLeft;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pLeft;
                    }
                } else {
                    // interpolate
                    if (!pLeft->isTransparent()) {
                        if (!pBottom->isTransparent()) {
                            // both are colored
                            BilinearInterpolate2Pixels(pLeft, pBottom, params, *pDest);
                        } else {
                            // only left is colored -> use the left
                            *pDest = *pLeft;
                        }
                    } else if (!pBottom->isTransparent()) {
                        // only bottom is colored -> use the bottom
                        *pDest = *pBottom;
                    } else {
                        // neither is colored -> make it alpha
                        *pDest = *pBottom;
                    }
                }
            }
        }
    }
    return false;
}

/*
  [f1 f1 f1]   f1f1 f1f1 f1f1    f1f1  f1f1  f1f1
  [   f1 f1] X f1f1 f1f1 f1f1 -> f1f1  f1f1  f1f1
      f1 f1         f1f1 f1f1         [f1f1] f1f1
         f1         f1f1 f1f1         [f1f1] f1f1
                    f1f1 f1f1         [??f1] f1f1
                    f1f1 f1f1         [??f1] f1f1
                         f1f1                f1f1
                         f1f1                f1f1
 */
static const BYTE patternFixFastDownRight[] = {
    // clang-format off
    3, 2,
    PTN_DNC, PTN_COLOR, PTN_COLOR,
    PTN_DNC, PTN_DNC,   PTN_COLOR,
    // clang-format on
};
static bool FixFastDownRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    D1GfxPixel *fP = FixColorCheck(x, y, origPixels, params, patternFixFastDownRight);
    if (fP == NULL)
        return false;

    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    if (newPixels[dy][dx] == *fP || newPixels[dy + multiplier][dx + multiplier] == *fP)
        return false;

    int len = 1;
    while (sy > len
        && newPixels[dy - len * multiplier][dx] != *fP && newPixels[dy - len * multiplier][dx + multiplier] == *fP && newPixels[dy - len * multiplier][dx + 2 * multiplier] == *fP) {
        len++;
    }

    if (sy <= len
        || newPixels[dy - len * multiplier][dx] != *fP || newPixels[dy - len * multiplier][dx + multiplier] != *fP || newPixels[dy - len * multiplier][dx + 2 * multiplier] != *fP)
        return false;

    if (len <= PATLEN)
        return false;

    sy -= len;
    dy -= len * multiplier;

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    sx += 1;
    sy += 1;

    for (int yy = 0; yy < len * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= (xx + 1) * len) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + yy + multiplier][dx + xx], params);
                bool leftFixed = isPixelFixed(&newPixels[dy + yy][dx + xx - multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + 1][sx];
                D1GfxPixel *pLeft = &origPixels[sy + yy / multiplier][sx - 1];
                if (leftFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the left is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!leftFixed) {
                        // only the bottom is fixed -> use the left
                        *pDest = *pLeft;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pLeft;
                    }
                } else {
                    // interpolate
                    if (!pLeft->isTransparent()) {
                        if (!pBottom->isTransparent()) {
                            // both are colored
                            BilinearInterpolate2Pixels(pLeft, pBottom, params, *pDest);
                        } else {
                            // only left is colored -> use the left
                            *pDest = *pLeft;
                        }
                    } else if (!pBottom->isTransparent()) {
                        // only bottom is colored -> use the bottom
                        *pDest = *pBottom;
                    } else {
                        // neither is colored -> make it alpha
                        *pDest = *pBottom;
                    }
                }
            }
        }
    }
    return false;
}

/*
  [f1 f1] f1 f1   f1f1 f1f1 f1f1 f1f1    f1f1  f1f1 f1f1  f1f1
  [f1 f1] f1    X f1f1 f1f1 f1f1 f1f1 -> f1f1  f1f1 f1f1  f1f1
  [f1   ]         f1f1 f1f1 f1f1         f1f1 [f1f1 f1f1] f1f1
                  f1f1 f1f1 f1f1         f1f1 [f1f1 ????]
                  f1f1                   f1f1
                  f1f1                   f1f1
 */
static const BYTE patternFixSlowDownLeft[] = {
    // clang-format off
    2, 3,
    PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_DNC,
    PTN_DNC,   PTN_DNC,
    // clang-format on
};
static bool FixSlowDownLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    D1GfxPixel *fP = FixColorCheck(x, y, origPixels, params, patternFixSlowDownLeft);
    if (fP == NULL)
        return false;

    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    if (newPixels[dy + multiplier][dx + multiplier] == *fP || newPixels[dy + 2 * multiplier][dx] == *fP)
        return false;

    int len = 1;
    while (sx > len
        && newPixels[dy][dx - len * multiplier] == *fP && newPixels[dy + multiplier][dx - len * multiplier] == *fP && newPixels[dy + 2 * multiplier][dx - len * multiplier] != *fP) {
        len++;
    }

    if (sx <= len
        || newPixels[dy][dx - len * multiplier] != *fP || newPixels[dy + multiplier][dx - len * multiplier] != *fP || newPixels[dy + 2 * multiplier][dx - len * multiplier] != *fP)
        return false;

    if (len <= PATLEN)
        return false;

    sx -= len;
    dx -= len * multiplier;

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    sx += 1;
    sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < len * multiplier; xx++) {
            if (yy >= multiplier - xx / len) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + yy + multiplier][dx + xx], params);
                bool rightFixed = isPixelFixed(&newPixels[dy + yy][dx + xx + multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + 1][sx + xx / multiplier];
                D1GfxPixel *pRight = &origPixels[sy][sx + xx / multiplier + 1];
                if (rightFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the right is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!rightFixed) {
                        // only the bottom is fixed -> use the right
                        *pDest = *pRight;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pRight;
                    }
                } else {
                    // interpolate
                    if (!pRight->isTransparent()) {
                        if (!pBottom->isTransparent()) {
                            // both are colored
                            BilinearInterpolate2Pixels(pRight, pBottom, params, *pDest);
                        } else {
                            // only right is colored -> use the right
                            *pDest = *pRight;
                        }
                    } else if (!pBottom->isTransparent()) {
                        // only bottom is colored -> use the bottom
                        *pDest = *pBottom;
                    } else {
                        // neither is colored -> make it alpha
                        *pDest = *pBottom;
                    }
                }
            }
        }
    }
    return false;
}

/*
  [f1 f1 f1]   f1f1 f1f1 f1f1    f1f1  f1f1  f1f1
  [f1 f1   ] X f1f1 f1f1 f1f1 -> f1f1  f1f1  f1f1
   f1 f1       f1f1 f1f1         f1f1 [f1f1]
   f1          f1f1 f1f1         f1f1 [f1f1]
               f1f1 f1f1         f1f1 [f1??]
               f1f1 f1f1         f1f1 [f1??]
               f1f1 f1f1         f1f1
               f1f1 f1f1         f1f1
 */
static const BYTE patternFixFastDownLeft[] = {
    // clang-format off
    3, 2,
    PTN_COLOR, PTN_COLOR, PTN_DNC,
    PTN_COLOR, PTN_DNC,   PTN_DNC,
    // clang-format on
};
static bool FixFastDownLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    D1GfxPixel *fP = FixColorCheck(x, y, origPixels, params, patternFixFastDownLeft);
    if (fP == NULL)
        return false;

    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    if (newPixels[dy][dx + 2 * multiplier] == *fP || newPixels[dy + multiplier][dx + multiplier] == *fP)
        return false;

    int len = 1;
    while (sy > len
        && newPixels[dy - len * multiplier][dx] == *fP && newPixels[dy - len * multiplier][dx + multiplier] == *fP && newPixels[dy - len * multiplier][dx + 2 * multiplier] != *fP) {
        len++;
    }

    if (sy <= len
        || newPixels[dy - len * multiplier][dx] != *fP || newPixels[dy - len * multiplier][dx + multiplier] != *fP || newPixels[dy - len * multiplier][dx + 2 * multiplier] != *fP)
        return false;

    if (len <= PATLEN)
        return false;

    sy -= len;
    dy -= len * multiplier;

    // move to [1; 1]
    dx += multiplier;
    dy += multiplier;
    sx += 1;
    sy += 1;

    for (int yy = 0; yy < len * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= len * (multiplier - xx)) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + yy + multiplier][dx + xx], params);
                bool rightFixed = isPixelFixed(&newPixels[dy + yy][dx + xx + multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + yy / multiplier + 1][sx];
                D1GfxPixel *pRight = &origPixels[sy + yy / multiplier][sx + 1];
                if (rightFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the right is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!rightFixed) {
                        // only the bottom is fixed -> use the right
                        *pDest = *pRight;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pRight;
                    }
                } else {
                    // interpolate
                    if (!pRight->isTransparent()) {
                        if (!pBottom->isTransparent()) {
                            // both are colored
                            BilinearInterpolate2Pixels(pRight, pBottom, params, *pDest);
                        } else {
                            // only right is colored -> use the right
                            *pDest = *pRight;
                        }
                    } else if (!pBottom->isTransparent()) {
                        // only bottom is colored -> use the bottom
                        *pDest = *pBottom;
                    } else {
                        // neither is colored -> make it alpha
                        *pDest = *pBottom;
                    }
                }
            }
        }
    }
    return false;
}

/*
  [     ]    f1                  f1f1         [         ] f1f1
  [   f1] f1 f1 X                f1f1 ->      [     F1F1] f1f1
  [f1 f1] f1 f1        f1f1 f1f1 f1f1          f1f1 f1f1  f1f1
                       f1f1 f1f1 f1f1          f1f1 f1f1  f1f1
                  f1f1 f1f1 f1f1 f1f1    f1f1  f1f1 f1f1  f1f1
                  f1f1 f1f1 f1f1 f1f1    f1f1  f1f1 f1f1  f1f1
 */
static const BYTE patternFixSlowUpRight[] = {
    // clang-format off
    2, 3,
    PTN_DNC,   PTN_COLOR,
    PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool FixSlowUpRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    D1GfxPixel *fP = FixColorCheck(x, y, origPixels, params, patternFixSlowUpRight);
    if (fP == NULL)
        return false;

    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    if (newPixels[dy][dx] == *fP)
        return false;

    int len = 1;
    while (sx > len
        && newPixels[dy][dx - len * multiplier] != *fP && newPixels[dy + multiplier][dx - len * multiplier] == *fP && newPixels[dy + 2 * multiplier][dx - len * multiplier] == *fP) {
        len++;
    }

    if (sx <= len
        || newPixels[dy][dx - len * multiplier] == *fP || newPixels[dy + multiplier][dx - len * multiplier] == *fP || newPixels[dy + 2 * multiplier][dx - len * multiplier] != *fP)
        return false;

    if (len <= PATLEN)
        return false;

    sx -= len;
    dx -= len * multiplier;

    // move to [1; 0]
    dx += multiplier;
    sx += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < len * multiplier; xx++) {
            if (yy >= multiplier - xx / len) {
                newPixels[dy + yy][dx + xx] = *fP;
            }
        }
    }
    return false;
}

/*
  [      f1]             f1f1                f1f1
  [   f1 f1] X           f1f1 ->             f1f1
      f1 f1         f1f1 f1f1    [    ] f1f1 f1f1
   f1 f1 f1         f1f1 f1f1    [    ] f1f1 f1f1
                    f1f1 f1f1    [  F1] f1f1 f1f1
                    f1f1 f1f1    [  F1] f1f1 f1f1
               f1f1 f1f1 f1f1     f1f1  f1f1 f1f1
               f1f1 f1f1 f1f1     f1f1  f1f1 f1f1
 */
static const BYTE patternFixFastUpRight[] = {
    // clang-format off
    3, 2,
    PTN_DNC,   PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool FixFastUpRight(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    D1GfxPixel *fP = FixColorCheck(x, y, origPixels, params, patternFixFastUpRight);
    if (fP == NULL)
        return false;

    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    if (newPixels[dy][dx] == *fP)
        return false;

    int len = 1;
    while (sy > len
        && newPixels[dy - len * multiplier][dx] != *fP && newPixels[dy - len * multiplier][dx + multiplier] == *fP && newPixels[dy - len * multiplier][dx + 2 * multiplier] == *fP) {
        len++;
    }

    if (sy <= len
        || newPixels[dy - len * multiplier][dx] == *fP || newPixels[dy - len * multiplier][dx + multiplier] == *fP || newPixels[dy - len * multiplier][dx + 2 * multiplier] != *fP)
        return false;

    if (len <= PATLEN)
        return false;

    sy -= len;
    dy -= len * multiplier;

    // move to [0; 1]
    dy += multiplier;
    sy += 1;

    for (int yy = 0; yy < len * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= len * (multiplier - xx)) {
                newPixels[dy + yy][dx + xx] = *fP;
            }
        }
    }
    return false;
}

/*
  [f1   ]         f1f1                   f1f1 [         ]
  [f1 f1] f1    X f1f1                -> f1f1 [F1F1     ]
  [f1 f1] f1 f1   f1f1 f1f1 f1f1         f1f1  f1f1 f1f1
                  f1f1 f1f1 f1f1         f1f1  f1f1 f1f1
                  f1f1 f1f1 f1f1 f1f1    f1f1  f1f1 f1f1  f1f1
                  f1f1 f1f1 f1f1 f1f1    f1f1  f1f1 f1f1  f1f1
 */
static const BYTE patternFixSlowUpLeft[] = {
    // clang-format off
    2, 3,
    PTN_DNC,   PTN_DNC,
    PTN_COLOR, PTN_DNC,
    PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool FixSlowUpLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    D1GfxPixel *fP = FixColorCheck(x, y, origPixels, params, patternFixSlowUpLeft);
    if (fP == NULL)
        return false;

    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    if (newPixels[dy][dx] == *fP || newPixels[dy + 1 * multiplier][dx + 1 * multiplier] == *fP)
        return false;

    int len = 1;
    while (sx > len
        && newPixels[dy][dx - len * multiplier] != *fP && newPixels[dy + multiplier][dx - len * multiplier] == *fP && newPixels[dy + 2 * multiplier][dx - len * multiplier] == *fP) {
        len++;
    }

    if (sx <= len
        || newPixels[dy][dx - len * multiplier] != *fP || newPixels[dy + multiplier][dx - len * multiplier] != *fP || newPixels[dy + 2 * multiplier][dx - len * multiplier] != *fP)
        return false;

    if (len <= PATLEN)
        return false;

    sx -= len;
    dx -= len * multiplier;

    // move to [1; 0]
    dx += multiplier;
    sx += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < len * multiplier; xx++) {
            if (yy > xx / len) {
                newPixels[dy + yy][dx + xx] = *fP;
            }
        }
    }
    return false;
}

/*
  [f1      ]   f1f1              f1f1
  [f1 f1   ] X f1f1           -> f1f1
   f1 f1       f1f1 f1f1         f1f1 f1f1 [    ]
   f1 f1 f1    f1f1 f1f1         f1f1 f1f1 [    ]
               f1f1 f1f1         f1f1 f1f1 [F1  ]
               f1f1 f1f1         f1f1 f1f1 [F1  ]
               f1f1 f1f1 f1f1    f1f1 f1f1  f1f1
               f1f1 f1f1 f1f1    f1f1 f1f1  f1f1
 */
static const BYTE patternFixFastUpLeft[] = {
    // clang-format off
    3, 2,
    PTN_COLOR, PTN_COLOR, PTN_DNC,
    PTN_COLOR, PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool FixFastUpLeft(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    D1GfxPixel *fP = FixColorCheck(x, y, origPixels, params, patternFixFastUpLeft);
    if (fP == NULL)
        return false;

    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    if (newPixels[dy][dx + 2 * multiplier] == *fP)
        return false;

    int len = 1;
    while (sy > len
        && newPixels[dy - len * multiplier][dx] == *fP && newPixels[dy - len * multiplier][dx + multiplier] == *fP && newPixels[dy - len * multiplier][dx + 2 * multiplier] != *fP) {
        len++;
    }

    if (sy <= len
        || newPixels[dy - len * multiplier][dx] != *fP || newPixels[dy - len * multiplier][dx + multiplier] == *fP || newPixels[dy - len * multiplier][dx + 2 * multiplier] == *fP)
        return false;

    if (len <= PATLEN)
        return false;

    sy -= len;
    dy -= len * multiplier;

    // move to [2; 1]
    dx += 2 * multiplier;
    dy += multiplier;
    sx += 2;
    sy += 1;

    for (int yy = 0; yy < len * multiplier; yy++) {
        for (int xx = 0; xx < multiplier; xx++) {
            if (yy >= len * (xx + 1)) {
                newPixels[dy + yy][dx + xx] = *fP;
            }
        }
    }
    return false;
}

/*
  a a a a   aa aa aa aa     aa  aa  aa aa
  a a c c   aa aa aa aa     aa  aa  aa aa
  c c c c X aa aa cc cc -> [aa  aa] cc cc
  a a c c   aa aa cc cc    [aa  CC] cc cc
  a a a a   cc cc cc cc    [cc  cc] cc cc
            cc cc cc cc    [AA  cc] cc cc
            aa aa cc cc     aa  aa  cc cc
            aa aa cc cc     aa  aa  cc cc
            aa aa aa aa     aa  aa  aa aa
            aa aa aa aa     aa  aa  aa aa
 */
static const BYTE patternLeftTriangle[] = {
    // clang-format off
    4, 5,
    PTN_ALPHA, PTN_ALPHA, PTN_ALPHA, PTN_ALPHA,
    PTN_ALPHA, PTN_ALPHA, PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_ALPHA, PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_ALPHA, PTN_ALPHA, PTN_ALPHA,
    // clang-format on
};
static bool LeftTriangle(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    // not on the left border and [-1; 2] is not alpha
    if (sx != 0 && !origPixels[sy + 2][sx - 1].isTransparent()) {
        return false;
    }

    // move to [0; 1]
    dy += multiplier;
    sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < 2 * multiplier; xx++) {
            if (yy >= multiplier - xx / 2) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + multiplier][dx], params);
                bool rightFixed = isPixelFixed(&newPixels[dy][dx + 2 * multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + 1][sx];
                D1GfxPixel *pRight = &origPixels[sy][sx + 2];
                if (rightFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the right is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!rightFixed) {
                        // only the bottom is fixed -> use the right
                        *pDest = *pRight;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pRight;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pRight, pBottom, params, *pDest);
                }
            }
        }
    }

    // move to [0; 2]
    dy += multiplier;
    sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < 2 * multiplier; xx++) {
            if (yy > xx / 2) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }

    return true;
}

/*
  a a a a   aa aa aa aa    aa aa  aa aa
  c c a a   aa aa aa aa    aa aa  aa aa
  c c c c X cc cc aa aa -> cc cc [aa aa]
  c c a a   cc cc aa aa    cc cc [CC aa]
  a a a a   cc cc cc cc    cc cc [cc cc]
            cc cc cc cc    cc cc [cc AA]
            cc cc aa aa    cc cc  aa aa
            cc cc aa aa    cc cc  aa aa
            aa aa aa aa    aa aa  aa aa
            aa aa aa aa    aa aa  aa aa
 */
static const BYTE patternRightTriangle[] = {
    // clang-format off
    4, 5,
    PTN_ALPHA, PTN_ALPHA, PTN_ALPHA, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR, PTN_ALPHA, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR,
    PTN_COLOR, PTN_COLOR, PTN_ALPHA, PTN_ALPHA,
    PTN_ALPHA, PTN_ALPHA, PTN_ALPHA, PTN_ALPHA,
    // clang-format on
};
static bool RightTriangle(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    // not on the right border and [4; 2] is not alpha
    int origWidth = origPixels[0].count();
    if (sx + 4 != origWidth && !origPixels[sy + 2][sx + 4].isTransparent()) {
        return false;
    }

    // move to [2; 1]
    dx += 2 * multiplier;
    dy += multiplier;
    sx += 2;
    sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < 2 * multiplier; xx++) {
            if (yy > xx / 2) {
                D1GfxPixel *pDest = &newPixels[dy + yy][dx + xx];
                bool bottomFixed = isPixelFixed(&newPixels[dy + multiplier][dx], params);
                bool leftFixed = isPixelFixed(&newPixels[dy][dx - multiplier], params);

                D1GfxPixel *pBottom = &origPixels[sy + 1][sx];
                D1GfxPixel *pLeft = &origPixels[sy][sx - 1];
                if (leftFixed || bottomFixed) {
                    if (!bottomFixed) {
                        // only the left is fixed -> use the bottom
                        *pDest = *pBottom;
                    } else if (!leftFixed) {
                        // only the bottom is fixed -> use the left
                        *pDest = *pLeft;
                    } else {
                        // both colors are fixed -> use one
                        *pDest = *pLeft;
                    }
                } else {
                    // interpolate
                    BilinearInterpolate2Pixels(pLeft, pBottom, params, *pDest);
                }
            }
        }
    }

    // move to [2; 2]
    dy += multiplier;
    sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < 2 * multiplier; xx++) {
            if (yy >= multiplier - xx / 2) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }

    return true;
}

/*
  a a a a a a a a   aa aa aa aa aa aa aa aa    aa aa [aa aa aa aa] aa aa
  a a c c c c a a X aa aa aa aa aa aa aa aa -> aa aa [aa CC CC aa] aa aa
  c c c c c c c c   aa aa cc cc cc cc aa aa    aa aa  cc cc cc cc  aa aa
                    aa aa cc cc cc cc aa aa    aa aa  cc cc cc cc  aa aa
                    cc cc cc cc cc cc cc cc    cc cc  cc cc cc cc  cc cc
                    cc cc cc cc cc cc cc cc    cc cc  cc cc cc cc  cc cc
 */
static const BYTE patternTopTriangle[] = {
    // clang-format off
    8, 3,
    PTN_ALPHA, PTN_ALPHA, PTN_ALPHA, PTN_ALPHA, PTN_ALPHA, PTN_ALPHA, PTN_ALPHA, PTN_ALPHA,
    PTN_ALPHA, PTN_ALPHA, PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_ALPHA, PTN_ALPHA,
    PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR,
    // clang-format on
};
static bool TopTriangle(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    // move to [2; 0]
    dx += 2 * multiplier;
    sx += 2;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < 2 * multiplier; xx++) {
            if (yy >= multiplier - xx / 2) {
                newPixels[dy + yy][dx + xx] = origPixels[sy + 1][sx + (xx >= multiplier ? 1 : 0)];
            }
        }
    }

    // move to [4; 0]
    dx += 2 * multiplier;
    sx += 2;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < 2 * multiplier; xx++) {
            if (yy > xx / 2) {
                newPixels[dy + yy][dx + xx] = origPixels[sy + 1][sx + (xx >= multiplier ? 1 : 0)];
            }
        }
    }
    return true;
}

/*
  c c c c c c c c   cc cc cc cc cc cc cc cc    cc cc  cc cc cc cc  cc cc
  a a c c c c a a X cc cc cc cc cc cc cc cc -> cc cc  cc cc cc cc  cc cc
                    aa aa cc cc cc cc aa aa    aa aa [cc cc cc cc] aa aa
                    aa aa cc cc cc cc aa aa    aa aa [AA cc cc AA] aa aa
 */
static const BYTE patternBottomTriangle[] = {
    // clang-format off
    8, 2,
    PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR,
    PTN_ALPHA, PTN_ALPHA, PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_COLOR, PTN_ALPHA, PTN_ALPHA,
    // clang-format on
};
static bool BottomTriangle(int x, int y, QList<QList<D1GfxPixel>> &origPixels, QList<QList<D1GfxPixel>> &newPixels, const UpscalingParam &params)
{
    const int multiplier = params.multiplier;
    int sx = 0, sy = y;
    int dx = 0, dy = y * multiplier;

    sx += x;
    dx += x * multiplier;

    // not at the bottom border
    int origHeight = origPixels.count();
    if (sy != origHeight - 2) {
        // [1; 2] .. [6; 2] is not alpha
        for (int i = 1; i < 7; i++) {
            if (!origPixels[sy + 2][sx + i].isTransparent())
                return false;
        }
    }

    // move to [2; 1]
    dx += 2 * multiplier;
    dy += multiplier;
    // sx += 2;
    // sy += 1;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < 2 * multiplier; xx++) {
            if (yy > xx / 2) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }

    // move to [4; 1]
    dx += 2 * multiplier;
    // sx += 2;

    for (int yy = 0; yy < multiplier; yy++) {
        for (int xx = 0; xx < 2 * multiplier; xx++) {
            if (yy >= multiplier - xx / 2) {
                newPixels[dy + yy][dx + xx] = D1GfxPixel::transparentPixel();
            }
        }
    }
    return true;
}

bool Upscaler::upscaleFrame(D1GfxFrame *frame, D1Pal *pal, const UpscaleParam &params)
{
    ANTI_ALIASING_MODE antiAliasingMode = params.antiAliasingMode;
    UpscalingParam upParams;
    upParams.multiplier = params.multiplier;
    upParams.firstfixcolor = params.firstfixcolor;
    upParams.lastfixcolor = params.lastfixcolor;
    upParams.pal = pal;
    { // setup dynColors
        QColor undefColor = pal->getUndefinedColor();
        for (int i = 0; i < D1PAL_COLORS; i++) {
            QColor palColor = pal->getColor(i);
            if (palColor != undefColor) {
                upParams.dynColors.append(palColor);
            }
        }
        if (upParams.dynColors.isEmpty()) {
            upParams.dynColors.append(undefColor);
        }
    }

    int multiplier = upParams.multiplier;
    // upscale the frame
    QList<QList<D1GfxPixel>> newPixels;
    for (int y = 0; y < frame->height; y++) {
        QList<D1GfxPixel> pixelLine;
        for (int x = 0; x < frame->width; x++) {
            if (ProgressDialog::wasCanceled()) {
                return false;
            }
            for (int n = 0; n < multiplier; n++) {
                pixelLine.append(frame->pixels[y][x]);
            }
        }
        for (int n = 0; n < multiplier; n++) {
            newPixels.append(pixelLine);
        }
    }
    { // resample the pixels
        int newHeight = frame->height * multiplier;
        int newWidth = frame->width * multiplier;
        int y;
        for (y = 0; y < newHeight - multiplier; y += multiplier) {
            int x;
            for (x = 0; x < newWidth - multiplier; x += multiplier) {
                if (ProgressDialog::wasCanceled()) {
                    return false;
                }
                D1GfxPixel *p0 = &newPixels[y][x];
                if (p0->isTransparent())
                    continue; // skip transparent pixels
                // skip 'protected' colors
                if (p0->getPaletteIndex() >= upParams.firstfixcolor && p0->getPaletteIndex() <= upParams.lastfixcolor)
                    continue;

                D1GfxPixel *pR = &newPixels[y][x + multiplier];
                D1GfxPixel *pD = &newPixels[y + multiplier][x];
                D1GfxPixel *pDR = &newPixels[y + multiplier][x + multiplier];
                for (int j = 0; j < multiplier; j++) {
                    for (int k = 0; k < multiplier; k++) {
                        D1GfxPixel *pp = &newPixels[y + j][x + k];
                        *pp = BilinearInterpolate(pp, pR, k, pD, j, pDR, multiplier, upParams);
                    }
                }
            }
            // resample right column as if the external pixels are transparent
            if (y < newHeight - multiplier) {
                D1GfxPixel *p0 = &newPixels[y][x];
                if (p0->isTransparent())
                    continue; // skip transparent pixels
                if (p0->getPaletteIndex() >= upParams.firstfixcolor && p0->getPaletteIndex() <= upParams.lastfixcolor)
                    continue; // skip 'protected' colors
                D1GfxPixel pDR = D1GfxPixel::transparentPixel();
                D1GfxPixel *pD = &newPixels[y + multiplier][x];
                for (int j = 0; j < multiplier; j++) {
                    for (int k = 0; k < multiplier; k++) {
                        D1GfxPixel *pp = &newPixels[y + j][x + k];
                        *pp = BilinearInterpolate(pp, &pDR, k, pD, j, &pDR, multiplier, upParams);
                    }
                }
            }
        }
        // resample bottom row as if the external pixels are transparent
        for (int x = 0; x < newWidth - multiplier; x += multiplier) {
            D1GfxPixel *p0 = &newPixels[y][x];
            if (p0->isTransparent())
                continue; // skip transparent pixels
            if (p0->getPaletteIndex() >= upParams.firstfixcolor && p0->getPaletteIndex() <= upParams.lastfixcolor)
                continue; // skip 'protected' colors
            D1GfxPixel *pR = &newPixels[y][x + multiplier];
            D1GfxPixel pDR = D1GfxPixel::transparentPixel();
            for (int j = 0; j < multiplier; j++) {
                for (int k = 0; k < multiplier; k++) {
                    D1GfxPixel *pp = &newPixels[y + j][x + k];
                    *pp = BilinearInterpolate(pp, pR, k, &pDR, j, &pDR, multiplier, upParams);
                }
            }
        }
    }

    // apply basic anti-aliasing filters
    if (antiAliasingMode != ANTI_ALIASING_MODE::NONE) {
#if PATLEN == 2
        UpscalePatterns patterns[] = {
            { patternLineDownRight, LineDownRight }, // 0
            { patternLineDownLeft, LineDownLeft },
            { patternLineUpRight, LineUpRight },
            { patternLineUpLeft, LineUpLeft },
            { patternSlowDownRight, SlowDownRight }, // 4
            { patternSlowDownLeft, SlowDownLeft },
            { patternSlowUpRight, SlowUpRight },
            { patternSlowUpLeft, SlowUpLeft },
            { patternFastDownRight, FastDownRight }, // 8
            { patternFastDownLeft, FastDownLeft },
            { patternFastUpRight, FastUpRight },
            { patternFastUpLeft, FastUpLeft },
            { patternLeftTriangle, LeftTriangle }, // 12
            { patternRightTriangle, RightTriangle },
            { patternTopTriangle, TopTriangle },
            { patternBottomTriangle, BottomTriangle },
            /*{ patternAnySlowDownRight, AnySlowDownRight }, // 16
            { patternAnySlowDownLeft, AnySlowDownLeft },
            { patternAnySlowUpRight, AnySlowUpRight },
            { patternAnySlowUpLeft, AnySlowUpLeft },
            { patternAnyFastDownRight, AnyFastDownRight }, // 20
            { patternAnyFastDownLeft, AnyFastDownLeft },
            { patternAnyFastUpRight, AnyFastUpRight },
            { patternAnyFastUpLeft, AnyFastUpLeft },*/
        };
#elif PATLEN == 1
        UpscalePatterns patterns[] = {
            { patternLineDownRight, LineDownRight }, // 0
            { patternLineDownLeft, LineDownLeft },
            { patternLineUpRight, LineUpRight },
            { patternLineUpLeft, LineUpLeft },
            //{ patternSlowDownRight, SlowDownRight },
            //{ patternSlowDownLeft, SlowDownLeft },
            //{ patternSlowUpRight, SlowUpRight },
            //{ patternSlowUpLeft, SlowUpLeft },
            //{ patternFastDownRight, FastDownRight },
            //{ patternFastDownLeft, FastDownLeft },
            //{ patternFastUpRight, FastUpRight },
            //{ patternFastUpLeft, FastUpLeft },
            { patternLeftTriangle, LeftTriangle }, // 4
            { patternRightTriangle, RightTriangle },
            { patternTopTriangle, TopTriangle },
            { patternBottomTriangle, BottomTriangle },
            { patternAnySlowDownRight, AnySlowDownRight }, // 8
            { patternAnySlowDownLeft, AnySlowDownLeft },
            { patternAnySlowUpRight, AnySlowUpRight },
            { patternAnySlowUpLeft, AnySlowUpLeft },
            { patternAnyFastDownRight, AnyFastDownRight }, // 12
            { patternAnyFastDownLeft, AnyFastDownLeft },
            { patternAnyFastUpRight, AnyFastUpRight },
            { patternAnyFastUpLeft, AnyFastUpLeft },
        };
#else
        UpscalePatterns patterns[] = {
            //{ patternLineDownRight, LineDownRight }, // 0
            //{ patternLineDownLeft, LineDownLeft },
            //{ patternLineUpRight, LineUpRight },
            //{ patternLineUpLeft, LineUpLeft },
            //{ patternSlowDownRight, SlowDownRight },
            //{ patternSlowDownLeft, SlowDownLeft },
            //{ patternSlowUpRight, SlowUpRight },
            //{ patternSlowUpLeft, SlowUpLeft },
            //{ patternFastDownRight, FastDownRight },
            //{ patternFastDownLeft, FastDownLeft },
            //{ patternFastUpRight, FastUpRight },
            //{ patternFastUpLeft, FastUpLeft },
            { patternLeftTriangle, LeftTriangle }, // 0
            { patternRightTriangle, RightTriangle },
            { patternTopTriangle, TopTriangle },
            { patternBottomTriangle, BottomTriangle },
            { patternAnySlowDownRight, AnySlowDownRight }, // 4
            { patternAnySlowDownLeft, AnySlowDownLeft },
            { patternAnySlowUpRight, AnySlowUpRight },
            { patternAnySlowUpLeft, AnySlowUpLeft },
            { patternAnyFastDownRight, AnyFastDownRight }, // 8
            { patternAnyFastDownLeft, AnyFastDownLeft },
            { patternAnyFastUpRight, AnyFastUpRight },
            { patternAnyFastUpLeft, AnyFastUpLeft },
            { patternAnySlowDownNarrow, AnySlowDownNarrow }, // 12
            { patternAnySlowUpNarrow, AnySlowUpNarrow },
            { patternAnyFastDownNarrow, AnyFastDownNarrow },
            { patternAnyFastUpNarrow, AnyFastUpNarrow },
            { patternFixSlowDownRight, FixSlowDownRight }, // 16
            { patternFixSlowDownLeft, FixSlowDownLeft },
            { patternFixSlowUpRight, FixSlowUpRight },
            { patternFixSlowUpLeft, FixSlowUpLeft },
            { patternFixFastDownRight, FixFastDownRight }, // 20
            { patternFixFastDownLeft, FixFastDownLeft },
            { patternFixFastUpRight, FixFastUpRight },
            { patternFixFastUpLeft, FixFastUpLeft },
        };
#endif
        for (int y = 0; y < frame->height; y++) {
            for (int x = 0; x < frame->width; x++) {
                if (ProgressDialog::wasCanceled()) {
                    return false;
                }
                for (int k = 0; k < lengthof(patterns); k++) {
                    UpscalePatterns &ptn = patterns[k];
                    BYTE w = ptn.pattern[0];
                    BYTE h = ptn.pattern[1];
                    if (frame->width - x < w)
                        continue; // pattern does not fit to width
                    if (frame->height - y < h)
                        continue; // pattern does not fit to height
                    const BYTE *ptnCol = &ptn.pattern[2];
                    bool match = true;
                    for (int yy = 0; yy < h && match; yy++) {
                        for (int xx = 0; xx < w && match; xx++, ptnCol++) {
                            D1GfxPixel *p = &frame->pixels[y + yy][x + xx];
                            switch (*ptnCol) {
                            case PTN_COLOR:
                                if (p->isTransparent()) {
                                    match = false;
                                }
                                break;
                            case PTN_ALPHA:
                                if (!p->isTransparent()) {
                                    match = false;
                                }
                                break;
                            case PTN_DNC:
                                break;
                            }
                        }
                    }
                    if (!match)
                        continue;
                    if (ptn.fnc(x, y, frame->pixels, newPixels, upParams)) {
                        break;
                    }
                }
            }
        }

        // postprocess min files
        if (antiAliasingMode == ANTI_ALIASING_MODE::TILESET) {
            bool leftFloorTile = true;
            int halfWidth = frame->width / 2;
            for (int y = halfWidth; y < halfWidth; y++) {
                for (int x = 0; x < halfWidth; x++) {
                    if (x >= (y * 2 - halfWidth) && frame->pixels[frame->height - halfWidth + y][x].isTransparent()) {
                        leftFloorTile = false;
                    }
                }
            }
            for (int y = 0; y < halfWidth / 2; y++) {
                for (int x = 0; x < halfWidth; x++) {
                    if (x >= (halfWidth - y * 2) && frame->pixels[frame->height - halfWidth + y][x].isTransparent()) {
                        leftFloorTile = false;
                    }
                }
            }
            if (leftFloorTile) {
                halfWidth *= multiplier;
                for (int y = halfWidth; y < halfWidth; y++) {
                    for (int x = 0; x < halfWidth; x++) {
                        if (x >= (y * 2 - halfWidth)) {
                            int dx = x;
                            int dy = newPixels.count() - halfWidth + y;

                            D1GfxPixel *pDest = &newPixels[dy][dx];
                            if (!pDest->isTransparent())
                                continue;
                            D1GfxPixel *pTop = &newPixels[dy - multiplier][dx];
                            D1GfxPixel *pRight = &newPixels[dy][dx + multiplier];
                            if (pTop->isTransparent()) {
                                *pDest = *pRight;
                                continue;
                            }
                            if (pRight->isTransparent()) {
                                *pDest = *pTop;
                                continue;
                            }
                            bool topFixed = isPixelFixed(pTop, upParams);
                            bool rightFixed = isPixelFixed(pRight, upParams);

                            if (rightFixed || topFixed) {
                                if (!topFixed) {
                                    // only the right is fixed -> use the top
                                    *pDest = *pTop;
                                } else if (!rightFixed) {
                                    // only the top is fixed -> use the right
                                    *pDest = *pRight;
                                } else {
                                    // both colors are fixed -> use one
                                    *pDest = *pRight;
                                }
                            } else {
                                // interpolate
                                BilinearInterpolate2Pixels(pRight, pTop, upParams, *pDest);
                            }
                        }
                    }
                }
                for (int y = 0; y < halfWidth / 2; y++) {
                    for (int x = 0; x < halfWidth; x++) {
                        if (x >= (halfWidth - y * 2)) {
                            int dx = x;
                            int dy = newPixels.count() - halfWidth + y;

                            D1GfxPixel *pDest = &newPixels[dy][dx];
                            if (!pDest->isTransparent())
                                continue;
                            D1GfxPixel *pBottom = &newPixels[dy + multiplier][dx];
                            D1GfxPixel *pRight = &newPixels[dy][dx + multiplier];
                            if (pBottom->isTransparent()) {
                                *pDest = *pRight;
                                continue;
                            }
                            if (pRight->isTransparent()) {
                                *pDest = *pBottom;
                                continue;
                            }
                            bool bottomFixed = isPixelFixed(pBottom, upParams);
                            bool rightFixed = isPixelFixed(&newPixels[dy][dx + multiplier], upParams);

                            if (rightFixed || bottomFixed) {
                                if (!bottomFixed) {
                                    // only the right is fixed -> use the bottom
                                    *pDest = *pBottom;
                                } else if (!rightFixed) {
                                    // only the bottom is fixed -> use the right
                                    *pDest = *pRight;
                                } else {
                                    // both colors are fixed -> use one
                                    *pDest = *pRight;
                                }
                            } else {
                                // interpolate
                                BilinearInterpolate2Pixels(pRight, pBottom, upParams, *pDest);
                            }
                        }
                    }
                }
            }
            bool rightFloorTile = true;
            halfWidth = frame->width / 2;
            for (int y = halfWidth / 2; y < halfWidth; y++) {
                for (int x = 0; x < halfWidth; x++) {
                    if (x < (2 * halfWidth - y * 2) && frame->pixels[frame->height - halfWidth + y][halfWidth + x].isTransparent()) {
                        rightFloorTile = false;
                    }
                }
            }
            for (int y = 0; y < halfWidth / 2; y++) {
                for (int x = 0; x < halfWidth; x++) {
                    if ((x < y * 2) && frame->pixels[frame->height - halfWidth + y][halfWidth + x].isTransparent()) {
                        rightFloorTile = false;
                    }
                }
            }
            if (rightFloorTile) {
                halfWidth *= multiplier;
                for (int y = halfWidth / 2; y < halfWidth; y++) {
                    for (int x = 0; x < halfWidth; x++) {
                        if (x < (2 * halfWidth - y * 2)) {
                            int dx = halfWidth + x;
                            int dy = newPixels.count() - halfWidth + y;

                            D1GfxPixel *pDest = &newPixels[dy][dx];
                            if (!pDest->isTransparent())
                                continue;
                            D1GfxPixel *pTop = &newPixels[dy - multiplier][dx];
                            D1GfxPixel *pLeft = &newPixels[dy][dx - multiplier];
                            if (pTop->isTransparent()) {
                                *pDest = *pLeft;
                                continue;
                            }
                            if (pLeft->isTransparent()) {
                                *pDest = *pTop;
                                continue;
                            }
                            bool topFixed = isPixelFixed(pTop, upParams);
                            bool leftFixed = isPixelFixed(pLeft, upParams);

                            if (leftFixed || topFixed) {
                                if (!topFixed) {
                                    // only the left is fixed -> use the top
                                    *pDest = *pTop;
                                } else if (!leftFixed) {
                                    // only the top is fixed -> use the left
                                    *pDest = *pLeft;
                                } else {
                                    // both colors are fixed -> use one
                                    *pDest = *pLeft;
                                }
                            } else {
                                // interpolate
                                BilinearInterpolate2Pixels(pLeft, pTop, upParams, *pDest);
                            }
                        }
                    }
                }
                for (int y = 0; y < halfWidth / 2; y++) {
                    for (int x = 0; x < halfWidth; x++) {
                        if (x < y * 2) {
                            int dx = halfWidth + x;
                            int dy = newPixels.count() - halfWidth + y;

                            D1GfxPixel *pDest = &newPixels[dy][dx];
                            if (!pDest->isTransparent())
                                continue;
                            D1GfxPixel *pBottom = &newPixels[dy + multiplier][dx];
                            D1GfxPixel *pLeft = &newPixels[dy][dx - multiplier];
                            if (pBottom->isTransparent()) {
                                *pDest = *pLeft;
                                continue;
                            }
                            if (pLeft->isTransparent()) {
                                *pDest = *pBottom;
                                continue;
                            }
                            bool bottomFixed = isPixelFixed(pBottom, upParams);
                            bool leftFixed = isPixelFixed(pLeft, upParams);

                            if (leftFixed || bottomFixed) {
                                if (!bottomFixed) {
                                    // only the left is fixed -> use the top
                                    *pDest = *pBottom;
                                } else if (!leftFixed) {
                                    // only the top is fixed -> use the left
                                    *pDest = *pLeft;
                                } else {
                                    // both colors are fixed -> use one
                                    *pDest = *pLeft;
                                }
                            } else {
                                // interpolate
                                BilinearInterpolate2Pixels(pLeft, pBottom, upParams, *pDest);
                            }
                        }
                    }
                }
            }
        }
    }

    // update the frame
    frame->pixels.swap(newPixels);
    frame->width *= multiplier;
    frame->height *= multiplier;
    return true;
}

bool Upscaler::upscaleGfx(D1Gfx *gfx, const UpscaleParam &params)
{
    int amount = gfx->getFrameCount();

    QList<D1GfxFrame *> newFrames;

    QPair<int, QString> progress;
    progress.first = -1;
    for (int i = 0; i < amount; i++) {
        progress.second = QString(QApplication::tr("Upscaling frame %1.")).arg(i + 1);
        dProgress() << progress;
        if (ProgressDialog::wasCanceled()) {
            qDeleteAll(newFrames);
            return false;
        }

        D1GfxFrame *newFrame = new D1GfxFrame(*gfx->getFrame(i));
        if (upscaleFrame(newFrame, gfx->palette, params))
            newFrames.append(newFrame);
        if (!ProgressDialog::incValue()) {
            qDeleteAll(newFrames);
            return false;
        }
    }
    gfx->frames.swap(newFrames);
    gfx->upscaled = true;
    gfx->modified = true;

    qDeleteAll(newFrames);
    progress.second = QString(QApplication::tr("Upscaled %n frame(s).", "", amount)).arg(amount);
    dProgress() << progress;
    return true;
}

D1GfxFrame *Upscaler::createSubtileFrame(const D1Gfx *gfx, const D1Min *min, int subtileIndex)
{
    D1GfxFrame *subtileFrame = new D1GfxFrame();

    subtileFrame->width = min->getSubtileWidth() * MICRO_WIDTH;
    subtileFrame->height = min->getSubtileHeight() * MICRO_HEIGHT;

    for (int i = 0; i < subtileFrame->height; i++) {
        subtileFrame->pixels.append(QList<D1GfxPixel>());
    }
    int x = 0;
    int y = 0;
    const QList<quint16> &frameReferences = min->getFrameReferences(subtileIndex);
    for (quint16 frameRef : frameReferences) {
        if (frameRef == 0) {
            for (int yy = 0; yy < MICRO_HEIGHT; yy++) {
                for (int xx = 0; xx < MICRO_WIDTH; xx++) {
                    subtileFrame->pixels[y + yy].append(D1GfxPixel::transparentPixel());
                }
            }
        } else {
            D1GfxFrame *microFrame = gfx->getFrame(frameRef - 1);
            for (int yy = 0; yy < MICRO_HEIGHT; yy++) {
                for (int xx = 0; xx < MICRO_WIDTH; xx++) {
                    subtileFrame->pixels[y + yy].append(microFrame->getPixel(xx, yy));
                }
            }
        }
        x += MICRO_WIDTH;
        if (x == subtileFrame->width) {
            x = 0;
            y += MICRO_HEIGHT;
        }
    }
    return subtileFrame;
}

void Upscaler::storeSubtileFrame(const D1GfxFrame *subtileFrame, QList<QList<quint16>> &newFrameReferences, QList<D1GfxFrame *> &newFrames)
{
    QList<quint16> subtileFramesRefs;

    int padding = (8 * (subtileFrame->width / MICRO_WIDTH) / 2 - (subtileFrame->height / MICRO_HEIGHT)) * (subtileFrame->width / MICRO_WIDTH);
    if (padding < 0) {
        if (newFrameReferences.isEmpty())
            dProgressWarn() << QApplication::tr("Subtile height is not supported by the game (Diablo 1/DevilutionX).");
    } else if (padding > 0) {
        if (newFrameReferences.isEmpty())
            dProgressWarn() << QApplication::tr("Empty subtiles are added to match the required height of the game (DevilutionX).");
        for (int i = 0; i < padding; i++) {
            subtileFramesRefs.append(0);
        }
    }

    int x = 0;
    for (int y = 0; y < subtileFrame->height;) {
        bool hasColor = false;
        for (int yy = 0; yy < MICRO_HEIGHT && !hasColor; yy++) {
            for (int xx = 0; xx < MICRO_WIDTH; xx++) {
                if (!subtileFrame->getPixel(x + xx, y + yy).isTransparent()) {
                    hasColor = true;
                    break;
                }
            }
        }
        if (hasColor) {
            newFrames.append(new D1GfxFrame());
            subtileFramesRefs.append(newFrames.count());
            D1GfxFrame *newFrame = newFrames.last();
            newFrame->width = MICRO_WIDTH;
            newFrame->height = MICRO_HEIGHT;
            for (int i = 0; i < MICRO_HEIGHT; i++) {
                newFrame->pixels.append(QList<D1GfxPixel>());
            }
            for (int yy = 0; yy < MICRO_HEIGHT; yy++) {
                for (int xx = 0; xx < MICRO_WIDTH; xx++) {
                    newFrame->pixels[yy].append(subtileFrame->getPixel(x + xx, y + yy));
                }
            }
            LevelTabFrameWidget::selectFrameType(newFrame);
        } else {
            subtileFramesRefs.append(0);
        }

        x += MICRO_WIDTH;
        if (x == subtileFrame->width) {
            x = 0;
            y += MICRO_HEIGHT;
        }
    }
    newFrameReferences.append(subtileFramesRefs);
}

bool Upscaler::upscaleTileset(D1Gfx *gfx, D1Min *min, const UpscaleParam &params)
{
    int amount = min->getSubtileCount();

    QList<D1GfxFrame *> newFrames;
    QList<QList<quint16>> newFrameReferences;

    QPair<int, QString> progress;
    progress.first = -1;
    for (int i = 0; i < amount; i++) {
        progress.second = QString(QApplication::tr("Upscaling subtile %1.")).arg(i + 1);
        dProgress() << progress;
        if (ProgressDialog::wasCanceled()) {
            qDeleteAll(newFrames);
            return false;
        }

        D1GfxFrame *subtileFrame = Upscaler::createSubtileFrame(gfx, min, i);
        if (Upscaler::upscaleFrame(subtileFrame, gfx->palette, params))
            Upscaler::storeSubtileFrame(subtileFrame, newFrameReferences, newFrames);
        delete subtileFrame;
        if (!ProgressDialog::incValue()) {
            qDeleteAll(newFrames);
            return false;
        }
    }
    // update gfx
    gfx->groupFrameIndices.clear();
    if (amount > 0)
        gfx->groupFrameIndices.append(qMakePair(0, amount - 1));
    gfx->frames.swap(newFrames);
    gfx->upscaled = true;
    gfx->modified = true;

    // update min
    min->subtileWidth *= params.multiplier;
    min->subtileHeight *= params.multiplier;
    min->frameReferences.swap(newFrameReferences);
    min->modified = true;

    qDeleteAll(newFrames);
    progress.second = QString(QApplication::tr("Upscaled %n subtile(s).", "", amount)).arg(amount);
    dProgress() << progress;
    return true;
}
