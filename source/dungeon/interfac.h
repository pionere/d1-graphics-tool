/**
 * @file interfac.h
 *
 * Interface of load screens.
 */
#ifndef __INTERFAC_H__
#define __INTERFAC_H__

#include <QString>

#define dev_fatal(msg, ...) ((void)0)

class D1Dun;
class D1Tileset;
class GenerateDunParam;
class LevelCelView;
struct MonsterStruct;

extern int ViewX;
extern int ViewY;
extern bool IsMultiGame;
extern bool IsHellfireGame;
extern bool HasTileset;
extern bool PatchDunFiles;
extern QString assetPath;
extern char infostr[256];

void EnterGameLevel(D1Dun *dun, D1Tileset *tileset, LevelCelView *view, const GenerateDunParam &params);
MonsterStruct* GetMonsterAt(int x, int y);

#endif /* __INTERFAC_H__ */
