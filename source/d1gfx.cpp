#include "d1gfx.h"

#include "d1image.h"

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

int D1GfxFrame::getWidth() const
{
    return this->width;
}

int D1GfxFrame::getHeight() const
{
    return this->height;
}

D1GfxPixel D1GfxFrame::getPixel(int x, int y) const
{
    if (x >= 0 && x < this->width && y >= 0 && y < this->height)
        return this->pixels[y][x];

    return D1GfxPixel::transparentPixel();
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

bool D1Gfx::isFrameSizeConstant()
{
    if (this->frames.isEmpty()) {
        return false;
    }

    int frameWidth = this->frames[0].getWidth();
    int frameHeight = this->frames[0].getHeight();

    for (int i = 1; i < this->frames.count(); i++) {
        if (this->frames[i].getWidth() != frameWidth
            || this->frames[i].getHeight() != frameHeight)
            return false;
    }

    return true;
}

// builds QImage from a D1CelFrame of given index
QImage D1Gfx::getFrameImage(quint16 frameIndex)
{
    if (this->palette == nullptr || frameIndex >= this->frames.count())
        return QImage();

    D1GfxFrame &frame = this->frames[frameIndex];

    QImage image = QImage(
        frame.getWidth(),
        frame.getHeight(),
        QImage::Format_ARGB32);

    for (int y = 0; y < frame.getHeight(); y++) {
        for (int x = 0; x < frame.getWidth(); x++) {
            D1GfxPixel d1pix = frame.getPixel(x, y);

            QColor color;
            if (d1pix.isTransparent())
                color = QColor(Qt::transparent);
            else
                color = this->palette->getColor(d1pix.getPaletteIndex());

            image.setPixel(x, y, color.rgba());
        }
    }

    return image;
}

D1GfxFrame *D1Gfx::insertFrame(int idx, const QImage &image)
{
    bool clipped;

    if (!this->frames.isEmpty()) {
        clipped = this->frames[0].isClipped();
    } else {
        clipped = this->type == D1CEL_TYPE::V2_MONO_GROUP || this->type == D1CEL_TYPE::V2_MULTIPLE_GROUPS;
    }

    D1GfxFrame frame;
    D1ImageFrame::load(frame, image, clipped, this->palette);
    this->frames.insert(idx, frame);
    this->modified = true;

    if (this->groupFrameIndices.isEmpty()) {
        // create new group if this is the first frame
        this->groupFrameIndices.append(qMakePair(0, 0));
    } else if (this->frames.count() == idx + 1) {
        // extend the last group if appending a frame
        this->groupFrameIndices.last().second = idx;
    } else {
        // extend the current group and adjust every group after it
        for (int i = 0; i < this->groupFrameIndices.count(); i++) {
            if (this->groupFrameIndices[i].second < idx)
                continue;
            if (this->groupFrameIndices[i].first > idx) {
                this->groupFrameIndices[i].first++;
            }
            this->groupFrameIndices[i].second++;
        }
    }
    return &this->frames[idx];
}

D1GfxFrame *D1Gfx::replaceFrame(int idx, const QImage &image)
{
    bool clipped = this->frames[idx].isClipped();

    D1GfxFrame frame;
    D1ImageFrame::load(frame, image, clipped, this->palette);
    this->frames[idx] = frame;
    this->modified = true;

    return &this->frames[idx];
}

void D1Gfx::removeFrame(quint16 idx)
{
    this->frames.removeAt(idx);
    this->modified = true;

    for (int i = 0; i < this->groupFrameIndices.count(); i++) {
        if (this->groupFrameIndices[i].second < idx)
            continue;
        if (this->groupFrameIndices[i].second == idx && this->groupFrameIndices[i].first == idx) {
            this->groupFrameIndices.removeAt(i);
            i--;
            continue;
        }
        if (this->groupFrameIndices[i].first > idx) {
            this->groupFrameIndices[i].first--;
        }
        this->groupFrameIndices[i].second--;
    }
}

void D1Gfx::remapFrames(const QMap<unsigned, unsigned> &remap)
{
    QList<D1GfxFrame> newFrames;
    // assert(this->groupFrameIndices.count() == 1);
    for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
        newFrames.append(this->frames.at(iter.value() - 1));
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

void D1Gfx::setModified()
{
    this->modified = true;
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

void D1Gfx::setPalette(D1Pal *pal)
{
    this->palette = pal;
}

int D1Gfx::getGroupCount() const
{
    return this->groupFrameIndices.count();
}

QPair<quint16, quint16> D1Gfx::getGroupFrameIndices(int groupIndex) const
{
    if (groupIndex < 0 || groupIndex >= this->groupFrameIndices.count())
        return qMakePair(0, 0);

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

    return &this->frames[frameIndex];
}

int D1Gfx::getFrameWidth(int frameIndex) const
{
    if (frameIndex < 0 || frameIndex >= this->frames.count())
        return 0;

    return this->frames[frameIndex].getWidth();
}

int D1Gfx::getFrameHeight(int frameIndex) const
{
    if (frameIndex < 0 || frameIndex >= this->frames.count())
        return 0;

    return this->frames[frameIndex].getHeight();
}

bool D1Gfx::setFrameType(int frameIndex, D1CEL_FRAME_TYPE frameType)
{
    if (this->frames[frameIndex].getFrameType() == frameType) {
        return false;
    }
    this->frames[frameIndex].setFrameType(frameType);
    this->modified = true;
    return true;
}
