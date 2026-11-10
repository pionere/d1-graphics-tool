#include "d2dat.h"

#include <array>

#include <QApplication>
#include <QChar>
#include <QDir>
#include <QFile>
#include <QMessageBox>

bool D2Dat::load(const QString &filePath, std::vector<D1Pal *> &pals)
{
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    size_t fileSize = file.size();
    if ((fileSize % D2PAL_COLORS) != 0)
        return false;

    const QByteArray fileData = file.readAll();
    // Read SLA binary data
    QDataStream in(fileData);

    unsigned numPals = fileSize / D2PAL_COLORS;
    for (unsigned i = 0; i < numPals; i++) {
        D1Pal *pal = new D1Pal();

        for (int i = 0; i < D2PAL_COLORS; i++) {
            quint8 red;
            in >> red;

            quint8 green;
            in >> green;

            quint8 blue;
            in >> blue;

            pal->colors[i] = QColor(red, green, blue);
        }


        QString counter = QString::number(i);
        counter = counter.rightJustified(2, QChar('0'));
        QString palFilePath = filePath;
        palFilePath.resize(filePath.length() - 4);
        palFilePath.append(counter + ".pal");
        pal->setFilePath(palFilePath);
        pals.push_back(pal);
    }
    return true;
}

bool D2Dat::save(const QString &filePath, const std::vector<D1Pal *> &pals)
{
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath)));
        return false;
    }

    // write to file
    QDataStream out(&outFile);

    for (const D1Pal *pal : pals) {
        for (int i = 0; i < D2PAL_COLORS; i++) {
            QColor color = pal->colors[i];
            quint8 byteToWrite;

            byteToWrite = color.red();
            out << byteToWrite;

            byteToWrite = color.green();
            out << byteToWrite;

            byteToWrite = color.blue();
            out << byteToWrite;
        }
    }

    return true;
}
