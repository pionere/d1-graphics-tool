/**
 * @file engine.cpp
 *
 * Implementation of basic engine helper functions:
 * - Sprite blitting
 * - Drawing
 * - Angle calculation
 * - RNG
 * - Memory allocation
 * - File loading
 * - Video playback
 */
#include "all.h"

#include <QApplication>
#include <QDir>
#include <QFile>

#include "../progressdialog.h"

DEVILUTION_BEGIN_NAMESPACE

/** Current game seed */
int32_t sglGameSeed;

/**
 * Specifies the increment used in the Borland C/C++ pseudo-random.
 */
static const uint32_t RndInc = 1;

/**
 * Specifies the multiplier used in the Borland C/C++ pseudo-random number generator algorithm.
 */
static const uint32_t RndMult = 0x015A4E35;

/**
 * @brief Calculate the best fit direction between two points
 * @param x1 Tile coordinate
 * @param y1 Tile coordinate
 * @param x2 Tile coordinate
 * @param y2 Tile coordinate
 * @return A value from the direction enum
 */
int GetDirection(int x1, int y1, int x2, int y2)
{
#if 0
#if UNOPTIMIZED_DIRECTION
	int mx, my;
	int md;

	mx = x2 - x1;
	my = y2 - y1;

	if (mx >= 0) {
		if (my >= 0) {
			md = DIR_S;
			if (2 * mx < my)
				return DIR_SW;
		} else {
			my = -my;
			md = DIR_E;
			if (2 * mx < my)
				return DIR_NE;
		}
		if (2 * my < mx)
			md = DIR_SE;
	} else {
		mx = -mx;
		if (my >= 0) {
			md = DIR_W;
			if (2 * mx < my)
				return DIR_SW;
		} else {
			my = -my;
			md = DIR_N;
			if (2 * mx < my)
				return DIR_NE;
		}
		if (2 * my < mx)
			md = DIR_NW;
	}

	return md;
#else
	int dx = x2 - x1;
	int dy = y2 - y1;
	unsigned adx = abs(dx);
	unsigned ady = abs(dy);
	//                        SE  NE  SW  NW
	const int BaseDirs[4] = {  7,  5,  1,  3 };
	int dir = BaseDirs[2 * (dx < 0) + (dy < 0)];
	//const int DeltaDir[2][4] = { { 0, 1, 2 }, { 2, 1, 0 } };
	const int DeltaDirs[2][4] = { { 1, 0, 2 }, { 1, 2, 0 } };
	const int(&DeltaDir)[4] = DeltaDirs[(dx < 0) ^ (dy < 0)];
	//dir += DeltaDir[2 * adx < ady ? 2 : (2 * ady < adx ? 0 : 1)];
	dir += DeltaDir[2 * adx < ady ? 2 : (2 * ady < adx ? 1 : 0)];
	return dir & 7;
#endif
#else
#if UNOPTIMIZED_DIRECTION
	int mx, my, md;

	mx = x2 - x1;
	my = y2 - y1;
	if (mx >= 0) {
		if (my >= 0) {
			if (5 * mx <= (my << 1)) // mx/my <= 0.4, approximation of tan(22.5)
				return 1;            // DIR_SW
			md = 0;                  // DIR_S
		} else {
			my = -my;
			if (5 * mx <= (my << 1))
				return 5; // DIR_NE
			md = 6;       // DIR_E
		}
		if (5 * my <= (mx << 1)) // my/mx <= 0.4
			md = 7;              // DIR_SE
	} else {
		mx = -mx;
		if (my >= 0) {
			if (5 * mx <= (my << 1))
				return 1; // DIR_SW
			md = 2;       // DIR_W
		} else {
			my = -my;
			if (5 * mx <= (my << 1))
				return 5; // DIR_NE
			md = 4;       // DIR_N
		}
		if (5 * my <= (mx << 1))
			md = 3; // DIR_NW
	}
	return md;
#else
	int dx = x2 - x1;
	int dy = y2 - y1;
	unsigned adx = abs(dx);
	unsigned ady = abs(dy);
	//                        SE  NE  SW  NW
	const int BaseDirs[4] = {  7,  5,  1,  3 };
	int dir = BaseDirs[2 * (dx < 0) + (dy < 0)];
	//const int DeltaDirs[2][4] = { {0, 1, 2}, {2, 1, 0} };
	const int DeltaDirs[2][4] = { { 1, 0, 2 }, { 1, 2, 0 } };
	const int(&DeltaDir)[4] = DeltaDirs[(dx < 0) ^ (dy < 0)];
	//dir += DeltaDir[5 * adx <= (ady << 1) ? 2 : (5 * ady <= (adx << 1) ? 0 : 1)];
	dir += DeltaDir[5 * adx <= (ady << 1) ? 2 : (5 * ady <= (adx << 1) ? 1 : 0)];
	return dir & 7;
#endif
#endif
}

