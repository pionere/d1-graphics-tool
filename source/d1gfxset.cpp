#include "d1gfxset.h"

#include <QApplication>
#include <QMessageBox>

#include "d1cl2.h"
#include "mainwindow.h"
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

static const char animletter[NUM_MON_ANIM] = { 'n', 'w', 'a', 'h', 'd', 's' };
// clang-format on

D1Gfxset::D1Gfxset(D1Gfx *g)
    : baseGfx(g)
{
}

D1Gfxset::~D1Gfxset()
{
    for (D1Gfx *gfx : this->gfxList) {
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

        const QString extension = QString(".") + celFileInfo.suffix();
        QString baseName = celFileInfo.completeBaseName();
        QString basePath = celFileInfo.absolutePath() + "/";
        std::vector<QString> filePaths;
        D1GFX_SET_TYPE type = D1GFX_SET_TYPE::Unknown;
        D1GFX_SET_CLASS_TYPE ctype = D1GFX_SET_CLASS_TYPE::Unknown;
        D1GFX_SET_ARMOR_TYPE atype = D1GFX_SET_ARMOR_TYPE::Unknown;
        D1GFX_SET_WEAPON_TYPE wtype = D1GFX_SET_WEAPON_TYPE::Unknown;
        if (!baseName.isEmpty() && baseName[baseName.length() - 1].isDigit()) {
            // ends with a number -> missile animation
            QChar lastDigit = baseName[baseName.length() - 1];
            baseName.chop(1);
            if (lastDigit.digitValue() <= 6 && !baseName.isEmpty() && baseName.endsWith('1')) {
                baseName.chop(1);
            }
            basePath += baseName;
            int n = 0, cn = 0, mn = 0;
            for (int i = 0; i < 16; i++) {
                QString filePath = basePath + QString::number(i + 1) + extension;
                if (gfxFilePath == filePath) {
                    cn = i;
                }
                if (QFileInfo::exists(filePath)) {
                    // n++;
                    if (i > mn) {
                        mn = i;
                    }
                }
            }
            n = (/*n > 8 ||*/ cn >= 8 || mn >= 8) ? 16 : 8;
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
                    QString filePath = baseMonPath + anim;
                    filePaths.push_back(filePath);
                }
            } else {
                type = D1GFX_SET_TYPE::Player;
                QString basePlrPath = basePath + baseName.mid(0, 3);
                for (int i = 0; i < lengthof(PlrAnimTypes); i++) {
                    QString anim;
                    anim = anim + PlrAnimTypes[i].patTxt[0] + PlrAnimTypes[i].patTxt[1] + extension;
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
    QString extension = QString(".CL2");
    if (!filePath.isEmpty()) {
        QFileInfo celFileInfo = QFileInfo(filePath);

        extension = QString(".") + celFileInfo.suffix();
        filePath.chop(extension.length());

        if (this->type == D1GFX_SET_TYPE::Missile) {
            if (filePath.endsWith('1')) {
                filePath.chop(1);
            }
        } else if (this->type == D1GFX_SET_TYPE::Monster) {
            if (filePath.endsWith(animletter[0])) {
                filePath.chop(1);
            }
        } else {
            // assert(this->type == D1GFX_SET_TYPE::Player);
            if (filePath.endsWith(QString() + PlrAnimTypes[0].patTxt[0] + PlrAnimTypes[0].patTxt[1])) {
                filePath.chop(2);
            }
        }
    }
    for (int i = 0; i < this->gfxList.count(); i++) {
        if (!filePath.isEmpty()) {
            QString anim;
            if (this->type == D1GFX_SET_TYPE::Missile) {
                anim = QString::number(i + 1);
            } else if (this->type == D1GFX_SET_TYPE::Monster) {
                anim = animletter[i];
            } else {
                // assert(this->type == D1GFX_SET_TYPE::Player);
                anim = anim + PlrAnimTypes[i].patTxt[0] + PlrAnimTypes[i].patTxt[1];
            }
            anim += extension;
            saveParams.celFilePath = filePath + anim;
        }
        D1Gfx *gfx = this->gfxList[i];
        if (gfx->getFrameCount() != 0) {
            D1Cl2::save(*gfx, saveParams);
        } else {
            // CL2 without content -> delete
            QString cl2FilePath = saveParams.celFilePath;
            if (QFile::exists(cl2FilePath) && !QFile::remove(cl2FilePath)) {
                dProgressFail() << QApplication::tr("Failed to remove file: %1.").arg(QDir::toNativeSeparators(cl2FilePath));
            } else {
                // treat empty files as non-modified
                gfx->setModified(false);
            }
        }
    }
}

void D1Gfxset::compareTo(const LoadFileContent *fileContent) const
{
    if (fileContent->gfxset != nullptr) {
        if (this->gfxList.count() == fileContent->gfxset->gfxList.count()) {
            // compare N to N
            for (int i = 0; i < this->gfxList.count(); i++) {
                QString header = QApplication::tr("Gfx %1 vs. %2").arg(i + 1).arg(i + 1);
                this->gfxList[i]->compareTo(fileContent->gfxset->gfxList[i], header);
                if (header.isEmpty())
                    dProgress() << "\n";
            }
        } else {
            if (this->gfxList.count() == 1) {
                // compare 1 to N
                for (int i = 0; i < fileContent->gfxset->gfxList.count(); i++) {
                    QString header = QApplication::tr("Gfx %1 vs. %2").arg(0 + 1).arg(i + 1);
                    this->gfxList[0]->compareTo(fileContent->gfxset->gfxList[i], header);
                    if (header.isEmpty())
                        dProgress() << "\n";
                }
            } else if (fileContent->gfxset->gfxList.count() == 1) {
                // compare N to 1
                for (int i = 0; i < this->gfxList.count(); i++) {
                    QString header = QApplication::tr("Gfx %1 vs. %2").arg(i + 1).arg(0 + 1);
                    this->gfxList[i]->compareTo(fileContent->gfxset->gfxList[0], header);
                    if (header.isEmpty())
                        dProgress() << "\n";
                }
            } else {
                // compare N1 to N2
                dProgress() << QApplication::tr("number of graphics is %1 (was %2)").arg(this->gfxList.count()).arg(fileContent->gfxset->gfxList.count());
            }
        }
    } else if (fileContent->gfx != nullptr) {
        // compare N to 1
        for (int i = 0; i < this->gfxList.count(); i++) {
            QString header = QApplication::tr("Gfx %1 vs. single gfx").arg(i + 1);
            this->gfxList[i]->compareTo(fileContent->gfx, header);
            if (header.isEmpty())
                dProgress() << "\n";
        }
    } else {
        dProgressErr() << QApplication::tr("Not a graphics file (%1)").arg(MainWindow::FileContentTypeToStr(fileContent->fileType));
    }
}

QRect D1Gfxset::getBoundary() const
{
    QRect rect = QRect();
    for (const D1Gfx *gfx : this->gfxList) {
        rect |= gfx->getBoundary();
        /*QRect gRect = gfx->getBoundary();
        if (rect.isNull()) {
            rect = gRect;
        } else {
            rect |= gRect;
        }*/

    }
    return rect;
}

bool D1Gfxset::checkGraphics(int frameCount, int animWidth, int gn, const D1Gfx* gfx) const
{
    bool result = false;
    D1Gfx* currGfx = this->getGfx(gn);
    if (gfx != nullptr && gfx != currGfx)
        return false;
    for (int i = 0; i < currGfx->getGroupCount(); i++) {
        std::pair<int, int> gfi = currGfx->getGroupFrameIndices(i);
        int fc = gfi.second - gfi.first + 1;
        if (fc != frameCount) {
            dProgress() << QApplication::tr("framecount of group %1 of %2 does not match with the game (%3 vs %4).").arg(i + 1).arg(this->getGfxLabel(gn)).arg(fc).arg(frameCount);
            result = true;
        }
        for (int ii = 0; ii < fc; ii++) {
            int w = currGfx->getFrame(gfi.first + ii)->getWidth();
            if (w != animWidth) {
                dProgress() << QApplication::tr("framewidth of frame %1 of group %2 in %3 does not match with the game (%4 vs %5).").arg(ii + 1).arg(i + 1).arg(this->getGfxLabel(gn)).arg(w).arg(animWidth);
                result = true;
            }
        }
    }
    return result;
}

bool D1Gfxset::check(const D1Gfx *gfx, int assetMpl) const
{
    bool result = false;
    int frameCount = -1, width = -1, height = -1;
    for (int gn = 0; gn < this->getGfxCount(); gn++) {
        D1Gfx *currGfx = this->getGfx(gn);
        if (gfx != nullptr && gfx != currGfx)
            continue;
        // test whether the graphics use colors from the level-dependent range 
        const std::pair<int, int> colors = { 1, 128 - 1 };
        for (int i = 0; i < currGfx->getFrameCount(); i++) {
            const std::vector<std::vector<D1GfxPixel>> pixelImage = currGfx->getFramePixelImage(i);
            int numPixels = D1GfxPixel::countAffectedPixels(pixelImage, colors);
            if (numPixels != 0) {
                dProgress() << QApplication::tr("Frame %1 of %2 has pixels in a range which is level-dependent in the game.").arg(i + 1).arg(this->getGfxLabel(gn));
                result = true;
            }
        }
        // test whether the graphics have the right number of groups
        int numGroups = this->type == D1GFX_SET_TYPE::Missile ? 1 : NUM_DIRS;
        if (this->type == D1GFX_SET_TYPE::Player) {
            if (gn == PGT_BLOCK) {
                if (this->getWeaponType() != D1GFX_SET_WEAPON_TYPE::Unknown && this->getWeaponType() != D1GFX_SET_WEAPON_TYPE::ShieldOnly
                 && this->getWeaponType() != D1GFX_SET_WEAPON_TYPE::SwordShield && this->getWeaponType() != D1GFX_SET_WEAPON_TYPE::BluntShield)
                    numGroups = 0;
            }
            if (gn == PGT_DEATH) {
                if (this->getArmorType() != D1GFX_SET_ARMOR_TYPE::Unknown && this->getWeaponType() != D1GFX_SET_WEAPON_TYPE::Unknown
                 && (this->getArmorType() != D1GFX_SET_ARMOR_TYPE::Light || this->getWeaponType() != D1GFX_SET_WEAPON_TYPE::Unarmed))
                    numGroups = 0;
            }
        }
        int gc = currGfx->getGroupCount();
        if (gc != numGroups) {
            dProgress() << QApplication::tr("%1 has %2 instead of %n groups.", "", numGroups).arg(this->getGfxLabel(gn)).arg(gc);
            result = true;
        }
        // test whether a graphic have the same frame-count in each group
        if (this->type != D1GFX_SET_TYPE::Missile) {
            frameCount = -1;
        }
        for (int n = 0; n < currGfx->getGroupCount(); n++) {
            std::pair<int, int> gfi = currGfx->getGroupFrameIndices(n);
            int fc = gfi.second - gfi.first + 1;
            if (fc != frameCount) {
                if (frameCount < 0) {
                    frameCount = fc;
                } else {
                    dProgress() << QApplication::tr("group %1 of %2 has inconsistent framecount (%3 vs %4).").arg(n + 1).arg(this->getGfxLabel(gn)).arg(fc).arg(frameCount);
                    result = true;
                }
            }
        }
        // test whether a graphic have the same frame-size in each group
        if (this->type != D1GFX_SET_TYPE::Missile) {
            width = -1;
            height = -1;
        }
        for (int i = 0; i < currGfx->getFrameCount(); i++) {
            D1GfxFrame* frame = currGfx->getFrame(i);
            int w = frame->getWidth();
            int h = frame->getHeight();
            if (w != width || h != height) {
                if (width < 0) {
                    width = w;
                    height = h;
                } else {
                    dProgress() << QApplication::tr("Frame %1 in group %2 has inconsistent framesize (%3x%4 vs %5x%6).").arg(i + 1).arg(this->getGfxLabel(gn)).arg(w).arg(h).arg(width).arg(height);
                    result = true;
                }
            }
        }
    }

    switch (this->type) {
    case D1GFX_SET_TYPE::Missile: {
        // - test against game code if possible
        if (this->getGfxCount() >= 1) {
            QString filePath = this->getGfx(0)->getFilePath();
            QString filePathLower = QDir::toNativeSeparators(filePath).toLower();
            for (const MisFileData &mfdata : misfiledata) {
                char pszName[DATA_ARCHIVE_MAX_PATH];
                int n = mfdata.mfAnimFAmt;
                const char* name = mfdata.mfName;
                if (name == NULL)
                    continue;
                if (n == 1) {
                    snprintf(pszName, sizeof(pszName), "Missiles\%s.CL2", name);
                } else {
                    snprintf(pszName, sizeof(pszName), "Missiles\\%s%d.CL2", name, 1);
                }
                QString misGfxName = QDir::toNativeSeparators(QString(pszName)).toLower();
                if (filePathLower.endsWith(misGfxName)) {
                    for (int gn = 0; gn < this->getGfxCount(); gn++) {
                        int frameCount = gn < lengthof(mfdata.mfAnimLen) ? mfdata.mfAnimLen[gn] : 0;
                        int animWidth = mfdata.mfAnimWidth * assetMpl;
                        result |= this->checkGraphics(frameCount, animWidth, gn, gfx);
                    }
                    break;
                }
            }
        }
    } break;
    case D1GFX_SET_TYPE::Monster: {
        // - test against game code if possible
        if (this->getGfxCount() >= (int)MA_STAND + 1) {
            QString filePath = this->getGfx(MA_STAND)->getFilePath();
            QString filePathLower = QDir::toNativeSeparators(filePath).toLower();
            for (const MonFileData &mfdata : monfiledata) {
                char strBuff[DATA_ARCHIVE_MAX_PATH];
                snprintf(strBuff, sizeof(strBuff), mfdata.moGfxFile, animletter[MA_STAND]);
                QString monGfxName = QDir::toNativeSeparators(QString(strBuff)).toLower();
                if (filePathLower.endsWith(monGfxName)) {
                    for (int gn = 0; gn < this->getGfxCount(); gn++) {
                        int frameCount = gn < lengthof(mfdata.moAnimFrames) ? mfdata.moAnimFrames[gn] : 0;
                        int animWidth = mfdata.moWidth * assetMpl;
                        result |= this->checkGraphics(frameCount, animWidth, gn, gfx);
                    }
                    break;
                }
            }
        }
    } break;
    case D1GFX_SET_TYPE::Player: {
        // - test against game code if possible
        D1GFX_SET_CLASS_TYPE classType = this->getClassType();
        D1GFX_SET_ARMOR_TYPE armorType = this->getArmorType();
        D1GFX_SET_WEAPON_TYPE weaponType = this->getWeaponType();
        if (classType != D1GFX_SET_CLASS_TYPE::Unknown && armorType != D1GFX_SET_ARMOR_TYPE::Unknown && weaponType != D1GFX_SET_WEAPON_TYPE::Unknown) {
            int pnum = 0;
            int pc = PC_WARRIOR;
            switch (classType) {
            case D1GFX_SET_CLASS_TYPE::Warrior: pc = PC_WARRIOR;  break;
            case D1GFX_SET_CLASS_TYPE::Rogue:   pc = PC_ROGUE;    break;
            case D1GFX_SET_CLASS_TYPE::Mage:    pc = PC_SORCERER; break;
            case D1GFX_SET_CLASS_TYPE::Monk:    pc = PC_MONK;     break;
            }
            int plrgfx = 0;
            switch (weaponType) {
            case D1GFX_SET_WEAPON_TYPE::Unarmed:     plrgfx = ANIM_ID_UNARMED;     break;
            case D1GFX_SET_WEAPON_TYPE::ShieldOnly:  plrgfx = ANIM_ID_UNARMED + 1; break;
            case D1GFX_SET_WEAPON_TYPE::Sword:       plrgfx = ANIM_ID_SWORD;       break;
            case D1GFX_SET_WEAPON_TYPE::SwordShield: plrgfx = ANIM_ID_SWORD + 1;   break;
            case D1GFX_SET_WEAPON_TYPE::Bow:         plrgfx = ANIM_ID_BOW;         break;
            case D1GFX_SET_WEAPON_TYPE::Axe:         plrgfx = ANIM_ID_AXE;         break;
            case D1GFX_SET_WEAPON_TYPE::Blunt:       plrgfx = ANIM_ID_MACE;        break;
            case D1GFX_SET_WEAPON_TYPE::BluntShield: plrgfx = ANIM_ID_MACE + 1;    break;
            case D1GFX_SET_WEAPON_TYPE::Staff:       plrgfx = ANIM_ID_STAFF;       break;
            }
            switch (armorType) {
            case D1GFX_SET_ARMOR_TYPE::Light:  plrgfx |= 0;                    break;
            case D1GFX_SET_ARMOR_TYPE::Medium: plrgfx |= ANIM_ID_MEDIUM_ARMOR; break;
            case D1GFX_SET_ARMOR_TYPE::Heavy:  plrgfx |= ANIM_ID_HEAVY_ARMOR;  break;
            }
            plr._pClass = pc;
            plr._pgfxnum = plrgfx;

            currLvl._dType = DTYPE_TOWN;
            SetPlrAnims(0);
            // assert(this->getGfxCount() == NUM_PGTS);
            for (int n = 0; n < NUM_PGXS; n++) {
                int gn = 0;
                switch (n) {
                case PGX_STAND:     gn = PGT_STAND_TOWN; break;
                case PGX_WALK:      gn = PGT_WALK_TOWN;  break;
                case PGX_ATTACK:    gn = PGT_ATTACK;     break;
                case PGX_FIRE:      gn = PGT_FIRE;       break;
                case PGX_LIGHTNING: gn = PGT_LIGHTNING;  break;
                case PGX_MAGIC:     gn = PGT_MAGIC;      break;
                case PGX_BLOCK:     gn = PGT_BLOCK;      break;
                case PGX_GOTHIT:    gn = PGT_GOTHIT;     break;
                case PGX_DEATH:     gn = PGT_DEATH;      break;
                }

                result |= this->checkGraphics(plr._pAnims[n].paFrames, plr._pAnims[n].paAnimWidth * assetMpl, gn, gfx);
            }

            currLvl._dType = DTYPE_CATHEDRAL;
            SetPlrAnims(0);

            for (int n = 0; n < NUM_PGXS; n++) {
                int gn = 0;
                switch (n) {
                case PGX_STAND: gn = PGT_STAND_DUNGEON; break;
                case PGX_WALK:  gn = PGT_WALK_DUNGEON;  break;
                case PGX_ATTACK:
                case PGX_FIRE:
                case PGX_LIGHTNING:
                case PGX_MAGIC:
                case PGX_BLOCK:
                case PGX_GOTHIT:
                case PGX_DEATH: continue;
                }

                result |= this->checkGraphics(plr._pAnims[n].paFrames, plr._pAnims[n].paAnimWidth * assetMpl, gn, gfx);
            }
        }
    } break;
    }
    return result;
}

void D1Gfxset::mask()
{
    int width, height, w, h;
    int groupSize, gs;
    bool first = true;
    for (D1Gfx *gfx : this->gfxList) {
        if (gfx->getFrameCount() == 0) continue;
        if (!gfx->isFrameSizeConstant()) {
            dProgressErr() << QApplication::tr("Framesize is not constant");
            return;
        }
        if (!gfx->isGroupSizeConstant()) {
            dProgressErr() << QApplication::tr("Groupsize is not constant");
            return;
        }
        w = gfx->getFrameWidth(0);
        h = gfx->getFrameHeight(0);
        gs = gfx->getGroupFrameIndices(0).second - gfx->getGroupFrameIndices(0).first + 1;
        if (!first) {
            if (w != width || h != height) {
                dProgressErr() << QApplication::tr("Framesize is not constant");
                return;
            }
            if (gs != groupSize) {
                dProgressErr() << QApplication::tr("Groupsize is not constant");
                return;
            }
        } else {
            width = w;
            height = h;
            groupSize = gs;
            first = false;
        }
    }
    for (D1Gfx *gfx : this->gfxList) {
        gfx->mask();
    }
    if (this->gfxList.count() <= 1)
        return;
    D1Gfx *gfxA = NULL;
    for (D1Gfx *gfxB : this->gfxList) {
        if (gfxB->getFrameCount() == 0) continue;
        if (gfxA != NULL) {
            if (gs == 1) {
                D1GfxFrame *frameA = gfxA->getFrame(0);
                D1GfxFrame *frameB = gfxB->getFrame(0);
                if (frameA->mask(frameB)) {
                    gfxA->setModified();
                }
            } else {
                for (int n = 0; n < gs; n++) {
                    D1GfxFrame *frameA = gfxA->getFrame(gfxA->getGroupFrameIndices(n).first);
                    D1GfxFrame *frameB = gfxB->getFrame(gfxB->getGroupFrameIndices(n).first);
                    if (frameA->mask(frameB)) {
                        gfxA->setModified();
                    }
                }
            }
        } else {
            gfxA = gfxB;
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
    return this->gfxList.count();
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

QList<D1Gfx *> &D1Gfxset::getGfxList() const
{
    return const_cast<QList<D1Gfx *> &>(this->gfxList);
}

QString D1Gfxset::getGfxLabel(int index) const
{
    switch (this->type) {
    case D1GFX_SET_TYPE::Unknown:
        return QApplication::tr("Unknown%1").arg(index + 1);
    case D1GFX_SET_TYPE::Missile:
        return QApplication::tr("Dir%1").arg(index + 1);
    case D1GFX_SET_TYPE::Monster:
        switch (index) {
        case MA_STAND:   return QApplication::tr("Stand");
        case MA_ATTACK:  return QApplication::tr("Attack");
        case MA_WALK:    return QApplication::tr("Walk");
        case MA_SPECIAL: return QApplication::tr("Spec");
        case MA_GOTHIT:  return QApplication::tr("Hit");
        case MA_DEATH:   return QApplication::tr("Death");
        }
        break;
    case D1GFX_SET_TYPE::Player:
        switch (index) {
        case PGT_STAND_TOWN:    return QApplication::tr("Stand (town)");
        case PGT_STAND_DUNGEON: return QApplication::tr("Stand (dungeon)");
        case PGT_WALK_TOWN:     return QApplication::tr("Walk (town)");
        case PGT_WALK_DUNGEON:  return QApplication::tr("Walk (dungeon)");
        case PGT_ATTACK:        return QApplication::tr("Attack");
        case PGT_FIRE:          return QApplication::tr("Fire");
        case PGT_LIGHTNING:     return QApplication::tr("Light");
        case PGT_MAGIC:         return QApplication::tr("Magic");
        case PGT_BLOCK:         return QApplication::tr("Block");
        case PGT_GOTHIT:        return QApplication::tr("Hit");
        case PGT_DEATH:         return QApplication::tr("Death");
        }
        break;
    }

    return QApplication::tr("N/A");
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
