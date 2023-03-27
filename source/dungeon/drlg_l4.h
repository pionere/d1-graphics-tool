/**
 * @file drlg_l4.h
 *
 * Interface of the hell level generation algorithms.
 */
#ifndef __DRLG_L4_H__
#define __DRLG_L4_H__

DEVILUTION_BEGIN_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

// position of the circle in front of Lazarus (single player)
#define LAZ_CIRCLE_X (DBORDERX + 19)
#define LAZ_CIRCLE_Y (DBORDERY + 30)

void CreateL4Dungeon();

#ifdef __cplusplus
}
#endif

DEVILUTION_END_NAMESPACE

#endif /* __DRLG_L4_H__ */
