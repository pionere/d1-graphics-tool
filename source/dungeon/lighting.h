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

#define MAXDARKNESS     15
#define NUM_COLOR_TRNS  MAXDARKNESS + 12
#define COLOR_TRN_RED   MAXDARKNESS + 1
#define COLOR_TRN_GRAY  MAXDARKNESS + 2
#define COLOR_TRN_CORAL MAXDARKNESS + 3
#define COLOR_TRN_UNIQ  MAXDARKNESS + 4

void DoLighting(int nXPos, int nYPos, int nRadius, unsigned lnum);
void DoUnVision(int nXPos, int nYPos, int nRadius);
void DoVision(int nXPos, int nYPos, int nRadius);
void InitLightGFX();
void InitLighting();
unsigned AddLight(int x, int y, int r);
void InitVision();

/* rdata */

extern const int8_t CrawlTable[2749];
extern const int CrawlNum[19];

DEVILUTION_END_NAMESPACE

#endif /* __LIGHTING_H__ */
