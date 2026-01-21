#pragma once

#include <map>
#include <vector>

#include <QImage>
#include <QList>
#include <QPair>
#include <QPointer>
#include <QRect>
#include <QString>
#include <QtEndian>

#include "d1pal.h"

// TODO: move these to some persistency class?
#define SUB_HEADER_SIZE 0x0A
#define CEL_BLOCK_HEIGHT 32

#define SwapLE16(X) qToLittleEndian((quint16)(X))
#define SwapLE32(X) qToLittleEndian((quint32)(X))

class D1SmkAudioData;
class RemapParam;
enum class RESIZE_PLACEMENT;
class ResizeParam;

class D1GfxPixel {
public:
    static D1GfxPixel transparentPixel();
    static D1GfxPixel colorPixel(quint8 color);

    static int countAffectedPixels(const std::vector<std::vector<D1GfxPixel>> &pixelImage, const std::pair<int, int>& colors);

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

enum class D1CEL_TYPE {
    V1_REGULAR,
    V1_COMPILATION,
    V1_LEVEL,
    V2_MONO_GROUP,
    V2_MULTIPLE_GROUPS,
    SMK,
};

class D1GfxFrame : public QObject {
    Q_OBJECT

    friend class D1Cel;
    friend class D1CelFrame;
    friend class D1Cl2;
    friend class D1Cl2Frame;
    friend class D1CelTileset;
    friend class D1CelTilesetFrame;
    friend class D1Gfx;
    friend class D1ImageFrame;
    friend class D1Pcx;
    friend class D1Wav;
    friend class D1Smk;
    friend class Upscaler;

public:
    static QString frameTypeToStr(D1CEL_FRAME_TYPE frameType);

    D1GfxFrame() = default;
    D1GfxFrame(const D1GfxFrame &o);
    ~D1GfxFrame();

    QRect getBoundary() const;

    int getWidth() const;
    void setWidth(int width);
    int getHeight() const;
    void setHeight(int height);
    D1GfxPixel getPixel(int x, int y) const;
    std::vector<std::vector<D1GfxPixel>> &getPixels() const;
    bool setPixel(int x, int y, const D1GfxPixel pixel);
    D1CEL_FRAME_TYPE getFrameType() const;
    void setFrameType(D1CEL_FRAME_TYPE type);
    bool addTo(const D1GfxFrame &frame);
    void addPixelLine(std::vector<D1GfxPixel> &&pixelLine);
    bool replacePixels(const QList<QPair<D1GfxPixel, D1GfxPixel>> &replacements);
    bool testResize(int width, int height, RESIZE_PLACEMENT placement, const D1GfxPixel &backPixel) const;
    bool resize(int width, int height, RESIZE_PLACEMENT placement, const D1GfxPixel &backPixel);
    bool shift(int dx, int dy, int sx, int sy, int ex, int ey);
    bool copy(int dx, int dy, const D1GfxFrame* srcFrame, int sx, int sy, int ex, int ey);
    bool flipHorizontal();
    bool flipVertical();
    bool mask(const D1GfxFrame *frame);
    bool optimize(D1CEL_TYPE type);

