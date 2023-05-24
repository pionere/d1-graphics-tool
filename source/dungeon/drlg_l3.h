/**
 * @file drlg_l3.h
 *
 * Interface of the caves level generation algorithms.
 */
#ifndef __DRLG_L3_H__
#define __DRLG_L3_H__

DEVILUTION_BEGIN_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

void DRLG_L3Subs();
void DRLG_L6Subs();
inline void DRLG_L3Shadows() { }
inline void DRLG_L6Shadows() { }
void DRLG_L3InitTransVals();
void CreateL3Dungeon();

#ifdef __cplusplus
}
#endif

DEVILUTION_END_NAMESPACE

#endif /* __DRLG_L3_H__ */
