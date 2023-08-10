#include "d1tit.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "progressdialog.h"

bool D1Tit::load(const QString &filePath, int tileCount, const OpenAsParam &params)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.titFilePath.isEmpty()) {
    //    filePath = params.titFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly) && !params.titFilePath.isEmpty()) {
            return false; // report read-error only if the file was explicitly requested
        }
    }

    this->clear();
    this->titFilePath = filePath;

    bool changed = false; // !file.isOpen();

    const QByteArray fileData = file.readAll();

    // File size check
    unsigned fileSize = fileData.size();
    if (fileSize % 2 != 0) {
        dProgressErr() << tr("Invalid TIT file.");
        return false;
    }

    int titTileCount = fileSize;
    if (titTileCount != tileCount && titTileCount != 0) {
        // warn about misalignment if the files are not empty
        if (tileCount != 0) {
            dProgressWarn() << tr("The size of TIT file does not align with TIL file.");
        }
        if (titTileCount > tileCount) {
            titTileCount = tileCount; // skip unusable data
        }
        changed = true;
    }

    // prepare empty lists with zeros
    for (int i = 0; i < tileCount; i++) {
        this->properties.append(0);
    }

    // Read AMP binary data
    QDataStream in(fileData);
    // in.setByteOrder(QDataStream::LittleEndian);

    for (int i = 0; i < titTileCount; i++) {
        quint8 readByte;
        in >> readByte;
        this->properties[i] = readByte;
    }

    this->modified = changed;
    return true;
}

bool D1Tit::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.titFilePath.isEmpty()) {
        filePath = params.titFilePath;
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
        // TIT with content -> create or change
        QDir().mkpath(QFileInfo(filePath).absolutePath());
        QFile outFile = QFile(filePath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            dProgressFail() << tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
            return false;
        }

        // write to file
        QDataStream out(&outFile);
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

    this->titFilePath = filePath; // this->load(filePath, allocate);
    this->modified = false;

    return true;
}

void D1Tit::clear()
{
    this->properties.clear();
    this->modified = true;
}

QString D1Tit::getFilePath() const
{
    return this->titFilePath;
}

bool D1Tit::isModified() const
{
    return this->modified;
}

quint8 D1Tit::getTileProperties(int tileIndex) const
{
    if (tileIndex < 0 || tileIndex >= this->properties.count()) {
#ifdef QT_DEBUG
        QMessageBox::critical(nullptr, "Error", QStringLiteral("Property of an invalid tile %1 requested. Properties count: %2").arg(tileIndex).arg(this->properties.count()));
#endif
        return 0;
    }

    return this->properties.at(tileIndex);
}

bool D1Tit::setTileProperties(int tileIndex, quint8 value)
{
    if (this->properties[tileIndex] == value) {
        return false;
    }
    this->properties[tileIndex] = value;
    this->modified = true;
    return true;
}

void D1Tit::insertTile(int tileIndex)
{
    this->properties.insert(tileIndex, 0);
    this->modified = true;
}

void D1Tit::createTile()
{
    this->properties.append(0);
    this->modified = true;
}

void D1Tit::removeTile(int tileIndex)
{
    this->properties.removeAt(tileIndex);
    this->modified = true;
}
