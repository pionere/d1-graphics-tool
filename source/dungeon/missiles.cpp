/**
 * @file missiles.cpp
 *
 * Implementation of missile functionality.
 */
#include "all.h"
#include "misproc.h"

#include <QApplication>
#include <QMessageBox>

#include "../d1hro.h"

DEVILUTION_BEGIN_NAMESPACE

#define MIS_VELO_SHIFT      0
#define MIS_BASE_VELO_SHIFT 16
#define MIS_SHIFTEDVEL(x)   ((x) << MIS_VELO_SHIFT)

static const BYTE BloodBoilLocs[][2] = {
	// clang-format off
	{ 3, 4 },  { 2, 1 },  { 3, 3 },  { 1, 1 },  { 2, 3 }, { 1, 0 },  { 4, 3 },  { 2, 2 },  { 3, 0 },  { 1, 2 }, 
	{ 2, 4 },  { 0, 1 },  { 4, 2 },  { 0, 3 },  { 2, 0 }, { 3, 2 },  { 1, 4 },  { 4, 1 },  { 0, 2 },  { 3, 1 }, { 1, 3 }
	// clang-format on
};

static double tickToSec(int tickCount)
{
	return tickCount / (double)gnTicksRate;
}

static void CalcHealHp(const D1Hero *hero, int sn, int spllvl, int *mind, int *maxd)
{
	int i;
	int minhp = 1;
	int maxhp = 10;
	for (i = spllvl; i > 0; i--) {
		minhp += 1;
		maxhp += 6;
	}
	for (i = hero->getLevel(); i > 0; i--) {
		minhp += 1;
		maxhp += 4;
	}
	if (sn == SPL_HEAL) {
		switch (hero->getClass()) {
		case PC_WARRIOR: minhp <<= 1; maxhp <<= 1;               break;
#ifdef HELLFIRE
		case PC_BARBARIAN:
		case PC_MONK:    minhp <<= 1; maxhp <<= 1;               break;
		case PC_BARD:
#endif
		case PC_ROGUE: minhp += minhp >> 1; maxhp += maxhp >> 1; break;
		case PC_SORCERER: break;
		default:
			ASSUME_UNREACHABLE
		}
	} else {
		switch (hero->getClass()) {
		case PC_WARRIOR:   minhp <<= 1; maxhp <<= 1;             break;
#ifdef HELLFIRE
		case PC_MONK:      minhp *= 3;  maxhp *= 3;              break;
		case PC_BARBARIAN: minhp <<= 1; maxhp <<= 1;             break;
		case PC_BARD:
#endif
		case PC_ROGUE: minhp += minhp >> 1; maxhp += maxhp >> 1; break;
		case PC_SORCERER: break;
		default:
			ASSUME_UNREACHABLE
		}
	}

	*mind = minhp;
	*maxd = maxhp;
}

void GetMissileDamage(int mtype, const MonsterStruct *mon, int *mindam, int *maxdam)
{
	int mind = mon->_mMinDamage;
	int maxd = mon->_mMaxDamage;
	// if (mtype == MIS_BLOODBOILC) {
	if (mtype == MIS_BLOODBOIL) {
		mind = mon->_mLevel >> 1;
		maxd = mon->_mLevel;
	} else if (mtype == MIS_FLASH) {
		mind = maxd = mon->_mLevel << 1;
	// } else if (mtype == MIS_LIGHTNINGC || mtype == MIS_LIGHTNINGC2) {
	} else if (mtype == MIS_LIGHTNING) {
		// mind = mon->_mMinDamage;
		maxd = maxd << 1;
	//} else if (mtype == MIS_CBOLTC) {
	//} else if (mtype == MIS_CBOLT) {
	//	mind = maxd; //  15 << gnDifficulty; // FIXME
	} else if (mtype == MIS_APOCAC2) {
		mind = maxd = 40 << gnDifficulty;
	}
	*mindam = mind;
	*maxdam = maxd;
}

