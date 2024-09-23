/**
 * @file lighting.h
 *
 * Interface of light and vision.
 */
#ifndef __LIGHTING_H__
#define __LIGHTING_H__

DEVILUTION_BEGIN_NAMESPACE

#define NO_LIGHT      MAXLIGHTS
#define NO_VISION     MAXVISION
#define MAX_LIGHT_RAD 15

#define MAXDARKNESS     0
#define NUM_COLOR_TRNS  MAXDARKNESS + 3
#define COLOR_TRN_RED   MAXDARKNESS + 1
#define COLOR_TRN_GRAY  MAXDARKNESS + 2

extern BYTE ColorTrns[NUM_COLOR_TRNS][NUM_COLORS];

extern void InitLighting();

DEVILUTION_END_NAMESPACE

#endif /* __LIGHTING_H__ */
