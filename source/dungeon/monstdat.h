/**
 * @file monstdat.h
 *
 * Interface of all monster data.
 */
#ifndef __MONSTDAT_H__
#define __MONSTDAT_H__

DEVILUTION_BEGIN_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

extern const MonsterData monsterdata[NUM_MTYPES];
extern const MonFileData monfiledata[NUM_MOFILE];
extern const BYTE MonstConvTbl[128];
extern const UniqMonData uniqMonData[92];

#ifdef __cplusplus
}
#endif

DEVILUTION_END_NAMESPACE

#endif /* __MONSTDAT_H__ */
