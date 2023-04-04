/**
 * @file monster.cpp
 *
 * Implementation of monster functionality, AI, actions, spawning, loading, etc.
 */
#include "all.h"

#include <QApplication>

#include "../progressdialog.h"

/* Limit the number of monsters to be placed. */
int totalmonsters;
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
#define MON_LIGHTRAD 3

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

static inline void InitMonsterTRN(MonAnimStruct (&anims)[NUM_MON_ANIM], const char* transFile)
{
	/*BYTE* cf;
	int i, j;
	const MonAnimStruct* as;
	BYTE trn[NUM_COLORS];

	// A TRN file contains a sequence of color transitions, represented
	// as indexes into a palette. (a 256 byte array of palette indices)
	LoadFileWithMem(transFile, trn);
	// patch TRN files - Monsters/*.TRN
	cf = trn;
	for (i = 0; i < NUM_COLORS; i++) {
		if (*cf == 255) {
			*cf = 0;
		}
		cf++;
	}

	for (i = 0; i < NUM_MON_ANIM; i++) {
		as = &anims[i];
		if (as->maFrames > 1) {
			for (j = 0; j < lengthof(as->maAnimData); j++) {
				Cl2ApplyTrans(as->maAnimData[j], trn, as->maFrames);
			}
		}
	}*/
}

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
	cmon->cmAFNum = mfdata->moAFNum;
	cmon->cmAFNum2 = mfdata->moAFNum2;

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
	cmon->cmMagic2 = mdata->mMagic2;
	cmon->cmArmorClass = mdata->mArmorClass;
	cmon->cmEvasion = mdata->mEvasion;
	cmon->cmMagicRes = mdata->mMagicRes;
	cmon->cmTreasure = mdata->mTreasure;
	cmon->cmExp = mdata->mExp;
	cmon->cmMinHP = mdata->mMinHP;
	cmon->cmMaxHP = mdata->mMaxHP;

	cmon->cmAI.aiInt += gnDifficulty;
	if (gnDifficulty == DIFF_NIGHTMARE) {
		cmon->cmMinHP = 2 * cmon->cmMinHP + 100;
		cmon->cmMaxHP = 2 * cmon->cmMaxHP + 100;
		cmon->cmLevel += NIGHTMARE_LEVEL_BONUS;
		cmon->cmExp = 2 * (cmon->cmExp + DIFFICULTY_EXP_BONUS);
		cmon->cmHit += NIGHTMARE_TO_HIT_BONUS;
		cmon->cmMagic += NIGHTMARE_MAGIC_BONUS;
		cmon->cmMinDamage = 2 * (cmon->cmMinDamage + 2);
		cmon->cmMaxDamage = 2 * (cmon->cmMaxDamage + 2);
		cmon->cmHit2 += NIGHTMARE_TO_HIT_BONUS;
		//cmon->cmMagic2 += NIGHTMARE_MAGIC_BONUS;
		cmon->cmMinDamage2 = 2 * (cmon->cmMinDamage2 + 2);
		cmon->cmMaxDamage2 = 2 * (cmon->cmMaxDamage2 + 2);
		cmon->cmArmorClass += NIGHTMARE_AC_BONUS;
		cmon->cmEvasion += NIGHTMARE_EVASION_BONUS;
	} else if (gnDifficulty == DIFF_HELL) {
		cmon->cmMinHP = 4 * cmon->cmMinHP + 200;
		cmon->cmMaxHP = 4 * cmon->cmMaxHP + 200;
		cmon->cmLevel += HELL_LEVEL_BONUS;
		cmon->cmExp = 4 * (cmon->cmExp + DIFFICULTY_EXP_BONUS);
		cmon->cmHit += HELL_TO_HIT_BONUS;
		cmon->cmMagic += HELL_MAGIC_BONUS;
		cmon->cmMinDamage = 4 * cmon->cmMinDamage + 6;
		cmon->cmMaxDamage = 4 * cmon->cmMaxDamage + 6;
		cmon->cmHit2 += HELL_TO_HIT_BONUS;
		//cmon->cmMagic2 += HELL_MAGIC_BONUS;
		cmon->cmMinDamage2 = 4 * cmon->cmMinDamage2 + 6;
		cmon->cmMaxDamage2 = 4 * cmon->cmMaxDamage2 + 6;
		cmon->cmArmorClass += HELL_AC_BONUS;
		cmon->cmEvasion += HELL_EVASION_BONUS;
		cmon->cmMagicRes = monsterdata[cmon->cmType].mMagicRes2;
	}
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
	}

	if (scatter && !mapMonTypes[i].cmPlaceScatter) {
		mapMonTypes[i].cmPlaceScatter = TRUE;
		monstimgtot -= monfiledata[monsterdata[type].moFileNum].moImage;
	}

	return i;
}

