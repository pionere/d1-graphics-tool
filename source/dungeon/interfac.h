/**
 * @file interfac.h
 *
 * Interface of load screens.
 */
#ifndef __INTERFAC_H__
#define __INTERFAC_H__

#include <QString>

class D1Dun;
class GenerateDunParam;
class LevelCelView;

extern int ViewX;
extern int ViewY;
extern bool IsMultiGame;
extern bool IsHellfireGame;
extern QString assetPath;
extern char infostr[256];

void LogErrorF(const char* msg, ...);
bool EnterGameLevel(D1Dun *dun, LevelCelView *view, const GenerateDunParam &params);

#endif /* __INTERFAC_H__ */
