/**
 * @file missiles.h
 *
 * Interface of missile functionality.
 */
#ifndef __MISSILES_H__
#define __MISSILES_H__

DEVILUTION_BEGIN_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

class D1Hero;
struct MonsterStruct;

void GetMissileDamage(int mtype, const MonsterStruct *source, int *mindam, int *maxdam);
void SkillMonByPlrDamage(int sn, int sl, int dist, int source, const MonsterStruct *target, int *mindam, int *maxdam);
void SkillPlrByPlrDamage(int sn, int sl, int dist, int source, int target, int *mindam, int *maxdam);
void GetSkillDesc(const D1Hero *hero, int sn, int sl);
int MissPlrHitByMonChance(int mtype, int dist, const MonsterStruct *mon, const D1Hero *hero);
int MissMonHitByPlrChance(int mtype, int dist, const D1Hero *hero, const MonsterStruct *mon);
int MissPlrHitByPlrChance(int mtype, int dist, const D1Hero *offHero, const D1Hero *defHero);
unsigned CalcMonsterDam(unsigned mor, BYTE mRes, unsigned damage, bool penetrates);
unsigned CalcPlrDam(int pnum, BYTE mRes, unsigned damage);
int GetBaseMissile(int mtype);
BYTE GetMissileElement(int mtype);
const char *GetElementColor(BYTE mRes);
int GetArrowVelocity(int misource);

inline int CheckHit(int hitper)
{
	if (hitper > 75) {
		hitper = 75 + ((hitper - 75) >> 2);
	} else if (hitper < 25) {
		hitper = 25 + ((hitper - 25) >> 2);
	}
    if (hitper < 0)
        hitper = 0;
    if (hitper > 100)
        hitper = 100;
    return hitper;
}

#ifdef __cplusplus
}
#endif

DEVILUTION_END_NAMESPACE

#endif /* __MISSILES_H__ */
