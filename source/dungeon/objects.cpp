/**
 * @file objects.cpp
 *
 * Implementation of object functionality, interaction, spawning, loading, etc.
 */
#include "all.h"

#include <QApplication>

#include "../progressdialog.h"

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
//int objectactive[MAXOBJECTS];
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

void AddObjectType(int ofindex)
{
	/*char filestr[DATA_ARCHIVE_MAX_PATH];

	if (objanimdata[ofindex] != NULL) {
		return;
	}

	snprintf(filestr, sizeof(filestr), "Objects\\%s.CEL", objfiledata[ofindex].ofName);
	objanimdata[ofindex] = LoadFileInMem(filestr);*/
}

void InitObjectGFX()
{
	/*const ObjectData* ods;
	bool themeload[NUM_THEMES];
	bool fileload[NUM_OFILE_TYPES];
	int i;

	static_assert(false == 0, "InitObjectGFX fills fileload and themeload with 0 instead of false values.");
	memset(fileload, 0, sizeof(fileload));
	memset(themeload, 0, sizeof(themeload));

	for (i = 0; i < numthemes; i++)
		themeload[themes[i]._tsType] = true;

	BYTE lvlMask = 1 << currLvl._dType;
	for (i = 0; i < NUM_OBJECTS; i++) {
		ods = &objectdata[i];
		if (!(ods->oLvlTypes & lvlMask)
		 && (ods->otheme == THEME_NONE || !themeload[ods->otheme])
		 && (ods->oquest == Q_INVALID || !QuestStatus(ods->oquest))) {
			continue;
		}
		if (fileload[ods->ofindex]) {
			continue;
		}
		fileload[ods->ofindex] = true;
		AddObjectType(ods->ofindex);
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

	tx = pWarps[DWARP_SIDE]._wx + 1;
	ty = pWarps[DWARP_SIDE]._wy;
	AddObject(OBJ_STORYCANDLE, tx - 2, ty + 1);
	AddObject(OBJ_STORYCANDLE, tx + 3, ty + 1);
	AddObject(OBJ_STORYCANDLE, tx - 1, ty + 2);
	AddObject(OBJ_STORYCANDLE, tx + 2, ty + 2);
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
	objects[oi]._oVar6 = objects[oi]._oAnimFrame + 1; // LEVER_BOOK_ANIM
	objects[oi]._oVar7 = qn; // LEVER_BOOK_QUEST
}

static void InitRndBarrels(int otype)
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

	// generate i number of groups of barrels
	for (i = RandRange(3, 7); i != 0; i--) {
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
	int i, j, pn;

	assert((objectdata[OBJ_L1LDOOR].oLvlTypes & DTM_CATHEDRAL) && (objectdata[OBJ_L1RDOOR].oLvlTypes & DTM_CATHEDRAL) && (objectdata[OBJ_L1LIGHT].oLvlTypes & DTM_CATHEDRAL));
	assert(objectdata[OBJ_L1LDOOR].oSetLvlType == DTYPE_CATHEDRAL && objectdata[OBJ_L1RDOOR].oSetLvlType == DTYPE_CATHEDRAL && objectdata[OBJ_L1LIGHT].oSetLvlType == DTYPE_CATHEDRAL);
	assert((objectdata[OBJ_L2LDOOR].oLvlTypes & DTM_CATACOMBS) && (objectdata[OBJ_L2RDOOR].oLvlTypes & DTM_CATACOMBS));
	assert(objectdata[OBJ_L2LDOOR].oSetLvlType == DTYPE_CATACOMBS && objectdata[OBJ_L2RDOOR].oSetLvlType == DTYPE_CATACOMBS);
	assert((objectdata[OBJ_L3LDOOR].oLvlTypes & DTM_CAVES) && (objectdata[OBJ_L3RDOOR].oLvlTypes & DTM_CAVES));
	assert(objectdata[OBJ_L3LDOOR].oSetLvlType == DTYPE_CAVES && objectdata[OBJ_L3RDOOR].oSetLvlType == DTYPE_CAVES);
#ifdef HELLFIRE
	assert((objectdata[OBJ_L5LDOOR].oLvlTypes & DTM_CRYPT) && (objectdata[OBJ_L5RDOOR].oLvlTypes & DTM_CRYPT));
	assert(objectdata[OBJ_L5LDOOR].oSetLvlType == DTYPE_CRYPT && objectdata[OBJ_L5RDOOR].oSetLvlType == DTYPE_CRYPT);
#endif
	switch (currLvl._dType) {
	case DTYPE_CATHEDRAL:
		for (j = y1; j <= y2; j++) {
			for (i = x1; i <= x2; i++) {
				pn = dPiece[i][j];
				if (pn == 270)
					AddObject(OBJ_L1LIGHT, i, j);
				// these pieces are closed doors which are placed directly
				if (pn == 51 || pn == 56) {
					dProgressErr() << QApplication::tr("Piece 51 and 56 are closed doors which are placed directly");
				}
				if (pn == 44 || /*pn == 51 ||*/ pn == 214)
					AddObject(OBJ_L1LDOOR, i, j);
				if (pn == 46 /*|| pn == 56*/)
					AddObject(OBJ_L1RDOOR, i, j);
			}
		}
		break;
	case DTYPE_CATACOMBS:
		for (j = y1; j <= y2; j++) {
			for (i = x1; i <= x2; i++) {
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
		break;
	case DTYPE_CAVES:
		for (j = y1; j <= y2; j++) {
			for (i = x1; i <= x2; i++) {
				pn = dPiece[i][j];
				// 531 and 534 pieces are closed doors which are placed directly
				if (pn == 534)
					AddObject(OBJ_L3LDOOR, i, j);
				if (pn == 531)
					AddObject(OBJ_L3RDOOR, i, j);
			}
		}
		break;
	case DTYPE_HELL:
		break;
#ifdef HELLFIRE
	case DTYPE_CRYPT:
		for (j = y1; j <= y2; j++) {
			for (i = x1; i <= x2; i++) {
				pn = dPiece[i][j];
				// 77 and 80 pieces are closed doors which are placed directly
				if (pn == 77)
					AddObject(OBJ_L5LDOOR, i, j);
				if (pn == 80)
					AddObject(OBJ_L5RDOOR, i, j);
			}
		}
		break;
	case DTYPE_NEST:
		break;
#endif
	default:
		ASSUME_UNREACHABLE
		break;
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
				AddObjectType(objectdata[oidx].ofindex);
				AddObject(oidx, i, j);
				}
			}
			lm++;
		}
	}
	//gbInitObjFlag = false;
}

