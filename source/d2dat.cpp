#include "d2dat.h"

#include <array>

#include <QApplication>
#include <QChar>
#include <QDir>
#include <QFile>
#include <QMessageBox>

#include "progressdialog.h"

#include "dungeon/all.h"

void D2Dat::loadRegularPalettes(QFile &file, const QString &baseFilePath, std::vector<D1Pal *> &pals)
{
    const QByteArray fileData = file.readAll();

    unsigned numPals = fileData.size() / D2PAL_SIZE_BYTES;
    unsigned cursor = 0;
    for (unsigned i = 0; i < numPals; i++) {
        D1Pal *pal = new D1Pal();

        for (int i = 0; i < D2PAL_COLORS; i++) {
            quint8 red = fileData[cursor];
            cursor++;

            quint8 green = fileData[cursor];
            cursor++;

            quint8 blue = fileData[cursor];
            cursor++;

            pal->colors[i] = QColor(red, green, blue);
        }

        QString counter = QString::number(i);
        counter = counter.rightJustified(2, QChar('0'));
        QString palFilePath = baseFilePath;
        palFilePath.append(counter + ".pal");
        pal->setFilePath(palFilePath);
        pals.push_back(pal);
    }
}

static const unsigned trnSize = D2PAL_COLORS;
static const unsigned trnpalSize = D2PAL_COLORS * 4 + trnSize * (32 + 16 + 3 * 256 + 256 + 256 + 111 + 1 + 1 + 1 + 14 + 256 + 1) + 13 * 3 + trnSize * 14;

static unsigned readPalette4(const QByteArray &fileData, unsigned cursor, const QString &filePath, std::vector<D1Pal *> &pals)
{
    D1Pal *pal = new D1Pal();
    
    for (int i = 0; i < D2PAL_COLORS; i++) {
        quint8 red = fileData[cursor];
        cursor++;

        quint8 green = fileData[cursor];
        cursor++;

        quint8 blue = fileData[cursor];
        cursor++;

        cursor++;

        pal->setColor(i, QColor(red, green, blue));
    }

    QColor undefinedColor = QColor(Config::getPaletteUndefinedColor());

    pal->setUndefinedColor(undefinedColor);
    pal->setFilePath(filePath);
    pals.push_back(pal);
    return cursor;
}

static unsigned read13Palette3(const QByteArray &fileData, unsigned cursor, const QString &filePath, std::vector<D1Pal *> &pals)
{
    D1Pal *pal = new D1Pal();
    int i = 0;
    for ( ; i < 13; i++) {
        quint8 red = fileData[cursor];
        cursor++;

        quint8 green = fileData[cursor];
        cursor++;

        quint8 blue = fileData[cursor];
        cursor++;

        cursor++;

        pal->setColor(i, QColor(red, green, blue));
    }

    QColor undefinedColor = QColor(Config::getPaletteUndefinedColor());
    for ( ; i < D2PAL_COLORS; i++) {
        pal->setColor(i, undefinedColor);
    }

    pal->setUndefinedColor(undefinedColor);
    pal->setFilePath(filePath);
    pals.push_back(pal);
    return cursor;
}

static unsigned readTransform(const QByteArray &fileData, unsigned cursor, const QString &filePath, D1Pal* pal, std::vector<D1Trn *> &trns)
{
    D1Trn *trn = new D1Trn();
    for (unsigned n = 0; n < D1TRN_TRANSLATIONS; n++) {
        quint8 readByte = fileData[cursor];
        cursor++;
        trn->setTranslation(n, readByte);
    }
    trn->setFilePath(filePath);
    trn->setPalette(pal);
    trn->refreshResultingPalette();
    trns.push_back(trn);
    return cursor;
}

