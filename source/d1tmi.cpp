#include "d1tmi.h"

#include <QBuffer>
#include <QDataStream>
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
    if (tmiSubtileCount == 0) {
        // prepare list based on the sol-flags
        for (int i = 0; i < subtileCount; i++) {
            quint8 solFlags = sol->getSubtileProperties(i);
            quint8 tmi = 0;
            if (solFlags & (1 << (4 - 1))) {
                // sol: enabled transparency -> tmi: enable transparency on the wall
                tmi |= 1 << (1 - 1);
            }
            if (solFlags & (1 << (5 - 1))) {
                // sol: left floor {CEL FRAME} has transparent part -> tmi: left floor {CEL FRAME} has transparent part
                tmi |= 1 << (4 - 1);
            }
            if (solFlags & (1 << (6 - 1))) {
                // sol: right floor {CEL FRAME} has transparent part -> tmi: right floor {CEL FRAME} has transparent part
                tmi |= 1 << (7 - 1);
            }
            this->subProperties.append(tmi);
        }
    } else {
        // prepare empty list with zeros
        for (int i = 0; i < subtileCount; i++) {
            this->subProperties.append(0);
        }
    }

    // Read TMI binary data
    QDataStream in(&fileBuffer);

    for (int i = 0; i < tmiSubtileCount; i++) {
        quint8 readBytr;
        in >> readBytr;
        this->subProperties[i] = readBytr;
    }

    this->tmiFilePath = filePath;
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

void D1Tmi::setSubtileProperties(int subtileIndex, quint8 value)
{
    this->subProperties[subtileIndex] = value;
}

void D1Tmi::insertSubtile(int subtileIndex, quint8 value)
{
    this->subProperties.insert(subtileIndex, value);
}

void D1Tmi::createSubtile()
{
    this->subProperties.append(0);
}

void D1Tmi::removeSubtile(int subtileIndex)
{
    this->subProperties.removeAt(subtileIndex);
}
