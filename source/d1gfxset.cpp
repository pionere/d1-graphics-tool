#include "d1gfxset.h"

#include <QApplication>
#include <QMessageBox>

#include "progressdialog.h"

D1Gfxset::D1Gfxset(D1Gfx *g)
{
    this->gfxList.push_back(g);
}

bool D1Gfxset::load(const QString &gfxFilePath, const OpenAsParam &params)
{
    if (!D1Cl2::load(*this->gfxList[0], gfxFilePath, params)) {
        dProgressErr() << QApplication::tr("Failed loading CL2 file: %1.").arg(QDir::toNativeSeparators(gfxFilePath));
    } else {
        QFileInfo celFileInfo = QFileInfo(gfxFilePath);

        QString basePath = celFileInfo.absolutePath() + "/" + celFileInfo.completeBaseName();


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

void D1Gfxset::getGfxCount() const
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
