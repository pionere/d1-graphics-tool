/**
 * @file themes.cpp
 *
 * Implementation of the theme room placing algorithms.
 */
#include "all.h"

#include <QApplication>
#include <QElapsedTimer>
#include "../progressdialog.h"

int numthemes;
static bool _gbShrineFlag;
static bool _gbSkelRoomFlag;
static bool _gbGoatFlag;
static bool _gbWeaponFlag;
static bool _gbArmorFlag;
static bool _gbTreasureFlag;
static bool _gbCauldronFlag;
static bool _gbMFountainFlag;
static bool _gbTFountainFlag;
static bool _gbPFountainFlag;
static bool _gbBFountainFlag;
int zharlib;
ThemeStruct themes[MAXTHEMES];
static int currThemeId;
#define THEMEQUERY 1
//#define THEMEAREA 1
/** Specifies the set of special theme IDs from which one will be selected at random. */
//static const int ThemeGood[4] = { THEME_GOATSHRINE, THEME_SHRINE, THEME_SKELROOM, THEME_LIBRARY };
static const int ThemeGood[] = { THEME_SHRINE, THEME_LIBRARY };
/** Specifies a 5x5 area to fit theme objects. */
static const int trm5x[] = {
	0, 0, 0, 0, 0,
	-1, -1, -1, -1, -1,
	-2, -2, -2, -2, -2,
	1, 1, 1, 1, 1,
	2, 2, 2, 2, 2
};
/** Specifies a 5x5 area to fit theme objects. */
static const int trm5y[] = {
	-2, -1, 0, 1, 2,
	-2, -1, 0, 1, 2,
	-2, -1, 0, 1, 2,
	-2, -1, 0, 1, 2,
	-2, -1, 0, 1, 2
};
/** Specifies a 3x3 area to fit theme objects. */
static const int trm3x[] = {
	0, 0, 0,
	-1, -1, -1,
	1, 1, 1
};
/** Specifies a 3x3 area to fit theme objects. */
static const int trm3y[] = {
	-1, 0, 1,
	-1, 0, 1,
	-1, 0, 1
};

static void QueryTheme(int themeId, void (func)(int x, int y, void *param), void *userParam)
{
	int xx, yy;
	BYTE tv = themes[themeId]._tsTransVal;

	for (xx = themes[themeId]._tsx1; xx < themes[themeId]._tsx2; xx++) {
		for (yy = themes[themeId]._tsy1; yy < themes[themeId]._tsy2; yy++) {
			// if (dTransVal[xx][yy] == tv)
				func(xx, yy, userParam);
		}
	}
}

static int TFit_Shrine(int themeId)
{
	int xx, yy, numMatches;
	BYTE tv = themes[themeId]._tsTransVal;

	numMatches = 0;
    dProgress() << QString("TFit_Shrine check theme %1 Room: %2:%3;%4:%5. ").arg(themeId).arg(themes[themeId]._tsx1).arg(themes[themeId]._tsy1).arg(themes[themeId]._tsx2).arg(themes[themeId]._tsy2);
	for (xx = themes[themeId]._tsx1 - 1; xx < themes[themeId]._tsx2 + 1; xx++) {
		for (yy = themes[themeId]._tsy1 - 1; yy < themes[themeId]._tsy2 + 1; yy++) {
			if (/*dTransVal[xx][yy] == tv &&*/ !nSolidTable[dPiece[xx][yy]]) {
            dProgress() << QString("TFit_Shrine check 0 non solid %1:%2 pnn%3 trap%4 solid%5:%6:%7:%8.").arg(xx).arg(yy).arg(dPiece[xx][yy - 1]).arg(nSpecTrapTable[dPiece[xx][yy - 1]] & PST_TRAP_TYPE)
                .arg(nSolidTable[dPiece[xx - 1][yy]]).arg(nSolidTable[dPiece[xx + 1][yy]]).arg(nSolidTable[dPiece[xx - 1][yy - 1]]).arg(nSolidTable[dPiece[xx + 1][yy - 1]]);
				if ((nSpecTrapTable[dPiece[xx][yy - 1]] & PST_TRAP_TYPE) != PST_NONE
				 // make sure the place is wide enough
				 // - on the inside
				 && !nSolidTable[dPiece[xx - 1][yy]]
				 && !nSolidTable[dPiece[xx + 1][yy]]
				 // - on the wall (to avoid doors)
				 && nSolidTable[dPiece[xx - 1][yy - 1]]
				 && nSolidTable[dPiece[xx + 1][yy - 1]]
				 // make sure it is in the same room
				 /*&& dTransVal[xx - 1][yy] == tv
				 && dTransVal[xx + 1][yy] == tv*/) {
					// assert(dObject[xx][yy] == 0);
					// assert(dObject[xx - 1][yy] == 0);
					// assert(dObject[xx + 1][yy] == 0);
                    if (dTransVal[xx - 1][yy] != tv)
                        dProgressErr() << QString("TFit_Shrine failed to check tv-mismatch at %1:%2. Room: %3:%4;%5:%6.").arg(xx - 1).arg(yy).arg(themes[themeId]._tsx1).arg(themes[themeId]._tsy1).arg(themes[themeId]._tsx2).arg(themes[themeId]._tsy2);
                    if (dTransVal[xx + 1][yy] != tv)
                        dProgressErr() << QString("TFit_Shrine failed to check tv-mismatch at %1:%2. Room: %3:%4;%5:%6.").arg(xx + 1).arg(yy).arg(themes[themeId]._tsx1).arg(themes[themeId]._tsy1).arg(themes[themeId]._tsx2).arg(themes[themeId]._tsy2);
					drlg.thLocs[numMatches].tpdx = xx;
					drlg.thLocs[numMatches].tpdy = yy;
					drlg.thLocs[numMatches].tpdvar1 = 1;
					drlg.thLocs[numMatches].tpdvar2 = 0;
					numMatches++;
					static_assert(lengthof(drlg.thLocs) >= (10 - 2 - (1 + 1)) * (10 - 2 - (1 + 1)), "TFit_Shrine skips limit checks assuming enough thLocs entries I.");
					// if (numMatches == lengthof(drlg.thLocs))
					//	goto done;
				}
            dProgress() << QString("TFit_Shrine check 1 non solid %1:%2 pnn%3 trap%4 solid%5:%6:%7:%8.").arg(xx).arg(yy).arg(dPiece[xx - 1][yy]).arg(nSpecTrapTable[dPiece[xx - 1][yy]] & PST_TRAP_TYPE)
                .arg(nSolidTable[dPiece[xx][yy - 1]]).arg(nSolidTable[dPiece[xx][yy + 1]]).arg(nSolidTable[dPiece[xx - 1][yy - 1]]).arg(nSolidTable[dPiece[xx - 1][yy + 1]]);
				if ((nSpecTrapTable[dPiece[xx - 1][yy]] & PST_TRAP_TYPE) != PST_NONE
				 // make sure the place is wide enough
				 // - on the inside
				 && !nSolidTable[dPiece[xx][yy - 1]]
				 && !nSolidTable[dPiece[xx][yy + 1]]
				 // - on the wall (to avoid doors)
				 && nSolidTable[dPiece[xx - 1][yy - 1]]
				 && nSolidTable[dPiece[xx - 1][yy + 1]]
				 // make sure it is in the same room
				 /*&& dTransVal[xx][yy - 1] == tv
				 && dTransVal[xx][yy + 1] == tv*/) {
					// assert(dObject[xx][yy] == 0);
					// assert(dObject[xx][yy - 1] == 0);
					// assert(dObject[xx][yy + 1] == 0);
                    if (dTransVal[xx - 1][yy] != tv)
                        dProgressErr() << QString("TFit_Shrine failed to check tv-mismatch at %1:%2. Room: %3:%4;%5:%6.").arg(xx).arg(yy - 1).arg(themes[themeId]._tsx1).arg(themes[themeId]._tsy1).arg(themes[themeId]._tsx2).arg(themes[themeId]._tsy2);
                    if (dTransVal[xx + 1][yy] != tv)
                        dProgressErr() << QString("TFit_Shrine failed to check tv-mismatch at %1:%2. Room: %3:%4;%5:%6.").arg(xx).arg(yy + 1).arg(themes[themeId]._tsx1).arg(themes[themeId]._tsy1).arg(themes[themeId]._tsx2).arg(themes[themeId]._tsy2);
					drlg.thLocs[numMatches].tpdx = xx;
					drlg.thLocs[numMatches].tpdy = yy;
					drlg.thLocs[numMatches].tpdvar1 = 0;
					drlg.thLocs[numMatches].tpdvar2 = 0;
					numMatches++;
					static_assert(lengthof(drlg.thLocs) >= (10 - 2 - (1 + 1)) * (10 - 2 - (1 + 1)), "TFit_Shrine skips limit checks assuming enough thLocs entries II.");
					// if (numMatches == lengthof(drlg.thLocs))
					//	goto done;
				}
			}
		}
	}
// done:
    dProgress() << QString("TFit_Shrine check matches %1. ").arg(numMatches);
	if (numMatches == 0)
		return -1;
	static_assert(lengthof(drlg.thLocs) < 0x7FFF, "TFit_Shrine uses random_low to select a matching location.");
	return random_low(0, numMatches);
}

