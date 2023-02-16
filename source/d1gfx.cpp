#include "d1gfx.h"

#include <QApplication>
#include <QMessageBox>

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

int D1GfxFrame::getHeight() const
{
    return this->height;
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
    return const_cast<std::vector<std::vector<D1GfxPixel>>>(this->pixels);
}

bool D1GfxFrame::setPixel(int x, int y, D1GfxPixel pixel)
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

void D1GfxFrame::replacePixels(quint8 startColorIndex, quint8 endColorIndex, D1GfxPixel replacement)
{
    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            D1GfxPixel d1pix = this->pixels[y][x]; // this->getPixel(x, y);

            if (d1pix.isTransparent())
                continue;

            if (d1pix.getPaletteIndex() >= startColorIndex && d1pix.getPaletteIndex() <= endColorIndex)
                this->pixels[y][x] = replacement;
        }
    }
}

D1Gfx::~D1Gfx()
{
    qDeleteAll(this->frames);
    this->frames.clear();
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
    if (this->palette == nullptr || frameIndex < 0 || frameIndex >= this->frames.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Image of an invalid frame %1 requested. Frame count: %2, palette: %3").arg(frameIndex).arg(this->frames.count()).arg(this->palette != nullptr));
#endif
        return QImage();
    }

    D1GfxFrame *frame = this->frames[frameIndex];

    QImage image = QImage(
        frame->getWidth(),
        frame->getHeight(),
        QImage::Format_ARGB32);

    for (int y = 0; y < frame->getHeight(); y++) {
        for (int x = 0; x < frame->getWidth(); x++) {
            D1GfxPixel d1pix = frame->getPixel(x, y);

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
