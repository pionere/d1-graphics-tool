#include "d1gfx.h"

#include <QApplication>
#include <QMessageBox>

#include "d1image.h"
#include "d1cel.h"
#include "d1cl2.h"
#include "d1smk.h"
#include "openasdialog.h"
#include "progressdialog.h"
#include "remapdialog.h"

#include "dungeon/enums.h"
#include "dungeon/patchdat.h"

D1GfxPixel D1GfxPixel::transparentPixel()
{
    D1GfxPixel pixel;
    pixel.transparent = true;
    pixel.paletteIndex = 0;
    return pixel;
}

D1GfxPixel D1GfxPixel::colorPixel(quint8 color)
{
    D1GfxPixel pixel;
    pixel.transparent = false;
    pixel.paletteIndex = color;
    return pixel;
}

int D1GfxPixel::countAffectedPixels(const std::vector<std::vector<D1GfxPixel>> &pixelImage, const std::pair<int, int>& colors)
{
    int result = 0;
    for (auto &pixelLine : pixelImage) {
        for (auto &pixel : pixelLine) {
            if ((unsigned)colors.first >= D1PAL_COLORS) {
                if (pixel.isTransparent()) {
                    result++;
                }
            } else {
                if (!pixel.isTransparent() && pixel.getPaletteIndex() >= colors.first && pixel.getPaletteIndex() <= colors.second) {
                    result++;
                }
            }
        }
    }
    return result;
}

bool D1GfxPixel::isTransparent() const
{
    return this->transparent;
}

quint8 D1GfxPixel::getPaletteIndex() const
{
    return this->paletteIndex;
}

QString D1GfxPixel::colorText(D1Pal *pal) const
{
    QString colorTxt;
    if (!this->transparent) {
        quint8 color = this->paletteIndex;
        if (pal == nullptr) {
            colorTxt = QString::number(color);
        } else {
            colorTxt = pal->getColor(color).name();
        }
    }
    return QString("%1;").arg(colorTxt, (pal == nullptr ? 3 : 7));
}

bool operator==(const D1GfxPixel &lhs, const D1GfxPixel &rhs)
{
    return lhs.transparent == rhs.transparent && lhs.paletteIndex == rhs.paletteIndex;
}

bool operator!=(const D1GfxPixel &lhs, const D1GfxPixel &rhs)
{
    return lhs.transparent != rhs.transparent || lhs.paletteIndex != rhs.paletteIndex;
}

QString D1GfxFrame::frameTypeToStr(D1CEL_FRAME_TYPE frameType)
{
    QString result = tr("Unknown");
    switch (frameType) {
    case D1CEL_FRAME_TYPE::Square:            result = tr("Square");             break;
    case D1CEL_FRAME_TYPE::TransparentSquare: result = tr("Transparent square"); break;
    case D1CEL_FRAME_TYPE::LeftTriangle:      result = tr("Left Triangle");      break;
    case D1CEL_FRAME_TYPE::RightTriangle:     result = tr("Right Triangle");     break;
    case D1CEL_FRAME_TYPE::LeftTrapezoid:     result = tr("Left Trapezoid");     break;
    case D1CEL_FRAME_TYPE::RightTrapezoid:    result = tr("Right Trapezoid");    break;
    case D1CEL_FRAME_TYPE::Empty:             result = tr("Empty");              break;
    }
    return result;
}

D1GfxFrame::D1GfxFrame(const D1GfxFrame &o)
{
    this->width = o.width;
    this->height = o.height;
    this->pixels = o.pixels;
    this->frameType = o.frameType;
}

D1GfxFrame::~D1GfxFrame()
{
    delete this->frameAudio;
}

int D1GfxFrame::getWidth() const
{
    return this->width;
}

void D1GfxFrame::setWidth(int width)
{
    this->width = width;
}

int D1GfxFrame::getHeight() const
{
    return this->height;
}

void D1GfxFrame::setHeight(int height)
{
    this->height = height;
}

D1GfxPixel D1GfxFrame::getPixel(int x, int y) const
{
    if (x < 0 || x >= this->width || y < 0 || y >= this->height) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Invalid pixel %1:%2 requested. dimensions: %3:%4").arg(x).arg(y).arg(this->width).arg(this->height));
#endif
        return D1GfxPixel::transparentPixel();
    }

    return this->pixels[y][x];
}

std::vector<std::vector<D1GfxPixel>> &D1GfxFrame::getPixels() const
{
    return const_cast<std::vector<std::vector<D1GfxPixel>> &>(this->pixels);
}

bool D1GfxFrame::setPixel(int x, int y, const D1GfxPixel pixel)
{
    if (x < 0 || x >= this->width || y < 0 || y >= this->height) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Invalid pixel %1:%2 set. dimensions: %3:%4").arg(x).arg(y).arg(this->width).arg(this->height));
#endif
        return false;
    }
    if (this->pixels[y][x] == pixel) {
        return false;
    }

    this->pixels[y][x] = pixel;
    return true;
}

D1CEL_FRAME_TYPE D1GfxFrame::getFrameType() const
{
    return this->frameType;
}

void D1GfxFrame::setFrameType(D1CEL_FRAME_TYPE type)
{
    this->frameType = type;
}

bool D1GfxFrame::addTo(const D1GfxFrame &frame)
{
    if (this->width != frame.width || this->height != frame.height) {
        dProgressFail() << QApplication::tr("Mismatching framesizes.");
        return false;
    }

    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            D1GfxPixel d1pix = frame.pixels[y][x];

            if (!d1pix.isTransparent()) {
                this->pixels[y][x] = d1pix;
            }
        }
    }
    return true;
}

void D1GfxFrame::addPixelLine(std::vector<D1GfxPixel> &&pixelLine)
{
    /* if (this->width != pixelLine.size()) {
        if (this->width != 0) {
            dProgressErr() << QString("Mismatching lines.");
        }*/
    if (this->width == 0) {
        this->width = pixelLine.size();
    }
    this->pixels.push_back(pixelLine);
    this->height++;
}

bool D1GfxFrame::replacePixels(const QList<QPair<D1GfxPixel, D1GfxPixel>> &replacements)
{
    bool result = false;
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            D1GfxPixel d1pix = this->pixels[y][x]; // this->getPixel(x, y);

            for (const QPair<D1GfxPixel, D1GfxPixel> &replacement : replacements) {
                if (d1pix == replacement.first) {
                    this->pixels[y][x] = replacement.second;
                    result = true;
                }
            }
        }
    }
    return result;
}

bool D1GfxFrame::mask(const D1GfxFrame *frame)
{
    bool result = false;
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            D1GfxPixel pixelA = this->pixels[y][x]; // this->getPixel(x, y);
            if (pixelA.isTransparent()) continue;
            D1GfxPixel pixelB = frame->pixels[y][x]; // frame->getPixel(x, y);
            if (pixelB.isTransparent() || pixelA.getPaletteIndex() != pixelB.getPaletteIndex()) {
                this->pixels[y][x] = D1GfxPixel::transparentPixel(); // this->setPixel(x, y, D1GfxPixel::transparentPixel());
                result = true;
            }
        }
    }
    return result;
}

bool D1GfxFrame::optimize(D1CEL_TYPE type)
{
    bool result = false;
    switch (type) {
    case D1CEL_TYPE::V1_REGULAR:
    case D1CEL_TYPE::V1_COMPILATION:
    {
        for (int y = 0; y < this->height; y++) {
            QList<QPair<int, int>> gaps;
            int sx = 0;
            for (int x = 0; x < this->width; x++) {
                D1GfxPixel d1pix = this->pixels[y][x]; // this->getPixel(x, y);
                if (!d1pix.isTransparent() && d1pix.getPaletteIndex() != 0) {
                    if (sx < x) {
                        // dProgress() << QApplication::tr("gap %1 len %2").arg(sx).arg(x - sx);
                        gaps.push_back(QPair<int, int>(sx, x - sx));
                    }
                    sx = x + 1;
                }
            }
            if (sx != this->width) {
                // dProgress() << QApplication::tr("gap %1 len %2").arg(sx).arg(this->width - sx);
                gaps.push_back(QPair<int, int>(sx, this->width - sx));
            }

            for (auto it = gaps.begin(); it != gaps.end(); ) {
                bool shortgap = it->second <= 2;
                if (shortgap) {
                    int drawlen = it->second;
                    int units = 0;
                    if (it != gaps.begin()) {
                        auto pit = it - 1;
                        int colorlen = it->first - (pit->first + pit->second);
                        drawlen += colorlen;
                        units += (colorlen - 1) / 0x7F + 1;
                    }
                    auto pit = it + 1;
                    if (pit != gaps.end()) {
                        int colorlen = pit->first - (it->first + it->second);
                        drawlen += colorlen;
                        units += (colorlen - 1) / 0x7F + 1;
                    }
                    int newunits = (drawlen - 1) / 0x7F + 1;
                    shortgap = newunits <= units; // drawlen <= 0x7F;
                }
                if (shortgap) {
                    // dProgress() << QApplication::tr("short gap %1 len %2").arg(it->first).arg(it->second);
                    for (int x = it->first; x < it->first + it->second; x++) {
                        result |= this->setPixel(x, y, D1GfxPixel::colorPixel(0));
                    }
                    it = gaps.erase(it);
                } else {
                    it++;
                }
            }
            for (auto it = gaps.begin(); it != gaps.end(); ) {
                int gaplen = it->second;
                bool shortgap = gaplen <= 4;
                if (shortgap) {
                    int drawlen = gaplen;
                    int units = 0;
                    if (it != gaps.begin()) {
                        auto pit = it - 1;
                        int colorlen = it->first - (pit->first + pit->second);
                        drawlen += colorlen;
                        units += (colorlen - 1) / 0x7F + 1;
                    }
                    auto pit = it + 1;
                    if (pit != gaps.end()) {
                        int colorlen = pit->first - (it->first + gaplen);
                        drawlen += colorlen;
                        units += (colorlen - 1) / 0x7F + 1;
                    }
                    int newunits = (drawlen - 1) / 0x7F + 1;
                    shortgap = drawlen >= gaplen * 4 && newunits < units; // drawlen <= 0x7F;
                }
                if (shortgap) {
                    // dProgress() << QApplication::tr("short gap %1 len %2").arg(it->first).arg(it->second);
                    for (int x = it->first; x < it->first + it->second; x++) {
                        result |= this->setPixel(x, y, D1GfxPixel::colorPixel(0));
                    }
                    it = gaps.erase(it);
                } else {
                    it++;
                }
            }
            for (auto it = gaps.begin(); it != gaps.end(); ) {
                int gaplen = it->second;
                bool shortgap = gaplen <= 8;
                if (shortgap) {
                    int drawlen = gaplen;
                    int units = 0;
                    if (it != gaps.begin()) {
                        auto pit = it - 1;
                        int colorlen = it->first - (pit->first + pit->second);
                        drawlen += colorlen;
                        units += (colorlen - 1) / 0x7F + 1;
                    }
                    auto pit = it + 1;
                    if (pit != gaps.end()) {
                        int colorlen = pit->first - (it->first + gaplen);
                        drawlen += colorlen;
                        units += (colorlen - 1) / 0x7F + 1;
                    }
                    int newunits = (drawlen - 1) / 0x7F + 1;
                    shortgap = drawlen >= gaplen * 4 && newunits < units; // drawlen <= 0x7F;
                }
                if (shortgap) {
                    // dProgress() << QApplication::tr("short gap %1 len %2").arg(it->first).arg(it->second);
                    for (int x = it->first; x < it->first + it->second; x++) {
                        result |= this->setPixel(x, y, D1GfxPixel::colorPixel(0));
                    }
                    it = gaps.erase(it);
                } else {
                    it++;
                }
            }
            for (auto it = gaps.begin(); it != gaps.end(); it++) {
                // dProgress() << QApplication::tr("long gap %1 len %2").arg(it->first).arg(it->second);
                for (int x = it->first; x < it->first + it->second; x++) {
                    result |= this->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
    } break;
    case D1CEL_TYPE::V1_LEVEL:
    case D1CEL_TYPE::V2_MONO_GROUP:
    case D1CEL_TYPE::V2_MULTIPLE_GROUPS:
    case D1CEL_TYPE::SMK:
        break;
    }
    return result;
}

QPointer<D1Pal>& D1GfxFrame::getFramePal()
{
    return this->framePal;
}

void D1GfxFrame::setFramePal(D1Pal *pal)
{
    this->framePal = pal;
}

D1SmkAudioData *D1GfxFrame::getFrameAudio()
{
    return this->frameAudio;
}

D1Gfx::~D1Gfx()
{
    this->clear();
}

void D1Gfx::clear()
{
    qDeleteAll(this->frames);
    this->frames.clear();
    this->groupFrameIndices.clear();
    // this->type ?
    // this->palette = nullptr;
    // this->upscaled ?
    this->modified = true;
}

static void reportDiff(const QString text, QString &header)
{
    if (!header.isEmpty()) {
        dProgress() << header;
        header.clear();
    }
    dProgress() << text;
}

static QString celTypeToStr(D1CEL_TYPE type)
{
    QString result = QApplication::tr("Unknown");
    switch (type) {
    case D1CEL_TYPE::V1_REGULAR:         result = QApplication::tr("regular (v1)");     break;
    case D1CEL_TYPE::V1_COMPILATION:     result = QApplication::tr("compilation (v1)"); break;
    case D1CEL_TYPE::V1_LEVEL:           result = QApplication::tr("level (v1)");       break;
    case D1CEL_TYPE::V2_MONO_GROUP:      result = QApplication::tr("mono group (v2)");  break;
    case D1CEL_TYPE::V2_MULTIPLE_GROUPS: result = QApplication::tr("multi group (v2)"); break;
    case D1CEL_TYPE::SMK:                result = QApplication::tr("smacker file");     break;
    }
    return result;
}

static int getFrameDiff(const D1GfxFrame *frameA, const D1GfxFrame *frameB)
{
    int width = frameB->getWidth();
    int myWidth = frameA->getWidth();
    int height = frameB->getHeight();
    int myHeight = frameA->getHeight();
    int result = 0;
    if (myWidth == width && myHeight == height) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                D1GfxPixel pixelA = frameA->getPixel(x, y);
                D1GfxPixel pixelB = frameB->getPixel(x, y);
                if (pixelA != pixelB) {
                    result++;
                }
            }
        }
    } else {
        result = INT_MAX;
    }
    return result;
}

static void reportFrameDiff(int i, int j, const D1GfxFrame *frameA, const D1GfxFrame *frameB, QString &header, bool patchData)
{
    int width = frameA->getWidth();
    int height = frameA->getHeight();
    // assert(frameB->getWidth() == width && frameB->getHeight() == height);
    bool firstInFrame = true;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            D1GfxPixel pixelA = frameA->getPixel(x, y);
            D1GfxPixel pixelB = frameB->getPixel(x, y);
            if (pixelA == pixelB) continue;
            if (patchData) {
                reportDiff(QString("{ %1, %2, %3, %4 },").arg(i + 1, 3).arg(x, 3).arg(y, 3).arg(pixelA.isTransparent() ? 256 : pixelA.getPaletteIndex(), 3), header);
            } else {
                if (firstInFrame) {
                    firstInFrame = false;
                    if (i == j)
                        reportDiff(QApplication::tr("Frame %1:").arg(i + 1), header);
                    else
                        reportDiff(QApplication::tr("Frame %1 (compared to %2):").arg(i + 1).arg(j + 1), header);
                }
                reportDiff(QApplication::tr("  pixel %1:%2 is %3 (was %4)").arg(x).arg(y)
                    .arg(pixelA.isTransparent() ? QApplication::tr("transparent") : QApplication::tr("color%1").arg(pixelA.getPaletteIndex()))
                    .arg(pixelB.isTransparent() ? QApplication::tr("transparent") : QApplication::tr("color%1").arg(pixelB.getPaletteIndex())), header);
            }
        }
    }
}

QString D1Gfx::clippedtoStr(bool clipped)
{
    return clipped ? QApplication::tr("clipped") : QApplication::tr("not clipped");
}

void D1Gfx::compareTo(const D1Gfx *gfx, QString &header, bool patchData) const
{
    if (gfx->type != this->type) {
        reportDiff(QApplication::tr("type is %1 (was %2)").arg(celTypeToStr(this->type)).arg(celTypeToStr(gfx->type)), header);
    }
    if (gfx->clipped != this->clipped) {
        reportDiff(QApplication::tr("%1 (was %2)").arg(D1Gfx::clippedtoStr(this->clipped)).arg(D1Gfx::clippedtoStr(gfx->clipped)), header);
    }
    size_t groupCount = gfx->groupFrameIndices.size();
    size_t myGroupCount = this->groupFrameIndices.size();
    if (groupCount == myGroupCount) {
        for (size_t i = 0; i < groupCount; i++) {
            if (this->groupFrameIndices[i].first != gfx->groupFrameIndices[i].first || 
                this->groupFrameIndices[i].second != gfx->groupFrameIndices[i].second) {
                reportDiff(QApplication::tr("group %1 is frames %2..%3 (was %4..%5)").arg(i + 1)
                    .arg(this->groupFrameIndices[i].first + 1).arg(this->groupFrameIndices[i].second + 1)
                    .arg(gfx->groupFrameIndices[i].first + 1).arg(gfx->groupFrameIndices[i].second + 1), header);
            }
        }
    } else {
        reportDiff(QApplication::tr("groupcount is %1 (was %2)").arg(myGroupCount).arg(groupCount), header);
    }
    int frameCount = gfx->getFrameCount();
    int myFrameCount = this->getFrameCount();
    if ((groupCount != 1 || groupCount != myGroupCount) && frameCount != myFrameCount) {
        reportDiff(QApplication::tr("framecount is %1 (was %2)").arg(myFrameCount).arg(frameCount), header);
    }
    /*if (frameCount == myFrameCount) {
        for (int i = 0; i < frameCount; i++) {
            D1GfxFrame *frameA = this->frames[i];
            D1GfxFrame *frameB = gfx->frames[i];
            int diff = getFrameDiff(frameA, frameB);
            if (diff == 0) continue;
            if (diff != INT_MAX) {
                reportFrameDiff(i, i, frameA, frameB, header, patchData);
            } else {
                reportDiff(QApplication::tr("frame %1 is %2x%3 pixel (was %4x%5)").arg(i + 1)
                    .arg(frameA->getWidth()).arg(frameA->getHeight())
                    .arg(frameB->getWidth()).arg(frameB->getHeight()), header);
            }
        }
    } else*/ {
        int i, j;
        for (i = 0, j = 0; i < myFrameCount && j < frameCount; ) {
            D1GfxFrame *frameA = this->frames[i];
            D1GfxFrame *frameB = gfx->frames[j];
            int diff = getFrameDiff(frameA, frameB);
            if (diff == 0) {
                i++;
                j++;
                continue;
            }

            int bestDiff = diff;
            int bestDist = 0;
            int bestOption = 0;
            for (int n = j + 1; n < frameCount; n++) {
                D1GfxFrame *frameB_ = gfx->frames[n];
                diff = getFrameDiff(frameA, frameB_);
                if (diff < bestDiff) {
                    bestDiff = diff;
                    bestOption = n;
                    bestDist = n - j;
                    if (bestDiff == 0)
                        break;
                }
            }
            if (bestDiff != 0) {
                for (int n = i + 1; n < myFrameCount; n++) {
                    D1GfxFrame *frameA_ = this->frames[n];
                    int dist = n - i;
                    diff = getFrameDiff(frameA_, frameB);
                    if (diff < bestDiff || (diff == bestDiff && (dist < bestDist))) {
                        bestDiff = diff;
                        bestOption = -n;
                        bestDist = dist;
                        if (bestDiff == 0)
                            break;
                    }
                }
            }

            if (bestDiff == 0) {
                // matching frame found
                // if (bestDist != 0) {
                    if (bestOption >= 0) {
                        // frame A matching later
                        reportDiff(QApplication::tr("%n frame deleted [%1..%2)", "", bestDist).arg(j + 1).arg(bestOption + 1), header);
                        j += bestDist;
                    } else {
                        // frame B matching later
                        reportDiff(QApplication::tr("%n frame added [%1..%2)", "", bestDist).arg(i + 1).arg((-bestOption) + 1), header);
                        i += bestDist;
                    }
                // }
                i++;
                j++;
                continue;
            }
            if (bestDiff == INT_MAX) {
                QString msg;
                if (i == j)
                    msg = QApplication::tr("frame %1 is %2x%3 pixel (was %4x%5)").arg(i + 1);
                else
                    msg = QApplication::tr("frame %1 (compared to %2) is %3x%4 pixel (was %5x%6)").arg(i + 1).arg(j + 1);
                reportDiff(msg
                    .arg(frameA->getWidth()).arg(frameA->getHeight())
                    .arg(frameB->getWidth()).arg(frameB->getHeight()), header);
            } else {
                reportFrameDiff(i, j, frameA, frameB, header, patchData);
            }
            i++;
            j++;
        }

        int remFrames = myFrameCount - i;
        if (remFrames) {
            reportDiff(QApplication::tr("%n frame added [%1..%2)", "", remFrames).arg(i + 1).arg(myFrameCount), header);
        }
        remFrames = frameCount - j;
        if (remFrames) {
            reportDiff(QApplication::tr("%n frame deleted [%1..%2)", "", remFrames).arg(j + 1).arg(frameCount), header);
        }
    }
}

QRect D1Gfx::getBoundary() const
{
    int minx = INT_MAX, maxx = INT_MIN, miny = INT_MAX, maxy = INT_MIN;
    if (this->frames.isEmpty()) {
        return QRect();
    }
    for (const D1GfxFrame *frame : this->frames) {
        for (int y = 0; y < frame->getHeight(); y++) {
            for (int x = 0; x < frame->getWidth(); x++) {
                if (frame->getPixel(x, y).isTransparent()) continue;
                if (x < minx)
                    minx = x;
                if (x > maxx)
                    maxx = x;
                if (y < miny)
                    miny = y;
                if (y > maxy)
                    maxy = y;
            }
        }
    }
    return QRect(minx, miny, maxx - minx + 1, maxy - miny + 1);
}

bool D1Gfx::isFrameSizeConstant() const
{
    if (this->frames.isEmpty()) {
        return false;
    }

    int frameWidth = this->frames[0]->getWidth();
    int frameHeight = this->frames[0]->getHeight();

    for (int i = 1; i < this->frames.count(); i++) {
        if (this->frames[i]->getWidth() != frameWidth
            || this->frames[i]->getHeight() != frameHeight)
            return false;
    }

    return true;
}

bool D1Gfx::isGroupSizeConstant() const
{
    if (this->frames.isEmpty()) {
        return false;
    }

    int groupSize = -1;
    for (auto git = this->groupFrameIndices.begin(); git != this->groupFrameIndices.end(); git++) {
        int gs = git->second - git->first + 1;
        if (groupSize != gs) {
            if (groupSize >= 0) {
                return false;
            }
            groupSize = gs;
        }
    }
    return true;
}

// builds QString from a D1CelFrame of given index
QString D1Gfx::getFramePixels(int frameIndex, bool values) const
{
    if (frameIndex < 0 || frameIndex >= this->frames.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Image of an invalid frame %1 requested. Frame count: %2, palette: %3").arg(frameIndex).arg(this->frames.count()).arg(this->palette != nullptr));
#endif
        return QString();
    }

    QString pixels;
    D1GfxFrame *frame = this->frames[frameIndex];
    D1Pal *pal = values ? nullptr : this->palette;
    for (int y = 0; y < frame->getHeight(); y++) {
        for (int x = 0; x < frame->getWidth(); x++) {
            D1GfxPixel d1pix = frame->getPixel(x, y);

            QString colorTxt = d1pix.colorText(pal);
            pixels.append(colorTxt);
        }
        pixels.append('\n');
    }
    pixels = pixels.trimmed();
    return pixels;
}

// builds QImage from a D1CelFrame of given index
QImage D1Gfx::getFrameImage(int frameIndex) const
{
    if (this->palette == nullptr || frameIndex < 0 || frameIndex >= this->frames.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Image of an invalid frame %1 requested. Frame count: %2, palette: %3").arg(frameIndex).arg(this->frames.count()).arg(this->palette != nullptr));
#endif
        return QImage();
    }

    D1GfxFrame *frame = this->frames[frameIndex];

    QImage image = QImage(frame->getWidth(), frame->getHeight(), QImage::Format_ARGB32_Premultiplied); // QImage::Format_ARGB32
    QRgb *destBits = reinterpret_cast<QRgb *>(image.bits());
    for (int y = 0; y < frame->getHeight(); y++) {
        for (int x = 0; x < frame->getWidth(); x++, destBits++) {
            D1GfxPixel d1pix = frame->getPixel(x, y);

            QColor color;
            if (d1pix.isTransparent())
                color = QColor(Qt::transparent);
            else
                color = this->palette->getColor(d1pix.getPaletteIndex());

            // image.setPixelColor(x, y, color);
            *destBits = color.rgba();
        }
    }

    return image;
}

std::vector<std::vector<D1GfxPixel>> D1Gfx::getFramePixelImage(int frameIndex) const
{
    D1GfxFrame *frame = this->frames[frameIndex];
    return frame->getPixels();
}

void D1Gfx::insertFrame(int idx, int width, int height)
{
    D1GfxFrame *frame = this->insertFrame(idx);

    for (int y = 0; y < height; y++) {
        std::vector<D1GfxPixel> pixelLine;
        for (int x = 0; x < width; x++) {
            pixelLine.push_back(D1GfxPixel::transparentPixel());
        }
        frame->addPixelLine(std::move(pixelLine));
    }
}

D1GfxFrame *D1Gfx::insertFrame(int idx)
{
    D1GfxFrame* newFrame = new D1GfxFrame();
    this->frames.insert(idx, newFrame);

    if (this->groupFrameIndices.empty()) {
        // create new group if this is the first frame
        this->groupFrameIndices.push_back(std::pair<int, int>(0, 0));
    } else if (this->frames.count() == idx + 1) {
        // extend the last group if appending a frame
        this->groupFrameIndices.back().second = idx;
    } else {
        // extend the current group and adjust every group after it
        for (unsigned i = 0; i < this->groupFrameIndices.size(); i++) {
            if (this->groupFrameIndices[i].second < idx)
                continue;
            if (this->groupFrameIndices[i].first > idx) {
                this->groupFrameIndices[i].first++;
            }
            this->groupFrameIndices[i].second++;
        }
    }

    this->modified = true;
    return newFrame;
}

D1GfxFrame *D1Gfx::insertFrame(int idx, const QString &pixels)
{
    D1GfxFrame *frame = this->insertFrame(idx);
    D1ImageFrame::load(*frame, pixels, this->palette);
    // this->modified = true;

    return this->frames[idx];
}

D1GfxFrame *D1Gfx::insertFrame(int idx, const QImage &image)
{
    D1GfxFrame *frame = this->insertFrame(idx);
    D1ImageFrame::load(*frame, image, this->palette);
    // this->modified = true;

    return this->frames[idx];
}

D1GfxFrame *D1Gfx::addToFrame(int idx, const D1GfxFrame &frame)
{
    if (this->frames.count() <= idx) {
        // assert(idx == this->frames.count());
        this->insertFrame(idx, frame.width, frame.height);
    }
    // assert(this->frames.count() > idx);
    if (!this->frames[idx]->addTo(frame)) {
        return nullptr;
    }
    this->modified = true;

    return this->frames[idx];
}

D1GfxFrame *D1Gfx::addToFrame(int idx, const QImage &image)
{
    D1GfxFrame frame;
    D1ImageFrame::load(frame, image, this->palette);

    return this->addToFrame(idx, frame);
}

D1GfxFrame *D1Gfx::replaceFrame(int idx, const QString &pixels)
{
    D1GfxFrame *frame = new D1GfxFrame();
    D1ImageFrame::load(*frame, pixels, this->palette);
    this->setFrame(idx, frame);

    return this->frames[idx];
}

D1GfxFrame *D1Gfx::replaceFrame(int idx, const QImage &image)
{
    D1GfxFrame *frame = new D1GfxFrame();
    D1ImageFrame::load(*frame, image, this->palette);
    this->setFrame(idx, frame);

    return this->frames[idx];
}

int D1Gfx::duplicateFrame(int idx, bool wholeGroup)
{
    const bool multiGroup = this->type == D1CEL_TYPE::V1_COMPILATION || this->type == D1CEL_TYPE::V2_MULTIPLE_GROUPS;
    int firstIdx, lastIdx, resIdx;
    if (wholeGroup && multiGroup) {
        unsigned i = 0;
        for ( ; i < this->groupFrameIndices.size(); i++) {
            if (/*this->groupFrameIndices[i].first <= idx &&*/ this->groupFrameIndices[i].second >= idx) {
                break;
            }
        }
        for (int n = this->groupFrameIndices[i].first; n <= this->groupFrameIndices[i].second; n++) {
            D1GfxFrame *frame = this->frames[n];
            frame = new D1GfxFrame(*frame);
            this->frames.push_back(frame);
        }
        firstIdx = this->groupFrameIndices.back().second + 1;
        resIdx = firstIdx + idx - this->groupFrameIndices[i].first;
        lastIdx = this->frames.count() - 1;
    } else {
        D1GfxFrame *frame = this->frames[idx];
        frame = new D1GfxFrame(*frame);
        this->frames.push_back(frame);

        firstIdx = lastIdx = resIdx = this->frames.count() - 1;
    }
    if (multiGroup) {
        this->groupFrameIndices.push_back(std::pair<int, int>(firstIdx, lastIdx));
    } else {
        if (this->groupFrameIndices.empty()) {
            this->groupFrameIndices.push_back(std::pair<int, int>(resIdx, resIdx));
        } else {
            this->groupFrameIndices.back().second = resIdx;
        }
    }

    return resIdx;
}

