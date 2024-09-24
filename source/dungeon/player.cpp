/**
 * @file player.cpp
 *
 * Implementation of player functionality, leveling, actions, creation, loading, etc.
 */
#include "all.h"
#include <time.h>

#include <QApplication>
#include <QMessageBox>

DEVILUTION_BEGIN_NAMESPACE

int mypnum;
PlayerStruct players[MAX_PLRS];
/** The current player while processing the players. */
static BYTE gbGameLogicPnum;

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
   STAND, WALK, ATTACK, SPELL, BLOCK, GOTHIT, DEATH
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

static void SetPlrAnims(int pnum)
{
	int pc, gn;
#if 0
	if ((unsigned)pnum >= MAX_PLRS) {
		dev_fatal("SetPlrAnims: illegal player %d", pnum);
	}
	plr._pAnims[PGX_STAND].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_WALK].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_ATTACK].paAnimWidth = 128 * ASSET_MPL;
	plr._pAnims[PGX_FIRE].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_LIGHTNING].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_MAGIC].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_BLOCK].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_GOTHIT].paAnimWidth = 96 * ASSET_MPL;
	plr._pAnims[PGX_DEATH].paAnimWidth = 128 * ASSET_MPL;
#endif
	pc = plr._pClass;
	plr._pAFNum = PlrGFXAnimActFrames[pc][0];
	plr._pSFNum = PlrGFXAnimActFrames[pc][1];

	plr._pAnims[PGX_STAND].paFrames = PlrGFXAnimLens[pc][PA_STAND];
	plr._pAnims[PGX_WALK].paFrames = PlrGFXAnimLens[pc][PA_WALK];
	plr._pAnims[PGX_ATTACK].paFrames = PlrGFXAnimLens[pc][PA_ATTACK];
	plr._pAnims[PGX_FIRE].paFrames = PlrGFXAnimLens[pc][PA_SPELL];
	plr._pAnims[PGX_LIGHTNING].paFrames = PlrGFXAnimLens[pc][PA_SPELL];
	plr._pAnims[PGX_MAGIC].paFrames = PlrGFXAnimLens[pc][PA_SPELL];
	plr._pAnims[PGX_BLOCK].paFrames = PlrGFXAnimLens[pc][PA_BLOCK];
	plr._pAnims[PGX_GOTHIT].paFrames = PlrGFXAnimLens[pc][PA_GOTHIT];
	plr._pAnims[PGX_DEATH].paFrames = PlrGFXAnimLens[pc][PA_DEATH];

	gn = plr._pgfxnum & 0xF;
	switch (pc) {
	case PC_WARRIOR:
		if (gn == ANIM_ID_BOW) {
			plr._pAnims[PGX_STAND].paFrames = 8;
//			plr._pAnims[PGX_ATTACK].paAnimWidth = 96 * ASSET_MPL;
			plr._pAFNum = 11;
		} else if (gn == ANIM_ID_AXE) {
			plr._pAnims[PGX_ATTACK].paFrames = 20;
			plr._pAFNum = 10;
		} else if (gn == ANIM_ID_STAFF) {
			// plr._pAnims[PGX_ATTACK].paFrames = 16;
			plr._pAFNum = 11;
		}
		break;
	case PC_ROGUE:
		if (gn == ANIM_ID_AXE) {
			plr._pAnims[PGX_ATTACK].paFrames = 22;
			plr._pAFNum = 13;
		} else if (gn == ANIM_ID_BOW) {
			plr._pAnims[PGX_ATTACK].paFrames = 12;
			plr._pAFNum = 7;
		} else if (gn == ANIM_ID_STAFF) {
			plr._pAnims[PGX_ATTACK].paFrames = 16;
			plr._pAFNum = 11;
		}
		break;
	case PC_SORCERER:
//		plr._pAnims[PGX_FIRE].paAnimWidth = 128 * ASSET_MPL;
//		plr._pAnims[PGX_LIGHTNING].paAnimWidth = 128 * ASSET_MPL;
//		plr._pAnims[PGX_MAGIC].paAnimWidth = 128 * ASSET_MPL;
		if (gn == ANIM_ID_UNARMED) {
			plr._pAnims[PGX_ATTACK].paFrames = 20;
		} else if (gn == ANIM_ID_UNARMED_SHIELD) {
			plr._pAFNum = 9;
		} else if (gn == ANIM_ID_BOW) {
			plr._pAnims[PGX_ATTACK].paFrames = 20;
			plr._pAFNum = 16;
		} else if (gn == ANIM_ID_AXE) {
			plr._pAnims[PGX_ATTACK].paFrames = 24;
			plr._pAFNum = 16;
		}
		break;
#ifdef HELLFIRE
	case PC_MONK:
#if 0
		plr._pAnims[PGX_STAND].paAnimWidth = 112 * ASSET_MPL;
		plr._pAnims[PGX_WALK].paAnimWidth = 112 * ASSET_MPL;
		plr._pAnims[PGX_ATTACK].paAnimWidth = 130 * ASSET_MPL;
		plr._pAnims[PGX_FIRE].paAnimWidth = 114 * ASSET_MPL;
		plr._pAnims[PGX_LIGHTNING].paAnimWidth = 114 * ASSET_MPL;
		plr._pAnims[PGX_MAGIC].paAnimWidth = 114 * ASSET_MPL;
		plr._pAnims[PGX_BLOCK].paAnimWidth = 98 * ASSET_MPL;
		plr._pAnims[PGX_GOTHIT].paAnimWidth = 98 * ASSET_MPL;
		plr._pAnims[PGX_DEATH].paAnimWidth = 160 * ASSET_MPL;
#endif
		switch (gn) {
		case ANIM_ID_UNARMED:
		case ANIM_ID_UNARMED_SHIELD:
			plr._pAnims[PGX_ATTACK].paFrames = 12;
			plr._pAFNum = 7;
			break;
		case ANIM_ID_BOW:
			plr._pAnims[PGX_ATTACK].paFrames = 20;
			plr._pAFNum = 14;
			break;
		case ANIM_ID_AXE:
			plr._pAnims[PGX_ATTACK].paFrames = 23;
			plr._pAFNum = 14;
			break;
		case ANIM_ID_STAFF:
			plr._pAnims[PGX_ATTACK].paFrames = 13;
			plr._pAFNum = 8;
			break;
		}
		break;
	case PC_BARD:
		if (gn == ANIM_ID_AXE) {
			plr._pAnims[PGX_ATTACK].paFrames = 22;
			plr._pAFNum = 13;
		} else if (gn == ANIM_ID_BOW) {
			plr._pAnims[PGX_ATTACK].paFrames = 12;
			plr._pAFNum = 11;
		} else if (gn == ANIM_ID_STAFF) {
			plr._pAnims[PGX_ATTACK].paFrames = 16;
			plr._pAFNum = 11;
		} else if (gn == ANIM_ID_SWORD_SHIELD || gn == ANIM_ID_SWORD) {
			plr._pAnims[PGX_ATTACK].paFrames = 10; // TODO: check for onehanded swords or daggers?
		}
		break;
	case PC_BARBARIAN:
		if (gn == ANIM_ID_AXE) {
			plr._pAnims[PGX_ATTACK].paFrames = 20;
			plr._pAFNum = 8;
		} else if (gn == ANIM_ID_BOW) {
			plr._pAnims[PGX_STAND].paFrames = 8;
//			plr._pAnims[PGX_ATTACK].paAnimWidth = 96 * ASSET_MPL;
			plr._pAFNum = 11;
		} else if (gn == ANIM_ID_STAFF) {
			//plr._pAnims[PGX_ATTACK].paFrames = 16;
			plr._pAFNum = 11;
		} else if (gn == ANIM_ID_MACE || gn == ANIM_ID_MACE_SHIELD) {
			plr._pAFNum = 8;
		}
		break;
#endif
	default:
		ASSUME_UNREACHABLE
		break;
	}
