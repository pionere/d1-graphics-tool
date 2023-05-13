#include "d1gfxset.h"

#include <QApplication>
#include <QMessageBox>

#include "d1cl2.h"
#include "progressdialog.h"

#include "dungeon/all.h"

// clang-format off
static const PlrAnimType PlrAnimTypes[NUM_PGTS] = {
    { "ST", PGX_STAND },     // PGT_STAND_TOWN
    { "AS", PGX_STAND },     // PGT_STAND_DUNGEON
    { "WL", PGX_WALK },      // PGT_WALK_TOWN
    { "AW", PGX_WALK },      // PGT_WALK_DUNGEON
    { "AT", PGX_ATTACK },    // PGT_ATTACK
    { "FM", PGX_FIRE },      // PGT_FIRE
    { "LM", PGX_LIGHTNING }, // PGT_LIGHTNING
    { "QM", PGX_MAGIC },     // PGT_MAGIC
    { "BL", PGX_BLOCK },     // PGT_BLOCK
    { "HT", PGX_GOTHIT },    // PGT_GOTHIT
    { "DT", PGX_DEATH },     // PGT_DEATH
};

/** Maps from armor animation to letter used in graphic files. */
const char ArmorChar[] = {
    'L', // light
    'M', // medium
    'H', // heavy
};
/** Maps from weapon animation to letter used in graphic files. */
const char WepChar[] = {
    'N', // unarmed
    'U', // no weapon + shield
    'S', // sword + no shield
    'D', // sword + shield
    'B', // bow
    'A', // axe
    'M', // blunt + no shield
    'H', // blunt + shield
    'T', // staff
};
/** Maps from player class to letter used in graphic files. */
const char CharChar[NUM_CLASSES] = { 'W', 'R', 'S', 'M' };

static const char animletter[NUM_MON_ANIM] = { 'N', 'W', 'A', 'H', 'D', 'S' };
// clang-format on

D1Gfxset::D1Gfxset(D1Gfx *g)
    : baseGfx(g)
{
}

D1Gfxset::~D1Gfxset()
{
    for (unsigned i = 0; i < this->gfxList.size(); i++) {
        D1Gfx *gfx = this->gfxList[i];
        if (gfx != this->baseGfx) {
            mem_free_dbg(gfx);
        }
    }
    this->gfxList.clear();
}

