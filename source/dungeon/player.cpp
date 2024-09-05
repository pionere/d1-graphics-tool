/**
 * @file player.cpp
 *
 * Implementation of player functionality, leveling, actions, creation, loading, etc.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

int mypnum;
PlayerStruct players[MAX_PLRS];

/**
 * @param c plr_classes value
 */
void CreatePlayer(const _uiheroinfo& heroinfo)
{
	int val, hp, mana;
	int i, pnum = 0;

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

void IncreasePlrStr(int pnum)
{
	int v;

	if ((unsigned)pnum >= MAX_PLRS) {
		dev_fatal("IncreasePlrStr: illegal player %d", pnum);
	}
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

	if ((unsigned)pnum >= MAX_PLRS) {
		dev_fatal("IncreasePlrMag: illegal player %d", pnum);
	}
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

	if ((unsigned)pnum >= MAX_PLRS) {
		dev_fatal("IncreasePlrDex: illegal player %d", pnum);
	}
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

	if ((unsigned)pnum >= MAX_PLRS) {
		dev_fatal("IncreasePlrVit: illegal player %d", pnum);
	}
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

void RestorePlrHpVit(int pnum)
{
	int hp;

	if ((unsigned)pnum >= MAX_PLRS) {
		dev_fatal("RestorePlrHpVit: illegal player %d", pnum);
	}
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


DEVILUTION_END_NAMESPACE
