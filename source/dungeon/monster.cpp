/**
 * @file monster.cpp
 *
 * Implementation of monster functionality, AI, actions, spawning, loading, etc.
 */
#include "all.h"

#include <QApplication>

#include "../progressdialog.h"

/* Limit the number of (scattered) monster-types on the current level by the required resources (In CRYPT the values are not valid). */
static int monstimgtot;
/* Number of active monsters on the current level (minions are considered active). */
int nummonsters;
/* The data of the monsters on the current level. */
MonsterStruct monsters[MAXMONSTERS];
/* Monster types on the current level. */
MapMonData mapMonTypes[MAX_LVLMTYPES];
/* The number of monster types on the current level. */
int nummtypes;

static_assert(MAX_LVLMTYPES <= UCHAR_MAX, "Monster-type indices are stored in a BYTE fields.");
/* The number of skeleton-monster types on the current level. */
BYTE numSkelTypes;
/* The number of goat-monster types on the current level. */
BYTE numGoatTypes;
/* Skeleton-monster types on the current level. */
BYTE mapSkelTypes[MAX_LVLMTYPES];
/* Goat-monster types on the current level. */
BYTE mapGoatTypes[MAX_LVLMTYPES];

/* The next light-index to be used for the trn of a unique monster. */
BYTE uniquetrans;

/** 'leader' of monsters without leaders. */
static_assert(MAXMONSTERS <= UCHAR_MAX, "Leader of monsters are stored in a BYTE field.");
#define MON_NO_LEADER MAXMONSTERS

/** Light radius of unique monsters */
#define MON_LIGHTRAD 7

/** Maximum distance of the pack-monster from its leader. */
#define MON_PACK_DISTANCE 3

/** Number of the monsters in packs. */
#define MON_PACK_SIZE 9

/** Minimum tick delay between steps if the walk is not continuous. */
#define MON_WALK_DELAY 20

/** Maps from walking path step to facing direction. */
//const int8_t walk2dir[9] = { 0, DIR_NE, DIR_NW, DIR_SE, DIR_SW, DIR_N, DIR_E, DIR_S, DIR_W };

/** Maps from monster action to monster animation letter. */
static const char animletter[NUM_MON_ANIM] = { 'n', 'w', 'a', 'h', 'd', 's' };
/** Maps from direction to delta X-offset. */
const int offset_x[NUM_DIRS] = { 1, 0, -1, -1, -1, 0, 1, 1 };
/** Maps from direction to delta Y-offset. */
const int offset_y[NUM_DIRS] = { 1, 1, 1, 0, -1, -1, -1, 0 };

static void InitMonsterGFX(int midx)
{
	MapMonData* cmon;
	const MonFileData* mfdata;
	int mtype, anim; // , i;
//	char strBuff[DATA_ARCHIVE_MAX_PATH];
//	BYTE* celBuf;

	cmon = &mapMonTypes[midx];
	mfdata = &monfiledata[cmon->cmFileNum];
//	cmon->cmWidth = mfdata->moWidth * ASSET_MPL;
//	cmon->cmXOffset = (cmon->cmWidth - TILE_WIDTH) >> 1;
//	cmon->cmAFNum = mfdata->moAFNum;
//	cmon->cmAFNum2 = mfdata->moAFNum2;

	mtype = cmon->cmType;
	auto& monAnims = cmon->cmAnims;
	// static_assert(lengthof(animletter) == lengthof(monsterdata[0].maFrames), "");
	for (anim = 0; anim < NUM_MON_ANIM; anim++) {
		monAnims[anim].maFrames = mfdata->moAnimFrames[anim];
		monAnims[anim].maFrameLen = mfdata->moAnimFrameLen[anim];
		/*if (mfdata->moAnimFrames[anim] > 0) {
			snprintf(strBuff, sizeof(strBuff), mfdata->moGfxFile, animletter[anim]);

			celBuf = LoadFileInMem(strBuff);
			assert(cmon->cmAnimData[anim] == NULL);
			cmon->cmAnimData[anim] = celBuf;

			if (mtype != MT_GOLEM || (anim != MA_SPECIAL && anim != MA_DEATH)) {
				for (i = 0; i < lengthof(monAnims[anim].maAnimData); i++) {
					monAnims[anim].maAnimData[i] = const_cast<BYTE*>(CelGetFrameStart(celBuf, i));
				}
			} else {
				for (i = 0; i < lengthof(monAnims[anim].maAnimData); i++) {
					monAnims[anim].maAnimData[i] = celBuf;
				}
			}
		}*/
	}

//	if (monsterdata[mtype].mTransFile != NULL) {
//		InitMonsterTRN(monAnims, monsterdata[mtype].mTransFile);
//	}
}

