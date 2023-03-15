/**
 * @file interfac.h
 *
 * Interface of load screens.
 */
#ifndef __INTERFAC_H__
#define __INTERFAC_H__

#include "../d1dun.h"
#include "../dungeongeneratedialog.h"

extern int gnDifficulty;
extern bool IsMultiGame;
extern QString assetPath;

bool EnterGameLevel(D1Dun *dun, const GenerateDunParam &params);

#endif /* __INTERFAC_H__ */