static int TFit_Obj5(int themeId)
{
	int xx, yy, i, numMatches;
	BYTE tv = themes[themeId]._tsTransVal;

	numMatches = 0;
	for (xx = themes[themeId]._tsx1 + 2; xx < themes[themeId]._tsx2 - 2; xx++) {
		for (yy = themes[themeId]._tsy1 + 2; yy < themes[themeId]._tsy2 - 2; yy++) {
			if (/*dTransVal[xx][yy] == tv &&*/ !nSolidTable[dPiece[xx][yy]]) {
				static_assert(lengthof(trm5x) == lengthof(trm5y), "Mismatching trm5 tables.");
				for (i = 0; i < lengthof(trm5x); i++) {
					if (nSolidTable[dPiece[xx + trm5x[i]][yy + trm5y[i]]]) {
						break;
					}
					if (dTransVal[xx + trm5x[i]][yy + trm5y[i]] != tv) {
                        dProgressErr() << QString("TFit_Obj5 failed to check tv-mismatch at %1:%2. Room: %3:%4;%5:%6.").arg(xx + trm5x[i]).arg(yy + trm5y[i]).arg(themes[themeId]._tsx1).arg(themes[themeId]._tsy1).arg(themes[themeId]._tsx2).arg(themes[themeId]._tsy2);
						// break;
					}
				}
				if (i == lengthof(trm5x)) {
					drlg.thLocs[numMatches].tpdx = xx;
					drlg.thLocs[numMatches].tpdy = yy;
					drlg.thLocs[numMatches].tpdvar1 = 0;
					drlg.thLocs[numMatches].tpdvar2 = 0;
					numMatches++;
					static_assert(lengthof(drlg.thLocs) >= (10 - 2 - (3 + 3)) * (10 - 2 - (3 + 3)), "TFit_Obj5 skips limit checks assuming enough thLocs entries II.");
					// if (numMatches == lengthof(drlg.thLocs))
					//	goto done;
				}
			}
		}
	}
// done:
	if (numMatches == 0)
		return -1;
	static_assert(lengthof(drlg.thLocs) < 0x7FFF, "TFit_Obj5 uses random_low to select a matching location.");
	return random_low(0, numMatches);
}

static bool CheckThemeObj3(int x, int y, BYTE tv)
{
	int i, xx, yy;

	static_assert(lengthof(trm3x) == lengthof(trm3y), "Mismatching trm3 tables.");
	for (i = 0; i < lengthof(trm3x); i++) {
		xx = x + trm3x[i];
		yy = y + trm3y[i];
		//if (xx < 0 || yy < 0)
		//	return false;
        if (dTransVal[xx][yy] != tv && !nSolidTable[dPiece[xx][yy]]) {
            dProgressErr() << QString("CheckThemeObj3 failed to check tv-mismatch at %1:%2. Room: %3:%4;%5:%6.").arg(xx).arg(yy).arg(themes[currThemeId]._tsx1).arg(themes[currThemeId]._tsy1).arg(themes[currThemeId]._tsx2).arg(themes[currThemeId]._tsy2);
            // return false;
        }
		if ((nSolidTable[dPiece[xx][yy]] | dObject[xx][yy]) != 0)
			return false;
	}

	return true;
}

