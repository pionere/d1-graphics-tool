/**
 * @file monster.h
 *
 * Interface of monster functionality, AI, actions, spawning, loading, etc.
 */
#ifndef __MONSTER_H__
#define __MONSTER_H__

// Ticks necessary to finish the current action and add the result to the delta
// ~ ACTION_LENGTH + (gbNetUpdateRate * gbEmptyTurns) * (MAXMONSTERS / (NET_TURN_MSG_SIZE / sizeof(TSyncMonster)))
#define SQUELCH_LOW 127
#define SQUELCH_MAX (SQUELCH_LOW + 240)
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

void InitLvlMonsters();
void GetLevelMTypes();
void InitMonsters();
void InitMonster(int mnum, int dir, int mtidx, int x, int y);
void AddMonster(int mtidx, int x, int y);
bool LineClear(int x1, int y1, int x2, int y2);
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
