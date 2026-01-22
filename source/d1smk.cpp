#include "d1smk.h"

#include <algorithm>    // std::sort/find

#include <QApplication>
#include <QAudioFormat>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioSink>
typedef QAudioSink SmkAudioOutput;
#else
#include <QAudioOutput>
typedef QAudioOutput SmkAudioOutput;
#endif
#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QObject>

#include "config.h"
#include "mainwindow.h"
#include "progressdialog.h"

#include "dungeon/all.h"
//#include <smacker.h>
//#include "../3rdParty/libsmacker/smacker.h"
#include "libsmacker/smacker.h"

class AudioBuffer;

static bool audioSemaphore[D1SMK_TRACKS] = { false };
static SmkAudioOutput *audioOutput[D1SMK_TRACKS] = { nullptr };
static AudioBuffer *smkAudioBuffer[D1SMK_TRACKS] = { nullptr };

class AudioBuffer : public QIODevice {
//    Q_OBJECT

public:
    AudioBuffer();
    ~AudioBuffer() = default;

    qint64 peek(char *data, qint64 maxSize);
    qint64 bytesAvailable() const override;
    bool   isSequential() const override;
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

    void clear();

private:
    QList<QPair<uint8_t *, unsigned long>> audioQueue;
    qint64 currPos = 0;
    qint64 availableBytes = 0;
};

AudioBuffer::AudioBuffer()
    : QIODevice()
{
}

qint64 AudioBuffer::bytesAvailable() const
{
    return availableBytes;
}

bool AudioBuffer::isSequential() const
{
    return true;
}

qint64 AudioBuffer::peek(char *data, qint64 maxSize)
{
    qint64 result = availableBytes;
    if (maxSize < result) {
        result = maxSize;
    }
    qint64 rem = result;
    for (int i = 0; i < audioQueue.count(); i++) {
        QPair<uint8_t *, unsigned long> &entry = audioQueue[i];
        uint8_t *entryData = entry.first;
        qint64 len = entry.second;
        if (i == 0) {
            entryData += currPos;
            len -= currPos;
        }
        if (len > rem) {
            len = rem;
            rem = 0;
        } else {
            rem -= len;
        }
        memcpy(data, entryData, len);
        data += len;
        if (rem == 0)
            break;
    }
    return result;
}

qint64 AudioBuffer::readData(char *data, qint64 maxSize)
{
    qint64 result = peek(data, maxSize);

    if (result != 0) {
        availableBytes -= result;

        qint64 rem = result;
        qint64 cp = currPos;
        int i = 0;
        for (; ; i++) {
            QPair<uint8_t *, unsigned long> &data = audioQueue[i];
            qint64 len = data.second;
            len -= cp;
            if (len > rem) {
                cp += rem;
                break;
            } else {
                rem -= len;
                cp = 0;
                if (rem == 0) {
                    i++;
                    break;
                }
            }
        }
        currPos = cp;
        if (i != 0) {
            audioQueue.erase(audioQueue.begin(), audioQueue.begin() + i);
        }
    }
    return result;
}

qint64 AudioBuffer::writeData(const char *data, qint64 maxSize)
{
    audioQueue.push_back(QPair<uint8_t *, unsigned long>((uint8_t *)data, (unsigned long)maxSize));
    availableBytes += maxSize;
    return maxSize;
}

void AudioBuffer::clear()
{
    audioQueue.clear();
    availableBytes = 0;
}

#define D1SMK_COLORS 256

D1SmkAudioData::D1SmkAudioData(unsigned ch, unsigned bd, unsigned long br)
    : channels(ch)
    , bitDepth(bd)
    , bitRate(br)
{
}

D1SmkAudioData::~D1SmkAudioData()
{
    for (int i = 0; i < D1SMK_TRACKS; i++) {
        mem_free_dbg(this->audio[i]);
    }
}

unsigned D1SmkAudioData::getChannels() const
{
    return this->channels;
}

unsigned D1SmkAudioData::getBitDepth() const
{
    return this->bitDepth;
}

bool D1SmkAudioData::setBitRate(unsigned br)
{
    if (this->bitRate == br) {
        return false;
    }
    this->bitRate = br;
    return true;
}

unsigned D1SmkAudioData::getBitRate() const
{
    return this->bitRate;
}

bool D1SmkAudioData::setAudio(unsigned track, uint8_t* data, unsigned long len)
{
    // assert((len % (this->channels * this->bitDepth / 8) == 0);
    bool result = this->len[track] != len || (len != 0 && memcmp(data, this->audio[track], len) != 0);
    free(this->audio[track]);
    this->audio[track] = data;
    this->len[track] = len;
    return result;
}

uint8_t* D1SmkAudioData::getAudio(unsigned track, unsigned long *len) const
{
    *len = this->len[track];
    return const_cast<uint8_t*>(this->audio[track]);
}

bool D1SmkAudioData::setCompress(unsigned track, uint8_t compress)
{
    if (this->compress[track] == compress) {
        return false;
    }
    this->compress[track] = compress;
    return true;
}

uint8_t D1SmkAudioData::getCompress(unsigned track) const
{
    return this->compress[track];
}

static D1Pal* LoadPalette(smk SVidSMK)
{
    D1Pal *pal = new D1Pal();

    const unsigned char *smkPal = smk_get_palette(SVidSMK);
    for (int i = 0; i < D1SMK_COLORS; i++) {
        pal->setColor(i, QColor(smkPal[i * 3 + 0], smkPal[i * 3 + 1], smkPal[i * 3 + 2]));
    }
    return pal;
}

static void RegisterPalette(D1Pal *pal, unsigned frameFrom, unsigned frameTo, QMap<QString, D1Pal *> &pals)
{
    QString palPath = QString("Frame%1-%2").arg(frameFrom + 1, 4, 10, QChar('0')).arg(frameTo + 1, 4, 10, QChar('0'));
    pal->setFilePath(palPath);
    pals[palPath] = pal;
}

