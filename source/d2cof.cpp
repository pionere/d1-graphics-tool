#include "d2cof.h"

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMap>
#include <QMessageBox>

#include "progressdialog.h"

#include "dungeon/all.h"

/*typedef enum Keyframe : uint8_t
{
    COFKEY_NONE,
    COFKEY_ATTACK,
    COFKEY_MISSILE,
    COFKEY_SOUND,
    COFKEY_SKILL,
    COFKEY_MAX,
};*/

#define DEBUGDCC 0
#if DEBUGDCC
#define LogDebug(exp, ...) LogErrorF(exp, __VA_ARGS__)
#else
#define LogDebug(exp, ...) do { (void) sizeof(exp); } while(0)
#endif

static constexpr const char* componentsNames[] = {
        "HD", "TR", "LG", "RA", "LA", "RH", "LH", "SH",
        "S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8"};

typedef struct D2CofLayer
{
    uint8_t component;        ///< See componentsNames
    uint8_t castsShadow;      ///< Does this layer cast a shadow
    uint8_t isSelectable;     ///< Can the layer be selected and used for the "hit" underlay
    uint8_t overrideTranslvl; ///<
    uint8_t newTranslvl;      ///<
    char    weaponClass[4];   ///<
} D2CofLayer;
/*
typedef struct D2CofHeaderInfo
{
    int numDirections;

    // QList<D2CofLayer> layers;
    // QList<int> keyFrames;
    // QList<uint8_t> layerOrder; // [directions][frame][layer]

    // RECT32 box;
} D2CofHeaderInfo;
*/
bool D2Cof::readContent(QIODevice *device, QDataStream &in, D1Gfx &gfx)
{
    auto fileSize = device->size();
    const int headerSize = 4 * 1 + 4 + 4 * 4 + 2 * 2;
    int cursor = 0;
    // CEL HEADER CHECKS
    // Read first DWORD
    if (fileSize < cursor + headerSize) {
        return false;
    }

    quint8 layersByte;
    in >> layersByte;

    // hi.numLayers = layersByte;

    quint8 framesByte;
    in >> framesByte;

    quint8 directionsByte;
    in >> directionsByte;

    // hi.numDirections = directionsByte;

    quint8 versionByte;
    in >> versionByte;

    if (versionByte != 20) {
        dProgressWarn() << QApplication::tr("version mismatch %1 vs %2").arg(versionByte).arg(20);
    }
    
    quint32 unknownDword;
    in >> unknownDword;

    qint32 xMinDWord;
    in >> xMinDWord;

    qint32 xMaxDWord;
    in >> xMaxDWord;

    qint32 yMinDWord;
    in >> yMinDWord;

    qint32 yMaxDWord;
    in >> yMaxDWord;
#if 0
    hi.box.x = xMinDWord;
    hi.box.y = yMinDWord;
    hi.box.w = xMaxDWord - xMinDWord;
    hi.box.h = yMaxDWord - yMinDWord;
#endif
    quint32 animRateDWord;
    in >> animRateDWord;

    gfx.frameLen = animRateDWord * 1000 / 256;

    cursor += headerSize;
    if (fileSize < cursor + layersByte * 9) {
        return false;
    }
LogDebug("Adding layers %d", layersByte);
    QMap<int, D1GfxComp*> compMap;
    for (int i = 0; i < layersByte; i++) {
        D2CofLayer layer;

        quint8 componentByte;
        in >> componentByte;

        quint8 castsShadowByte;
        in >> castsShadowByte;

        quint8 isSelectableByte;
        in >> isSelectableByte;

        quint8 overrideTranslvlByte;
        in >> overrideTranslvlByte;

        quint8 newTranslvlByte;
        in >> newTranslvlByte;

        quint8 weaponClass0Byte;
        in >> weaponClass0Byte;

        quint8 weaponClass1Byte;
        in >> weaponClass1Byte;

        quint8 weaponClass2Byte;
        in >> weaponClass2Byte;

        quint8 weaponClass3Byte;
        in >> weaponClass3Byte;

        layer.component = componentByte;
        layer.castsShadow = castsShadowByte;
        layer.isSelectable = isSelectableByte;
        layer.overrideTranslvl = overrideTranslvlByte;
        layer.newTranslvl = newTranslvlByte;

        layer.weaponClass[0] = weaponClass0Byte;
        layer.weaponClass[1] = weaponClass1Byte;
        layer.weaponClass[2] = weaponClass2Byte;
        layer.weaponClass[3] = weaponClass3Byte;

        // hi.layers.push_Back(layer);

        D1Gfx* cg = new D1Gfx();
        cg->setPalette(gfx.getPalette());
        if (componentByte >= lengthof(componentsNames))
            dProgressWarn() << QApplication::tr("unknown component %1 (max %2).").arg(componentByte).arg(lengthof(componentsNames) - 1);
        cg->setFilePath(componentByte >= lengthof(componentsNames) ? "UNKNOWN" : componentsNames[componentByte]);
        D1GfxComp* comp = new D1GfxComp(cg, &gfx);

        if (compMap.contains(componentByte)) {
            dProgressErr() << QApplication::tr("duplicate layer %1").arg(componentByte);
        }
        compMap[componentByte] = comp;
        gfx.components.push_back(comp);
    }

    // add empty frames
LogDebug("Creating frames %d", directionsByte * framesByte);
    for (int i = 0; i < directionsByte * framesByte; i++) {
        gfx.insertFrame(i);

        for (int n = 0; n < layersByte; n++) {
            D1GfxComp* comp = gfx.components[n];
            D1Gfx* cg = comp->getGFX();
            cg->insertFrame(i);
            comp->getCompFrame(i)->cfFrameRef = i + 1;
        }
    }
LogDebug("Creating groups %d", directionsByte);
    for (int d = 0; d < directionsByte; d++) {
        if (d == 0)
            gfx.groupFrameIndices[0].second = (d + 1) * framesByte - 1;
        else
            gfx.groupFrameIndices.push_back(std::pair<int, int>(d * framesByte, (d + 1) * framesByte - 1));

        for (int n = 0; n < layersByte; n++) {
            D1GfxComp* comp = gfx.components[n];
            D1Gfx* cg = comp->getGFX();
            if (d == 0)
                cg->groupFrameIndices[0].second = (d + 1) * framesByte - 1;
            else
                cg->groupFrameIndices.push_back(std::pair<int, int>(d * framesByte, (d + 1) * framesByte - 1));
        }
    }

    cursor += layersByte * 9;
    if (fileSize < cursor + framesByte) {
        return false;
    }
LogDebug("Adding metas %d", framesByte);
    for (int i = 0; i < framesByte; i++) {
        quint8 keyFrameByte;
        in >> keyFrameByte;

        // hi.keyFrames.push_back(keyFrameByte);

        if (keyFrameByte != 0) {
            QString frames = gfx.metas[CELMETA_ACTIONFRAMES].getContent();
            if (!frames.isEmpty())
                frames.append(",");
            frames.append(QString::number(i + 1));
            gfx.metas[CELMETA_ACTIONFRAMES].setContent(frames);
        }
    }

    cursor += framesByte;
    int layersOrders = directionsByte * framesByte * layersByte;
    if (fileSize < cursor + layersOrders) {
        return false;
    }
LogDebug("Adding components %d", directionsByte);
    for (int d = 0; d < directionsByte; d++) {
        for (int n = 0; n < framesByte; n++) {
            for (int i = 0; i < layersByte; i++) {
                quint8 layerByte;
                in >> layerByte;

                // hi.layerOrder.push_back(layerByte);

                if (!compMap.contains(layerByte)) {
                    dProgressErr() << QApplication::tr("undefined layer (%1) referenced").arg(layerByte);
                    continue;
                }
                D1GfxComp* comp = compMap[layerByte];
                D1GfxCompFrame* compFrame = comp->getCompFrame(d * framesByte + n);
                compFrame->cfZOrder = i;
            }
        }
    }

LogDebug("Load done.");
    return true;
}

bool D2Cof::load(D1Gfx &gfx, const QString &filePath, const OpenAsParam &params)
{
    gfx.clear();

    // Opening COF file and load it in RAM
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QByteArray fileData = file.readAll();

    // Read COF binary data
    QDataStream in(fileData);
    in.setByteOrder(QDataStream::LittleEndian);

    QIODevice *device = in.device();
    return readContent(device, in, gfx);
}
