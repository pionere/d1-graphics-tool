/**
 * @file drlg_l2.cpp
 *
 * Implementation of the catacombs level generation algorithms.
 *
 * drlgFlags matrix is used as a BOOLEAN matrix to protect the quest room.
 */

#include "all.h"
#include <vector>

DEVILUTION_BEGIN_NAMESPACE

/** Starting position of the megatiles. */
#define BASE_MEGATILE_L2 (12 - 1)
/** Default megatile if the tile is zero. */
#define DEFAULT_MEGATILE_L2 3
/** Shadow type of the base floor(3). */
#define SF 3

#define AREA_MIN    2
#define ROOM_MAX    10
#define ROOM_MIN    4
enum HALL_DIR {
	HDIR_NONE,
	HDIR_UP,
	HDIR_RIGHT,
	HDIR_DOWN,
	HDIR_LEFT,
};
static int nRoomCnt;
const int Dir_Xadd[5] = { 0, 0, 1, 0, -1 };
const int Dir_Yadd[5] = { 0, -1, 0, 1, 0 };
/*
 * Maps tile IDs to their corresponding undecorated tile ID.
 * Values with a single entry are commented out, because pointless to randomize a single option.
 */
const BYTE L2BTYPES[161] = {
	// clang-format off
	0, 1, 2, 3, 0/*4*/, 0/*5*/, 0/*6*/, 0/*7*/, 8, 0/*9*/,
	0, 0, 0, 0, 0, 0, 0, 0/*17*/, 0/*18*/, 1,
	1, 2, 2, 1, 1, 1, 1, 1, 1, 2,
	2, 2, 2, 2, 0, 0, 0, 0, 8, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	1, 0, 0, 2, 2, 2, 0, 0, 0, 1,
	1, 1, 1, 0, 2, 2, 2, 8, 3, 3,
	3, 3, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 3, 3, 3, 0, 3, // 100...
	0, 3, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0
	// clang-format on
};
/*
 * Specifies where the given tile ID should spread the room ID (transval).
 */
const BYTE L2FTYPES[161] = {
	// clang-format off
	 0, 10, 12, 15, 10, 12, 14, 10,  8, 12,
	 0,  0,  0,  0,  0,  0,  0,  0,  0, 10, // 10..
	10, 12, 12, 10, 10, 10, 10, 10, 10, 12, // 20..
	12, 12, 12, 12, 10, 10, 12, 12,  8, 10, // 30..
	12,  8,  8,  8, 15, 15, 15, 15, 15, 15, // 40..
	15, 15, 10, 10, 10,  8, 12, 12, 12, 15, // 50..
	15, 15, 15, 15, 15, 15, 15, 15, 10, 10, // 60..
	10,  7, 15, 12, 12, 12, 15, 10,  0, 10, // 70..
	10, 10, 10, 14, 12, 12, 12,  8, 15, 15, // 80..
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, // 90..
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, //100..
	15, 15, 15, 15, 15, 15, 10, 10, 12, 12, //110..
	15, 15, 15, 15, 10, 10, 12, 12, 15, 15, //120..
	15, 15, 10, 10, 15, 15, 12, 12, 15, 15, //130..
	10, 12, 12,  0,  0,  0,  0,  0,  0,  0, //140..
	10, 12, 10, 12, 10, 12, 10, 12, 15, 15, //150..
	10
	// clang-format on
};
/** Miniset: Stairs up. */
const BYTE L2USTAIRS[] = {
	// clang-format off
	4, 4, // width, height

	3, 3, 3, 3, // search
	3, 3, 3, 3,
	3, 3, 3, 3,
	3, 3, 3, 3,

	0,  0,  0, 0, // replace
	0, 72, 77, 0,
	0, 76,  0, 0,
	0,  0,  0, 0,
	// clang-format on
};
/** Miniset: Stairs down. */
const BYTE L2DSTAIRS[] = {
	// clang-format off
	4, 4, // width, height

	3, 3, 3, 3, // search
	3, 3, 3, 3,
	3, 3, 3, 3,
	3, 3, 3, 3,

	0,  0,  0, 0, // replace
	0, 48, 71, 0,
	0, 50, 78, 0,
	0,  0,  0, 0,
	// clang-format on
};
/** Miniset: Stairs to town. */
const BYTE L2TWARP[] = {
	// clang-format off
	4, 4, // width, height

	3, 3, 3, 3, // search
	3, 3, 3, 3,
	3, 3, 3, 3,
	3, 3, 3, 3,

	0,   0,   0, 0, // replace
	0, 158, 160, 0,
	0, 159,   0, 0,
	0,   0,   0, 0,
	// clang-format on
};
/** Miniset: Crumbled south pillar. */
const BYTE CRUSHCOL[] = {
	// clang-format off
	3, 3, // width, height

	3, 1, 3, // search
	2, 6, 3,
	3, 3, 3,

	0,  0, 0, // replace
	0, 83, 0,
	0,  0, 0,
	// clang-format on
};
/** Miniset: Vertical oil spill. */
const BYTE BIG1[] = {
	// clang-format off
	2, 2, // width, height

	3, 3, // search
	3, 3,

	113, 0, // replace
	112, 0,
	// clang-format on
};
/** Miniset: Horizontal oil spill. */
const BYTE BIG2[] = {
	// clang-format off
	2, 2, // width, height

	3, 3, // search
	3, 3,

	114, 115, // replace
	  0,   0,
	// clang-format on
};
/** Miniset: Horizontal platform. */
const BYTE BIG3[] = {
	// clang-format off
	1, 2, // width, height

	1, // search
	1,

	117, // replace
	116,
	// clang-format on
};
/** Miniset: Vertical platform. */
const BYTE BIG4[] = {
	// clang-format off
	2, 1, // width, height

	2, 2, // search

	118, 119, // replace
	// clang-format on
};
/** Miniset: Large oil spill. */
const BYTE BIG5[] = {
	// clang-format off
	2, 2, // width, height

	3, 3, // search
	3, 3,

	120, 122, // replace
	121, 123,
	// clang-format on
};
/** Miniset: Vertical wall with debris. */
const BYTE BIG6[] = {
	// clang-format off
	1, 2, // width, height

	1, // search
	1,

	125, // replace
	124,
	// clang-format on
};
/** Miniset: Horizontal wall with debris. */
const BYTE BIG7[] = {
	// clang-format off
	2, 1, // width, height

	2, 2, // search

	126, 127, // replace
	// clang-format on
};
/** Miniset: Rock pile. */
const BYTE BIG8[] = {
	// clang-format off
	2, 2, // width, height

	3, 3, // search
	3, 3,

	128, 130, // replace
	129, 131,
	// clang-format on
};
/** Miniset: Vertical wall collapsed. */
const BYTE BIG9[] = {
	// clang-format off
	2, 2, // width, height

	1, 3, // search
	1, 3,

	133, 135, // replace
	132, 134,
	// clang-format on
};
/** Miniset: Horizontal wall collapsed. */
const BYTE BIG10[] = {
	// clang-format off
	2, 2, // width, height

	2, 2, // search
	3, 3,

	136, 137, // replace
	  3,   3,
	// clang-format on
};
/** Miniset: Crumbled vertical wall 1. */
/*const BYTE RUINS1[] = {
	// clang-format off
	1, 1, // width, height

	1, // search

	80, // replace
	// clang-format on
};*/
/** Miniset: Crumbled vertical wall 2. */
/*const BYTE RUINS2[] = {
	// clang-format off
	1, 1, // width, height

	1, // search

	81, // replace
	// clang-format on
};*/
/** Miniset: Crumbled vertical wall 3. */
/*const BYTE RUINS3[] = {
	// clang-format off
	1, 1, // width, height

	1, // search

	82, // replace
	// clang-format on
};*/
/** Miniset: Crumbled horizontal wall 1. */
/*const BYTE RUINS4[] = {
	// clang-format off
	1, 1, // width, height

	2, // search

	84, // replace
	// clang-format on
};*/
/** Miniset: Crumbled horizontal wall 2. */
/*const BYTE RUINS5[] = {
	// clang-format off
	1, 1, // width, height

	2, // search

	85, // replace
	// clang-format on
};*/
/** Miniset: Crumbled horizontal wall 3. */
/*const BYTE RUINS6[] = {
	// clang-format off
	1, 1, // width, height

	2, // search

	86, // replace
	// clang-format on
};*/
/** Miniset: Crumbled north pillar. */
/*const BYTE RUINS7[] = {
	// clang-format off
	1, 1, // width, height

	8, // search

	87, // replace
	// clang-format on
};*/
/** Miniset: Bloody gib 1. */
const BYTE PANCREAS1[] = {
	// clang-format off
	5, 3, // width, height

	3, 3, 3, 3, 3, // search
	3, 3, 3, 3, 3,
	3, 3, 3, 3, 3,

	0, 0,   0, 0, 0, // replace
	0, 0, 108, 0, 0,
	0, 0,   0, 0, 0,
	// clang-format on
};
/** Miniset: Bloody gib 2. */
const BYTE PANCREAS2[] = {
	// clang-format off
	5, 3, // width, height

	3, 3, 3, 3, 3, // search
	3, 3, 3, 3, 3,
	3, 3, 3, 3, 3,

	0, 0,   0, 0, 0, // replace
	0, 0, 110, 0, 0,
	0, 0,   0, 0, 0,
	// clang-format on
};

/*
 * Patterns with length of 9 (3x3) and the replacement.
 * The remaining(6) values are for alignment.
 */
