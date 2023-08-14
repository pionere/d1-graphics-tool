#include "d1smp.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "progressdialog.h"

#include "dungeon/all.h"

bool D1Smp::load(const QString &filePath, int subtileCount, const OpenAsParam &params)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.smpFilePath.isEmpty()) {
    //    filePath = params.smpFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly) && !params.smpFilePath.isEmpty()) {
            return false; // report read-error only if the file was explicitly requested
        }
    }

    this->clear();
    this->smpFilePath = filePath;

    bool changed = false; // !file.isOpen();

    const QByteArray fileData = file.readAll();

    // File size check
    unsigned fileSize = fileData.size();
    int smpSubtileCount = fileSize;
    if (smpSubtileCount != subtileCount && smpSubtileCount != 0) {
        // warn about misalignment if the files are not empty
        if (subtileCount != 0) {
            dProgressWarn() << tr("The size of SMP file does not align with SOL file.");
        }
        if (smpSubtileCount > subtileCount + 1) {
            smpSubtileCount = subtileCount + 1; // skip unusable data
        }
        changed = true;
    }

    // prepare empty lists with zeros
    for (int i = 0; i < subtileCount; i++) {
        this->types.append(0);
        this->properties.append(0);
    }

    // Read SMP binary data
    QDataStream in(fileData);
    // in.setByteOrder(QDataStream::LittleEndian);

    // skip the first byte
    quint8 readByte;
    in >> readByte;
    for (int i = 0; i < smpSubtileCount - 1; i++) {
        in >> readByte;
        this->types[i] = readByte & MAT_TYPE;
        this->properties[i] = readByte & ~MAT_TYPE;
    }

    this->modified = changed;
    return true;
}

bool D1Smp::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.smpFilePath.isEmpty()) {
        filePath = params.smpFilePath;
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

    bool isEmpty = true;
    for (int i = 0; i < this->types.size(); i++) {
        if (this->types[i] != 0 || this->properties[i] != 0) {
            isEmpty = false;
            break;
        }
    }
    if (!isEmpty) {
        // SMP with content -> create or change
        QDir().mkpath(QFileInfo(filePath).absolutePath());
        QFile outFile = QFile(filePath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            dProgressFail() << tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
            return false;
        }

        // write to file
        QDataStream out(&outFile);
        out << (quint8)0; // add leading zero
        for (int i = 0; i < this->types.size(); i++) {
			quint8 writeByte;
			writeByte = this->types[i] | this->properties[i];
            out << writeByte;
        }
    } else {
        // SMP without content -> delete
        if (QFile::exists(filePath)) {
            if (!QFile::remove(filePath)) {
                dProgressFail() << tr("Failed to remove file: %1.").arg(QDir::toNativeSeparators(filePath));
                return false;
            }
        }
    }

    this->smpFilePath = filePath; // this->load(filePath, allocate);
    this->modified = false;

    return true;
}

void D1Smp::clear()
{
    this->types.clear();
    this->properties.clear();
    this->modified = true;
}

QString D1Smp::getFilePath() const
{
    return this->smpFilePath;
}

bool D1Smp::isModified() const
{
    return this->modified;
}

quint8 D1Smp::getSubtileType(int subtileIndex) const
{
    if (subtileIndex < 0 || subtileIndex >= this->types.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("SMP-Type of an invalid subtile %1 requested. Types count: %2").arg(subtileIndex).arg(this->types.count()));
#endif
        return 0;
    }

    return this->types.at(subtileIndex);
}

quint8 D1Smp::getSubtileProperties(int subtileIndex) const
{
    if (subtileIndex < 0 || subtileIndex >= this->properties.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("SMP-Property of an invalid subtile %1 requested. Properties count: %2").arg(subtileIndex).arg(this->properties.count()));
#endif
        return 0;
    }

    return this->properties.at(subtileIndex);
}

bool D1Smp::setSubtileType(int subtileIndex, quint8 value)
{
    if (this->types[subtileIndex] == value) {
        return false;
    }
    this->types[subtileIndex] = value;
    this->modified = true;
    return true;
}

bool D1Smp::setSubtileProperties(int subtileIndex, quint8 value)
{
    if (this->properties[subtileIndex] == value) {
        return false;
    }
    this->properties[subtileIndex] = value;
    this->modified = true;
    return true;
}

void D1Smp::insertSubtile(int subtileIndex)
{
    this->types.insert(subtileIndex, 0);
    this->properties.insert(subtileIndex, 0);
    this->modified = true;
}

void D1Smp::createSubtile()
{
    this->types.append(0);
    this->properties.append(0);
    this->modified = true;
}

void D1Smp::removeSubtile(int subtileIndex)
{
    this->types.removeAt(subtileIndex);
    this->properties.removeAt(subtileIndex);
    this->modified = true;
}

void D1Smp::remapSubtiles(const std::map<unsigned, unsigned> &remap)
{
    QList<quint8> newTypes;
    QList<quint8> newProperties;

    for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
        newTypes.append(this->types.at(iter->second));
        newProperties.append(this->properties.at(iter->second));
    }
    this->types.swap(newTypes);
    this->properties.swap(newProperties);
    this->modified = true;
}
