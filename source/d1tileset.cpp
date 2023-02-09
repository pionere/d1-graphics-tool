#include "d1tileset.h"

#include <QApplication>

#include "progressdialog.h"

D1Tileset::D1Tileset(D1Gfx *g)
    : gfx(g)
{
    this->min = new D1Min();
    this->til = new D1Til();
    this->sol = new D1Sol();
    this->amp = new D1Amp();
    this->tmi = new D1Tmi();
}

D1Tileset::~D1Tileset()
{
    delete min;
    delete til;
    delete sol;
    delete amp;
    delete tmi;
}

void D1Tileset::save(const SaveAsParam &params)
{
    this->min->save(params);
    this->til->save(params);
    this->sol->save(params);
    this->amp->save(params);
    this->tmi->save(params);
}

void D1Tileset::createSubtile()
{
    this->min->createSubtile();
    this->sol->createSubtile();
    this->tmi->createSubtile();
}

void D1Tileset::removeSubtile(int subtileIndex)
{
    this->min->removeSubtile(subtileIndex);
    this->sol->removeSubtile(subtileIndex);
    this->tmi->removeSubtile(subtileIndex);
    // shift references
    // - shift subtile indices of the tiles
    unsigned refIndex = subtileIndex;
    for (int i = 0; i < this->til->getTileCount(); i++) {
        QList<quint16> &subtileIndices = this->til->getSubtileIndices(i);
        for (int n = 0; n < subtileIndices.count(); n++) {
            if (subtileIndices[n] >= refIndex) {
                // assert(subtileIndices[n] != refIndex);
                subtileIndices[n] -= 1;
                this->til->setModified();
            }
        }
    }
}

bool D1Tileset::reuseFrames(std::set<int> &removedIndices)
{
    ProgressDialog::incBar(QApplication::tr("Reusing frames..."), this->gfx->getFrameCount());
    int result = 0;
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        if (ProgressDialog::wasCanceled()) {
            result |= 2;
            break;
        }
        for (int j = i + 1; j < this->gfx->getFrameCount(); j++) {
            D1GfxFrame *frame0 = this->gfx->getFrame(i);
            D1GfxFrame *frame1 = this->gfx->getFrame(j);
            int width = frame0->getWidth();
            int height = frame0->getHeight();
            if (width != frame1->getWidth() || height != frame1->getHeight()) {
                continue; // should not happen, but better safe than sorry
            }
            bool match = true;
            for (int y = 0; y < height && match; y++) {
                for (int x = 0; x < width; x++) {
                    if (frame0->getPixel(x, y) == frame1->getPixel(x, y)) {
                        continue;
                    }
                    match = false;
                    break;
                }
            }
            if (!match) {
                continue;
            }
            // reuse frame0 instead of frame1
            const unsigned frameRef = j + 1;
            for (int n = 0; n < this->min->getSubtileCount(); n++) {
                QList<quint16> &frameReferences = this->min->getFrameReferences(n);
                for (auto iter = frameReferences.begin(); iter != frameReferences.end(); iter++) {
                    if (*iter == frameRef) {
                        *iter = i + 1;
                        this->min->setModified();
                    }
                }
            }
            // eliminate frame1
            this->gfx->removeFrame(j);
            // calculate the original indices
            int originalIndexI = i;
            for (auto iter = removedIndices.cbegin(); iter != removedIndices.cend(); ++iter) {
                if (*iter <= originalIndexI) {
                    originalIndexI++;
                    continue;
                }
                break;
            }
            int originalIndexJ = j;
            for (auto iter = removedIndices.cbegin(); iter != removedIndices.cend(); ++iter) {
                if (*iter <= originalIndexJ) {
                    originalIndexJ++;
                    continue;
                }
                break;
            }
            removedIndices.insert(originalIndexJ);
            dProgress() << QApplication::tr("Using frame %1 instead of %2.").arg(originalIndexI + 1).arg(originalIndexJ + 1);
            result = 1;
            j--;
        }
        if (!ProgressDialog::incValue()) {
            result |= 2;
            break;
        }
    }
    ProgressDialog::decBar();
    return result != 0;
}

bool D1Tileset::reuseSubtiles(std::set<int> &removedIndices)
{
    ProgressDialog::incBar(QApplication::tr("Reusing subtiles..."), this->min->getSubtileCount());
    int result = 0;
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        if (ProgressDialog::wasCanceled()) {
            result |= 2;
            break;
        }
        for (int j = i + 1; j < this->min->getSubtileCount(); j++) {
            QList<quint16> &frameReferences0 = this->min->getFrameReferences(i);
            QList<quint16> &frameReferences1 = this->min->getFrameReferences(j);
            if (frameReferences0.count() != frameReferences1.count()) {
                continue; // should not happen, but better safe than sorry
            }
            bool match = true;
            for (int x = 0; x < frameReferences0.count(); x++) {
                if (frameReferences0[x] == frameReferences1[x]) {
                    continue;
                }
                match = false;
                break;
            }
            if (!match) {
                continue;
            }
            // use subtile 'i' instead of subtile 'j'
            const unsigned refIndex = j;
            for (int n = 0; n < this->til->getTileCount(); n++) {
                QList<quint16> &subtileIndices = this->til->getSubtileIndices(n);
                for (auto iter = subtileIndices.begin(); iter != subtileIndices.end(); iter++) {
                    if (*iter == refIndex) {
                        *iter = i;
                        this->til->setModified();
                    }
                }
            }
            // eliminate subtile 'j'
            this->removeSubtile(j);
            // calculate the original indices
            int originalIndexI = i;
            for (auto iter = removedIndices.cbegin(); iter != removedIndices.cend(); ++iter) {
                if (*iter <= originalIndexI) {
                    originalIndexI++;
                    continue;
                }
                break;
            }
            int originalIndexJ = j;
            for (auto iter = removedIndices.cbegin(); iter != removedIndices.cend(); ++iter) {
                if (*iter <= originalIndexJ) {
                    originalIndexJ++;
                    continue;
                }
                break;
            }
            removedIndices.insert(originalIndexJ);
            dProgress() << QApplication::tr("Using subtile %1 instead of %2.").arg(originalIndexI + 1).arg(originalIndexJ + 1);
            result = true;
            j--;
        }
        if (!ProgressDialog::incValue()) {
            result |= 2;
            break;
        }
    }
    ProgressDialog::decBar();
    return result != 0;
}