bool D1Gfxset::load(const QString &gfxFilePath, const OpenAsParam &params)
{
    if (!D1Cl2::load(*this->baseGfx, gfxFilePath, params)) {
        dProgressErr() << QApplication::tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(gfxFilePath));
    } else {
        QFileInfo celFileInfo = QFileInfo(gfxFilePath);

        const bool uppercase = gfxFilePath.endsWith(".CL2");
        const QString extension = uppercase ? ".CL2" : ".cl2";
        QString baseName = celFileInfo.completeBaseName();
        QString basePath = celFileInfo.absolutePath() + "/";
        std::vector<QString> filePaths;
        D1GFX_SET_TYPE type = D1GFX_SET_TYPE::Unknown;
        D1GFX_SET_CLASS_TYPE ctype = D1GFX_SET_CLASS_TYPE::Unknown;
        D1GFX_SET_ARMOR_TYPE atype = D1GFX_SET_ARMOR_TYPE::Unknown;
        D1GFX_SET_WEAPON_TYPE wtype = D1GFX_SET_WEAPON_TYPE::Unknown;
        if (!baseName.isEmpty() && baseName[baseName.length() - 1].isDigit()) {
            // ends with a number -> missile animation
            while (!baseName.isEmpty() && baseName[baseName.length() - 1].isDigit()) {
                baseName.chop(1);
            }
            int n = 0;
            basePath += baseName;
            while (true) {
                n++;
                QString filePath = basePath + QString::number(n) + extension;
                if (!QFileInfo::exists(filePath))
                    break;
            }
            n = n > 9 ? 16 : 8;
            for (int i = 1; i <= n; i++) {
                QString filePath = basePath + QString::number(i) + extension;
                filePaths.push_back(filePath);
            }
            type = D1GFX_SET_TYPE::Missile;
        } else {
            std::vector<QString> fileMatchesPlr;
            if (baseName.length() == 5) {
                bool plrMatch = false;
                QString basePlrPath = basePath + baseName.mid(0, 3);
                for (int i = 0; i < lengthof(PlrAnimTypes); i++) {
                    QString filePath = basePlrPath + PlrAnimTypes[i].patTxt[0] + PlrAnimTypes[i].patTxt[1] + extension;
                    if (!QFileInfo::exists(filePath))
                        continue;
                    if (filePath.compare(gfxFilePath, Qt::CaseInsensitive) != 0) {
                        fileMatchesPlr.push_back(filePath);
                    } else {
                        plrMatch = true;
                    }
                }
                if (!plrMatch) {
                    fileMatchesPlr.clear();
                }
            }
            std::vector<QString> fileMatchesMon;
            if (baseName.length() > 1) {
                bool monMatch = false;
                QString baseMonPath = basePath + baseName;
                baseMonPath.chop(1);
                for (int i = 0; i < lengthof(animletter); i++) {
                    QString filePath = baseMonPath + animletter[i] + extension;
                    if (!QFileInfo::exists(filePath))
                        continue;
                    if (filePath.compare(gfxFilePath, Qt::CaseInsensitive) != 0) {
                        fileMatchesMon.push_back(filePath);
                    } else {
                        monMatch = true;
                    }
                }
                if (!monMatch) {
                    fileMatchesMon.clear();
                }
            }
            if (fileMatchesMon.size() >= fileMatchesPlr.size()) {
                type = D1GFX_SET_TYPE::Monster;
                QString baseMonPath = basePath + baseName;
                baseMonPath.chop(1);
                for (int i = 0; i < lengthof(animletter); i++) {
                    QString anim;
                    anim = anim + animletter[i] + extension;
                    if (!uppercase)
                        anim = anim.toLower();
                    QString filePath = baseMonPath + anim;
                    filePaths.push_back(filePath);
                }
            } else {
                type = D1GFX_SET_TYPE::Player;
                QString basePlrPath = basePath + baseName.mid(0, 3);
                for (int i = 0; i < lengthof(PlrAnimTypes); i++) {
                    QString anim;
                    anim = anim + PlrAnimTypes[i].patTxt[0] + PlrAnimTypes[i].patTxt[1] + extension;
                    if (!uppercase)
                        anim = anim.toLower();
                    QString filePath = basePlrPath + anim;
                    filePaths.push_back(filePath);
                }
                for (int i = 0; i < lengthof(CharChar); i++) {
                    if (baseName[0].toUpper() == CharChar[i]) {
                        ctype = (D1GFX_SET_CLASS_TYPE)i;
                    }
                }
                for (int i = 0; i < lengthof(ArmorChar); i++) {
                    if (baseName[1].toUpper() == ArmorChar[i]) {
                        atype = (D1GFX_SET_ARMOR_TYPE)i;
                    }
                }
                for (int i = 0; i < lengthof(WepChar); i++) {
                    if (baseName[2].toUpper() == WepChar[i]) {
                        wtype = (D1GFX_SET_WEAPON_TYPE)i;
                    }
                }
            }
        }
        this->type = type;
        this->ctype = ctype;
        this->atype = atype;
        this->wtype = wtype;

        // load the files
        std::vector<D1Gfx *> gfxs;
        D1Pal *pal = this->baseGfx->getPalette();
        bool baseMatch = false;
        for (const QString &filePath : filePaths) {
            D1Gfx *gfx;
            if (filePath.compare(gfxFilePath, Qt::CaseInsensitive) != 0) {
                gfx = new D1Gfx();
                gfx->setPalette(pal);
                if (!D1Cl2::load(*gfx, filePath, params) || this->baseGfx->getType() != gfx->getType()) {
                    gfx->setType(this->baseGfx->getType());
                    gfx->setFilePath(filePath);
                    // treat empty files as non-modified
                    if (gfx->getFrameCount() == 0) {
                        gfx->setModified(false);
                    }
                }
            } else {
                gfx = this->baseGfx;
                baseMatch = true;
            }
            this->gfxList.push_back(gfx);
            // validate the gfx
            if (gfx->getGroupCount() == 0) {
                dProgressWarn() << QApplication::tr("%1 is empty.").arg(gfx->getFilePath());
            } else if (type != D1GFX_SET_TYPE::Missile && gfx->getGroupCount() != NUM_DIRS) {
                dProgressWarn() << QApplication::tr("%1 has invalid group-count.").arg(gfx->getFilePath());
            } else if (type == D1GFX_SET_TYPE::Missile && gfx->getGroupCount() != 1) {
                dProgressWarn() << QApplication::tr("%1 has more than one group.").arg(gfx->getFilePath());
            }
        }
        if (!baseMatch) {
            QString firstFilePath = this->gfxList[0]->getFilePath();
            delete this->gfxList[0];
            this->gfxList[0] = this->baseGfx;
            this->baseGfx->setFilePath(firstFilePath);
        }
        return true;
    }
    // clear possible inconsistent data
    // this->gfxList.clear();
    return false;
}

