/**
 * @file monster.h
 *
 * Interface of monster functionality, AI, actions, spawning, loading, etc.
 */
#ifndef __MONSTER_H__
#define __MONSTER_H__

#define OPPOSITE(x) (((x) + 4) & 7)

extern int nummonsters;
extern MonsterStruct monsters[MAXMONSTERS];
extern MapMonData mapMonTypes[MAX_LVLMTYPES];
extern int nummtypes;
extern BYTE numSkelTypes;
/* The number of goat-monster types on the current level. */
extern BYTE numGoatTypes;
/* Skeleton-monster types on the current level. */
extern BYTE mapSkelTypes[MAX_LVLMTYPES];
/* Goat-monster types on the current level. */
extern BYTE mapGoatTypes[MAX_LVLMTYPES];

void InitLevelMonsters();
void GetLevelMTypes();
void InitMonsters();
void SetMapMonsters(BYTE* pMap, int startx, int starty);
void InitMonster(int mnum, int dir, int mtidx, int x, int y);
void AddMonster(int x, int y, int dir, int mtidx);
int PreSpawnSkeleton();

inline void SetMonsterLoc(MonsterStruct* mon, int x, int y)
{
	mon->_mx = x;
	mon->_my = y;
}

/* data */

extern const int offset_x[NUM_DIRS];
extern const int offset_y[NUM_DIRS];

#endif /* __MONSTER_H__ */