/*enum L2_TILES {
	L2_ANY = 0,
	L2_BORDER = 1,
	L2_ROOM = 2,
	L2_DOOR = 3,
	L2_EMPTY            = 4,
	L2_BORDER_EMPTY     = 5,//L2_DOOR_ROOM        = 5,
	L2_DOOR_BORDER      = 6,
	L2_EMPTY_ROOM       = 7,
	L2_DOOR_BORDER_ROOM = 8,
};*/
const BYTE Patterns[][16] = {
	// clang-format off
	// bad/obsolete { 0, 7, 0, 0, 1, 0, 0, 3, 0, 2 }, // { 0, 5, 0, 0, 1, 0, 0, 7, 0, 1 },
	// bad/obsolete { 0, 3, 0, 0, 1, 0, 0, 7, 0, 2 }, // { 0, 7, 0, 0, 1, 0, 0, 5, 0, 1 },
/* 0*/	{ 0, 7, 0, 0, 1, 0, 0, 7, 0, 2 }, // horizontal wall (on top)
	// bad/obsolete { 0, 0, 0, 7, 1, 3, 0, 0, 0, 1 }, // { 0, 0, 0, 5, 1, 7, 0, 0, 0, 1 },
	// bad/obsolete { 0, 0, 0, 3, 1, 7, 0, 0, 0, 1 }, // { 0, 0, 0, 7, 1, 5, 0, 0, 0, 1 },
/* 1*/	{ 0, 0, 0, 7, 1, 7, 0, 0, 0, 1 }, // vertical wall (on the left)
/* 2*/	{ 0, 6, 0, 6, 1, 0, 0, 0, 0, 6 }, // concav (top-left)
/* 3*/	{ 0, 6, 0, 0, 1, 6, 0, 0, 0, 9 }, // concav (top-right)
/* 4*/	{ 0, 0, 0, 6, 1, 0, 0, 6, 0, 7 }, // concav (bottom-left) 
/* 5*/	{ 0, 0, 0, 0, 1, 6, 0, 6, 0, 8 }, // concav (bottom-right) / edge (top-left)
/* 6*/	{ 0, 6, 0, 6, 1, 0, 8, 6, 0, 7 }, // { 0, 6, 0, 6, 6, 0, 8, 6, 0, 7 },
/* 7*/	{ 0, 6, 8, 6, 1, 6, 0, 0, 0, 9 }, // { 0, 6, 8, 6, 6, 6, 0, 0, 0, 9 },
/* 8*/	{ 0, 6, 0, 0, 1, 6, 0, 6, 8, 8 }, // { 0, 6, 0, 0, 6, 6, 0, 6, 8, 8 },
// ==	{ 4, 1, 2, 1, 1, 1, 2, 1, 1, 8 },
// ==	{ 0, 6, 0, 0, 1, 6, 0, 6, 2, 8 },
/* 9*/	{ 1, 1, 1, 1, 1, 1, 0, 6, 0, 8 }, // { 6, 6, 6, 6, 6, 6, 0, 6, 0, 8 },
// or	{ 1, 1, 1, 1, 1, 1, 7, 6, 7, 8 },
	// useless { 2, 6, 6, 6, 6, 6, 0, 6, 0, 8 },
/*10*/	{ 7, 7, 7, 6, 1, 6, 0, 6, 0, 8 }, // { 7, 7, 7, 6, 6, 6, 0, 6, 0, 8 },
//	==	{ 2, 2, 2, 6, 1, 6, 0, 6, 0, 8 }
//	==	{ 4, 4, 4, 1, 1, 1, 0, 6, 0, 8 }
/*11*/	{ 1, 1, 2, 1, 1, 6, 0, 6, 0, 8 }, // { 6, 6, 2, 6, 6, 6, 0, 6, 0, 8 },
/*12*/	{ 6, 2, 6, 1, 1, 1, 0, 6, 0, 8 }, // { 6, 2, 6, 6, 6, 6, 0, 6, 0, 8 },
/*13*/	{ 2, 1, 1, 6, 1, 1, 0, 6, 0, 8 }, // { 2, 6, 6, 6, 6, 6, 0, 6, 0, 8 },
/*14*/	{ 6, 7, 7, 1, 1, 6, 0, 6, 0, 8 }, // { 6, 7, 7, 6, 6, 6, 0, 6, 0, 8 },
//  ==	{ 6, 2, 2, 1, 1, 6, 0, 6, 0, 8 },
//  ==	{ 1, 4, 4, 1, 1, 1, 0, 6, 0, 8 },
/*15*/	{ 4, 4, 1, 1, 1, 1, 2, 6, 2, 8 }, // { 4, 4, 6, 6, 6, 6, 2, 6, 2, 8 },
	// obsolete { 2, 2, 2, 2, 1, 2, 2, 1, 2, 7 }, // { 2, 2, 2, 2, 6, 2, 2, 6, 2, 7 },
	// obsolete { 2, 2, 2, 2, 1, 2, 6, 1, 6, 7 }, // { 2, 2, 2, 2, 6, 2, 6, 6, 6, 7 },
	// obsolete { 2, 2, 6, 2, 1, 1, 2, 2, 6, 9 }, // { 2, 2, 6, 2, 6, 6, 2, 2, 6, 9 },
	// obsolete { 2, 1, 2, 2, 1, 2, 2, 2, 2, 6 }, // { 2, 6, 2, 2, 6, 2, 2, 2, 2, 6 },
	// obsolete { 2, 2, 2, 2, 6, 6, 2, 2, 2, 9 },
	// obsolete { 2, 2, 2, 1, 1, 2, 2, 2, 2, 6 }, // { 2, 2, 2, 6, 6, 2, 2, 2, 2, 6 },
	// obsolete { 2, 2, 0, 2, 1, 1, 2, 2, 0, 9 }, // { 2, 2, 0, 2, 6, 6, 2, 2, 0, 9 },
/*16*/	{ 0, 1, 0, 0, 1, 4, 0, 1, 0, 10 },
/*17*/	{ 0, 0, 0, 1, 1, 1, 0, 4, 0, 11 },
/*18*/	{ 0, 0, 0, 6, 1, 4, 0, 1, 0, 14 },
/*19*/	{ 0, 6, 0, 1, 1, 0, 0, 4, 0, 16 },
/*20*/	{ 0, 0, 0, 0, 1, 1, 0, 1, 4, 13 },
/*21*/	{ 8, 8, 8, 8, 1, 1, 0, 1, 1, 13 },
/*22*/	{ 8, 1, 4, 8, 1, 1, 0, 1, 1, 10 }, // { 8, 8, 4, 8, 1, 1, 0, 1, 1, 10 },
/*23*/	{ 0, 1, 0, 1, 1, 4, 1, 1, 0, 16 },
	// obsolete	{ 0, 0, 0, 1, 1, 1, 1, 1, 1, 11 },
/*24*/	{ 0, 0, 0, 1, 1, 1, 1, 1, 5, 11 },
/*25*/	{ 2, 2, 8, 1, 1, 1, 4, 1, 1, 11 }, // horizontal wall (on bottom)
/*26*/	{ 0, 1, 1, 0, 1, 1, 0, 1, 1, 10 }, // vertical wall (on the right)
/*27*/	{ 0, 4, 0, 1, 1, 1, 0, 1, 1, 14 }, // edge (top-right)
/*28*/	{ 0, 6, 0, 0, 1, 1, 0, 4, 0, 15 }, // edge (bottom-left)
/*29*/	{ 0, 1, 0, 4, 1, 1, 0, 1, 1, 15 },
/*30*/	{ 0, 1, 1, 2, 1, 1, 2, 1, 4, 10 },
// or	{ 8, 1, 1, 2, 1, 1, 2, 1, 4, 10 },
/*31*/	{ 2, 1, 1, 1, 1, 1, 0, 4, 0, 16 }, // edge (bottom-right)
	// obsolete	{ 1, 1, 0, 1, 1, 2, 1, 1, 0, 1 },
	// // or	{ 1, 1, 8, 1, 1, 2, 1, 1, 8, 1 },
/*32*/	{ 5, 1, 0, 1, 1, 2, 1, 1, 0, 1 }, // fixed { 4, 1, 0, 1, 1, 0, 1, 1, 0, 1 },
// or	{ 4, 1, 8, 1, 1, 2, 1, 1, 8, 1 },
		// useless { 1, 1, 4, 1, 1, 2, 0, 1, 2, 1 },
/*33*///{ 1, 1, 2, 1, 1, 2, 4, 1, 0, 1 },
/* or*/	{ 1, 1, 2, 1, 1, 2, 4, 1, 8, 1 },
	// obsolete { 4, 1, 1, 1, 1, 1, 2, 2, 0, 2 },
	/* or*///	{ 4, 1, 1, 1, 1, 1, 2, 2, 8, 2 },
/*34*///{ 5, 1, 1, 1, 1, 1, 2, 2, 0, 2 },
/* or*/	{ 5, 1, 1, 1, 1, 1, 2, 2, 8, 2 },
	// obsolete	{ 4, 1, 1, 1, 1, 1, 6, 2, 6, 2 },
/*35*/	{ 5, 1, 1, 1, 1, 1, 6, 2, 6, 2 },
/*36*/	{ 1, 1, 4, 1, 1, 1, 0, 2, 2, 2 },
// or	{ 1, 1, 4, 1, 1, 1, 8, 2, 2, 2 },
/*37*/	{ 1, 1, 1, 1, 1, 1, 0, 2, 2, 2 },
// or	{ 1, 1, 1, 1, 1, 1, 8, 2, 2, 2 },
/*38*/	{ 4, 1, 1, 1, 1, 1, 1, 2, 2, 2 },
/*39*/	{ 1, 1, 4, 1, 1, 1, 2, 2, 1, 2 },
/*40*/	{ 1, 2, 2, 1, 1, 1, 4, 1, 1, 11 },
/*41*/	{ 2, 1, 1, 3, 1, 1, 2, 1, 1, 14 },
/*42*/	{ 2, 1, 1, 2, 1, 1, 1, 1, 4, 10 },
	// obsolete	{ 2, 1, 1, 1, 1, 4, 4, 1, 1, 16 }, // new
/*43*/	{ 2, 1, 5, 1, 1, 4, 4, 1, 1, 16 }, // new
/*44*/	{ 2, 1, 4, 1, 1, 1, 4, 4, 1, 16 },
/*45*/	{ 2, 1, 1, 1, 1, 1, 1, 1, 1, 16 },
	// obsolete	{ 4, 1, 2, 1, 1, 1, 1, 1, 1, 15 }, // new
/*46*/	{ 5, 1, 2, 1, 1, 1, 1, 1, 1, 15 },
	// obsolete { 4, 1, 1, 1, 1, 1, 2, 1, 1, 14 },
/*47*/	{ 5, 1, 1, 1, 1, 1, 2, 1, 1, 14 },
/*48*/	{ 4, 1, 2, 1, 1, 3, 1, 1, 2, 8 }, // new
/*49*/	{ 5, 1, 2, 1, 1, 1, 2, 1, 5, 15 }, // new - not an exact replacement. A mirror of 8 would be better, but does not exist yet.
	// useless { 1, 1, 1, 1, 1, 1, 1, 1, 2, 8 },
	//{ 0, 1, 0, 0, 3, 0, 0, 1, 0, 4 }, // vertical door
	//{ 0, 0, 0, 1, 3, 1, 0, 0, 0, 5 }, // horizontal door
	//{ 0, 0, 0, 0, 2, 0, 0, 0, 0, 3 }, // room
	//{ 0, 0, 0, 0, 4, 0, 0, 0, 0, 12 }, // void
	// clang-format on
};

static void DRLG_L2PlaceRndSet(const BYTE* miniset, int rndper)
{
	int sx, sy, sw, sh, xx, yy, ii;
	bool found;

	sw = miniset[0];
	sh = miniset[1];

	for (sy = 0; sy < DMAXY - sh; sy++) {
		for (sx = 0; sx < DMAXX - sw; sx++) {
			found = true;
			ii = 2;
			for (yy = sy; yy < sy + sh && found; yy++) {
				for (xx = sx; xx < sx + sw && found; xx++) {
					if (miniset[ii] != 0 && dungeon[xx][yy] != miniset[ii]) {
						found = false;
					}
					if (drlgFlags[xx][yy]) {
						found = false;
					}
					ii++;
				}
			}
			if (!found)
				continue;
			// assert(ii == sw * sh + 2);
			for (yy = std::max(sy - sh, 0); yy < std::min(sy + 2 * sh, DMAXY) && found; yy++) {
				for (xx = std::max(sx - sw, 0); xx < std::min(sx + 2 * sw, DMAXX) && found; xx++) {
					// BUGFIX: yy and xx can go out of bounds (fixed)
					if (dungeon[xx][yy] == miniset[ii]) {
						found = false;
					}
				}
			}
			if (found && random_(0, 100) < rndper) {
				for (yy = sy; yy < sy + sh; yy++) {
					for (xx = sx; xx < sx + sw; xx++) {
						if (miniset[ii] != 0) {
							dungeon[xx][yy] = miniset[ii];
						}
						ii++;
					}
				}
			}
		}
	}
}

static void DRLG_L2Subs()
{
	int x, y; //, i, j;
	BYTE c, k;
	int8_t rv;
	const unsigned MAX_MATCH = 16;
	const unsigned NUM_L2TYPES = 112;
	static_assert(MAX_MATCH <= INT8_MAX, "MAX_MATCH does not fit to rv(int8_t) in DRLG_L2Subs.");
	static_assert(NUM_L2TYPES <= UCHAR_MAX, "NUM_L2TYPES does not fit to i(BYTE) in DRLG_L2Subs.");
#if DEBUG_MODE
	for (int i = sizeof(L2BTYPES) - 1; i >= 0; i--) {
		if (L2BTYPES[i] != 0) {
			if (i >= NUM_L2TYPES)
				app_fatal("Value %d is ignored in L2BTYPES at %d", L2BTYPES[i], i);
			break;
		}
	}

	for (int i = 0; i < sizeof(L2BTYPES); i++) {
		c = L2BTYPES[i];
		if (c == 0)
			continue;
		x = 0;
		for (int j = 0; j < sizeof(L2BTYPES); j++) {
			if (c == L2BTYPES[j])
				x++;
		}
		if (x > MAX_MATCH)
			app_fatal("Too many(%d) matching('%d') values in L2BTYPES", x, c);
	}
#endif
	for (x = 0; x < DMAXX; x++) {
		for (y = 0; y < DMAXY; y++) {
			if (drlgFlags[x][y])
				continue;
			if (random_(0, 4) == 0) {
				c = L2BTYPES[dungeon[x][y]];
				if (c != 0) {
					rv = random_(0, MAX_MATCH);
					k = 0;
					while (TRUE) {
						if (c == L2BTYPES[k] && --rv < 0) {
							break;
						}
						if (++k == NUM_L2TYPES)
							k = 0;
					}
					/*for (j = y - 2; j < y + 2; j++) {
						for (i = x - 2; i < x + 2; i++) {
							if (dungeon[i][j] == k) {
								j = y + 3;
								i = x + 2;
							}
						}
					}
					if (j < y + 3) {*/
						dungeon[x][y] = k;
					//}
				}
			}
		}
	}
}

/*
 * Replace open doors with either a closed one or a doorway.
 * New dungeon values: 150 151 156 157
 */