static void InitMonsterStats(int midx)
{
	MapMonData* cmon;
	const MonsterData* mdata;
	unsigned baseLvl, lvlBonus, monLvl;

	cmon = &mapMonTypes[midx];

	mdata = &monsterdata[cmon->cmType];

	cmon->cmName = mdata->mName;
	cmon->cmFileNum = mdata->moFileNum;
	cmon->cmLevel = mdata->mLevel;
	cmon->cmSelFlag = mdata->mSelFlag;
	cmon->cmAI = mdata->mAI;
	cmon->cmFlags = mdata->mFlags;
	cmon->cmHit = mdata->mHit;
	cmon->cmMinDamage = mdata->mMinDamage;
	cmon->cmMaxDamage = mdata->mMaxDamage;
	cmon->cmHit2 = mdata->mHit2;
	cmon->cmMinDamage2 = mdata->mMinDamage2;
	cmon->cmMaxDamage2 = mdata->mMaxDamage2;
	cmon->cmMagic = mdata->mMagic;
	cmon->cmArmorClass = mdata->mArmorClass;
	cmon->cmEvasion = mdata->mEvasion;
	cmon->cmMagicRes = mdata->mMagicRes;
	cmon->cmExp = mdata->mExp;
	cmon->cmMinHP = mdata->mMinHP;
	cmon->cmMaxHP = mdata->mMaxHP;

	lvlBonus = currLvl._dLevelBonus;
	cmon->cmAI.aiInt += lvlBonus / 16;

	cmon->cmHit += lvlBonus * 5 / 2;
	cmon->cmHit2 += lvlBonus * 5 / 2;
	cmon->cmMagic += lvlBonus * 5 / 2;
	cmon->cmEvasion += lvlBonus * 5 / 2;
	cmon->cmArmorClass += lvlBonus * 5 / 2;

	baseLvl = cmon->cmLevel;
	monLvl = baseLvl + lvlBonus;
	cmon->cmLevel = monLvl;
	cmon->cmMinHP = monLvl * cmon->cmMinHP / baseLvl;
	cmon->cmMaxHP = monLvl * cmon->cmMaxHP / baseLvl;
	cmon->cmExp = monLvl * cmon->cmExp / baseLvl;
	cmon->cmMinDamage = monLvl * cmon->cmMinDamage / baseLvl;
	cmon->cmMaxDamage = monLvl * cmon->cmMaxDamage / baseLvl;
	cmon->cmMinDamage2 = monLvl * cmon->cmMinDamage2 / baseLvl;
	cmon->cmMaxDamage2 = monLvl * cmon->cmMaxDamage2 / baseLvl;

	if (gnDifficulty == DIFF_HELL) {
		cmon->cmMagicRes = monsterdata[cmon->cmType].mMagicRes2;
	}

	int mpl = currLvl._dLevelPlyrs;
	// assert(mpl != 0);
	mpl++;
	cmon->cmMinHP = (cmon->cmMinHP * mpl) >> 1;
	cmon->cmMaxHP = (cmon->cmMaxHP * mpl) >> 1;
	cmon->cmExp = (cmon->cmExp * mpl) >> 1;
}

