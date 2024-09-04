#pragma once

#include <map>
#include <vector>

#include <QImage>
#include <QList>
#include <QPair>
#include <QPointer>
#include <QtEndian>

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
    QString colorText(D1Pal *pal) const;

    friend bool operator==(const D1GfxPixel &lhs, const D1GfxPixel &rhs);
    friend bool operator!=(const D1GfxPixel &lhs, const D1GfxPixel &rhs);

private:
    D1GfxPixel() = default;

    bool transparent = false;
    quint8 paletteIndex = 0;
};

typedef struct FramePixel {
    FramePixel(const QPoint &p, D1GfxPixel px);

    QPoint pos;
    D1GfxPixel pixel;
} FramePixel;

enum class D1CEL_FRAME_TYPE {
    Square,            // opaque square (bitmap)
    TransparentSquare, // bitmap with transparent pixels
    LeftTriangle,      // opaque triangle on its left edge
    RightTriangle,     // opaque triangle on its right edge
    LeftTrapezoid,     // bottom half is a left triangle, upper half is a square
    RightTrapezoid,    // bottom half is a right triangle, upper half is a square
    Empty = -1,        // transparent frame (only for efficiency tests)
};

class D1GfxFrame : public QObject {
    Q_OBJECT

    friend class D1Cel;
    friend class D1CelFrame;
    friend class D1Cl2;
    friend class D1Cl2Frame;
    friend class D1Gfx;
    friend class D1ImageFrame;
    friend class D1Pcx;

public:
    D1GfxFrame() = default;
    D1GfxFrame(const D1GfxFrame &o);
    ~D1GfxFrame();

    int getWidth() const;
    void setWidth(int width);
    int getHeight() const;
    void setHeight(int height);
    D1GfxPixel getPixel(int x, int y) const;
    std::vector<std::vector<D1GfxPixel>> &getPixels() const;
    bool setPixel(int x, int y, const D1GfxPixel pixel);
    bool isClipped() const;
    D1CEL_FRAME_TYPE getFrameType() const;
    void setFrameType(D1CEL_FRAME_TYPE type);
    bool addTo(const D1GfxFrame &frame);
    void addPixelLine(std::vector<D1GfxPixel> &&pixelLine);
    bool replacePixels(const QList<QPair<D1GfxPixel, D1GfxPixel>> &replacements);

    // functions for smk-frames
    QPointer<D1Pal>& getFramePal();
    void setFramePal(D1Pal *pal);

protected:
    int width = 0;
    int height = 0;
    std::vector<std::vector<D1GfxPixel>> pixels;
    // fields of cel/cl2-frames
    bool clipped = false;
    // fields of tileset-frames
    D1CEL_FRAME_TYPE frameType = D1CEL_FRAME_TYPE::TransparentSquare;
    // fields of smk-frames
    QPointer<D1Pal> framePal = nullptr;
};

typedef enum gfx_file_index {
    GFX_OBJ_L1DOORS, // graphics of the doors in the Cathedral (L1Doors.CEL)
    GFX_OBJ_L2DOORS, // graphics of the doors in the Catacombs (L2Doors.CEL)
    GFX_OBJ_L3DOORS, // graphics of the doors in the Caves (L3Doors.CEL)
    GFX_OBJ_MCIRL,   // graphics of the magic circle object (Mcirl.CEL)
    GFX_OBJ_CANDLE2, // graphics of a light stand with a candle (Candle2.CEL)
    GFX_OBJ_LSHR,    // graphics of the west-facing shrine (LShrineG.CEL)
    GFX_OBJ_RSHR,    // graphics of the east-facing shrine (RShrineG.CEL)
    GFX_OBJ_L5LIGHT, // graphics of the light stand in Crypt (L5Light.CEL)
    GFX_PLR_WMHAS,   // graphics of the warrior with shield and mace standing in the dungeon (WMHAS.CL2)
    GFX_MON_FALLGW,  // graphics of the Devil Kin Brute walking (Fallgw.CL2)
    GFX_MON_GOATLD,  // graphics of the Satyr Lord dying (GoatLd.CL2)
    GFX_SPL_ICONS,   // spell icons (SpelIcon.CEL)
    GFX_CURS_ICONS,  // cursor icons (ObjCurs.CEL)
} gfx_file_index;

enum class D1CEL_TYPE {
    V1_REGULAR,
    V1_COMPILATION,
    V1_LEVEL,
    V2_MONO_GROUP,
    V2_MULTIPLE_GROUPS,
    SMK,
};

class D1Gfx : public QObject {
    Q_OBJECT

    friend class D1Cel;
    friend class D1Cl2;
    friend class D1Pcx;

public:
    D1Gfx() = default;
    ~D1Gfx();

    void compareTo(const D1Gfx *gfx, QString header) const;

    void clear();

    bool isFrameSizeConstant() const;
    QString getFramePixels(int frameIndex, bool values) const;
    QImage getFrameImage(int frameIndex) const;
    std::vector<std::vector<D1GfxPixel>> getFramePixelImage(int frameIndex) const;
    void insertFrame(int frameIndex, int width, int height);
    D1GfxFrame *insertFrame(int frameIndex);
    D1GfxFrame *insertFrame(int frameIndex, const QString &pixels);
    D1GfxFrame *insertFrame(int frameIndex, const QImage &image);
    D1GfxFrame *addToFrame(int frameIndex, const QImage &image);
    D1GfxFrame *addToFrame(int frameIndex, const D1GfxFrame &frame);
    D1GfxFrame *replaceFrame(int frameIndex, const QString &pixels);
    D1GfxFrame *replaceFrame(int frameIndex, const QImage &image);
    int duplicateFrame(int frameIndex, bool wholeGroup);
    void removeFrame(int frameIndex, bool wholeGroup);
    void remapFrames(const std::map<unsigned, unsigned> &remap);
    void swapFrames(unsigned frameIndex0, unsigned frameIndex1);
    void mergeFrames(unsigned frameIndex0, unsigned frameIndex1);
    void addGfx(D1Gfx *gfx);

    D1CEL_TYPE getType() const;
    void setType(D1CEL_TYPE type);
    bool isUpscaled() const;
    void setUpscaled(bool upscaled);
    unsigned getFrameLen() const;
    void setFrameLen(unsigned frameLen);
    QString getFilePath() const;
    void setFilePath(const QString &filePath);
    bool isModified() const;
    void setModified(bool modified = true);
    D1Pal *getPalette() const;
    void setPalette(D1Pal *pal);
    int getGroupCount() const;
    std::pair<int, int> getGroupFrameIndices(int groupIndex) const;
    int getFrameCount() const;
    D1GfxFrame *getFrame(int frameIndex) const;
    void setFrame(int frameIndex, D1GfxFrame *frame);
    int getFrameWidth(int frameIndex) const;
    int getFrameHeight(int frameIndex) const;
    bool setFrameType(int frameIndex, D1CEL_FRAME_TYPE frameType);

private:
    bool isClipped(int frameIndex) const;

protected:
    D1CEL_TYPE type = D1CEL_TYPE::V1_REGULAR;
    QString gfxFilePath;
    bool modified;
    D1Pal *palette = nullptr;
    std::vector<std::pair<int, int>> groupFrameIndices;
    QList<D1GfxFrame *> frames;
    // fields of tilesets
    bool upscaled = false;
    // fields of smk
    unsigned frameLen; // microsec
};