void InitLevelMonsters()
{
	int i;

	nummtypes = 0;
	numSkelTypes = 0;
	numGoatTypes = 0;
	uniquetrans = COLOR_TRN_UNIQ;
	monstimgtot = MAX_LVLMIMAGE - monfiledata[monsterdata[MT_GOLEM].moFileNum].moImage;
	totalmonsters = MAXMONSTERS;

	// reset monsters
	for (i = 0; i < MAXMONSTERS; i++) {
		monsters[i]._mmode = MM_UNUSED;
		// reset _mMTidx value to simplify SyncMonsterAnim (loadsave.cpp)
		monsters[i]._mMTidx = 0;
		// reset _muniqtype value to simplify SyncMonsterAnim (loadsave.cpp)
		// reset _mlid value to simplify SyncMonsterLight, DeltaLoadLevel, SummonMonster and InitTownerInfo
		monsters[i]._muniqtype = 0;
		monsters[i]._muniqtrans = 0;
		monsters[i]._mNameColor = COL_WHITE;
		monsters[i]._mlid = NO_LIGHT;
		// reset _mleaderflag value to simplify GroupUnity
		monsters[i]._mleader = MON_NO_LEADER;
		monsters[i]._mleaderflag = MLEADER_NONE;
		monsters[i]._mpacksize = 0;
		monsters[i]._mvid = NO_VISION;
	}
	// reserve minions
	nummonsters = MAX_MINIONS;
	if (currLvl._dLevelIdx != DLV_TOWN) {
		AddMonsterType(MT_GOLEM, FALSE);
		for (i = 0; i < MAX_MINIONS; i++) {
			InitMonster(i, 0, 0, 0, 0);
			monsters[i]._mmode = MM_RESERVED;
		}
	}
}

