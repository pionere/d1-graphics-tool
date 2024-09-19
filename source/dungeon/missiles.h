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
void GetSkillDamage(int sn, int sl, const D1Hero *source, const MonsterStruct *target, int *mindam, int *maxdam);
void GetSkillDesc(const D1Hero *hero, int sn, int sl);
int GetMonMisHitChance(int mtype, const MonsterStruct *mon, const D1Hero *hero);
int GetPlrMisHitChance(int mtype, const D1Hero *hero, const MonsterStruct *mon);
unsigned CalcMonsterDam(unsigned mor, BYTE mRes, unsigned damage, bool penetrates);
unsigned CalcPlrDam(const D1Hero *hero, BYTE mRes, unsigned damage);
int GetBaseMissile(int mtype);
BYTE GetMissileElement(int mtype);
const char *GetElementColor(BYTE mRes);

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
