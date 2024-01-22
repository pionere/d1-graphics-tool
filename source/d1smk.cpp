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

void D1SmkAudioData::setAudio(unsigned track, uint8_t* data, unsigned long len)
{
    this->audio[track] = data;
    this->len[track] = len;
}

uint8_t* D1SmkAudioData::getAudio(unsigned track, unsigned long *len) const
{
    *len = this->len[track];
    return const_cast<uint8_t*>(this->audio[track]);
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
    // gfx.frames.clear();
    if (params.celWidth != 0) {
        dProgressWarn() << QApplication::tr("Width setting is ignored when a SMK file is loaded.");
    }
    const bool clipped = params.clipped == OPEN_CLIPPED_TYPE::TRUE;
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
        frame->clipped = clipped;
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
            unsigned long len = smk_get_audio_size(SVidSMK, i);
            unsigned char* ct = nullptr;
            if (len != 0) {
                unsigned char* track = smk_get_audio(SVidSMK, i);
                ct = (unsigned char *)malloc(len);
                memcpy(ct, track, len);
            }
            audio->setAudio(i, ct, len);
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

    gfx.groupFrameIndices.clear();
    gfx.groupFrameIndices.push_back(std::pair<int, int>(0, frameNum - 1));

    gfx.type = D1CEL_TYPE::SMK;
    gfx.frameLen = SVidFrameLength;

    gfx.gfxFilePath = filePath;
    gfx.modified = false;
    return true;
}

#define SMK_TREE_MMAP    0
#define SMK_TREE_MCLR    1
#define SMK_TREE_FULL    2
#define SMK_TREE_TYPE    3
#define SMK_TREE_COUNT    4

typedef struct _SmkHeader {
    quint32 SmkMarker;
    quint32 VideoWidth;
    quint32 VideoHeight;
    quint32 FrameCount;
    quint32 FrameLen;
    quint32 VideoFlags;
    quint32 AudioMaxChunkLength[D1SMK_TRACKS]; // uncompressed length
    quint32 VideoTreeDataSize;
    quint32 VideoTreeSize[SMK_TREE_COUNT];     // uncompressed size
    quint32 AudioType[D1SMK_TRACKS];
    quint32 Dummy;
    // quint32 FrameDataSize[FrameCount];
    // quint8 FrameType[FrameCount];
    // quint8 VideoTreeData[VideoTreeDataSize];
    // quint8 FrameData[FrameDataSize][FrameCount];    [PAL_SIZE PALETTE] [AUDIO_SIZE{4} (AUDIO_DATA | UNCOMPRESSED_SIZE{4} 1 CH W TREE_DATA[4] AUDIO_DATA)][7] [VIDEO_DATA]
} SMKHEADER;

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
    unsigned treeJointCount;
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

    cacheValues[2] = cacheValues[1];
    cacheValues[1] = cacheValues[0];
    cacheValues[0] = value;
}

