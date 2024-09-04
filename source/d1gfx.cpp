#include "d1gfx.h"

#include <QApplication>

#include "d1image.h"
#include "d1cel.h"
#include "d1cl2.h"
#include "openasdialog.h"
#include "progressdialog.h"

//#include "dungeon/all.h"

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

D1GfxFrame::D1GfxFrame(const D1GfxFrame &o)
{
    this->width = o.width;
    this->height = o.height;
    this->pixels = o.pixels;
    this->clipped = o.clipped;
    this->frameType = o.frameType;
}

D1GfxFrame::~D1GfxFrame()
{
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
        return false;
    }
    if (this->pixels[y][x] == pixel) {
        return false;
    }

    this->pixels[y][x] = pixel;
    return true;
}

bool D1GfxFrame::isClipped() const
{
    return this->clipped;
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
        dProgressFail() << QApplication::tr("Mismatching frame-sizes.");
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

QPointer<D1Pal>& D1GfxFrame::getFramePal()
{
    return this->framePal;
}

void D1GfxFrame::setFramePal(D1Pal *pal)
{
    this->framePal = pal;
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
static QString CelTypeTxt(D1CEL_TYPE type)
{
    QString result;
    switch (type) {
    case D1CEL_TYPE::V1_REGULAR:         result = QApplication::tr("regular (v1)");     break;
    case D1CEL_TYPE::V1_COMPILATION:     result = QApplication::tr("compilation (v1)"); break;
    case D1CEL_TYPE::V1_LEVEL:           result = QApplication::tr("level (v1)");       break;
    case D1CEL_TYPE::V2_MONO_GROUP:      result = QApplication::tr("mono group (v2)");  break;
    case D1CEL_TYPE::V2_MULTIPLE_GROUPS: result = QApplication::tr("multi group (v2)"); break;
    case D1CEL_TYPE::SMK:                result = "smk";                                break;
    default: result = "???"; break;
    }
    return result;
}

void D1Gfx::compareTo(const D1Gfx *gfx, QString header) const
{
    if (gfx->type != this->type) {
        reportDiff(QApplication::tr("type is %1 (was %2)").arg(CelTypeTxt(this->type)).arg(CelTypeTxt(gfx->type)), header);
    }
    if (gfx->groupFrameIndices.size() == this->groupFrameIndices.size()) {
        for (unsigned i = 0; i < this->groupFrameIndices.size(); i++) {
            if (this->groupFrameIndices[i].first != gfx->groupFrameIndices[i].first || 
                this->groupFrameIndices[i].second != gfx->groupFrameIndices[i].second) {
                reportDiff(QApplication::tr("group %1 is frames %2..%3 (was %4..%5)").arg(i + 1)
                    .arg(this->groupFrameIndices[i].first + 1).arg(this->groupFrameIndices[i].second + 1)
                    .arg(gfx->groupFrameIndices[i].first + 1).arg(gfx->groupFrameIndices[i].second + 1), header);
            }
        }
    } else {
        reportDiff(QApplication::tr("group-count is %1 (was %2)").arg(this->groupFrameIndices.size()).arg(gfx->groupFrameIndices.size()), header);
    }
    if (gfx->getFrameCount() == this->getFrameCount()) {
        for (int i = 0; i < this->getFrameCount(); i++) {
            D1GfxFrame *frameA = this->frames[i];
            D1GfxFrame *frameB = gfx->frames[i];
            if (frameA->getWidth() == frameB->getWidth() && frameA->getHeight() == frameB->getHeight()) {
                bool firstInFrame = true;
                for (int y = 0; y < frameA->getHeight(); y++) {
                    for (int x = 0; x < frameA->getWidth(); x++) {
                        D1GfxPixel pixelA = frameA->getPixel(x, y);
                        D1GfxPixel pixelB = frameB->getPixel(x, y);
                        if (pixelA != pixelB) {
                            if (firstInFrame) {
                                firstInFrame = false;
                                reportDiff(QApplication::tr("Frame %1:").arg(i + 1), header);
                            }
                            reportDiff(QApplication::tr("  pixel %1:%2 is %3 (was %4)").arg(x).arg(y)
                                .arg(pixelA.isTransparent() ? QApplication::tr("transparent") : QApplication::tr("color%1").arg(pixelA.getPaletteIndex()))
                                .arg(pixelB.isTransparent() ? QApplication::tr("transparent") : QApplication::tr("color%1").arg(pixelB.getPaletteIndex())), header);
                        }
                    }
                }
            } else {
                reportDiff(QApplication::tr("frame %1 is %2x%3 pixel (was %4x%5)").arg(i + 1)
                    .arg(frameA->getWidth()).arg(frameA->getHeight())
                    .arg(frameB->getWidth()).arg(frameB->getHeight()), header);
            }
        }
    } else {
        reportDiff(QApplication::tr("frame-count is %1 (was %2)").arg(this->getFrameCount()).arg(gfx->getFrameCount()), header);
    }
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

// builds QString from a D1CelFrame of given index
QString D1Gfx::getFramePixels(int frameIndex, bool values) const
{
    if (frameIndex < 0 || frameIndex >= this->frames.count()) {
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
        return QImage();
    }

    D1GfxFrame *frame = this->frames[frameIndex];

    QImage image = QImage(frame->getWidth(), frame->getHeight(), QImage::Format_ARGB32);
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

bool D1Gfx::isClipped(int frameIndex) const
{
    bool clipped;
    if (this->frames.count() > frameIndex) {
        clipped = this->frames[frameIndex]->isClipped();
    } else {
        clipped = this->type == D1CEL_TYPE::V2_MONO_GROUP || this->type == D1CEL_TYPE::V2_MULTIPLE_GROUPS;
    }
    return clipped;
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
    bool clipped = this->isClipped(0);

    D1GfxFrame* newFrame = new D1GfxFrame();
    newFrame->clipped = clipped;
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
    D1ImageFrame::load(*frame, pixels, frame->isClipped(), this->palette);
    // this->modified = true;

    return this->frames[idx];
}

D1GfxFrame *D1Gfx::insertFrame(int idx, const QImage &image)
{
    D1GfxFrame *frame = this->insertFrame(idx);
    D1ImageFrame::load(*frame, image, frame->isClipped(), this->palette);
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
    bool clipped = false;
    D1GfxFrame frame;
    D1ImageFrame::load(frame, image, clipped, this->palette);

    return this->addToFrame(idx, frame);
}

D1GfxFrame *D1Gfx::replaceFrame(int idx, const QString &pixels)
{
    bool clipped = this->isClipped(idx);

    D1GfxFrame *frame = new D1GfxFrame();
    D1ImageFrame::load(*frame, pixels, clipped, this->palette);
    this->setFrame(idx, frame);

    return this->frames[idx];
}

D1GfxFrame *D1Gfx::replaceFrame(int idx, const QImage &image)
{
    bool clipped = this->isClipped(idx);

    D1GfxFrame *frame = new D1GfxFrame();
    D1ImageFrame::load(*frame, image, clipped, this->palette);
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
    bool clipped = this->isClipped(0);
    for (int i = 0; i < numNewFrames; i++) {
        const D1GfxFrame* frame = gfx->getFrame(i);
        D1GfxFrame* newFrame = new D1GfxFrame(*frame);
        newFrame->clipped = clipped;
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
        return 0;
    }

    return this->frames[frameIndex]->getWidth();
}

int D1Gfx::getFrameHeight(int frameIndex) const
{
    if (frameIndex < 0 || frameIndex >= this->frames.count()) {
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
