/**
 * @file missiles.h
 *
 * Interface of missile functionality.
 */
#ifndef __MISSILES_H__
#define __MISSILES_H__

DEVILUTION_BEGIN_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

extern int missileactive[MAXMISSILES];
extern MissileStruct missile[MAXMISSILES];
extern int nummissiles;

int GetDirection16(int x1, int y1, int x2, int y2);
int GetDirection8(int x1, int y1, int x2, int y2);
int AddMissile(int sx, int sy, int dx, int dy, int midir, int mitype, int micaster, int misource, int spllvl);
void LoadMissileGFX(BYTE midx);
void InitGameMissileGFX();
void FreeGameMissileGFX();
void FreeMonMissileGFX();
void InitMissiles();
void ProcessMissiles();

#ifdef __cplusplus
}
#endif

DEVILUTION_END_NAMESPACE

#endif /* __MISSILES_H__ */