    // functions for smk-frames
    QPointer<D1Pal>& getFramePal();
    void setFramePal(D1Pal *pal);
    D1SmkAudioData *getFrameAudio();

protected:
    int width = 0;
    int height = 0;
    std::vector<std::vector<D1GfxPixel>> pixels;
    // fields of tileset-frames
    D1CEL_FRAME_TYPE frameType = D1CEL_FRAME_TYPE::TransparentSquare;
    // fields of smk-frames
    QPointer<D1Pal> framePal = nullptr;
    D1SmkAudioData *frameAudio = nullptr;
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
    GFX_PLR_RHTAT,   // graphics of rogue in heavy armor with staff attacking (RHTAT.CL2)
    GFX_PLR_RHUHT,   // graphics of rogue in heavy armor with shield getting hit (RHUHT.CL2)
    GFX_PLR_RHUQM,   // graphics of rogue in heavy armor with shield casting magic (RHUQM.CL2)
    GFX_PLR_RLHAS,   // graphics of rogue in light armor with mace and shield standing in dungeon (RLHAS.CL2)
    GFX_PLR_RLHAT,   // graphics of rogue in light armor with mace and shield attacking (RLHAT.CL2)
    GFX_PLR_RLHAW,   // graphics of rogue in light armor with mace and shield walking in dungeon (RLHAW.CL2)
    GFX_PLR_RLHBL,   // graphics of rogue in light armor with mace and shield blocking (RLHBL.CL2)
    GFX_PLR_RLHFM,   // graphics of rogue in light armor with mace and shield casting fire (RLHFM.CL2)
    GFX_PLR_RLHLM,   // graphics of rogue in light armor with mace and shield casting lightning (RLHLM.CL2)
    GFX_PLR_RLHHT,   // graphics of rogue in light armor with mace and shield getting hit (RLHHT.CL2)
    GFX_PLR_RLHQM,   // graphics of rogue in light armor with mace and shield casting magic (RLHQM.CL2)
    GFX_PLR_RLHST,   // graphics of rogue in light armor with mace and shield standing in town (RLHST.CL2)
    GFX_PLR_RLHWL,   // graphics of rogue in light armor with mace and shield walking in town (RLHWL.CL2)
    GFX_PLR_RLMAT,   // graphics of rogue in light armor with mace attacking (RLMAT.CL2)
    GFX_PLR_RMDAW,   // graphics of rogue in medium armor with sword and shield walking in dungeon (RMDAW.CL2)
    GFX_PLR_RMHAT,   // graphics of rogue in medium armor with mace and shield attacking (RMHAT.CL2)
    GFX_PLR_RMMAT,   // graphics of rogue in medium armor with mace attacking (RMMAT.CL2)
    GFX_PLR_RMBFM,   // graphics of rogue in medium armor with bow casting fire (RMBFM.CL2)
    GFX_PLR_RMBLM,   // graphics of rogue in medium armor with bow casting lightning (RMBLM.CL2)
    GFX_PLR_RMBQM,   // graphics of rogue in medium armor with bow casting magic (RMBQM.CL2)
    GFX_PLR_RMTAT,   // graphics of rogue in medium armor with staff attacking (RMTAT.CL2)
    GFX_PLR_WHMAT,   // graphics of warrior in heavy armor with mace attacking (WHMAT.CL2)
    GFX_PLR_WLNLM,   // graphics of warrior in light armor unarmed casting lightning (WLNLM.CL2)
    GFX_PLR_WMDLM,   // graphics of warrior in medium armor with sword and shield casting lightning (WMDLM.CL2)
    GFX_PLR_WMHAS,   // graphics of warrior in medium armor with mace and shield standing in dungeon (WMHAS.CL2)
    GFX_MON_ACIDD,   // graphics of the Acid Beast dying (Acidd.CL2)
    GFX_MON_FALLGD,  // graphics of the Devil Kin Brute dying (Fallgd.CL2)
    GFX_MON_FALLGW,  // graphics of the Devil Kin Brute walking (Fallgw.CL2)
    GFX_MON_MAGMAD,  // graphics of the Magma Demon dying (Magmad.CL2)
    GFX_MON_MAGMAW,  // graphics of the Magma Demon walking (Magmaw.CL2)
    GFX_MON_GOATBD,  // graphics of the Flesh Clan (Bow) dying (GoatBd.CL2)
    GFX_MON_GOATLD,  // graphics of the Satyr Lord dying (GoatLd.CL2)
    GFX_MON_SCAVH,   // graphics of the Scavenger getting hit (Scavh.CL2)
    GFX_MON_SKLAXD,  // graphics of the Skeleton Axe dying (SklAxd.CL2)
    GFX_MON_SKLBWD,  // graphics of the Skeleton Bow dying (SklBwd.CL2)
    GFX_MON_SKINGS,  // graphics of the Skeleton King special movement (Skings.CL2)
    GFX_MON_SKINGW,  // graphics of the Skeleton King walking (Skingw.CL2)
    GFX_MON_SKLSRD,  // graphics of the Skeleton Sword dying (SklSrd.CL2)
    GFX_MON_SNAKEH,  // graphics of the Snake getting hit (Snakeh.CL2)
    GFX_MON_UNRAVA,  // graphics of the The Shredded attacking (Unrava.CL2)
    GFX_MON_UNRAVD,  // graphics of the The Shredded dying (Unravd.CL2)
    GFX_MON_UNRAVH,  // graphics of the The Shredded getting hit (Unravh.CL2)
    GFX_MON_UNRAVN,  // graphics of the The Shredded standing (Unravn.CL2)
    GFX_MON_UNRAVS,  // graphics of the The Shredded special movement (Unravs.CL2)
    GFX_MON_UNRAVW,  // graphics of the The Shredded walking (Unravw.CL2)
    GFX_MON_ZOMBIED, // graphics of the Zombie dying (Zombied.CL2)
    GFX_MIS_ACIDBF1,  // missile animation (Acidbf1.CL2)
    GFX_MIS_ACIDBF10, // missile animation (Acidbf10.CL2)
    GFX_MIS_ACIDBF11, // missile animation (Acidbf11.CL2)
    GFX_MIS_FIREBA2,  // missile animation (Fireba2.CL2)
    GFX_MIS_FIREBA3,  // missile animation (Fireba3.CL2)
    GFX_MIS_FIREBA5,  // missile animation (Fireba5.CL2)
    GFX_MIS_FIREBA6,  // missile animation (Fireba6.CL2)
    GFX_MIS_FIREBA8,  // missile animation (Fireba8.CL2)
    GFX_MIS_FIREBA9,  // missile animation (Fireba9.CL2)
    GFX_MIS_FIREBA10, // missile animation (Fireba10.CL2)
    GFX_MIS_FIREBA11, // missile animation (Fireba11.CL2)
    GFX_MIS_FIREBA12, // missile animation (Fireba12.CL2)
    GFX_MIS_FIREBA15, // missile animation (Fireba15.CL2)
    GFX_MIS_FIREBA16, // missile animation (Fireba16.CL2)
    GFX_MIS_HOLY2,    // missile animation (Holy2.CL2)
    GFX_MIS_HOLY3,    // missile animation (Holy3.CL2)
    GFX_MIS_HOLY5,    // missile animation (Holy5.CL2)
    GFX_MIS_HOLY6,    // missile animation (Holy6.CL2)
    GFX_MIS_HOLY8,    // missile animation (Holy8.CL2)
    GFX_MIS_HOLY9,    // missile animation (Holy9.CL2)
    GFX_MIS_HOLY10,   // missile animation (Holy10.CL2)
    GFX_MIS_HOLY11,   // missile animation (Holy11.CL2)
    GFX_MIS_HOLY12,   // missile animation (Holy12.CL2)
    GFX_MIS_HOLY15,   // missile animation (Holy15.CL2)
    GFX_MIS_HOLY16,   // missile animation (Holy16.CL2)
    GFX_MIS_MAGBALL2, // missile animation (Magball2.CL2)
    GFX_SPL_ICONS,   // spell icons (SpelIcon.CEL)
    GFX_CURS_ICONS,  // cursor icons (ObjCurs.CEL)
    GFX_ITEM_ARMOR2,   // item drop animation (Armor2.CEL)
    GFX_ITEM_GOLDFLIP, // item drop animation (GoldFlip.CEL)
    GFX_ITEM_MACE,     // item drop animation (Mace.CEL)
    GFX_ITEM_STAFF,    // item drop animation (Staff.CEL)
    GFX_ITEM_RING,     // item drop animation (Ring.CEL)
    GFX_ITEM_CROWNF,   // item drop animation (CrownF.CEL)
    GFX_ITEM_LARMOR,   // item drop animation (LArmor.CEL)
    GFX_ITEM_WSHIELD,  // item drop animation (WShield.CEL)
    GFX_ITEM_SCROLL,   // item drop animation (Scroll.CEL)
    GFX_ITEM_FEAR,     // item drop animation (FEar.CEL)
    GFX_ITEM_FBRAIN,   // item drop animation (FBrain.CEL)
    GFX_ITEM_FMUSH,    // item drop animation (FMush.CEL)
    GFX_ITEM_INNSIGN,  // item drop animation (Innsign.CEL)
    GFX_ITEM_BLDSTN,   // item drop animation (Bldstn.CEL)
    GFX_ITEM_FANVIL,   // item drop animation (Fanvil.CEL)
    GFX_ITEM_FLAZSTAF, // item drop animation (FLazStaf.CEL)
    GFX_ITEM_TEDDYS1,  // item drop animation (teddys1.CEL)
    GFX_ITEM_COWS1,    // item drop animation (cows1.CEL)
    GFX_ITEM_DONKYS1,  // item drop animation (donkys1.CEL)
    GFX_ITEM_MOOSES1,  // item drop animation (mooses1.CEL)
} gfx_file_index;

class D1Gfx;
typedef struct D1GfxCompFrame {
    unsigned cfFrameRef;
    int cfZOrder;
    int cfOffsetX;
    int cfOffsetY;
} D1GfxCompFrame;
class D1GfxComp : public QObject {
    Q_OBJECT

