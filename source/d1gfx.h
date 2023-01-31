#pragma once

#include <QImage>
#include <QMap>
#include <QtEndian>

#include "d1celtilesetframe.h"
#include "d1pal.h"

// TODO: move these to some persistency class?
#define SUB_HEADER_SIZE 0x0A
#define CEL_BLOCK_HEIGHT 32

#define SwapLE16(X) qToLittleEndian((quint16)(X))
#define SwapLE32(X) qToLittleEndian((quint32)(X))

class D1GfxPixel {
public:
    static D1GfxPixel transparentPixel();
    static D1GfxPixel colorPixel(quint8 color);

    ~D1GfxPixel() = default;

    bool isTransparent() const;
    quint8 getPaletteIndex() const;

    friend bool operator==(const D1GfxPixel &lhs, const D1GfxPixel &rhs);
    friend bool operator!=(const D1GfxPixel &lhs, const D1GfxPixel &rhs);

private:
    D1GfxPixel() = default;

    bool transparent = false;
    quint8 paletteIndex = 0;
};

class D1GfxFrame : public QObject {
    Q_OBJECT

    friend class D1Cel;
    friend class D1CelFrame;
    friend class D1Cl2;
    friend class D1Cl2Frame;
    friend class D1CelTileset;
    friend class D1CelTilesetFrame;
    friend class D1ImageFrame;
    friend class D1Pcx;
    friend class Upscaler;

public:
    D1GfxFrame() = default;
    D1GfxFrame(D1GfxFrame &o);
    ~D1GfxFrame() = default;

    int getWidth() const;
    int getHeight() const;
    D1GfxPixel getPixel(const QPoint &pos) const;
    bool setPixel(const QPoint &pos, D1GfxPixel pixel);
    bool isClipped() const;
    D1CEL_FRAME_TYPE getFrameType() const;
    void setFrameType(D1CEL_FRAME_TYPE type);
    bool addTo(const D1GfxFrame &frame);
    void addPixelLine(QList<D1GfxPixel> &pixelLine);
    void replacePixels(quint8 startColorIndex, quint8 endColorIndex, D1GfxPixel pixel);

protected:
    int width = 0;
    int height = 0;
    QList<QList<D1GfxPixel>> pixels;
    // fields of cel/cl2-frames
    bool clipped = false;
    // fields of tileset-frames
    D1CEL_FRAME_TYPE frameType = D1CEL_FRAME_TYPE::TransparentSquare;
};

enum class D1CEL_TYPE {
    V1_REGULAR,
    V1_COMPILATION,
    V1_LEVEL,
    V2_MONO_GROUP,
    V2_MULTIPLE_GROUPS,
};

class D1Gfx : public QObject {
    Q_OBJECT

    friend class D1Cel;
    friend class D1Cl2;
    friend class D1CelTileset;
    friend class D1Min;
    friend class Upscaler;

public:
    D1Gfx() = default;
    ~D1Gfx();

    bool isFrameSizeConstant();
    QImage getFrameImage(quint16 frameIndex);
    D1GfxFrame *insertFrame(int frameIndex, bool *clipped);
    D1GfxFrame *insertFrame(int frameIndex, const QImage &image);
    D1GfxFrame *addToFrame(int frameIndex, const QImage &image);
    D1GfxFrame *addToFrame(int frameIndex, const D1GfxFrame &frame);
    D1GfxFrame *replaceFrame(int frameIndex, const QImage &image);
    void removeFrame(quint16 frameIndex);
    void remapFrames(const QMap<unsigned, unsigned> &remap);

    D1CEL_TYPE getType() const;
    void setType(D1CEL_TYPE type);
    bool isUpscaled() const;
    void setUpscaled(bool upscaled);
    QString getFilePath() const;
    bool isModified() const;
    void setModified(bool modified = true);
    D1Pal *getPalette();
    void setPalette(D1Pal *pal);
    int getGroupCount() const;
    QPair<quint16, quint16> getGroupFrameIndices(int groupIndex) const;
    int getFrameCount() const;
    D1GfxFrame *getFrame(int frameIndex) const;
    void setFrame(int frameIndex, D1GfxFrame *frame);
    int getFrameWidth(int frameIndex) const;
    int getFrameHeight(int frameIndex) const;
    bool setFrameType(int frameIndex, D1CEL_FRAME_TYPE frameType);

protected:
    D1CEL_TYPE type = D1CEL_TYPE::V1_REGULAR;
    QString gfxFilePath;
    bool modified;
    D1Pal *palette = nullptr;
    QList<QPair<quint16, quint16>> groupFrameIndices;
    QList<D1GfxFrame *> frames;
    // fields of tilesets
    bool upscaled = false;
};
