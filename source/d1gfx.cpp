#include "d1gfx.h"

#include <QApplication>

#include "d1image.h"
#include "progressdialog.h"

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

bool operator==(const D1GfxPixel &lhs, const D1GfxPixel &rhs)
{
    return lhs.transparent == rhs.transparent && lhs.paletteIndex == rhs.paletteIndex;
}

bool operator!=(const D1GfxPixel &lhs, const D1GfxPixel &rhs)
{
    return lhs.transparent != rhs.transparent || lhs.paletteIndex != rhs.paletteIndex;
}

D1GfxFrame::D1GfxFrame(D1GfxFrame &o)
{
    this->width = o.width;
    this->height = o.height;
    this->pixels = o.pixels;
    this->clipped = o.clipped;
    this->frameType = o.frameType;
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

void D1GfxFrame::replacePixels(const std::vector<std::pair<D1GfxPixel, D1GfxPixel>> &replacements)
{
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            D1GfxPixel d1pix = this->pixels[y][x]; // this->getPixel(x, y);

            for (const std::pair<D1GfxPixel, D1GfxPixel> &replacement : replacements) {
                if (d1pix == replacement.first) {
                    this->pixels[y][x] = replacement.second;
                }
            }
        }
    }
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

// builds QImage from a D1CelFrame of given index
QImage D1Gfx::getFrameImage(int frameIndex) const
{
    if (this->palette == nullptr || frameIndex >= this->frames.count())
        return QImage();

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

D1GfxFrame *D1Gfx::insertFrame(int idx, bool *clipped)
{
    if (!this->frames.isEmpty()) {
        *clipped = this->frames[0]->isClipped();
    } else {
        *clipped = this->type == D1CEL_TYPE::V2_MONO_GROUP || this->type == D1CEL_TYPE::V2_MULTIPLE_GROUPS;
    }

    this->frames.insert(idx, new D1GfxFrame());

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
    return this->frames[idx];
}

D1GfxFrame *D1Gfx::insertFrame(int idx, const QImage &image)
{
    bool clipped;

    D1GfxFrame *frame = this->insertFrame(idx, &clipped);
    D1ImageFrame::load(*frame, image, clipped, this->palette);
    // this->modified = true;

    return this->frames[idx];
}

D1GfxFrame *D1Gfx::addToFrame(int idx, const D1GfxFrame &frame)
{
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

D1GfxFrame *D1Gfx::replaceFrame(int idx, const QImage &image)
{
    bool clipped = this->frames[idx]->isClipped();

    D1GfxFrame *frame = new D1GfxFrame();
    D1ImageFrame::load(*frame, image, clipped, this->palette);
    this->setFrame(idx, frame);

    return this->frames[idx];
}

void D1Gfx::removeFrame(int idx)
{
    delete this->frames[idx];
    this->frames.removeAt(idx);
    this->modified = true;

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

D1CEL_TYPE D1Gfx::getType() const
{
    return this->type;
}

void D1Gfx::setType(D1CEL_TYPE type)
{
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
    this->upscaled = upscaled;
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
    if (groupIndex < 0 || (unsigned)groupIndex >= this->groupFrameIndices.size())
        return std::pair<int, int>(0, 0);

    return this->groupFrameIndices[groupIndex];
}

int D1Gfx::getFrameCount() const
{
    return this->frames.count();
}

D1GfxFrame *D1Gfx::getFrame(int frameIndex) const
{
    if (frameIndex < 0 || frameIndex >= this->frames.count())
        return nullptr;

    return const_cast<D1GfxFrame *>(this->frames[frameIndex]);
}

void D1Gfx::setFrame(int frameIndex, D1GfxFrame *frame)
{
    delete this->frames[frameIndex];
    this->frames[frameIndex] = frame;
    this->modified = true;
}

int D1Gfx::getFrameWidth(int frameIndex) const
{
    if (frameIndex < 0 || frameIndex >= this->frames.count())
        return 0;

    return this->frames[frameIndex]->getWidth();
}

int D1Gfx::getFrameHeight(int frameIndex) const
{
    if (frameIndex < 0 || frameIndex >= this->frames.count())
        return 0;

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

void D1Gfx::patch(int gfxFileIndex, bool silent)
{
    bool change = false;
    switch (gfxFileIndex) {
    case GFX_L2DOORS: // patch L2Doors.CEL
        change = this->patchCatacombsDoors(silent);
        break;
    case GFX_L3DOORS: // patch L3Doors.CEL
        change = this->patchCavesDoors(silent);
        break;
    }
    if (!change && !silent) {
        dProgress() << tr("No change was necessary.");
    }
}