static void SkillPlrDamage(int sn, int sl, int dist, int mypnum, const MonsterStruct *mon, int pnum, int *mindam, int *maxdam)
{
	int k, magic, mind, maxd;

	// assert((unsigned)sn < NUM_SPELLS);
	magic = myplr._pMagic;
#ifdef HELLFIRE
	if (SPELL_RUNE(sn))
		sl += myplr._pDexterity >> 3;
#endif
	switch (sn) {
	case SPL_GUARDIAN:
	case SPL_FIREBOLT:
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
	case SPL_INFRA:
	case SPL_TELEKINESIS:
	case SPL_TELEPORT:
	case SPL_RNDTELEPORT:
	case SPL_TOWN:
	case SPL_RESURRECT:
	case SPL_IDENTIFY:
	case SPL_REPAIR:
	case SPL_RECHARGE:
	case SPL_DISARM:
	case SPL_RAGE:
	case SPL_STONE:
	case SPL_SWIPE:
	case SPL_WALLOP:
	case SPL_WHIPLASH:
#ifdef HELLFIRE
	case SPL_BUCKLE:
	case SPL_WHITTLE:
	case SPL_RUNESTONE:
#endif
	case SPL_HEAL:
	case SPL_HEALOTHER:
	case SPL_MANASHIELD:
	case SPL_ATTRACT:
	case SPL_SHROUD:
	case SPL_SWAMP:
		QMessageBox::critical(nullptr, "Error", QApplication::tr("Unhandled missile skill %1 in SkillPlrDamage.").arg(sn));
		break;
	case SPL_CHARGE:
		dist *= GetChargeSpeed(mypnum);
		dist -= 24;
		if (dist > 32)
			dist = 32;
		mind = myplr._pIChMinDam;
		maxd = myplr._pIChMaxDam;
		mind = ((64 + dist) * mind) >> 5;
		maxd = ((64 + dist) * maxd) >> 5;
		if (maxd <= 0) {
			mind = 0;
			maxd = 0;
		}
		// hper = sl * 16 - mon->_mArmorClass;
		break;
	case SPL_RATTACK:
	case SPL_POINT_BLANK:
	case SPL_FAR_SHOT:
	case SPL_PIERCE_SHOT:
	case SPL_MULTI_SHOT: {
		bool tmac = (myplr._pIFlags & ISPL_PENETRATE_PHYS) != 0;
		mind = 0;
		maxd = 0;
		if (mon != nullptr) {
			int sldam = myplr._pISlMaxDam;
			if (sldam != 0) {
				maxd += CalcMonsterDam(mon->_mMagicRes, MISR_SLASH, sldam, tmac);
				mind += CalcMonsterDam(mon->_mMagicRes, MISR_SLASH, myplr._pISlMinDam, tmac);
			}
			int bldam = myplr._pIBlMaxDam;
			if (bldam != 0) {
				maxd += CalcMonsterDam(mon->_mMagicRes, MISR_BLUNT, bldam, tmac);
				mind += CalcMonsterDam(mon->_mMagicRes, MISR_BLUNT, myplr._pIBlMinDam, tmac);
			}
			int pcdam = myplr._pIPcMaxDam;
			if (pcdam != 0) {
				maxd += CalcMonsterDam(mon->_mMagicRes, MISR_PUNCTURE, pcdam, tmac);
				mind += CalcMonsterDam(mon->_mMagicRes, MISR_PUNCTURE, myplr._pIPcMinDam, tmac);
			}

			// if (random_(6, 200) < myplr._pICritChance) {
			//	dam <<= 1;
			// }

			switch (sn) {
			case SPL_RATTACK:
				break;
			case SPL_POINT_BLANK:
				mind = (mind * (64 + 32 - 16 * dist + sl)) >> 6;
				maxd = (mind * (64 + 32 - 16 * dist + sl)) >> 6;
				break;
			case SPL_FAR_SHOT:
				mind = (mind * (8 * dist - 16 + sl)) >> 5;
				maxd = (maxd * (8 * dist - 16 + sl)) >> 5;
				break;
			case SPL_PIERCE_SHOT:
				mind = (mind * (32 + sl)) >> 6;
				maxd = (maxd * (32 + sl)) >> 6;
				break;
			case SPL_MULTI_SHOT:
				mind = (mind * (16 + sl)) >> 6;
				maxd = (maxd * (16 + sl)) >> 6;
				break;
			}

			int fdam = myplr._pIFMaxDam;
			if (fdam != 0) {
				maxd += CalcMonsterDam(mon->_mMagicRes, MISR_FIRE, fdam, false);
				mind += CalcMonsterDam(mon->_mMagicRes, MISR_FIRE, myplr._pIFMinDam, false); // myplr._pIFMinDam
			}
			int ldam = myplr._pILMaxDam;
			if (ldam != 0) {
				maxd += CalcMonsterDam(mon->_mMagicRes, MISR_LIGHTNING, ldam, false);
				mind += CalcMonsterDam(mon->_mMagicRes, MISR_LIGHTNING, myplr._pILMinDam, false); // myplr._pILMinDam
			}
			int mdam = myplr._pIMMaxDam;
			if (mdam != 0) {
				maxd += CalcMonsterDam(mon->_mMagicRes, MISR_MAGIC, mdam, false);
				mind += CalcMonsterDam(mon->_mMagicRes, MISR_MAGIC, myplr._pIMMinDam, false); // myplr._pIMMinDam
			}
			int adam = myplr._pIAMaxDam;
			if (adam != 0) {
				maxd += CalcMonsterDam(mon->_mMagicRes, MISR_ACID, adam, false);
				mind += CalcMonsterDam(mon->_mMagicRes, MISR_ACID, myplr._pIAMinDam, false); // 
			}
		} else {
			int sldam = myplr._pISlMaxDam;
			if (sldam != 0) {
				maxd += CalcPlrDam(pnum, MISR_SLASH, sldam);
				mind += CalcPlrDam(pnum, MISR_SLASH, myplr._pISlMinDam);
			}
			int bldam = myplr._pIBlMaxDam;
			if (bldam != 0) {
				maxd += CalcPlrDam(pnum, MISR_BLUNT, bldam);
				mind += CalcPlrDam(pnum, MISR_BLUNT, myplr._pIBlMinDam);
			}
			int pcdam = myplr._pIPcMaxDam;
			if (pcdam != 0) {
				maxd += CalcPlrDam(pnum, MISR_PUNCTURE, pcdam);
				mind += CalcPlrDam(pnum, MISR_PUNCTURE, myplr._pIPcMinDam);
			}
			// if (random_(6, 200) < myplr._pICritChance) {
			//	dam <<= 1;
			// }
			// add modifiers from arrow-type
			switch (sn) {
			case SPL_RATTACK:
				break;
			case SPL_POINT_BLANK:
				mind = (mind * (64 + 32 - 16 * dist + sl)) >> 6; // MISDIST
				maxd = (maxd * (64 + 32 - 16 * dist + sl)) >> 6; // MISDIST
				break;
			case SPL_FAR_SHOT:
				mind = (mind * (8 * dist - 16 + sl)) >> 5; // MISDIST
				maxd = (maxd * (8 * dist - 16 + sl)) >> 5; // MISDIST
				break;
			case SPL_PIERCE_SHOT:
				mind = (mind * (32 + sl)) >> 6;
				maxd = (maxd * (32 + sl)) >> 6;
				break;
			case SPL_MULTI_SHOT:
				mind = (mind * (16 + sl)) >> 6;
				maxd = (maxd * (16 + sl)) >> 6;
				break;
			}

			int fdam = myplr._pIFMaxDam;
			if (fdam != 0) {
				maxd += CalcPlrDam(pnum, MISR_FIRE, fdam);
				mind += CalcPlrDam(pnum, MISR_FIRE, myplr._pIFMinDam);
			}
			int ldam = myplr._pILMaxDam;
			if (ldam != 0) {
				maxd += CalcPlrDam(pnum, MISR_LIGHTNING, ldam);
				mind += CalcPlrDam(pnum, MISR_LIGHTNING, myplr._pILMinDam);
			}
			int mdam = myplr._pIMMaxDam;
			if (mdam != 0) {
				maxd += CalcPlrDam(pnum, MISR_MAGIC, mdam);
				mind += CalcPlrDam(pnum, MISR_MAGIC, myplr._pIMMinDam);
			}
			int adam = myplr._pIAMaxDam;
			if (adam != 0) {
				maxd += CalcPlrDam(pnum, MISR_ACID, adam);
				mind += CalcPlrDam(pnum, MISR_ACID, myplr._pIAMinDam);
			}
		}

		mind >>= 6;
		maxd >>= 6;

		*mindam = mind;
		*maxdam = maxd;
	} return;
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
		sl = sl > 0 ? sl - 1 : 0;
		k = monsterdata[MT_GOLEM].mLevel;
		sl = k + sl;
		// mon->_mLevel = sl;
		mind = sl * monsterdata[MT_GOLEM].mMinDamage / k;
		maxd = sl * monsterdata[MT_GOLEM].mMaxDamage / k;
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

	int mtype = spelldata[sn].sMissile;
	// mtype = GetBaseMissile(mtype);
	BYTE mRes = GetMissileElement(mtype);

	if (mon != nullptr) {
		mind = CalcMonsterDam(mon->_mMagicRes, mRes, mind, false);
		maxd = CalcMonsterDam(mon->_mMagicRes, mRes, maxd, false);
	} else {
		mind = CalcPlrDam(pnum, mRes, mind);
		maxd = CalcPlrDam(pnum, mRes, maxd);
	}

	*mindam = mind;
	*maxdam = maxd;
}

void SkillMonByPlrDamage(int sn, int sl, int dist, int source, const MonsterStruct *mon, int *mindam, int *maxdam)
{
	SkillPlrDamage(sn, sl, dist, source, mon, MAX_PLRS, mindam, maxdam);
}

void SkillPlrByPlrDamage(int sn, int sl, int dist, int source, int target, int *mindam, int *maxdam)
{
	SkillPlrDamage(sn, sl, dist, source, nullptr, target, mindam, maxdam);
}

void GetSkillDesc(const D1Hero *hero, int sn, int sl)
{
	int k, magic, mind, maxd = 0, dur = 0;

	// assert((unsigned)sn < NUM_SPELLS);
	magic = hero->getMagic(); //  myplr._pMagic;
#ifdef HELLFIRE
	if (SPELL_RUNE(sn))
		sl += hero->getDexterity() /*myplr._pDexterity*/ >> 3;
#endif
	switch (sn) {
	case SPL_GUARDIAN:
		dur = (sl + (hero->getLevel() >> 1)) * misfiledata[MFILE_GUARD].mfAnimLen[1];
	case SPL_FIREBOLT:
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
	case SPL_RATTACK:
	case SPL_INFRA:
	case SPL_TELEKINESIS:
	case SPL_TELEPORT:
	case SPL_RNDTELEPORT:
	case SPL_TOWN:
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
		break;
	case SPL_HEAL:
	case SPL_HEALOTHER:
		CalcHealHp(hero, sn, sl, &mind, &maxd);
		snprintf(infostr, sizeof(infostr), "Heal: %d-%d", mind, maxd);
		return;
	case SPL_CHARGE:
		mind = hero->getChMinDam(); // myplr._pIChMinDam
		maxd = hero->getChMaxDam(); // myplr._pIChMaxDam
		// mind = ((64 /*+ dist*/) * mind) >> 5;
		// maxd = ((64 /*+ dist*/) * maxd) >> 5;
		// hper = sl * 16 - mon->_mArmorClass;
		break;
	case SPL_SWIPE:
		k = (100 * (48 + sl)) >> 6;
		snprintf(infostr, sizeof(infostr), "Dam Mpl.: %d%%", k);
		return;
	case SPL_WALLOP:
		k = (100 * (112 + sl)) >> 6;
		snprintf(infostr, sizeof(infostr), "Dam Mpl.: %d%%", k);
		return;
	case SPL_WHIPLASH:
		k = (100 * (24 + sl)) >> 6;
		snprintf(infostr, sizeof(infostr), "Dam Mpl.: %d%%", k);
		return;
	case SPL_POINT_BLANK:
		k = (100 * (64 + /*32 - 16 * mis->_miVar7 +*/ sl)) >> 6;
		snprintf(infostr, sizeof(infostr), "Dam Mpl. <= %d%%", k);
		return;
	case SPL_FAR_SHOT:
		k = (100 * (/*8 * mis->_miVar7 - 16 +*/ sl)) >> 5;
		snprintf(infostr, sizeof(infostr), "Dam Mpl. >= %d%%", k);
		return;
	case SPL_PIERCE_SHOT:
		k = (100 * (32 + sl)) >> 6;
		snprintf(infostr, sizeof(infostr), "Dam Mpl.: %d%%", k);
		return;
	case SPL_MULTI_SHOT:
		k = (100 * (16 + sl)) >> 6;
		snprintf(infostr, sizeof(infostr), "Dam Mpl.: %d%%", k);
		return;
	case SPL_MANASHIELD:
		k = (std::min(sl + 1, 16) * 100) >> 6;
		snprintf(infostr, sizeof(infostr), "Dam Red.: %d%%", k);
		return;
	case SPL_ATTRACT:
		k = 4 + (sl >> 2);
		if (k > 9)
			k = 9;
		snprintf(infostr, sizeof(infostr), "Range: %d", k);
		return;
	case SPL_RAGE:
		dur = 32 * sl + 245;
		break;
	case SPL_SHROUD:
		dur = 32 * sl + 160;
		break;
	case SPL_STONE:
		dur = (sl + 1) << (7 + 6);
		dur >>= 5;
		if (dur < 15)
			dur = 0;
		if (dur > 239)
			dur = 239;
		snprintf(infostr, sizeof(infostr), "Dur <= %.1fs", tickToSec(dur));
		return;
#ifdef HELLFIRE
	case SPL_FIRERING:
#endif
	case SPL_FIREWALL:
		mind = ((magic >> 3) + sl + 5) << (-3 + 5);
		maxd = ((magic >> 3) + sl * 2 + 10) << (-3 + 5);
		dur = 64 * sl + 160;
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
	case SPL_SWAMP:
		dur = (lengthof(BloodBoilLocs) + sl * 2) * misfiledata[MFILE_BLODBURS].mfAnimFrameLen[0] * misfiledata[MFILE_BLODBURS].mfAnimLen[0] / 2;
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
		sl = sl > 0 ? sl - 1 : 0;
		k = monsterdata[MT_GOLEM].mLevel;
		sl = k + sl;
		// mon->_mLevel = sl;
		dur = sl * monsterdata[MT_GOLEM].mMinHP / k;
		mind = sl * monsterdata[MT_GOLEM].mMinDamage / k;
		maxd = sl * monsterdata[MT_GOLEM].mMaxDamage / k;
		snprintf(infostr, sizeof(infostr), "Lvl: %d Hp: %d Dam: %d-%d", sl, dur, mind, maxd);
		return;
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

	infostr[0] = '\0';
	if (dur != 0) {
		if (maxd != 0)
			snprintf(infostr, sizeof(infostr), "Dam: %d-%d Dur: %.1fs", mind, maxd, tickToSec(dur));
		else
			snprintf(infostr, sizeof(infostr), "Dur: %.1fs", tickToSec(dur));
	} else if (maxd != 0) {
		snprintf(infostr, sizeof(infostr), "Dam: %d-%d", mind, maxd);
	}
}

int MissPlrHitByMonChance(int mtype, int dist, const MonsterStruct *mon, const D1Hero *hero)
{
	int hper;
	if (missiledata[mtype].mdFlags & MIF_ARROW) {
		hper = 30 + mon->_mHit + (2 * mon->_mLevel) - hero->getAC();
		hper -= abs(dist - 6) << 1; // MISDIST
	} else if (missiledata[mtype].mdFlags & MIF_AREA) {
		hper = 40 + 2 * mon->_mLevel;
		hper -= 2 * hero->getLevel();
	} else {
		hper = 50 + mon->_mMagic;
		hper -= hero->getEvasion();
	}
	return hper;
}

int MissMonHitByPlrChance(int mtype, int dist, const D1Hero *hero, const MonsterStruct *mon)
{
	int hper;
	if (missiledata[mtype].mdFlags & MIF_ARROW) {
		hper = hero->getHitChance() - mon->_mArmorClass;
		// hper -= ((dist - 4) * (dist - 4) >> 1); // MISDIST
		hper -= abs(dist - 6) << 1;
	} else if (missiledata[mtype].mdFlags & MIF_AREA) {
		hper = 40 + 2 * hero->getLevel();
		hper -= 2 * mon->_mLevel;
	} else {
		hper = 50 + hero->getMagic();
		hper -= 2 * mon->_mLevel + mon->_mEvasion;
		// hper -= dist; // TODO: either don't care about it, or set it!
	}
	return hper;
}

int MissPlrHitByPlrChance(int mtype, int dist, const D1Hero *offHero, const D1Hero *defHero)
{
	int hper;
	if (missiledata[mtype].mdFlags & MIF_ARROW) {
		hper = offHero->getHitChance();
		hper -= defHero->getAC();
		// hper -= (dist * dist >> 1); // MISDIST
		hper -= abs(dist - 6) << 1;
	} else if (missiledata[mtype].mdFlags & MIF_AREA) {
		hper = 40 + 2 * offHero->getLevel();
		hper -= 2 * defHero->getLevel();
	} else {
		hper = 50 + offHero->getMagic();
		hper -= defHero->getEvasion();
		// hper -= dist; // TODO: either don't care about it, or set it!
	}
	return hper;
}

unsigned CalcMonsterDam(unsigned mor, BYTE mRes, unsigned damage, bool penetrates)
{
	unsigned dam;
	BYTE resist;

	switch (mRes) {
	case MISR_NONE:
		resist = MORT_NONE;
		break;
	case MISR_SLASH:
		mor &= MORS_SLASH_IMMUNE;
		resist = mor >> MORS_IDX_SLASH;
		break;
	case MISR_BLUNT:
		mor &= MORS_BLUNT_IMMUNE;
		resist = mor >> MORS_IDX_BLUNT;
		break;
	case MISR_PUNCTURE:
		mor &= MORS_PUNCTURE_IMMUNE;
		resist = mor >> MORS_IDX_PUNCTURE;
		break;
	case MISR_FIRE:
		mor &= MORS_FIRE_IMMUNE;
		resist = mor >> MORS_IDX_FIRE;
		break;
	case MISR_LIGHTNING:
		mor &= MORS_LIGHTNING_IMMUNE;
		resist = mor >> MORS_IDX_LIGHTNING;
		break;
	case MISR_MAGIC:
		mor &= MORS_MAGIC_IMMUNE;
		resist = mor >> MORS_IDX_MAGIC;
		break;
	case MISR_ACID:
		mor &= MORS_ACID_IMMUNE;
		resist = mor >> MORS_IDX_ACID;
		break;
	default:
		ASSUME_UNREACHABLE
		resist = MORT_NONE;
		break;
	}
	dam = damage;
	switch (resist) {
	case MORT_NONE:
		break;
	case MORT_PROTECTED:
		if (!penetrates) {
			dam >>= 1;
			dam += dam >> 2;
		}
		break;
	case MORT_RESIST:
		dam >>= penetrates ? 1 : 2;
		break;
	case MORT_IMMUNE:
		dam = 0;
		break;
	default: ASSUME_UNREACHABLE;
	}
	return dam;
}

unsigned CalcPlrDam(int pnum, BYTE mRes, unsigned damage)
{
	int dam;
	int8_t resist;

	switch (mRes) {
	case MISR_NONE:
	case MISR_SLASH: // TODO: add plr._pSlash/Blunt/PunctureResist
	case MISR_BLUNT:
	case MISR_PUNCTURE:
		resist = 0;
		break;
	case MISR_FIRE:
		resist = plr._pFireResist;
		break;
	case MISR_LIGHTNING:
		resist = plr._pLghtResist;
		break;
	case MISR_MAGIC:
		resist = plr._pMagResist;
		break;
	case MISR_ACID:
		resist = plr._pAcidResist;
		break;
	default:
		ASSUME_UNREACHABLE
		break;
	}

	dam = damage;
	if (resist != 0)
		dam -= dam * resist / 100;
	return dam;
}

int GetBaseMissile(int mtype)
{
	switch (mtype) {
	case MIS_ARROW:
	case MIS_PBARROW:
	case MIS_ASARROW:
	case MIS_MLARROW:
    case MIS_PCARROW:
    case MIS_FIREBOLT:
    case MIS_FIREBALL:
    case MIS_HBOLT:
    case MIS_FLARE:
    case MIS_SNOWWICH:
    case MIS_HLSPWN:
    case MIS_SOLBRNR:
    case MIS_MAGE:
    case MIS_MAGMABALL:
    case MIS_ACID:
    case MIS_ACIDPUD:
    case MIS_EXACIDP:
    case MIS_EXFIRE:
    case MIS_EXFBALL:
    case MIS_EXLGHT:
    case MIS_EXMAGIC:
    case MIS_EXACID:
    case MIS_EXHOLY:
    case MIS_EXFLARE:
    case MIS_EXSNOWWICH:
    case MIS_EXHLSPWN:
    case MIS_EXSOLBRNR:
    case MIS_EXMAGE:
    case MIS_POISON:
    case MIS_WIND:
    case MIS_LIGHTBALL: break;
    case MIS_LIGHTNINGC: mtype = MIS_LIGHTNING; break;
    case MIS_LIGHTNING: break;
    case MIS_LIGHTNINGC2: mtype = MIS_LIGHTNING; break;
    case MIS_LIGHTNING2: break;
    case MIS_BLOODBOILC: mtype = MIS_BLOODBOIL; break;
    case MIS_BLOODBOIL: break;
    case MIS_SWAMPC: mtype = MIS_SWAMP; break;
    case MIS_SWAMP:
    case MIS_TOWN:
    case MIS_RPORTAL:
    case MIS_FLASH:
    case MIS_FLASH2:
    case MIS_CHAIN:
    //case MIS_BLODSTAR:	// TODO: Check beta
    //case MIS_BONE:		// TODO: Check beta
    //case MIS_METLHIT:	// TODO: Check beta
    case MIS_RHINO:
    case MIS_CHARGE:
    case MIS_TELEPORT:
    case MIS_RNDTELEPORT:
    //case MIS_FARROW:
    //case MIS_DOOMSERP:
    case MIS_STONE:
    case MIS_SHROUD: break;
    //case MIS_INVISIBL:
    case MIS_GUARDIAN: mtype = MIS_FIREBOLT; break;
    case MIS_GOLEM:
    //case MIS_ETHEREALIZE:
    case MIS_BLEED: break;
    //case MIS_EXAPOCA:
    case MIS_FIREWALLC: mtype = MIS_FIREWALL; break;
    case MIS_FIREWALL: break;
    case MIS_FIREWAVEC: mtype = MIS_FIREWAVE; break;
    case MIS_FIREWAVE:
    case MIS_METEOR: break;
    case MIS_LIGHTNOVAC: mtype = MIS_LIGHTBALL; break;
    //case MIS_APOCAC:
    case MIS_HEAL:
    case MIS_HEALOTHER:
    case MIS_RESURRECT:
    case MIS_ATTRACT:
    case MIS_TELEKINESIS:
    //case MIS_LARROW:
    case MIS_OPITEM:
    case MIS_REPAIR:
    case MIS_DISARM: break;
    case MIS_INFERNOC: mtype = MIS_FIREWALL; break;
    case MIS_INFERNO:
    //case MIS_FIRETRAP:
    case MIS_BARRELEX: break;
    //case MIS_FIREMAN:	// TODO: Check beta
    //case MIS_KRULL:		// TODO: Check beta
    case MIS_CBOLTC: mtype = MIS_CBOLT; break;
    case MIS_CBOLT:
    case MIS_ELEMENTAL:
    //case MIS_BONESPIRIT:
    case MIS_APOCAC2:
    case MIS_EXAPOCA2:
    case MIS_MANASHIELD:
    case MIS_INFRA:
    case MIS_RAGE: break;
#ifdef HELLFIRE
    //case MIS_LIGHTWALLC:
    //case MIS_LIGHTWALL:
    //case MIS_FIRENOVAC:
    //case MIS_FIREBALL2:
    //case MIS_REFLECT:
    case MIS_FIRERING:  mtype = MIS_FIREWALL;  break;
    //case MIS_MANATRAP:
    //case MIS_LIGHTRING:
    case MIS_RUNEFIRE:  mtype = MIS_FIREEXP;   break;
    case MIS_RUNELIGHT: mtype = MIS_LIGHTNING; break;
    case MIS_RUNENOVA:  mtype = MIS_LIGHTBALL; break;
    case MIS_RUNEWAVE:  mtype = MIS_FIREWAVE;  break;
    case MIS_RUNESTONE: mtype = MIS_STONE;     break;
    case MIS_FIREEXP:
    case MIS_HORKDMN:
    case MIS_PSYCHORB:
    case MIS_LICH:
    case MIS_BONEDEMON:
    case MIS_ARCHLICH:
    case MIS_NECROMORB:
    case MIS_EXPSYCHORB:
    case MIS_EXLICH:
    case MIS_EXBONEDEMON:
    case MIS_EXARCHLICH:
    case MIS_EXNECROMORB: break;
#endif
	}
	return mtype;
}

BYTE GetMissileElement(int mtype)
{
	mtype = GetBaseMissile(mtype);
	return missiledata[mtype].mResist;
}

const char *GetElementColor(BYTE mRes)
{
	const char *color = "";
	switch (mRes) {
	case MISR_FIRE:      color = "color:red;";     break;
	case MISR_MAGIC:     color = "color:blue;";    break;
	case MISR_LIGHTNING: color = "color:#FBB117;"; break; // amber
	case MISR_ACID:      color = "color:green;";   break;
	case MISR_PUNCTURE:  color = "color:olive;";   break;
	case MISR_BLUNT:     color = "color:maroon;";  break;
	}
	return color;
}

int GetArrowVelocity(int misource)
{
    int av = MIS_SHIFTEDVEL(32);
    av += MIS_SHIFTEDVEL((int)plx(misource)._pIArrowVelBonus);

    return av;
}

#ifdef HELLFIRE
int AddFireRune(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddLightRune(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddNovaRune(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddWaveRune(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddStoneRune(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddHorkSpawn(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
//int AddLightwall(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddFireexp(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddRingC(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
//int AddFireball2(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
#endif
int AddArrow(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddFirebolt(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddMage(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddMagmaball(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddLightball(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddPoison(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddWind(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddAcid(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddAcidpud(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
//int AddKrull(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddTeleport(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddRndTeleport(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddFirewall(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
//int AddFireball(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddLightningC(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddLightning(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddBloodBoilC(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddBloodBoil(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddBleed(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddMisexp(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddFlash(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddFlash2(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddFireWave(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddMeteor(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddChain(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddRhino(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddCharge(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
//int AddFireman(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
//int AddFlare(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddStone(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddShroud(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddGuardian(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddGolem(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddHeal(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddHealOther(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddElemental(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddWallC(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddFireWaveC(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddNovaC(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddOpItem(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddDisarm(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddInferno(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddInfernoC(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
//int AddFireTrap(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddBarrelExp(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddCboltC(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddCbolt(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddResurrect(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddAttract(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddTelekinesis(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddTown(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddPortal(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddApocaC2(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddManashield(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddInfra(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
int AddRage(int mi, int sx, int sy, int dx, int dy, int midir, int micaster, int misource, int spllvl) { return 0; }
void MI_Dummy(int mi) { }
void MI_Arrow(int mi) { }
void MI_AsArrow(int mi) { }
void MI_Firebolt(int mi) { }
void MI_Lightball(int mi) { }
void MI_Poison(int mi) { }
void MI_Mage(int mi) { }
void MI_Wind(int mi) { }
//void MI_Krull(int mi) { }
void MI_Acid(int mi) { }
void MI_Acidpud(int mi) { }
void MI_Firewall(int mi) { }
//void MI_Fireball(int mi) { }
#ifdef HELLFIRE
void MI_HorkSpawn(int mi) { }
void MI_Rune(int mi) { }
//void MI_Lightwall(int mi) { }
#endif
void MI_LightningC(int mi) { }
void MI_Lightning(int mi) { }
void MI_BloodBoilC(int mi) { }
void MI_BloodBoil(int mi) { }
void MI_SwampC(int mi) { }
void MI_Bleed(int mi) { }
void MI_Portal(int mi) { }
void MI_Flash(int mi) { }
void MI_Flash2(int mi) { }
void MI_FireWave(int mi) { }
void MI_Meteor(int mi) { }
void MI_Guardian(int mi) { }
void MI_Chain(int mi) { }
void MI_Misexp(int mi) { }
void MI_MiniExp(int mi) { }
void MI_LongExp(int mi) { }
void MI_Acidsplat(int mi) { }
void MI_Stone(int mi) { }
void MI_Shroud(int mi) { }
void MI_Rhino(int mi) { }
void MI_Charge(int mi) { }
//void MI_Fireman(int mi) { }
void MI_WallC(int mi) { }
void MI_ApocaC(int mi) { }
void MI_Inferno(int mi) { }
void MI_InfernoC(int mi) { }
//void MI_FireTrap(int mi) { }
void MI_Cbolt(int mi) { }
void MI_Elemental(int mi) { }
void MI_Resurrect(int mi) { }

DEVILUTION_END_NAMESPACE
