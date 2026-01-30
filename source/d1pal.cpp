#include "d1pal.h"

#include <set>

#include <QApplication>
#include <QDataStream>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>

#include "config.h"
#include "d1image.h"
#include "d1pcx.h"
#include "progressdialog.h"

#include "libsmacker/smacker.h"

PaletteColor::PaletteColor(const QColor &color, int index)
    : xv(index)
    , rv(color.red())
    , gv(color.green())
    , bv(color.blue())
{
}

PaletteColor::PaletteColor(const QColor &color)
    : rv(color.red())
    , gv(color.green())
    , bv(color.blue())
{
}

PaletteColor::PaletteColor(int r, int g, int b, int x)
    : xv(x)
    , rv(r)
    , gv(g)
    , bv(b)
{
}

PaletteColor::PaletteColor(int r, int g, int b)
    : rv(r)
    , gv(g)
    , bv(b)
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

bool D1Pal::genColors(const QString &imagefilePath, bool forSmk)
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

    return this->genColors(image, forSmk);
}
static std::pair<quint8, int> getPalColor(const std::vector<PaletteColor> &colors, const PaletteColor &color);
static void smackColor(PaletteColor &col, bool forSmk)
{
    if (!forSmk) {
        return;
    }
    unsigned char smkColor[3];
    smkColor[0] = col.red();
    smkColor[1] = col.green();
    smkColor[2] = col.blue();
#if 0
    for (int n = 0; n < 3; n++) {
        unsigned char cv = smkColor[n];
        const unsigned char *p = &palmap[0];
        for (; /*p < lengthof(palmap)*/; p++) {
            if (cv <= p[0]) {
                if (cv != p[0]) {
                    if (p[0] - cv > cv - p[-1]) {
                        p--;
                    }
                    if (n == 0) {
                        col.setRed(*p);
                    }
                    if (n == 1) {
                        col.setGreen(*p);
                    }
                    if (n == 2) {
                        col.setBlue(*p);
                    }
                }
                break;
            }
        }
    }
#else
    std::vector<PaletteColor> colors;
    colors.push_back(PaletteColor(col.red(), col.green(), col.blue(), 0));
    for (int n = 0; n < 3; n++) {
        unsigned char cv = smkColor[n];
        const unsigned char *p = &palmap[0];
        for ( ; /*p < lengthof(palmap)*/; p++) {
            if (cv <= p[0]) {
                if (cv != p[0]) {
                    int num = colors.size();
                    for (int i = 0; i < num; i++) {
                        PaletteColor &c0 = colors[i];
                        PaletteColor c1 = PaletteColor(colors[i]);
                        c1.setIndex(num + i);
                        if (n == 0) {
                            c0.setRed(p[0]);
                            c1.setRed(p[-1]);
                        }
                        if (n == 1) {
                            c0.setGreen(p[0]);
                            c1.setGreen(p[-1]);
                        }
                        if (n == 2) {
                            c0.setBlue(p[0]);
                            c1.setBlue(p[-1]);
                        }
                        colors.push_back(c1);
                    }
                }
                break;
            }
        }
    }
    PaletteColor &pc = colors[getPalColor(colors, col).first];
    col.setRed(pc.red());
    col.setGreen(pc.green());
    col.setBlue(pc.blue());
#endif
}

static int colorWeight(PaletteColor color, bool forSmk)
{
    smackColor(color, forSmk);
    int r = color.red(), g = color.green(), b = color.blue();
    return (r * 256 * 256 + g * 256 + b);
}
static PaletteColor weightColor(unsigned weight)
{
    unsigned c = weight;
    unsigned r, g, b;
    r = c / (256 * 256); g = c / 256; b = c;
    return PaletteColor(r % 256, g % 256, b % 256);
}