void D1Gfx::removeFrame(int idx, bool wholeGroup)
{
    const bool multiGroup = this->type == D1CEL_TYPE::V1_COMPILATION || this->type == D1CEL_TYPE::V2_MULTIPLE_GROUPS;
    if (wholeGroup && multiGroup) {
        for (unsigned i = 0; i < this->groupFrameIndices.size(); i++) {
            if (this->groupFrameIndices[i].second < idx)
                continue;
            if (this->groupFrameIndices[i].first <= idx) {
                idx = this->groupFrameIndices[i].first;
                for (int n = idx; n <= this->groupFrameIndices[i].second; n++) {
                    delete this->frames[idx];
                    this->frames.removeAt(idx);
                }
                this->groupFrameIndices.erase(this->groupFrameIndices.begin() + i);
                i--;
                continue;
            }
            int lastIdx = idx + this->groupFrameIndices[i].second - this->groupFrameIndices[i].first;
            this->groupFrameIndices[i].first = idx;
            this->groupFrameIndices[i].second = lastIdx;
            idx = lastIdx + 1;
        }
    } else {
        delete this->frames[idx];
        this->frames.removeAt(idx);

        for (unsigned i = 0; i < this->groupFrameIndices.size(); i++) {
            if (this->groupFrameIndices[i].second < idx)
                continue;
            if (this->groupFrameIndices[i].second == idx && this->groupFrameIndices[i].first == idx) {
                this->groupFrameIndices.erase(this->groupFrameIndices.begin() + i);
                i--;
                continue;
            }
            if (this->groupFrameIndices[i].first > idx) {
                this->groupFrameIndices[i].first--;
            }
            this->groupFrameIndices[i].second--;
        }
    }
    this->modified = true;
}

void D1Gfx::remapFrames(const std::map<unsigned, unsigned> &remap)
{
    QList<D1GfxFrame *> newFrames;
    // assert(this->groupFrameIndices.size() == 1);
    for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
        newFrames.append(this->frames.at(iter->second - 1));
    }
    this->frames.swap(newFrames);
    this->modified = true;
}

void D1Gfx::swapFrames(unsigned frameIndex0, unsigned frameIndex1)
{
    const unsigned numFrames = this->frames.count();
    if (frameIndex0 >= numFrames) {
        // move frameIndex1 to the front
        if (frameIndex1 == 0 || frameIndex1 >= numFrames) {
            return;
        }
        D1GfxFrame *tmp = this->frames.takeAt(frameIndex1);
        this->frames.push_front(tmp);
    } else if (frameIndex1 >= numFrames) {
        // move frameIndex0 to the end
        if (frameIndex0 == numFrames - 1) {
            return;
        }
        D1GfxFrame *tmp = this->frames.takeAt(frameIndex0);
        this->frames.push_back(tmp);
    } else {
        // swap frameIndex0 and frameIndex1
        if (frameIndex0 == frameIndex1) {
            return;
        }
        D1GfxFrame *tmp = this->frames[frameIndex0];
        this->frames[frameIndex0] = this->frames[frameIndex1];
        this->frames[frameIndex1] = tmp;
    }
    this->modified = true;
}

void D1Gfx::mergeFrames(unsigned frameIndex0, unsigned frameIndex1)
{
    // assert(frameIndex0 >= 0 && frameIndex0 < frameIndex1 && frameIndex1 < getFrameCount());
    for (unsigned frameIdx = frameIndex0 + 1; frameIdx <= frameIndex1; frameIdx++) {
        D1GfxFrame* currFrame = getFrame(frameIndex0 + 1);
        addToFrame(frameIndex0, *currFrame);
        removeFrame(frameIndex0 + 1, false);
    }
}

void D1Gfx::addGfx(D1Gfx *gfx)
{
    int numNewFrames = gfx->getFrameCount();
    if (numNewFrames == 0) {
        return;
    }
    for (int i = 0; i < numNewFrames; i++) {
        const D1GfxFrame* frame = gfx->getFrame(i);
        D1GfxFrame* newFrame = new D1GfxFrame(*frame);
        // if (this->type != D1CEL_TYPE::V1_LEVEL) {
        //    newFrame->frameType = D1CEL_FRAME_TYPE::TransparentSquare;
        // }
        this->frames.append(newFrame);
    }
    const bool multiGroup = this->type == D1CEL_TYPE::V1_COMPILATION || this->type == D1CEL_TYPE::V2_MULTIPLE_GROUPS;
    if (multiGroup) {
        int lastFrameIdx = 0;
        if (!this->groupFrameIndices.empty()) {
            lastFrameIdx = this->groupFrameIndices.back().second;
        }        
        for (auto git = gfx->groupFrameIndices.begin(); git != gfx->groupFrameIndices.end(); git++) {
            this->groupFrameIndices.push_back(std::pair<int, int>(git->first + lastFrameIdx, git->second + lastFrameIdx));
        }
    } else {
        if (this->groupFrameIndices.empty()) {
            this->groupFrameIndices.push_back(std::pair<int, int>(0, numNewFrames - 1));
        } else {
            this->groupFrameIndices.back().second += numNewFrames;
        }
    }
    this->modified = true;
}

void D1Gfx::replacePixels(const QList<QPair<D1GfxPixel, D1GfxPixel>> &replacements, const RemapParam &params, int verbose)
{
    int rangeFrom = params.frames.first;
    if (rangeFrom != 0) {
        rangeFrom--;
    }
    int rangeTo = params.frames.second;
    if (rangeTo == 0 || rangeTo >= this->getFrameCount()) {
        rangeTo = this->getFrameCount();
    }
    rangeTo--;

    if (verbose != 0) {
        QString msg = tr("Replacing ");
        for (const QPair<D1GfxPixel, D1GfxPixel> &replacement : replacements) {
            msg.append(tr(" color %1 with %2,").arg(replacement.first.getPaletteIndex()).arg(replacement.second.getPaletteIndex()));
        }
        msg.chop(1);
        bool ofTxt = false;
        if (rangeFrom != 0 || rangeTo != this->getFrameCount()) {
            msg.append(tr(" in frame(s) %1-%2", "", rangeTo - rangeFrom + 1).arg(rangeFrom + 1).arg(rangeTo + 1));
            ofTxt = true;
        }
        if (verbose != 1) {
            QFileInfo fileInfo(this->gfxFilePath);
            QString labelText = ofTxt ? tr(" of %1") : tr(" in %1");
            msg = msg.append(labelText.arg(fileInfo.fileName()));
        }
        msg.append(".");
        dProgress() << msg;
    }

    for (int i = rangeFrom; i <= rangeTo; i++) {
        D1GfxFrame *frame = this->getFrame(i);
        if (frame->replacePixels(replacements)) {
            this->setModified();
        }
    }
}

void D1Gfx::mask()
{
    if (this->getFrameCount() <= 1)
        return;
    if (!this->isFrameSizeConstant()) {
        dProgressErr() << tr("Framesize is not constant");
        return;
    }

    if (this->groupFrameIndices.size() <= 1) {
        D1GfxFrame *frameA = this->frames[0];
        for (const D1GfxFrame *frameB : this->frames) {
            if (frameA->mask(frameB)) {
                this->setModified();
            }
        }
    } else {
        if (!this->isGroupSizeConstant()) {
            dProgressErr() << tr("Groupsize is not constant");
            return;
        }
        for (int n = this->groupFrameIndices[0].first; n <= this->groupFrameIndices[0].second; n++) {
            D1GfxFrame *frameA = this->frames[n];
            auto git = this->groupFrameIndices.begin();
            while (true) {
                git++;
                if (git == this->groupFrameIndices.end()) {
                    break;
                }
                const D1GfxFrame *frameB = this->frames[git->first + n - this->groupFrameIndices[0].first];
                if (frameA->mask(frameB)) {
                    this->setModified();
                }
            }
        }
    }
}

void D1Gfx::optimize()
{
    for (int i = 0; i < this->getFrameCount(); i++) {
        D1GfxFrame *frame = this->frames[i];
        if (frame->optimize(this->type)) {
            this->setModified();
            dProgress() << QApplication::tr("Frame %1 is modified.").arg(i + 1);
        }
    }
}

D1CEL_TYPE D1Gfx::getType() const
{
    return this->type;
}

void D1Gfx::setType(D1CEL_TYPE type)
{
    if (this->type == type)
        return;
    this->type = type;
    this->modified = true;
}

bool D1Gfx::isModified() const
{
    return this->modified;
}

void D1Gfx::setModified(bool modified)
{
    this->modified = modified;
}

bool D1Gfx::isPatched() const
{
    return this->patched;
}

void D1Gfx::setPatched(bool patched)
{
    if (this->patched == patched)
        return;
    this->patched = patched;
    this->modified = true;
}

bool D1Gfx::isClipped() const
{
    return this->clipped;
}

bool D1Gfx::setClipped(bool clipped)
{
    if (this->clipped == clipped)
        return false;
    this->clipped = clipped;
    this->modified = true;
    return true;
}

bool D1Gfx::isUpscaled() const
{
    return this->upscaled;
}

void D1Gfx::setUpscaled(bool upscaled)
{
    if (this->upscaled == upscaled)
        return;
    this->upscaled = upscaled;
    this->modified = true;
}

unsigned D1Gfx::getFrameLen() const
{
    return this->frameLen;
}

void D1Gfx::setFrameLen(unsigned frameLen)
{
    if (this->frameLen == frameLen)
        return;
    this->frameLen = frameLen;
    this->modified = true;
}

QString D1Gfx::getFilePath() const
{
    return this->gfxFilePath;
}

void D1Gfx::setFilePath(const QString &filePath)
{
    this->gfxFilePath = filePath;
    this->modified = true;
}

D1Pal *D1Gfx::getPalette() const
{
    return const_cast<D1Pal *>(this->palette);
}

void D1Gfx::setPalette(D1Pal *pal)
{
    this->palette = pal;
}

int D1Gfx::getGroupCount() const
{
    return this->groupFrameIndices.size();
}

std::pair<int, int> D1Gfx::getGroupFrameIndices(int groupIndex) const
{
    if (groupIndex < 0 || (unsigned)groupIndex >= this->groupFrameIndices.size()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Invalid group %1 requested. Group size: %2").arg(groupIndex).arg(this->groupFrameIndices.size()));
#endif
        return std::pair<int, int>(0, 0);
    }

    return this->groupFrameIndices[groupIndex];
}

int D1Gfx::getFrameCount() const
{
    return this->frames.count();
}

D1GfxFrame *D1Gfx::getFrame(int frameIndex) const
{
    if (frameIndex < 0 || frameIndex >= this->frames.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Invalid frame %1 requested. Frame count: %2").arg(frameIndex).arg(this->frames.count()));
#endif
        return nullptr;
    }

    return const_cast<D1GfxFrame *>(this->frames[frameIndex]);
}

void D1Gfx::setFrame(int frameIndex, D1GfxFrame *frame)
{
    if (this->frames.count() <= frameIndex) {
        // assert(frameIndex == this->frames.count());
        this->insertFrame(frameIndex);
    }
    delete this->frames[frameIndex];
    this->frames[frameIndex] = frame;
    this->modified = true;
}

int D1Gfx::getFrameWidth(int frameIndex) const
{
    if (frameIndex < 0 || frameIndex >= this->frames.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Width of an invalid frame %1 requested. Frame count: %2").arg(frameIndex).arg(this->frames.count()));
#endif
        return 0;
    }

    return this->frames[frameIndex]->getWidth();
}

int D1Gfx::getFrameHeight(int frameIndex) const
{
    if (frameIndex < 0 || frameIndex >= this->frames.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Height of an invalid frame %1 requested. Frame count: %2").arg(frameIndex).arg(this->frames.count()));
#endif
        return 0;
    }

    return this->frames[frameIndex]->getHeight();
}

bool D1Gfx::setFrameType(int frameIndex, D1CEL_FRAME_TYPE frameType)
{
    if (this->frames[frameIndex]->getFrameType() == frameType) {
        return false;
    }
    this->frames[frameIndex]->setFrameType(frameType);
    this->modified = true;
    return true;
}

