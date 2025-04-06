#include "d1sla.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "d1tileset.h"
#include "progressdialog.h"

#include "dungeon/all.h"

bool D1Sla::load(const QString &filePath)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.slaFilePath.isEmpty()) {
    //    filePath = params.slaFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
    }

    this->clear();
    this->slaFilePath = filePath;

    bool changed = false; // !file.isOpen();

    const QByteArray fileData = file.readAll();

    // File size check
    unsigned fileSize = fileData.size();
    if ((fileSize % 4) != 0) {
        dProgressErr() << tr("Invalid SLA file.");
        return false;
    }
    unsigned subtileCount = fileSize / 4;
    subtileCount = subtileCount != 0 ? subtileCount - 1 : 0;

    // prepare empty lists with zeros
    for (unsigned i = 0; i < subtileCount; i++) {
        this->insertSubtile(i);
    }

    // Read SLA binary data
    QDataStream in(fileData);
    // in.setByteOrder(QDataStream::LittleEndian);

    quint8 readByte;
    // read the sub-properties
    // skip the first byte
    in >> readByte;
    for (unsigned i = 0; i < subtileCount; i++) {
        in >> readByte;
        this->subProperties[i] = readByte & (PSF_BLOCK_MISSILE | PSF_BLOCK_LIGHT | PSF_BLOCK_PATH);
        this->lightRadius[i] = readByte & PSF_LIGHT_RADIUS;
    }
    // read the trap/spec-properties
    // skip the first byte
    in >> readByte;
    for (unsigned i = 0; i < subtileCount; i++) {
        in >> readByte;
        this->trapProperties[i] = readByte & PST_TRAP_TYPE;
        this->specProperties[i] = readByte & PST_SPEC_TYPE;
    }
    // read the render-properties
    // skip the first byte
    in >> readByte;
    for (unsigned i = 0; i < subtileCount; i++) {
        in >> readByte;
        this->renderProperties[i] = readByte;
    }
    // read the map-properties
    // skip the first byte
    in >> readByte;
    for (unsigned i = 0; i < subtileCount; i++) {
        in >> readByte;
        this->mapTypes[i] = readByte & MAT_TYPE;
        this->mapProperties[i] = readByte & ~MAT_TYPE;
    }

    this->modified = changed;
    return true;
}

bool D1Sla::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.slaFilePath.isEmpty()) {
        filePath = params.slaFilePath;
        if (!params.autoOverwrite && QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to overwrite %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    } else if (!this->isModified()) {
        return false;
    }

    if (filePath.isEmpty()) {
        return false;
    }

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    bool isMapEmpty = true;
    for (int i = 0; i < this->mapTypes.size(); i++) {
        if (this->mapTypes[i] != 0 || this->mapProperties[i] != 0) {
            isMapEmpty = false;
            break;
        }
    }
    bool isLightEmpty = true;
    for (int i = 0; i < this->lightRadius.size(); i++) {
        if (this->lightRadius[i] != 0) {
            isLightEmpty = false;
            break;
        }
    }

    // write to file
    QDataStream out(&outFile);

    // write the sub-properties
    out << (quint8)(isLightEmpty ? 1 : 0);
    for (int i = 0; i < this->subProperties.size(); i++) {
        quint8 writeByte;
        writeByte = this->subProperties[i] | this->lightRadius[i];
        out << writeByte;
    }
    // write the trap/spec-properties
    out << (quint8)0; // add leading zero
    for (int i = 0; i < this->specProperties.size(); i++) {
        quint8 writeByte;
        writeByte = this->specProperties[i] | this->trapProperties[i];
        out << writeByte;
    }
    // write the render-properties
    out << (quint8)0; // add leading zero
    for (int i = 0; i < this->renderProperties.size(); i++) {
        out << this->renderProperties[i];
    }
    // write the map-properties
    out << (quint8)(isMapEmpty ? 1 : 0);
    for (int i = 0; i < this->mapTypes.size(); i++) {
        quint8 writeByte;
        writeByte = this->mapTypes[i] | this->mapProperties[i];
        out << writeByte;
    }

    this->slaFilePath = filePath; // this->load(filePath, allocate);
    this->modified = false;

    return true;
}

void D1Sla::clear()
{
    this->subProperties.clear();
    this->lightRadius.clear();
    this->trapProperties.clear();
    this->specProperties.clear();
    this->renderProperties.clear();
    this->mapTypes.clear();
    this->mapProperties.clear();
    this->modified = true;
}