void GetLevelMTypes()
{
	int i, mtype;
	int montypes[lengthof(AllLevels[0].dMonTypes)];
	const LevelData* lds;
	BYTE lvl;

	int nt; // number of types

	lvl = currLvl._dLevelIdx;
	assert(!currLvl._dSetLvl);
	//if (!currLvl._dSetLvl) {
		if (lvl == DLV_HELL4) {
			AddMonsterType(MT_BMAGE, TRUE);
			AddMonsterType(MT_GBLACK, TRUE);
			// AddMonsterType(MT_NBLACK, FALSE);
			// AddMonsterType(uniqMonData[UMT_DIABLO].mtype, FALSE);
			return;
		}

#ifdef HELLFIRE
		if (lvl == DLV_NEST2)
			AddMonsterType(MT_HORKSPWN, TRUE);
		if (lvl == DLV_NEST3) {
			AddMonsterType(MT_HORKSPWN, TRUE);
			AddMonsterType(uniqMonData[UMT_HORKDMN].mtype, FALSE);
		}
		if (lvl == DLV_NEST4)
			AddMonsterType(uniqMonData[UMT_DEFILER].mtype, FALSE);
		if (lvl == DLV_CRYPT4) {
			AddMonsterType(MT_ARCHLICH, TRUE);
			// AddMonsterType(uniqMonData[UMT_NAKRUL].mtype, FALSE);
		}
#endif
		//if (QuestStatus(Q_BUTCHER))
		//	AddMonsterType(uniqMonData[UMT_BUTCHER].mtype, FALSE);
		if (QuestStatus(Q_GARBUD))
			AddMonsterType(uniqMonData[UMT_GARBUD].mtype, FALSE);
		if (QuestStatus(Q_ZHAR))
			AddMonsterType(uniqMonData[UMT_ZHAR].mtype, FALSE);
		//if (QuestStatus(Q_BANNER)) {
		//	AddMonsterType(uniqMonData[UMT_SNOTSPIL].mtype, FALSE);
		//	// AddMonsterType(MT_NFAT, FALSE);
		//}
		//if (QuestStatus(Q_ANVIL)) {
		//	AddMonsterType(MT_GGOATBW, FALSE);
		//	AddMonsterType(MT_DRHINO, FALSE);
		//}
		//if (QuestStatus(Q_BLIND)) {
		//	AddMonsterType(MT_YSNEAK, FALSE);
		//}
		//if (QuestStatus(Q_BLOOD)) {
		//	AddMonsterType(MT_NRHINO, FALSE);
		//}
		if (QuestStatus(Q_VEIL))
			AddMonsterType(uniqMonData[UMT_LACHDAN].mtype, TRUE);
		if (QuestStatus(Q_WARLORD))
			AddMonsterType(uniqMonData[UMT_WARLORD].mtype, TRUE);
		//if (QuestStatus(Q_BETRAYER) && IsMultiGame) {
		//if (currLvl._dLevelIdx == questlist[Q_BETRAYER]._qdlvl && IsMultiGame) {
		//	AddMonsterType(uniqMonData[UMT_LAZARUS].mtype, FALSE);
		//	AddMonsterType(uniqMonData[UMT_RED_VEX].mtype, FALSE);
		//  assert(uniqMonData[UMT_RED_VEX].mtype == uniqMonData[UMT_BLACKJADE].mtype);
		//}
		lds = &AllLevels[lvl];
		for (nt = 0; nt < lengthof(lds->dMonTypes); nt++) {
			mtype = lds->dMonTypes[nt];
			if (mtype == MT_INVALID)
				break;
			montypes[nt] = mtype;
		}

#if DEBUG_MODE
		if (monstdebug) {
			for (i = 0; i < debugmonsttypes; i++)
				AddMonsterType(DebugMonsters[i], TRUE);
			return;
		}
#endif
		while (monstimgtot > 0/* && nummtypes < MAX_LVLMTYPES*/) { // nummtypes test is pointless, because PlaceSetMapMonsters can break it anyway...
			for (i = 0; i < nt; ) {
				if (monfiledata[monsterdata[montypes[i]].moFileNum].moImage > monstimgtot) {
					montypes[i] = montypes[--nt];
					continue;
				}

				i++;
			}

			if (nt == 0)
				break;

			i = random_low(88, nt);
			AddMonsterType(montypes[i], TRUE);
			montypes[i] = montypes[--nt];
		}
	//} else {
	//	if (lvl == SL_SKELKING) {
	//		AddMonsterType(uniqMonData[UMT_SKELKING].mtype, FALSE);
	//	} else if (lvl == SL_VILEBETRAYER) {
	//		AddMonsterType(uniqMonData[UMT_LAZARUS].mtype, FALSE);
	//		AddMonsterType(uniqMonData[UMT_RED_VEX].mtype, FALSE);
	//		assert(uniqMonData[UMT_RED_VEX].mtype == uniqMonData[UMT_BLACKJADE].mtype);
	//	}
	//}
}

void InitMonster(int mnum, int dir, int mtidx, int x, int y)
{
	MapMonData* cmon = &mapMonTypes[mtidx];
	MonsterStruct* mon = &monsters[mnum];

	mon->_mMTidx = mtidx;
	mon->_mx = x;
	mon->_my = y;
	mon->_mdir = dir;
	mon->_mType = cmon->cmType;
	mon->_mmaxhp = RandRangeLow(cmon->cmMinHP, cmon->cmMaxHP);
	mon->_mAnimFrameLen = cmon->cmAnims[MA_STAND].maFrameLen;
	mon->_mAnimCnt = random_low(88, mon->_mAnimFrameLen);
	mon->_mAnimLen = cmon->cmAnims[MA_STAND].maFrames;
	mon->_mAnimFrame = mon->_mAnimLen == 0 ? 1 : RandRangeLow(1, mon->_mAnimLen);
	mon->_mRndSeed = NextRndSeed();

	mon->_muniqtype = 0;
	mon->_muniqtrans = 0;
	mon->_mNameColor = COL_WHITE;
	mon->_mlid = NO_LIGHT;

	mon->_mleader = MON_NO_LEADER;
	mon->_mleaderflag = MLEADER_NONE;
	mon->_mpacksize = 0;
	mon->_mvid = NO_VISION;
}

