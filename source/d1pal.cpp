#include "d1pal.h"

#include <QApplication>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>

#include "config.h"
#include "d1image.h"
#include "d1pcx.h"
#include "progressdialog.h"

PaletteColor::PaletteColor(const QColor &color, int index)
    : xv(index)
    , rv(color.red())
    , gv(color.green())
    , bv(color.blue())
{
}

PaletteColor::PaletteColor(const PaletteColor &o)
    : xv(o.index())
    , rv(o.red())
    , gv(o.green())
    , bv(o.blue())
{
}

D1Pal::D1Pal(const D1Pal &opal)
    : QObject()
{
    this->palFilePath = opal.palFilePath;
    this->modified = opal.modified;
    this->undefinedColor = opal.undefinedColor;
    this->currentCycleCounter = opal.currentCycleCounter;
    this->updateColors(opal);
}

bool D1Pal::load(const QString &filePath)
{
    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly))
        return false;

    if (file.size() == D1PAL_SIZE_BYTES) {
        this->loadRegularPalette(file);
    } else if (!this->loadJascPalette(file)) {
        return false;
    }

    this->undefinedColor = QColor(Config::getPaletteUndefinedColor());
    if (filePath == D1Pal::DEFAULT_PATH && this->colors[1] != this->undefinedColor) {
        for (int i = 1; i < 128; i++) {
            this->colors[i] = this->undefinedColor;
            // this->modified = true; ?
        }
    }

    this->palFilePath = filePath;
    this->modified = false;
    return true;
}

void D1Pal::loadRegularPalette(QFile &file)
{
    QDataStream in(&file);

    for (int i = 0; i < D1PAL_COLORS; i++) {
        quint8 red;
        in >> red;

        quint8 green;
        in >> green;

        quint8 blue;
        in >> blue;

        this->colors[i] = QColor(red, green, blue);
    }
}

bool D1Pal::loadJascPalette(QFile &file)
{
    QTextStream txt(&file);
    QString line;
    QStringList lineParts;
    quint16 lineNumber = 0;

    while (!txt.atEnd()) {
        line = txt.readLine();
        lineNumber++;

        if (lineNumber == 1 && line != "JASC-PAL")
            return false;
        if (lineNumber == 3 && line != "256")
            return false;

        if (lineNumber <= 3)
            continue;
        if (lineNumber > 256 + 3)
            continue;

        lineParts = line.split(" ");
        if (lineParts.size() != 3) {
            return false;
        }

        quint8 red = lineParts[0].toInt();
        quint8 green = lineParts[1].toInt();
        quint8 blue = lineParts[2].toInt();
        // assert(D1PAL_COLORS == 256);
        this->colors[lineNumber - 4] = QColor(red, green, blue);
    }

    return lineNumber >= D1PAL_COLORS + 3;
}

bool D1Pal::save(const QString &filePath)
{
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath)));
        return false;
    }

    QDataStream out(&outFile);
    for (int i = 0; i < D1PAL_COLORS; i++) {
        QColor color = this->colors[i];
        quint8 byteToWrite;

        byteToWrite = color.red();
        out << byteToWrite;

        byteToWrite = color.green();
        out << byteToWrite;

        byteToWrite = color.blue();
        out << byteToWrite;
    }

    if (this->palFilePath == filePath) {
        this->modified = false;
    } else {
        // -- do not update, the user is creating a new one and the original needs to be preserved
        // this->modified = false;
        // this->palFilePath = filePath;
    }
    return true;
}

void D1Pal::compareTo(const D1Pal *pal, QString header) const
{
    for (int i = 0; i < D1PAL_COLORS; i++) {
        QColor colorA = this->colors[i];
        QColor colorB = pal->colors[i];
        if (colorA != colorB) {
            QString text = QApplication::tr("color %1 is %2 (was %3)").arg(i).arg(colorA.name()).arg(colorB.name());
            // reportDiff
            if (!header.isEmpty()) {
                dProgress() << header;
                header.clear();
            }
            dProgress() << text;
        }
    }
}

bool D1Pal::reloadConfig()
{
    bool change = false;
    QColor undefColor = QColor(Config::getPaletteUndefinedColor());

    for (int i = 0; i < D1PAL_COLORS; i++) {
        if (this->colors[i] == this->undefinedColor) {
            this->colors[i] = undefColor;
            change = true;
        }
    }
    this->undefinedColor = undefColor;
    if (change) {
        this->modified = true;
    }
    return change;
}

bool D1Pal::isModified() const
{
    return this->modified;
}

QString D1Pal::getFilePath() const
{
    return this->palFilePath;
}

void D1Pal::setFilePath(const QString &path)
{
    this->palFilePath = path;
}

QColor D1Pal::getUndefinedColor() const
{
    return this->undefinedColor;
}