static void DRLG_L2DoorSubs()
{
	int x, y;

	for (x = 0; x < DMAXX; x++) {
		for (y = 0; y < DMAXY; y++) {
			if (drlgFlags[x][y])
				continue;
			if (dungeon[x][y] == 4) {
				// LDOOR
				dungeon[x][y] = random_(136, 2) ? 150 : 156; // closed door / doorway
			} else if (dungeon[x][y] == 5) {
				// HDOOR
				dungeon[x][y] = random_(136, 2) ? 151 : 157; // closed door / doorway
			}
		}
	}
}

/*
 * Place shadows under arches and pillars.
 * New dungeon values: 45..51   72   140 141 142
 * TODO: use DRLG_PlaceMiniSet instead?
 */
static void DRLG_L2Shadows()
{
	BYTE bv;
	bool pillar, harch, varch;
	int x, y;

	for (x = 1; x < DMAXX; x++) {
		for (y = 1; y < DMAXY; y++) {
			bv = dungeon[x][y];
			pillar = false;
			harch = false;
			varch = false;
			switch (bv) {
			case 6:
			case 9:
			case 78:
				pillar = true;
				break;
			case 77:
				// stairs
				if (dungeon[x - 1][y] == 3) {
					dungeon[x - 1][y] = 72;
				}
				break;
			case 4:
				// fake shadow of the vertical doors
				if (dungeon[x - 1][y] == 3) {
					dungeon[x - 1][y] = 47;
				}
				if (dungeon[x - 1][y - 1] == 3) {
					dungeon[x - 1][y - 1] = 51;
				}
				break;
			case 5:
				// fake shadow of the horizontal doors
				if (dungeon[x][y - 1] == 3) {
					dungeon[x][y - 1] = 46;
				}
				if (dungeon[x - 1][y - 1] == 3) {
					dungeon[x - 1][y - 1] = 49;
				}
				break;
			case 41:
				// arch to both directions
				pillar = true;
				harch = true;
				varch = true;
				break;
			case 39:
			case 42:
				// - vertical arch
				pillar = true;
				varch = true;
				break;
			case 40:
				// horizontal arch - ending
				pillar = true;
				harch = true;
				break;
			case 43:
				harch = true;
				break;
			}
			if (varch) {
				if (dungeon[x - 1][y] == 3) {
					if (dungeon[x - 1][y + 1] == 3 || dungeon[x - 1][y + 1] == 46) { // overlapping shadows (missing tile to match the other part)
						if (dungeon[x - 1][y - 1] == 3) {
							// 3, 0,  search
							// 3, 39/41/42,
							// 3/46, 0,

							//48, 0, replace
							//51, 0,
							//47, 0,
							dungeon[x - 1][y - 1] = 48;
							dungeon[x - 1][y] = 51;
							dungeon[x - 1][y + 1] = 47;
							pillar = false;
						} else if (dungeon[x - 1][y - 1] == 2) {
							// 2, 0,  search
							// 3, 39/41/42,
							// 3/46, 0,

							//142, 0, replace
							//51, 0,
							//47, 0,
							dungeon[x - 1][y - 1] = 142;
							dungeon[x - 1][y] = 51;
							dungeon[x - 1][y + 1] = 47;
							pillar = false;
						} else if (dungeon[x - 1][y - 1] == 47 || dungeon[x - 1][y - 1] == 46) { // overlapping shadows
							// 46/47, 0,  search
							// 3, 39/41/42,
							// 3/46, 0,

							// 0, 0, replace
							//51, 0,
							//47, 0,
							dungeon[x - 1][y] = 51;
							dungeon[x - 1][y + 1] = 47;
							pillar = false;
						}
					}
				} else if (dungeon[x - 1][y] == 2) {
					if (dungeon[x - 1][y + 1] == 3) {
						// 2, 39/41/42,  search
						// 3, 0,

						//141, 0, replace
						//47, 0,
						dungeon[x - 1][y] = 141;
						dungeon[x - 1][y + 1] = 47;
						pillar = false;
					} else if (dungeon[x - 1][y + 1] == 46) { // overlapping shadows (missing tile to match the other part)
						// 2, 39/41/42,  search
						// 46, 0,

						//141, 0, replace
						// 0, 0,
						dungeon[x - 1][y] = 141;
						pillar = false;
					}
				}
			}
			if (harch) {
				// - horizontal arch
				if (dungeon[x][y - 1] == 3) {
					if (dungeon[x + 1][y - 1] == 3) {
						// 3, 3,  search
						// 40/41/43, 0

						//49,46, replace
						// 0, 0,
						dungeon[x][y - 1] = 49;
						dungeon[x + 1][y - 1] = 46;
					//} else if (dungeon[x + 1][y - 1] == 47) { // overlapping shadows (missing tile to match the other part)
					//	dungeon[x][y - 1] = 49;
					}
				} else if (dungeon[x][y - 1] == 1) {
					if (dungeon[x + 1][y - 1] == 3) {
						// 1, 3,  search
						// 40/41/43, 0

						//140,46, replace
						// 0, 0,
						dungeon[x][y - 1] = 140;
						dungeon[x + 1][y - 1] = 46;
					//} else if (dungeon[x + 1][y - 1] == 47) { // overlapping shadows (missing tile to match the other part)
					//	dungeon[x][y - 1] = 49;
					}
				}
			}
			if (pillar) {
				// pillars
				if (dungeon[x - 1][y] == 3) {
					if (dungeon[x - 1][y - 1] == 3) {
						// 3, 0,  search
						// 3, 6/7/39/40/41/42/78

						//48, 0, replace
						//50, 0,
						dungeon[x - 1][y] = 50;
						dungeon[x - 1][y - 1] = 48;
					} else if (dungeon[x - 1][y - 1] == 2) {
						// 2, 0,  search
						// 3, 6/7/39/40/41/42/78

						//142, 0, replace
						//50, 0,
						dungeon[x - 1][y] = 50;
						dungeon[x - 1][y - 1] = 142;
					} else if (dungeon[x - 1][y - 1] == 47) { // overlapping shadows
						//47, 0,  search
						// 3, 6/7/39/40/41/42/78

						// 0, 0, replace
						//50, 0,
						dungeon[x - 1][y] = 50;
					} else if (dungeon[x - 1][y - 1] == 46) { // overlapping shadows
						//46, 0,  search
						// 3, 6/7/39/40/41/42/78

						// 0, 0, replace
						//45, 0,
						dungeon[x - 1][y] = 45;
					}
				}
			}
		}
	}
}

static void DRLG_LoadL2SP()
{
	// assert(pSetPieces[0]._spData == NULL);
	if (QuestStatus(Q_BLIND)) {
		pSetPieces[0]._spData = LoadFileInMem("Levels\\L2Data\\Blind1.DUN");
		if (pSetPieces[0]._spData == NULL) {
			return;
		}
		// patch the map - Blind1.DUN
		// place pieces with closed doors
		pSetPieces[0]._spData[(2 + 4 + 3 * 11) * 2] = 150;
		pSetPieces[0]._spData[(2 + 6 + 7 * 11) * 2] = 150;
		// ensure the changing tiles are reserved
		/*for (int y = 0; y < 11; y++) {
			for (int x = 0; x < 11; x++) {
				if (pSetPieces[0]._spData[(2 + x + y * 11) * 2] == 0)
					pSetPieces[0]._spData[(2 + x + y * 11) * 2] = DEFAULT_MEGATILE_L2;
			}
		}*/
		// protect the main structure
		for (int y = 0; y < 7; y++) {
			for (int x = 0; x < 7; x++) {
				pSetPieces[0]._spData[(2 + 11 * 11 + x + y * 11) * 2] = 1;
			}
		}
		for (int y = 4; y < 11; y++) {
			for (int x = 4; x < 11; x++) {
				pSetPieces[0]._spData[(2 + 11 * 11 + x + y * 11) * 2] = 1;
			}
		}
		pSetPieces[0]._sptype = SPT_BLIND;
	} else if (QuestStatus(Q_BLOOD)) {
		pSetPieces[0]._spData = LoadFileInMem("Levels\\L2Data\\Blood1.DUN");
		if (pSetPieces[0]._spData == NULL) {
			return;
		}
		// ensure the inner tiles are reserved
		// pSetPieces[0]._spData[(2 + 5 + 12 * 10) * 2] = 3;
		// ensure the changing tiles are reserved
		for (int y = 3; y <= 7; y++) {
			for (int x = 0; x <= 9; x++) {
				pSetPieces[0]._spData[(2 + 10 * 16 + x + y * 10) * 2] = 1;
			}
		}
		for (int y = 0; y < 3; y++) {
			for (int x = 2; x <= 6; x++) {
				pSetPieces[0]._spData[(2 + 10 * 16 + x + y * 10) * 2] = 1;
			}
		}
		// protect the main room
		for (int y = 7; y < 16; y++) {
			for (int x = 2; x < 8; x++) {
				pSetPieces[0]._spData[(2 + 10 * 16 + x + y * 10) * 2] = 1;
			}
		}
		pSetPieces[0]._sptype = SPT_BLOOD;
	} else if (QuestStatus(Q_BCHAMB)) {
		pSetPieces[0]._spData = LoadFileInMem("Levels\\L2Data\\Bonestr2.DUN");
		if (pSetPieces[0]._spData == NULL) {
			return;
		}
		// patch the map - Bonestr2.DUN
		// - remove tile to leave space for shadow
		pSetPieces[0]._spData[(2 + 2 + 4 * 7) * 2] = 0;
		pSetPieces[0]._sptype = SPT_BCHAMB;
		// protect the main room
		for (int y = 1; y < 6; y++) {
			for (int x = 1; x < 6; x++) {
				pSetPieces[0]._spData[(2 + 7 * 7 + x + y * 7) * 2] = 1;
			}
		}
	}
}

/*
 * Draw set-room + reserve its tiles.
 */
static void DRLG_L2SetRoom(int idx)
{
	DRLG_LoadSP(idx, DEFAULT_MEGATILE_L2);
}

static void DL2_DrawRoom(int x1, int y1, int x2, int y2)
{
	int i, j;

	//assert(x1 >= 0 && x2 < DMAXX && y1 >= 0 && y2 < DMAXY);
	for (j = y1; j <= y2; j++) {
		pdungeon[x1][j] = 35;
		pdungeon[x2][j] = 35;
	}
	for (i = x1; i <= x2; i++) {
		pdungeon[i][y1] = 35;
		pdungeon[i][y2] = 35;
	}
	for (i = x1 + 1; i < x2; i++) {
		for (j = y1 + 1; j < y2; j++) {
			pdungeon[i][j] = 46;
		}
	}
}

/*static void CreateDoorType(int nX, int nY)
{
	if (pdungeon[nX - 1][nY] != 68
	 && pdungeon[nX + 1][nY] != 68
	 && pdungeon[nX][nY - 1] != 68
	 && pdungeon[nX][nY + 1] != 68
	 && (pdungeon[nX][nY] < 65 || pdungeon[nX][nY] > 69)) {
		pdungeon[nX][nY] = 68;
	}
}*/

/**
 * Draws a random room rectangle, and then subdivides the rest of the passed in rectangle into 4 and recurses.
 * @param nX1 Lower X boundary of the area to draw into.
 * @param nY1 Lower Y boundary of the area to draw into.
 * @param nX2 Upper X boundary of the area to draw into.
 * @param nY2 Upper Y boundary of the area to draw into.
 * @param nRDest The room number of the parent room this call was invoked for. negative if empty
 * @param nHDir The direction of the hall from nRDest to this room.
 * @param nW Width of the room - 1, if set
 * @param nH Height of the room - 1, if not zero.
 */