/**
 * Check the location if a monster can be placed there in the init phase.
 * Must not consider the player's position, since it is already initialized
 * and messes up the pseudo-random generated dungeon.
 */
static bool MonstPlace(int xp, int yp)
{
	static_assert(DBORDERX >= MON_PACK_DISTANCE, "MonstPlace does not check IN_DUNGEON_AREA but expects a large enough border I.");
	static_assert(DBORDERY >= MON_PACK_DISTANCE, "MonstPlace does not check IN_DUNGEON_AREA but expects a large enough border II.");
	return (dMonster[xp][yp] | /*dPlayer[xp][yp] |*/ nSolidTable[dPiece[xp][yp]]
		 | (dFlags[xp][yp] & (BFLAG_ALERT | BFLAG_POPULATED))) == 0;
}

static int PlaceMonster(int mtidx, int x, int y)
{
	int mnum, dir;

	mnum = nummonsters;
	nummonsters++;
	dMonster[x][y] = mnum + 1;

	dir = random_(90, NUM_DIRS);
	InitMonster(mnum, dir, mtidx, x, y);
	return mnum;
}

void AddMonster(int mtidx, int x, int y)
{
	if (nummonsters < MAXMONSTERS) {
		PlaceMonster(mtidx, x, y);
	}
}

static void PlaceGroup(int mtidx, int num, int leaderf, int leader)
{
	int placed, offset, try1, try2;
	int xp, yp, x1, y1, x2, y2, mnum;

	if (num + nummonsters > totalmonsters) {
		num = totalmonsters - nummonsters;
	}

	placed = 0;
	for (try1 = 0; try1 < 10; try1++) {
		while (placed != 0) {
			nummonsters--;
			placed--;
			dMonster[monsters[nummonsters]._mx][monsters[nummonsters]._my] = 0;
		}

		if (leaderf & UMF_GROUP) {
			x1 = monsters[leader]._mx;
			y1 = monsters[leader]._my;
		} else {
			do {
				x1 = random_(93, DSIZEX) + DBORDERX;
				y1 = random_(93, DSIZEY) + DBORDERY;
			} while (!MonstPlace(x1, y1));
		}

		if (dTransVal[x1][y1] == 0) {
			dProgressErr() << QApplication::tr("Missing room-ID for possible monster-placement at %1:%2").arg(x1).arg(y1);
			continue;
		}
		//assert(dTransVal[x1][y1] != 0);
		static_assert(DBORDERX >= 1, "PlaceGroup expects a large enough border I.");
		static_assert(DBORDERY >= 1, "PlaceGroup expects a large enough border II.");
		xp = x1; yp = y1;
		for (try2 = 0; placed < num && try2 < 128; try2++) {
			offset = random_(94, NUM_DIRS);
			x2 = xp + offset_x[offset];
			y2 = yp + offset_y[offset];
			assert((unsigned)x2 < MAXDUNX);
			assert((unsigned)y2 < MAXDUNX);
			if (dTransVal[x2][y2] != dTransVal[x1][y1]
			 || ((leaderf & UMF_LEADER) && ((abs(x2 - x1) > MON_PACK_DISTANCE) || (abs(y2 - y1) > MON_PACK_DISTANCE)))) {
				continue;
			}
			xp = x2;
			yp = y2;
			if ((!MonstPlace(xp, yp)) || random_(0, 2) != 0)
				continue;
			// assert(nummonsters < MAXMONSTERS);
			mnum = PlaceMonster(mtidx, xp, yp);
			if (leaderf & UMF_GROUP) {
				monsters[mnum]._mNameColor = COL_BLUE;
				monsters[mnum]._mmaxhp *= 2;
				monsters[mnum]._mAI.aiInt = monsters[leader]._mAI.aiInt;

				if (leaderf & UMF_LEADER) {
					monsters[mnum]._mleader = leader;
					monsters[mnum]._mleaderflag = MLEADER_PRESENT;
					monsters[mnum]._mAI = monsters[leader]._mAI;
				}
			}
			placed++;
		}

		if (placed >= num) {
			break;
		}
	}

	if (leaderf & UMF_LEADER) {
		monsters[leader]._mleaderflag = MLEADER_SELF;
		monsters[leader]._mpacksize = placed;
	}
}

