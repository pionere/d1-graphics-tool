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

void CreatePlayer(const _uiheroinfo& heroinfo);
void InitPlayer(int pnum);
void ClrPlrPath(int pnum);

void IncreasePlrStr(int pnum);
void IncreasePlrMag(int pnum);
void IncreasePlrDex(int pnum);
void IncreasePlrVit(int pnum);
void RestorePlrHpVit(int pnum);

void CalculateGold(int pnum);

DEVILUTION_END_NAMESPACE

#endif /* __PLAYER_H__ */
