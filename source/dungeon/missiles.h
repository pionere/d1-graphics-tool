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

void GetSkillDesc(D1Hero *hero, int sn, int sl);
unsigned CalcMonsterDam(unsigned mor, BYTE mRes, unsigned damage, bool penetrates);
unsigned CalcPlrDam(int pnum, BYTE mRes, unsigned damage);

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
