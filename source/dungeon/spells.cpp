/**
 * @file spells.cpp
 *
 * Implementation of functionality for casting player spells.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

int GetManaAmount(int pnum, int sn)
{
	// mana amount, spell level, adjustment, min mana
	int ma, sl, adj, mm;

	ma = spelldata[sn].sManaCost;
	if (sn == SPL_HEAL || sn == SPL_HEALOTHER) {
		ma += 2 * plr._pLevel;
	}

	sl = plr._pSkillLvl[sn] - 1;
	if (sl < 0)
		sl = 0;
	adj = sl * spelldata[sn].sManaAdj;
	adj >>= 1;
	ma -= adj;
	mm = spelldata[sn].sMinMana;
	if (mm > ma)
		ma = mm;
	ma <<= 6;

	//return ma * (100 - plr._pISplCost) / 100;
	return ma;
}

int GetSkillCost(int sn, int sklvl, int plvl)
{
	// mana amount, spell level, adjustment, min mana
	int ma, sl, adj, mm;

	ma = spelldata[sn].sManaCost;
	if (sn == SPL_HEAL || sn == SPL_HEALOTHER) {
		ma += 2 * plvl;
	}

	sl = sklvl - 1;
	if (sl < 0)
		sl = 0;
	adj = sl * spelldata[sn].sManaAdj;
	adj >>= 1;
	ma -= adj;
	mm = spelldata[sn].sMinMana;
	if (mm > ma)
		ma = mm;
	// ma <<= 6;

	//return ma * (100 - plr._pISplCost) / 100;
	return ma;
}

bool HasSkillDamage(int sn)
{
    bool result = false;
    switch (sn) {
    case SPL_NULL:
    case SPL_WALK:
    case SPL_BLOCK:
    case SPL_RAGE:
    case SPL_SHROUD:
    case SPL_SWAMP:
    case SPL_STONE:
    case SPL_INFRA:
    case SPL_MANASHIELD:
    case SPL_ATTRACT:
    case SPL_TELEKINESIS:
    case SPL_TELEPORT:
    case SPL_RNDTELEPORT:
    case SPL_TOWN:
    case SPL_HEAL:
    case SPL_HEALOTHER:
    case SPL_RESURRECT:
    case SPL_IDENTIFY:
    case SPL_OIL:
    case SPL_REPAIR:
    case SPL_RECHARGE:
    case SPL_DISARM:
#ifdef HELLFIRE
    case SPL_BUCKLE:
    case SPL_WHITTLE:
    case SPL_RUNESTONE:
#endif
         result = false; break;
    case SPL_ATTACK:
    case SPL_WHIPLASH:
    case SPL_WALLOP:
    case SPL_SWIPE:
    case SPL_RATTACK:
    case SPL_POINT_BLANK:
    case SPL_FAR_SHOT:
    case SPL_PIERCE_SHOT:
    case SPL_MULTI_SHOT:
    case SPL_CHARGE:
    case SPL_FIREBOLT:
    case SPL_CBOLT:
    case SPL_HBOLT:
    case SPL_LIGHTNING:
    case SPL_FLASH:
    case SPL_FIREWALL:
    case SPL_FIREBALL:
    case SPL_METEOR:
    case SPL_BLOODBOIL:
    case SPL_CHAIN:
    case SPL_WAVE:
    case SPL_NOVA:
    case SPL_INFERNO:
    case SPL_ELEMENTAL:
    case SPL_FLARE:
    case SPL_POISON:
    case SPL_WIND:
    case SPL_GUARDIAN:
    case SPL_GOLEM:
#ifdef HELLFIRE
	//case SPL_LIGHTWALL:
	//case SPL_IMMOLAT:
    case SPL_FIRERING:
    case SPL_RUNEFIRE:
    case SPL_RUNELIGHT:
    case SPL_RUNENOVA:
    case SPL_RUNEWAVE:
#endif
        result = true; break;
    }
    return result;
}

BYTE GetSkillElement(int sn)
{
    BYTE res = MISR_NONE;
    if (spelldata[sn].sType != STYPE_NONE || (spelldata[sn].sUseFlags & SFLAG_RANGED)) {
        res = GetMissileElement(spelldata[sn].sMissile);
    }
    return res;
}

void GetSkillName(int sn)
{
    snprintf(infostr, sizeof(infostr), SPELL_RUNE(sn) ? "%s (rune)" : "%s", spelldata[sn].sNameText);
}

DEVILUTION_END_NAMESPACE
