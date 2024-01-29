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
// LogErrorF("D1Smk::load 0 %d", fileSize);
    smk SVidSMK = smk_open_memory(SVidBuffer, fileSize);
    if (SVidSMK == NULL) {
        MemFreeDbg(SVidBuffer);
        dProgressErr() << QApplication::tr("Invalid SMK file.");
        return false;
    }
// LogErrorF("D1Smk::load 1 %d", fileSize);
    unsigned long SVidWidth, SVidHeight;
    double SVidFrameLength;
    smk_info_all(SVidSMK, &SVidWidth, &SVidHeight, &SVidFrameLength);
// LogErrorF("D1Smk::load 2 %d %d %d", SVidWidth, SVidHeight, SVidFrameLength);
    unsigned char channels, depth;
    unsigned long rate;
    smk_info_audio(SVidSMK, &channels, &depth, &rate);
// LogErrorF("D1Smk::load 3 %d %d %d", channels, depth, rate);
    smk_enable_video(SVidSMK, true);
    for (int i = 0; i < D1SMK_TRACKS; i++) {
        smk_enable_audio(SVidSMK, i, false); // true);
    }
    // Decode first frame
// LogErrorF("D1Smk::load 4");
    char result = smk_first(SVidSMK);
    if (SMK_ERR(result)) {
        MemFreeDbg(SVidBuffer);
        dProgressErr() << QApplication::tr("Empty SMK file.");
        return false;
    }
// LogErrorF("D1Smk::load 5 %d", result);
    // load the first palette
    D1Pal *pal = LoadPalette(SVidSMK);
// LogErrorF("D1Smk::load 6");
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
// LogErrorF("D1Smk::load 7 %d: %d", frameNum, palUpdate);
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
// LogErrorF("D1Smk::load 8 %d: %d", frameNum, i, len);
                unsigned char* track = smk_get_audio(SVidSMK, i);
// LogErrorF("D1Smk::load 9");
                ct = (unsigned char *)malloc(len);
                memcpy(ct, track, len);
            }
            audio->setAudio(i, ct, len);
        }
        frame->frameAudio = audio;

        gfx.frames.append(frame);
        frameNum++;
    } while ((result = smk_next(SVidSMK)) == SMK_MORE && frameNum < 90);

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
    unsigned uncompressedTreeSize;
    QMap<unsigned, QPair<unsigned, uint32_t>> paths;
unsigned VideoTreeIndex;
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
static bool deepDeb = false;
static uint8_t *main_start;
static unsigned mainCounter = 0;
static unsigned leafCounter = 0;
static uint8_t *buildTreeData(QList<QPair<unsigned, unsigned>> leafs, uint8_t *cursor, unsigned &bitNum, uint32_t branch, unsigned depth,
//    unsigned &joints, QMap<unsigned, QPair<unsigned, uint32_t>> &paths, QMap<unsigned, QPair<unsigned, uint32_t>> (&leafPaths)[2])
    unsigned &joints, QMap<unsigned, QPair<unsigned, uint32_t>> &paths, QMap<unsigned, QPair<unsigned, uint32_t>> *leafPaths)
{
    joints++;
if (mainCounter > 0) {
	mainCounter--;
	LogErrorF("buildTreeData - INFO: rec-building %d", leafs.count());
}
    if (leafs.count() == 1) {
        cursor = writeBit(0, cursor, bitNum);
        unsigned leaf = leafs[0].first;
        paths[leaf] = QPair<unsigned, uint32_t>(depth, branch);
        
        if (leafPaths == nullptr) {
            cursor = writeNBits(leaf, 8, cursor, bitNum);
        } else {
// LogErrorF("TreeData leafPaths access");
if (leafCounter > 0) {
	LogErrorF("buildTreeData - INFO: leaf %d at %d:%d\n", leaf, branch, depth);
}
uint8_t *tmpPtr = cursor; unsigned tmpBitNum = bitNum;
            {
                auto it = leafPaths[0].find(leaf & 0xFF);
                if (it == leafPaths[0].end()) {
                    LogErrorF("ERROR: Missing entry for leaf %d in the low paths.", leaf & 0xFF);
                } else {
                    QPair<unsigned, uint32_t> &theEntryPair = it.value();
//                    LogErrorF("TreeData writeNBits value %d length %d for lo-leaf %d", theEntryPair.second, theEntryPair.first, leaf & 0xFF);
                    cursor = writeNBits(theEntryPair.second, theEntryPair.first, cursor, bitNum);
                }
            }
            {
                auto it = leafPaths[1].find((leaf >> 8) & 0xFF);
                if (it == leafPaths[1].end()) {
                    LogErrorF("ERROR: Missing entry for leaf %d in the high paths.", (leaf >> 8) & 0xFF);
                } else {
                    QPair<unsigned, uint32_t> theEntryPair = it.value();
//                    LogErrorF("TreeData writeNBits value %d length %d for hi-leaf %d", theEntryPair.second, theEntryPair.first, (leaf >> 8) & 0xFF);
                    cursor = writeNBits(theEntryPair.second, theEntryPair.first, cursor, bitNum);
                }
            }
if (leafCounter > 0) {
	leafCounter--;
			LogErrorF("buildTreeData - INFO: leaf %d at %d:%d len%d (ebn%d fbn%d) offset%d", leaf, branch, depth, ((size_t)cursor - (size_t)tmpPtr) * 8 + tmpBitNum - bitNum, tmpBitNum, bitNum, (size_t)tmpPtr - (size_t)main_start);
}
            /*for (auto it = leafPaths[0].begin(); it != leafPaths[0].end(); it++) {
                if (it->first == (leaf & 0xFF)) {
                    cursor = writeNBits(it->second.second, it->second.first, cursor, bitNum);
                    break;
                }
            }
            for (auto it = leafPaths[1].begin(); it != leafPaths[1].end(); it++) {
                if (it->first == ((leaf >> 8) & 0xFF)) {
                    cursor = writeNBits(it->second, it->first, cursor, bitNum);
                    break;
                }
            }*/
        }
        return cursor;
    }
    cursor = writeBit(1, cursor, bitNum);
    depth++;
	if (depth > 32) {
		LogErrorF("buildTreeData ERROR: depth %d too much.", depth);
    }
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
//    if (deepDeb)
//        LogErrorF("TreeData joint %d depth %d value %d length %d / %d", joints, depth, branch, leafs.count(), rightLeafs.count());
    // branch <<= 1;
    cursor = buildTreeData(leafs, cursor, bitNum, branch, depth, joints, paths, leafPaths);

    branch |= 1 << (depth - 1);
    cursor = buildTreeData(rightLeafs, cursor, bitNum, branch, depth, joints, paths, leafPaths);

    return cursor;
}