bool D1Smk::load(D1Gfx &gfx, QMap<QString, D1Pal *> &pals, const QString &filePath, const OpenAsParam &params)
{
    gfx.clear();

    QFile file = QFile(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        dProgressErr() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    const quint64 fileSize = file.size();
    unsigned char *SVidBuffer = (unsigned char *)malloc(fileSize);
    if (file.read((char *)SVidBuffer, fileSize) != fileSize) {
        MemFreeDbg(SVidBuffer);
        dProgressErr() << QApplication::tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    smk SVidSMK = smk_open_memory(SVidBuffer, fileSize);
    if (SVidSMK == NULL) {
        MemFreeDbg(SVidBuffer);
        dProgressErr() << QApplication::tr("Invalid SMK file.");
        return false;
    }

    unsigned long SVidWidth, SVidHeight;
    double SVidFrameLength;
    smk_info_all(SVidSMK, &SVidWidth, &SVidHeight, &SVidFrameLength);

    unsigned char channels, depth;
    unsigned long rate;
    smk_info_audio(SVidSMK, &channels, &depth, &rate);

    smk_enable_video(SVidSMK, true);
    for (int i = 0; i < D1SMK_TRACKS; i++) {
        smk_enable_audio(SVidSMK, i, true);
    }
    // Decode first frame

    char result = smk_first(SVidSMK);
    if (SMK_ERR(result)) {
        MemFreeDbg(SVidBuffer);
        dProgressErr() << QApplication::tr("Empty SMK file.");
        return false;
    }

    // load the first palette
    D1Pal *pal = LoadPalette(SVidSMK);

    // load the frames
    if (params.celWidth != 0) {
        dProgressWarn() << QApplication::tr("Width setting is ignored when a SMK file is loaded.");
    }
    gfx.clipped = params.clipped == OPEN_CLIPPED_TYPE::TRUE;
    unsigned frameNum = 0;
    unsigned prevPalFrame = 0;
    const unsigned char *smkFrame = smk_get_video(SVidSMK);
    do {
        bool palUpdate = smk_palette_updated(SVidSMK);
        if (palUpdate && frameNum != 0) {
            RegisterPalette(pal, prevPalFrame, frameNum - 1, pals);
            prevPalFrame = frameNum;
            // load the new palette
            pal = LoadPalette(SVidSMK);
        }
        // create a new frame
        D1GfxFrame *frame = new D1GfxFrame();
        frame->framePal = palUpdate ? pal : nullptr;
        const unsigned char *smkFrameCursor = smkFrame;
        for (unsigned y = 0; y < SVidHeight; y++) {
            std::vector<D1GfxPixel> pixelLine;
            for (unsigned x = 0; x < SVidWidth; x++, smkFrameCursor++) {
                pixelLine.push_back(D1GfxPixel::colorPixel(*smkFrameCursor));
            }
            frame->addPixelLine(std::move(pixelLine));
        }

        D1SmkAudioData *audio = new D1SmkAudioData(channels, depth, rate);
        for (unsigned i = 0; i < D1SMK_TRACKS; i++) {
            uint8_t compress;
            unsigned long len = smk_get_audio_size(SVidSMK, i, &compress);
            unsigned char* ct = nullptr;
            if (len != 0) {
                unsigned char* track = smk_get_audio(SVidSMK, i);
                ct = (unsigned char *)malloc(len);
                memcpy(ct, track, len);
            }
            audio->setAudio(i, ct, len);
            audio->setCompress(i, compress);
        }
        frame->frameAudio = audio;

        gfx.frames.append(frame);
        frameNum++;
    } while ((result = smk_next(SVidSMK)) == SMK_MORE);

    if (SMK_ERR(result)) {
        dProgressErr() << QApplication::tr("SMK not fully loaded.");
    }
    RegisterPalette(pal, prevPalFrame, frameNum - 1, pals);

    smk_close(SVidSMK);
    MemFreeDbg(SVidBuffer);

    gfx.groupFrameIndices.push_back(std::pair<int, int>(0, frameNum - 1));

    gfx.type = D1CEL_TYPE::SMK;
    gfx.frameLen = SVidFrameLength;

    gfx.gfxFilePath = filePath;
    gfx.modified = false;
    return true;
}

typedef struct _SmkAudioInfo {
    unsigned bitRate;
    unsigned bitDepth;
    unsigned channels;
    unsigned maxChunkLength;
    bool compress;
} SmkAudioInfo;

typedef struct _SmkFrameInfo {
    unsigned FrameSize;
    uint8_t FrameType;
    uint8_t *FrameData;
} SmkFrameInfo;

typedef struct _SmkTreeInfo {
    QList<QPair<unsigned, unsigned>> treeStat;
    QList<QPair<unsigned, unsigned>> cacheStat[3];
    unsigned cacheCount[3];
    unsigned uncompressedTreeSize;
    QMap<unsigned, QPair<unsigned, uint32_t>> paths;
} SmkTreeInfo;

static void addTreeValue(uint16_t value, SmkTreeInfo &tree, unsigned (&cacheValues)[3])
{
    QList<QPair<unsigned, unsigned>> *stat = &tree.treeStat;
    for (int i = 0; i < 3; i++) {
        if (value == cacheValues[i]) {
            stat = &tree.cacheStat[i];
            tree.cacheCount[i]++;
            break;
        }
    }

    /*auto val = std::find_if(stat->begin(), stat->end(), [value](const QPair<unsigned, unsigned> &entry) { return entry.first == value; });
    if (val == tree.treeStat.end()) {
        stat->push_back(QPair<unsigned, unsigned>(value, 1));
    } else {
        stat->second.second++;
    }*/
    auto it = stat->begin();
    for (; it != stat->end(); it++) {
        if (it->first == value) {
            it->second++;
            break;
        }
    }
    if (it == stat->end()) {
        stat->push_back(QPair<unsigned, unsigned>(value, 1));
    }

    if (cacheValues[0] != value) {
        cacheValues[2] = cacheValues[1];
        cacheValues[1] = cacheValues[0];
        cacheValues[0] = value;
    }
}

static void addTreeTypeValue(int type, int typelen, SmkTreeInfo &tree, unsigned (&cacheValues)[3])
{
    if (typelen != 0) {
        for (int i = lengthof(sizetable) - 1; /*i >= 0*/; ) {
            if (sizetable[i] <= typelen) {
                if (sizetable[i] != typelen && sizetable[i] == 59) {
                    // FIXME: delay to reuse existing leafs
                }

                addTreeValue(type | i << 2, tree, cacheValues);
                typelen -= sizetable[i];
                if (typelen == 0) {
                    break;
                }
            } else {
                i--;
            }
        }
    }
}

static uint8_t* writeNBits(unsigned value, unsigned length, uint8_t *cursor, unsigned &bitNum)
{
    for (unsigned i = 0; i < length; i++) {
        // *cursor |= ((value >> (length - i - 1)) & 1) << bitNum;
        *cursor |= ((value >> i) & 1) << bitNum;
        bitNum++;
        if (bitNum == 8) {
            bitNum = 0;
            cursor++;
        }
    }

    return cursor;
}

static uint8_t* writeBit(unsigned value, uint8_t *cursor, unsigned &bitNum)
{
    *cursor |= value << bitNum;
    bitNum++;
    if (bitNum == 8) {
        bitNum = 0;
        cursor++;
    }
    return cursor;
}

typedef struct _TreeLeaf {
    bool isBranch;
    _TreeLeaf *left;
    _TreeLeaf *right;
    unsigned value;
    unsigned weight;
} TreeLeaf;

static uint8_t *writeTreeLeafs(TreeLeaf* treeLeaf, uint8_t *cursor, unsigned &bitNum, uint32_t branch, unsigned depth,
    unsigned &joints, QMap<unsigned, QPair<unsigned, uint32_t>> &paths, QMap<unsigned, QPair<unsigned, uint32_t>> *leafPaths)
{
    joints++;
    if (!treeLeaf->isBranch) {
        cursor = writeBit(0, cursor, bitNum);
        unsigned leaf = treeLeaf->value;
        paths[leaf] = QPair<unsigned, uint32_t>(depth, branch);

        if (leafPaths == nullptr) {
            cursor = writeNBits(leaf, 8, cursor, bitNum);
        } else {
            {
                auto it = leafPaths[0].find(leaf & 0xFF);
                if (it == leafPaths[0].end()) {
                    dProgressErr() << QApplication::tr("ERROR: Missing entry for leaf %1 in the low paths.").arg(leaf & 0xFF);
                } else {
                    QPair<unsigned, uint32_t> &theEntryPair = it.value();
                    cursor = writeNBits(theEntryPair.second, theEntryPair.first, cursor, bitNum);
                }
            }
            {
                auto it = leafPaths[1].find((leaf >> 8) & 0xFF);
                if (it == leafPaths[1].end()) {
                    dProgressErr() << QApplication::tr("ERROR: Missing entry for leaf %1 in the high paths.").arg((leaf >> 8) & 0xFF);
                } else {
                    QPair<unsigned, uint32_t> theEntryPair = it.value();
                    cursor = writeNBits(theEntryPair.second, theEntryPair.first, cursor, bitNum);
                }
            }
        }
        return cursor;
    }
    cursor = writeBit(1, cursor, bitNum);
    depth++;
    if (depth > 32) {
        dProgressErr() << QApplication::tr("writeTreeLeafs ERROR: depth %1 too much.").arg(depth);
    }
    // branch <<= 1;
    cursor = writeTreeLeafs(treeLeaf->left, cursor, bitNum, branch, depth, joints, paths, leafPaths);

    branch |= 1 << (depth - 1);
    cursor = writeTreeLeafs(treeLeaf->right, cursor, bitNum, branch, depth, joints, paths, leafPaths);

    return cursor;
}

static uint8_t *writeTree(QList<QPair<unsigned, unsigned>> leafs, uint8_t *cursor, unsigned &bitNum,
    unsigned &joints, QMap<unsigned, QPair<unsigned, uint32_t>> &paths, QMap<unsigned, QPair<unsigned, uint32_t>> *leafPaths)
{
    // std::sort(leafs.begin(), leafs.end(), leafSorter);

    QList<TreeLeaf*> treeLeafs;
    for (auto it = leafs.begin(); it != leafs.end(); it++) {
        TreeLeaf *tl = new TreeLeaf();
        tl->value = it->first;
        tl->weight = it->second;
        tl->isBranch = false;
        treeLeafs.push_back(tl);
    }
    while (treeLeafs.size() > 1) {
        TreeLeaf *tl0 = treeLeafs.back();
        treeLeafs.pop_back();
        TreeLeaf *tl1 = treeLeafs.back();
        treeLeafs.pop_back();
        TreeLeaf *tls = new TreeLeaf();
        tls->isBranch = true;
        tls->left = tl0;
        tls->right = tl1;
        tls->weight = tl0->weight + tl1->weight;

        size_t size = treeLeafs.size();
        auto it = treeLeafs.begin();
        if (size != 0) {
            it += size - 1;
            for ( ; ; it--) {
                if ((*it)->weight > tls->weight) {
                    it++;
                    break;
                }

                if (it == treeLeafs.begin()) {
                    break;
                }
            }
        }
        treeLeafs.insert(it, tls);
    }

    uint8_t *res = writeTreeLeafs(treeLeafs.front(), cursor, bitNum, 0, 0, joints, paths, leafPaths);

    while (!treeLeafs.empty()) {
        TreeLeaf *tls = treeLeafs.back();
        treeLeafs.pop_back();
        if (tls->isBranch) {
            treeLeafs.push_back(tls->left);
            treeLeafs.push_back(tls->right);
        }
        free(tls);
    }
    return res;
}

bool leafSorter(const QPair<unsigned, unsigned> &e1, const QPair<unsigned, unsigned> &e2)
{
    return e1.second > e2.second || (e1.second == e2.second && e1.first < e2.first);
}

static void addTreeValue(unsigned byteValue, QList<QPair<unsigned, unsigned>> &byteLeafs)
{
    auto it = byteLeafs.begin();
    for ( ; it != byteLeafs.end(); it++) {
        if (it->first == byteValue) {
            it->second++;
            break;
        }
    }
    if (it == byteLeafs.end()) {
        byteLeafs.push_back(QPair<unsigned, unsigned>(byteValue, 1));
    }
}

static void prepareVideoTree(SmkTreeInfo &tree, uint8_t *treeData, size_t &cursor, unsigned &bitNum)
{
    // reduce inflated cache frequencies
    for (int i = 0; i < 3; i++) {
        for (int n = 0; n < tree.cacheStat[i].count(); n++) {
            unsigned value = tree.cacheStat[i][n].first;
            for (auto it = tree.treeStat.begin(); it != tree.treeStat.end(); it++) {
                if (it->first == value) {
                    if (it->second + tree.cacheStat[i][n].second >= tree.cacheCount[i] - tree.cacheStat[i][n].second) {
                        // use the 'normal' leaf instead of the cache
                        it->second += tree.cacheStat[i][n].second;
                        tree.cacheCount[i] -= tree.cacheStat[i][n].second;
                        tree.cacheStat[i].removeAt(n);
                        n = -1;
                        i = 0;
                    }
                    break;
                }
            }
        }
    }
    // convert cache values to normal values
    bool hasEntries = !tree.treeStat.isEmpty();
    int dummyLeaf = UINT16_MAX + 1;
    for (int i = 0; i < 3; i++) {
        bool hasEntry = tree.cacheCount[i] != 0;
        hasEntries |= hasEntry;
        unsigned n;
        if (!hasEntry && dummyLeaf != UINT16_MAX + 1) {
            n = dummyLeaf; // reuse previous dummy entry
        } else {
            for (n = 0; n < UINT16_MAX; n++) {
                if (n == dummyLeaf) {
                    continue;
                }
                auto it = tree.treeStat.begin();
                for ( ; it != tree.treeStat.end(); it++) {
                    if (it->first == n) {
                        break;
                    }
                }
                // auto it = std::find_if(tree.treeStat.begin(), tree.treeStat.end(), [n](const QPair<unsigned, unsigned> &entry) { return entry.first == n; });
                if (it == tree.treeStat.end()) {
                    break;
                }
            }
            if (n >= UINT16_MAX) {
                dProgressFail() << QApplication::tr("Congratulation, you managed to break SMK.");
            }
        }
        if (hasEntry) {
            tree.treeStat.push_back(QPair<unsigned, unsigned>(n, tree.cacheCount[i]));
        } else {
            dummyLeaf = n;
        }
        tree.cacheCount[i] = n; // replace cache-count with the 'fake' leaf value
    }

    if (!hasEntries) {
        // empty tree
        // add open/close bits
        uint8_t* res = &treeData[cursor];
        res = writeNBits(0, 2, res, bitNum);
        cursor = (size_t)res - (size_t)treeData;
        tree.uncompressedTreeSize = 0;
        return;
    }
    // sort treeStat
    std::sort(tree.treeStat.begin(), tree.treeStat.end(), leafSorter);
    // prepare the sub-trees
    QList<QPair<unsigned, unsigned>> lowByteLeafs, hiByteLeafs;
    for (QPair<unsigned, unsigned> &leaf : tree.treeStat) {
        unsigned lowByte = leaf.first & 0xFF;
        unsigned hiByte = (leaf.first >> 8) & 0xFF;

        addTreeValue(lowByte, lowByteLeafs);
        addTreeValue(hiByte, hiByteLeafs);
    }

    uint8_t* res = &treeData[cursor];
    { // start the main tree
        res = writeBit(1, res, bitNum);
    }
    QMap<unsigned, QPair<unsigned, uint32_t>> bytePaths[2];
    unsigned joints;
    {
        // start the low sub-tree
        res = writeBit(1, res, bitNum);
        // add the low sub-tree
        // joints = 0;
        res = writeTree(lowByteLeafs, res, bitNum, joints, bytePaths[0], nullptr);
        // close the low-sub-tree
        res = writeBit(0, res, bitNum);
    }
    {
        // start the hi sub-tree
        res = writeBit(1, res, bitNum);
        // add the hi sub-tree
        // joints = 0;
        res = writeTree(hiByteLeafs, res, bitNum, joints, bytePaths[1], nullptr);
        // close the hi-sub-tree
        res = writeBit(0, res, bitNum);
    }
    {
        // add the cache values
        for (int i = 0; i < 3; i++) {
            res = writeNBits(tree.cacheCount[i], 16, res, bitNum);
        }
    }
    {
        // add the main tree
        joints = 0;
        res = writeTree(tree.treeStat, res, bitNum, joints, tree.paths, bytePaths);
        // close the main tree
        res = writeBit(0, res, bitNum);
    }

    tree.uncompressedTreeSize = (joints + 3) * 4;

    cursor = (size_t)res - (size_t)treeData;
}

static uint8_t *writeTreeValue(unsigned value, const SmkTreeInfo &videoTree, unsigned (&cacheValues)[3], uint8_t *cursor, unsigned &bitNum)
{
    unsigned v = value;
    for (int i = 0; i < 3; i++) {
        if (cacheValues[i] == value) {
            // check if the value was not removed from the cache (in prepareVideoTree)
            for (const QPair<unsigned, unsigned> &entry : videoTree.cacheStat[i]) {
                if (entry.first == value) {
                    v = videoTree.cacheCount[i]; // use the 'fake' leaf value
                }
            }
            break;
        }
    }

    if (cacheValues[0] != value) {
        cacheValues[2] = cacheValues[1];
        cacheValues[1] = cacheValues[0];
        cacheValues[0] = value;
    }

    auto it = videoTree.paths.find(v);
    if (it == videoTree.paths.end()) {
        dProgressErr() << QApplication::tr("ERROR: writeTreeValue missing entry %1 (%2)").arg(v).arg(value);
        return cursor;
    }

    QPair<unsigned, uint32_t> theEntryPair = it.value();
    return writeNBits(theEntryPair.second, theEntryPair.first, cursor, bitNum);
}

static void encodePixels(int x, int y, D1GfxFrame *frame, int type, int typelen, const SmkTreeInfo (&videoTree)[SMK_TREE_COUNT], 
    unsigned (&cacheValues)[SMK_TREE_COUNT][3], uint8_t *frameData, size_t &cursor, unsigned &bitNum)
{
    int width = frame->getWidth();
    int dy = (typelen * 4) / width;
    int dx = (typelen * 4) % width;
    x -= dx;
    if (x < 0) {
        x += width;
        dy++;
    }
    y -= dy * 4;
    uint8_t *res = &frameData[cursor];
    if (typelen != 0) {
        for (int i = lengthof(sizetable) - 1; /*i >= 0*/; ) {
            if (sizetable[i] <= typelen) {
                res = writeTreeValue(type | i << 2, videoTree[SMK_TREE_TYPE], cacheValues[SMK_TREE_TYPE], res, bitNum);

                for (int n = 0; n < sizetable[i]; n++) {
                    switch (type & 3) {
                    case 0: { // 2COLOR BLOCK  SMK_TREE_MMAP/SMK_TREE_MCLR
                        unsigned color1, color2, colors = 0;
                        color1 = frame->getPixel(x + 0, y + 0).getPaletteIndex();
                        for (int yy = 4 - 1; yy >= 0; yy--) {
                            for (int xx = 4 - 1; xx >= 0; xx--) {
                                colors <<= 1;
                                unsigned color = frame->getPixel(x + xx, y + yy).getPaletteIndex();
                                if (color == color1) {
                                    colors |= 1;
                                } else {
                                    color2 = color;
                                }
                            }
                        }
                        res = writeTreeValue(color1 << 8 | color2, videoTree[SMK_TREE_MCLR], cacheValues[SMK_TREE_MCLR], res, bitNum);
                        res = writeTreeValue(colors, videoTree[SMK_TREE_MMAP], cacheValues[SMK_TREE_MMAP], res, bitNum);
                    } break;
                    case 1: { // FULL BLOCK  SMK_TREE_FULL
                        unsigned color1, color2;
                        for (int yy = 0; yy < 4; yy++) {
                            color1 = frame->getPixel(x + 2, y + yy).getPaletteIndex();
                            color2 = frame->getPixel(x + 3, y + yy).getPaletteIndex();
                            res = writeTreeValue(color2 << 8 | color1, videoTree[SMK_TREE_FULL], cacheValues[SMK_TREE_FULL], res, bitNum);
                            color1 = frame->getPixel(x + 0, y + yy).getPaletteIndex();
                            color2 = frame->getPixel(x + 1, y + yy).getPaletteIndex();
                            res = writeTreeValue(color2 << 8 | color1, videoTree[SMK_TREE_FULL], cacheValues[SMK_TREE_FULL], res, bitNum);
                        }
                    } break;
                    case 2: // VOID BLOCK
                    case 3: // SOLID BLOCK
                        break;
                    default:
                        ASSUME_UNREACHABLE
                    }

                    x += 4;
                    if (x == width) {
                        x = 0;
                        y += 4;
                    }
                }
                
                typelen -= sizetable[i];
                if (typelen == 0) {
                    break;
                }
            } else {
                i--;
            }
        }
    }

    cursor = (size_t)res - (size_t)frameData;
}

static unsigned char oldPalette[D1SMK_COLORS][3];
static unsigned encodePalette(D1Pal *pal, int frameNum, bool palUse[D1SMK_COLORS], uint8_t *dest)
{
    // convert pal to smk-palette
    unsigned char newPalette[D1SMK_COLORS][3];
    static_assert(D1PAL_COLORS == D1SMK_COLORS, "encodePalette conversion from D1PAL to SMK_PAL must be adjusted.");
    for (int i = 0; i < D1PAL_COLORS; i++) {
        QColor col = pal->getColor(i);
        // assert(col->getAlpha() == 255);
        newPalette[i][0] = col.red();
        newPalette[i][1] = col.green();
        newPalette[i][2] = col.blue();
        for (int n = 0; n < 3; n ++) {
            unsigned char cv = newPalette[i][n];
            const unsigned char *p = &palmap[0];
            for ( ; /*p < lengthof(palmap)*/; p++) {
                if (cv <= p[0]) {
                    if (cv != p[0]) {
                        if (p[0] - cv > cv - p[-1]) {
                            p--;
                        }
                        const char *compontent[3] = { "red", "green", "blue"};
                        dProgressWarn() << QApplication::tr("Could not find matching color value for the %1 component of color %2 in the palette of frame %3. Using %4 instead of %5.").arg(compontent[n]).arg(i).arg(frameNum + 1).arg(*p).arg(cv);
                    }
                    break;
                }
            }
            newPalette[i][n] = (size_t)p - (size_t)&palmap[0];
        }
    }

    unsigned len;
    if (frameNum != 0) {
        // prepare statistic
        typedef struct _SmkPalStat {
            int directMatch;
            int bestMatch;
            int bestIdx;
        } SmkPalStat;
        SmkPalStat palStat[D1SMK_COLORS];

        for (int i = 0; i < D1PAL_COLORS; i++) {
            int best = -1, bestIdx, direct;
            for (int n = 0; n < D1PAL_COLORS; n++) {
                int curr = 0;
                for (int k = n; k < D1PAL_COLORS; k++, curr++) {
                    if (!palUse[i + curr]) {
                        continue; // destination color is unused -> ok
                    }
                    if (!palUse[k]) {
                        break; // source color is unused while the destination is used -> stop
                    }
                    if (oldPalette[k][0] != newPalette[i + curr][0] || oldPalette[k][1] != newPalette[i + curr][1] || oldPalette[k][2] != newPalette[i + curr][2]) {
                        break; // mismatching colors -> stop
                    }
                }
                if (curr > best) {
                    best = curr;
                    bestIdx = n;
                }
                if (n == i) {
                    direct = curr;
                }
            }
            palStat[i].bestIdx = bestIdx;
            palStat[i].bestMatch = best;
            palStat[i].directMatch = direct;
        }
        // encode the smk-palette
        len = 0;
        for (int i = 0; i < D1PAL_COLORS; i++) {
            int direct = palStat[i].directMatch;
            int best = palStat[i].bestMatch;
            int bestIdx = palStat[i].bestIdx;
            if (direct != 0) {
                if (direct > 64    // - the range is long for copy block
                 || best <= direct // - the copy block is not longer than the skip block
                 || (best / 64) > (best - direct) / 64) { // - placing the skip block reduces the number of copy blocks
                    // 0x80: Skip block(s) -> (n + 127) / 128
                    int dn = direct;
                    // - place the whole range
                    if (dn > 128) {
                        dn -= 128;
                        dest[len] = 0x80 | 127;
                        len++;
                    }
                    dest[len] = 0x80 | (dn - 1);
                    len++;

                    i += direct - 1;
                    continue;
                }
                // prefer longer copy range
            }
            if (best > 0) {
                // 0x40: Color-shift block -> ((n + 63) / 64) * 2
                int dn = best;
                // limit range if there is a skip block ahead
                for (int n = i + direct + 1; n < i + best && dn > 64; n++) {
                    int sb = palStat[n].directMatch;
                    if (sb + n > i + best || sb > 64) {
                        // cut copy block to prefer an upcoming skip block
                        dn = best = n - i;
                        break;
                    }
                }
                // place the selected range
                while (dn > 64) {
                    dn -= 64;
                    dest[len] = 0x40 | 63;
                    len++;
                    dest[len] = bestIdx;
                    len++;
                    bestIdx += 64;
                }
                dest[len] = 0x40 | (dn - 1);
                len++;
                dest[len] = bestIdx;
                len++;

                i += best - 1;
                continue;
            }
            // 0x00: Set Color block
            dest[len] = newPalette[i][0];
            len++;
            dest[len] = newPalette[i][1];
            len++;
            dest[len] = newPalette[i][2];
            len++;
        }
    } else {
        len = 0;
        for (int i = 0; i < D1PAL_COLORS; i++) {
            int direct;
            if (palUse[i]) {
                // 0x00: Set Color block
                dest[len] = newPalette[i][0];
                len++;
                dest[len] = newPalette[i][1];
                len++;
                dest[len] = newPalette[i][2];
                len++;
            } else {
                // 0x80: Skip block(s)
                for (direct = 0; (i + direct) < D1PAL_COLORS; direct++) {
                    if (palUse[i + direct]) {
                        break;
                    }
                    // do not write garbage to the file even if it does not matter
                    /*newPalette[i][0] = 0;
                    newPalette[i][1] = 0;
                    newPalette[i][2] = 0;*/
                }
                int dn = direct;
                // - place the whole range
                if (dn > 128) {
                    dn -= 128;
                    dest[len] = 0x80 | 127;
                    len++;
                }
                dest[len] = 0x80 | (dn - 1);
                len++;

                i += direct - 1;
            }
        }
    }

    memcpy(oldPalette, newPalette, sizeof(newPalette));
    return len;
}

static uint8_t *writeTreeValue(unsigned v, const QMap<unsigned, QPair<unsigned, uint32_t>> &bytePaths, uint8_t *cursor, unsigned &bitNum)
{
    auto it = bytePaths.find(v);
    if (it == bytePaths.end()) {
        dProgressErr() << QApplication::tr("ERROR: writeTreeValue missing entry %1 in byte paths.").arg(v);
        return cursor;
    }

    QPair<unsigned, uint32_t> theEntryPair = it.value();
    return writeNBits(theEntryPair.second, theEntryPair.first, cursor, bitNum);
}

static size_t encodeAudio(uint8_t *audioData, size_t len, const SmkAudioInfo &audioInfo, uint8_t *cursor)
{
    unsigned bitNum = 0;
    uint8_t *res = cursor;
    // assert(len != 0);
    if (!audioInfo.compress) {
        memcpy(cursor, audioData, len);
        res += len;
    } else {
        uint8_t *audioDataEnd = &audioData[len];
        unsigned dw;
        int channels = audioInfo.channels;
        int bitdepth = audioInfo.bitDepth;
        // assert((len % (channels * bitdepth / 8)) == 0);
        // assert(channels < D1SMK_CHANNELS);
        // assert(bitdepth == 8 || bitdepth == 16);
        *((uint32_t*)res) = SwapLE32(len);
        res += 4;

        res = writeBit(1, res, bitNum);

        dw = channels == 2 ? 2 : 1;
        res = writeBit(channels == 2 ? 1 : 0, res, bitNum);

        dw *= bitdepth == 16 ? 2 : 1;
        res = writeBit(bitdepth == 16 ? 1 : 0, res, bitNum);
        /* prepare the trees */
        QList<QPair<unsigned, unsigned>> audioBytes[4];
        QMap<unsigned, QPair<unsigned, uint32_t>> bytePaths[4];
        uint8_t *audioCursor = &audioData[dw];
        while (audioCursor < audioDataEnd) {
            if (bitdepth == 16) {
                int16_t dv = ((int16_t *)audioCursor)[0] - ((int16_t *)audioCursor)[ - channels];
                addTreeValue(dv & 0xFF, audioBytes[0]);
                addTreeValue((dv >> 8) & 0xFF, audioBytes[1]);
                audioCursor += 2;
            } else {
                int8_t dv = ((int8_t *)audioCursor)[0] - ((int8_t *)audioCursor)[ - channels];
                addTreeValue(dv, audioBytes[0]);
                audioCursor++;
            }

            if (channels == 2) {
                if (bitdepth == 16) {
                    int16_t dv = ((int16_t *)audioCursor)[0] - ((int16_t *)audioCursor)[ - 2];
                    addTreeValue(dv & 0xFF, audioBytes[2]);
                    addTreeValue((dv >> 8) & 0xFF, audioBytes[3]);
                    audioCursor += 2;
                } else {
                    int8_t dv = ((int8_t *)audioCursor)[0] - ((int8_t *)audioCursor)[ - 2];
                    addTreeValue(dv, audioBytes[2]);
                    audioCursor++;
                }
            }
        }
        /* write the trees */
        for (int i = 0; i < 4; i++) {
            if (!audioBytes[i].isEmpty()) {
                unsigned joints;
                std::sort(audioBytes[i].begin(), audioBytes[i].end(), leafSorter);
                // start the sub-tree
                res = writeBit(1, res, bitNum);
                // add the sub-tree
                res = writeTree(audioBytes[i], res, bitNum, joints, bytePaths[i], nullptr);
                // close the sub-tree
                res = writeBit(0, res, bitNum);
            }
        }

        /* write initial sound level */
        if (channels == 2) {
            if (bitdepth == 16) {
                res = writeNBits(audioData[3], 8, res, bitNum);
                res = writeNBits(audioData[2], 8, res, bitNum);
            } else {
                res = writeNBits(audioData[1], 8, res, bitNum);
            }
        }

        if (bitdepth == 16) {
            res = writeNBits(audioData[1], 8, res, bitNum);
            res = writeNBits(audioData[0], 8, res, bitNum);
        } else {
            res = writeNBits(audioData[0], 8, res, bitNum);
        }

        audioData += dw;
        /* write the rest of the data */
        while (audioData != audioDataEnd) {
            if (bitdepth == 16) {
                int16_t dv = ((int16_t *)audioData)[0] - ((int16_t *)audioData)[ - channels];
                res = writeTreeValue(dv & 0xFF, bytePaths[0], res, bitNum);
                res = writeTreeValue((dv >> 8) & 0xFF, bytePaths[1], res, bitNum);
                audioData += 2;
            } else {
                int8_t dv = ((int8_t *)audioData)[0] - ((int8_t *)audioData)[ - channels];
                res = writeTreeValue(dv, bytePaths[0], res, bitNum);
                audioData++;
            }

            if (channels == 2) {
                if (bitdepth == 16) {
                    int16_t dv = ((int16_t *)audioData)[0] - ((int16_t *)audioData)[ - 2];
                    res = writeTreeValue(dv & 0xFF, bytePaths[2], res, bitNum);
                    res = writeTreeValue((dv >> 8) & 0xFF, bytePaths[3], res, bitNum);
                    audioData += 2;
                } else {
                    int8_t dv = ((int8_t *)audioData)[0] - ((int8_t *)audioData)[ - 2];
                    res = writeTreeValue(dv, bytePaths[2], res, bitNum);
                    audioData++;
                }
            }
        }

        if (bitNum != 0) {
            res++;
        }
    }

    return (size_t)res - (size_t)cursor;
}

bool D1Smk::save(D1Gfx &gfx, const SaveAsParam &params)
{
    // validate the content
    int width = 0, height = 0;
    int frameCount = gfx.getFrameCount();
    bool palUse[D1PAL_COLORS] = { 0 };
    for (int i = 0; i < frameCount; i++) {
        D1GfxFrame *frame = gfx.getFrame(i);
        if (i == 0) {
            width = frame->getWidth();
            height = frame->getHeight();
            if ((width & 3) || (height & 3)) {
                dProgressErr() << QApplication::tr("SMK requires width/height to be multiple of 4.").arg(i + 1);
                return false;
            }
        } else if (width != frame->getWidth() || height != frame->getHeight()) {
            dProgressErr() << QApplication::tr("Mismatching framesize (%1).").arg(i + 1);
            return false;
        }
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                D1GfxPixel pixel = frame->getPixel(x, y);
                if (pixel.isTransparent()) {
                    dProgressErr() << QApplication::tr("Transparent pixel in frame %1. at %2:%3").arg(i + 1).arg(x).arg(y);
                    return false;
                }
                palUse[pixel.getPaletteIndex()] = true;
            }
        }
    }

    QList<SmkFrameInfo> frameInfo;
    for (int n = 0; n < frameCount; n++) {
        uint8_t frameType = 0;
        D1GfxFrame *frame = gfx.getFrame(n);
        if (!frame->getFramePal().isNull()) {
            frameType |= 1;
        }
        D1SmkAudioData *audioData = frame->getFrameAudio();
        if (audioData != nullptr) {
            for (int i = 0; i < D1SMK_TRACKS; i++) {
                unsigned long len;
                audioData->getAudio(i, &len);
                if (len != 0) {
                    frameType |= 2 << i;
                }
            }
        }

        SmkFrameInfo fi;
        fi.FrameType = frameType;
        fi.FrameData = nullptr;
        fi.FrameSize = 0;
        frameInfo.push_back(fi);
    }

    SmkAudioInfo audioInfo[D1SMK_TRACKS];
    for (int i = 0; i < D1SMK_TRACKS; i++) {
        unsigned bitRate = 0;
        unsigned bitDepth = 0;
        unsigned channels = 0;
        unsigned maxChunkLength = 0;
        uint8_t compress = 0;
        bool first = true;
        for (int n = 0; n < frameCount; n++) {
            D1GfxFrame *frame = gfx.getFrame(n);
            D1SmkAudioData *audioData = frame->getFrameAudio();
            if (audioData != nullptr) {
                unsigned long len;
                audioData->getAudio(i, &len);
                if (len == 0) {
                    continue;
                }
                frameInfo[n].FrameType |= 2 << i;
                if (len > UINT32_MAX) {
                    dProgressErr() << QApplication::tr("Audio chunk of frame %1 (track %2) is too large (%3 Max. %4).").arg(n + 1).arg(i + 1).arg(len).arg(UINT32_MAX);
                    return false;
                }
                if (len > maxChunkLength) {
                    maxChunkLength = len;
                }
                // assert(len % (audioData->channels * audioData->bitDepth / 8));
                if (!first) {
                    if (compress != audioData->compress[i]) {
                        dProgressErr() << QApplication::tr("Audio chunk of frame %1 (track %2) has mismatching compression setting (%3 vs %4).").arg(n + 1).arg(i + 1).arg(audioData->compress[i]).arg(compress);
                        return false;
                    }
                    if (bitDepth != audioData->bitDepth) {
                        dProgressErr() << QApplication::tr("Audio chunk of frame %1 (track %2) has mismatching sample size (%3 vs %4).").arg(n + 1).arg(i + 1).arg(audioData->bitDepth).arg(bitDepth);
                        return false;
                    }
                    if (channels != audioData->channels) {
                        dProgressErr() << QApplication::tr("Audio chunk of frame %1 (track %2) has mismatching channel-count (%3 vs %4).").arg(n + 1).arg(i + 1).arg(audioData->channels).arg(channels);
                        return false;
                    }
                    if (bitRate != audioData->bitRate) {
                        dProgressErr() << QApplication::tr("Audio chunk of frame %1 (track %2) has mismatching bitrate (%3 vs %4).").arg(n + 1).arg(i + 1).arg(audioData->bitRate).arg(bitRate);
                        return false;
                    }
                } else {
                    first = false;
                    compress = audioData->compress[i];
                    bitDepth = audioData->bitDepth;
                    channels = audioData->channels;
                    bitRate = audioData->bitRate;
                    if (compress != 0 && compress != 1) {
                        dProgressErr() << QApplication::tr("Compression mode of the audio chunk of frame %1 (track %2) is not supported (%3 Must be 0 or 1).").arg(n + 1).arg(i + 1).arg(compress);
                        return false;
                    }
                    if (bitDepth != 8 && bitDepth != 16) {
                        dProgressErr() << QApplication::tr("Sample size of the audio chunk of frame %1 (track %2) is incompatible with SMK (%3 Must be 8 or 16).").arg(n + 1).arg(i + 1).arg(bitDepth);
                        return false;
                    }
                    if (channels != 1 && channels != 2) {
                        dProgressErr() << QApplication::tr("Channel-count of the audio chunk of frame %1 (track %2) is incompatible with SMK (%3 Must be 1 or 2).").arg(n + 1).arg(i + 1).arg(channels);
                        return false;
                    }
                    if (bitRate == 0 || bitRate > 0x00FFFFFF) {
                        dProgressErr() << QApplication::tr("Bitrate of the audio chunk of frame %1 (track %2) is incompatible with SMK (%3 Must be 1-%4).").arg(n + 1).arg(i + 1).arg(bitRate).arg(0x00FFFFFF);
                        return false;
                    }
                }
            }
        }

        audioInfo[i].bitRate = bitRate;
        audioInfo[i].bitDepth = bitDepth;
        audioInfo[i].channels = channels;
        audioInfo[i].maxChunkLength = maxChunkLength;
        audioInfo[i].compress = compress;
    }

    int frameLen = gfx.getFrameLen(); 
    if (frameLen == 0) {
        dProgressWarn() << QApplication::tr("Frame length is not set. Defaulting to 100000us.");
    } else if (frameLen % 1000 == 0) {
        frameLen = frameLen / 1000;
    } else {
        if (frameLen % 10 != 0) {
            dProgressWarn() << QApplication::tr("Frame length is rounded to %1.").arg((frameLen / 10) * 10);
        }
        frameLen = - (frameLen / 10);
    }

    QString filePath = gfx.gfxFilePath;
    if (!params.celFilePath.isEmpty()) {
        filePath = params.celFilePath;
        if (!params.autoOverwrite && QFile::exists(filePath)) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, QApplication::tr("Confirmation"), QApplication::tr("Are you sure you want to overwrite %1?").arg(QDir::toNativeSeparators(filePath)), QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                return false;
            }
        }
    } else if (!gfx.isModified()) {
        return false;
    }

    if (filePath.isEmpty()) {
        return false;
    }

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile outFile = QFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        dProgressFail() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }

    // prepare the trees
    SmkTreeInfo videoTree[SMK_TREE_COUNT];
    for (int i = 0; i < SMK_TREE_COUNT; i++) {
        memset(videoTree[i].cacheCount, 0, sizeof(videoTree[i].cacheCount));
    }
    D1GfxFrame *prevFrame = nullptr;
    for (int n = 0; n < frameCount; n++) {
        D1GfxFrame *frame = gfx.getFrame(n);
        unsigned cacheValues[SMK_TREE_COUNT][3] = { 0 };
        int type = -1; unsigned typelen = 0;
        for (int y = 0; y < height; y += 4) {
            for (int x = 0; x < width; x += 4) {
                int ctype = 1;
                if (prevFrame != nullptr) {
                    int yy = 0;
                    for ( ; yy < 4; yy++) {
                        for (int xx = 0; xx < 4; xx++) {
                            if (prevFrame->getPixel(x + xx, y + yy) != frame->getPixel(x + xx, y + yy)) {
                                yy = 10;
                                break;
                            }
                        }
                    }
                    if (yy != 10 + 1) {
                        ctype = 2; // VOID BLOCK -> skip
                    }
                }
                if (ctype == 1) {
                    unsigned numColors = 1, color1, color2 = D1PAL_COLORS, colors = 0;
                    color1 = frame->getPixel(x + 0, y + 0).getPaletteIndex();
                    for (int yy = 4 - 1; yy >= 0 && numColors <= 2; yy--) {
                        for (int xx = 4 - 1; xx >= 0; xx--) {
                            colors <<= 1;
                            unsigned color = frame->getPixel(x + xx, y + yy).getPaletteIndex();
                            if (color == color1) {
                                colors |= 1;
                                continue;
                            }
                            if (color == color2) {
                                continue;
                            }
                            ++numColors;
                            if (numColors == 2) {
                                color2 = color;
                                continue;
                            }
                            break;
                        }
                    }
                    if (numColors <= 2) {
                        if (numColors == 2) {
                            // 2COLOR BLOCK -> SMK_TREE_MMAP/SMK_TREE_MCLR
                            addTreeValue(color1 << 8 | color2, videoTree[SMK_TREE_MCLR], cacheValues[SMK_TREE_MCLR]);
                            addTreeValue(colors, videoTree[SMK_TREE_MMAP], cacheValues[SMK_TREE_MMAP]);
                            ctype = 0;
                        } else {
                            // SOLID BLOCK
                            ctype = 3 | (color1 << 8);
                        }
                    }
                }
                if (ctype == 1) {
                    // FULL BLOCK -> SMK_TREE_FULL
                    unsigned color1, color2;
                    for (int yy = 0; yy < 4; yy++) {
                        color1 = frame->getPixel(x + 2, y + yy).getPaletteIndex();
                        color2 = frame->getPixel(x + 3, y + yy).getPaletteIndex();
                        addTreeValue(color2 << 8 | color1, videoTree[SMK_TREE_FULL], cacheValues[SMK_TREE_FULL]);
                        color1 = frame->getPixel(x + 0, y + yy).getPaletteIndex();
                        color2 = frame->getPixel(x + 1, y + yy).getPaletteIndex();
                        addTreeValue(color2 << 8 | color1, videoTree[SMK_TREE_FULL], cacheValues[SMK_TREE_FULL]);
                    }
                    // ctype = 1;
                }
                if (type != ctype) {
                    addTreeTypeValue(type, typelen, videoTree[SMK_TREE_TYPE], cacheValues[SMK_TREE_TYPE]);
                    type = ctype;
                    typelen = 0;
                }
                typelen++;
            }
        }
        addTreeTypeValue(type, typelen, videoTree[SMK_TREE_TYPE], cacheValues[SMK_TREE_TYPE]);
        prevFrame = frame;
    }
    size_t allocSize = 1 + (1 + (UINT8_MAX + 1) * 1 * 2 * 8 + 1) + (1 + (UINT8_MAX + 1) * 1 * 2 * 8 + 1) + 3 * sizeof(uint16_t) * 8 + ((UINT16_MAX + 1) * sizeof(uint16_t) * 2 * 8) + 1;
    allocSize *= 4;
    allocSize = (allocSize + 7) / 8;
    uint8_t *videoTreeData = (uint8_t *)calloc(1, allocSize);
    size_t cursor = 0; unsigned bitNum = 0;
    for (int i = 0; i < SMK_TREE_COUNT; i++) {
        prepareVideoTree(videoTree[i], videoTreeData, cursor, bitNum);
    }
    unsigned videoTreeDataSize = cursor;
    if (bitNum != 0) {
        videoTreeDataSize++;
    }
    // write the header to the file
    smk_header header;
    header.SmkMarker[0] = 'S'; header.SmkMarker[1] = 'M'; header.SmkMarker[2] = 'K'; header.SmkMarker[3] = '2';
    header.VideoWidth = SwapLE32(width);
    header.VideoHeight = SwapLE32(height);
    header.FrameCount = SwapLE32(frameCount);
    header.FrameLen = SwapLE32(frameLen);
    header.VideoTreeDataSize = SwapLE32(videoTreeDataSize);
    header.VideoFlags = 0;
    header.Dummy = 0;

    for (int i = 0; i < SMK_TREE_COUNT; i++) {
        header.VideoTreeSize[i] = SwapLE32(videoTree[i].uncompressedTreeSize);
    }
    unsigned maxAudioLength = 0;
    for (int i = 0; i < D1SMK_TRACKS; i++) {
        unsigned maxChunkLength = audioInfo[i].maxChunkLength;
        unsigned audioFlags = 0;
        if (maxChunkLength != 0) {
            maxAudioLength += maxChunkLength;

            audioFlags = 0x40000000 | audioInfo[i].bitRate;
            if (audioInfo[i].compress) {
                audioFlags |= 0x80000000;
            }
            if (audioInfo[i].bitDepth == 16) {
                audioFlags |= 0x20000000;
            }
            if (audioInfo[i].channels == 2) {
                audioFlags |= 0x20000000;
            }
        }
        header.AudioMaxChunkLength[i] = SwapLE32(maxChunkLength);
        header.AudioType[i] = SwapLE32(audioFlags);
    }
    // add space for the framesizes
    outFile.write((const char*)&header, sizeof(header));
    for (int i = 0; i < frameCount; i++) {
        outFile.write((const char*)&header.Dummy, 4);
    }
    // write the frame-types
    for (int i = 0; i < frameCount; i++) {
        outFile.write((const char*)&frameInfo[i].FrameType, 1);
    }
    // write the trees
    outFile.write((const char*)videoTreeData, videoTreeDataSize);
    // write the content of the frames
    /*D1GfxFrame **/ prevFrame = nullptr;
    uint8_t *frameData = (uint8_t*)calloc(1, D1SMK_COLORS * 3 + maxAudioLength + 4 * width * height);
    QList<uint32_t> frameLengths;
    for (int n = 0; n < frameCount; n++) {
        D1GfxFrame *frame = gfx.getFrame(n);
        // reset pointers of the work-buffer
        size_t cursor = 0; unsigned bitNum = 0;
        // add optional palette
        D1Pal *framePal = frame->getFramePal().data();
        if (framePal == nullptr && n == 0) {
            dProgressWarn() << QApplication::tr("The palette is not set in the first frame. Defaulting to the current palette.");
            framePal = gfx.getPalette();
        }
        if (framePal != nullptr) {
            unsigned pallen = encodePalette(framePal, n, palUse, frameData + cursor + 1);
            pallen = (pallen + 1 + 3) / 4;
            *&frameData[cursor] = pallen;
            cursor += pallen * 4;
        }
        // add optional audio
        D1SmkAudioData *audioData = frame->getFrameAudio();
        if (audioData != nullptr) {
            for (int i = 0; i < D1SMK_TRACKS; i++) {
                unsigned long length;
                uint8_t *data = audioData->getAudio(i, &length);
                if (length != 0) {
                    // assert(frameInfo[i].FrameType & (0x02 << track));
                    size_t audiolen = encodeAudio(data, length, audioInfo[i], frameData + cursor + 4);
                    audiolen += 4;
                    *(uint32_t*)&frameData[cursor] = SwapLE32(audiolen);
                    cursor += audiolen;
                }
            }
        }
        // add the pixels
        unsigned cacheValues[SMK_TREE_COUNT][3] = { 0 };
        int type = -1; unsigned typelen = 0;
        for (int y = 0; y < height; y += 4) {
            for (int x = 0; x < width; x += 4) {
                int ctype = 1;
                if (prevFrame != nullptr) {
                    int yy = 0;
                    for ( ; yy < 4; yy++) {
                        for (int xx = 0; xx < 4; xx++) {
                            if (prevFrame->getPixel(x + xx, y + yy) != frame->getPixel(x + xx, y + yy)) {
                                yy = 10;
                                break;
                            }
                        }
                    }
                    if (yy != 10 + 1) {
                        ctype = 2; // VOID BLOCK -> skip
                    }
                }
                if (ctype == 1) {
                    unsigned numColors = 1, color1, color2 = D1PAL_COLORS;
                    color1 = frame->getPixel(x + 0, y + 0).getPaletteIndex();
                    for (int yy = 4 - 1; yy >= 0 && numColors <= 2; yy--) {
                        for (int xx = 4 - 1; xx >= 0; xx--) {
                            unsigned color = frame->getPixel(x + xx, y + yy).getPaletteIndex();
                            if (color == color1) {
                                continue;
                            }
                            if (color == color2) {
                                continue;
                            }
                            ++numColors;
                            if (numColors == 2) {
                                color2 = color;
                                continue;
                            }
                            break;
                        }
                    }
                    if (numColors <= 2) {
                        if (numColors == 2) {
                            // 2COLOR BLOCK -> SMK_TREE_MMAP/SMK_TREE_MCLR
                            ctype = 0;
                        } else {
                            // SOLID BLOCK
                            ctype = 3 | (color1 << 8);
                        }
                    }
                }
                if (ctype == 1) {
                    // FULL BLOCK -> SMK_TREE_FULL
                    // ctype = 1;
                }
                if (type != ctype) {
                    encodePixels(x, y, frame, type, typelen, videoTree, cacheValues, frameData, cursor, bitNum);

                    type = ctype;
                    typelen = 0;
                }
                typelen++;
            }
        }
        encodePixels(0, height, frame, type, typelen, videoTree, cacheValues, frameData, cursor, bitNum);
        if (bitNum != 0) {
            cursor++;
        }
        cursor = (cursor + 3) & ~3;

        outFile.write((const char*)frameData, cursor);
        memset(frameData, 0, cursor);
        frameLengths.push_back(cursor);

        prevFrame = frame;
    }

    free(frameData);

    outFile.seek(sizeof(header));
    for (uint32_t chunkSize : frameLengths) {
        chunkSize = SwapLE32(chunkSize);
        outFile.write((const char*)&chunkSize, 4);
    }

    gfx.gfxFilePath = filePath; //  D1Smk::load(gfx, filePath);
    gfx.modified = false;
    return true;
}