static int colorValue(const QColor &color)
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
static PaletteColor valueColor(unsigned cv, bool forSmk)
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
    PaletteColor color = PaletteColor(r, g, b);
    smackColor(color, forSmk);
    return color;
}
int D1Pal::getColorDist(const PaletteColor &color, const PaletteColor &palColor)
{
    int currR = color.red() - palColor.red();
    int currG = color.green() - palColor.green();
    int currB = color.blue() - palColor.blue();
    int curr = currR * currR + currG * currG + currB * currB;
    return curr;
}
static std::pair<quint8, int> getPalColor(const std::vector<PaletteColor> &colors, const PaletteColor &color)
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
static bool debugSort = false;
// sort colors by the number of users and/or by the RGB values
static bool sortNewColors(std::vector<PaletteColor> &next_colors, unsigned prev_colornum,
    const std::map<int, std::pair<std::vector<std::pair<int, uint64_t>>, uint64_t>>* umap, const std::map<int, uint64_t> &wmap)
{
if (debugSort) {
    dProgress() << "++++++++++++++++++++";
}
    // prepare a separate vector with the user-count information
    std::vector<std::pair<PaletteColor, uint64_t>> new_colors;
    for (auto it = next_colors.begin() + prev_colornum; it != next_colors.end(); it++) {
        uint64_t uc = 0;
        if (umap) {
            for (const std::pair<int, uint64_t>& user : umap->at(it->index()).first) {
                uc += wmap.at(user.first);
            }
        }
        new_colors.push_back(std::pair<PaletteColor, uint64_t>(*it, uc));
if (debugSort) {
    dProgress() << QApplication::tr("%1.(%2:%3:%4) : %5").arg(it->index()).arg(it->red()).arg(it->green()).arg(it->blue()).arg(uc);
}
    }
    // sort the new vector
    std::sort(new_colors.begin(), new_colors.end(), [](std::pair<PaletteColor, uint64_t> &a, std::pair<PaletteColor, uint64_t> &b) {
        if (a.second < b.second) {
            return false;
        }
        if (b.second < a.second) {
            return true;
        }
        if (a.first.red() < b.first.red()) {
            return true;
        }
        if (b.first.red() < a.first.red()) {
            return false;
        }
        if (a.first.green() < b.first.green()) {
            return true;
        }
        if (b.first.green() < a.first.green()) {
            return false;
        }
        return a.first.blue() < b.first.blue();
    });
    // collect the available indices
    std::set<int> freeIdxs;
    for (const std::pair<PaletteColor, uint64_t> &nc : new_colors) {
        freeIdxs.insert(nc.first.index());
    }
    // update the colors
    bool change = false;
    auto ni = new_colors.cbegin();
    auto fi = freeIdxs.cbegin();
if (debugSort) {
    dProgress() << "-------------------";
}
    for (auto it = next_colors.begin() + prev_colornum; it != next_colors.end(); it++, fi++, ni++) {
        if (it->eq(ni->first) && it->index() == *fi) continue;
if (debugSort) {
    dProgress() << QApplication::tr("%1.(%2:%3:%4) -> %5. (%6:%7:%8)").arg(it->index()).arg(it->red()).arg(it->green()).arg(it->blue()).arg(*fi).arg(ni->first.red()).arg(ni->first.green()).arg(ni->first.blue());
}
        *it = PaletteColor(ni->first.red(), ni->first.green(), ni->first.blue(), *fi);
        change = true;
    }
if (debugSort) {
    dProgress() << "-------------------";
}
    return change;
}

