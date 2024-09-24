/**
 * @file player.h
 *
 * Interface of player functionality, leveling, actions, creation, loading, etc.
 */
#ifndef __PLAYER_H__
#define __PLAYER_H__

DEVILUTION_BEGIN_NAMESPACE

#define myplr          players[mypnum]
#define plr            players[pnum]
#define plx(x)         players[x]
#define PLR_WALK_SHIFT 8

extern int mypnum;
extern PlayerStruct players[MAX_PLRS];

void CreatePlayer(int pnum, const _uiheroinfo& heroinfo);
void InitPlayer(int pnum);
void ClrPlrPath(int pnum);

int GetWalkSpeedInTicks(int pnum);
int GetAttackSpeedInTicks(int pnum, int sn);
int GetCastSpeedInTicks(int pnum, int sn);
int GetRecoverySpeedInTicks(int pnum);
int GetChargeSpeed(int pnum);
void GetMonByPlrDamage(int pnum, int sn, int sl, const MonsterStruct *mon, int *mindam, int *maxdam);
void GetPlrByPlrDamage(int offp, int sn, int sl, int defp, int *mindam, int *maxdam);

void IncreasePlrStr(int pnum);
void IncreasePlrMag(int pnum);
void IncreasePlrDex(int pnum);
void IncreasePlrVit(int pnum);
void RestorePlrHpVit(int pnum);
void DecreasePlrMaxHp(int pnum);

void DecreasePlrStr(int pnum);
void DecreasePlrMag(int pnum);
void DecreasePlrDex(int pnum);
void DecreasePlrVit(int pnum);

extern const BYTE PlrAnimFrameLens[NUM_PGXS];
extern const int StrengthTbl[NUM_CLASSES];
extern const int MagicTbl[NUM_CLASSES];
extern const int DexterityTbl[NUM_CLASSES];
extern const int VitalityTbl[NUM_CLASSES];
extern const BYTE Abilities[NUM_CLASSES];
extern const unsigned PlrExpLvlsTbl[MAXCHARLEVEL + 1];
extern const unsigned SkillExpLvlsTbl[MAXSPLLEVEL + 1];

DEVILUTION_END_NAMESPACE

#endif /* __PLAYER_H__ */
