#pragma once

#include <map>
#include <set>
#include <vector>

#include "d1gfx.h"
#include "d1min.h"
#include "d1sla.h"
#include "d1til.h"
#include "d1tla.h"
#include "openasdialog.h"
#include "saveasdialog.h"

#include "dungeon/defs.h"
#include "dungeon/enums.h"

struct _CelMicro;

class D1Tileset {
public:
    D1Tileset(D1Gfx *gfx);
    ~D1Tileset();

    bool loadCls(const QString &clsFilePath, const OpenAsParam &params);
    bool load(const OpenAsParam &params);
    void save(const SaveAsParam &params);

    void createFrame(int frameIndex);
    void removeFrame(int frameIndex, int replacement);
    void createTile(int tileIndex);
    void insertTile(int tileIndex, const std::vector<int> &subtileIndices);
    int duplicateTile(int tileIndex, bool deepCopy);
    void removeTile(int tileIndex);
    void createSubtile(int subtileIndex);
    void insertSubtile(int subtileIndex, const std::vector<unsigned> &frameReferencesList);
    int duplicateSubtile(int subtileIndex, bool deepCopy);
    void removeSubtile(int subtileIndex, int replacement);
    void resetSubtileFlags(int subtileIndex);
    void remapSubtiles(const std::map<unsigned, unsigned> &remap);
    void swapFrames(unsigned frameIndex0, unsigned frameIndex1);
    void swapSubtiles(unsigned subtileIndex0, unsigned subtileIndex1);
    void swapTiles(unsigned tileIndex0, unsigned tileIndex1);
    bool reuseFrames(std::set<int> &removedIndices, bool silent);
    bool reuseSubtiles(std::map<unsigned, unsigned> &remap);
    bool reuseTiles(std::map<unsigned, unsigned> &remap);

    void patch(int dunType, bool silent);

    D1Gfx *gfx;
    D1Gfx *cls;
    D1Min *min;
    D1Til *til;
    D1Sla *sla;
    D1Tla *tla;

private:
    void saveCls(const SaveAsParam &params);

    std::pair<unsigned, D1GfxFrame *> getFrame(int subtileIndex, int blockSize, unsigned microIndex);

    bool maskMicro(int idx, int x0, int x1, int y0, int y1, int blockSize, const _CelMicro* micros);
    bool moveMicroPixels(int src, int dst, int blockSize, const _CelMicro* micros);
    bool moveUpperMicroPixels(int src, int dst, int blockSize, const _CelMicro* micros);
    bool moveLowerMicroPixels(int src, int dst, int blockSize, const _CelMicro* micros);
    bool moveLimitedMicroPixels(int src, int dst, int x0, int x1, int blockSize, const _CelMicro* micros);
    bool moveLimitedUpperMicroPixels(int src, int dst, int x0, int x1, int blockSize, const _CelMicro* micros);
    bool moveLimitedLowerMicroPixels(int src, int dst, int x0, int x1, int blockSize, const _CelMicro* micros);
    bool shiftMicrosDown(int m0, int m1, int blockSize, const _CelMicro* micros);

    void patchTownPot(int potLeftSubtileRef, int potRightSubtileRef, bool silent);
    bool patchTownCathedral(bool silent);
    bool patchTownFloor(bool silent);
    bool patchTownDoor(bool silent);
    bool patchTownLight(bool silent);
    void patchTownChop(bool silent);
    void cleanupTown(std::set<unsigned> &deletedFrames, bool silent);
    //void patchTownSpec(bool silent);

    bool patchCathedralFloor(bool silent);
    bool fixCathedralShadows(bool silent);
    void cleanupCathedral(std::set<unsigned> &deletedFrames, bool silent);
    void patchCathedralSpec(bool silent);

    bool patchCatacombsStairs(int backTileIndex1, int backTileIndex2, int extTileIndex1, int extTileIndex2, int stairsSubtileRef1, int stairsSubtileRef2, bool silent);
    bool patchCatacombsFloor(bool silent);
    bool fixCatacombsShadows(bool silent);
    void cleanupCatacombs(std::set<unsigned> &deletedFrames, bool silent);
    void patchCatacombsSpec(bool silent);

    bool patchCavesFloor(bool silent);
    bool patchCavesStairs(bool silent);
    bool patchCavesWall1(bool silent);
    bool patchCavesWall2(bool silent);
    void cleanupCaves(std::set<unsigned> &deletedFrames, bool silent);

    bool patchHellChaos(bool silent);
    bool patchHellFloor(bool silent);
    bool patchHellStairs(bool silent);
    bool patchHellWall1(bool silent);
    bool patchHellWall2(bool silent);
    void cleanupHell(std::set<unsigned> &deletedFrames, bool silent);

    bool patchNestFloor(bool silent);
    bool patchNestStairs(bool silent);
    bool patchNestWall1(bool silent);
    bool patchNestWall2(bool silent);
    void cleanupNest(std::set<unsigned> &deletedFrames, bool silent);

    void patchCryptFloor(bool silent);
    void maskCryptBlacks(bool silent);
    void fixCryptShadows(bool silent);
    void cleanupCrypt(std::set<unsigned> &deletedFrames, bool silent);
    void patchCryptSpec(bool silent);
};
