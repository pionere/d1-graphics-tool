#include "d1amp.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "progressdialog.h"

bool D1Amp::load(QString filePath, int tileCount, const OpenAsParam &params)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.ampFilePath.isEmpty()) {
    //    filePath = params.ampFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly) && !params.ampFilePath.isEmpty()) {
            return false; // report read-error only if the file was explicitly requested
        }
    }

    QByteArray fileData = file.readAll();
    QBuffer fileBuffer(&fileData);

    if (!fileBuffer.open(QIODevice::ReadOnly)) {
        return false;
    }

    // File size check
    auto fileSize = file.size();
    if (fileSize % 2 != 0) {
        dProgressErr() << tr("Invalid AMP file.");
        return false;
    }

    int ampTileCount = fileSize / 2;
    if (ampTileCount != tileCount) {
        if (ampTileCount != 0) {
            dProgressWarn() << tr("The size of AMP file does not align with TIL file.");
        }
        if (ampTileCount > tileCount) {
            ampTileCount = tileCount; // skip unusable data
        }
    }

    // prepare empty lists with zeros
    this->properties.clear();
    this->types.clear();
    for (int i = 0; i < tileCount; i++) {
        this->types.append(0);
        this->properties.append(0);
    }

    // Read AMP binary data
    QDataStream in(&fileBuffer);
    // in.setByteOrder(QDataStream::LittleEndian);

    for (int i = 0; i < ampTileCount; i++) {
        quint8 readByte;
        in >> readByte;
        this->types[i] = readByte;
        in >> readByte;
        this->properties[i] = readByte;
    }

    this->ampFilePath = filePath;
    this->modified = !file.isOpen();
    return true;
}

bool D1Amp::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.ampFilePath.isEmpty()) {
        filePath = params.ampFilePath;
        if (!params.autoOverwrite && QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to overwrite %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    }

    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    // write to file
    QDataStream out(&outFile);
    for (int i = 0; i < this->types.size(); i++) {
        out << this->types[i];
        out << this->properties[i];
    }

    this->ampFilePath = filePath; // this->load(filePath, allocate);
    this->modified = false;

    return true;
}

QString D1Amp::getFilePath()
{
    return this->ampFilePath;
}

bool D1Amp::isModified() const
{
    return this->modified;
}

quint8 D1Amp::getTileType(quint16 tileIndex)
{
    if (tileIndex >= this->types.count())
        return 0;

    return this->types.at(tileIndex);
}

quint8 D1Amp::getTileProperties(quint16 tileIndex)
{
    if (tileIndex >= this->properties.count())
        return 0;

    return this->properties.at(tileIndex);
}

bool D1Amp::setTileType(quint16 tileIndex, quint8 value)
{
    if (this->types[tileIndex] == value) {
        return false;
    }
    this->types[tileIndex] = value;
    this->modified = true;
    return true;
}

bool D1Amp::setTileProperties(quint16 tileIndex, quint8 value)
{
    if (this->properties[tileIndex] == value) {
        return false;
    }
    this->properties[tileIndex] = value;
    this->modified = true;
    return true;
}

void D1Amp::createTile()
{
    this->types.append(0);
    this->properties.append(0);
    this->modified = true;
}

void D1Amp::removeTile(int tileIndex)
{
    this->types.removeAt(tileIndex);
    this->properties.removeAt(tileIndex);
    this->modified = true;
}
