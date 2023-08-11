#include "d1tla.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "progressdialog.h"

bool D1Tla::load(const QString &filePath, int tileCount, const OpenAsParam &params)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.tlaFilePath.isEmpty()) {
    //    filePath = params.tlaFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly) && !params.tlaFilePath.isEmpty()) {
            return false; // report read-error only if the file was explicitly requested
        }
    }

    this->clear();
    this->tlaFilePath = filePath;

    bool changed = false; // !file.isOpen();

    const QByteArray fileData = file.readAll();

    // File size check
    unsigned fileSize = fileData.size();
    if (fileSize % 2 != 0) {
        dProgressErr() << tr("Invalid TLA file.");
        return false;
    }

    int tlaTileCount = fileSize;
    if (tlaTileCount != tileCount + 1 && tlaTileCount != 0) {
        // warn about misalignment if the files are not empty
        if (tileCount != 0) {
            dProgressWarn() << tr("The size of TLA file does not align with TIL file.");
        }
        if (tlaTileCount > tileCount + 1) {
            tlaTileCount = tileCount + 1; // skip unusable data
        }
        changed = true;
    }

    // prepare empty lists with zeros
    for (int i = 0; i < tileCount; i++) {
        this->properties.append(0);
    }

    // Read TLA binary data
    QDataStream in(fileData);
    // in.setByteOrder(QDataStream::LittleEndian);

    // skip the first byte
    quint8 readByte;
    in >> readByte;
    for (int i = 0; i < tlaTileCount; i++) {
        in >> readByte;
        this->properties[i] = readByte;
    }

    this->modified = changed;
    return true;
}

bool D1Tla::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.tlaFilePath.isEmpty()) {
        filePath = params.tlaFilePath;
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
    for (int i = 0; i < this->properties.size(); i++) {
        if (this->properties[i] != 0) {
            isEmpty = false;
            break;
        }
    }
    if (!isEmpty) {
        // TLA with content -> create or change
        QDir().mkpath(QFileInfo(filePath).absolutePath());
        QFile outFile = QFile(filePath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            dProgressFail() << tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
            return false;
        }

        // write to file
        QDataStream out(&outFile);
        out << (quint8)0; // add leading zero
        for (int i = 0; i < this->properties.size(); i++) {
            out << this->properties[i];
        }
    } else {
        // AMP without content -> delete
        if (QFile::exists(filePath)) {
            if (!QFile::remove(filePath)) {
                dProgressFail() << tr("Failed to remove file: %1.").arg(QDir::toNativeSeparators(filePath));
                return false;
            }
        }
    }

    this->tlaFilePath = filePath; // this->load(filePath, allocate);
    this->modified = false;

    return true;
}

void D1Tla::clear()
{
    this->properties.clear();
    this->modified = true;
}

QString D1Tla::getFilePath() const
{
    return this->tlaFilePath;
}

bool D1Tla::isModified() const
{
    return this->modified;
}

quint8 D1Tla::getTileProperties(int tileIndex) const
{
    if (tileIndex < 0 || tileIndex >= this->properties.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Property of an invalid tile %1 requested. Properties count: %2").arg(tileIndex).arg(this->properties.count()));
#endif
        return 0;
    }

    return this->properties.at(tileIndex);
}

bool D1Tla::setTileProperties(int tileIndex, quint8 value)
{
    if (this->properties[tileIndex] == value) {
        return false;
    }
    this->properties[tileIndex] = value;
    this->modified = true;
    return true;
}

void D1Tla::insertTile(int tileIndex)
{
    this->properties.insert(tileIndex, 0);
    this->modified = true;
}

void D1Tla::createTile()
{
    this->properties.append(0);
    this->modified = true;
}

void D1Tla::removeTile(int tileIndex)
{
    this->properties.removeAt(tileIndex);
    this->modified = true;
}