static void CreateRoom(int nX1, int nY1, int nX2, int nY2, int nRDest, int nHDir, int nW, int nH)
{
	int nAw, nAh, nRw, nRh, nRx1, nRy1, nRx2, nRy2, nHx1, nHy1, nHx2, nHy2, nRid;

	if (nRoomCnt >= lengthof(drlg.RoomList))
		return;
	//assert(nX1 <= DMAXX - 2 || nX1 >= nX2);
	//assert(nX1 >= 1 || nX1 >= nX2);
	//assert(nY1 <= DMAXY - 2 || nY1 >= nY2);
	//assert(nY1 >= 1 || nY1 >= nY2);
	/*if (nX1 < 1)
		nX1 = 1;
	if (nX2 > DMAXX - 2)
		nX2 = DMAXX - 2;
	if (nY1 < 1)
		nY1 = 1;
	if (nY2 > DMAXY - 2)
		nY2 = DMAXY - 2;*/
	nAw = nX2 - nX1;
	nAh = nY2 - nY1;
	if (nAw < AREA_MIN || nAh < AREA_MIN) {
		return;
	}

	if (nAw >= ROOM_MAX) {
		nRw = RandRange(ROOM_MIN, ROOM_MAX);
	} else if (nAw > ROOM_MIN) {
		static_assert(DMAXX - ROOM_MIN < 0x7FFF, "CreateRoom uses RandRangeLow to set the width.");
		nRw = RandRangeLow(ROOM_MIN, nAw);
	} else {
		nRw = nAw;
	}
	if (nAh >= ROOM_MAX) {
		nRh = RandRange(ROOM_MIN, ROOM_MAX);
	} else if (nAh > ROOM_MIN) {
		static_assert(DMAXY - ROOM_MIN < 0x7FFF, "CreateRoom uses RandRangeLow to set the height.");
		nRh = RandRangeLow(ROOM_MIN, nAh);
	} else {
		nRh = nAh;
	}

	if (nW != 0) {
		nRw = nW;
		nRh = nH;
	}

	nRx1 = RandRange(nX1, nX2 - nRw);
	nRy1 = RandRange(nY1, nY2 - nRh);
	nRx2 = nRw + nRx1;
	nRy2 = nRh + nRy1;

	// draw the room
	DL2_DrawRoom(nRx1, nRy1, nRx2, nRy2);
	pdungeon[nRx1][nRy1] = 67; // 67    66
	pdungeon[nRx1][nRy2] = 69; //
	pdungeon[nRx2][nRy1] = 66; //
	pdungeon[nRx2][nRy2] = 65; // 69    65

	// add entry to RoomList
	drlg.RoomList[nRoomCnt].nRoomParent = nRDest;
	drlg.RoomList[nRoomCnt].nRoomx1 = nRx1;
	drlg.RoomList[nRoomCnt].nRoomy1 = nRy1;
	drlg.RoomList[nRoomCnt].nRoomx2 = nRx2;
	drlg.RoomList[nRoomCnt].nRoomy2 = nRy2;
	if (nRDest >= 0) {
		ROOMHALLNODE& parentRoom = drlg.RoomList[nRDest];
		switch (nHDir) {
		case HDIR_UP:
			nHx1 = RandRange(nRx1 + 1, nRx2 - 1);
			nHy1 = nRy1;
			nHx2 = RandRange(parentRoom.nRoomx1 + 1, parentRoom.nRoomx2 - 1);
			nHy2 = parentRoom.nRoomy2;
			break;
		case HDIR_DOWN:
			nHx1 = RandRange(nRx1 + 1, nRx2 - 1);
			nHy1 = nRy2;
			nHx2 = RandRange(parentRoom.nRoomx1 + 1, parentRoom.nRoomx2 - 1);
			nHy2 = parentRoom.nRoomy1;
			break;
		case HDIR_RIGHT:
			nHx1 = nRx2;
			nHy1 = RandRange(nRy1 + 1, nRy2 - 1);
			nHx2 = parentRoom.nRoomx1;
			nHy2 = RandRange(parentRoom.nRoomy1 + 1, parentRoom.nRoomy2 - 1);
			break;
		case HDIR_LEFT:
			nHx1 = nRx1;
			nHy1 = RandRange(nRy1 + 1, nRy2 - 1);
			nHx2 = parentRoom.nRoomx2;
			nHy2 = RandRange(parentRoom.nRoomy1 + 1, parentRoom.nRoomy2 - 1);
			break;
		default:
			ASSUME_UNREACHABLE
			break;
		}
		drlg.RoomList[nRoomCnt].nHallx1 = nHx1;
		drlg.RoomList[nRoomCnt].nHally1 = nHy1;
		drlg.RoomList[nRoomCnt].nHallx2 = nHx2;
		drlg.RoomList[nRoomCnt].nHally2 = nHy2;
		drlg.RoomList[nRoomCnt].nHalldir = nHDir;
	}

	nRid = nRoomCnt;
	nRoomCnt++;

	/*nX1 += 1;
	nY1 += 1;
	nX2 -= 1;
	nY2 -= 1;*/
	if (nRh > nRw) {
		/* +---+
		 * |   |
		 * |   | +--+
		 * +---+ |XX|
		 *       +--+
		 */
		//CreateRoom(nX1 /*+ 2*/, nY1 /*+ 2*/, nRx1 - 2, nRy2 /*- 2*/, nRid, HDIR_RIGHT, 0, 0);
		CreateRoom(nX1, nY1, nRx1 - 2, nRy2, nRid, HDIR_RIGHT, 0, 0);
		/*       +--+
		 *       |XX| +---+
		 *       +--+ |   |
		 *            |   |
		 *            +---+
		 */
		//CreateRoom(nRx2 + 2, nRy1 /*+ 2*/, nX2 /*- 2*/, nY2 /*- 2*/, nRid, HDIR_LEFT, 0, 0);
		CreateRoom(nRx2 + 2, nRy1, nX2, nY2, nRid, HDIR_LEFT, 0, 0);
		/*       +--+
		 *       |XX|
		 *       +--+
		 * +-------+
		 * |       |
		 * +-------+
		 */
		//CreateRoom(nX1 /*+ 2*/, nRy2 + 2, nRx2 /*- 2*/, nY2 /*- 2*/, nRid, HDIR_UP, 0, 0);
		CreateRoom(nX1, nRy2 + 2, nRx2, nY2, nRid, HDIR_UP, 0, 0);
		/*        +-------+
		 *        |       |
		 *        +-------+
		 *       +--+
		 *       |XX|
		 *       +--+
		 */
		//CreateRoom(nRx1 /*+ 2*/, nY1 /*+ 2*/, nX2 /*- 2*/, nRy1 - 2, nRid, HDIR_DOWN, 0, 0);
		CreateRoom(nRx1, nY1, nX2, nRy1 - 2, nRid, HDIR_DOWN, 0, 0);
	} else {
		/* +-------+
		 * |       |
		 * +-------+
		 *       +--+
		 *       |XX|
		 *       +--+
		 */
		//CreateRoom(nX1 /*+ 2*/, nY1 /*+ 2*/, nRx2 /*- 2*/, nRy1 - 2, nRid, HDIR_DOWN, 0, 0);
		CreateRoom(nX1, nY1, nRx2, nRy1 - 2, nRid, HDIR_DOWN, 0, 0);
		/*       +--+
		 *       |XX|
		 *       +--+
		 *        +-------+
		 *        |       |
		 *        +-------+
		 */
		//CreateRoom(nRx1 /*+ 2*/, nRy2 + 2, nX2 /*- 2*/, nY2 /*- 2*/, nRid, HDIR_UP, 0, 0);
		CreateRoom(nRx1, nRy2 + 2, nX2, nY2, nRid, HDIR_UP, 0, 0);
		/*       +--+
		 * +---+ |XX|
		 * |   | +--+
		 * |   |
		 * +---+
		 */
		//CreateRoom(nX1 /*+ 2*/, nRy1 /*+ 2*/, nRx1 - 2, nY2 /*- 2*/, nRid, HDIR_RIGHT, 0, 0);
		CreateRoom(nX1, nRy1, nRx1 - 2, nY2, nRid, HDIR_RIGHT, 0, 0);
		/*            +---+
		 *            |   |
		 *       +--+ |   |
		 *       |XX| +---+
		 *       +--+
		 */
		//CreateRoom(nRx2 + 2, nY1 /*+ 2*/, nX2 /*- 2*/, nRy2 /*- 2*/, nRid, HDIR_LEFT, 0, 0);
		CreateRoom(nRx2 + 2, nY1, nX2, nRy2, nRid, HDIR_LEFT, 0, 0);
	}
}

static void PlaceHallExt(int extDir, int hallDir, int nX, int nY)
{
	int xx, yy;

	if (extDir & 1) {
		xx = nX;
		yy = nY;
		if (hallDir != HDIR_UP && hallDir != HDIR_DOWN) {
			yy--;
		} else {
			xx--;
		}
		//assert(xx >= 0 && xx < DMAXX && yy >= 0 && yy < DMAXY);
		if (pdungeon[xx][yy] == 32) {
			pdungeon[xx][yy] = 44;
		}
	}
	if (extDir >= 2) {
		if (hallDir != HDIR_UP && hallDir != HDIR_DOWN) {
			nY++;
		} else {
			nX++;
		}
		//assert(nX >= 0 && nX < DMAXX && nY >= 0 && nY < DMAXY);
		if (pdungeon[nX][nY] == 32) {
			pdungeon[nX][nY] = 44;
		}
	}
}

