/**
 * @file player.cpp
 *
 * Implementation of player functionality, leveling, actions, creation, loading, etc.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

int mypnum;
PlayerStruct players[MAX_PLRS];

/* Data related to the player-animation types. */
static const PlrAnimType PlrAnimTypes[NUM_PGTS] = {
	// clang-format off
	{ "ST", PGX_STAND },     // PGT_STAND_TOWN
	{ "AS", PGX_STAND },     // PGT_STAND_DUNGEON
	{ "WL", PGX_WALK },      // PGT_WALK_TOWN
	{ "AW", PGX_WALK },      // PGT_WALK_DUNGEON
	{ "AT", PGX_ATTACK },    // PGT_ATTACK
	{ "FM", PGX_FIRE },      // PGT_FIRE
	{ "LM", PGX_LIGHTNING }, // PGT_LIGHTNING
	{ "QM", PGX_MAGIC },     // PGT_MAGIC
	{ "BL", PGX_BLOCK },     // PGT_BLOCK
	{ "HT", PGX_GOTHIT },    // PGT_GOTHIT
	{ "DT", PGX_DEATH },     // PGT_DEATH
	// clang-format on
};
/**
 * Specifies the number of frames of each animation for each player class.
   STAND, ATTACK, WALK, BLOCK, DEATH, SPELL, GOTHIT
 */
const BYTE PlrGFXAnimLens[NUM_CLASSES][NUM_PLR_ANIMS] = {
	// clang-format off
	{ 10, 8, 16, 20, 2, 6, 20 },
	{  8, 8, 18, 16, 4, 7, 20 },
	{  8, 8, 16, 12, 6, 8, 20 },
#ifdef HELLFIRE
	{  8, 8, 16, 18, 3, 6, 20 },
	{  8, 8, 18, 16, 4, 7, 20 },
	{ 10, 8, 16, 20, 2, 6, 20 },
#endif
	// clang-format on
};
/** Specifies the frame of attack and spell animation for which the action is triggered, for each player class. */
const BYTE PlrGFXAnimActFrames[NUM_CLASSES][2] = {
	// clang-format off
	{  9, 14 },
	{ 10, 12 },
	{ 12,  9 },
#ifdef HELLFIRE
	{ 12, 13 },
	{ 10, 12 },
	{  9, 14 },
#endif
	// clang-format on
};
/** Specifies the length of a frame for each animation (player_graphic_idx). */
const BYTE PlrAnimFrameLens[NUM_PGXS] = { 4, 1, 1, 1, 1, 1, 3, 1, 2 };

/** Maps from player_class to starting stat in strength. */
const int StrengthTbl[NUM_CLASSES] = {
	// clang-format off
	20,
	15,
	10,
#ifdef HELLFIRE
	20,
	15,
	35,
#endif
	// clang-format on
};
/** Maps from player_class to starting stat in magic. */
const int MagicTbl[NUM_CLASSES] = {
	// clang-format off
	10,
	20,
	30,
#ifdef HELLFIRE
	15,
	20,
	 0,
#endif
	// clang-format on
};
/** Maps from player_class to starting stat in dexterity. */
const int DexterityTbl[NUM_CLASSES] = {
	// clang-format off
	20,
	25,
	20,
#ifdef HELLFIRE
	20,
	25,
	10,
#endif
	// clang-format on
};
/** Maps from player_class to starting stat in vitality. */
const int VitalityTbl[NUM_CLASSES] = {
	// clang-format off
	30,
	20,
	20,
#ifdef HELLFIRE
	25,
	20,
	35,
#endif
	// clang-format on
};
const BYTE Abilities[NUM_CLASSES] = {
	SPL_REPAIR, SPL_DISARM, SPL_RECHARGE,
#ifdef HELLFIRE
	SPL_WHITTLE, SPL_IDENTIFY, SPL_BUCKLE,
#endif
};

/** Specifies the experience point limit of each player level. */
const unsigned PlrExpLvlsTbl[MAXCHARLEVEL + 1] = {
	0,
	2000,
	4620,
	8040,
	12489,
	18258,
	25712,
	35309,
	47622,
	63364,
	83419,
	108879,
	141086,
	181683,
	231075,
	313656,
	424067,
	571190,
	766569,
	1025154,
	1366227,
	1814568,
	2401895,
	3168651,
	4166200,
	5459523,
	7130496,
	9281874,
	12042092,
	15571031,
	20066900,
	25774405,
	32994399,
	42095202,
	53525811,
	67831218,
	85670061,
	107834823,
	135274799,
	169122009,
	210720231,
	261657253,
	323800420,
	399335440,
	490808349,
	601170414,
	733825617,
	892680222,
	1082908612,
	1310707109,
	1583495809
};

