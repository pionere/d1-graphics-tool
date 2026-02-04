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
void LoadFileWithMem(const char* pszName, BYTE* p);

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

#if defined(_MSC_VER)
#define DIAG_PRAGMA(x)                                            __pragma(warning(x))
#define DISABLE_WARNING(gcc_unused, clang_unused, msvc_errorcode) DIAG_PRAGMA(push) DIAG_PRAGMA(disable:##msvc_errorcode)
#define ENABLE_WARNING(gcc_unused, clang_unused, msvc_errorcode)  DIAG_PRAGMA(pop)
//#define DISABLE_WARNING(gcc_unused,clang_unused,msvc_errorcode) __pragma(warning(suppress: msvc_errorcode))
//#define ENABLE_WARNING(gcc_unused,clang_unused,msvc_unused) ((void)0)
#else
#define DIAG_STR(s)              #s
#define DIAG_JOINSTR(x, y)       DIAG_STR(x ## y)
#define DO_DIAG_PRAGMA(x)        _Pragma(#x)
#define DIAG_PRAGMA(compiler, x) DO_DIAG_PRAGMA(compiler diagnostic x)
#if defined(__clang__)
# define DISABLE_WARNING(gcc_unused, clang_option, msvc_unused) DIAG_PRAGMA(clang, push) DIAG_PRAGMA(clang, ignored DIAG_JOINSTR(-W, clang_option))
# define ENABLE_WARNING(gcc_unused, clang_option, msvc_unused)  DIAG_PRAGMA(clang, pop)
#elif defined(__GNUC__)
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
# define DISABLE_WARNING(gcc_option, clang_unused, msvc_unused) DIAG_PRAGMA(GCC, push) DIAG_PRAGMA(GCC, ignored DIAG_JOINSTR(-W, gcc_option))
# define ENABLE_WARNING(gcc_option, clang_unused, msvc_unused)  DIAG_PRAGMA(GCC, pop)
#else
# define DISABLE_WARNING(gcc_option, clang_unused, msvc_unused) DIAG_PRAGMA(GCC, ignored DIAG_JOINSTR(-W, gcc_option))
# define ENABLE_WARNING(gcc_option, clang_option, msvc_unused)  DIAG_PRAGMA(GCC, warning DIAG_JOINSTR(-W, gcc_option))
#endif
#else
#define DISABLE_WARNING(gcc_unused, clang_unused, msvc_unused) ;
#define ENABLE_WARNING(gcc_unused, clang_unused, msvc_unused)  ;
#endif
#endif
/*
 * Copy string from src to dest.
 * The NULL terminated content of src is copied to dest.
 */
template <size_t N1, size_t N2>
inline void copy_str(char (&dest)[N1], const char (&src)[N2])
{
	static_assert(N1 >= N2, "String does not fit the destination.");
	DISABLE_WARNING(deprecated-declarations, deprecated-declarations, 4996)
	strcpy(dest, src);
	ENABLE_WARNING(deprecated-declarations, deprecated-declarations, 4996)
}

/*
 * Copy constant string from src to dest.
 * The whole (padded) length of the src array is copied.
 */
template <size_t N1, size_t N2>
inline void copy_cstr(char (&dest)[N1], const char (&src)[N2])
{
	static_assert(N1 >= N2, "String does not fit the destination.");
	constexpr size_t src_len = ((N2 + sizeof(int) - 1) / sizeof(int)) * sizeof(int);
	constexpr size_t len = N1 >= src_len ? src_len : N1;
	memcpy(dest, src, len);
}

/*  SStrCopy @ 501
 *
 *  Copies a string from src to dest (including NULL terminator)
 *  until the max_length is reached.
 *
 *  dest:         The destination array.
 *  src:          The source array.
 *  max_length:   The maximum length of dest.
 *
 */
void SStrCopy(char* dest, const char* src, int max_length);

DEVILUTION_END_NAMESPACE

#endif /* __ENGINE_H__ */