void D1Gfxset::save(const SaveAsParam &params)
{
    SaveAsParam saveParams = params;
    QString filePath = saveParams.celFilePath;
    bool uppercase = false;
    if (!filePath.isEmpty()) {
        if (filePath.toLower().endsWith(".cl2")) {
            uppercase = filePath[filePath.length() - 2] == 'L';
            filePath.chop(4);
        }
        if (this->type == D1GFX_SET_TYPE::Missile) {
            if (filePath[filePath.length() - 1] == '1') {
                filePath.chop(1);
            }
        } else if (this->type == D1GFX_SET_TYPE::Monster) {
            if (filePath[filePath.length() - 1].toUpper() == animletter[0]) {
                filePath.chop(1);
            }
        } else {
            // assert(this->type == D1GFX_SET_TYPE::Player);
            if ((filePath[filePath.length() - 2].toUpper() == PlrAnimTypes[0].patTxt[0])
                && (filePath[filePath.length() - 1].toUpper() == PlrAnimTypes[0].patTxt[1]))
                filePath.chop(2);
        }
    }
    for (unsigned i = 0; i < this->gfxList.size(); i++) {
        if (!filePath.isEmpty()) {
            QString anim;
            if (this->type == D1GFX_SET_TYPE::Missile) {
                anim = QString::number(i + 1);
            } else if (this->type == D1GFX_SET_TYPE::Monster) {
                anim = animletter[i];
            } else {
                // assert(this->type == D1GFX_SET_TYPE::Player);
                anim = anim + QChar(PlrAnimTypes[i].patTxt[0]) + QChar(PlrAnimTypes[i].patTxt[1]);
            }
            anim += ".cl2";
            saveParams.celFilePath = filePath + (uppercase ? anim : anim.toLower());
        }
        D1Gfx *gfx = this->gfxList[i];
        if (gfx->getFrameCount() != 0) {
            D1Cl2::save(*gfx, saveParams);
        } else {
            // CL2 without content -> delete
            QString cl2FilePath = saveParams.celFilePath;
            if (QFile::exists(cl2FilePath) && !QFile::remove(cl2FilePath)) {
                dProgressFail() << tr("Failed to remove file: %1.").arg(QDir::toNativeSeparators(cl2FilePath));
            } else {
                // treat empty files as non-modified
                gfx->setModified(false);
            }
        }
    }
}

D1GFX_SET_TYPE D1Gfxset::getType() const
{
    return this->type;
}

D1GFX_SET_CLASS_TYPE D1Gfxset::getClassType() const
{
    return this->ctype;
}

D1GFX_SET_ARMOR_TYPE D1Gfxset::getArmorType() const
{
    return this->atype;
}

D1GFX_SET_WEAPON_TYPE D1Gfxset::getWeaponType() const
{
    return this->wtype;
}

int D1Gfxset::getGfxCount() const
{
    return this->gfxList.size();
}

void D1Gfxset::setGfx(D1Gfx *gfx)
{
    this->baseGfx = gfx;
}

D1Gfx *D1Gfxset::getGfx(int index) const
{
    return this->gfxList[index];
}

D1Gfx *D1Gfxset::getBaseGfx() const
{
    return this->baseGfx;
}

void D1Gfxset::replacePixels(const std::vector<std::pair<D1GfxPixel, D1GfxPixel>> &replacements, const RemapParam &params)
{
    for (D1Gfx *gfx : this->gfxList) {
        int rangeFrom = params.frames.first;
        if (rangeFrom != 0) {
            rangeFrom--;
        }
        int rangeTo = params.frames.second;
        if (rangeTo == 0 || rangeTo >= gfx->getFrameCount()) {
            rangeTo = gfx->getFrameCount();
        }
        rangeTo--;

        for (int i = rangeFrom; i <= rangeTo; i++) {
            D1GfxFrame *frame = gfx->getFrame(i);
            frame->replacePixels(replacements);
        }
        gfx->setModified();
    }
}

void D1Gfxset::frameModified(D1GfxFrame *frame)
{
    for (D1Gfx *gfx : this->gfxList) {
        for (int i = 0; i < gfx->getFrameCount(); i++) {
            if (gfx->getFrame(i) == frame) {
                gfx->setModified();
            }
        }
    }
}

void D1Gfxset::setPalette(D1Pal *pal)
{
    for (D1Gfx *gfx : this->gfxList) {
        gfx->setPalette(pal);
    }
}
