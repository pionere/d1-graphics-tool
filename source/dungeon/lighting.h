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
//#define NUM_COLOR_TRNS  MAXDARKNESS + 12
#define NUM_COLOR_TRNS  MAXDARKNESS
#define COLOR_TRN_RED   MAXDARKNESS + 1
#define COLOR_TRN_GRAY  MAXDARKNESS + 2
#define COLOR_TRN_CORAL MAXDARKNESS + 3
#define COLOR_TRN_UNIQ  MAXDARKNESS + 4
extern BYTE ColorTrns[NUM_COLOR_TRNS][NUM_COLORS];

void DoLighting(int nXPos, int nYPos, int nRadius, unsigned lnum);
void DoUnVision(int nXPos, int nYPos, int nRadius);
void DoVision(int nXPos, int nYPos, int nRadius);
void MakeLightTable();
void InitLightGFX();
void InitLighting();
unsigned AddLight(int x, int y, int r);
void InitVision();

/* rdata */

extern const int8_t CrawlTable[2749];
extern const int CrawlNum[19];

/* Maximum offset in a tile */
#define MAX_OFFSET 8
/* Maximum tile-distance till the precalculated tables are maintained. */
#define MAX_TILE_DIST (MAX_LIGHT_RAD + 1)
/* Maximum light-distance till the precalculated tables are maintained. */
#define MAX_LIGHT_DIST (MAX_TILE_DIST * MAX_OFFSET - 1)

extern BYTE darkTable[MAX_LIGHT_RAD + 1][MAX_LIGHT_DIST + 1];
extern BYTE distMatrix[MAX_OFFSET][MAX_OFFSET][MAX_TILE_DIST][MAX_TILE_DIST];

extern BYTE dLight[2 * (MAX_LIGHT_RAD + 1) + 1][2 * (MAX_LIGHT_RAD + 1) + 1];

DEVILUTION_END_NAMESPACE

#endif /* __LIGHTING_H__ */