static const char* mapTypeToStr(quint8 mapType)
{
    const char* result = "N/A";
    switch (mapType) {
    case 0: result = "None";      break;
    case 1: result = "Extern";    break;
    case 2: result = "Stairs";    break;
    case 3: result = "West Door"; break;
    case 4: result = "East Door"; break;
    }
    return result;
}

void D1Sla::compareTo(const D1Sla *sla) const
{
    unsigned subtileCount = sla->subProperties.size();
    unsigned mySubtileCount = this->subProperties.size();
    if (mySubtileCount != subtileCount) {
        dProgress() << tr("The number of tiles is %1 (was %2)").arg(mySubtileCount).arg(subtileCount);
        subtileCount = std::min(mySubtileCount, subtileCount);
    }

    for (int i = 0; i < subtileCount; i++) {
        quint8 subProps = sla->subProperties[i];
        quint8 mySubProps = this->subProperties[i];
        if (mySubProps != subProps) {
            dProgress() << tr("The collision settings of tile %1 is [%2:%3:%4] (was [%5:%6:%7])").arg(i + 1)
                .arg((mySubProps & PSF_BLOCK_PATH) != 0).arg((mySubProps & PSF_BLOCK_LIGHT) != 0).arg((mySubProps & PSF_BLOCK_MISSILE) != 0)
                .arg((subProps & PSF_BLOCK_PATH) != 0).arg((subProps & PSF_BLOCK_LIGHT) != 0).arg((subProps & PSF_BLOCK_MISSILE) != 0);
        }

        quint8 lr = sla->lightRadius[i];
        quint8 mylr = this->lightRadius[i];
        if (mylr != lr) {
            dProgress() << tr("The light radius of tile %1 is %2 (was %3)").arg(i + 1).arg(mylr).arg(lr);
        }

        int trapProps = sla->trapProperties[i];
        int myTrapProps = this->trapProperties[i];
        if (myTrapProps != trapProps) {
            dProgress() << tr("The trap settings of tile %1 is '%2' (was '%3')").arg(i + 1)
                .arg(myTrapProps == PST_NONE ? tr("None") : (myTrapProps == PST_LEFT ? tr("Left") : (myTrapProps == PST_RIGHT ? tr("Right") : tr("N/A"))))
                .arg(trapProps == PST_NONE ? tr("None") : (trapProps == PST_LEFT ? tr("Left") : (trapProps == PST_RIGHT ? tr("Right") : tr("N/A"))));
        }
        int spec = sla->specProperties[i];
        int myspec = this->specProperties[i];
        if (myspec != spec) {
            dProgress() << tr("The special cell of tile %1 is %2 (was %3)").arg(i + 1).arg(myspec).arg(spec);
        }

        quint8 renderProps = sla->renderProperties[i];
        quint8 myRenderProps = this->renderProperties[i];
        if (myRenderProps != renderProps) {
            dProgress() << tr("The render settings of tile %1 is [%2 left %3:%4:%5 right %6:%7:%8] (was [%9 left %10:%11:%12 right %13:%14:%15])").arg(i + 1)
                .arg((myRenderProps & TMIF_WALL_TRANS) != 0).arg((myRenderProps & TMIF_LEFT_REDRAW) != 0).arg((myRenderProps & TMIF_LEFT_FOLIAGE) != 0).arg((myRenderProps & TMIF_LEFT_WALL_TRANS) != 0)
                    .arg((myRenderProps & TMIF_RIGHT_REDRAW) != 0).arg((myRenderProps & TMIF_RIGHT_FOLIAGE) != 0).arg((myRenderProps & TMIF_RIGHT_WALL_TRANS) != 0)
                .arg((renderProps & TMIF_WALL_TRANS) != 0).arg((renderProps & TMIF_LEFT_REDRAW) != 0).arg((renderProps & TMIF_LEFT_FOLIAGE) != 0).arg((renderProps & TMIF_LEFT_WALL_TRANS) != 0)
                    .arg((renderProps & TMIF_RIGHT_REDRAW) != 0).arg((renderProps & TMIF_RIGHT_FOLIAGE) != 0).arg((renderProps & TMIF_RIGHT_WALL_TRANS) != 0);
        }


        quint8 mapType = sla->mapTypes[i];
        quint8 myMapType = this->mapTypes[i];
        if (myMapType != mapType) {
            dProgress() << tr("The map type of tile %1 is '%2' (was '%3')").arg(i + 1).arg(mapTypeToStr(myMapType)).arg(mapTypeToStr(mapType));
        }

        quint8 mapProps = sla->mapProperties[i];
        quint8 myMapProps = this->mapProperties[i];
        if (myMapProps != mapProps) {
            dProgress() << tr("The map walls of tile %1 is [%2:%3:%4:%5] (was [%6:%7:%8:%9])").arg(i + 1)
                .arg((myMapProps & MAT_WALL_NW) != 0).arg((myMapProps & MAT_WALL_NE) != 0).arg((myMapProps & MAT_WALL_SW) != 0).arg((myMapProps & MAT_WALL_SE) != 0)
                .arg((mapProps & MAT_WALL_NW) != 0).arg((mapProps & MAT_WALL_NE) != 0).arg((mapProps & MAT_WALL_SW) != 0).arg((mapProps & MAT_WALL_SE) != 0);
        }
    }
}