static void ConnectHall(int nX1, int nY1, int nX2, int nY2, int nHd)
{
	int nCurrd, nDx, nDy, nRp, extDir;

	//assert(pdungeon[nX1][nY1] == 35 || pdungeon[nX1][nY1] == 68);
	pdungeon[nX1][nY1] = 68; // CreateDoorType(nX1, nY1);
	//assert(pdungeon[nX2][nY2] == 35 || pdungeon[nX2][nY2] == 68);
	pdungeon[nX2][nY2] = 68; // CreateDoorType(nX2, nY2);

	extDir = random_(0, 4);
	nCurrd = nHd;
	nX2 -= Dir_Xadd[nCurrd];
	nY2 -= Dir_Yadd[nCurrd];
	PlaceHallExt(extDir, nCurrd, nX2, nY2);
	//assert(nX2 >= 0 && nX2 < DMAXX && nY2 >= 0 && nY2 < DMAXY);
	pdungeon[nX2][nY2] = 44;

	while (TRUE) {
		//assert(nX1 < DMAXX - 2 || nCurrd != HDIR_RIGHT);
		//assert(nX1 > 1 || nCurrd != HDIR_LEFT);
		//assert(nY1 < DMAXX - 2 || nCurrd != HDIR_DOWN);
		//assert(nY1 > 1 || nCurrd != HDIR_UP);
		/*if (nX1 >= DMAXX - 2 && nCurrd == HDIR_RIGHT) {
			nCurrd = HDIR_LEFT;
		}
		if (nX1 <= 1 && nCurrd == HDIR_LEFT) {
			nCurrd = HDIR_RIGHT;
		}
		if (nY1 >= DMAXY - 2 && nCurrd == HDIR_DOWN) {
			nCurrd = HDIR_UP;
		}
		if (nY1 <= 1 && nCurrd == HDIR_UP) {
			nCurrd = HDIR_DOWN;
		}*/
		nX1 += Dir_Xadd[nCurrd];
		nY1 += Dir_Yadd[nCurrd];
		if (pdungeon[nX1][nY1] == 32) {
			PlaceHallExt(extDir, nCurrd, nX1, nY1);
			pdungeon[nX1][nY1] = 44;
		} else if (pdungeon[nX1][nY1] != 44) {
			if (pdungeon[nX1][nY1] == 67) {
			// top left corner of a room
				if (nCurrd == HDIR_RIGHT) {
					//assert(pdungeon[nX1 - 1][nY1 + 1] == 44 || pdungeon[nX1 - 1][nY1 + 1] == 32);
					pdungeon[nX1 - 1][nY1 + 1] = 44;
					nY1++;
				} else {
					//assert(nCurrd == HDIR_DOWN);
					//assert(pdungeon[nX1 + 1][nY1 - 1] == 44 || pdungeon[nX1 + 1][nY1 - 1] == 32);
					pdungeon[nX1 + 1][nY1 - 1] = 44;
					nX1++;
				}
			} else if (pdungeon[nX1][nY1] == 66) {
			// top right corner of a room
				if (nCurrd == HDIR_LEFT) {
					//assert(pdungeon[nX1 + 1][nY1 + 1] == 44 || pdungeon[nX1 + 1][nY1 + 1] == 32);
					pdungeon[nX1 + 1][nY1 + 1] = 44;
					nY1++;
				} else {
					//assert(nCurrd == HDIR_DOWN);
					//assert(pdungeon[nX1 - 1][nY1 - 1] == 44 || pdungeon[nX1 - 1][nY1 - 1] == 32);
					pdungeon[nX1 - 1][nY1 - 1] = 44;
					nX1--;
				}
			} else if (pdungeon[nX1][nY1] == 69) {
			// bottom left corner of a room
				if (nCurrd == HDIR_RIGHT) {
					//assert(pdungeon[nX1 - 1][nY1 - 1] == 44 || pdungeon[nX1 - 1][nY1 - 1] == 32);
					pdungeon[nX1 - 1][nY1 - 1] = 44;
					nY1--;
				} else {
					//assert(nCurrd == HDIR_UP);
					//assert(pdungeon[nX1 + 1][nY1 + 1] == 44 || pdungeon[nX1 + 1][nY1 + 1] == 32);
					pdungeon[nX1 + 1][nY1 + 1] = 44;
					nX1++;
				}
			} else if (pdungeon[nX1][nY1] == 65) {
			// bottom right corner of a room
				if (nCurrd == HDIR_LEFT) {
					//assert(pdungeon[nX1 + 1][nY1 - 1] == 44 || pdungeon[nX1 + 1][nY1 - 1] == 32);
					pdungeon[nX1 + 1][nY1 - 1] = 44;
					nY1--;
				} else {
					//assert(nCurrd == HDIR_UP);
					//assert(pdungeon[nX1 - 1][nY1 + 1] == 44 || pdungeon[nX1 - 1][nY1 + 1] == 32);
					pdungeon[nX1 - 1][nY1 + 1] = 44;
					nX1--;
				}
			}

			// add entry door to the room
			//assert(pdungeon[nX1][nY1] == 35 || pdungeon[nX1][nY1] == 68);
			pdungeon[nX1][nY1] = 68; // CreateDoorType(nX1, nY1);
			// find exit from the room
			switch (nCurrd) {
			case HDIR_UP:
				// proceed up till a wall is hit
				do {
					nY1--;
				} while (pdungeon[nX1][nY1] == 46);
				if (nY1 <= nY2) {
					// the room is too large -> walk back and left/right
					if (pdungeon[nX1][nY2] != 46) {
						// the path should be on the wall -> proceed next to the wall
						nY1 = nY1 == nY2 ? nY2 + 1 : nY2 - 1;
					} else {
						nY1 = nY2;
					}
					//assert(pdungeon[nX1][nY1] == 46);
					if (nX1 > nX2) {
						do {
							nX1--;
						} while (pdungeon[nX1][nY1] == 46);
						nCurrd = HDIR_LEFT;
					} else {
						do {
							nX1++;
						} while (pdungeon[nX1][nY1] == 46);
						nCurrd = HDIR_RIGHT;
					}
				}
				break;
			case HDIR_RIGHT:
				// proceed up till a wall is hit
				do {
					nX1++;
				} while (pdungeon[nX1][nY1] == 46);
				if (nX1 >= nX2) {
					// the room is too large -> walk back and up/down
					if (pdungeon[nX2][nY1] != 46) {
						// the path should be on the wall -> proceed next to the wall
						nX1 = nX1 == nX2 ? nX2 - 1 : nX2 + 1;
					} else {
						nX1 = nX2;
					}
					//assert(pdungeon[nX1][nY1] == 46);
					if (nY1 > nY2) {
						do {
							nY1--;
						} while (pdungeon[nX1][nY1] == 46);
						nCurrd = HDIR_UP;
					} else {
						do {
							nY1++;
						} while (pdungeon[nX1][nY1] == 46);
						nCurrd = HDIR_DOWN;
					}
				}
				break;
			case HDIR_DOWN:
				// proceed up till a wall is hit
				do {
					nY1++;
				} while (pdungeon[nX1][nY1] == 46);
				if (nY1 >= nY2) {
					// the room is too large -> walk back and left/right
					if (pdungeon[nX1][nY2] != 46) {
						// the path should be on the wall -> proceed next to the wall
						nY1 = nY1 == nY2 ? nY2 - 1 : nY2 + 1;
					} else {
						nY1 = nY2;
					}
					//assert(pdungeon[nX1][nY1] == 46);
					if (nX1 > nX2) {
						do {
							nX1--;
						} while (pdungeon[nX1][nY1] == 46);
						nCurrd = HDIR_LEFT;
					} else {
						do {
							nX1++;
						} while (pdungeon[nX1][nY1] == 46);
						nCurrd = HDIR_RIGHT;
					}
				}
				break;
			case HDIR_LEFT:
				// proceed up till a wall is hit
				do {
					nX1--;
				} while (pdungeon[nX1][nY1] == 46);
				if (nX1 <= nX2) {
					// the room is too large -> walk back and up/down
					if (pdungeon[nX2][nY1] != 46) {
						// the path should be on the wall -> proceed next to the wall
						nX1 = nX1 == nX2 ? nX2 + 1 : nX2 - 1;
					} else {
						nX1 = nX2;
					}
					//assert(pdungeon[nX1][nY1] == 46);
					if (nY1 > nY2) {
						do {
							nY1--;
						} while (pdungeon[nX1][nY1] == 46);
						nCurrd = HDIR_UP;
					} else {
						do {
							nY1++;
						} while (pdungeon[nX1][nY1] == 46);
						nCurrd = HDIR_DOWN;
					}
				}
				break;
			default:
				ASSUME_UNREACHABLE;
			}
			// add exit door to the room
			//assert(pdungeon[nX1][nY1] == 35 || pdungeon[nX1][nY1] == 68);
			pdungeon[nX1][nY1] = 68; // CreateDoorType(nX1, nY1);
			continue;
		}
		nDx = abs(nX2 - nX1);
		nDy = abs(nY2 - nY1);
		if (nDx == 0) {
			if (nDy == 0)
				break;
			if (nCurrd == HDIR_RIGHT || nCurrd == HDIR_LEFT)
				nCurrd = nY2 <= nY1 ? HDIR_UP : HDIR_DOWN;
			continue;
		} else if (nDy == 0) {
			if (nCurrd == HDIR_UP || nCurrd == HDIR_DOWN)
				nCurrd = nX2 <= nX1 ? HDIR_LEFT : HDIR_RIGHT;
			continue;
		}
		nRp = random_low(0, 2 * (nDx + nDy));
		if (nRp < (nDx + nDy)) {
			if (nRp < nDx)
				nCurrd = nX2 <= nX1 ? HDIR_LEFT : HDIR_RIGHT;
			else
				nCurrd = nY2 <= nY1 ? HDIR_UP : HDIR_DOWN;
		}
	}
}

/*
 * Prepare the dungeon
 * Dungeon values: 1 .. 16
 */
static void DRLG_L2MakeMegas()
{
	int x, y, i, j, xx, yy;
	BYTE bv;

	memset(dungeon, 3, sizeof(dungeon));

	for (y = 0; y < DMAXY; y++) {
		for (x = 0; x < DMAXX; x++) {
			bv = pdungeon[x][y];
			if (bv == 32) { // void
				dungeon[x][y] = 12;
				continue;
			}
			if (bv == 46) { // room
				dungeon[x][y] = 3;
				continue;
			}
			if (bv == 68) { // door
				/*if (pdungeon[x + 1][y] == 46) {
					assert(pdungeon[x - 1][y] == 46);
					dungeon[x][y] = 4; // vertical door
				} else {
					assert(pdungeon[x][y + 1] == 46);
					assert(pdungeon[x][y - 1] == 46);
					dungeon[x][y] = 5; // horizontal door
				}*/
				dungeon[x][y] = pdungeon[x + 1][y] == 46 ? 4 : 5; // vertical : horizontal door
				continue;
			}
			//assert(bv == 35); // wall
			for (i = lengthof(Patterns) - 1; i >= 0; i--) {
				xx = x - 1;
				yy = y - 1;
				for (j = 0; j < 9; j++, xx++) {
					if (j == 3 || j == 6) {
						yy++;
						xx = x - 1;
					}
					if (xx >= 0 && xx < DMAXX && yy >= 0 && yy < DMAXY)
						bv = pdungeon[xx][yy];
					else
						bv = 32;
					switch (Patterns[i][j]) {
					case 0:
						continue;
					case 1:
						// border of a room
						if (bv == 35) {
							continue;
						}
						break;
					case 2:
						// inside of a room
						if (bv == 46) {
							continue;
						}
						break;
					case 3:
						// door
						if (bv == 68) {
							continue;
						}
						break;
					case 4:
						// empty tile
						if (bv == 32) {
							continue;
						}
						break;
					/*case 5:
						// door or inside of a room
						if (bv == 68 || bv == 46) {
							continue;
						}
						break;*/
					case 5:
						// border or empty tile
						if (bv == 35 || bv == 32) {
							continue;
						}
						break;
					case 6:
						// door or border of a room
						if (bv == 68 || bv == 35) {
							continue;
						}
						break;
					case 7:
						// empty tile or inside of a room
						if (bv == 32 || bv == 46) {
							continue;
						}
						break;
					case 8:
						// not empty (door or border of a room or inside of a room)
						if (bv != 32) {
							//assert(bv == 68 || bv == 35 || bv == 46);
							continue;
						}
						break;
					default:
						ASSUME_UNREACHABLE
						break;
					}
					break;
				}
				if (j == 9) {
					dungeon[x][y] = Patterns[i][9];
					break;
				}
			}
			//assert(i >= 0);
		}
	}
}

static void L2TileFix()
{
	/* commented out because this is no longer necessary
	int i, j;

	for (j = 0; j < DMAXY; j++) {
		for (i = 0; i < DMAXX; i++) {
			if (dungeon[i][j] == 1 && dungeon[i][j + 1] == 3) {
				dungeon[i][j + 1] = 1;
			}
			if (dungeon[i][j] == 3 && dungeon[i][j + 1] == 1) {
				dungeon[i][j + 1] = 3;
			}
			if (dungeon[i][j] == 3 && dungeon[i + 1][j] == 7) {
				dungeon[i + 1][j] = 3;
			}
			if (dungeon[i][j] == 2 && dungeon[i + 1][j] == 3) {
				dungeon[i + 1][j] = 2;
			}
			if (dungeon[i][j] == 11 && dungeon[i + 1][j] == 14) {
				dungeon[i + 1][j] = 16;
			}
		}
	}*/
}

static int DL2_NumNoChar()
{
	int i, rv;
	BYTE* pTmp;

	rv = 0;
	static_assert(sizeof(pdungeon) == DMAXX * DMAXY, "Linear traverse of pdungeon does not work in DL2_NumNoChar.");
	pTmp = &pdungeon[0][0];
	for (i = 0; i < DMAXX * DMAXY; i++, pTmp++)
		if (*pTmp == 32)
			rv++;

	return rv;
}

/*
 * remove the longest straight wall of the room.
 */
static void DL2_KnockWalls(int x1, int y1, int x2, int y2)
{
	int i, j, currLenA, currLenB, bestPos, bestDir = HDIR_NONE, bestLen = 0;

	//assert(y1 >= 1 && y2 <= DMAXY - 2);
	currLenA = currLenB = 0;
	for (i = x1 + 1; i < x2; i++) {
		if (pdungeon[i][y1 - 1] == 46) {
			//assert(pdungeon[i][y1 + 1] == 46);
			currLenA++;
		} else {
			if (currLenA > bestLen) {
				bestLen = currLenA;
				bestPos = i - 1;
				bestDir = HDIR_UP;
			}
			currLenA = 0;
		}
		if (pdungeon[i][y2 + 1] == 46) {
			//assert(pdungeon[i][y2 - 1] == 46);
			currLenB++;
		} else {
			if (currLenB > bestLen) {
				bestLen = currLenA;
				bestPos = i - 1;
				bestDir = HDIR_DOWN;
			}
			currLenB = 0;
		}
	}
	if (currLenA > bestLen) {
		bestLen = currLenA;
		bestPos = x2 - 1;
		bestDir = HDIR_UP;
	}
	if (currLenB > bestLen) {
		bestLen = currLenA;
		bestPos = x2 - 1;
		bestDir = HDIR_DOWN;
	}
	//assert(x1 >= 1 && x2 <= DMAXX - 2);
	currLenA = currLenB = 0;
	for (j = y1 + 1; j < y2; j++) {
		if (pdungeon[x1 - 1][j] == 46) {
			//assert(pdungeon[x1 + 1][j] == 46);
			currLenA++;
		} else {
			if (currLenA > bestLen) {
				bestLen = currLenA;
				bestPos = j - 1;
				bestDir = HDIR_LEFT;
			}
			currLenA = 0;
		}
		if (pdungeon[x2 + 1][j] == 46) {
			//assert(pdungeon[x2 - 1][j] == 46);
			currLenB++;
		} else {
			if (currLenB > bestLen) {
				bestLen = currLenB;
				bestPos = j - 1;
				bestDir = HDIR_RIGHT;
			}
			currLenB = 0;
		}
	}
	if (currLenA > bestLen) {
		bestLen = currLenA;
		bestPos = y2 - 1;
		bestDir = HDIR_LEFT;
	}
	if (currLenB > bestLen) {
		bestLen = currLenA;
		bestPos = y2 - 1;
		bestDir = HDIR_RIGHT;
	}
	switch (bestDir) {
	case HDIR_UP:
		for (i = 0; i < bestLen; i++)
			pdungeon[bestPos - i][y1] = 46;
		break;
	case HDIR_RIGHT:
		for (i = 0; i < bestLen; i++)
			pdungeon[x2][bestPos - i] = 46;
		break;
	case HDIR_DOWN:
		for (i = 0; i < bestLen; i++)
			pdungeon[bestPos - i][y2] = 46;
		break;
	case HDIR_LEFT:
		for (i = 0; i < bestLen; i++)
			pdungeon[x1][bestPos - i] = 46;
		break;
	default:
		ASSUME_UNREACHABLE
		break;
	}
}

