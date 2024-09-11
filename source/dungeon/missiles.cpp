/**
 * @file missiles.cpp
 *
 * Implementation of missile functionality.
 */
#include "all.h"

#include "../d1hro.h"

DEVILUTION_BEGIN_NAMESPACE

void GetSkillDesc(D1Hero *hero, int sn, int sl)
{
	int k, magic, mind, maxd;

	// assert((unsigned)sn < NUM_SPELLS);
    magic = hero->getMagic(); //  myplr._pMagic;
#ifdef HELLFIRE
	if (SPELL_RUNE(sn))
		sl += hero->getDexterity() /*myplr._pDexterity*/ >> 3;
#endif
	switch (sn) {
	case SPL_FIREBOLT:
	case SPL_GUARDIAN:
		k = (magic >> 3) + sl;
		mind = k + 1;
		maxd = k + 10;
		break;
#ifdef HELLFIRE
	case SPL_RUNELIGHT:
#endif
	case SPL_LIGHTNING:
		mind = 1;
		maxd = ((magic + (sl << 3)) * (6 + (sl >> 1))) >> 3;
		break;
	case SPL_FLASH:
		mind = magic >> 1;
		for (k = 0; k < sl; k++)
			mind += mind >> 3;

		mind *= misfiledata[MFILE_BLUEXFR].mfAnimLen[0];
		maxd = mind << 3;
		mind >>= 6;
		maxd >>= 6;
		break;
	case SPL_NULL:
	case SPL_WALK:
	case SPL_BLOCK:
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
	case SPL_RAGE:
	case SPL_SHROUD:
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
	case SPL_REPAIR:
	case SPL_RECHARGE:
	case SPL_DISARM:
#ifdef HELLFIRE
	case SPL_BUCKLE:
	case SPL_WHITTLE:
	case SPL_RUNESTONE:
#endif
		mind = -1;
		maxd = -1;
		break;
#ifdef HELLFIRE
	case SPL_FIRERING:
#endif
	case SPL_FIREWALL:
		mind = ((magic >> 3) + sl + 5) << (-3 + 5);
		maxd = ((magic >> 3) + sl * 2 + 10) << (-3 + 5);
		break;
	case SPL_FIREBALL:
		mind = (magic >> 2) + 10;
		maxd = mind + 10;
		for (k = 0; k < sl; k++) {
			mind += mind >> 3;
			maxd += maxd >> 3;
		}
		break;
	case SPL_METEOR:
		mind = (magic >> 2) + (sl << 3) + 40;
		maxd = (magic >> 2) + (sl << 4) + 40;
		break;
	case SPL_BLOODBOIL:
		mind = (magic >> 2) + (sl << 2) + 10;
		maxd = (magic >> 2) + (sl << 3) + 10;
		break;
	case SPL_CHAIN:
		mind = 1;
		maxd = magic;
		break;
#ifdef HELLFIRE
	case SPL_RUNEWAVE:
#endif
	case SPL_WAVE:
		mind = ((magic >> 3) + 2 * sl + 1) * 4;
		maxd = ((magic >> 3) + 4 * sl + 2) * 4;
		break;
#ifdef HELLFIRE
	case SPL_RUNENOVA:
#endif
	case SPL_NOVA:
		mind = 1;
		maxd = (magic >> 1) + (sl << 5);
		break;
	case SPL_INFERNO:
		mind = (magic * 20) >> 6;
		maxd = ((magic + (sl << 4)) * 30) >> 6;
		break;
	case SPL_GOLEM:
		mind = 2 * sl + 8;
		maxd = 2 * sl + 16;
		break;
	case SPL_ELEMENTAL:
		mind = (magic >> 3) + 2 * sl + 4;
		maxd = (magic >> 3) + 4 * sl + 20;
		for (k = 0; k < sl; k++) {
			mind += mind >> 3;
			maxd += maxd >> 3;
		}
		break;
	case SPL_CBOLT:
		mind = 1;
		maxd = (magic >> 2) + (sl << 2);
		break;
	case SPL_HBOLT:
		mind = (magic >> 2) + sl;
		maxd = mind + 9;
		break;
	case SPL_FLARE:
		mind = (magic * (sl + 1)) >> 3;
		maxd = mind;
		break;
	case SPL_POISON:
		mind = ((magic >> 4) + sl + 2) << (-3 + 5);
		maxd = ((magic >> 4) + sl + 4) << (-3 + 5);
		break;
	case SPL_WIND:
		mind = (magic >> 3) + 7 * sl + 1;
		maxd = (magic >> 3) + 8 * sl + 1;
		// (dam * 2 * misfiledata[MFILE_WIND].mfAnimLen[0] / 16) << (-3 + 5)
		mind = mind * 3;
		maxd = maxd * 3;
		break;
#ifdef HELLFIRE
	/*case SPL_LIGHTWALL:
		mind = 1;
		maxd = ((magic >> 1) + sl) << (-3 + 5);
		break;
	case SPL_RUNEWAVE:
	case SPL_IMMOLAT:
		mind = 1 + (magic >> 3);
		maxd = mind + 4;
		for (k = 0; k < sl; k++) {
			mind += mind >> 3;
			maxd += maxd >> 3;
		}
		break;*/
	case SPL_RUNEFIRE:
		mind = 1 + (magic >> 1) + 16 * sl;
		maxd = 1 + (magic >> 1) + 32 * sl;
		break;
#endif
	default:
		ASSUME_UNREACHABLE
		break;
	}

    if (mind != -1) {
        snprintf(infostr, sizeof(infostr), "Dam: %d-%d", mind, maxd);
    } else {
        infostr[0] = '\0';
    }
}

DEVILUTION_END_NAMESPACE
