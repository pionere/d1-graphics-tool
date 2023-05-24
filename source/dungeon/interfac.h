/**
 * @file interfac.h
 *
 * Interface of load screens.
 */
#ifndef __INTERFAC_H__
#define __INTERFAC_H__

#include <QString>

class D1Dun;
class D1Tileset;
class DecorateDunParam;
class GenerateDunParam;
class LevelCelView;

extern int ViewX;
extern int ViewY;
extern bool IsMultiGame;
extern bool IsHellfireGame;
extern bool HasTileset;
extern QString assetPath;
extern char infostr[256];

void LogErrorF(const char* msg, ...);
void DecorateGameLevel(D1Dun *dun, D1Tileset *tileset, LevelCelView *view, const DecorateDunParam &params);
void EnterGameLevel(D1Dun *dun, D1Tileset *tileset, LevelCelView *view, const GenerateDunParam &params);

#endif /* __INTERFAC_H__ */
