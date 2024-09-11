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

#ifdef __cplusplus
}
#endif

DEVILUTION_END_NAMESPACE

#endif /* __MISSILES_H__ */
