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
    int refIndex = subtileIndex;
    for (int i = 0; i < this->til->getTileCount(); i++) {
        std::vector<int> &subtileIndices = this->til->getSubtileIndices(i);
        for (unsigned n = 0; n < subtileIndices.size(); n++) {
            if (subtileIndices[n] >= refIndex) {
                // assert(subtileIndices[n] != refIndex);
                subtileIndices[n] -= 1;
                this->til->setModified();
            }
        }
    }
}

bool D1Tileset::reuseFrames(std::set<int> &removedIndices, bool silent)
{
    QString label = QApplication::tr("Reusing frames...");
    ProgressDialog::incBar(silent ? QStringLiteral("") : label, this->gfx->getFrameCount());

    QPair<int, QString> progress = { -1, label };
    if (silent) {
        dProgress() << progress;
    }
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
                std::vector<unsigned> &frameReferences = this->min->getFrameReferences(n);
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
            if (!silent) {
                dProgress() << QApplication::tr("Using frame %1 instead of %2.").arg(originalIndexI + 1).arg(originalIndexJ + 1);
            }
            result = 1;
            j--;
        }
        if (!ProgressDialog::incValue()) {
            result |= 2;
            break;
        }
    }
    auto amount = removedIndices.size();
    progress.second = QString(QApplication::tr("Reused %n frame(s).", "", amount)).arg(amount);
    dProgress() << progress;

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
            std::vector<unsigned> &frameReferences0 = this->min->getFrameReferences(i);
            std::vector<unsigned> &frameReferences1 = this->min->getFrameReferences(j);
            if (frameReferences0.size() != frameReferences1.size()) {
                continue; // should not happen, but better safe than sorry
            }
            bool match = true;
            for (unsigned x = 0; x < frameReferences0.size(); x++) {
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
            const int refIndex = j;
            for (int n = 0; n < this->til->getTileCount(); n++) {
                std::vector<int> &subtileIndices = this->til->getSubtileIndices(n);
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
