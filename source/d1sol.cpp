#include "d1sol.h"

#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include "progressdialog.h"

bool D1Sol::load(const QString &filePath)
{
    // prepare file data source
    QFile file;
    // done by the caller
    // if (!params.solFilePath.isEmpty()) {
    //    filePath = params.solFilePath;
    // }
    if (!filePath.isEmpty()) {
        file.setFileName(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
    }

    const QByteArray fileData = file.readAll();

    int subtileCount = fileData.size();

    this->subProperties.clear();

    // Read SOL binary data
    QDataStream in(fileData);
    in.setByteOrder(QDataStream::LittleEndian);

    for (int i = 0; i < subtileCount; i++) {
        quint8 readBytr;
        in >> readBytr;
        this->subProperties.append(readBytr);
    }

    this->solFilePath = filePath;
    this->modified = !file.isOpen();
    return true;
}

bool D1Sol::save(const SaveAsParam &params)
{
    QString filePath = this->getFilePath();
    if (!params.solFilePath.isEmpty()) {
        filePath = params.solFilePath;
        if (!params.autoOverwrite && QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("Are you sure you want to overwrite %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    }

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    // write to file
    QDataStream out(&outFile);
    for (int i = 0; i < this->subProperties.size(); i++) {
        out << this->subProperties[i];
    }

    this->solFilePath = filePath; // this->load(filePath);
    this->modified = false;

    return true;
}

QString D1Sol::getFilePath() const
{
    return this->solFilePath;
}

bool D1Sol::isModified() const
{
    return this->modified;
}

int D1Sol::getSubtileCount() const
{
    return this->subProperties.count();
}

quint8 D1Sol::getSubtileProperties(int subtileIndex) const
{
    // if (subtileIndex >= this->subProperties.count())
    //    return 0;

    return this->subProperties.at(subtileIndex);
}

void D1Sol::insertSubtile(int subtileIndex, quint8 value)
{
    this->subProperties.insert(subtileIndex, value);
    this->modified = true;
}

bool D1Sol::setSubtileProperties(int subtileIndex, quint8 value)
{
    if (this->subProperties[subtileIndex] == value) {
        return false;
    }
    this->subProperties[subtileIndex] = value;
    this->modified = true;
    return true;
}

void D1Sol::createSubtile()
{
    this->subProperties.append(0);
    this->modified = true;
}

void D1Sol::removeSubtile(int subtileIndex)
{
    this->subProperties.removeAt(subtileIndex);
    this->modified = true;
}

void D1Sol::remapSubtiles(const std::map<unsigned, unsigned> &remap)
{
    QList<quint8> newSubProperties;

    for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
        newSubProperties.append(this->subProperties.at(iter->second));
    }
    this->subProperties.swap(newSubProperties);
    this->modified = true;
}