    friend class D1Gfx;
public:
    D1GfxComp(D1Gfx *gfx);
    D1GfxComp(const D1GfxComp &o);
    ~D1GfxComp();

    D1Gfx *getGFX() const;
    void setGFX(D1Gfx *g);
    QString getLabel() const;
    void setLabel(QString lbl);
    int getCompFrameCount() const;
    D1GfxCompFrame *getCompFrame(int frameIdx) const;

private:
    D1Gfx *gfx;
    QString label;
    QList<D1GfxCompFrame> compFrames;
};

class D1Gfx : public QObject {
    Q_OBJECT

    friend class D1Cel;
    friend class D1Cl2;
    friend class D1CelTileset;
    friend class D1Font;
    friend class D1Min;
    friend class D1Pcx;
    friend class D1Wav;
    friend class D1Smk;
    friend class Upscaler;

public:
    D1Gfx() = default;
    ~D1Gfx();

    void compareTo(const D1Gfx *gfx, QString &header, bool patchData) const;
    QRect getBoundary() const;

    void clear();

    QSize getFrameSize() const;
    int getGroupSize() const;
    QString getFramePixels(int frameIndex, bool values) const;
    QImage getFrameImage(int frameIndex, int component = 0) const;
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
    void flipHorizontalFrame(int frameIndex, bool wholeGroup);
    void flipVerticalFrame(int frameIndex, bool wholeGroup);
    void remapFrames(const std::map<unsigned, unsigned> &remap);
    void swapFrames(unsigned frameIndex0, unsigned frameIndex1);
    void mergeFrames(unsigned frameIndex0, unsigned frameIndex1);
    void addGfx(D1Gfx *gfx);
    void replacePixels(const QList<QPair<D1GfxPixel, D1GfxPixel>> &replacements, const RemapParam &params, int verbose);
    void inefficientFrames() const;
    int testResize(const ResizeParam &params);
    bool resize(const ResizeParam &params);
    void mask();
    bool squash();
    void optimize();
    bool check() const;