QString D1Sla::getFilePath() const
{
    return this->slaFilePath;
}

bool D1Sla::isModified() const
{
    return this->modified;
}

int D1Sla::getSubtileCount() const
{
    return this->subProperties.size();
}

quint8 D1Sla::getSubProperties(int subtileIndex) const
{
    if ((unsigned)subtileIndex >= (unsigned)this->subProperties.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("SLA-Properties of an invalid subtile %1 requested. Sub-Properties count: %2").arg(subtileIndex).arg(this->subProperties.count()));
#endif
        return 0;
    }

    return this->subProperties.at(subtileIndex);
}

bool D1Sla::setSubProperties(int subtileIndex, quint8 value)
{
    if (this->subProperties[subtileIndex] == value) {
        return false;
    }
    this->subProperties[subtileIndex] = value;
    this->modified = true;
    return true;
}

quint8 D1Sla::getLightRadius(int subtileIndex) const
{
    if ((unsigned)subtileIndex >= (unsigned)this->lightRadius.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Light radius of an invalid subtile %1 requested. Light radius count: %2").arg(subtileIndex).arg(this->lightRadius.count()));
#endif
        return 0;
    }

    return this->lightRadius.at(subtileIndex);
}

bool D1Sla::setLightRadius(int subtileIndex, quint8 value)
{
    if (this->lightRadius[subtileIndex] == value) {
        return false;
    }
    this->lightRadius[subtileIndex] = value;
    this->modified = true;
    return true;
}

int D1Sla::getTrapProperty(int subtileIndex) const
{
    if ((unsigned)subtileIndex >= (unsigned)this->trapProperties.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("SLA-Properties of an invalid subtile %1 requested. Trap-Properties count: %2").arg(subtileIndex).arg(this->trapProperties.count()));
#endif
        return 0;
    }

    return this->trapProperties.at(subtileIndex);
}

bool D1Sla::setTrapProperty(int subtileIndex, int value)
{
    if (this->trapProperties[subtileIndex] == value) {
        return false;
    }
    this->trapProperties[subtileIndex] = value;
    this->modified = true;
    return true;
}

int D1Sla::getSpecProperty(int subtileIndex) const
{
    if ((unsigned)subtileIndex >= (unsigned)this->specProperties.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("SLA-Properties of an invalid subtile %1 requested. Spec-Properties count: %2").arg(subtileIndex).arg(this->specProperties.count()));
#endif
        return 0;
    }

    return this->specProperties.at(subtileIndex);
}

bool D1Sla::setSpecProperty(int subtileIndex, int value)
{
    if (this->specProperties[subtileIndex] == value) {
        return false;
    }
    this->specProperties[subtileIndex] = value;
    this->modified = true;
    return true;
}

quint8 D1Sla::getRenderProperties(int subtileIndex) const
{
    if ((unsigned)subtileIndex >= (unsigned)this->renderProperties.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("SLA-Properties of an invalid subtile %1 requested. Render-Properties count: %2").arg(subtileIndex).arg(this->renderProperties.count()));
#endif
        return 0;
    }

    return this->renderProperties.at(subtileIndex);
}