static void InitUniqueMonster(int mnum, int uniqindex)
{
//	char filestr[DATA_ARCHIVE_MAX_PATH];
	const UniqMonData* uniqm;
	MonsterStruct* mon;

	mon = &monsters[mnum];
	mon->_mNameColor = COL_GOLD;
	mon->_muniqtype = uniqindex + 1;

	uniqm = &uniqMonData[uniqindex];
	mon->_mLevel = uniqm->muLevel;

	mon->_mName = uniqm->mName;
	mon->_mmaxhp = uniqm->mmaxhp;

	mon->_mAI = uniqm->mAI;
	/*mon->_mMinDamage = uniqm->mMinDamage;
	mon->_mMaxDamage = uniqm->mMaxDamage;
	mon->_mMinDamage2 = uniqm->mMinDamage2;
	mon->_mMaxDamage2 = uniqm->mMaxDamage2;*/

	if (uniqm->mTrnName != NULL) {
		/*snprintf(filestr, sizeof(filestr), "Monsters\\Monsters\\%s.TRN", uniqm->mTrnName);
		LoadFileWithMem(filestr, ColorTrns[uniquetrans]);
		// patch TRN for 'Blighthorn Steelmace' - BHSM.TRN
		if (uniqindex == UMT_STEELMACE) {
			// assert(ColorTrns[uniquetrans][188] == 255);
			ColorTrns[uniquetrans][188] = 0;
		}
		// patch TRN for 'Baron Sludge' - BSM.TRN
		if (uniqindex == UMT_BARON) {
			// assert(ColorTrns[uniquetrans][241] == 255);
			ColorTrns[uniquetrans][241] = 0;
		}*/

		mon->_muniqtrans = uniquetrans++;
	}

//	mon->_mHit += uniqm->mUnqHit;
//	mon->_mHit2 += uniqm->mUnqHit2;
//	mon->_mMagic += uniqm->mUnqMag;
//	mon->_mEvasion += uniqm->mUnqEva;
//	mon->_mArmorClass += uniqm->mUnqAC;
	mon->_mAI.aiInt += gnDifficulty;

	if (gnDifficulty == DIFF_NIGHTMARE) {
		mon->_mmaxhp = 2 * mon->_mmaxhp + 100;
		mon->_mLevel += NIGHTMARE_LEVEL_BONUS;
//		mon->_mMinDamage = 2 * (mon->_mMinDamage + 2);
//		mon->_mMaxDamage = 2 * (mon->_mMaxDamage + 2);
//		mon->_mMinDamage2 = 2 * (mon->_mMinDamage2 + 2);
//		mon->_mMaxDamage2 = 2 * (mon->_mMaxDamage2 + 2);
	} else if (gnDifficulty == DIFF_HELL) {
		mon->_mmaxhp = 4 * mon->_mmaxhp + 200;
		mon->_mLevel += HELL_LEVEL_BONUS;
//		mon->_mMinDamage = 4 * mon->_mMinDamage + 6;
//		mon->_mMaxDamage = 4 * mon->_mMaxDamage + 6;
//		mon->_mMinDamage2 = 4 * mon->_mMinDamage2 + 6;
//		mon->_mMaxDamage2 = 4 * mon->_mMaxDamage2 + 6;
//		mon->_mMagicRes = uniqm->mMagicRes2;
	}
}

