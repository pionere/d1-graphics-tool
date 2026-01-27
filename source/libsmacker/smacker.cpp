/**
	libsmacker - A C library for decoding .smk Smacker Video files
	Copyright (C) 2012-2021 Greg Kennedy

	See smacker.h for more information.

	smacker.c
		Main implementation file of libsmacker.
		Open, close, query, render, advance and seek an smk
*/

#include "smacker.h"

#include "smk_malloc.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <cstdarg>

#include <QApplication>
#include <QByteArray>
#include "progressdialog.h"
#include "../dungeon/defs.h"
#include "../dungeon/interfac.h"

/* endianness */
#define smk_swap_le16(X) SwapLE16(X)
#define smk_swap_le32(X) SwapLE32(X)

#ifdef FULL
#undef DEBUG_MODE
#define DEBUG_MODE 1
#endif

/* logging replacements */
#if DEBUG_MODE
#define PrintError(msg)    perror(msg);
#define LogErrorMsg(msg)   fputs(msg, stderr);
#define LogError(msg, ...) fprintf(stderr, msg, __VA_ARGS__);
#else
static void LogErrorSF(const QString msg, ...)
{
	char tmp[256];
	QByteArray ba = msg.toLocal8Bit();
	va_list va;

	va_start(va, msg);

	vsnprintf(tmp, sizeof(tmp), ba.data(), va);

	va_end(va);

	dProgressErr() << QString(tmp);
}
#define PrintError(msg) \
{ \
	dProgressErr() << QApplication::tr(msg); \
}
#define LogErrorMsg(msg) \
{ \
	dProgressErr() << QApplication::tr(msg); \
}
#define LogError(msg, ...) \
{ \
	LogErrorSF(QApplication::tr(msg), __VA_ARGS__); \
}
#endif // DEBUG_MODE

/* ************************************************************************* */
/* BITSTREAM Structure */
/* ************************************************************************* */
/* Wraps a block of memory and adds functions to read 1 or 8 bits at a time */
struct smk_bit_t {
	const unsigned char * buffer, * end;
	unsigned int bit_num;
};

/* ************************************************************************* */
/* BITSTREAM Functions */
/* ************************************************************************* */
/** Initialize a bitstream wrapper */
static void smk_bs_init(struct smk_bit_t * const bs, const unsigned char * const b, const size_t size)
{
	/* null check */
	assert(bs);
	assert(b);
	/* set up the pointer to bitstream start and end, and set the bit pointer to 0 */
	bs->buffer = b;
	bs->end = b + size;
	bs->bit_num = 0;
}

/* Reads a bit
	Returns -1 if error encountered */