void D1Pal::setUndefinedColor(QColor color)
{
    this->undefinedColor = color;
}

QColor D1Pal::getColor(quint8 index) const
{
    return this->colors[index];
}

void D1Pal::setColor(quint8 index, QColor color)
{
    this->colors[index] = color;
    this->modified = true;
}

void D1Pal::getValidColors(std::vector<PaletteColor> &colors) const
{
    for (int i = 0; i < D1PAL_COLORS; i++) {
        if (this->colors[i] != this->undefinedColor) {
            colors.push_back(PaletteColor(this->colors[i], i));
        }
    }
}

void D1Pal::updateColors(const D1Pal &opal)
{
    for (int i = 0; i < D1PAL_COLORS; i++) {
        this->colors[i] = opal.colors[i];
    }
}

bool D1Pal::genColors(const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this);
        bool success = D1Pcx::load(frame, imagefilePath, &basePal, this, &palMod);
        if (!success) {
            dProgressFail() << tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
            return false;
        }
        if (palMod) {
            // update the palette
            this->updateColors(basePal);
        }
        // update the view - done by the caller
        // this->displayFrame();
        return palMod;
    }

    QImage image = QImage(imagefilePath);

    return this->genColors(image);
}

static int colorWeight(const QColor color)
{
    int r = color.red(), g = color.green(), b = color.blue();
    return (r * 256 * 256 + g * 256 + b);
}
static QColor weightColor(unsigned weight)
{
    unsigned c = weight;
    unsigned r, g, b;
    r = c / (256 * 256); g = c / 256; b = c;
    return QColor(r % 256, g % 256, b % 256);
}

static int colorValue(const QColor color)
{
    int cv = 0;
    unsigned r = color.red(), g = color.green(), b = color.blue();
    for (int n = 0; n < 8; n++) {
        cv |= ((r & 1) | ((g & 1) << 1) | ((b & 1) << 2)) << (n * 3);
        r >>= 1;
        g >>= 1;
        b >>= 1;
    }
    return cv;
}
static QColor valueColor(unsigned cv)
{
    unsigned r = 0;
    unsigned g = 0;
    unsigned b = 0;
    for (int n = 0; n < 8; n++) {
        r |= ((cv & 1) >> 0) << n;
        g |= ((cv & 2) >> 1) << n;
        b |= ((cv & 4) >> 2) << n;
        cv >>= 3;
    }
    return QColor(r, g, b);
}
static std::pair<quint8, int> getPalColor(const std::vector<PaletteColor> &colors, QColor color)
{
    unsigned res = 0;
    int best = INT_MAX;

    for (const PaletteColor &palColor : colors) {
        int currR = color.red() - palColor.red();
        int currG = color.green() - palColor.green();
        int currB = color.blue() - palColor.blue();
        int curr = currR * currR + currG * currG + currB * currB;
        if (curr < best) {
            best = curr;
            res = palColor.index();
        }
    }

    return std::pair<quint8, int>(res, best);
}

