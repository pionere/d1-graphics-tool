/**
 * @file objects.cpp
 *
 * Implementation of object functionality, interaction, spawning, loading, etc.
 */
#include "all.h"

#include <QApplication>

#include "../progressdialog.h"

DEVILUTION_BEGIN_NAMESPACE

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
// static BYTE* objanimdata[NUM_OFILE_TYPES] = { 0 };
// static int objanimdim[NUM_OFILE_TYPES];
// int objectactive[MAXOBJECTS];
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
const char* const BookName[NUM_BOOKS] = {
/*BK_STORY_MAINA_1*/  "The Great Conflict",
/*BK_STORY_MAINA_2*/  "The Wages of Sin are War",
/*BK_STORY_MAINA_3*/  "The Tale of the Horadrim",
/*BK_STORY_MAINB_1*/  "The Dark Exile",
/*BK_STORY_MAINB_2*/  "The Sin War",
/*BK_STORY_MAINB_3*/  "The Binding of the Three",
/*BK_STORY_MAINC_1*/  "The Realms Beyond",
/*BK_STORY_MAINC_2*/  "Tale of the Three",
/*BK_STORY_MAINC_3*/  "The Black King",
/*BK_BLOOD*/          "Book of Blood",
/*BK_ANCIENT*/        "Ancient Book",
/*BK_STEEL*/          "Steel Tome",
/*BK_BLIND*/          "Book of the Blind",
/*BK_MYTHIC*/         "Mythical Book",
/*BK_VILENESS*/       "Book of Vileness",
#ifdef HELLFIRE
	//"Journal: The Ensorcellment",
/*BK_STORY_NAKRUL_1*/ "Journal: The Meeting",
/*BK_STORY_NAKRUL_2*/ "Journal: The Tirade",
/*BK_STORY_NAKRUL_3*/ "Journal: His Power Grows",
/*BK_STORY_NAKRUL_4*/ "Journal: NA-KRUL",
/*BK_STORY_NAKRUL_5*/ "Journal: The End",
/*BK_NAKRUL_SPELL*/   "Spellbook",
#endif
};

static void AddObjectType(const ObjectData* ods)
{
#if 0
	const int ofindex = ods->ofindex;
	char filestr[DATA_ARCHIVE_MAX_PATH];

	if (objanimdata[ofindex] != NULL) {
		return;
	}

	snprintf(filestr, sizeof(filestr), "Objects\\%s.CEL", objfiledata[ofindex].ofName);
	objanimdata[ofindex] = LoadFileInMem(filestr);
	objanimdim[ofindex] = CelClippedWidth(objanimdata[ofindex]);
#endif
}

void InitObjectGFX()
{
	/*const ObjectData* ods;
	bool themeload[NUM_THEMES];
	int i;

	static_assert(false == 0, "InitObjectGFX fills themeload with 0 instead of false values.");
	memset(themeload, 0, sizeof(themeload));

	for (i = 0; i < numthemes; i++)
		themeload[themes[i]._tsType] = true;

	BYTE lvlMask = 1 << currLvl._dType;
	for (i = NUM_OBJECTS - 1; i >= 0; i--) {
		ods = &objectdata[i];
		if (!(ods->oLvlTypes & lvlMask)
		 && (ods->otheme == THEME_NONE || !themeload[ods->otheme])
		 && (ods->oquest == Q_INVALID || !QuestStatus(ods->oquest))) {
			continue;
		}
		AddObjectType(ods);
	}*/
}

void FreeObjectGFX()
{
	/*int i;

	for (i = 0; i < NUM_OFILE_TYPES; i++) {
		MemFreeDbg(objanimdata[i]);
	}*/
}

void InitLvlObjects()
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

static void SetObjMapRange(int oi, int x1, int y1, int x2, int y2, int v)
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

/**
 * Check the location if an object can be placed there in the init phase.
 * Must not consider the player's position, since it could change the dungeon
 * when a player re-enters the dungeon.
 */
static bool RndLocOk(int xp, int yp)
{
	if ((dMonster[xp][yp] |/* dPlayer[xp][yp] |*/ dObject[xp][yp]
	 | nSolidTable[dPiece[xp][yp]] | (dFlags[xp][yp] & BFLAG_OBJ_PROTECT)) != 0)
		return false;
	// should be covered by Freeupstairs.
	//if (currLvl._dDunType != DGT_CATHEDRAL || dPiece[xp][yp] <= 126 || dPiece[xp][yp] >= 144)
		return true;
	//return false;
}

