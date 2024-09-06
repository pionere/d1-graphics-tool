#include "d1hro.h"

#include <vector>

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>

#include "dungeon/all.h"

#include "progressdialog.h"

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

void D1Hero::create(unsigned index)
{
    _uiheroinfo selhero_heroInfo;

    //SelheroClassSelectorFocus(unsigned index)

	selhero_heroInfo.hiIdx = MAX_CHARACTERS;
	selhero_heroInfo.hiLevel = 1;
	selhero_heroInfo.hiClass = index;
	//selhero_heroInfo.hiRank = 0;
	selhero_heroInfo.hiStrength = StrengthTbl[index];   //defaults.dsStrength;
	selhero_heroInfo.hiMagic = MagicTbl[index];         //defaults.dsMagic;
	selhero_heroInfo.hiDexterity = DexterityTbl[index]; //defaults.dsDexterity;
	selhero_heroInfo.hiVitality = VitalityTbl[index];   //defaults.dsVitality;

    selhero_heroInfo.hiName[0] = '\0';

    CreatePlayer(this->pnum, &hi);

    this->filePath.clear();
    this->modified = true;
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

const char* D1Hero::getName() const
{
    return players[this->pnum]._pName;
}

void D1Hero::setName(const QString &name)
{
    QString currName = QString(players[this->pnum]._pName);
    if (currName == name)
        return;

    memcpy(players[this->pnum]._pName, name.constData(), name.length() > lengthof(players[this->pnum]._pName) ? lengthof(players[this->pnum]._pName) : name.length());

    players[this->pnum]._pName[lengthof(players[this->pnum]._pName) - 1] = '\0';

    this->modified = true;
}

int D1Hero::getClass() const
{
    return players[this->pnum]._pClass;
}

void D1Hero::setClass(int cls)
{
    if (players[this->pnum]._pClass == cls)
        return;
    players[this->pnum]._pClass = cls;

    CalcPlrInv(this->pnum, false);

    this->modified = true;
}

int D1Hero::getLevel() const
{
    return players[this->pnum]._pLevel;
}

int D1Hero::getStatPoints() const
{
    return players[this->pnum]._pStatPts;
}

void D1Hero::setLevel(int level)
{
    int dlvl;
    dlvl = players[this->pnum]._pLevel - level;
    if (dlvl == 0)
        return;
    players[this->pnum]._pLevel = level;
    if (dlvl > 0) {
        players[this->pnum]._pStatPts += 4 * dlvl;
    } else {
        // FIXME...
    }
    players[this->pnum]._pExperience = PlrExpLvlsTbl[level - 1];

    CalcPlrInv(this->pnum, false);

    this->modified = true;
}

int D1Hero::getStrength() const
{
    return players[this->pnum]._pStrength;
}

int D1Hero::getBaseStrength() const
{
    return players[this->pnum]._pBaseStr;
}

void D1Hero::addStrength()
{
    IncreasePlrStr(this->pnum);
    this->modified = true;
}

int D1Hero::getDexterity() const
{
    return players[this->pnum]._pDexterity;
}

int D1Hero::getBaseDexterity() const
{
    return players[this->pnum]._pBaseDex;
}

void D1Hero::addDexterity()
{
    IncreasePlrDex(this->pnum);
    this->modified = true;
}

int D1Hero::getMagic() const
{
    return players[this->pnum]._pMagic;
}

int D1Hero::getBaseMagic() const
{
    return players[this->pnum]._pBaseMag;
}

void D1Hero::addMagic()
{
    IncreasePlrMag(this->pnum);
    this->modified = true;
}

int D1Hero::getVitality() const
{
    return players[this->pnum]._pVitality;
}

int D1Hero::getBaseVitality() const
{
    return players[this->pnum]._pBaseVit;
}

void D1Hero::addVitality()
{
    IncreasePlrVit(this->pnum);
    this->modified = true;
}

int D1Hero::getLife() const
{
    return players[this->pnum]._pMaxHP;
}

int D1Hero::getBaseLife() const
{
    return players[this->pnum]._pMaxHPBase;
}

int D1Hero::getMana() const
{
    return players[this->pnum]._pMaxMana;
}

int D1Hero::getBaseMana() const
{
    return players[this->pnum]._pMaxManaBase;
}

int D1Hero::getMagicResist() const
{
    return players[this->pnum]._pMagResist;
}

int D1Hero::getFireResist() const
{
    return players[this->pnum]._pFireResist;
}

int D1Hero::getLightningResist() const
{
    return players[this->pnum]._pLghtResist;
}

int D1Hero::getAcidResist() const
{
    return players[this->pnum]._pAcidResist;
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
    } else if (!this->isModified()) {
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