    D1CEL_TYPE getType() const;
    void setType(D1CEL_TYPE type);
    bool isPatched() const;
    void setPatched(bool patched);
    bool isClipped() const;
    bool setClipped(bool clipped);
    bool isUpscaled() const;
    void setUpscaled(bool upscaled);
    unsigned getFrameLen() const;
    void setFrameLen(unsigned frameLen);
    QString getFilePath() const;
    void setFilePath(const QString &filePath);
    bool isModified() const;
    void setModified(bool modified = true);
    void frameModified(const D1GfxFrame *frame);
    D1Pal *getPalette() const;
    void setPalette(D1Pal *pal);
    void reencode(D1Pal *pal);
    int getGroupCount() const;
    std::pair<int, int> getGroupFrameIndices(int groupIndex) const;
    int getFrameCount() const;
    D1GfxFrame *getFrame(int frameIndex) const;
    void setFrame(int frameIndex, D1GfxFrame *frame);
    int getFrameWidth(int frameIndex) const;
    int getFrameHeight(int frameIndex) const;
    bool setFrameType(int frameIndex, D1CEL_FRAME_TYPE frameType);

    QString getCompFilePath() const;
    void setCompFilePath(const QString &filePath);
    D1Gfx *loadComponentGFX(QString gfxFilePath);
    void saveComponents();
    int getComponentCount() const;
    D1GfxComp *getComponent(int compIndex);
    void removeComponent(int compIndex);
    D1GfxComp *insertComponent(int compIndex, D1Gfx *gfx);
    QRect getFrameRect(int frameIndex, bool full) const;

    void patch(int gfxFileIndex, bool silent); // gfx_file_index
    static int getPatchFileIndex(QString &filePath);
    static QString clippedtoStr(bool clipped);

private:
    bool patchCathedralDoors(bool silent);
    bool patchCatacombsDoors(bool silent);
    bool patchCavesDoors(bool silent);
    bool patchMagicCircle(bool silent);
    bool patchCandle(bool silent);
    bool patchLeftShrine(bool silent);
    bool patchRightShrine(bool silent);
    bool patchCryptLight(bool silent);
    bool patchPlrFrames(int gfxFileIndex, bool silent);
    bool patchMonFrames(int gfxFileIndex, bool silent);    
    bool patchRogueExtraPixels(int gfxFileIndex, bool silent);
    bool patchWarriorStand(bool silent);
    bool patchFallGDie(bool silent);
    bool patchFallGWalk(bool silent);
    bool patchMagmaDie(bool silent);
    bool patchGoatBDie(bool silent);
    bool patchGoatLDie(bool silent);
    bool patchSklAxDie(bool silent);
    bool patchSklBwDie(bool silent);
    bool patchSklSrDie(bool silent);
    bool patchUnrav(int gfxFileIndex, bool silent);
    bool patchZombieDie(bool silent);
    bool patchAcidbf(int gfxFileIndex, bool silent);
    bool patchFireba(int gfxFileIndex, bool silent);
    bool patchHoly(int gfxFileIndex, bool silent);
    bool patchMagball(bool silent);
    bool patchSplIcons(bool silent);
    bool patchCursorIcons(bool silent);
    bool patchItemFlips(int gfxFileIndex, bool silent);

protected:
    D1CEL_TYPE type = D1CEL_TYPE::V1_REGULAR;
    QString gfxFilePath;
    bool modified;
    D1Pal *palette = nullptr;
    std::vector<std::pair<int, int>> groupFrameIndices;
    QList<D1GfxFrame *> frames;
    // fields of cel/cl2-frames
    QList<D1GfxComp *> components;
    QString compFilePath;
    bool clipped = false;
    // fields of tilesets
    bool patched = false;
    bool upscaled = false;
    // fields of smk
    unsigned frameLen; // microsec
};