static POS32 RndLoc3x3()
{
	int xp, yp, i, j, tries;
	static_assert(DBORDERX != 0, "RndLoc3x3 returns 0;0 position as a failed location.");
	tries = 0;
	while (true) {
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
	while (true) {
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
	while (true) {
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
	while (true) {
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

static void InitRndLocObj(int numobjs, int objtype)
{
	int i;
	POS32 pos;

	for (i = numobjs; i > 0; i--) {
		pos = RndLoc3x3();
		if (pos.x == 0)
			break;
		AddObject(objtype, pos.x, pos.y);
	}
}

static void InitRndSarcs(int numobjs, int objtype)
{
	int i;
	POS32 pos;

	for (i = numobjs; i > 0; i--) {
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

static void AddCandles()
{
	int tx, ty;

	tx = pWarps[DWARP_SIDE]._wx;
	ty = pWarps[DWARP_SIDE]._wy;
	AddObject(OBJ_STORYCANDLE, tx - 2, ty + 1);
	AddObject(OBJ_STORYCANDLE, tx + 2, ty + 1);
	AddObject(OBJ_STORYCANDLE, tx - 1, ty + 2);
	AddObject(OBJ_STORYCANDLE, tx + 1, ty + 2);
}

static void AddBookLever(int type, int x1, int y1, int x2, int y2, int qn)
{
	int oi;
	POS32 pos;

	pos = RndLoc5x5();
	if (pos.x == 0)
		return;

	oi = AddObject(type, pos.x, pos.y);
	SetObjMapRange(oi, x1, y1, x2, y2, leverid);
	leverid++;
	objects[oi]._oVar6 = objects[oi]._oGfxFrame + 1; // LEVER_BOOK_ANIM
	objects[oi]._oVar7 = qn; // LEVER_BOOK_QUEST
}

// generate numobjs groups of barrels
static void InitRndBarrels(int numobjs, int otype)
{
	int i, xp, yp;
	int dir;
	int t; // number of tries of placing next barrel in current group
	int c; // number of barrels in current group

	// assert(otype == OBJ_BARREL || otype == OBJ_URN || otype == OBJ_POD);
	static_assert((int)OBJ_BARREL + 1 == (int)OBJ_BARRELEX, "InitRndBarrels expects ordered BARREL enum I.");
#ifdef HELLFIRE
	static_assert((int)OBJ_URN + 1 == (int)OBJ_URNEX, "InitRndBarrels expects ordered BARREL enum II.");
	static_assert((int)OBJ_POD + 1 == (int)OBJ_PODEX, "InitRndBarrels expects ordered BARREL enum III.");
#endif

	for (i = numobjs; i > 0; i--) {
		do {
			xp = random_(143, DSIZEX) + DBORDERX;
			yp = random_(143, DSIZEY) + DBORDERY;
		} while (!RndLocOk(xp, yp));
		AddObject(otype + (random_(143, 4) == 0 ? 1 : 0), xp, yp);
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
			AddObject(otype + (random_(143, 5) == 0 ? 1 : 0), xp, yp);
			c++;
		} while (random_low(143, c >> 1) == 0);
	}
}

static void AddDunObjs(int x1, int y1, int x2, int y2)
{
	int i, j, pn, wdoor, edoor, type;

	assert((objectdata[OBJ_L1LDOOR].oLvlTypes & DTM_CATHEDRAL) && (objectdata[OBJ_L1RDOOR].oLvlTypes & DTM_CATHEDRAL) && (objectdata[OBJ_L1LIGHT].oLvlTypes & DTM_CATHEDRAL));
	assert((objectdata[OBJ_L2LDOOR].oLvlTypes & DTM_CATACOMBS) && (objectdata[OBJ_L2RDOOR].oLvlTypes & DTM_CATACOMBS));
	assert((objectdata[OBJ_L3LDOOR].oLvlTypes & DTM_CAVES) && (objectdata[OBJ_L3RDOOR].oLvlTypes & DTM_CAVES));
#ifdef HELLFIRE
	assert((objectdata[OBJ_L5LDOOR].oLvlTypes & DTM_CRYPT) && (objectdata[OBJ_L5RDOOR].oLvlTypes & DTM_CRYPT));
#endif
	switch (currLvl._dType) {
	case DTYPE_TOWN:
		return;
	case DTYPE_CATHEDRAL:
		/*for (j = y1; j <= y2; j++) {
			for (i = x1; i <= x2; i++) {
				pn = dPiece[i][j];
				if (pn == 270)
					AddObject(OBJ_L1LIGHT, i, j);
			}
		}*/
		wdoor = OBJ_L1LDOOR;
		edoor = OBJ_L1RDOOR;
		break;
	case DTYPE_CATACOMBS:
		wdoor = OBJ_L2LDOOR;
		edoor = OBJ_L2RDOOR;
		break;
	case DTYPE_CAVES:
		wdoor = OBJ_L3LDOOR;
		edoor = OBJ_L3RDOOR;
		break;
	case DTYPE_HELL:
		return;
#ifdef HELLFIRE
	case DTYPE_CRYPT:
		wdoor = OBJ_L5LDOOR;
		edoor = OBJ_L5RDOOR;
		break;
	case DTYPE_NEST:
		return;
#endif
	default:
		ASSUME_UNREACHABLE
		return;
	}
	for (j = y1; j <= y2; j++) {
		for (i = x1; i <= x2; i++) {
			pn = dPiece[i][j];
			type = automaptype[pn] & MAT_TYPE;
			if ((type == MAT_DOOR_WEST || type == MAT_DOOR_EAST) && !nSolidTable[pn]) {
				dProgressErr() << QApplication::tr("Non-blocking door pn:%1 type:%2 tile:%3").arg(pn).arg(type).arg(dungeon[(i - DBORDERX) >> 1][(j - DBORDERY) >> 1]);
			}
			if (type == MAT_DOOR_WEST) {
				AddObject(wdoor, i, j);
			} else if (type == MAT_DOOR_EAST) {
				AddObject(edoor, i, j);
			}
		}
	}
}

static void AddL2Torches()
{
	int i, j;
	// place torches on NW->SE walls
	for (i = DBORDERX; i < DBORDERX + DSIZEX; i++) {
		for (j = DBORDERY; j < DBORDERY + DSIZEY; j++) {
			// skip setmap pieces
			if (dFlags[i][j] & BFLAG_OBJ_PROTECT)
				continue;
			// select 'trapable' position
			if ((nSpecTrapTable[dPiece[i][j]] & PST_TRAP_TYPE) != PST_LEFT)
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
	// place torches on NE->SW walls
	for (j = DBORDERY; j < DBORDERY + DSIZEY; j++) {
		for (i = DBORDERX; i < DBORDERX + DSIZEX; i++) {
			// skip setmap pieces
			if (dFlags[i][j] & BFLAG_OBJ_PROTECT)
				continue;
			// select 'trapable' position
			if ((nSpecTrapTable[dPiece[i][j]] & PST_TRAP_TYPE) != PST_RIGHT)
				continue;
			if (random_(145, 32) != 0)
				continue;
			// assert(nSolidTable[dPiece[i - 1][j]] | nSolidTable[dPiece[i + 1][j]]);
			if (!nSolidTable[dPiece[i][j + 1]]) {
				if (dObject[i][j] == 0) // check torches from the previous loop
					AddObject(OBJ_TORCHR1, i, j);
			} else {
				if (dObject[i][j - 1] == 0) // check torches from the previous loop
					AddObject(OBJ_TORCHR2, i, j - 1);
			}
			// skip a few tiles to prevent close placement
			i += 4;
		}
	}
}

static void AddObjTraps()
{
	int i, ox, oy, tx, ty, on, rndv;

	rndv = 10 + (currLvl._dLevel >> 1);
	for (i = numobjects - 1; i >= 0; i--) {
		int oi = i; // objectactive[i];
		ObjectStruct* os = &objects[oi];
		if (!objectdata[os->_otype].oTrapFlag)
			continue;
		if (random_(144, 128) >= rndv)
			continue;
		ox = os->_ox;
		oy = os->_oy;
		if (random_(144, 2) == 0) {
			tx = ox - 1;
			while (!nSolidTable[dPiece[tx][oy]])
				tx--;

			if (ox - tx <= 1)
				continue;

			ty = oy;
			on = OBJ_TRAPL;
		} else {
			ty = oy - 1;
			while (!nSolidTable[dPiece[ox][ty]])
				ty--;

			if (oy - ty <= 1)
				continue;

			tx = ox;
			on = OBJ_TRAPR;
		}
		if (dFlags[tx][ty] & BFLAG_OBJ_PROTECT)
			continue;
		if (dObject[tx][ty] != 0)
			continue;
		if ((nSpecTrapTable[dPiece[tx][ty]] & PST_TRAP_TYPE) == PST_NONE)
			continue;
		on = AddObject(on, tx, ty);
		if (on == -1)
			return;
		objects[on]._oVar1 = oi; // TRAP_OI_REF
		objects[oi]._oTrapChance = RandRange(1, 64);
		objects[oi]._oVar5 = on + 1; // TRAP_OI_BACKREF
	}
}

static void AddChestTraps()
{
	int i;

	for (i = numobjects - 1; i >= 0; i--) {
		int oi = i; // objectactive[i];
		ObjectStruct* os = &objects[oi];
		if (os->_otype >= OBJ_CHEST1 && os->_otype <= OBJ_CHEST3 && os->_oTrapChance == 0 && random_(0, 100) < 10) {
			os->_otype += OBJ_TCHEST1 - OBJ_CHEST1;
			os->_oTrapChance = RandRange(1, 64);
			os->_oVar5 = 0; // TRAP_OI_BACKREF
		}
	}
}

static void LoadMapSetObjects(int idx)
{
	int startx = DBORDERX + pSetPieces[idx]._spx * 2;
	int starty = DBORDERY + pSetPieces[idx]._spy * 2;
	const BYTE* pMap = pSetPieces[idx]._spData;
	int i, j;
	uint16_t rw, rh, *lm, oidx;

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

	rw += startx;
	rh += starty;
	for (j = starty; j < rh; j++) {
		for (i = startx; i < rw; i++) {
			if (*lm != 0) {
				oidx = SwapLE16(*lm);
				if (oidx >= lengthof(ObjConvTbl) || ObjConvTbl[oidx] == 0) {
					dProgressErr() << QApplication::tr("Invalid object %1 at %2:%3").arg(oidx).arg(i).arg(j);
				} else {
				oidx = ObjConvTbl[oidx];
				AddObjectType(&objectdata[oidx]);
				AddObject(oidx, i, j);
				}
			}
			lm++;
		}
	}
	//gbInitObjFlag = false;
}

std::pair<int, int> themeLoc(int x, int y)
{
	for (int i = 0; i < numthemes; i++) {
		if (themes[i]._tsx1 <= x && themes[i]._tsx2 > x && themes[i]._tsy1 <= y && themes[i]._tsy2 > y) {
			return std::pair<int, bool>(i, 2);
		}
		if (themes[i]._tsTransVal == dTransVal[x][y]) {
			return std::pair<int, bool>(i, 1);
		}
	}
	return std::pair<int, bool>(0, 0);
}

static int SetupObject(int type, int ox, int oy)
{
	int oi;
	ObjectStruct* os;
	const ObjectData* ods;
	const ObjFileData* ofd;

	if (numobjects >= MAXOBJECTS)
		return -1;

//	oi = objectavail[0];
	oi = numobjects;
	// objectactive[numobjects] = oi;
	numobjects++;
//	objectavail[0] = objectavail[MAXOBJECTS - numobjects];

	os = &objects[oi];
	os->_otype = type;
	ods = &objectdata[type];
	os->_oMissFlag = ods->oMissFlag;
	os->_oDoorFlag = ods->oDoorFlag;
	os->_oSelFlag = ods->oSelFlag;
	os->_oPreFlag = ods->oPreFlag;
	os->_oProc = ods->oProc;
	os->_oModeFlags = ods->oModeFlags;
	os->_oGfxFrame = ods->oBaseFrame;
//	os->_oAnimData = objanimdata[ods->ofindex];
	ofd = &objfiledata[ods->ofindex];
//	os->_oSFX = ofd->oSFX;
//	os->_oSFXCnt = ofd->oSFXCnt;
	os->_oAnimFlag = ofd->oAnimFlag;
	os->_oAnimFrameLen = ofd->oAnimFrameLen;
	os->_oAnimLen = ofd->oAnimLen;
	//os->_oAnimCnt = 0;
	os->_oAnimFrame = 0;
	if (ofd->oAnimFlag != OAM_NONE) {
		if (ofd->oAnimFlag == OAM_SINGLE) {
			os->_oAnimFlag = OAM_NONE;
			os->_oAnimFrame = 1;
		} else {
			// assert(ofd->oAnimFlag == OAM_LOOP);
			os->_oAnimCnt = random_low(146, os->_oAnimFrameLen);
			os->_oAnimFrame = RandRangeLow(1, os->_oAnimLen);
		}
	}
//	os->_oAnimWidth = objanimdim[ods->ofindex];
//	os->_oAnimXOffset = (os->_oAnimWidth - TILE_WIDTH) >> 1;
	os->_oSolidFlag = ofd->oSolidFlag;
	os->_oBreak = ofd->oBreak;
	// os->_oDelFlag = FALSE; - unused
	os->_oTrapChance = 0;
	// place object
	os->_ox = ox;
	os->_oy = oy;
	if (dObject[ox][oy] != 0) {
		int on = dObject[ox][oy];
		on = on >= 0 ? on - 1 : -(on + 1);
		std::pair<int, bool> tl = themeLoc(ox, oy);
		QString msg = QApplication::tr("Multiple objects on tile %1:%2 - type %3 with index %4 and type %5 with index %6. Theme loc %7:%8, pos %9:%10 .. %11:%12 tv %13");
		msg = msg.arg(ox).arg(oy).arg(type).arg(oi).arg(objects[on]._otype).arg(on);
		int i = tl.first;
		msg = msg.arg(i).arg(tl.second).arg(themes[i]._tsx1).arg(themes[i]._tsy1).arg(themes[i]._tsx2).arg(themes[i]._tsy2).arg(themes[i]._tsTransVal);
		dProgressErr() << msg;
	}
	dObject[ox][oy] = oi + 1;
	// dFlags[ox][oy] |= BFLAG_OBJ_PROTECT | BFLAG_MON_PROTECT;
	if (nSolidTable[dPiece[ox][oy]] && (os->_oModeFlags & OMF_FLOOR)) {
		dObject[ox][oy] = 0;
		os->_oModeFlags |= OMF_RESERVED;
		os->_oSelFlag = 0;
		dProgress() << QApplication::tr("Reserved object on tile %1:%2 - type %3 with index %4.").arg(ox).arg(oy).arg(type).arg(oi);
	} else {
		/*if (ods->oLightRadius != 0) {
#if FLICKER_LIGHT
			if (type == OBJ_L1LIGHT) {
				os->_olid = NO_LIGHT;
			} else
#endif
			{
				TraceLightSource(ox + ods->oLightOffX, oy + ods->oLightOffY, ods->oLightRadius);
			}
		}*/
		if (ods->oDoorFlag != ODT_NONE) {
			os->_oVar4 = DOOR_CLOSED;
			//os->_oPreFlag = FALSE;
			//os->_oSelFlag = 3;
			//os->_oSolidFlag = FALSE; // TODO: should be TRUE;
			//os->_oMissFlag = FALSE;
			//os->_oDoorFlag = ldoor ? ODT_LEFT : ODT_RIGHT;
			os->_oVar1 = dPiece[ox][oy]; // DOOR_PIECE_CLOSED
		}
	}
	return oi;
}

static int ObjIndex(int x, int y)
{
	int oi = dObject[x][y];
	if (oi == 0) {
		dProgressErr() << QApplication::tr("ObjIndex: Active object not found at (%1,%2)").arg(x).arg(y);
		return 0;
	}
	oi = oi >= 0 ? oi - 1 : -(oi + 1);
	return oi;
}

static void AddSKingObjs()
{
	SetObjMapRange(ObjIndex(DBORDERX + 48, DBORDERY + 18), 20, 7, 23, 10, 1);
	SetObjMapRange(ObjIndex(DBORDERX + 48, DBORDERY + 43), 20, 14, 21, 16, 2);
	SetObjMapRange(ObjIndex(DBORDERX + 11, DBORDERY + 21), 8, 1, 15, 11, 3);
	SetObjMapRange(ObjIndex(DBORDERX + 30, DBORDERY + 19), 8, 1, 15, 11, 3);
	SetObjMapRange(ObjIndex(DBORDERX + 33, DBORDERY + 37), 8, 1, 15, 11, 3);
	SetObjMapRange(ObjIndex(DBORDERX + 11, DBORDERY + 37), 8, 1, 15, 11, 3);
}

static void AddBChamObjs()
{
	SetObjMapRange(ObjIndex(DBORDERX + 21, DBORDERY + 14), 17, 0, 21, 5, 1);
	SetObjMapRange(ObjIndex(DBORDERX + 21, DBORDERY + 30), 13, 0, 16, 5, 2);
}

static void AddVileObjs()
{
	SetObjMapRange(ObjIndex(DBORDERX + 10, DBORDERY + 29), 3, 4, 8, 10, 1);
	SetObjMapRange(ObjIndex(DBORDERX + 29, DBORDERY + 30), 11, 4, 16, 10, 2);
	//SetObjMapRange(ObjIndex(DBORDERX + 19, DBORDERY + 20), 7, 11, 13, 18, 3);
}

/*static void AddMazeObjs()
{
	SetObjMapRange(ObjIndex(DBORDERX + 33, DBORDERY + 25), 0?, 0?, 45?, ?, 1);
	SetObjMapRange(ObjIndex(DBORDERX + 15, DBORDERY + 51), ?, ?, ?, ?, ?);
	SetObjMapRange(ObjIndex(DBORDERX + 27, DBORDERY + 51), ?, ?, ?, ?, ?);
	SetObjMapRange(ObjIndex(DBORDERX + 33, DBORDERY + 57), ?, ?, ?, ?, ?);
	SetObjMapRange(ObjIndex(DBORDERX + 79, DBORDERY + 51), ?, ?, ?, ?, ?);
}*/

static void AddDiabObjs()
{
	SetObjMapRange(ObjIndex(DBORDERX + 2 * pSetPieces[0]._spx + 5, DBORDERY + 2 * pSetPieces[0]._spy + 5), pSetPieces[1]._spx, pSetPieces[1]._spy, pSetPieces[1]._spx + 11, pSetPieces[1]._spy + 12, 1);
	SetObjMapRange(ObjIndex(DBORDERX + 2 * pSetPieces[1]._spx + 13, DBORDERY + 2 * pSetPieces[1]._spy + 10), pSetPieces[2]._spx, pSetPieces[2]._spy, pSetPieces[2]._spx + 11, pSetPieces[2]._spy + 11, 2);
	SetObjMapRange(ObjIndex(DBORDERX + 2 * pSetPieces[2]._spx + 8, DBORDERY + 2 * pSetPieces[2]._spy + 2), pSetPieces[3]._spx, pSetPieces[3]._spy, pSetPieces[3]._spx + 9, pSetPieces[3]._spy + 9, 3);
	SetObjMapRange(ObjIndex(DBORDERX + 2 * pSetPieces[2]._spx + 8, DBORDERY + 2 * pSetPieces[2]._spy + 14), pSetPieces[3]._spx, pSetPieces[3]._spy, pSetPieces[3]._spx + 9, pSetPieces[3]._spy + 9, 3);
}

#ifdef HELLFIRE
static void AddL5StoryBook(int bookidx, int ox, int oy)
{
	ObjectStruct* os;
	int oi = AddObject(OBJ_L5BOOK, ox, oy);
	// assert(oi != -1);

	os = &objects[oi];
	// assert(os->_oGfxFrame == objectdata[OBJ_L5BOOK].oBaseFrame);
	os->_oVar4 = objectdata[OBJ_L5BOOK].oBaseFrame + 1; // STORY_BOOK_READ_FRAME
	os->_oVar2 = TEXT_BOOK4 + bookidx;                  // STORY_BOOK_MSG
	os->_oVar5 = BK_STORY_NAKRUL_1 + bookidx;           // STORY_BOOK_NAME
}

static void AddNakrulBook(int oi)
{
	ObjectStruct* os;

	int bookidx;
	if (leverid == 1) {
		bookidx = random_(11, 3); // select base book index
		leverid = 16 | bookidx;   // store the selected book index + a flag
	} else {
		if ((leverid & 32) == 0) {
			bookidx = random_(12, 2); // select from the remaining two books
			leverid |= 32 | (bookidx << 2); // store second choice + a flag
			bookidx = (bookidx & 1) ? 4 : 2; // make the choice
		} else {
			bookidx = (leverid >> 2); // read second choice
			bookidx = (bookidx & 1) ? 2 : 4; // choose the remaining one
		}
		bookidx += leverid & 3; // add base book index
		bookidx = bookidx % 3;
	}
	bookidx += QNB_BOOK_A;

	os = &objects[oi];
	// assert(os->_oGfxFrame == objectdata[OBJ_NAKRULBOOK].oBaseFrame);
	os->_oVar4 = objectdata[OBJ_NAKRULBOOK].oBaseFrame + 1; // STORY_BOOK_READ_FRAME
	os->_oVar2 = TEXT_BOOKA + bookidx - QNB_BOOK_A;         // STORY_BOOK_MSG
	os->_oVar3 = bookidx;                                   // STORY_BOOK_NAKRUL_IDX
	os->_oVar5 = BK_NAKRUL_SPELL;                           // STORY_BOOK_NAME
}

static void AddLvl2xBooks(int bookidx)
{
	POS32 pos = RndLoc5x5();

	if (pos.x == 0)
		return;

	AddL5StoryBook(bookidx, pos.x, pos.y);
	AddObject(OBJ_L5CANDLE, pos.x - 1, pos.y - 1);
	AddObject(OBJ_L5CANDLE, pos.x - 1, pos.y + 1);
}
#endif

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
			ttv = nSpecTrapTable[dPiece[i][j]] & PST_TRAP_TYPE;
			if (ttv == PST_NONE)
				continue;
			if (dFlags[i][j] & BFLAG_OBJ_PROTECT)
				continue;
			type = random_(0, 32);
			if (type >= 3)
				continue;
			if (ttv == PST_LEFT) {
				type = OBJ_TORTUREL1 + type;
			} else {
				// assert(ttv == PST_RIGHT);
				type = OBJ_TORTURER1 + type;
			}
			AddObject(type, i, j);
		}
	}
}

void InitObjects()
{
	//gbInitObjFlag = true;
	if (QuestStatus(Q_ROCK)) // place first to make the life of PlaceRock easier
		InitRndLocObj5x5(OBJ_STAND);
	if (QuestStatus(Q_PWATER))
		AddCandles();
	if (QuestStatus(Q_MUSHROOM))
		InitRndLocObj5x5(OBJ_MUSHPATCH);
	for (int i = lengthof(pSetPieces) - 1; i >= 0; i--) {
		if (pSetPieces[i]._spData != NULL) { // pSetPieces[i]._sptype != SPT_NONE
			LoadMapSetObjects(i);
		}
	}
	if (pSetPieces[0]._sptype == SPT_WARLORD) { // QuestStatus(Q_WARLORD)
		AddBookLever(OBJ_STEELTOME, pSetPieces[0]._spx + 7, pSetPieces[0]._spy + 1, pSetPieces[0]._spx + 7, pSetPieces[0]._spy + 5, Q_WARLORD);
	}
	if (pSetPieces[0]._sptype == SPT_BCHAMB) { // QuestStatus(Q_BCHAMB)
		AddBookLever(OBJ_MYTHICBOOK, pSetPieces[0]._spx, pSetPieces[0]._spy, pSetPieces[0]._spx + 5, pSetPieces[0]._spy + 5, Q_BCHAMB);
	}
	if (pSetPieces[0]._sptype == SPT_BLIND) { // QuestStatus(Q_BLIND)
		AddBookLever(OBJ_BLINDBOOK, pSetPieces[0]._spx, pSetPieces[0]._spy + 1, pSetPieces[0]._spx + 11, pSetPieces[0]._spy + 10, Q_BLIND);
	}
	if (pSetPieces[0]._sptype == SPT_LVL_SKELKING) {
		AddSKingObjs();
	}
	if (pSetPieces[0]._sptype == SPT_LVL_BCHAMB) {
		AddBChamObjs();
	}
	if (pSetPieces[0]._sptype == SPT_LVL_BETRAYER) {
		AddVileObjs();
	}
	switch (currLvl._dLevelIdx) {
	case DLV_CATHEDRAL4:
		AddStoryBook();
		break;
	case DLV_CATACOMBS4:
		AddStoryBook();
		break;
	case DLV_CAVES4:
		AddStoryBook();
		break;
	case DLV_HELL4:
		AddDiabObjs();
		return;
#ifdef HELLFIRE
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
#endif
	}
	AddDunObjs(DBORDERX, DBORDERY, MAXDUNX - DBORDERX - 1, MAXDUNY - DBORDERY - 1);
	static_assert(NUM_DTYPES <= sizeof(BYTE) * 8, "Level mask might overflow in InitObjects.");
	BYTE lvlMask = 1 << currLvl._dType;
	assert(objectdata[OBJ_TORCHL1].oLvlTypes == objectdata[OBJ_TORCHL2].oLvlTypes && objectdata[OBJ_TORCHL1].oLvlTypes == objectdata[OBJ_TORCHR1].oLvlTypes && objectdata[OBJ_TORCHR1].oLvlTypes == objectdata[OBJ_TORCHR2].oLvlTypes);
	if (lvlMask & objectdata[OBJ_TORCHL1].oLvlTypes) {
		AddL2Torches();
	}
	assert(objectdata[OBJ_TORTUREL1].oLvlTypes == objectdata[OBJ_TORTUREL2].oLvlTypes);
	assert(objectdata[OBJ_TORTUREL1].oLvlTypes == objectdata[OBJ_TORTUREL3].oLvlTypes);
	assert(objectdata[OBJ_TORTUREL1].oLvlTypes == objectdata[OBJ_TORTURER1].oLvlTypes);
	assert(objectdata[OBJ_TORTUREL1].oLvlTypes == objectdata[OBJ_TORTURER2].oLvlTypes);
	assert(objectdata[OBJ_TORTUREL1].oLvlTypes == objectdata[OBJ_TORTURER3].oLvlTypes);
	if (lvlMask & objectdata[OBJ_TORTUREL1].oLvlTypes) {
		AddHookedBodies();
	}

	unsigned na = 0;
	for (int xx = DBORDERX; xx < DSIZEX + DBORDERX; xx++)
		for (int yy = DBORDERY; yy < DSIZEY + DBORDERY; yy++)
			if ((nSolidTable[dPiece[xx][yy]] | (dFlags[xx][yy] & BFLAG_OBJ_PROTECT)) == 0)
				na++;
	na = na * AllLevels[currLvl._dLevelNum].dObjDensity / 32;

	if (lvlMask & objectdata[OBJ_L1LIGHT].oLvlTypes) {
		static_assert(DSIZEX * DSIZEY < 0x7FFF, "InitObjects uses RandRangeLow.");
		unsigned num = RandRangeLow(na, na * 2 + 1);
		InitRndLocObj(num / 512, OBJ_L1LIGHT);
	}
	if (lvlMask & objectdata[OBJ_SARC].oLvlTypes) {
		static_assert(DSIZEX * DSIZEY < 0x7FFF, "InitObjects uses RandRangeLow I.");
		unsigned num = RandRangeLow(na, na * 2 + 1);
		InitRndSarcs(num / 512, OBJ_SARC);
	}
#ifdef HELLFIRE
	if (lvlMask & objectdata[OBJ_L5SARC].oLvlTypes) {
		static_assert(DSIZEX * DSIZEY < 0x7FFF, "InitObjects uses RandRangeLow II.");
		unsigned num = RandRangeLow(na, na * 2 + 1);
		InitRndSarcs(num / 512, OBJ_L5SARC);
	}
#endif
	if (lvlMask & objectdata[OBJ_TNUDEM].oLvlTypes) {
		static_assert(DSIZEX * DSIZEY * 2 < 0x7FFF, "InitObjects uses RandRangeLow III.");
		unsigned num = RandRangeLow(na, na * 3 + 1);
		InitRndLocObj(num / 512, OBJ_TNUDEM);
	}
	if (lvlMask & objectdata[OBJ_TNUDEW].oLvlTypes) {
		static_assert(DSIZEX * DSIZEY * 2 < 0x7FFF, "InitObjects uses RandRangeLow IV.");
		unsigned num = RandRangeLow(na, na * 3 + 1);
		InitRndLocObj(num / 768, OBJ_TNUDEW);
	}
	if (lvlMask & objectdata[OBJ_DECAP].oLvlTypes) {
		static_assert(DSIZEX * DSIZEY < 0x7FFF, "InitObjects uses RandRangeLow V.");
		unsigned num = RandRangeLow(na, na * 2 + 1);
		InitRndLocObj(num / 1024, OBJ_DECAP);
	}
	if (lvlMask & objectdata[OBJ_CAULDRON].oLvlTypes) {
		static_assert(DSIZEX * DSIZEY < 0x7FFF, "InitObjects uses RandRangeLow VI.");
		unsigned num = RandRangeLow(na, na * 2 + 1);
		InitRndLocObj(num / 2048, OBJ_CAULDRON);
	}
	assert(objectdata[OBJ_BARREL].oLvlTypes == objectdata[OBJ_BARRELEX].oLvlTypes);
	if (lvlMask & objectdata[OBJ_BARREL].oLvlTypes) {
		static_assert(DSIZEX * DSIZEY < 0x7FFF, "InitObjects uses RandRangeLow VII.");
		unsigned num = RandRangeLow(na, na * 2 + 1);
		InitRndBarrels(num / 1024, OBJ_BARREL);
	}
#ifdef HELLFIRE
	assert(objectdata[OBJ_URN].oLvlTypes == objectdata[OBJ_URNEX].oLvlTypes);
	if (lvlMask & objectdata[OBJ_URN].oLvlTypes) {
		static_assert(DSIZEX * DSIZEY < 0x7FFF, "InitObjects uses RandRangeLow VIII.");
		unsigned num = RandRangeLow(na, na * 2 + 1);
		InitRndBarrels(num / 1024, OBJ_URN);
	}
	assert(objectdata[OBJ_POD].oLvlTypes == objectdata[OBJ_PODEX].oLvlTypes);
	if (lvlMask & objectdata[OBJ_POD].oLvlTypes) {
		static_assert(DSIZEX * DSIZEY < 0x7FFF, "InitObjects uses RandRangeLow IX.");
		unsigned num = RandRangeLow(na, na * 2 + 1);
		InitRndBarrels(num / 1024, OBJ_POD);
	}
#endif
	assert(objectdata[OBJ_CHEST1].oLvlTypes == objectdata[OBJ_CHEST2].oLvlTypes && objectdata[OBJ_CHEST1].oLvlTypes == objectdata[OBJ_CHEST3].oLvlTypes);
	if (lvlMask & objectdata[OBJ_TCHEST1].oLvlTypes) {
		static_assert(DSIZEX * DSIZEY < 0x7FFF, "InitObjects uses RandRangeLow X.");
		unsigned num = RandRangeLow(na, na * 2 + 1);
		InitRndLocObj(num / 512, OBJ_CHEST1);
		num = RandRangeLow(na, na * 2 + 1);
		InitRndLocObj(num / 1024, OBJ_CHEST2);
		num = RandRangeLow(na, na * 2 + 1);
		InitRndLocObj(num / 2048, OBJ_CHEST3);
	}
	assert(objectdata[OBJ_TRAPL].oLvlTypes == objectdata[OBJ_TRAPR].oLvlTypes);
	if (lvlMask & objectdata[OBJ_TRAPL].oLvlTypes) {
		AddObjTraps();
	}
	assert(objectdata[OBJ_TCHEST1].oLvlTypes == objectdata[OBJ_TCHEST2].oLvlTypes && objectdata[OBJ_TCHEST1].oLvlTypes == objectdata[OBJ_TCHEST3].oLvlTypes); // trapped chest are placed in one place
	assert((objectdata[OBJ_TCHEST1].oLvlTypes & ~objectdata[OBJ_CHEST1].oLvlTypes) == 0); // no point to place traps if there are not matching chests
	assert((objectdata[OBJ_TCHEST2].oLvlTypes & ~objectdata[OBJ_CHEST2].oLvlTypes) == 0);
	assert((objectdata[OBJ_TCHEST3].oLvlTypes & ~objectdata[OBJ_CHEST3].oLvlTypes) == 0);
	if (lvlMask & objectdata[OBJ_TCHEST1].oLvlTypes) {
		AddChestTraps();
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

static void AddChest(int oi)
{
	ObjectStruct* os;
	int num, rnum, itype;

	os = &objects[oi];
	os->_oGfxFrame = random_(147, 2) ? 1 : 3;
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

	while (true) {
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
	os->_oRndSeed = NextRndSeed();
	os->_oVar1 = FindValidShrine(NUM_SHRINETYPE); // SHRINE_TYPE
	/* -- disabled because the animation is shifted and not much point without trap (+ cel file is cut in the patcher)
	if (random_(150, 2) != 0) {
		os->_oAnimFrame = 12;
		os->_oAnimLen = 22;
	}*/
}

static void ObjAddRndSeed(int oi)
{
	objects[oi]._oRndSeed = NextRndSeed();
}

static void ObjAddBloodBook(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	os->_oRndSeed = NextRndSeed();
	os->_oVar5 = BK_BLOOD;                   // STORY_BOOK_NAME
	os->_oVar6 = os->_oGfxFrame + 1;         // LEVER_BOOK_ANIM
	os->_oVar7 = Q_BLOOD;                    // LEVER_BOOK_QUEST
	SetObjMapRange(oi, 0, 0, 0, 0, leverid); // NULL_LVR_EFFECT
	leverid++;
}

static void ObjAddBook(int oi, int bookidx)
{
	ObjectStruct* os;

	os = &objects[oi];
	os->_oVar5 = bookidx; // STORY_BOOK_NAME
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
	os->_oGfxFrame = RandRange(1, 8);
}

static void AddMagicCircle(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	//os->_oRndSeed = NextRndSeed();
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
	os->_oVar2 = 3 * bookframe + idx + TEXT_BOOK11;      // STORY_BOOK_MSG
	os->_oVar5 = 3 * bookframe + idx + BK_STORY_MAINA_1; // STORY_BOOK_NAME
	os->_oGfxFrame = 5 - 2 * bookframe;                  //
	os->_oVar4 = os->_oGfxFrame + 1;                     // STORY_BOOK_READ_FRAME
}

static void AddTorturedMaleBody(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	//os->_oRndSeed = NextRndSeed();
	os->_oGfxFrame = RandRange(1, 4);
}

static void AddTorturedFemaleBody(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	//os->_oRndSeed = NextRndSeed();
	os->_oGfxFrame = RandRange(1, 3);
}

int AddObject(int type, int ox, int oy)
{
	int oi = SetupObject(type, ox, oy);
	if (oi >= 0) {
		// init object
		switch (type) {
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
#ifdef HELLFIRE
		case OBJ_L5SARC:
#endif
			AddSarc(oi);
			break;
		case OBJ_TRAPL:
		case OBJ_TRAPR:
			AddTrap(oi);
			break;
		case OBJ_BARREL:
#ifdef HELLFIRE
		case OBJ_URN:
		case OBJ_POD:
#endif
			AddBarrel(oi);
			break;
		case OBJ_SHRINEL:
		case OBJ_SHRINER:
			AddShrine(oi);
			break;
		case OBJ_DECAP:
			AddDecap(oi);
			break;
		case OBJ_BOOKCASEL:
		case OBJ_BOOKCASER:
		case OBJ_BARRELEX:
#ifdef HELLFIRE
		case OBJ_URNEX:
		case OBJ_PODEX:
#endif
		case OBJ_BOOK2L:
		case OBJ_BOOK2R:
		case OBJ_PEDESTAL:
		case OBJ_ARMORSTAND:
		case OBJ_WEAPONRACKL:
		case OBJ_WEAPONRACKR:
			ObjAddRndSeed(oi);
			break;
		case OBJ_BLOODBOOK:
			ObjAddBloodBook(oi);
			break;
		case OBJ_ANCIENTBOOK:
			ObjAddBook(oi, BK_ANCIENT);
			break;
		case OBJ_STEELTOME:
			ObjAddBook(oi, BK_STEEL);
			break;
		case OBJ_BLINDBOOK:
			ObjAddBook(oi, BK_BLIND);
			break;
		case OBJ_MYTHICBOOK:
			ObjAddBook(oi, BK_MYTHIC);
			break;
		case OBJ_VILEBOOK:
			ObjAddBook(oi, BK_VILENESS);
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
		//case OBJ_BLOODFTN:
		//case OBJ_TEARFTN:
		//	ObjAddRndSeed(oi);
		//	break;
		case OBJ_MCIRCLE1:
		case OBJ_MCIRCLE2:
			AddMagicCircle(oi);
			break;
		case OBJ_STORYBOOK:
			AddStoryBook(oi);
			break;
		case OBJ_TNUDEM:
			AddTorturedMaleBody(oi);
			break;
		case OBJ_TNUDEW:
			AddTorturedFemaleBody(oi);
			break;
#ifdef HELLFIRE
		case OBJ_NAKRULBOOK:
			AddNakrulBook(oi);
			break;
#endif
		}
	}
	return oi;
}

void GetObjectStr(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	switch (os->_otype) {
	case OBJ_LEVER:
#ifdef HELLFIRE
	case OBJ_NAKRULLEVER:
#endif
	//case OBJ_FLAMELVR:
		copy_cstr(infostr, "Lever");
		break;
	case OBJ_CHEST1:
	case OBJ_TCHEST1:
		copy_cstr(infostr, "Small Chest");
		break;
	case OBJ_L1LDOOR:
	case OBJ_L1RDOOR:
#ifdef HELLFIRE
	case OBJ_L5LDOOR:
	case OBJ_L5RDOOR:
#endif
	case OBJ_L2LDOOR:
	case OBJ_L2RDOOR:
	case OBJ_L3LDOOR:
	case OBJ_L3RDOOR:
		if (os->_oVar4 == DOOR_OPEN)
			copy_cstr(infostr, "Open Door");
		else if (os->_oVar4 == DOOR_CLOSED)
			copy_cstr(infostr, "Closed Door");
		else // if (os->_oVar4 == DOOR_BLOCKED)
			copy_cstr(infostr, "Blocked Door");
		break;
	case OBJ_SWITCHSKL:
		copy_cstr(infostr, "Skull Lever");
		break;
	case OBJ_CHEST2:
	case OBJ_TCHEST2:
		copy_cstr(infostr, "Chest");
		break;
	case OBJ_CHEST3:
	case OBJ_TCHEST3:
	case OBJ_SIGNCHEST:
		copy_cstr(infostr, "Large Chest");
		break;
	case OBJ_CRUXM:
	case OBJ_CRUXR:
	case OBJ_CRUXL:
		copy_cstr(infostr, "Crucified Skeleton");
		break;
	case OBJ_SARC:
#ifdef HELLFIRE
	case OBJ_L5SARC:
#endif
		copy_cstr(infostr, "Sarcophagus");
		break;
	//case OBJ_BOOKSHELF:
	//	copy_cstr(infostr, "Bookshelf");
	//	break;
#ifdef HELLFIRE
	case OBJ_URN:
	case OBJ_URNEX:
		copy_cstr(infostr, "Urn");
		break;
	case OBJ_POD:
	case OBJ_PODEX:
		copy_cstr(infostr, "Pod");
		break;
#endif
	case OBJ_BARREL:
	case OBJ_BARRELEX:
		copy_cstr(infostr, "Barrel");
		break;
	case OBJ_SHRINEL:
	case OBJ_SHRINER:
		snprintf(infostr, sizeof(infostr), "%s Shrine", shrinestrs[os->_oVar1]); // SHRINE_TYPE
		break;
	case OBJ_BOOKCASEL:
	case OBJ_BOOKCASER:
		copy_cstr(infostr, "Bookcase");
		break;
	case OBJ_BOOK2L:
	case OBJ_BOOK2R:
		copy_cstr(infostr, "Lectern");
		break;
	case OBJ_BLOODFTN:
		copy_cstr(infostr, "Blood Fountain");
		break;
	case OBJ_DECAP:
		copy_cstr(infostr, "Decapitated Body");
		break;
	case OBJ_PEDESTAL:
		copy_cstr(infostr, "Pedestal of Blood");
		break;
	case OBJ_PURIFYINGFTN:
		copy_cstr(infostr, "Purifying Spring");
		break;
	case OBJ_ARMORSTAND:
		copy_cstr(infostr, "Armor");
		break;
	case OBJ_GOATSHRINE:
		copy_cstr(infostr, "Goat Shrine");
		break;
	case OBJ_CAULDRON:
		copy_cstr(infostr, "Cauldron");
		break;
	case OBJ_MURKYFTN:
		copy_cstr(infostr, "Murky Pool");
		break;
	case OBJ_TEARFTN:
		copy_cstr(infostr, "Fountain of Tears");
		break;
	case OBJ_ANCIENTBOOK:
	case OBJ_VILEBOOK:
	case OBJ_MYTHICBOOK:
	case OBJ_BLOODBOOK:
	case OBJ_BLINDBOOK:
	case OBJ_STEELTOME:
	case OBJ_STORYBOOK:
#ifdef HELLFIRE
	case OBJ_L5BOOK:
	case OBJ_NAKRULBOOK:
#endif
		SStrCopy(infostr, BookName[os->_oVar5], sizeof(infostr)); // STORY_BOOK_NAME
		break;
	case OBJ_WEAPONRACKL:
	case OBJ_WEAPONRACKR:
		copy_cstr(infostr, "Weapon Rack");
		break;
	case OBJ_MUSHPATCH:
		copy_cstr(infostr, "Mushroom Patch");
		break;
	case OBJ_LAZSTAND:
		copy_cstr(infostr, "Vile Stand");
		break;
	default:
		// ASSUME_UNREACHABLE
		infostr[0] = '\0';
		break;
	}
	// infoclr = COL_WHITE;
	// if (os->_oTrapChance != 0 && (3 * currLvl._dLevel + os->_oTrapChance) < myplr._pBaseDex) { // TRAP_CHANCE
	if (os->_oTrapChance != 0) { // TRAP_CHANCE
		char tempstr[256];
		snprintf(tempstr, sizeof(tempstr), "Trapped %s", infostr);
		copy_str(infostr, tempstr);
		// infoclr = COL_RED;
	}
}

DEVILUTION_END_NAMESPACE
