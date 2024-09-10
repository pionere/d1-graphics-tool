/**
 * @file cursor.h
 *
 * Interface of cursor tracking functionality.
 */
#ifndef __CURSOR_H__
#define __CURSOR_H__

DEVILUTION_BEGIN_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

class D1Gfx;
class D1Pal;
extern D1Gfx *pCursCels;

void InitCursorGFX(D1Pal *pal);

/* rdata */
#define MAX_CURSOR_AREA 8192
extern const int InvItemWidth[(int)CURSOR_FIRSTITEM + (int)NUM_ICURS];
extern const int InvItemHeight[(int)CURSOR_FIRSTITEM + (int)NUM_ICURS];

#ifdef __cplusplus
}
#endif

DEVILUTION_END_NAMESPACE

#endif /* __CURSOR_H__ */
