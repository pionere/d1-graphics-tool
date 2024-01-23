#include "d1gfx.h"

#include <QApplication>
#include <QMessageBox>

#include "d1image.h"
#include "d1cl2.h"
#include "d1smk.h"
#include "openasdialog.h"
#include "progressdialog.h"
#include "remapdialog.h"

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
    bool clipped;
    if (!this->frames.isEmpty()) {
        clipped = this->frames[0]->isClipped();
    } else {
        clipped = this->type == D1CEL_TYPE::V2_MONO_GROUP || this->type == D1CEL_TYPE::V2_MULTIPLE_GROUPS;
    }

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
    bool clipped = this->frames[idx]->isClipped();

    D1GfxFrame *frame = new D1GfxFrame();
    D1ImageFrame::load(*frame, pixels, clipped, this->palette);
    this->setFrame(idx, frame);

    return this->frames[idx];
}

D1GfxFrame *D1Gfx::replaceFrame(int idx, const QImage &image)
{
    bool clipped = this->frames[idx]->isClipped();

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
    bool clipped;
    if (!this->frames.isEmpty()) {
        clipped = this->frames[0]->isClipped();
    } else {
        clipped = this->type == D1CEL_TYPE::V2_MONO_GROUP || this->type == D1CEL_TYPE::V2_MULTIPLE_GROUPS;
    }
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
        QString msg = tr("Using color %1 instead of %2 in frame(s) %3-%4", "", rangeTo - rangeFrom + 1).arg(i).arg(colIdx).arg(rangeFrom + 1).arg(rangeTo + 1);
        if (verbose != 1) {
            QFileInfo fileInfo(this->gfxFilePath);
            QString labelText = fileInfo.fileName();
            msg = msg.append(" of %1").arg(labelText);
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

unsigned D1Gfx::getFrameLen() const
{
    return this->frameLen;
}

void D1Gfx::setFrameLen(unsigned frameLen)
{
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
    constexpr int height = 96;
    constexpr int width = 96;

    if (this->getGroupCount() < 2) {
        dProgressErr() << tr("Not enough frame groups in the graphics.");
        return false;
    }
    if (this->getGroupFrameIndices(1).first != frameCount && this->getGroupFrameIndices(1).second != 2 * frameCount - 1) {
        dProgressErr() << tr("Not enough frames in the first frame group.");
        return false;
    }
    if (stdGfx.getGroupCount() < 2) {
        dProgressErr() << tr("Not enough frame groups in '%1'.").arg(QDir::toNativeSeparators(stdPath));
        return false;
    }
    if (stdGfx.getGroupFrameIndices(1).first != frameCount && stdGfx.getGroupFrameIndices(1).second != 2 * frameCount - 1) {
        dProgressErr() << tr("Not enough frames in the first frame group of '%1'.").arg(QDir::toNativeSeparators(stdPath));
        return false;
    }
    if (atkGfx.getGroupCount() < 2) {
        dProgressErr() << tr("Not enough frame groups in '%1'.").arg(QDir::toNativeSeparators(atkPath));
        return false;
    }

    bool result = false;
    for (int n = 1; n < frameCount + 1; n++) {
        D1GfxFrame* frameSrcStd = stdGfx.getFrame(stdGfx.getGroupFrameIndices(1).first + n - 1);
        if (frameSrcStd->getWidth() != width || frameSrcStd->getHeight() != height) {
            dProgressErr() << tr("Frame size of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(stdPath)).arg(width).arg(height);
            return false;
        }
        constexpr int atkWidth = 128;
        D1GfxFrame* frameSrcAtk = atkGfx.getFrame(atkGfx.getGroupFrameIndices(1).first);
        if (frameSrcAtk->getWidth() != atkWidth || frameSrcAtk->getHeight() != height) {
            dProgressErr() << tr("Frame size of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(atkPath)).arg(atkWidth).arg(height);
            return false;
        }
        // copy the shield to the stand frame
        int dy = 0;
        switch (n) {
        case 1: dy = 0; break;
        case 2: dy = 1; break;
        case 3: dy = 2; break;
        case 4: dy = 2; break;
        case 5: dy = 3; break;
        case 6: dy = 3; break;
        case 7: dy = 3; break;
        case 8: dy = 2; break;
        case 9: dy = 2; break;
        case 10: dy = 1; break;
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
        //if (n > 2 && n < 10) {
        if (dy > 1) {
            // if (n >= 5 && n <= 7) {
            if (dy == 3) {
                frameSrcStd->setPixel(17, 75, D1GfxPixel::colorPixel(0));
            }
            else {
                frameSrcStd->setPixel(15, 74, D1GfxPixel::colorPixel(0));
            }
        }
        // - sink effect on the top-right side
        // if (n > 2 && n < 10) {
        if (dy > 1) {
            frameSrcStd->setPixel(27, 75, D1GfxPixel::colorPixel(0));
            // if (n > 4 && n < 8) {
            if (dy == 3) {
                frameSrcStd->setPixel(26, 74, D1GfxPixel::colorPixel(0));
            }
        }
        // - sink effect on the bottom
        // if (n > 1) {
        if (dy != 0) {
            frameSrcStd->setPixel(28, 80, D1GfxPixel::colorPixel(0));
        }
        // if (n > 2 && n < 6) {
        if (dy > 1) {
            frameSrcStd->setPixel(29, 80, D1GfxPixel::colorPixel(0));
        }
        // if (n > 4 && n < 8) {
        if (dy == 3) {
            frameSrcStd->setPixel(27, 80, D1GfxPixel::colorPixel(0));
        }

        // copy the result to the active graphics
        D1GfxFrame* frame = this->getFrame(this->getGroupFrameIndices(1).first + n - 1);
        if (frame->getWidth() != width || frame->getHeight() != height) {
            dProgressErr() << tr("Frame size of '%1' does not fit (Expected %2x%3).").arg(QDir::toNativeSeparators(this->getFilePath())).arg(width).arg(height);
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
            if (!silent) {
                dProgress() << QApplication::tr("Frame %1 of group 2 is modified.").arg(n);
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
    case GFX_PLR_WMHAS: // patch WMHAS.CL2
        change = this->patchWarriorStand(silent);
        break;
    case GFX_SPL_ICONS: // patch SpelIcon.CEL
        change = this->patchSplIcons(silent);
        break;
    }
    if (!change && !silent) {
        dProgress() << tr("No change was necessary.");
    }
}
