/**
 * @file gendung.h
 *
 * Interface of general dungeon generation code.
 */
#ifndef __GENDUNG_H__
#define __GENDUNG_H__

DEVILUTION_BEGIN_NAMESPACE

extern BYTE dungeon[DMAXX][DMAXY];
extern BYTE pdungeon[DMAXX][DMAXY];
extern BYTE drlgFlags[DMAXX][DMAXY];
extern DrlgMem drlg;
extern SetPieceStruct pSetPieces[4];
extern WarpStruct pWarps[NUM_DWARP];
extern uint16_t* pMegaTiles;
extern BYTE* pSolidTbl;
extern BYTE microFlags[MAXTILES + 1];
extern bool nBlockTable[MAXTILES + 1];
extern bool nSolidTable[MAXTILES + 1];
extern BYTE nTrapTable[MAXTILES + 1];
extern bool nMissileTable[MAXTILES + 1];
extern int gnDifficulty;
extern LevelStruct currLvl;
extern int MicroTileLen;
extern BYTE numtrans;
extern bool TransList[256];
extern int dPiece[MAXDUNX][MAXDUNY];
extern BYTE dTransVal[MAXDUNX][MAXDUNY];
extern BYTE dFlags[MAXDUNX][MAXDUNY];
extern int dMonster[MAXDUNX][MAXDUNY];
extern int8_t dObject[MAXDUNX][MAXDUNY];
extern BYTE dItem[MAXDUNX][MAXDUNY];
extern BYTE dSpecial[MAXDUNX][MAXDUNY];

void InitLvlDungeon();
void FreeSetPieces();
void FreeLvlDungeon();
void DRLG_Init_Globals();
void DRLG_PlaceRndTile(BYTE search, BYTE replace, BYTE rndper);
POS32 DRLG_PlaceMiniSet(const BYTE* miniset);
void DRLG_PlaceMegaTiles(int idx);
void DRLG_DrawMap(int idx);
void DRLG_InitTrans();
void DRLG_MRectTrans(int x1, int y1, int x2, int y2, int tv);
void DRLG_RectTrans(int x1, int y1, int x2, int y2);
void DRLG_ListTrans(int num, const BYTE* List);
void DRLG_AreaTrans(int num, const BYTE* List);
void DRLG_FloodTVal();
void DRLG_SetMapTrans(BYTE* pMap);
void DRLG_SetPC();
void DRLG_PlaceThemeRooms(int minSize, int maxSize, int floor, int freq, bool rndSize);
bool NearThemeRoom(int x, int y);

inline void DRLG_CopyTrans(int sx, int sy, int dx, int dy)
{
	dTransVal[dx][dy] = dTransVal[sx][sy];
}

DEVILUTION_END_NAMESPACE

#endif /* __GENDUNG_H__ */
