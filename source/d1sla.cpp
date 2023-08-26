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

bool D1Sla::load(const QString &filePath, D1Tileset *tileset, const OpenAsParam &params)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.slaFilePath.isEmpty()) {
    //    filePath = params.slaFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly) && !params.slaFilePath.isEmpty()) {
            return false; // report read-error only if the file was explicitly requested
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
    int subtileCount = tileset->sol->getSubtileCount();
    int slaSubtileCount = fileSize / 4;
    if (slaSubtileCount != subtileCount + 1 && slaSubtileCount != 0) {
        dProgressErr() << tr("The size of SLA file does not align with TIL file.");
        changed = true;
    }

    // prepare empty lists with zeros
    for (int i = 0; i < subtileCount; i++) {
        this->renderProperties.append(0);
        this->trapProperties.append(0);
        this->specProperties.append(0);
        this->subProperties.append(0);
        this->mapTypes.append(0);
        this->mapProperties.append(0);
    }

    if (slaSubtileCount == 0) {
        for (int i = 0; i < subtileCount; i++) {
            this->renderProperties[i] = tileset->tmi->getSubtileProperties(i);
            this->trapProperties[i] = tileset->spt->getSubtileTrapProperty(i);
            this->specProperties[i] = tileset->spt->getSubtileSpecProperty(i);
            this->subProperties[i] = tileset->sol->getSubtileProperties(i);
            this->mapTypes[i] = tileset->smp->getSubtileType(i);
            this->mapProperties[i] = tileset->smp->getSubtileProperties(i);
        }
        return true;
    }

    // Read SLA binary data
    QDataStream in(fileData);
    // in.setByteOrder(QDataStream::LittleEndian);

    quint8 readByte;
    // read the sub-properties
    // skip the first byte
    in >> readByte;
    for (int i = 0; i < slaSubtileCount - 1; i++) {
        in >> readByte;
        this->subProperties[i] = readByte;
    }
    // read the trap/spec-properties
    // skip the first byte
    in >> readByte;
    for (int i = 0; i < slaSubtileCount - 1; i++) {
        in >> readByte;
        this->trapProperties[i] = (readByte >> 6) & 3;
        this->specProperties[i] = readByte & PST_SPEC_TYPE;
    }
    // read the render-properties
    // skip the first byte
    in >> readByte;
    for (int i = 0; i < slaSubtileCount - 1; i++) {
        in >> readByte;
        this->renderProperties[i] = readByte;
    }
    // read the map-properties
    // skip the first byte
    in >> readByte;
    for (int i = 0; i < slaSubtileCount - 1; i++) {
        in >> readByte;
        this->mapTypes[i] = readByte & MAT_TYPE;
        this->mapProperties[i] = readByte & ~MAT_TYPE;
    }

    if (slaSubtileCount != 0) {
        for (int i = 0; i < subtileCount; i++) {
            if (this->renderProperties[i] != tileset->tmi->getSubtileProperties(i)) {
                dProgressErr() << tr("Render property mismatch: %1.: %2 vs. %3").arg(i).arg(this->renderProperties[i]).arg(tileset->tmi->getSubtileProperties(i));
            }
            if (this->trapProperties[i] != tileset->spt->getSubtileTrapProperty(i)) {
                dProgressErr() << tr("Trap property mismatch: %1.: %2 vs. %3").arg(i).arg(this->trapProperties[i]).arg(tileset->spt->getSubtileTrapProperty(i));
            }
            if (this->specProperties[i] != tileset->spt->getSubtileSpecProperty(i)) {
                dProgressErr() << tr("Spec property mismatch: %1.: %2 vs. %3").arg(i).arg(this->specProperties[i]).arg(tileset->spt->getSubtileSpecProperty(i));
            }
            if (this->subProperties[i] != tileset->sol->getSubtileProperties(i)) {
                dProgressErr() << tr("Sub property mismatch: %1.: %2 vs. %3").arg(i).arg(this->subProperties[i]).arg(tileset->sol->getSubtileProperties(i));
            }
            if (this->mapTypes[i] != tileset->smp->getSubtileType(i)) {
                dProgressErr() << tr("Map type mismatch: %1.: %2 vs. %3").arg(i).arg(this->mapTypes[i]).arg(tileset->smp->getSubtileType(i));
            }
            if (this->mapProperties[i] != tileset->smp->getSubtileProperties(i)) {
                dProgressErr() << tr("Map property mismatch: %1.: %2 vs. %3").arg(i).arg(this->mapProperties[i]).arg(tileset->smp->getSubtileProperties(i));
            }
        }
        return true;
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

    // write to file
    QDataStream out(&outFile);

    quint8 readByte;
    // write the sub-properties
    out << (quint8)0; // add leading zero
    for (int i = 0; i < this->subProperties.size(); i++) {
        out << this->subProperties[i];
    }
    // write the trap/spec-properties
    out << (quint8)0; // add leading zero
    for (int i = 0; i < this->specProperties.size(); i++) {
        quint8 writeByte;
        writeByte = this->specProperties[i];
        writeByte |= this->trapProperties[i] << 6;
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
    this->trapProperties.clear();
    this->specProperties.clear();
    this->renderProperties.clear();
    this->mapTypes.clear();
    this->mapProperties.clear();
    this->modified = true;
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
    this->trapProperties.insert(subtileIndex, 0);
    this->specProperties.insert(subtileIndex, 0);
    this->renderProperties.insert(subtileIndex, 0);
    this->mapTypes.insert(subtileIndex, 0);
    this->mapProperties.insert(subtileIndex, 0);
    this->modified = true;
}

void D1Sla::createSubtile()
{
    this->subProperties.append(0);
    this->trapProperties.append(0);
    this->specProperties.append(0);
    this->renderProperties.append(0);
    this->mapTypes.append(0);
    this->mapProperties.append(0);
    this->modified = true;
}

void D1Sla::removeSubtile(int subtileIndex)
{
    this->subProperties.removeAt(subtileIndex);
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