/** Specifies the experience point limit of skill-level. */
const unsigned SkillExpLvlsTbl[MAXSPLLEVEL + 1] = {
	8040,
	25712,
	63364,
	141086,
	313656,
	766569,
	1814568,
	4166200,
	9281874,
	20066900,
	42095202,
	85670061,
	169122009,
	323800420,
	601170414,
	1082908612,
};


/**
 * @param c plr_classes value
 */
void CreatePlayer(int pnum, const _uiheroinfo& heroinfo)
{
	int val, hp, mana;
	int i; // , pnum = 0;

	memset(&plr, 0, sizeof(PlayerStruct));
	//SetRndSeed(SDL_GetTicks());

	plr._pLevel = heroinfo.hiLevel;
	plr._pClass = heroinfo.hiClass;
	//plr._pRank = heroinfo.hiRank;
	copy_cstr(plr._pName, heroinfo.hiName);

	val = heroinfo.hiStrength;
	//plr._pStrength = val;
	plr._pBaseStr = val;

	val = heroinfo.hiDexterity;
	//plr._pDexterity = val;
	plr._pBaseDex = val;

	val = heroinfo.hiVitality;
	//plr._pVitality = val;
	plr._pBaseVit = val;

	hp = val << (6 + 1);
	/*plr._pHitPoints = plr._pMaxHP =*/ plr._pHPBase = plr._pMaxHPBase = hp;

	val = heroinfo.hiMagic;
	//plr._pMagic = val;
	plr._pBaseMag = val;

	mana = val << (6 + 1);
	/*plr._pMana = plr._pMaxMana =*/ plr._pManaBase = plr._pMaxManaBase = mana;

	//plr._pNextExper = PlrExpLvlsTbl[1];
	plr._pLightRad = 10;

	//plr._pAblSkills = SPELL_MASK(Abilities[c]);
	//plr._pAblSkills |= SPELL_MASK(SPL_WALK) | SPELL_MASK(SPL_ATTACK) | SPELL_MASK(SPL_RATTACK) | SPELL_MASK(SPL_BLOCK);

	//plr._pAtkSkill = SPL_ATTACK;
	//plr._pAtkSkillType = RSPLTYPE_ABILITY;
	//plr._pMoveSkill = SPL_WALK;
	//plr._pMoveSkillType = RSPLTYPE_ABILITY;
	//plr._pAltAtkSkill = SPL_INVALID;
	//plr._pAltAtkSkillType = RSPLTYPE_INVALID;
	//plr._pAltMoveSkill = SPL_INVALID;
	//plr._pAltMoveSkillType = RSPLTYPE_INVALID;

	for (i = 0; i < lengthof(plr._pAtkSkillHotKey); i++)
		plr._pAtkSkillHotKey[i] = SPL_INVALID;
	for (i = 0; i < lengthof(plr._pAtkSkillTypeHotKey); i++)
		plr._pAtkSkillTypeHotKey[i] = RSPLTYPE_INVALID;
	for (i = 0; i < lengthof(plr._pMoveSkillHotKey); i++)
		plr._pMoveSkillHotKey[i] = SPL_INVALID;
	for (i = 0; i < lengthof(plr._pMoveSkillTypeHotKey); i++)
		plr._pMoveSkillTypeHotKey[i] = RSPLTYPE_INVALID;
	for (i = 0; i < lengthof(plr._pAltAtkSkillHotKey); i++)
		plr._pAltAtkSkillHotKey[i] = SPL_INVALID;
	for (i = 0; i < lengthof(plr._pAltAtkSkillTypeHotKey); i++)
		plr._pAltAtkSkillTypeHotKey[i] = RSPLTYPE_INVALID;
	for (i = 0; i < lengthof(plr._pAltMoveSkillHotKey); i++)
		plr._pAltMoveSkillHotKey[i] = SPL_INVALID;
	for (i = 0; i < lengthof(plr._pAltMoveSkillTypeHotKey); i++)
		plr._pAltMoveSkillTypeHotKey[i] = RSPLTYPE_INVALID;
	for (i = 0; i < lengthof(plr._pAtkSkillSwapKey); i++)
		plr._pAtkSkillSwapKey[i] = SPL_INVALID;
	for (i = 0; i < lengthof(plr._pAtkSkillTypeSwapKey); i++)
		plr._pAtkSkillTypeSwapKey[i] = RSPLTYPE_INVALID;
	for (i = 0; i < lengthof(plr._pMoveSkillSwapKey); i++)
		plr._pMoveSkillSwapKey[i] = SPL_INVALID;
	for (i = 0; i < lengthof(plr._pMoveSkillTypeSwapKey); i++)
		plr._pMoveSkillTypeSwapKey[i] = RSPLTYPE_INVALID;
	for (i = 0; i < lengthof(plr._pAltAtkSkillSwapKey); i++)
		plr._pAltAtkSkillSwapKey[i] = SPL_INVALID;
	for (i = 0; i < lengthof(plr._pAltAtkSkillTypeSwapKey); i++)
		plr._pAltAtkSkillTypeSwapKey[i] = RSPLTYPE_INVALID;
	for (i = 0; i < lengthof(plr._pAltMoveSkillSwapKey); i++)
		plr._pAltMoveSkillSwapKey[i] = SPL_INVALID;
	for (i = 0; i < lengthof(plr._pAltMoveSkillTypeSwapKey); i++)
		plr._pAltMoveSkillTypeSwapKey[i] = RSPLTYPE_INVALID;

	if (plr._pClass == PC_SORCERER) {
		plr._pSkillLvlBase[SPL_FIREBOLT] = 2;
		plr._pSkillExp[SPL_FIREBOLT] = SkillExpLvlsTbl[1];
		plr._pMemSkills = SPELL_MASK(SPL_FIREBOLT);
	}

	CreatePlrItems(pnum);

	// TODO: at the moment player is created and right after that unpack is called
	//  this makes the two calls below unnecessary, but CreatePlayer would be more
	//  complete if these are enabled...
	//InitPlayer(pnum);
	//CalcPlrInv(pnum, false);

	//SetRndSeed(0);
}

