/**
 * @file towners.h
 *
 * Interface of functionality for loading and spawning towners.
 */
#ifndef __TOWNERS_H__
#define __TOWNERS_H__

DEVILUTION_BEGIN_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TownerStruct {
    char* tsPath;
    int8_t* tsAnimOrder;
} TownerStruct;

void InitTowners();

extern int numtowners;
extern TownerStruct towners[];

#ifdef __cplusplus
}
#endif

DEVILUTION_END_NAMESPACE

#endif /* __TOWNERS_H__ */