static int TFit_Obj3(int themeId)
{
	int xx, yy, numMatches;
	BYTE tv = themes[themeId]._tsTransVal;
    currThemeId = themeId;
	numMatches = 0;
	for (xx = themes[themeId]._tsx1 + 1; xx < themes[themeId]._tsx2 - 1; xx++) {
		for (yy = themes[themeId]._tsy1 + 1; yy < themes[themeId]._tsy2 - 1; yy++) {
			if (CheckThemeObj3(xx, yy, tv)) {
				drlg.thLocs[numMatches].tpdx = xx;
				drlg.thLocs[numMatches].tpdy = yy;
				drlg.thLocs[numMatches].tpdvar1 = 0;
				drlg.thLocs[numMatches].tpdvar2 = 0;
				numMatches++;
				static_assert(lengthof(drlg.thLocs) >= (10 - 2 - (2 + 2)) * (10 - 2 - (2 + 2)), "TFit_Obj3 skips limit checks assuming enough thLocs entries II.");
				// if (numMatches == lengthof(drlg.thLocs))
				//	goto done;
			}
		}
	}
// done:
	if (numMatches == 0)
		return -1;
	static_assert(lengthof(drlg.thLocs) < 0x7FFF, "TFit_Obj3 uses random_low to select a matching location.");
	return random_low(0, numMatches);
}

static bool SpecialThemeFit(int themeId, int themeType)
{
	bool rv;
	BYTE req;
	int loc;

	switch (themeType) {
	case THEME_BARREL:
	case THEME_MONSTPIT:
		rv = true;
		req = 0;
		break;
	case THEME_SHRINE:
	case THEME_LIBRARY:
		rv = _gbShrineFlag;
		req = 1;
		break;
	case THEME_SKELROOM:
		rv = _gbSkelRoomFlag;
		req = 3;
		break;
	case THEME_BLOODFOUNTAIN:
		rv = _gbBFountainFlag;
		req = 3;
		_gbBFountainFlag = false;
		break;
	case THEME_PURIFYINGFOUNTAIN:
		rv = _gbPFountainFlag;
		req = 3;
		_gbPFountainFlag = false;
		break;
	case THEME_MURKYFOUNTAIN:
		rv = _gbMFountainFlag;
		req = 3;
		_gbMFountainFlag = false;
		break;
	case THEME_TEARFOUNTAIN:
		rv = _gbTFountainFlag;
		req = 3;
		_gbTFountainFlag = false;
		break;
	case THEME_CAULDRON:
		rv = _gbCauldronFlag;
		req = 3;
		_gbCauldronFlag = false;
		break;
	case THEME_GOATSHRINE:
		rv = _gbGoatFlag;
		req = 3;
		break;
	case THEME_WEAPONRACK:
		rv = _gbWeaponFlag;
		req = 2;
		break;
	case THEME_ARMORSTAND:
		rv = _gbArmorFlag;
		req = 2;
		break;
	case THEME_TORTURE:
	case THEME_DECAPITATED:
	case THEME_BRNCROSS:
		rv = true;
		req = 2;
		break;
	case THEME_TREASURE:
		rv = _gbTreasureFlag;
		req = 0;
		_gbTreasureFlag = false;
		break;
	default:
		ASSUME_UNREACHABLE
		rv = false;
		req = 0;
		break;
	}
	loc = -1;
	if (rv) {
		switch (req) {
		case 0:
			loc = 0;
			break;
		case 1:
			loc = TFit_Shrine(themeId);
			break;
		case 2:
			loc = TFit_Obj3(themeId);
			break;
		case 3:
			loc = TFit_Obj5(themeId);
			break;
		default:
			ASSUME_UNREACHABLE
		}
	}
	if (loc < 0)
		return false;
	themes[themeId]._tsType = themeType;
	themes[themeId]._tsObjX = drlg.thLocs[loc].tpdx;
	themes[themeId]._tsObjY = drlg.thLocs[loc].tpdy;
	themes[themeId]._tsObjVar1 = drlg.thLocs[loc].tpdvar1;
	// themes[themeId]._tsObjVar2 = drlg.thLocs[loc].tpdvar2;
	return true;
}

void InitLvlThemes()
{
	numthemes = 0;
	zharlib = -1;
}

void InitThemes()
{
	int i, j, x, y, x1, y1, x2, y2;

	// assert(currLvl._dType != DTYPE_TOWN);
	if (numthemes == 0)
		return;
	// assert(currLvl._dLevelNum < DLV_HELL4 || (currLvl._dDynLvl && currLvl._dLevelNum == DLV_HELL4)); // there are no themes in hellfire (and on diablo-level)
	for (i = 0; i < numthemes; i++) {
		x1 = themes[i]._tsx1;
		y1 = themes[i]._tsy1;
		x2 = themes[i]._tsx2;
		y2 = themes[i]._tsy2;
		// convert to subtile-coordinates
		x1 = DBORDERX + 2 * x1 + 1;
		y1 = DBORDERY + 2 * y1 + 1;
		x2 = DBORDERX + 2 * x2;
		y2 = DBORDERY + 2 * y2;
		themes[i]._tsx1 = x1;
		themes[i]._tsy1 = y1;
		themes[i]._tsx2 = x2;
		themes[i]._tsy2 = y2;
		// select transval
		themes[i]._tsTransVal = dTransVal[x1 + 1][y1 + 1];
		if (themes[i]._tsTransVal == 0) {
			dProgressErr() << QApplication::tr("Invalid theme room @%1:%2 .. %3:%4.").arg(themes[i]._tsx1).arg(themes[i]._tsy1).arg(themes[i]._tsx2).arg(themes[i]._tsy2);
		}
		// protect themes with dFlags
		// v = themes[i]._tsTransVal;
		for (x = x1 - 1; x <= x2; x++) {
			for (y = y1 - 1; y <= y2; y++) {
				// if (dTransVal[x][y] == v) { -- wall?
					dFlags[x][y] |= BFLAG_MON_PROTECT | BFLAG_OBJ_PROTECT;
				// }
			}
		}
	}

	// select theme types
	// TODO: use dType instead
	_gbShrineFlag = currLvl._dDunType != DGT_CAVES && currLvl._dDunType != DGT_HELL;
	_gbSkelRoomFlag = _gbShrineFlag && numSkelTypes != 0;
	_gbGoatFlag = numGoatTypes != 0;
	_gbWeaponFlag = currLvl._dDunType != DGT_CATHEDRAL;
	_gbArmorFlag = currLvl._dDunType != DGT_CATHEDRAL;
	_gbCauldronFlag = currLvl._dDunType == DGT_HELL;
	_gbBFountainFlag = true;
	_gbMFountainFlag = true;
	_gbPFountainFlag = true;
	_gbTFountainFlag = true;
	_gbTreasureFlag = true;

	if (QuestStatus(Q_ZHAR)) {
		for (i = 0; i < numthemes; i++) {
			if (SpecialThemeFit(i, THEME_LIBRARY)) {
				zharlib = i;
				break;
			}
		}
	}
	for (i = 0; i < numthemes; i++) {
		if (i != zharlib) {
			j = ThemeGood[random_(0, lengthof(ThemeGood))];
			while (!SpecialThemeFit(i, j))
				j = random_(0, NUM_THEMES);
		}
	}
}