static bool IsSkel(int mt)
{
	return (mt >= MT_WSKELAX && mt <= MT_XSKELAX)
	    || (mt >= MT_WSKELBW && mt <= MT_XSKELBW)
	    || (mt >= MT_WSKELSD && mt <= MT_XSKELSD);
}

static bool IsGoat(int mt)
{
	return (mt >= MT_NGOATMC && mt <= MT_GGOATMC)
	    || (mt >= MT_NGOATBW && mt <= MT_GGOATBW);
}

static int AddMonsterType(int type, BOOL scatter)
{
	int i;

	for (i = 0; i < nummtypes && mapMonTypes[i].cmType != type; i++)
		;

	if (i == nummtypes) {
		nummtypes++;
		assert(nummtypes <= MAX_LVLMTYPES);
		if (IsGoat(type)) {
			mapGoatTypes[numGoatTypes] = i;
			numGoatTypes++;
		}
		if (IsSkel(type)) {
			mapSkelTypes[numSkelTypes] = i;
			numSkelTypes++;
		}
		mapMonTypes[i].cmType = type;
		mapMonTypes[i].cmPlaceScatter = FALSE;
		InitMonsterStats(i); // init stats first because InitMonsterGFX depends on it (cmFileNum)
		InitMonsterGFX(i);
		// InitMonsterSFX(i);
	}

	if (scatter && !mapMonTypes[i].cmPlaceScatter) {
		mapMonTypes[i].cmPlaceScatter = TRUE;
		monstimgtot -= monfiledata[monsterdata[type].moFileNum].moImage;
	}

	return i;
}

void InitLvlMonsters()
{
	int i;

	nummtypes = 0;

	// reset monsters
	for (i = 0; i < MAXMONSTERS; i++) {
		monsters[i]._mmode = MM_UNUSED;
		// reset _mMTidx value to simplify SyncMonsterAnim (loadsave.cpp)
		monsters[i]._mMTidx = 0;
		// reset _muniqtype value to simplify SyncMonsterAnim (loadsave.cpp)
		// reset _mlid value to simplify SyncMonstersLight, DeltaLoadLevel, SummonMonster and InitTownerInfo
		monsters[i]._muniqtype = 0;
		monsters[i]._muniqtrans = 0;
		monsters[i]._mNameColor = COL_WHITE;
		monsters[i]._mlid = NO_LIGHT;
		// reset _mleaderflag value to simplify GroupUnity
		// monsters[i]._mleader = MON_NO_LEADER;
		// monsters[i]._mleaderflag = MLEADER_NONE;
		// monsters[i]._mpacksize = 0;
		// monsters[i]._mvid = NO_VISION;
	}
}

