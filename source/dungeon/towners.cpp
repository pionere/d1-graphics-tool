/**
 * @file towners.cpp
 *
 * Implementation of functionality for loading and spawning towners.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

int numtowners;
TownerStruct towners[32];

/** Specifies the animation frame sequence of a given NPC. */
static const int8_t AnimOrder[6][144] = {
	// clang-format off
/*SMITH*/{ 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	    14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	    5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	    14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	    5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	    14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	    5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	    14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	    5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	    14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
	    5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	    15, 16, 1, 1, 1, 1, 1, 1, 1, 1,
	    1, 1, 1, 1, 1, 1, 1, 2, 3, 4,
	    0 },
/*HEALER*/{ 1, 2, 3, 3, 2, 1, 20, 19, 19, 20,
	    1, 2, 3, 3, 2, 1, 20, 19, 19, 20,
	    1, 2, 3, 3, 2, 1, 20, 19, 19, 20,
	    1, 2, 3, 3, 2, 1, 20, 19, 19, 20,
	    1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	    11, 12, 13, 14, 15, 16, 15, 14, 13, 12,
	    11, 10, 9, 8, 7, 6, 5, 4, 5, 6,
	    7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	    15, 14, 13, 12, 11, 10, 9, 8, 7, 6,
	    5, 4, 5, 6, 7, 8, 9, 10, 11, 12,
	    13, 14, 15, 16, 17, 18, 19, 20, 0 },
/*STORY*/{ 1, 1, 1, 1, 24, 23, 22, 21, 20, 19,
	    18, 17, 16, 15, 16, 17, 18, 19, 20, 21,
	    22, 23, 24, 1, 1, 1, 1, 1, 1, 1,
	    1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	    11, 12, 13, 14, 15, 14, 13, 12, 11, 10,
	    9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
/*TAVERN*/{ 1, 2, 3, 3, 2, 1, 16, 15, 14, 14,
	    15, 16, 1, 2, 3, 3, 2, 1, 16, 15,
	    14, 14, 15, 16, 1, 2, 3, 3, 2, 1,
	    16, 15, 14, 14, 15, 16, 1, 2, 3, 3,
	    2, 1, 16, 15, 14, 14, 15, 16, 1, 2,
	    3, 3, 2, 1, 16, 15, 14, 14, 15, 16,
	    1, 2, 3, 3, 2, 1, 16, 15, 14, 14,
	    15, 16, 1, 2, 3, 3, 2, 1, 16, 15,
	    14, 14, 15, 16, 1, 2, 3, 2, 1, 16,
	    15, 14, 14, 15, 16, 1, 2, 3, 4, 5,
	    6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	    16, 0 },
/*DRUNK*/{ 1, 1, 1, 2, 3, 4, 5, 6, 7, 8,
	    9, 8, 7, 7, 7, 7, 6, 5, 4, 3,
	    2, 1, 10, 10, 1, 1, 1, 10, 1, 2,
	    3, 4, 5, 6, 7, 8, 7, 6, 5, 4,
	    3, 2, 1, 10, 1, 2, 3, 4, 5, 5,
	    5, 4, 3, 2, 0 },
/*WITCH*/{ 4, 4, 4, 5, 6, 6, 6, 5, 4, 15,
	    14, 13, 13, 13, 14, 15, 4, 5, 6, 6,
	    6, 5, 4, 4, 4, 5, 6, 6, 6, 5,
	    4, 15, 14, 13, 13, 13, 14, 15, 4, 5,
	    6, 6, 6, 5, 4, 4, 4, 5, 6, 6,
	    6, 5, 4, 15, 14, 13, 13, 13, 14, 15,
	    4, 5, 6, 6, 6, 5, 4, 3, 2, 1,
	    1, 18, 1, 1, 2, 1, 1, 18, 1, 1,
	    2, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	    10, 11, 12, 13, 14, 15, 15, 15, 14, 13,
	    13, 13, 13, 14, 15, 15, 15, 14, 13, 12,
	    12, 12, 11, 10, 10, 10, 9, 8, 9, 10,
	    10, 11, 12, 13, 14, 15, 16, 17, 18, 1,
	    1, 2, 1, 1, 18, 1, 1, 2, 1, 2,
	    3, 0 }
	// clang-format on
};

static void InitTownerAnim(int tnum, const char* pAnimFile, int Delay, int ao)
{
    TownerStruct* tw;

    tw = &towners[tnum];

    tw->tsPath = pAnimFile;
	tw->tsAnimOrder = ao < 0 ? NULL : &AnimOrder[ao][0]; // TNR_ANIM_ORDER
}

/**
 * @brief Load Griswold into the game

 */
static void InitSmith()
{
	InitTownerAnim(numtowners, "Towners\\Smith\\SmithN.CEL", 3, 0);
	numtowners++;
}

static void InitTavern()
{
	InitTownerAnim(numtowners, "Towners\\TwnF\\TwnFN.CEL", 3, 3);
	numtowners++;
}

static void InitDeadguy()
{
	InitTownerAnim(numtowners, "Towners\\Butch\\Deadguy.CEL", 6, -1);
	numtowners++;
}

static void InitWitch()
{
	InitTownerAnim(numtowners, "Towners\\TownWmn1\\Witch.CEL", 6, 5);
	numtowners++;
}

static void InitBarmaid()
{
	InitTownerAnim(numtowners, "Towners\\TownWmn1\\WmnN.CEL", 6, -1);
	numtowners++;
}

static void InitPegboy()
{
	InitTownerAnim(numtowners, "Towners\\TownBoy\\PegKid1.CEL", 6, -1);
	numtowners++;
}

static void InitHealer()
{
	InitTownerAnim(numtowners, "Towners\\Healer\\Healer.CEL", 6, 1);
	numtowners++;
}

static void InitStory()
{
	InitTownerAnim(numtowners, "Towners\\Strytell\\Strytell.CEL", 3, 2);
	numtowners++;
}

static void InitDrunk()
{
	InitTownerAnim(numtowners, "Towners\\Drunk\\TwnDrunk.CEL", 3, 4);
	numtowners++;
}

static void InitPriest()
{
	InitTownerAnim(numtowners, "Towners\\Priest\\Priest8.CEL", 4, -1);
	numtowners++;
}

#ifdef HELLFIRE
static void InitFarmer()
{
	InitTownerAnim(numtowners, "Towners\\Farmer\\Farmrn2.CEL", 3, -1);
	numtowners++;
}

static void InitCowFarmer()
{
	InitTownerAnim(numtowners, "Towners\\Farmer\\cfrmrn2.CEL", 3, -1);
	numtowners++;

	InitTownerAnim(numtowners, "Towners\\Farmer\\mfrmrn2.CEL", 3, -1);
	numtowners++;
}

static void InitGirl()
{
	InitTownerAnim(numtowners, "Towners\\Girl\\Girlw1.CEL", 6, -1);
	numtowners++;

	InitTownerAnim(numtowners, "Towners\\Girl\\Girls1.CEL", 6, -1);
	numtowners++;
}
#endif

void InitTowners()
{
	numtowners = 0;
	InitSmith();
	InitHealer();
	InitTavern();
	InitStory();
	InitDrunk();
	InitWitch();
	InitBarmaid();
	InitPegboy();
	InitPriest();
		InitDeadguy(); // in vanilla game the dead body was gone after the quest is completed, but it might cause de-sync
#ifdef HELLFIRE
		InitCowFarmer();
		InitFarmer(); // in vanilla hellfire the farmer was gone after the quest is completed, but there is no reason for that
		InitGirl();
#endif
}

DEVILUTION_END_NAMESPACE