bool D1Pal::genColors(const QImage &image, bool forSmk)
{
    // find new color options
    std::set<int> col32s;
    std::set<int> freeIdxs;
    for (int i = 0; i < D1PAL_COLORS; i++) {
        if (this->colors[i] == this->undefinedColor) {
            freeIdxs.insert(i);
        } else if (this->colors[i].alpha() == 255) {
            col32s.insert(colorValue(this->colors[i]));
        }
    }
    //if (forSmk && !freeIdxs.empty()) {
    //    freeIdxs.erase(std::prev(freeIdxs.end()));
    //}
    if (freeIdxs.empty()) {
        return false; // no place for new colors -> done
    }
    // cover the color-space as much as possible
    typedef struct colorData {
        int colorCode;
        int rangeLen;
        int closed;
        int marbles;
    } colorData;
    std::vector<colorData> ranges;
    if (col32s.size() == 0) {
        colorData cd;
        cd.colorCode = 0;
        cd.rangeLen = 0xFFFFFF;
        cd.closed = 0;
        cd.marbles = freeIdxs.size();
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
        for (const int cc : col32s) {
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
        for (const int idx : freeIdxs) {
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
    std::vector<colorData> colors;
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
                PaletteColor color = valueColor(it->colorCode, forSmk);
                color.setIndex(*fi);
                new_colors.push_back(color);
                fi++;
            }
        }
        freeIdxs.clear();
    }

    // use the colors of the image to optimize the new colors
    if (!image.isNull()) {
        const QRgb *srcBits = reinterpret_cast<const QRgb *>(image.bits());
        std::map<int, uint64_t> wmap;
        for (int y = 0; y < image.height(); y++) {
            for (int x = 0; x < image.width(); x++, srcBits++) {
                // QColor color = image.pixelColor(x, y);
                QColor color = QColor::fromRgba(*srcBits);
                int w;
                // if (color == QColor(Qt::transparent)) {
                if (color.alpha() < COLOR_ALPHA_LIMIT) {
                    ;
                } else {
                    w = colorWeight(PaletteColor(color), forSmk);
                    if (wmap.count(w) == 0) {
                        PaletteColor wc = weightColor(w);
                        dProgressWarn() << QApplication::tr("New color %1:%2:%3 w %4 at %5:%6 -> %7:%8:%9").arg(color.red()).arg(color.green()).arg(color.blue()).arg(w).arg(x).arg(y).arg(wc.red()).arg(wc.green()).arg(wc.blue());
                    }
                    wmap[w] += 1;
                }
            }
        }

        std::vector<PaletteColor> next_colors;
        this->getValidColors(next_colors);
        const unsigned prev_colornum = next_colors.size();
        for (const PaletteColor pc : next_colors) {
            wmap.erase(colorWeight(pc, false));
        }
debugSort = true;
        if (wmap.size() <= new_colors.size()) {
#if 0
            new_colors.erase(new_colors.begin() + wmap.size(), new_colors.end());
            auto ni = new_colors.begin();
            for (auto it = wmap.cbegin(); it != wmap.cend(); it++, ni++) {
                PaletteColor color = weightColor(it->first);
                color.setIndex(ni->first.index());
                *ni = std::pair<PaletteColor, uint64_t>(color, it->second);
            }
#else
            std::map<int, std::pair<std::vector<std::pair<int, uint64_t>>, uint64_t>> umap;
if (debugSort) {
    dProgress() << QApplication::tr("******************** ... %1 vs %2").arg(wmap.size()).arg(new_colors.size());
}
            auto ni = new_colors.begin();
            for (auto it = wmap.cbegin(); it != wmap.cend(); it++, ni++) {
                const int w = it->first;
                PaletteColor color = weightColor(w);
                const int idx = ni->index();
                color.setIndex(idx);
                // *ni = color;
                next_colors.push_back(color);
                umap[idx].first.push_back(std::pair<int, uint64_t>(w, 0));
if (debugSort) {
    dProgress() << QApplication::tr("%1.(%2:%3:%4) : %5").arg(idx).arg(color.red()).arg(color.green()).arg(color.blue()).arg(it->second);
}
            }

            // next_colors.insert(next_colors.end(), new_colors.begin(), new_colors.begin() + wmap.size());
            new_colors.clear();

            sortNewColors(next_colors, prev_colornum, forSmk ? &umap : nullptr, wmap);
#endif
        } else {
            next_colors.insert(next_colors.end(), new_colors.begin(), new_colors.end());
            new_colors.clear();

            int ch = 0;
            while (true) {
                bool change = false;
                // check the next color mapping

                std::map<int, std::pair<std::vector<std::pair<int, uint64_t>>, uint64_t>> umap; // idx -> ([w, dist], sum-dist)
                for (auto it = wmap.cbegin(); it != wmap.cend(); it++) {
                    PaletteColor color = weightColor(it->first);
                    auto pc = getPalColor(next_colors, color);
                    uint64_t dist = (uint64_t)it->second * pc.second;
                    umap[pc.first].first.push_back(std::pair<int, uint64_t>(it->first, dist));
                    umap[pc.first].second += dist;
                }
                // eliminate unused new colors
                for (auto it = next_colors.begin() + prev_colornum; it != next_colors.end(); ) {
                    if (umap.count(it->index()) != 0) {
                        it++;
                        continue;
                    } else {
                        freeIdxs.insert(it->index());
                        it = next_colors.erase(it);
                    }
                }
#if 0
                // add new color to replace an eliminated ones
                while (!freeIdxs.empty()) {
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
                    // change = true;

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
                        PaletteColor color = weightColor(res);
                        auto fi = freeIdxs.begin();
                        color.setIndex(*fi)
                        next_colors.push_back(color);
                        freeIdxs.erase(fi);
                    }

                    // split the users
                    // umap[res].second = 0;
                    // umap[res].first.clear();
                    umap.erase(res);
                    for (const std::pair<int, uint64_t> &user : users) {
                        auto it = wmap.find(user.first);
                        PaletteColor color = weightColor(it->first);
                        auto pc = getPalColor(next_colors, color);
                        uint64_t dist = (uint64_t)it->second * pc.second;
                        umap[pc.first].first.push_back(std::pair<int, uint64_t>(it->first, dist));
                        umap[pc.first].second += dist;
                    }
                }
#else
                // add new color to replace an eliminated one
                if (!freeIdxs.empty()) {
                    // select the largest group
                    int res = 0;
                    uint64_t best = 0;
                    for (auto mi = umap.cbegin(); mi != umap.cend(); mi++) {
                        if (mi->second.second > best) {
                            best = mi->second.second;
                            res = mi->first;
                        }
                    }
                    if (best != 0) {
                        // umap[res].second = 0;
                        // select the largest distance
                        best = 0;
                        for (const std::pair<int, uint64_t>& user : umap[res].first) {
                            if (user.second > best) {
                                best = user.second;
                                res = user.first;
                            }
                        }

                        PaletteColor color = weightColor(res);
                        auto fi = freeIdxs.begin();
                        color.setIndex(*fi);
                        next_colors.push_back(color);
                        freeIdxs.erase(fi);
                        continue;
                    }
                }
#endif

                // select better colors for the color-groups with new colors

                for (auto it = next_colors.begin() + prev_colornum; it != next_colors.end(); it++) {
                    uint64_t r = 0, g = 0, b = 0; uint64_t tw = 0;
                    for (const std::pair<int, uint64_t>& user : umap[it->index()].first) {
                        PaletteColor wc = weightColor(user.first);
                        uint64_t cw = wmap[user.first];
                        r += cw * (wc.red() * wc.red());
                        g += cw * (wc.green() * wc.green());
                        b += cw * (wc.blue() * wc.blue());
                        tw += cw;
                    }

                    r = round(sqrt((double)r / tw));
                    g = round(sqrt((double)g / tw));
                    b = round(sqrt((double)b / tw));

                    PaletteColor color = PaletteColor(r, g, b, it->index());
                    smackColor(color, forSmk);

                    // ensure the new color is unique
                    auto nit = next_colors.begin();
                    for ( ; nit != next_colors.end(); nit++) {
                        if (nit->red() == color.red() && nit->green() == color.green() && nit->red() == color.red()) {
                            break;
                        }
                    }
                    if (nit != next_colors.end()) continue;

                    // check if there is really a gain
                    {
                        int64_t dd = 0;
                        for (const std::pair<int, uint64_t>& user : umap[it->index()].first) {
                            PaletteColor uc = weightColor(user.first);
                            uint64_t cw = wmap[user.first];

                            uint64_t pd = D1Pal::getColorDist(uc, *it);
                            uint64_t nd = D1Pal::getColorDist(uc, color);
                            dd += (int64_t)cw * (int64_t)(nd - pd);
                        }
                        if (dd <= 0) continue;
                    }
                    // select the new color
                    *it = color;
                    change = true;
                }

                if (change) {
                    continue;
                }

                change = sortNewColors(next_colors, prev_colornum, forSmk ? &umap : nullptr, wmap);

                /*if (change) {
                    if (++ch < 10) {
                    continue;
                    }
                    debugSort = true;
                    if (++ch < 20) {
                    continue;
                    }
                }*/

                break;
            }
        }

        // new_colors.clear();
        new_colors.insert(new_colors.end(), next_colors.begin() + prev_colornum, next_colors.end());
    } else {
        dProgress() << tr("Palette generated without image-file.");
    }

    // update the palette
    for (const PaletteColor &pc : new_colors) {
        const QColor nc = pc.color();
        if (nc == this->undefinedColor)
            dProgressWarn() << tr("The undefined color is selected as a valid palette-entry.");
        this->colors[pc.index()] = nc;
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