/* Recursive sub-func for building a tree into an array. */
typedef struct _huff8_t {
	/* Unfortunately, smk files do not store the alloc size of a small tree.
		511 entries is the pessimistic case (N codes and N-1 branches,
		with N=256 for 8 bits) */
	size_t size;
	unsigned short tree[511];
} huff8_t;
typedef struct _bit_t {
	const unsigned char * buffer, * end;
	unsigned int bit_num;
} bit_t;
#define HUFF8_BRANCH 0x8000
#define HUFF8_LEAF_MASK 0x7FFF
static int bs_read_1(bit_t * const bs)
{
	int ret;
	/* null check */
	assert(bs);

	/* don't die when running out of bits, but signal */
	if (bs->buffer >= bs->end) {
		LogErrorF("libsmacker::bs_read_1(): ERROR: bitstream exhausted.\n");
		return -1;
	}

	/* get next bit and store for return */
	ret = (*bs->buffer >> bs->bit_num) & 1;

	/* advance to next bit */
	if (bs->bit_num >= 7) {
		/* Out of bits in this byte: next! */
		bs->buffer ++;
		bs->bit_num = 0;
	} else
		bs->bit_num ++;

	return ret;
}
/* Reads a byte
	Returns -1 if error. */
static int bs_read_8(bit_t * const bs)
{
	/* don't die when running out of bits, but signal */
	if (bs->buffer + (bs->bit_num > 0) >= bs->end) {
		LogErrorF("libsmacker::bs_read_8(): ERROR: bitstream exhausted.\n");
		return -1;
	}
#ifdef FULL
	if (bs->bit_num) {
		/* unaligned read */
#endif
		int ret = *bs->buffer >> bs->bit_num;
		bs->buffer ++;
		return ret | (*bs->buffer << (8 - bs->bit_num) & 0xFF);
#ifdef FULL
	}

	/* aligned read */
	return *bs->buffer++;
#endif
}
static int huff8_build_rec(huff8_t * const t, bit_t * const bs)
{
	int bit, value;

	/* Make sure we aren't running out of bounds */
	if (t->size >= 511) {
		LogErrorF("libsmacker::huff8_build_rec() - ERROR: size exceeded\n");
		return 0;
	}

	/* Read the next bit */
	if ((bit = bs_read_1(bs)) < 0) {
		LogErrorF("libsmacker::huff8_build_rec() - ERROR: get_bit returned -1\n");
		return 0;
	}

	if (bit) {
		/* Bit set: this forms a Branch node.
			what we have to do is build the left-hand branch,
			assign the "jump" address,
			then build the right hand branch from there.
		*/
		/* track the current index */
		value = t->size ++;

		/* go build the left branch */
		if (! huff8_build_rec(t, bs)) {
			LogErrorF("libsmacker::huff8_build_rec() - ERROR: failed to build left sub-tree\n");
			return 0;
		}

		/* now go back to our current location, and
			mark our location as a "jump" */
		t->tree[value] = HUFF8_BRANCH | t->size;

		/* continue building the right side */
		if (! huff8_build_rec(t, bs)) {
			LogErrorF("libsmacker::huff8_build_rec() - ERROR: failed to build right sub-tree\n");
			return 0;
		}
	} else {
		/* Bit unset signifies a Leaf node. */
		/* Attempt to read value */
		if ((value = bs_read_8(bs)) < 0) {
			LogErrorF("libsmacker::huff8_build_rec() - ERROR: get_byte returned -1\n");
			return 0;
		}

//if (deepDebug)
//LogErrorFF("huff8_build_rec leaf[%d]=%d (%d:%d:%d)", t->size, value);
		/* store to tree */
		t->tree[t->size ++] = value;
	}

	return 1;
}
static int huff8_build(huff8_t * const t, bit_t * const bs)
{
	int bit;

	/* Smacker huff trees begin with a set-bit. */
	if ((bit = bs_read_1(bs)) < 0) {
		LogErrorF("libsmacker::huff8_build() - ERROR: initial get_bit returned -1\n");
		return 0;
	}

	/* OK to fill out the struct now */
	t->size = 0;

	/* First bit indicates whether a tree is present or not. */
	/*  Very small or audio-only files may have no tree. */
	if (bit) {
		if (! huff8_build_rec(t, bs)) {
			LogErrorF("libsmacker::huff8_build() - ERROR: tree build failed\n");
			return 0;
		}
	} else
		t->tree[0] = 0;

	/* huff trees end with an unset-bit */
	if ((bit = bs_read_1(bs)) < 0) {
		LogErrorF("libsmacker::huff8_build() - ERROR: final get_bit returned -1\n");
		return 0;
	}

	/* a 0 is expected here, a 1 generally indicates a problem! */
	if (bit) {
		LogErrorF("libsmacker::huff8_build() - ERROR: final get_bit returned 1\n");
		return 0;
	}

	return 1;
}

/* Look up an 8-bit value from a basic huff tree.
	Return -1 on error. */
static int huff8_lookup(const huff8_t * const t, bit_t * const bs)
{
	int bit, index = 0;

	while (t->tree[index] & HUFF8_BRANCH) {
		if ((bit = bs_read_1(bs)) < 0) {
			LogErrorF("libsmacker::huff8_lookup() - ERROR: get_bit returned -1\n");
			return -1;
		}
		int prevIndex = index;
		if (bit) {
			/* take the right branch */
			index = t->tree[index] & HUFF8_LEAF_MASK;
		} else {
			/* take the left branch */
			index ++;
		}
        if (index >= sizeof(t->tree)) {
            LogErrorF("libsmacker::huff8_lookup() - ERROR: invalid leaf index %d at %d (%d) \n", index, prevIndex, bit);
            return -1;
        }
	}

	/* at leaf node.  return the value at this point. */
	return t->tree[index];
}

