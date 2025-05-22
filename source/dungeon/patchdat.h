/**
 * @file patchdat.h
 *
 * Interface of data related to patches.
 */
#ifndef __PATCHDAT_H__
#define __PATCHDAT_H__

#include "../all.h"

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

extern const DeltaFrameData deltaRMHAT[5039];
extern const DeltaFrameData deltaRMMAT[5273];

#ifdef __cplusplus
}
#endif

DEVILUTION_END_NAMESPACE

#endif /* __PATCHDAT_H__ */