static bool DL2_FillVoids()
{
	int i, j, xx, yy, x1, x2, y1, y2;
	bool xLeft, xRight, xUp, xDown;
	int tries;

	tries = 0;
	while (TRUE) {
		if (DL2_NumNoChar() <= DMAXX * DMAXY / 2)
			return true;
next_try:
		if (++tries > 200)
			break;
		// find a 3-tile wide room-wall-empty space
		do {
			xx = RandRange(2, DMAXX - 3);
			yy = RandRange(2, DMAXY - 3);
		} while (pdungeon[xx][yy] != 35);
		if (pdungeon[xx - 1][yy] == 32 && pdungeon[xx + 1][yy] == 46) {
			if (pdungeon[xx + 1][yy - 1] != 46
			 || pdungeon[xx + 1][yy + 1] != 46
			 || pdungeon[xx - 1][yy - 1] != 32
			 || pdungeon[xx - 1][yy + 1] != 32)
				goto next_try;
			xRight = false;
			xLeft = xUp = xDown = true;
		} else if (pdungeon[xx + 1][yy] == 32 && pdungeon[xx - 1][yy] == 46) {
			if (pdungeon[xx - 1][yy - 1] != 46
			 || pdungeon[xx - 1][yy + 1] != 46
			 || pdungeon[xx + 1][yy - 1] != 32
			 || pdungeon[xx + 1][yy + 1] != 32)
				goto next_try;
			xLeft = false;
			xRight = xUp = xDown = true;
		} else if (pdungeon[xx][yy - 1] == 32 && pdungeon[xx][yy + 1] == 46) {
			if (pdungeon[xx - 1][yy + 1] != 46
			 || pdungeon[xx + 1][yy + 1] != 46
			 || pdungeon[xx - 1][yy - 1] != 32
			 || pdungeon[xx + 1][yy - 1] != 32)
				goto next_try;
			xDown = false;
			xUp = xLeft = xRight = true;
		} else if (pdungeon[xx][yy + 1] == 32 && pdungeon[xx][yy - 1] == 46) {
			if (pdungeon[xx - 1][yy - 1] != 46
			 || pdungeon[xx + 1][yy - 1] != 46
			 || pdungeon[xx - 1][yy + 1] != 32
			 || pdungeon[xx + 1][yy + 1] != 32)
				goto next_try;
			xUp = false;
			xDown = xLeft = xRight = true;
		} else
			goto next_try;
		x1 = xLeft ? xx - 1 : xx;
		x2 = xRight ? xx + 1 : xx;
		y1 = xUp ? yy - 1 : yy;
		y2 = xDown ? yy + 1 : yy;
		if (!xLeft || !xRight) {
			// widen the area up/down till possible
			while (xUp || xDown) {
				if (y1 == 0) {
					xUp = false;
				}
				if (y2 == DMAXY - 1) {
					xDown = false;
				}
				if (y2 - y1 >= ROOM_MAX) {
					xUp = false;
					xDown = false;
				}
				if (xUp) {
					y1--;
				}
				if (xDown) {
					y2++;
				}
				if (pdungeon[x2][y1] != 32) {
					xUp = false;
				}
				if (pdungeon[x2][y2] != 32) {
					xDown = false;
				}
			}
			y1 += 2;
			y2 -= 2;
			if (y2 - y1 < 5)
				goto next_try;
			if (!xLeft) {
				while (TRUE) {
					if (x2 == DMAXX - 1) {
						break;
					}
					if (x2 - x1 >= ROOM_MAX) {
						break;
					}
					for (j = y1; j <= y2; j++) {
						if (pdungeon[x2][j] != 32) {
							break;
						}
					}
					if (j <= y2)
						break;
					x2++;
				}
				x2 -= 2;
			} else {
				// assert(!xRight);
				while (TRUE) {
					if (x1 == 0) {
						break;
					}
					if (x2 - x1 >= ROOM_MAX) {
						break;
					}
					for (j = y1; j <= y2; j++) {
						if (pdungeon[x1][j] != 32) {
							break;
						}
					}
					if (j <= y2) {
						break;
					}
					x1--;
				}
				x1 += 2;
			}
			if (x2 - x1 < 5)
				goto next_try;
		} else {
			// assert(!xUp || !xDown);
			// widen the area left/right till possible
			while (xLeft || xRight) {
				if (x1 == 0) {
					xLeft = false;
				}
				if (x2 == DMAXX - 1) {
					xRight = false;
				}
				if (x2 - x1 >= ROOM_MAX) {
					xLeft = false;
					xRight = false;
				}
				if (xLeft) {
					x1--;
				}
				if (xRight) {
					x2++;
				}
				if (pdungeon[x1][y2] != 32) {
					xLeft = false;
				}
				if (pdungeon[x2][y2] != 32) {
					xRight = false;
				}
			}
			x1 += 2;
			x2 -= 2;
			if (x2 - x1 < 5)
				goto next_try;
			if (!xUp) {
				while (TRUE) {
					if (y2 == DMAXY - 1) {
						break;
					}
					if (y2 - y1 >= 12) {
						break;
					}
					for (i = x1; i <= x2; i++) {
						if (pdungeon[i][y2] != 32) {
							break;
						}
					}
					if (i <= x2) {
						break;
					}
					y2++;
				}
				y2 -= 2;
			} else {
				// assert(!xDown);
				while (TRUE) {
					if (y1 == 0) {
						break;
					}
					if (y2 - y1 >= ROOM_MAX) {
						break;
					}
					for (i = x1; i <= x2; i++) {
						if (pdungeon[i][y1] != 32) {
							break;
						}
					}
					if (i <= x2) {
						break;
					}
					y1--;
				}
				y1 += 2;
			}
			if (y2 - y1 < 5)
				goto next_try;
		}
		DL2_DrawRoom(x1, y1, x2, y2);
		DL2_KnockWalls(x1, y1, x2, y2);
	}

	return false;
}

static void DRLG_L2CreateDungeon()
{
	int i, j, k, ForceW, ForceH;

	ForceW = 0;
	ForceH = 0;

	if (pSetPieces[0]._spData != NULL) { // pSetPieces[0]._sptype != SPT_NONE
		ForceW = SwapLE16(*(uint16_t*)&pSetPieces[0]._spData[0]) + 3; // TODO: add border to the setmaps?
		ForceH = SwapLE16(*(uint16_t*)&pSetPieces[0]._spData[2]) + 3;
	}

	nRoomCnt = 0;
	CreateRoom(1, 1, DMAXX - 2, DMAXY - 2, -1, HDIR_NONE, ForceW, ForceH);

	if (pSetPieces[0]._spData != NULL) { // pSetPieces[0]._sptype != SPT_NONE
		pSetPieces[0]._spx = drlg.RoomList[0].nRoomx1 + 2;
		pSetPieces[0]._spy = drlg.RoomList[0].nRoomy1 + 2;
	}

	for (i = 1; i < nRoomCnt; i++) {
		ConnectHall(drlg.RoomList[i].nHallx1, drlg.RoomList[i].nHally1, drlg.RoomList[i].nHallx2, drlg.RoomList[i].nHally2, drlg.RoomList[i].nHalldir);
	}

	// prevent standalone walls between the hallway-tiles
	for (i = 1; i < DMAXX - 1; i++) {
		for (j = 1; j < DMAXY - 1; j++) {
			if (pdungeon[i][j] == 32) {
				k = pdungeon[i + 1][j] == 44 ? 1 : 0;
				k += pdungeon[i - 1][j] == 44 ? 1 : 0;
				k += pdungeon[i][j + 1] == 44 ? 1 : 0;
				k += pdungeon[i][j - 1] == 44 ? 1 : 0;
				if (k >= 3) {
					pdungeon[i][j] = 44;
					i--;
					j--;
					if (i == 0)
						break;
				}
			}
		}
	}

	for (i = 0; i < DMAXX; i++) {     /// BUGFIX: change '<=' to '<' (fixed)
		for (j = 0; j < DMAXY; j++) { /// BUGFIX: change '<=' to '<' (fixed)
			// convert room corners to walls
			if (pdungeon[i][j] == 67
			 || pdungeon[i][j] == 66
			 || pdungeon[i][j] == 69
			 || pdungeon[i][j] == 65) {
				pdungeon[i][j] = 35;
			} else if (pdungeon[i][j] == 44) {
				// convert hallways to rooms
				pdungeon[i][j] = 46;
				//assert(i > 0 && i < DMAXX - 1 && j > 0 && j < DMAXY - 1);
				// add walls to hallways
				//for (k = 0; k < lengthof(offset_x); k++)
				//	if (pdungeon[i + offset_x[k]][j + offset_y[k]] == 32)
				//		pdungeon[i + offset_x[k]][j + offset_y[k]] = 35;
				if (pdungeon[i - 1][j - 1] == 32) {
					pdungeon[i - 1][j - 1] = 35;
				}
				if (pdungeon[i - 1][j] == 32) {
					pdungeon[i - 1][j] = 35;
				}
				if (pdungeon[i - 1][1 + j] == 32) {
					pdungeon[i - 1][1 + j] = 35;
				}
				if (pdungeon[i + 1][j - 1] == 32) {
					pdungeon[i + 1][j - 1] = 35;
				}
				if (pdungeon[i + 1][j] == 32) {
					pdungeon[i + 1][j] = 35;
				}
				if (pdungeon[i + 1][1 + j] == 32) {
					pdungeon[i + 1][1 + j] = 35;
				}
				if (pdungeon[i][j - 1] == 32) {
					pdungeon[i][j - 1] = 35;
				}
				if (pdungeon[i][j + 1] == 32) {
					pdungeon[i][j + 1] = 35;
				}
			}
		}
	}
}

/*
 * Spread transVals further.
 * - spread transVals on corner tiles to make the bottom room-tiles visible.
 */
static void DRLG_L2TransFix()
{
	int i, j, xx, yy;

	yy = DBORDERY;
	for (j = 0; j < DMAXY; j++) {
		xx = DBORDERX;
		for (i = 0; i < DMAXX; i++) {
			switch (dungeon[i][j]) {
			// fix transVals of corners
			// case 12:
			// case 16:
			// case 145:
			case 149:
				DRLG_CopyTrans(xx, yy, xx + 1, yy);
				DRLG_CopyTrans(xx, yy, xx, yy + 1);
				DRLG_CopyTrans(xx, yy, xx + 1, yy + 1);
				break;
			// case 14:
			// case 10:
			case 147:
				//if (dungeon[i][j - 1] != 143) {
				//	break;
				//}
				/* fall-through */
			case 143:
				DRLG_CopyTrans(xx, yy, xx + 1, yy);
				DRLG_CopyTrans(xx, yy, xx + 1, yy + 1);
				break;
			// case 15:
			// case 11:
			case 148:
				//if (dungeon[i + 1][j] != 11, 144) {
				//	break;
				//}
				/* fall-through */
			case 144:
				DRLG_CopyTrans(xx, yy, xx, yy + 1);
				DRLG_CopyTrans(xx, yy, xx + 1, yy + 1);
				break;
			}
			xx += 2;
		}
		yy += 2;
	}
}

void DRLG_L2InitTransVals()
{
	int i, j;

	static_assert(sizeof(drlg.transvalMap) == sizeof(dungeon), "transvalMap vs dungeon mismatch.");
	memcpy(drlg.transvalMap, dungeon, sizeof(dungeon));
	// block arches with walls to stop the spread of transVals
	for (i = 0; i < DMAXX; i++) {
		for (j = 0; j < DMAXY; j++) {
			switch (drlg.transvalMap[i][j]) {
			case 39:
			case 42:
				drlg.transvalMap[i][j + 1] = 1;
				break;
			case 41:
				drlg.transvalMap[i][j + 1] = 1;
				drlg.transvalMap[i + 1][j] = 2;
				break;
			case 40:
			case 43:
				drlg.transvalMap[i + 1][j] = 2;
				break;
			}
		}
	}

	DRLG_FloodTVal(L2FTYPES);
	DRLG_L2TransFix();
}

/*
 * Replace tiles with complete ones to hide rendering glitch of transparent corners.
 * New dungeon values: 143..149
 * Obsolete dungeon values: 10..16
 */
