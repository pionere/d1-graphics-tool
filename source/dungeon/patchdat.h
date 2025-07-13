/**
 * @file patchdat.h
 *
 * Interface of data related to patches.
 */
#ifndef __PATCHDAT_H__
#define __PATCHDAT_H__

#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

typedef struct DeltaFrameData {
	BYTE dfFrameNum;
	BYTE dfx;
	BYTE dfy;
	BYTE color;
} DeltaFrameData;

#ifdef __cplusplus
extern "C" {
#endif

extern const DeltaFrameData deltaRLHAS[3242];
extern const DeltaFrameData deltaRLHAW[2968];
extern const DeltaFrameData deltaRLHST[11936];
extern const DeltaFrameData deltaRLHBL[2243];
extern const DeltaFrameData deltaRLHQM[2968];
extern const DeltaFrameData deltaRLHWL[3834];
extern const DeltaFrameData deltaRLMAT[6037];
extern const DeltaFrameData deltaRMHAT[5043];
extern const DeltaFrameData deltaRMMAT[5280];
extern const DeltaFrameData deltaRMBFM[1153];
extern const DeltaFrameData deltaRMBLM[2470];
extern const DeltaFrameData deltaRMBQM[2123];

#ifdef __cplusplus
}
#endif

DEVILUTION_END_NAMESPACE

#endif /* __PATCHDAT_H__ */
