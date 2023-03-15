/**
 * @file defs.h
 *
 * Global definitions and Macros.
 */
//#ifndef _DEFS_H
//#define _DEFS_H

//#undef DEV_MODE
#define DEV_MODE 1
#define HELLFIRE 1

#define DEVILUTION_BEGIN_NAMESPACE
#define DEVILUTION_END_NAMESPACE
#define ALIGN32
#define ALIGN64
#define ALIGNMENT

#define FALSE 0
#define TRUE 1

// MAXDUN = DSIZE + 2 * DBORDER
// DSIZE = 2 * DMAX
#define DMAXX					40
#define DMAXY					40
#define DBORDERX				16
#define DBORDERY				16
#define DSIZEX					80
#define DSIZEY					80
#define MAXDUNX					112
#define MAXDUNY					112

#define MAX_LVLMTYPES			12
#define MAX_LVLMIMAGE			4000

#ifdef HELLFIRE
#define MAXTRIGGERS				7
#else
#define MAXTRIGGERS				5
#endif

#define DEAD_MULTI				0xFF
#define MAXITEMS				127
#define ITEM_NONE				0xFF
#define MAXBELTITEMS			8
#define MAXLIGHTS				32
#define MAXMONSTERS				200
#define MON_NONE				0xFF
#define MAXOBJECTS				127
#define OBJ_NONE				0xFF
#define MAXTHEMES				50
#define MAXTILES				2047
#define MAXVISION				(MAX_PLRS + MAX_MINIONS)
#define MDMAXX					40
#define MDMAXY					40
#ifdef HELLFIRE
#define ITEMTYPES				43
#define BASESTAFFCHARGES		18
#else
#define ITEMTYPES				35
#define BASESTAFFCHARGES		40
#endif

// Item indestructible durability
#define DUR_INDESTRUCTIBLE		255

#define GOLD_SMALL_LIMIT		1000
#define GOLD_MEDIUM_LIMIT		2500
#define GOLD_MAX_LIMIT			5000

#define DIFFICULTY_EXP_BONUS   800

#define NIGHTMARE_LEVEL_BONUS   16
#define HELL_LEVEL_BONUS        32

#define NIGHTMARE_TO_HIT_BONUS  85
#define HELL_TO_HIT_BONUS      120

#define NIGHTMARE_AC_BONUS 50
#define HELL_AC_BONUS      80

#define NIGHTMARE_EVASION_BONUS 35
#define HELL_EVASION_BONUS      50

#define NIGHTMARE_MAGIC_BONUS 35
#define HELL_MAGIC_BONUS      50

#define POS_IN_RECT(x, y, rx, ry, rw, rh) \
	((x) >= (rx)                          \
	&& (x) < (rx + rw)                    \
	&& (y) >= (ry)                        \
	&& (y) < (ry + rh))

#define IN_DUNGEON_AREA(x, y) \
	(x >= 0                   \
	&& x < MAXDUNX            \
	&& y >= 0                 \
	&& y < MAXDUNY)

#undef assert

#ifdef _MSC_VER
#define ASSUME_UNREACHABLE __assume(0);
#elif defined(__clang__)
#define ASSUME_UNREACHABLE __builtin_unreachable();
#elif defined(__GNUC__)
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 405
#define ASSUME_UNREACHABLE __builtin_unreachable();
#else
#define ASSUME_UNREACHABLE assert(0);
#endif
#endif

#if DEBUG_MODE || DEV_MODE
#undef ASSUME_UNREACHABLE
#define ASSUME_UNREACHABLE assert(0);
#endif

//#endif /* _DEFS_H */