void InitMonster(int mnum, int dir, int mtidx, int x, int y)
{
	MapMonData* cmon = &mapMonTypes[mtidx];
	MonsterStruct* mon = &monsters[mnum];

	mon->_mMTidx = mtidx;
	mon->_mx = x;
	mon->_my = y;
	mon->_mdir = dir;

	mon->_mName = cmon->cmName;
	// mon->_mFileNum = cmon->cmFileNum;
	mon->_mLevel = cmon->cmLevel;
	mon->_mSelFlag = cmon->cmSelFlag;
	mon->_mAI = cmon->cmAI; // aiType, aiInt, aiParam1, aiParam2
	mon->_mFlags = cmon->cmFlags;
	mon->_mHit = cmon->cmHit;
	mon->_mMinDamage = cmon->cmMinDamage;
	mon->_mMaxDamage = cmon->cmMaxDamage;
	mon->_mHit2 = cmon->cmHit2;
	mon->_mMinDamage2 = cmon->cmMinDamage2;
	mon->_mMaxDamage2 = cmon->cmMaxDamage2;
	mon->_mMagic = cmon->cmMagic;
	mon->_mArmorClass = cmon->cmArmorClass;
	mon->_mEvasion = cmon->cmEvasion;
	mon->_mMagicRes = cmon->cmMagicRes;
	// mon->_mAlign_1 = cmon->cmAlign_1;
	mon->_mExp = cmon->cmExp;
	// mon->_mAnimWidth = cmon->cmWidth;
	// mon->_mAnimXOffset = cmon->cmXOffset;
	// mon->_mAFNum = cmon->cmAFNum;
	// mon->_mAFNum2 = cmon->cmAFNum2;
	// mon->_mAlign_0 = cmon->cmAlign_0;
	mon->_mmaxhp = cmon->cmMaxHP;
    mon->_mhitpoints = cmon->cmMinHP;
	mon->_mAnimFrameLen = cmon->cmAnims[MA_STAND].maFrameLen;
	// mon->_mAnimCnt = random_low(88, mon->_mAnimFrameLen);
	mon->_mAnimLen = cmon->cmAnims[MA_STAND].maFrames;
	// mon->_mAnimFrame = mon->_mAnimLen == 0 ? 1 : RandRangeLow(1, mon->_mAnimLen);
	mon->_mmode = MM_STAND;
	// mon->_mRndSeed = NextRndSeed();

	mon->_muniqtype = 0;
	mon->_muniqtrans = 0;
	mon->_mNameColor = COL_WHITE;
	mon->_mlid = NO_LIGHT;

	mon->_mleader = MON_NO_LEADER;
	mon->_mleaderflag = MLEADER_NONE;
	mon->_mpacksize = 0;
	mon->_mvid = NO_VISION;
}

static int PlaceMonster(int mtidx, int x, int y)
{
    int mnum = MAX_MINIONS + 1;
    InitMonster(mnum, 0, mtidx, x, y);
    return mnum;
}

static void PlaceGroup(int mtidx, int num, int leaderf, int leader)
{
    int xp = 0, yp = 0, mnum;

    mnum = PlaceMonster(mtidx, xp, yp);
    if (leaderf) {
        // assert(leaderf & UMF_GROUP);
        monsters[mnum]._mNameColor = COL_BLUE;
        monsters[mnum]._mmaxhp *= 2;
        monsters[mnum]._mhitpoints = monsters[mnum]._mmaxhp;
        monsters[mnum]._mAI.aiInt = monsters[leader]._mAI.aiInt;

        if (leaderf & UMF_LEADER) {
            monsters[mnum]._mleader = leader;
            monsters[mnum]._mleaderflag = MLEADER_PRESENT;
            monsters[mnum]._mAI = monsters[leader]._mAI;
        }
    }
}

