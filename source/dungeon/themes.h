/**
 * @file themes.h
 *
 * Interface of the theme room placing algorithms.
 */
#ifndef __THEMES_H__
#define __THEMES_H__

extern int numthemes;
extern int zharlib;
extern ThemeStruct themes[MAXTHEMES];

void InitLvlThemes();
void InitThemes();

/**
 * CreateThemeRooms adds thematic elements to rooms.
 */
void CreateThemeRooms();

#endif /* __THEMES_H__ */
