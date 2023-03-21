/**
 * @file engine.h
 *
 *  of basic engine helper functions:
 * - Sprite blitting
 * - Drawing
 * - Angle calculation
 * - RNG
 * - Memory allocation
 * - File loading
 * - Video playback
 */
#ifndef __ENGINE_H__
#define __ENGINE_H__

/* Set the current RNG seed */
void SetRndSeed(int32_t s);
/* Retrieve the current RNG seed */
int32_t GetRndSeed();
/* Get the next RNG seed */
int32_t NextRndSeed();
/* Retrieve the next pseudo-random number in the range of 0 <= x < v, where v is a non-negative (32bit) integer. The result is zero if v is negative. */
int random_(BYTE idx, int v);
/* Retrieve the next pseudo-random number in the range of 0 <= x < v, where v is a positive integer and less than or equal to 0x7FFF. */
int random_low(BYTE idx, int v);
/* Retrieve the next pseudo-random number in the range of minVal <= x <= maxVal, where (maxVal - minVal) is a non-negative (32bit) integer. The result is minVal if (maxVal - minVal) is negative. */
inline int RandRange(int minVal, int maxVal)
{
	return minVal + random_(0, maxVal - minVal + 1);
}
/* Retrieve the next pseudo-random number in the range of minVal <= x <= maxVal, where (maxVal - minVal) is a non-negative integer and less than 0x7FFF. */
inline int RandRangeLow(int minVal, int maxVal)
{
	return minVal + random_low(0, maxVal - minVal + 1);
}

BYTE* LoadFileInMem(const char* pszName, size_t* pdwFileLen = NULL);

void mem_free_dbg(void* p);
#define MemFreeDbg(p)       \
	{                       \
		void* p__p;         \
		p__p = p;           \
		p = NULL;           \
		mem_free_dbg(p__p); \
	}

/*
 * Helper function to simplify 'sizeof(list) / sizeof(list[0])' expressions.
 */
template <class T, int N>
inline constexpr int lengthof(T (&array)[N])
{
	return N;
}

DEVILUTION_END_NAMESPACE

#endif /* __ENGINE_H__ */