/*
 * Initialize player fields at startup(unpack).
 *  - calculate derived values
 */
void InitPlayer(int pnum)
{
	dev_assert((unsigned)pnum < MAX_PLRS, "InitPlayer: illegal player %d", pnum);
	
	// calculate derived values
	CalculateGold(pnum);

	plr._pNextExper = PlrExpLvlsTbl[plr._pLevel];

	plr._pAblSkills = SPELL_MASK(Abilities[plr._pClass]);
	plr._pAblSkills |= SPELL_MASK(SPL_WALK) | SPELL_MASK(SPL_BLOCK) | SPELL_MASK(SPL_ATTACK) | SPELL_MASK(SPL_RATTACK);

	plr._pWalkpath[MAX_PATH_LENGTH] = DIR_NONE;
}

void ClrPlrPath(int pnum)
{
	dev_assert((unsigned)pnum < MAX_PLRS, "ClrPlrPath: illegal player %d", pnum);

	plr._pWalkpath[0] = DIR_NONE;
	//memset(plr._pWalkpath, DIR_NONE, sizeof(plr._pWalkpath));
}

void IncreasePlrStr(int pnum)
{
	int v;

	dev_assert((unsigned)pnum < MAX_PLRS, "IncreasePlrStr: illegal player %d", pnum);
	if (plr._pStatPts <= 0)
		return;
	plr._pStatPts--;
	switch (plr._pClass) {
	case PC_WARRIOR:	v = (((plr._pBaseStr - StrengthTbl[PC_WARRIOR]) % 5) == 2) ? 3 : 2; break;
	case PC_ROGUE:		v = 1; break;
	case PC_SORCERER:	v = 1; break;
#ifdef HELLFIRE
	case PC_MONK:		v = 2; break;
	case PC_BARD:		v = 1; break;
	case PC_BARBARIAN:	v = 3; break;
#endif
	default:
		ASSUME_UNREACHABLE
		break;
	}
	//plr._pStrength += v;
	plr._pBaseStr += v;

	CalcPlrInv(pnum, true);
}

