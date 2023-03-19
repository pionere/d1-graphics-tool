/**
 * @file objects.cpp
 *
 * Implementation of object functionality, interaction, spawning, loading, etc.
 */
#include "all.h"

#define DOOR_CLOSED  0
#define DOOR_OPEN    1
#define DOOR_BLOCKED 2

#define TRAP_ACTIVE   0
#define TRAP_INACTIVE 1

#define FLAMETRAP_DIR_X          2
#define FLAMETRAP_DIR_Y          0
#define FLAMETRAP_FIRE_ACTIVE    1
#define FLAMETRAP_FIRE_INACTIVE  0
#define FLAMETRAP_ACTIVE_FRAME   1
#define FLAMETRAP_INACTIVE_FRAME 2

int trapid;
static BYTE* objanimdata[NUM_OFILE_TYPES] = { 0 };
int objectactive[MAXOBJECTS];
/** Specifies the number of active objects. */
int numobjects;
int leverid;
//int objectavail[MAXOBJECTS];
ObjectStruct objects[MAXOBJECTS];
//bool gbInitObjFlag;

/** Specifies the X-coordinate delta between barrels. */
const int bxadd[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
/** Specifies the Y-coordinate delta between barrels. */
const int byadd[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
/** Maps from shrine_id to shrine name. */
const char* const shrinestrs[NUM_SHRINETYPE] = {
	"Hidden",
	"Gloomy",
	"Weird",
	"Religious",
	"Magical",
	"Stone",
	"Creepy",
	"Thaumaturgic",
	"Fascinating",
	"Shimmering",
	"Cryptic",
	"Eldritch",
	"Eerie",
	"Spooky",
	"Quiet",
	"Divine",
	"Holy",
	"Sacred",
	"Ornate",
	"Spiritual",
	"Secluded",
	"Glimmering",
	"Tainted",
	"Glistening",
	"Sparkling",
	"Murphy's",
#ifdef HELLFIRE
	"Solar",
#endif
};
/**
 * Specifies the game type for which each shrine may appear.
 * SHRINETYPE_ANY - 0 - sp & mp
 * SHRINETYPE_SINGLE - 1 - sp only
 * SHRINETYPE_MULTI - 2 - mp only
 */
const BYTE shrineavail[NUM_SHRINETYPE] = {
	SHRINETYPE_ANY,    // SHRINE_HIDDEN
	SHRINETYPE_ANY,    // SHRINE_GLOOMY
	SHRINETYPE_ANY,    // SHRINE_WEIRD
	SHRINETYPE_ANY,    // SHRINE_RELIGIOUS
	SHRINETYPE_ANY,    // SHRINE_MAGICAL
	SHRINETYPE_ANY,    // SHRINE_STONE
	SHRINETYPE_ANY,    // SHRINE_CREEPY
	SHRINETYPE_SINGLE, // SHRINE_THAUMATURGIC
	SHRINETYPE_ANY,    // SHRINE_FASCINATING
	SHRINETYPE_ANY,    // SHRINE_SHIMMERING
	SHRINETYPE_ANY,    // SHRINE_CRYPTIC
	SHRINETYPE_ANY,    // SHRINE_ELDRITCH
	SHRINETYPE_MULTI,  // SHRINE_EERIE
	SHRINETYPE_MULTI,  // SHRINE_SPOOKY
	SHRINETYPE_MULTI,  // SHRINE_QUIET
	SHRINETYPE_ANY,    // SHRINE_DIVINE
	SHRINETYPE_ANY,    // SHRINE_HOLY
	SHRINETYPE_ANY,    // SHRINE_SACRED
	SHRINETYPE_ANY,    // SHRINE_ORNATE
	SHRINETYPE_ANY,    // SHRINE_SPIRITUAL
	SHRINETYPE_ANY,    // SHRINE_SECLUDED
	SHRINETYPE_ANY,    // SHRINE_GLIMMERING
	SHRINETYPE_MULTI,  // SHRINE_TAINTED
	SHRINETYPE_ANY,    // SHRINE_GLISTENING
	SHRINETYPE_ANY,    // SHRINE_SPARKLING
	SHRINETYPE_ANY,    // SHRINE_MURPHYS
#ifdef HELLFIRE
	SHRINETYPE_ANY,    // SHRINE_SOLAR
#endif
};
/** Maps from book_id to book name. */
const char StoryBookName[][28] = {
	"The Great Conflict",
	"The Wages of Sin are War",
	"The Tale of the Horadrim",
	"The Dark Exile",
	"The Sin War",
	"The Binding of the Three",
	"The Realms Beyond",
	"Tale of the Three",
	"The Black King",
#ifdef HELLFIRE
	//"Journal: The Ensorcellment",
	"Journal: The Meeting",
	"Journal: The Tirade",
	"Journal: His Power Grows",
	"Journal: NA-KRUL",
	"Journal: The End",
	"A Spellbook",
#endif
};
/** Specifies the speech IDs of each dungeon type narrator book. */
const int StoryText[3][3] = {
	{ TEXT_BOOK11, TEXT_BOOK12, TEXT_BOOK13 },
	{ TEXT_BOOK21, TEXT_BOOK22, TEXT_BOOK23 },
	{ TEXT_BOOK31, TEXT_BOOK32, TEXT_BOOK33 }
};

void InitObjectGFX()
{
	/*const ObjectData* ods;
	bool themeload[NUM_THEMES];
	bool fileload[NUM_OFILE_TYPES];
	char filestr[DATA_ARCHIVE_MAX_PATH];
	int i;

	static_assert(false == 0, "InitObjectGFX fills fileload and themeload with 0 instead of false values.");
	memset(fileload, 0, sizeof(fileload));
	memset(themeload, 0, sizeof(themeload));

	for (i = 0; i < numthemes; i++)
		themeload[themes[i].ttype] = true;

	BYTE lvlMask = 1 << currLvl._dType;
	for (i = 0; i < NUM_OBJECTS; i++) {
		ods = &objectdata[i];
		if ((ods->oLvlTypes & lvlMask)
		 || (ods->otheme != THEME_NONE && themeload[ods->otheme])
		 || (ods->oquest != Q_INVALID && QuestStatus(ods->oquest))) {
			fileload[ods->ofindex] = true;
		}
	}

	for (i = 0; i < NUM_OFILE_TYPES; i++) {
		if (fileload[i]) {
			snprintf(filestr, sizeof(filestr), "Objects\\%s.CEL", objfiledata[i].ofName);
			assert(objanimdata[i] == NULL);
			objanimdata[i] = LoadFileInMem(filestr);
		}
	}*/
}

void FreeObjectGFX()
{
	/*int i;

	for (i = 0; i < NUM_OFILE_TYPES; i++) {
		MemFreeDbg(objanimdata[i]);
	}*/
}

/**
 * Check the location if an object can be placed there in the init phase.
 * Must not consider the player's position, since it could change the dungeon
 * when a player re-enters the dungeon.
 */
static bool RndLocOk(int xp, int yp)
{
	if ((dMonster[xp][yp] | /*dPlayer[xp][yp] |*/ dObject[xp][yp]
	 | nSolidTable[dPiece[xp][yp]] | (dFlags[xp][yp] & BFLAG_POPULATED)) != 0)
		return false;
	// should be covered by Freeupstairs.
	//if (currLvl._dDunType != DTYPE_CATHEDRAL || dPiece[xp][yp] <= 126 || dPiece[xp][yp] >= 144)
		return true;
	//return false;
}

static POS32 RndLoc3x3()
{
	int xp, yp, i, j, tries;
	static_assert(DBORDERX != 0, "RndLoc3x3 returns 0;0 position as a failed location.");
	tries = 0;
	while (TRUE) {
		xp = random_(140, DSIZEX) + DBORDERX;
		yp = random_(140, DSIZEY) + DBORDERY;
		for (i = -1; i <= 1; i++) {
			for (j = -1; j <= 1; j++) {
				if (!RndLocOk(xp + i, yp + j))
					goto fail;
			}
		}
		return { xp, yp };
fail:
		if (++tries > 20000)
			break;
	}
	return { 0, 0 };
}

static POS32 RndLoc3x4()
{
	int xp, yp, i, j, tries;
	static_assert(DBORDERX != 0, "RndLoc3x4 returns 0;0 position as a failed location.");
	tries = 0;
	while (TRUE) {
		xp = random_(140, DSIZEX) + DBORDERX;
		yp = random_(140, DSIZEY) + DBORDERY;
		for (i = -1; i <= 1; i++) {
			for (j = -2; j <= 1; j++) {
				if (!RndLocOk(xp + i, yp + j))
					goto fail;
			}
		}
		return { xp, yp };
fail:
		if (++tries > 20000)
			break;
	}
	return { 0, 0 };
}

static POS32 RndLoc5x5()
{
	int xp, yp, i, j, tries;
	static_assert(DBORDERX != 0, "RndLoc5x5 returns 0;0 position as a failed location.");
	tries = 0;
	while (TRUE) {
		xp = random_(140, DSIZEX) + DBORDERX;
		yp = random_(140, DSIZEY) + DBORDERY;
		for (i = -2; i <= 2; i++) {
			for (j = -2; j <= 2; j++) {
				if (!RndLocOk(xp + i, yp + j))
					goto fail;
			}
		}
		return { xp, yp };
fail:
		if (++tries > 20000)
			break;
	}
	return { 0, 0 };
}

static POS32 RndLoc7x5()
{
	int xp, yp, i, j, tries;
	static_assert(DBORDERX != 0, "RndLoc7x5 returns 0;0 position as a failed location.");
	tries = 0;
	while (TRUE) {
		xp = random_(140, DSIZEX) + DBORDERX;
		yp = random_(140, DSIZEY) + DBORDERY;
		for (i = -3; i <= 3; i++) {
			for (j = -2; j <= 2; j++) {
				if (!RndLocOk(xp + i, yp + j))
					goto fail;
			}
		}
		return { xp, yp };
fail:
		if (++tries > 20000)
			break;
	}
	return { 0, 0 };
}

static POS32 RndLoc6x7()
{
	int xp, yp, i, j, tries;
	static_assert(DBORDERX != 0, "RndLoc6x7 returns 0;0 position as a failed location.");
	tries = 0;
	while (TRUE) {
		xp = random_(140, DSIZEX) + DBORDERX;
		yp = random_(140, DSIZEY) + DBORDERY;
		for (i = -2; i <= 3; i++) {
			for (j = -3; j <= 3; j++) {
				if (!RndLocOk(xp + i, yp + j))
					goto fail;
			}
		}
		return { xp, yp };
fail:
		if (++tries > 20000)
			break;
	}
	return { 0, 0 };
}

static void InitRndLocObj(int min, int max, int objtype)
{
	int i, numobjs;
	POS32 pos;

	//assert(max >= min);
	//assert(max - min < 0x7FFF);
	numobjs = RandRangeLow(min, max);
	for (i = 0; i < numobjs; i++) {
		pos = RndLoc3x3();
		if (pos.x == 0)
			break;
		AddObject(objtype, pos.x, pos.y);
	}
}

static void InitRndSarcs(int objtype)
{
	int i, numobjs;
	POS32 pos;

	numobjs = RandRange(10, 15);
	for (i = 0; i < numobjs; i++) {
		pos = RndLoc3x4();
		if (pos.x == 0)
			break;
		AddObject(objtype, pos.x, pos.y);
	}
}

static void InitRndLocObj5x5(int objtype)
{
	POS32 pos = RndLoc5x5();

	if (pos.x != 0)
		AddObject(objtype, pos.x, pos.y);
}

void InitLevelObjects()
{
//	int i;

	numobjects = 0;

	//memset(objects, 0, sizeof(objects));
	//memset(objectactive, 0, sizeof(objectactive));

//	for (i = 0; i < MAXOBJECTS; i++)
//		objectavail[i] = i;

	trapid = 1;
	leverid = 1;
}

static void AddCandles()
{
	int tx, ty;

	// tx = quests[Q_PWATER]._qtx;
	// ty = quests[Q_PWATER]._qty;
	tx = pWarps[DWARP_SIDE]._wx + 1; // pWarps[DWARP_SIDE]._wx * 2 + DBORDERX + 1;
	ty = pWarps[DWARP_SIDE]._wy;     // pWarps[DWARP_SIDE]._wy * 2 + DBORDERY;
	AddObject(OBJ_STORYCANDLE, tx - 2, ty + 1);
	AddObject(OBJ_STORYCANDLE, tx + 3, ty + 1);
	AddObject(OBJ_STORYCANDLE, tx - 1, ty + 2);
	AddObject(OBJ_STORYCANDLE, tx + 2, ty + 2);
}

static void AddBookLever(int type, int x, int y, int x1, int y1, int x2, int y2, int qn)
{
	int oi;
	POS32 pos;

	if (x == -1) {
		pos = RndLoc5x5();
		if (pos.x == 0)
			return;
		x = pos.x;
		y = pos.y;
	}

	oi = AddObject(type, x, y);
	SetObjMapRange(oi, x1, y1, x2, y2, leverid);
	leverid++;
	objects[oi]._oVar6 = objects[oi]._oAnimFrame + 1; // LEVER_BOOK_ANIM
	objects[oi]._oVar7 = qn; // LEVER_BOOK_QUEST
}

static void InitRndBarrels()
{
	int i, xp, yp;
	int o; // type of the object
	int dir;
	int t; // number of tries of placing next barrel in current group
	int c; // number of barrels in current group

	static_assert((int)OBJ_BARREL + 1 == (int)OBJ_BARRELEX, "InitRndBarrels expects ordered BARREL enum I.");
	o = OBJ_BARREL;
	static_assert((int)OBJ_URN + 1 == (int)OBJ_URNEX, "InitRndBarrels expects ordered BARREL enum II.");
	static_assert((int)OBJ_POD + 1 == (int)OBJ_PODEX, "InitRndBarrels expects ordered BARREL enum III.");
	if (currLvl._dType == DTYPE_CRYPT)
		o = OBJ_URN;
	else if (currLvl._dType == DTYPE_NEST)
		o = OBJ_POD;

	// generate i number of groups of barrels
	for (i = RandRange(3, 7); i != 0; i--) {
		do {
			xp = random_(143, DSIZEX) + DBORDERX;
			yp = random_(143, DSIZEY) + DBORDERY;
		} while (!RndLocOk(xp, yp));
		AddObject(o + (random_(143, 4) == 0 ? 1 : 0), xp, yp);
		c = 1;
		do {
			for (t = 0; t < 3; t++) {
				dir = random_(143, NUM_DIRS);
				xp += bxadd[dir];
				yp += byadd[dir];
				if (RndLocOk(xp, yp))
					break;
			}
			if (t == 3)
				break;
			AddObject(o + (random_(143, 5) == 0 ? 1 : 0), xp, yp);
			c++;
		} while (random_low(143, c >> 1) == 0);
	}
}

void AddL1Objs(int x1, int y1, int x2, int y2)
{
	int i, j, pn;

	for (j = y1; j < y2; j++) {
		for (i = x1; i < x2; i++) {
			pn = dPiece[i][j];
			if (pn == 270)
				AddObject(OBJ_L1LIGHT, i, j);
			// these pieces are closed doors which are placed directly
			assert(pn != 51 && pn != 56);
			if (pn == 44 || /*pn == 51 ||*/ pn == 214)
				AddObject(OBJ_L1LDOOR, i, j);
			if (pn == 46 /*|| pn == 56*/)
				AddObject(OBJ_L1RDOOR, i, j);
		}
	}
}

static void AddL5Objs(int x1, int y1, int x2, int y2)
{
	int i, j, pn;

	for (j = y1; j < y2; j++) {
		for (i = x1; i < x2; i++) {
			pn = dPiece[i][j];
			// 77 and 80 pieces are closed doors which are placed directly
			if (pn == 77)
				AddObject(OBJ_L5LDOOR, i, j);
			if (pn == 80)
				AddObject(OBJ_L5RDOOR, i, j);
		}
	}
}

void AddL2Objs(int x1, int y1, int x2, int y2)
{
	int i, j, pn;

	for (j = y1; j < y2; j++) {
		for (i = x1; i < x2; i++) {
			pn = dPiece[i][j];
			// 13 and 17 pieces are open doors and not handled at the moment
			// 541 and 542 are doorways which are no longer handled as doors
			// 538 and 540 pieces are closed doors
			if (/*pn == 13 ||*/ pn == 538 /*|| pn == 541*/)
				AddObject(OBJ_L2LDOOR, i, j);
			if (/*pn == 17 ||*/ pn == 540 /*|| pn == 542*/)
				AddObject(OBJ_L2RDOOR, i, j);
		}
	}
}

static void AddL3Objs(int x1, int y1, int x2, int y2)
{
	int i, j, pn;

	for (j = y1; j < y2; j++) {
		for (i = x1; i < x2; i++) {
			pn = dPiece[i][j];
			// 531 and 534 pieces are closed doors which are placed directly
			if (pn == 534)
				AddObject(OBJ_L3LDOOR, i, j);
			if (pn == 531)
				AddObject(OBJ_L3RDOOR, i, j);
		}
	}
}

static void AddL2Torches()
{
	int i, j;
	// place torches on NE->SW walls
	for (i = DBORDERX; i < DBORDERX + DSIZEX; i++) {
		for (j = DBORDERY; j < DBORDERY + DSIZEY; j++) {
			// skip setmap pieces
			if (dFlags[i][j] & BFLAG_POPULATED)
				continue;
			// select 'trapable' position
			if (nTrapTable[dPiece[i][j]] != PTT_LEFT)
				continue;
			if (random_(145, 32) != 0)
				continue;
			// assert(nSolidTable[dPiece[i][j - 1]] | nSolidTable[dPiece[i][j + 1]]);
			if (!nSolidTable[dPiece[i + 1][j]]) {
				AddObject(OBJ_TORCHL1, i, j);
			} else {
				AddObject(OBJ_TORCHL2, i - 1, j);
			}
			// skip a few tiles to prevent close placement
			j += 4;
		}
	}
	// place torches on NW->SE walls
	for (j = DBORDERY; j < DBORDERY + DSIZEY; j++) {
		for (i = DBORDERX; i < DBORDERX + DSIZEX; i++) {
			// skip setmap pieces
			if (dFlags[i][j] & BFLAG_POPULATED)
				continue;
			// select 'trapable' position
			if (nTrapTable[dPiece[i][j]] != PTT_RIGHT)
				continue;
			if (random_(145, 32) != 0)
				continue;
			// assert(nSolidTable[dPiece[i - 1][j]] | nSolidTable[dPiece[i + 1][j]]);
			if (!nSolidTable[dPiece[i][j + 1]]) {
				AddObject(OBJ_TORCHR1, i, j);
			} else {
				if (dObject[i][j - 1] == 0)
					AddObject(OBJ_TORCHR2, i, j - 1);
			}
			// skip a few tiles to prevent close placement
			i += 4;
		}
	}
}

static void AddObjTraps()
{
	int i, j, tx, ty, on, rndv;
	int8_t oi;

	rndv = 10 + (currLvl._dLevel >> 1);
	for (j = DBORDERY; j < DBORDERY + DSIZEY; j++) {
		for (i = DBORDERX; i < DBORDERX + DSIZEX; i++) {
			oi = dObject[i][j];
			oi--;
			if (oi < 0)
				continue;
			if (!objectdata[objects[oi]._otype].oTrapFlag)
				continue;

			if (random_(144, 128) >= rndv)
				continue;
			if (random_(144, 2) == 0) {
				tx = i - 1;
				while (!nSolidTable[dPiece[tx][j]])
					tx--;

				if (i - tx <= 1)
					continue;

				ty = j;
				on = OBJ_TRAPL;
			} else {
				ty = j - 1;
				while (!nSolidTable[dPiece[i][ty]])
					ty--;

				if (j - ty <= 1)
					continue;

				tx = i;
				on = OBJ_TRAPR;
			}
			if (dFlags[tx][ty] & BFLAG_POPULATED)
				continue;
			if (dObject[tx][ty] != 0)
				continue;
			if (nTrapTable[dPiece[tx][ty]] == PTT_NONE)
				continue;
			on = AddObject(on, tx, ty);
			if (on == -1)
				return;
			objects[on]._oVar1 = oi; // TRAP_OI_REF
			objects[oi]._oTrapChance = RandRange(1, 64);
			objects[oi]._oVar5 = on + 1; // TRAP_OI_BACKREF
		}
	}
}

static void AddChestTraps()
{
	int i, j;
	int8_t oi;

	for (j = DBORDERY; j < DBORDERY + DSIZEY; j++) {
		for (i = DBORDERX; i < DBORDERX + DSIZEX; i++) {
			oi = dObject[i][j];
			if (oi > 0) {
				oi--;
				if (objects[oi]._otype >= OBJ_CHEST1 && objects[oi]._otype <= OBJ_CHEST3 && objects[oi]._oTrapChance == 0 && random_(0, 100) < 10) {
					objects[oi]._otype += OBJ_TCHEST1 - OBJ_CHEST1;
					objects[oi]._oTrapChance = RandRange(1, 64);
					objects[oi]._oVar5 = 0; // TRAP_OI_BACKREF
				}
			}
		}
	}
}

typedef struct LeverRect {
	int x1;
	int y1;
	int x2;
	int y2;
	int leveridx;
} LeverRect;
static void LoadMapSetObjects(const char* map, int startx, int starty, const LeverRect* lvrRect)
{
	BYTE* pMap = LoadFileInMem(map);
	int i, j, oi;
	uint16_t rw, rh, *lm;

	if (pMap == NULL) {
		return;
    }
	//gbInitObjFlag = true;
	lm = (uint16_t*)pMap;
	rw = SwapLE16(*lm);
	lm++;
	rh = SwapLE16(*lm);
	lm++;
	lm += rw * rh; // skip dun
	rw <<= 1;
	rh <<= 1;
	lm += 2 * rw * rh; // skip items?, monsters

	startx += DBORDERX;
	starty += DBORDERY;
	rw += startx;
	rh += starty;
	for (j = starty; j < rh; j++) {
		for (i = startx; i < rw; i++) {
			if (*lm != 0) {
				assert(SwapLE16(*lm) < lengthof(ObjConvTbl) && ObjConvTbl[SwapLE16(*lm)] != 0);
//				assert(objanimdata[objectdata[ObjConvTbl[SwapLE16(*lm)]].ofindex] != NULL);
				oi = AddObject(ObjConvTbl[SwapLE16(*lm)], i, j);
				if (lvrRect != NULL)
					SetObjMapRange(oi, lvrRect->x1, lvrRect->y1, lvrRect->x2, lvrRect->y2, lvrRect->leveridx);
			}
			lm++;
		}
	}
	//gbInitObjFlag = false;

	mem_free_dbg(pMap);
}

static void LoadMapSetObjs(const char* map)
{
	LoadMapSetObjects(map, 2 * setpc_x, 2 * setpc_y, NULL);
}

static void SetupObject(int oi, int x, int y, int type)
{
	ObjectStruct* os;
	const ObjectData* ods;
	const ObjFileData* ofd;

	os = &objects[oi];
	os->_ox = x;
	os->_oy = y;
	os->_otype = type;
	ods = &objectdata[type];
	os->_oSelFlag = ods->oSelFlag;
	os->_oDoorFlag = ods->oDoorFlag;
//	os->_oProc = ods->oProc;
	os->_oAnimFrame = ods->oAnimBaseFrame;
//	os->_oAnimData = objanimdata[ods->ofindex];
	ofd = &objfiledata[ods->ofindex];
//	os->_oSFX = ofd->oSFX;
//	os->_oSFXCnt = ofd->oSFXCnt;
	os->_oAnimFlag = ofd->oAnimFlag;
	os->_oAnimFrameLen = ofd->oAnimFrameLen;
	os->_oAnimLen = ofd->oAnimLen;
	//os->_oAnimCnt = 0;
	if (ofd->oAnimFlag != OAM_NONE) {
		os->_oAnimCnt = random_low(146, os->_oAnimFrameLen);
		os->_oAnimFrame = RandRangeLow(1, os->_oAnimLen);
	}
//	os->_oAnimWidth = ofd->oAnimWidth * ASSET_MPL;
//	os->_oAnimXOffset = (os->_oAnimWidth - TILE_WIDTH) >> 1;
	os->_oSolidFlag = ofd->oSolidFlag;
	os->_oMissFlag = ofd->oMissFlag;
	os->_oLightFlag = ofd->oLightFlag;
	os->_oBreak = ofd->oBreak;
	// os->_oDelFlag = FALSE; - unused
	os->_oPreFlag = FALSE;
	os->_oTrapChance = 0;
}

static void AddDiabObjs()
{
	LeverRect lr;
	lr = { DIAB_QUAD_2X, DIAB_QUAD_2Y, DIAB_QUAD_2X + 11, DIAB_QUAD_2Y + 12, 1 };
	LoadMapSetObjects("Levels\\L4Data\\diab1.DUN", 2 * DIAB_QUAD_1X, 2 * DIAB_QUAD_1Y, &lr);
	lr = { DIAB_QUAD_3X, DIAB_QUAD_3Y, DIAB_QUAD_3X + 11, DIAB_QUAD_3Y + 11, 2 };
	LoadMapSetObjects("Levels\\L4Data\\diab2a.DUN", 2 * DIAB_QUAD_2X, 2 * DIAB_QUAD_2Y, &lr);
	lr = { DIAB_QUAD_4X, DIAB_QUAD_4Y, DIAB_QUAD_4X + 9, DIAB_QUAD_4Y + 9, 3 };
	LoadMapSetObjects("Levels\\L4Data\\diab3a.DUN", 2 * DIAB_QUAD_3X, 2 * DIAB_QUAD_3Y, &lr);
}

static void AddHBooks(int bookidx, int ox, int oy)
{
	ObjectStruct* os;
	constexpr int bookframe = 1;
	int oi = AddObject(OBJ_L5BOOK, ox, oy);

	if (oi == -1)
		return;

	os = &objects[oi];
	// os->_oVar1 = bookframe;
	os->_oAnimFrame = 5 - 2 * bookframe;
	os->_oVar4 = os->_oAnimFrame + 1; // STORY_BOOK_READ_FRAME
	if (bookidx >= QNB_BOOK_A) {
		os->_oVar2 = TEXT_BOOKA + bookidx - QNB_BOOK_A; // STORY_BOOK_MSG
		os->_oVar3 = 14;                                // STORY_BOOK_NAME
		os->_oVar8 = bookidx;                           // STORY_BOOK_NAKRUL_IDX
	} else {
		os->_oVar2 = TEXT_BOOK4 + bookidx; // STORY_BOOK_MSG
		os->_oVar3 = bookidx + 9;          // STORY_BOOK_NAME
		os->_oVar8 = 0;                    // STORY_BOOK_NAKRUL_IDX
	}
}

static void AddLvl2xBooks(int bookidx)
{
	POS32 pos = RndLoc7x5();

	if (pos.x == 0)
		return;

	AddHBooks(bookidx, pos.x, pos.y);
	AddObject(OBJ_L5CANDLE, pos.x - 2, pos.y + 1);
	AddObject(OBJ_L5CANDLE, pos.x - 2, pos.y);
	AddObject(OBJ_L5CANDLE, pos.x - 1, pos.y - 1);
	AddObject(OBJ_L5CANDLE, pos.x + 1, pos.y - 1);
	AddObject(OBJ_L5CANDLE, pos.x + 2, pos.y);
	AddObject(OBJ_L5CANDLE, pos.x + 2, pos.y + 1);
}

static void AddUberLever()
{
	int oi;

	oi = AddObject(OBJ_L5LEVER, 2 * setpc_x + DBORDERX + 7, 2 * setpc_y + DBORDERY + 5);
	SetObjMapRange(oi, setpc_x + 2, setpc_y + 2, setpc_x + 2, setpc_y + 3, 1);
}

static void AddLvl24Books()
{
	BYTE books[4];

	AddUberLever();
	switch (random_(0, 6)) {
	case 0:
		books[0] = QNB_BOOK_A; books[1] = QNB_BOOK_B; books[2] = QNB_BOOK_C; books[3] = 0;
		break;
	case 1:
		books[0] = QNB_BOOK_A; books[1] = QNB_BOOK_C; books[2] = QNB_BOOK_B; books[3] = 0;
		break;
	case 2:
		books[0] = QNB_BOOK_B; books[1] = QNB_BOOK_A; books[2] = QNB_BOOK_C; books[3] = 0;
		break;
	case 3:
		books[0] = QNB_BOOK_B; books[1] = QNB_BOOK_C; books[2] = QNB_BOOK_A; books[3] = 0;
		break;
	case 4:
		books[0] = QNB_BOOK_C; books[1] = QNB_BOOK_B; books[2] = QNB_BOOK_A; books[3] = 0;
		break;
	case 5:
		books[0] = QNB_BOOK_C; books[1] = QNB_BOOK_A; books[2] = QNB_BOOK_B; books[3] = 0;
		break;
	default:
		ASSUME_UNREACHABLE
		break;
	}
	AddHBooks(books[0], 2 * setpc_x + DBORDERX + 7, 2 * setpc_y + DBORDERY + 6);
	AddHBooks(books[1], 2 * setpc_x + DBORDERX + 6, 2 * setpc_y + DBORDERY + 3);
	AddHBooks(books[2], 2 * setpc_x + DBORDERX + 6, 2 * setpc_y + DBORDERY + 8);
}

static void Alloc2x2Obj(int oi)
{
	int ox, oy;

	ox = objects[oi]._ox;
	oy = objects[oi]._oy;
	oi = -(oi + 1);
	dObject[ox][oy - 1] = oi;
	dObject[ox - 1][oy] = oi;
	dObject[ox - 1][oy - 1] = oi;
}

static void AddStoryBook()
{
	POS32 pos = RndLoc7x5();

	if (pos.x == 0)
		return;

	AddObject(OBJ_STORYBOOK, pos.x, pos.y);
	AddObject(OBJ_STORYCANDLE, pos.x - 2, pos.y + 1);
	AddObject(OBJ_STORYCANDLE, pos.x - 2, pos.y);
	AddObject(OBJ_STORYCANDLE, pos.x - 1, pos.y - 1);
	AddObject(OBJ_STORYCANDLE, pos.x + 1, pos.y - 1);
	AddObject(OBJ_STORYCANDLE, pos.x + 2, pos.y);
	AddObject(OBJ_STORYCANDLE, pos.x + 2, pos.y + 1);
}

static void AddHookedBodies()
{
	int i, j, ttv, type;
	// TODO: straight loop (in dlrgs)?
	for (j = DBORDERY; j < DBORDERY + DSIZEY; j++) {
		for (i = DBORDERX; i < DBORDERX + DSIZEX; i++) {
			ttv = nTrapTable[dPiece[i][j]];
			if (ttv == PTT_NONE)
				continue;
			if (dFlags[i][j] & BFLAG_POPULATED)
				continue;
			type = random_(0, 32);
			if (type >= 3)
				continue;
			if (ttv == PTT_LEFT) {
				type = OBJ_TORTUREL1 + type;
			} else {
				// assert(ttv == PTT_RIGHT);
				type = OBJ_TORTURER1 + type;
			}
			AddObject(type, i, j);
		}
	}
}

static void AddL4Goodies()
{
	AddHookedBodies();
	InitRndLocObj(2 * 4, 6 * 4, OBJ_TNUDEM);
	InitRndLocObj(2 * 3, 6 * 3, OBJ_TNUDEW);
	InitRndLocObj(2, 6, OBJ_DECAP);
	InitRndLocObj(1, 3, OBJ_CAULDRON);
}

static void AddLazStand()
{
	POS32 pos;

	if (IsMultiGame) {
		AddObject(OBJ_ALTBOY, 2 * setpc_x + DBORDERX + 4, 2 * setpc_y + DBORDERY + 6);
		return;
	}
	pos = RndLoc6x7();
	if (pos.x == 0) {
		InitRndLocObj(1, 1, OBJ_LAZSTAND);
		return;
	}
	AddObject(OBJ_LAZSTAND, pos.x, pos.y);
	AddObject(OBJ_TNUDEM, pos.x, pos.y + 2);
	AddObject(OBJ_STORYCANDLE, pos.x + 1, pos.y + 2);
	AddObject(OBJ_TNUDEM, pos.x + 2, pos.y + 2);
	AddObject(OBJ_TNUDEW, pos.x, pos.y - 2);
	AddObject(OBJ_STORYCANDLE, pos.x + 1, pos.y - 2);
	AddObject(OBJ_TNUDEW, pos.x + 2, pos.y - 2);
	AddObject(OBJ_STORYCANDLE, pos.x - 1, pos.y - 1);
	AddObject(OBJ_TNUDEW, pos.x - 1, pos.y);
	AddObject(OBJ_STORYCANDLE, pos.x - 1, pos.y + 1);
}

void InitObjects()
{
	//gbInitObjFlag = true;
	switch (currLvl._dType) {
	case DTYPE_CATHEDRAL:
		if (currLvl._dLevelIdx == DLV_CATHEDRAL4)
			AddStoryBook();
		if (QuestStatus(Q_PWATER))
			AddCandles();
		if (setpc_type == SPT_BUTCHER) // QuestStatus(Q_BUTCHER)
			LoadMapSetObjs("Levels\\L1Data\\Butcher.DUN");
		if (setpc_type == SPT_BANNER) // QuestStatus(Q_BANNER)
			AddObject(OBJ_SIGNCHEST, 2 * setpc_x + DBORDERX + 10, 2 * setpc_y + DBORDERY + 3);
		InitRndSarcs(OBJ_SARC);
		AddL1Objs(DBORDERX, DBORDERY, DBORDERX + DSIZEX, DBORDERY + DSIZEY);
		break;
	case DTYPE_CATACOMBS:
		if (currLvl._dLevelIdx == DLV_CATACOMBS4)
			AddStoryBook();
		if (QuestStatus(Q_ROCK))
			InitRndLocObj5x5(OBJ_STAND);
		if (setpc_type == SPT_BCHAMB) { // QuestStatus(Q_BCHAMB)
			AddBookLever(OBJ_BOOK2R, -1, 0, setpc_x, setpc_y, setpc_w + setpc_x, setpc_h + setpc_y, Q_BCHAMB);
		}
		if (setpc_type == SPT_BLIND) { // QuestStatus(Q_BLIND)
			AddBookLever(OBJ_BLINDBOOK, -1, 0, setpc_x, setpc_y, setpc_w + setpc_x, setpc_h + setpc_y, Q_BLIND);
			// LoadMapSetObjs("Levels\\L2Data\\Blind2.DUN");
		}
		if (setpc_type == SPT_BLOOD) { // QuestStatus(Q_BLOOD)
			AddBookLever(OBJ_BLOODBOOK, 2 * setpc_x + DBORDERX + 9, 2 * setpc_y + DBORDERY + 24, 0, 0, 0, 0, Q_BLOOD); // NULL_LVR_EFFECT
			AddObject(OBJ_PEDISTAL, 2 * setpc_x + DBORDERX + 9, 2 * setpc_y + DBORDERY + 16);
		}
		AddL2Objs(DBORDERX, DBORDERY, DBORDERX + DSIZEX, DBORDERY + DSIZEY);
		AddL2Torches();
		break;
	case DTYPE_CAVES:
		if (currLvl._dLevelIdx == DLV_CAVES4)
			AddStoryBook();
		else if (currLvl._dLevelIdx == DLV_CAVES1 && !IsMultiGame)
			InitRndLocObj5x5(OBJ_SLAINHERO);
		if (QuestStatus(Q_MUSHROOM))
			InitRndLocObj5x5(OBJ_MUSHPATCH);
		AddL3Objs(DBORDERX, DBORDERY, DBORDERX + DSIZEX, DBORDERY + DSIZEY);
		break;
	case DTYPE_HELL:
		if (currLvl._dLevelIdx == DLV_HELL4) {
			AddDiabObjs();
			return;
		}
		if (setpc_type == SPT_WARLORD) { // QuestStatus(Q_WARLORD)
			AddBookLever(OBJ_STEELTOME, -1, 0, setpc_x + 7, setpc_y + 1, setpc_x + 7, setpc_y + 5, Q_WARLORD);
			LoadMapSetObjs("Levels\\L4Data\\Warlord.DUN");
		}
		if (setpc_type == SPT_BETRAYER) // QuestStatus(Q_BETRAYER)
			AddLazStand();
		AddL4Goodies();
	break;
	case DTYPE_CRYPT:
		switch (currLvl._dLevelIdx) {
		case DLV_CRYPT1:
			AddLvl2xBooks(QNB_BOOK_1);
			break;
		case DLV_CRYPT2:
			AddLvl2xBooks(QNB_BOOK_2);
			AddLvl2xBooks(QNB_BOOK_3);
			break;
		case DLV_CRYPT3:
			AddLvl2xBooks(QNB_BOOK_4);
			AddLvl2xBooks(QNB_BOOK_5);
			break;
		case DLV_CRYPT4:
			AddLvl24Books();
			break;
		default:
			ASSUME_UNREACHABLE
			break;
		}
		InitRndSarcs(OBJ_L5SARC);
		AddL5Objs(DBORDERX, DBORDERY, DBORDERX + DSIZEX, DBORDERY + DSIZEY);
		break;
	case DTYPE_NEST:
		break;
	default:
		ASSUME_UNREACHABLE
		break;
	}
	InitRndBarrels();
	InitRndLocObj(5, 10, OBJ_CHEST1);
	InitRndLocObj(3, 6, OBJ_CHEST2);
	InitRndLocObj(1, 5, OBJ_CHEST3);
	// TODO: use dType instead?
	if (currLvl._dDunType != DTYPE_HELL && currLvl._dDunType != DTYPE_CAVES)
		AddObjTraps();
	if (currLvl._dType != DTYPE_CATHEDRAL)
		AddChestTraps();
	//gbInitObjFlag = false;
}

void SetMapObjects(BYTE* pMap)
{
	int i, j;
	uint16_t rw, rh, *lm; // , *h;
//	bool fileload[NUM_OFILE_TYPES];
//	char filestr[DATA_ARCHIVE_MAX_PATH];

//	static_assert(false == 0, "SetMapObjects fills fileload with 0 instead of false values.");
//	memset(fileload, 0, sizeof(fileload));
	//gbInitObjFlag = true;

//	for (i = 0; i < NUM_OBJECTS; i++) { // TODO: use dType instead?
//		if (currLvl._dDunType == objectdata[i].oSetLvlType)
//			fileload[objectdata[i].ofindex] = true;
//	}
	if (pMap == NULL) {
		return;
    }

	lm = (uint16_t*)pMap;
	rw = SwapLE16(*lm);
	lm++;
	rh = SwapLE16(*lm);
	lm++;
	lm += rw * rh; // skip dun
	rw <<= 1;
	rh <<= 1;
	lm += 2 * rw * rh; // skip items?, monsters

//	h = lm;

//	for (j = 0; j < rh; j++) {
//		for (i = 0; i < rw; i++) {
//			if (*lm != 0) {
//				assert(SwapLE16(*lm) < lengthof(ObjConvTbl) && ObjConvTbl[SwapLE16(*lm)] != 0);
//				fileload[objectdata[ObjConvTbl[SwapLE16(*lm)]].ofindex] = true;
//			}
//			lm++;
//		}
//	}

//	for (i = 0; i < NUM_OFILE_TYPES; i++) {
//		if (!fileload[i])
//			continue;

//		snprintf(filestr, sizeof(filestr), "Objects\\%s.CEL", objfiledata[i].ofName);
//		assert(objanimdata[i] == NULL);
//		objanimdata[i] = LoadFileInMem(filestr);
//	}

//	lm = h;
	rw += DBORDERX;
	rh += DBORDERY;
	for (j = DBORDERY; j < rh; j++) {
		for (i = DBORDERX; i < rw; i++) {
			if (*lm != 0)
				AddObject(ObjConvTbl[SwapLE16(*lm)], i, j);
			lm++;
		}
	}
	//gbInitObjFlag = false;
}

/*static void DeleteObject_(int oi, int idx)
{
	//objectavail[MAXOBJECTS - numobjects] = oi;
	dObject[objects[oi]._ox][objects[oi]._oy] = 0;
	//objectavail[numobjects] = oi;
	numobjects--;
	if (numobjects > 0 && idx != numobjects)
		objectactive[idx] = objectactive[numobjects];
}*/

void SetObjMapRange(int oi, int x1, int y1, int x2, int y2, int v)
{
	ObjectStruct* os;

	os = &objects[oi];
	// LEVER_EFFECT
	os->_oVar1 = x1;
	os->_oVar2 = y1;
	os->_oVar3 = x2;
	os->_oVar4 = y2;
	// LEVER_INDEX
	os->_oVar8 = v;
}

static void AddChest(int oi)
{
	ObjectStruct* os;
	int num, rnum, itype;

	os = &objects[oi];
	if (random_(147, 2) == 0)
		os->_oAnimFrame += 3;
	os->_oRndSeed = NextRndSeed(); // CHEST_ITEM_SEED1
	//assert(os->_otype >= OBJ_CHEST1 && os->_otype <= OBJ_CHEST3
	//	|| os->_otype >= OBJ_TCHEST1 && os->_otype <= OBJ_TCHEST3);
	num = os->_otype;
	num = (num >= OBJ_TCHEST1 && num <= OBJ_TCHEST3) ? num - OBJ_TCHEST1 + 1 : num - OBJ_CHEST1 + 1;
	rnum = random_low(147, num + 1); // CHEST_ITEM_SEED2
	if (!currLvl._dSetLvl)
		num = rnum;
	os->_oVar1 = num;        // CHEST_ITEM_NUM
	itype = random_(147, 8); // CHEST_ITEM_SEED3
	if (currLvl._dSetLvl)
		itype = 8;
	os->_oVar2 = itype;      // CHEST_ITEM_TYPE
	//assert(num <= 3); otherwise the seeds are not 'reserved'
}

static void AddDoor(int oi)
{
	ObjectStruct* os;
	int x, y, bx, by;

	os = &objects[oi];
	x = os->_ox;
	y = os->_oy;
	os->_oVar4 = DOOR_CLOSED;
	//os->_oPreFlag = FALSE;
	//os->_oSelFlag = 3;
	//os->_oSolidFlag = FALSE; // TODO: should be TRUE;
	//os->_oMissFlag = FALSE;
	//os->_oDoorFlag = ldoor ? ODT_LEFT : ODT_RIGHT;
	os->_oVar1 = dPiece[x][y]; // DOOR_PIECE_CLOSED
	// DOOR_SIDE_PIECE_CLOSED
	bx = x;
	by = y;
	if (os->_oDoorFlag == ODT_LEFT)
		by--;
	else
		bx--;
	os->_oVar2 = dPiece[bx][by];
}

static void AddSarc(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	dObject[os->_ox][os->_oy - 1] = -(oi + 1);
	os->_oVar1 = random_(153, 10);       // SARC_ITEM
	os->_oRndSeed = NextRndSeed();
	if (os->_oVar1 >= 8)
		os->_oVar2 = PreSpawnSkeleton(); // SARC_SKELE
}

static void AddTrap(int oi)
{
	ObjectStruct* os;
	int mt;

	mt = currLvl._dLevel;
	mt = mt / 6 + 1;
	mt = random_low(148, mt) & 3;
	os = &objects[oi];
	os->_oRndSeed = NextRndSeed();
	// TRAP_MISTYPE
	os->_oVar3 = MIS_ARROW;
	if (mt == 1)
		os->_oVar3 = MIS_FIREBOLT;
	else if (mt == 2)
		os->_oVar3 = MIS_LIGHTNINGC;
	os->_oVar4 = TRAP_ACTIVE; // TRAP_LIVE
}

static void AddObjLight(int oi, int diffr, int dx, int dy)
{
	ObjectStruct* os;

	os = &objects[oi];
	DoLighting(os->_ox + dx, os->_oy + dy, diffr, NO_LIGHT);
}

static void AddBarrel(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	os->_oRndSeed = NextRndSeed();
	os->_oVar3 = random_(149, 3);  // BARREL_ITEM_TYPE
	os->_oVar2 = random_(149, 10); // BARREL_ITEM
	if (os->_oVar2 >= 8)
		os->_oVar4 = PreSpawnSkeleton(); // BARREL_SKELE
}

static int FindValidShrine(int filter)
{
	int rv;
	BYTE excl = IsMultiGame ? SHRINETYPE_SINGLE : SHRINETYPE_MULTI;

	while (TRUE) {
		rv = random_(0, NUM_SHRINETYPE);
		if (rv != filter && shrineavail[rv] != excl)
			break;
	}
	return rv;
}

static void AddShrine(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	os->_oPreFlag = TRUE;
	os->_oRndSeed = NextRndSeed();
	os->_oVar1 = FindValidShrine(NUM_SHRINETYPE); // SHRINE_TYPE
	if (random_(150, 2) != 0) {
		os->_oAnimFrame = 12;
		os->_oAnimLen = 22;
	}
}

static void AddBookcase(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	os->_oRndSeed = NextRndSeed();
	os->_oPreFlag = TRUE;
}

static void ObjAddRndSeed(int oi)
{
	objects[oi]._oRndSeed = NextRndSeed();
}

static void AddArmorStand(int oi)
{
	objects[oi]._oMissFlag = TRUE;
}

static void AddCauldronGoatShrine(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	os->_oRndSeed = NextRndSeed();
	os->_oVar1 = FindValidShrine(SHRINE_THAUMATURGIC); // SHRINE_TYPE
}

static void AddDecap(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	os->_oRndSeed = NextRndSeed();
	os->_oAnimFrame = RandRange(1, 8);
	os->_oPreFlag = TRUE;
}

static void AddVileBook(int oi)
{
	if (currLvl._dLevelIdx == SL_VILEBETRAYER) {
		objects[oi]._oAnimFrame = 4;
	}
}

static void AddMagicCircle(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	//os->_oRndSeed = NextRndSeed();
	os->_oPreFlag = TRUE;
	os->_oVar5 = 0; // VILE_CIRCLE_PROGRESS
}

static void AddStoryBook(int oi)
{
	ObjectStruct* os;
	BYTE bookframe, idx;

	static_assert((int)DLV_CATHEDRAL4 == 4, "AddStoryBook converts DLV to index with shift I.");
	static_assert((int)DLV_CATACOMBS4 == 8, "AddStoryBook converts DLV to index with shift II.");
	static_assert((int)DLV_CAVES4 == 12, "AddStoryBook converts DLV to index with shift III.");
	idx = (currLvl._dLevelIdx >> 2) - 1;
	bookframe = quests[Q_DIABLO]._qvar1;

	os = &objects[oi];
	// os->_oVar1 = bookframe;
	os->_oVar2 = StoryText[bookframe][idx]; // STORY_BOOK_MSG
	os->_oVar3 = 3 * bookframe + idx;       // STORY_BOOK_NAME
	os->_oAnimFrame = 5 - 2 * bookframe;    //
	os->_oVar4 = os->_oAnimFrame + 1;       // STORY_BOOK_READ_FRAME
}

static void AddWeaponRack(int oi)
{
	objects[oi]._oMissFlag = TRUE;
}

static void AddTorturedMaleBody(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	//os->_oRndSeed = NextRndSeed();
	os->_oAnimFrame = RandRange(1, 4);
	//os->_oPreFlag = TRUE;
}

static void AddTorturedFemaleBody(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	//os->_oRndSeed = NextRndSeed();
	os->_oAnimFrame = RandRange(1, 3);
	//os->_oPreFlag = TRUE;
}

int AddObject(int type, int ox, int oy)
{
	int oi;

	if (numobjects >= MAXOBJECTS)
		return -1;

//	oi = objectavail[0];
	oi = numobjects;
	objectactive[numobjects] = oi;
	numobjects++;
//	objectavail[0] = objectavail[MAXOBJECTS - numobjects];
	assert(dObject[ox][oy] == 0);
	dObject[ox][oy] = oi + 1;
	SetupObject(oi, ox, oy, type);
	switch (type) {
	case OBJ_L1LIGHT:
		AddObjLight(oi, 10, 0, 0);
		break;
	case OBJ_SKFIRE:
	//case OBJ_CANDLE1:
	case OBJ_CANDLE2:
	case OBJ_BOOKCANDLE:
		AddObjLight(oi, 5, 0, 0);
		break;
	case OBJ_STORYCANDLE:
	case OBJ_L5CANDLE:
		AddObjLight(oi, 3, 0, 0);
		break;
	case OBJ_TORCHL1:
		AddObjLight(oi, 8, 1, 0);
		break;
	case OBJ_TORCHR1:
		AddObjLight(oi, 8, 0, 1);
		break;
	case OBJ_TORCHR2:
	case OBJ_TORCHL2:
		AddObjLight(oi, 8, 0, 0);
		break;
	case OBJ_L1LDOOR:
	case OBJ_L1RDOOR:
	case OBJ_L2LDOOR:
	case OBJ_L2RDOOR:
	case OBJ_L3LDOOR:
	case OBJ_L3RDOOR:
	case OBJ_L5LDOOR:
	case OBJ_L5RDOOR:
		AddDoor(oi);
		break;
	case OBJ_CHEST1:
	case OBJ_CHEST2:
	case OBJ_CHEST3:
		AddChest(oi);
		break;
	case OBJ_TCHEST1:
	case OBJ_TCHEST2:
	case OBJ_TCHEST3:
		AddChest(oi);
		objects[oi]._oTrapChance = RandRange(1, 64);
		objects[oi]._oVar5 = 0; // TRAP_OI_BACKREF
		break;
	case OBJ_SARC:
	case OBJ_L5SARC:
		AddSarc(oi);
		break;
	case OBJ_TRAPL:
	case OBJ_TRAPR:
		AddTrap(oi);
		break;
	case OBJ_BARREL:
	case OBJ_URN:
	case OBJ_POD:
		AddBarrel(oi);
		break;
	case OBJ_SHRINEL:
	case OBJ_SHRINER:
		AddShrine(oi);
		break;
	case OBJ_BOOKCASEL:
	case OBJ_BOOKCASER:
		AddBookcase(oi);
		break;
	case OBJ_DECAP:
		AddDecap(oi);
		break;
	case OBJ_BARRELEX:
	case OBJ_URNEX:
	case OBJ_PODEX:
	case OBJ_BOOKSTAND:
	case OBJ_SKELBOOK:
	case OBJ_BLOODBOOK:
	case OBJ_PEDISTAL:
	case OBJ_ARMORSTAND:
	case OBJ_WEAPONRACKL:
	case OBJ_WEAPONRACKR:
	case OBJ_SLAINHERO:
		ObjAddRndSeed(oi);
		break;
	case OBJ_ARMORSTANDN:
		AddArmorStand(oi);
		break;
	case OBJ_WEAPONRACKLN:
	case OBJ_WEAPONRACKRN:
		AddWeaponRack(oi);
		break;
	case OBJ_GOATSHRINE:
	case OBJ_CAULDRON:
		AddCauldronGoatShrine(oi);
		break;
	case OBJ_PURIFYINGFTN:
	case OBJ_MURKYFTN:
	case OBJ_MUSHPATCH:
		Alloc2x2Obj(oi);
		break;
	//case OBJ_TEARFTN:
	//	ObjAddRndSeed(oi);
	//	break;
	case OBJ_BOOK2L:
		AddVileBook(oi);
		break;
	case OBJ_MCIRCLE1:
	case OBJ_MCIRCLE2:
		AddMagicCircle(oi);
		break;
	case OBJ_STORYBOOK:
		AddStoryBook(oi);
		break;
	case OBJ_TBCROSS:
		// ObjAddRndSeed(oi);
		AddObjLight(oi, 10, 0, 0);
		break;
	case OBJ_TNUDEM:
		AddTorturedMaleBody(oi);
		break;
	case OBJ_TNUDEW:
		AddTorturedFemaleBody(oi);
		break;
	}
	return oi;
}