bool D1Gfx::patchCathedralDoors(bool silent)
{
    constexpr int FRAME_WIDTH = 64;
    constexpr int FRAME_HEIGHT = 160;

    bool result = false;
    for (int i = 0; i < this->getFrameCount(); i++) {
        D1GfxFrame *frame = this->frames[i];
        if (frame->getWidth() != FRAME_WIDTH || frame->getHeight() != FRAME_HEIGHT) {
            dProgressErr() << tr("Framesize of the Cathedal-Doors does not match. (%1:%2 expected %3:%4. Index %5.)").arg(frame->getWidth()).arg(frame->getHeight()).arg(FRAME_WIDTH).arg(FRAME_HEIGHT).arg(i + 1);
            return result;
        }
        bool change = false;
        if (i == 0) {
            // add missing pixels after patchCathedralSpec
            change |= frame->setPixel(29, 81, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(30, 80, D1GfxPixel::colorPixel(110));
            change |= frame->setPixel(31, 79, D1GfxPixel::colorPixel(47));
        }
        if (i == 1) {
            // add missing pixels after patchCathedralSpec
            change |= frame->setPixel(31, 79, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(32, 79, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(33, 80, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(34, 81, D1GfxPixel::colorPixel(47));
            // move the door-handle to the right
            if (frame->getPixel(17, 112).getPaletteIndex() == 42) {
                // copy the door-handle to the right
                for (int y = 108; y < 119; y++) {
                    for (int x = 16; x < 24; x++) {
                        D1GfxPixel pixel = frame->getPixel(x, y);
                        quint8 color = pixel.getPaletteIndex();
                        if (color == 47) {
                            change |= frame->setPixel(x + 21, y + 7, D1GfxPixel::colorPixel(0));
                        }

                    }
                }
                change |= frame->setPixel(22 + 21, 116 + 7, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(18 + 21, 116 + 7, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(18 + 21, 117 + 7, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(17 + 21, 111 + 7, D1GfxPixel::colorPixel(0));
                // remove the original door-handle
                for (int y = 108; y < 120; y++) {
                    for (int x = 16; x < 24; x++) {
                        change |= frame->setPixel(x, y, frame->getPixel(x + 8, y + 7));
                    }
                }
            }
        }
        if (i == 2) {
            // add missing pixels after patchCathedralSpec
            change |= frame->setPixel(29, 81, D1GfxPixel::colorPixel(47));
            change |= frame->setPixel(30, 80, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(31, 79, D1GfxPixel::colorPixel(63));
            // add pixels for better outline
            for (int y = 72; y < 99; y++) {
                for (int x = 18; x < 42; x++) {
                    if (y == 72 && (x < 23 || x > 32)) {
                        continue;
                    }
                    if (y == 73 && (x < 19 || x > 35)) {
                        continue;
                    }
                    if (y == 74 && x > 38) {
                        continue;
                    }
                    if (y == 75 && x > 40) {
                        continue;
                    }
                    if (!frame->getPixel(x, y).isTransparent()) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(47));
                }
            }
        }
        if (i == 3) {
            // add missing pixels after patchCathedralSpec
            change |= frame->setPixel(31, 79, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(32, 79, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(33, 80, D1GfxPixel::colorPixel(46));
            change |= frame->setPixel(34, 81, D1GfxPixel::colorPixel(46));
            // add pixels for better outline
            for (int y = 72; y < 102; y++) {
                for (int x = 23; x < 47; x++) {
                    if (y == 72 && (x < 32 || x > 41)) {
                        continue;
                    }
                    if (y == 73 && (x < 29 || x > 45)) {
                        continue;
                    }
                    if (y == 74 && x < 26) {
                        continue;
                    }
                    if (y == 75 && x < 24) {
                        continue;
                    }
                    if (!frame->getPixel(x, y).isTransparent()) {
                        continue;
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(47));
                }
            }
        }
        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 is modified.").arg(i + 1);
            }
        }
    }
    return result;
}

bool D1Gfx::patchCatacombsDoors(bool silent)
{
    typedef struct {
        int frameIndex;
        int frameWidth;
        int frameHeight;
    } CelFrame;
    const CelFrame celFrames[] = {
        { 0, 64, 128 },
        { 1, 64, 128 },
    };

    bool result = false;
    int idx = 0;
    for (int i = 0; i < this->getFrameCount(); i++) {
        const CelFrame &cFrame = celFrames[idx];
        if (i == cFrame.frameIndex) {
            D1GfxFrame *frame = this->frames[i];
            if (frame->getWidth() != cFrame.frameWidth || frame->getHeight() != cFrame.frameHeight) {
                dProgressErr() << tr("Framesize of the Catacombs-Doors does not match. (%1:%2 expected %3:%4. Index %5.)").arg(frame->getWidth()).arg(frame->getHeight()).arg(cFrame.frameWidth).arg(cFrame.frameHeight).arg(cFrame.frameIndex + 1);
                return result;
            }
            bool change = false;
            if (idx == 0) {
                for (int y = 44; y < 55; y++) {
                    change |= frame->setPixel(41 - (y - 44) * 2, y, D1GfxPixel::colorPixel(62));
                }
            }
            if (idx == 1) {
                for (int x = 19; x < 40; x++) {
                    change |= frame->setPixel(x, 44 + (x / 2 - 10), D1GfxPixel::colorPixel(62));
                }
                for (int y = 55; y < 112; y++) {
                    change |= frame->setPixel(40, y, D1GfxPixel::transparentPixel());
                }
            }
            if (change) {
                result = true;
                this->setModified();
                if (!silent) {
                    dProgress() << QApplication::tr("Frame %1 is modified.").arg(i + 1);
                }
            }

            idx++;
        }
    }
    return result;
}

bool D1Gfx::patchCavesDoors(bool silent)
{
    typedef struct {
        int frameIndex;
        int frameWidth;
        int frameHeight;
    } CelFrame;
    const CelFrame celFrames[] = {
        { 0, 64, 128 },
        { 1, 64, 128 },
        { 2, 64, 128 },
        { 3, 64, 128 },
    };

    bool result = false;
    int idx = 0;
    for (int i = 0; i < this->getFrameCount(); i++) {
        const CelFrame &cFrame = celFrames[idx];
        if (i == cFrame.frameIndex) {
            D1GfxFrame *frame = this->frames[i];
            if (frame->getWidth() != cFrame.frameWidth || frame->getHeight() != cFrame.frameHeight) {
                dProgressErr() << tr("Framesize of the Caves-Doors does not match. (%1:%2 expected %3:%4. Index %5.)").arg(frame->getWidth()).arg(frame->getHeight()).arg(cFrame.frameWidth).arg(cFrame.frameHeight).arg(cFrame.frameIndex + 1);
                return result;
            }

            bool change = false;
            if (idx == 0) {
                // add cross section
                change |= frame->setPixel(33, 101, D1GfxPixel::colorPixel(125));
                // change |= frame->setPixel(34, 101, D1GfxPixel::colorPixel(124));

                change |= frame->setPixel(31, 100, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(32, 100, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(33, 100, D1GfxPixel::colorPixel(93));
                change |= frame->setPixel(34, 100, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(35, 100, D1GfxPixel::colorPixel(127));
                change |= frame->setPixel(30,  99, D1GfxPixel::colorPixel(92));
                change |= frame->setPixel(31,  99, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(32,  99, D1GfxPixel::colorPixel(93));
                change |= frame->setPixel(33,  99, D1GfxPixel::colorPixel(60));
                change |= frame->setPixel(34,  99, D1GfxPixel::colorPixel(93));
                change |= frame->setPixel(35,  99, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(36,  99, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(30,  98, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(31,  98, D1GfxPixel::colorPixel(92));
                change |= frame->setPixel(32,  98, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(33,  98, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(34,  98, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(35,  98, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(36,  98, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(37,  98, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(30,  97, D1GfxPixel::colorPixel(93));
                change |= frame->setPixel(31,  97, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(32,  97, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(33,  97, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(34,  97, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(35,  97, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(36,  97, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(37,  97, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(38,  97, D1GfxPixel::colorPixel(125));

                change |= frame->setPixel(31,  96, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(32,  96, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(33,  96, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(34,  96, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(35,  96, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(36,  96, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(37,  96, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(38,  96, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(39,  96, D1GfxPixel::colorPixel(125));

                change |= frame->setPixel(32,  95, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(33,  95, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(34,  95, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(35,  95, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(36,  95, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(37,  95, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(38,  95, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(39,  95, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(40,  95, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(41,  95, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(33,  94, D1GfxPixel::colorPixel(60));
                change |= frame->setPixel(34,  94, D1GfxPixel::colorPixel(127));
                change |= frame->setPixel(35,  94, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(36,  94, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(37,  94, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(38,  94, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(39,  94, D1GfxPixel::colorPixel(127));
                change |= frame->setPixel(40,  94, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(41,  94, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(34,  93, D1GfxPixel::colorPixel(60));
                change |= frame->setPixel(35,  93, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(36,  93, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(37,  93, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(38,  93, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(39,  93, D1GfxPixel::colorPixel(127));
                change |= frame->setPixel(35,  92, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(36,  92, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(37,  92, D1GfxPixel::colorPixel(126));

                // remove frame
                change |= frame->setPixel(54, 53, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(55, 53, D1GfxPixel::transparentPixel());

                // fix outline
                change |= frame->setPixel(26, 107, D1GfxPixel::colorPixel(62));
                change |= frame->setPixel(31, 109, D1GfxPixel::colorPixel(62));
                change |= frame->setPixel(50,  88, D1GfxPixel::colorPixel(125));

                // extend to the right
                for (int y = 58; y < 116; y++) {
                    change |= frame->setPixel(56, y, frame->getPixel(55, y - 1));
                }
            }

            if (idx == 1) {
                // add cross section
                change |= frame->setPixel(34, 41, D1GfxPixel::colorPixel(70));

                change |= frame->setPixel(21, 76, D1GfxPixel::colorPixel(124));

                change |= frame->setPixel(13, 109, D1GfxPixel::colorPixel(91));
                change |= frame->setPixel(14, 109, D1GfxPixel::colorPixel(109));
                change |= frame->setPixel(13, 108, D1GfxPixel::colorPixel(122));
                change |= frame->setPixel(14, 108, D1GfxPixel::colorPixel(91));
                change |= frame->setPixel(13, 107, D1GfxPixel::colorPixel(75));
                change |= frame->setPixel(14, 107, D1GfxPixel::colorPixel(89));
                change |= frame->setPixel(15, 107, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(13, 106, D1GfxPixel::colorPixel(91));
                change |= frame->setPixel(14, 106, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(15, 106, D1GfxPixel::colorPixel(75));
                change |= frame->setPixel(13, 105, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(14, 105, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(15, 105, D1GfxPixel::colorPixel(91));
                change |= frame->setPixel(16, 105, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(13, 104, D1GfxPixel::colorPixel(89));
                change |= frame->setPixel(14, 104, D1GfxPixel::colorPixel(122));
                change |= frame->setPixel(15, 104, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(16, 104, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(17, 104, D1GfxPixel::colorPixel(63));
                change |= frame->setPixel(13, 103, D1GfxPixel::colorPixel(88));
                change |= frame->setPixel(14, 103, D1GfxPixel::colorPixel(122));
                change |= frame->setPixel(15, 103, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(16, 103, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(17, 103, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(13, 102, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(14, 102, D1GfxPixel::colorPixel(122));
                change |= frame->setPixel(15, 102, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(16, 102, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(17, 102, D1GfxPixel::colorPixel(75));
                change |= frame->setPixel(18, 102, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(13, 101, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(14, 101, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(15, 101, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(16, 101, D1GfxPixel::colorPixel(91));
                change |= frame->setPixel(17, 101, D1GfxPixel::colorPixel(91));
                change |= frame->setPixel(18, 101, D1GfxPixel::colorPixel(122));
                change |= frame->setPixel(13, 100, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(14, 100, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(15, 100, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(16, 100, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(17, 100, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(18, 100, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(19, 100, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(13,  99, D1GfxPixel::colorPixel(127));
                change |= frame->setPixel(14,  99, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(15,  99, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(16,  99, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(17,  99, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(18,  99, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(19,  99, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(20,  99, D1GfxPixel::colorPixel(93));
                change |= frame->setPixel(15,  98, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(16,  98, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(17,  98, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(18,  98, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(19,  98, D1GfxPixel::colorPixel(93));
                change |= frame->setPixel(20,  98, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(21,  98, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(17,  97, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(18,  97, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(19,  97, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(20,  97, D1GfxPixel::colorPixel(93));
                change |= frame->setPixel(21,  97, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(19,  96, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(20,  96, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(21,  96, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(22,  96, D1GfxPixel::colorPixel(93));
                change |= frame->setPixel(21,  95, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(22,  95, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(23,  94, D1GfxPixel::colorPixel(126));

                // fix outline
                change |= frame->setPixel(36,  89, D1GfxPixel::colorPixel(93));
                change |= frame->setPixel(37, 107, D1GfxPixel::colorPixel(62));

                // remove frame
                change |= frame->setPixel(9, 53, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(8, 53, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(8, 54, D1GfxPixel::transparentPixel());
            }
            if (idx == 2) {
                // improve cross sections
                change |= frame->setPixel(37, 61, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(39, 79, D1GfxPixel::colorPixel(124));

                // fix outline
                // for (int y = 83; y < 91; y++) {
                //    change |= frame->setPixel(58, y, frame->getPixel(58, y - 20));
                // }
                // change |= frame->setPixel(57, 91, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(56, 91, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(55, 92, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(54, 92, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(53, 92, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(52, 92, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(51, 93, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(50, 93, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(49, 94, D1GfxPixel::colorPixel(126));
                // change |= frame->setPixel(48, 94, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(47, 95, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(46, 95, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(45, 96, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(44, 96, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(43, 97, D1GfxPixel::colorPixel(126));
                // change |= frame->setPixel(42, 97, D1GfxPixel::colorPixel(126));
                // change |= frame->setPixel(41, 98, D1GfxPixel::colorPixel(126));

                change |= frame->setPixel(21, 107, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(22, 108, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(23, 108, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(24, 108, D1GfxPixel::colorPixel(126));

                // remove the shadow
                for (int y = 88; y < 95; y++) {
                    for (int x = 43; x < 49; x++) {
                        change |= frame->setPixel(x, y, frame->getPixel(x + 6, y - 3));
                    }
                }
                for (int y = 93; y < 99; y++) {
                    for (int x = 41; x < 43; x++) {
                        change |= frame->setPixel(x, y, frame->getPixel(x - 2, y + 1));
                    }
                }
                change |= frame->setPixel(45, 87, D1GfxPixel::colorPixel(120));
                change |= frame->setPixel(46, 87, D1GfxPixel::colorPixel(71));
                change |= frame->setPixel(42, 89, D1GfxPixel::colorPixel(119));
                change |= frame->setPixel(42, 90, D1GfxPixel::colorPixel(73));
                change |= frame->setPixel(43, 95, D1GfxPixel::colorPixel(91));
                change |= frame->setPixel(44, 95, D1GfxPixel::colorPixel(122));
                change |= frame->setPixel(45, 95, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(46, 96, D1GfxPixel::colorPixel(124));

                // remove hidden pixels
                // change |= frame->setPixel(57, 42, D1GfxPixel::transparentPixel());
                for (int y = 43; y < 92; y++) {
                    for (int x = 57; x < 59; x++) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            if (idx == 3) {
                // improve cross sections
                change |= frame->setPixel(9, 53, D1GfxPixel::colorPixel(124));
                change |= frame->setPixel(9, 54, D1GfxPixel::colorPixel(123));
                change |= frame->setPixel(9, 60, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(9, 61, D1GfxPixel::transparentPixel());

                // fix outline
                change |= frame->setPixel( 8,  91, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel( 9,  92, D1GfxPixel::colorPixel(126));

                change |= frame->setPixel(27, 101, D1GfxPixel::colorPixel(62));
                change |= frame->setPixel(29, 102, D1GfxPixel::colorPixel(62));
                change |= frame->setPixel(31, 103, D1GfxPixel::colorPixel(62));
                change |= frame->setPixel(32, 103, D1GfxPixel::colorPixel(94));
                change |= frame->setPixel(33, 104, D1GfxPixel::colorPixel(62));
                change |= frame->setPixel(34, 104, D1GfxPixel::colorPixel(126));
                change |= frame->setPixel(35, 106, D1GfxPixel::colorPixel(125));
                change |= frame->setPixel(37, 107, D1GfxPixel::colorPixel(126));

                // remove hidden pixels
                change |= frame->setPixel( 3, 40, D1GfxPixel::transparentPixel());
                change |= frame->setPixel( 4, 40, D1GfxPixel::transparentPixel());
                for (int y = 41; y < 92; y++) {
                    for (int x = 1; x < 8; x++) {
                        if (x == 7 && y == 41) {
                            continue;
                        }
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }

            if (change) {
                result = true;
                this->setModified();
                if (!silent) {
                    dProgress() << QApplication::tr("Frame %1 is modified.").arg(i + 1);
                }
            }

            idx++;
        }
    }
    return result;
}

bool D1Gfx::patchMagicCircle(bool silent)
{
    constexpr int FRAME_WIDTH = 96;
    constexpr int FRAME_HEIGHT = 96;

    for (int i = 0; i < this->getFrameCount(); i++) {
        D1GfxFrame *frame = this->frames[i];
        if (frame->getWidth() != FRAME_WIDTH || frame->getHeight() != FRAME_HEIGHT) {
            dProgressErr() << tr("Framesize of the Magic Circle does not match. (%1:%2 expected %3:%4. Index %5.)").arg(frame->getWidth()).arg(frame->getHeight()).arg(FRAME_WIDTH).arg(FRAME_HEIGHT).arg(i + 1);
            return false;
        }
        if (i == 0 && frame->getPixel(5, 70).isTransparent()) {
            return false; // assume it is already done
        }

        bool change = false;
        int sy = 52;
        int ey = 91;
        int sx = 5;
        int ex = 85;
        int nw = (ex - sx) / 2;
        int nh = (ey - sy) / 2;
        // down-scale to half
        for (int y = sy; y < ey; y += 2) {
            for (int x = sx; x < ex; x += 2) {
                change |= frame->setPixel(sx + (x - sx) / 2, (sy + (y - sy) / 2), frame->getPixel(x, y));
            }
        }
        for (int y = sy; y < ey; y++) {
            for (int x = sx + nw; x < FRAME_WIDTH; x++) {
                change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
            }
        }
        for (int y = sy + nh; y < ey; y++) {
            for (int x = sx; x < FRAME_WIDTH; x++) {
                change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
            }
        }
        // move to the center
        constexpr int offx = 4;
        constexpr int offy = -2;
        for (int y = ey - 1; y >= sy + nh; y--) {
            for (int x = nw / 2; x < FRAME_WIDTH; x++) {
                change |= frame->setPixel(x + offx, y + offy, frame->getPixel(x - nw / 2, y - nh));
            }
            for (int x = 0; x < FRAME_WIDTH - nw / 2; x++) {
                change |= frame->setPixel(x, y - nh, D1GfxPixel::transparentPixel());
            }
        }
        // fix colors
        if (i == 0) {
            change |= frame->setPixel(57, 70, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(65, 84, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(58, 87, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(34, 85, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(40, 87, D1GfxPixel::transparentPixel());

            change |= frame->setPixel(46, 69, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(47, 69, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(48, 69, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(49, 69, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(50, 69, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(51, 69, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(46, 88, D1GfxPixel::colorPixel(248));
            change |= frame->setPixel(47, 88, D1GfxPixel::colorPixel(248));
            change |= frame->setPixel(48, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(49, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(50, 88, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(51, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(40, 70, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(30, 75, D1GfxPixel::colorPixel(248));
            change |= frame->setPixel(31, 74, D1GfxPixel::colorPixel(248));
            change |= frame->setPixel(33, 73, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(37, 71, D1GfxPixel::colorPixel(251));
            change |= frame->setPixel(67, 75, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(66, 74, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(63, 72, D1GfxPixel::colorPixel(251));
            change |= frame->setPixel(61, 86, D1GfxPixel::colorPixel(252));

            change |= frame->setPixel(29, 77, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(29, 78, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(29, 79, D1GfxPixel::colorPixel(251));
            change |= frame->setPixel(29, 80, D1GfxPixel::colorPixel(249));
        }
        if (i == 1) {
            change |= frame->setPixel(57, 70, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(61, 71, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(65, 73, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(68, 81, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(58, 87, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(65, 84, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(40, 87, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(46, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(47, 88, D1GfxPixel::colorPixel(166));
            change |= frame->setPixel(48, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(49, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(50, 88, D1GfxPixel::colorPixel(169));
            change |= frame->setPixel(51, 88, D1GfxPixel::colorPixel(248));
            change |= frame->setPixel(30, 75, D1GfxPixel::colorPixel(248));
            change |= frame->setPixel(31, 74, D1GfxPixel::colorPixel(248));
            change |= frame->setPixel(30, 82, D1GfxPixel::colorPixel(166));
            change |= frame->setPixel(31, 83, D1GfxPixel::colorPixel(166));
            change |= frame->setPixel(51, 69, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(46, 69, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(29, 77, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(29, 78, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(29, 80, D1GfxPixel::colorPixel(248));
        }
        if (i == 2) {
            change |= frame->setPixel(37, 71, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(40, 70, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(40, 87, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(57, 70, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(63, 72, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(58, 87, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(65, 84, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(67, 75, D1GfxPixel::colorPixel(251));
            change |= frame->setPixel(66, 74, D1GfxPixel::colorPixel(251));
            change |= frame->setPixel(29, 77, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(29, 78, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(29, 79, D1GfxPixel::colorPixel(251));
            change |= frame->setPixel(29, 80, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(30, 75, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(31, 74, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(30, 82, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(31, 83, D1GfxPixel::colorPixel(250));

            change |= frame->setPixel(46, 88, D1GfxPixel::colorPixel(248));
            change |= frame->setPixel(47, 88, D1GfxPixel::colorPixel(248));
            change |= frame->setPixel(48, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(49, 88, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(50, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(51, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(46, 69, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(47, 69, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(48, 69, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(49, 69, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(50, 69, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(51, 69, D1GfxPixel::colorPixel(249));
        }
        if (i == 3) {
            change |= frame->setPixel(30, 78, D1GfxPixel::colorPixel(167));
            change |= frame->setPixel(29, 77, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(29, 78, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(29, 79, D1GfxPixel::colorPixel(251));
            change |= frame->setPixel(29, 80, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(40, 87, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(63, 85, D1GfxPixel::colorPixel(252));
            change |= frame->setPixel(57, 70, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(61, 71, D1GfxPixel::transparentPixel());
            change |= frame->setPixel(30, 75, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(31, 74, D1GfxPixel::colorPixel(248));
            change |= frame->setPixel(30, 82, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(31, 83, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(47, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(48, 88, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(49, 88, D1GfxPixel::colorPixel(251));
            change |= frame->setPixel(67, 75, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(66, 74, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(48, 69, D1GfxPixel::colorPixel(251));
            change |= frame->setPixel(47, 69, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(40, 70, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(37, 71, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(61, 86, D1GfxPixel::colorPixel(250));
            change |= frame->setPixel(63, 72, D1GfxPixel::colorPixel(251));

            change |= frame->setPixel(46, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(50, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(51, 88, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(46, 69, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(50, 69, D1GfxPixel::colorPixel(249));
            change |= frame->setPixel(51, 69, D1GfxPixel::colorPixel(249));
        }

        // if (change) {
            // result = true;
            this->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 is modified.").arg(i + 1);
            }
        // }
    }
    return true;
}

bool D1Gfx::patchCandle(bool silent)
{
    constexpr int FRAME_WIDTH = 96;
    constexpr int FRAME_HEIGHT = 96;

    bool result = false;
    for (int i = 0; i < this->getFrameCount(); i++) {
        D1GfxFrame *frame = this->frames[i];
        if (frame->getWidth() != FRAME_WIDTH || frame->getHeight() != FRAME_HEIGHT) {
            dProgressErr() << tr("Framesize of the Candle does not match. (%1:%2 expected %3:%4. Index %5.)").arg(frame->getWidth()).arg(frame->getHeight()).arg(FRAME_WIDTH).arg(FRAME_HEIGHT).arg(i + 1);
            return false;
        }
        // if (i == 0 && frame->getPixel(32, 65).isTransparent()) {
        //    return false; // assume it is already done
        // }

        bool change = false;
        // remove shadow
        for (int y = 63; y < 80; y++) {
            for (int x = 28; x < 45; x++) {
                D1GfxPixel pixel = frame->getPixel(x, y);
                if (!pixel.isTransparent() && pixel.getPaletteIndex() == 0) {
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()) ? 1 : 0;
                }
            }
        }

        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 is modified.").arg(i + 1);
            }
        }
    }
    return result;
}

bool D1Gfx::patchLeftShrine(bool silent)
{
    constexpr int FRAME_WIDTH = 128;
    constexpr int FRAME_HEIGHT = 128;

    const int resCelEntries = 11;

    bool result = false;
    int removedFrames = 0;
    for (int i = 0; i < this->getFrameCount(); i++) {
        D1GfxFrame *frame = this->frames[i];
        if (frame->getWidth() != FRAME_WIDTH || frame->getHeight() != FRAME_HEIGHT) {
            dProgressErr() << tr("Framesize of the west-facing shrine does not match. (%1:%2 expected %3:%4. Index %5.)").arg(frame->getWidth()).arg(frame->getHeight()).arg(FRAME_WIDTH).arg(FRAME_HEIGHT).arg(i + 1);
            return false;
        }
        int change = 0;
        if (i > resCelEntries - 1) {
            this->removeFrame(i, false);
            change |= 2;
            removedFrames++;
            i--;
        } else if (this->frames.count() > 11) {
            // use the more rounded shrine-graphics
            D1GfxFrame *frameSrc = this->frames[11];
            for (int y = 88; y < 110; y++) {
                for (int x = 28; x < 80; x++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent() && pixel.getPaletteIndex() == 248) {
                        continue; // preserve the dirt/shine on the floor
                    }
                    change |= frame->setPixel(x, y, frameSrc->getPixel(x + 7, y - 2)) ? 1 : 0;
                }
            }
        }

        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                if (change & 2) {
                    dProgress() << QApplication::tr("Frame %1 is removed.").arg(i + 1 + removedFrames);
                } else {
                    dProgress() << QApplication::tr("Frame %1 is modified.").arg(i + 1);
                }
            }
        }
    }
    return result;
}

bool D1Gfx::patchRightShrine(bool silent)
{
    constexpr int FRAME_WIDTH = 128;
    constexpr int FRAME_HEIGHT = 128;

    const int resCelEntries = 11;

    bool result = false;
    int removedFrames = 0;
    for (int i = 0; i < this->getFrameCount(); i++) {
        D1GfxFrame *frame = this->frames[i];
        if (frame->getWidth() != FRAME_WIDTH || frame->getHeight() != FRAME_HEIGHT) {
            dProgressErr() << tr("Framesize of the east-facing shrine does not match. (%1:%2 expected %3:%4. Index %5.)").arg(frame->getWidth()).arg(frame->getHeight()).arg(FRAME_WIDTH).arg(FRAME_HEIGHT).arg(i + 1);
            return false;
        }
        int change = 0;
        if (i > resCelEntries - 1) {
            this->removeFrame(i, false);
            change |= 2;
            removedFrames++;
            i--;
        } else {
            change |= frame->setPixel(85, 101, D1GfxPixel::transparentPixel()) ? 1 : 0;
            change |= frame->setPixel(88, 100, D1GfxPixel::transparentPixel()) ? 1 : 0;
        }

        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                if (change & 2) {
                    dProgress() << QApplication::tr("Frame %1 is removed.").arg(i + 1 + removedFrames);
                } else {
                    dProgress() << QApplication::tr("Frame %1 is modified.").arg(i + 1);
                }
            }
        }
    }
    return result;
}

bool D1Gfx::patchCryptLight(bool silent)
{
    constexpr int FRAME_WIDTH = 96;
    constexpr int FRAME_HEIGHT = 96;

    const int resCelEntries = 1;

    bool result = false;
    int removedFrames = 0;
    for (int i = 0; i < this->getFrameCount(); i++) {
        D1GfxFrame *frame = this->frames[i];
        if (frame->getWidth() != FRAME_WIDTH || frame->getHeight() != FRAME_HEIGHT) {
            dProgressErr() << tr("Framesize of the Light stand in Crypt does not match. (%1:%2 expected %3:%4. Index %5.)").arg(frame->getWidth()).arg(frame->getHeight()).arg(FRAME_WIDTH).arg(FRAME_HEIGHT).arg(i + 1);
            return false;
        }

        int change = 0;
        if (i > resCelEntries - 1) {
            this->removeFrame(i, false);
            change |= 2;
            removedFrames++;
            i--;
        } else {
            // remove shadow
            for (int y = 61; y < 86; y++) {
                for (int x = 15; x < 63; x++) {
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (!pixel.isTransparent() && pixel.getPaletteIndex() == 0) {
                        change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel()) ? 1 : 0;
                    }
                }
            }
        }

        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                if (change & 2) {
                    dProgress() << QApplication::tr("Frame %1 is removed.").arg(i + 1 + removedFrames);
                } else {
                    dProgress() << QApplication::tr("Frame %1 is modified.").arg(i + 1);
                }
            }
        }
    }
    return result;
}

bool D1Gfx::patchPlrFrames(int gfxFileIndex, bool silent)
{
    int frameCount = 0, width = 0, height = 0;
    switch (gfxFileIndex) {
    case GFX_PLR_RHTAT: frameCount = 18 - 2; width = 128; height = 128; break;
    case GFX_PLR_RHUHT: frameCount =  8 - 1; width =  96; height =  96; break;
    case GFX_PLR_RHUQM: frameCount = 17 - 1; width =  96; height =  96; break;
    case GFX_PLR_RMTAT: frameCount = 18 - 2; width = 128; height = 128; break;
    case GFX_PLR_WHMAT: frameCount = 17 - 1; width = 128; height =  96; break;
    case GFX_PLR_WLNLM: frameCount = 21 - 1; width =  96; height =  96; break;
    case GFX_PLR_WMDLM: frameCount = 21 - 1; width =  96; height =  96; break;
    }

    if (this->getGroupCount() != NUM_DIRS) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }

    for (int ii = 0; ii < NUM_DIRS; ii++) {
        if ((this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first + 1) < frameCount) {
            dProgressErr() << tr("Not enough frames in the frame group %1.").arg(ii + 1);
            return false;
        }
#if 1
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
                dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
                return false;
            }
        }
#endif
    }
    bool result = false;
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        if (gfxFileIndex == GFX_PLR_RHTAT || gfxFileIndex == GFX_PLR_RMTAT) {
            int i = this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first;
            if (i + 1 == 18) {
                i = 13;
                this->removeFrame(this->getGroupFrameIndices(ii).first + i, false);
                dProgress() << tr("Removed frame %1 of group %2.").arg(i + 1).arg(ii + 1);
                i = 11;
                this->removeFrame(this->getGroupFrameIndices(ii).first + i, false);
                dProgress() << tr("Removed frame %1 of group %2.").arg(i + 1).arg(ii + 1);
                result = true;
            }
        }
        while (true) {
            int i = this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first;
            if (i < frameCount)
                break;
            this->removeFrame(this->getGroupFrameIndices(ii).first + i, false);
            dProgress() << tr("Removed frame %1 of group %2.").arg(i + 1).arg(ii + 1);
            result = true;
        }
    }
    return result;
}

bool D1Gfx::patchRogueExtraPixels(int gfxFileIndex, bool silent)
{
    constexpr BYTE TRANS_COLOR = 1;
    int frameCount = 0, width = 0, height = 0;
    switch (gfxFileIndex) {
    case GFX_PLR_RLHAS: frameCount =  8; width =  96; height =  96; break;
    case GFX_PLR_RLHAT: frameCount = 18; width = 128; height = 128; break;
    case GFX_PLR_RLHAW: frameCount =  8; width =  96; height =  96; break;
    case GFX_PLR_RLHBL: frameCount =  4; width =  96; height =  96; break;
    case GFX_PLR_RLHFM: frameCount = 16; width =  96; height =  96; break;
    case GFX_PLR_RLHLM: frameCount = 16; width =  96; height =  96; break;
    case GFX_PLR_RLHHT: frameCount =  7; width =  96; height =  96; break;
    case GFX_PLR_RLHQM: frameCount = 16; width =  96; height =  96; break;
    case GFX_PLR_RLHST: frameCount = 20; width =  96; height =  96; break;
    case GFX_PLR_RLHWL: frameCount =  8; width =  96; height =  96; break;
    case GFX_PLR_RLMAT: frameCount = 18; width = 128; height = 128; break;
    case GFX_PLR_RMDAW: frameCount =  8; width =  96; height =  96; break;
    case GFX_PLR_RMHAT: frameCount = 18; width = 128; height = 128; break;
    case GFX_PLR_RMMAT: frameCount = 18; width = 128; height = 128; break;
    case GFX_PLR_RMBFM: frameCount = 16; width =  96; height =  96; break;
    case GFX_PLR_RMBLM: frameCount = 16; width =  96; height =  96; break;
    case GFX_PLR_RMBQM: frameCount = 16; width =  96; height =  96; break;
    }

    if (this->getGroupCount() != NUM_DIRS) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }

    for (int ii = 0; ii < NUM_DIRS; ii++) {
        if ((this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first + 1) < frameCount) {
            dProgressErr() << tr("Not enough frames in the frame group %1.").arg(ii + 1);
            return false;
        }
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
                dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
                return false;
            }
        }
    }
    bool result = false;
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            bool change = false;
            int nn = n;
            switch (gfxFileIndex) {
            case GFX_PLR_RLHAS:
                for (int i = 0; i < lengthof(deltaRLHAS); i++) {
                    if (deltaRLHAS[i].dfFrameNum == nn + 1) {
                        change |= currFrame->setPixel(deltaRLHAS[i].dfx, deltaRLHAS[i].dfy, deltaRLHAS[i].color == TRANS_COLOR ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(deltaRLHAS[i].color));
                    }
                }
                break;
            case GFX_PLR_RLHAT:
                for (int i = 0; i < lengthof(deltaRLHAT); i++) {
                    if (deltaRLHAT[i].dfFrameNum == nn + 1) {
                        change |= currFrame->setPixel(deltaRLHAT[i].dfx, deltaRLHAT[i].dfy, deltaRLHAT[i].color == TRANS_COLOR ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(deltaRLHAT[i].color));
                    }
                }
                break;
            case GFX_PLR_RLHAW:
                for (int i = 0; i < lengthof(deltaRLHAW); i++) {
                    if (deltaRLHAW[i].dfFrameNum == nn + 1) {
                        change |= currFrame->setPixel(deltaRLHAW[i].dfx, deltaRLHAW[i].dfy, deltaRLHAW[i].color == TRANS_COLOR ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(deltaRLHAW[i].color));
                    }
                }
                break;
            case GFX_PLR_RLHBL:
                for (int i = 0; i < lengthof(deltaRLHBL); i++) {
                    if (deltaRLHBL[i].dfFrameNum == nn + 1) {
                        change |= currFrame->setPixel(deltaRLHBL[i].dfx, deltaRLHBL[i].dfy, deltaRLHBL[i].color == TRANS_COLOR ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(deltaRLHBL[i].color));
                    }
                }
                break;
            case GFX_PLR_RLHFM:
                for (int i = 0; i < lengthof(deltaRLHFM); i++) {
                    if (deltaRLHFM[i].dfFrameNum == nn + 1) {
                        change |= currFrame->setPixel(deltaRLHFM[i].dfx, deltaRLHFM[i].dfy, deltaRLHFM[i].color == TRANS_COLOR ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(deltaRLHFM[i].color));
                    }
                }
                break;
            case GFX_PLR_RLHLM:
                for (int i = 0; i < lengthof(deltaRLHLM); i++) {
                    if (deltaRLHLM[i].dfFrameNum == nn + 1) {
                        change |= currFrame->setPixel(deltaRLHLM[i].dfx, deltaRLHLM[i].dfy, deltaRLHLM[i].color == TRANS_COLOR ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(deltaRLHLM[i].color));
                    }
                }
                break;
            case GFX_PLR_RLHHT:
                for (int i = 0; i < lengthof(deltaRLHHT); i++) {
                    if (deltaRLHHT[i].dfFrameNum == nn + 1) {
                        change |= currFrame->setPixel(deltaRLHHT[i].dfx, deltaRLHHT[i].dfy, deltaRLHHT[i].color == TRANS_COLOR ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(deltaRLHHT[i].color));
                    }
                }
                break;
            case GFX_PLR_RLHQM:
                for (int i = 0; i < lengthof(deltaRLHQM); i++) {
                    if (deltaRLHQM[i].dfFrameNum == nn + 1) {
                        change |= currFrame->setPixel(deltaRLHQM[i].dfx, deltaRLHQM[i].dfy, deltaRLHQM[i].color == TRANS_COLOR ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(deltaRLHQM[i].color));
                    }
                }
                break;
            case GFX_PLR_RLHST:
                for (int i = 0; i < lengthof(deltaRLHST); i++) {
                    if (deltaRLHST[i].dfFrameNum == nn + 1) {
                        change |= currFrame->setPixel(deltaRLHST[i].dfx, deltaRLHST[i].dfy, deltaRLHST[i].color == TRANS_COLOR ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(deltaRLHST[i].color));
                    }
                }
                break;
            case GFX_PLR_RLHWL:
                for (int i = 0; i < lengthof(deltaRLHWL); i++) {
                    if (deltaRLHWL[i].dfFrameNum == nn + 1) {
                        change |= currFrame->setPixel(deltaRLHWL[i].dfx, deltaRLHWL[i].dfy, deltaRLHWL[i].color == TRANS_COLOR ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(deltaRLHWL[i].color));
                    }
                }
                break;
            case GFX_PLR_RLMAT:
                for (int i = 0; i < lengthof(deltaRLMAT); i++) {
                    if (deltaRLMAT[i].dfFrameNum == nn + 1) {
                        if (deltaRLMAT[i].color == TRANS_COLOR)
                            change |= currFrame->setPixel(deltaRLMAT[i].dfx, deltaRLMAT[i].dfy, D1GfxPixel::transparentPixel());
                        else
                            change |= currFrame->setPixel(deltaRLMAT[i].dfx, deltaRLMAT[i].dfy, D1GfxPixel::colorPixel(deltaRLMAT[i].color));
                    }
                }
                break;
            case GFX_PLR_RMDAW:
                for (int i = 0; i < lengthof(deltaRMDAW); i++) {
                    if (deltaRMDAW[i].dfFrameNum == nn + 1) {
                        change |= currFrame->setPixel(deltaRMDAW[i].dfx, deltaRMDAW[i].dfy, deltaRMDAW[i].color == TRANS_COLOR ? D1GfxPixel::transparentPixel() : D1GfxPixel::colorPixel(deltaRMDAW[i].color));
                    }
                }
                break;
            case GFX_PLR_RMHAT:
                for (int i = 0; i < lengthof(deltaRMHAT); i++) {
                    if (deltaRMHAT[i].dfFrameNum == nn + 1) {
                        if (deltaRMHAT[i].color == TRANS_COLOR)
                            change |= currFrame->setPixel(deltaRMHAT[i].dfx, deltaRMHAT[i].dfy, D1GfxPixel::transparentPixel());
                        else
                            change |= currFrame->setPixel(deltaRMHAT[i].dfx, deltaRMHAT[i].dfy, D1GfxPixel::colorPixel(deltaRMHAT[i].color));
                    }
                }
                break;
            case GFX_PLR_RMMAT:
                for (int i = 0; i < lengthof(deltaRMMAT); i++) {
                    if (deltaRMMAT[i].dfFrameNum == nn + 1) {
                        if (deltaRMMAT[i].color == TRANS_COLOR)
                            change |= currFrame->setPixel(deltaRMMAT[i].dfx, deltaRMMAT[i].dfy, D1GfxPixel::transparentPixel());
                        else
                            change |= currFrame->setPixel(deltaRMMAT[i].dfx, deltaRMMAT[i].dfy, D1GfxPixel::colorPixel(deltaRMMAT[i].color));
                    }
                }
                break;
            case GFX_PLR_RMBFM:
                for (int i = 0; i < lengthof(deltaRMBFM); i++) {
                    if (deltaRMBFM[i].dfFrameNum == nn + 1) {
                        if (deltaRMBFM[i].color == TRANS_COLOR)
                            change |= currFrame->setPixel(deltaRMBFM[i].dfx, deltaRMBFM[i].dfy, D1GfxPixel::transparentPixel());
                        else
                            change |= currFrame->setPixel(deltaRMBFM[i].dfx, deltaRMBFM[i].dfy, D1GfxPixel::colorPixel(deltaRMBFM[i].color));
                    }
                }
                break;
            case GFX_PLR_RMBLM:
                for (int i = 0; i < lengthof(deltaRMBLM); i++) {
                    if (deltaRMBLM[i].dfFrameNum == nn + 1) {
                        if (deltaRMBLM[i].color == TRANS_COLOR)
                            change |= currFrame->setPixel(deltaRMBLM[i].dfx, deltaRMBLM[i].dfy, D1GfxPixel::transparentPixel());
                        else
                            change |= currFrame->setPixel(deltaRMBLM[i].dfx, deltaRMBLM[i].dfy, D1GfxPixel::colorPixel(deltaRMBLM[i].color));
                    }
                }
                break;
            case GFX_PLR_RMBQM:
                for (int i = 0; i < lengthof(deltaRMBQM); i++) {
                    if (deltaRMBQM[i].dfFrameNum == nn + 1) {
                        if (deltaRMBQM[i].color == TRANS_COLOR)
                            change |= currFrame->setPixel(deltaRMBQM[i].dfx, deltaRMBQM[i].dfy, D1GfxPixel::transparentPixel());
                        else
                            change |= currFrame->setPixel(deltaRMBQM[i].dfx, deltaRMBQM[i].dfy, D1GfxPixel::colorPixel(deltaRMBQM[i].color));
                    }
                }
                break;
            }

            if (change) {
                result = true;
                this->setModified();
                if (!silent) {
                    dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(n + 1).arg(DIR_SW + 1);
                }
            }
        }
    }
    return result;
}

bool D1Gfx::patchWarriorStand(bool silent)
{
    QString baseFilePath = this->getFilePath();
    if (baseFilePath.length() < 13) {
        dProgressErr() << tr("Unrecognized file-path. Expected *WMH\\WMHAS.CL2");
        return false;
    }
    // read WMHAT.CL2 from the same folder
    QString atkPath = baseFilePath;
    atkPath[atkPath.length() - 5] = QChar('T');

    if (!QFileInfo::exists(atkPath)) {
        dProgressErr() << tr("Could not find %1 to be used as a template file").arg(QDir::toNativeSeparators(atkPath));
        return false;
    }

    // read WMMAS.CL2 from the same folder
    QString stdPath = baseFilePath;
    stdPath[stdPath.length() - 7] = QChar('M');
    if (!QFileInfo::exists(stdPath)) {
        // check for the standard file-structure of the MPQ file (WMMAS.CL2 in the WMM folder)
        stdPath[stdPath.length() - 11] = QChar('M');
        if (!QFileInfo::exists(stdPath)) {
            dProgressErr() << tr("Could not find %1 to be used as a template file").arg(QDir::toNativeSeparators(stdPath));
            return false;
        }
    }

    OpenAsParam opParams = OpenAsParam();
    D1Gfx atkGfx;
    atkGfx.setPalette(this->palette);
    if (!D1Cl2::load(atkGfx, atkPath, opParams)) {
        dProgressErr() << (tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(atkPath)));
        return false;
    }

    D1Gfx stdGfx;
    stdGfx.setPalette(this->palette);
    if (!D1Cl2::load(stdGfx, stdPath, opParams)) {
        dProgressErr() << (tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(stdPath)));
        return false;
    }

    constexpr int frameCount = 10;
    constexpr int width = 96;
    constexpr int height = 96;

    if (this->getGroupCount() <= DIR_SW) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    if (this->getGroupFrameIndices(DIR_SW).first != frameCount && this->getGroupFrameIndices(DIR_SW).second != 2 * frameCount - 1) {
        dProgressErr() << tr("Not enough frames in the frame group %1.").arg((int)DIR_SW + 1 - 1);
        return false;
    }
    if (stdGfx.getGroupCount() <= DIR_SW) {
        dProgressErr() << tr("Not enough frame groups in '%1'.").arg(QDir::toNativeSeparators(stdPath));
        return false;
    }
    if (stdGfx.getGroupFrameIndices(DIR_SW).first != frameCount && stdGfx.getGroupFrameIndices(DIR_SW).second != 2 * frameCount - 1) {
        dProgressErr() << tr("Not enough frames in the frame group %1 in '%2'.").arg((int)DIR_SW + 1 - 1).arg(QDir::toNativeSeparators(stdPath));
        return false;
    }
    if (atkGfx.getGroupCount() <= DIR_SW) {
        dProgressErr() << tr("Not enough frame groups in '%1'.").arg(QDir::toNativeSeparators(atkPath));
        return false;
    }
    constexpr int atkWidth = 128;
    const D1GfxFrame* frameSrcAtk = atkGfx.getFrame(atkGfx.getGroupFrameIndices(DIR_SW).first);
    if (frameSrcAtk->getWidth() != atkWidth || frameSrcAtk->getHeight() != height) {
        dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(atkPath)).arg(atkWidth).arg(height);
        return false;
    }

    bool result = false;
    for (int n = 0; n < frameCount; n++) {
        D1GfxFrame* frameSrcStd = stdGfx.getFrame(stdGfx.getGroupFrameIndices(DIR_SW).first + n);
        if (frameSrcStd->getWidth() != width || frameSrcStd->getHeight() != height) {
            dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(stdPath)).arg(width).arg(height);
            return false;
        }
        // copy the shield to the stand frame
        int dy = 0;
        switch (n) {
        case 0: dy = 0; break;
        case 1: dy = 1; break;
        case 2: dy = 2; break;
        case 3: dy = 2; break;
        case 4: dy = 3; break;
        case 5: dy = 3; break;
        case 6: dy = 3; break;
        case 7: dy = 2; break;
        case 8: dy = 2; break;
        case 9: dy = 1; break;
        }
        for (int y = 38; y < 66; y++) {
            for (int x = 19; x < 32; x++) {
                if (x == 31 && y >= 60) {
                    break;
                }
                D1GfxPixel pixel = frameSrcAtk->getPixel(x + 17, y);
                if (pixel.isTransparent() || pixel.getPaletteIndex() == 0) {
                    continue;
                }
                frameSrcStd->setPixel(x, y + dy, pixel);
            }
        }
        // fix the shadow
        // - main shield
        for (int y = 72; y < 80; y++) {
            for (int x = 12; x < 31; x++) {
                D1GfxPixel pixel = frameSrcStd->getPixel(x, y);
                if (!pixel.isTransparent()) {
                    continue;
                }
                if (y < 67 + x / 2 && y > x + 48) {
                    frameSrcStd->setPixel(x, y, D1GfxPixel::colorPixel(0));
                }
            }
        }
        // -  sink effect on the top-left side
        //if (n > 1 && n < 9) {
        if (dy > 1) {
            // if (n >= 4 && n <= 6) {
            if (dy == 3) {
                frameSrcStd->setPixel(17, 75, D1GfxPixel::colorPixel(0));
            }
            else {
                frameSrcStd->setPixel(15, 74, D1GfxPixel::colorPixel(0));
            }
        }
        // - sink effect on the top-right side
        // if (n > 1 && n < 9) {
        if (dy > 1) {
            frameSrcStd->setPixel(27, 75, D1GfxPixel::colorPixel(0));
            // if (n > 3 && n < 7) {
            if (dy == 3) {
                frameSrcStd->setPixel(26, 74, D1GfxPixel::colorPixel(0));
            }
        }
        // - sink effect on the bottom
        // if (n > 0) {
        if (dy != 0) {
            frameSrcStd->setPixel(28, 80, D1GfxPixel::colorPixel(0));
        }
        // if (n > 1 && n < 5) {
        if (dy > 1) {
            frameSrcStd->setPixel(29, 80, D1GfxPixel::colorPixel(0));
        }
        // if (n > 3 && n < 7) {
        if (dy == 3) {
            frameSrcStd->setPixel(27, 80, D1GfxPixel::colorPixel(0));
        }

        // copy the result to the active graphics
        D1GfxFrame* frame = this->getFrame(this->getGroupFrameIndices(DIR_SW).first + n);
        if (frame->getWidth() != width || frame->getHeight() != height) {
            dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
            return result;
        }

        bool change = false;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                change |= frame->setPixel(x, y, frameSrcStd->getPixel(x, y));
            }
        }
        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(n + 1).arg(DIR_SW + 1);
            }
        }
    }
    return result;
}

static bool ShiftFrame(D1GfxFrame *frame, int dx, int dy, int sx, int sy, int ex, int ey)
{
    if (dx == 0 && dy == 0)
        return false;
    int width = frame->getWidth();
    int height = frame->getHeight();
    bool change = false;
    if (dx <= 0) {
        if (dy <= 0) {
            // for (int y = std::max(sy, -dy); y < ey; y++) {
            for (int y = sy; y < ey; y++) {
                // for (int x = std::max(sx, -dx); x < ex; x++) {
                for (int x = sx; x < ex; x++) {
                    if (x + dx >= 0 /*&& x + dx < width*/ && y + dy >= 0 /*&& y + dy < height*/)
                    {
                        D1GfxPixel pixel = frame->getPixel(x, y);
                        if (pixel.isTransparent())
                            continue;
                        change |= frame->setPixel(x + dx, y + dy, pixel);
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        } else {
            // for (int y = std::min(ey, height - dy) - 1; y >= sy; y--) {
            for (int y = ey - 1; y >= sy; y--) {
                // for (int x = std::max(sx, -dx); x < ex; x++) {
                for (int x = sx; x < ex; x++) {
                    if (x + dx >= 0 /*&& x + dx < width && y + dy >= 0 */&& y + dy < height)
                    {
                        D1GfxPixel pixel = frame->getPixel(x, y);
                        if (pixel.isTransparent())
                            continue;
                        change |= frame->setPixel(x + dx, y + dy, pixel);
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
    } else {
        if (dy <= 0) {
            // for (int y = std::max(sy, -dy); y < ey; y++) {
            for (int y = sy; y < ey; y++) {
                // for (int x = std::min(ex, width - dx) - 1; x >= sx; x--) {
                for (int x = ex - 1; x >= sx; x--) {
                    if (/*x + dx >= 0 && */x + dx < width && y + dy >= 0 /*&& y + dy < height*/)
                    {
                        D1GfxPixel pixel = frame->getPixel(x, y);
                        if (pixel.isTransparent())
                            continue;
                        change |= frame->setPixel(x + dx, y + dy, pixel);
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        } else {
            // for (int y = std::min(ey, height - dy) - 1; y >= sy; y--) {
            for (int y = ey - 1; y >= sy; y--) {
                // for (int x = std::min(ex, width - dx) - 1; x >= sx; x--) {
                for (int x = ex - 1; x >= sx; x--) {
                    if (/*x + dx >= 0 && */x + dx < width /*&& y + dy >= 0 */&& y + dy < height)
                    {
                        D1GfxPixel pixel = frame->getPixel(x, y);
                        if (pixel.isTransparent())
                            continue;
                        change |= frame->setPixel(x + dx, y + dy, pixel);
                    }
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
    }
    return change;
}

static bool CopyFrame(D1GfxFrame* dstFrame, int dx, int dy, const D1GfxFrame* srcFrame, int sx, int sy, int ex, int ey)
{
    bool change = false;
    for (int y = sy; y < ey; y++) {
        for (int x = sx; x < ex; x++) {
            D1GfxPixel pixel = srcFrame->getPixel(x, y);
            if (pixel.isTransparent())
                continue;
            change |= dstFrame->setPixel(x + dx, y + dy, pixel);
        }
    }
    return change;
}

bool D1Gfx::patchFallGDie(bool silent)
{
    constexpr int frameCount = 17;
    constexpr int width = 128;
    constexpr int height = 128;

    if (this->getGroupCount() != NUM_DIRS) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        if ((this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first + 1) < frameCount) {
            dProgressErr() << tr("Not enough frames in the frame group %1.").arg(ii + 1);
            return false;
        }
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
                dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
                return false;
            }
        }
        if (ii + 1 == 1) {
            int i = 1 - 1;
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getPixel(28, 108).isTransparent()) {
                return false; // assume it is already done
            }
        }
    }

    bool result = false;
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            bool change = false;
            int dx = 9, dy = -2;

            change |= ShiftFrame(currFrame, dx, dy, 0, 0, width, height);

            // add missing pixels
            if (ii + 1 == 1) {
                if (i + 1 >= 13) {
                    // draw leg
                    change |= currFrame->setPixel(50, 126, D1GfxPixel::colorPixel(174));
                    change |= currFrame->setPixel(51, 126, D1GfxPixel::colorPixel(237));
                    change |= currFrame->setPixel(52, 126, D1GfxPixel::colorPixel(238));
                    change |= currFrame->setPixel(53, 126, D1GfxPixel::colorPixel(237));
                    change |= currFrame->setPixel(54, 126, D1GfxPixel::colorPixel(237));
                    change |= currFrame->setPixel(55, 126, D1GfxPixel::colorPixel(235));
                    change |= currFrame->setPixel(56, 126, D1GfxPixel::colorPixel(234));
                    change |= currFrame->setPixel(57, 126, D1GfxPixel::colorPixel(172));
                    change |= currFrame->setPixel(58, 126, D1GfxPixel::colorPixel(238));
                    change |= currFrame->setPixel(51, 127, D1GfxPixel::colorPixel(174));
                    change |= currFrame->setPixel(52, 127, D1GfxPixel::colorPixel(237));
                    change |= currFrame->setPixel(53, 127, D1GfxPixel::colorPixel(235));
                    change |= currFrame->setPixel(54, 127, D1GfxPixel::colorPixel(238));
                    change |= currFrame->setPixel(55, 127, D1GfxPixel::colorPixel(238));
                }
            }
            if (ii + 1 == 2) {
                if (i + 1 >= 13) {
                    // draw club
                    change |= currFrame->setPixel(71, 126, D1GfxPixel::colorPixel(174));
                    change |= currFrame->setPixel(72, 126, D1GfxPixel::colorPixel(174));
                    change |= currFrame->setPixel(73, 126, D1GfxPixel::colorPixel(237));
                    change |= currFrame->setPixel(74, 126, D1GfxPixel::colorPixel(237));
                    change |= currFrame->setPixel(75, 126, D1GfxPixel::colorPixel(174));
                    change |= currFrame->setPixel(76, 126, D1GfxPixel::colorPixel(188));
                    change |= currFrame->setPixel(77, 126, D1GfxPixel::colorPixel(203));
                    change |= currFrame->setPixel(78, 126, D1GfxPixel::colorPixel(204));
                    change |= currFrame->setPixel(79, 126, D1GfxPixel::colorPixel(204));
                    change |= currFrame->setPixel(80, 126, D1GfxPixel::colorPixel(204));
                    change |= currFrame->setPixel(81, 126, D1GfxPixel::colorPixel(203));
                    change |= currFrame->setPixel(82, 126, D1GfxPixel::colorPixel(203));
                    change |= currFrame->setPixel(83, 126, D1GfxPixel::colorPixel(204));
                    change |= currFrame->setPixel(84, 126, D1GfxPixel::colorPixel(175));
                    change |= currFrame->setPixel(85, 126, D1GfxPixel::colorPixel(174));
                    change |= currFrame->setPixel(86, 126, D1GfxPixel::colorPixel(175));
                    change |= currFrame->setPixel(95, 126, D1GfxPixel::colorPixel(175));
                    change |= currFrame->setPixel(96, 126, D1GfxPixel::colorPixel(174));
                    change |= currFrame->setPixel(97, 126, D1GfxPixel::colorPixel(174));
                    change |= currFrame->setPixel(98, 126, D1GfxPixel::colorPixel(252));
                    change |= currFrame->setPixel(99, 126, D1GfxPixel::colorPixel(204));
                    change |= currFrame->setPixel(100, 126, D1GfxPixel::colorPixel(188));
                    change |= currFrame->setPixel(101, 126, D1GfxPixel::colorPixel(188));
                    change |= currFrame->setPixel(73, 127, D1GfxPixel::colorPixel(174));
                    change |= currFrame->setPixel(74, 127, D1GfxPixel::colorPixel(174));
                    change |= currFrame->setPixel(75, 127, D1GfxPixel::colorPixel(237));
                    change |= currFrame->setPixel(76, 127, D1GfxPixel::colorPixel(174));
                    change |= currFrame->setPixel(77, 127, D1GfxPixel::colorPixel(174));
                    change |= currFrame->setPixel(78, 127, D1GfxPixel::colorPixel(188));
                }
            }
            if (ii + 1 == 3) {
                // add shadow
                if (i + 1 == 6) {
                    change |= currFrame->setPixel(8, 107, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 108, D1GfxPixel::colorPixel(0));
                }
                if (i + 1 == 7) {
                    change |= currFrame->setPixel(7, 107, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 106, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 107, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 108, D1GfxPixel::colorPixel(0));
                }
                if (i + 1 == 8) {
                    change |= currFrame->setPixel(8, 107, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 108, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 109, D1GfxPixel::colorPixel(0));
                }
                if (i + 1 == 9) {
                    change |= currFrame->setPixel(8, 108, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 110, D1GfxPixel::colorPixel(0));
                }
                if (i + 1 >= 13) {
                    // draw club based on frame 1 of group 8
                    for (int y = 123; y < 126; y++) {
                        for (int x = 40; x < 46; x++) {
                            change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }
                    int bn = this->getGroupFrameIndices(8 - 1).first + 1 - 1;
                    D1GfxFrame* baseFrame = this->getFrame(bn);
                    for (int y = 97 + 2; y < 110 + 2 - 2; y++) {
                        for (int x = 83 - 9; x < 103 - 9; x++) {
                            D1GfxPixel pixel = baseFrame->getPixel(x, y);
                            if (pixel.isTransparent())
                                continue;
                            change |= currFrame->setPixel(37 + x - (83 - 9), y + 18, pixel);
                        }
                    }
                }
            }
            if (ii + 1 == 4) {
                // add shadow
                if (i + 1 == 3) {
                    change |= currFrame->setPixel(7, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 101, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 101, D1GfxPixel::colorPixel(0));
                }
                if (i + 1 == 4) {
                    change |= currFrame->setPixel(5, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(5, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 101, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 101, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 101, D1GfxPixel::colorPixel(0));
                }
                if (i + 1 == 5) {
                    change |= currFrame->setPixel(6, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 101, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 101, D1GfxPixel::colorPixel(0));
                }
                if (i + 1 == 6) {
                    change |= currFrame->setPixel(5, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(5, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 97, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 97, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 100, D1GfxPixel::colorPixel(0));
                }
                if (i + 1 == 7) {
                    change |= currFrame->setPixel(6, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 97, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 99, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 97, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 99, D1GfxPixel::colorPixel(0));
                }
                if (i + 1 == 8) {
                    change |= currFrame->setPixel(8, 98, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 99, D1GfxPixel::colorPixel(0));
                }
                if (i + 1 == 10) {
                    change |= currFrame->setPixel(8, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 101, D1GfxPixel::colorPixel(0));
                }
                if (i + 1 == 11) {
                    change |= currFrame->setPixel(8, 100, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 101, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 102, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(6, 103, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 101, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 102, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(7, 103, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 101, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 102, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(8, 103, D1GfxPixel::colorPixel(0));
                }
                // draw club
                if (i + 1 >= 13) {
                    change |= currFrame->setPixel(24, 126, D1GfxPixel::colorPixel(173));
                    change |= currFrame->setPixel(25, 126, D1GfxPixel::colorPixel(173));
                    change |= currFrame->setPixel(26, 126, D1GfxPixel::colorPixel(204));
                    change |= currFrame->setPixel(27, 126, D1GfxPixel::colorPixel(203));
                    change |= currFrame->setPixel(28, 126, D1GfxPixel::colorPixel(203));
                    change |= currFrame->setPixel(29, 126, D1GfxPixel::colorPixel(203));
                    change |= currFrame->setPixel(30, 126, D1GfxPixel::colorPixel(202));
                    change |= currFrame->setPixel(31, 126, D1GfxPixel::colorPixel(203));
                    change |= currFrame->setPixel(32, 126, D1GfxPixel::colorPixel(203));
                    change |= currFrame->setPixel(33, 126, D1GfxPixel::colorPixel(204));
                    change |= currFrame->setPixel(24, 127, D1GfxPixel::colorPixel(254));
                    change |= currFrame->setPixel(25, 127, D1GfxPixel::colorPixel(173));
                    change |= currFrame->setPixel(26, 127, D1GfxPixel::colorPixel(204));
                    change |= currFrame->setPixel(27, 127, D1GfxPixel::colorPixel(204));
                    change |= currFrame->setPixel(28, 127, D1GfxPixel::colorPixel(203));
                    change |= currFrame->setPixel(29, 127, D1GfxPixel::colorPixel(203));
                    change |= currFrame->setPixel(30, 127, D1GfxPixel::colorPixel(203));
                    change |= currFrame->setPixel(31, 127, D1GfxPixel::colorPixel(203));
                    change |= currFrame->setPixel(32, 127, D1GfxPixel::colorPixel(204));
                    change |= currFrame->setPixel(33, 127, D1GfxPixel::colorPixel(204));
                }
            }
            if (ii + 1 == 5) {
                // draw club
                if (i + 1 >= 13) {
                    change |= currFrame->setPixel(8, 107, D1GfxPixel::colorPixel(190));
                    change |= currFrame->setPixel(8, 108, D1GfxPixel::colorPixel(173));
                    change |= currFrame->setPixel(8, 109, D1GfxPixel::colorPixel(253));
                    change |= currFrame->setPixel(8, 110, D1GfxPixel::colorPixel(254));
                    change |= currFrame->setPixel(8, 111, D1GfxPixel::colorPixel(223));
                    change |= currFrame->setPixel(7, 108, D1GfxPixel::colorPixel(223));
                    change |= currFrame->setPixel(7, 109, D1GfxPixel::colorPixel(190));
                }
            }

            if (change) {
                result = true;
                this->setModified();
                if (!silent) {
                    dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(i + 1).arg(ii + 1);
                }
            }
        }
    }

    return result;
}

bool D1Gfx::patchMagmaDie(bool silent)
{
    constexpr int frameCount = 18;
    constexpr int width = 128;
    constexpr int height = 128;

    if (this->getGroupCount() != NUM_DIRS) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }

    QString baseFilePath = this->getFilePath();
    if (baseFilePath.length() < 10) {
        dProgressErr() << tr("Unrecognized file-path. Expected *Magmad.CL2");
        return false;
    }
    // read Fallgn.CL2 from the same folder
    QString stdPath = baseFilePath;
    stdPath[stdPath.length() - 5] = QChar('h');

    if (!QFileInfo::exists(stdPath)) {
        dProgressErr() << tr("Could not find %1 to be used as a template file").arg(QDir::toNativeSeparators(stdPath));
        return false;
    }

    OpenAsParam opParams = OpenAsParam();
    D1Gfx stdGfx;
    stdGfx.setPalette(this->palette);
    if (!D1Cl2::load(stdGfx, stdPath, opParams)) {
        dProgressErr() << (tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(stdPath)));
        return false;
    }
    if (stdGfx.getGroupCount() <= DIR_S) {
        dProgressErr() << tr("Not enough frame groups in '%1'.").arg(QDir::toNativeSeparators(stdPath));
        return false;
    }
    if ((stdGfx.getGroupFrameIndices(DIR_S).second - stdGfx.getGroupFrameIndices(DIR_S).first + 1) < 7) {
        dProgressErr() << tr("Not enough frames in the frame group %1 in '%2'.").arg((int)DIR_S + 1).arg(QDir::toNativeSeparators(stdPath));
        return false;
    }

    for (int ii = 0; ii < NUM_DIRS; ii++) {
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* frame = this->getFrame(n);
            if (frame->getWidth() != width || frame->getHeight() != height) {
                dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
                return false;
            }
        }
        if (ii + 1 == 1) {
            int i = 1 - 1;
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getPixel(51, 51).isTransparent()) {
                return false; // assume it is already done
            }
        }
    }

    bool result = false;
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            bool change = false;
            if (ii + 1 == 1) {
                if (i + 1 <= 3) {
                    int si;
                    switch (i + 1) {
                    case 1: si = 1 - 1; break;
                    case 2: si = 6 - 1; break;
                    case 3: si = 7 - 1; break;
                    }
                    int sn = stdGfx.getGroupFrameIndices(ii).first + si;
                    D1GfxFrame* frameSrcStd = stdGfx.getFrame(sn);
                    if (frameSrcStd->getWidth() != width || frameSrcStd->getHeight() != height) {
                        dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(stdPath)).arg(width).arg(height);
                        return result;
                    }

                    for (int y = 0; y < height; y++) {
                        for (int x = 0; x < width; x++) {
                            // preserve pixels
                            if (i + 1 == 3) {
                                if (y < 51 || (x > 65 && y < 55) || (x >= 57 && x < 60 && y < 54 + 57 - x) || (x == 56 && y == 52))
                                    continue;
                            }
                            if (i + 1 == 2) {
                                if (x >= 72 && y < 72 && y < 64 + x - 72) {
                                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                                    if (!pixel.isTransparent()) {
                                        change |= currFrame->setPixel(x - 16, y, pixel);
                                        change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                                        continue;
                                    }
                                }
                            }
                            // copy pixels with 'trn'
                            D1GfxPixel pixel = frameSrcStd->getPixel(x, y);
                            if (!pixel.isTransparent()) {
                                quint8 color = pixel.getPaletteIndex();
                                if (color != 0) {
                                    if (color < 188)
                                        pixel = D1GfxPixel::colorPixel(color - 1);
                                    else if (color >= 214 && color <= 217)
                                        pixel = D1GfxPixel::colorPixel(155);
                                    else if (color >= 218 && color <= 220)
                                        pixel = D1GfxPixel::colorPixel(140 + color - 218);
                                    else if (color >= 221 && color <= 222)
                                        pixel = D1GfxPixel::colorPixel(142);
                                    else if (color >= 232 && color <= 233)
                                        pixel = D1GfxPixel::colorPixel(154);
                                    else if (color >= 234 && color <= 235)
                                        pixel = D1GfxPixel::colorPixel(156);
                                    else if (color >= 236 && color <= 239)
                                        pixel = D1GfxPixel::colorPixel(color - 4);
                                }
                            }
                            change |= currFrame->setPixel(x, y, pixel);
                        }
                    }

                    if (i + 1 == 3) {
                        // add bits from frame 4
                        D1GfxFrame* baseFrame = this->getFrame(n + 1);
                        for (int y = 62; y < 73; y++) {
                            for (int x = 93; x < 106; x++) {
                                D1GfxPixel pixel = baseFrame->getPixel(x, y);
                                if (pixel.isTransparent())
                                    continue;
                                int dx = 59 - 93, dy = 88 - 62;
                                D1GfxPixel currPixel = currFrame->getPixel(x + dx, y + dy);
                                if (currPixel.isTransparent() || currPixel.getPaletteIndex() == 0)
                                    change |= currFrame->setPixel(x + dx, y + dy, pixel);
                            }
                        }
                        for (int y = 79; y < 88; y++) {
                            for (int x = 88; x < 98; x++) {
                                D1GfxPixel pixel = baseFrame->getPixel(x, y);
                                if (pixel.isTransparent())
                                    continue;
                                int dx = 56 - 88, dy = 95 - 79;
                                D1GfxPixel currPixel = currFrame->getPixel(x + dx, y + dy);
                                if (currPixel.isTransparent() || currPixel.getPaletteIndex() == 0)
                                    change |= currFrame->setPixel(x + dx, y + dy, pixel);
                            }
                        }
                    }
                }
            } else {
                if (i + 1 == 1) {
                    // draw leg
                    int si = 1 - 1;
                    int sn = stdGfx.getGroupFrameIndices(ii).first + si;
                    D1GfxFrame* frameSrcStd = stdGfx.getFrame(sn);
                    if (frameSrcStd->getWidth() != width || frameSrcStd->getHeight() != height) {
                        dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(stdPath)).arg(width).arg(height);
                        return result;
                    }
                    int sx = 0, sy = 0, ex = 0, ey = 0;
                    switch (ii + 1) {
                    case 2: sx = 53; sy = 87; ex = 62, ey = 97; break;
                    case 3: sx = 69; sy = 94; ex = 72, ey = 101; break;
                    case 4: sx = 66; sy = 80; ex = 79, ey = 92; break;
                    case 5: sx = 56; sy = 83; ex = 78, ey = 115; break;
                    case 6: sx = 56; sy = 89; ex = 76, ey = 113; break;
                    case 7: sx = 66; sy = 94; ex = 69, ey = 101; break;
                    case 8: sx = 39; sy = 80; ex = 62, ey = 114; break;
                    }
                    for (int y = sy; y < ey; y++) {
                        for (int x = sx; x < ex; x++) {
                            D1GfxPixel pixel = frameSrcStd->getPixel(x, y);
                            if (pixel.isTransparent())
                                continue;
                            int dx = 0, dy = 0;
                            D1GfxPixel currPixel = currFrame->getPixel(x + dx, y + dy);
                            if (currPixel.isTransparent() || currPixel.getPaletteIndex() == 0) {
                                change |= currFrame->setPixel(x + dx, y + dy, pixel);
                            }
                        }
                    }
                }

                switch (n + 1) {
                case 19:
                    change |= currFrame->setPixel(54, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(41, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(42, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(43, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(44, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(45, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(43, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(44, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(45, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 117, D1GfxPixel::transparentPixel());
                    break;
                case 20:
                    change |= currFrame->setPixel(40, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(41, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(41, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(42, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(42, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(43, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(43, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(44, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(45, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(44, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(45, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(41, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(42, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(43, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(44, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(45, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(43, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(44, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(45, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 117, D1GfxPixel::transparentPixel());
                    break;
                case 21:
                    change |= currFrame->setPixel(53, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(41, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(42, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(43, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(44, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(45, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(43, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(44, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(45, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 117, D1GfxPixel::transparentPixel());
                    break;
                case 37:
                    change |= currFrame->setPixel(49, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 115, D1GfxPixel::transparentPixel());
                    break;
                case 38:
                    change |= currFrame->setPixel(53, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 115, D1GfxPixel::transparentPixel());
                    break;
                case 39:
                    change |= currFrame->setPixel(50, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 115, D1GfxPixel::transparentPixel());
                    break;
                case 55:
                    change |= currFrame->setPixel(68, 102, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(70, 103, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(71, 103, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(61, 108, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 108, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 108, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 108, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 108, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(61, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(63, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(60, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 114, D1GfxPixel::transparentPixel());
                    break;
                case 56:
                    change |= currFrame->setPixel(65, 107, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(64, 108, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 108, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(68, 108, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(69, 108, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(63, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(71, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(62, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 114, D1GfxPixel::transparentPixel());
                    break;
                case 57:
                    change |= currFrame->setPixel(69, 108, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(63, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(60, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(63, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 114, D1GfxPixel::transparentPixel());
                    break;
                case 58:
                    change |= currFrame->setPixel(71, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(76, 114, D1GfxPixel::transparentPixel());
                    break;
                case 91:
                    change |= currFrame->setPixel(45, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(44, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(45, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(45, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(58, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(59, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(60, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(58, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(59, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 115, D1GfxPixel::transparentPixel());
                    break;
                case 92:
                    change |= currFrame->setPixel(59, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(60, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(61, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(62, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(59, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(60, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(61, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(62, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(63, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(64, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(46, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(60, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(61, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(62, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(63, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(64, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(65, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(47, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(59, 112, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(60, 112, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(61, 112, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(62, 112, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(63, 112, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(47, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(58, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(47, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(58, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(59, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 115, D1GfxPixel::transparentPixel());
                    break;
                case 93:
                    change |= currFrame->setPixel(45, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(63, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(64, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(65, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(66, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(67, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(44, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(56, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(57, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(58, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(59, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(60, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(61, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(62, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(63, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(64, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(65, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(66, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(67, 110, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(45, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(46, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(58, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(59, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(60, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(61, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(62, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(63, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(64, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(47, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 112, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(62, 112, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(63, 112, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(47, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(58, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(59, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(60, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 113, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(47, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(54, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(55, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(58, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(59, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(48, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(49, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(50, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(51, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(52, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(53, 115, D1GfxPixel::transparentPixel());
                    break;
                case 127:
                    change |= currFrame->setPixel(67, 105, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(67, 106, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(67, 107, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(67, 108, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(68, 108, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(69, 108, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(66, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 109, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 110, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(56, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(57, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(58, 111, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(60, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(57, 112, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(60, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(76, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(76, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(77, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(78, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(79, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(80, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(76, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(77, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(78, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(79, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(80, 117, D1GfxPixel::transparentPixel());
                    break;
                case 128:
                    change |= currFrame->setPixel(67, 107, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(67, 108, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(68, 108, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(69, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(70, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(71, 109, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(62, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 111, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(60, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 112, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(59, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(60, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(61, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(62, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(63, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 113, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(58, 114, D1GfxPixel::colorPixel(0));
                    change |= currFrame->setPixel(63, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 114, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(64, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(65, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(66, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(67, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(68, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(69, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(70, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(71, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(76, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(77, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(78, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(79, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(80, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(76, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(77, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(78, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(79, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(80, 117, D1GfxPixel::transparentPixel());
                    break;
                case 129:
                    change |= currFrame->setPixel(73, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(76, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(76, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(77, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(78, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(79, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(80, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(76, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(77, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(78, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(79, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(80, 117, D1GfxPixel::transparentPixel());
                    break;
                case 130:
                    change |= currFrame->setPixel(73, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(76, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(77, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(78, 115, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(76, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(77, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(78, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(79, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(80, 116, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(72, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(73, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(74, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(75, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(76, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(77, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(78, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(79, 117, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(80, 117, D1GfxPixel::transparentPixel());
                    break;
                }
            }

            if (change) {
                result = true;
                this->setModified();
                if (!silent) {
                    dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(i + 1).arg(ii + 1);
                }
            }
        }
    }
    return result;
}

bool D1Gfx::patchFallGWalk(bool silent)
{
    QString baseFilePath = this->getFilePath();
    if (baseFilePath.length() < 10) {
        dProgressErr() << tr("Unrecognized file-path. Expected *Fallgw.CL2");
        return false;
    }
    // read Fallgn.CL2 from the same folder
    QString stdPath = baseFilePath;
    stdPath[stdPath.length() - 5] = QChar('n');

    if (!QFileInfo::exists(stdPath)) {
        dProgressErr() << tr("Could not find %1 to be used as a template file").arg(QDir::toNativeSeparators(stdPath));
        return false;
    }

    OpenAsParam opParams = OpenAsParam();
    D1Gfx stdGfx;
    stdGfx.setPalette(this->palette);
    if (!D1Cl2::load(stdGfx, stdPath, opParams)) {
        dProgressErr() << (tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(stdPath)));
        return false;
    }

    constexpr int frameCount = 8;
    constexpr int width = 128;
    constexpr int height = 128;

    if (this->getGroupCount() <= DIR_E || this->getGroupCount() <= DIR_W) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    if ((this->getGroupFrameIndices(DIR_E).second - this->getGroupFrameIndices(DIR_E).first + 1) != frameCount) {
        dProgressErr() << tr("Not enough frames in the frame group %1.").arg((int)DIR_E + 1);
        return false;
    }
    if ((this->getGroupFrameIndices(DIR_W).second - this->getGroupFrameIndices(DIR_W).first + 1) != frameCount) {
        dProgressErr() << tr("Not enough frames in the frame group %1.").arg((int)DIR_W + 1);
        return false;
    }
    if (stdGfx.getGroupCount() <= DIR_E) {
        dProgressErr() << tr("Not enough frame groups in '%1'.").arg(QDir::toNativeSeparators(stdPath));
        return false;
    }
    if ((stdGfx.getGroupFrameIndices(DIR_E).second - stdGfx.getGroupFrameIndices(DIR_E).first + 1) < 10) {
        dProgressErr() << tr("Not enough frames in the frame group %1 in '%2'.").arg((int)DIR_E + 1).arg(QDir::toNativeSeparators(stdPath));
        return false;
    }
    // prepare a 'work'-frame
    D1GfxFrame* frame = new D1GfxFrame();
    for (int y = 0; y < height; y++) {
        std::vector<D1GfxPixel> pixelLine;
        for (int x = 0; x < width; x++) {
            pixelLine.push_back(D1GfxPixel::transparentPixel());
        }
        frame->addPixelLine(std::move(pixelLine));
    }
    bool result = false;
    for (int i = 0; i < frameCount; i++) {
        int n = this->getGroupFrameIndices(DIR_E).first + i;
        D1GfxFrame* currFrame = this->getFrame(n);
        if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
            dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
            break;
        }
        D1GfxFrame* walkWestFrame = this->getFrame(this->getGroupFrameIndices(DIR_W).first + i);
        if (walkWestFrame->getWidth() != width || walkWestFrame->getHeight() != height) {
            dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
            break;
        }
        bool change = false;
        // duplicate the current frame
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                frame->setPixel(x, y, currFrame->getPixel(x, y));
            }
        }
        // mirror the west-walk frame
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                D1GfxPixel wPixel = walkWestFrame->getPixel(width - x - 1, y);
                quint8 color = wPixel.getPaletteIndex();
                if (!wPixel.isTransparent()) {
                    if ((color >= 170 && color <= 175) || (color >= 190 && color <= 205) || color >= 251) {
                        if (i == 0) {
                            if (x >= 71 && y >= 99 && y <= 112) {
                                continue;
                            }
                        }
                        if (i == 1) {
                            if (x >= 67 && y >= 101 && y <= 110) {
                                continue;
                            }
                        }
                        if (i == 2) {
                            if (x >= 62 && y >= 105 && y <= 114) {
                                continue;
                            }
                        }
                        if (i == 3) {
                            if (x >= 58 && y >= 109 && y <= 118) {
                                continue;
                            }
                        }
                        if (i == 4) {
                            if (x >= 57 && y >= 110 && y <= 121) {
                                continue;
                            }
                        }
                        if (i == 5) {
                            if (x >= 58 && y >= 110 && y <= 119) {
                                continue;
                            }
                        }
                        if (i == 6) {
                            if (x >= 62 && y >= 106 && y <= 114) {
                                continue;
                            }
                        }
                        if (i == 7) {
                            if (x >= 66 && y >= 96 && y <= 112) {
                                continue;
                            }
                        }
                        /*if (x >= 62 && y >= 99 && y <= 121) {
                            continue;
                        }*/
                    }
                    if (color == 0) {
                        if (i == 0) {
                            if (/*x >= 87 || */(x >= 81 && y >= 118 + 10 - x / 8)) {
                                // wPixel = D1GfxPixel::transparentPixel();
                                wPixel = D1GfxPixel::transparentPixel();
                            }
                        }
                        if (i == 1) {
                            if (/*x >= 87 || */(x >= 79 && y >= 118 + 10 - x / 8)) {
                                wPixel = D1GfxPixel::transparentPixel();
                            }
                        }
                        if (i == 2) {
                            if (/*x >= 87 || */(x >= 75 && y >= 118 + 10 - x / 8)) {
                                wPixel = D1GfxPixel::transparentPixel();
                            }
                        }
                        if (i == 3) {
                            if (/*x >= 87 || */(x >= 70 && y >= 118)) {
                                wPixel = D1GfxPixel::transparentPixel();
                            }
                        }
                        if (i == 4) {
                            if (/*x >= 87 || */(x >= 67 && y >= 119)) {
                                wPixel = D1GfxPixel::transparentPixel();
                            }
                        }
                        if (i == 5) {
                            if (/*x >= 87 || */(x >= 71 && y >= 119)) {
                                wPixel = D1GfxPixel::transparentPixel();
                            }
                        }
                        if (i == 6) {
                            if (/*x >= 87 || */(x >= 72 && y >= 117 && y >= 120 + 72 - x)) {
                                wPixel = D1GfxPixel::transparentPixel();
                            }
                        }
                        if (i == 7) {
                            if (/*x >= 87 || */(x >= 85 && y <= 118 && y >= 118 + 85 - x)) {
                                wPixel = D1GfxPixel::transparentPixel();
                            }
                        }
                        /*if (x >= 65 && y >= 115 && y <= 122) {
                            continue;
                        }*/
                    }
                }
                change |= frame->setPixel(x, y, wPixel);
            }
        }

        // copy the club from the stand frame
        int fn, dx, dy;
        switch (i) {
        case 0: fn = 8; dx = -11; dy = 11; break;
        case 1: fn = 9; dx =  -8; dy =  9; break;
        case 2: fn = 9; dx =  -7; dy =  9; break;
        case 3: fn = 9; dx =  -5; dy =  9; break;
        case 4: fn = 9; dx =  -4; dy =  9; break;
        case 5: fn = 9; dx =  -5; dy =  8; break;
        case 6: fn = 8; dx =  -2; dy =  5; break;
        case 7: fn = 9; dx =  -8; dy =  8; break;
        }
        D1GfxFrame* stdEastFrame = stdGfx.getFrame(stdGfx.getGroupFrameIndices(DIR_E).first + fn);
        if (stdEastFrame->getWidth() != width || stdEastFrame->getHeight() != height) {
            dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(stdPath)).arg(width).arg(height);
            break;
        }

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                D1GfxPixel sPixel = stdEastFrame->getPixel(x, y);
                if (sPixel.isTransparent())
                    continue;
                quint8 color = sPixel.getPaletteIndex();
                switch (fn) {
                case 7:
                    if (x < 60 || y < 88 || y > 111 || (color != 0 && !(color >= 170 && color <= 175) && !(color >= 188 && color <= 205) && color != 223 && color != 251 && color != 252))
                        continue;
                    break;
                case 8:
                case 9:
                    if (x < 80 || y < 86 || y > 109 || (color != 0 && !(color >= 170 && color <= 175) && !(color >= 188 && color <= 205) && color != 223 && color != 251 && color != 252))
                        continue;
                    break;
                }
                D1GfxPixel wPixel = frame->getPixel(x + dx, y + dy);
                color = wPixel.getPaletteIndex();
                if (wPixel.isTransparent()
                    || (i == 0 && color == 0 && x >= 75 + 11)) {
                    change |= frame->setPixel(x + dx, y + dy, sPixel);
                }
            }
        }

        // fix artifacts
        switch (i) {
        case 0: dx = 85; break;
        case 1: dx = 92; break;
        case 2: dx = 93; break;
        case 3: dx = 95; break;
        case 4: dx = 96; break;
        case 5: dx = 95; break;
        case 6: dx = 94; break;
        case 7: dx = 93; break;
        }
        for (int y = 85; y < height; y++) {
            for (int x = dx; x < width; x++) {
                change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
            }
        }
        if (i == 0) {
            change |= frame->setPixel(76, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 110, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 110, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(78, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(82, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(83, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(82, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(83, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(69, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(70, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 118, D1GfxPixel::colorPixel(0)); // was tp
        }
        if (i == 1) {
            for (int y = 106; y < 114; y++) {
                for (int x = 73; x < 86; x++) {
                    if (y < 276 - 2 * x) {
                        change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(0));
                    }
                }
            }
            for (int y = 105; y < 109; y++) {
                for (int x = 84; x < 88; x++) {
                    if (x != 87 || (y != 105 || y != 108)) {
                        change |= frame->setPixel(x, y, D1GfxPixel::colorPixel(0));
                    }
                }
            }
            change |= frame->setPixel(73, 100, D1GfxPixel::colorPixel(175)); // was tp
            change |= frame->setPixel(74, 100, D1GfxPixel::colorPixel(175)); // was tp
            change |= frame->setPixel(91, 102, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 103, D1GfxPixel::transparentPixel()); // color191)
            change |= frame->setPixel(84, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(82, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(83, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 109, D1GfxPixel::transparentPixel()); // color207)
            change |= frame->setPixel(82, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(82, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(83, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(82, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(83, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(76, 118, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(77, 118, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(78, 118, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(79, 118, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(74, 119, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(75, 119, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(76, 119, D1GfxPixel::transparentPixel()); // color0)
        }
        if (i == 2) {
            change |= frame->setPixel(71, 96, D1GfxPixel::colorPixel(238)); // was color191)
            change |= frame->setPixel(71, 97, D1GfxPixel::colorPixel(237)); // was color191)
            change |= frame->setPixel(72, 97, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(73, 97, D1GfxPixel::colorPixel(237)); // was color191)
            change |= frame->setPixel(75, 97, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(76, 97, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(71, 98, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(72, 98, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(73, 98, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(74, 98, D1GfxPixel::colorPixel(236)); // was color203)
            change |= frame->setPixel(70, 99, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(71, 99, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(72, 99, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(73, 99, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(74, 99, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(70, 100, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(71, 100, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(72, 100, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(73, 100, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(74, 100, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(75, 100, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(69, 101, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(70, 101, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(71, 101, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(72, 101, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(73, 101, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(74, 101, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(75, 101, D1GfxPixel::colorPixel(173)); // was tp
            change |= frame->setPixel(76, 101, D1GfxPixel::colorPixel(173)); // was tp
            change |= frame->setPixel(69, 102, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(70, 102, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(71, 102, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(72, 102, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(73, 102, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(74, 102, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(70, 103, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(71, 103, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(72, 103, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(92, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(70, 104, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(71, 104, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(72, 104, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(73, 104, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(84, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(70, 105, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(71, 105, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(72, 105, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(73, 105, D1GfxPixel::colorPixel(238)); // was color170)
            change |= frame->setPixel(85, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(70, 106, D1GfxPixel::colorPixel(236)); // was color238)
            change |= frame->setPixel(71, 106, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(72, 106, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(73, 106, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(92, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(71, 107, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(72, 107, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(73, 107, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 107, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 107, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 107, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(68, 108, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(69, 108, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(70, 108, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(71, 108, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(73, 108, D1GfxPixel::colorPixel(0)); // was color191)
            change |= frame->setPixel(76, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(68, 109, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(69, 109, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(70, 109, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(73, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(74, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(88, 109, D1GfxPixel::transparentPixel()); // color207)
            change |= frame->setPixel(73, 110, D1GfxPixel::colorPixel(0)); // was color189)
            change |= frame->setPixel(74, 110, D1GfxPixel::colorPixel(0)); // was color189)
            change |= frame->setPixel(75, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(86, 110, D1GfxPixel::transparentPixel()); // color223)
            change |= frame->setPixel(71, 111, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(83, 111, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(71, 112, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(72, 112, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(73, 112, D1GfxPixel::colorPixel(238)); // was color0)
            change |= frame->setPixel(74, 112, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(81, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(70, 113, D1GfxPixel::colorPixel(236)); // was color0)
            change |= frame->setPixel(71, 113, D1GfxPixel::colorPixel(236)); // was color0)
            change |= frame->setPixel(72, 113, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(73, 113, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(83, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(89, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(73, 114, D1GfxPixel::colorPixel(236)); // was color0)
            change |= frame->setPixel(85, 114, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(86, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(75, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(79, 118, D1GfxPixel::transparentPixel()); // color0)
        }
        if (i == 3) { // 52
            change |= frame->setPixel(68, 87, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(69, 87, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(70, 88, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(75, 88, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(70, 89, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(71, 89, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(72, 90, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(72, 91, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(76, 91, D1GfxPixel::colorPixel(175)); // was color237)
            change |= frame->setPixel(77, 91, D1GfxPixel::transparentPixel()); // color237)
            change |= frame->setPixel(77, 92, D1GfxPixel::colorPixel(175)); // was tp
            change |= frame->setPixel(76, 94, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(76, 95, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(77, 95, D1GfxPixel::colorPixel(175)); // was tp
            change |= frame->setPixel(66, 96, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(76, 96, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(77, 96, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(78, 96, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(66, 97, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(76, 97, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(77, 97, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(78, 97, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(66, 98, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(75, 98, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(73, 99, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(74, 99, D1GfxPixel::colorPixel(174)); // was tp
            change |= frame->setPixel(75, 99, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(76, 99, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(77, 99, D1GfxPixel::colorPixel(203)); // was color170)
            change |= frame->setPixel(73, 100, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(74, 100, D1GfxPixel::colorPixel(174)); // was tp
            change |= frame->setPixel(75, 100, D1GfxPixel::colorPixel(174)); // was tp
            change |= frame->setPixel(76, 100, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(77, 100, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(67, 101, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(73, 101, D1GfxPixel::colorPixel(174)); // was tp
            change |= frame->setPixel(80, 101, D1GfxPixel::colorPixel(175)); // was tp
            change |= frame->setPixel(81, 101, D1GfxPixel::colorPixel(172)); // was tp
            change |= frame->setPixel(67, 102, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(73, 102, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(66, 103, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(67, 103, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(68, 103, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(73, 103, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(75, 103, D1GfxPixel::transparentPixel()); // color189)
            change |= frame->setPixel(76, 103, D1GfxPixel::transparentPixel()); // color188)
            change |= frame->setPixel(86, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(66, 104, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(67, 104, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(68, 104, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(69, 104, D1GfxPixel::colorPixel(235)); // was color207)
            change |= frame->setPixel(81, 104, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(66, 105, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(67, 105, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(68, 105, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(72, 105, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(75, 105, D1GfxPixel::transparentPixel()); // color170)
            change |= frame->setPixel(84, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(66, 106, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(67, 106, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(68, 106, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(72, 106, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(73, 106, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(85, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(66, 107, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(67, 107, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(68, 107, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(72, 107, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(85, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(66, 108, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(67, 108, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(68, 108, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(84, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(67, 109, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(68, 109, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(71, 109, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(83, 109, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 109, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(68, 110, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(72, 110, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(76, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 110, D1GfxPixel::colorPixel(0)); // was 235
            change |= frame->setPixel(80, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(71, 111, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(72, 111, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(73, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(74, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(69, 112, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(73, 112, D1GfxPixel::colorPixel(237)); // was color0
            change |= frame->setPixel(84, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(86, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(68, 113, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(73, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(74, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(88, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(72, 114, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 114, D1GfxPixel::colorPixel(0)); // was color188
            change |= frame->setPixel(88, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(72, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(70, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(71, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(72, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(70, 118, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(71, 118, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(72, 118, D1GfxPixel::colorPixel(0)); // was tp
        }
        if (i == 4) { // 53
            change |= frame->setPixel(72, 89, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(73, 90, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(73, 91, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(74, 91, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(74, 92, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(75, 94, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(75, 95, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(77, 95, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(78, 95, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(79, 95, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(76, 96, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(77, 96, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(78, 96, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(79, 96, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(80, 96, D1GfxPixel::colorPixel(234)); // was tp
            change |= frame->setPixel(77, 97, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(78, 97, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(79, 97, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(65, 98, D1GfxPixel::colorPixel(238)); // was color190)
            change |= frame->setPixel(66, 98, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(67, 98, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(76, 98, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(65, 99, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(66, 99, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(67, 99, D1GfxPixel::colorPixel(238)); // was tp
            change |= frame->setPixel(76, 99, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(77, 99, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(66, 100, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(67, 100, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(75, 100, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(76, 100, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(77, 100, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(78, 100, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(65, 101, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(66, 101, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(74, 101, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(75, 101, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(76, 101, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(75, 102, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(75, 103, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(77, 103, D1GfxPixel::transparentPixel()); // color188)
            change |= frame->setPixel(75, 104, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(85, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(95, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(75, 105, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(83, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(95, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(75, 106, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(83, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(95, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(74, 107, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(84, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(95, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(75, 108, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(76, 108, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(86, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(75, 109, D1GfxPixel::colorPixel(236)); // was color0)
            change |= frame->setPixel(76, 109, D1GfxPixel::colorPixel(236)); // was color0)
            change |= frame->setPixel(86, 109, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 109, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 109, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 109, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 109, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 109, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(76, 110, D1GfxPixel::colorPixel(219)); // was color0)
            change |= frame->setPixel(77, 110, D1GfxPixel::colorPixel(236)); // was color0)
            change |= frame->setPixel(78, 110, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(77, 111, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(78, 111, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(79, 111, D1GfxPixel::colorPixel(236)); // was color0)
            change |= frame->setPixel(80, 111, D1GfxPixel::colorPixel(237)); // was color188)
            change |= frame->setPixel(81, 111, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(83, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 111, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(77, 112, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(79, 112, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(82, 112, D1GfxPixel::colorPixel(236)); // was color219)
            change |= frame->setPixel(83, 112, D1GfxPixel::colorPixel(237)); // was color219)
            change |= frame->setPixel(85, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(64, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(65, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(66, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(67, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(68, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(69, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(70, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(71, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(69, 114, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(70, 114, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(71, 114, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(72, 114, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(73, 114, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(74, 114, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 114, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(87, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(72, 115, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(73, 115, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(74, 115, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 115, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 115, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 115, D1GfxPixel::colorPixel(0)); // was color172)
            change |= frame->setPixel(89, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(60, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(61, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(62, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(71, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(72, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(73, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(74, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 116, D1GfxPixel::colorPixel(0)); // was color172)
            change |= frame->setPixel(89, 116, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(69, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(70, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(71, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(72, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(73, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(74, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(80, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(67, 118, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(68, 118, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(69, 118, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(70, 118, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(71, 118, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 118, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(76, 118, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(77, 118, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 118, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 118, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(56, 120, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 120, D1GfxPixel::transparentPixel()); // color223)
        }
        if (i == 5) { // 54
            change |= frame->setPixel(74, 94, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(73, 95, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(74, 95, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(75, 95, D1GfxPixel::colorPixel(173)); // was tp
            change |= frame->setPixel(76, 95, D1GfxPixel::colorPixel(172)); // was tp
            change |= frame->setPixel(77, 95, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(76, 96, D1GfxPixel::colorPixel(174)); // was tp
            change |= frame->setPixel(77, 96, D1GfxPixel::colorPixel(173)); // was tp
            change |= frame->setPixel(78, 96, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(74, 97, D1GfxPixel::colorPixel(172)); // was color235)
            change |= frame->setPixel(75, 97, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(75, 98, D1GfxPixel::colorPixel(172)); // was tp
            change |= frame->setPixel(76, 98, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(73, 99, D1GfxPixel::colorPixel(237)); // was color0)
            change |= frame->setPixel(75, 99, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(76, 99, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(77, 99, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(73, 100, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(74, 100, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(75, 100, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(76, 100, D1GfxPixel::colorPixel(172)); // was tp
            change |= frame->setPixel(77, 100, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(73, 101, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(74, 101, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(75, 101, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(73, 102, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(74, 102, D1GfxPixel::colorPixel(220)); // was tp
            change |= frame->setPixel(92, 102, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(73, 103, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(74, 103, D1GfxPixel::colorPixel(220)); // was tp
            change |= frame->setPixel(75, 103, D1GfxPixel::colorPixel(172)); // was tp
            change |= frame->setPixel(87, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(73, 104, D1GfxPixel::colorPixel(220)); // was tp
            change |= frame->setPixel(74, 104, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(85, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(73, 105, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(74, 105, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(75, 105, D1GfxPixel::colorPixel(172)); // was tp
            change |= frame->setPixel(86, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(72, 106, D1GfxPixel::colorPixel(236)); // was color191)
            change |= frame->setPixel(73, 106, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(87, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(94, 108, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(69, 110, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(70, 110, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(72, 110, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(87, 110, D1GfxPixel::transparentPixel()); // color188)
            change |= frame->setPixel(70, 111, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(71, 111, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(72, 111, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(73, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(74, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(68, 112, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(73, 112, D1GfxPixel::colorPixel(236)); // was color0)
            change |= frame->setPixel(82, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(86, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(87, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(69, 113, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(70, 113, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(72, 113, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(73, 113, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(74, 113, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(75, 113, D1GfxPixel::colorPixel(236)); // was color0)
            change |= frame->setPixel(77, 113, D1GfxPixel::colorPixel(236)); // was color0)
            change |= frame->setPixel(89, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(69, 114, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(74, 114, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(75, 114, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(89, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(73, 115, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(88, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 115, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(81, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(90, 117, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 118, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 118, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 118, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 118, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(60, 119, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(57, 120, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(58, 120, D1GfxPixel::colorPixel(0)); // was tp
        }
        if (i == 6) { // 55
            change |= frame->setPixel(70, 90, D1GfxPixel::colorPixel(173)); // was color237)
            change |= frame->setPixel(71, 91, D1GfxPixel::colorPixel(235)); // was color237)
            change |= frame->setPixel(72, 91, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(73, 92, D1GfxPixel::colorPixel(235)); // was tp
            change |= frame->setPixel(71, 93, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(72, 93, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(73, 93, D1GfxPixel::colorPixel(173)); // was tp
            change |= frame->setPixel(74, 93, D1GfxPixel::colorPixel(173)); // was tp
            change |= frame->setPixel(72, 94, D1GfxPixel::colorPixel(173)); // was tp
            change |= frame->setPixel(73, 94, D1GfxPixel::colorPixel(173)); // was tp
            change |= frame->setPixel(74, 94, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 94, D1GfxPixel::colorPixel(173)); // was tp
            change |= frame->setPixel(72, 95, D1GfxPixel::colorPixel(221)); // was tp
            change |= frame->setPixel(73, 95, D1GfxPixel::colorPixel(173)); // was tp
            change |= frame->setPixel(74, 95, D1GfxPixel::colorPixel(173)); // was tp
            change |= frame->setPixel(75, 95, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(76, 95, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(77, 95, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(78, 95, D1GfxPixel::colorPixel(252)); // was tp
            change |= frame->setPixel(73, 96, D1GfxPixel::colorPixel(173)); // was tp
            change |= frame->setPixel(74, 96, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(75, 96, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(76, 96, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(77, 96, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(78, 96, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(79, 96, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(80, 96, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(81, 96, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(74, 97, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(75, 97, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(76, 97, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(77, 97, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(78, 97, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(79, 97, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(80, 97, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(74, 98, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 98, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(76, 98, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(77, 98, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(78, 98, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(79, 98, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(75, 99, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 99, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 99, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(78, 99, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(79, 99, D1GfxPixel::colorPixel(203)); // was tp
            change |= frame->setPixel(77, 100, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 100, D1GfxPixel::colorPixel(171)); // was tp
            change |= frame->setPixel(77, 101, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(93, 102, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(70, 104, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(71, 104, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(72, 104, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(84, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(69, 105, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(70, 105, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(83, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(68, 106, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(69, 106, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(83, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 106, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(68, 107, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(83, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 107, D1GfxPixel::transparentPixel()); // color191)
            change |= frame->setPixel(93, 107, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(61, 108, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(62, 108, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(67, 108, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(80, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(59, 109, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(68, 109, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(69, 109, D1GfxPixel::colorPixel(236)); // was tp
            change |= frame->setPixel(72, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(73, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(74, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(71, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(72, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(73, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(74, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 110, D1GfxPixel::colorPixel(0)); // was color171)
            change |= frame->setPixel(79, 110, D1GfxPixel::colorPixel(0)); // was color188)
            change |= frame->setPixel(80, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(88, 110, D1GfxPixel::transparentPixel()); // color207)
            change |= frame->setPixel(77, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(86, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(88, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 112, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(76, 113, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(88, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 113, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(75, 114, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(90, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(93, 114, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(66, 115, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(67, 115, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(83, 115, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 115, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 115, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(86, 115, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(87, 115, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(88, 115, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(64, 116, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(83, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(86, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(87, 116, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(73, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(74, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(86, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(87, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(64, 118, D1GfxPixel::colorPixel(237)); // was tp
            change |= frame->setPixel(84, 118, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 118, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(86, 118, D1GfxPixel::colorPixel(0)); // was tp
        }
        if (i == 7) { // 56
            change |= frame->setPixel(90, 102, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 102, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 102, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(90, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(91, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 103, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(88, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(89, 104, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(92, 104, D1GfxPixel::transparentPixel()); // color191)
            change |= frame->setPixel(83, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(84, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(85, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(86, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(87, 105, D1GfxPixel::transparentPixel()); // color0)
            change |= frame->setPixel(82, 107, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 107, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 108, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(78, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 109, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(75, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(76, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(77, 110, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(79, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 111, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(80, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(81, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(82, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(83, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 112, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(86, 113, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(86, 114, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 117, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 118, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(85, 119, D1GfxPixel::colorPixel(0)); // was tp
            change |= frame->setPixel(84, 120, D1GfxPixel::colorPixel(0)); // was tp
        }

        // check for change
        change = false;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                change |= currFrame->setPixel(x, y, frame->getPixel(x, y));
            }
        }

        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(n + 1).arg(DIR_E + 1);
            }
        }
    }
    delete frame;
    return result;
}

bool D1Gfx::patchGoatBDie(bool silent)
{
    constexpr int frameCount = 20;
    constexpr int width = 128;
    constexpr int height = 128;

    if (this->getGroupCount() != NUM_DIRS) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        if ((this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first + 1) < frameCount) {
            dProgressErr() << tr("Not enough frames in the frame group %1.").arg(ii + 1);
            return false;
        }
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
                dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
                return false;
            }
        }
        if (ii + 1 == 1) {
            int i = 4 - 1;
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getPixel(50, 126).isTransparent()) {
                return false; // assume it is already done
            }
        }
    }

    bool result = false;
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            bool change = false;

            // fix bouncying bow
            if (i + 1 == 4) {
                change |= ShiftFrame(currFrame, -1, -3, 0, 106, width, height);
            }

            if (change) {
                result = true;
                this->setModified();
                if (!silent) {
                    dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(i + 1).arg(ii + 1);
                }
            }
        }
    }

    return result;
}

/*bool D1Gfx::patchGoatLDieDio(bool silent)
{
    constexpr int frameCount = 16;
    constexpr int width = 160;
    constexpr int height = 160;

    if (this->getGroupCount() <= DIR_NW || this->getGroupCount() <= DIR_NE) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    if ((this->getGroupFrameIndices(DIR_NW).second - this->getGroupFrameIndices(DIR_NW).first + 1) != frameCount) {
        dProgressErr() << tr("Not enough frames in the frame group to North-West.");
        return false;
    }
    if ((this->getGroupFrameIndices(DIR_NE).second - this->getGroupFrameIndices(DIR_NE).first + 1) != frameCount) {
        dProgressErr() << tr("Not enough frames in the frame group to North-East.");
        return false;
    }

    bool result = false;
    for (int i = 0; i < frameCount; i++) {
        int n = this->getGroupFrameIndices(DIR_NE).first + i;
        D1GfxFrame* currFrame = this->getFrame(n);
        if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
            dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
            break;
        }
        bool change = false;
        switch (i) {
        case 4:
            if (currFrame->getPixel(63, 50).isTransparent()) {
                i = frameCount;
                continue; // assume it is already done
            }
        case 5:
        case 6:
        case 7: {
            // shift the bottom part (shadow) with (0;18) down
            for (int y = height - 18 - 1; y >= 116; y--) {
                for (int x = width - 1; x >= 0; x--) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent() || pixel.getPaletteIndex() != 0)
                        continue;
                    change |= currFrame->setPixel(x, y + 18, pixel);
                    change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
            // shift the upper part (monster) with (0;32) down
            for (int y = height - 32 - 1; y >= 0; y--) {
                for (int x = width - 1; x >= 0; x--) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent() || pixel.getPaletteIndex() == 0)
                        continue;
                    change |= currFrame->setPixel(x, y + 32, pixel);
                    change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        } break;
        case 8:
        case 9:
        case 10:
        case 11: {
            // shift the bottom part (shadow) with (0;28) down
            for (int y = height - 28 - 1; y >= 100; y--) {
                for (int x = width - 1; x >= 0; x--) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent() || pixel.getPaletteIndex() != 0)
                        continue;
                    change |= currFrame->setPixel(x, y + 28, pixel);
                    change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
            // shift the upper part (monster) with (0;64) down
            for (int y = height - 64 - 1; y >= 0; y--) {
                for (int x = width - 1; x >= 0; x--) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent() || pixel.getPaletteIndex() == 0)
                        continue;
                    change |= currFrame->setPixel(x, y + 64, pixel);
                    change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        } break;
        case 12:
        case 13:
        case 14:
        case 15: {
            // shift the bottom part (shadow) with (18;35) down
            for (int y = height - 35 - 1; y >= 100; y--) {
                for (int x = width - 18 - 1; x >= 0; x--) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent() || pixel.getPaletteIndex() != 0)
                        continue;
                    change |= currFrame->setPixel(x + 18, y + 35, pixel);
                    change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
            // shift the upper part (monster) with (0;96) down
            for (int y = height - 96 - 1; y >= 0; y--) {
                for (int x = width - 1; x >= 0; x--) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent() || pixel.getPaletteIndex() == 0)
                        continue;
                    change |= currFrame->setPixel(x, y + 96, pixel);
                    change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        } break;
        }
        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(n + 1).arg(DIR_NE + 1);
            }
        }
    }

    for (int i = 0; i < frameCount; i++) {
        int n = this->getGroupFrameIndices(DIR_NW).first + i;
        D1GfxFrame* currFrame = this->getFrame(n);
        if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
            dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
            break;
        }
        bool change = false;
        switch (i) {
        case 4:
            if (currFrame->getPixel(111, 46).isTransparent()) {
                i = frameCount;
                continue; // assume it is already done
            }
        case 5:
        case 6:
        case 7: {
            int sy;
            switch (i) {
            case 4: sy = 111; break;
            case 5: sy = 115; break;
            case 6: sy = 118; break;
            case 7: sy = 118; break;
            }
            // shift the bottom part (shadow) with (dx;15) down
            int dx = 10;
            if (i == 5)
                dx = 15;
            for (int y = height - 15 - 1; y >= sy; y--) {
                for (int x = width - 15 - 1; x >= 0; x--) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent())
                        continue;
                    change |= currFrame->setPixel(x + dx, y + 15, pixel);
                    change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
            // shift the upper part (monster) with (0;30) down
            for (int y = sy - 1; y >= 0; y--) {
                for (int x = width - 1; x >= 0; x--) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent())
                        continue;
                    change |= currFrame->setPixel(x, y + 30, pixel);
                    change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        } break;
        case 8:
        case 9:
        case 10:
        case 11: {
            int sy = 110;
            // shift the bottom part (shadow) with (dx;25) down
            int dx = 15;
            if (i == 11)
                dx = 10;
            for (int y = height - 25 - 1; y >= sy; y--) {
                for (int x = width - 15 - 1; x >= 0; x--) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent())
                        continue;
                    change |= currFrame->setPixel(x + dx, y + 25, pixel);
                    change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
            // shift the upper part (monster) with (0;dy) down
            int dy = 60;
            if (i == 11)
                dy = 62;
            for (int y = sy - 1; y >= 0; y--) {
                for (int x = width - 1; x >= 0; x--) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent())
                        continue;
                    change |= currFrame->setPixel(x, y + dy, pixel);
                    change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        } break;
        case 12:
        case 13:
        case 14:
        case 15: {
            int sy = 100;
            // shift the bottom part (shadow) with (16;35) down
            for (int y = height - 35 - 1; y >= sy; y--) {
                for (int x = width - 16 - 1; x >= 0; x--) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent())
                        continue;
                    change |= currFrame->setPixel(x + 16, y + 35, pixel);
                    change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
            // shift the upper part (monster) with (0;96) down
            for (int y = height - 96 - 1; y >= 0; y--) {
                for (int x = width - 1; x >= 0; x--) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent())
                        continue;
                    change |= currFrame->setPixel(x, y + 96, pixel);
                    change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        } break;
        }
        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(n + 1).arg(DIR_NW + 1);
            }
        }
    }

    return result;
}*/

bool D1Gfx::patchGoatLDie(bool silent)
{
    constexpr int frameCount = 16;
    constexpr int width = 160;
    constexpr int height = 128;

    if (this->getGroupCount() <= DIR_SW || this->getGroupCount() <= DIR_W || this->getGroupCount() <= DIR_NW || this->getGroupCount() <= DIR_E || this->getGroupCount() <= DIR_SE) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    if ((this->getGroupFrameIndices(DIR_SW).second - this->getGroupFrameIndices(DIR_SW).first + 1) != frameCount) {
        dProgressErr() << tr("Not enough frames in the frame group %1.").arg((int)DIR_SW + 1);
        return false;
    }
    if ((this->getGroupFrameIndices(DIR_W).second - this->getGroupFrameIndices(DIR_W).first + 1) != frameCount) {
        dProgressErr() << tr("Not enough frames in the frame group %1.").arg((int)DIR_W + 1);
        return false;
    }
    if ((this->getGroupFrameIndices(DIR_NW).second - this->getGroupFrameIndices(DIR_NW).first + 1) != frameCount) {
        dProgressErr() << tr("Not enough frames in the frame group %1.").arg((int)DIR_NW + 1);
        return false;
    }
    if ((this->getGroupFrameIndices(DIR_E).second - this->getGroupFrameIndices(DIR_E).first + 1) != frameCount) {
        dProgressErr() << tr("Not enough frames in the frame group %1.").arg((int)DIR_E + 1);
        return false;
    }
    if ((this->getGroupFrameIndices(DIR_SE).second - this->getGroupFrameIndices(DIR_SE).first + 1) != frameCount) {
        dProgressErr() << tr("Not enough frames in the frame group %1.").arg((int)DIR_SE + 1);
        return false;
    }

    bool result = false;
    for (int ii = 0; ii < this->getGroupCount(); ii++) {
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
                dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
                break;
            }
            bool change = false;
            switch (ii + 1) {
            case 2: {
                switch (i + 1) {
                case 9:
                    if (currFrame->getPixel(71, 127).isTransparent()) {
                        i = frameCount;
                        ii = INT_MAX - 1;
                        continue; // assume it is already done
                    }
                case 10:
                case 11:
                case 12: {
                    // shift the monster with (0;16) up
                    for (int y = 16; y < height; y++) {
                        for (int x = 0; x < width; x++) {
                            D1GfxPixel pixel = currFrame->getPixel(x, y);
                            if (pixel.isTransparent())
                                continue;
                            change |= currFrame->setPixel(x, y - 16, pixel);
                            change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }
                    // copy pixels from the followup frames
                    if (i + 1 != 9) {
                        D1GfxFrame* oFrame = this->getFrame(n + 4);
                        for (int y = 4 - 1; y >= 0; y--) {
                            for (int x = 0; x < width; x++) {
                                D1GfxPixel pixel = oFrame->getPixel(x, y);
                                if (pixel.isTransparent())
                                    continue;
                                change |= currFrame->setPixel(x, y + 112, pixel);
                            }
                        }
                    }
                } break;
                case 14:
                case 15:
                case 16: {
                    // clear pixels from the first rows
                    for (int y = 4 - 1; y >= 0; y--) {
                        for (int x = 0; x < width; x++) {
                            change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }
                } break;
                }
            } break;
            case 3: {
                switch (i + 1) {
                case 9:
                case 10:
                case 11:
                case 12: {
                    // shift the monster with (0;16) up
                    for (int y = 16; y < height; y++) {
                        for (int x = 0; x < width; x++) {
                            D1GfxPixel pixel = currFrame->getPixel(x, y);
                            if (pixel.isTransparent())
                                continue;
                            change |= currFrame->setPixel(x, y - 16, pixel);
                            change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }
                    // copy pixels from the followup frames
                    {
                        D1GfxFrame* oFrame = this->getFrame(n + 4);
                        for (int y = 16 - 1; y >= 0; y--) {
                            for (int x = 0; x < width; x++) {
                                D1GfxPixel pixel = oFrame->getPixel(x, y);
                                if (pixel.isTransparent())
                                    continue;
                                change |= currFrame->setPixel(x, y + 112, pixel);
                            }
                        }
                    }
                } break;
                case 13:
                case 14:
                case 15:
                case 16: {
                    // shift the monster with (1;9) up/right
                    for (int y = 16; y < height; y++) {
                        for (int x = width - 1 - 1; x >= 0; x--) {
                            D1GfxPixel pixel = currFrame->getPixel(x, y);
                            if (pixel.isTransparent())
                                continue;
                            change |= currFrame->setPixel(x + 1, y - 9, pixel);
                            change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }
                    // clear pixels from the first rows
                    for (int y = 16 - 1; y >= 0; y--) {
                        for (int x = 0; x < width; x++) {
                            change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }
                } break;
                }
            } break;
            case 4: {
                switch (i + 1) {
                case 12:
                case 13:
                case 14:
                case 15:
                case 16: {
                    // shift the monster with (0;4) down
                    for (int y = height - 4 - 1; y >= 0; y--) {
                        for (int x = width - 1; x >= 0; x--) {
                            D1GfxPixel pixel = currFrame->getPixel(x, y);
                            if (pixel.isTransparent())
                                continue;
                            change |= currFrame->setPixel(x, y + 4, pixel);
                            change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }
                } break;
                }
            } break;
            case 7: {
                switch (i + 1) {
                case 9:
                case 10:
                case 11:
                case 12: {
                    // clear pixels from the first rows
                    for (int y = 4 - 1; y >= 0; y--) {
                        for (int x = 0; x < width; x++) {
                            change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }
                } break;
                }
            } break;
            case 8: {
                switch (i + 1) {
                case 12:
                case 13:
                case 14:
                case 15:
                case 16: {
                    // shift the monster with (0;16) up
                    for (int y = 16; y < height; y++) {
                        for (int x = 0; x < width; x++) {
                            D1GfxPixel pixel = currFrame->getPixel(x, y);
                            if (pixel.isTransparent())
                                continue;
                            change |= currFrame->setPixel(x, y - 16, pixel);
                            change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }
                } break;
                }
            } break;
            }

            if (change) {
                result = true;
                this->setModified();
                if (!silent) {
                    dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(i + 1).arg(ii + 1);
                }
            }
        }
    }

    return result;
}

bool D1Gfx::patchSklAxDie(bool silent)
{
    constexpr int frameCount = 17;
    constexpr int width = 128;
    constexpr int height = 96;

    if (this->getGroupCount() != NUM_DIRS) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        if ((this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first + 1) < frameCount) {
            dProgressErr() << tr("Not enough frames in the frame group %1.").arg(ii + 1);
            return false;
        }
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
                dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
                return false;
            }
        }
        if (ii + 1 == 1) {
            int i = 1 - 1;
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getPixel(49, 12).isTransparent()) {
                return false; // assume it is already done
            }
        }
    }

    bool result = false;
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            bool change = false;
            int dx = 0, dy = 0;
            switch (i + 1) {
            case 1: dx = 0; dy = 15; break;
            case 2: dx = 0; dy = 12; break;
            case 3: dx = 0; dy = 10; break;
            case 4: dx = 0; dy = 6; break;
            case 5:
            case 6:
            case 7: dx = 0; dy = 5; break;
            case 8:
            case 9:
            case 10: dx = 0; dy = 3; break;
            case 11:
                if (ii + 1 != 4) {
                    dx = 0;
                    dy = 3;
                }
                break;
            case 12:
                if (ii + 1 != 4) {
                    dx = 0;
                    dy = 3;
                }
                break;
            case 13:
                if (ii + 1 != 4 && ii + 1 != 5) {
                    dx = 0;
                    dy = 3;
                }
                break;
            case 14:
                if (ii + 1 != 4 && ii + 1 != 5) {
                    dx = 0;
                    dy = 3;
                }
                break;
            case 15:
                if (ii + 1 != 4 && ii + 1 != 5) {
                    dx = 0;
                    dy = 3;
                }
                break;
            case 16:
                if (ii + 1 != 4 && ii + 1 != 5) {
                    dx = 0;
                    dy = 3;
                }
                break;
            case 17:
                if (ii + 1 != 1 && ii + 1 != 2 && ii + 1 != 3 && ii + 1 != 4 && ii + 1 != 5 && ii + 1 != 6 && ii + 1 != 8) {
                    dx = 0;
                    dy = 3;
                }
                break;
            }

            change |= ShiftFrame(currFrame, dx, dy, 0, 0, width, height);

            switch (ii + 1) {
            case 1:
                if (i + 1 == 17) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 82);
                }
                break;
            case 2:
                if (i + 1 == 17) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 82);
                }
                break;
            case 3:
                if (i + 1 == 17) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 88);
                }
                break;
            case 4:
                if (i + 1 == 11) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 83);
                }
                if (i + 1 == 12) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 82);
                }
                if (i + 1 == 13) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 89);
                }
                if (i + 1 == 14) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 67, 87, 70, 88);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 87);
                }
                if (i + 1 == 15) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 67, 85, 70, 87);
                    change |= ShiftFrame(currFrame, 0, 3, 91, 85, 93, 86);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 85);
                }
                if (i + 1 == 16) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 67, 87, 70, 89);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 87);
                }
                if (i + 1 == 17) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 65, 83, 67, 87);
                    change |= ShiftFrame(currFrame, 0, 3, 88, 83, 97, 85);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 83);
                }
                break;
            case 5:
                if (i + 1 == 13) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 67, 85, 75, 89);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 85);
                }
                if (i + 1 == 14) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 67, 83, 75, 88);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 83);
                }
                if (i + 1 == 15) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 56, 81, 76, 90);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 81);
                }
                if (i + 1 == 16) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 57, 85, 67, 90); // axe
                    change |= ShiftFrame(currFrame, 0, 3, 70, 83, width, height); // right arm
                    change |= ShiftFrame(currFrame, 0, 3, 40, 77, width, 83); // body
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 77); // body

                    change |= ShiftFrame(currFrame, 0, -3, 48, 87, 54, 92); // left arm
                }
                if (i + 1 == 17) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 57, 85, 67, 91); // axe
                    change |= ShiftFrame(currFrame, 0, 3, 65, 83, 74, 85); // axe
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 83);  // body
                }
                break;
            case 6:
                if (i + 1 == 17) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 72, 83, 74, 85);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 83);
                }
                break;
            case 8:
                if (i + 1 == 17) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 65, 83, 67, 85);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 83);
                }
                break;
            }

            // shift bone
            if (ii + 1 == 6) {
                if (i + 1 == 9) {
                    change |= ShiftFrame(currFrame, 0, -10, 52, 78, 54, 80);
                    change |= ShiftFrame(currFrame, 0, -10, 49, 79, 52, 81);
                }
                if (i + 1 == 10) {
                    change |= ShiftFrame(currFrame, 0, -10, 50, 83, 53, 85);
                    change |= ShiftFrame(currFrame, 0, -10, 48, 85, 52, 89);
                }
                if (i + 1 == 11) {
                    change |= ShiftFrame(currFrame, 4, -6, 48, 87, 53, 93);
                    change |= ShiftFrame(currFrame, 4, -6, 47, 91, 48, 92);
                }
            } else {
                if (i + 1 == 9) {
                    if (ii + 1 == 1 || ii + 1 == 2) {
                        change |= ShiftFrame(currFrame, 0, -10, 51, 77, 54, 79);
                        change |= ShiftFrame(currFrame, 0, -10, 48, 79, 53, 82);
                    } else if (ii + 1 == 4) {
                        change |= ShiftFrame(currFrame, 0, -10, 49, 78, 54, 82);
                    } else {
                        change |= ShiftFrame(currFrame, 0, -10, 48, 77, 54, 82);
                    }
                }
                if (i + 1 == 10) {
                    if (ii + 1 == 3) {
                        // TODO:....
                    } else {
                        change |= ShiftFrame(currFrame, 0, -10, 47, 83, 53, 89);
                    }
                }
                if (i + 1 == 11) {
                    change |= ShiftFrame(currFrame, 4, -6, 47, 87, 53, 93);
                }
            }

            if (i + 1 == 12) {
                change |= ShiftFrame(currFrame, 7, -2, 49, 89, 53, 94);
            }
            if (i + 1 == 13) {
                change |= ShiftFrame(currFrame, 7, -2, 50, 92, 53, 95);
            }
            if (i + 1 == 14) {
                change |= ShiftFrame(currFrame, 7, -2, 50, 91, 53, 93);
            }
            if (i + 1 == 15) {
                change |= ShiftFrame(currFrame, 7, -2, 49, 88, 54, 92);
            }
            if (i + 1 == 16) {
                change |= ShiftFrame(currFrame, 7, -2, 48, 90, 54, 95);
            }
            if (i + 1 == 17) {
                if (ii + 1 == 3) {
                    for (int y = 92; y < 96; y++) {
                        for (int x = 49; x < 65; x++) {
                            change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                        }
                    }

                    D1GfxFrame* prevFrame = this->getFrame(n - 1);
                    change |= CopyFrame(currFrame, 0, 0, prevFrame, 56, 90, 61, 93);
                } else {
                    change |= ShiftFrame(currFrame, 7, -2, 49, 92, 54, 95);
                }
            }

            if (change) {
                result = true;
                this->setModified();
                if (!silent) {
                    dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(i + 1).arg(ii + 1);
                }
            }
        }
    }

    return result;
}

bool D1Gfx::patchSklBwDie(bool silent)
{
    constexpr int frameCount = 16 - 3;
    constexpr int width = 128;
    constexpr int height = 96;

    if (this->getGroupCount() != NUM_DIRS) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        if ((this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first + 1) < frameCount) {
            dProgressErr() << tr("Not enough frames in the frame group %1.").arg(ii + 1);
            return false;
        }
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
                dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
                return false;
            }
        }
        if (ii + 1 == 1) {
            int i = 2 - 1;
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getPixel(76, 92).isTransparent()) {
                return false; // assume it is already done
            }
        }
    }

    bool result = false;
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        while (true) {
            int i = this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first;
            if (i < frameCount)
                break;
            this->removeFrame(this->getGroupFrameIndices(ii).first + i, false);
            dProgress() << tr("Removed frame %1 of group %2.").arg(i + 1).arg(ii + 1);
            result = true;
        }
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            bool change = false;
            int dx = 0, dy = 0;
            switch (ii + 1) {
            case 1:
                if (i + 1 == 2) {
                    dx = -3;
                    dy = -3;
                }
                if (i + 1 == 3) {
                    dx = -3;
                    dy = -7;
                }
                if (i + 1 == 4 || i + 1 == 5 || i + 1 == 6) {
                    dx = -3;
                    dy = -5;
                }
                if (i + 1 == 7 || i + 1 == 8 || i + 1 == 9 || i + 1 == 10 || i + 1 == 11 || i + 1 == 12 || i + 1 == 13) {
                    dx = -2;
                    dy = -6;
                }
                break;
            case 2:
                if (i + 1 == 2) {
                    dx = -3;
                    dy = -3;
                }
                if (i + 1 == 3) {
                    dx = -3;
                    dy = -7;
                }
                if (i + 1 == 4 || i + 1 == 5 || i + 1 == 6) {
                    dx = -3;
                    dy = -6;
                }
                if (i + 1 == 7 || i + 1 == 8 || i + 1 == 9 || i + 1 == 10 || i + 1 == 11 || i + 1 == 12 || i + 1 == 13) {
                    dx = -3;
                    dy = -7;
                }
                break;
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                if (i + 1 == 2) {
                    dx = 0;
                    dy = -3;
                }
                if (i + 1 == 3) {
                    dx = 0;
                    dy = -5;
                }
                if (i + 1 == 4 || i + 1 == 5 || i + 1 == 6) {
                    dx = 0;
                    dy = -6;
                }
                if (i + 1 == 7 || i + 1 == 8 || i + 1 == 9 || i + 1 == 10 || i + 1 == 11 || i + 1 == 12 || i + 1 == 13) {
                    dx = 0;
                    dy = -7;
                }
                break;
            }

            change |= ShiftFrame(currFrame, dx, dy, 0, 0, width, height);

            if (change) {
                result = true;
                this->setModified();
                if (!silent) {
                    dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(i + 1).arg(ii + 1);
                }
            }
        }
    }

    return result;
}

bool D1Gfx::patchSklSrDie(bool silent)
{
    constexpr int frameCount = 15;
    constexpr int width = 128;
    constexpr int height = 96;

    if (this->getGroupCount() != NUM_DIRS) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        if ((this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first + 1) < frameCount) {
            dProgressErr() << tr("Not enough frames in the frame group %1.").arg(ii + 1);
            return false;
        }
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
                dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
                return false;
            }
        }
        if (ii + 1 == 1) {
            int i = 1 - 1;
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getPixel(31, 14).isTransparent()) {
                return false; // assume it is already done
            }
        }
    }

    bool result = false;
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            bool change = false;
            int dx = 0, dy = 0;
            switch (i + 1) {
            case 1: dx = 0; dy = 15; break;
            case 2: dx = 0; dy = 12; break;
            case 3: dx = 0; dy = 10; break;
            case 4: dx = 0; dy = 6;  break;
            case 5:
            case 6:
            case 7: dx = 0; dy = 5; break;
            case 8:
            case 9:
            case 10:
            case 11: dx = 0; dy = 3; break;
            case 12: 
                if (ii + 1 != 7 && ii + 1 != 8) {
                    dx = 0;
                    dy = 3;
                }
                break;
            case 13:
                if (ii + 1 != 5 && ii + 1 != 7 && ii + 1 != 8) {
                    dx = 0;
                    dy = 3;
                }
                break;
            case 14:
                if (ii + 1 != 4 && ii + 1 != 5 && ii + 1 != 6 && ii + 1 != 7 && ii + 1 != 8) {
                    dx = 0;
                    dy = 3;
                }
                break;
            case 15:
                if (ii + 1 != 1 && ii + 1 != 4 && ii + 1 != 5 && ii + 1 != 6 && ii + 1 != 7 && ii + 1 != 8) {
                    dx = 0;
                    dy = 3;
                }
                break;
            }

            change |= ShiftFrame(currFrame, dx, dy, 0, 0, width, height);

            switch (ii + 1) {
            case 1:
                // shift the sword
                if (i + 1 == 9) {
                    change |= ShiftFrame(currFrame, -1, 3, 47, 50, 61, 60);
                }
                if (i + 1 == 10) {
                    change |= ShiftFrame(currFrame, -2, 9, 47, 41, 64, 58);
                }
                if (i + 1 == 11) {
                    change |= ShiftFrame(currFrame, 1, 14, 42, 37, 60, 53);
                }
                if (i + 1 == 12) {
                    change |= ShiftFrame(currFrame, 5, 15, 37, 36, 55, 52);
                }
                if (i + 1 == 13) {
                    change |= ShiftFrame(currFrame, 7, 15, 34, 34, 51, 52);
                }
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, 9, 14, 34, 39, 50, 54);
                }
                // shift the shadow of the sword
                if (i + 1 == 13) {
                    change |= ShiftFrame(currFrame, 3, 1, 25, 63, 43, 70);
                }
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, 3, 0, 26, 65, 43, 70);
                }
                if (i + 1 == 15) {
                    // shift the main body
                    change |= ShiftFrame(currFrame, 0, 3, 28, 66, 30, 68);
                    change |= ShiftFrame(currFrame, 0, 3, 30, 56, 76, 87);
                    change |= ShiftFrame(currFrame, 0, 3, 76, 56, 109, 80);
                    change |= ShiftFrame(currFrame, 0, 3, 73, 37, 95, 56);
                    // shift the sword
                    change |= ShiftFrame(currFrame, 9, 13, 34, 41, 50, 56);
                    // shift the shadow of the sword
                    change |= ShiftFrame(currFrame, 2, 0, 28, 66, 38, 71);
                }
                // shift the left-leg
                if (i + 1 == 14 || i + 1 == 15) {
                    change |= ShiftFrame(currFrame, -1, -1, 69, 78, 75, 90);
                    change |= ShiftFrame(currFrame, -1, -4, 75, 78, 82, 96);
                    change |= ShiftFrame(currFrame, -1, -4, 74, 92, 75, 96);
                }
                // shift the right-leg
                if (i + 1 == 11) {
                    change |= ShiftFrame(currFrame, 3, -3, 17, 72, 40, 80);
                }
                if (i + 1 == 12) {
                    change |= ShiftFrame(currFrame, 6, -6, 13, 78, 16, 79);
                    change |= ShiftFrame(currFrame, 6, -6, 32, 77, 36, 79);
                    change |= ShiftFrame(currFrame, 6, -6, 14, 79, 36, 85);
                    // eliminate shadow(?)
                    change |= currFrame->setPixel(16, 78, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(17, 78, D1GfxPixel::transparentPixel());
                }
                if (i + 1 == 13) {
                    change |= ShiftFrame(currFrame, 10, -6, 30, 77, 33, 79);
                    change |= ShiftFrame(currFrame, 10, -6, 9, 79, 33, 85);
                    // eliminate shadow(?)
                    change |= currFrame->setPixel(9, 78, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(10, 78, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(11, 78, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(12, 78, D1GfxPixel::transparentPixel());
                    change |= currFrame->setPixel(13, 78, D1GfxPixel::transparentPixel());
                }
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, 11, -6, 8, 79, 32, 87);
                }
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, 13, -3, 6, 76, 30, 85);
                }
                break;
            case 2:
                // shift the right-leg
                if (i + 1 == 9) {
                    change |= ShiftFrame(currFrame, 0, 3, 24, 57, 43, 67);
                }
                if (i + 1 == 10) {
                    change |= ShiftFrame(currFrame, 0, 6, 23, 53, 41, 65);
                }
                if (i + 1 == 11) {
                    change |= ShiftFrame(currFrame, 1, 6, 19, 54, 38, 64);
                }
                if (i + 1 == 12) {
                    // shadow
                    change |= ShiftFrame(currFrame, 5, 0, 9, 71, 30, 74);
                    // leg
                    change |= ShiftFrame(currFrame, 5, 6, 14, 57, 33, 66);
                }
                if (i + 1 == 13) {
                    // shadow
                    change |= ShiftFrame(currFrame, 9, 0, 5, 71, 26, 74);
                    // leg
                    change |= ShiftFrame(currFrame, 8, 7, 10, 57, 29, 65);
                }
                if (i + 1 == 14) {
                    // shadow
                    change |= ShiftFrame(currFrame, 10, 0, 3, 71, 24, 74);
                    // leg
                    change |= ShiftFrame(currFrame, 9, 7, 8, 58, 27, 66);
                }
                if (i + 1 == 15) {
                    // shadow
                    change |= ShiftFrame(currFrame, 12, 0, 1, 71, 23, 74);
                    // leg
                    change |= ShiftFrame(currFrame, 10, 7, 6, 59, 26, 66);
                }

                // shift the left-leg
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, 2, -2, 35, 76, 58, 93);
                }
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, 2, -2, 36, 76, 58, 96);
                    change |= ShiftFrame(currFrame, 2, -2, 38, 82, 49, 94);
                }
                // shift the shield
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, 0, -2, 93, 76, 97, 78);
                    change |= ShiftFrame(currFrame, 0, -2, 82, 78, 97, 79);
                    change |= ShiftFrame(currFrame, 0, -2, 79, 79, 97, 81);
                    change |= ShiftFrame(currFrame, 0, -2, 70, 81, 97, 85);
                }
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, 0, -4, 83, 79, 85, 80);
                    change |= ShiftFrame(currFrame, 0, -4, 79, 80, 95, 81);
                    change |= ShiftFrame(currFrame, 0, -4, 70, 81, 97, 87);
                    change |= ShiftFrame(currFrame, 0, -4, 75, 87, 88, 88);
                }
                break;
            case 4:
                // shift the main body
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 85);
                }
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 85);
                }
                // shift the shield
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, 0, -2, 30, 72, 46, 85);
                    change |= ShiftFrame(currFrame, 0, -2, 46, 76, 47, 77);
                    change |= ShiftFrame(currFrame, 0, -2, 46, 77, 48, 85);
                    change |= ShiftFrame(currFrame, 0, -2, 48, 78, 50, 81);
                }
                break;
            case 5:
                // shift the main body
                if (i + 1 == 13) {
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, 70, 77);
                    change |= ShiftFrame(currFrame, 0, 3, 70, 0, width, 68);
                }
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, 70, 79);
                    change |= ShiftFrame(currFrame, 0, 3, 70, 0, width, 69);
                }
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, 70, 80);
                    change |= ShiftFrame(currFrame, 0, 3, 70, 0, width, 69);
                }
                // shift the sword
                if (i + 1 == 12) {
                    change |= ShiftFrame(currFrame, -2, -1, 62, 73, width, height);
                    change |= ShiftFrame(currFrame, -2, 0, 79, 72, width, height);
                }
                if (i + 1 == 13) {
                    change |= ShiftFrame(currFrame, -3, 0, 64, 75, width, height);
                    change |= ShiftFrame(currFrame, -4, 0, 83, 75, width, height);
                }
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, -2, -2, 64, 77, width, height);
                    change |= ShiftFrame(currFrame, -4, 0, 84, 75, width, height);
                }
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, -2, -5, 64, 81, width, height);
                    change |= ShiftFrame(currFrame, -4, 0, 84, 76, width, height);
                }
                // complete the sword
                if (i + 1 == 14) {
                    change |= currFrame->setPixel(87, 94, D1GfxPixel::colorPixel(248));
                    change |= currFrame->setPixel(88, 94, D1GfxPixel::colorPixel(164));
                }
                if (i + 1 == 15) {
                    change |= CopyFrame(currFrame, -2, 3, currFrame, 88, 88, 92, 91);
                    change |= currFrame->setPixel(86, 94, D1GfxPixel::colorPixel(248));
                    change |= currFrame->setPixel(87, 94, D1GfxPixel::colorPixel(247));
                }
                break;
            case 6:
                // shift the main body
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 80);
                }
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 79);
                }
                // shift the sword
                if (i + 1 == 12) {
                    change |= ShiftFrame(currFrame, 3, -4, 66, 81, 68, 93);
                    change |= ShiftFrame(currFrame, 3, -4, 40, 82, 68, 93);
                }
                if (i + 1 == 13) {
                    change |= ShiftFrame(currFrame, 4, -7, 39, 86, 68, 96);
                }
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, 6, -7, 38, 87, 68, 96);
                }
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, 9, -12, 54, 92, 67, 96);
                }
                // complete the sword
                if (i + 1 == 15) {
                    D1GfxFrame* prevFrame = this->getFrame(n - 1);
                    change |= CopyFrame(currFrame, 2, 0, prevFrame, 44, 84, 71, 89);
                }
                break;
            case 7:
                // shift the main body
                if (i + 1 == 12) {
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 79);
                }
                if (i + 1 == 13) {
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 79);
                }
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, 0, 3, 73, 79, 76, 81);
                    change |= ShiftFrame(currFrame, 0, 3, 29, 74, 76, 79);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 74);
                }
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, 0, 3, 29, 79, 76, 84);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 79);
                }
                // shift the sword
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, -1, -5, 4, 78, 38, 87);
                }
                // shift the right-leg
                if (i + 1 == 11) {
                    change |= ShiftFrame(currFrame, -1, -3, 81, 78, 91, 92);
                }
                if (i + 1 == 12) {
                    change |= ShiftFrame(currFrame, -5, -6, 85, 79, 94, 94);
                }
                if (i + 1 == 13) {
                    change |= ShiftFrame(currFrame, -8, -7, 88, 80, 97, 88);
                    change |= ShiftFrame(currFrame, -9, -7, 88, 88, 97, 95);
                }
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, -8, -8, 90, 81, 99, 83);
                    change |= ShiftFrame(currFrame, -9, -8, 90, 83, 99, 84);
                    change |= ShiftFrame(currFrame, -10, -8, 90, 84, 99, 87);
                    change |= ShiftFrame(currFrame, -11, -8, 90, 87, 99, 89);
                    change |= ShiftFrame(currFrame, -12, -8, 90, 89, 99, 96);
                }
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, -8, -8, 90, 82, 99, 84);
                    change |= ShiftFrame(currFrame, -9, -8, 90, 84, 99, 85);
                    change |= ShiftFrame(currFrame, -10, -8, 90, 85, 99, 88);
                    change |= ShiftFrame(currFrame, -11, -8, 90, 88, 99, 90);
                    change |= ShiftFrame(currFrame, -12, -8, 90, 90, 99, 91);
                    change |= ShiftFrame(currFrame, -13, -8, 90, 91, 99, 96);
                }
                break;
            case 8:
                // shift the main body
                if (i + 1 == 12) {
                    change |= ShiftFrame(currFrame, 0, 3, 61, 82, width, height);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 82);
                }
                if (i + 1 == 13) {
                    change |= ShiftFrame(currFrame, 0, 3, 61, 83, width, height);
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 83);
                }
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 85);
                }
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, 0, 3, 0, 0, width, 86);
                }
                // shift the left-leg
                if (i + 1 == 11) {
                    change |= ShiftFrame(currFrame, 2, -3, 45, 81, 61, 93);
                }
                if (i + 1 == 12) {
                    change |= ShiftFrame(currFrame, 1, -6, 44, 82, 60, 96);
                }
                if (i + 1 == 13) {
                    change |= ShiftFrame(currFrame, 0, -7, 43, 83, 61, 96);
                }
                if (i + 1 == 14) {
                    change |= ShiftFrame(currFrame, -2, -8, 45, 85, 61, 96);
                }
                if (i + 1 == 15) {
                    change |= ShiftFrame(currFrame, -1, -9, 49, 86, 60, 96);
                }
                // complete the left-leg
                if (i + 1 == 14) {
                    D1GfxFrame* prevFrame = this->getFrame(n - 1);
                    change |= CopyFrame(currFrame, 0, 0, prevFrame, 44, 86, 48, 87);
                    change |= CopyFrame(currFrame, 0, 1, prevFrame, 45, 87, 49, 88);
                    change |= currFrame->setPixel(45, 87, D1GfxPixel::colorPixel(165));
                }
                if (i + 1 == 15) {
                    D1GfxFrame* prevFrame = this->getFrame(n - 1);
                    change |= CopyFrame(currFrame, 0, 0, prevFrame, 43, 86, 51, 89);
                }
                break;
            }

            if (change) {
                result = true;
                this->setModified();
                if (!silent) {
                    dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(i + 1).arg(ii + 1);
                }
            }
        }
    }

    return result;
}

bool D1Gfx::patchUnrav(int gfxFileIndex, bool silent)
{
    constexpr int width = 96;
    constexpr int height = 128;

    if (this->getGroupCount() != NUM_DIRS) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    int frameCount = this->getFrameCount();
    for (int n = 0; n < frameCount; n++) {
        D1GfxFrame* currFrame = this->getFrame(n);
        if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
            dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
            return false;
        }

        if (n == 0) {
            int x, y;
            switch (gfxFileIndex) {
            case GFX_MON_UNRAVA: x = 79; y = 76; break;
            case GFX_MON_UNRAVD: x = 80; y = 74; break;
            case GFX_MON_UNRAVH: x = 78; y = 66; break;
            case GFX_MON_UNRAVN: x = 80; y = 76; break;
            case GFX_MON_UNRAVS: x = 80; y = 77; break;
            case GFX_MON_UNRAVW: x = 79; y = 76; break;
            }
            if (currFrame->getPixel(x, y).isTransparent()) {
                return false; // assume it is already done
            }
        }
    }
    bool result = false;
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        int frameCount = this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first + 1;
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            bool change = false;
            int dx = 0, dy = 0;
            switch (gfxFileIndex) {
            case GFX_MON_UNRAVA: dx = -15; dy = 0; break;
            case GFX_MON_UNRAVD: dx = -16; dy = 0; break;
            case GFX_MON_UNRAVH: dx = -16; dy = 0; break;
            case GFX_MON_UNRAVN: dx = -16; dy = 0; break;
            case GFX_MON_UNRAVS: dx = -16; dy = 0; break;
            case GFX_MON_UNRAVW: dx = -15; dy = 0; break;
            }

            change |= ShiftFrame(currFrame, dx, dy, 0, 0, width, height);

            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (!pixel.isTransparent() && pixel.getPaletteIndex() == 0) {
                        change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }

            if (change) {
                result = true;
                this->setModified();
                if (!silent) {
                    dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(i + 1).arg(ii + 1);
                }
            }
        }
    }
    return result;
}

bool D1Gfx::patchZombieDie(bool silent)
{
    constexpr int frameCount = 16;
    constexpr int width = 128;
    constexpr int height = 96;

    if (this->getGroupCount() != NUM_DIRS) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        if ((this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first + 1) < frameCount) {
            dProgressErr() << tr("Not enough frames in the frame group %1.").arg(ii + 1);
            return false;
        }
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
                dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
                return false;
            }
        }
        if (ii + 1 == 2) {
            int i = 1 - 1;
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            if (currFrame->getPixel(40, 74).isTransparent()) {
                return false; // assume it is already done
            }
        }
    }

    bool result = false;
    for (int ii = 0; ii < NUM_DIRS; ii++) {
        for (int i = 0; i < frameCount; i++) {
            int n = this->getGroupFrameIndices(ii).first + i;
            D1GfxFrame* currFrame = this->getFrame(n);
            bool change = false;
            int dx = 0, dy = 0;
            switch (ii + 1) {
            case 2:
                if (i + 1 == 1) {
                    dx = 8;
                    dy = -3;
                } else if (i + 1 == 2) {
                    dx = 4;
                    dy = 0;
                } else {
                    dx = 2;
                    dy = 0;
                }
                break;
            case 3:
                if (i + 1 == 1) {
                    dx = 10;
                    dy = -3;
                } else if (i + 1 == 2) {
                    dx = 5;
                    dy = 0;
                } else {
                    dx = 4;
                    dy = 0;
                }
                break;
            case 4:
                switch (i + 1) {
                case 1: dx = 6; dy = 13; break;
                case 2: dx = 6; dy = 10; break;
                case 3: dx = 6; dy = 7; break;
                case 4: dx = 5; dy = 4; break;
                case 5: dx = 5; dy = 3; break;
                default:dx = 4; dy = 2; break; // 6.. 16
                }
                break;
            case 5:
                switch (i + 1) {
                case 1: dx = -1; dy = 13; break;
                case 2: dx = -1; dy = 10; break;
                case 3: dx = -1; dy = 7; break;
                case 4: dx = -1; dy = 4; break;
                case 5: dx = -1; dy = 2; break;
                default: dx = -1; dy = 0; break; // 6.. 16
                }
                break;
            }

            change |= ShiftFrame(currFrame, dx, dy, 0, 0, width, height);

            if (change) {
                result = true;
                this->setModified();
                if (!silent) {
                    dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(i + 1).arg(ii + 1);
                }
            }
        }
    }

    return result;
}

bool D1Gfx::patchAcidbf(int gfxFileIndex, bool silent)
{
    constexpr int frameCount = 9;
    constexpr int obsoleteFrameCount = 1;

    if (this->getGroupCount() == 0) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }

    bool result = false;
    int ii = 0;
    while (true) {
        int i = this->getGroupFrameIndices(ii).second - this->getGroupFrameIndices(ii).first;
        if (i < frameCount - obsoleteFrameCount)
            break;
        this->removeFrame(this->getGroupFrameIndices(ii).first + i, false);
        dProgress() << tr("Removed frame %1 of group %2.").arg(i + 1).arg(ii + 1);
        result = true;
    }

    return result;
}

bool D1Gfx::patchFireba(int gfxFileIndex, bool silent)
{
    constexpr int frameCount = 14;
    constexpr int width = 96;
    constexpr int height = 96;

    if (this->getGroupCount() == 0) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    if (this->getFrameCount() != frameCount) {
        dProgressErr() << tr("Not enough frames in the graphics.");
        return false;
    }

    for (int i = 0; i < frameCount; i++) {
        int n = i;
        D1GfxFrame* currFrame = this->getFrame(n);
        if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
            dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
            return false;
        }

        int nn = 0, x, y;
        switch (gfxFileIndex) {
        case GFX_MIS_FIREBA2:  nn = 1; x = 41; y = 66; break;
        case GFX_MIS_FIREBA3:  nn = 2; x = 37; y = 65; break;
        case GFX_MIS_FIREBA11: nn = 4; x = 49; y = 54; break;
        case GFX_MIS_FIREBA15: nn = 2; x = 55; y = 63; break;
        case GFX_MIS_FIREBA16: nn = 1; x = 54; y = 66; break;

        case GFX_MIS_HOLY2:  nn = 1; x = 41; y = 66; break;
        case GFX_MIS_HOLY3:  nn = 2; x = 37; y = 65; break;
        case GFX_MIS_HOLY11: nn = 4; x = 49; y = 54; break;
        case GFX_MIS_HOLY15: nn = 2; x = 55; y = 63; break;
        case GFX_MIS_HOLY16: nn = 1; x = 54; y = 66; break;
        default: continue;
        }

        if (nn != 0 && i + 1 == nn && currFrame->getPixel(x, y).isTransparent()) {
            return false; // assume it is already done
        }
    }

    bool result = false;
    int ii = 0;
    for (int i = 0; i < frameCount; i++) {
        int n = i;
        D1GfxFrame* currFrame = this->getFrame(n);
        bool change = false;
        int dx = 0, dy = 0;

        switch (gfxFileIndex) {
        case GFX_MIS_FIREBA2:
        case GFX_MIS_HOLY2:
            switch (i + 1) {
            case 1:
            case 2:
            case 7:
            case 8:
            case 13:
            case 14: dx = 1; dy = 0; break;
            }
            break;
        case GFX_MIS_FIREBA3:
        case GFX_MIS_HOLY3:
            if (i + 1 == 2) {
                dx = 3;
                dy = 0;
            }
            break;
        case GFX_MIS_FIREBA5:
        case GFX_MIS_HOLY5:
            if (i + 1 == 6) {
                change |= currFrame->setPixel(0, 57, D1GfxPixel::transparentPixel());
                change |= currFrame->setPixel(0, 58, D1GfxPixel::transparentPixel());
            }
            break;
        case GFX_MIS_FIREBA6:
        case GFX_MIS_HOLY6:
            if (i + 1 == 10 || i + 1 == 11) {
                for (int y = 63; y < 68; y++) {
                    for (int x = 81; x < 84; x++) {
                        change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            break;
        case GFX_MIS_FIREBA8:
        case GFX_MIS_HOLY8:
            if (i + 1 == 2) {
                change |= currFrame->setPixel(37, 59, D1GfxPixel::transparentPixel());
                change |= currFrame->setPixel(37, 60, D1GfxPixel::transparentPixel());
            }
            break;
        case GFX_MIS_FIREBA9:
        case GFX_MIS_HOLY9:
            if (i + 1 == 5) {
                change |= currFrame->setPixel(49, 42, D1GfxPixel::transparentPixel());
            }
            break;
        case GFX_MIS_FIREBA10:
        case GFX_MIS_HOLY10:
            if (i + 1 == 2) {
                change |= currFrame->setPixel(58, 59, D1GfxPixel::transparentPixel());
                change |= currFrame->setPixel(58, 60, D1GfxPixel::transparentPixel());
            }
            break;
        case GFX_MIS_FIREBA11:
        case GFX_MIS_HOLY11:
            switch (i + 1) {
            case 4: dx = 0; dy = 1; break;
            case 5:
            case 6: dx = 0; dy = 2; break;
            case 7:
            case 8:
            case 9:
            case 10: dx = 0; dy = -1; break;
            }
            break;
        case GFX_MIS_FIREBA12:
        case GFX_MIS_HOLY12:
            if (i + 1 == 5) {
                for (int y = 66; y < 72; y++) {
                    for (int x = 91; x < width; x++) {
                        change |= currFrame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            break;
        case GFX_MIS_FIREBA15:
        case GFX_MIS_HOLY15:
            if (i + 1 == 2) {
                dx = -3;
                dy = 0;
            }
            break;
        case GFX_MIS_FIREBA16:
        case GFX_MIS_HOLY16:
            switch (i + 1) {
            case 1:
            case 2:
            case 7:
            case 8:
            case 13:
            case 14: dx = -1; dy = 0; break;
            }
            break;
        }

        change |= ShiftFrame(currFrame, dx, dy, 0, 0, width, height);

        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(i + 1).arg(ii + 1);
            }
        }
    }

    return result;
}

bool D1Gfx::patchHoly(int gfxFileIndex, bool silent)
{
    return this->patchFireba(gfxFileIndex, silent);
}

bool D1Gfx::patchMagball(bool silent)
{
    constexpr int frameCount = 16;
    constexpr int width = 128;
    constexpr int height = 128;

    for (int i = 0; i < frameCount; i++) {
        int n = i;
        D1GfxFrame* currFrame = this->getFrame(n);
        if (currFrame->getWidth() != width || currFrame->getHeight() != height) {
            dProgressErr() << tr("Framesize of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
            return false;
        }
    }

    bool result = false;
    int ii = 0;
    for (int i = 0; i < frameCount; i++) {
        int n = i;
        D1GfxFrame* currFrame = this->getFrame(n);
        bool change = false;

        if (i + 1 == 6) {
            change |= currFrame->setPixel(52, 99, D1GfxPixel::transparentPixel());
            change |= currFrame->setPixel(52, 100, D1GfxPixel::transparentPixel());
            change |= currFrame->setPixel(52, 101, D1GfxPixel::transparentPixel());
        }

        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of group %2 is modified.").arg(i + 1).arg(ii + 1);
            }
        }
    }

    return result;
}

bool D1Gfx::patchCursorIcons(bool silent)
{
    int frameCount = this->getFrameCount();

    if (frameCount < 179) {
        dProgressErr() << tr("Invalid ObjCurs.CEL (Number of frames: %1. Expected at least 179.)").arg(frameCount);
        return false;
    }

    if (frameCount == 179) {
        QString baseFilePath = this->getFilePath();
        // read ObjCurs2.CEL from the same folder
        QString stdPath = baseFilePath;
        stdPath.insert(stdPath.length() - 4, "2");

        if (QFileInfo::exists(stdPath)) {
            OpenAsParam opParams = OpenAsParam();
            D1Gfx hfGfx;
            hfGfx.setPalette(this->palette);
            if (!D1Cel::load(hfGfx, stdPath, opParams)) {
                dProgressErr() << tr("Failed loading CEL file: %1.").arg(QDir::toNativeSeparators(stdPath));
                return false;
            }
            if (hfGfx.getFrameCount() != 61) {
                dProgressErr() << tr("Invalid file: %1. (Number of frames: %2. Expected 61.)").arg(QDir::toNativeSeparators(stdPath)).arg(hfGfx.getFrameCount());
                return false;
            }

            // remove the last two entries
            hfGfx.removeFrame(59, false);
            hfGfx.removeFrame(59, false);

            // merge the two cel files
            this->addGfx(&hfGfx);
            frameCount += 61 - 2;
        } else {
            dProgressWarn() << tr("File not found (%1).").arg(stdPath);
        }
    }

    if (frameCount != 179 + 61 - 2) {
        dProgressWarn() << tr("Skipped CEL-merge for Hellfire (%1).").arg(frameCount == 179 ? tr("ObjCurs2.CEL not found") : tr("Frame-count is %1").arg(frameCount));
    }

    bool result = false;
    for (int i = 0; i < frameCount; i++) {
        D1GfxFrame* currFrame = this->getFrame(i);
        int width = currFrame->getWidth();
        int height = currFrame->getHeight();
        bool change = false;
        int dx = 0, dy = 0;
        switch (i + 1) {
        case 236: dx = 0; dy = -2; break;
        case 234: dx = 0; dy = -2; break;
        case 233: dx = 0; dy = -1; break;
        case 232: dx = -2; dy = -2; break;
        case 231: dx = 0; dy = -1; break;
        case 230: dx = 2; dy = 0; break;
        case 229: dx = -1; dy = -3; break;
        case 228: dx = 0; dy = -2; break;
        case 227: dx = -1; dy = -2; break;
        case 226: dx = -1; dy = -2; break;
        case 225: dx = 0; dy = 2; break;
        case 224: dx = -1; dy = 1; break;
        case 223: dx = 0; dy = 1; break;
        case 222: dx = -2; dy = 3; break;
        case 221: dx = -1; dy = 2; break;
        case 220: dx = -1; dy = 0; break;
        case 219: dx = -1; dy = 0; break;
        case 218: dx = -1; dy = 0; break;
        case 217: dx = 0; dy = 1; break;
        case 216: dx = -1; dy = 9; break; // a
        case 215: dx = 0; dy = 12; break; // a
        case 213: dx = -1; dy = 0; break;
        case 179: dx = 1; dy = 5; break;
        case 177: dx = 0; dy = 2; break;
        case 176: dx = 1; dy = 0; break;
        case 173: dx = 1; dy = 0; break;
        case 171: dx = 0; dy = -3; break; // a
        case 167: dx = -1; dy = 0; break;
        case 165: dx = 2; dy = -3; // a
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    D1GfxPixel pixel = currFrame->getPixel(x, y);
                    if (pixel.isTransparent())
                        continue;
                    quint8 color = pixel.getPaletteIndex();
                    if (color == 144 || color == 145)
                        change |= currFrame->setPixel(x, y, D1GfxPixel::colorPixel(color + 96));
                }
            }
            break;
        case 164: dx = 0; dy = 4; break;// a
        case 162: dx = 0; dy = -4; break;// a
        case 161: dx = 0; dy = 1; break;
        case 159: dx = 1; dy = 1; break;
        case 158: dx = 0; dy = 3; break;
        case 157: dx = 1; dy = -1; break;
        case 156: dx = 0; dy = 1; break;
        case 154: dx = 1; dy = 0; break;
        case 153: dx = -1; dy = 1; break;
        case 152: dx = 0; dy = 2; break;
        case 151: dx = 0; dy = 2; break;// a
        case 147: dx = 0; dy = 2; break;// a
        case 146: dx = -2; dy = -2; break;
        case 145: dx = 0; dy = -2; break;
        case 144: dx = 0; dy = 1; break;
        case 142: dx = 1; dy = 2; break;
        case 141: dx = 0; dy = 4; break;// a
        case 140: dx = 0; dy = 0;  break; //a
        case 139: dx = 1; dy = 1; break;// a
        case 138: dx = 3; dy = 1; break;
        case 137: dx = 1; dy = 0; break;
        case 135: dx = -1; dy = 0; break;
        case 134: dx = 0; dy = 2; break;
        case 133: dx = -1; dy = -1; break;
        case 130: dx = -1; dy = 2; break;
        case 127: dx = 0; dy = 1; break;
        case 126: dx = 1; dy = 4; break;// a
        case 124: dx = 0; dy = 3; break;
        case 123: dx = -1; dy = 3; break;// a
        case 120: dx = 0; dy = 1; break;
        case 119: dx = 0; dy = 1; break;
        case 116: dx = 0; dy = 1; break;
        case 115: dx = 0; dy = 4; break;// a
        case 114: dx = 0; dy = 1; break;
        case 112: dx = 0; dy = -1; break;
        case 109: dx = 3; dy = 3; break;
        case 108: dx = 2; dy = 3; break;
        case 106: dx = 0; dy = 2; break;
        case 105: dx = 1; dy = 0; break;
        case 104: dx = 0; dy = 1; break;
        case 103: dx = 0; dy = 1; break;
        case 102: dx = 0; dy = 1; break;
        case 100: dx = 0; dy = 3; break;
        case 99: dx = 1; dy = 2; break;
        case 98: dx = 0; dy = 2; break;
        case 95: dx = 1; dy = 1; break;
        case 94: dx = 1; dy = 0; break;
        case 93: dx = 0; dy = 2; break;
        case 92: dx = 0; dy = 2; break;
        case 90: dx = 0; dy = 1; break;
        case 89: dx = 1; dy = 8; break;
        case 88: dx = 0; dy = 1; break;
        case 87: dx = 0; dy = 1; break;
        case 86: dx = 0; dy = 2; break;
        case 85: dx = 0; dy = 2; break;
        case 84: dx = 1; dy = 1; break;
        case 83: dx = 0; dy = 3; break;
        case 82: dx = 1; dy = 2; break;
        case 81: dx = -2; dy = -3; break;
        case 76: dx = -1; dy = -3; break;
        case 75: dx = -2; dy = 0; break;
        case 74: dx = 2; dy = 0; break;
        case 72: dx = -1; dy = 0; break;
        case 69: dx = 0; dy = 1; break;
        case 68: dx = 1; dy = 0; break;
        case 66: dx = 0; dy = 1; break;
        case 65: dx = 1; dy = 0; break;
        case 64: dx = 0; dy = 4; break;
        case 63: dx = 0; dy = 2; break;
        case 60: dx = 0; dy = 1; break;// ?
        case 57: dx = 0; dy = 1; break;// ?
        case 56: dx = 0; dy = 1;
            change |= currFrame->setPixel(16, 25, D1GfxPixel::colorPixel(188));
            change |= currFrame->setPixel(17, 25, D1GfxPixel::colorPixel(186));
            change |= currFrame->setPixel(18, 25, D1GfxPixel::colorPixel(185));
            change |= currFrame->setPixel(19, 25, D1GfxPixel::colorPixel(184));
            change |= currFrame->setPixel(20, 25, D1GfxPixel::colorPixel(183));
            break;
        case 55: dx = -1; dy = 3; break;
        case 52: dx = 0; dy = 1; break;
        case 51: dx = -1; dy = 0; break;
        case 49: dx = 1; dy = 0; break;
        case 43: dx = -1; dy = 0; break;
        case 42: dx = 1; dy = 0; break;
        case 39: dx = 0; dy = -1; break;
        case 36: dx = 1; dy = 0; break;
        case 37: dx = 0; dy = 1; break;
        case 35: dx = 1; dy = 0; break;
        case 34: dx = 2; dy = 1; break;
        case 33: dx = 0; dy = 1; break;
        case 32: dx = -1; dy = 1; break;
        case 31: dx = 0; dy = 1; break;
        case 30: dx = 2; dy = 0; break;
        case 27: dx = 0; dy = 1; break;
        case 26: dx = 1; dy = 0; break;
        case 25: dx = 0; dy = 1; break;
        case 24: dx = 1; dy = 0; break;
        case 22: dx = 0; dy = 1; break;
        case 20: dx = 1; dy = 2; break;
        case 19: dx = 1; dy = 1; break;
        case 13: dx = 1; dy = 0;
            if (!currFrame->getPixel(25, 4).isTransparent()) {
                i = frameCount;
                continue; // assume it is already done
            }
            break;
        }

        change = ShiftFrame(currFrame, dx, dy, 0, 0, width, height);
        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 is modified.").arg(i + 1);
            }
        }
    }

    return result;
}

bool D1Gfx::patchSplIcons(bool silent)
{
    int frameCount = this->getFrameCount();
    int lastIdx = 0;
    if (frameCount == 60) {
        // SpelIcon.CEL of hellfire
        lastIdx = 52;
    } else if (frameCount == 51) {
        // SpelIcon.CEL of hellfire
        lastIdx = 43;
    } else {
        if (frameCount != 52 && frameCount != 43) {
            dProgressWarn() << tr("Invalid SpelIcon.CEL (Number of frames: %1. Expected: 43 or 52.)").arg(frameCount);
        }
        return false;
    }
    for (int i = 0; i < 8; i++) {
        this->removeFrame(lastIdx, false);
    }
    if (!silent) {
        dProgress() << QApplication::tr("Removed the last 8 frames.");
    }
    return true;
}

bool D1Gfx::patchItemFlips(int gfxFileIndex, bool silent)
{
    constexpr int FRAME_WIDTH = 96;
    int FRAME_HEIGHT = (gfxFileIndex == GFX_ITEM_CROWNF || gfxFileIndex == GFX_ITEM_FEAR || gfxFileIndex == GFX_ITEM_LARMOR || gfxFileIndex == GFX_ITEM_WSHIELD) ? 128 : 160;

    bool result = false;
    for (int i = 0; i < this->getFrameCount(); i++) {
        D1GfxFrame *frame = this->frames[i];
        if (frame->getWidth() != FRAME_WIDTH || frame->getHeight() != FRAME_HEIGHT) {
            dProgressErr() << tr("Framesize of the Item animation (%1) does not match. (%2:%3 expected %4:%5. Index %6.)").arg(gfxFileIndex).arg(frame->getWidth()).arg(frame->getHeight()).arg(FRAME_WIDTH).arg(FRAME_HEIGHT).arg(i + 1);
            return false;
        }
        bool change = false;
        // center frames
        // - shift crown, larmor, wshield (-12;0), ear (-16;0)
        if (gfxFileIndex == GFX_ITEM_CROWNF || gfxFileIndex == GFX_ITEM_FEAR || gfxFileIndex == GFX_ITEM_LARMOR || gfxFileIndex == GFX_ITEM_WSHIELD) {
            // check if it is already done
            if (i == 0) {
                for (int y = 0; y < FRAME_HEIGHT; y++) {
                    for (int x = 0; x < FRAME_WIDTH / 2 - 1; x++) {
                        if (frame->getPixel(x, y).isTransparent()) continue;
                        return result; // assume it is already done
                    }
                }
            }
            for (int y = 0; y < FRAME_HEIGHT; y++) {
                for (int x = 16; x < FRAME_WIDTH; x++) {
                    change |= frame->setPixel((x - (gfxFileIndex == GFX_ITEM_FEAR ? 16 : 12)), y, frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift mace (+2;-2)
        if (gfxFileIndex == GFX_ITEM_MACE) {
            // check if it is already done
            if (i == 0 && frame->getPixel(41, 91).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = 2; y < FRAME_HEIGHT; y++) {
                for (int x = FRAME_WIDTH - 1 - 2; x >= 0; x--) {
                    change |= frame->setPixel((x + 2), (y - 2), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift scroll (0;-2)
        if (gfxFileIndex == GFX_ITEM_SCROLL) {
            // check if it is already done
            if (i == 0 && frame->getPixel(51, 94).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = 2; y < FRAME_HEIGHT; y++) {
                for (int x = FRAME_WIDTH - 1 - 0; x >= 0; x--) {
                    change |= frame->setPixel((x + 0), (y - 2), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift ring (0;-3)
        if (gfxFileIndex == GFX_ITEM_RING) {
            // check if it is already done
            if (i == 0 && frame->getPixel(45, 87).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = 3; y < FRAME_HEIGHT; y++) {
                for (int x = FRAME_WIDTH - 1 - 0; x >= 0; x--) {
                    change |= frame->setPixel((x + 0), (y - 3), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift staff (-7;+5)
        if (gfxFileIndex == GFX_ITEM_STAFF) {
            // check if it is already done
            if (i == 0 && frame->getPixel(53, 52).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = FRAME_HEIGHT - 1 - 5; y >= 0; y--) {
                for (int x = 7; x < FRAME_WIDTH; x++) {
                    change |= frame->setPixel((x - 7), (y + 5), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift brain (+5;+11)
        if (gfxFileIndex == GFX_ITEM_FBRAIN) {
            // check if it is already done
            if (i == 0 && frame->getPixel(40, 85).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = FRAME_HEIGHT - 1 - 11; y >= 0; y--) {
                for (int x = FRAME_WIDTH - 1 - 5; x >= 0; x--) {
                    change |= frame->setPixel((x + 5), (y + 11), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift mushroom (0;+6)
        if (gfxFileIndex == GFX_ITEM_FMUSH) {
            // check if it is already done
            if (i == 0 && frame->getPixel(43, 83).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = FRAME_HEIGHT - 1 - 6; y >= 0; y--) {
                for (int x = FRAME_WIDTH - 1 - 0; x >= 0; x--) {
                    change |= frame->setPixel((x + 0), (y + 6), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift innsign (+14;+8)
        if (gfxFileIndex == GFX_ITEM_INNSIGN) {
            // check if it is already done
            if (i == 0 && frame->getPixel(18, 96).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = FRAME_HEIGHT - 1 - 8; y >= 0; y--) {
                for (int x = FRAME_WIDTH - 1 - 14; x >= 0; x--) {
                    change |= frame->setPixel((x + 14), (y + 8), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift bloodstone (0;+5)
        if (gfxFileIndex == GFX_ITEM_BLDSTN) {
            // check if it is already done
            if (i == 0 && frame->getPixel(45, 75).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = FRAME_HEIGHT - 1 - 5; y >= 0; y--) {
                for (int x = FRAME_WIDTH - 1; x >= 0; x--) {
                    change |= frame->setPixel((x + 0), (y + 5), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift anvil (+3;+6)
        if (gfxFileIndex == GFX_ITEM_FANVIL) {
            // check if it is already done
            if (i == 0 && frame->getPixel(22, 80).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = FRAME_HEIGHT - 1 - 6; y >= 0; y--) {
                for (int x = FRAME_WIDTH - 1 - 3; x >= 0; x--) {
                    change |= frame->setPixel((x + 3), (y + 6), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift lazarus's staff (-3;+8)
        if (gfxFileIndex == GFX_ITEM_FLAZSTAF) {
            // check if it is already done
            if (i == 0 && frame->getPixel(30, 58).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = FRAME_HEIGHT - 1 - 8; y >= 0; y--) {
                for (int x = 3; x < FRAME_WIDTH; x++) {
                    change |= frame->setPixel((x - 3), (y + 8), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
            // fix the shadow of the staff
            if (i == 6) {
                change |= frame->setPixel(51, 127, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(63, 131, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(64, 131, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(35, 140, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(36, 140, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(39, 140, D1GfxPixel::colorPixel(0));
            }
            if (i == 7) {
                for (int x = 27; x < 38; x++)
                    change |= frame->setPixel(x, 140, D1GfxPixel::transparentPixel());
                for (int x = 62; x < 66; x++)
                    change |= frame->setPixel(x, 139, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(73, 140, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(74, 140, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(81, 139, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(48, 140, D1GfxPixel::colorPixel(0));
                change |= frame->setPixel(49, 140, D1GfxPixel::colorPixel(0));
            }
        }
        // - shift armor (0;-2)
        if (gfxFileIndex == GFX_ITEM_ARMOR2) {
            // check if it is already done
            if (i == 0 && frame->getPixel(29, 111).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = 2; y < FRAME_HEIGHT; y++) {
                for (int x = FRAME_WIDTH - 1; x >= 0; x--) {
                    change |= frame->setPixel((x + 0), (y - 2), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
            // mask shadow
            if (i == 14) {
                change |= frame->setPixel(22, 147, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(23, 147, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(20, 148, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(21, 148, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(22, 148, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(23, 148, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(23, 149, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(24, 149, D1GfxPixel::transparentPixel());
                change |= frame->setPixel(25, 149, D1GfxPixel::transparentPixel());
            }
        }
        // - shift cowhide (+2;+4)
        if (gfxFileIndex == GFX_ITEM_COWS1) {
            // check if it is already done
            if (i == 0 && frame->getPixel(30, 78).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = FRAME_HEIGHT - 1 - 4; y >= 0; y--) {
                for (int x = FRAME_WIDTH - 1 - 2; x >= 0; x--) {
                    change |= frame->setPixel((x + 2), (y + 4), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift last frame of donkeyhide (+2;+4)
        if (i == 14 && gfxFileIndex == GFX_ITEM_DONKYS1) {
            // check if it is already done
            if (frame->getPixel(40, 119).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = FRAME_HEIGHT - 1 - 4; y >= 0; y--) {
                for (int x = FRAME_WIDTH - 1 - 2; x >= 0; x--) {
                    change |= frame->setPixel((x + 2), (y + 4), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift moosehide (0;+3)
        if (gfxFileIndex == GFX_ITEM_MOOSES1) {
            // check if it is already done
            if (i == 0 && frame->getPixel(44, 83).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = FRAME_HEIGHT - 1 - 3; y >= 0; y--) {
                for (int x = FRAME_WIDTH - 1 - 0; x >= 0; x--) {
                    change |= frame->setPixel((x + 0), (y + 3), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // - shift teddy (0;+6)
        if (gfxFileIndex == GFX_ITEM_TEDDYS1) {
            // check if it is already done
            if (i == 0 && frame->getPixel(46, 100).isTransparent()) {
                return result; // assume it is already done
            }
            for (int y = FRAME_HEIGHT - 1 - 6; y >= 0; y--) {
                for (int x = FRAME_WIDTH - 1 - 0; x >= 0; x--) {
                    change |= frame->setPixel((x + 0), (y + 6), frame->getPixel(x, y));
                    change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                }
            }
        }
        // reduce gold stack
        if (gfxFileIndex == GFX_ITEM_GOLDFLIP) {
            // check if it is already done
            if (i == 4 && frame->getPixel(22, 147).isTransparent()) {
                return result; // assume it is already done
            }
            if (i == 4) {
                for (int y = 0; y < FRAME_HEIGHT; y++) {
                    for (int x = 0; x < FRAME_WIDTH; x++) {
                        if (y >= 144 + x - 27)
                            change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            if (i == 5) {
                for (int y = 0; y < FRAME_HEIGHT; y++) {
                    for (int x = 0; x < FRAME_WIDTH; x++) {
                        if (x <= 23)
                            change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            if (i == 6) {
                for (int y = 0; y < FRAME_HEIGHT; y++) {
                    for (int x = 0; x < FRAME_WIDTH; x++) {
                        if (y >= 150 - x + 63 || y >= 146 + x - 24)
                            change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            if (i == 7) {
                for (int y = 0; y < FRAME_HEIGHT; y++) {
                    for (int x = 0; x < FRAME_WIDTH; x++) {
                        if (x <= 23 || y >= 152 - x + 63 || y >= 146 + x - 23 || (x <= 34 && y >= 150))
                            change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            if (i == 8 || i == 9) {
                for (int y = 0; y < FRAME_HEIGHT; y++) {
                    for (int x = 0; x < FRAME_WIDTH; x++) {
                        if (y >= 152 - x + 63 || y >= 146 + x - 23 || (x <= 34 && y >= 150) || (x <= 39 && y >= 152) || (x >= 57 && y >= 153) || (x <= 34 && y <= 136))
                            change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                    }
                }
            }
            if (i == 9) {
                change |= frame->setPixel(37, 149, D1GfxPixel::colorPixel(203)); // (was color204)
                change |= frame->setPixel(38, 149, D1GfxPixel::colorPixel(198)); // (was color204)
                change |= frame->setPixel(37, 150, D1GfxPixel::colorPixel(199)); // (was transparent)
                change |= frame->setPixel(38, 150, D1GfxPixel::colorPixel(196)); // (was transparent)
                change |= frame->setPixel(39, 150, D1GfxPixel::colorPixel(196)); // (was color204)
                change |= frame->setPixel(40, 150, D1GfxPixel::colorPixel(197)); // (was color204)
                change |= frame->setPixel(37, 151, D1GfxPixel::colorPixel(204)); // (was transparent)
                change |= frame->setPixel(38, 151, D1GfxPixel::colorPixel(201)); // (was transparent)
                change |= frame->setPixel(39, 151, D1GfxPixel::colorPixel(198)); // (was transparent)
                change |= frame->setPixel(40, 151, D1GfxPixel::colorPixel(204)); // (was transparent)
                change |= frame->setPixel(40, 153, D1GfxPixel::transparentPixel()); // (was color203)
                change |= frame->setPixel(41, 153, D1GfxPixel::transparentPixel()); // (was color198)
                change |= frame->setPixel(42, 153, D1GfxPixel::transparentPixel()); // (was color198)
                change |= frame->setPixel(44, 153, D1GfxPixel::colorPixel(202)); // (was color198)
                change |= frame->setPixel(40, 154, D1GfxPixel::transparentPixel()); // (was color197)
                change |= frame->setPixel(41, 154, D1GfxPixel::transparentPixel()); // (was color196)
                change |= frame->setPixel(42, 154, D1GfxPixel::transparentPixel()); // (was color196)
                change |= frame->setPixel(43, 154, D1GfxPixel::transparentPixel()); // (was color196)
                change |= frame->setPixel(44, 154, D1GfxPixel::transparentPixel()); // (was color202)
                change |= frame->setPixel(40, 155, D1GfxPixel::transparentPixel()); // (was color199)
                change |= frame->setPixel(41, 155, D1GfxPixel::transparentPixel()); // (was color196)
                change |= frame->setPixel(42, 155, D1GfxPixel::transparentPixel()); // (was color196)
                change |= frame->setPixel(43, 155, D1GfxPixel::transparentPixel()); // (was color197)
                change |= frame->setPixel(44, 155, D1GfxPixel::transparentPixel()); // (was color204)
                change |= frame->setPixel(40, 156, D1GfxPixel::transparentPixel()); // (was color204)
                change |= frame->setPixel(41, 156, D1GfxPixel::transparentPixel()); // (was color201)
                change |= frame->setPixel(42, 156, D1GfxPixel::transparentPixel()); // (was color198)
                change |= frame->setPixel(43, 156, D1GfxPixel::transparentPixel()); // (was color204
            }
        }
#if 0
        // mask with the floor
        for (int y = 0; y < FRAME_HEIGHT; y++) {
            for (int x = 0; x < FRAME_WIDTH; x++) {
                if (frame->getPixel(x, y).isTransparent()) continue;
                //if (i != srcCelEntries - 1) {
                    if (y < FRAME_HEIGHT + ((x - 47) / 2)
                 // && y >= FRAME_HEIGHT - TILE_HEIGHT + ((x - 47) / 2)
                 && y < FRAME_HEIGHT + ((48 - x) / 2))
                 // && y >= FRAME_HEIGHT - TILE_HEIGHT + ((48 - x) / 2))
                    continue;
                /*} else {
                if (y < FRAME_HEIGHT + ((x - 47) / 2)
                 && y >= FRAME_HEIGHT - TILE_HEIGHT + ((x - 47) / 2)
                 && y < FRAME_HEIGHT + ((48 - x) / 2)
                 && y >= FRAME_HEIGHT - TILE_HEIGHT + ((48 - x) / 2))
                     continue;
                }*/
                change |= frame->setPixel(x, y, D1GfxPixel::transparentPixel());
                LogErrorF("Patched %s %d. @ %d:%d", filesToPatch[gfxFileIndex], i, x, y);
            }
        }
#endif
        if (change) {
            result = true;
            this->setModified();
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 is modified.").arg(i + 1);
            }
        }
    }

    return result;
}

void D1Gfx::patch(int gfxFileIndex, bool silent)
{
    bool change = false;
    switch (gfxFileIndex) {
    case GFX_OBJ_L1DOORS: // patch L1Doors.CEL
        change = this->patchCathedralDoors(silent);
        break;
    case GFX_OBJ_L2DOORS: // patch L2Doors.CEL
        change = this->patchCatacombsDoors(silent);
        break;
    case GFX_OBJ_L3DOORS: // patch L3Doors.CEL
        change = this->patchCavesDoors(silent);
        break;
    case GFX_OBJ_MCIRL: // patch Mcirl.CEL
        change = this->patchMagicCircle(silent);
        break;
    case GFX_OBJ_CANDLE2: // patch Candle2.CEL
        change = this->patchCandle(silent);
        break;
    case GFX_OBJ_LSHR: // patch LShrineG.CEL
        change = this->patchLeftShrine(silent);
        break;
    case GFX_OBJ_RSHR: // patch RShrineG.CEL
        change = this->patchRightShrine(silent);
        break;
    case GFX_OBJ_L5LIGHT: // patch L5Light.CEL
        change = this->patchCryptLight(silent);
        break;
    case GFX_PLR_RHTAT: // patch RHTAT.CL2
    case GFX_PLR_RHUHT: // patch RHUHT.CL2
    case GFX_PLR_RHUQM: // patch RHUQM.CL2
    case GFX_PLR_RMTAT: // patch RMTAT.CL2
    case GFX_PLR_WHMAT: // patch WHMAT.CL2
    case GFX_PLR_WLNLM: // patch WLNLM.CL2
    case GFX_PLR_WMDLM: // patch WMDLM.CL2
        change = this->patchPlrFrames(gfxFileIndex, silent);
        break;
    case GFX_PLR_RLHAS:
    case GFX_PLR_RLHAT:
    case GFX_PLR_RLHAW:
    case GFX_PLR_RLHBL:
    case GFX_PLR_RLHFM:
    case GFX_PLR_RLHLM:
    case GFX_PLR_RLHHT:
    case GFX_PLR_RLHQM:
    case GFX_PLR_RLHST:
    case GFX_PLR_RLHWL:
    case GFX_PLR_RLMAT:
    case GFX_PLR_RMDAW:
    case GFX_PLR_RMHAT:
    case GFX_PLR_RMMAT:
    case GFX_PLR_RMBFM:
    case GFX_PLR_RMBLM:
    case GFX_PLR_RMBQM:
        change = this->patchRogueExtraPixels(gfxFileIndex, silent);
        break;
    case GFX_PLR_WMHAS: // patch WMHAS.CL2
        change = this->patchWarriorStand(silent);
        break;
    case GFX_MON_FALLGD: // patch Fallgd.CL2
        change = this->patchFallGDie(silent);
        break;
    case GFX_MON_FALLGW: // patch Fallgw.CL2
        change = this->patchFallGWalk(silent);
        break;
    case GFX_MON_MAGMAD: // patch Magmad.CL2
        change = this->patchMagmaDie(silent);
        break;
    case GFX_MON_GOATBD: // patch GoatBd.CL2
        change = this->patchGoatBDie(silent);
        break;
    case GFX_MON_GOATLD: // patch GoatLd.CL2
        change = this->patchGoatLDie(silent);
        break;
    case GFX_MON_SKLAXD: // patch SklAxd.CL2
        change = this->patchSklAxDie(silent);
        break;
    case GFX_MON_SKLBWD: // patch SklBwd.CL2
        change = this->patchSklBwDie(silent);
        break;
    case GFX_MON_SKLSRD: // patch SklSrd.CL2
        change = this->patchSklSrDie(silent);
        break;
    case GFX_MON_UNRAVA: // patch Unrava.CL2
    case GFX_MON_UNRAVD: // patch Unravd.CL2
    case GFX_MON_UNRAVH: // patch Unravh.CL2
    case GFX_MON_UNRAVN: // patch Unravn.CL2
    case GFX_MON_UNRAVS: // patch Unravs.CL2
    case GFX_MON_UNRAVW: // patch Unravw.CL2
        change = this->patchUnrav(gfxFileIndex, silent);
        break;
    case GFX_MON_ZOMBIED: // patch Zombied.CL2
        change = this->patchZombieDie(silent);
        break;
    case GFX_MIS_ACIDBF1:
    case GFX_MIS_ACIDBF10:
    case GFX_MIS_ACIDBF11: // patch Acidbf*.CL2
        change = this->patchAcidbf(gfxFileIndex, silent);
        break;
    case GFX_MIS_FIREBA2:
    case GFX_MIS_FIREBA3:
    case GFX_MIS_FIREBA5:
    case GFX_MIS_FIREBA6:
    case GFX_MIS_FIREBA8:
    case GFX_MIS_FIREBA9:
    case GFX_MIS_FIREBA10:
    case GFX_MIS_FIREBA11:
    case GFX_MIS_FIREBA12:
    case GFX_MIS_FIREBA15:
    case GFX_MIS_FIREBA16: // patch Fireba*.CL2
        change = this->patchFireba(gfxFileIndex, silent);
        break;
    case GFX_MIS_HOLY2:
    case GFX_MIS_HOLY3:
    case GFX_MIS_HOLY5:
    case GFX_MIS_HOLY6:
    case GFX_MIS_HOLY8:
    case GFX_MIS_HOLY9:
    case GFX_MIS_HOLY10:
    case GFX_MIS_HOLY11:
    case GFX_MIS_HOLY12:
    case GFX_MIS_HOLY15:
    case GFX_MIS_HOLY16: // patch Holy*.CL2
        change = this->patchHoly(gfxFileIndex, silent);
        break;
    case GFX_MIS_MAGBALL2: // patch Magball2.CL2
        change = this->patchMagball(silent);
        break;
    case GFX_SPL_ICONS: // patch SpelIcon.CEL
        change = this->patchSplIcons(silent);
        break;
    case GFX_CURS_ICONS: // patch ObjCurs.CEL
        change = this->patchCursorIcons(silent);
        break;
    case GFX_ITEM_ARMOR2:   // patch Armor2.CEL
    case GFX_ITEM_GOLDFLIP: // patch GoldFlip.CEL
    case GFX_ITEM_MACE:     // patch Mace.CEL
    case GFX_ITEM_STAFF:    // patch Staff.CEL
    case GFX_ITEM_RING:     // patch Ring.CEL
    case GFX_ITEM_CROWNF:   // patch CrownF.CEL
    case GFX_ITEM_LARMOR:   // patch LArmor.CEL
    case GFX_ITEM_WSHIELD:  // patch WShield.CEL
    case GFX_ITEM_SCROLL:   // patch Scroll.CEL
    case GFX_ITEM_FEAR:     // patch FEar.CEL
    case GFX_ITEM_FBRAIN:   // patch FBrain.CEL
    case GFX_ITEM_FMUSH:    // patch FMush.CEL
    case GFX_ITEM_INNSIGN:  // patch Innsign.CEL
    case GFX_ITEM_BLDSTN:   // patch Bldstn.CEL
    case GFX_ITEM_FANVIL:   // patch Fanvil.CEL
    case GFX_ITEM_FLAZSTAF: // patch FLazStaf.CEL
    case GFX_ITEM_TEDDYS1:  // patch teddys1.CEL
    case GFX_ITEM_COWS1:    // patch cows1.CEL
    case GFX_ITEM_DONKYS1:  // patch donkys1.CEL
    case GFX_ITEM_MOOSES1:  // patch mooses1.CEL
        change = this->patchItemFlips(gfxFileIndex, silent);
        break;

    }
    if (!change && !silent) {
        dProgress() << tr("No change was necessary.");
    }
}

int D1Gfx::getPatchFileIndex(QString &filePath)
{
    int fileIndex = -1;
    QString baseName = QFileInfo(filePath).completeBaseName().toLower();
    // cel files
    if (baseName == "l1doors") {
        fileIndex = GFX_OBJ_L1DOORS;
    }
    if (baseName == "l2doors") {
        fileIndex = GFX_OBJ_L2DOORS;
    }
    if (baseName == "l3doors") {
        fileIndex = GFX_OBJ_L3DOORS;
    }
    if (baseName == "mcirl") {
        fileIndex = GFX_OBJ_MCIRL;
    }
    if (baseName == "candle2") {
        fileIndex = GFX_OBJ_CANDLE2;
    }
    if (baseName == "lshrineg") {
        fileIndex = GFX_OBJ_LSHR;
    }
    if (baseName == "rshrineg") {
        fileIndex = GFX_OBJ_RSHR;
    }
    if (baseName == "l5light") {
        fileIndex = GFX_OBJ_L5LIGHT;
    }
    if (baseName == "spelicon") {
        fileIndex = GFX_SPL_ICONS;
    }
    if (baseName == "objcurs") {
        fileIndex = GFX_CURS_ICONS;
    }
    if (baseName == "armor2") {
        fileIndex = GFX_ITEM_ARMOR2;
    }
    if (baseName == "goldflip") {
        fileIndex = GFX_ITEM_GOLDFLIP;
    }
    if (baseName == "mace") {
        fileIndex = GFX_ITEM_MACE;
    }
    if (baseName == "staff") {
        fileIndex = GFX_ITEM_STAFF;
    }
    if (baseName == "ring") {
        fileIndex = GFX_ITEM_RING;
    }
    if (baseName == "crownf") {
        fileIndex = GFX_ITEM_CROWNF;
    }
    if (baseName == "larmor") {
        fileIndex = GFX_ITEM_LARMOR;
    }
    if (baseName == "wshield") {
        fileIndex = GFX_ITEM_WSHIELD;
    }
    if (baseName == "scroll") {
        fileIndex = GFX_ITEM_SCROLL;
    }
    if (baseName == "fear") {
        fileIndex = GFX_ITEM_FEAR;
    }
    if (baseName == "fbrain") {
        fileIndex = GFX_ITEM_FBRAIN;
    }
    if (baseName == "fmush") {
        fileIndex = GFX_ITEM_FMUSH;
    }
    if (baseName == "innsign") {
        fileIndex = GFX_ITEM_INNSIGN;
    }
    if (baseName == "bldstn") {
        fileIndex = GFX_ITEM_BLDSTN;
    }
    if (baseName == "fanvil") {
        fileIndex = GFX_ITEM_FANVIL;
    }
    if (baseName == "flazstaf") {
        fileIndex = GFX_ITEM_FLAZSTAF;
    }
    if (baseName == "teddys1") {
        fileIndex = GFX_ITEM_TEDDYS1;
    }
    if (baseName == "cows1") {
        fileIndex = GFX_ITEM_COWS1;
    }
    if (baseName == "donkys1") {
        fileIndex = GFX_ITEM_DONKYS1;
    }
    if (baseName == "mooses1") {
        fileIndex = GFX_ITEM_MOOSES1;
    }
    // cl2 files
    // - players
    if (baseName == "rhtat") {
        fileIndex = GFX_PLR_RHTAT;
    }
    if (baseName == "rhuht") {
        fileIndex = GFX_PLR_RHUHT;
    }
    if (baseName == "rhuqm") {
        fileIndex = GFX_PLR_RHUQM;
    }
    if (baseName == "rlhas") {
        fileIndex = GFX_PLR_RLHAS;
    }
    if (baseName == "rlhat") {
        fileIndex = GFX_PLR_RLHAT;
    }
    if (baseName == "rlhaw") {
        fileIndex = GFX_PLR_RLHAW;
    }
    if (baseName == "rlhbl") {
        fileIndex = GFX_PLR_RLHBL;
    }
    if (baseName == "rlhfm") {
        fileIndex = GFX_PLR_RLHFM;
    }
    if (baseName == "rlhlm") {
        fileIndex = GFX_PLR_RLHLM;
    }
    if (baseName == "rlhht") {
        fileIndex = GFX_PLR_RLHHT;
    }
    if (baseName == "rlhqm") {
        fileIndex = GFX_PLR_RLHQM;
    }
    if (baseName == "rlhst") {
        fileIndex = GFX_PLR_RLHST;
    }
    if (baseName == "rlhwl") {
        fileIndex = GFX_PLR_RLHWL;
    }
    if (baseName == "rlmat") {
        fileIndex = GFX_PLR_RLMAT;
    }
    if (baseName == "rmdaw") {
        fileIndex = GFX_PLR_RMDAW;
    }
    if (baseName == "rmhat") {
        fileIndex = GFX_PLR_RMHAT;
    }
    if (baseName == "rmmat") {
        fileIndex = GFX_PLR_RMMAT;
    }
    if (baseName == "rmbfm") {
        fileIndex = GFX_PLR_RMBFM;
    }
    if (baseName == "rmblm") {
        fileIndex = GFX_PLR_RMBLM;
    }
    if (baseName == "rmbqm") {
        fileIndex = GFX_PLR_RMBQM;
    }
    if (baseName == "rmtat") {
        fileIndex = GFX_PLR_RMTAT;
    }
    if (baseName == "whmat") {
        fileIndex = GFX_PLR_WHMAT;
    }
    if (baseName == "wlnlm") {
        fileIndex = GFX_PLR_WLNLM;
    }
    if (baseName == "wmdlm") {
        fileIndex = GFX_PLR_WMDLM;
    }
    if (baseName == "wmhas") {
        fileIndex = GFX_PLR_WMHAS;
    }
    // - monsters
    if (baseName == "fallgd") {
        fileIndex = GFX_MON_FALLGD;
    }
    if (baseName == "fallgw") {
        fileIndex = GFX_MON_FALLGW;
    }
    if (baseName == "magmad") {
        fileIndex = GFX_MON_MAGMAD;
    }
    if (baseName == "goatbd") {
        fileIndex = GFX_MON_GOATBD;
    }
    if (baseName == "goatld") {
        fileIndex = GFX_MON_GOATLD;
    }
    if (baseName == "sklaxd") {
        fileIndex = GFX_MON_SKLAXD;
    }
    if (baseName == "sklbwd") {
        fileIndex = GFX_MON_SKLBWD;
    }
    if (baseName == "sklsrd") {
        fileIndex = GFX_MON_SKLSRD;
    }
    if (baseName == "unrava") {
        fileIndex = GFX_MON_UNRAVA;
    }
    if (baseName == "unravd") {
        fileIndex = GFX_MON_UNRAVD;
    }
    if (baseName == "unravh") {
        fileIndex = GFX_MON_UNRAVH;
    }
    if (baseName == "unravn") {
        fileIndex = GFX_MON_UNRAVN;
    }
    if (baseName == "unravs") {
        fileIndex = GFX_MON_UNRAVS;
    }
    if (baseName == "unravw") {
        fileIndex = GFX_MON_UNRAVW;
    }
    if (baseName == "zombied") {
        fileIndex = GFX_MON_ZOMBIED;
    }
    // - missiles
    if (baseName == "acidbf1") {
        fileIndex = GFX_MIS_ACIDBF1;
    }
    if (baseName == "acidbf10") {
        fileIndex = GFX_MIS_ACIDBF10;
    }
    if (baseName == "acidbf11") {
        fileIndex = GFX_MIS_ACIDBF11;
    }
    if (baseName == "fireba2") {
        fileIndex = GFX_MIS_FIREBA2;
    }
    if (baseName == "fireba3") {
        fileIndex = GFX_MIS_FIREBA3;
    }
    if (baseName == "fireba5") {
        fileIndex = GFX_MIS_FIREBA5;
    }
    if (baseName == "fireba6") {
        fileIndex = GFX_MIS_FIREBA6;
    }
    if (baseName == "fireba8") {
        fileIndex = GFX_MIS_FIREBA8;
    }
    if (baseName == "fireba9") {
        fileIndex = GFX_MIS_FIREBA9;
    }
    if (baseName == "fireba10") {
        fileIndex = GFX_MIS_FIREBA10;
    }
    if (baseName == "fireba11") {
        fileIndex = GFX_MIS_FIREBA11;
    }
    if (baseName == "fireba12") {
        fileIndex = GFX_MIS_FIREBA12;
    }
    if (baseName == "fireba15") {
        fileIndex = GFX_MIS_FIREBA15;
    }
    if (baseName == "fireba16") {
        fileIndex = GFX_MIS_FIREBA16;
    }
    if (baseName == "holy2") {
        fileIndex = GFX_MIS_HOLY2;
    }
    if (baseName == "holy3") {
        fileIndex = GFX_MIS_HOLY3;
    }
    if (baseName == "holy5") {
        fileIndex = GFX_MIS_HOLY5;
    }
    if (baseName == "holy6") {
        fileIndex = GFX_MIS_HOLY6;
    }
    if (baseName == "holy8") {
        fileIndex = GFX_MIS_HOLY8;
    }
    if (baseName == "holy9") {
        fileIndex = GFX_MIS_HOLY9;
    }
    if (baseName == "holy10") {
        fileIndex = GFX_MIS_HOLY10;
    }
    if (baseName == "holy11") {
        fileIndex = GFX_MIS_HOLY11;
    }
    if (baseName == "holy12") {
        fileIndex = GFX_MIS_HOLY12;
    }
    if (baseName == "holy15") {
        fileIndex = GFX_MIS_HOLY15;
    }
    if (baseName == "holy16") {
        fileIndex = GFX_MIS_HOLY16;
    }
    if (baseName == "magball2") {
        fileIndex = GFX_MIS_MAGBALL2;
    }
    return fileIndex;
}