static void DRLG_L2Corners()
{
	int i;
	BYTE* pTmp;

	static_assert(sizeof(dungeon) == DMAXX * DMAXY, "Linear traverse of dungeon does not work in DRLG_L2Corners.");
	pTmp = &dungeon[0][0];
	for (i = 0; i < DMAXX * DMAXY; i++, pTmp++) {
		if (*pTmp >= 10 && *pTmp <= 16 /*&& *pTmp != 12*/)
			*pTmp += 133;
	}
	/*int i, j;

	// check borders out-of-order
	for (i = 0; i < DMAXX; i++) {
		if (dungeon[i][DMAXY - 1] == 10)
			dungeon[i][DMAXY - 1] = 143;
		else if (dungeon[i][DMAXY - 1] == 13)
			dungeon[i][DMAXY - 1] = 146;
		else if (dungeon[i][DMAXY - 1] == 14)
			dungeon[i][DMAXY - 1] = 147;
	}
	for (j = 0; j < DMAXY; j++) {
		if (dungeon[DMAXX - 1][j] == 11)
			dungeon[DMAXX - 1][j] = 144;
		else if (dungeon[DMAXX - 1][j] == 13)
			dungeon[DMAXX - 1][j] = 146;
		else if (dungeon[DMAXX - 1][j] == 15)
			dungeon[DMAXX - 1][j] = 148;
	}
	// check the rest of the map
	for (i = 0; i < DMAXX; i++) {
		for (j = 0; j < DMAXY; j++) {
			switch (dungeon[i][j]) {
			case 10:
				if (dungeon[i][j + 1] != 10)
					dungeon[i][j] = 143;
				break;
			case 11:
				if (dungeon[i + 1][j] != 11)
					dungeon[i][j] = 144;
				break;
			case 13:
				if (dungeon[i + 1][j] != 11 || dungeon[i][j + 1] != 10)
					dungeon[i][j] = 146;
				break;
			case 14:
				if (dungeon[i][j + 1] != 15)
					dungeon[i][j] = 147;
				break;
			case 15:
				if (dungeon[i + 1][j] != 11)
					dungeon[i][j] = 148;
				break;
			}
		}
	}*/
}

/*
 * Ensure the dungeon is 'minimally' traversible.
 */
static void L2LockoutFix()
{
	int i, j;
	bool doorok;

	/* commented out, because this is no longer necessary
	for (j = 0; j < DMAXY; j++) {
		for (i = 0; i < DMAXX; i++) {
			// replace doors with walls if there is no empty space in front
			if (dungeon[i][j] == 4 && dungeon[i - 1][j] != 3) {
				dungeon[i][j] = 1;
			}
			if (dungeon[i][j] == 5 && dungeon[i][j - 1] != 3) {
				dungeon[i][j] = 2;
			}
		}
	}*/
	// make sure there is a (single) door on the horizontal walls
	for (j = 1; j < DMAXY - 1; j++) {
		for (i = 1; i < DMAXX - 1; i++) {
			if (drlgFlags[i][j] != 0) {
				continue;
			}
			doorok = dungeon[i][j] == 5;
			if ((dungeon[i][j] == 2 || doorok) && dungeon[i][j - 1] == 3 && dungeon[i][j + 1] == 3) {
				//assert(j >= 1 && j <= DMAXY - 2);
				while (TRUE) {
					i++;
					//assert(i < DMAXX);
					if (dungeon[i][j - 1] != 3 || dungeon[i][j + 1] != 3) {
						break;
					}
					if (dungeon[i][j] == 5) {
						if (doorok && !drlgFlags[i][j])
							dungeon[i][j] = 2;
						doorok = true;
					} else if (dungeon[i][j] != 2)
						break;
				}
				if (!doorok && !drlgFlags[i - 1][j]) {
					dungeon[i - 1][j] = 5;
				}
			}
		}
	}
	// make sure there is a (single) door on the vertical walls
	for (i = 0; i < DMAXX; i++) {
		for (j = 0; j < DMAXY; j++) {
			if (drlgFlags[i][j]) {
				continue;
			}
			doorok = dungeon[i][j] == 4;
			if ((dungeon[i][j] == 1 || doorok) && dungeon[i - 1][j] == 3 && dungeon[i + 1][j] == 3) {
				//assert(i >= 1 && i <= DMAXX - 2);
				while (TRUE) {
					j++;
					//assert(j < DMAXY);
					if (dungeon[i - 1][j] != 3 || dungeon[i + 1][j] != 3) {
						break;
					}
					if (dungeon[i][j] == 4) {
						if (doorok && !drlgFlags[i][j])
							dungeon[i][j] = 1;
						doorok = true;
					} else if (dungeon[i][j] != 1)
						break;
				}
				if (!doorok && !drlgFlags[i][j - 1]) {
					dungeon[i][j - 1] = 4;
				}
			}
		}
	}
}

static void L2DoorFix()
{
	/* commented out, because this is no longer necessary
	int i, j;

	for (j = 1; j < DMAXY; j++) {
		for (i = 1; i < DMAXX; i++) {
			// vertical door with empty space at the bottom
			if (dungeon[i][j] == 4 && dungeon[i][j - 1] == 3) {
				dungeon[i][j] = 7;
			}
			// horizontal door with empty space on the back
			if (dungeon[i][j] == 5 && dungeon[i - 1][j] == 3) {
				dungeon[i][j] = 9;
			}
		}
	}*/
}

static bool IsPillar(BYTE bv)
{
	return (bv >= 6 && bv <= 9) || (bv >= 146 && bv <= 149);
}

/*
 * Move doors away from pillars.
 */
static void L2DoorFix2()
{
	int i, j;

	for (i = 1; i < DMAXX - 1; i++) {
		for (j = 1; j < DMAXY - 1; j++) {
			if (dungeon[i][j] != 4 || drlgFlags[i][j])
				continue;
			if (IsPillar(dungeon[i][j + 1])) {
				//3, 1, 3,  search
				//0, 4, 0,
				//0, P, 0,

				//0, 4, 0, replace
				//0, 1, 0,
				//0, 0, 0,
				if (dungeon[i][j - 1] == 1
				 && dungeon[i - 1][j - 1] == 3
				 && dungeon[i + 1][j - 1] == 3) {
					dungeon[i][j] = 1;
					dungeon[i][j - 1] = 4;
				}
			} else if (IsPillar(dungeon[i][j - 1])) {
				//0, P, 0,  search
				//0, 4, 0,
				//3, 1, 3,

				//0, 0, 0, replace
				//0, 1, 0,
				//0, 4, 0,
				if (dungeon[i][j + 1] == 1
				 && dungeon[i - 1][j + 1] == 3
				 && dungeon[i + 1][j + 1] == 3) {
					dungeon[i][j] = 1;
					dungeon[i][j + 1] = 4;
				}
			} else if (IsPillar(dungeon[i + 1][j])) {
				//3, 0, 0,  search
				//1, 4, P,
				//3, 0, 0,

				//0, 0, 0, replace
				//4, 1, 0,
				//0, 0, 0,
				if (dungeon[i - 1][j] == 1
				 && dungeon[i - 1][j + 1] == 3
				 && dungeon[i - 1][j - 1] == 3) {
					dungeon[i][j] = 1;
					dungeon[i - 1][j] = 4;
				}
			} else if (IsPillar(dungeon[i - 1][j])) {
				//0, 0, 3,  search
				//P, 4, 1,
				//0, 0, 3,

				//0, 0, 0, replace
				//0, 1, 4,
				//0, 0, 0,
				if (dungeon[i + 1][j] == 1
				 && dungeon[i + 1][j - 1] == 3
				 && dungeon[i + 1][j + 1] == 3) {
					dungeon[i][j] = 1;
					dungeon[i + 1][j] = 4;
				}
			}
		}
	}
}

/*
 * Replace doors with arches.
 * TODO: skip if there is no corresponding shadow?
 * New dungeon values: (3) 39 40 41 42 43
 */
static void L2CreateArches()
{
	BYTE pn;
	int x, y;
	// TODO: skip replace if there is no matching shadow?
	for (x = 0; x < DMAXX; x++) {
		for (y = 0; y < DMAXY; y++) {
			if (drlgFlags[x][y])
				continue;
			if (dungeon[x][y] == 4) {
				// vertical door
				// assert(y > 0 && y < DMAXY - 1);
				pn = dungeon[x][y + 1];
				if (IsPillar(pn)) {
					pn = dungeon[x][y - 1];
					// assert(!drlgFlags[x][y - 1]);
					if (pn == 1 || pn == 7) {
						// 1/7,  search
						// 4,
						// P,

						//39, replace
						//3,
						//0,
						dungeon[x][y - 1] = 39;
						dungeon[x][y] = 3;
					} else if (pn == 8) {
						// 8,  search
						// 4,
						// P,

						//42, replace
						//3,
						//0,
						dungeon[x][y - 1] = 42;
						dungeon[x][y] = 3;
					} else if (pn == 43) {
						// 43,  search
						// 4,
						// P,

						//41, replace
						//3,
						//0,
						dungeon[x][y - 1] = 41;
						dungeon[x][y] = 3;
					}
				} else if (pn == 1 && y < DMAXY - 2) {
					if (IsPillar(dungeon[x][y + 2])) {
						// 4,  search
						// 1,
						// P,

						//39, replace
						//3,
						//0,
						// assert(!drlgFlags[x][y + 1]);
						dungeon[x][y + 1] = 3;
						dungeon[x][y] = 39;
					}
				}
			} else if (dungeon[x][y] == 5) {
				// horizontal door
				// assert(x > 0 && x < DMAXX - 1);
				pn = dungeon[x + 1][y];
				if (IsPillar(pn)) {
					pn = dungeon[x - 1][y];
					// assert(!drlgFlags[x - 1][y]);
					if (pn == 2 || pn == 9) {
						// 2/9, 5, P,  search

						//40, 3, 0, replace
						dungeon[x - 1][y] = 40;
						dungeon[x][y] = 3;
					} else if (pn == 8) {
						// 8, 5, P,  search

						//43, 3, 0, replace
						dungeon[x - 1][y] = 43;
						dungeon[x][y] = 3;
					}
				} else if (pn == 2 && x < DMAXX - 2) {
					if (IsPillar(dungeon[x + 2][y])) {
						// 5, 2, P,  search

						//40, 3, 0, replace
						// assert(!drlgFlags[x + 1][y]);
						dungeon[x + 1][y] = 3;
						dungeon[x][y] = 40;
					}
				}
			}
		}
	}
}