static void PlaceUniqueMonst(int uniqindex, int mtidx)
{
	int xp, yp, x, y;
	int count2;
	int mnum, count;
	static_assert(NUM_COLOR_TRNS <= UCHAR_MAX, "Color transform index stored in BYTE field.");
	if (uniquetrans >= NUM_COLOR_TRNS) {
		return;
	}

	switch (uniqindex) {
	case UMT_ZHAR:
		assert(nummonsters == MAX_MINIONS);
		if (zharlib == -1)
			return;
		xp = 2 * themes[zharlib]._tsx + DBORDERX + 4;
		yp = 2 * themes[zharlib]._tsy + DBORDERY + 4;
		break;
	default:
		count = 0;
		while (TRUE) {
			xp = random_(91, DSIZEX) + DBORDERX;
			yp = random_(91, DSIZEY) + DBORDERY;
			count2 = 0;
			for (x = xp - MON_PACK_DISTANCE; x <= xp + MON_PACK_DISTANCE; x++) {
				for (y = yp - MON_PACK_DISTANCE; y <= yp + MON_PACK_DISTANCE; y++) {
					if (MonstPlace(x, y)) {
						count2++;
					}
				}
			}

			if (count2 < 2 * MON_PACK_SIZE) {
				count++;
				if (count < 1000) {
					continue;
				}
			}

			if (MonstPlace(xp, yp)) {
				break;
			}
		}
	}
	// assert(nummonsters < MAXMONSTERS);
	mnum = PlaceMonster(mtidx, xp, yp);
	InitUniqueMonster(mnum, uniqindex);
}

static void PlaceUniques()
{
	int u, mt;

	for (u = 0; uniqMonData[u].mtype != MT_INVALID; u++) {
		if (uniqMonData[u].muLevelIdx != currLvl._dLevelIdx)
			continue;
		if (uniqMonData[u].mQuestId != Q_INVALID
		 && quests[uniqMonData[u].mQuestId]._qactive == QUEST_NOTAVAIL)
			continue;
		for (mt = 0; mt < nummtypes; mt++) {
			if (mapMonTypes[mt].cmType == uniqMonData[u].mtype) {
				PlaceUniqueMonst(u, mt);
				if (uniqMonData[u].mUnqFlags & UMF_GROUP) {
					// assert(mnum == nummonsters - 1);
					PlaceGroup(mt, MON_PACK_SIZE - 1, uniqMonData[u].mUnqFlags, nummonsters - 1);
				}
				break;
			}
		}
	}
}

static void PlaceSetMapMonsters()
{
	for (int i = lengthof(pSetPieces) - 1; i >= 0; i--) {
		if (pSetPieces[i]._spData != NULL) { // pSetPieces[i]._sptype != SPT_NONE
			SetMapMonsters(pSetPieces[i]._spData, pSetPieces[i]._spx, pSetPieces[i]._spy);
		}
	}
}