#define HUFF16_BRANCH    0x80000000
#define HUFF16_CACHE     0x40000000

typedef struct _huff16_t {
	unsigned int * tree;
	size_t size;

	/* recently-used values cache */
	unsigned short cache[3];
} huff16_t;
static const unsigned char *huff16_start;
static unsigned huffCounter = 0;
static unsigned huffLeafCounter = 0;
static int huff16_build_rec(huff16_t * const t, bit_t * const bs, const huff8_t * const low8, const huff8_t * const hi8, const size_t limit, int depth)
{
	int bit, value;
	/* Make sure we aren't running out of bounds */
	if (t->size >= limit) {
		LogErrorF("libsmacker::huff16_build_rec() - ERROR: size exceeded\n");
		return 0;
	}

	/* Read the first bit */
	if ((bit = bs_read_1(bs)) < 0) {
		LogErrorF("libsmacker::huff16_build_rec() - ERROR: get_bit returned -1\n");
		return 0;
	}
if (huffCounter > 0) {
	huffCounter--;
	LogErrorF("libsmacker::huff16_build_rec() - INFO: rec-reading %d", bit);
}
	if (bit) {
		/* See tree-in-array explanation for HUFF8 above */
		/* track the current index */
		value = t->size ++;

		/* go build the left branch */
		if (! huff16_build_rec(t, bs, low8, hi8, limit, depth + 1)) {
			LogErrorF("libsmacker::huff16_build_rec() - ERROR: failed to build left sub-tree\n");
			return 0;
		}

		/* now go back to our current location, and
			mark our location as a "jump" */
		t->tree[value] = HUFF16_BRANCH | t->size;
//		treeValue[depth] = true;
		/* continue building the right side */
		if (! huff16_build_rec(t, bs, low8, hi8, limit, depth + 1)) {
			LogErrorF("libsmacker::huff16_build_rec() - ERROR: failed to build right sub-tree\n");
			return 0;
		}
//		treeValue[depth] = false;
	} else {
		/* Bit unset signifies a Leaf node. */
const unsigned char * tmpBuffer = bs->buffer; unsigned tmpBitNum = bs->bit_num;
		/* Attempt to read LOW value */
		if ((value = huff8_lookup(low8, bs)) < 0) {
			LogErrorF("libsmacker::huff16_build_rec() - ERROR: get LOW value returned -1\n");
			return 0;
		}

		t->tree[t->size] = value;

		/* now read HIGH value */
		if ((value = huff8_lookup(hi8, bs)) < 0) {
			LogErrorF("libsmacker::huff16_build_rec() - ERROR: get HIGH value returned -1\n");
			return 0;
		}

		/* Looks OK: we got low and hi values. Return a new LEAF */
		t->tree[t->size] |= (value << 8);
if (huffLeafCounter > 0) {
	huffLeafCounter--;
		LogErrorF("libsmacker::huff16_build_rec() - INFO: leaf %d at %d len%d (ebn%d fbn%d) offset%d\n", t->tree[t->size], t->size, ((size_t)bs->buffer - (size_t)tmpBuffer) * 8 + tmpBitNum - bs->bit_num, tmpBitNum, bs->bit_num, (size_t)tmpBuffer - (size_t)huff16_start);
}
// bool deepDebug = t->tree[t->size] == t->cache[0] || t->tree[t->size] == t->cache[1] || t->tree[t->size] == t->cache[2];
/*if (deepDebug) {
// LogErrorFF("huff16_build leaf[%d]=%d (%d,%d) d%d c(%d:%d:%d)", t->size, t->tree[t->size], t->tree[t->size] & 0xFF, value, depth, t->tree[t->size] == t->cache[0], t->tree[t->size] == t->cache[1], t->tree[t->size] == t->cache[2]);
	/*int i = sizeof(fullLeafs) / sizeof(fullLeafs[0]) - 1;
	for ( ; i >= 0; i--) {
		if (fullLeafs[i] == t->tree[t->size])
			break;
    }
	if (i >= 0) {* /
        unsigned char bytes[4] = { 0 };
		for (int n = 0; n < depth; n++) {
			if (treeValue[n])
				bytes[n / 8] |= 1 << (n % 8);
        }
		LogErrorFF("huff16_build leaf[%d]=%d (%d,%d) d%d %d c%d:%d:%d", t->size, t->tree[t->size], t->tree[t->size] & 0xFF, value, depth, *(unsigned*)(&bytes[0]), t->tree[t->size] == t->cache[0], t->tree[t->size] == t->cache[1], t->tree[t->size] == t->cache[2]);
    // }
}*/
		/* Last: when building the tree, some Values may correspond to cache positions.
			Identify these values and set the Escape code byte accordingly. */
		if (t->tree[t->size] == t->cache[0])
			t->tree[t->size] = HUFF16_CACHE;
		else if (t->tree[t->size] == t->cache[1])
			t->tree[t->size] = HUFF16_CACHE | 1;
		else if (t->tree[t->size] == t->cache[2])
			t->tree[t->size] = HUFF16_CACHE | 2;

		t->size ++;
	}

	return 1;
}