static void DRLG_L2()
{
	while (true) {
		do {
			static_assert(sizeof(pdungeon) == DMAXX * DMAXY, "Linear traverse of pdungeon does not work in DRLG_L2.");
			memset(pdungeon, 32, sizeof(pdungeon));
			DRLG_L2CreateDungeon();
		} while (!DL2_FillVoids());

		DRLG_L2MakeMegas();

		L2TileFix();
		memset(drlgFlags, 0, sizeof(drlgFlags));
		if (pSetPieces[0]._spData != NULL) { // pSetPieces[0]._sptype != SPT_NONE
			DRLG_L2SetRoom(0);
		}

		POS32 warpPos = DRLG_PlaceMiniSet(L2USTAIRS); // L2USTAIRS (5, 3)
		if (warpPos.x < 0) {
			continue;
		}
		pWarps[DWARP_ENTRY]._wx = warpPos.x + 2;
		pWarps[DWARP_ENTRY]._wy = warpPos.y + 1;
		pWarps[DWARP_ENTRY]._wx = 2 * pWarps[DWARP_ENTRY]._wx + DBORDERX;
		pWarps[DWARP_ENTRY]._wy = 2 * pWarps[DWARP_ENTRY]._wy + DBORDERY;
		pWarps[DWARP_ENTRY]._wtype = WRPT_L2_UP;
		warpPos = DRLG_PlaceMiniSet(L2DSTAIRS); // L2DSTAIRS (3, 5)
		if (warpPos.x < 0) {
			continue;
		}
		pWarps[DWARP_EXIT]._wx = warpPos.x + 2;
		pWarps[DWARP_EXIT]._wy = warpPos.y + 2;
		pWarps[DWARP_EXIT]._wx = 2 * pWarps[DWARP_EXIT]._wx + DBORDERX;
		pWarps[DWARP_EXIT]._wy = 2 * pWarps[DWARP_EXIT]._wy + DBORDERY;
		pWarps[DWARP_EXIT]._wtype = WRPT_L2_DOWN;
		if (currLvl._dLevelIdx == DLV_CATACOMBS1) {
			warpPos = DRLG_PlaceMiniSet(L2TWARP); // L2TWARP (5, 3)
			if (warpPos.x < 0) {
				continue;
			}
			pWarps[DWARP_TOWN]._wx = warpPos.x + 2;
			pWarps[DWARP_TOWN]._wy = warpPos.y + 1;
			pWarps[DWARP_TOWN]._wx = 2 * pWarps[DWARP_TOWN]._wx + DBORDERX;
			pWarps[DWARP_TOWN]._wy = 2 * pWarps[DWARP_TOWN]._wy + DBORDERY;
			pWarps[DWARP_TOWN]._wtype = WRPT_L2_UP;
		}

		if (pSetPieces[0]._sptype == SPT_BCHAMB) {
			pWarps[DWARP_SIDE]._wx = pSetPieces[0]._spx + 3; // L2USTAIRS (5, 3)
			pWarps[DWARP_SIDE]._wy = pSetPieces[0]._spy + 3;
			pWarps[DWARP_SIDE]._wx = 2 * pWarps[DWARP_SIDE]._wx + DBORDERX;
			pWarps[DWARP_SIDE]._wy = 2 * pWarps[DWARP_SIDE]._wy + DBORDERY;
			pWarps[DWARP_SIDE]._wtype = WRPT_L2_UP;
		}
		break;
	}

	L2LockoutFix();
	// L2DoorFix();

	DRLG_PlaceThemeRooms(6, 10, DEFAULT_MEGATILE_L2, 0, false);

	L2CreateArches();
	L2DoorFix2();

	DRLG_L2Shadows();
	DRLG_L2Corners();

	DRLG_L2PlaceRndSet(CRUSHCOL, 99);
	//DRLG_L2PlaceRndSet(RUINS1, 10);
	//DRLG_L2PlaceRndSet(RUINS2, 10);
	//DRLG_L2PlaceRndSet(RUINS3, 10);
	//DRLG_L2PlaceRndSet(RUINS4, 10);
	//DRLG_L2PlaceRndSet(RUINS5, 10);
	//DRLG_L2PlaceRndSet(RUINS6, 10);
	//DRLG_L2PlaceRndSet(RUINS7, 50);
	DRLG_L2PlaceRndSet(PANCREAS1, 1);
	DRLG_L2PlaceRndSet(PANCREAS2, 1);
	DRLG_L2PlaceRndSet(BIG1, 3);
	DRLG_L2PlaceRndSet(BIG2, 3);
	DRLG_L2PlaceRndSet(BIG3, 3);
	DRLG_L2PlaceRndSet(BIG4, 3);
	DRLG_L2PlaceRndSet(BIG5, 3);
	DRLG_L2PlaceRndSet(BIG6, 20);
	DRLG_L2PlaceRndSet(BIG7, 20);
	DRLG_L2PlaceRndSet(BIG8, 3);
	DRLG_L2PlaceRndSet(BIG9, 20);
	DRLG_L2PlaceRndSet(BIG10, 20);
	DRLG_L2Subs();
	DRLG_L2DoorSubs();

	memcpy(pdungeon, dungeon, sizeof(pdungeon));

	if (pSetPieces[0]._sptype == SPT_BLIND) {
		// load pre-map
		MemFreeDbg(pSetPieces[0]._spData);
		pSetPieces[0]._spData = LoadFileInMem("Levels\\L2Data\\Blind2.DUN");
		// patch the map - Blind2.DUN
		// replace the door with wall
		pSetPieces[0]._spData[(2 + 4 + 3 * 11) * 2] = 25;
		// protect the inner subtiles
		for (int y = 0; y < 7; y++) {
			for (int x = 0; x < 7; x++) {
				pSetPieces[0]._spData[(2 + 11 * 11 + x + y * 11) * 2] = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
			}
		}
		for (int y = 4; y < 11; y++) {
			for (int x = 4; x < 11; x++) {
				pSetPieces[0]._spData[(2 + 11 * 11 + x + y * 11) * 2] = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
			}
		}
		DRLG_DrawMap(0);
	} else if (pSetPieces[0]._sptype == SPT_BLOOD) {
		// load pre-map
		MemFreeDbg(pSetPieces[0]._spData);
		pSetPieces[0]._spData = LoadFileInMem("Levels\\L2Data\\Blood2.DUN");
		if (pSetPieces[0]._spData != NULL) {
		// patch the map - Blood2.DUN
		uint16_t* lm = (uint16_t*)pSetPieces[0]._spData;
		// - place pieces with closed doors
		lm[2 + 4 + 10 * 10] = SwapLE16(151);
		lm[2 + 4 + 15 * 10] = SwapLE16(151);
		lm[2 + 5 + 15 * 10] = SwapLE16(151);
		// shadow of the external-left column -- do not place to prevent overwriting large decorations
		//dungeon[pSetPieces[0]._spx - 1][pSetPieces[0]._spy + 7] = 48;
		//dungeon[pSetPieces[0]._spx - 1][pSetPieces[0]._spy + 8] = 50;
		// - shadow of the bottom-left column(s) -- one is missing
		lm[2 + 1 + 13 * 10] = SwapLE16(48);
		lm[2 + 1 + 14 * 10] = SwapLE16(50);
		// - shadow of the internal column next to the pedistal
		lm[2 + 5 + 7 * 10] = SwapLE16(142);
		lm[2 + 5 + 8 * 10] = SwapLE16(50);
		// - add book and pedistal
		lm[2 + 10 * 16 + 10 * 16 * 2 * 2 + 10 * 16 * 2 * 2 + 9 + 24 * 10 * 2] = SwapLE16(15);
		lm[2 + 10 * 16 + 10 * 16 * 2 * 2 + 10 * 16 * 2 * 2 + 9 + 16 * 10 * 2] = SwapLE16(91);
		// - remove torches
		lm[2 + 10 * 16 + 10 * 16 * 2 * 2 + 10 * 16 * 2 * 2 + 11 + 8 * 10 * 2] = 0;
		lm[2 + 10 * 16 + 10 * 16 * 2 * 2 + 10 * 16 * 2 * 2 + 11 + 10 * 10 * 2] = 0;
		lm[2 + 10 * 16 + 10 * 16 * 2 * 2 + 10 * 16 * 2 * 2 + 11 + 12 * 10 * 2] = 0;
		lm[2 + 10 * 16 + 10 * 16 * 2 * 2 + 10 * 16 * 2 * 2 + 6 + 8 * 10 * 2] = 0;
		lm[2 + 10 * 16 + 10 * 16 * 2 * 2 + 10 * 16 * 2 * 2 + 6 + 10 * 10 * 2] = 0;
		lm[2 + 10 * 16 + 10 * 16 * 2 * 2 + 10 * 16 * 2 * 2 + 6 + 12 * 10 * 2] = 0;
		// protect the main room
		for (int y = 7; y < 16; y++) {
			for (int x = 2; x < 8; x++) {
				lm[2 + 10 * 16 + x + y * 10] = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
			}
		}
		}
		DRLG_DrawMap(0);
	} else if (pSetPieces[0]._sptype == SPT_BCHAMB) {
		// load pre-map
		MemFreeDbg(pSetPieces[0]._spData);
		pSetPieces[0]._spData = LoadFileInMem("Levels\\L2Data\\Bonestr1.DUN");
		if (pSetPieces[0]._spData != NULL) {
		// patch the map - Bonestr1.DUN
		// shadow of the external-left column
		pSetPieces[0]._spData[(2 + 0 + 4 * 7) * 2] = 48;
		pSetPieces[0]._spData[(2 + 0 + 5 * 7) * 2] = 50;
		// protect the main room
		for (int y = 1; y < 6; y++) {
			for (int x = 1; x < 6; x++) {
				pSetPieces[0]._spData[(2 + 7 * 7 + x + y * 7) * 2] = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
			}
		}
		}
		DRLG_DrawMap(0);
	}
}

void DRLG_InitL2Specials(int x1, int y1, int x2, int y2)
{
	int i, j, pn;

	for (j = y1; j <= y2; j++) {
		for (i = x1; i <= x2; i++) {
			pn = dPiece[i][j];
			// 13 and 17 are open doors
			// 178, 551 and 553 are doorways (TODO: add 541 and 542?)
			if (pn == 13 || pn == 178 || pn == 551)
				pn = 5;
			else if (pn == 17 || pn == 553)
				pn = 6;
			else
				pn = 0;
			dSpecial[i][j] = pn;
		}
	}
	for (j = y1; j <= y2; j++) {
		for (i = x1; i <= x2; i++) {
			pn = dPiece[i][j];
			// 132 is L-arch
			// 135 and 139 are R-arch
			if (pn == 132) {
				dSpecial[i][j + 1] = 2;
				dSpecial[i][j + 2] = 1;
			} else if (pn == 135 || pn == 139) {
				dSpecial[i + 1][j] = 3;
				dSpecial[i + 2][j] = 4;
			}
		}
	}
}

void CreateL2Dungeon()
{
	// in the original version the function was executed twice in case the quest of the
	// current level was not available (only in single player mode). The point of this
	// could have been to share the same layout between levels, but that does not make too
	// much sense due to the stairs placement are 'wrong' anyway. Just to have a reasonable
	// sized main room, changing DRLG_L2CreateDungeon would have been much cheaper solution.
	DRLG_LoadL2SP();
	DRLG_L2();
	DRLG_PlaceMegaTiles(BASE_MEGATILE_L2);
	DRLG_L2InitTransVals();
	DRLG_Init_Globals();
	DRLG_InitL2Specials(DBORDERX, DBORDERY, MAXDUNX - DBORDERX - 1, MAXDUNY - DBORDERY - 1);
	DRLG_SetPC();
}

static void DRLG_L2SetMapFix()
{
	// this logic should not be applied to 'proper' set-levels.
	assert(currLvl._dLevelIdx == SL_BONECHAMB);
	// patch the map - Bonecha1.DUN
	// place pieces with closed doors
	dungeon[17][11] = 150;
	// place shadows
	// - right corridor
	dungeon[12][6] = 47;
	dungeon[12][7] = 51;
	dungeon[16][6] = 47;
	dungeon[16][7] = 51;
	dungeon[16][8] = 47;
	// - central room (top)
	dungeon[17][8] = 49;
	dungeon[18][8] = 46;
	dungeon[19][8] = 49;
	dungeon[20][8] = 46;
	// - central room (bottom)
	dungeon[18][12] = 46;
	dungeon[19][12] = 49;
	// - left corridor
	dungeon[12][14] = 47;
	dungeon[12][15] = 51;
	dungeon[16][14] = 47;
	dungeon[16][15] = 51;
	// fix corners
	DRLG_L2Corners();
}

static void LoadL2DungeonData(const char* sFileName)
{
	// memset(drlgFlags, 0, sizeof(drlgFlags)); - unused on setmaps
	static_assert(sizeof(dungeon[0][0]) == 1, "memset on dungeon does not work in LoadL2DungeonData.");
	memset(dungeon, BASE_MEGATILE_L2 + 1, sizeof(dungeon));

	pSetPieces[0]._spx = 0;
	pSetPieces[0]._spy = 0;
	pSetPieces[0]._spData = LoadFileInMem(sFileName);
	if (pSetPieces[0]._spData == NULL) {
		return;
	}

	DRLG_LoadSP(0, DEFAULT_MEGATILE_L2);
}

void LoadL2Dungeon(const LevelData* lds)
{
	pWarps[DWARP_ENTRY]._wx = lds->dSetLvlDunX;
	pWarps[DWARP_ENTRY]._wy = lds->dSetLvlDunY;
	pWarps[DWARP_ENTRY]._wtype = lds->dSetLvlWarp;

	// load pre-dungeon
	LoadL2DungeonData(lds->dSetLvlPreDun);

	MemFreeDbg(pSetPieces[0]._spData);

	memcpy(pdungeon, dungeon, sizeof(pdungeon));

	// load dungeon
	LoadL2DungeonData(lds->dSetLvlDun);
	DRLG_L2SetMapFix();

	DRLG_PlaceMegaTiles(BASE_MEGATILE_L2);
	DRLG_L2InitTransVals();
	DRLG_Init_Globals();
	DRLG_InitL2Specials(DBORDERX, DBORDERY, MAXDUNX - DBORDERX - 1, MAXDUNY - DBORDERY - 1);

	SetMapMonsters(pSetPieces[0]._spData, 0, 0);
	SetMapObjects(pSetPieces[0]._spData);

	MemFreeDbg(pSetPieces[0]._spData);
}

DEVILUTION_END_NAMESPACE