static void SetupObject(int oi, int type)
{
	ObjectStruct* os;
	const ObjectData* ods;
	const ObjFileData* ofd;

	os = &objects[oi];
	os->_otype = type;
	ods = &objectdata[type];
	os->_oSelFlag = ods->oSelFlag;
	os->_oDoorFlag = ods->oDoorFlag;
	os->_oProc = ods->oProc;
	os->_oModeFlags = ods->oModeFlags;
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
	// assert(os->_oAnimFrame == objectdata[OBJ_L5BOOK].oAnimBaseFrame);
	os->_oVar4 = objectdata[OBJ_L5BOOK].oAnimBaseFrame + 1; // STORY_BOOK_READ_FRAME
	os->_oVar2 = TEXT_BOOK4 + bookidx;                      // STORY_BOOK_MSG
	os->_oVar5 = BK_STORY_NAKRUL_1 + bookidx;               // STORY_BOOK_NAME
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
	// assert(os->_oAnimFrame == objectdata[OBJ_NAKRULBOOK].oAnimBaseFrame);
	os->_oVar4 = objectdata[OBJ_NAKRULBOOK].oAnimBaseFrame + 1; // STORY_BOOK_READ_FRAME
	os->_oVar2 = TEXT_BOOKA + bookidx - QNB_BOOK_A;             // STORY_BOOK_MSG
	os->_oVar3 = bookidx;                                       // STORY_BOOK_NAKRUL_IDX
	os->_oVar5 = BK_NAKRUL_SPELL;                               // STORY_BOOK_NAME
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

int minNa = INT32_MAX;
int maxNa = 0;
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
	switch (currLvl._dLevelIdx) {
	case DLV_CATHEDRAL4:
		AddStoryBook();
		break;
	case DLV_CATACOMBS4:
		AddStoryBook();
		break;
	case DLV_CAVES1:
		if (!IsMultiGame)
			InitRndLocObj5x5(OBJ_SLAINHERO);
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

	int	na = 0;
		for (xx = DBORDERX; xx < DSIZEX + DBORDERX; xx++)
			for (yy = DBORDERY; yy < DSIZEY + DBORDERY; yy++)
				if ((nSolidTable[dPiece[xx][yy]] | (dFlags[xx][yy] & (BFLAG_ALERT | BFLAG_POPULATED))) == 0)
					na++;
if (na > maxNa) {
	maxNa = na;
}
if (na < minNa) {
	minNa = na;
}

	BYTE lvlMask = 1 << currLvl._dType;
	if (lvlMask & objectdata[OBJ_SARC].oLvlTypes) {
		InitRndSarcs(OBJ_SARC);
	}
#ifdef HELLFIRE
	if (lvlMask & objectdata[OBJ_L5SARC].oLvlTypes) {
		InitRndSarcs(OBJ_L5SARC);
	}
#endif
	assert(objectdata[OBJ_TORCHL1].oLvlTypes == objectdata[OBJ_TORCHL2].oLvlTypes && objectdata[OBJ_TORCHL1].oLvlTypes == objectdata[OBJ_TORCHR1].oLvlTypes && objectdata[OBJ_TORCHR1].oLvlTypes == objectdata[OBJ_TORCHR2].oLvlTypes);
	if (lvlMask & objectdata[OBJ_TORCHL1].oLvlTypes) {
		AddL2Torches();
	}
	assert(objectdata[OBJ_TNUDEM].oLvlTypes == objectdata[OBJ_TNUDEW].oLvlTypes);
	assert(objectdata[OBJ_TNUDEM].oLvlTypes == objectdata[OBJ_DECAP].oLvlTypes);
	assert(objectdata[OBJ_TNUDEM].oLvlTypes == objectdata[OBJ_CAULDRON].oLvlTypes);
	assert(objectdata[OBJ_TNUDEM].oLvlTypes == objectdata[OBJ_TORTUREL1].oLvlTypes);
	assert(objectdata[OBJ_TNUDEM].oLvlTypes == objectdata[OBJ_TORTUREL2].oLvlTypes);
	assert(objectdata[OBJ_TNUDEM].oLvlTypes == objectdata[OBJ_TORTUREL3].oLvlTypes);
	assert(objectdata[OBJ_TNUDEM].oLvlTypes == objectdata[OBJ_TORTURER1].oLvlTypes);
	assert(objectdata[OBJ_TNUDEM].oLvlTypes == objectdata[OBJ_TORTURER2].oLvlTypes);
	assert(objectdata[OBJ_TNUDEM].oLvlTypes == objectdata[OBJ_TORTURER3].oLvlTypes);
	if (lvlMask & objectdata[OBJ_TNUDEM].oLvlTypes) {
		AddL4Goodies();
	}
	assert(objectdata[OBJ_BARREL].oLvlTypes == objectdata[OBJ_BARRELEX].oLvlTypes);
	if (lvlMask & objectdata[OBJ_BARREL].oLvlTypes) {
		InitRndBarrels(OBJ_BARREL);
	}
#ifdef HELLFIRE
	assert(objectdata[OBJ_URN].oLvlTypes == objectdata[OBJ_URNEX].oLvlTypes);
	if (lvlMask & objectdata[OBJ_URN].oLvlTypes) {
		InitRndBarrels(OBJ_URN);
	}
	assert(objectdata[OBJ_POD].oLvlTypes == objectdata[OBJ_PODEX].oLvlTypes);
	if (lvlMask & objectdata[OBJ_POD].oLvlTypes) {
		InitRndBarrels(OBJ_POD);
	}
#endif
	assert(objectdata[OBJ_CHEST1].oLvlTypes == DTM_ANY && objectdata[OBJ_CHEST2].oLvlTypes == DTM_ANY && objectdata[OBJ_CHEST3].oLvlTypes == DTM_ANY);
	InitRndLocObj(5, 10, OBJ_CHEST1);
	InitRndLocObj(3, 6, OBJ_CHEST2);
	InitRndLocObj(1, 5, OBJ_CHEST3);
	assert(objectdata[OBJ_TRAPL].oLvlTypes == objectdata[OBJ_TRAPR].oLvlTypes);
	if (lvlMask & objectdata[OBJ_TRAPL].oLvlTypes) {
		AddObjTraps();
	}
	assert(objectdata[OBJ_TCHEST1].oLvlTypes == objectdata[OBJ_TCHEST2].oLvlTypes && objectdata[OBJ_TCHEST1].oLvlTypes == objectdata[OBJ_TCHEST3].oLvlTypes);
	if (lvlMask & objectdata[OBJ_TCHEST1].oLvlTypes) {
		AddChestTraps();
	}
	//gbInitObjFlag = false;
}

void SetMapObjects()
{
	int i;
	//gbInitObjFlag = true;

	for (i = 0; i < NUM_OBJECTS; i++) {
		if (currLvl._dType == objectdata[i].oSetLvlType)
			AddObjectType(objectdata[i].ofindex);
	}

	AddDunObjs(DBORDERX, DBORDERY, MAXDUNX - DBORDERX - 1, MAXDUNY - DBORDERY - 1);

	LoadMapSetObjects(0);
	//gbInitObjFlag = false; -- setmap levers?
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

static void ObjAddBloodBook(int oi)
{
	ObjectStruct* os;

	os = &objects[oi];
	os->_oRndSeed = NextRndSeed();
	os->_oVar5 = BK_BLOOD;                   // STORY_BOOK_NAME
	os->_oVar6 = os->_oAnimFrame + 1;        // LEVER_BOOK_ANIM
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
	os->_oVar2 = 3 * bookframe + idx + TEXT_BOOK11;      // STORY_BOOK_MSG
	os->_oVar5 = 3 * bookframe + idx + BK_STORY_MAINA_1; // STORY_BOOK_NAME
	os->_oAnimFrame = 5 - 2 * bookframe;                 //
	os->_oVar4 = os->_oAnimFrame + 1;                    // STORY_BOOK_READ_FRAME
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
	// objectactive[numobjects] = oi;
	numobjects++;
//	objectavail[0] = objectavail[MAXOBJECTS - numobjects];
	SetupObject(oi, type);
	// place object
	ObjectStruct* os = &objects[oi];
	os->_ox = ox;
	os->_oy = oy;
	if (dObject[ox][oy] != 0) {
		int on = dObject[ox][oy];
		on = on >= 0 ? on - 1 : -(on + 1);
		dProgressErr() << QApplication::tr("Multiple objects on tile %1:%2 - type %3 with index %4 and type %5 with index %6.").arg(ox).arg(oy).arg(type).arg(oi).arg(objects[on]._otype).arg(on);
	}
	dObject[ox][oy] = oi + 1;
	// dFlags[ox][oy] |= BFLAG_POPULATED;
	if (nSolidTable[dPiece[ox][oy]] && (os->_oModeFlags & OMF_FLOOR)) {
		dObject[ox][oy] = 0;
		os->_oModeFlags |= OMF_RESERVED;
		os->_oSelFlag = 0;
		dProgress() << QApplication::tr("Reserved object on tile %1:%2 - type %3 with index %4.").arg(ox).arg(oy).arg(type).arg(oi);
	}
	// init object
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
#ifdef HELLFIRE
	case OBJ_L5CANDLE:
#endif
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
#ifdef HELLFIRE
	case OBJ_L5LDOOR:
	case OBJ_L5RDOOR:
#endif
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
	case OBJ_BOOKCASEL:
	case OBJ_BOOKCASER:
		AddBookcase(oi);
		break;
	case OBJ_DECAP:
		AddDecap(oi);
		break;
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
	case OBJ_SLAINHERO:
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
#ifdef HELLFIRE
	case OBJ_NAKRULBOOK:
		AddNakrulBook(oi);
		break;
#endif
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
	case OBJ_SLAINHERO:
		copy_cstr(infostr, "Slain Hero");
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
