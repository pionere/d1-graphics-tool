#include "d1clc.h"

#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonValue>

#include "d1cel.h"
#include "d1cl2.h"
#include "progressdialog.h"

bool D1Clc::load(D1Gfx &gfx, const QString &jsonFilePath, const OpenAsParam &params)
{
    gfx.clear();

    QFile file(jsonFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // file does not exists or not readable
        return false;
    }
    QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll());
    QJsonObject jsonObj = jsonDoc.object();
    file.close();

    if (!jsonObj.contains(D1Clc::CLC_MAIN_GFX)) {
        // path to the main gfx is missing
        dProgressErr() << QApplication::tr("path to the main gfx is missing.");
        return false;
    }

    QFileInfo jsonfi = QFileInfo(jsonFilePath);
    QDir jsonDir = jsonfi.dir();
    QString workDir = jsonDir.path() + "/";
    QString gfxFilePath = workDir + jsonObj.value(D1Clc::CLC_MAIN_GFX).toString();
    gfxFilePath = QDir::cleanPath(gfxFilePath);

    QString lowerFilePath = gfxFilePath.toLower();
    if (lowerFilePath.endsWith("cel")) {
        if (!D1Cel::load(gfx, gfxFilePath, params)) {
            // main gfx file can not be loaded
            dProgressErr() << QApplication::tr("main gfx cel file can not be loaded %1.").arg(gfxFilePath);
            return false;
        }
    } else {
        if (!D1Cl2::load(gfx, gfxFilePath, params)) {
            // main gfx file can not be loaded
            dProgressErr() << QApplication::tr("main gfx cl2 file can not be loaded %1.").arg(gfxFilePath);
            return false;
        }
    }

    if (jsonObj.contains(D1Clc::CLC_COMPONENTS)) {
       QJsonValue jsonVal = jsonObj.value(D1Clc::CLC_COMPONENTS);
       if (jsonVal.isArray()) {
           QJsonArray jsonArray = jsonVal.toArray();
           for (const QJsonValue val : jsonArray) {
               if (!D1Clc::loadComponent(gfx, val, workDir)) {
                   // invalid format
                   // gfx.clear();
                   dProgressErr() << QApplication::tr("invalid component in array");
                   return false;
               }
           }
       } else {
           if (!D1Clc::loadComponent(gfx, jsonVal, workDir)) {
               // invalid format
               dProgressErr() << QApplication::tr("invalid flat component");
               // gfx.clear();
               return false;
           }
       }
    }

    return true;
}

static void invalidJsonValue(const QJsonValue &jsonVal, QString field)
{
    if (!jsonVal.isUndefined()) {
        dProgressErr() << QApplication::tr("%1 is invalid (%2).").arg(jsonVal.isString() ? jsonVal.toString() : QString("*type*: %1").arg(jsonVal.type()));
    }
}

bool D1Clc::loadCompFrame(D1GfxComp &comp, const QJsonValue &jsonVal)
{
    if (!jsonVal.isObject()) {
        invalidJsonValue(jsonVal, "Frame-object");
        return false;
    }

    QJsonObject jsonObj = jsonVal.toObject();
    QJsonValue idxVal = jsonObj.value(D1Clc::CLC_COMP_FRAME_IDX);
    if (!idxVal.isDouble() || (int)idxVal.toDouble() != idxVal.toInt()) {
        if (idxVal.isUndefined()) {
            dProgressErr() << QApplication::tr("Frame-index is missing.");
        } else {
            invalidJsonValue(idxVal, QApplication::tr("Frame-index"));
        }
        return false;
    }
    int frameIdx = idxVal.toInt();
    if (comp.getCompFrameCount() <= frameIdx) {
        dProgressErr() << QApplication::tr("Frame-index out of bound (%1 vs %2).").arg(frameIdx).arg(comp.getCompFrameCount());
        return false;
    }
    D1GfxCompFrame *compFrame = comp.getCompFrame(frameIdx);
    QJsonValue refVal = jsonObj.value(D1Clc::CLC_COMP_FRAME_REF);
    if (refVal.isDouble() && (int)refVal.toDouble() == refVal.toInt()) {
        compFrame->cfFrameRef = refVal.toInt();
    } else {
        invalidJsonValue(refVal, QApplication::tr("Frame-reference"));
    }
    QJsonValue zVal = jsonObj.value(D1Clc::CLC_COMP_FRAME_Z);
    if (zVal.isDouble() && (int)refVal.toDouble() == refVal.toInt()) {
        compFrame->cfZOrder = zVal.toInt();
    } else {
        invalidJsonValue(refVal, QApplication::tr("Z-order"));
    }
    QJsonValue xVal = jsonObj.value(D1Clc::CLC_COMP_FRAME_X);
    if (xVal.isDouble() && (int)refVal.toDouble() == refVal.toInt()) {
        compFrame->cfOffsetX = xVal.toInt();
    } else {
        invalidJsonValue(refVal, QApplication::tr("X-offset"));
    }
    QJsonValue yVal = jsonObj.value(D1Clc::CLC_COMP_FRAME_Y);
    if (yVal.isDouble() && (int)refVal.toDouble() == refVal.toInt()) {
        compFrame->cfOffsetY = yVal.toInt();
    } else {
        invalidJsonValue(refVal, QApplication::tr("Y-offset"));
    }
    return true;
}