void InitMonsters()
{
	TriggerStruct* ts;
	unsigned na, numplacemonsters, numscattypes;
	int i, j, xx, yy;
	int mtidx;
	int scatteridx[MAX_LVLMTYPES];
	const int tdx[4] = { -1, -1,  2,  2 };
	const int tdy[4] = { -1,  2, -1,  2 };

	// reserve the entry/exit area
	for (i = 0; i < numtrigs; i++) {
		ts = &trigs[i];
		if (ts->_tmsg == DVL_DWM_TWARPUP || ts->_tmsg == DVL_DWM_PREVLVL
		 || (ts->_tmsg == DVL_DWM_NEXTLVL && currLvl._dLevelIdx != DLV_HELL3)) {
			static_assert(MAX_LIGHT_RAD >= 15, "Tile reservation in InitMonsters requires at least 15 light radius.");
			for (j = 0; j < lengthof(tdx); j++)
				DoVision(ts->_tx + tdx[j], ts->_ty + tdy[j], 15);
		}
	}
	// if (currLvl._dLevelIdx == DLV_HELL3) {
	//	DoVision(quests[Q_BETRAYER]._qtx + 2, quests[Q_BETRAYER]._qty + 2, 4);
	// }
	// place the setmap/setpiece monsters
	PlaceSetMapMonsters();
	// if (!currLvl._dSetLvl) {
		// calculate the available space for monsters
		na = 0;
		for (xx = DBORDERX; xx < DSIZEX + DBORDERX; xx++)
			for (yy = DBORDERY; yy < DSIZEY + DBORDERY; yy++)
				if ((nSolidTable[dPiece[xx][yy]] | (dFlags[xx][yy] & (BFLAG_ALERT | BFLAG_POPULATED))) == 0)
					na++;
		numplacemonsters = na / 30;
		if (IsMultiGame)
			numplacemonsters += numplacemonsters >> 1;
		totalmonsters = nummonsters + numplacemonsters;
		if (totalmonsters > MAXMONSTERS - 10)
			totalmonsters = MAXMONSTERS - 10;
		// place quest/unique monsters
		PlaceUniques();
		numscattypes = 0;
		for (i = 0; i < nummtypes; i++) {
			if (mapMonTypes[i].cmPlaceScatter) {
				scatteridx[numscattypes] = i;
				numscattypes++;
			}
		}
		// assert(numscattypes != 0);
		i = currLvl._dLevelIdx;
		while (nummonsters < totalmonsters) {
			mtidx = scatteridx[random_low(95, numscattypes)];
			if (i == DLV_CATHEDRAL1 || random_(95, 2) == 0)
				na = 1;
#ifdef HELLFIRE
			else if (i == DLV_CATHEDRAL2 || (i >= DLV_CRYPT1 && i <= DLV_CRYPT4))
#else
			else if (i == DLV_CATHEDRAL2)
#endif
				na = RandRange(2, 3);
			else
				na = RandRange(3, 5);
			PlaceGroup(mtidx, na, 0, 0);
		}
	// }
	// revert entry/exit area reservation
	for (i = 0; i < numtrigs; i++) {
		ts = &trigs[i];
		if (ts->_tmsg == DVL_DWM_TWARPUP || ts->_tmsg == DVL_DWM_PREVLVL
		 || (ts->_tmsg == DVL_DWM_NEXTLVL && currLvl._dLevelIdx != DLV_HELL3)) {
			for (j = 0; j < lengthof(tdx); j++)
				DoUnVision(trigs[i]._tx + tdx[j], trigs[i]._ty + tdy[j], 15);
		}
	}
	// if (currLvl._dLevelIdx == DLV_HELL3) {
	//	DoUnVision(quests[Q_BETRAYER]._qtx + 2, quests[Q_BETRAYER]._qty + 2, 4, false);
	// }
}

void SetMapMonsters(BYTE* pMap, int startx, int starty)
{
	uint16_t rw, rh, *lm, mtype;
	int i, j;
	int mtidx, mnum;
	bool posOk;

	if (pMap == NULL) {
		return;
	}
	lm = (uint16_t*)pMap;
	rw = SwapLE16(*lm);
	lm++;
	rh = SwapLE16(*lm);
	lm++;
	lm += rw * rh; // skip dun
	rw <<= 1;
	rh <<= 1;
	lm += rw * rh; // skip items?

	startx *= 2;
	startx += DBORDERX;
	starty *= 2;
	starty += DBORDERY;
	rw += startx;
	rh += starty;
	for (j = starty; j < rh; j++) {
		for (i = startx; i < rw; i++) {
			if (*lm != 0) {
				mtype = SwapLE16(*lm);
				// assert(nummonsters < MAXMONSTERS);
				posOk = PosOkActor(i, j);
				if ((mtype & (1 << 15)) == 0) {
					mtidx = AddMonsterType(MonstConvTbl[mtype], FALSE);
					mnum = PlaceMonster(mtidx, i, j);
				} else {
					mtype = (mtype & INT16_MAX) - 1;
					mtidx = AddMonsterType(uniqMonData[mtype].mtype, FALSE);
					// assert(uniquetrans < NUM_COLOR_TRNS);
					mnum = PlaceMonster(mtidx, i, j);
					InitUniqueMonster(mnum, mtype);
				}
				if (!posOk) {
					dMonster[i][j] = 0;
					monsters[mnum]._mmode = MM_RESERVED;
					// ChangeLightRadius(monsters[mnum]._mlid, 0);
				}
			}
			lm++;
		}
	}
}

int PreSpawnSkeleton()
{
	int n = numSkelTypes, mnum = -1;

	if (n != 0 && nummonsters < MAXMONSTERS) {
		mnum = nummonsters;
		nummonsters++;
		n = mapSkelTypes[random_low(136, n)];
		InitMonster(mnum, 0, n, 0, 0);
		monsters[mnum]._mmode = MM_RESERVED;
	}
	return mnum;
}