static void reportDiff(const QString text, QString &header)
{
    if (!header.isEmpty()) {
        dProgress() << header;
        header.clear();
    }
    dProgress() << text;
}
void D1Smk::compare(D1Gfx &gfx, QMap<QString, D1Pal *> &pals, const LoadFileContent *fileContent, bool patchData)
{
    QString header = QApplication::tr("Content:");
    gfx.compareTo(fileContent->gfx, header, patchData);
    {
        header = QApplication::tr("Palettes:");
        const QMap<QString, D1Pal *> *palsB = &fileContent->pals;
        for (auto it = pals.begin(); it != pals.end(); it++) {
            if (palsB->contains(it.key())) {
                it.value()->compareTo((*palsB)[it.key()], header);
            } else {
                reportDiff(QApplication::tr("Palette '%1' is added.").arg(it.key()), header);
            }
        }
        for (auto it = palsB->begin(); it != palsB->end(); it++) {
            if (!pals.contains(it.key())) {
                reportDiff(QApplication::tr("Palette '%1' is removed.").arg(it.key()), header);
            }
        }
    }
}

static void audioCallback(int track, QAudio::State newState)
{
    if (newState == QAudio::IdleState) {   // finished playing (i.e., no more data)
        if (!smkAudioBuffer[track]->atEnd() && !audioSemaphore[track]) {
            audioSemaphore[track] = true;
            audioOutput[track]->start(smkAudioBuffer[track]);
            audioSemaphore[track] = false;
        }
    }
}

