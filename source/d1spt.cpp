#include "d1spt.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "d1sol.h"
#include "progressdialog.h"

bool D1Spt::load(const QString &filePath, const D1Sol *sol, const OpenAsParam &params)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.sptFilePath.isEmpty()) {
    //    filePath = params.sptFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly) && !params.sptFilePath.isEmpty()) {
            return false; // report read-error only if the file was explicitly requested
        }
    }

    this->clear();
    this->sptFilePath = filePath;

    bool changed = !file.isOpen();

    const QByteArray fileData = file.readAll();

    // File size check
    unsigned fileSize = fileData.size();
    int subtileCount = sol->getSubtileCount();
    int sptSubtileCount = fileSize;
    if (sptSubtileCount != subtileCount + 1) {
        // warn about misalignment if the files are not empty
        if (sptSubtileCount != 0 && subtileCount != 0) {
            dProgressWarn() << tr("The size of SPT file does not align with SOL file.");
        }
        if (sptSubtileCount > subtileCount + 1) {
            sptSubtileCount = subtileCount + 1; // skip unusable data
        }
        changed = true;
    }

    // prepare empty list with zeros
    for (int i = 0; i < subtileCount; i++) {
        this->trapProperties.append(0);
        this->specProperties.append(0);
    }

    // Read TMI binary data
    QDataStream in(fileData);
    // in.setByteOrder(QDataStream::LittleEndian);

    // skip the first byte
    quint8 readByte;
    in >> readByte;
    for (int i = 0; i < sptSubtileCount - 1; i++) {
        in >> readByte;
        this->trapProperties[i] = (readByte >> 6) & 3;
        this->specProperties[i] = readByte & ((1 << 6) - 1);
    }

    this->modified = changed;
    return true;
}

bool D1Spt::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.sptFilePath.isEmpty()) {
        filePath = params.sptFilePath;
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

    // write to file
    QDataStream out(&outFile);
    out << (quint8)0; // add leading zero
    quint8 writeByte;
    for (int i = 0; i < this->trapProperties.size(); i++) {
        writeByte = this->specProperties[i];
        writeByte |= this->trapProperties[i] << 6;
        out << writeByte;
    }

    this->sptFilePath = filePath; // this->load(filePath, allocate);
    this->modified = false;

    return true;
}

void D1Spt::clear()
{
    this->trapProperties.clear();
    this->specProperties.clear();
    this->modified = true;
}

QString D1Spt::getFilePath() const
{
    return this->sptFilePath;
}

bool D1Spt::isModified() const
{
    return this->modified;
}

int D1Spt::getSubtileTrapProperty(int subtileIndex) const
{
    return this->trapProperties.at(subtileIndex);
}

bool D1Spt::setSubtileTrapProperty(int subtileIndex, int value)
{
    if (this->trapProperties[subtileIndex] == value) {
        return false;
    }
    this->trapProperties[subtileIndex] = value;
    this->modified = true;
    return true;
}

int D1Spt::getSubtileSpecProperty(int subtileIndex) const
{
    return this->specProperties.at(subtileIndex);
}

bool D1Spt::setSubtileSpecProperty(int subtileIndex, int value)
{
    if (this->specProperties[subtileIndex] == value) {
        return false;
    }
    this->specProperties[subtileIndex] = value;
    this->modified = true;
    return true;
}

void D1Spt::insertSubtile(int subtileIndex)
{
    this->trapProperties.insert(subtileIndex, 0);
    this->specProperties.insert(subtileIndex, 0);
    this->modified = true;
}

void D1Spt::createSubtile()
{
    this->trapProperties.append(0);
    this->specProperties.append(0);
    this->modified = true;
}

void D1Spt::removeSubtile(int subtileIndex)
{
    this->trapProperties.removeAt(subtileIndex);
    this->specProperties.removeAt(subtileIndex);
    this->modified = true;
}

void D1Spt::remapSubtiles(const std::map<unsigned, unsigned> &remap)
{
    QList<int> newTrapProperties;
    QList<int> newSpecProperties;

    for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
        newTrapProperties.append(this->trapProperties.at(iter->second));
        newSpecProperties.append(this->specProperties.at(iter->second));
    }
    this->trapProperties.swap(newTrapProperties);
    this->specProperties.swap(newSpecProperties);
    this->modified = true;
}