void IncreasePlrMag(int pnum)
{
	int v, ms;

	dev_assert((unsigned)pnum < MAX_PLRS, "IncreasePlrMag: illegal player %d", pnum);
	if (plr._pStatPts <= 0)
		return;
	plr._pStatPts--;
	switch (plr._pClass) {
	case PC_WARRIOR:	v = 1; break;
	case PC_ROGUE:		v = 2; break;
	case PC_SORCERER:	v = 3; break;
#ifdef HELLFIRE
	case PC_MONK:		v = (((plr._pBaseMag - MagicTbl[PC_MONK]) % 3) == 1) ? 2 : 1; break;
	case PC_BARD:		v = (((plr._pBaseMag - MagicTbl[PC_BARD]) % 3) == 1) ? 2 : 1; break;
	case PC_BARBARIAN:	v = 1; break;
#endif
	default:
		ASSUME_UNREACHABLE
		break;
	}

	//plr._pMagic += v;
	plr._pBaseMag += v;

	ms = v << (6 + 1);

	plr._pMaxManaBase += ms;
	//plr._pMaxMana += ms;
	//if (!(plr._pIFlags & ISPL_NOMANA)) {
		plr._pManaBase += ms;
		//plr._pMana += ms;
	//}

	CalcPlrInv(pnum, true);
}

void IncreasePlrDex(int pnum)
{
	int v;

	dev_assert((unsigned)pnum < MAX_PLRS, "IncreasePlrDex: illegal player %d", pnum);
	if (plr._pStatPts <= 0)
		return;
	plr._pStatPts--;
	switch (plr._pClass) {
	case PC_WARRIOR:	v = (((plr._pBaseDex - DexterityTbl[PC_WARRIOR]) % 3) == 1) ? 2 : 1; break;
	case PC_ROGUE:		v = 3; break;
	case PC_SORCERER:	v = (((plr._pBaseDex - DexterityTbl[PC_SORCERER]) % 3) == 1) ? 2 : 1; break;
#ifdef HELLFIRE
	case PC_MONK:		v = 2; break;
	case PC_BARD:		v = 3; break;
	case PC_BARBARIAN:	v = 1; break;
#endif
	default:
		ASSUME_UNREACHABLE
		break;
	}

	//plr._pDexterity += v;
	plr._pBaseDex += v;

	CalcPlrInv(pnum, true);
}

void IncreasePlrVit(int pnum)
{
	int v, ms;

	dev_assert((unsigned)pnum < MAX_PLRS, "IncreasePlrVit: illegal player %d", pnum);
	if (plr._pStatPts <= 0)
		return;
	plr._pStatPts--;
	switch (plr._pClass) {
	case PC_WARRIOR:	v = 2; break;
	case PC_ROGUE:		v = 1; break;
	case PC_SORCERER:	v = (((plr._pBaseVit - VitalityTbl[PC_SORCERER]) % 3) == 1) ? 2 : 1; break;
#ifdef HELLFIRE
	case PC_MONK:		v = (((plr._pBaseVit - VitalityTbl[PC_MONK]) % 3) == 1) ? 2 : 1; break;
	case PC_BARD:		v = (((plr._pBaseVit - VitalityTbl[PC_BARD]) % 3) == 1) ? 2 : 1; break;
	case PC_BARBARIAN:	v = 2; break;
#endif
	default:
		ASSUME_UNREACHABLE
		break;
	}

	//plr._pVitality += v;
	plr._pBaseVit += v;

	ms = v << (6 + 1);

	plr._pHPBase += ms;
	plr._pMaxHPBase += ms;
	//plr._pHitPoints += ms;
	//plr._pMaxHP += ms;

	CalcPlrInv(pnum, true);
}

void DecreasePlrMaxHp(int pnum)
{
	int tmp;
	dev_assert((unsigned)pnum < MAX_PLRS, "DecreasePlrMaxHp: illegal player %d", pnum);
	if (plr._pMaxHPBase > (1 << 6) && plr._pMaxHP > (1 << 6)) {
		tmp = plr._pMaxHP - (1 << 6);
		plr._pMaxHP = tmp;
		if (plr._pHitPoints > tmp) {
			plr._pHitPoints = tmp;
		}
		tmp = plr._pMaxHPBase - (1 << 6);
		plr._pMaxHPBase = tmp;
		if (plr._pHPBase > tmp) {
			plr._pHPBase = tmp;
		}
	}
}

void RestorePlrHpVit(int pnum)
{
	int hp;

	dev_assert((unsigned)pnum < MAX_PLRS, "RestorePlrHpVit: illegal player %d", pnum);
	// base hp
	hp = plr._pBaseVit << (6 + 1);

	// check the delta
	hp -= plr._pMaxHPBase;
	assert(hp >= 0);

	// restore the lost hp
	plr._pMaxHPBase += hp;
	//plr._pMaxHP += hp;

	// fill hp
	plr._pHPBase = plr._pMaxHPBase;
	//PlrFillHp(pnum);

	CalcPlrInv(pnum, true);
}