static void audioCallback0(QAudio::State newState)
{
    audioCallback(0, newState);
}
static void audioCallback1(QAudio::State newState)
{
    audioCallback(1, newState);
}
static void audioCallback2(QAudio::State newState)
{
    audioCallback(2, newState);
}
static void audioCallback3(QAudio::State newState)
{
    audioCallback(3, newState);
}
static void audioCallback4(QAudio::State newState)
{
    audioCallback(4, newState);
}
static void audioCallback5(QAudio::State newState)
{
    audioCallback(5, newState);
}
static void audioCallback6(QAudio::State newState)
{
    audioCallback(6, newState);
}
static void (*cbfunc[D1SMK_TRACKS])(QAudio::State) = { &audioCallback0, &audioCallback1, &audioCallback2, &audioCallback3, &audioCallback4, &audioCallback5, &audioCallback6 };

void D1Smk::playAudio(D1GfxFrame &gfxFrame, int trackIdx)
{
    D1SmkAudioData *frameAudio;
    unsigned long audioDataLen;
    uint8_t *audioData;
    unsigned channels, bitDepth, bitRate;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QAudioFormat::SampleFormat sampleFormat;
#endif
    frameAudio = gfxFrame.getFrameAudio();
    if (frameAudio != nullptr) {
        channels = frameAudio->getChannels();
        bitDepth = frameAudio->getBitDepth();
        bitRate = frameAudio->getBitRate();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        sampleFormat = bitDepth == 16 ? QAudioFormat::Int16 : QAudioFormat::UInt8;
#endif
        for (int track = 0; track < D1SMK_TRACKS; track++) {
            if (trackIdx >= 0 && track != trackIdx) {
                continue;
            }
            audioData = frameAudio->getAudio(track, &audioDataLen);
            if (audioDataLen == 0) {
                continue;
            }
            if (audioOutput[track] != nullptr) {
                QAudioFormat m_audioFormat = audioOutput[track]->format();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                if (m_audioFormat.sampleRate() != bitRate || m_audioFormat.sampleFormat() != sampleFormat || m_audioFormat.channelCount() != channels) {
#else
                if (m_audioFormat.sampleRate() != bitRate || m_audioFormat.sampleSize() != bitDepth || m_audioFormat.channelCount() != channels) {
#endif
                    audioOutput[track]->stop();
                    delete audioOutput[track];
                    audioOutput[track] = nullptr;
                    smkAudioBuffer[track]->clear();
                }
            }
            if (audioOutput[track] == nullptr) {
                QAudioFormat m_audioFormat = QAudioFormat();
                m_audioFormat.setSampleRate(bitRate);
                m_audioFormat.setChannelCount(channels);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                m_audioFormat.setSampleFormat(sampleFormat);
#else
                m_audioFormat.setSampleSize(bitDepth);
                m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
                m_audioFormat.setSampleType(QAudioFormat::SignedInt);
                m_audioFormat.setCodec("audio/pcm");
#endif
                audioOutput[track] = new SmkAudioOutput(m_audioFormat); // , this);
                // connect up signal stateChanged to a lambda to get feedback
                QObject::connect(audioOutput[track], &SmkAudioOutput::stateChanged, cbfunc[track]);

                if (smkAudioBuffer[track] == nullptr) {
                    smkAudioBuffer[track] = new AudioBuffer();
                    smkAudioBuffer[track]->open(QIODevice::ReadWrite | QIODevice::Unbuffered);
                }
            }
            smkAudioBuffer[track]->write((char *)audioData, audioDataLen);

            QAudio::State state = audioOutput[track]->state();
            if (state != QAudio::ActiveState) {
                audioCallback(track, QAudio::IdleState);
            }
        }
    }
}

void D1Smk::stopAudio()
{
    for (int track = 0; track < D1SMK_TRACKS; track++) {
        if (audioOutput[track] != nullptr) {
            audioOutput[track]->stop();
        }
        if (smkAudioBuffer[track] != nullptr) {
            smkAudioBuffer[track]->clear();
        }
    }
}

static QString addDetails(QString &msg, int verbose, D1SmkColorFix &fix)
{
    if (fix.frameFrom != 0 || fix.frameTo != fix.gfx->getFrameCount()) {
        msg = msg.append(QApplication::tr(" for frame(s) %1-%2", "", fix.frameTo - fix.frameFrom + 1).arg(fix.frameFrom + 1).arg(fix.frameTo + 1));
    }
    if (verbose) {
        QFileInfo fileInfo(fix.gfx->getFilePath());
        QString labelText = fileInfo.fileName();
        msg = msg.append(QApplication::tr(" of %1").arg(labelText));
    }
    msg.append(".");
    return msg;
}

static bool fixPalColors(D1SmkColorFix &fix, int verbose)
{
    if (fix.frameFrom == fix.frameTo) {
        return false;
    }
    D1Pal *pal = fix.pal;
    QColor undefColor = pal->getUndefinedColor();
    QList<quint8> ignored;
    bool result = false;
    for (unsigned i = 0; i < D1PAL_COLORS; i++) {
        QColor col = pal->getColor(i);
        if (col == undefColor) {
            ignored.push_back(i);
            continue;
        }
        unsigned char smkColor[3];
        smkColor[0] = col.red();
        smkColor[1] = col.green();
        smkColor[2] = col.blue();
        bool change = false;
        for (int n = 0; n < 3; n++) {
            unsigned char cv = smkColor[n];
            const unsigned char *p = &palmap[0];
            for ( ; /*p < lengthof(palmap)*/; p++) {
                if (cv <= p[0]) {
                    if (cv != p[0]) {
                        if (p[0] - cv > cv - p[-1]) {
                            p--;
                        }
                        const char *compontent[3] = { "red", "green", "blue" };
                        QString msg = QApplication::tr("The %1 component of color %2 is adjusted in the palette").arg(compontent[n]).arg(i);
                        msg = addDetails(msg, verbose, fix);
                        msg = msg.append(QApplication::tr(" (Using %1 instead of %2)").arg(*p).arg(cv));
                        dProgress() << msg;
                        if (n == 0) {
                            col.setRed(*p);
                        }
                        if (n == 1) {
                            col.setGreen(*p);
                        }
                        if (n == 2) {
                            col.setBlue(*p);
                        }
                        change = true;
                    }
                    break;
                }
            }
        }
        if (change) {
            // fix.colors.push_back(i);
            // find possible replacement for the modified color
            std::vector<PaletteColor> colors;
            pal->getValidColors(colors);
            for (const PaletteColor pc : colors) {
                if (col.red() == pc.red() && col.green() == pc.green() && col.blue() == pc.blue() && pc.index() != i) {
                    dProgress() << QApplication::tr("Using %1 instead of %2 in frames [%3..%4)").arg(pc.index()).arg(i).arg(fix.frameFrom).arg(fix.frameTo);
                    QList<QPair<D1GfxPixel, D1GfxPixel>> replacements;
                    replacements.push_back(QPair<D1GfxPixel, D1GfxPixel>(D1GfxPixel::colorPixel(i), D1GfxPixel::colorPixel(pc.index())));
                    RemapParam params;
                    params.frames = std::pair<int, int>(fix.frameFrom, fix.frameTo);
                    fix.gfx->replacePixels(replacements, params, verbose);

                    col = undefColor;
                    break;
                }
            }
            pal->setColor(i, col);

            result = true;
        }
    }
    QString ignoredColors;
    const int numIgnored = ignored.count();
    for (int n = 0; n < numIgnored; n++) {
        quint8 c0 = ignored[n];
        int i = n + 1;
        for ( ; i < ignored.count(); i++) {
            if (ignored[i] != c0 + i - n) {
                break;
            }
        }
        if (i >= n + 3) {
            ignoredColors.append(QString("%1-%2, ").arg(c0).arg(ignored[i - 1]));
            n = i - 1;
        } else {
            ignoredColors.append(QString("%1, ").arg(c0));
        }
    }
    if (numIgnored != 0) {
        ignoredColors.chop(2);
        QString msg = QApplication::tr("Ignored the %1 undefined color(s) in the palette", "", numIgnored).arg(ignoredColors);
        msg = addDetails(msg, verbose, fix);
        dProgressWarn() << msg;
    }
    // if (fix.colors.isEmpty()) {
    if (!result) {
        QString msg = QApplication::tr("The palette");
        msg = addDetails(msg, verbose, fix);
        msg.chop(1);
        msg.append(QApplication::tr(" is SMK compliant"));
        dProgress() << msg;
    }
    return result;
}

void D1Smk::fixColors(D1Gfxset *gfxSet, D1Gfx *g, D1Pal *p, QList<D1SmkColorFix> &frameColorMods)
{
    QList<D1Gfx *> gfxs;
    int verbose;
    if (gfxSet != nullptr) {
        verbose = 1;
        gfxs.append(gfxSet->getGfxList());
    } else {
        verbose = 0;
        gfxs.append(g);
    }

    ProgressDialog::incBar(QApplication::tr("Fixing graphics..."), gfxs.count() + 1);
    for (D1Gfx *gfx : gfxs) {
        // adjust colors of the palette(s)
        D1SmkColorFix cf;
        cf.pal = p;
        cf.gfx = gfx;
        cf.frameFrom = 0;
        int i = 0;
        ProgressDialog::incBar(QApplication::tr("Fixing frame..."), cf.gfx->getFrameCount() + 1);
        for ( ; i < cf.gfx->getFrameCount(); i++) {
            QPointer<D1Pal> &fp = cf.gfx->getFrame(i)->getFramePal();
            if (!fp.isNull()) {
                cf.frameTo = i;
                if (fixPalColors(cf, verbose)) {
                    frameColorMods.push_back(cf);
                    cf.colors.clear();
                }
                cf.frameFrom = i;
                cf.pal = fp.data();
            }
        }
        cf.frameTo = i;
        if (fixPalColors(cf, verbose)) {
            frameColorMods.push_back(cf);
            // cf.colors.clear();
        }
        // eliminate matching palettes
        D1Pal *pal = nullptr;
        for (int i = 0; i < cf.gfx->getFrameCount(); i++) {
            if (ProgressDialog::wasCanceled()) {
                break;
            }
            QPointer<D1Pal> &fp = cf.gfx->getFrame(i)->getFramePal();
            if (!fp.isNull()) {
                D1Pal *cp = fp.data();
                if (pal != nullptr) {
#if 1
                    std::vector<PaletteColor> currColors;
                    cp->getValidColors(currColors);

                    std::vector<PaletteColor> prevColors;
                    pal->getValidColors(prevColors);

                    if (prevColors.size() >= currColors.size()) {
                        // try to use the previous palette
                        unsigned iden = 0;
                        QList<QPair<D1GfxPixel, D1GfxPixel>> replacements;

                        for (const PaletteColor cc : currColors) {
                            for (const PaletteColor pc : prevColors) {
                                if (cc.red() == pc.red() && cc.green() == pc.green() && cc.blue() == pc.blue()) {
                                    if (cc.index() == pc.index()) {
                                        iden++;
                                    } else {
                                        replacements.push_back(QPair<D1GfxPixel, D1GfxPixel>(D1GfxPixel::colorPixel(cc.index()), D1GfxPixel::colorPixel(pc.index())));
                                    }
                                    break;
                                }
                            }
                        }
                        if (iden + replacements.size() == currColors.size()) {
                            if (!replacements.isEmpty()) {
                                int n = i;
                                while (++n < cf.gfx->getFrameCount() && cf.gfx->getFrame(n)->getFramePal().isNull()) {
                                    ;
                                }
                                RemapParam params;
                                params.frames = std::pair<int, int>(i, n);
                                cf.gfx->replacePixels(replacements, params, verbose);
                            }
                            fp.clear();
                            cf.gfx->setModified();
                            dProgress() << QApplication::tr("Palette of frame %1 is obsolete.").arg(i + 1);
                            cp = pal;
                        }
                    /*} else {
                        // try to use the current palette instead of the previous one
                        unsigned iden = 0;
                        QList<QPair<D1GfxPixel, D1GfxPixel>> replacements;

                        for (const PaletteColor cc : prevColors) {
                            for (const PaletteColor pc : currColors) {
                                if (cc.red() == pc.red() && cc.green() == pc.green() && cc.blue() == pc.blue()) {
                                    if (cc.index() == pc.index()) {
                                        iden++;
                                    } else {
                                        replacements.push_back(QPair<D1GfxPixel, D1GfxPixel>(D1GfxPixel::colorPixel(pc.index()), D1GfxPixel::colorPixel(cc.index())));
                                    }
                                    break;
                                }
                            }
                        }
                        if (iden + replacements.size() == prevColors.size()) {
                            int n = i;
                            while (cf.gfx->getFrame(--n)->getFramePal().isNull()) {
                                ;
                            }
                            if (!replacements.isEmpty()) {
                                RemapParam params;
                                params.frames = std::pair<int, int>(n, i);
                                cf.gfx->replacePixels(replacements, params, verbose);
                            }
                            // QPointer<D1Pal> &pp = cf.gfx->getFrame(n)->getFramePal();
                            // pp.clear();
                            cf.gfx->getFrame(n)->setFramePal(cp);
                            fp.clear();
                            cf.gfx->setModified();
                            dProgress() << QApplication::tr("Palette of frame %1 is replaced by the palette of frame %2.").arg(n + 1).arg(i + 1);*/
                        }
                    }
#else
                    int n = 0;
                    for ( ; n < D1PAL_COLORS; n++) {
                        if (pal->getColor(n) != cp->getColor(n)) {
                            break;
                        }
                    }
                    if (n >= D1PAL_COLORS) {
                        fp.clear();
                        cf.gfx->setModified();
                        dProgress() << QApplication::tr("Palette of frame %1 is obsolete.").arg(i + 1);
                        cp = pal;
                    }
#endif
                }
                pal = cp;
            }
            if (!ProgressDialog::incValue()) {
                break;
            }
        }
        ProgressDialog::decBar();
    }

    ProgressDialog::decBar();
}