void D2Dat::loadTransformationPalette(QFile &file, const QString &baseFilePath, std::vector<D1Pal *> &pals, std::vector<D1Trn *> &trns)
{
/*
    Palette	256 * 4 bytes	0's between each R,G,B byte sequence
Dark-to-mid Light Levels	32 transforms	a gradient of transforms, each being lighter
Mid-to-bright Light Levels	16 transforms	a gradient of transforms, each being lighter
Alpha-blend	3 * 256 transforms	25%, 50%, 75% opacity blends against each other color in the palette
Add blend-mode	256 transforms	the "add" blend against each color in the palette
Add multiply-mode	256 transforms	the "multiply" blend against each color in the palette
Color variations	111 transforms	various hue-shifting transformations
Red Tones	1 transform	a transform of only red tones
Green Tones	1 transform	same, but only green tones
Blue Tones	1 transform	same, but only blue tones
Unknown(?)	14 transforms	
Max Component blend	256 transforms	
Darkened color shift	1 transform	
Text color palette	256 * 4 bytes	this is another color palette, but for text
Text color shifts	14 transforms	color-shifts used for text
*/
    const QByteArray fileData = file.readAll();


    unsigned cursor = 0;
    {
        QString filePath = baseFilePath + "base.pal";
        cursor = readPalette4(fileData, cursor, filePath, pals);
    }

    D1Pal* pal = pals.front();

    for (unsigned i = 0; i < 32; i++) {
        QString filePath = baseFilePath + QString("dark%1.trn").arg(i, 2, QChar('0'));
        cursor = readTransform(fileData, cursor, filePath, pal, trns);
    }

    for (unsigned i = 0; i < 16; i++) {
        QString filePath = baseFilePath + QString("light%1.trn").arg(i, 2, QChar('0'));
        cursor = readTransform(fileData, cursor, filePath, pal, trns);
    }

    for (unsigned n = 0; n < 3; n++) {
        for (unsigned i = 0; i < 256; i++) {
            QString filePath = baseFilePath + QString("alpha%1_%2.trn").arg((n + 1) * 25).arg(i, 3, QChar('0'));
            cursor = readTransform(fileData, cursor, filePath, pal, trns);
        }
    }

    for (unsigned i = 0; i < 256; i++) {
        QString filePath = baseFilePath + QString("add%1.trn").arg(i, 2, QChar('0'));
        cursor = readTransform(fileData, cursor, filePath, pal, trns);
    }

    for (unsigned i = 0; i < 256; i++) {
        QString filePath = baseFilePath + QString("mul%1.trn").arg(i, 2, QChar('0'));
        cursor = readTransform(fileData, cursor, filePath, pal, trns);
    }

    for (unsigned i = 0; i < 111; i++) {
        QString filePath = baseFilePath + QString("hue%1.trn").arg(i, 2, QChar('0'));
        cursor = readTransform(fileData, cursor, filePath, pal, trns);
    }

    for (unsigned i = 0; i < 1; i++) {
        QString filePath = baseFilePath + QString("red.trn");
        cursor = readTransform(fileData, cursor, filePath, pal, trns);
    }

    for (unsigned i = 0; i < 1; i++) {
        QString filePath = baseFilePath + QString("green.trn");
        cursor = readTransform(fileData, cursor, filePath, pal, trns);
    }

    for (unsigned i = 0; i < 1; i++) {
        QString filePath = baseFilePath + QString("blue.trn");
        cursor = readTransform(fileData, cursor, filePath, pal, trns);
    }

    for (unsigned i = 0; i < 14; i++) {
        QString filePath = baseFilePath + QString("un%1.trn").arg(i, 2, QChar('0'));
        cursor = readTransform(fileData, cursor, filePath, pal, trns);
    }

    for (unsigned i = 0; i < 256; i++) {
        QString filePath = baseFilePath + QString("blend%1.trn").arg(i, 3, QChar('0'));
        cursor = readTransform(fileData, cursor, filePath, pal, trns);
    }

    for (unsigned i = 0; i < 1; i++) {
        QString filePath = baseFilePath + QString("shift.trn");
        cursor = readTransform(fileData, cursor, filePath, pal, trns);
    }

    {
        QString filePath = baseFilePath + "text.pal";
        cursor = readPalette4(fileData, cursor, filePath, pals);
    }

    for (unsigned i = 0; i < 14; i++) {
        QString filePath = baseFilePath + QString("txtshift%1.trn").arg(i, 2, QChar('0'));
        cursor = readTransform(fileData, cursor, filePath, pal, trns);
    }
}

bool D2Dat::load(const QString &filePath, std::vector<D1Pal *> &pals, std::vector<D1Trn *> &trns)
{
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    QString baseFilePath = filePath;
    baseFilePath.resize(baseFilePath.length() - 4);

    size_t fileSize = file.size();
    if (fileSize == trnpalSize) {
        loadTransformationPalette(file, baseFilePath, pals, trns);
    } else if ((fileSize % D1PAL_SIZE_BYTES) == 0) {
        loadRegularPalettes(file, baseFilePath, pals);
    } else {
        return false;
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
