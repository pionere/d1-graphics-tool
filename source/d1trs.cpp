#include "d1trs.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>

#include "trngeneratedialog.h"
#include "dungeon/all.h"

bool D1Trs::load(const QString &filePath, D1Pal *pal, std::vector<D1Trn *> &trns)
{
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    size_t fileSize = file.size();
    if ((fileSize % D1TRN_TRANSLATIONS) != 0)
        return false;

    const QByteArray fileData = file.readAll();
    // Read SLA binary data
    QDataStream in(fileData);

    unsigned numTrns = fileSize / D1TRN_TRANSLATIONS;
    for (unsigned i = 0; i < numTrns; i++) {
        D1Trn *trn = new D1Trn();
        quint8 readByte;
        for (unsigned n = 0; n < D1TRN_TRANSLATIONS; n++) {
            in >> readByte;
            trn->setTranslation(n, readByte);
        }
        QString trnFilePath = filePath;
        trnFilePath.insert(filePath.length() - 4, QString::number(i));
        trn->setFilePath(trnFilePath);
        trn->setPalette(pal);
        trn->refreshResultingPalette();
        trns.push_back(trn);
    }
    return true;
}

bool D1Trs::save(const QString &filePath, const std::vector<D1Trn *> &trns)
{
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath)));
        return false;
    }

    // write to file
    QDataStream out(&outFile);

    for (const D1Trn *trn : trns) {
        quint8 writeByte;
        for (unsigned n = 0; n < D1TRN_TRANSLATIONS; n++) {
            writeByte = trn->getTranslation(n);
            out << writeByte;
        }
    }

    return true;
}

void D1Trs::generateLightTranslations(const GenerateTrnParam &params, std::vector<D1Trn *> &trns)
{
    currLvl._dType = DTYPE_TOWN;
    MakeLightTable();

    QString filePath = QApplication::tr("Light%1.trn");
    for (int i = 0; i <= MAXDARKNESS; i++) {
        D1Trn *trn = new D1Trn();
        for (unsigned n = 0; n < D1TRN_TRANSLATIONS; n++) {
            trn->setTranslation(n, ColorTrns[i][n]);
        }
        QString trnFilePath = filePath.arg(i, 2, 10, '0');
        trn->setFilePath(trnFilePath);
        trn->setPalette(pal);
        trn->refreshResultingPalette();
        trns.push_back(trn);
    }
}
