/**
 * @file player.h
 *
 * Interface of player functionality, leveling, actions, creation, loading, etc.
 */
#ifndef __PLAYER_H__
#define __PLAYER_H__

DEVILUTION_BEGIN_NAMESPACE

#define plr            players[pnum]
#define plx(x)         players[x]

extern int mypnum;
extern PlayerStruct players[MAX_PLRS];

void SetPlrAnims(int pnum);
bool PosOkActor(int x, int y);
void PlrIncHp(int pnum, int hp);
bool PlrDecHp(int pnum, int hp, int dmgtype);

DEVILUTION_END_NAMESPACE

#endif /* __PLAYER_H__ */