/**
 * @brief Set the RNG seed
 * @param s RNG seed
 */
void SetRndSeed(int32_t s)
{
	sglGameSeed = s;
}

/**
 * @bried Return the current RNG seed
 * @return RNG seed
 */
int32_t GetRndSeed()
{
	return sglGameSeed;
}

/**
 * @brief Get the next RNG seed
 * @return RNG seed
 */
int32_t NextRndSeed()
{
	sglGameSeed = RndMult * static_cast<uint32_t>(sglGameSeed) + RndInc;
	return sglGameSeed;
}

static unsigned NextRndValue()
{
	return abs(NextRndSeed());
}

/**
 * @brief Main RNG function
 * @param idx Unused
 * @param v The upper limit for the return value
 * @return A random number from 0 to (v-1)
 */
int random_(BYTE idx, int v)
{
	if (v <= 0)
		return 0;
	if (v < 0x7FFF)
		return (NextRndValue() >> 16) % v;
	return NextRndValue() % v;
}

/**
 * @brief Same as random_ but assumes 0 < v < 0x7FFF
 * @param idx Unused
 * @param v The upper limit for the return value
 * @return A random number from 0 to (v-1)
 */
int random_low(BYTE idx, int v)
{
	// assert(v > 0);
	// assert(v < 0x7FFF);
	return (NextRndValue() >> 16) % v;
}

/**
 * @brief Multithreaded safe malloc
 * @param dwBytes Byte size to allocate
 */
static BYTE* DiabloAllocPtr(size_t dwBytes)
{
	BYTE* buf;
	buf = (BYTE*)malloc(dwBytes);

//	if (buf == NULL)
//		app_fatal("Out of memory");

	return buf;
}

/**
 * @brief Multithreaded safe memfree
 * @param p Memory pointer to free
 */
void mem_free_dbg(void* p)
{
	if (p != NULL) {
		free(p);
	}
}

/**
 * @brief Load a file in to a buffer
 * @param pszName Path of file
 * @param pdwFileLen Will be set to file size if non-NULL
 * @return Buffer with content of file
 */
BYTE* LoadFileInMem(const char* pszName, size_t* pdwFileLen)
{
	QString path = assetPath + "/" + pszName;
	QFile file = QFile(path);
	BYTE* buf = NULL;

	if (!file.open(QIODevice::ReadOnly)) {
		dProgressErr() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(path));
		return buf;
    }

	const QByteArray fileData = file.readAll();

	unsigned fileLen = fileData.size();
	if (pdwFileLen != NULL) {
		*pdwFileLen = fileLen;
    }

	if (fileLen != 0) {
		buf = (BYTE*)DiabloAllocPtr(fileLen);
		memcpy(buf, fileData.constData(), fileLen);
	}

	return buf;
}

/**
 * @brief Load a file in to the given buffer
 * @param pszName Path of file
 * @param p Target buffer
 */
void LoadFileWithMem(const char* pszName, BYTE* p)
{
	QString path = assetPath + "/" + pszName;

	if (p == NULL) {
		dProgressErr() << QApplication::tr("Skipping file: %1.").arg(QDir::toNativeSeparators(path));
		return;
	}
	QFile file = QFile(path);

	if (!file.open(QIODevice::ReadOnly)) {
		dProgressErr() << QApplication::tr("Failed to open file: %1.").arg(QDir::toNativeSeparators(path));
		return;
	}

	const QByteArray fileData = file.readAll();

	unsigned fileLen = fileData.size();
	if (fileLen != 0) {
		memcpy(p, fileData.constData(), fileLen);
	}
}

void SStrCopy(char* dest, const char* src, int max_length)
{
	snprintf(dest, max_length, "%s", src);
}

DEVILUTION_END_NAMESPACE