void DecreasePlrStr(int pnum)
{
	int v;

	if (plr._pBaseStr <= StrengthTbl[plr._pClass])
		return;
	switch (plr._pClass) {
	case PC_WARRIOR:	v = (((plr._pBaseStr - StrengthTbl[PC_WARRIOR] - 3) % 5) == 2) ? 3 : 2; break;
	case PC_ROGUE:		v = 1; break;
	case PC_SORCERER:	v = 1; break;
#ifdef HELLFIRE
	case PC_MONK:		v = 2; break;
	case PC_BARD:		v = 1; break;
	case PC_BARBARIAN:	v = 3; break;
#endif
	default:
		ASSUME_UNREACHABLE
		break;
	}
	//plr._pStrength -= v;
	plr._pBaseStr -= v;

	plr._pStatPts++;

	CalcPlrInv(pnum, true);
}

void IncreasePlrMag(int pnum)
{
	int v, ms;

	if (plr._pBaseMag <= MagicTbl[plr._pClass])
		return;
	switch (plr._pClass) {
	case PC_WARRIOR:	v = 1; break;
	case PC_ROGUE:		v = 2; break;
	case PC_SORCERER:	v = 3; break;
#ifdef HELLFIRE
	case PC_MONK:		v = (((plr._pBaseMag - MagicTbl[PC_MONK] - 2) % 3) == 1) ? 2 : 1; break;
	case PC_BARD:		v = (((plr._pBaseMag - MagicTbl[PC_BARD] - 2) % 3) == 1) ? 2 : 1; break;
	case PC_BARBARIAN:	v = 1; break;
#endif
	default:
		ASSUME_UNREACHABLE
		break;
	}

	//plr._pMagic -= v;
	plr._pBaseMag -= v;

	plr._pStatPts++;

	ms = v << (6 + 1);

	plr._pMaxManaBase -= ms;
	//plr._pMaxMana -= ms;
	//if (!(plr._pIFlags & ISPL_NOMANA)) {
		plr._pManaBase -= ms;
		//plr._pMana -= ms;
	//}

	CalcPlrInv(pnum, true);
}

void IncreasePlrDex(int pnum)
{
	int v;

	if (plr._pBaseDex <= MagicTbl[plr._pClass])
		return;
	switch (plr._pClass) {
	case PC_WARRIOR:	v = (((plr._pBaseDex - DexterityTbl[PC_WARRIOR] - 2) % 3) == 1) ? 2 : 1; break;
	case PC_ROGUE:		v = 3; break;
	case PC_SORCERER:	v = (((plr._pBaseDex - DexterityTbl[PC_SORCERER] - 2) % 3) == 1) ? 2 : 1; break;
#ifdef HELLFIRE
	case PC_MONK:		v = 2; break;
	case PC_BARD:		v = 3; break;
	case PC_BARBARIAN:	v = 1; break;
#endif
	default:
		ASSUME_UNREACHABLE
		break;
	}

	//plr._pDexterity -= v;
	plr._pBaseDex -= v;

	plr._pStatPts++;

	CalcPlrInv(pnum, true);
}

void IncreasePlrVit(int pnum)
{
	int v, ms;

	if (plr._pBaseVit <= MagicTbl[plr._pClass])
		return;
	switch (plr._pClass) {
	case PC_WARRIOR:	v = 2; break;
	case PC_ROGUE:		v = 1; break;
	case PC_SORCERER:	v = (((plr._pBaseVit - VitalityTbl[PC_SORCERER] - 2) % 3) == 1) ? 2 : 1; break;
#ifdef HELLFIRE
	case PC_MONK:		v = (((plr._pBaseVit - VitalityTbl[PC_MONK] - 2) % 3) == 1) ? 2 : 1; break;
	case PC_BARD:		v = (((plr._pBaseVit - VitalityTbl[PC_BARD] - 2) % 3) == 1) ? 2 : 1; break;
	case PC_BARBARIAN:	v = 2; break;
#endif
	default:
		ASSUME_UNREACHABLE
		break;
	}

	//plr._pVitality -= v;
	plr._pBaseVit -= v;

	plr._pStatPts++;

	ms = v << (6 + 1);

	plr._pHPBase -= ms;
	plr._pMaxHPBase -= ms;
	//plr._pHitPoints += ms;
	//plr._pMaxHP += ms;

	CalcPlrInv(pnum, true);
}


DEVILUTION_END_NAMESPACE
