#include "d1tmi.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "d1sol.h"

bool D1Tmi::load(QString filePath, D1Sol *sol, const OpenAsParam &params)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.tmiFilePath.isEmpty()) {
    //    filePath = params.tmiFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly) && !params.tmiFilePath.isEmpty()) {
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
    int subtileCount = sol->getSubtileCount();
    int tmiSubtileCount = fileSize;
    if (tmiSubtileCount != subtileCount) {
        if (tmiSubtileCount != 0) {
            qDebug() << "The size of tmi-file does not align with sol-file";
        }
        if (tmiSubtileCount > subtileCount) {
            tmiSubtileCount = subtileCount; // skip unusable data
        }
    }

    this->subProperties.clear();
    // prepare empty list with zeros
    for (int i = 0; i < subtileCount; i++) {
        this->subProperties.append(0);
    }

    // Read TMI binary data
    QDataStream in(&fileBuffer);

    for (int i = 0; i < tmiSubtileCount; i++) {
        quint8 readByte;
        in >> readByte;
        this->subProperties[i] = readByte;
    }

    this->tmiFilePath = filePath;
    this->modified = filePath.isEmpty();
    return true;
}

bool D1Tmi::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.tmiFilePath.isEmpty()) {
        filePath = params.tmiFilePath;
        if (QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, "Confirmation", "Are you sure you want to overwrite " + filePath + "?", QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    }

    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly | QFile::Truncate)) {
        QMessageBox::critical(nullptr, "Error", "Failed open file: " + filePath);
        return false;
    }

    // write to file
    QDataStream out(&outFile);
    for (int i = 0; i < this->subProperties.size(); i++) {
        out << this->subProperties[i];
    }

    this->tmiFilePath = filePath; // this->load(filePath, allocate);
    this->modified = false;

    return true;
}

QString D1Tmi::getFilePath()
{
    return this->tmiFilePath;
}

quint8 D1Tmi::getSubtileProperties(int subtileIndex)
{
    if (subtileIndex >= this->subProperties.count())
        return 0;

    return this->subProperties.at(subtileIndex);
}

bool D1Tmi::setSubtileProperties(int subtileIndex, quint8 value)
{
    if (this->subProperties[subtileIndex] == value) {
        return false;
    }
    this->subProperties[subtileIndex] = value;
    this->modified = true;
    return true;
}

void D1Tmi::insertSubtile(int subtileIndex, quint8 value)
{
    this->subProperties.insert(subtileIndex, value);
    this->modified = true;
}

void D1Tmi::createSubtile()
{
    this->subProperties.append(0);
    this->modified = true;
}

void D1Tmi::removeSubtile(int subtileIndex)
{
    this->subProperties.removeAt(subtileIndex);
    this->modified = true;
}

void D1Tmi::remapSubtiles(const QMap<unsigned, unsigned> &remap)
{
    QList<quint8> newSubProperties;

    for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
        newSubProperties.append(this->subProperties.at(iter.value()));
    }
    this->subProperties.swap(newSubProperties);
    this->modified = true;
}
