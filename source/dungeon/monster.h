/**
 * @file monster.h
 *
 * Interface of monster functionality, AI, actions, spawning, loading, etc.
 */
#ifndef __MONSTER_H__
#define __MONSTER_H__

#define OPPOSITE(x) (((x) + 4) & 7)

extern MonsterStruct monsters[MAXMONSTERS];
// extern MapMonData mapMonTypes[MAX_LVLMTYPES];
// extern int nummtypes;

void InitUniqMonster(int type, int numplrs, int lvlbonus, bool minion);
void InitLvlMonster(int type, int numplrs, int lvlbonus);

// void InitLvlMonsters();
// void GetLevelMTypes();
// void InitMonster(int mnum, int dir, int mtidx, int x, int y);

inline void SetMonsterLoc(MonsterStruct* mon, int x, int y)
{
	mon->_mx = x;
	mon->_my = y;
}

/* data */

extern const int offset_x[NUM_DIRS];
extern const int offset_y[NUM_DIRS];

#endif /* __MONSTER_H__ */