/*
 * Place a theme object with the specified frequency.
 * @param tv: room id in the dungeon matrix.
 * @param type: the type of the object to place
 * @param rndfrq: the frequency to place the object
 */
typedef struct PlaceObj3QueryParam {
	int rndfrq;
	int type;
} PlaceObj3QueryParam;
static void Place_Obj3_Query(int xx, int yy, void* userParam)
{
	PlaceObj3QueryParam* param = (PlaceObj3QueryParam*)userParam;
	if (CheckThemeObj3(xx, yy, dTransVal[xx][yy]) && random_low(0, param->rndfrq) == 0) {
		AddObject(param->type, xx, yy);
	}
}
static void Place_Obj3(int themeId, BYTE tv, int type, int rndfrq)
{
#ifdef THEMEQUERY
	PlaceObj3QueryParam param;
	param.rndfrq = rndfrq;
	param.type = type;
	QueryTheme(themeId, Place_Obj3_Query, &param);
#else
	int xx, yy;
	// assert(rndfrq > 0);
#ifdef THEMEAREA
	for (xx = themes[themeId]._tsx1; xx <= themes[themeId]._tsx2; xx++) {
		for (yy = themes[themeId]._tsy1; yy <= themes[themeId]._tsy2; yy++) {
#else
	for (xx = DBORDERX + 1; xx < DBORDERX + DSIZEX - 1; xx++) {
		for (yy = DBORDERY + 1; yy < DBORDERY + DSIZEY - 1; yy++) {
#endif
			if (CheckThemeObj3(xx, yy, tv) && random_low(0, rndfrq) == 0) {
				AddObject(type, xx, yy);
			}
		}
	}
#endif
}
/**
 * PlaceThemeMonsts places theme monsters with the specified frequency.
 *
 * @param tv: room id in the dungeon matrix.
 */
typedef struct ThemeMonstsParam {
	int rndfrq;
	int mtype;
} ThemeMonstsParam;
static void Theme_Monsts_Query(int xx, int yy, void* userParam)
{
	ThemeMonstsParam* param = (ThemeMonstsParam*)userParam;
	if (/*dTransVal[xx][yy] == tv &&*/ (nSolidTable[dPiece[xx][yy]] | dItem[xx][yy] | dObject[xx][yy]) == 0) {
		if (random_low(0, param->rndfrq) == 0) {
			AddMonster(param->mtype, xx, yy);
		}
	}
}
static void PlaceThemeMonsts(int themeId, BYTE tv)
{
	int xx, yy;
	int scattertypes[MAX_LVLMTYPES];
	int numscattypes, mtype, i;
	const BYTE monstrnds[4] = { 6, 7, 3, 9 };
	const BYTE rndfrq = monstrnds[currLvl._dDunType - 1]; // TODO: use dType instead?

	numscattypes = 0;
	for (i = 0; i < nummtypes; i++) {
		if (mapMonTypes[i].cmPlaceScatter) {
			scattertypes[numscattypes] = i;
			numscattypes++;
		}
	}
	// assert(numscattypes > 0);
	mtype = scattertypes[random_low(0, numscattypes)];
#ifdef THEMEQUERY
	ThemeMonstsParam param;
	param.rndfrq = rndfrq;
	param.mtype = mtype;
	QueryTheme(themeId, Theme_Monsts_Query, &param);
#else
#ifdef THEMEAREA
	for (xx = themes[themeId]._tsx1; xx <= themes[themeId]._tsx2; xx++) {
		for (yy = themes[themeId]._tsy1; yy <= themes[themeId]._tsy2; yy++) {
#else
	for (xx = DBORDERX; xx < DBORDERX + DSIZEX; xx++) {
		for (yy = DBORDERY; yy < DBORDERY + DSIZEY; yy++) {
#endif
			if (dTransVal[xx][yy] == tv && (nSolidTable[dPiece[xx][yy]] | dItem[xx][yy] | dObject[xx][yy]) == 0) {
				if (random_low(0, rndfrq) == 0) {
					AddMonster(mtype, xx, yy);
				}
			}
		}
	}
#endif
}

/**
 * Theme_Barrel initializes the barrel theme.
 *
 * @param tv: room id in the dungeon matrix.
 */
typedef struct ThemeBarrelParam {
	int barrnd;
} ThemeBarrelParam;
static void Theme_Barrel_Query(int xx, int yy, void* userParam)
{
	ThemeBarrelParam* param = (ThemeBarrelParam*)userParam;
	if (/*dTransVal[xx][yy] == tv &&*/ !nSolidTable[dPiece[xx][yy]]) {
		if (random_low(0, param->barrnd) == 0) {
			int r = random_low(0, param->barrnd) == 0 ? OBJ_BARREL : OBJ_BARRELEX;
			AddObject(r, xx, yy);
		}
	}
}
static void Theme_Barrel(int themeId, BYTE tv)
{
	int r, xx, yy;
	const BYTE barrnds[4] = { 2, 6, 4, 8 };
	const BYTE barrnd = barrnds[currLvl._dDunType - 1];     // TODO: use dType instead?

#ifdef THEMEQUERY
	ThemeBarrelParam param;
	param.barrnd = barrnd;
	QueryTheme(themeId, Theme_Barrel_Query, &param);
#else
#ifdef THEMEAREA
	for (xx = themes[themeId]._tsx1; xx <= themes[themeId]._tsx2; xx++) {
		for (yy = themes[themeId]._tsy1; yy <= themes[themeId]._tsy2; yy++) {
#else
	for (xx = DBORDERX; xx < DBORDERX + DSIZEX; xx++) {
		for (yy = DBORDERY; yy < DBORDERY + DSIZEY; yy++) {
#endif
			if (dTransVal[xx][yy] == tv && !nSolidTable[dPiece[xx][yy]]) {
				if (random_low(0, barrnd) == 0) {
					r = random_low(0, barrnd) == 0 ? OBJ_BARREL : OBJ_BARRELEX;
					AddObject(r, xx, yy);
				}
			}
		}
	}
#endif
	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_Shrine initializes the shrine theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_Shrine(int themeId, BYTE tv)
{
	int xx, yy;

	xx = themes[themeId]._tsObjX;
	yy = themes[themeId]._tsObjY;
	if (themes[themeId]._tsObjVar1 != 0) {
		AddObject(OBJ_CANDLE2, xx - 1, yy);
		AddObject(OBJ_SHRINER, xx, yy);
		AddObject(OBJ_CANDLE2, xx + 1, yy);
	} else {
		AddObject(OBJ_CANDLE2, xx, yy - 1);
		AddObject(OBJ_SHRINEL, xx, yy);
		AddObject(OBJ_CANDLE2, xx, yy + 1);
	}
	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_MonstPit initializes the monster pit theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
typedef struct ThemeMonstPitParam {
	int r;
} ThemeMonstPitParam;
static void Theme_MonstPit_Query(int xx, int yy, void* userParam)
{
	ThemeMonstPitParam* param = (ThemeMonstPitParam*)userParam;
	if (/*dTransVal[xx][yy] == tv &&*/ !nSolidTable[dPiece[xx][yy]] && param->r >= 0 && --param->r < 0) {
		CreateRndItem(xx, yy, CFDQ_GOOD);
	}
}
static void Theme_MonstPit(int themeId, BYTE tv)
{
	int r, xx, yy;

	r = random_(11, (themes[themeId]._tsx2 - themes[themeId]._tsx1) * (themes[themeId]._tsy2 - themes[themeId]._tsy1));
#ifdef THEMEQUERY
	ThemeMonstPitParam param;
	param.r = r;
restart:
	QueryTheme(themeId, Theme_MonstPit_Query, &param);
	if (param.r >= 0)
		goto restart;
#else
	while (true) {
	for (xx = themes[themeId]._tsx1 + 1; xx < themes[themeId]._tsx2; xx++) {
		for (yy = themes[themeId]._tsy1 + 1; yy < themes[themeId]._tsy2; yy++) {
			if (dTransVal[xx][yy] == tv && !nSolidTable[dPiece[xx][yy]] && --r < 0) {
				CreateRndItem(xx, yy, CFDQ_GOOD);
				goto done;
			}
		}
	}
	}
done:
#endif

	PlaceThemeMonsts(themeId, tv);
}

static void AddSkelMonster(int x, int y)
{
	if (!PosOkActor(x, y)) {
		dProgressErr() << QString("AddSkelMonster failed to place monster to %1:%2 room-id:%3").arg(x).arg(y).arg(dTransVal[x][y]);
		return;
	}
	AddMonster(mapSkelTypes[random_low(136, numSkelTypes)], x, y);
}

/**
 * Theme_SkelRoom initializes the skeleton room theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_SkelRoom(int themeId, BYTE tv)
{
	int xx, yy;
	const BYTE monstrnds[4] = { 6, 7, 3, 9 };
	BYTE monstrnd;

	xx = themes[themeId]._tsObjX;
	yy = themes[themeId]._tsObjY;

	AddObject(OBJ_SKFIRE, xx, yy);

	monstrnd = monstrnds[currLvl._dDunType - 1]; // TODO: use dType instead?

	if (random_low(0, monstrnd) != 0) {
		AddSkelMonster(xx - 1, yy - 1);
	} else {
		AddObject(OBJ_BANNERL, xx - 1, yy - 1);
	}

	AddSkelMonster(xx, yy - 1);

	if (random_low(0, monstrnd) != 0) {
		AddSkelMonster(xx + 1, yy - 1);
	} else {
		AddObject(OBJ_BANNERR, xx + 1, yy - 1);
	}
	if (random_low(0, monstrnd) != 0) {
		AddSkelMonster(xx - 1, yy);
	} else {
		AddObject(OBJ_BANNERM, xx - 1, yy);
	}
	if (random_low(0, monstrnd) != 0) {
		AddSkelMonster(xx + 1, yy);
	} else {
		AddObject(OBJ_BANNERM, xx + 1, yy);
	}
	if (random_low(0, monstrnd) != 0) {
		AddSkelMonster(xx - 1, yy + 1);
	} else {
		AddObject(OBJ_BANNERR, xx - 1, yy + 1);
	}

	AddSkelMonster(xx, yy + 1);

	if (random_low(0, monstrnd) != 0) {
		AddSkelMonster(xx + 1, yy + 1);
	} else {
		AddObject(OBJ_BANNERL, xx + 1, yy + 1);
	}

	if ((dObject[xx][yy - 3] == 0 || !objects[dObject[xx][yy - 3] - 1]._oDoorFlag)   // not a door
	 && (nSolidTable[dPiece[xx][yy - 3]] || !nSolidTable[dPiece[xx + 1][yy - 3]])) { // or a single path to NE TODO: allow if !nSolidTable[dPiece[xx - 1][yy - 3]]?
		if (dObject[xx][yy - 2] != 0 || (dObject[xx][yy - 3] != 0 && objects[dObject[xx][yy - 3] - 1]._otype != 0)) {
            extern bool stopgen;
            stopgen = true;
            LogErrorF("object to north-east %d type%d x:%d y:%d vs %d", dObject[xx][yy - 3], objects[dObject[xx][yy - 3]- 1]._otype, xx, themes[themeId]._tsy1, yy - 3);
        }
		// assert(dObject[xx][yy - 2] == 0);
		AddObject(OBJ_BOOK2R, xx, yy - 2);
	} else {
        // if (dObject[xx][yy - 3] == 0 && dObject[xx][yy - 2] == 0 && dPiece[xx][yy - 3] != 11 && dPiece[xx][yy - 3] != 249 && dPiece[xx][yy - 3] != 325 && dPiece[xx + 1][yy - 3] != 270) {
        //if (dObject[xx][yy - 3] == 0 && dObject[xx][yy - 2] == 0 && dPiece[xx][yy - 3] != 553 && dPiece[xx][yy - 3] != 424 /*&& dPiece[xx][yy - 3] != 325 && dPiece[xx + 1][yy - 3] != 270*/) {
        if (dObject[xx][yy - 3] == 0 && dObject[xx][yy - 2] == 0
         && (currLvl._dType != DTYPE_CATHEDRAL || (dPiece[xx][yy - 3] != 11 && dPiece[xx][yy - 3] != 249 && dPiece[xx][yy - 3] != 325 && dPiece[xx + 1][yy - 3] != 270))
         && (currLvl._dType != DTYPE_CATACOMBS || (dPiece[xx][yy - 3] != 553 && dPiece[xx][yy - 3] != 424))) {
            extern bool stopgen;
            stopgen = true;
            LogErrorF("no object to north-east x:%d y:%d vs %d pn%d type %d", xx, themes[themeId]._tsy1, yy - 3, dPiece[xx][yy - 3], automaptype[dPiece[xx][yy - 3]]);
        }
	}
	if ((dObject[xx][yy + 3] == 0 || !objects[dObject[xx][yy + 3] - 1]._oDoorFlag)   // not a door
	 && (nSolidTable[dPiece[xx][yy + 3]] || !nSolidTable[dPiece[xx + 1][yy + 3]])) { // or a single path to SW TODO: allow if !nSolidTable[dPiece[xx - 1][yy + 3]]?
        if (dObject[xx][yy + 2] != 0 || (dObject[xx][yy + 3] != 0 && objects[dObject[xx][yy + 3] - 1]._otype != 0)) {
            extern bool stopgen;
            stopgen = true;
            LogErrorF("object to south-east %d type%d x:%d y:%d vs %d", dObject[xx][yy + 3], objects[dObject[xx][yy + 3] - 1]._otype, xx, themes[themeId]._tsy2, yy + 3);
        }
		// assert(dObject[xx][yy + 2] == 0);
		AddObject(OBJ_BOOK2R, xx, yy + 2);
	} else {
        // if (dObject[xx][yy + 3] == 0 && dObject[xx][yy + 2] == 0 && dPiece[xx][yy + 3] != 11 && dPiece[xx][yy + 3] != 249 && dPiece[xx][yy + 3] != 325 && dPiece[xx + 1][yy + 3] != 270) {
        //if (dObject[xx][yy + 3] == 0 && dObject[xx][yy + 2] == 0 && dPiece[xx][yy + 3] != 553 /*&& dPiece[xx][yy + 3] != 249 && dPiece[xx][yy + 3] != 325 && dPiece[xx + 1][yy + 3] != 270*/) {
        if (dObject[xx][yy + 3] == 0 && dObject[xx][yy + 2] == 0
         && (currLvl._dType != DTYPE_CATHEDRAL || (dPiece[xx][yy + 3] != 11 && dPiece[xx][yy + 3] != 249 && dPiece[xx][yy + 3] != 325 && dPiece[xx + 1][yy + 3] != 270))
         && (currLvl._dType != DTYPE_CATACOMBS || (dPiece[xx][yy + 3] != 553))) {
            extern bool stopgen;
            stopgen = true;
            LogErrorF("no object to south-east x:%d y:%d vs %d pn%d type %d", xx, themes[themeId]._tsy2, yy + 3, dPiece[xx][yy + 3], automaptype[dPiece[xx][yy + 3]]);
        }
	}
}

/**
 * Theme_Treasure initializes the treasure theme.
 *
 * @param tv: theme id in the dungeon matrix.
 */
typedef struct ThemeTreasureParam {
	int treasrnd;
} ThemeTreasureParam;
static void Theme_Treasure_Query(int xx, int yy, void* userParam)
{
	const ThemeTreasureParam* param = (const ThemeTreasureParam*)userParam;
	if (/*dTransVal[xx][yy] == tv &&*/ !nSolidTable[dPiece[xx][yy]]) {
		if (random_low(0, param->treasrnd) == 0) {
			CreateTypeItem(xx, yy, CFDQ_NORMAL, ITYPE_GOLD, IMISC_NONE);
		} else if (random_low(0, param->treasrnd) == 0) {
			CreateRndItem(xx, yy, CFDQ_NORMAL);
		}
	}
}
static void Theme_Treasure(int themeId, BYTE tv)
{
	int xx, yy;
	const BYTE treasrnds[4] = { 6, 9, 7, 10 };
	const BYTE treasrnd = treasrnds[currLvl._dDunType - 1]; // TODO: use dType instead?

#ifdef THEMEQUERY
	ThemeTreasureParam param;
	param.treasrnd = treasrnd;
	QueryTheme(themeId, Theme_Treasure_Query, &param);
#else
#ifdef THEMEAREA
	for (xx = themes[themeId]._tsx1; xx <= themes[themeId]._tsx2; xx++) {
		for (yy = themes[themeId]._tsy1; yy <= themes[themeId]._tsy2; yy++) {
#else
	for (xx = DBORDERX; xx < DBORDERX + DSIZEX; xx++) {
		for (yy = DBORDERY; yy < DBORDERY + DSIZEY; yy++) {
#endif
			if (dTransVal[xx][yy] == tv && !nSolidTable[dPiece[xx][yy]]) {
				if (random_low(0, treasrnd) == 0) {
					CreateTypeItem(xx, yy, CFDQ_NORMAL, ITYPE_GOLD, IMISC_NONE);
				} else if (random_low(0, treasrnd) == 0) {
					CreateRndItem(xx, yy, CFDQ_NORMAL);
				}
			}
		}
	}
#endif
	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_Library initializes the library theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
typedef struct ThemeLibraryParam {
	int librnd;
} ThemeLibraryParam;
static void Theme_Library_Query(int xx, int yy, void* userParam)
{
	const ThemeLibraryParam* param = (const ThemeLibraryParam*)userParam;
	if (CheckThemeObj3(xx, yy, dTransVal[xx][yy]) && dMonster[xx][yy] == 0 && random_low(0, param->librnd) == 0) {
		int oi = AddObject(OBJ_BOOK2L, xx, yy);
		if (random_low(0, 2 * param->librnd) != 0 && oi != -1) { /// BUGFIX: check AddObject succeeded (fixed)
			objects[oi]._oSelFlag = 0;
			objects[oi]._oAnimFrame += 2;
		}
	}
}
static void Theme_Library(int themeId, BYTE tv)
{
	int xx, yy, oi;
	const BYTE librnds[4] = { 1, 2, 0, 0 };
	BYTE librnd;

	xx = themes[themeId]._tsObjX;
	yy = themes[themeId]._tsObjY;
	if (themes[themeId]._tsObjVar1 != 0) {
		AddObject(OBJ_BOOKCANDLE, xx - 1, yy);
		AddObject(OBJ_BOOKCASER, xx, yy);
		AddObject(OBJ_BOOKCANDLE, xx + 1, yy);
	} else {
		AddObject(OBJ_BOOKCANDLE, xx, yy - 1);
		AddObject(OBJ_BOOKCASEL, xx, yy);
		AddObject(OBJ_BOOKCANDLE, xx, yy + 1);
	}
	static_assert(DTYPE_CATHEDRAL == 1 && DTYPE_CATACOMBS == 2, "Theme_Library uses dungeon_type as an array-index.");
	// assert(currLvl._dDunType == 1 /* DTYPE_CATHEDRAL */ || currLvl._dDunType == 2 /* DTYPE_CATACOMBS */);
	librnd = librnds[currLvl._dDunType - 1];     // TODO: use dType instead?
#ifdef THEMEQUERY
	ThemeLibraryParam param;
	param.librnd = librnd;
	QueryTheme(themeId, Theme_Library_Query, &param);
#else
#ifdef THEMEAREA
	for (xx = themes[themeId]._tsx1; xx <= themes[themeId]._tsx2; xx++) {
		for (yy = themes[themeId]._tsy1; yy <= themes[themeId]._tsy2; yy++) {
#else
	for (xx = DBORDERX + 1; xx < DBORDERX + DSIZEX - 1; xx++) {
		for (yy = DBORDERY + 1; yy < DBORDERY + DSIZEY - 1; yy++) {
#endif
			if (CheckThemeObj3(xx, yy, tv) && dMonster[xx][yy] == 0 && random_low(0, librnd) == 0) {
				oi = AddObject(OBJ_BOOK2L, xx, yy);
				if (random_low(0, 2 * librnd) != 0 && oi != -1) { /// BUGFIX: check AddObject succeeded (fixed)
					objects[oi]._oSelFlag = 0;
					objects[oi]._oAnimFrame += 2;
				}
			}
		}
	}
#endif
	if (/*QuestStatus(Q_ZHAR) &&*/ themeId == zharlib)
		return;

	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_Torture initializes the torture theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_Torture(int themeId, BYTE tv)
{
	const BYTE tortrnds[4] = { 6 * 2, 8 * 2, 3 * 2, 8 * 2 };
	const BYTE tortrnd = tortrnds[currLvl._dDunType - 1];   // TODO: use dType instead?

	AddObject(random_(46, 2) ? OBJ_TNUDEW : OBJ_TNUDEM, themes[themeId]._tsObjX, themes[themeId]._tsObjY);
	Place_Obj3(themeId, tv, OBJ_TNUDEM, tortrnd);
	Place_Obj3(themeId, tv, OBJ_TNUDEW, tortrnd);
	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_BloodFountain initializes the blood fountain theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_BloodFountain(int themeId, BYTE tv)
{
	AddObject(OBJ_BLOODFTN, themes[themeId]._tsObjX, themes[themeId]._tsObjY);
	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_Decap initializes the decapitated theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_Decap(int themeId, BYTE tv)
{
	const BYTE decaprnds[4] = { 6, 8, 3, 8 };
	const BYTE decaprnd = decaprnds[currLvl._dDunType - 1]; // TODO: use dType instead?

	AddObject(OBJ_DECAP, themes[themeId]._tsObjX, themes[themeId]._tsObjY);
	Place_Obj3(themeId, tv, OBJ_DECAP, decaprnd);
	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_PurifyingFountain initializes the purifying fountain theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_PurifyingFountain(int themeId, BYTE tv)
{
	AddObject(OBJ_PURIFYINGFTN, themes[themeId]._tsObjX, themes[themeId]._tsObjY);
	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_ArmorStand initializes the armor stand theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_ArmorStand(int themeId, BYTE tv)
{
	const BYTE armorrnds[4] = { 6, 8, 3, 8 };
	const BYTE armorrnd = armorrnds[currLvl._dDunType - 1]; // TODO: use dType instead?

	AddObject(_gbArmorFlag ? OBJ_ARMORSTAND : OBJ_ARMORSTANDN, themes[themeId]._tsObjX, themes[themeId]._tsObjY);
	_gbArmorFlag = false;
	Place_Obj3(themeId, tv, OBJ_ARMORSTANDN, armorrnd);
	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_GoatShrine initializes the goat shrine theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_GoatShrine(int themeId, BYTE tv)
{
	int i, xx, yy, x, y;

	xx = themes[themeId]._tsObjX;
	yy = themes[themeId]._tsObjY;
	AddObject(OBJ_GOATSHRINE, xx, yy);
	for (i = 0; i < lengthof(offset_x); i++) {
		x = xx + offset_x[i];
		y = yy + offset_y[i];
        if (dTransVal[x][y] != tv) {
            dProgressErr() << QString("Theme_GoatShrine failed to check tv-mismatch at %1:%2. Room: %3:%4;%5:%6.").arg(x).arg(y).arg(themes[themeId]._tsx1).arg(themes[themeId]._tsy1).arg(themes[themeId]._tsx2).arg(themes[themeId]._tsy2);
        }
        if (nSolidTable[dPiece[x][y]]) {
            dProgressErr() << QString("Theme_GoatShrine has a solid subtile at %1:%2. Room: %3:%4;%5:%6.").arg(x).arg(y).arg(themes[themeId]._tsx1).arg(themes[themeId]._tsy1).arg(themes[themeId]._tsx2).arg(themes[themeId]._tsy2);
        }
		// assert(dTransVal[x][y] == tv && !nSolidTable[dPiece[x][y]]);
		AddMonster(mapGoatTypes[0], x, y); // OPPOSITE(i)
	}
}

/**
 * Theme_Cauldron initializes the cauldron theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_Cauldron(int themeId, BYTE tv)
{
	AddObject(OBJ_CAULDRON, themes[themeId]._tsObjX, themes[themeId]._tsObjY);
	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_MurkyFountain initializes the murky fountain theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_MurkyFountain(int themeId, BYTE tv)
{
	AddObject(OBJ_MURKYFTN, themes[themeId]._tsObjX, themes[themeId]._tsObjY);
	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_TearFountain initializes the tear fountain theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_TearFountain(int themeId, BYTE tv)
{
	AddObject(OBJ_TEARFTN, themes[themeId]._tsObjX, themes[themeId]._tsObjY);
	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_BrnCross initializes the burning cross theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_BrnCross(int themeId, BYTE tv)
{
	const BYTE bcrossrnds[4] = { 5, 7, 3, 8 };
	const BYTE bcrossrnd = bcrossrnds[currLvl._dDunType - 1]; // TODO: use dType instead?

	AddObject(OBJ_TBCROSS, themes[themeId]._tsObjX, themes[themeId]._tsObjY);
	Place_Obj3(themeId, tv, OBJ_TBCROSS, bcrossrnd);
	PlaceThemeMonsts(themeId, tv);
}

/**
 * Theme_WeaponRack initializes the weapon rack theme.
 *
 * @param themeId: theme id.
 * @param tv: room id in the dungeon matrix.
 */
static void Theme_WeaponRack(int themeId, BYTE tv)
{
	int type;
	const BYTE weaponrnds[4] = { 6, 8, 5, 8 };
	const BYTE weaponrnd = weaponrnds[currLvl._dDunType - 1]; // TODO: use dType instead?

	static_assert(OBJ_WEAPONRACKL + 2 == OBJ_WEAPONRACKR, "Theme_WeaponRack depends on the order of WEAPONRACKL/R");
	type = OBJ_WEAPONRACKL + 2 * random_(0, 2);
	static_assert(OBJ_WEAPONRACKL + 1 == OBJ_WEAPONRACKLN, "Theme_WeaponRack depends on the order of WEAPONRACKL(N)");
	static_assert(OBJ_WEAPONRACKR + 1 == OBJ_WEAPONRACKRN, "Theme_WeaponRack depends on the order of WEAPONRACKR(N)");
	AddObject(type + (_gbWeaponFlag ? 0 : 1), themes[themeId]._tsObjX, themes[themeId]._tsObjY);
	_gbWeaponFlag = false;
	type += 1;
	Place_Obj3(themeId, tv, type, weaponrnd);
	PlaceThemeMonsts(themeId, tv);
}

/**
 * UpdateL4Trans sets each value of the transparency map to 1.
 */
/*static void UpdateL4Trans()
{
	int i;
	BYTE *pTmp;

	static_assert(sizeof(dTransVal) == MAXDUNX * MAXDUNY, "Linear traverse of dTransVal does not work in UpdateL4Trans.");
	pTmp = &dTransVal[0][0];
	for (i = 0; i < MAXDUNX * MAXDUNY; i++, pTmp++)
		if (*pTmp != 0)
			*pTmp = 1;
}*/

void CreateThemeRooms()
{
	int i;
	BYTE tv;
	// assert(currLvl._dLevelNum < DLV_HELL4 || (currLvl._dDynLvl && currLvl._dLevelNum == DLV_HELL4) || numthemes == 0); // there are no themes in hellfire (and on diablo-level)
	//gbInitObjFlag = true;
    for (i = 0; i < numthemes; i++) {
        tv = themes[i]._tsTransVal;
        dProgress() << QString("Themeroom %1: %2:%3;%4:%5 type%6.").arg(i).arg(themes[i]._tsx1).arg(themes[i]._tsy1).arg(themes[i]._tsx2).arg(themes[i]._tsy2).arg(themes[i]._tsType);
        for (int xx = themes[i]._tsx1; xx < themes[i]._tsx2; xx++) {
            for (int yy = themes[i]._tsy1; yy < themes[i]._tsy2; yy++) {
                if (dTransVal[xx][yy] != tv && !nSolidTable[dPiece[xx][yy]]) {
                    dProgressErr() << QString("Themeroom %1 has a non-tv subtile at %2:%3. Room: %4:%5;%6:%7.").arg(i).arg(xx).arg(yy).arg(themes[i]._tsx1).arg(themes[i]._tsy1).arg(themes[i]._tsx2).arg(themes[i]._tsy2);
                }
                /*if ((xx == themes[i]._tsx1 || xx == themes[i]._tsx2 || yy == themes[i]._tsy1 || yy == themes[i]._tsy2)
                 && !nSolidTable[dPiece[xx][yy]]) {
                }*/
            }
        }
    }
    extern QElapsedTimer* timer;
    extern quint64 dt[16];
    dt[0] -= timer->nsecsElapsed();
	for (i = 0; i < numthemes; i++) {
        currThemeId = i;
		tv = themes[i]._tsTransVal;
		switch (themes[i]._tsType) {
		case THEME_BARREL:
			Theme_Barrel(i, tv);
			break;
		case THEME_SHRINE:
			Theme_Shrine(i, tv);
			break;
		case THEME_MONSTPIT:
			Theme_MonstPit(i, tv);
			break;
		case THEME_SKELROOM:
			Theme_SkelRoom(i, tv);
			break;
		case THEME_TREASURE:
			Theme_Treasure(i, tv);
			break;
		case THEME_LIBRARY:
			Theme_Library(i, tv);
			break;
		case THEME_TORTURE:
			Theme_Torture(i, tv);
			break;
		case THEME_BLOODFOUNTAIN:
			Theme_BloodFountain(i, tv);
			break;
		case THEME_DECAPITATED:
			Theme_Decap(i, tv);
			break;
		case THEME_PURIFYINGFOUNTAIN:
			Theme_PurifyingFountain(i, tv);
			break;
		case THEME_ARMORSTAND:
			Theme_ArmorStand(i, tv);
			break;
		case THEME_GOATSHRINE:
			Theme_GoatShrine(i, tv);
			break;
		case THEME_CAULDRON:
			Theme_Cauldron(i, tv);
			break;
		case THEME_MURKYFOUNTAIN:
			Theme_MurkyFountain(i, tv);
			break;
		case THEME_TEARFOUNTAIN:
			Theme_TearFountain(i, tv);
			break;
		case THEME_BRNCROSS:
			Theme_BrnCross(i, tv);
			break;
		case THEME_WEAPONRACK:
			Theme_WeaponRack(i, tv);
			break;
		default:
			ASSUME_UNREACHABLE
			break;
		}
	}
    dt[0] += timer->nsecsElapsed();
    /*dt[1] -= timer->nsecsElapsed();
    dt[1] += timer->nsecsElapsed();
    dt[2] -= timer->nsecsElapsed();
    dt[2] += timer->nsecsElapsed();
    dt[3] -= timer->nsecsElapsed();
    dt[3] += timer->nsecsElapsed();*/
	//gbInitObjFlag = false;
	// TODO: why was this necessary in the vanilla code?
	//if (currLvl._dType == DTYPE_HELL && numthemes > 0) {
	//	UpdateL4Trans();
	//}
}