static void addTreeTypeValue(int type, int typelen, SmkTreeInfo &tree, unsigned (&cacheValues)[3])
{
    if (typelen != 0) {
        for (int i = lengthof(sizetable) - 1; /*i >= 0*/; ) {
            if (sizetable[i] <= typelen) {
                if (sizetable[i] != typelen && sizetable[i] == 59) {
                    // FIXME: delay to reuse existing leafs
                }

                addTreeValue(SwapLE16(type | i << 2), tree, cacheValues);
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
        *cursor |= ((value >> (length - i - 1)) & 1) << bitNum;
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

static uint8_t *buildTreeData(QList<QPair<unsigned, unsigned>> leafs, uint8_t *cursor, unsigned &bitNum, uint32_t branch, unsigned depth,
    unsigned &joints, QMap<unsigned, QPair<unsigned, uint32_t>> &paths, QMap<unsigned, QPair<unsigned, uint32_t>> *leafPaths)
{
    joints++;

    if (leafs.count() == 1) {
        cursor = writeBit(0, cursor, bitNum);
        unsigned leaf = leafs[0].first;
        paths[leaf] = QPair<unsigned, uint32_t>(depth, branch);
        
        if (leafPaths == nullptr) {
            cursor = writeNBits(leaf, 8, cursor, bitNum);
        } else {
            for (auto it = leafPaths[0].begin(); it != leafPaths[0].end(); it++) {
                if (it->first == (leaf & 0xFF)) {
                    cursor = writeNBits(it->second.second, it->second.first, cursor, bitNum);
                    break;
                }
            }
            for (auto it = leafPaths[1].begin(); it != leafPaths[1].end(); it++) {
                if (it->first == ((leaf >> 8) & 0xFF)) {
                    cursor = writeNBits(it->second.second, it->second.first, cursor, bitNum);
                    break;
                }
            }
        }
        return cursor;
    }
    cursor = writeBit(1, cursor, bitNum);
    depth++;
    QList<QPair<unsigned, unsigned>> rightLeafs;
    unsigned leftCount = 0, rightCount = 0;
    for (auto it = leafs.begin(); it != leafs.end(); ) {
        if (leftCount <= rightCount) {
            leftCount += it->second;
            it++;
        } else {
            rightCount += it->second;
            rightLeafs.push_back(*it);
            it = leafs.erase(it);
        }
    }
    branch <<= 1;
    cursor = buildTreeData(leafs, cursor, bitNum, branch, depth, joints, paths, leafPaths);

    branch |= 1;
    cursor = buildTreeData(rightLeafs, cursor, bitNum, branch, depth, joints, paths, leafPaths);

    return cursor;
}


static uint8_t *prepareVideoTree(SmkTreeInfo &tree, uint8_t *treeData, size_t &allocSize, size_t &cursor, unsigned &bitNum)
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
                    }
                    break;
                }
            }
        }
    }
    // convert cache values to normal values
    for (int i = 0; i < 3; i++) {
        unsigned n = 0;
        for ( ; n < UINT16_MAX; n++) {
            auto it = tree.treeStat.begin();
            for ( ; it != tree.treeStat.end(); it++) {
                if (it->first == n) {
                    break;
                }
            }
            if (it == tree.treeStat.end()) {
                tree.treeStat.push_back(QPair<unsigned, unsigned>(n, tree.cacheCount[i]));
                tree.cacheCount[i] = n; // replace cache-count with the 'fake' leaf value
                break;
            }
            /*auto val = std::find_if(tree.treeStat.begin(), tree.treeStat.end(), [n](const QPair<unsigned, unsigned> &entry) { return entry.first == n; });
            if (val == tree.treeStat.end()) {
                tree.treeStat.push_back(QPair<unsigned, unsigned>(n, tree.cacheCount[i]));
                tree.cacheCount[i] = n;
                break;
            }*/
        }
        if (n >= UINT16_MAX) {
            dProgressFail() << QApplication::tr("Congratulation, you managed to break SMK.");
        }
    }
    // sort treeStat
    std::sort(tree.treeStat.begin(), tree.treeStat.end(), [](const QPair<unsigned, unsigned> &e1, const QPair<unsigned, unsigned> &e2) { return e1.second < e2.second; });

    if (tree.treeStat.count() == 0) {
        // empty tree
        if (cursor >= allocSize) {
            allocSize += 1;
            uint8_t *treeDataNext = (uint8_t *)realloc(treeData, allocSize);
            if (treeDataNext == nullptr) {
                free(treeData);
                return nullptr;
            }
            treeData = treeDataNext;
        }
        // add open/close bits
        uint8_t* res = &treeData[cursor];
        res = writeNBits(0, 2, res, bitNum);
        cursor = (size_t)res - (size_t)treeData;
        return treeData;
    }
    // prepare the sub-trees
    QList<QPair<unsigned, unsigned>> lowByteLeafs, hiByteLeafs;
    for (QPair<unsigned, unsigned> &leaf : tree.treeStat) {
        unsigned lowByte = leaf.first & 0xFF;
        unsigned hiByte = (leaf.first >> 8) & 0xFF;
        auto it1 = lowByteLeafs.begin();
        for (; it1 != lowByteLeafs.end(); it1++) {
            if (it1->first == lowByte) {
                it1->second++;
                break;
            }
        }
        if (it1 == lowByteLeafs.end()) {
            lowByteLeafs.push_back(QPair<unsigned, unsigned>(lowByte, 1));
        }
        auto it2 = hiByteLeafs.begin();
        for (; it2 != hiByteLeafs.end(); it2++) {
            if (it2->first == hiByte) {
                it2->second++;
                break;
            }
        }
        if (it2 == hiByteLeafs.end()) {
            hiByteLeafs.push_back(QPair<unsigned, unsigned>(hiByte, 1));
        }
    }

    size_t maxSize = 1 + (1 + lowByteLeafs.count() * sizeof(uint8_t) * 2 * 8 + 1) + (1 + hiByteLeafs.count() * sizeof(uint8_t) * 2 * 8 + 1) + lengthof(tree.cacheCount) * sizeof(uint16_t) * 8 + (tree.treeStat.count() * sizeof(uint16_t) * 2 * 8) + 1;
    maxSize = (maxSize + 7) / 8;

    allocSize += maxSize;

    uint8_t *treeDataNext = (uint8_t *)realloc(treeData, allocSize);
    if (treeDataNext == nullptr) {
        free(treeData);
        return nullptr;
    }
    treeData = treeDataNext;

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
        res = buildTreeData(lowByteLeafs, res, bitNum, 0, 0, joints, bytePaths[0], nullptr);
        // close the low-sub-tree
        res = writeBit(0, res, bitNum);
    }
    {
        // start the hi sub-tree
        res = writeBit(1, res, bitNum);
        // add the hi sub-tree
        // joints = 0;
        res = buildTreeData(hiByteLeafs, res, bitNum, 0, 0, joints, bytePaths[1], nullptr);
        // close the hi-sub-tree
        res = writeBit(0, res, bitNum);
    }
    {
        // add the cache values
        for (int i = 0; i < 3; i++) {
            res = writeNBits(SwapLE16(tree.cacheCount[i]), 16, res, bitNum);
        }
    }
    {
        // add the main tree
        joints = 0;
        res = buildTreeData(tree.treeStat, res, bitNum, 0, 0, joints, tree.paths, bytePaths);
        // close the main tree
        res = writeBit(0, res, bitNum);
    }

    tree.treeJointCount = joints;

    cursor = (size_t)res - (size_t)treeData;
    return treeData;
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

    cacheValues[2] = cacheValues[1];
    cacheValues[1] = cacheValues[0];
    cacheValues[0] = value;

    auto it = videoTree.paths.find(v);
    assert(it != videoTree.paths.end());

    return writeNBits(it->second.second, it->second.first, cursor, bitNum);
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

                res = writeTreeValue(SwapLE16(type | i << 2), videoTree[SMK_TREE_TYPE], cacheValues[SMK_TREE_TYPE], res, bitNum);

                for (int n = 0; n < sizetable[i]; n++) {
                    switch (type) {
                    case 0: { // 2COLOR BLOCK  SMK_TREE_MMAP/SMK_TREE_MCLR
                        unsigned color1, color2, colors = 0;
                        color1 = frame->getPixel(x + 0, y + 0).getPaletteIndex();
                        for (int yy = 4 - 1; yy >= 0; yy--) {
                            for (int xx = 4 - 1; xx >= 0; xx--) {
                                colors <<= 1;
                                uint8_t color = frame->getPixel(x + xx, y + yy).getPaletteIndex();
                                if (color == color1) {
                                    colors |= 1;
                                } else {
                                    color2 = color;
                                }
                            }
                        }

                        res = writeTreeValue(SwapLE16(color1 << 8 | color2), videoTree[SMK_TREE_MCLR], cacheValues[SMK_TREE_MCLR], res, bitNum);
                        res = writeTreeValue(SwapLE16(colors), videoTree[SMK_TREE_MMAP], cacheValues[SMK_TREE_MMAP], res, bitNum);
                    } break;
                    case 1: { // FULL BLOCK  SMK_TREE_FULL
                        unsigned color1, color2;
                        for (int yy = 0; yy < 4; yy++) {
                            color1 = frame->getPixel(x + 2, y + yy).getPaletteIndex();
                            color2 = frame->getPixel(x + 3, y + yy).getPaletteIndex();
                            res = writeTreeValue(SwapLE16(color1 << 8 | color2), videoTree[SMK_TREE_FULL], cacheValues[SMK_TREE_FULL], res, bitNum);
                            color1 = frame->getPixel(x + 0, y + yy).getPaletteIndex();
                            color2 = frame->getPixel(x + 1, y + yy).getPaletteIndex();
                            res = writeTreeValue(SwapLE16(color1 << 8 | color2), videoTree[SMK_TREE_FULL], cacheValues[SMK_TREE_FULL], res, bitNum);
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
}

static unsigned encodePalette(D1Pal *pal, int frameNum, uint8_t *dest)
{
    static unsigned char oldPalette[D1SMK_COLORS][3];
    // convert pal to smk-palette
    unsigned char newPalette[D1SMK_COLORS][3];
    static_assert(D1PAL_COLORS == D1SMK_COLORS, "encodePalette conversion from D1PAL to SMK_PAL must be adjusted.");
    for (int i = 0; i < D1PAL_COLORS; i++) {
        QColor col = pal->getColor(i);
        // assert(col->getAlpha() == 255);
        newPalette[i][0] = col.red();
        newPalette[i][1] = col.green();
        newPalette[i][3] = col.blue();
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
            newPalette[i][n] = (size_t)*p - (size_t)&palmap[0];
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
                    if (oldPalette[k][0] != newPalette[i + curr][0] || oldPalette[k][1] != newPalette[i + curr][1] || oldPalette[k][2] != newPalette[i + curr][2]) {
                        break;
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
                LogErrorF("Ignoring skip block of %d from %d to use copy of %d instead (frame %d).", direct, i, best, frameNum + 1);
            }
            if (best > 0) {
                // 0x40: Color-shift block -> ((n + 63) / 64) * 2
                int dn = best;
                // limit range if there is a skip block ahead
                for (int n = i + direct + 1; n < i + best && dn > 64; n++) {
                    int sb = palStat[n].directMatch;
                    if (sb + n > i + best || sb > 64) {
                        LogErrorF("Cutting copy block of %d from %d to %d instead because there is a skip block of %d from %d (frame %d).", best, i, n - i, sb, n, frameNum + 1);
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
        len = sizeof(newPalette);
        memcpy(dest, newPalette, len);
    }

    memcpy(oldPalette, newPalette, sizeof(newPalette));
    return len;
}

static size_t encodeAudio(uint8_t *audioData, size_t len, const SmkAudioInfo &audioInfo, uint8_t *dest)
{
    unsigned bitNum;
    size_t result;

    if (!audioInfo.compress) {
        memcpy(dest, audioData, len);
        result = len;
    } else {
        bitNum = 0;
        result = 0;

        // FIXME... UNCOMPRESSED_SIZE{4} 1 CH W TREE_DATA[4] AUDIO_DATA

        if (bitNum != 0) {
            result++;
        }
    }

    return result;
}

bool D1Smk::save(D1Gfx &gfx, D1Pal *pal, const SaveAsParam &params)
{
    // validate the content
    int width = 0, height = 0;
    int frameCount = gfx.getFrameCount();
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
            dProgressErr() << QApplication::tr("Mismatching frame-size (%1).").arg(i + 1);
            return false;
        }
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (frame->getPixel(x, y).isTransparent()) {
                    dProgressErr() << QApplication::tr("Transparent pixel in frame %1. at %2:%3").arg(i + 1).arg(x).arg(y);
                    return false;
                }
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
                if (audioData->bitDepth != 8 && audioData->bitDepth != 16) {
                    dProgressErr() << QApplication::tr("Sample size of the audio chunk of frame %1 (track %2) is incompatible with SMK (%3 Must be 8 or 16).").arg(n + 1).arg(i + 1).arg(audioData->bitDepth);
                    return false;
                }
                if (bitDepth != audioData->bitDepth) {
                    if (bitDepth == 0) {
                        bitDepth = audioData->bitDepth;
                    } else {
                        dProgressErr() << QApplication::tr("Audio chunk of frame %1 (track %2) has mismatching sample size (%3 vs %4).").arg(n + 1).arg(i + 1).arg(audioData->bitDepth).arg(bitDepth);
                        return false;
                    }
                }
                if (audioData->channels != 1 && audioData->channels != 2) {
                    dProgressErr() << QApplication::tr("Channel-count of the audio chunk of frame %1 (track %2) is incompatible with SMK (%3 Must be 1 or 2).").arg(n + 1).arg(i + 1).arg(audioData->channels);
                    return false;
                }
                if (channels != audioData->channels) {
                    if (channels == 0) {
                        channels = audioData->channels;
                    } else {
                        dProgressErr() << QApplication::tr("Audio chunk of frame %1 (track %2) has mismatching channel-count (%3 vs %4).").arg(n + 1).arg(i + 1).arg(audioData->channels).arg(channels);
                        return false;
                    }
                }
                if (audioData->bitRate == 0 || audioData->bitRate > 0x00FFFFFF) {
                    dProgressErr() << QApplication::tr("Bitrate of the audio chunk of frame %1 (track %2) is incompatible with SMK (%3 Must be 1-%4).").arg(n + 1).arg(i + 1).arg(audioData->bitRate).arg(0x00FFFFFF);
                    return false;
                }
                if (bitRate != audioData->bitRate) {
                    if (bitRate == 0) {
                        bitRate = audioData->bitRate;
                    } else {
                        dProgressErr() << QApplication::tr("Audio chunk of frame %1 (track %2) has mismatching bitrate (%3 vs %4).").arg(n + 1).arg(i + 1).arg(audioData->bitRate).arg(bitRate);
                        return false;
                    }
                }
            }
        }

        audioInfo[i].bitRate = bitRate;
        audioInfo[i].bitDepth = bitDepth;
        audioInfo[i].channels = channels;
        audioInfo[i].maxChunkLength = maxChunkLength;
        audioInfo[i].compress = false;
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
                    if (yy != 10) {
                        ctype = 2; // VOID BLOCK -> skip
                    }
                }
                if (ctype == 1) {
                    unsigned numColors = 0, color1 = 256, color2 = 256, colors = 0;
                    for (int yy = 4 - 1; yy >= 0 && numColors < 3; yy--) {
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
                            if (numColors == 1) {
                                color1 = color;
                                continue;
                            }
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
                            addTreeValue(SwapLE16(color1 << 8 | color2), videoTree[SMK_TREE_MCLR], cacheValues[SMK_TREE_MCLR]);
                            addTreeValue(SwapLE16(colors), videoTree[SMK_TREE_MMAP], cacheValues[SMK_TREE_MMAP]);
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
                        addTreeValue(SwapLE16(color1 << 8 | color2), videoTree[SMK_TREE_FULL], cacheValues[SMK_TREE_FULL]);
                        color1 = frame->getPixel(x + 0, y + yy).getPaletteIndex();
                        color2 = frame->getPixel(x + 1, y + yy).getPaletteIndex();
                        addTreeValue(SwapLE16(color1 << 8 | color2), videoTree[SMK_TREE_FULL], cacheValues[SMK_TREE_FULL]);
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
    uint8_t *videoTreeData = (uint8_t *)malloc(1);
    size_t allocSize = 1, cursor = 0; unsigned bitNum = 0;
    for (int i = 0; i < SMK_TREE_COUNT; i++) {
        videoTreeData = prepareVideoTree(videoTree[i], videoTreeData, allocSize, cursor, bitNum);
        if (videoTreeData == nullptr) {
            dProgressFail() << QApplication::tr("Out of memory");
            return false;
        }
    }
    unsigned videoTreeDataSize = cursor;
    if (bitNum != 0) {
        videoTreeDataSize++;
    }

    SMKHEADER header;
    header.SmkMarker = SwapLE32(*((uint32_t*)"SMK2"));
    header.VideoWidth = SwapLE32(width);
    header.VideoHeight = SwapLE32(height);
    header.FrameCount = SwapLE32(frameCount);
    header.FrameLen = SwapLE32(frameLen);
    header.VideoTreeDataSize = SwapLE32(videoTreeDataSize);
    header.VideoFlags = 0;
    header.Dummy = 0;

    for (int i = 0; i < SMK_TREE_COUNT; i++) {
        header.VideoTreeSize[i] = SwapLE32(videoTree[i].treeJointCount);
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


    outFile.writeData((const char*)&header, sizeof(header));
    for (int i = 0; i < frameCount; i++) {
        outFile.writeData((const char*)&header.Dummy, 4);
    }
    for (int i = 0; i < frameCount; i++) {
        outFile.writeData((const char*)&frameInfo[i].FrameType, 1);
    }
    outFile.writeData((const char*)videoTreeData, videoTreeDataSize);

    /*D1GfxFrame **/ prevFrame = nullptr;
    uint8_t *frameData = (uint8_t*)malloc(256 * 3 + maxAudioLength + 4 * width * height);
    QList<unsigned> frameLengths;
    for (int n = 0; n < frameCount; n++) {
        D1GfxFrame *frame = gfx.getFrame(n);
        // reset pointers of the work-buffer
        size_t cursor = 0; unsigned bitNum = 0;
        // add optional palette
        D1Pal *framePal = frame->getFramePal().data();
        if (framePal == nullptr && n == 0) {
            dProgressWarn() << QApplication::tr("The palette is not set in the first frame. Defaulting to the current palette.");
            framePal = pal;
        }
        if (framePal != nullptr) {
            unsigned len = encodePalette(framePal, n, frameData + cursor + 4);
            len += 4;
            *(uint32_t*)&frameData[cursor] = SwapLE32(len);
            cursor += len;
        }
        // add optional audio
        D1SmkAudioData *audioData = frame->getFrameAudio();
        if (audioData != nullptr) {
            for (int i = 0; i < D1SMK_TRACKS; i++) {
                unsigned long length;
                uint8_t *data = audioData->getAudio(i, &length);
                if (length != 0) {
                    // assert(frameInfo[i].FrameType & (0x02 << track));
                    size_t len = encodeAudio(data, length, audioInfo[i], frameData + cursor + 4);
                    len += 4;
                    *(uint32_t*)&frameData[cursor] = SwapLE32(len);
                    cursor += len;
                }
            }
        }

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
                    if (yy != 10) {
                        ctype = 2; // VOID BLOCK -> skip
                    }
                }
                if (ctype == 1) {
                    unsigned numColors = 0, color1 = 256, color2 = 256, colors = 0;
                    for (int yy = 4 - 1; yy >= 0 && numColors < 3; yy--) {
                        for (int xx = 4 - 1; xx >= 0; xx--, colors <<= 1) {
                            unsigned color = frame->getPixel(x + xx, y + yy).getPaletteIndex();
                            if (color == color1) {
                                colors |= 1;
                                continue;
                            }
                            if (color == color2) {
                                continue;
                            }
                            ++numColors;
                            if (numColors == 1) {
                                color1 = color;
                                continue;
                            }
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
        encodePixels(width, height - 4, frame, type, typelen, videoTree, cacheValues, frameData, cursor, bitNum);

        if (bitNum != 0) {
            cursor++;
        }
        outFile.writeData((const char*)frameData, cursor);
        frameLengths.push_back(cursor);

        prevFrame = frame;
    }

    free(frameData);
    return false;
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

void D1Smk::fixColors(D1Pal *pal, QList<quint8> &colors)
{
    QColor undefColor = pal->getUndefinedColor();
    QList<quint8> ignored;
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
                        dProgress() << QApplication::tr("The %1 component of color %2 is adjusted (Using %4 instead of %5).").arg(compontent[n]).arg(i).arg(*p).arg(cv);
                        if (n == 0) {
                            col.setRed(*p);
                        }
                        if (n == 1) {
                            col.setGreen(*p);
                        }
                        if (n == 2) {
                            col.setBlue(*p);
                        }
                        pal->setColor(i, col);
                        if (colors.isEmpty() || colors.back() != i) {
                            colors.push_back(i);
                        }
                    }
                    break;
                }
            }
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
        dProgressWarn() << QApplication::tr("Ignored the %1 undefined color(s).", "", numIgnored).arg(ignoredColors);
    } else if (colors.isEmpty()) {
        dProgress() << QApplication::tr("The palette is SMK compliant.");
    }
}