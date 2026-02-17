#include "d1gfxset.h"

#include <QApplication>
#include <QMessageBox>

#include "d1cl2.h"
#include "d1clc.h"
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

bool D1Gfxset::load(const QString &gfxFilePath, const OpenAsParam &params, bool optional)
{
    const QFileInfo gfxFileInfo = QFileInfo(gfxFilePath);
    QString extension = gfxFileInfo.suffix();
    if (extension.compare("cl2", Qt::CaseInsensitive) == 0) {
        if (!D1Cl2::load(*this->baseGfx, gfxFilePath, params)) {
            if (!optional) {
                dProgressErr() << QApplication::tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(gfxFilePath));
            }
            return false;
        }
    } else {
        if (optional) {
            return false;
        }
        if (!D1Clc::load(*this->baseGfx, gfxFilePath, params)) {
            dProgressErr() << QApplication::tr("Failed loading CLC file: %1.").arg(QDir::toNativeSeparators(gfxFilePath));
            return false;
        }
        // assume(extension.toLower() == "clc");
        extension.chop(1);
        extension.push_back('2');
    }
    const QDir folder = gfxFileInfo.dir();
    QString baseName = gfxFileInfo.completeBaseName();
    std::vector<QString> fileNames;
    D1GFX_SET_TYPE type = D1GFX_SET_TYPE::Unknown;
    D1GFX_SET_CLASS_TYPE ctype = D1GFX_SET_CLASS_TYPE::Unknown;
    D1GFX_SET_ARMOR_TYPE atype = D1GFX_SET_ARMOR_TYPE::Unknown;
    D1GFX_SET_WEAPON_TYPE wtype = D1GFX_SET_WEAPON_TYPE::Unknown;
    if (!baseName.isEmpty() && baseName[baseName.length() - 1].isDigit()) {
        // ends with a number -> missile animation
        QChar lastDigit = baseName[baseName.length() - 1];
        int cn = lastDigit.digitValue();
        baseName.chop(1);
        if (cn <= 6 && !baseName.isEmpty() && baseName.endsWith('1')) {
            baseName.chop(1);
            cn += 10;
        }
        int mn = cn;
        int fileMatchesMis = 0;
        {
            const QStringList files = folder.entryList();
            for (int i = 1; i <= 16; i++) {
                QString misName = baseName + QString::number(i);
                for (const QString fileName : files) {
                    if (!fileName.endsWith(".cl2", Qt::CaseInsensitive) && !fileName.endsWith(".clc", Qt::CaseInsensitive))
                        continue;
                    QString fileBase = fileName;
                    fileBase.chop(4);
                    if (misName.compare(fileBase, Qt::CaseInsensitive) == 0) {
                        mn = i;
                        fileMatchesMis++;
                        break;
                    }
                }
            }
        }
        if (optional && (fileMatchesMis != 8 && fileMatchesMis != 16)) {
            return false;
        }
        int n = mn > 8 ? 16 : 8;
        for (int i = 1; i <= n; i++) {
            QString fileName = baseName + QString::number(i);
            fileNames.push_back(fileName);
        }
        type = D1GFX_SET_TYPE::Missile;
    } else {
        int fileMatchesPlr = 0, fileInMatchesPlr = 0;
        if (baseName.length() == 5) {
            QString basePlrName = baseName.mid(0, 3);
            const QStringList files = folder.entryList();
            for (int i = 0; i < lengthof(PlrAnimTypes); i++) {
                QString plrName = basePlrName + PlrAnimTypes[i].patTxt[0] + PlrAnimTypes[i].patTxt[1];
                for (const QString fileName : files) {
                    if (!fileName.endsWith(".cl2", Qt::CaseInsensitive) && !fileName.endsWith(".clc", Qt::CaseInsensitive))
                        continue;
                    QString fileBase = fileName;
                    fileBase.chop(4);
                    if (plrName.compare(fileBase, Qt::CaseInsensitive) == 0) {
                        fileInMatchesPlr++;
                        if (plrName.compare(fileBase, Qt::CaseSensitive) == 0) {
                            fileMatchesPlr++;
                        }
                        break;
                    }
                }
            }
        }
        int fileMatchesMon = 0, fileInMatchesMon = 0;
        if (baseName.length() > 1) {
            QString baseMonName = baseName;
            baseMonName.chop(1);
            const QStringList files = folder.entryList();
            for (int i = 0; i < lengthof(animletter); i++) {
                QString monName = baseMonName + animletter[i];
                for (const QString fileName : files) {
                    if (!fileName.endsWith(".cl2", Qt::CaseInsensitive) && !fileName.endsWith(".clc", Qt::CaseInsensitive))
                        continue;
                    QString fileBase = fileName;
                    fileBase.chop(4);
                    if (monName.compare(fileBase, Qt::CaseInsensitive) == 0) {
                        fileInMatchesMon++;
                        if (monName.compare(fileBase, Qt::CaseSensitive) == 0) {
                            fileMatchesMon++;
                        }
                        break;
                    }
                }
            }
        }
        if (fileInMatchesMon == fileInMatchesPlr) {
            fileInMatchesPlr = fileMatchesPlr;
            fileInMatchesMon = fileMatchesMon;
        }
        if (optional && std::max(fileInMatchesMon, fileInMatchesPlr) <= 3) {
            return false;
        }
        if (fileInMatchesMon >= fileInMatchesPlr) {
            type = D1GFX_SET_TYPE::Monster;
            QString baseMonName = baseName;
            baseMonName.chop(1);
            for (int i = 0; i < lengthof(animletter); i++) {
                QString fileName = baseMonName + animletter[i];
                fileNames.push_back(fileName);
            }
        } else {
            type = D1GFX_SET_TYPE::Player;
            QString basePlrName = baseName.mid(0, 3);
            for (int i = 0; i < lengthof(PlrAnimTypes); i++) {
                QString fileName = basePlrName + PlrAnimTypes[i].patTxt[0] + PlrAnimTypes[i].patTxt[1];
                fileNames.push_back(fileName);
            }
            for (int i = 0; i < lengthof(CharChar); i++) {
                if (basePlrName[0].toUpper() == CharChar[i]) {
                    ctype = (D1GFX_SET_CLASS_TYPE)i;
                }
            }
            for (int i = 0; i < lengthof(ArmorChar); i++) {
                if (basePlrName[1].toUpper() == ArmorChar[i]) {
                    atype = (D1GFX_SET_ARMOR_TYPE)i;
                }
            }
            for (int i = 0; i < lengthof(WepChar); i++) {
                if (basePlrName[2].toUpper() == WepChar[i]) {
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
    D1Pal *pal = this->baseGfx->getPalette();
    bool baseMatch = false;
    const QStringList files = folder.entryList();
    for (const QString &baseName : fileNames) {
        D1Gfx *gfx;
        if (gfxFileInfo.completeBaseName().compare(baseName, Qt::CaseInsensitive) != 0) {
            QString filePath;
            for (const QString fileName : files) {
                if (!fileName.endsWith(".cl2", Qt::CaseInsensitive) && !fileName.endsWith(".clc", Qt::CaseInsensitive))
                    continue;
                QString fileBase = fileName;
                fileBase.chop(4);
                if (baseName.compare(fileBase, Qt::CaseInsensitive) == 0) {
                    filePath = fileName;
                    break;
                }
            }
            if (filePath.isEmpty()) {
                filePath = baseName + "." + extension;
            }
            QFileInfo qfi = QFileInfo(folder, filePath);
            filePath = qfi.filePath();

            gfx = new D1Gfx();
            gfx->setPalette(pal);
            bool loaded;
            if (filePath.endsWith(".cl2", Qt::CaseInsensitive)) {
                loaded = D1Cl2::load(*gfx, filePath, params);
                if (!loaded) {
                    dProgressErr() << QApplication::tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(filePath));
                }
            } else {
                loaded = D1Clc::load(*gfx, filePath, params);
                if (!loaded) {
                    dProgressErr() << QApplication::tr("Failed loading CLC file: %1.").arg(QDir::toNativeSeparators(filePath));
                }
            }
            if (!loaded || this->baseGfx->getType() != gfx->getType()) {
                if (loaded) {
                    dProgressErr() << QApplication::tr("Mismatching type in %1 (%2 vs %3)").arg(QDir::toNativeSeparators(filePath)).arg(D1Gfx::gfxTypeToStr(this->baseGfx->getType())).arg(D1Gfx::gfxTypeToStr(gfx->getType()));
                }
                gfx->setType(this->baseGfx->getType());
                filePath.chop(1);
                filePath.push_back('2');
                gfx->setFilePath(filePath);
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

void D1Gfxset::save(const SaveAsParam &params)
{
    SaveAsParam saveParams = params;
    QString filePath = saveParams.celFilePath;
    QString extension = QString(".CL2");
    if (!filePath.isEmpty()) {
        const QFileInfo gfxFileInfo = QFileInfo(filePath);

        extension = QString(".") + gfxFileInfo.suffix();
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
            // save the components and the meta-info
            if (gfx->getComponentCount() != 0 || !gfx->getCompFilePath().isEmpty()) {
                gfx->saveComponents();
                D1Clc::save(*gfx, saveParams);
            }
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

static QString getGfxName(const D1Gfx* gfx)
{
    QString path = gfx->getFilePath();
    QFileInfo qfi = QFileInfo(path);
    return qfi.completeBaseName();
}

bool D1Gfxset::isClippedConstant() const
{
    int clipped = -1;
    for (const D1Gfx* gfx : this->gfxList) {
        int gc = gfx->isClipped() ? 1 : 0;
        if (clipped >= 0) {
            if (gc != clipped) {
                return false;
            }
        } else {
            clipped = gc;
        }
    }
    return true;
}

bool D1Gfxset::isGroupsConstant() const
{
    int numGroups = -1;
    for (const D1Gfx* gfx : this->gfxList) {
        int gc = gfx->getGroupCount();
        if (numGroups >= 0) {
            if (gc != numGroups) {
                return false;
            }
        } else {
            numGroups = gc;
        }
    }
    return true;
}

void D1Gfxset::compareTo(const LoadFileContent *fileContent, bool patchData) const
{
    if (fileContent->gfxset != nullptr) {
        if (this->gfxList.count() == fileContent->gfxset->gfxList.count()) {
            // compare N to N
            for (int i = 0; i < this->gfxList.count(); i++) {
                const D1Gfx *gfxA = this->gfxList[i];
                const D1Gfx *gfxB = fileContent->gfxset->gfxList[i];
                QString nameA = getGfxName(gfxA);
                QString nameB = getGfxName(gfxB);
                QString header;
                // treat empty/modified graphics as missing/deleted
                if (gfxB->getFrameCount() == 0 && gfxB->isModified()) {
                    if (gfxA->getFrameCount() == 0 && gfxA->isModified()) {
                        continue;
                    }
                    dProgress() << QApplication::tr("Added gfx %1.").arg(nameA);
                    continue;
                } else if (gfxA->getFrameCount() == 0 && gfxA->isModified()) {
                    dProgress() << QApplication::tr("Deleted gfx %1.").arg(nameB);
                    continue;
                }
                // compare non-empty graphics
                if (nameA == nameB) {
                    header = QApplication::tr("Gfx %1 (%2):").arg(nameA).arg(i + 1);
                } else {
                    header = QApplication::tr("Gfx %1 vs. %2 (%3):").arg(nameA).arg(nameB).arg(i + 1);
                }
                this->gfxList[i]->compareTo(fileContent->gfxset->gfxList[i], header, patchData);
                if (header.isEmpty())
                    dProgress() << "\n";
            }
        } else {
            // compare N1 to N2
            QSet<const D1Gfx *> checked;
            for (int i = 0; i < this->gfxList.count(); i++) {
                const D1Gfx *gfxA = this->gfxList[i];
                QString nameA = getGfxName(gfxA);
                int n = 0;
                for ( ; n < fileContent->gfxset->gfxList.count(); n++) {
                    const D1Gfx *gfxB = fileContent->gfxset->gfxList[n];
                    if (checked.contains(gfxB))
                        continue;
                    QString nameB = getGfxName(gfxB);
                    if (nameB.toLower() == nameA.toLower()) {
                        QString header;
                        checked.insert(gfxB);
                        // treat empty/modified graphics as missing/deleted
                        if (gfxB->getFrameCount() == 0 && gfxB->isModified()) {
                            if (gfxA->getFrameCount() == 0 && gfxA->isModified()) {
                                break;
                            }
                            dProgress() << QApplication::tr("Added gfx %1.").arg(nameA);
                            break;
                        } else if (gfxA->getFrameCount() == 0 && gfxA->isModified()) {
                            dProgress() << QApplication::tr("Deleted gfx %1.").arg(nameB);
                            break;
                        }
                        // compare non-empty graphics
                        if (nameA == nameB) {
                            if (i == n) {
                                header = QApplication::tr("Gfx %1 (%2 vs %3):").arg(nameA).arg(i + 1).arg(n + 1);
                            } else {
                                header = QApplication::tr("Gfx %1 (%2):").arg(nameA).arg(i + 1);
                            }
                        } else {
                            if (i == n) {
                                header = QApplication::tr("Gfx %1 vs %2 (%3):").arg(nameA).arg(nameB).arg(i + 1);
                            } else {
                                header = QApplication::tr("Gfx %1 (%2) vs %3 (%4):").arg(nameA).arg(i + 1).arg(nameB).arg(n + 1);
                            }
                        }
                        gfxA->compareTo(gfxB, header, patchData);
                        if (header.isEmpty())
                            dProgress() << "\n";
                        break;
                    }
                }
                if (n >= fileContent->gfxset->gfxList.count()) {
                    dProgress() << QApplication::tr("Added gfx %1.").arg(nameA);
                }
            }

            for (int n = 0; n < fileContent->gfxset->gfxList.count(); n++) {
                D1Gfx *gfxB = fileContent->gfxset->gfxList[n];
                if (checked.contains(gfxB))
                    continue;

                QString nameB = getGfxName(gfxB);
                dProgress() << QApplication::tr("Deleted gfx %1.").arg(nameB);
            }
        }
    } else if (fileContent->gfx != nullptr) {
        // compare N to 1
        const D1Gfx *gfxB = fileContent->gfx;
        for (int i = 0; i < this->gfxList.count(); i++) {
            const D1Gfx *gfxA = this->gfxList[i];
            QString nameA = getGfxName(gfxA);
            QString header = QApplication::tr("Gfx %1 (%2):").arg(nameA).arg(i + 1);
            gfxA->compareTo(gfxB, header, patchData);
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

bool D1Gfxset::check(const D1Gfx *gfx, int assetMpl) const
{
    bool result = false;
    bool typetested = false;
    int frameCount = -1, width = -1, height = -1;
    for (int gn = 0; gn < this->getGfxCount(); gn++) {
        D1Gfx *currGfx = this->getGfx(gn);
        if (gfx != nullptr && gfx != currGfx)
            continue;
        // test the graphics
        result |= currGfx->check(assetMpl, &typetested);
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
        if (this->type == D1GFX_SET_TYPE::Monster && gc == 0) {
            ; // accept empty/missing monster-graphics (might be optional or non-extant like the golem standing animation)
        } else if (gc != numGroups) {
            dProgress() << QApplication::tr("%1 has %2 instead of %n groups.", "", numGroups).arg(this->getGfxLabel(gn)).arg(gc);
            result = true;
        }
        // test whether a graphic have the same frame-count in each group
        int fc = currGfx->getGroupSize();
        if (fc < 0) {
            // dProgress() << QApplication::tr("Groupsize of gfx %1 is not constant.").arg(this->getGfxLabel(gn));
            // result = true;
        } else if (this->type == D1GFX_SET_TYPE::Missile) {
            if (frameCount != fc) {
                if (frameCount < 0) {
                    frameCount = fc;
                } else {
                    dProgress() << QApplication::tr("Gfx %1 has inconsistent groupsize (%2 vs %3).").arg(this->getGfxLabel(gn)).arg(fc).arg(frameCount);
                    result = true;
                }
            }
        }
        // test whether a graphic have the same frame-size in each group
        QSize fs = currGfx->getFrameSize();
        if (!fs.isValid()) {
            // dProgress() << QApplication::tr("Framesize of gfx %1 is not constant.").arg(this->getGfxLabel(gn));
            // result = true;
        } else if (this->type == D1GFX_SET_TYPE::Missile) {
            int w = fs.width();
            int h = fs.height();
            if (w != width || h != height) {
                if (width < 0) {
                    width = w;
                    height = h;
                } else {
                    dProgress() << QApplication::tr("Gfx %1 has inconsistent framesize (%2x%3 vs %4x%5).").arg(this->getGfxLabel(gn)).arg(w).arg(h).arg(width).arg(height);
                    result = true;
                }
            }
        }
    }
    switch (this->type) {
    case D1GFX_SET_TYPE::Missile:
        break;
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
                        D1Gfx* currGfx = this->getGfx(gn);
                        if (gfx != nullptr && gfx != currGfx)
                            continue;

                        const int gc = currGfx->getGroupCount();
                        if (gc != NUM_DIRS) {
                            dProgress() << QApplication::tr("Groupcount of %1 is not handled by the game (expected %2 got %3).").arg(this->getGfxLabel(gn)).arg(NUM_DIRS).arg(gc);
                            result = true;
                        }
                        const int fc = currGfx->getGroupSize();
                        if (gn == MA_STAND && fc > 0x7FFF) {
                            dProgress() << QApplication::tr("Framecount of %1 is not handled by the game (InitMonster expects < %2 got %3).").arg(this->getGfxLabel(gn)).arg(0x7FFF).arg(fc);
                            result = true;
                        }
                        if (gn == MA_WALK && fc > 24) { // lengthof(MWVel)
                            dProgress() << QApplication::tr("Framecount of %1 is not handled by the game (MonWalkDir expects <= %2 got %3).").arg(this->getGfxLabel(gn)).arg(24).arg(fc);
                            result = true;
                        }
                        if (&mfdata == &monfiledata[MOFILE_SNAKE] && gn == MA_ATTACK && fc != 13) {
                            dProgress() << QApplication::tr("Framecount of %1 is not handled by the game (MI_Rhino expects %2 got %3).").arg(this->getGfxLabel(gn)).arg(13).arg(fc);
                            result = true;
                        }
                        if (gn == MA_SPECIAL && (mfdata.moAnimFrameLen[gn] == 0) != (fc == 0)) {
                            if (fc == 0)
                                dProgress() << QApplication::tr("Missing/empty special animation (%1).").arg(this->getGfxLabel(gn));
                            else
                                dProgress() << QApplication::tr("Special animation (%1) is not used.").arg(this->getGfxLabel(gn));
                            result = true;
                        }
                        if ((gn == MA_WALK || gn == MA_ATTACK || gn == MA_SPECIAL) && fc * mfdata.moAnimFrameLen[gn] >= SQUELCH_LOW) {
                            dProgress() << QApplication::tr("Animation (%1) too long to finish before relax (expected < %2 got %3).").arg(this->getGfxLabel(gn)).arg(SQUELCH_LOW).arg(fc * mfdata.moAnimFrameLen[gn]);
                            result = true;
                        }
                        for (int m = 0; m < NUM_CELMETA; m++) {
                            if (currGfx->getMeta(m)->isStored()) {
                                dProgress() << QApplication::tr("Meta %1 of the monster animation %2 is not used by the game.").arg(D1GfxMeta::metaTypeToStr(m)).arg(this->getGfxLabel(gn));
                            }
                        }
                    }
                    typetested = true;
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

            SetPlrAnims(0);

            for (int gn = 0; gn < this->getGfxCount(); gn++) {
                D1Gfx* currGfx = this->getGfx(gn);
                if (gfx != nullptr && gfx != currGfx)
                    continue;
                const int gc = currGfx->getGroupCount();
                if (gc != NUM_DIRS) {
                    dProgress() << QApplication::tr("Groupcount of %1 is not handled by the game (expected %2 got %3).").arg(this->getGfxLabel(gn)).arg(NUM_DIRS).arg(gc);
                    result = true;
                }
                const unsigned fc = currGfx->getGroupSize();
                if ((gn == PGT_WALK_TOWN || gn == PGT_WALK_DUNGEON) && fc != PLR_WALK_ANIMLEN) {
                    dProgress() << QApplication::tr("Framecount of %1 is not handled by the game (StartWalk expects %2 got %3).").arg(this->getGfxLabel(gn)).arg(PLR_WALK_ANIMLEN).arg(fc);
                    result = true;
                }
                if ((gn == PGT_FIRE || gn == PGT_MAGIC || gn == PGT_LIGHTNING) && plr._pSFNum > fc) {
                    dProgress() << QApplication::tr("Framecount of %1 is not handled by the game (PlrDoSpell expects >= %2 got %3).").arg(this->getGfxLabel(gn)).arg(plr._pSFNum).arg(fc);
                    result = true;
                }
                if (gn == PGT_ATTACK && plr._pAFNum > fc) {
                    dProgress() << QApplication::tr("Framecount of %1 is not handled by the game (PlrDoSpell expects >= %2 got %3).").arg(this->getGfxLabel(gn)).arg(plr._pAFNum).arg(fc);
                    result = true;
                }
                for (int m = 0; m < NUM_CELMETA; m++) {
                    if (currGfx->getMeta(m)->isStored()) {
                        dProgress() << QApplication::tr("Meta %1 of the player animation %2 is not used by the game.").arg(D1GfxMeta::metaTypeToStr(m)).arg(this->getGfxLabel(gn));
                        result = true;
                    }
                }
            }

            typetested = true;
        }
    } break;
    }
    if (!typetested) {
        dProgress() << QApplication::tr("Unrecognized graphics (type: %1, base-path:%2) -> Checking with game-code is skipped.").arg(this->getGfxLabel(-1)).arg(this->getBaseGfx()->getFilePath());
        result = true;
    }
    return result;
}

void D1Gfxset::mask(int frameIndex, bool subtract)
{
    int width, height, w, h;
    int groupSize, gs;
    int groupCount, gc;
    bool first = true;
    for (D1Gfx *gfx : this->gfxList) {
        if (gfx->getFrameCount() == 0) continue;
        QSize fs = gfx->getFrameSize();
        if (!fs.isValid()) {
            dProgressErr() << QApplication::tr("Framesize is not constant");
            return;
        }
        gs = gfx->getGroupSize();
        if (gs < 0) {
            dProgressErr() << QApplication::tr("Groupsize is not constant");
            return;
        }
        gc = gfx->getGroupCount();
        w = fs.width();
        h = fs.height();
        if (!first) {
            if (w != width || h != height) {
                dProgressErr() << QApplication::tr("Framesize is not constant");
                return;
            }
            if (gs != groupSize) {
                dProgressErr() << QApplication::tr("Groupsize is not constant");
                return;
            }
            if (gc != groupCount) {
                dProgressErr() << QApplication::tr("Groupcount is not constant");
                return;
            }
        } else {
            width = w;
            height = h;
            groupSize = gs;
            groupCount = gc;
            first = false;
        }
    }
    for (D1Gfx *gfx : this->gfxList) {
        gfx->mask(frameIndex, subtract);
    }
    D1Gfx *gfxA = this->baseGfx;
    if (this->gfxList.indexOf(gfxA) < 0)
        return;
    for (D1Gfx *gfxB : this->gfxList) {
        if (gfxB->getFrameCount() == 0) continue;
        if (gfxA == gfxB) continue;
        if (gfxA->getGroupCount() == 1) {
            D1GfxFrame *frameA = gfxA->getFrame(frameIndex);
            D1GfxFrame *frameB = gfxB->getFrame(frameIndex);
            if (subtract) {
                if (frameB->subtract(frameA)) {
                    gfxB->setModified();
                }
            } else {
                if (frameA->mask(frameB)) {
                    gfxA->setModified();
                }
            }
        } else {
            const int gi = frameIndex / gs;
            for (int n = 0; n < gs; n++) {
                D1GfxFrame *frameA = gfxA->getFrame(gfxA->getGroupFrameIndices(gi).first + n);
                D1GfxFrame *frameB = gfxB->getFrame(gfxB->getGroupFrameIndices(gi).first + n);
                if (subtract) {
                    if (frameB->subtract(frameA)) {
                        gfxB->setModified();
                    }
                } else {
                    if (frameA->mask(frameB)) {
                        gfxA->setModified();
                    }
                }
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
    QString res = QString("\"%1\" [%2]");
    switch (this->type) {
    case D1GFX_SET_TYPE::Unknown:
        res = QApplication::tr("Unknown%1").arg(index + 1);
        break;
    case D1GFX_SET_TYPE::Missile:
        res = QApplication::tr("Dir%1").arg(index + 1);
        break;
    case D1GFX_SET_TYPE::Monster:
        if ((unsigned)index >= NUM_MON_ANIM) {
            res = QApplication::tr("Monster");
            break;
        }
        res = res.arg(QChar(animletter[index]).toUpper());
        switch (index) {
        case MA_STAND:   res = res.arg(QApplication::tr("Stand"));
        case MA_ATTACK:  res = res.arg(QApplication::tr("Attack"));
        case MA_WALK:    res = res.arg(QApplication::tr("Walk"));
        case MA_SPECIAL: res = res.arg(QApplication::tr("Spec"));
        case MA_GOTHIT:  res = res.arg(QApplication::tr("Hit"));
        case MA_DEATH:   res = res.arg(QApplication::tr("Death"));
        }
        break;
    case D1GFX_SET_TYPE::Player:
        if ((unsigned)index >= NUM_PGTS) {
            res = QApplication::tr("Player");
            break;
        }
        res = res.arg(PlrAnimTypes[index].patTxt);
        switch (index) {
        case PGT_STAND_TOWN:    res = res.arg(QApplication::tr("Stand (town)"));
        case PGT_STAND_DUNGEON: res = res.arg(QApplication::tr("Stand (dungeon)"));
        case PGT_WALK_TOWN:     res = res.arg(QApplication::tr("Walk (town)"));
        case PGT_WALK_DUNGEON:  res = res.arg(QApplication::tr("Walk (dungeon)"));
        case PGT_ATTACK:        res = res.arg(QApplication::tr("Attack"));
        case PGT_FIRE:          res = res.arg(QApplication::tr("Fire"));
        case PGT_LIGHTNING:     res = res.arg(QApplication::tr("Light"));
        case PGT_MAGIC:         res = res.arg(QApplication::tr("Magic"));
        case PGT_BLOCK:         res = res.arg(QApplication::tr("Block"));
        case PGT_GOTHIT:        res = res.arg(QApplication::tr("Hit"));
        case PGT_DEATH:         res = res.arg(QApplication::tr("Death"));
        }
        break;
    }
    return res;
}

void D1Gfxset::frameModified(const D1GfxFrame *frame)
{
    for (D1Gfx *gfx : this->gfxList) {
        gfx->frameModified(frame);
    }
}

void D1Gfxset::setPalette(D1Pal *pal)
{
    for (D1Gfx *gfx : this->gfxList) {
        gfx->setPalette(pal);
    }
}