bool D1Pal::genColors(const QImage &image)
{
    // find new color options
    QSet<int> col32s;
    QSet<int> freeIdxs;
    for (int i = 0; i < D1PAL_COLORS; i++) {
        if (this->colors[i] == this->undefinedColor) {
            freeIdxs.insert(i);
        } else if (this->colors[i].alpha() == 255) {
            col32s.insert(colorValue(this->colors[i]));
        }
    }

    if (freeIdxs.isEmpty()) {
        return false; // no place for new colors -> done
    }
    // cover the color-space as much as possible
    typedef struct colorData {
        int colorCode;
        int rangeLen;
        int closed;
        int marbles;
    } colorData;
    QList<colorData> ranges;
    if (col32s.count() == 0) {
        colorData cd;
        cd.colorCode = 0;
        cd.rangeLen = 0xFFFFFF;
        cd.closed = 0;
        cd.marbles = freeIdxs.count();
        ranges.push_back(cd);
    } else {
        if (*col32s.begin() != 0) {
            colorData cd;
            cd.colorCode = 0;
            cd.rangeLen = 0;
            cd.closed = 1;
            cd.marbles = 0;
            ranges.push_back(cd);
        }
        for (auto it = col32s.begin(); it != col32s.end(); it++) {
            int cc = *it;
            colorData cd;
            cd.colorCode = cc;
            cd.rangeLen = 0;
            cd.closed = 2;
            cd.marbles = 0;
            ranges.push_back(cd);
        }
        if (ranges.back().colorCode != 0xFFFFFF) {
            ranges.back().closed = 1;
        }
        // initialize rangeLen
        for (auto it = ranges.begin(); it != ranges.end(); it++) {
            auto nit = it + 1;
            if (nit != ranges.end()) {
                it->rangeLen = nit->colorCode - it->colorCode;
            } else {
                it->rangeLen = 0xFFFFFF - it->colorCode;
            }
        }
        // select colors at the longest gaps
        for (int i = 0; i < freeIdxs.count(); i++) {
            std::sort(ranges.begin(), ranges.end(), [](colorData &a, colorData &b) {
                int acw = a.rangeLen;
                int bcw = b.rangeLen;
                if (a.closed == 1 && a.marbles == 0)
                    acw *= 2;
                else
                    bcw *= a.marbles + (a.closed == 1 ? 0 : 1);
                if (b.closed == 1 && b.marbles == 0)
                    bcw *= 2;
                else
                    acw *= b.marbles + (b.closed == 1 ? 0 : 1);
                return acw > bcw || (acw == bcw && a.colorCode < b.colorCode);
            });

            ranges.front().marbles++;
        }
    }
    
    // designate the colors
    QList<colorData> colors;
    for (auto it = ranges.begin(); it != ranges.end(); it++) {
        int cc = it->colorCode;
        int n = it->marbles;
        int rl = it->rangeLen;
        if (it->closed == 0) {
            // assert(n > 0);
            n -= 2;
            {
                colorData cd;
                cd.colorCode = 0;
                cd.marbles = 1;
                colors.push_back(cd);
            }
            if (n < 0) {
                continue;
            }
            {
                colorData cd;
                cd.colorCode = 0xFFFFFF;
                cd.marbles = 1;
                colors.push_back(cd);
            }
        } else if (it->closed == 1) {
            n--;            
            if (cc == 0) {
                if (n < 0) {
                    continue;
                }

                colorData cd;
                cd.colorCode = 0;
                cd.marbles = 1;
                colors.push_back(cd);
            } else {
                colorData cd;
                cd.colorCode = cc;
                cd.marbles = 0;
                colors.push_back(cd);
                if (n < 0) {
                    continue;
                }

                colorData cde;
                cde.colorCode = 0xFFFFFF;
                cde.marbles = 1;
                colors.push_back(cde);
            }
        } else {
            {
                colorData cd;
                cd.colorCode = cc;
                cd.marbles = 0;
                colors.push_back(cd);
            }
        }

        for (int i = 0; i < n; i++) {
            colorData cd;
            cd.colorCode = cc + rl * (i + 1) / (n + 1);
            cd.marbles = 1;
            colors.push_back(cd);
        }
    }

    // prepare the new colors
    std::vector<PaletteColor> new_colors;
    {
        auto fi = freeIdxs.cbegin();
        for (auto it = colors.cbegin(); it != colors.cend(); it++) {
            if (it->marbles != 0) {
                QColor color = valueColor(it->colorCode);
                new_colors.push_back(PaletteColor(color, *fi));
                fi++;
            }
        }
    }

    // use the colors of the image to optimize the new colors
    if (!image.isNull()) {
        const QRgb *srcBits = reinterpret_cast<const QRgb *>(image.bits());
        std::map<int, int> wmap;
        for (int y = 0; y < image.height(); y++) {
            for (int x = 0; x < image.width(); x++, srcBits++) {
                // QColor color = image.pixelColor(x, y);
                QColor color = QColor::fromRgba(*srcBits);
                int w;
                // if (color == QColor(Qt::transparent)) {
                if (color.alpha() < COLOR_ALPHA_LIMIT) {
                    ;
                } else {
                    w = colorWeight(color);
                    // if (wmap.count(w) == 0) {
                    //     dProgressWarn() << QApplication::tr("New color %1:%2:%3 w %4 at %5:%6").arg(color.red()).arg(color.green()).arg(color.blue()).arg(w).arg(x).arg(y);
                    // }
                    wmap[w] += 1;
                }
            }
        }

        std::vector<PaletteColor> curr_colors;
        this->getValidColors(curr_colors);
        for (const PaletteColor pc : curr_colors) {
            wmap.erase(colorWeight(pc.color()));
        }

        if (wmap.size() < new_colors.size()) {
            new_colors.erase(new_colors.begin() + wmap.size(), new_colors.end());
            auto ni = new_colors.begin();
            for (auto it = wmap.cbegin(); it != wmap.cend(); it++, ni++) {
                QColor color = weightColor(it->first);
                *ni = PaletteColor(color, ni->index());
            }
        } else {
            freeIdxs.clear();

            while (true) {
                bool change = false;
                // check the next color mapping
                std::vector<PaletteColor> next_colors = curr_colors;
                next_colors.insert(next_colors.end(), new_colors.begin(), new_colors.end());

                std::map<int, std::pair<std::vector<std::pair<int, uint64_t>>, uint64_t>> umap;
                for (auto it = wmap.cbegin(); it != wmap.cend(); it++) {
                    QColor color = weightColor(it->first);
                    auto pc = getPalColor(next_colors, color);
                    uint64_t dist = (uint64_t)it->second * pc.second;
                    umap[pc.first].first.push_back(std::pair<int, uint64_t>(it->first, dist));
                    umap[pc.first].second += dist;
                }
                // eliminate unused new colors
                {
                    auto ni = next_colors.begin() + curr_colors.size();
                    for (auto it = new_colors.begin(); it != new_colors.end(); ) {
                        if (umap.count(it->index()) != 0) {
                            it++;
                            ni++;
                            continue;
                        }
                        else {
                            freeIdxs.insert(it->index());
                            it = new_colors.erase(it);
                            ni = next_colors.erase(ni);
                        }
                    }
                }
                // add new color to replace an eliminated one
                while (!freeIdxs.isEmpty()) {
                    // select the largest group
                    int res = 0;
                    uint64_t best = 0;
                    for (auto mi = umap.cbegin(); mi != umap.cend(); mi++) {
                        if (mi->second.second > best) {
                            best = mi->second.second;
                            res = mi->first;
                        }
                    }
                    if (best == 0) {
                        break;
                    }
                    change = true;

                    // select the largest distance
                    const std::vector<std::pair<int, uint64_t>> users = umap[res].first;
                    best = 0;
                    for (const std::pair<int, uint64_t>& user : users) {
                        if (user.second > best) {
                            best = user.second;
                            res = user.first;
                        }
                    }
                    {   // add the new color
                        QColor color = weightColor(res);
                        auto fi = freeIdxs.begin();
                        const PaletteColor pc = PaletteColor(color, *fi);
                        new_colors.push_back(pc);
                        next_colors.push_back(pc);
                        freeIdxs.erase(fi);
                    }

                    // split the users
                    umap[res].second = 0;
                    umap[res].first.clear();
                    for (const std::pair<int, uint64_t> &user : users) {
                        auto it = wmap.find(user.first);
                        QColor color = weightColor(it->first);
                        auto pc = getPalColor(next_colors, color);
                        uint64_t dist = (uint64_t)it->second * pc.second;
                        umap[pc.first].first.push_back(std::pair<int, uint64_t>(it->first, dist));
                        umap[pc.first].second += dist;
                    }
                }

                // select better colors for the color-groups



                if (!change) {
                    break;
                }
            }
        }
    } else {
        dProgress() << tr("Palette generated without image-file.");
    }

    // update the palette
    for (const PaletteColor pc : new_colors) {
        this->colors[pc.index()] = pc.color();
    }

    // update the view - done by the caller
    // this->displayFrame();
    return true;
}

