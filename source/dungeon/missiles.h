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

int AddMissile(int sx, int sy, int dx, int dy, int midir, int mitype, int micaster, int misource, int spllvl);
void LoadMissileGFX(BYTE midx);
void InitGameMissileGFX();
void FreeGameMissileGFX();
void FreeMonMissileGFX();
void InitMissiles();
void ProcessMissiles();

int GetMisDirection16(int x1, int y1, int x2, int y2);
int GetMisDirection8(int x1, int y1, int x2, int y2);
/**
 * @brief Shifting the view area along the logical grid
 *        Note: this won't allow you to shift between even and odd rows
 * @param horizontal Shift the screen left or right
 * @param vertical Shift the screen up or down
 */
#define SHIFT_GRID(x, y, horizontal, vertical) \
	{                                          \
		x += (vertical) + (horizontal);        \
		y += (vertical) - (horizontal);        \
	}

#ifdef __cplusplus
}
#endif

DEVILUTION_END_NAMESPACE

#endif /* __MISSILES_H__ */
