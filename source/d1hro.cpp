#include "d1hro.h"

#include <vector>

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>

#include "dungeon/all.h"

#include "progressdialog.h"

bool D1Hero::load(const QString &filePath, const OpenAsParam &params)
{
    // Opening CEL file and load it in RAM
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QByteArray fileData = file.readAll();

    if (fileData.size() != sizeof(PkPlayerStruct))
        return false;

    UnPackPlayer((const PkPlayerStruct*)fileData.constData(), this->pnum);

    this->filePath = filePath;
    this->modified = false;

    return true;
}

D1Hero* D1Hero::instance()
{
    for (int pnum = 0; pnum < MAX_PLRS; pnum++) {
        if (!plr._pActive) {
            plr._pActive = TRUE;
            D1Hero* hero = new D1Hero();
            hero->pnum = pnum;
            return hero;
        }
    }
    return nullptr;
}

D1Hero::~D1Hero()
{
    plr._pActive = FALSE;
}

void D1Hero::compareTo(const D1Hero *hero, QString header) const
{

}

QString D1Hero::getFilePath() const
{
    return this->filePath;
}

void D1Hero::setFilePath(const QString &filePath)
{
    this->filePath = filePath;
    this->modified = true;
}

bool D1Hero::isModified() const
{
    return this->modified;
}

void D1Hero::setModified(bool modified)
{
    this->modified = modified;
}

bool D1Hero::save(const SaveAsParam &params)
{
    QString filePath = this->filePath;
    if (!params.filePath.isEmpty()) {
        filePath = params.filePath;
        if (!params.autoOverwrite && QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, QApplication::tr("Confirmation"), QApplication::tr("Are you sure you want to overwrite %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    }
    else if (!this->isModified()) {
        return false;
    }

    if (filePath.isEmpty()) {
        return false;
    }

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    PkPlayerStruct pps;
    PackPlayer(&pps, this->pnum);

    // write to file
    QDataStream out(&outFile);
    bool result = out.writeRawData((char *)&pps, sizeof(PkPlayerStruct)) == sizeof(PkPlayerStruct);
    if (result) {
        this->filePath = filePath; //  D1Hero::load(gfx, filePath);
        this->modified = false;
    }

    return result;
}
