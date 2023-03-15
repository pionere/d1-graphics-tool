/**
 * @file interfac.h
 *
 * Interface of load screens.
 */
#ifndef __INTERFAC_H__
#define __INTERFAC_H__

#include <QString>

class GenerateDunParam;
class D1Dun;

extern int gnDifficulty;
extern int ViewX;
extern int ViewY;
extern bool IsMultiGame;
extern QString assetPath;

bool EnterGameLevel(D1Dun *dun, const GenerateDunParam &params);

#endif /* __INTERFAC_H__ */