/* Entry point for building a big 16-bit tree. */
static int huff16_build(huff16_t * const t, bit_t * const bs, const unsigned int alloc_size)
{
	huff8_t low8, hi8;
	size_t limit;
	int value, i, bit;

	/* Smacker huff trees begin with a set-bit. */
	if ((bit = bs_read_1(bs)) < 0) {
		LogErrorF("libsmacker::huff16_build() - ERROR: initial get_bit returned -1\n");
		return 0;
	}

	t->size = 0;

	/* First bit indicates whether a tree is present or not. */
	/*  Very small or audio-only files may have no tree. */
	if (bit) {
// LogErrorFF("huff16_build 1 %d", alloc_size);
		/* build low-8-bits tree */
		if (! huff8_build(&low8, bs)) {
			LogErrorF("libsmacker::huff16_build() - ERROR: failed to build LOW tree\n");
			return 0;
		}
// LogErrorFF("huff16_build 2");
		/* build hi-8-bits tree */
		if (! huff8_build(&hi8, bs)) {
			LogErrorF("libsmacker::huff16_build() - ERROR: failed to build HIGH tree\n");
			return 0;
		}
// LogErrorFF("huff16_build 3");
		/* Init the escape code cache. */

//		LogErrorF("libsmacker::huff16_build() - INFO: cache starting bn%d [%d,%d,%d,%d,%d]\n", bs->bit_num, bs->buffer[0], bs->buffer[1], bs->buffer[2], bs->buffer[3], bs->buffer[4]);
		for (i = 0; i < 3; i ++) {
			if ((value = bs_read_8(bs)) < 0) {
				LogErrorF("libsmacker::huff16_build() - ERROR: get LOW value for cache %d returned -1\n", i);
				return 0;
			}

			t->cache[i] = value;

			/* now read HIGH value */
			if ((value = bs_read_8(bs)) < 0) {
				LogErrorF("libsmacker::huff16_build() - ERROR: get HIGH value for cache %d returned -1\n", i);
				return 0;
			}

			t->cache[i] |= (value << 8);
//			LogErrorF("libsmacker::huff16_build() - INFO: cache %d : %d\n", i, t->cache[i]);
		}
		/* Everything looks OK so far. Time to malloc structure. */
		if (alloc_size < 12 || alloc_size % 4) {
			LogErrorF("libsmacker::huff16_build() - ERROR: illegal value %u for alloc_size\n", alloc_size);
			return 0;
		}
// LogErrorFF("huff16_build 4");
		limit = (alloc_size - 12) / 4;
		if ((t->tree = (unsigned int*)malloc(limit * sizeof(unsigned int))) == NULL) {
			LogErrorF("libsmacker::huff16_build() - ERROR: failed to malloc() huff16 tree");
			return 0;
		}
//		LogErrorF("libsmacker::huff16_build() - INFO: main starting bn%d [%d,%d,%d,%d,%d]\n", bs->bit_num, bs->buffer[0], bs->buffer[1], bs->buffer[2], bs->buffer[3], bs->buffer[4]);
		/* Finally, call recursive function to retrieve the Bigtree. */
		if (! huff16_build_rec(t, bs, &low8, &hi8, limit, 0)) {
			LogErrorF("libsmacker::huff16_build() - ERROR: failed to build huff16 tree\n");
			goto error;
		}
// LogErrorFF("huff16_build 5");
		/* check that we completely filled the tree */
		if (limit != t->size) {
// LogErrorFF("failed to completely decode huff16 tree %d vs %d", limit, t->size);
			LogErrorF("libsmacker::huff16_build() - ERROR: failed to completely decode huff16 tree (%d vs %d)\n", limit, t->size);
			goto error;
		}
	} else {
		if ((t->tree = (unsigned int*)malloc(sizeof(unsigned int))) == NULL) {
			LogErrorF("libsmacker::huff16_build() - ERROR: failed to malloc() huff16 tree");
			return 0;
		}

		t->tree[0] = 0;
		//t->cache[0] = t->cache[1] = t->cache[2] = 0;
	}

	/* Check final end tag. */
	if ((bit = bs_read_1(bs)) < 0) {
		LogErrorF("libsmacker::huff16_build() - ERROR: final get_bit returned -1\n");
		goto error;
	}

	/* a 0 is expected here, a 1 generally indicates a problem! */
	if (bit) {
		LogErrorF("libsmacker::huff16_build() - ERROR: final get_bit returned 1\n");
		goto error;
	}

	return 1;
error:
	return 0;
}

