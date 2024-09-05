/**
 * @file inv.h
 *
 * Interface of player inventory.
 */
#ifndef __INV_H__
#define __INV_H__

DEVILUTION_BEGIN_NAMESPACE

#define TWOHAND_WIELD(pp, ii) ((ii)->_iLoc == ILOC_TWOHAND && ((ii)->_itype == ITYPE_BOW || (pp)->_pBaseStr < (ii)->_iMinStr * 4))

#ifdef __cplusplus
extern "C" {
#endif

extern const InvXY InvRect[NUM_XY_SLOTS];
extern const BYTE InvSlotTbl[NUM_XY_SLOTS];

void CheckInvClick(bool bShift);
BYTE CheckInvItem();
void CalculateGold(int pnum);

#ifdef __cplusplus
}
#endif

DEVILUTION_END_NAMESPACE

#endif /* __INV_H__ */
