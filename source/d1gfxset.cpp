#include "d1gfxset.h"

#include <QApplication>
#include <QMessageBox>

#include "d1cl2.h"
#include "progressdialog.h"

#include "dungeon/all.h"

static const PlrAnimType PlrAnimTypes[NUM_PGTS] = {
    // clang-format off
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
    // clang-format on
};

static const char animletter[NUM_MON_ANIM] = { 'n', 'w', 'a', 'h', 'd', 's' };

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

        const QString extension = gfxFilePath.endsWith(".CL2") ? ".CL2" : ".cl2";
        QString baseName = celFileInfo.completeBaseName();
        QString basePath = celFileInfo.absolutePath() + "/";
        std::vector<QString> filePaths;
        D1GFX_SET_TYPE type;
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
                    QString filePath = baseMonPath + animletter[i] + extension;
                    filePaths.push_back(filePath);
                }
            } else {
                type = D1GFX_SET_TYPE::Player;
                QString basePlrPath = basePath + baseName.mid(0, 3);
                for (int i = 0; i < lengthof(PlrAnimTypes); i++) {
                    QString filePath = basePlrPath + PlrAnimTypes[i].patTxt[0] + PlrAnimTypes[i].patTxt[1] + extension;
                    filePaths.push_back(filePath);
                }
            }
        }
        this->type = type;

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
    for (D1Gfx *gfx : this->gfxList) {
        D1Cl2::save(*gfx, params);
    }
}

int D1Gfxset::getGfxCount() const
{
    return this->gfxList.size();
}

D1Gfx *D1Gfxset::getGfx(int index) const
{
    return this->gfxList[index];
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