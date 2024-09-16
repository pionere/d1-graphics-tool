/**
 * @file missiles.cpp
 *
 * Implementation of missile functionality.
 */
#include "all.h"
#include "misproc.h"

#include "../d1hro.h"

DEVILUTION_BEGIN_NAMESPACE

static const BYTE BloodBoilLocs[][2] = {
	// clang-format off
	{ 3, 4 },  { 2, 1 },  { 3, 3 },  { 1, 1 },  { 2, 3 }, { 1, 0 },  { 4, 3 },  { 2, 2 },  { 3, 0 },  { 1, 2 }, 
	{ 2, 4 },  { 0, 1 },  { 4, 2 },  { 0, 3 },  { 2, 0 }, { 3, 2 },  { 1, 4 },  { 4, 1 },  { 0, 2 },  { 3, 1 }, { 1, 3 }
	// clang-format on
};

static int gnTicksRate = SPEED_NORMAL;

static double tickToSec(int tickCount)
{
	return tickCount / (double)gnTicksRate;
}

static void CalcHealHp(D1Hero *hero, int sn, int spllvl, int *mind, int *maxd)
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

void GetSkillDesc(D1Hero *hero, int sn, int sl)
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
        k = (100 * (64 + 32 /*- 16 * mis->_miVar7*/ + sl)) >> 6;
        snprintf(infostr, sizeof(infostr), "Dam Mpl. <= %d%%", k);
        return;
	case SPL_FAR_SHOT:
        k = (100 * (/*8 * mis->_miVar7*/ - 16 + sl)) >> 5;
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