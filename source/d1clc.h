#pragma once

#include <QDir>
#include <QJsonArray>
#include <QJsonValue>
#include <QString>

#include "d1gfx.h"
#include "openasdialog.h"
#include "saveasdialog.h"

class D1GfxComp;

class D1Clc {
private:
    static constexpr const char *CLC_MAIN_GFX = "MainGfx";
    static constexpr const char *CLC_COMPONENTS = "Components";
    static constexpr const char *CLC_COMP_GFX = "Gfx";
    static constexpr const char *CLC_COMP_LABEL = "Label";
    static constexpr const char *CLC_COMP_FRAMES = "Frames";
    static constexpr const char *CLC_COMP_FRAME_IDX = "idx";
    static constexpr const char *CLC_COMP_FRAME_REF = "ref";
    static constexpr const char *CLC_COMP_FRAME_Z = "z";
    static constexpr const char *CLC_COMP_FRAME_X = "x";
    static constexpr const char *CLC_COMP_FRAME_Y = "y";

public:
    static bool load(D1Gfx &gfx, const QString &clcFilePath, const OpenAsParam &params);
    static bool save(D1Gfx &gfx, const SaveAsParam &params);

private:
    static bool loadCompFrame(D1GfxComp &comp, const QJsonValue &jsonVal);
    static bool loadComponent(D1Gfx &gfx, const QJsonValue &jsonVal, const QString &workDir);
    static void storeCompFrame(QJsonArray &jsonArray, const D1GfxComp &comp, int frameIdx);
    static void storeComponent(QJsonArray &jsonArray, const D1GfxComp &comp, const QDir &workDir);
};
