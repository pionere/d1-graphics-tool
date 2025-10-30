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