#if 0
	if (currLvl._dType == DTYPE_TOWN) {
		plr._pAnims[PGX_STAND].paFrames = 20;
		//plr._pAnims[PGX_WALK].paFrames = 8;
	}
#endif
}

/**
 * @param c plr_classes value
 */
void CreatePlayer(int pnum, const _uiheroinfo& heroinfo)
{
	int val, hp, mana;
	int i; // , pnum = 0;

	memset(&plr, 0, sizeof(PlayerStruct));
	SetRndSeed(time(NULL));

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

static void ProcessPlayer(int pnum)
{
	plr._pAnimCnt++;
	if (plr._pAnimCnt >= plr._pAnimFrameLen) {
		// if (plr._pmode == PM_WALK || plr._pmode == PM_WALK2 || lastMode == PM_WALK || lastMode == PM_WALK2)
		//	LogErrorF("step anim %d(%d) at %d:%d", plr._pAnimFrame, plr._pAnimCnt, plr._px, plr._py);
		plr._pAnimCnt = 0;
		plr._pAnimFrame++;
		if (plr._pAnimFrame > plr._pAnimLen) {
			plr._pAnimFrame = 1;
		}
	}
}

static void NewPlrAnim(int pnum, unsigned animIdx) //, int dir)
{
	PlrAnimStruct* anim;

	anim = &plr._pAnims[animIdx];

	// plr._pdir = dir;
	// plr._pAnimData = anim->paAnimData[dir];
	plr._pAnimLen = anim->paFrames;
	plr._pAnimFrame = 1;
	plr._pAnimCnt = (gbGameLogicProgress < GLP_PLAYERS_DONE && gbGameLogicPnum <= pnum) ? -1 : 0;
	plr._pAnimFrameLen = PlrAnimFrameLens[animIdx];
	// plr._pAnimWidth = anim->paAnimWidth;
	// plr._pAnimXOffset = (anim->paAnimWidth - TILE_WIDTH) >> 1;
}

static void StartWalk(int pnum)
{
	// plr._pmode = PM_WALK;
	plr._pVar8 = 0;

	NewPlrAnim(pnum, PGX_WALK); //, dir);
}

static inline void PlrStepAnim(int pnum)
{
	plr._pAnimCnt++;
	if (plr._pAnimCnt >= plr._pAnimFrameLen) {
		// LogErrorF("skip anim %d(%d) at %d:%d", plr._pAnimFrame, plr._pAnimCnt, plr._px, plr._py);
		plr._pAnimCnt = 0;
		plr._pAnimFrame++;
	}
}

int GetWalkSpeedInTicks(int pnum)
{
	gbGameLogicProgress = GLP_NONE;
	gbGameLogicPnum = pnum;

	SetPlrAnims(pnum);

	StartWalk(pnum);

	int result = 0;
	while (true) {
		bool stepAnim = false;

		plr._pVar8++; // WALK_TICK
		switch (plr._pIWalkSpeed) {
		case 0:
			stepAnim = false;
			break;
		case 1:
			stepAnim = (plr._pVar8 & 3) == 2;
			break;
		case 2:
			stepAnim = (plr._pVar8 & 1) == 1;
			break;
		case 3:
			stepAnim = true;
			break;
		default:
			ASSUME_UNREACHABLE
			break;
		}
		if (stepAnim) {
			PlrStepAnim(pnum);
		}
		if (plr._pAnimFrame < plr._pAnimLen) {
			result++;

			ProcessPlayer(pnum);
			continue;
		}
		break;
	}
	return result;
}

static void StartAttack(int pnum)
{
	int sn, ss;

	// plr._pmode = PM_ATTACK;
	sn = plr._pDestParam3;
	ss = plr._pIBaseAttackSpeed;
	if (sn == SPL_WHIPLASH) {
		ss += 3;
		if (ss > 4)
			ss = 4;
	} else if (sn == SPL_WALLOP) {
		ss -= 3;
	}
	plr._pVar4 = ss; // ATTACK_SPEED
	plr._pVar8 = 0;  // ATTACK_TICK

	NewPlrAnim(pnum, PGX_ATTACK); //, dir);
}

int GetAttackSpeedInTicks(int pnum, int sn)
{
	gbGameLogicProgress = GLP_NONE;
	gbGameLogicPnum = pnum;

	SetPlrAnims(pnum);

	plr._pDestParam3 = sn;

	StartAttack(pnum);

	int res = 0, result = 0;
	while (true) {
		bool stepAnim = false;

		plr._pVar8++;         // ATTACK_TICK
		switch (plr._pVar4) { // ATTACK_SPEED
		/*case -4:
			if ((plr._pVar8 & 1) == 1)
				plr._pAnimCnt--;
			break;*/
		case -3:
			if ((plr._pVar8 % 3u) == 0)
				plr._pAnimCnt--;
			break;
		case -2:
			if ((plr._pVar8 & 3) == 2)
				plr._pAnimCnt--;
			break;
		case -1:
			if ((plr._pVar8 & 7) == 4)
				plr._pAnimCnt--;
			break;
		case 0:
			break;
		case 1:
			stepAnim = (plr._pVar8 & 7) == 4;
			break;
		case 2:
			stepAnim = (plr._pVar8 & 3) == 2;
			break;
		case 3:
			stepAnim = (plr._pVar8 & 1) == 1;
			break;
		case 4:
			stepAnim = true;
			break;
		default:
			ASSUME_UNREACHABLE
			break;
		}
		if (stepAnim) {
			PlrStepAnim(pnum);
		}
		if (plr._pAnimFrame < plr._pAnimLen) {
			if (plr._pAnimFrame <= plr._pAFNum)
				res++;
			result++;

			ProcessPlayer(pnum);
			continue;
		}
		break;
	}
	return result;
}

static void StartSpell(int pnum)
{
	int animIdx;
	const SpellData* sd;

	plr._pVar8 = 0;                // SPELL_TICK
	plr._pVar5 = plr._pDestParam3; // SPELL_NUM

	sd = &spelldata[plr._pVar5]; // SPELL_NUM
	animIdx = PGX_FIRE + sd->sType - STYPE_FIRE;

	NewPlrAnim(pnum, animIdx); //, plr._pdir);
}

int GetCastSpeedInTicks(int pnum, int sn)
{
	gbGameLogicProgress = GLP_NONE;
	gbGameLogicPnum = pnum;

	SetPlrAnims(pnum);

	plr._pDestParam3 = sn;

	StartSpell(pnum);

	int res = 0, result = 0;
	while (true) {
		bool stepAnim = false;

		plr._pVar8++; // SPELL_TICK
		switch (plr._pIBaseCastSpeed) {
		case 0:
			stepAnim = false;
			break;
		case 1:
			stepAnim = (plr._pVar8 & 3) == 2;
			break;
		case 2:
			stepAnim = (plr._pVar8 & 1) == 1;
			break;
		case 3:
			stepAnim = true;
			break;
		default:
			ASSUME_UNREACHABLE
			break;
		}
		if (stepAnim) {
			PlrStepAnim(pnum);
		}
		if (plr._pAnimFrame < plr._pAnimLen) {
			if (plr._pAnimFrame <= plr._pSFNum)
				res++;
			result++;

			ProcessPlayer(pnum);
			continue;
		}
		break;
	}
	return result;
}

static void PlrStartGetHit(int pnum) // , int dir)
{
	NewPlrAnim(pnum, PGX_GOTHIT); //, dir);

	plr._pVar8 = 0; // GOTHIT_TICK
}

int GetRecoverySpeedInTicks(int pnum)
{
	gbGameLogicProgress = GLP_NONE;
	gbGameLogicPnum = pnum;

	SetPlrAnims(pnum);

	PlrStartGetHit(pnum);

	int res = 0, result = 0;
	while (true) {
		bool stepAnim = false;

		plr._pVar8++; // GOTHIT_TICK
		switch (plr._pIRecoverySpeed) {
		case 0:
			stepAnim = false;
			break;
		case 1:
			stepAnim = (plr._pVar8 & 3) == 2;
			break;
		case 2:
			stepAnim = (plr._pVar8 & 1) == 1;
			break;
		case 3:
			stepAnim = true;
			break;
		default:
			ASSUME_UNREACHABLE
			break;
		}
		if (stepAnim) {
			PlrStepAnim(pnum);
		}
		if (plr._pAnimFrame < plr._pAnimLen) {
			// if (plr._pAnimFrame <= plr._pSFNum)
			res++;
			result++;

			ProcessPlayer(pnum);
			continue;
		}
		break;
	}
	return result;
}

int GetChargeSpeed(int pnum)
{
	int result = 2;
	if (plr._pIWalkSpeed != 0) {
		if (plr._pIWalkSpeed == 3) {
			// ISPL_FASTESTWALK
			result = 4;
		} else {
			// (ISPL_FASTERWALK | ISPL_FASTWALK)
			result = 3;
		}
	}
	return result;
}

void GetMonByPlrDamage(int pnum, int sn, int sl, const MonsterStruct *mon, int *mindam, int *maxdam)
{
	int mind, maxd, damsl, dambl, dampc;
	bool tmac;

	mind = 0;
	maxd = 0;
	tmac = (plr._pIFlags & ISPL_PENETRATE_PHYS) != 0;
	damsl = plr._pISlMaxDam;
	if (damsl != 0) {
		maxd += CalcMonsterDam(mon->_mMagicRes, MISR_SLASH, damsl, tmac);
		mind += CalcMonsterDam(mon->_mMagicRes, MISR_SLASH, plr._pISlMinDam, tmac);
	}
	dambl = plr._pIBlMaxDam;
	if (dambl != 0) {
		maxd += CalcMonsterDam(mon->_mMagicRes, MISR_BLUNT, dambl, tmac);
		mind += CalcMonsterDam(mon->_mMagicRes, MISR_BLUNT, plr._pIBlMinDam, tmac);
	}
	dampc = plr._pIPcMaxDam;
	if (dampc != 0) {
		maxd += CalcMonsterDam(mon->_mMagicRes, MISR_PUNCTURE, dampc, tmac);
		mind += CalcMonsterDam(mon->_mMagicRes, MISR_PUNCTURE, plr._pIPcMinDam, tmac);
	}

	// tmp = sn == SPL_SWIPE ? 800 : 200;
	// if (random_low(6, tmp) < plr._pICritChance) {
	//	dam <<= 1;
	// }

	switch (sn) {
	case SPL_ATTACK:
		break;
	case SPL_SWIPE:
		mind = (mind * (48 + sl)) >> 6;
		maxd = (maxd * (48 + sl)) >> 6;
		break;
	case SPL_WALLOP:
		mind = (mind * (112 + sl)) >> 6;
		maxd = (maxd * (112 + sl)) >> 6;
		break;
	case SPL_WHIPLASH:
		mind = (mind * (24 + sl)) >> 6;
		maxd = (maxd * (24 + sl)) >> 6;
		break;
	default:
		QMessageBox::critical(nullptr, "Error", QApplication::tr("Unhandled h2h skill %1 in GetMonByPlrDamage.").arg(sn));
		ASSUME_UNREACHABLE
		break;
	}

	int fdam = plr._pIFMaxDam;
	if (fdam != 0) {
		maxd += CalcMonsterDam(mon->_mMagicRes, MISR_FIRE, fdam, false);
		mind += CalcMonsterDam(mon->_mMagicRes, MISR_FIRE, plr._pIFMinDam, false);
	}
	int ldam = plr._pILMaxDam;
	if (ldam != 0) {
		maxd += CalcMonsterDam(mon->_mMagicRes, MISR_LIGHTNING, ldam, false);
		mind += CalcMonsterDam(mon->_mMagicRes, MISR_LIGHTNING, plr._pILMinDam, false);
	}
	int mdam = plr._pIMMaxDam;
	if (mdam != 0) {
		maxd += CalcMonsterDam(mon->_mMagicRes, MISR_MAGIC, mdam, false);
		mind += CalcMonsterDam(mon->_mMagicRes, MISR_MAGIC, plr._pIMMinDam, false);
	}
	int adam = plr._pIAMaxDam;
	if (adam != 0) {
		maxd += CalcMonsterDam(mon->_mMagicRes, MISR_ACID, adam, false);
		mind += CalcMonsterDam(mon->_mMagicRes, MISR_ACID, plr._pIAMinDam, false);
	}

	mind >>= 6;
	maxd >>= 6;

	*mindam = mind;
	*maxdam = maxd;
}

void GetPlrByPlrDamage(int offp, int sn, int sl, int pnum, int *mindam, int *maxdam)
{
	int mind, maxd, damsl, dambl, dampc;

	mind = 0;
	maxd = 0;
	damsl = plx(offp)._pISlMaxDam;
	if (damsl != 0) {
		maxd += CalcPlrDam(pnum, MISR_SLASH, damsl);
		mind += CalcPlrDam(pnum, MISR_SLASH, plx(offp)._pISlMinDam);
	}
	dambl = plx(offp)._pIBlMaxDam;
	if (dambl != 0) {
		maxd += CalcPlrDam(pnum, MISR_BLUNT, dambl);
		mind += CalcPlrDam(pnum, MISR_BLUNT, plx(offp)._pIBlMinDam);
	}
	dampc = plx(offp)._pIPcMaxDam;
	if (dampc != 0) {
		maxd += CalcPlrDam(pnum, MISR_PUNCTURE, dampc);
		mind += CalcPlrDam(pnum, MISR_PUNCTURE, plx(offp)._pIPcMinDam);
	}

	// tmp = sn == SPL_SWIPE ? 800 : 200;
	// if (random_low(6, tmp) < plx(offp)._pICritChance) {
	// 	dam <<= 1;
	// }

	switch (sn) {
	case SPL_ATTACK:
		break;
	case SPL_SWIPE:
		mind = (mind * (48 + sl)) >> 6;
		maxd = (maxd * (48 + sl)) >> 6;
		break;
	case SPL_WALLOP:
		mind = (mind * (112 + sl)) >> 6;
		maxd = (maxd * (112 + sl)) >> 6;
		break;
	case SPL_WHIPLASH:
		mind = (mind * (24 + sl)) >> 6;
		maxd = (maxd * (24 + sl)) >> 6;
		break;
	default:
        QMessageBox::critical(nullptr, "Error", QApplication::tr("Unhandled h2h skill %1 in GetPlrByPlrDamage.").arg(sn));
		ASSUME_UNREACHABLE
		break;
	}

	int fdam = plx(offp)._pIFMaxDam;
	if (fdam != 0) {
		maxd += CalcPlrDam(pnum, MISR_FIRE, fdam);
		mind += CalcPlrDam(pnum, MISR_FIRE, plx(offp)._pIFMinDam);
	}
	int ldam = plx(offp)._pILMaxDam;
	if (ldam != 0) {
		maxd += CalcPlrDam(pnum, MISR_LIGHTNING, ldam);
		mind += CalcPlrDam(pnum, MISR_LIGHTNING, plx(offp)._pILMinDam);
	}
	int mdam = plx(offp)._pIMMaxDam;
	if (mdam != 0) {
		maxd += CalcPlrDam(pnum, MISR_LIGHTNING, mdam);
		mind += CalcPlrDam(pnum, MISR_LIGHTNING, plx(offp)._pIMMinDam);
	}
	int adam = plx(offp)._pIAMaxDam;
	if (adam != 0) {
		maxd += CalcPlrDam(pnum, MISR_ACID, adam);
		mind += CalcPlrDam(pnum, MISR_ACID, plx(offp)._pIAMinDam);
	}

	mind >>= 6;
	maxd >>= 6;

	*mindam = mind;
	*maxdam = maxd;
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

void DecreasePlrMag(int pnum)
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

void DecreasePlrDex(int pnum)
{
	int v;

	if (plr._pBaseDex <= DexterityTbl[plr._pClass])
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

void DecreasePlrVit(int pnum)
{
	int v, ms;

	if (plr._pBaseVit <= VitalityTbl[plr._pClass])
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