static uint8_t *prepareVideoTree(SmkTreeInfo &tree, uint8_t *treeData, size_t &allocSize, size_t &cursor, unsigned &bitNum)
{
// LogErrorF("D1Smk::prepareVideoTree 0");
    // reduce inflated cache frequencies
    for (int i = 0; i < 3; i++) {
// LogErrorF("D1Smk::prepareVideoTree cache clean c%d as%d bn%d", i, tree.cacheStat[i].count());
        for (int n = 0; n < tree.cacheStat[i].count(); n++) {
            unsigned value = tree.cacheStat[i][n].first;
            for (auto it = tree.treeStat.begin(); it != tree.treeStat.end(); it++) {
                if (it->first == value) {
                    if (it->second + tree.cacheStat[i][n].second >= tree.cacheCount[i] - tree.cacheStat[i][n].second) {
                        // use the 'normal' leaf instead of the cache
LogErrorF("D1Smk::prepareVideoTree using normal leaf instead of cache for %d refs%d cacherefs%d of %d in tree %d", value, it->second, tree.cacheStat[i][n].second, tree.cacheCount[i], tree.VideoTreeIndex);
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
// LogErrorF("D1Smk::prepareVideoTree 1");
    // convert cache values to normal values
    bool hasEntries = !tree.treeStat.isEmpty();
    for (int i = 0; i < 3; i++) {
        hasEntries |= tree.cacheCount[i] != 0;
        unsigned n = 0;
        for ( ; n < UINT16_MAX; n++) {
            auto it = tree.treeStat.begin();
            for ( ; it != tree.treeStat.end(); it++) {
                if (it->first == n) {
                    break;
                }
            }
            if (it == tree.treeStat.end()) {
// LogErrorF("D1Smk::prepareVideoTree cache normal i%d n%d cc%d", i, n, tree.cacheCount[i]);
                if (tree.cacheCount[i] != 0) {
                    tree.treeStat.push_back(QPair<unsigned, unsigned>(n, tree.cacheCount[i]));
                }
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

    if (!hasEntries) {
LogErrorF("D1Smk::prepareVideoTree empty tree c%d as%d bn%d", cursor, allocSize, bitNum);
        // empty tree
        /*if (cursor >= allocSize || (bitNum == 7 && cursor + 1 == allocSize)) {
            allocSize += 1;
LogErrorF("D1Smk::prepareVideoTree realloc tree %d", allocSize);
            uint8_t *treeDataNext = (uint8_t *)realloc(treeData, allocSize);
            if (treeDataNext == nullptr) {
                free(treeData);
                return nullptr;
            }
            treeData = treeDataNext;
        }*/
        // add open/close bits
        uint8_t* res = &treeData[cursor];
// LogErrorF("D1Smk::prepareVideoTree writing %d", cursor);
        res = writeNBits(0, 2, res, bitNum);
        cursor = (size_t)res - (size_t)treeData;
LogErrorF("D1Smk::prepareVideoTree new cursor %d", cursor);
        tree.uncompressedTreeSize = 0;
        return treeData;
    }
    // sort treeStat
LogErrorF("D1Smk::prepareVideoTree sort %d", tree.treeStat.count());
    std::sort(tree.treeStat.begin(), tree.treeStat.end(), [](const QPair<unsigned, unsigned> &e1, const QPair<unsigned, unsigned> &e2) { return e1.second < e2.second || (e1.second == e2.second && e1.first < e2.first); });
// LogErrorF("D1Smk::prepareVideoTree sorted %d", tree.treeStat.count());
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
LogErrorF("D1Smk::prepareVideoTree subtrees %d and %d. As%d, c%d bn%d", lowByteLeafs.count(), hiByteLeafs.count(), allocSize, cursor, bitNum);
/*    size_t maxSize = 1 + (1 + lowByteLeafs.count() * sizeof(uint8_t) * 2 * 8 + 1) + (1 + hiByteLeafs.count() * sizeof(uint8_t) * 2 * 8 + 1) + lengthof(tree.cacheCount) * sizeof(uint16_t) * 8 + (tree.treeStat.count() * sizeof(uint16_t) * 2 * 8) + 1;
    maxSize = (maxSize + 7) / 8;
LogErrorF("D1Smk::prepareVideoTree new maxis %d and %d, c%d bn%d", maxSize, allocSize + maxSize, cursor, bitNum);
    allocSize += maxSize;

    uint8_t *treeDataNext = (uint8_t *)realloc(treeData, allocSize);
    if (treeDataNext == nullptr) {
        free(treeData);
        return nullptr;
    }
    treeData = treeDataNext;*/

    uint8_t* res = &treeData[cursor];
unsigned startBitNum = bitNum;
    { // start the main tree
        res = writeBit(1, res, bitNum);
    }
    QMap<unsigned, QPair<unsigned, uint32_t>> bytePaths[2];
    unsigned joints;
uint8_t *tmpPtr = res; unsigned tmpBitNum = bitNum;
main_start = res;
    {
        // start the low sub-tree
        res = writeBit(1, res, bitNum);
        // add the low sub-tree
        // joints = 0;
joints = 0;
        res = buildTreeData(lowByteLeafs, res, bitNum, 0, 0, joints, bytePaths[0], nullptr);
        // close the low-sub-tree
        res = writeBit(0, res, bitNum);
    }
LogErrorF("D1Smk::prepareVideoTree low added %d bn%d", (size_t)res - (size_t)treeData, bitNum);
    {
		huff8_t testTree;
		memset(&testTree, 0, sizeof(testTree));
		bit_t bt;
		bt.buffer = tmpPtr;
		bt.end = res + ((bitNum != 0) ? 1 : 0);
		bt.bit_num = tmpBitNum;
		if (!huff8_build(&testTree, &bt)) {
			LogErrorF("ERROR D1Smk::prepareVideoTree huff8_build 0 failed");
        } else {
			// LogErrorF("D1Smk::prepareVideoTree huff8_build 0 success joints%d leafs %d, treesize%d", joints, bytePaths[0].size(), testTree.size);
			unsigned leafs = 0;
			unsigned dbgLeafCount = 0;
			for (unsigned i = 0; i < testTree.size; i++) {
				if (testTree.tree[i] & HUFF8_BRANCH) {
					// LogErrorF("D1Smk::prepareVideoTree branch at %d to %d", i, testTree.tree[i] & HUFF8_LEAF_MASK);
                } else {
					leafs++;
					// LogErrorF("D1Smk::prepareVideoTree leaf %d at %d", testTree.tree[i], i);
					auto it = bytePaths[0].find(testTree.tree[i]);
					if (it == bytePaths[0].end()) {
						LogErrorF("ERROR D1Smk::prepareVideoTree leaf not planned");
                    } else {
						// LogErrorF("D1Smk::prepareVideoTree 0 encoded leaf value%d depth%d branch%d", it.key(), it.value().first, it.value().second);
						if (dbgLeafCount > 0) {
							dbgLeafCount--;
							unsigned branch = it.value().second;
							unsigned short *leafPtr = testTree.tree;
							unsigned char route[256];
							unsigned depth = it.value().first;
							route[depth] = '\0';
							unsigned index = 0;
							for (unsigned n = 0; n < depth; n++) {
								if (testTree.tree[index] & HUFF8_BRANCH) {
									if (branch & 1) {
										route[n] = 'R';
										index = testTree.tree[index] & HUFF8_LEAF_MASK;
									} else {
										route[n] = 'L';
										index++;
									}
									branch >>= 1;
									if (index >= testTree.size) {
										LogErrorF("ERROR D1Smk::prepareVideoTree out of range at %d (step %d)", index, n);
                                    }
                                } else {
									route[n] = 'X';
									LogErrorF("ERROR D1Smk::prepareVideoTree lost branch at %d", n);
									break;
                                }
							}
							LogErrorF("D1Smk::prepareVideoTree route %s", route);
							if (testTree.tree[index] != it.key()) {
								LogErrorF("ERROR D1Smk::prepareVideoTree wrong value %d (@ %d) vs %d", testTree.tree[index], it.key(), index);
                            }
                        }
                    }
                }
            }
			if (leafs != bytePaths[0].size())
				LogErrorF("ERROR D1Smk::prepareVideoTree huff8_build 0 res leafs %d vs %d", leafs, bytePaths[0].size());
            else
				; // LogErrorF("D1Smk::prepareVideoTree huff8_build 0 res leafs %d", leafs);
        }
    }
tmpPtr = res; tmpBitNum = bitNum;
    {
        // start the hi sub-tree
        res = writeBit(1, res, bitNum);
        // add the hi sub-tree
        // joints = 0;
joints = 0;
        res = buildTreeData(hiByteLeafs, res, bitNum, 0, 0, joints, bytePaths[1], nullptr);
        // close the hi-sub-tree
        res = writeBit(0, res, bitNum);
    }
LogErrorF("D1Smk::prepareVideoTree hi added %d bn%d", (size_t)res - (size_t)treeData, bitNum);
    {
		huff8_t testTree;
		memset(&testTree, 0, sizeof(testTree));
		bit_t bt;
		bt.buffer = tmpPtr;
		bt.end = res + ((bitNum != 0) ? 1 : 0);
		bt.bit_num = tmpBitNum;
		if (!huff8_build(&testTree, &bt)) {
			LogErrorF("ERROR D1Smk::prepareVideoTree huff8_build 1 failed");
        } else {
			// LogErrorF("D1Smk::prepareVideoTree huff8_build 1 success joints%d leafs %d, treesize%d", joints, bytePaths[1].size(), testTree.size);
			unsigned leafs = 0;
			for (unsigned i = 0; i < testTree.size; i++) {
				if (testTree.tree[i] & HUFF8_BRANCH) {
					// LogErrorF("D1Smk::prepareVideoTree branch at %d to %d", i, testTree.tree[i] & HUFF8_LEAF_MASK);
                } else {
					leafs++;
					// LogErrorF("D1Smk::prepareVideoTree leaf %d at %d", testTree.tree[i], i);
					auto it = bytePaths[1].find(testTree.tree[i]);
					if (it == bytePaths[1].end()) {
						LogErrorF("ERROR D1Smk::prepareVideoTree leaf not planned");
                    } else {
						// LogErrorF("D1Smk::prepareVideoTree 1 encoded leaf value%d depth%d branch%d", it.key(), it.value().first, it.value().second);
                    }
                }
            }
			if (leafs != bytePaths[1].size())
				LogErrorF("ERROR D1Smk::prepareVideoTree huff16_build 1 res leafs %d vs %d", leafs, bytePaths[1].size());
            else
				; // LogErrorF("D1Smk::prepareVideoTree huff8_build 1 res leafs %d", leafs);
        }
    }
tmpPtr = res; tmpBitNum = bitNum;
    {
        // add the cache values
        for (int i = 0; i < 3; i++) {
            res = writeNBits(tree.cacheCount[i], 16, res, bitNum);
LogErrorF("D1Smk::prepareVideoTree INFO: cache %d : %d", i, tree.cacheCount[i]);
        }
    }
LogErrorF("D1Smk::prepareVideoTree cache added %d bn%d from bn%d [%d,%d,%d,%d,%d]", (size_t)res - (size_t)treeData, bitNum, tmpBitNum, tmpPtr[0], tmpPtr[1], tmpPtr[2], tmpPtr[3], tmpPtr[4]);
// deepDeb = true;
tmpPtr = res; tmpBitNum = bitNum;
mainCounter = 0;
leafCounter = 0;
    {
        // add the main tree
        joints = 0;
        res = buildTreeData(tree.treeStat, res, bitNum, 0, 0, joints, tree.paths, bytePaths);
        // close the main tree
        res = writeBit(0, res, bitNum);
    }
// deepDeb = false;
LogErrorF("D1Smk::prepareVideoTree main added %d bn%d js%d from bn%d [%d,%d,%d,%d,%d]", (size_t)res - (size_t)treeData, bitNum, joints, tmpBitNum, tmpPtr[0], tmpPtr[1], tmpPtr[2], tmpPtr[3], tmpPtr[4]);
    {
		huff16_t testTree;
		memset(&testTree, 0, sizeof(testTree));
		bit_t bt;
		bt.buffer = &treeData[cursor];
		bt.end = res + ((bitNum != 0) ? 1 : 0);
		bt.bit_num = startBitNum;
		unsigned alloc_size = (joints + 3) * 4;
		huff16_start = bt.buffer;
		huffCounter = 0;
		huffLeafCounter = 0;
		if (!huff16_build(&testTree, &bt, alloc_size)) {
			LogErrorF("ERROR D1Smk::prepareVideoTree huff16_build failed");
        } else {
			// LogErrorF("D1Smk::prepareVideoTree huff16_build success joints%d leafs %d, treesize%d", joints, tree.paths.size(), testTree.size);
			unsigned leafs = 0;
			for (unsigned i = 0; i < testTree.size; i++) {
				if (testTree.tree[i] & HUFF16_BRANCH) {
					// LogErrorF("D1Smk::prepareVideoTree branch at %d", i);
                } else if (testTree.tree[i] & HUFF16_CACHE) {
					leafs++;
					unsigned cacheIdx = testTree.tree[i] & ~HUFF16_CACHE;
					// LogErrorF("D1Smk::prepareVideoTree cache leaf %d (%d) at %d", testTree.tree[i], cacheIdx, i);
					if (cacheIdx > 2) {
						LogErrorF("ERROR D1Smk::prepareVideoTree bad cache index %d", cacheIdx);
                    } else {
						unsigned value = tree.cacheCount[cacheIdx];
						auto it = tree.paths.find(value);
						if (it == tree.paths.end()) {
							LogErrorF("ERROR D1Smk::prepareVideoTree cache (%d) not planned", value);
						} else {
							// LogErrorF("D1Smk::prepareVideoTree encoded cache depth%d value%d", it.value().first, it.value().second);
						}
                    }
                } else {
					leafs++;
					// LogErrorF("D1Smk::prepareVideoTree leaf %d at %d", testTree.tree[i], i);
					auto it = tree.paths.find(testTree.tree[i]);
					if (it == tree.paths.end()) {
						LogErrorF("ERROR D1Smk::prepareVideoTree leaf not planned");
                    } else {
						// LogErrorF("D1Smk::prepareVideoTree encoded leaf depth%d value%d", it.value().first, it.value().second);
                    }
                }
            }
			if (leafs != tree.paths.size())
				LogErrorF("ERROR D1Smk::prepareVideoTree huff16_build res leafs %d vs %d", leafs, tree.paths.size());
            else
				; // LogErrorF("D1Smk::prepareVideoTree huff16_build res leafs %d", leafs);
        }
		free(testTree.tree);
    }

    tree.uncompressedTreeSize = (joints + 3) * 4;

    cursor = (size_t)res - (size_t)treeData;
    return treeData;
}

static uint8_t *writeTreeValue(unsigned value, const SmkTreeInfo &videoTree, unsigned (&cacheValues)[3], uint8_t *cursor, unsigned &bitNum)
{
    unsigned v = value;
    for (int i = 0; i < 3; i++) {
        if (cacheValues[i] == value) {
            // check if the value was not removed from the cache (in prepareVideoTree)
bool cached = false;
            for (const QPair<unsigned, unsigned> &entry : videoTree.cacheStat[i]) {
                if (entry.first == value) {
                    v = videoTree.cacheCount[i]; // use the 'fake' leaf value
					cached = true;
                }
            }
if (!cached) {
	// LogErrorF("D1Smk::writeTreeValue does not use cache value (%d) for %d in tree %d", videoTree.cacheCount[i], value, videoTree.VideoTreeIndex);
}
            break;
        }
    }

    cacheValues[2] = cacheValues[1];
    cacheValues[1] = cacheValues[0];
    cacheValues[0] = value;

    auto it = videoTree.paths.find(v);
    if (it == videoTree.paths.end()) {
        LogErrorF("ERROR: writeTreeValue missing entry %d (%d) in %d", v, value, videoTree.VideoTreeIndex);
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
int sx = x, sy = y, slen = typelen;
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
if (deepDeb)
                LogErrorF("D1Smk::encodePixels treetype %d (ty%d len %d=%d data%d) offset%d bn%d", type | i << 2, type, i, sizetable[i], (type >> 8) & 0xFF, (size_t)res - (size_t)&frameData[cursor], bitNum);
                res = writeTreeValue(type | i << 2, videoTree[SMK_TREE_TYPE], cacheValues[SMK_TREE_TYPE], res, bitNum);
if (deepDeb)
                LogErrorF("D1Smk::encodePixels treetype end offset%d bn%d", (size_t)res - (size_t)&frameData[cursor], bitNum);

                for (int n = 0; n < sizetable[i]; n++) {
                    switch (type & 3) {
                    case 0: { // 2COLOR BLOCK  SMK_TREE_MMAP/SMK_TREE_MCLR
                        unsigned color1, color2, colors = 0;
color2 = 256;
                        color1 = frame->getPixel(x + 0, y + 0).getPaletteIndex();
                        for (int yy = 4 - 1; yy >= 0; yy--) {
                            for (int xx = 4 - 1; xx >= 0; xx--) {
                                colors <<= 1;
                                unsigned color = frame->getPixel(x + xx, y + yy).getPaletteIndex();
                                if (color == color1) {
                                    colors |= 1;
                                } else {
if (color2 != 256 && color2 != color)
LogErrorF("ERROR D1Smk::encodePixels not 2color %d:%d vs %d at %d:%d", color1, color2, color, x, y);
                                    color2 = color;
                                }
                            }
                        }
//                        LogErrorF("D1Smk::encodePixels 2color %d:%d offset%d bn%d", color1, color2, (size_t)res - (size_t)&frameData[cursor], bitNum);
                        res = writeTreeValue(color1 << 8 | color2, videoTree[SMK_TREE_MCLR], cacheValues[SMK_TREE_MCLR], res, bitNum);
//                        LogErrorF("D1Smk::encodePixels colors%d offset%d bn%d", colors, (size_t)res - (size_t)&frameData[cursor], bitNum);
                        res = writeTreeValue(colors, videoTree[SMK_TREE_MMAP], cacheValues[SMK_TREE_MMAP], res, bitNum);
//                        LogErrorF("D1Smk::encodePixels ok offset%d bn%d", (size_t)res - (size_t)&frameData[cursor], bitNum);
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
if (x != sx || y != sy) {
    LogErrorF("ERROR D1Smk::encodePixels not complete %d:%d vs %d:%d len%d", sx, sy, x, y, slen);
}
	cursor = (size_t)res - (size_t)frameData;
}

static unsigned char oldPalette[D1SMK_COLORS][3];
static unsigned encodePalette(D1Pal *pal, int frameNum, uint8_t *dest)
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
LogErrorF("ERROR D1Smk::save non-matching palette %d[%d]: %d vs %d", i, n, cv, p[0]);
                        if (p[0] - cv > cv - p[-1]) {
                            p--;
                        }
                        const char *compontent[3] = { "red", "green", "blue"};
                        dProgressWarn() << QApplication::tr("Could not find matching color value for the %1 component of color %2 in the palette of frame %3. Using %4 instead of %5.").arg(compontent[n]).arg(i).arg(frameNum + 1).arg(*p).arg(cv);
                    }
                    break;
                }
            }
// LogErrorF("D1Smk::save palette value for %d:%d is %d vs. %d at %d", i, n, cv, p[0], (size_t)p - (size_t)&palmap[0]);
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
// LogErrorF("D1Smk::save first palette %d to %d", len, dest);
        memcpy(dest, newPalette, len);
// LogErrorF("D1Smk::saved first palette");
    }
// LogErrorF("D1Smk::saving keepsake palette 0 %d", len);
    memcpy(oldPalette, newPalette, sizeof(newPalette));
// LogErrorF("D1Smk::saved keepsake palette 1 %d", len);
// LogErrorF("D1Smk::save - 1 encoded palette len %d", len);
// LogErrorF("D1Smk::save - 2 encoded palette len %d", len);
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

bool D1Smk::save(D1Gfx &gfx, const SaveAsParam &params)
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
LogErrorF("D1Smk::save 0 %dx%d %d", width, height, frameCount);
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
LogErrorF("D1Smk::save 1 %d", frameInfo.count());
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
LogErrorF("D1Smk::save 2 fl%d br%d bd%d c%d cmp%d len%d... %d", frameLen, audioInfo[0].bitRate, audioInfo[0].bitDepth, audioInfo[0].channels, audioInfo[0].compress, audioInfo[0].maxChunkLength, audioInfo[1].maxChunkLength);
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
videoTree[i].VideoTreeIndex = i;
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
//if (n == 1)
//LogErrorF("D1Smk::prepTree 2color %d:%d, %d offset%d bn%d", color1, color2, colors);
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
    //uint8_t *videoTreeData = (uint8_t *)malloc(1);
    //size_t allocSize = 1, cursor = 0; unsigned bitNum = 0;
    size_t allocSize = 1 + (1 + (UINT8_MAX + 1) * 1 * 2 * 8 + 1) + (1 + (UINT8_MAX + 1) * 1 * 2 * 8 + 1) + 3 * sizeof(uint16_t) * 8 + ((UINT16_MAX + 1) * sizeof(uint16_t) * 2 * 8) + 1;
    allocSize *= 4;
    allocSize = (allocSize + 7) / 8;
    uint8_t *videoTreeData = (uint8_t *)calloc(1, allocSize);
    size_t cursor = 0; unsigned bitNum = 0;
    for (int i = 0; i < SMK_TREE_COUNT; i++) {
LogErrorF("D1Smk::save 4:%d as%d c%d bn%d ts%d cs%d:%d:%d (tc%d:%d:%d)", i, allocSize, cursor, bitNum,
        videoTree[i].treeStat.count(), videoTree[i].cacheStat[0].count(), videoTree[i].cacheStat[1].count(), videoTree[i].cacheStat[2].count(), videoTree[i].cacheCount[0], videoTree[i].cacheCount[1], videoTree[i].cacheCount[2]);
        videoTreeData = prepareVideoTree(videoTree[i], videoTreeData, allocSize, cursor, bitNum);
        if (videoTreeData == nullptr) {
            dProgressFail() << QApplication::tr("Out of memory");
            return false;
        }
LogErrorF("D1Smk::save 5:%d as%d c%d bn%d jc%d pc%d cs%d:%d:%d (tc%d:%d:%d)", i, allocSize, cursor, bitNum, videoTree[i].uncompressedTreeSize,
        videoTree[i].paths.count(), videoTree[i].cacheStat[0].count(), videoTree[i].cacheStat[1].count(), videoTree[i].cacheStat[2].count(), videoTree[i].cacheCount[0], videoTree[i].cacheCount[1], videoTree[i].cacheCount[2]);
    }
    unsigned videoTreeDataSize = cursor;
    if (bitNum != 0) {
        videoTreeDataSize++;
    }
LogErrorF("D1Smk::save 6: siz%d as%d", videoTreeDataSize, allocSize);
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
LogErrorF("D1Smk::save 7[%d]:%d", i, videoTree[i].uncompressedTreeSize);
        header.VideoTreeSize[i] = SwapLE32(videoTree[i].uncompressedTreeSize);
    }
LogErrorF("D1Smk::save 7e");
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
LogErrorF("D1Smk::save 8:%d", maxAudioLength);
    outFile.write((const char*)&header, sizeof(header));
    for (int i = 0; i < frameCount; i++) {
        outFile.write((const char*)&header.Dummy, 4);
    }
    for (int i = 0; i < frameCount; i++) {
        outFile.write((const char*)&frameInfo[i].FrameType, 1);
    }
LogErrorF("D1Smk::save 9:%d", videoTreeDataSize);
    outFile.write((const char*)videoTreeData, videoTreeDataSize);
LogErrorF("D1Smk::save 10:%d", 256 * 3 + maxAudioLength + 4 * width * height);
    /*D1GfxFrame **/ prevFrame = nullptr;
    uint8_t *frameData = (uint8_t*)calloc(1, D1SMK_COLORS * 3 + maxAudioLength + 4 * width * height);
    QList<uint32_t> frameLengths;
    for (int n = 0; n < frameCount; n++) {
LogErrorF("D1Smk::save frame %d dp%d to %d", n, frameData, outFile.pos());
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
LogErrorF("D1Smk::save encode palette of frame %d offset %d", n, cursor + 1);
            unsigned pallen = encodePalette(framePal, n, frameData + cursor + 1);
LogErrorF("D1Smk::save encoded palette len %d", pallen, (pallen + 1 + 3) & ~3);
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
LogErrorF("D1Smk::save encode audio:%d[%d] offset %d", n, i, cursor + 4);
                    size_t audiolen = encodeAudio(data, length, audioInfo[i], frameData + cursor + 4);
LogErrorF("D1Smk::save encoded len:%d", audiolen);
                    audiolen += 4;
                    *(uint32_t*)&frameData[cursor] = SwapLE32(audiolen);
                    cursor += audiolen;
                }
            }
        }
LogErrorF("D1Smk::save pixels of frame %d offset%d", n, cursor);
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
if (n == 0) {
	deepDeb = true;
 LogErrorF("D1Smk::save encode pixels %d:%d type%d len%d from %d;%d", x, y, type, typelen, cursor, bitNum);
}
                    encodePixels(x, y, frame, type, typelen, videoTree, cacheValues, frameData, cursor, bitNum);
if (n == 0) {
	deepDeb = false;
LogErrorF("D1Smk::save encoded pixels:%d;%d", cursor, bitNum);
}

                    type = ctype;
                    typelen = 0;
                }
                typelen++;
            }
        }
if (n == 0) {
	deepDeb = true;
LogErrorF("D1Smk::save encode pixels %d:%d type%d len%d from %d;%d", width, height - 4, type, typelen, cursor, bitNum);
}
        encodePixels(0, height, frame, type, typelen, videoTree, cacheValues, frameData, cursor, bitNum);
if (n == 0) {
	deepDeb = false;
LogErrorF("D1Smk::save encoded last pixels:%d;%d", cursor, bitNum);
}
        if (bitNum != 0) {
            cursor++;
        }
        cursor = (cursor + 3) & ~3;
LogErrorF("D1Smk::saved frame %d size%d", n, cursor);
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
                        pal->setColor(i, col);
                        if (fix.colors.isEmpty() || fix.colors.back() != i) {
                            fix.colors.push_back(i);
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
        QString msg = QApplication::tr("Ignored the %1 undefined color(s) in the palette", "", numIgnored).arg(ignoredColors);
        msg = addDetails(msg, verbose, fix);
        dProgressWarn() << msg;
    }
    if (fix.colors.isEmpty()) {
        QString msg = QApplication::tr("The palette");
        msg = addDetails(msg, verbose, fix);
        msg.chop(1);
        msg.append(QApplication::tr(" is SMK compliant"));
        dProgress() << msg;
        return false;
    }
    return true;
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

    for (D1Gfx *gfx : gfxs) {
        // adjust colors of the palette(s)
        D1SmkColorFix cf;
        cf.pal = p;
        cf.gfx = gfx;
        cf.frameFrom = 0;
        int i = 0;
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
            QPointer<D1Pal> &fp = cf.gfx->getFrame(i)->getFramePal();
            if (!fp.isNull()) {
                D1Pal *cp = fp.data();
                if (pal != nullptr) {
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
                }
                pal = cp;
            }
            
        }
    }
}