bool D1Clc::loadComponent(D1Gfx &gfx, const QJsonValue &jsonVal, const QString &workDir)
{
    if (!jsonVal.isObject()) {
        invalidJsonValue(jsonVal, "Component-object");
        return false;
    }

    QJsonObject jsonObj = jsonVal.toObject();
    QJsonValue gfxVal = jsonObj.value(D1Clc::CLC_COMP_GFX);
    if (!gfxVal.isString()) {
        // path to the component gfx is missing
        if (gfxVal.isUndefined()) {
            dProgressErr() << QApplication::tr("Component-gfx is missing.");
        } else {
            invalidJsonValue(gfxVal, QApplication::tr("Component-gfx"));
        }
        return false;
    }

    QString gfxFilePath = workDir + gfxVal.toString();
    gfxFilePath = QDir::cleanPath(gfxFilePath);

    D1Gfx *compGfx = gfx.loadComponentGFX(gfxFilePath);
    if (compGfx == nullptr) {
        // component gfx file can not be loaded
        // dProgressErr() << QApplication::tr("component gfx file can not be loaded %1.").arg(gfxFilePath);
        return false;
    }

    int compIdx = gfx.getComponentCount();
    D1GfxComp *comp = gfx.insertComponent(compIdx, compGfx);
    if (jsonObj.contains(D1Clc::CLC_COMP_LABEL)) {
        comp->setLabel(jsonObj.value(D1Clc::CLC_COMP_LABEL).toString());
    }

    QJsonValue framesVal = jsonObj.value(D1Clc::CLC_COMP_FRAMES);
    if (!framesVal.isUndefined()) {
        if (framesVal.isArray()) {
           QJsonArray jsonArray = framesVal.toArray();
           for (const QJsonValue val : jsonArray) {
               if (!D1Clc::loadCompFrame(*comp, val)) {
                   // invalid format
                   // gfx.clear();
                   return false;
               }
           }
       } else {
           if (!D1Clc::loadCompFrame(*comp, framesVal)) {
               // invalid format
               // gfx.clear();
               return false;
           }
       }
    }
}

void D1Clc::storeCompFrame(QJsonArray &jsonArray, const D1GfxComp &comp, int frameIdx)
{
    const D1GfxCompFrame *frame = comp.getCompFrame(frameIdx);
    if (frame->cfFrameRef == 0 && frame->cfZOrder == 0 && frame->cfOffsetX == 0 && frame->cfOffsetY == 0) {
        return;
    }

    QJsonObject saveObj;
    {
        QJsonValue jsonVal = QJsonValue(frameIdx);
        saveObj.insert(D1Clc::CLC_COMP_FRAME_IDX, frameIdx);
    }
    if (frame->cfFrameRef != 0) {
        QJsonValue jsonVal = QJsonValue((int)frame->cfFrameRef);
        saveObj.insert(D1Clc::CLC_COMP_FRAME_REF, jsonVal);
    }
    if (frame->cfZOrder != 0) {
        QJsonValue jsonVal = QJsonValue(frame->cfZOrder);
        saveObj.insert(D1Clc::CLC_COMP_FRAME_Z, jsonVal);
    }
    if (frame->cfOffsetX != 0) {
        QJsonValue jsonVal = QJsonValue(frame->cfOffsetX);
        saveObj.insert(D1Clc::CLC_COMP_FRAME_X, jsonVal);
    }
    if (frame->cfOffsetY != 0) {
        QJsonValue jsonVal = QJsonValue(frame->cfOffsetY);
        saveObj.insert(D1Clc::CLC_COMP_FRAME_Y, jsonVal);
    }

    jsonArray.push_back(saveObj);
}

void D1Clc::storeComponent(QJsonArray &jsonArray, const D1GfxComp &comp, const QDir &jsonDir)
{
    QString gfxFilePath = comp.getGFX()->getFilePath();
    QJsonObject saveObj;

    saveObj.insert(D1Clc::CLC_COMP_GFX, jsonDir.relativeFilePath(gfxFilePath));
    saveObj.insert(D1Clc::CLC_COMP_LABEL, comp.getLabel());

    QJsonArray jsonFrames;
    for (int i = 0; i < comp.getCompFrameCount(); i++) {
        D1Clc::storeCompFrame(jsonFrames, comp, i);
    }
    saveObj.insert(D1Clc::CLC_COMP_FRAMES, jsonFrames);

    jsonArray.push_back(saveObj);
}

bool D1Clc::save(D1Gfx &gfx, const SaveAsParam &params)
{
    QString jsonFilePath = gfx.getCompFilePath();
    QString gfxFilePath = gfx.getFilePath();
    if (!params.celFilePath.isEmpty()) {
        gfxFilePath = params.celFilePath;
    }
    if (jsonFilePath.isEmpty()) {
        QFileInfo gfxfi = QFileInfo(gfxFilePath);
        jsonFilePath = gfxfi.absolutePath() + "/" + gfxfi.completeBaseName() + ".clc";
    }

    QFile saveJson(jsonFilePath);
    if (!saveJson.open(QIODevice::WriteOnly | QIODevice::Text)) {
        dProgressFail() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(jsonFilePath));
        return false;
    }

    QFileInfo jsonfi = QFileInfo(jsonFilePath);
    QDir jsonDir = jsonfi.dir();

    QJsonObject saveObj;
    saveObj.insert(D1Clc::CLC_MAIN_GFX, jsonDir.relativeFilePath(gfxFilePath));

    QJsonArray jsonComps;
    for (int i = 0; i < gfx.getComponentCount(); i++) {
        D1Clc::storeComponent(jsonComps, *gfx.getComponent(i), jsonDir);
    }
    saveObj.insert(D1Clc::CLC_COMPONENTS, jsonComps);

    // store the (json)file
    QJsonDocument saveDoc(saveObj);
    saveJson.write(saveDoc.toJson());

    // update the file-path
    // bool wasModified = gfx.isModified();
    gfx.setCompFilePath(jsonFilePath);
    // gfx.setModified(wasModified);

    return true;
}
