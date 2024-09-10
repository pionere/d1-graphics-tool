/**
 * @file interfac.h
 *
 * Interface of load screens.
 */
#ifndef __INTERFAC_H__
#define __INTERFAC_H__

#include <QString>

extern int ViewX;
extern int ViewY;
extern bool IsMultiGame;
extern bool IsHellfireGame;
extern bool HasTileset;
extern bool PatchDunFiles;
extern QString assetPath;
extern char infostr[256];
extern char tempstr[256];

void LogErrorF(const char* msg, ...);

#endif /* __INTERFAC_H__ */