static int smk_bs_read_1(struct smk_bit_t * const bs)
{
	int ret;
	/* null check */
	assert(bs);

	/* don't die when running out of bits, but signal */
	if (bs->buffer >= bs->end) {
		LogErrorMsg("libsmacker::smk_bs_read_1(): ERROR: bitstream exhausted.\n");
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
static int smk_bs_read_8(struct smk_bit_t * const bs)
{
	/* null check */
	assert(bs);

	/* don't die when running out of bits, but signal */
	if (bs->buffer + (bs->bit_num > 0) >= bs->end) {
		LogErrorMsg("libsmacker::smk_bs_read_8(): ERROR: bitstream exhausted.\n");
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

static bool deepDebug = false;
static unsigned char *bufMem;
static size_t bufSize;
// static bool treeValue[48];
static unsigned short fullLeafs[] = {
	/*(22 | 99 << 8),
	(0  | 217 << 8),
	(20 | 236 << 8),
    (21 | 236 << 8),
    (0  | 210 << 8),
    (205| 236 << 8),
	(29 | 33 << 8),
    (19 | 35 << 8),
	(48 | 43 << 8),
	(36 | 36 << 8),
	(35 | 23 << 8),
	(40 | 18 << 8),
    (35 | 19 << 8),
	(33 | 13 << 8),
	(27 | 16 << 8),
	(38 | 29 << 8),
    (19 | 38 << 8),
    (33 | 35 << 8),
	(22 | 22 << 8),
    (26 | 26 << 8),
    (13 | 10 << 8),
    (15 | 11 << 8),
    (16 | 17 << 8),
    (14 | 17 << 8),
    (16 | 10 << 8),
    (20 | 20 << 8),
    (10 | 15 << 8),
    (14 | 19 << 8),
	(28 | 30 << 8),
    (29 | 22 << 8),
    (22 | 36 << 8),
    (19 | 35 << 8),
	(35 | 25 << 8),
    (36 | 39 << 8),
    (29 | 33 << 8),
	(20 | 18 << 8),
    (19 | 18 << 8),
	(20 | 182 << 8),*/
	(12 | 13 << 8),
	(13 | 12 << 8),
	(16 | 27 << 8),
	(25 | 28 << 8),
	(28 | 25 << 8),
	(27 | 16 << 8),
	(11 | 22 << 8),
	(23 | 11 << 8),
	(14 | 13 << 8),
	(13 | 22 << 8),
	(14 | 20 << 8),
	(13 | 18 << 8),
	(13 | 19 << 8),
	(10 | 23 << 8),
	(18 | 13 << 8),
	(18 | 98 << 8),
	(10 | 100 << 8),
	(96 | 14 << 8),

	// (222,11)
	// (21,11) (38,30)
};

static void LogErrorFF(const char* msg, ...)
{
	char tmp[256];
	char tmsg[256];
	va_list va;

	va_start(va, msg);

	vsnprintf(tmsg, sizeof(tmsg), msg, va);

	va_end(va);

	// dProgressErr() << QString(tmsg);
	
	snprintf(tmp, sizeof(tmp), "f:\\logdebug%d.txt", 0);
	FILE* f0 = fopen(tmp, "a+");
	if (f0 == NULL)
		return;

	fputs(tmsg, f0);

	fputc('\n', f0);

	fclose(f0);
}

/* ************************************************************************* */
/* HUFF8 Structure */
/* ************************************************************************* */
#define SMK_HUFF8_BRANCH 0x8000
#define SMK_HUFF8_LEAF_MASK 0x7FFF

struct smk_huff8_t {
	/* Unfortunately, smk files do not store the alloc size of a small tree.
		511 entries is the pessimistic case (N codes and N-1 branches,
		with N=256 for 8 bits) */
	size_t size;
	unsigned short tree[511];
};

/* ************************************************************************* */
/* HUFF8 Functions */
/* ************************************************************************* */
/* Recursive sub-func for building a tree into an array. */
static int _smk_huff8_build_rec(struct smk_huff8_t * const t, struct smk_bit_t * const bs)
{
	int bit, value;
	assert(t);
	assert(bs);

	/* Make sure we aren't running out of bounds */
	if (t->size >= 511) {
		LogErrorMsg("libsmacker::_smk_huff8_build_rec() - ERROR: size exceeded\n");
		return 0;
	}

	/* Read the next bit */
	if ((bit = smk_bs_read_1(bs)) < 0) {
		LogErrorMsg("libsmacker::_smk_huff8_build_rec() - ERROR: get_bit returned -1\n");
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
		if (! _smk_huff8_build_rec(t, bs)) {
			LogErrorMsg("libsmacker::_smk_huff8_build_rec() - ERROR: failed to build left sub-tree\n");
			return 0;
		}

		/* now go back to our current location, and
			mark our location as a "jump" */
		t->tree[value] = SMK_HUFF8_BRANCH | t->size;

		/* continue building the right side */
		if (! _smk_huff8_build_rec(t, bs)) {
			LogErrorMsg("libsmacker::_smk_huff8_build_rec() - ERROR: failed to build right sub-tree\n");
			return 0;
		}
	} else {
		/* Bit unset signifies a Leaf node. */
		/* Attempt to read value */
		if ((value = smk_bs_read_8(bs)) < 0) {
			LogErrorMsg("libsmacker::_smk_huff8_build_rec() - ERROR: get_byte returned -1\n");
			return 0;
		}

//if (deepDebug)
//LogErrorFF("smk_huff8_build leaf[%d]=%d (%d:%d:%d)", t->size, value);
		/* store to tree */
		t->tree[t->size ++] = value;
	}

	return 1;
}

/**
	Build an 8-bit Hufftree out of a Bitstream.
*/
static int smk_huff8_build(struct smk_huff8_t * const t, struct smk_bit_t * const bs)
{
	int bit;
	/* null check */
	assert(t);
	assert(bs);

	/* Smacker huff trees begin with a set-bit. */
	if ((bit = smk_bs_read_1(bs)) < 0) {
		LogErrorMsg("libsmacker::smk_huff8_build() - ERROR: initial get_bit returned -1\n");
		return 0;
	}

	/* OK to fill out the struct now */
	t->size = 0;

	/* First bit indicates whether a tree is present or not. */
	/*  Very small or audio-only files may have no tree. */
	if (bit) {
		if (! _smk_huff8_build_rec(t, bs)) {
			LogErrorMsg("libsmacker::smk_huff8_build() - ERROR: tree build failed\n");
			return 0;
		}
	} else
		t->tree[0] = 0;

	/* huff trees end with an unset-bit */
	if ((bit = smk_bs_read_1(bs)) < 0) {
		LogErrorMsg("libsmacker::smk_huff8_build() - ERROR: final get_bit returned -1\n");
		return 0;
	}

	/* a 0 is expected here, a 1 generally indicates a problem! */
	if (bit) {
		LogErrorMsg("libsmacker::smk_huff8_build() - ERROR: final get_bit returned 1\n");
		return 0;
	}

	return 1;
}

/* Look up an 8-bit value from a basic huff tree.
	Return -1 on error. */
static int smk_huff8_lookup(const struct smk_huff8_t * const t, struct smk_bit_t * const bs)
{
	int bit, index = 0;
	/* null check */
	assert(t);
	assert(bs);

	while (t->tree[index] & SMK_HUFF8_BRANCH) {
		if ((bit = smk_bs_read_1(bs)) < 0) {
			LogErrorMsg("libsmacker::smk_huff8_lookup() - ERROR: get_bit returned -1\n");
			return -1;
		}

		if (bit) {
			/* take the right branch */
			index = t->tree[index] & SMK_HUFF8_LEAF_MASK;
		} else {
			/* take the left branch */
			index ++;
		}
	}

	/* at leaf node.  return the value at this point. */
	return t->tree[index];
}

/* ************************************************************************* */
/* HUFF16 Structure */
/* ************************************************************************* */
#define SMK_HUFF16_BRANCH    0x80000000
#define SMK_HUFF16_CACHE     0x40000000
#ifdef FULL
#define SMK_HUFF16_LEAF_MASK 0x3FFFFFFF
#else
#define SMK_HUFF16_LEAF_MASK 0x7FFFFFFF
#define SMK_HUFF16_CACHE_MSK 0x3
#endif

struct smk_huff16_t {
	unsigned int * tree;
	size_t size;

	/* recently-used values cache */
	unsigned short cache[3];
};

/* ************************************************************************* */
/* HUFF16 Functions */
/* ************************************************************************* */
/* Recursive sub-func for building a tree into an array. */
static int _smk_huff16_build_rec(struct smk_huff16_t * const t, struct smk_bit_t * const bs, const struct smk_huff8_t * const low8, const struct smk_huff8_t * const hi8, const size_t limit, int depth)
{
	int bit, value;
	assert(t);
	assert(bs);
	assert(low8);
	assert(hi8);

	/* Make sure we aren't running out of bounds */
	if (t->size >= limit) {
		LogErrorMsg("libsmacker::_smk_huff16_build_rec() - ERROR: size exceeded\n");
		return 0;
	}

	/* Read the first bit */
	if ((bit = smk_bs_read_1(bs)) < 0) {
		LogErrorMsg("libsmacker::_smk_huff16_build_rec() - ERROR: get_bit returned -1\n");
		return 0;
	}

	if (bit) {
		/* See tree-in-array explanation for HUFF8 above */
		/* track the current index */
		value = t->size ++;

		/* go build the left branch */
		if (! _smk_huff16_build_rec(t, bs, low8, hi8, limit, depth + 1)) {
			LogErrorMsg("libsmacker::_smk_huff16_build_rec() - ERROR: failed to build left sub-tree\n");
			return 0;
		}

		/* now go back to our current location, and
			mark our location as a "jump" */
		t->tree[value] = SMK_HUFF16_BRANCH | t->size;
//		treeValue[depth] = true;
		/* continue building the right side */
		if (! _smk_huff16_build_rec(t, bs, low8, hi8, limit, depth + 1)) {
			LogErrorMsg("libsmacker::_smk_huff16_build_rec() - ERROR: failed to build right sub-tree\n");
			return 0;
		}
//		treeValue[depth] = false;
	} else {
		/* Bit unset signifies a Leaf node. */
		/* Attempt to read LOW value */
		if ((value = smk_huff8_lookup(low8, bs)) < 0) {
			LogErrorMsg("libsmacker::_smk_huff16_build_rec() - ERROR: get LOW value returned -1\n");
			return 0;
		}

		t->tree[t->size] = value;

		/* now read HIGH value */
		if ((value = smk_huff8_lookup(hi8, bs)) < 0) {
			LogErrorMsg("libsmacker::_smk_huff16_build_rec() - ERROR: get HIGH value returned -1\n");
			return 0;
		}

		/* Looks OK: we got low and hi values. Return a new LEAF */
		t->tree[t->size] |= (value << 8);
// bool deepDebug = t->tree[t->size] == t->cache[0] || t->tree[t->size] == t->cache[1] || t->tree[t->size] == t->cache[2];
/*if (deepDebug) {
// LogErrorFF("smk_huff16_build leaf[%d]=%d (%d,%d) d%d c(%d:%d:%d)", t->size, t->tree[t->size], t->tree[t->size] & 0xFF, value, depth, t->tree[t->size] == t->cache[0], t->tree[t->size] == t->cache[1], t->tree[t->size] == t->cache[2]);
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
		LogErrorFF("smk_huff16_build leaf[%d]=%d (%d,%d) d%d %d c%d:%d:%d", t->size, t->tree[t->size], t->tree[t->size] & 0xFF, value, depth, *(unsigned*)(&bytes[0]), t->tree[t->size] == t->cache[0], t->tree[t->size] == t->cache[1], t->tree[t->size] == t->cache[2]);
    // }
}*/
		/* Last: when building the tree, some Values may correspond to cache positions.
			Identify these values and set the Escape code byte accordingly. */
		if (t->tree[t->size] == t->cache[0])
			t->tree[t->size] = SMK_HUFF16_CACHE;
		else if (t->tree[t->size] == t->cache[1])
			t->tree[t->size] = SMK_HUFF16_CACHE | 1;
		else if (t->tree[t->size] == t->cache[2])
			t->tree[t->size] = SMK_HUFF16_CACHE | 2;

		t->size ++;
	}

	return 1;
}

/* Entry point for building a big 16-bit tree. */
static int smk_huff16_build(struct smk_huff16_t * const t, struct smk_bit_t * const bs, const unsigned int alloc_size)
{
	struct smk_huff8_t low8, hi8;
	size_t limit;
	int value, i, bit;
	/* null check */
	assert(t);
	assert(bs);

	/* Smacker huff trees begin with a set-bit. */
	if ((bit = smk_bs_read_1(bs)) < 0) {
		LogErrorMsg("libsmacker::smk_huff16_build() - ERROR: initial get_bit returned -1\n");
		return 0;
	}

	t->size = 0;

	/* First bit indicates whether a tree is present or not. */
	/*  Very small or audio-only files may have no tree. */
	if (bit) {
// LogErrorFF("smk_huff16_build 1 %d", alloc_size);
		/* build low-8-bits tree */
		if (! smk_huff8_build(&low8, bs)) {
			LogErrorMsg("libsmacker::smk_huff16_build() - ERROR: failed to build LOW tree\n");
			return 0;
		}
// LogErrorFF("smk_huff16_build 2");
		/* build hi-8-bits tree */
		if (! smk_huff8_build(&hi8, bs)) {
			LogErrorMsg("libsmacker::smk_huff16_build() - ERROR: failed to build HIGH tree\n");
			return 0;
		}
// LogErrorFF("smk_huff16_build 3");
		/* Init the escape code cache. */
		for (i = 0; i < 3; i ++) {
			if ((value = smk_bs_read_8(bs)) < 0) {
				LogError("libsmacker::smk_huff16_build() - ERROR: get LOW value for cache %d returned -1\n", i);
				return 0;
			}

			t->cache[i] = value;

			/* now read HIGH value */
			if ((value = smk_bs_read_8(bs)) < 0) {
				LogError("libsmacker::smk_huff16_build() - ERROR: get HIGH value for cache %d returned -1\n", i);
				return 0;
			}

			t->cache[i] |= (value << 8);
if (deepDebug)
LogErrorFF("smk_huff16_build cache[%d]:%d", i, t->cache[i]);
		}
		/* Everything looks OK so far. Time to malloc structure. */
		if (alloc_size < 12 || alloc_size % 4) {
			LogError("libsmacker::smk_huff16_build() - ERROR: illegal value %u for alloc_size\n", alloc_size);
			return 0;
		}
// LogErrorFF("smk_huff16_build 4");
		limit = (alloc_size - 12) / 4;
		if ((t->tree = (unsigned int*)malloc(limit * sizeof(unsigned int))) == NULL) {
			PrintError("libsmacker::smk_huff16_build() - ERROR: failed to malloc() huff16 tree");
			return 0;
		}

		/* Finally, call recursive function to retrieve the Bigtree. */
		if (! _smk_huff16_build_rec(t, bs, &low8, &hi8, limit, 0)) {
			LogErrorMsg("libsmacker::smk_huff16_build() - ERROR: failed to build huff16 tree\n");
			goto error;
		}
// LogErrorFF("smk_huff16_build 5");
		/* check that we completely filled the tree */
		if (limit != t->size) {
// LogErrorFF("failed to completely decode huff16 tree %d vs %d", limit, t->size);
			LogErrorMsg("libsmacker::smk_huff16_build() - ERROR: failed to completely decode huff16 tree\n");
			goto error;
		}
	} else {
		if ((t->tree = (unsigned int*)malloc(sizeof(unsigned int))) == NULL) {
			PrintError("libsmacker::smk_huff16_build() - ERROR: failed to malloc() huff16 tree");
			return 0;
		}

		t->tree[0] = 0;
		//t->cache[0] = t->cache[1] = t->cache[2] = 0;
	}

	/* Check final end tag. */
	if ((bit = smk_bs_read_1(bs)) < 0) {
		LogErrorMsg("libsmacker::smk_huff16_build() - ERROR: final get_bit returned -1\n");
		goto error;
	}

	/* a 0 is expected here, a 1 generally indicates a problem! */
	if (bit) {
		LogErrorMsg("libsmacker::smk_huff16_build() - ERROR: final get_bit returned 1\n");
		goto error;
	}

	return 1;
error:
	free(t->tree);
	t->tree = NULL;
	return 0;
}

/* Look up a 16-bit value from a large huff tree.
	Return -1 on error.
	Note that this also updates the recently-used-values cache. */
static int smk_huff16_lookup(struct smk_huff16_t * const t, struct smk_bit_t * const bs)
{
	int bit, value, index = 0;
	/* null check */
	assert(t);
	assert(bs);
int depth = 0;
	while (t->tree[index] & SMK_HUFF16_BRANCH) {
		if ((bit = smk_bs_read_1(bs)) < 0) {
			LogErrorMsg("libsmacker::smk_huff16_lookup() - ERROR: get_bit returned -1\n");
			return -1;
		}
		depth++;
//if (deepDebug)
//LogErrorFF("smk_huff16_lookup idx %d value%d bit%d", index, t->tree[index], bit);
		if (bit) {
			/* take the right branch */
			index = t->tree[index] & SMK_HUFF16_LEAF_MASK;
		} else {
			/* take the left branch */
			index ++;
		}
	}

	/* Get the value at this point */
	value = t->tree[index];
//if (deepDebug)
//LogErrorFF("smk_huff16_lookup value %d cache%d depth%d ", value, (value & SMK_HUFF16_CACHE) != 0, depth);
	if (value & SMK_HUFF16_CACHE) {
		/* uses cached value instead of actual value */
		value = t->cache[value & SMK_HUFF16_CACHE_MSK];
	}
if (deepDebug)
LogErrorFF("smk_bw_skip(&bw, %d); // mem (%d:%d) value (%d,%d)", depth, (size_t)bs->buffer - (size_t)bufMem - depth / 8, bs->bit_num, value & 0xFF, (value >> 8) & 0xFF);
	if (t->cache[0] != value) {
		/* Update the cache, by moving val to the front of the queue,
			if it isn't already there. */
		t->cache[2] = t->cache[1];
		t->cache[1] = t->cache[0];
		t->cache[0] = value;
	}

	return value;
}

/* ************************************************************************* */
/* SMACKER Structure */
/* ************************************************************************* */
/* tree processing order */
#ifdef FULL
#define SMK_TREE_MMAP	0
#define SMK_TREE_MCLR	1
#define SMK_TREE_FULL	2
#define SMK_TREE_TYPE	3
#endif

struct smk_t {
	/* meta-info */
#ifdef FULL
	/* file mode: see flags, smacker.h */
	unsigned char	mode;
#endif
	/* microsec per frame - stored as a double to handle scaling
		(large positive millisec / frame values may overflow a ul) */
	double	usf;
#ifdef FULL
	/* total frames */
	unsigned long	f;
	/* does file have a ring frame? (in other words, does file loop?) */
	unsigned char	ring_frame;
#else
	unsigned long	total_frames; /* f + ring_frame */
#endif
	/* Index of current frame */
	unsigned long	cur_frame;

	/* SOURCE union.
		Where the data is going to be read from (or be stored),
		depending on the file mode. */
	union {
#ifdef FULL
		struct {
			/* on-disk mode */
			FILE * fp;
			unsigned long * chunk_offset;
		} file;
#endif
		/* in-memory mode: unprocessed chunks */
		unsigned char ** chunk_data;
	} source;

	/* shared array of "chunk sizes"*/
	unsigned long * chunk_size;

#ifdef FULL
	/* Holds per-frame flags (i.e. 'keyframe') */
	unsigned char * keyframe;
#endif
	/* Holds per-frame type mask (e.g. 'audio track 3, 2, and palette swap') */
	unsigned char * frame_type;

	/* video and audio structures */
	/* Video data type: enable/disable decode switch,
		video info and flags,
		pointer to last-decoded-palette */
	struct smk_video_t {
		/* enable/disable decode switch */
		unsigned char enable;

		/* video info */
		unsigned long	w;
		unsigned long	h;
#ifdef FULL
		/* Y scale mode (constants defined in smacker.h)
			0: unscaled
			1: doubled
			2: interlaced */
		unsigned char	y_scale_mode;

		/* version ('2' or '4') */
		unsigned char	v;

		/* Huffman trees */
		unsigned long tree_size[4];
#else
		/* version ('2' or '4') */
		unsigned char	v;
#endif
		struct smk_huff16_t tree[SMK_TREE_COUNT];

		/* Palette data type: pointer to last-decoded-palette */
#ifdef FULL
		unsigned char palette[256][3];
#else
		unsigned char palette[256][4];
#endif
		/* Last-unpacked frame */
		unsigned char * frame;
	} video;

	/* audio structure */
	struct smk_audio_t {
#ifdef FULL
		/* set if track exists in file */
		unsigned char exists;
#endif

		/* enable/disable switch (per track) */
		unsigned char enable;

		/* Info */
		unsigned char	channels;
		unsigned char	bitdepth;
		unsigned long	rate;
		unsigned long	max_buffer;

		/* compression type
			0: raw PCM
			1: SMK DPCM
			2: Bink (Perceptual), unsupported */
		unsigned char	compress;

		/* pointer to last-decoded-audio-buffer */
		void * buffer;
		unsigned long	buffer_size;
	} audio[SMK_AUDIO_TRACKS];
};

union smk_read_t {
#ifdef FULL
	FILE * file;
#endif
	unsigned char * ram;
};

/* ************************************************************************* */
/* SMACKER Functions */
/* ************************************************************************* */
/* An fread wrapper: consumes N bytes, or returns -1
	on failure (when size doesn't match expected) */
#ifdef FULL
static char smk_read_file(void * buf, const size_t size, FILE * fp)
{
	/* don't bother checking buf or fp, fread does it for us */
	size_t bytesRead = fread(buf, 1, size, fp);

	if (bytesRead != size) {
		LogError("libsmacker::smk_read_file(buf,%lu,fp) - ERROR: Short read, %lu bytes returned\n", (unsigned long)size, (unsigned long)bytesRead);
		PrintError("\tReason");
		return -1;
	}

	return 0;
}
#endif
/* A memcpy wrapper: consumes N bytes, or returns -1
	on failure (when size too low) */
static char smk_read_memory(void * buf, const unsigned long size, unsigned char ** p, unsigned long * p_size)
{
	if (size > *p_size) {
		LogError("libsmacker::smk_read_memory(buf,%lu,p,%lu) - ERROR: Short read\n", (unsigned long)size, (unsigned long)*p_size);
		return -1;
	}

	memcpy(buf, *p, size);
	*p += size;
	*p_size -= size;
	return 0;
}
#ifndef FULL_ORIG
static char smk_read_in_memory(unsigned char ** buf, const unsigned long size, unsigned char ** p, unsigned long * p_size)
{
	if (size > *p_size) {
		LogError("libsmacker::smk_read_in_memory(buf,%lu,p,%lu) - ERROR: Short read\n", (unsigned long)size, (unsigned long)*p_size);
		return -1;
	}

	*buf = *p;
	*p += size;
	*p_size -= size;
	return 0;
}
#endif
/* Helper functions to do the reading, plus
	byteswap from LE to host order */
/* read n bytes from (source) into ret */
#ifdef FULL
#define smk_read(ret,n) \
{ \
	if (m) \
	{ \
		r = (smk_read_file(ret,n,fp.file)); \
	} \
	else \
	{ \
		r = (smk_read_memory(ret,n,&fp.ram,&size)); \
	} \
	if (r < 0) \
	{ \
		LogError("libsmacker::smk_read(...) - Errors encountered on read, bailing out (file: %s, line: %lu)\n", __FILE__, (unsigned long)__LINE__); \
		goto error; \
	} \
}
#else
#define smk_read(ret,n) \
{ \
	{ \
		r = (smk_read_memory(ret,n,&fp.ram,&size)); \
	} \
	if (r < 0) \
	{ \
		LogError("libsmacker::smk_read(...) - Errors encountered on read, bailing out (file: %s, line: %lu)\n", __FILE__, (unsigned long)__LINE__); \
		goto error; \
	} \
}
#endif
#ifndef FULL_ORIG
#define smk_read_in(ret, n) \
{ \
	{ \
		r = (smk_read_in_memory(&ret,n,&fp.ram,&size)); \
	} \
	if (r < 0) \
	{ \
		LogError("libsmacker::smk_read_in(...) - Errors encountered on read, bailing out (file: %s, line: %lu)\n", __FILE__, (unsigned long)__LINE__); \
		goto error; \
	} \
}
#endif
/* Calls smk_read, but returns a ul */
#ifdef FULL
#define smk_read_ul(p) \
{ \
	smk_read(buf,4); \
	p = ((unsigned long) buf[3] << 24) | \
		((unsigned long) buf[2] << 16) | \
		((unsigned long) buf[1] << 8) | \
		((unsigned long) buf[0]); \
}
#else
#define smk_read_ul(p) \
{ \
	smk_read(buf,4); \
	p = smk_swap_le32(*(uint32_t*)&buf[0]); \
}
#endif

struct smk_bitw_t {
	unsigned char * buffer, * end;
	unsigned int bit_num;
};

static void smk_bw_init(struct smk_bitw_t * bs, unsigned char * b, const size_t size)
{
	/* null check */
	assert(bs);
	assert(b);
	/* set up the pointer to bitstream start and end, and set the bit pointer to 0 */
	bs->buffer = b;
	bs->end = b + size;
	bs->bit_num = 0;
}

static void smk_bw_skip(struct smk_bitw_t * bs, const size_t size)
{
	bs->buffer += size / 8;
	bs->bit_num += size % 8;
	if (bs->bit_num >= 8) {
		bs->bit_num -= 8;
		bs->buffer++;
    }
}

static void smk_bw_write(struct smk_bitw_t * bs, size_t value, const size_t size)
{
	for (unsigned i = 0; i < size; i++) {
		unsigned char v = *bs->buffer;
		v = 1 << bs->bit_num;
		if (value & 1) {
//if ((*bs->buffer & v) == 0) {
//	LogErrorFF("smk_bw_write 0 %d vs %d, i%d offset%d bit%d", *bs->buffer, v, i, (size_t)bs->buffer - (size_t)bufMem, bs->bit_num);
//}
			*bs->buffer |= v;
        } else {
//if ((*bs->buffer & v) != 0) {
//	LogErrorFF("smk_bw_write 1 %d vs %d, i%d offset%d bit%d", *bs->buffer, v, i, (size_t)bs->buffer - (size_t)bufMem, bs->bit_num);
//}
			*bs->buffer &= ~v;
        }
		bs->bit_num++;
		if (bs->bit_num >= 8) {
			bs->bit_num -= 8;
			bs->buffer++;
		}
		value >>= 1;
    }
}

static int patchCounter = 0;
static void patchFile()
{
	smk_bitw_t bw;

	smk_bw_init(&bw, bufMem, bufSize);

if (patchCounter == 0) {
	patchCounter++;

	bw.buffer += 2365112;
	bw.bit_num = 4;
/*
smk_bw_skip(&bw, 6); // (0,0)
// Full block 0:0 value0 (offsetend2365113 bitend2) 30,31:136 (0:0) = 0
smk_bw_write(&bw, 59, 6); // *5); // (0,0)
// Full block 0:1 value0 (offsetend2365113 bitend7) 28,29:136 (0:0) = 0
smk_bw_write(&bw, 24360, 15); // *20); // (22,99) -> 0:222
// Full block 1:0 value25366 (offsetend2365116 bitend3) 30,31:137 (22:99) = 25366
smk_bw_write(&bw, 59, 6); // *4); // (0,0)
// Full block 1:1 value0 (offsetend2365116 bitend7) 28,29:137 (0:0) = 0
smk_bw_write(&bw, 24360, 15); // *19); // (23,236) -> 0:222
// Full block 2:0 value60439 (offsetend2365119 bitend2) 30,31:138 (23:236) = 60439
smk_bw_write(&bw, 59, 6); // *4); // (0,0)
// Full block 2:1 value0 (offsetend2365119 bitend6) 28,29:138 (0:0) = 0
smk_bw_write(&bw, 59, 6); // *5); // (0,0)
// Full block 3:0 value0 (offsetend2365120 bitend3) 30,31:139 (0:0) = 0
smk_bw_write(&bw, 59, 6); // *5); // (0,0)
// Full block 3:1 value0 (offsetend2365121 bitend0) 28,29:139 (0:0) = 0
smk_bw_write(&bw, 59, 6); // *5); // (0,0)
// Full block 0:0 value0 (offsetend2365121 bitend5) 34,35:136 (0:0) = 0
smk_bw_write(&bw, 59, 6); // *5); // (0,0)
// Full block 0:1 value0 (offsetend2365122 bitend2) 32,33:136 (0:0) = 0
smk_bw_write(&bw, 24360, 15); // *19); // (89,236) -> 0:222
// Full block 1:0 value60505 (offsetend2365124 bitend5) 34,35:137 (89:236) = 60505
smk_bw_write(&bw, 24360, 15); // *11); // (236,236) -> 0:222
// Full block 1:1 value60652 (offsetend2365126 bitend0) 32,33:137 (236:236) = 60652
smk_bw_write(&bw, 24360, 15); // *19); // (236,82) -> 0:222
// Full block 2:0 value21228 (offsetend2365128 bitend3) 34,35:138 (236:82) = 21228
smk_bw_write(&bw, 24360, 15); // *20); // (82,236) -> 0:222
// Full block 2:1 value60498 (offsetend2365130 bitend7) 32,33:138 (82:236) = 60498
smk_bw_write(&bw, 24360, 15); // *14); // (222,223) -> 0:222
// Full block 3:0 value57310 (offsetend2365132 bitend5) 34,35:139 (222:223) = 57310
smk_bw_write(&bw, 91, 14); // (222,0)
// Full block 3:1 value222 (offsetend2365134 bitend3) 32,33:139 (222:0) = 222
smk_bw_write(&bw, 15651, 14); // *13); // (49,43) -> 50:57
// Full block 0:0 value11057 (offsetend2365136 bitend0) 38,39:136 (49:43) = 11057
smk_bw_write(&bw, 7801, 13); // (0,37)
// Full block 0:1 value9472 (offsetend2365137 bitend5) 36,37:136 (0:37) = 9472
smk_bw_write(&bw, 1794, 11); // (128,128) -> 43:43
// Full block 1:0 value32896 (offsetend2365139 bitend0) 38,39:137 (128:128) = 32896
smk_bw_write(&bw, 48822, 19); // (236,128) -> 222:25
// Full block 1:1 value33004 (offsetend2365141 bitend3) 36,37:137 (236:128) = 33004
smk_bw_write(&bw, 1773, 11); // *5); // (236,128) -> 25:30
// Full block 2:0 value33004 (offsetend2365142 bitend0) 38,39:138 (236:128) = 33004
smk_bw_write(&bw, 196786, 18); // (236,74) -> 10:222
// Full block 2:1 value19180 (offsetend2365144 bitend2) 36,37:138 (236:74) = 19180
smk_bw_write(&bw, 164023, 18); // (222,37)
// Full block 3:0 value9694 (offsetend2365146 bitend4) 38,39:139 (222:37) = 9694
smk_bw_write(&bw, 91, 14); // (222,223) -> 222:0
// Full block 3:1 value57310 (offsetend2365148 bitend2) 36,37:139 (222:223) = 57310
smk_bw_write(&bw, 3790, 14); // *11); // (30,25) -> 57:50
// Full block 0:0 value6430 (offsetend2365149 bitend5) 42,43:136 (30:25) = 6430
smk_bw_write(&bw, 360, 9); // *11); // (37,37) -> 57:57
// Full block 0:1 value9509 (offsetend2365151 bitend0) 40,41:136 (37:37) = 9509
smk_bw_write(&bw, 1794, 11); // (128,128) -> 43:43
// Full block 1:0 value32896 (offsetend2365152 bitend3) 42,43:137 (128:128) = 32896
smk_bw_write(&bw, 360, 9); // *5); // (128,128) -> 57:57
// Full block 1:1 value32896 (offsetend2365153 bitend0) 40,41:137 (128:128) = 32896
smk_bw_write(&bw, 1794, 11); // *19); // (236,128) -> 43:43
// Full block 2:0 value33004 (offsetend2365155 bitend3) 42,43:138 (236:128) = 33004
smk_bw_write(&bw, 360, 9); // *5); // (236,128) -> 57:57
// Full block 2:1 value33004 (offsetend2365156 bitend0) 40,41:138 (236:128) = 33004
smk_bw_write(&bw, 1794, 11); // *13); // (37,14) -> 43:43
// Full block 3:0 value3621 (offsetend2365157 bitend5) 42,43:139 (37:14) = 3621
smk_bw_write(&bw, 11367, 16); // *14); // (55,37) -> 44:57
// Full block 3:1 value9527 (offsetend2365159 bitend3) 40,41:139 (55:37) = 9527
smk_bw_write(&bw, 3284, 13); // *6); // (0,0) -> 18:10
// Full block 0:0 value0 (offsetend2365160 bitend1) 46,47:136 (0:0) = 0
smk_bw_write(&bw, 8202, 14); // *11); // (14,11) -> 30:18
// Full block 0:1 value2830 (offsetend2365161 bitend4) 44,45:136 (14:11) = 2830
smk_bw_write(&bw, 8710, 14); // *11); // (236,236) -> 18:11
// Full block 1:0 value60652 (offsetend2365162 bitend7) 46,47:137 (236:236) = 60652
smk_bw_write(&bw, 8202, 14); // *19); // (236,128) -> 30:18
// Full block 1:1 value33004 (offsetend2365165 bitend2) 44,45:137 (236:128) = 33004
smk_bw_write(&bw, 8710, 14); // *19); // (89,236) -> 18:11
// Full block 2:0 value60505 (offsetend2365167 bitend5) 46,47:138 (89:236) = 60505
smk_bw_write(&bw, 6210, 13); // *21); // (128,236) -> 43:18
// Full block 2:1 value60544 (offsetend2365170 bitend2) 44,45:138 (128:236) = 60544
smk_bw_write(&bw, 678, 10); // *16); // (223,0) -> 14:0
// Full block 3:0 value223 (offsetend2365172 bitend2) 46,47:139 (223:0) = 223
smk_bw_write(&bw, 2728, 12); // *8); // (10,10) -> 37:30
// Full block 3:1 value2570 (offsetend2365173 bitend2) 44,45:139 (10:10) = 2570
smk_bw_write(&bw, 59, 6); // (0,0)
// Full block 0:0 value0 (offsetend2365174 bitend0) 50,51:136 (0:0) = 0
smk_bw_write(&bw, 59, 6); // *5); // (0,0)
// Full block 0:1 value0 (offsetend2365174 bitend5) 48,49:136 (0:0) = 0
smk_bw_write(&bw, 24360, 15); // *20); // (236,89) -> 0:222
// Full block 1:0 value23020 (offsetend2365177 bitend1) 50,51:137 (236:89) = 23020
smk_bw_write(&bw, 196786, 18); // *19); // (89,236) -> 10:222
// Full block 1:1 value60505 (offsetend2365179 bitend4) 48,49:137 (89:236) = 60505
smk_bw_write(&bw, 24360, 15); // *11); // (236,236) -> 0:222
// Full block 2:0 value60652 (offsetend2365180 bitend7) 50,51:138 (236:236) = 60652
smk_bw_write(&bw, 196786, 18); // *19); // (236,82) -> 10:222
// Full block 2:1 value21228 (offsetend2365183 bitend2) 48,49:138 (236:82) = 21228
smk_bw_write(&bw, 24360, 15); // *15); // (0,222)
// Full block 3:0 value56832 (offsetend2365185 bitend1) 50,51:139 (0:222) = 56832
smk_bw_write(&bw, 24360, 15); // *14); // (222,223) -> 0:222
// Full block 3:1 value57310 (offsetend2365186 bitend7) 48,49:139 (222:223) = 57310
smk_bw_write(&bw, 59, 6); // (0,0)
// Full block 0:0 value0 (offsetend2365187 bitend5) 54,55:136 (0:0) = 0
smk_bw_write(&bw, 59, 6); // *5); // (0,0)
// Full block 0:1 value0 (offsetend2365188 bitend2) 52,53:136 (0:0) = 0
smk_bw_write(&bw, 24360, 15); // *19); // (89,236) -> 0:222
// Full block 1:0 value60505 (offsetend2365190 bitend5) 54,55:137 (89:236) = 60505
smk_bw_write(&bw, 24360, 15); // *11); // (236,236) -> 0:222
// Full block 1:1 value60652 (offsetend2365192 bitend0) 52,53:137 (236:236) = 60652
smk_bw_write(&bw, 24360, 15); // *19); // (236,82) -> 0:222
// Full block 2:0 value21228 (offsetend2365194 bitend3) 54,55:138 (236:82) = 21228
smk_bw_write(&bw, 24360, 15); // *20); // (82,236) -> 0:222
// Full block 2:1 value60498 (offsetend2365196 bitend7) 52,53:138 (82:236) = 60498
smk_bw_write(&bw, 24360, 15); // *14); // (222,223) -> 0:222
// Full block 3:0 value57310 (offsetend2365198 bitend5) 54,55:139 (222:223) = 57310
smk_bw_write(&bw, 24360, 15); // *16); // (223,0) -> 0:222
// Full block 3:1 value223 (offsetend2365200 bitend5) 52,53:139 (223:0) = 223
smk_bw_write(&bw, 59, 6); // (0,0)
// Full block 0:0 value0 (offsetend2365201 bitend3) 58,59:136 (0:0) = 0
smk_bw_write(&bw, 59, 6); // *5); // (0,0)
// Full block 0:1 value0 (offsetend2365202 bitend0) 56,57:136 (0:0) = 0
smk_bw_write(&bw, 24360, 15); // *11); // (236,236) -> 0:222
// Full block 1:0 value60652 (offsetend2365203 bitend3) 58,59:137 (236:236) = 60652
smk_bw_write(&bw, 24360, 15); // *20); // (236,89) -> 0:222
// Full block 1:1 value23020 (offsetend2365205 bitend7) 56,57:137 (236:89) = 23020
smk_bw_write(&bw, 24360, 15); // *20); // (82,236) -> 0:222
// Full block 2:0 value60498 (offsetend2365208 bitend3) 58,59:138 (82:236) = 60498
smk_bw_write(&bw, 59, 6); // (236,236) -> 0:0
// Full block 2:1 value60652 (offsetend2365209 bitend1) 56,57:138 (236:236) = 60652
smk_bw_write(&bw, 24360, 15); // *16); // (223,0) -> 0:222
// Full block 3:0 value223 (offsetend2365211 bitend1) 58,59:139 (223:0) = 223
smk_bw_write(&bw, 24360, 15); // (0,222)
// Full block 3:1 value56832 (offsetend2365213 bitend0) 56,57:139 (0:222) = 56832
smk_bw_write(&bw, 59, 6); // (0,0)
// Full block 0:0 value0 (offsetend2365213 bitend6) 62,63:136 (0:0) = 0
smk_bw_write(&bw, 59, 6); // *5); // (0,0)
// Full block 0:1 value0 (offsetend2365214 bitend3) 60,61:136 (0:0) = 0
smk_bw_write(&bw, 24360, 15); // (236,89) -> 0:222
// Full block 1:0 value23020 (offsetend2365216 bitend7) 62,63:137 (236:89) = 23020
smk_bw_write(&bw, 24360, 15); // *19); // (89,236) -> 0:222
// Full block 1:1 value60505 (offsetend2365219 bitend2) 60,61:137 (89:236) = 60505
smk_bw_write(&bw, 24360, 15); // *11); // (236,236) -> 0:222
// Full block 2:0 value60652 (offsetend2365220 bitend5) 62,63:138 (236:236) = 60652
smk_bw_write(&bw, 24360, 15); // *19); // (236,82) -> 0:222
// Full block 2:1 value21228 (offsetend2365223 bitend0) 60,61:138 (236:82) = 21228
smk_bw_write(&bw, 24360, 15); // *15); // (0,222)
// Full block 3:0 value56832 (offsetend2365224 bitend7) 62,63:139 (0:222) = 56832
smk_bw_write(&bw, 24360, 15); // (222,223) -> 0:222
// Full block 3:1 value57310 (offsetend2365226 bitend5) 60,61:139 (222:223) = 57310
smk_bw_write(&bw, 59, 6); // (0,0)
// Full block 0:0 value0 (offsetend2365227 bitend3) 66,67:136 (0:0) = 0
smk_bw_write(&bw, 59, 6); // *5); // (0,0)
// Full block 0:1 value0 (offsetend2365228 bitend0) 64,65:136 (0:0) = 0
smk_bw_write(&bw, 24360, 15); // *19); // (89,236) -> 0:222
// Full block 1:0 value60505 (offsetend2365230 bitend3) 66,67:137 (89:236) = 60505
smk_bw_write(&bw, 24360, 15); // *11); // (236,236) -> 0:222
// Full block 1:1 value60652 (offsetend2365231 bitend6) 64,65:137 (236:236) = 60652
smk_bw_write(&bw, 24360, 15); // *19); // (236,82) -> 0:222
// Full block 2:0 value21228 (offsetend2365234 bitend1) 66,67:138 (236:82) = 21228
smk_bw_write(&bw, 24360, 15); // *20); // (82,236) -> 0:222
// Full block 2:1 value60498 (offsetend2365236 bitend5) 64,65:138 (82:236) = 60498
smk_bw_write(&bw, 24360, 15); // *14); // (222,223) -> 0:222
// Full block 3:0 value57310 (offsetend2365238 bitend3) 66,67:139 (222:223) = 57310
smk_bw_write(&bw, 24360, 15); // *16); // (223,0) -> 0:222
// Full block 3:1 value223 (offsetend2365240 bitend3) 64,65:139 (223:0) = 223
smk_bw_write(&bw, 138, 8); // (0,10)
// Full block 0:0 value2560 (offsetend2365241 bitend3) 70,71:136 (0:10) = 2560
smk_bw_write(&bw, 59, 6); // (0,0)
// Full block 0:1 value0 (offsetend2365242 bitend1) 68,69:136 (0:0) = 0
smk_bw_write(&bw, 48822, 19); // (236,128) -> 222:25
// Full block 1:0 value33004 (offsetend2365244 bitend4) 70,71:137 (236:128) = 33004
smk_bw_write(&bw, 24360, 15); // *20); // (236,89) -> 0:222
// Full block 1:1 value23020 (offsetend2365247 bitend0) 68,69:137 (236:89) = 23020
smk_bw_write(&bw, 211211, 21); // (128,236) -> 222:30
// Full block 2:0 value60544 (offsetend2365249 bitend5) 70,71:138 (128:236) = 60544
smk_bw_write(&bw, 24360, 15); // *11); // (236,236) -> 0:222
// Full block 2:1 value60652 (offsetend2365251 bitend0) 68,69:138 (236:236) = 60652
smk_bw_write(&bw, 606, 11); // *14); // (11,18) -> 30:25
// Full block 3:0 value4619 (offsetend2365252 bitend6) 70,71:139 (11:18) = 4619
smk_bw_write(&bw, 138, 8); // (0,10)
// Full block 3:1 value2560 (offsetend2365253 bitend6) 68,69:139 (0:10) = 2560
smk_bw_write(&bw, 1949, 11); // *9); // (18,18) -> 30:30
// Full block 0:0 value4626 (offsetend2365254 bitend7) 74,75:136 (18:18) = 4626
smk_bw_write(&bw, 338, 10); // (14,14) -> 25:25
// Full block 0:1 value3598 (offsetend2365256 bitend1) 72,73:136 (14:14) = 3598
smk_bw_write(&bw, 1949, 11); // *19); // (236,128) -> 30:30
// Full block 1:0 value33004 (offsetend2365258 bitend4) 74,75:137 (236:128) = 33004
smk_bw_write(&bw, 1949, 11); // *5); // (236,128) -> 30:30
// Full block 1:1 value33004 (offsetend2365259 bitend1) 72,73:137 (236:128) = 33004
smk_bw_write(&bw, 1949, 11); // *5); // (236,128) -> 30:30
// Full block 2:0 value33004 (offsetend2365259 bitend6) 74,75:138 (236:128) = 33004
smk_bw_write(&bw, 338, 10); // *11); // (128,128) -> 25:25
// Full block 2:1 value32896 (offsetend2365261 bitend1) 72,73:138 (128:128) = 32896
smk_bw_write(&bw, 7975, 13); // (25,18) -> 37:43
// Full block 3:0 value4633 (offsetend2365262 bitend6) 74,75:139 (25:18) = 4633
smk_bw_write(&bw, 16484, 15); // *10); // (16,21) -> 25:43
// Full block 3:1 value5392 (offsetend2365264 bitend0) 72,73:139 (16:21) = 5392
smk_bw_write(&bw, 338, 10); // (14,14) -> 25:25
// Full block 0:0 value3598 (offsetend2365265 bitend2) 78,79:136 (14:14) = 3598
smk_bw_write(&bw, 36, 9); // (18,18) 
// Full block 0:1 value4626 (offsetend2365266 bitend3) 76,77:136 (18:18) = 4626
smk_bw_write(&bw, 338, 10); // *19); // (236,128) -> 25:25
// Full block 1:0 value33004 (offsetend2365268 bitend6) 78,79:137 (236:128) = 33004
smk_bw_write(&bw, 338, 10); // *5); // (236,128) -> 25:25
// Full block 1:1 value33004 (offsetend2365269 bitend3) 76,77:137 (236:128) = 33004
smk_bw_write(&bw, 338, 10); // *5); // (236,128) -> 25:25
// Full block 2:0 value33004 (offsetend2365270 bitend0) 78,79:138 (236:128) = 33004
smk_bw_write(&bw, 1949, 11); // (128,128) -> 30:30
// Full block 2:1 value32896 (offsetend2365271 bitend3) 76,77:138 (128:128) = 32896
smk_bw_write(&bw, 1949, 11); // *12); // (25,14) -> 30:30
// Full block 3:0 value3609 (offsetend2365272 bitend7) 78,79:139 (25:14) = 3609
smk_bw_write(&bw, 1794, 11); // *9); // (16,16) -> 43:43
// Full block 3:1 value4112 (offsetend2365274 bitend0) 76,77:139 (16:16) = 4112
smk_bw_write(&bw, 338, 10); // *8); // (10,0) -> 25:25
// Full block 0:0 value10 (offsetend2365275 bitend0) 82,83:136 (10:0) = 10
smk_bw_write(&bw, 338, 10); // (14,14) -> 25:25
// Full block 0:1 value3598 (offsetend2365276 bitend2) 80,81:136 (14:14) = 3598
smk_bw_write(&bw, 1033142, 20); // *11); // (236,236) -> 12:222
// Full block 1:0 value60652 (offsetend2365277 bitend5) 82,83:137 (236:236) = 60652
smk_bw_write(&bw, 606, 11); // *19); // (236,128) -> 30:25
// Full block 1:1 value33004 (offsetend2365280 bitend0) 80,81:137 (236:128) = 33004
smk_bw_write(&bw, 1033142, 20); // *19); // (236,82) -> 27:11
// Full block 2:0 value21228 (offsetend2365282 bitend3) 82,83:138 (236:82) = 21228
smk_bw_write(&bw, 606, 11); // *16); // (128,89) -> 30:25
// Full block 2:1 value22912 (offsetend2365284 bitend3) 80,81:138 (128:89) = 22912
smk_bw_write(&bw, 59, 6); // *14); // (222,223) -> 0:0
// Full block 3:0 value57310 (offsetend2365286 bitend1) 82,83:139 (222:223) = 57310
smk_bw_write(&bw, 1963, 14); // *15); // (19,231) -> 31:25
// Full block 3:1 value59155 (offsetend2365288 bitend0) 80,81:139 (19:231) = 59155
smk_bw_write(&bw, 3068, 13); // *6); // (0,0) -> 14:18
// Full block 0:0 value0 (offsetend2365288 bitend6) 86,87:136 (0:0) = 0
smk_bw_write(&bw, 91, 14); // *5); // -> 222:0
// Full block 0:1 value0 (offsetend2365289 bitend3) 84,85:136 (0:0) = 0
smk_bw_write(&bw, 1029619, 20); // (236,89) -> 222:11
// Full block 1:0 value23020 (offsetend2365291 bitend7) 86,87:137 (236:89) = 23020
smk_bw_write(&bw, 91, 14); // *19); // (89,236) -> 222:0
// Full block 1:1 value60505 (offsetend2365294 bitend2) 84,85:137 (89:236) = 60505
smk_bw_write(&bw, 24360, 15); // *11); // (236,236) -> 0:222
// Full block 2:0 value60652 (offsetend2365295 bitend5) 86,87:138 (236:236) = 60652
smk_bw_write(&bw, 91, 14); // *19); // (236,82) -> 222:0
// Full block 2:1 value21228 (offsetend2365298 bitend0) 84,85:138 (236:82) = 21228
smk_bw_write(&bw, 24360, 15); // (0,222)
// Full block 3:0 value56832 (offsetend2365299 bitend7) 86,87:139 (0:222) = 56832
smk_bw_write(&bw, 59, 6); // *14); // (222,223) -> 0:0
// Full block 3:1 value57310 (offsetend2365301 bitend5) 84,85:139 (222:223) = 57310
smk_bw_write(&bw, 678, 10); // *6); // (0,0) -> 14:0
// Full block 0:0 value0 (offsetend2365302 bitend3) 90,91:136 (0:0) = 0
smk_bw_write(&bw, 2980, 13); // *5*); -> 18:14
// Full block 0:1 value0 (offsetend2365303 bitend0) 88,89:136 (0:0) = 0
smk_bw_write(&bw, 24360, 15); // *19); // (89,236) -> 0:222
// Full block 1:0 value60505 (offsetend2365305 bitend3) 90,91:137 (89:236) = 60505
smk_bw_write(&bw, 24360, 15); // *11); // (236,236) -> 0:222
// Full block 1:1 value60652 (offsetend2365306 bitend6) 88,89:137 (236:236) = 60652
smk_bw_write(&bw, 24360, 15); // *19); // (236,82) -> 0:222
// Full block 2:0 value21228 (offsetend2365309 bitend1) 90,91:138 (236:82) = 21228
smk_bw_write(&bw, 24360, 15); // *20); // (82,236) -> 0:222
// Full block 2:1 value60498 (offsetend2365311 bitend5) 88,89:138 (82:236) = 60498
smk_bw_write(&bw, 24360, 15); // *14); // (222,223) -> 0:222
// Full block 3:0 value57310 (offsetend2365313 bitend3) 90,91:139 (222:223) = 57310
smk_bw_write(&bw, 24360, 15); // *16); // (223,0) -> 0:222
// Full block 3:1 value223 (offsetend2365315 bitend3) 88,89:139 (223:0) = 223
smk_bw_write(&bw, 75514, 17); // (185,192) -> 185:199
// Full block 0:0 value49337 (offsetend2365317 bitend4) 94,95:136 (185:192) = 49337
smk_bw_write(&bw, 6764, 16); // (0,99)
// Full block 0:1 value25344 (offsetend2365319 bitend4) 92,93:136 (0:99) = 25344
smk_bw_write(&bw, 459268, 19); // (239,203)
// Full block 1:0 value52207 (offsetend2365321 bitend7) 94,95:137 (239:203) = 52207
smk_bw_write(&bw, 1029619, 20); // (236,89)
// Full block 1:1 value23020 (offsetend2365324 bitend3) 92,93:137 (236:89) = 23020
smk_bw_write(&bw, 191200, 20); // (128,182)
// Full block 2:0 value46720 (offsetend2365326 bitend7) 94,95:138 (128:182) = 46720
smk_bw_write(&bw, 4782, 14); // *11); // (236,236) -> 222:225
// Full block 2:1 value60652 (offsetend2365328 bitend2) 92,93:138 (236:236) = 60652
smk_bw_write(&bw, 6764, 16); // *21); // (222,99) -> 0:99
// Full block 3:0 value25566 (offsetend2365330 bitend7) 94,95:139 (222:99) = 25566
smk_bw_write(&bw, 4130, 13); // *8); // (0,10) -> 0:30
// Full block 3:1 value2560 (offsetend2365331 bitend7) 92,93:139 (0:10) = 2560
smk_bw_write(&bw, 886, 14); // (138,144) 
// Full block 0:0 value37002 (offsetend2365333 bitend5) 98,99:136 (138:144) = 37002
smk_bw_write(&bw, 308914, 19); // (192,177) 
// Full block 0:1 value45504 (offsetend2365336 bitend0) 96,97:136 (192:177) = 45504
smk_bw_write(&bw, 51, 11); // (160,160)
// Full block 1:0 value41120 (offsetend2365337 bitend3) 98,99:137 (160:160) = 41120
smk_bw_write(&bw, 93490, 17); // (194,174)
// Full block 1:1 value44738 (offsetend2365339 bitend4) 96,97:137 (194:174) = 44738
smk_bw_write(&bw, 2753, 13); // (174,182) 
// Full block 2:0 value46766 (offsetend2365341 bitend1) 98,99:138 (174:182) = 46766
smk_bw_write(&bw, 69387, 19); // (203,181)
// Full block 2:1 value46539 (offsetend2365343 bitend4) 96,97:138 (203:181) = 46539
smk_bw_write(&bw, 972019, 20); // (181,238)
// Full block 3:0 value61109 (offsetend2365346 bitend0) 98,99:139 (181:238) = 61109
smk_bw_write(&bw, 91715, 17); // (181,199)
// Full block 3:1 value51125 (offsetend2365348 bitend1) 96,97:139 (181:199) = 51125
smk_bw_write(&bw, 726, 12); // (238,238)
// Full block 0:0 value61166 (offsetend2365349 bitend5) 102,103:136 (238:238) = 61166
smk_bw_write(&bw, 404832, 19); // (185,239)
// Full block 0:1 value61369 (offsetend2365352 bitend0) 100,101:136 (185:239) = 61369
smk_bw_write(&bw, 726, 12); // (239,239) -> 238:238
// Full block 1:0 value61423 (offsetend2365353 bitend4) 102,103:137 (239:239) = 61423
smk_bw_write(&bw, 394422, 19); // (182,241) 
// Full block 1:1 value61878 (offsetend2365355 bitend7) 100,101:137 (182:241) = 61878
smk_bw_write(&bw, 124573, 18); // *15); // (239,240) -> 238:148
// Full block 2:0 value61679 (offsetend2365357 bitend6) 102,103:138 (239:240) = 61679
smk_bw_write(&bw, 726, 12); // *15); // (241,239) -> 238:238
// Full block 2:1 value61425 (offsetend2365359 bitend5) 100,101:138 (241:239) = 61425
smk_bw_write(&bw, 1731, 12); // (239,239) 
// Full block 3:0 value61423 (offsetend2365361 bitend1) 102,103:139 (239:239) = 61423
smk_bw_write(&bw, 1731, 12); // *5); // (239,239) 
// Full block 3:1 value61423 (offsetend2365361 bitend6) 100,101:139 (239:239) = 61423
smk_bw_write(&bw, 161395, 18); // (128,115) 
// Full block 0:0 value29568 (offsetend2365364 bitend0) 106,107:136 (128:115) = 29568
smk_bw_write(&bw, 4876, 15); // (237,239) 
// Full block 0:1 value61421 (offsetend2365365 bitend7) 104,105:136 (237:239) = 61421
smk_bw_write(&bw, 329715, 20); // (238,142) 
// Full block 1:0 value36590 (offsetend2365368 bitend3) 106,107:137 (238:142) = 36590
smk_bw_write(&bw, 726, 12); // (239,239) -> 238:238
// Full block 1:1 value61423 (offsetend2365369 bitend7) 104,105:137 (239:239) = 61423
smk_bw_write(&bw, 39508, 18); // *20); // (241,237) -> 169:157
// Full block 2:0 value60913 (offsetend2365372 bitend3) 106,107:138 (241:237) = 60913
smk_bw_write(&bw, 65737, 17); // *15); // (239,241) -> 238:169
// Full block 2:1 value61935 (offsetend2365374 bitend2) 104,105:138 (239:241) = 61935
smk_bw_write(&bw, 229042, 19); // (185,148) 
// Full block 3:0 value38073 (offsetend2365376 bitend5) 106,107:139 (185:148) = 38073
smk_bw_write(&bw, 4109, 15); // (238,239) 
// Full block 3:1 value61422 (offsetend2365378 bitend4) 104,105:139 (238:239) = 61422
smk_bw_write(&bw, 218635, 19); // (169,21) 
// Full block 0:0 value5545 (offsetend2365380 bitend7) 110,111:136 (169:21) = 5545
smk_bw_write(&bw, 45588, 16); // (108,142) 
// Full block 0:1 value36460 (offsetend2365382 bitend7) 108,109:136 (108:142) = 36460
smk_bw_write(&bw, 680672, 20); // (169,236) 
// Full block 1:0 value60585 (offsetend2365385 bitend3) 110,111:137 (169:236) = 60585
smk_bw_write(&bw, 41448, 17); // (142,181) 
// Full block 1:1 value46478 (offsetend2365387 bitend4) 108,109:137 (142:181) = 46478
smk_bw_write(&bw, 56808, 19); // (236,82) 
// Full block 2:0 value21228 (offsetend2365389 bitend7) 110,111:138 (236:82) = 21228
smk_bw_write(&bw, 349110, 20); // (161,194) 
// Full block 2:1 value49825 (offsetend2365392 bitend3) 108,109:138 (161:194) = 49825
smk_bw_write(&bw, 91, 14); // (222,223) -> 222:0
// Full block 3:0 value57310 (offsetend2365394 bitend1) 110,111:139 (222:223) = 57310
smk_bw_write(&bw, 1164555, 21); // (181,148) 
// Full block 3:1 value38069 (offsetend2365396 bitend6) 108,109:139 (181:148) = 38069
smk_bw_write(&bw, 59, 6); // (0,0)
// Full block 0:0 value0 (offsetend2365397 bitend4) 114,115:136 (0:0) = 0
smk_bw_write(&bw, 59, 6); // *5); 
// Full block 0:1 value0 (offsetend2365398 bitend1) 112,113:136 (0:0) = 0
smk_bw_write(&bw, 196786, 18); // (236,74) -> 10:222
// Full block 1:0 value19180 (offsetend2365400 bitend3) 114,115:137 (236:74) = 19180
smk_bw_write(&bw, 81772, 19); // (89,236) -> 228:10
// Full block 1:1 value60505 (offsetend2365402 bitend6) 112,113:137 (89:236) = 60505
smk_bw_write(&bw, 196786, 18); // *4); // (236,74) -> 10:222
// Full block 2:0 value19180 (offsetend2365403 bitend2) 114,115:138 (236:74) = 19180
smk_bw_write(&bw, 81772, 19); // (236,82) -> 228:10
// Full block 2:1 value21228 (offsetend2365405 bitend5) 112,113:138 (236:82) = 21228
smk_bw_write(&bw, 91, 14); // (222,223) -> 222:0
// Full block 3:0 value57310 (offsetend2365407 bitend3) 114,115:139 (222:223) = 57310
smk_bw_write(&bw, 91, 14); // *5); // (222,223) -> 222:0
// Full block 3:1 value57310 (offsetend2365408 bitend0) 112,113:139 (222:223) = 57310

	LogErrorFF("Patchlen %d", (size_t)bw.buffer - (size_t)bufMem - 2365112);
	uint32_t *lm = (uint32_t*)&bufMem[2365112];
	for (int i = 0; i < ((size_t)bw.buffer + (bw.bit_num != 0 ? 1 : 0) - (size_t)bufMem - 2365112 + 3) / 4; i++) {
		LogErrorFF("	lm[%d] = SwapLE32(%d);", i, lm[i]);
	}
*/
uint32_t *cur = (uint32_t*)&bufMem[2365112];
	cur[0] = -550965321;
	cur[1] = -272898787;
	cur[2] = 2091106043;
	cur[3] = 1596505681;
	cur[4] = 2009739156;
	cur[5] = -202827765;
	cur[6] = 2104344764;
	cur[7] = -2141622575;
	cur[8] = 48038255;
	cur[9] = 45358492;
	cur[10] = -1267717305;
	cur[11] = -1587331326;
	cur[12] = -2120218010;
	cur[13] = -2011561816;
	cur[14] = 1414846530;
	cur[15] = -1349224483;
	cur[16] = 1596506156;
	cur[17] = -1101954983;
	cur[18] = 1199431464;
	cur[19] = 1367122681;
	cur[20] = -1805702978;
	cur[21] = -545797585;
	cur[22] = 1596505681;
	cur[23] = -225513580;
	cur[24] = 1005517125;
	cur[25] = -1947920438;
	cur[26] = -1560721934;
	cur[27] = -71413380;
	cur[28] = -112856434;
	cur[29] = -1101955934;
	cur[30] = -1349230808;
	cur[31] = 199978722;
	cur[32] = -834474092;
	cur[33] = 1232795968;
	cur[34] = 1968370609;
	cur[35] = -1751256130;
	cur[36] = 26469610;
	cur[37] = -1792990555;
	cur[38] = -1099607468;
	cur[39] = 1251410675;
	cur[40] = -136193238;
	cur[41] = 2079387026;
	cur[42] = -130195607;
	cur[43] = 1596987095;
	cur[44] = -905946117;
	cur[45] = -113243273;
	cur[46] = 781362926;
	cur[47] = 399126420;
	cur[48] = 1173523429;
	cur[49] = -176381191;
	cur[50] = 275362381;
	cur[51] = -155288568;
	cur[52] = 1439028673;
	cur[53] = -2129603742;
	cur[54] = 766280566;
	cur[55] = 919806055;
	cur[56] = 1681665368;
	cur[57] = -863511906;
	cur[58] = -1957164198;
	cur[59] = -1063860819;
	cur[60] = -883300707;
	cur[61] = -1663365712;
	cur[62] = -107803491;
	cur[63] = 706130563;
	cur[64] = -905866675;
	cur[65] = -1308512774;
	cur[66] = 1493841248;
	cur[67] = 256192880;
	cur[68] = -619848059;
	cur[69] = 369285801;
	cur[70] = -876682358;
	cur[71] = 334941186;
	cur[72] = 2128183385;
	cur[73] = 23856562;
} else if (patchCounter == 1) {
	patchCounter++;

// Vic2
	bw.buffer += 2303427;
	bw.bit_num = 1;
/*
smk_bw_skip(&bw, 6); // mem (2303427:7) value (0,0)
// Full block 0:0 value0 (offsetend2303427 bitend7) 30,31:136 (0:0) = 0
smk_bw_write(&bw, 43, 6); // *4); // mem (2303428:3) value (0,0)
// Full block 0:1 value0 (offsetend2303428 bitend3) 28,29:136 (0:0) = 0
smk_bw_write(&bw, 3228, 15); // *20); // mem (2303428:7) value (22,99) -> 0:222
// Full block 1:0 value25366 (offsetend2303430 bitend7) 30,31:137 (22:99) = 25366
smk_bw_write(&bw, 43, 6); // *4); // mem (2303431:3) value (0,0)
// Full block 1:1 value0 (offsetend2303431 bitend3) 28,29:137 (0:0) = 0
smk_bw_write(&bw, 3228, 15); // *18); // mem (2303431:5) value (23,236) -> 0:222
// Full block 2:0 value60439 (offsetend2303433 bitend5) 30,31:138 (23:236) = 60439
smk_bw_write(&bw, 43, 6); // *4); // mem (2303434:1) value (0,0)
// Full block 2:1 value0 (offsetend2303434 bitend1) 28,29:138 (0:0) = 0
smk_bw_write(&bw, 43, 6); // *4); // mem (2303434:5) value (0,0)
// Full block 3:0 value0 (offsetend2303434 bitend5) 30,31:139 (0:0) = 0
smk_bw_write(&bw, 43, 6); // *4); // mem (2303435:1) value (0,0)
// Full block 3:1 value0 (offsetend2303435 bitend1) 28,29:139 (0:0) = 0
smk_bw_write(&bw, 7, 4); // *4); // mem (2303435:5) value (0,0)
// Full block 0:0 value0 (offsetend2303435 bitend5) 34,35:136 (0:0) = 0
smk_bw_write(&bw, 7, 4); // *4); // mem (2303436:1) value (0,0)
// Full block 0:1 value0 (offsetend2303436 bitend1) 32,33:136 (0:0) = 0
smk_bw_write(&bw, 3228, 15); // *19); // mem (2303436:4) value (89,236) -> 0:222
// Full block 1:0 value60505 (offsetend2303438 bitend4) 34,35:137 (89:236) = 60505
smk_bw_write(&bw, 3228, 15); // *11); // mem (2303438:7) value (236,236) -> 0:222
// Full block 1:1 value60652 (offsetend2303439 bitend7) 32,33:137 (236:236) = 60652
smk_bw_write(&bw, 3228, 15); // *19); // mem (2303440:2) value (236,82) -> 0:222
// Full block 2:0 value21228 (offsetend2303442 bitend2) 34,35:138 (236:82) = 21228
smk_bw_write(&bw, 3228, 15); // *20); // mem (2303442:6) value (82,236) -> 0:222
// Full block 2:1 value60498 (offsetend2303444 bitend6) 32,33:138 (82:236) = 60498
smk_bw_write(&bw, 3228, 15); // *14); // mem (2303445:4) value (222,223) -> 0:222
// Full block 3:0 value57310 (offsetend2303446 bitend4) 34,35:139 (222:223) = 57310
smk_bw_write(&bw, 13757, 14); // mem (2303447:2) value (222,0)
// Full block 3:1 value222 (offsetend2303448 bitend2) 32,33:139 (222:0) = 222
smk_bw_write(&bw, 16054, 14); // *13); // mem (2303448:7) value (49,43) -> 50:57
// Full block 0:0 value11057 (offsetend2303449 bitend7) 38,39:136 (49:43) = 11057
smk_bw_write(&bw, 2403, 13); // mem (2303450:4) value (0,37)
// Full block 0:1 value9472 (offsetend2303451 bitend4) 36,37:136 (0:37) = 9472
smk_bw_write(&bw, 220, 11); // mem (2303451:7) value (128,128) -> 43:43
// Full block 1:0 value32896 (offsetend2303452 bitend7) 38,39:137 (128:128) = 32896
smk_bw_write(&bw, 399396, 19); // mem (2303453:2) value (236,128) -> 222:25
// Full block 1:1 value33004 (offsetend2303455 bitend2) 36,37:137 (236:128) = 33004
smk_bw_write(&bw, 3545, 13); // *4); // mem (2303455:6) value (236,128) -> 25:30
// Full block 2:0 value33004 (offsetend2303455 bitend6) 38,39:138 (236:128) = 33004
smk_bw_write(&bw, 76906, 19); // mem (2303456:1) value (236,74) -> 10:222
// Full block 2:1 value19180 (offsetend2303458 bitend1) 36,37:138 (236:74) = 19180
smk_bw_write(&bw, 55289, 18); // mem (2303458:3) value (222,37)
// Full block 3:0 value9694 (offsetend2303460 bitend3) 38,39:139 (222:37) = 9694
smk_bw_write(&bw, 13757, 14); // mem (2303461:1) value (222,223) -> 222:0
// Full block 3:1 value57310 (offsetend2303462 bitend1) 36,37:139 (222:223) = 57310
smk_bw_write(&bw, 11642, 14); // *13); // mem (2303462:6) value (30,25) -> 57:50
// Full block 0:0 value6430 (offsetend2303463 bitend6) 42,43:136 (30:25) = 6430
smk_bw_write(&bw, 244, 9); // *11); // mem (2303464:1) value (37,37) -> 57:57
// Full block 0:1 value9509 (offsetend2303465 bitend1) 40,41:136 (37:37) = 9509
smk_bw_write(&bw, 220, 11); // mem (2303465:4) value (128,128) -> 43:43
// Full block 1:0 value32896 (offsetend2303466 bitend4) 42,43:137 (128:128) = 32896
smk_bw_write(&bw, 244, 9); // *4); // mem (2303467:0) value (128,128) -> 57:57
// Full block 1:1 value32896 (offsetend2303467 bitend0) 40,41:137 (128:128) = 32896
smk_bw_write(&bw, 220, 11); // *19); // mem (2303467:3) value (236,128) -> 43:43
// Full block 2:0 value33004 (offsetend2303469 bitend3) 42,43:138 (236:128) = 33004
smk_bw_write(&bw, 244, 9); // *4); // mem (2303469:7) value (236,128) -> 57:57
// Full block 2:1 value33004 (offsetend2303469 bitend7) 40,41:138 (236:128) = 33004
smk_bw_write(&bw, 220, 11); // *13); // mem (2303470:4) value (37,14) -> 43:43
// Full block 3:0 value3621 (offsetend2303471 bitend4) 42,43:139 (37:14) = 3621
smk_bw_write(&bw, 105456, 17); // *14); // mem (2303472:2) value (55,37) -> 44:57
// Full block 3:1 value9527 (offsetend2303473 bitend2) 40,41:139 (55:37) = 9527
smk_bw_write(&bw, 4960, 13); // *6); // mem (2303474:0) value (0,0) -> 18:10
// Full block 0:0 value0 (offsetend2303474 bitend0) 46,47:136 (0:0) = 0
smk_bw_write(&bw, 11078, 14); // *11); // mem (2303474:3) value (14,11) -> 30:18
// Full block 0:1 value2830 (offsetend2303475 bitend3) 44,45:136 (14:11) = 2830
smk_bw_write(&bw, 4299, 13); // *11); // mem (2303475:6) value (236,236) -> 18:11
// Full block 1:0 value60652 (offsetend2303476 bitend6) 46,47:137 (236:236) = 60652
smk_bw_write(&bw, 11078, 14); // *19); // mem (2303477:1) value (236,128) -> 30:18
// Full block 1:1 value33004 (offsetend2303479 bitend1) 44,45:137 (236:128) = 33004
smk_bw_write(&bw, 4299, 13); // *19); // mem (2303479:4) value (89,236) -> 18:11
// Full block 2:0 value60505 (offsetend2303481 bitend4) 46,47:138 (89:236) = 60505
smk_bw_write(&bw, 6130, 13); // *21); // mem (2303482:1) value (128,236) -> 43:18
// Full block 2:1 value60544 (offsetend2303484 bitend1) 44,45:138 (128:236) = 60544
smk_bw_write(&bw, 374, 10); // *16); // mem (2303484:1) value (223,0) -> 14:0
// Full block 3:0 value223 (offsetend2303486 bitend1) 46,47:139 (223:0) = 223
smk_bw_write(&bw, 2265, 12); // *8); // mem (2303486:1) value (10,10) -> 37:30
// Full block 3:1 value2570 (offsetend2303487 bitend1) 44,45:139 (10:10) = 2570
smk_bw_write(&bw, 43, 6); // mem (2303487:7) value (0,0)
// Full block 0:0 value0 (offsetend2303487 bitend7) 50,51:136 (0:0) = 0
smk_bw_write(&bw, 43, 6); // *4); // mem (2303488:3) value (0,0)
// Full block 0:1 value0 (offsetend2303488 bitend3) 48,49:136 (0:0) = 0
smk_bw_write(&bw, 3228, 15); // *20); // mem (2303488:7) value (236,89) -> 0:222
// Full block 1:0 value23020 (offsetend2303490 bitend7) 50,51:137 (236:89) = 23020
smk_bw_write(&bw, 76906, 19); // *19); // mem (2303491:2) value (89,236) -> 10:222
// Full block 1:1 value60505 (offsetend2303493 bitend2) 48,49:137 (89:236) = 60505
smk_bw_write(&bw, 3228, 15); // *11); // mem (2303493:5) value (236,236) -> 0:222
// Full block 2:0 value60652 (offsetend2303494 bitend5) 50,51:138 (236:236) = 60652
smk_bw_write(&bw, 76906, 19); // *19); // mem (2303495:0) value (236,82) -> 10:222
// Full block 2:1 value21228 (offsetend2303497 bitend0) 48,49:138 (236:82) = 21228
smk_bw_write(&bw, 3228, 15); // mem (2303497:7) value (0,222)
// Full block 3:0 value56832 (offsetend2303498 bitend7) 50,51:139 (0:222) = 56832
smk_bw_write(&bw, 3228, 15); // *14); // mem (2303499:5) value (222,223) -> 0:222
// Full block 3:1 value57310 (offsetend2303500 bitend5) 48,49:139 (222:223) = 57310
smk_bw_write(&bw, 43, 6); // mem (2303501:3) value (0,0)
// Full block 0:0 value0 (offsetend2303501 bitend3) 54,55:136 (0:0) = 0
smk_bw_write(&bw, 43, 6); // *4); // mem (2303501:7) value (0,0)
// Full block 0:1 value0 (offsetend2303501 bitend7) 52,53:136 (0:0) = 0
smk_bw_write(&bw, 3228, 15); // *19); // mem (2303502:2) value (89,236) -> 0:222
// Full block 1:0 value60505 (offsetend2303504 bitend2) 54,55:137 (89:236) = 60505
smk_bw_write(&bw, 3228, 15); // *11); // mem (2303504:5) value (236,236) -> 0:222
// Full block 1:1 value60652 (offsetend2303505 bitend5) 52,53:137 (236:236) = 60652
smk_bw_write(&bw, 3228, 15); // *19); // mem (2303506:0) value (236,82) -> 0:222
// Full block 2:0 value21228 (offsetend2303508 bitend0) 54,55:138 (236:82) = 21228
smk_bw_write(&bw, 3228, 15); // *20); // mem (2303508:4) value (82,236) -> 0:222
// Full block 2:1 value60498 (offsetend2303510 bitend4) 52,53:138 (82:236) = 60498
smk_bw_write(&bw, 3228, 15); // *14); // mem (2303511:2) value (222,223) -> 0:222
// Full block 3:0 value57310 (offsetend2303512 bitend2) 54,55:139 (222:223) = 57310
smk_bw_write(&bw, 3228, 15); // *16); // mem (2303512:2) value (223,0) -> 0:222
// Full block 3:1 value223 (offsetend2303514 bitend2) 52,53:139 (223:0) = 223
smk_bw_write(&bw, 43, 6); // mem (2303515:0) value (0,0)
// Full block 0:0 value0 (offsetend2303515 bitend0) 58,59:136 (0:0) = 0
smk_bw_write(&bw, 43, 6); // *4); // mem (2303515:4) value (0,0)
// Full block 0:1 value0 (offsetend2303515 bitend4) 56,57:136 (0:0) = 0
smk_bw_write(&bw, 3228, 15); // *11); // mem (2303515:7) value (236,236) -> 0:222
// Full block 1:0 value60652 (offsetend2303516 bitend7) 58,59:137 (236:236) = 60652
smk_bw_write(&bw, 3228, 15); // *20); // mem (2303517:3) value (236,89) -> 0:222
// Full block 1:1 value23020 (offsetend2303519 bitend3) 56,57:137 (236:89) = 23020
smk_bw_write(&bw, 13757, 14); // *20); // mem (2303519:7) value (82,236) -> 222:0
// Full block 2:0 value60498 (offsetend2303521 bitend7) 58,59:138 (82:236) = 60498
smk_bw_write(&bw, 43, 6); // mem (2303522:5) value (236,236) -> 0:0
// Full block 2:1 value60652 (offsetend2303522 bitend5) 56,57:138 (236:236) = 60652
smk_bw_write(&bw, 43, 6); // *16); // mem (2303522:5) value (223,0) -> 0:222
// Full block 3:0 value223 (offsetend2303524 bitend5) 58,59:139 (223:0) = 223
smk_bw_write(&bw, 3228, 15); // mem (2303525:4) value (0,222)
// Full block 3:1 value56832 (offsetend2303526 bitend4) 56,57:139 (0:222) = 56832
smk_bw_write(&bw, 43, 6); // mem (2303527:2) value (0,0)
// Full block 0:0 value0 (offsetend2303527 bitend2) 62,63:136 (0:0) = 0
smk_bw_write(&bw, 43, 6); // *4); // mem (2303527:6) value (0,0)
// Full block 0:1 value0 (offsetend2303527 bitend6) 60,61:136 (0:0) = 0
smk_bw_write(&bw, 3228, 15); // *20); // mem (2303528:2) value (236,89) -> 0:222
// Full block 1:0 value23020 (offsetend2303530 bitend2) 62,63:137 (236:89) = 23020
smk_bw_write(&bw, 3228, 15); // *19); // mem (2303530:5) value (89,236) -> 0:222
// Full block 1:1 value60505 (offsetend2303532 bitend5) 60,61:137 (89:236) = 60505
smk_bw_write(&bw, 3228, 15); // *11); // mem (2303533:0) value (236,236) -> 0:222
// Full block 2:0 value60652 (offsetend2303534 bitend0) 62,63:138 (236:236) = 60652
smk_bw_write(&bw, 3228, 15); // *19); // mem (2303534:3) value (236,82) -> 0:222
// Full block 2:1 value21228 (offsetend2303536 bitend3) 60,61:138 (236:82) = 21228
smk_bw_write(&bw, 3228, 15); // mem (2303537:2) value (0,222)
// Full block 3:0 value56832 (offsetend2303538 bitend2) 62,63:139 (0:222) = 56832
smk_bw_write(&bw, 3228, 15); // *14); // mem (2303539:0) value (222,223) -> 0:222
// Full block 3:1 value57310 (offsetend2303540 bitend0) 60,61:139 (222:223) = 57310
smk_bw_write(&bw, 43, 6); // mem (2303540:6) value (0,0)
// Full block 0:0 value0 (offsetend2303540 bitend6) 66,67:136 (0:0) = 0
smk_bw_write(&bw, 7, 4); // mem (2303541:2) value (0,0)
// Full block 0:1 value0 (offsetend2303541 bitend2) 64,65:136 (0:0) = 0
smk_bw_write(&bw, 3228, 15); // *19); // mem (2303541:5) value (89,236) -> 0:222
// Full block 1:0 value60505 (offsetend2303543 bitend5) 66,67:137 (89:236) = 60505
smk_bw_write(&bw, 3228, 15); // *11); // mem (2303544:0) value (236,236) -> 0:222
// Full block 1:1 value60652 (offsetend2303545 bitend0) 64,65:137 (236:236) = 60652
smk_bw_write(&bw, 3228, 15); // *19); // mem (2303545:3) value (236,82) -> 0:222
// Full block 2:0 value21228 (offsetend2303547 bitend3) 66,67:138 (236:82) = 21228
smk_bw_write(&bw, 3228, 15); // *20); // mem (2303547:7) value (82,236) -> 0:222
// Full block 2:1 value60498 (offsetend2303549 bitend7) 64,65:138 (82:236) = 60498
smk_bw_write(&bw, 3228, 15); // *14); // mem (2303550:5) value (222,223) -> 0:222
// Full block 3:0 value57310 (offsetend2303551 bitend5) 66,67:139 (222:223) = 57310
smk_bw_write(&bw, 3228, 15); // *16); // mem (2303551:5) value (223,0) -> 0:222
// Full block 3:1 value223 (offsetend2303553 bitend5) 64,65:139 (223:0) = 223
smk_bw_write(&bw, 252, 8); // mem (2303553:5) value (0,10)
// Full block 0:0 value2560 (offsetend2303554 bitend5) 70,71:136 (0:10) = 2560
smk_bw_write(&bw, 43, 6); // mem (2303555:3) value (0,0)
// Full block 0:1 value0 (offsetend2303555 bitend3) 68,69:136 (0:0) = 0
smk_bw_write(&bw, 399396, 19); // mem (2303555:6) value (236,128) -> 222:25
// Full block 1:0 value33004 (offsetend2303557 bitend6) 70,71:137 (236:128) = 33004
smk_bw_write(&bw, 3228, 15); // *20); // mem (2303558:2) value (236,89) -> 0:222
// Full block 1:1 value23020 (offsetend2303560 bitend2) 68,69:137 (236:89) = 23020
smk_bw_write(&bw, 2388205, 22); // *21); // mem (2303560:7) value (128,236) -> 222:30
// Full block 2:0 value60544 (offsetend2303562 bitend7) 70,71:138 (128:236) = 60544
smk_bw_write(&bw, 3228, 15); // *11); // mem (2303563:2) value (236,236) -> 0:222
// Full block 2:1 value60652 (offsetend2303564 bitend2) 68,69:138 (236:236) = 60652
smk_bw_write(&bw, 6200, 13); // *14); // mem (2303565:0) value (11,18) -> 30:25
// Full block 3:0 value4619 (offsetend2303566 bitend0) 70,71:139 (11:18) = 4619
smk_bw_write(&bw, 252, 8); // mem (2303566:0) value (0,10)
// Full block 3:1 value2560 (offsetend2303567 bitend0) 68,69:139 (0:10) = 2560
smk_bw_write(&bw, 1240, 12); // *8); // mem (2303567:0) value (18,18) -> 30:30
// Full block 0:0 value4626 (offsetend2303568 bitend0) 74,75:136 (18:18) = 4626
smk_bw_write(&bw, 248, 11); // *9); // mem (2303568:1) value (14,14) -> 25:25
// Full block 0:1 value3598 (offsetend2303569 bitend1) 72,73:136 (14:14) = 3598
smk_bw_write(&bw, 1240, 12); // *19); // mem (2303569:4) value (236,128) -> 30:30
// Full block 1:0 value33004 (offsetend2303571 bitend4) 74,75:137 (236:128) = 33004
smk_bw_write(&bw, 1240, 12); // *4); // mem (2303572:0) value (236,128) -> 30:30
// Full block 1:1 value33004 (offsetend2303572 bitend0) 72,73:137 (236:128) = 33004
smk_bw_write(&bw, 1240, 12); // *4); // mem (2303572:4) value (236,128) -> 30:30
// Full block 2:0 value33004 (offsetend2303572 bitend4) 74,75:138 (236:128) = 33004
smk_bw_write(&bw, 248, 11); // mem (2303572:7) value (128,128) -> 25:25
// Full block 2:1 value32896 (offsetend2303573 bitend7) 72,73:138 (128:128) = 32896
smk_bw_write(&bw, 6499, 13); // mem (2303574:4) value (25,18) -> 37:43
// Full block 3:0 value4633 (offsetend2303575 bitend4) 74,75:139 (25:18) = 4633
smk_bw_write(&bw, 1570, 15); // *10); // mem (2303575:6) value (16,21) -> 25:43
// Full block 3:1 value5392 (offsetend2303576 bitend6) 72,73:139 (16:21) = 5392
smk_bw_write(&bw, 248, 11); // *9); // mem (2303576:7) value (14,14) -> 25:25
// Full block 0:0 value3598 (offsetend2303577 bitend7) 78,79:136 (14:14) = 3598
smk_bw_write(&bw, 127, 8); // mem (2303577:7) value (18,18)
// Full block 0:1 value4626 (offsetend2303578 bitend7) 76,77:136 (18:18) = 4626
smk_bw_write(&bw, 248, 11); // *19); // mem (2303579:2) value (236,128) -> 25:25
// Full block 1:0 value33004 (offsetend2303581 bitend2) 78,79:137 (236:128) = 33004
smk_bw_write(&bw, 248, 11); // *4); // mem (2303581:6) value (236,128) -> 25:25
// Full block 1:1 value33004 (offsetend2303581 bitend6) 76,77:137 (236:128) = 33004
smk_bw_write(&bw, 7, 4); // mem (2303582:2) value (236,128)
// Full block 2:0 value33004 (offsetend2303582 bitend2) 78,79:138 (236:128) = 33004
smk_bw_write(&bw, 1240, 12); // *11); // mem (2303582:5) value (128,128) -> 30:30
// Full block 2:1 value32896 (offsetend2303583 bitend5) 76,77:138 (128:128) = 32896
smk_bw_write(&bw, 1240, 12); // mem (2303584:1) value (25,14) -> 30:30
// Full block 3:0 value3609 (offsetend2303585 bitend1) 78,79:139 (25:14) = 3609
smk_bw_write(&bw, 220, 11); // *9); // mem (2303585:2) value (16,16) -> 43:43
// Full block 3:1 value4112 (offsetend2303586 bitend2) 76,77:139 (16:16) = 4112
smk_bw_write(&bw, 248, 11); // *8); // mem (2303586:2) value (10,0) -> 25:25
// Full block 0:0 value10 (offsetend2303587 bitend2) 82,83:136 (10:0) = 10
smk_bw_write(&bw, 248, 11); // *9); // mem (2303587:3) value (14,14) -> 25:25
// Full block 0:1 value3598 (offsetend2303588 bitend3) 80,81:136 (14:14) = 3598
smk_bw_write(&bw, 159198, 19); // *11); // mem (2303588:6) value (236,236) -> 12:222
// Full block 1:0 value60652 (offsetend2303589 bitend6) 82,83:137 (236:236) = 60652
smk_bw_write(&bw, 863, 14); // *19); // mem (2303590:1) value (236,128) -> 30:25
// Full block 1:1 value33004 (offsetend2303592 bitend1) 80,81:137 (236:128) = 33004
smk_bw_write(&bw, 254, 15); // *19); // mem (2303592:4) value (236,82) -> 27:11
// Full block 2:0 value21228 (offsetend2303594 bitend4) 82,83:138 (236:82) = 21228
smk_bw_write(&bw, 6200, 13); // *16); // mem (2303594:4) value (128,89) -> 30:25
// Full block 2:1 value22912 (offsetend2303596 bitend4) 80,81:138 (128:89) = 22912
smk_bw_write(&bw, 43, 6); // *14); // mem (2303597:2) value (222,223) -> 0:0
// Full block 3:0 value57310 (offsetend2303598 bitend2) 82,83:139 (222:223) = 57310
smk_bw_write(&bw, 863, 14); // *15); // mem (2303599:1) value (19,231) -> 31:25
// Full block 3:1 value59155 (offsetend2303600 bitend1) 80,81:139 (19:231) = 59155
smk_bw_write(&bw, 191, 12); // *6); // mem (2303600:7) value (0,0) -> 14:18
// Full block 0:0 value0 (offsetend2303600 bitend7) 86,87:136 (0:0) = 0
smk_bw_write(&bw, 13757, 14); // *4); // mem (2303601:3) value (0,0) -> 222:0
// Full block 0:1 value0 (offsetend2303601 bitend3) 84,85:136 (0:0) = 0
smk_bw_write(&bw, 51869, 20); // mem (2303601:7) value (236,89) -> 222:11
// Full block 1:0 value23020 (offsetend2303603 bitend7) 86,87:137 (236:89) = 23020
smk_bw_write(&bw, 13757, 14); // *19); // mem (2303604:2) value (89,236) -> 222:0
// Full block 1:1 value60505 (offsetend2303606 bitend2) 84,85:137 (89:236) = 60505
smk_bw_write(&bw, 3228, 15); // *11); // mem (2303606:5) value (236,236) -> 0:222
// Full block 2:0 value60652 (offsetend2303607 bitend5) 86,87:138 (236:236) = 60652
smk_bw_write(&bw, 13757, 14); // *19); // mem (2303608:0) value (236,82) -> 222:0
// Full block 2:1 value21228 (offsetend2303610 bitend0) 84,85:138 (236:82) = 21228
smk_bw_write(&bw, 3228, 15); // mem (2303610:7) value (0,222)
// Full block 3:0 value56832 (offsetend2303611 bitend7) 86,87:139 (0:222) = 56832
smk_bw_write(&bw, 43, 6); // *14); // mem (2303612:5) value (222,223) -> 0:0
// Full block 3:1 value57310 (offsetend2303613 bitend5) 84,85:139 (222:223) = 57310
smk_bw_write(&bw, 374, 10); // *6); // mem (2303614:3) value (0,0) -> 14:0
// Full block 0:0 value0 (offsetend2303614 bitend3) 90,91:136 (0:0) = 0
smk_bw_write(&bw, 1291, 12); // *4); // mem (2303614:7) value (0,0) -> 18:14
// Full block 0:1 value0 (offsetend2303614 bitend7) 88,89:136 (0:0) = 0
smk_bw_write(&bw, 3228, 15); // *19); // mem (2303615:2) value (89,236) -> 0:222
// Full block 1:0 value60505 (offsetend2303617 bitend2) 90,91:137 (89:236) = 60505
smk_bw_write(&bw, 3228, 15); // *11); // mem (2303617:5) value (236,236) -> 0:222
// Full block 1:1 value60652 (offsetend2303618 bitend5) 88,89:137 (236:236) = 60652
smk_bw_write(&bw, 3228, 15); // *19); // mem (2303619:0) value (236,82) -> 0:222
// Full block 2:0 value21228 (offsetend2303621 bitend0) 90,91:138 (236:82) = 21228
smk_bw_write(&bw, 3228, 15); // *20); // mem (2303621:4) value (82,236) -> 0:222
// Full block 2:1 value60498 (offsetend2303623 bitend4) 88,89:138 (82:236) = 60498
smk_bw_write(&bw, 3228, 15); // *14); // mem (2303624:2) value (222,223) -> 0:222
// Full block 3:0 value57310 (offsetend2303625 bitend2) 90,91:139 (222:223) = 57310
smk_bw_write(&bw, 3228, 15); // *16); // mem (2303625:2) value (223,0) -> 0:222
// Full block 3:1 value223 (offsetend2303627 bitend2) 88,89:139 (223:0) = 223
smk_bw_write(&bw, 39186, 17); // mem (2303627:3) value (185,192) -> 185:199
// Full block 0:0 value49337 (offsetend2303629 bitend3) 94,95:136 (185:192) = 49337
smk_bw_write(&bw, 62444, 16); // mem (2303629:3) value (0,99)
// Full block 0:1 value25344 (offsetend2303631 bitend3) 92,93:136 (0:99) = 25344
smk_bw_write(&bw, 277284, 19); // mem (2303631:6) value (239,203)
// Full block 1:0 value52207 (offsetend2303633 bitend6) 94,95:137 (239:203) = 52207
smk_bw_write(&bw, 51869, 20); // mem (2303634:2) value (236,89)
// Full block 1:1 value23020 (offsetend2303636 bitend2) 92,93:137 (236:89) = 23020
smk_bw_write(&bw, 426660, 20); // mem (2303636:6) value (128,182)
// Full block 2:0 value46720 (offsetend2303638 bitend6) 94,95:138 (128:182) = 46720
smk_bw_write(&bw, 6186, 14); // *11); // mem (2303639:1) value (236,236) -> 222:225
// Full block 2:1 value60652 (offsetend2303640 bitend1) 92,93:138 (236:236) = 60652
smk_bw_write(&bw, 62444, 16); // *21); // mem (2303640:6) value (222,99) -> 0:99
// Full block 3:0 value25566 (offsetend2303642 bitend6) 94,95:139 (222:99) = 25566
smk_bw_write(&bw, 1914, 13); // *8); // mem (2303642:6) value (0,10) -> 0:30
// Full block 3:1 value2560 (offsetend2303643 bitend6) 92,93:139 (0:10) = 2560
smk_bw_write(&bw, 10646, 14); // mem (2303644:4) value (138,144)
// Full block 0:0 value37002 (offsetend2303645 bitend4) 98,99:136 (138:144) = 37002
smk_bw_write(&bw, 305948, 19); // mem (2303645:7) value (192,177)
// Full block 0:1 value45504 (offsetend2303647 bitend7) 96,97:136 (192:177) = 45504
smk_bw_write(&bw, 1007, 11); // mem (2303648:2) value (160,160)
// Full block 1:0 value41120 (offsetend2303649 bitend2) 98,99:137 (160:160) = 41120
smk_bw_write(&bw, 47139, 17); // mem (2303649:3) value (194,174)
// Full block 1:1 value44738 (offsetend2303651 bitend3) 96,97:137 (194:174) = 44738
smk_bw_write(&bw, 6437, 13); // mem (2303652:0) value (174,182)
// Full block 2:0 value46766 (offsetend2303653 bitend0) 98,99:138 (174:182) = 46766
smk_bw_write(&bw, 441840, 19); // mem (2303653:3) value (203,181)
// Full block 2:1 value46539 (offsetend2303655 bitend3) 96,97:138 (203:181) = 46539
smk_bw_write(&bw, 367597, 20); // mem (2303655:7) value (181,238)
// Full block 3:0 value61109 (offsetend2303657 bitend7) 98,99:139 (181:238) = 61109
smk_bw_write(&bw, 83485, 17); // mem (2303658:0) value (181,199)
// Full block 3:1 value51125 (offsetend2303660 bitend0) 96,97:139 (181:199) = 51125
smk_bw_write(&bw, 1314, 12); // mem (2303660:4) value (238,238)
// Full block 0:0 value61166 (offsetend2303661 bitend4) 102,103:136 (238:238) = 61166
smk_bw_write(&bw, 352896, 19); // mem (2303661:7) value (185,239)
// Full block 0:1 value61369 (offsetend2303663 bitend7) 100,101:136 (185:239) = 61369
smk_bw_write(&bw, 1314, 12); // mem (2303664:3) value (239,239)
// Full block 1:0 value61423 (offsetend2303665 bitend3) 102,103:137 (239:239) = 61423
smk_bw_write(&bw, 172510, 19); // mem (2303665:6) value (182,241)
// Full block 1:1 value61878 (offsetend2303667 bitend6) 100,101:137 (182:241) = 61878
smk_bw_write(&bw, 24095, 15); // mem (2303668:5) value (239,240)						(238,148) d18 185593
// Full block 2:0 value61679 (offsetend2303669 bitend5) 102,103:138 (239:240) = 61679
smk_bw_write(&bw, 15023, 14); // mem (2303670:3) value (241,239)
// Full block 2:1 value61425 (offsetend2303671 bitend3) 100,101:138 (241:239) = 61425
smk_bw_write(&bw, 1721, 12); // mem (2303671:7) value (239,239)
// Full block 3:0 value61423 (offsetend2303672 bitend7) 102,103:139 (239:239) = 61423
smk_bw_write(&bw, 7, 4); // mem (2303673:3) value (239,239)
// Full block 3:1 value61423 (offsetend2303673 bitend3) 100,101:139 (239:239) = 61423
smk_bw_write(&bw, 72960, 19); // mem (2303673:6) value (128,115)
// Full block 0:0 value29568 (offsetend2303675 bitend6) 106,107:136 (128:115) = 29568
smk_bw_write(&bw, 32572, 15); // mem (2303676:5) value (237,239)
// Full block 0:1 value61421 (offsetend2303677 bitend5) 104,105:136 (237:239) = 61421
smk_bw_write(&bw, 952813, 20); // mem (2303678:1) value (238,142)
// Full block 1:0 value36590 (offsetend2303680 bitend1) 106,107:137 (238:142) = 36590
smk_bw_write(&bw, 1314, 12); // mem (2303680:5) value (239,239)
// Full block 1:1 value61423 (offsetend2303681 bitend5) 104,105:137 (239:239) = 61423
smk_bw_write(&bw, 25628, 19); // *20); // mem (2303682:1) value (241,237)
// Full block 2:0 value60913 (offsetend2303684 bitend1) 106,107:138 (241:237) = 60913
smk_bw_write(&bw, 31436, 16); // *15); // mem (2303685:0) value (239,241)
// Full block 2:1 value61935 (offsetend2303686 bitend0) 104,105:138 (239:241) = 61935
smk_bw_write(&bw, 484380, 19); // mem (2303686:3) value (185,148)
// Full block 3:0 value38073 (offsetend2303688 bitend3) 106,107:139 (185:148) = 38073
smk_bw_write(&bw, 1998, 15); // mem (2303689:2) value (238,239)
// Full block 3:1 value61422 (offsetend2303690 bitend2) 104,105:139 (238:239) = 61422
smk_bw_write(&bw, 499741, 19); // mem (2303690:5) value (169,21)
// Full block 0:0 value5545 (offsetend2303692 bitend5) 110,111:136 (169:21) = 5545
smk_bw_write(&bw, 8760, 16); // mem (2303692:5) value (108,142)
// Full block 0:1 value36460 (offsetend2303694 bitend5) 108,109:136 (108:142) = 36460
smk_bw_write(&bw, 821376, 20); // mem (2303695:1) value (169,236)
// Full block 1:0 value60585 (offsetend2303697 bitend1) 110,111:137 (169:236) = 60585
smk_bw_write(&bw, 122864, 17); // mem (2303697:2) value (142,181)
// Full block 1:1 value46478 (offsetend2303699 bitend2) 108,109:137 (142:181) = 46478
smk_bw_write(&bw, 457200, 19); // mem (2303699:5) value (236,82)
// Full block 2:0 value21228 (offsetend2303701 bitend5) 110,111:138 (236:82) = 21228
smk_bw_write(&bw, 103530, 20); // mem (2303702:1) value (161,194)
// Full block 2:1 value49825 (offsetend2303704 bitend1) 108,109:138 (161:194) = 49825
smk_bw_write(&bw, 13757, 14); // mem (2303704:7) value (222,223) -> 222:0
// Full block 3:0 value57310 (offsetend2303705 bitend7) 110,111:139 (222:223) = 57310
smk_bw_write(&bw, 420589, 21); // mem (2303706:4) value (181,148)
// Full block 3:1 value38069 (offsetend2303708 bitend4) 108,109:139 (181:148) = 38069
smk_bw_write(&bw, 43, 6); // mem (2303709:2) value (0,0)
// Full block 0:0 value0 (offsetend2303709 bitend2) 114,115:136 (0:0) = 0
smk_bw_write(&bw, 7, 4); // mem (2303709:6) value (0,0)
// Full block 0:1 value0 (offsetend2303709 bitend6) 112,113:136 (0:0) = 0
smk_bw_write(&bw, 76906, 19); // mem (2303710:1) value (236,74) -> 10:222
// Full block 1:0 value19180 (offsetend2303712 bitend1) 114,115:137 (236:74) = 19180
smk_bw_write(&bw, 141930, 19); // mem (2303712:4) value (89,236) -> 228:10
// Full block 1:1 value60505 (offsetend2303714 bitend4) 112,113:137 (89:236) = 60505
smk_bw_write(&bw, 76906, 19); // *4); // mem (2303715:0) value (236,74) -> 10:222
// Full block 2:0 value19180 (offsetend2303715 bitend0) 114,115:138 (236:74) = 19180
smk_bw_write(&bw, 141930, 19); // mem (2303715:3) value (236,82) -> 228:10
// Full block 2:1 value21228 (offsetend2303717 bitend3) 112,113:138 (236:82) = 21228
smk_bw_write(&bw, 13757, 14); // mem (2303718:1) value (222,223) -> 222:0
// Full block 3:0 value57310 (offsetend2303719 bitend1) 114,115:139 (222:223) = 57310
smk_bw_write(&bw, 7, 4); // mem (2303719:5) value (222,223)
// Full block 3:1 value57310 (offsetend2303719 bitend5) 112,113:139 (222:223) = 57310

	LogErrorFF("Patchlen(1) %d", (size_t)bw.buffer - (size_t)bufMem - 2303424);
	uint32_t *lm = (uint32_t*)&bufMem[2303424];
	for (int i = 0; i < ((size_t)bw.buffer + (bw.bit_num != 0 ? 1 : 0) - (size_t)bufMem - 2303424 + 3) / 4; i++) {
		LogErrorFF("	lm[%d] = SwapLE32(%d);", i, lm[i]);
	}

*/
uint32_t *cur = (uint32_t*)&bufMem[2303424];
	cur[0] = -692176717;
	cur[1] = 1924240277;
	cur[2] = -1116350926;
	cur[3] = 846226659;
	cur[4] = 211556664;
	cur[5] = 1836009038;
	cur[6] = 1847955435;
	cur[7] = -1153933168;
	cur[8] = -14372439;
	cur[9] = -1352999270;
	cur[10] = 2047723429;
	cur[11] = 231186652;
	cur[12] = -748630536;
	cur[13] = 1183210856;
	cur[14] = -1080806677;
	cur[15] = -1159502474;
	cur[16] = 1490301554;
	cur[17] = 1666238914;
	cur[18] = -1820121335;
	cur[19] = 211594929;
	cur[20] = -2094594482;
	cur[21] = -523648621;
	cur[22] = 657173604;
	cur[23] = 1540461443;
	cur[24] = 1663511471;
	cur[25] = -1676068515;
	cur[26] = 654724620;
	cur[27] = -910060669;
	cur[28] = -1047294120;
	cur[29] = 1885659337;
	cur[30] = -1676068814;
	cur[31] = 76938764;
	cur[32] = 1988926659;
	cur[33] = -2121035208;
	cur[34] = 162658691;
	cur[35] = 1300551711;
	cur[36] = -1315994408;
	cur[37] = -943693268;
	cur[38] = 1911569951;
	cur[39] = -598899496;
	cur[40] = -1136785472;
	cur[41] = -130681637;
	cur[42] = -84905981;
	cur[43] = 1866660917;
	cur[44] = -1123243555;
	cur[45] = -1214044363;
	cur[46] = 1991009510;
	cur[47] = -2094590931;
	cur[48] = -523648621;
	cur[49] = 942829668;
	cur[50] = -661056999;
	cur[51] = -663336473;
	cur[52] = -2103178071;
	cur[53] = -810450266;
	cur[54] = -1798627861;
	cur[55] = 468686179;
	cur[56] = -510503487;
	cur[57] = 1505680763;
	cur[58] = 172312093;
	cur[59] = -450712496;
	cur[60] = -284186083;
	cur[61] = 124493483;
	cur[62] = 2141065680;
	cur[63] = 344695419;
	cur[64] = -174581497;
	cur[65] = 2096023608;
	cur[66] = -1908604696;
	cur[67] = -1021173752;
	cur[68] = -1211136129;
	cur[69] = -688626406;
	cur[70] = 1030122221;
	cur[71] = -1448569291;
	cur[72] = 1780845896;
	cur[73] = -810685910;
} else {
	patchCounter++;
// Vic3
	bw.buffer += 2313522;
	bw.bit_num = 7;
/*
smk_bw_skip(&bw, 6); // mem (2313523:5) value (0,0)
// Full block 0:0 value0 (offsetend2313523 bitend5) 30,31:136 (0:0) = 0
smk_bw_write(&bw, 3, 4); // *4); // mem (2313524:1) value (0,0)
// Full block 0:1 value0 (offsetend2313524 bitend1) 28,29:136 (0:0) = 0
smk_bw_write(&bw, 14802, 15); // *20); // mem (2313524:5) value (22,99) -> 0:222
// Full block 1:0 value25366 (offsetend2313526 bitend5) 30,31:137 (22:99) = 25366
smk_bw_write(&bw, 23, 6); // *4); // mem (2313527:1) value (0,0)
// Full block 1:1 value0 (offsetend2313527 bitend1) 28,29:137 (0:0) = 0
smk_bw_write(&bw, 14802, 15); // *18); // mem (2313527:3) value (23,236) -> 0:222
// Full block 2:0 value60439 (offsetend2313529 bitend3) 30,31:138 (23:236) = 60439
smk_bw_write(&bw, 23, 6); // *4); // mem (2313529:7) value (0,0)
// Full block 2:1 value0 (offsetend2313529 bitend7) 28,29:138 (0:0) = 0
smk_bw_write(&bw, 3, 4); // *4); // mem (2313530:3) value (0,0)
// Full block 3:0 value0 (offsetend2313530 bitend3) 30,31:139 (0:0) = 0
smk_bw_write(&bw, 3, 4); // *4); // mem (2313530:7) value (0,0)
// Full block 3:1 value0 (offsetend2313530 bitend7) 28,29:139 (0:0) = 0
smk_bw_write(&bw, 3, 4); // *4); // mem (2313531:3) value (0,0)
// Full block 0:0 value0 (offsetend2313531 bitend3) 34,35:136 (0:0) = 0
smk_bw_write(&bw, 3, 4); // mem (2313531:7) value (0,0)
// Full block 0:1 value0 (offsetend2313531 bitend7) 32,33:136 (0:0) = 0
smk_bw_write(&bw, 14802, 15); // *19); // mem (2313532:2) value (89,236) -> 0:222
// Full block 1:0 value60505 (offsetend2313534 bitend2) 34,35:137 (89:236) = 60505
smk_bw_write(&bw, 14802, 15); // *10); // mem (2313534:4) value (236,236) -> 0:222
// Full block 1:1 value60652 (offsetend2313535 bitend4) 32,33:137 (236:236) = 60652
smk_bw_write(&bw, 14802, 15); // *19); // mem (2313535:7) value (236,82) -> 0:222
// Full block 2:0 value21228 (offsetend2313537 bitend7) 34,35:138 (236:82) = 21228
smk_bw_write(&bw, 14802, 15); // *20); // mem (2313538:3) value (82,236) -> 0:222
// Full block 2:1 value60498 (offsetend2313540 bitend3) 32,33:138 (82:236) = 60498
smk_bw_write(&bw, 14802, 15); // mem (2313541:1) value (222,223) -> 0:222
// Full block 3:0 value57310 (offsetend2313542 bitend1) 34,35:139 (222:223) = 57310
smk_bw_write(&bw, 7463, 14); // mem (2313542:7) value (222,0)
// Full block 3:1 value222 (offsetend2313543 bitend7) 32,33:139 (222:0) = 222
smk_bw_write(&bw, 12525, 14); // *13); // mem (2313544:4) value (49,43) -> 50:57
// Full block 0:0 value11057 (offsetend2313545 bitend4) 38,39:136 (49:43) = 11057
smk_bw_write(&bw, 2043, 13); // mem (2313546:1) value (0,37)
// Full block 0:1 value9472 (offsetend2313547 bitend1) 36,37:136 (0:37) = 9472
smk_bw_write(&bw, 1023, 10); // *11); // mem (2313547:4) value (128,128) -> 43:43
// Full block 1:0 value32896 (offsetend2313548 bitend4) 38,39:137 (128:128) = 32896
smk_bw_write(&bw, 41428, 19); // mem (2313548:7) value (236,128) -> 222:25
// Full block 1:1 value33004 (offsetend2313550 bitend7) 36,37:137 (236:128) = 33004
smk_bw_write(&bw, 6713, 13); // *4); // mem (2313551:3) value (236,128) -> 25:30
// Full block 2:0 value33004 (offsetend2313551 bitend3) 38,39:138 (236:128) = 33004
smk_bw_write(&bw, 326950, 19); // mem (2313551:6) value (236,74) -> 10:222
// Full block 2:1 value19180 (offsetend2313553 bitend6) 36,37:138 (236:74) = 19180
smk_bw_write(&bw, 258453, 18); // mem (2313554:0) value (222,37)
// Full block 3:0 value9694 (offsetend2313556 bitend0) 38,39:139 (222:37) = 9694
smk_bw_write(&bw, 7463, 14); // mem (2313556:6) value (222,223) -> 222:0
// Full block 3:1 value57310 (offsetend2313557 bitend6) 36,37:139 (222:223) = 57310
smk_bw_write(&bw, 1797, 14); // *13); // mem (2313558:3) value (30,25) -> 57:50
// Full block 0:0 value6430 (offsetend2313559 bitend3) 42,43:136 (30:25) = 6430
smk_bw_write(&bw, 84, 9); // *11); // mem (2313559:6) value (37,37) -> 57:57
// Full block 0:1 value9509 (offsetend2313560 bitend6) 40,41:136 (37:37) = 9509
smk_bw_write(&bw, 1023, 10); // *11); // mem (2313561:1) value (128,128) -> 43:43
// Full block 1:0 value32896 (offsetend2313562 bitend1) 42,43:137 (128:128) = 32896
smk_bw_write(&bw, 84, 9); // *4); // mem (2313562:5) value (128,128) -> 57:57
// Full block 1:1 value32896 (offsetend2313562 bitend5) 40,41:137 (128:128) = 32896
smk_bw_write(&bw, 1023, 10); // *19); // mem (2313563:0) value (236,128) -> 43:43
// Full block 2:0 value33004 (offsetend2313565 bitend0) 42,43:138 (236:128) = 33004
smk_bw_write(&bw, 84, 9); // *4); // mem (2313565:4) value (236,128) -> 57:57
// Full block 2:1 value33004 (offsetend2313565 bitend4) 40,41:138 (236:128) = 33004
smk_bw_write(&bw, 1023, 10); // *13); // mem (2313566:1) value (37,14) -> 43:43
// Full block 3:0 value3621 (offsetend2313567 bitend1) 42,43:139 (37:14) = 3621
smk_bw_write(&bw, 19656, 17); // *14); // mem (2313567:7) value (55,37) -> 44:57
// Full block 3:1 value9527 (offsetend2313568 bitend7) 40,41:139 (55:37) = 9527
smk_bw_write(&bw, 1928, 13); // *6); // mem (2313569:5) value (0,0) -> 18:10
// Full block 0:0 value0 (offsetend2313569 bitend5) 46,47:136 (0:0) = 0
smk_bw_write(&bw, 7471, 13); // *11); // mem (2313570:0) value (14,11) -> 30:18
// Full block 0:1 value2830 (offsetend2313571 bitend0) 44,45:136 (14:11) = 2830
smk_bw_write(&bw, 2439, 13); // *10); // mem (2313571:2) value (236,236) -> 18:11
// Full block 1:0 value60652 (offsetend2313572 bitend2) 46,47:137 (236:236) = 60652
smk_bw_write(&bw, 7471, 13); // *19); // mem (2313572:5) value (236,128) -> 30:18
// Full block 1:1 value33004 (offsetend2313574 bitend5) 44,45:137 (236:128) = 33004
smk_bw_write(&bw, 2439, 13); // *19); // mem (2313575:0) value (89,236) -> 18:11
// Full block 2:0 value60505 (offsetend2313577 bitend0) 46,47:138 (89:236) = 60505
smk_bw_write(&bw, 3244, 13); // *21); // mem (2313577:5) value (128,236) -> 43:18
// Full block 2:1 value60544 (offsetend2313579 bitend5) 44,45:138 (128:236) = 60544
smk_bw_write(&bw, 193, 10); // *16); // mem (2313579:5) value (223,0) -> 14:0
// Full block 3:0 value223 (offsetend2313581 bitend5) 46,47:139 (223:0) = 223
smk_bw_write(&bw, 2949, 12); // *8); // mem (2313581:5) value (10,10) -> 37:30
// Full block 3:1 value2570 (offsetend2313582 bitend5) 44,45:139 (10:10) = 2570
smk_bw_write(&bw, 23, 6); // mem (2313583:3) value (0,0)
// Full block 0:0 value0 (offsetend2313583 bitend3) 50,51:136 (0:0) = 0
smk_bw_write(&bw, 3, 4); // mem (2313583:7) value (0,0)
// Full block 0:1 value0 (offsetend2313583 bitend7) 48,49:136 (0:0) = 0
smk_bw_write(&bw, 14802, 15); // *20); // mem (2313584:3) value (236,89) -> 0:222
// Full block 1:0 value23020 (offsetend2313586 bitend3) 50,51:137 (236:89) = 23020
smk_bw_write(&bw, 326950, 19); // *19); // mem (2313586:6) value (89,236) -> 10:222
// Full block 1:1 value60505 (offsetend2313588 bitend6) 48,49:137 (89:236) = 60505
smk_bw_write(&bw, 14802, 15); // *10); // mem (2313589:0) value (236,236) -> 0:222
// Full block 2:0 value60652 (offsetend2313590 bitend0) 50,51:138 (236:236) = 60652
smk_bw_write(&bw, 326950, 19); // *19); // mem (2313590:3) value (236,82) -> 10:222
// Full block 2:1 value21228 (offsetend2313592 bitend3) 48,49:138 (236:82) = 21228
smk_bw_write(&bw, 14802, 15); // mem (2313593:2) value (0,222)
// Full block 3:0 value56832 (offsetend2313594 bitend2) 50,51:139 (0:222) = 56832
smk_bw_write(&bw, 7463, 14); // mem (2313595:0) value (222,223) -> 222:0
// Full block 3:1 value57310 (offsetend2313596 bitend0) 48,49:139 (222:223) = 57310
smk_bw_write(&bw, 23, 6); // mem (2313596:6) value (0,0)
// Full block 0:0 value0 (offsetend2313596 bitend6) 54,55:136 (0:0) = 0
smk_bw_write(&bw, 3, 4); // mem (2313597:2) value (0,0)
// Full block 0:1 value0 (offsetend2313597 bitend2) 52,53:136 (0:0) = 0
smk_bw_write(&bw, 14802, 15); // *19); // mem (2313597:5) value (89,236) -> 0:222
// Full block 1:0 value60505 (offsetend2313599 bitend5) 54,55:137 (89:236) = 60505
smk_bw_write(&bw, 14802, 15); // *10); // mem (2313599:7) value (236,236) -> 0:222
// Full block 1:1 value60652 (offsetend2313600 bitend7) 52,53:137 (236:236) = 60652
smk_bw_write(&bw, 14802, 15); // *19); // mem (2313601:2) value (236,82) -> 0:222
// Full block 2:0 value21228 (offsetend2313603 bitend2) 54,55:138 (236:82) = 21228
smk_bw_write(&bw, 14802, 15); // *20); // mem (2313603:6) value (82,236) -> 0:222
// Full block 2:1 value60498 (offsetend2313605 bitend6) 52,53:138 (82:236) = 60498
smk_bw_write(&bw, 14802, 15); // *14); // mem (2313606:4) value (222,223) -> 0:222
// Full block 3:0 value57310 (offsetend2313607 bitend4) 54,55:139 (222:223) = 57310
smk_bw_write(&bw, 14802, 15); // *16); // mem (2313607:4) value (223,0) -> 0:222
// Full block 3:1 value223 (offsetend2313609 bitend4) 52,53:139 (223:0) = 223
smk_bw_write(&bw, 23, 6); // mem (2313610:2) value (0,0)
// Full block 0:0 value0 (offsetend2313610 bitend2) 58,59:136 (0:0) = 0
smk_bw_write(&bw, 3, 4); // mem (2313610:6) value (0,0)
// Full block 0:1 value0 (offsetend2313610 bitend6) 56,57:136 (0:0) = 0
smk_bw_write(&bw, 14802, 15); // *10); // mem (2313611:0) value (236,236) -> 0:222
// Full block 1:0 value60652 (offsetend2313612 bitend0) 58,59:137 (236:236) = 60652
smk_bw_write(&bw, 14802, 15); // *20); // mem (2313612:4) value (236,89) -> 0:222
// Full block 1:1 value23020 (offsetend2313614 bitend4) 56,57:137 (236:89) = 23020
smk_bw_write(&bw, 14802, 15); // *20); // mem (2313615:0) value (82,236) -> 0:222
// Full block 2:0 value60498 (offsetend2313617 bitend0) 58,59:138 (82:236) = 60498
smk_bw_write(&bw, 23, 6); // mem (2313617:6) value (236,236) -> 0:0
// Full block 2:1 value60652 (offsetend2313617 bitend6) 56,57:138 (236:236) = 60652
smk_bw_write(&bw, 14802, 15); // *16); // mem (2313617:6) value (223,0) -> 0:222
// Full block 3:0 value223 (offsetend2313619 bitend6) 58,59:139 (223:0) = 223
smk_bw_write(&bw, 14802, 15); // mem (2313620:5) value (0,222)
// Full block 3:1 value56832 (offsetend2313621 bitend5) 56,57:139 (0:222) = 56832
smk_bw_write(&bw, 23, 6); // mem (2313622:3) value (0,0)
// Full block 0:0 value0 (offsetend2313622 bitend3) 62,63:136 (0:0) = 0
smk_bw_write(&bw, 3, 4); // *4); // mem (2313622:7) value (0,0)
// Full block 0:1 value0 (offsetend2313622 bitend7) 60,61:136 (0:0) = 0
smk_bw_write(&bw, 14802, 15); // *20); // mem (2313623:3) value (236,89) -> 0:222
// Full block 1:0 value23020 (offsetend2313625 bitend3) 62,63:137 (236:89) = 23020
smk_bw_write(&bw, 14802, 15); // *19); // mem (2313625:6) value (89,236) -> 0:222
// Full block 1:1 value60505 (offsetend2313627 bitend6) 60,61:137 (89:236) = 60505
smk_bw_write(&bw, 14802, 15); // *10); // mem (2313628:0) value (236,236) -> 0:222
// Full block 2:0 value60652 (offsetend2313629 bitend0) 62,63:138 (236:236) = 60652
smk_bw_write(&bw, 14802, 15); // *19); // mem (2313629:3) value (236,82) -> 0:222
// Full block 2:1 value21228 (offsetend2313631 bitend3) 60,61:138 (236:82) = 21228
smk_bw_write(&bw, 14802, 15); // *15); // mem (2313632:2) value (0,222) -> 0:222
// Full block 3:0 value56832 (offsetend2313633 bitend2) 62,63:139 (0:222) = 56832
smk_bw_write(&bw, 7463, 14); // mem (2313634:0) value (222,223) -> 222:0
// Full block 3:1 value57310 (offsetend2313635 bitend0) 60,61:139 (222:223) = 57310
smk_bw_write(&bw, 23, 6); // mem (2313635:6) value (0,0)
// Full block 0:0 value0 (offsetend2313635 bitend6) 66,67:136 (0:0) = 0
smk_bw_write(&bw, 3, 4); // mem (2313636:2) value (0,0)
// Full block 0:1 value0 (offsetend2313636 bitend2) 64,65:136 (0:0) = 0
smk_bw_write(&bw, 14802, 15); // *19); // mem (2313636:5) value (89,236) -> 0:222
// Full block 1:0 value60505 (offsetend2313638 bitend5) 66,67:137 (89:236) = 60505
smk_bw_write(&bw, 14802, 15); // *10); // mem (2313638:7) value (236,236) -> 0:222
// Full block 1:1 value60652 (offsetend2313639 bitend7) 64,65:137 (236:236) = 60652
smk_bw_write(&bw, 14802, 15); // *19); // mem (2313640:2) value (236,82) -> 0:222
// Full block 2:0 value21228 (offsetend2313642 bitend2) 66,67:138 (236:82) = 21228
smk_bw_write(&bw, 14802, 15); // *20); // mem (2313642:6) value (82,236) -> 0:222
// Full block 2:1 value60498 (offsetend2313644 bitend6) 64,65:138 (82:236) = 60498
smk_bw_write(&bw, 14802, 15); // *14); // mem (2313645:4) value (222,223) -> 0:222
// Full block 3:0 value57310 (offsetend2313646 bitend4) 66,67:139 (222:223) = 57310
smk_bw_write(&bw, 14802, 15); // *16); // mem (2313646:4) value (223,0) -> 0:222
// Full block 3:1 value223 (offsetend2313648 bitend4) 64,65:139 (223:0) = 223
smk_bw_write(&bw, 60, 8); // mem (2313648:4) value (0,10)
// Full block 0:0 value2560 (offsetend2313649 bitend4) 70,71:136 (0:10) = 2560
smk_bw_write(&bw, 23, 6); // mem (2313650:2) value (0,0)
// Full block 0:1 value0 (offsetend2313650 bitend2) 68,69:136 (0:0) = 0
smk_bw_write(&bw, 41428, 19); // mem (2313650:5) value (236,128) -> 222:25
// Full block 1:0 value33004 (offsetend2313652 bitend5) 70,71:137 (236:128) = 33004
smk_bw_write(&bw, 14802, 15); // *20); // mem (2313653:1) value (236,89) -> 0:222
// Full block 1:1 value23020 (offsetend2313655 bitend1) 68,69:137 (236:89) = 23020
smk_bw_write(&bw, 2408587, 22); // *21); // mem (2313655:6) value (128,236) -> 222:30
// Full block 2:0 value60544 (offsetend2313657 bitend6) 70,71:138 (128:236) = 60544
smk_bw_write(&bw, 14802, 15); // *10); // mem (2313658:0) value (236,236) -> 0:222
// Full block 2:1 value60652 (offsetend2313659 bitend0) 68,69:138 (236:236) = 60652
smk_bw_write(&bw, 4312, 13); // *14); // mem (2313659:6) value (11,18) -> 30:25
// Full block 3:0 value4619 (offsetend2313660 bitend6) 70,71:139 (11:18) = 4619
smk_bw_write(&bw, 60, 8); // mem (2313660:6) value (0,10)
// Full block 3:1 value2560 (offsetend2313661 bitend6) 68,69:139 (0:10) = 2560
smk_bw_write(&bw, 3840, 12); // *9); // mem (2313661:7) value (18,18) -> 30:30
// Full block 0:0 value4626 (offsetend2313662 bitend7) 74,75:136 (18:18) = 4626
smk_bw_write(&bw, 452, 11); // *9); // mem (2313663:0) value (14,14) -> 25:25
// Full block 0:1 value3598 (offsetend2313664 bitend0) 72,73:136 (14:14) = 3598
smk_bw_write(&bw, 3840, 12); // *19); // mem (2313664:3) value (236,128) -> 30:30
// Full block 1:0 value33004 (offsetend2313666 bitend3) 74,75:137 (236:128) = 33004
smk_bw_write(&bw, 3840, 12); // mem (2313666:7) value (236,128) -> 30:30
// Full block 1:1 value33004 (offsetend2313666 bitend7) 72,73:137 (236:128) = 33004
smk_bw_write(&bw, 3840, 12); // mem (2313667:3) value (236,128) -> 30:30
// Full block 2:0 value33004 (offsetend2313667 bitend3) 74,75:138 (236:128) = 33004
smk_bw_write(&bw, 452, 11); // mem (2313667:6) value (128,128) -> 25:25
// Full block 2:1 value32896 (offsetend2313668 bitend6) 72,73:138 (128:128) = 32896
smk_bw_write(&bw, 1565, 13); // mem (2313669:3) value (25,18) -> 37:43
// Full block 3:0 value4633 (offsetend2313670 bitend3) 74,75:139 (25:18) = 4633
smk_bw_write(&bw, 13266, 15); // *10); // mem (2313670:5) value (16,21) -> 25:43
// Full block 3:1 value5392 (offsetend2313671 bitend5) 72,73:139 (16:21) = 5392
smk_bw_write(&bw, 452, 11); // *9); // mem (2313671:6) value (14,14) -> 25:25
// Full block 0:0 value3598 (offsetend2313672 bitend6) 78,79:136 (14:14) = 3598
smk_bw_write(&bw, 80, 9); // mem (2313672:7) value (18,18)
// Full block 0:1 value4626 (offsetend2313673 bitend7) 76,77:136 (18:18) = 4626
smk_bw_write(&bw, 452, 11); // *19); // mem (2313674:2) value (236,128) -> 25:25
// Full block 1:0 value33004 (offsetend2313676 bitend2) 78,79:137 (236:128) = 33004
smk_bw_write(&bw, 452, 11); // mem (2313676:6) value (236,128) -> 25:25
// Full block 1:1 value33004 (offsetend2313676 bitend6) 76,77:137 (236:128) = 33004
smk_bw_write(&bw, 452, 11); // mem (2313677:2) value (236,128) -> 25:25
// Full block 2:0 value33004 (offsetend2313677 bitend2) 78,79:138 (236:128) = 33004
smk_bw_write(&bw, 3840, 12); // *11); // mem (2313677:5) value (128,128) -> 30:30
// Full block 2:1 value32896 (offsetend2313678 bitend5) 76,77:138 (128:128) = 32896
smk_bw_write(&bw, 3840, 12); // mem (2313679:1) value (25,14) -> 30:30
// Full block 3:0 value3609 (offsetend2313680 bitend1) 78,79:139 (25:14) = 3609
smk_bw_write(&bw, 1023, 10); // *9); // mem (2313680:2) value (16,16) -> 43:43
// Full block 3:1 value4112 (offsetend2313681 bitend2) 76,77:139 (16:16) = 4112
smk_bw_write(&bw, 452, 11); // *8); // mem (2313681:2) value (10,0) -> 25:25
// Full block 0:0 value10 (offsetend2313682 bitend2) 82,83:136 (10:0) = 10
smk_bw_write(&bw, 452, 11); // *9); // mem (2313682:3) value (14,14) -> 25:25
// Full block 0:1 value3598 (offsetend2313683 bitend3) 80,81:136 (14:14) = 3598
smk_bw_write(&bw, 20, 10); // mem (2313683:5) value (236,236) -> 12:222
// Full block 1:0 value60652 (offsetend2313684 bitend5) 82,83:137 (236:236) = 60652
smk_bw_write(&bw, 4312, 13); // *19); // mem (2313685:0) value (236,128) -> 30:25
// Full block 1:1 value33004 (offsetend2313687 bitend0) 80,81:137 (236:128) = 33004
smk_bw_write(&bw, 415682, 19); // mem (2313687:3) value (236,82) -> 27:11
// Full block 2:0 value21228 (offsetend2313689 bitend3) 82,83:138 (236:82) = 21228
smk_bw_write(&bw, 4312, 13); // *16); // mem (2313689:3) value (128,89) -> 30:25
// Full block 2:1 value22912 (offsetend2313691 bitend3) 80,81:138 (128:89) = 22912
smk_bw_write(&bw, 23, 6); // *14); // mem (2313692:1) value (222,223) -> 0:0
// Full block 3:0 value57310 (offsetend2313693 bitend1) 82,83:139 (222:223) = 57310
smk_bw_write(&bw, 26224, 15); // mem (2313694:0) value (19,231) -> 31:25
// Full block 3:1 value59155 (offsetend2313695 bitend0) 80,81:139 (19:231) = 59155
smk_bw_write(&bw, 1760, 13); // *6); // mem (2313695:6) value (0,0) -> 14:18
// Full block 0:0 value0 (offsetend2313695 bitend6) 86,87:136 (0:0) = 0
smk_bw_write(&bw, 7463, 14); // mem (2313696:2) value (0,0) -> 222:0
// Full block 0:1 value0 (offsetend2313696 bitend2) 84,85:136 (0:0) = 0
smk_bw_write(&bw, 444939, 20); // mem (2313696:6) value (236,89) -> 222:11
// Full block 1:0 value23020 (offsetend2313698 bitend6) 86,87:137 (236:89) = 23020
smk_bw_write(&bw, 7463, 14); // *19); // mem (2313699:1) value (89,236) -> 222:0
// Full block 1:1 value60505 (offsetend2313701 bitend1) 84,85:137 (89:236) = 60505
smk_bw_write(&bw, 14802, 15); // *10); // mem (2313701:3) value (236,236) -> 0:222
// Full block 2:0 value60652 (offsetend2313702 bitend3) 86,87:138 (236:236) = 60652
smk_bw_write(&bw, 7463, 14); // *19); // mem (2313702:6) value (236,82) -> 222:0
// Full block 2:1 value21228 (offsetend2313704 bitend6) 84,85:138 (236:82) = 21228
smk_bw_write(&bw, 14802, 15); // mem (2313705:5) value (0,222)
// Full block 3:0 value56832 (offsetend2313706 bitend5) 86,87:139 (0:222) = 56832
smk_bw_write(&bw, 23, 6); // *14); // mem (2313707:3) value (222,223) -> 0:0
// Full block 3:1 value57310 (offsetend2313708 bitend3) 84,85:139 (222:223) = 57310
smk_bw_write(&bw, 193, 10); // *6); // mem (2313709:1) value (0,0) -> 14:0
// Full block 0:0 value0 (offsetend2313709 bitend1) 90,91:136 (0:0) = 0
smk_bw_write(&bw, 4091, 12); // *4); // mem (2313709:5) value (0,0) -> 18:14
// Full block 0:1 value0 (offsetend2313709 bitend5) 88,89:136 (0:0) = 0
smk_bw_write(&bw, 14802, 15); // *19); // mem (2313710:0) value (89,236) -> 0:222
// Full block 1:0 value60505 (offsetend2313712 bitend0) 90,91:137 (89:236) = 60505
smk_bw_write(&bw, 14802, 15); // *10); // mem (2313712:2) value (236,236) -> 0:222
// Full block 1:1 value60652 (offsetend2313713 bitend2) 88,89:137 (236:236) = 60652
smk_bw_write(&bw, 14802, 15); // *19); // mem (2313713:5) value (236,82) -> 0:222
// Full block 2:0 value21228 (offsetend2313715 bitend5) 90,91:138 (236:82) = 21228
smk_bw_write(&bw, 14802, 15); // *20); // mem (2313716:1) value (82,236) -> 0:222
// Full block 2:1 value60498 (offsetend2313718 bitend1) 88,89:138 (82:236) = 60498
smk_bw_write(&bw, 14802, 15); // *14); // mem (2313718:7) value (222,223) -> 0:222
// Full block 3:0 value57310 (offsetend2313719 bitend7) 90,91:139 (222:223) = 57310
smk_bw_write(&bw, 14802, 15); // *16); // mem (2313719:7) value (223,0) -> 0:222
// Full block 3:1 value223 (offsetend2313721 bitend7) 88,89:139 (223:0) = 223
smk_bw_write(&bw, 57962, 17); // *18); // mem (2313722:1) value (185,192) -> 185:199
// Full block 0:0 value49337 (offsetend2313724 bitend1) 94,95:136 (185:192) = 49337
smk_bw_write(&bw, 8898, 16); // mem (2313724:1) value (0,99)
// Full block 0:1 value25344 (offsetend2313726 bitend1) 92,93:136 (0:99) = 25344
smk_bw_write(&bw, 293428, 19); // mem (2313726:4) value (239,203)
// Full block 1:0 value52207 (offsetend2313728 bitend4) 94,95:137 (239:203) = 52207
smk_bw_write(&bw, 444939, 20); // mem (2313729:0) value (236,89)
// Full block 1:1 value23020 (offsetend2313731 bitend0) 92,93:137 (236:89) = 23020
smk_bw_write(&bw, 547168, 20); // mem (2313731:4) value (128,182)
// Full block 2:0 value46720 (offsetend2313733 bitend4) 94,95:138 (128:182) = 46720
smk_bw_write(&bw, 10874, 14); // *10); // mem (2313733:6) value (236,236) -> 222:225
// Full block 2:1 value60652 (offsetend2313734 bitend6) 92,93:138 (236:236) = 60652
smk_bw_write(&bw, 8898, 16); // *21); // mem (2313735:3) value (222,99) -> 0:99
// Full block 3:0 value25566 (offsetend2313737 bitend3) 94,95:139 (222:99) = 25566
smk_bw_write(&bw, 1618, 13); // *8); // mem (2313737:3) value (0,10) -> 0:30
// Full block 3:1 value2560 (offsetend2313738 bitend3) 92,93:139 (0:10) = 2560
smk_bw_write(&bw, 9793, 14); // mem (2313739:1) value (138,144)
// Full block 0:0 value37002 (offsetend2313740 bitend1) 98,99:136 (138:144) = 37002
smk_bw_write(&bw, 129941, 18); // mem (2313740:3) value (192,177)
// Full block 0:1 value45504 (offsetend2313742 bitend3) 96,97:136 (192:177) = 45504
smk_bw_write(&bw, 939, 11); // mem (2313742:6) value (160,160)
// Full block 1:0 value41120 (offsetend2313743 bitend6) 98,99:137 (160:160) = 41120
smk_bw_write(&bw, 110529, 17); // mem (2313743:7) value (194,174)
// Full block 1:1 value44738 (offsetend2313745 bitend7) 96,97:137 (194:174) = 44738
smk_bw_write(&bw, 6005, 13); // mem (2313746:4) value (174,182)
// Full block 2:0 value46766 (offsetend2313747 bitend4) 98,99:138 (174:182) = 46766
smk_bw_write(&bw, 61280, 19); // mem (2313747:7) value (203,181)
// Full block 2:1 value46539 (offsetend2313749 bitend7) 96,97:138 (203:181) = 46539
smk_bw_write(&bw, 170891, 20); // mem (2313750:3) value (181,238)
// Full block 3:0 value61109 (offsetend2313752 bitend3) 98,99:139 (181:238) = 61109
smk_bw_write(&bw, 13899, 17); // mem (2313752:4) value (181,199)
// Full block 3:1 value51125 (offsetend2313754 bitend4) 96,97:139 (181:199) = 51125
smk_bw_write(&bw, 1967, 11); // mem (2313754:7) value (238,238)
// Full block 0:0 value61166 (offsetend2313755 bitend7) 102,103:136 (238:238) = 61166
smk_bw_write(&bw, 57184, 19); // mem (2313756:2) value (185,239)
// Full block 0:1 value61369 (offsetend2313758 bitend2) 100,101:136 (185:239) = 61369
smk_bw_write(&bw, 3276, 12); // mem (2313758:6) value (239,239)
// Full block 1:0 value61423 (offsetend2313759 bitend6) 102,103:137 (239:239) = 61423
smk_bw_write(&bw, 247657, 19); // mem (2313760:1) value (182,241)
// Full block 1:1 value61878 (offsetend2313762 bitend1) 100,101:137 (182:241) = 61878
smk_bw_write(&bw, 7899, 15); // mem (2313763:0) value (239,240)
// Full block 2:0 value61679 (offsetend2313764 bitend0) 102,103:138 (239:240) = 61679
smk_bw_write(&bw, 4854, 14); // mem (2313764:6) value (241,239)
// Full block 2:1 value61425 (offsetend2313765 bitend6) 100,101:138 (241:239) = 61425
smk_bw_write(&bw, 3185, 12); // mem (2313766:2) value (239,239)
// Full block 3:0 value61423 (offsetend2313767 bitend2) 102,103:139 (239:239) = 61423
smk_bw_write(&bw, 3, 4); // mem (2313767:6) value (239,239)
// Full block 3:1 value61423 (offsetend2313767 bitend6) 100,101:139 (239:239) = 61423
smk_bw_write(&bw, 215904, 19); // mem (2313768:1) value (128,115)
// Full block 0:0 value29568 (offsetend2313770 bitend1) 106,107:136 (128:115) = 29568
smk_bw_write(&bw, 16540, 15); // mem (2313771:0) value (237,239)
// Full block 0:1 value61421 (offsetend2313772 bitend0) 104,105:136 (237:239) = 61421
smk_bw_write(&bw, 420235, 20); // mem (2313772:4) value (238,142)
// Full block 1:0 value36590 (offsetend2313774 bitend4) 106,107:137 (238:142) = 36590
smk_bw_write(&bw, 3276, 12); // mem (2313775:0) value (239,239) -> 237:237
// Full block 1:1 value61423 (offsetend2313776 bitend0) 104,105:137 (239:239) = 61423
smk_bw_write(&bw, 26050, 19); // *20); // mem (2313776:4) value (241,237) -> 169:147
// Full block 2:0 value60913 (offsetend2313778 bitend4) 106,107:138 (241:237) = 60913
smk_bw_write(&bw, 336, 16); // *15); // mem (2313779:3) value (239,241) -> 238:237
// Full block 2:1 value61935 (offsetend2313780 bitend3) 104,105:138 (239:241) = 61935
smk_bw_write(&bw, 484802, 19); // mem (2313780:6) value (185,148)
// Full block 3:0 value38073 (offsetend2313782 bitend6) 106,107:139 (185:148) = 38073
smk_bw_write(&bw, 13665, 15); // mem (2313783:5) value (238,239)
// Full block 3:1 value61422 (offsetend2313784 bitend5) 104,105:139 (238:239) = 61422
smk_bw_write(&bw, 284747, 19); // mem (2313785:0) value (169,21)
// Full block 0:0 value5545 (offsetend2313787 bitend0) 110,111:136 (169:21) = 5545
smk_bw_write(&bw, 23364, 16); // mem (2313787:0) value (108,142)
// Full block 0:1 value36460 (offsetend2313789 bitend0) 108,109:136 (108:142) = 36460
smk_bw_write(&bw, 20832, 20); // mem (2313789:4) value (169,236)
// Full block 1:0 value60585 (offsetend2313791 bitend4) 110,111:137 (169:236) = 60585
smk_bw_write(&bw, 87784, 17); // mem (2313791:5) value (142,181)
// Full block 1:1 value46478 (offsetend2313793 bitend5) 108,109:137 (142:181) = 46478
smk_bw_write(&bw, 192744, 19); // mem (2313794:0) value (236,82)
// Full block 2:0 value21228 (offsetend2313796 bitend0) 110,111:138 (236:82) = 21228
smk_bw_write(&bw, 379590, 20); // mem (2313796:4) value (161,194)
// Full block 2:1 value49825 (offsetend2313798 bitend4) 108,109:138 (161:194) = 49825
smk_bw_write(&bw, 7463, 14); // mem (2313799:2) value (222,223) -> 222:0
// Full block 3:0 value57310 (offsetend2313800 bitend2) 110,111:139 (222:223) = 57310
smk_bw_write(&bw, 862859, 21); // mem (2313800:7) value (181,148)
// Full block 3:1 value38069 (offsetend2313802 bitend7) 108,109:139 (181:148) = 38069
smk_bw_write(&bw, 23, 6); // mem (2313803:5) value (0,0)
// Full block 0:0 value0 (offsetend2313803 bitend5) 114,115:136 (0:0) = 0
smk_bw_write(&bw, 3, 4); // mem (2313804:1) value (0,0)
// Full block 0:1 value0 (offsetend2313804 bitend1) 112,113:136 (0:0) = 0
smk_bw_write(&bw, 326950, 19); // mem (2313804:4) value (236,74) -> 10:222
// Full block 1:0 value19180 (offsetend2313806 bitend4) 114,115:137 (236:74) = 19180
smk_bw_write(&bw, 211750, 19); // mem (2313806:7) value (89,236) -> 228:10
// Full block 1:1 value60505 (offsetend2313808 bitend7) 112,113:137 (89:236) = 60505
smk_bw_write(&bw, 326950, 19); // mem (2313809:3) value (236,74) -> 10:222
// Full block 2:0 value19180 (offsetend2313809 bitend3) 114,115:138 (236:74) = 19180
smk_bw_write(&bw, 415682, 19); // mem (2313809:6) value (236,82) -> 228:10
// Full block 2:1 value21228 (offsetend2313811 bitend6) 112,113:138 (236:82) = 21228
smk_bw_write(&bw, 7463, 14); // mem (2313812:4) value (222,223) -> 222:0
// Full block 3:0 value57310 (offsetend2313813 bitend4) 114,115:139 (222:223) = 57310
smk_bw_write(&bw, 3, 4); // mem (2313814:0) value (222,223)
// Full block 3:1 value57310 (offsetend2313814 bitend0) 112,113:139 (222:223) = 57310

	LogErrorFF("Patchlen(2) %d", (size_t)bw.buffer - (size_t)bufMem - 2313520);
	uint32_t *lm = (uint32_t*)&bufMem[2313520];
	for (int i = 0; i < ((size_t)bw.buffer + (bw.bit_num != 0 ? 1 : 0) - (size_t)bufMem - 2313520 + 3) / 4; i++) {
		LogErrorFF("	lm[%d] = SwapLE32(%d);", i, lm[i]);
	}

*/
uint32_t *cur = (uint32_t*)&bufMem[2313520];
	cur[0] = 1810378268;
	cur[1] = 1956082596;
	cur[2] = -1852204306;
	cur[3] = -1528346162;
	cur[4] = -382086541;
	cur[5] = 248990172;
	cur[6] = -1442865169;
	cur[7] = 1302565187;
	cur[8] = -15115782;
	cur[9] = 1349539657;
	cur[10] = -3497991;
	cur[11] = 1715994196;
	cur[12] = -94472062;
	cur[13] = -2014741919;
	cur[14] = 1392874889;
	cur[15] = -414656584;
	cur[16] = -1658193332;
	cur[17] = 1957161267;
	cur[18] = 1186702574;
	cur[19] = -1818417350;
	cur[20] = -1528346162;
	cur[21] = 1807340147;
	cur[22] = 970093476;
	cur[23] = -1658462999;
	cur[24] = 593350291;
	cur[25] = 1238274973;
	cur[26] = -764173081;
	cur[27] = 451843001;
	cur[28] = 1316265193;
	cur[29] = -1818417350;
	cur[30] = 2028423630;
	cur[31] = 1213262382;
	cur[32] = 1233196775;
	cur[33] = -466215703;
	cur[34] = 14841857;
	cur[35] = 1006878780;
	cur[36] = -192136591;
	cur[37] = -2008008564;
	cur[38] = 14818371;
	cur[39] = 1342160956;
	cur[40] = -2142182884;
	cur[41] = -1934654195;
	cur[42] = 859320077;
	cur[43] = 384383416;
	cur[44] = -1817907820;
	cur[45] = -764109362;
	cur[46] = -40358983;
	cur[47] = -414593385;
	cur[48] = 970093476;
	cur[49] = 1316265193;
	cur[50] = 145792077;
	cur[51] = -1810424179;
	cur[52] = -183817203;
	cur[53] = 689004884;
	cur[54] = -296328173;
	cur[55] = -673137993;
	cur[56] = 502011765;
	cur[57] = 1689426827;
	cur[58] = -547293725;
	cur[59] = -474683808;
	cur[60] = -1511228563;
	cur[61] = -1515184584;
	cur[62] = -753466767;
	cur[63] = -880436852;
	cur[64] = 1545606400;
	cur[65] = 316322038;
	cur[66] = -1061779178;
	cur[67] = 719126690;
	cur[68] = -1785938886;
	cur[69] = 1415292139;
	cur[70] = -191309975;
	cur[71] = 644310227;
	cur[72] = -222423811;
	cur[73] = 1019295561;
}

}

/* PUBLIC FUNCTIONS */
/* open an smk (from a generic Source) */
#ifdef FULL
static smk smk_open_generic(const unsigned char m, union smk_read_t fp, unsigned long size, const unsigned char process_mode)
#else
static smk smk_open_generic(union smk_read_t fp, unsigned long size)
#endif
{
	/* Smacker structure we intend to work on / return */
	smk s;
	/* Temporary variables */
	long temp_l;
	unsigned long temp_u;
	/* r is used by macros above for return code */
	char r;
	unsigned char buf[4];// = {'\0'};
	/* video hufftrees are stored as a large chunk (bitstream)
		these vars are used to load, then decode them */
	unsigned char * hufftree_chunk = NULL;
	unsigned long tree_size;
	/* a bitstream struct */
	struct smk_bit_t bs;

	/** **/
	/* safe malloc the structure */
#ifdef FULL
	if ((s = calloc(1, sizeof(struct smk_t))) == NULL) {
		PrintError("libsmacker::smk_open_generic() - ERROR: failed to malloc() smk structure");
		return NULL;
	}

	/* Check for a valid signature */
	smk_read(buf, 4);

	if (buf[0] != 'S' || buf[1] != 'M' || buf[2] != 'K') {
		LogError("libsmacker::smk_open_generic() - ERROR: invalid SMKn signature (got: %s)\n", buf);
		goto error;
	}

	/* Read .smk file version */
	//smk_read(buf, 1);
	if (buf[3] != '2' && buf[3] != '4') {
		LogError("libsmacker::smk_open_generic() - Warning: invalid SMK version %c (expected: 2 or 4)\n", buf[3]);

		/* take a guess */
		if (buf[3] < '4')
			buf[3] = '2';
		else
			buf[3] = '4';

		LogError("\tProcessing will continue as type %c\n", buf[3]);
	}
	s->video.v = buf[3];

	/* width, height, total num frames */
	smk_read_ul(s->video.w);
	smk_read_ul(s->video.h);
	smk_read_ul(s->f);
	/* frames per second calculation */
	smk_read_ul(temp_u);
	temp_l = (int)temp_u;

	if (temp_l > 0) {
		/* millisec per frame */
		s->usf = temp_l * 1000;
	} else if (temp_l < 0) {
		/* 10 microsec per frame */
		s->usf = temp_l * -10;
	} else {
		/* defaults to 10 usf (= 100000 microseconds) */
		s->usf = 100000;
	}

	/* Video flags follow.
		Ring frame is important to libsmacker.
		Y scale / Y interlace go in the Video flags.
		The user should scale appropriately. */
	smk_read_ul(temp_u);
	if (temp_u & 0x01)
		s->ring_frame = 1;

	if (temp_u & 0x02)
		s->video.y_scale_mode = SMK_FLAG_Y_DOUBLE;

	if (temp_u & 0x04) {
		if (s->video.y_scale_mode == SMK_FLAG_Y_DOUBLE)
			LogErrorMsg("libsmacker::smk_open_generic() - Warning: SMK file specifies both Y-Double AND Y-Interlace.\n");

		s->video.y_scale_mode = SMK_FLAG_Y_INTERLACE;
	}
	/* Max buffer size for each audio track - used to pre-allocate buffers */
	for (temp_l = 0; temp_l < 7; temp_l ++)
		smk_read_ul(s->audio[temp_l].max_buffer);

	/* Read size of "hufftree chunk" - save for later. */
	smk_read_ul(tree_size);

	/* "unpacked" sizes of each huff tree */
	for (temp_l = 0; temp_l < 4; temp_l ++)
		smk_read_ul(s->video.tree_size[temp_l]);

	/* read audio rate data */
	for (temp_l = 0; temp_l < 7; temp_l ++) {
		smk_read_ul(temp_u);

		if (temp_u & 0x40000000) {
			/* Audio track specifies "exists" flag, malloc structure and copy components. */
			s->audio[temp_l].exists = 1;

			/* and for all audio tracks */
			smk_malloc(s->audio[temp_l].buffer, s->audio[temp_l].max_buffer);

			if (temp_u & 0x80000000)
				s->audio[temp_l].compress = 1;

			s->audio[temp_l].bitdepth = ((temp_u & 0x20000000) ? 16 : 8);
			s->audio[temp_l].channels = ((temp_u & 0x10000000) ? 2 : 1);

			if (temp_u & 0x0c000000) {
				LogError("libsmacker::smk_open_generic() - Warning: audio track %ld is compressed with Bink (perceptual) Audio Codec: this is currently unsupported by libsmacker\n", temp_l);
				s->audio[temp_l].compress = 2;
			}

			/* Bits 25 & 24 are unused. */
			s->audio[temp_l].rate = (temp_u & 0x00FFFFFF);
		}
	}

	/* Skip over Dummy field */
	smk_read_ul(temp_u);
#else
	unsigned long video_tree_size[SMK_TREE_COUNT];
 bufMem = fp.ram;
 bufSize = size;
// patchFile();
	s = NULL;
	smk_mallocc(s, sizeof(struct smk_t), smk_t);
	smk_header *hdr = (smk_header*)fp.ram;
	if (size < sizeof(smk_header)) {
		LogError("libsmacker::smk_open_generic() - ERROR: SMK content is too short. (got: %d, required: %d)\n", size, sizeof(smk_header));
		goto error;
	}
	size -= sizeof(smk_header);
	fp.ram += sizeof(smk_header);

	/* Check for a valid signature */
	if (hdr->SmkMarker[0] != 'S' || hdr->SmkMarker[1] != 'M' || hdr->SmkMarker[2] != 'K') {
		LogError("libsmacker::smk_open_generic() - ERROR: invalid SMKn signature (got: %c%c%c)\n", hdr->SmkMarker[0], hdr->SmkMarker[1], hdr->SmkMarker[2]);
		goto error;
	}
	/* Read .smk file version */
	if (hdr->SmkMarker[3] != '2') {
		LogError("libsmacker::smk_open_generic() - Warning: SMK version %c is not supported by the game.\n", hdr->SmkMarker[3]);
	}
	s->video.v = hdr->SmkMarker[3];
	s->video.w = smk_swap_le32(hdr->VideoWidth);
	s->video.h = smk_swap_le32(hdr->VideoHeight);
	s->total_frames = smk_swap_le32(hdr->FrameCount);
	/* frames per second calculation */
	temp_u = smk_swap_le32(hdr->FrameLen);
	/* frames per second calculation */
	temp_l = (int)temp_u;

	if (temp_l > 0) {
		/* millisec per frame */
		s->usf = temp_l * 1000;
	} else if (temp_l < 0) {
		/* 10 microsec per frame */
		s->usf = temp_l * -10;
	} else {
		/* defaults to 10 usf (= 100000 microseconds) */
		s->usf = 100000;
	}
	/* Video flags follow.
		Ring frame is important to libsmacker.
		Y scale / Y interlace go in the Video flags.
		The user should scale appropriately. */
	temp_u = smk_swap_le32(hdr->VideoFlags);
	if (temp_u & 0x01) {
		LogErrorMsg("libsmacker::smk_open_generic() - Warning: ring_frames are no supported by the game.\n");
		s->total_frames++;
	}

	/* Max buffer size for each audio track - used to pre-allocate buffers */
	for (temp_l = 0; temp_l < SMK_AUDIO_TRACKS; temp_l ++)
		s->audio[temp_l].max_buffer = smk_swap_le32(hdr->AudioMaxChunkLength[temp_l]);
	/* Read size of "hufftree chunk" - save for later. */
	tree_size = smk_swap_le32(hdr->VideoTreeDataSize);
	/* "unpacked" sizes of each huff tree */
	for (temp_l = 0; temp_l < SMK_TREE_COUNT; temp_l ++) {
		video_tree_size[temp_l] = smk_swap_le32(hdr->VideoTreeSize[temp_l]);
	}
	/* read audio rate data */
	for (temp_l = 0; temp_l < SMK_AUDIO_TRACKS; temp_l ++) {
		temp_u = smk_swap_le32(hdr->AudioType[temp_l]);

		if (temp_u & 0x40000000) {
			/* and for all audio tracks */
			smk_malloc(s->audio[temp_l].buffer, s->audio[temp_l].max_buffer);
#ifdef FULL
			if (temp_u & 0x80000000)
				s->audio[temp_l].compress = 1;
#else
			s->audio[temp_l].compress = ((temp_u & 0x80000000) ? 1 : 0);
#endif
			s->audio[temp_l].bitdepth = ((temp_u & 0x20000000) ? 16 : 8);
			s->audio[temp_l].channels = ((temp_u & 0x10000000) ? 2 : 1);

			if (temp_u & 0x0c000000) {
				LogError("libsmacker::smk_open_generic() - Warning: audio track %ld is compressed with Bink (perceptual) Audio Codec: this is currently unsupported by libsmacker\n", temp_l);
				s->audio[temp_l].compress = 2;
			}

			/* Bits 25 & 24 are unused. */
			s->audio[temp_l].rate = (temp_u & 0x00FFFFFF);
		}
	}
#endif // FULL
	/* FrameSizes and Keyframe marker are stored together. */
#ifdef FULL
	smk_malloc(s->keyframe, (s->f + s->ring_frame));

	smk_malloc(s->chunk_size, (s->f + s->ring_frame) * sizeof(unsigned long));

	for (temp_u = 0; temp_u < (s->f + s->ring_frame); temp_u ++) {
#else
	smk_mallocc(s->chunk_size, s->total_frames * sizeof(unsigned long), unsigned long);

	for (temp_u = 0; temp_u < s->total_frames; temp_u ++) {
#endif
		smk_read_ul(s->chunk_size[temp_u]);
// LogErrorFF("smk_open_generic 6 %d. %d", temp_u, s->chunk_size[temp_u]);
#ifdef FULL
		/* Set Keyframe */
		if (s->chunk_size[temp_u] & 0x01)
			s->keyframe[temp_u] = 1;
#endif
		/* Bits 1 is used, but the purpose is unknown. */
		s->chunk_size[temp_u] &= 0xFFFFFFFC;
	}

	/* That was easy... Now read FrameTypes! */
#ifdef FULL_ORIG
	smk_malloc(s->frame_type, (s->f + s->ring_frame));

	for (temp_u = 0; temp_u < (s->f + s->ring_frame); temp_u ++)
		smk_read(&s->frame_type[temp_u], 1);
#else
	smk_read_in(s->frame_type, s->total_frames);
#endif
	/* HuffmanTrees
		We know the sizes already: read and assemble into
		something actually parse-able at run-time */
	smk_mallocc(hufftree_chunk, tree_size, unsigned char);
	smk_read(hufftree_chunk, tree_size);
// LogErrorFF("smk_open_generic 7");
	/* set up a Bitstream */
	smk_bs_init(&bs, hufftree_chunk, tree_size);

	/* create some tables */
	for (temp_u = 0; temp_u < SMK_TREE_COUNT; temp_u ++) {
// LogErrorFF("smk_open_generic 8 %d: %d", temp_u, video_tree_size[temp_u]);
		// deepDebug = temp_u == SMK_TREE_FULL;
#ifdef FULL
		if (! smk_huff16_build(&s->video.tree[temp_u], &bs, s->video.tree_size[temp_u])) {
#else
		if (! smk_huff16_build(&s->video.tree[temp_u], &bs, video_tree_size[temp_u])) {
#endif
			LogError("libsmacker::smk_open_generic() - ERROR: failed to create huff16 tree %lu\n", temp_u);
			goto error;
		}
	}
// LogErrorFF("smk_open_generic 9");
	/* clean up */
	smk_free(hufftree_chunk);
	/* Go ahead and malloc storage for the video frame */
	smk_mallocc(s->video.frame, s->video.w * s->video.h, unsigned char);
#ifdef FULL
	/* final processing: depending on ProcessMode, handle what to do with rest of file data */
	s->mode = process_mode;

	/* Handle the rest of the data.
		For MODE_MEMORY, read the chunks and store */
	if (s->mode == SMK_MODE_MEMORY) {
		smk_malloc(s->source.chunk_data, (s->f + s->ring_frame) * sizeof(unsigned char *));
#else
	{
		smk_mallocc(s->source.chunk_data, s->total_frames * sizeof(unsigned char *), unsigned char*);
#endif

#ifdef FULL_ORIG
		for (temp_u = 0; temp_u < (s->f + s->ring_frame); temp_u ++) {
			smk_malloc(s->source.chunk_data[temp_u], s->chunk_size[temp_u]);
			smk_read(s->source.chunk_data[temp_u], s->chunk_size[temp_u]);
		}
#else
		for (temp_u = 0; temp_u < s->total_frames; temp_u ++) {
// LogErrorFF("smk_open_generic 10 %d. %d", temp_u, s->chunk_size[temp_u]);
			smk_read_in(s->source.chunk_data[temp_u], s->chunk_size[temp_u]);
		}
#endif
#ifdef FULL
	} else {
		/* MODE_STREAM: don't read anything now, just precompute offsets.
			use fseek to verify that the file is "complete" */
		smk_malloc(s->source.file.chunk_offset, (s->f + s->ring_frame) * sizeof(unsigned long));

		for (temp_u = 0; temp_u < (s->f + s->ring_frame); temp_u ++) {
			s->source.file.chunk_offset[temp_u] = ftell(fp.file);

			if (fseek(fp.file, s->chunk_size[temp_u], SEEK_CUR)) {
				LogError("libsmacker::smk_open_generic() - ERROR: fseek to frame %lu not OK.\n", temp_u);
				PrintError("\tError reported was");
				goto error;
			}
		}
#endif
	}

	return s;
error:
	smk_free(hufftree_chunk);
	smk_close(s);
	return NULL;
}

/* open an smk (from a memory buffer) */
smk smk_open_memory(const unsigned char * buffer, const unsigned long size)
{
	smk s = NULL;
	union smk_read_t fp;

#ifdef FULL
	if (buffer == NULL) {
		LogErrorMsg("libsmacker::smk_open_memory() - ERROR: buffer pointer is NULL\n");
		return NULL;
	}
#else
	assert(buffer);
#endif

	/* set up the read union for Memory mode */
	fp.ram = (unsigned char *)buffer;

#ifdef FULL
	if (!(s = smk_open_generic(0, fp, size, SMK_MODE_MEMORY))) {
#else
	if (!(s = smk_open_generic(fp, size))) {
#endif
		LogError("libsmacker::smk_open_memory(buffer,%lu) - ERROR: Fatal error in smk_open_generic, returning NULL.\n", size);
	}
	return s;
}

#ifdef FULL
/* open an smk (from a file) */
smk smk_open_filepointer(FILE * file, const unsigned char mode)
{
	smk s = NULL;
	union smk_read_t fp;

	if (file == NULL) {
		LogErrorMsg("libsmacker::smk_open_filepointer() - ERROR: file pointer is NULL\n");
		return NULL;
	}

	/* Copy file ptr to internal union */
	fp.file = file;

	if (!(s = smk_open_generic(1, fp, 0, mode))) {
		LogError("libsmacker::smk_open_filepointer(file,%u) - ERROR: Fatal error in smk_open_generic, returning NULL.\n", mode);
		fclose(fp.file);
		goto error;
	}

	if (mode == SMK_MODE_MEMORY)
		fclose(fp.file);
	else
		s->source.file.fp = fp.file;

	/* fall through, return s or null */
error:
	return s;
}

/* open an smk (from a file) */
smk smk_open_file(const char * filename, const unsigned char mode)
{
	FILE * fp;

	if (filename == NULL) {
		LogErrorMsg("libsmacker::smk_open_file() - ERROR: filename is NULL\n");
		return NULL;
	}

	if (!(fp = fopen(filename, "rb"))) {
		LogError("libsmacker::smk_open_file(%s,%u) - ERROR: could not open file\n", filename, mode);
		PrintError("\tError reported was");
		goto error;
	}

	/* kick processing to smk_open_filepointer */
	return smk_open_filepointer(fp, mode);
	/* fall through, return s or null */
error:
	return NULL;
}
#endif // FULL

/* close out an smk file and clean up memory */
void smk_close(smk s)
{
	unsigned long u;

	if (s == NULL) {
#ifdef FULL
		LogErrorMsg("libsmacker::smk_close() - ERROR: smk is NULL\n");
#endif
		return;
	}

	/* free video sub-components */
	for (u = 0; u < SMK_TREE_COUNT; u ++) {
#ifdef FULL
		if (s->video.tree[u].tree) free(s->video.tree[u].tree);
#else
		free(s->video.tree[u].tree);
#endif
	}

	smk_free(s->video.frame);

	/* free audio sub-components */
	for (u = 0; u < SMK_AUDIO_TRACKS; u++) {
#ifdef FULL
		if (s->audio[u].buffer)
			smk_free(s->audio[u].buffer);
#else
		free(s->audio[u].buffer);
#endif
	}

#ifdef FULL
	smk_free(s->keyframe);
#endif
#ifdef FULL_ORIG
	smk_free(s->frame_type);
#endif

#ifdef FULL
	if (s->mode == SMK_MODE_DISK) {
		/* disk-mode */
		if (s->source.file.fp)
			fclose(s->source.file.fp);

		smk_free(s->source.file.chunk_offset);
	} else {
#endif
		/* mem-mode */
#ifdef FULL_ORIG
		if (s->source.chunk_data != NULL) {
			for (u = 0; u < (s->f + s->ring_frame); u++)
				smk_free(s->source.chunk_data[u]);

			smk_free(s->source.chunk_data);
		}
#else
		free(s->source.chunk_data);
#endif
#ifdef FULL
	}
#endif
	smk_free(s->chunk_size);
	smk_free(s);
}

/* tell some info about the file */
#ifdef FULL
char smk_info_all(const smk object, unsigned long * frame, unsigned long * frame_count, double * usf)
{
	/* null check */
	if (object == NULL) {
		LogErrorMsg("libsmacker::smk_info_all() - ERROR: smk is NULL\n");
		return -1;
	}

	if (!frame && !frame_count && !usf) {
		LogErrorMsg("libsmacker::smk_info_all() - ERROR: Request for info with all-NULL return references\n");
		goto error;
	}

	if (frame)
		*frame = (object->cur_frame % object->f);

	if (frame_count)
		*frame_count = object->f;

	if (usf)
		*usf = object->usf;

	return 0;

error:
	return -1;
}

char smk_info_video(const smk object, unsigned long * w, unsigned long * h, unsigned char * y_scale_mode)
{
	/* null check */
	if (object == NULL) {
		LogErrorMsg("libsmacker::smk_info_video() - ERROR: smk is NULL\n");
		return -1;
	}

	if (!w && !h && !y_scale_mode) {
		LogErrorMsg("libsmacker::smk_info_video() - ERROR: Request for info with all-NULL return references\n");
		return -1;
	}

	if (w)
		*w = object->video.w;

	if (h)
		*h = object->video.h;

	if (y_scale_mode)
		*y_scale_mode = object->video.y_scale_mode;

	return 0;
}

char smk_info_audio(const smk object, unsigned char * track_mask, unsigned char channels[7], unsigned char bitdepth[7], unsigned long audio_rate[7])
{
	unsigned char i;

	/* null check */
	if (object == NULL) {
		LogErrorMsg("libsmacker::smk_info_audio() - ERROR: smk is NULL\n");
		return -1;
	}

	if (!track_mask && !channels && !bitdepth && !audio_rate) {
		LogErrorMsg("libsmacker::smk_info_audio() - ERROR: Request for info with all-NULL return references\n");
		return -1;
	}

	if (track_mask) {
		*track_mask = ((object->audio[0].exists) |
				((object->audio[1].exists) << 1) |
				((object->audio[2].exists) << 2) |
				((object->audio[3].exists) << 3) |
				((object->audio[4].exists) << 4) |
				((object->audio[5].exists) << 5) |
				((object->audio[6].exists) << 6));
	}

	if (channels) {
		for (i = 0; i < 7; i ++)
			channels[i] = object->audio[i].channels;
	}

	if (bitdepth) {
		for (i = 0; i < 7; i ++)
			bitdepth[i] = object->audio[i].bitdepth;
	}

	if (audio_rate) {
		for (i = 0; i < 7; i ++)
			audio_rate[i] = object->audio[i].rate;
	}

	return 0;
}
#else
char smk_info_all(const smk object, unsigned long * w, unsigned long * h, double * usf)
{
	assert(object);
	assert(w);
	assert(h);
	assert(usf);

	*w = object->video.w;
	*h = object->video.h;

	*usf = object->usf;

	return 0;
}

void smk_info_audio(const smk object, unsigned char * channels, unsigned char * bitdepth, unsigned long * audio_rate)
{
	*channels = object->audio[0].channels;
	*bitdepth = object->audio[0].bitdepth;
	*audio_rate = object->audio[0].rate;
}
#endif /* FULL */

/* Enable-disable switches */
#ifdef FULL
char smk_enable_all(smk object, const unsigned char mask)
{
	unsigned char i;

	/* null check */
	if (object == NULL) {
		LogErrorMsg("libsmacker::smk_enable_all() - ERROR: smk is NULL\n");
		return -1;
	}

	/* set video-enable */
	object->video.enable = (mask & 0x80);

	for (i = 0; i < 7; i ++) {
		if (object->audio[i].exists)
			object->audio[i].enable = (mask & (1 << i));
	}

	return 0;
}
#endif
char smk_enable_video(smk object, const unsigned char enable)
{
	/* null check */
#ifdef FULL
	if (object == NULL) {
		LogErrorMsg("libsmacker::smk_enable_video() - ERROR: smk is NULL\n");
		return -1;
	}
#else
	assert(object);
#endif

	object->video.enable = enable;
	return 0;
}

char smk_enable_audio(smk object, const unsigned char track, const unsigned char enable)
{
	/* null check */
#ifdef FULL
	if (object == NULL) {
		LogErrorMsg("libsmacker::smk_enable_audio() - ERROR: smk is NULL\n");
		return -1;
	}
#else
	assert(object);
#endif

	object->audio[track].enable = enable;
	return 0;
}

const unsigned char * smk_get_palette(const smk object)
{
	/* null check */
#ifdef FULL
	if (object == NULL) {
		LogErrorMsg("libsmacker::smk_get_palette() - ERROR: smk is NULL\n");
		return NULL;
	}
#else
	assert(object);
#endif

	return (unsigned char *)object->video.palette;
}
const unsigned char * smk_get_video(const smk object)
{
	/* null check */
#ifdef FULL
	if (object == NULL) {
		LogErrorMsg("libsmacker::smk_get_video() - ERROR: smk is NULL\n");
		return NULL;
	}
#else
	assert(object);
#endif

	return object->video.frame;
}
#ifdef FULL_ORIG
const unsigned char * smk_get_audio(const smk object, const unsigned char t)
#else
unsigned char * smk_get_audio(const smk object, const unsigned char t)
#endif
{
	/* null check */
#ifdef FULL
	if (object == NULL) {
		LogErrorMsg("libsmacker::smk_get_audio() - ERROR: smk is NULL\n");
		return NULL;
	}
#else
	assert(object);
#endif

	return (unsigned char *)object->audio[t].buffer;
}
unsigned long smk_get_audio_size(const smk object, const unsigned char t, unsigned char * compress)
{
	/* null check */
#ifdef FULL
	if (object == NULL) {
		LogErrorMsg("libsmacker::smk_get_audio_size() - ERROR: smk is NULL\n");
		return 0;
	}
#else
	assert(object);

	*compress = object->audio[t].compress;
#endif

	return object->audio[t].buffer_size;
}

const unsigned char palmap[64] = {
    0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C,
    0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
    0x41, 0x45, 0x49, 0x4D, 0x51, 0x55, 0x59, 0x5D,
    0x61, 0x65, 0x69, 0x6D, 0x71, 0x75, 0x79, 0x7D,
    0x82, 0x86, 0x8A, 0x8E, 0x92, 0x96, 0x9A, 0x9E,
    0xA2, 0xA6, 0xAA, 0xAE, 0xB2, 0xB6, 0xBA, 0xBE,
    0xC3, 0xC7, 0xCB, 0xCF, 0xD3, 0xD7, 0xDB, 0xDF,
    0xE3, 0xE7, 0xEB, 0xEF, 0xF3, 0xF7, 0xFB, 0xFF
};
/* Decompresses a palette-frame. */
static char smk_render_palette(struct smk_t::smk_video_t * s, unsigned char * p, unsigned long size)
{
	/* Index into palette */
	unsigned short i = 0;
	/* Helper variables */
	unsigned short count, src;
#ifdef FULL
	static unsigned char oldPalette[256][3];
#else
	static unsigned char oldPalette[256][4];
#endif
#ifdef FULL
	/* Smacker palette map: smk colors are 6-bit, this table expands them to 8. */
	const unsigned char palmap[64] = {
		0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C,
		0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
		0x41, 0x45, 0x49, 0x4D, 0x51, 0x55, 0x59, 0x5D,
		0x61, 0x65, 0x69, 0x6D, 0x71, 0x75, 0x79, 0x7D,
		0x82, 0x86, 0x8A, 0x8E, 0x92, 0x96, 0x9A, 0x9E,
		0xA2, 0xA6, 0xAA, 0xAE, 0xB2, 0xB6, 0xBA, 0xBE,
		0xC3, 0xC7, 0xCB, 0xCF, 0xD3, 0xD7, 0xDB, 0xDF,
		0xE3, 0xE7, 0xEB, 0xEF, 0xF3, 0xF7, 0xFB, 0xFF
	};
#endif
	/* null check */
	assert(s);
	assert(p);
	/* Copy palette to old palette */
#ifdef FULL
	memcpy(oldPalette, s->palette, 256 * 3);
#else
	memcpy(oldPalette, s->palette, 256 * 4);
#endif

	/* Loop until palette is complete, or we are out of bytes to process */
	while ((i < 256) && (size > 0)) {
		if ((*p) & 0x80) {
			/* 0x80: Skip block
				(preserve C+1 palette entries from previous palette) */
			count = ((*p) & 0x7F) + 1;
			p ++;
			size --;

			/* check for overflow condition */
			if (i + count > 256) {
				LogError("libsmacker::smk_render_palette() - ERROR: overflow, 0x80 attempt to skip %d entries from %d\n", count, i);
				goto error;
			}

			/* finally: advance the index. */
			i += count;
		} else if ((*p) & 0x40) {
			/* 0x40: Color-shift block
				Copy (c + 1) color entries of the previous palette,
				starting from entry (s),
				to the next entries of the new palette. */
			if (size < 2) {
				LogErrorMsg("libsmacker::smk_render_palette() - ERROR: 0x40 ran out of bytes for copy\n");
				goto error;
			}

			/* pick "count" items to copy */
			count = ((*p) & 0x3F) + 1;
			p ++;
			size --;
			/* start offset of old palette */
			src = *p;
			p ++;
			size --;

			/* overflow: see if we write/read beyond 256colors, or overwrite own palette */
			if (i + count > 256 || src + count > 256) { // condition fixed to prevent artifacts in the final video
				LogError("libsmacker::smk_render_palette() - ERROR: overflow, 0x40 attempt to copy %d entries from %d to %d\n", count, src, i);
				goto error;
			}

			/* OK!  Copy the color-palette entries. */
#ifdef FULL
			memmove(&s->palette[i][0], &oldPalette[src][0], count * 3);
#else
			memcpy(&s->palette[i][0], &oldPalette[src][0], count * 4);
#endif
			i += count;
		} else {
			/* 0x00: Set Color block
				Direct-set the next 3 bytes for palette index */
			if (size < 3) {
				LogError("libsmacker::smk_render_palette() - ERROR: 0x3F ran out of bytes for copy, size=%lu\n", size);
				goto error;
			}

			for (count = 0; count < 3; count ++) {
				if (*p > 0x3F) {
					LogError("libsmacker::smk_render_palette() - ERROR: palette index exceeds 0x3F (entry [%u][%u])\n", i, count);
					goto error;
				}

				s->palette[i][count] = palmap[*p];
				p++;
				size --;
			}
#ifndef FULL
			s->palette[i][3] = 1;
#endif

			i ++;
		}
	}

	if (i < 256) {
		LogError("libsmacker::smk_render_palette() - ERROR: did not completely fill palette (idx=%u)\n", i);
		goto error;
	}

	return 0;
error:
	/* Error, return -1
		The new palette probably has errors but is preferrable to a black screen */
	return -1;
}
static int frameCount = 0;
const unsigned short sizetable[64] = {
    1,	 2,	3,	4,	5,	6,	7,	8,
    9,	10,	11,	12,	13,	14,	15,	16,
    17,	18,	19,	20,	21,	22,	23,	24,
    25,	26,	27,	28,	29,	30,	31,	32,
    33,	34,	35,	36,	37,	38,	39,	40,
    41,	42,	43,	44,	45,	46,	47,	48,
    49,	50,	51,	52,	53,	54,	55,	56,
    57,	58,	59,	128,	256,	512,	1024,	2048
};
static char smk_render_video(struct smk_t::smk_video_t * s, unsigned char * p, unsigned int size, smk ss)
{
	unsigned char * t = s->frame;
	unsigned char s1, s2;
#ifdef FULL
	unsigned short temp;
#endif
	unsigned long i, j, k, row, col, skip;
	/* used for video decoding */
	struct smk_bit_t bs;
	/* results from a tree lookup */
	int unpack;
	/* unpack, broken into pieces */
	unsigned char type;
	unsigned char blocklen;
	unsigned char typedata;
	char bit;
#ifdef FULL
	const unsigned short sizetable[64] = {
		1,	 2,	3,	4,	5,	6,	7,	8,
		9,	10,	11,	12,	13,	14,	15,	16,
		17,	18,	19,	20,	21,	22,	23,	24,
		25,	26,	27,	28,	29,	30,	31,	32,
		33,	34,	35,	36,	37,	38,	39,	40,
		41,	42,	43,	44,	45,	46,	47,	48,
		49,	50,	51,	52,	53,	54,	55,	56,
		57,	58,	59,	128,	256,	512,	1024,	2048
	};
#endif
//bool doDebug = false; // frameCount == 174 || frameCount == 173;
bool doDebug = false; // frameCount == 174;
	/* null check */
	assert(s);
	assert(p);
	row = 0;
	col = 0;
	/* Set up a bitstream for video unpacking */
	smk_bs_init(&bs, p, size);

	/* Reset the cache on all bigtrees */
	for (i = 0; i < SMK_TREE_COUNT; i++)
		memset(&s->tree[i].cache, 0, 3 * sizeof(unsigned short));

	while (row < s->h) {
		if ((unpack = smk_huff16_lookup(&s->tree[SMK_TREE_TYPE], &bs)) < 0) {
			unsigned char * pp = ss->source.chunk_data[ss->cur_frame];
			LogError("libsmacker::smk_render_video() - ERROR: failed to lookup from TYPE tree (frame=%d, row=%d, chunk offset%d size%d [%d;%d;%d;%d;%d]).\n", ss->cur_frame, row, p - pp, ss->chunk_size[ss->cur_frame], pp[0], pp[1], pp[2], pp[3], pp[4]);
			return -1;
		}

		type = ((unpack & 0x0003));
		blocklen = ((unpack & 0x00FC) >> 2);
		typedata = ((unpack & 0xFF00) >> 8);
/*if (ss->cur_frame == 0) {
			unsigned char * pp = ss->source.chunk_data[ss->cur_frame];
LogError("libsmacker::smk_render_video() - INFO: lookup from TYPE tree %d (ty%d len %d=%d data%d) bn%d (frame=%d, row=%d, chunk offset%d size%d [%d;%d;%d;%d;%d]).\n", unpack, type, blocklen, sizetable[blocklen], typedata, bs.bit_num, ss->cur_frame, row, p - pp, ss->chunk_size[ss->cur_frame], pp[0], pp[1], pp[2], pp[3], pp[4]);
}*/

		/* support for v4 full-blocks */
		if (type == 1 && s->v == '4') {
			if ((bit = smk_bs_read_1(&bs)) < 0) {
				LogErrorMsg("libsmacker::smk_render_video() - ERROR: first subtype of v4 returned -1\n");
				return -1;
			}

			if (bit)
				type = 4;
			else {
				if ((bit = smk_bs_read_1(&bs)) < 0) {
					LogErrorMsg("libsmacker::smk_render_video() - ERROR: second subtype of v4 returned -1\n");
					return -1;
				}

				if (bit)
					type = 5;
			}
		}

unsigned firstRow = row;
unsigned firstCol = col;
		for (j = 0; (j < sizetable[blocklen]) && (row < s->h); j ++) {
//if (doDebug && row >= 136 && row < 140 /*&& col >= 28 && col <= 116*/)
//	LogErrorFF("smk_render_video row%d col%d type%d blocklen%d data%d firstRow%d Col%d offset%d=%x (%d=%x) bit%d", row, col, type, blocklen, typedata, firstRow, firstCol, (size_t)bs.buffer - (size_t)p, (size_t)bs.buffer - (size_t)p, (size_t)bs.buffer - (size_t)bufMem, (size_t)bs.buffer - (size_t)bufMem, bs.bit_num);

			skip = (row * s->w) + col;

			switch (type) {
			case 0: /* 2COLOR BLOCK */
				if ((unpack = smk_huff16_lookup(&s->tree[SMK_TREE_MCLR], &bs)) < 0) {
					LogErrorMsg("libsmacker::smk_render_video() - ERROR: failed to lookup from MCLR tree.\n");
					return -1;
				}

				s1 = (unpack & 0xFF00) >> 8;
				s2 = (unpack & 0x00FF);

				if ((unpack = smk_huff16_lookup(&s->tree[SMK_TREE_MMAP], &bs)) < 0) {
					LogErrorMsg("libsmacker::smk_render_video() - ERROR: failed to lookup from MMAP tree.\n");
					return -1;
				}
#ifdef FULL
				temp = 0x01;
#endif
				for (k = 0; k < 4; k ++) {
					for (i = 0; i < 4; i ++) {
#ifdef FULL
						if (unpack & temp)
							t[skip + i] = s1;
						else
							t[skip + i] = s2;

						temp = temp << 1;
#else
						if (unpack & 0x01)
							t[skip + i] = s1;
						else
							t[skip + i] = s2;

						unpack = (unsigned)unpack >> 1;
#endif
					}

					skip += s->w;
				}

				break;

			case 1: /* FULL BLOCK */
{
deepDebug = doDebug && row >= 136 && row < 140 && col >= 28 && col <= 116;
				for (k = 0; k < 4; k ++) {
					if ((unpack = smk_huff16_lookup(&s->tree[SMK_TREE_FULL], &bs)) < 0) {
						LogErrorMsg("libsmacker::smk_render_video() - ERROR: failed to lookup from FULL tree.\n");
						return -1;
					}

#ifdef FULL
					t[skip + 3] = ((unpack & 0xFF00) >> 8);
					t[skip + 2] = (unpack & 0x00FF);
#else
					*(uint16_t*)&t[skip + 2] = smk_swap_le16(unpack);
#endif
if (deepDebug)
LogErrorFF("Full block %d:0 value%d (offsetend%d bitend%d) %d,%d:%d (%d:%d) = %d", k, unpack, (size_t)bs.buffer - (size_t)bufMem, bs.bit_num, col + 2, col + 3, row + k, t[skip + 2], t[skip + 3], *(uint16_t*)&t[skip + 2]);
					if ((unpack = smk_huff16_lookup(&s->tree[SMK_TREE_FULL], &bs)) < 0) {
						LogErrorMsg("libsmacker::smk_render_video() - ERROR: failed to lookup from FULL tree.\n");
						return -1;
					}
#ifdef FULL
					t[skip + 1] = ((unpack & 0xFF00) >> 8);
					t[skip] = (unpack & 0x00FF);
#else
					*(uint16_t*)&t[skip] = smk_swap_le16(unpack);
#endif
if (deepDebug)
LogErrorFF("Full block %d:1 value%d (offsetend%d bitend%d) %d,%d:%d (%d:%d) = %d", k, unpack, (size_t)bs.buffer - (size_t)bufMem, bs.bit_num, col, col + 1, row + k, t[skip + 0], t[skip + 1], *(uint16_t*)&t[skip + 0]);
					skip += s->w;
				}
deepDebug = false;
}
				break;

			case 2: /* VOID BLOCK */
				/* break;
				if (s->frame)
				{
					memcpy(&t[skip], &s->frame[skip], 4);
					skip += s->w;
					memcpy(&t[skip], &s->frame[skip], 4);
					skip += s->w;
					memcpy(&t[skip], &s->frame[skip], 4);
					skip += s->w;
					memcpy(&t[skip], &s->frame[skip], 4);
				} */
				break;

			case 3: /* SOLID BLOCK */ {
#ifdef FULL
				memset(&t[skip], typedata, 4);
				skip += s->w;
				memset(&t[skip], typedata, 4);
				skip += s->w;
				memset(&t[skip], typedata, 4);
				skip += s->w;
				memset(&t[skip], typedata, 4);
#else
				uint32_t value;
				memset(&value, typedata, 4);
				*(uint32_t*)&t[skip] = value;
				skip += s->w;
				*(uint32_t*)&t[skip] = value;
				skip += s->w;
				*(uint32_t*)&t[skip] = value;
				skip += s->w;
				*(uint32_t*)&t[skip] = value;
#endif
			} break;

			case 4: /* V4 DOUBLE BLOCK */
				for (k = 0; k < 2; k ++) {
					if ((unpack = smk_huff16_lookup(&s->tree[SMK_TREE_FULL], &bs)) < 0) {
						LogErrorMsg("libsmacker::smk_render_video() - ERROR: failed to lookup from FULL tree.\n");
						return -1;
					}

					for (i = 0; i < 2; i ++) {
						memset(&t[skip + 2], (unpack & 0xFF00) >> 8, 2);
						memset(&t[skip], (unpack & 0x00FF), 2);
						skip += s->w;
					}
				}

				break;

			case 5: /* V4 HALF BLOCK */
				for (k = 0; k < 2; k ++) {
					if ((unpack = smk_huff16_lookup(&s->tree[SMK_TREE_FULL], &bs)) < 0) {
						LogErrorMsg("libsmacker::smk_render_video() - ERROR: failed to lookup from FULL tree.\n");
						return -1;
					}

					t[skip + 3] = ((unpack & 0xFF00) >> 8);
					t[skip + 2] = (unpack & 0x00FF);
					t[skip + s->w + 3] = ((unpack & 0xFF00) >> 8);
					t[skip + s->w + 2] = (unpack & 0x00FF);

					if ((unpack = smk_huff16_lookup(&s->tree[SMK_TREE_FULL], &bs)) < 0) {
						LogErrorMsg("libsmacker::smk_render_video() - ERROR: failed to lookup from FULL tree.\n");
						return -1;
					}

					t[skip + 1] = ((unpack & 0xFF00) >> 8);
					t[skip] = (unpack & 0x00FF);
					t[skip + s->w + 1] = ((unpack & 0xFF00) >> 8);
					t[skip + s->w] = (unpack & 0x00FF);
					skip += (s->w << 1);
				}

				break;

			}

			col += 4;

			if (col >= s->w) {
				col = 0;
				row += 4;
			}
		}
	}

	return 0;
}

/* Decompress audio track i. */
static char smk_render_audio(struct smk_t::smk_audio_t * s, unsigned char * p, unsigned long size)
{
	struct smk_bit_t bs;
	char bit;
#ifdef FULL
	unsigned int j, k;
	unsigned char * t = s->buffer;
	short unpack, unpack2;
#else
	uint8_t *end_buf, *cur_buf;
	unsigned offset;
	uint8_t bitdepth, channels;
	uint8_t unpack, unpack2;
#endif
	/* used for audio decoding */
	struct smk_huff8_t aud_tree[4];
	/* null check */
	assert(s);
	assert(p);

	if (!s->compress) {
		/* Raw PCM data, update buffer size and perform copy */
		s->buffer_size = size;
		memcpy(s->buffer, p, size);
	} else if (s->compress == 1) {
		/* SMACKER DPCM compression */
		/* need at least 4 bytes to process */
		if (size < 4) {
			LogErrorMsg("libsmacker::smk_render_audio() - ERROR: need 4 bytes to get unpacked output buffer size.\n");
			goto error;
		}

		/* chunk is compressed (huff-compressed dpcm), retrieve unpacked buffer size */
#ifdef FULL
		s->buffer_size = ((unsigned int) p[3] << 24) |
			((unsigned int) p[2] << 16) |
			((unsigned int) p[1] << 8) |
			((unsigned int) p[0]);
#else
		s->buffer_size = smk_swap_le32(*(uint32_t*)&p[0]);
#endif
		p += 4;
		size -= 4;
		/* Compressed audio: must unpack here */
		/*  Set up a bitstream */
		smk_bs_init(&bs, p, size);
		if ((bit = smk_bs_read_1(&bs)) < 0) {
			LogErrorMsg("libsmacker::smk_render_audio() - ERROR: initial get_bit returned -1\n");
			goto error;
		}
#ifdef FULL
		if (!bit) {
			LogErrorMsg("libsmacker::smk_render_audio() - ERROR: initial get_bit returned 0\n");
			goto error;
		}

		bit = smk_bs_read_1(&bs);

		if (s->channels != (bit == 1 ? 2 : 1))
			LogErrorMsg("libsmacker::smk_render_audio() - ERROR: mono/stereo mismatch\n");

		bit = smk_bs_read_1(&bs);

		if (s->bitdepth != (bit == 1 ? 16 : 8))
			LogErrorMsg("libsmacker::smk_render_audio() - ERROR: 8-/16-bit mismatch\n");

		/* build the trees */
		smk_huff8_build(&aud_tree[0], &bs);
		j = 1;
		k = 1;

		if (s->bitdepth == 16) {
			smk_huff8_build(&aud_tree[1], &bs);
			k = 2;
		}

		if (s->channels == 2) {
			smk_huff8_build(&aud_tree[2], &bs);
			j = 2;
			k = 2;

			if (s->bitdepth == 16) {
				smk_huff8_build(&aud_tree[3], &bs);
				k = 4;
			}
		}

		/* read initial sound level */
		if (s->channels == 2) {
			unpack = smk_bs_read_8(&bs);

			if (s->bitdepth == 16) {
				((short *)t)[1] = smk_bs_read_8(&bs);
				((short *)t)[1] |= (unpack << 8);
			} else
				((unsigned char *)t)[1] = (unsigned char)unpack;
		}

		unpack = smk_bs_read_8(&bs);

		if (s->bitdepth == 16) {
			((short *)t)[0] = smk_bs_read_8(&bs);
			((short *)t)[0] |= (unpack << 8);
		} else
			((unsigned char *)t)[0] = (unsigned char)unpack;

		/* All set: let's read some DATA! */
		while (k < s->buffer_size) {
			if (s->bitdepth == 8) {
				unpack = smk_huff8_lookup(&aud_tree[0], &bs);
				((unsigned char *)t)[j] = (char)unpack + ((unsigned char *)t)[j - s->channels];
				j ++;
				k++;
			} else {
				unpack = smk_huff8_lookup(&aud_tree[0], &bs);
				unpack2 = smk_huff8_lookup(&aud_tree[1], &bs);
				((short *)t)[j] = (short)(unpack | (unpack2 << 8)) + ((short *)t)[j - s->channels];
				j ++;
				k += 2;
			}

			if (s->channels == 2) {
				if (s->bitdepth == 8) {
					unpack = smk_huff8_lookup(&aud_tree[2], &bs);
					((unsigned char *)t)[j] = (char)unpack + ((unsigned char *)t)[j - 2];
					j ++;
					k++;
				} else {
					unpack = smk_huff8_lookup(&aud_tree[2], &bs);
					unpack2 = smk_huff8_lookup(&aud_tree[3], &bs);
					((short *)t)[j] = (short)(unpack | (unpack2 << 8)) + ((short *)t)[j - 2];
					j ++;
					k += 2;
				}
			}
		}
#else
		if (!bit) {
			LogErrorMsg("libsmacker::smk_render_audio() - ERROR: initial get_bit returned 0\n");
		}
		if ((bit = smk_bs_read_1(&bs)) < 0) {
			LogErrorMsg("libsmacker::smk_render_audio() - ERROR: channels_bit returned -1\n");
			goto error;
		}
		bitdepth = s->bitdepth;
		channels = s->channels;
		if (channels != (bit == 1 ? 2 : 1)) {
			LogErrorMsg("libsmacker::smk_render_audio() - ERROR: mono/stereo mismatch\n");
		}
		if ((bit = smk_bs_read_1(&bs)) < 0) {
			LogErrorMsg("libsmacker::smk_render_audio() - ERROR: bitdepth_bit returned -1\n");
			goto error;
		}

		if (bitdepth != (bit == 1 ? 16 : 8)) {
			LogErrorMsg("libsmacker::smk_render_audio() - ERROR: 8-/16-bit mismatch\n");
		}
		/* build the trees */
		smk_huff8_build(&aud_tree[0], &bs);

		if (bitdepth == 16) {
			smk_huff8_build(&aud_tree[1], &bs);
		}

		if (channels == 2) {
			smk_huff8_build(&aud_tree[2], &bs);

			if (bitdepth == 16) {
				smk_huff8_build(&aud_tree[3], &bs);
			}
		}

		cur_buf = (uint8_t*)s->buffer;
		/* read initial sound level */
bool doDebug = false; // frameCount >= 173 && frameCount <= 175;
		offset = 1;
		if (channels == 2) {
			offset = 2;

			unpack = smk_bs_read_8(&bs);
			if (bitdepth == 16) {
				unpack2 = smk_bs_read_8(&bs);
				((uint16_t *)cur_buf)[1] = unpack2 | ((uint16_t)unpack << 8);
if (doDebug)
LogErrorFF("Audio %d 16bit2ch %d,%d (%d,%d)", frameCount, unpack2, unpack, cur_buf[1], cur_buf[2]);
			} else {
				((uint8_t *)cur_buf)[1] = unpack;
if (doDebug)
LogErrorFF("Audio %d 8bit2ch %d", frameCount, unpack);
			}
		}

		unpack = smk_bs_read_8(&bs);
		if (bitdepth == 16) {
			offset *= 2;

			unpack2 = smk_bs_read_8(&bs);
			((uint16_t *)cur_buf)[0] = unpack2 | ((uint16_t)unpack << 8);
if (doDebug)
LogErrorFF("Audio %d 16bit1ch %d,%d (%d,%d)", frameCount, unpack2, unpack, cur_buf[0], cur_buf[1]);
		} else {
			((uint8_t *)cur_buf)[0] = unpack;
if (doDebug)
LogErrorFF("Audio %d 8bit1ch %d", frameCount, unpack);
		}

		cur_buf += offset;
uint8_t* bufStart = cur_buf;
		/* All set: let's read some DATA! */
		end_buf = cur_buf - offset + s->buffer_size;
		while (cur_buf < end_buf) {
			unpack = smk_huff8_lookup(&aud_tree[0], &bs);
			if (bitdepth == 16) {
				unpack2 = smk_huff8_lookup(&aud_tree[1], &bs);
				*(int16_t *)cur_buf = (unpack | ((int16_t)unpack2 << 8)) + ((int16_t *)cur_buf)[ - channels];
				cur_buf += 2;
			} else {
				*(uint8_t *)cur_buf = (int8_t)unpack + ((uint8_t *)cur_buf)[ - channels];
				cur_buf++;
			}

			if (channels == 2) {
				unpack = smk_huff8_lookup(&aud_tree[2], &bs);
				if (bitdepth == 16) {
					unpack2 = smk_huff8_lookup(&aud_tree[3], &bs);
					*(int16_t *)cur_buf = (unpack | ((int16_t)unpack2 << 8)) + ((int16_t *)cur_buf)[ - 2];
					cur_buf += 2;
				} else {
					*(uint8_t *)cur_buf = (int8_t)unpack + ((uint8_t *)cur_buf)[ - 2];
					cur_buf++;
				}
			}
		}
// bool doDebug = frameCount >= 173 && frameCount <= 175;
if (doDebug) {
	char tmp[256];
	snprintf(tmp, sizeof(tmp), "f:\\logdebug%d_%d.txt", 0, frameCount);
	FILE* f0 = fopen(tmp, "a+");
	if (f0 == NULL)
		return 0;

	fwrite(bufStart, 1, (size_t)cur_buf - (size_t)bufStart, f0);

	fclose(f0);
}
#endif // FULL
	}

	return 0;
error:
	return -1;
}

/* "Renders" (unpacks) the frame at cur_frame
	Preps all the image and audio pointers */
static char smk_render(smk s)
{
	unsigned long i, size;
	unsigned char * buffer = NULL, * p, track;
	/* null check */
	assert(s);

	/* Retrieve current chunk_size for this frame. */
	if (!(i = s->chunk_size[s->cur_frame])) {
		LogError("libsmacker::smk_render() - Warning: frame %lu: chunk_size is 0.\n", s->cur_frame);
		goto error;
	}
	++frameCount;
#ifdef FULL
	if (s->mode == SMK_MODE_DISK) {
		/* Skip to frame in file */
		if (fseek(s->source.file.fp, s->source.file.chunk_offset[s->cur_frame], SEEK_SET)) {
			LogError("libsmacker::smk_render() - ERROR: fseek to frame %lu (offset %lu) failed.\n", s->cur_frame, s->source.file.chunk_offset[s->cur_frame]);
			PrintError("\tError reported was");
			goto error;
		}

		/* In disk-streaming mode: make way for our incoming chunk buffer */
		if ((buffer = malloc(i)) == NULL) {
			PrintError("libsmacker::smk_render() - ERROR: failed to malloc() buffer");
			return -1;
		}

		/* Read into buffer */
		if (smk_read_file(buffer, i/*s->chunk_size[s->cur_frame]*/, s->source.file.fp) < 0) {
			LogError("libsmacker::smk_render() - ERROR: frame %lu (offset %lu): smk_read had errors.\n", s->cur_frame, s->source.file.chunk_offset[s->cur_frame]);
			goto error;
		}
	} else {
#endif
		/* Just point buffer at the right place */
		if (!s->source.chunk_data[s->cur_frame]) {
			LogError("libsmacker::smk_render() - ERROR: frame %lu: memory chunk is a NULL pointer.\n", s->cur_frame);
			goto error;
		}

		buffer = s->source.chunk_data[s->cur_frame];
#ifdef FULL
	}
#endif

	p = buffer;

	/* Palette record first */
	if (s->frame_type[s->cur_frame] & 0x01) {
		/* need at least 1 byte to process */
		assert(i != 0);

		/* Byte 1 in block, times 4, tells how many
			subsequent bytes are present */
		size = 4 * (*p);

		if (i < size) {
			LogError("libsmacker::smk_render() - ERROR: frame %lu: insufficient data for a palette content.\n", s->cur_frame);
			goto error;
		}

		/* If video rendering enabled, kick this off for decode. */
		if (s->video.enable) {
			if (size < 1) {
				LogError("libsmacker::smk_render() - ERROR: frame %lu: invalid palette size.\n", s->cur_frame);
				goto error;
			}

			smk_render_palette(&(s->video), p + 1, size - 1);
		}

		p += size;
		i -= size;
	}

	/* Unpack audio chunks */
	for (track = 0; track < SMK_AUDIO_TRACKS; track ++) {
		if (s->frame_type[s->cur_frame] & (0x02 << track)) {
			/* need at least 4 byte to process */
			if (i < 4) {
				LogError("libsmacker::smk_render() - ERROR: frame %lu: insufficient data for audio[%u] rec.\n", s->cur_frame, track);
				goto error;
			}

			/* First 4 bytes in block tell how many
				subsequent bytes are present */
#ifdef FULL
			size = (((unsigned int) p[3] << 24) |
					((unsigned int) p[2] << 16) |
					((unsigned int) p[1] << 8) |
					((unsigned int) p[0]));
#else
			size = smk_swap_le32(*(uint32_t*)&p[0]);
#endif
			if (i < size) {
				LogError("libsmacker::smk_render() - ERROR: frame %lu: insufficient data for audio[%u] content.\n", s->cur_frame, track);
				goto error;
			}

			/* If audio rendering enabled, kick this off for decode. */
			if (s->audio[track].enable) {
				if (size < 4) {
					LogError("libsmacker::smk_render() - ERROR: frame %lu: invalid data size for audio[%u] content.\n", s->cur_frame, track);
					goto error;
				}

				smk_render_audio(&s->audio[track], p + 4, size - 4);
			}

			p += size;
			i -= size;
		} else
			s->audio[track].buffer_size = 0;
	}
//if (frameCount == 173 || frameCount == 174)
//LogErrorFF("smk_render frame %d from %d", frameCount, (size_t)p - (size_t)bufMem);
	/* Unpack video chunk */
	if (s->video.enable) {
		if (smk_render_video(&(s->video), p, i, s) < 0) {
			LogError("libsmacker::smk_render() - ERROR: frame %lu: failed to render video.\n", s->cur_frame);
			goto error;
		}
	}

#ifdef FULL
	if (s->mode == SMK_MODE_DISK) {
		/* Remember that buffer we allocated?  Trash it */
		smk_free(buffer);
	}
#endif

	return 0;
error:

#ifdef FULL
	if (s->mode == SMK_MODE_DISK) {
		/* Remember that buffer we allocated?  Trash it */
		smk_free(buffer);
	}
#endif

	return -1;
}

/* rewind to first frame and unpack */
char smk_first(smk s)
{
#ifdef FULL
	/* null check */
	if (s == NULL) {
		LogErrorMsg("libsmacker::smk_first() - ERROR: smk is NULL\n");
		return -1;
	}

	s->cur_frame = 0;

	if (smk_render(s) < 0) {
		LogError("libsmacker::smk_first(s) - Warning: frame %lu: smk_render returned errors.\n", s->cur_frame);
		return -1;
	}

	if (s->f == 1) return SMK_LAST;

	return SMK_MORE;
#else
	char result = SMK_MORE;

	assert(s);

	s->cur_frame = 0;

	if (smk_render(s) < 0) {
		result = SMK_ERROR;
	}

	return result;
#endif
}

/* advance to next frame */
char smk_next(smk s)
{
#ifdef FULL
	/* null check */
	if (s == NULL) {
		LogErrorMsg("libsmacker::smk_next() - ERROR: smk is NULL\n");
		return -1;
	}

	if (s->cur_frame + 1 < (s->f + s->ring_frame)) {
		s->cur_frame ++;

		if (smk_render(s) < 0) {
			LogError("libsmacker::smk_next(s) - Warning: frame %lu: smk_render returned errors.\n", s->cur_frame);
			return -1;
		}

		if (s->cur_frame + 1 == (s->f + s->ring_frame))
			return SMK_LAST;

		return SMK_MORE;
	} else if (s->ring_frame) {
		s->cur_frame = 1;

		if (smk_render(s) < 0) {
			LogError("libsmacker::smk_next(s) - Warning: frame %lu: smk_render returned errors.\n", s->cur_frame);
			return -1;
		}

		if (s->cur_frame + 1 == (s->f + s->ring_frame))
			return SMK_LAST;

		return SMK_MORE;
	}

	return SMK_DONE;
#else
	char result = SMK_DONE;

	assert(s);

	if (s->cur_frame + 1 < s->total_frames) {
		s->cur_frame ++;

		result = SMK_MORE;
		if (smk_render(s) < 0) {
			result = SMK_ERROR;
		}
	}

	return result;
#endif
}

/* seek to a keyframe in an smk */
#ifdef FULL
char smk_seek_keyframe(smk s, unsigned long f)
{
	/* null check */
	if (s == NULL) {
		LogErrorMsg("libsmacker::smk_seek_keyframe() - ERROR: smk is NULL\n");
		return -1;
	}

	/* rewind (or fast forward!) exactly to f */
	s->cur_frame = f;

	/* roll back to previous keyframe in stream, or 0 if no keyframes exist */
	while (s->cur_frame > 0 && !(s->keyframe[s->cur_frame]))
		s->cur_frame --;

	/* render the frame: we're ready */
	if (smk_render(s) < 0) {
		LogError("libsmacker::smk_seek_keyframe(s,%lu) - Warning: frame %lu: smk_render returned errors.\n", f, s->cur_frame);
		return -1;
	}

	return 0;
}
#endif
unsigned char smk_palette_updated(smk s)
{
	return s->frame_type[s->cur_frame] & 0x01;
}