void D1Pal::cycleColors(D1PAL_CYCLE_TYPE type)
{
    QColor celColor;
    int i;

    switch (type) {
    case D1PAL_CYCLE_TYPE::CAVES:
    case D1PAL_CYCLE_TYPE::HELL:
        // celColor = this->getColor(1);
        celColor = this->colors[1];
        for (i = 1; i < 31; i++) {
            // this->setColor(i, this->getColor(i + 1));
            this->colors[i] = this->colors[i + 1];
        }
        // this->setColor(i, celColor);
        this->colors[i] = celColor;
        break;
    case D1PAL_CYCLE_TYPE::NEST:
        if (--this->currentCycleCounter != 0)
            break;
        this->currentCycleCounter = 3;
        // celColor = this->getColor(8);
        celColor = this->colors[8];
        for (i = 8; i > 1; i--) {
            // this->setColor(i, this->getColor(i - 1));
            this->colors[i] = this->colors[i - 1];
        }
        // this->setColor(i, celColor);
        this->colors[i] = celColor;

        // celColor = this->getColor(15);
        celColor = this->colors[15];
        for (i = 15; i > 9; i--) {
            // this->setColor(i, this->getColor(i - 1));
            this->colors[i] = this->colors[i - 1];
        }
        // this->setColor(i, celColor);
        this->colors[i] = celColor;
        break;
    case D1PAL_CYCLE_TYPE::CRYPT:
        if (--this->currentCycleCounter == 0) {
            this->currentCycleCounter = 3;

            // celColor = this->getColor(15);
            celColor = this->colors[15];
            for (i = 15; i > 1; i--) {
                // this->setColor(i, this->getColor(i - 1));
                this->colors[i] = this->colors[i - 1];
            }
            // this->setColor(i, celColor);
            this->colors[i] = celColor;
        }

        // celColor = this->getColor(31);
        celColor = this->colors[31];
        for (i = 31; i > 16; i--) {
            // this->setColor(i, this->getColor(i - 1));
            this->colors[i] = this->colors[i - 1];
        }
        // this->setColor(i, celColor);
        this->colors[i] = celColor;
        break;
    }
}

int D1Pal::getCycleColors(D1PAL_CYCLE_TYPE type)
{
    return type == D1PAL_CYCLE_TYPE::NEST ? 16 : 32;
}