static unsigned InitUniqueMonster(int mnum, int uniqindex)
{
//	char filestr[DATA_ARCHIVE_MAX_PATH];
	const UniqMonData* uniqm;
	MonsterStruct* mon;
	unsigned baseLvl, lvlBonus, monLvl;

#ifdef HELLFIRE
    if (uniqindex == UMT_NAKRUL && mnum != MAX_MINIONS) {
        dProgressErr() << QApplication::tr("Bad Na-Krul placement. Received-Id:%1 instead of %2.").arg(mnum).arg(MAX_MINIONS);
    }
#endif
	mon = &monsters[mnum];
	mon->_mNameColor = COL_GOLD;
	mon->_muniqtype = uniqindex + 1;

	uniqm = &uniqMonData[uniqindex];
	mon->_mLevel = uniqm->muLevel;

	mon->_mExp *= 2;
	mon->_mName = uniqm->mName;
	mon->_mhitpoints = mon->_mmaxhp = uniqm->mmaxhp;

	mon->_mAI = uniqm->mAI;
	mon->_mMinDamage = uniqm->mMinDamage;
	mon->_mMaxDamage = uniqm->mMaxDamage;
	mon->_mMinDamage2 = uniqm->mMinDamage2;
	mon->_mMaxDamage2 = uniqm->mMaxDamage2;
	mon->_mMagicRes = uniqm->mMagicRes;

	mon->_mHit += uniqm->mUnqHit;
	mon->_mHit2 += uniqm->mUnqHit2;
	mon->_mMagic += uniqm->mUnqMag;
	mon->_mEvasion += uniqm->mUnqEva;
	mon->_mArmorClass += uniqm->mUnqAC;

	lvlBonus = currLvl._dLevelBonus;
	mon->_mAI.aiInt += lvlBonus / 16;

	/*mon->_mHit += lvlBonus * 5 / 2;
	mon->_mHit2 += lvlBonus * 5 / 2;
	mon->_mMagic += lvlBonus * 5 / 2;
	mon->_mEvasion += lvlBonus * 5 / 2;
	mon->_mArmorClass += lvlBonus * 5 / 2;*/

	baseLvl = mon->_mLevel;
	monLvl = baseLvl + lvlBonus;
	mon->_mLevel = monLvl;
	mon->_mmaxhp = monLvl * mon->_mmaxhp / baseLvl;
	// mon->_mExp = monLvl * mon->_mExp / baseLvl;
	mon->_mMinDamage = monLvl * mon->_mMinDamage / baseLvl;
	mon->_mMaxDamage = monLvl * mon->_mMaxDamage / baseLvl;
	mon->_mMinDamage2 = monLvl * mon->_mMinDamage2 / baseLvl;
	mon->_mMaxDamage2 = monLvl * mon->_mMaxDamage2 / baseLvl;

	if (gnDifficulty == DIFF_HELL) {
		mon->_mMagicRes = uniqm->mMagicRes2;
	}

	int mpl = currLvl._dLevelPlyrs;
	// assert(mpl != 0);
	mpl++;
	/*mon->_mmaxhp = (mon->_mmaxhp * mpl) >> 1;
	// mon->_mExp = (mon->_mExp * mpl) >> 1;

	mon->_mmaxhp <<= 6;*/
	mon->_mhitpoints = mon->_mmaxhp = (mon->_mmaxhp * mpl) >> 1;

	unsigned flags = uniqm->mUnqFlags;
	if (flags & UMF_NODROP)
		mon->_mFlags |= MFLAG_NODROP;
//	static_assert(MAX_LIGHT_RAD >= MON_LIGHTRAD, "Light-radius of unique monsters are too high.");
//	if (flags & UMF_LIGHT) {
//		mon->_mlid = AddLight(mon->_mx, mon->_my, MON_LIGHTRAD);
//	}
	return flags;
}

static int InitBaseMonster(int type, int numplrs, int lvlBonus)
{
    int mnum = MAX_MINIONS;
    int mtidx = 0;
    monstimgtot = MAX_LVLMIMAGE;
    nummtypes = 0;
    currLvl._dLevelPlyrs = numplrs;
    if (gnDifficulty == DIFF_NIGHTMARE) {
        lvlBonus += NIGHTMARE_LEVEL_BONUS;
    } else if (gnDifficulty == DIFF_HELL) {
        lvlBonus += HELL_LEVEL_BONUS;
    }
    currLvl._dLevelBonus = lvlBonus;
    AddMonsterType(type, false);
    InitMonster(mnum, 0, mtidx, 0, 0);
    return mnum;
}

void InitLvlMonster(int type, int numplrs, int lvlBonus)
{
    InitBaseMonster(type, numplrs, lvlBonus);
}

void InitUniqMonster(int uniqindex, int numplrs, int lvlbonus, bool minion)
{
    int mnum = InitBaseMonster(uniqMonData[uniqindex].mtype, numplrs, lvlbonus);
    int flags = InitUniqueMonster(mnum, uniqindex);
    int mtidx = 0;
    if (minion) {
        PlaceGroup(mtidx, MON_PACK_SIZE - 1, flags, mnum);
        memcpy(&monsters[mnum], &monsters[mnum + 1], sizeof(MonsterStruct));
    }
}
