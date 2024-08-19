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

/**
 * @brief InitLvlThemes resets the global variables of the theme rooms.
 */
void InitLvlThemes();

/**
 * @brief InitThemes marks theme rooms as populated and selects their type.
 */
void InitThemes();

/**
 * @brief CreateThemeRooms adds thematic elements to rooms.
 */
void CreateThemeRooms();

#endif /* __THEMES_H__ */
