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

bool D1Gfxset::load(const QString &gfxFilePath, const OpenAsParam &params)
{
    const QFileInfo gfxFileInfo = QFileInfo(gfxFilePath);
    QString extension = gfxFileInfo.suffix();
    if (extension.compare("cl2", Qt::CaseInsensitive) == 0) {
        if (!D1Cl2::load(*this->baseGfx, gfxFilePath, params)) {
            dProgressErr() << QApplication::tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(gfxFilePath));
            return false;
        }
    } else {
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
        if (mn <= 8) {
            const QStringList files = folder.entryList();
            for (int i = 9; i <= 16; i++) {
                QString misName = baseName + QString::number(i);
                for (const QString fileName : files) {
                    if (!fileName.endsWith(".cl2", Qt::CaseInsensitive) && !fileName.endsWith(".clc", Qt::CaseInsensitive))
                        continue;
                    QString fileBase = fileName;
                    fileBase.chop(4);
                    if (misName.compare(fileBase, Qt::CaseInsensitive) == 0) {
                        mn = i;
                        break;
                    }
                }
            }
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
    std::vector<D1Gfx *> gfxs;
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
            bool loaded = false;
            if (filePath.endsWith(".cl2", Qt::CaseInsensitive)) {
                loaded = D1Cl2::load(*gfx, filePath, params);
            } else {
                loaded = D1Clc::load(*gfx, filePath, params);
            }
            if (!loaded || this->baseGfx->getType() != gfx->getType()) {
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
            dProgress() << QApplication::tr("Framecount of group %1 of %2 does not match with the game (%3 vs %4).").arg(i + 1).arg(this->getGfxLabel(gn)).arg(fc).arg(frameCount);
            result = true;
        }
        for (int ii = 0; ii < fc; ii++) {
            int w = currGfx->getFrame(gfi.first + ii)->getWidth();
            if (w != animWidth) {
                dProgress() << QApplication::tr("Framewidth of frame %1 of group %2 in %3 does not match with the game (%4 vs %5).").arg(ii + 1).arg(i + 1).arg(this->getGfxLabel(gn)).arg(w).arg(animWidth);
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
        int fc = currGfx->getGroupSize();
        if (fc < 0) {
            dProgress() << QApplication::tr("Groupsize of gfx %1 is not constant.").arg(this->getGfxLabel(gn));
            result = true;
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
            dProgress() << QApplication::tr("Framesize of gfx %1 is not constant.").arg(this->getGfxLabel(gn));
            result = true;
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
    bool typetested = false;
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
                    snprintf(pszName, sizeof(pszName), "Missiles\\%s.CL2", name);
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
                    typetested = true;
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

            typetested = true;
        }
    } break;
    }
    if (!typetested) {
        dProgress() << QApplication::tr("Unrecognized graphics -> Checking with game-code is skipped.");
        result = true;
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
    QString res = QString("\"%1\" [%2]");
    switch (this->type) {
    case D1GFX_SET_TYPE::Unknown:
        return QApplication::tr("Unknown%1").arg(index + 1);
    case D1GFX_SET_TYPE::Missile:
        return QApplication::tr("Dir%1").arg(index + 1);
    case D1GFX_SET_TYPE::Monster:
        if ((unsigned)index >= NUM_MON_ANIM) break;
        res = res.arg(QChar(animletter[index]).toUpper());
        switch (index) {
        case MA_STAND:   res = res.arg(QApplication::tr("Stand"));
        case MA_ATTACK:  res = res.arg(QApplication::tr("Attack"));
        case MA_WALK:    res = res.arg(QApplication::tr("Walk"));
        case MA_SPECIAL: res = res.arg(QApplication::tr("Spec"));
        case MA_GOTHIT:  res = res.arg(QApplication::tr("Hit"));
        case MA_DEATH:   res = res.arg(QApplication::tr("Death"));
        }
        return res;
    case D1GFX_SET_TYPE::Player:
        if ((unsigned)index >= NUM_PGTS) break;
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
        return res;
    }

    return QApplication::tr("N/A");
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