bool D1Sla::setRenderProperties(int subtileIndex, quint8 value)
{
    if (this->renderProperties[subtileIndex] == value) {
        return false;
    }
    this->renderProperties[subtileIndex] = value;
    this->modified = true;
    return true;
}

quint8 D1Sla::getMapType(int subtileIndex) const
{
    if ((unsigned)subtileIndex >= (unsigned)this->mapTypes.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("SLA-Properties of an invalid subtile %1 requested. Map-Type count: %2").arg(subtileIndex).arg(this->mapTypes.count()));
#endif
        return 0;
    }

    return this->mapTypes.at(subtileIndex);
}

bool D1Sla::setMapType(int subtileIndex, quint8 value)
{
    if (this->mapTypes[subtileIndex] == value) {
        return false;
    }
    this->mapTypes[subtileIndex] = value;
    this->modified = true;
    return true;
}

quint8 D1Sla::getMapProperties(int subtileIndex) const
{
    if ((unsigned)subtileIndex >= (unsigned)this->mapProperties.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("SLA-Properties of an invalid subtile %1 requested. Map-Properties count: %2").arg(subtileIndex).arg(this->mapProperties.count()));
#endif
        return 0;
    }

    return this->mapProperties.at(subtileIndex);
}

bool D1Sla::setMapProperties(int subtileIndex, quint8 value)
{
    if (this->mapProperties[subtileIndex] == value) {
        return false;
    }
    this->mapProperties[subtileIndex] = value;
    this->modified = true;
    return true;
}

void D1Sla::insertSubtile(int subtileIndex)
{
    this->subProperties.insert(subtileIndex, 0);
    this->lightRadius.insert(subtileIndex, 0);
    this->trapProperties.insert(subtileIndex, 0);
    this->specProperties.insert(subtileIndex, 0);
    this->renderProperties.insert(subtileIndex, 0);
    this->mapTypes.insert(subtileIndex, 0);
    this->mapProperties.insert(subtileIndex, 0);
    this->modified = true;
}

void D1Sla::removeSubtile(int subtileIndex)
{
    this->subProperties.removeAt(subtileIndex);
    this->lightRadius.removeAt(subtileIndex);
    this->trapProperties.removeAt(subtileIndex);
    this->specProperties.removeAt(subtileIndex);
    this->renderProperties.removeAt(subtileIndex);
    this->mapTypes.removeAt(subtileIndex);
    this->mapProperties.removeAt(subtileIndex);
    this->modified = true;
}

void D1Sla::remapSubtiles(const std::map<unsigned, unsigned> &remap)
{
    { // remap sub-properties
        QList<quint8> newSubProperties;

        for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
            newSubProperties.append(this->subProperties.at(iter->second));
        }
        this->subProperties.swap(newSubProperties);
    }
    { // remap light radii
        QList<quint8> newLightRadius;

        for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
            newLightRadius.append(this->lightRadius.at(iter->second));
        }
        this->lightRadius.swap(newLightRadius);
    }
    { // remap trap-properties
        QList<int> newTrapProperties;

        for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
            newTrapProperties.append(this->trapProperties.at(iter->second));
        }
        this->trapProperties.swap(newTrapProperties);
    }
    { // remap spec-properties
        QList<int> newSpecProperties;

        for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
            newSpecProperties.append(this->specProperties.at(iter->second));
        }
        this->specProperties.swap(newSpecProperties);
    }
    { // remap sub-properties
        QList<quint8> newRenderProperties;

        for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
            newRenderProperties.append(this->renderProperties.at(iter->second));
        }
        this->renderProperties.swap(newRenderProperties);
    }
    { // remap map-types
        QList<quint8> newMapTypes;

        for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
            newMapTypes.append(this->mapTypes.at(iter->second));
        }
        this->mapTypes.swap(newMapTypes);
    }
    { // remap map-properties
        QList<quint8> newMapProperties;

        for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
            newMapProperties.append(this->mapProperties.at(iter->second));
        }
        this->mapProperties.swap(newMapProperties);
    }

    this->modified = true;
}

void D1Sla::resetSubtileFlags(int subtileIndex)
{
    this->setSubProperties(subtileIndex, 0);
    this->setTrapProperty(subtileIndex, 0);
    this->setSpecProperty(subtileIndex, 0);
    this->setRenderProperties(subtileIndex, 0);
    this->setMapType(subtileIndex, 0);
    this->setMapProperties(subtileIndex, 0);
}
