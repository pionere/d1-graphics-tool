/**
 * @file questdat.cpp
 *
 * Implementation of all quest and level data.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

const LevelData AllLevels[NUM_LEVELS] = {
	// clang-format off
					// dLevel, dSetLvl, dType,           dDunType,        dMusic,      dMicroTileLen, dBlocks, dLevelName,
/*DLV_TOWN*/		{       0, FALSE,   DTYPE_TOWN,      DTYPE_TOWN,      TMUSIC_TOWN,            16,      16, "Tristram",
					//  dAutomapData,             dSolidTable,                   dMicroFlags,                   dMicroCels,                    dMegaTiles,                    dMiniTiles,                    dSpecCels,                     dPalName,                        dLoadCels,               dLoadPal,                dLoadBarOnTop, dLoadBarColor, dMonDensity, dObjDensity, dSetLvlDunX,   dSetLvlDunY,              dSetLvlWarp,  dSetLvlPiece,
#ifdef HELLFIRE
						NULL,                     "NLevels\\TownData\\Town.SOL", "NLevels\\TownData\\Town.TMI", "NLevels\\TownData\\Town.CEL", "NLevels\\TownData\\Town.TIL", "NLevels\\TownData\\Town.MIN", "Levels\\TownData\\TownS.CEL", "Levels\\TownData\\Town.pal",    "Gendata\\Cuttt.CEL",    "Gendata\\Cuttt.pal",            FALSE,            43, 0,           0,           0,             0,                        WRPT_NONE,    SPT_NONE,
#else
						NULL,                     "Levels\\TownData\\Town.SOL",  "Levels\\TownData\\Town.TMI",  "Levels\\TownData\\Town.CEL",  "Levels\\TownData\\Town.TIL",  "Levels\\TownData\\Town.MIN",  "Levels\\TownData\\TownS.CEL", "Levels\\TownData\\Town.pal",    "Gendata\\Cuttt.CEL",    "Gendata\\Cuttt.pal",            FALSE,            43, 0,           0,           0,             0,                        WRPT_NONE,    SPT_NONE,
#endif
					//  dMonTypes,
						{ MT_INVALID }, ALIGN32
					},
/*DLV_CATHEDRAL1*/	{       2, FALSE,   DTYPE_CATHEDRAL, DTYPE_CATHEDRAL, TMUSIC_L1,              10,      10, "Cathedral 1",
						"Levels\\L1Data\\L1.AMP", "Levels\\L1Data\\L1.SOL",      "Levels\\L1Data\\L1.TMI",      "Levels\\L1Data\\L1.CEL",      "Levels\\L1Data\\L1.TIL",      "Levels\\L1Data\\L1.MIN",      "Levels\\L1Data\\L1S.CEL",     "Levels\\L1Data\\L1_%d.PAL",     "Gendata\\Cutl1d.CEL",   "Gendata\\Cutl1d.pal",            TRUE,           138, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_NZOMBIE, MT_RFALLSP, MT_WSKELAX, MT_RFALLSD, MT_NSCAV, MT_WSKELSD, MT_INVALID }, ALIGN32
					},
/*DLV_CATHEDRAL2*/	{       4, FALSE,   DTYPE_CATHEDRAL, DTYPE_CATHEDRAL, TMUSIC_L1,              10,      10, "Cathedral 2",
						"Levels\\L1Data\\L1.AMP", "Levels\\L1Data\\L1.SOL",      "Levels\\L1Data\\L1.TMI",      "Levels\\L1Data\\L1.CEL",      "Levels\\L1Data\\L1.TIL",      "Levels\\L1Data\\L1.MIN",      "Levels\\L1Data\\L1S.CEL",     "Levels\\L1Data\\L1_%d.PAL",     "Gendata\\Cutl1d.CEL",   "Gendata\\Cutl1d.pal",            TRUE,           138, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_NZOMBIE, MT_RFALLSP, MT_WSKELAX, MT_RFALLSD, MT_NSCAV, MT_WSKELSD, MT_BZOMBIE, MT_GZOMBIE,  MT_DFALLSP, MT_YFALLSP, MT_TSKELAX, MT_RSKELAX, MT_DFALLSD, MT_YFALLSD, MT_WSKELBW, MT_TSKELBW, MT_BSCAV, MT_TSKELSD, MT_NSNEAK, MT_RBAT, MT_INVALID }, ALIGN32
					},
/*DLV_CATHEDRAL3*/	{       6, FALSE,   DTYPE_CATHEDRAL, DTYPE_CATHEDRAL, TMUSIC_L1,              10,      10, "Cathedral 3",
						"Levels\\L1Data\\L1.AMP", "Levels\\L1Data\\L1.SOL",      "Levels\\L1Data\\L1.TMI",      "Levels\\L1Data\\L1.CEL",      "Levels\\L1Data\\L1.TIL",      "Levels\\L1Data\\L1.MIN",      "Levels\\L1Data\\L1S.CEL",     "Levels\\L1Data\\L1_%d.PAL",     "Gendata\\Cutl1d.CEL",   "Gendata\\Cutl1d.pal",            TRUE,           138, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_NSCAV, MT_WSKELSD, MT_BZOMBIE, MT_GZOMBIE, MT_DFALLSP, MT_YFALLSP, MT_TSKELAX, MT_RSKELAX, MT_DFALLSD, MT_YFALLSD, MT_WSKELBW, MT_TSKELBW, MT_BSCAV, MT_TSKELSD, MT_NSNEAK, MT_RBAT, MT_NBAT, MT_YZOMBIE, MT_BFALLSP, MT_XSKELAX, MT_BFALLSD, MT_WSCAV, MT_RSKELBW, MT_RSKELSD, MT_INVALID }, ALIGN32
					},
/*DLV_CATHEDRAL4*/	{       8, FALSE,   DTYPE_CATHEDRAL, DTYPE_CATHEDRAL, TMUSIC_L1,              10,      10, "Cathedral 4",
						"Levels\\L1Data\\L1.AMP", "Levels\\L1Data\\L1.SOL",      "Levels\\L1Data\\L1.TMI",      "Levels\\L1Data\\L1.CEL",      "Levels\\L1Data\\L1.TIL",      "Levels\\L1Data\\L1.MIN",      "Levels\\L1Data\\L1S.CEL",     "Levels\\L1Data\\L1_%d.PAL",     "Gendata\\Cutl1d.CEL",   "Gendata\\Cutl1d.pal",            TRUE,           138, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_GZOMBIE, MT_YFALLSP, MT_RSKELAX, MT_YFALLSD, MT_TSKELBW, MT_BSCAV, MT_TSKELSD, MT_NSNEAK, MT_NBAT, MT_YZOMBIE, MT_BFALLSP, MT_XSKELAX, MT_BFALLSD, MT_WSCAV, MT_RSKELBW, MT_RSKELSD, MT_GBAT, MT_YSCAV, MT_XSKELBW, MT_XSKELSD, MT_NGOATMC, MT_NGOATBW, MT_INVALID }, ALIGN32
					},
/*DLV_CATACOMBS1*/	{      10, FALSE,   DTYPE_CATACOMBS, DTYPE_CATACOMBS, TMUSIC_L2,              10,      10, "Catacombs 1",
						"Levels\\L2Data\\L2.AMP", "Levels\\L2Data\\L2.SOL",      "Levels\\L2Data\\L2.TMI",      "Levels\\L2Data\\L2.CEL",      "Levels\\L2Data\\L2.TIL",      "Levels\\L2Data\\L2.MIN",      "Levels\\L2Data\\L2S.CEL",     "Levels\\L2Data\\L2_%d.PAL",     "Gendata\\Cut2.CEL",     "Gendata\\Cut2.pal",              TRUE,           254, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_NSNEAK, MT_NBAT, MT_YZOMBIE, MT_BFALLSP, MT_XSKELAX, MT_BFALLSD, MT_WSCAV, MT_RSKELBW, MT_RSKELSD, MT_GBAT, MT_YSCAV, MT_XSKELBW, MT_XSKELSD, MT_NGOATMC, MT_NGOATBW, MT_BGOATBW, MT_BGOATMC, MT_RSNEAK, MT_NFAT, MT_NGARG, MT_INVALID }, ALIGN32
					},
/*DLV_CATACOMBS2*/	{      12, FALSE,   DTYPE_CATACOMBS, DTYPE_CATACOMBS, TMUSIC_L2,              10,      10, "Catacombs 2",
						"Levels\\L2Data\\L2.AMP", "Levels\\L2Data\\L2.SOL",      "Levels\\L2Data\\L2.TMI",      "Levels\\L2Data\\L2.CEL",      "Levels\\L2Data\\L2.TIL",      "Levels\\L2Data\\L2.MIN",      "Levels\\L2Data\\L2S.CEL",     "Levels\\L2Data\\L2_%d.PAL",     "Gendata\\Cut2.CEL",     "Gendata\\Cut2.pal",              TRUE,           254, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_GBAT, MT_YSCAV, MT_XSKELBW, MT_XSKELSD, MT_NGOATMC, MT_NGOATBW, MT_BGOATBW, MT_BGOATMC, MT_RSNEAK, MT_NFAT, MT_NGARG, MT_BSNEAK, MT_RGOATMC, MT_XBAT, MT_RGOATBW, MT_NACID, MT_INVALID }, ALIGN32
					},
/*DLV_CATACOMBS3*/	{      14, FALSE,   DTYPE_CATACOMBS, DTYPE_CATACOMBS, TMUSIC_L2,              10,      10, "Catacombs 3",
						"Levels\\L2Data\\L2.AMP", "Levels\\L2Data\\L2.SOL",      "Levels\\L2Data\\L2.TMI",      "Levels\\L2Data\\L2.CEL",      "Levels\\L2Data\\L2.TIL",      "Levels\\L2Data\\L2.MIN",      "Levels\\L2Data\\L2S.CEL",     "Levels\\L2Data\\L2_%d.PAL",     "Gendata\\Cut2.CEL",     "Gendata\\Cut2.pal",              TRUE,           254, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_BGOATBW, MT_BGOATMC, MT_RSNEAK, MT_NFAT, MT_NGARG, MT_BSNEAK, MT_RGOATMC, MT_XBAT, MT_RGOATBW, MT_NACID, MT_GGOATBW, MT_GGOATMC, MT_BFAT, MT_NRHINO, MT_XGARG, MT_INVALID }, ALIGN32
					},
/*DLV_CATACOMBS4*/	{      16, FALSE,   DTYPE_CATACOMBS, DTYPE_CATACOMBS, TMUSIC_L2,              10,      10, "Catacombs 4",
						"Levels\\L2Data\\L2.AMP", "Levels\\L2Data\\L2.SOL",      "Levels\\L2Data\\L2.TMI",      "Levels\\L2Data\\L2.CEL",      "Levels\\L2Data\\L2.TIL",      "Levels\\L2Data\\L2.MIN",      "Levels\\L2Data\\L2S.CEL",     "Levels\\L2Data\\L2_%d.PAL",     "Gendata\\Cut2.CEL",     "Gendata\\Cut2.pal",              TRUE,           254, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_BSNEAK, MT_RGOATMC, MT_XBAT, MT_RGOATBW, MT_NACID, MT_GGOATBW, MT_GGOATMC, MT_BFAT, MT_NRHINO, MT_XGARG, MT_XRHINO, MT_YSNEAK, MT_RACID, MT_XFAT, MT_NMAGMA, MT_YMAGMA, MT_INVALID }, ALIGN32
					},
/*DLV_CAVES1*/		{      18, FALSE,   DTYPE_CAVES,     DTYPE_CAVES,     TMUSIC_L3,              10,      10, "Caves 1",
						"Levels\\L3Data\\L3.AMP", "Levels\\L3Data\\L3.SOL",      "Levels\\L3Data\\L3.TMI",      "Levels\\L3Data\\L3.CEL",      "Levels\\L3Data\\L3.TIL",      "Levels\\L3Data\\L3.MIN",      "Levels\\L1Data\\L1S.CEL",     "Levels\\L3Data\\L3_%d.PAL",     "Gendata\\Cut3.CEL",     "Gendata\\Cut3.pal",             FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_GGOATBW, MT_GGOATMC, MT_BFAT, MT_NRHINO, MT_XGARG, MT_XRHINO, MT_YSNEAK, MT_RACID, MT_XFAT, MT_NMAGMA, MT_YMAGMA, MT_DGARG, MT_BMAGMA, MT_WMAGMA, MT_BRHINO, MT_RTHIN, MT_INVALID }, ALIGN32
					},
/*DLV_CAVES2*/		{      20, FALSE,   DTYPE_CAVES,     DTYPE_CAVES,     TMUSIC_L3,              10,      10, "Caves 2",
						"Levels\\L3Data\\L3.AMP", "Levels\\L3Data\\L3.SOL",      "Levels\\L3Data\\L3.TMI",      "Levels\\L3Data\\L3.CEL",      "Levels\\L3Data\\L3.TIL",      "Levels\\L3Data\\L3.MIN",      "Levels\\L1Data\\L1S.CEL",     "Levels\\L3Data\\L3_%d.PAL",     "Gendata\\Cut3.CEL",     "Gendata\\Cut3.pal",             FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_XRHINO, MT_YSNEAK, MT_RACID, MT_XFAT, MT_YMAGMA, MT_DGARG, MT_BMAGMA, MT_WMAGMA, MT_BRHINO, MT_RTHIN, MT_DRHINO, MT_BACID, MT_RFAT, MT_NTHIN, MT_BGARG, MT_NMEGA, MT_INVALID }, ALIGN32
					},
/*DLV_CAVES3*/		{      22, FALSE,   DTYPE_CAVES,     DTYPE_CAVES,     TMUSIC_L3,              10,      10, "Caves 3",
						"Levels\\L3Data\\L3.AMP", "Levels\\L3Data\\L3.SOL",      "Levels\\L3Data\\L3.TMI",      "Levels\\L3Data\\L3.CEL",      "Levels\\L3Data\\L3.TIL",      "Levels\\L3Data\\L3.MIN",      "Levels\\L1Data\\L1S.CEL",     "Levels\\L3Data\\L3_%d.PAL",     "Gendata\\Cut3.CEL",     "Gendata\\Cut3.pal",             FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_DGARG, MT_BMAGMA, MT_WMAGMA, MT_BRHINO, MT_RTHIN, MT_DRHINO, MT_BACID, MT_RFAT, MT_NTHIN, MT_BGARG, MT_NMEGA, MT_DMEGA, MT_NSNAKE, MT_XTHIN, MT_INVALID }, ALIGN32
					},
/*DLV_CAVES4*/		{      24, FALSE,   DTYPE_CAVES,     DTYPE_CAVES,     TMUSIC_L3,              10,      10, "Caves 4",
						"Levels\\L3Data\\L3.AMP", "Levels\\L3Data\\L3.SOL",      "Levels\\L3Data\\L3.TMI",      "Levels\\L3Data\\L3.CEL",      "Levels\\L3Data\\L3.TIL",      "Levels\\L3Data\\L3.MIN",      "Levels\\L1Data\\L1S.CEL",     "Levels\\L3Data\\L3_%d.PAL",     "Gendata\\Cut3.CEL",     "Gendata\\Cut3.pal",             FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_DRHINO, MT_BACID, MT_RFAT, MT_NTHIN, MT_BGARG, MT_NMEGA, MT_DMEGA, MT_NSNAKE, MT_XTHIN, MT_RSNAKE, MT_NBLACK, MT_NSUCC, MT_BMEGA, MT_XACID, MT_GTHIN, MT_INVALID }, ALIGN32
					},
/*DLV_HELL1*/		{      26, FALSE,   DTYPE_HELL,      DTYPE_HELL,      TMUSIC_L4,              12,      16, "Hell 1",
						"Levels\\L4Data\\L4.AMP", "Levels\\L4Data\\L4.SOL",      "Levels\\L4Data\\L4.TMI",      "Levels\\L4Data\\L4.CEL",      "Levels\\L4Data\\L4.TIL",      "Levels\\L4Data\\L4.MIN",      "Levels\\L2Data\\L2S.CEL",     "Levels\\L4Data\\L4_%d.PAL",     "Gendata\\Cut4.CEL",     "Gendata\\Cut4.pal",             FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_DMEGA, MT_NSNAKE, MT_XTHIN, MT_RSNAKE, MT_NBLACK, MT_NSUCC, MT_BMEGA, MT_XACID, MT_GTHIN, MT_RMEGA, MT_BSNAKE, MT_RBLACK, MT_GBLACK, MT_GSUCC, MT_NMAGE, MT_INVALID }, ALIGN32
					},
/*DLV_HELL2*/		{      28, FALSE,   DTYPE_HELL,      DTYPE_HELL,      TMUSIC_L4,              12,      16, "Hell 2",
						"Levels\\L4Data\\L4.AMP", "Levels\\L4Data\\L4.SOL",      "Levels\\L4Data\\L4.TMI",      "Levels\\L4Data\\L4.CEL",      "Levels\\L4Data\\L4.TIL",      "Levels\\L4Data\\L4.MIN",      "Levels\\L2Data\\L2S.CEL",     "Levels\\L4Data\\L4_%d.PAL",     "Gendata\\Cut4.CEL",     "Gendata\\Cut4.pal",             FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_RSNAKE, MT_NBLACK, MT_NSUCC, MT_BMEGA, MT_XACID, MT_GTHIN, MT_RMEGA, MT_BSNAKE, MT_RBLACK, MT_GBLACK, MT_GSUCC, MT_NMAGE, MT_GMAGE, MT_BBLACK, MT_RSUCC, MT_INVALID }, ALIGN32
					},
/*DLV_HELL3*/		{      30, FALSE,   DTYPE_HELL,      DTYPE_HELL,      TMUSIC_L4,              12,      16, "Hell 3",
						"Levels\\L4Data\\L4.AMP", "Levels\\L4Data\\L4.SOL",      "Levels\\L4Data\\L4.TMI",      "Levels\\L4Data\\L4.CEL",      "Levels\\L4Data\\L4.TIL",      "Levels\\L4Data\\L4.MIN",      "Levels\\L2Data\\L2S.CEL",     "Levels\\L4Data\\L4_%d.PAL",     "Gendata\\Cut4.CEL",     "Gendata\\Cut4.pal",             FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_RMEGA, MT_RBLACK, MT_GSUCC, MT_GMAGE, MT_BBLACK, MT_RSUCC, MT_GSNAKE, MT_BSUCC, MT_XMAGE, MT_INVALID }, ALIGN32
					},
/*DLV_HELL4*/		{      32, FALSE,   DTYPE_HELL,      DTYPE_HELL,      TMUSIC_L4,              12,      16, "Diablo",
						"Levels\\L4Data\\L4.AMP", "Levels\\L4Data\\L4.SOL",      "Levels\\L4Data\\L4.TMI",      "Levels\\L4Data\\L4.CEL",      "Levels\\L4Data\\L4.TIL",      "Levels\\L4Data\\L4.MIN",      "Levels\\L2Data\\L2S.CEL",     "Levels\\L4Data\\L4_%d.PAL",     "Gendata\\Cutgate.CEL",  "Gendata\\Cutgate.pal",          FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_INVALID }, /* MT_GBLACK, MT_BMAGE, MT_NBLACK, MT_DIABLO */ ALIGN32
					},
#ifdef HELLFIRE
/*DLV_NEST1*/		{      18, FALSE,   DTYPE_NEST,      DTYPE_CAVES,     TMUSIC_L6,              10,      10, "Nest 1",
						"NLevels\\L6Data\\L6.AMP", "NLevels\\L6Data\\L6.SOL",    "NLevels\\L6Data\\L6.TMI",     "NLevels\\L6Data\\L6.CEL",     "NLevels\\L6Data\\L6.TIL",     "NLevels\\L6Data\\L6.MIN",     "Levels\\L1Data\\L1S.CEL",     "NLevels\\L6Data\\L6Base%d.PAL", "Nlevels\\Cutl6.CEL",    "Nlevels\\Cutl6.pal",            FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_HELLBOAR, MT_STINGER, MT_PSYCHORB, MT_ARACHNON, MT_FELLTWIN, MT_UNRAV, MT_INVALID }, ALIGN32
					},
/*DLV_NEST2*/		{      20, FALSE,   DTYPE_NEST,      DTYPE_CAVES,     TMUSIC_L6,              10,      10, "Nest 2",
						"NLevels\\L6Data\\L6.AMP", "NLevels\\L6Data\\L6.SOL",    "NLevels\\L6Data\\L6.TMI",     "NLevels\\L6Data\\L6.CEL",     "NLevels\\L6Data\\L6.TIL",     "NLevels\\L6Data\\L6.MIN",     "Levels\\L1Data\\L1S.CEL",     "NLevels\\L6Data\\L6Base%d.PAL", "Nlevels\\Cutl6.CEL",    "Nlevels\\Cutl6.pal",            FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_HELLBOAR, MT_STINGER, MT_PSYCHORB, MT_ARACHNON, MT_FELLTWIN, MT_UNRAV, MT_INVALID }, /* MT_HORKSPWN */ ALIGN32
					},
/*DLV_NEST3*/		{      22, FALSE,   DTYPE_NEST,      DTYPE_CAVES,     TMUSIC_L6,              10,      10, "Nest 3",
						"NLevels\\L6Data\\L6.AMP", "NLevels\\L6Data\\L6.SOL",    "NLevels\\L6Data\\L6.TMI",     "NLevels\\L6Data\\L6.CEL",     "NLevels\\L6Data\\L6.TIL",     "NLevels\\L6Data\\L6.MIN",     "Levels\\L1Data\\L1S.CEL",     "NLevels\\L6Data\\L6Base%d.PAL", "Nlevels\\Cutl6.CEL",    "Nlevels\\Cutl6.pal",            FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_VENMTAIL, MT_NECRMORB, MT_SPIDLORD, MT_LASHWORM, MT_TORCHANT, MT_INVALID }, /* MT_HORKSPWN, MT_HORKDMN */ ALIGN32
					},
/*DLV_NEST4*/		{      24, FALSE,   DTYPE_NEST,      DTYPE_CAVES,     TMUSIC_L6,              10,      10, "Nest 4",
						"NLevels\\L6Data\\L6.AMP", "NLevels\\L6Data\\L6.SOL",    "NLevels\\L6Data\\L6.TMI",     "NLevels\\L6Data\\L6.CEL",     "NLevels\\L6Data\\L6.TIL",     "NLevels\\L6Data\\L6.MIN",     "Levels\\L1Data\\L1S.CEL",     "NLevels\\L6Data\\L6Base%d.PAL", "Nlevels\\Cutl6.CEL",    "Nlevels\\Cutl6.pal",            FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_VENMTAIL, MT_NECRMORB, MT_SPIDLORD, MT_LASHWORM, MT_TORCHANT, MT_INVALID }, /* MT_DEFILER */ ALIGN32
					},
/*DLV_CRYPT1*/		{      26, FALSE,   DTYPE_CRYPT,     DTYPE_CATHEDRAL, TMUSIC_L5,              10,      10, "Crypt 1",
						"NLevels\\L5Data\\L5.AMP", "NLevels\\L5Data\\L5.SOL",    "NLevels\\L5Data\\L5.TMI",     "NLevels\\L5Data\\L5.CEL",     "NLevels\\L5Data\\L5.TIL",     "NLevels\\L5Data\\L5.MIN",     "NLevels\\L5Data\\L5S.CEL",    "NLevels\\L5Data\\L5Base.PAL",   "Nlevels\\Cutl5.CEL",    "Nlevels\\Cutl5.pal",            FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_LRDSAYTR, MT_GRAVEDIG, MT_BIGFALL, MT_TOMBRAT, MT_FIREBAT, MT_LICH, MT_INVALID }, ALIGN32
					},
/*DLV_CRYPT2*/		{      28, FALSE,   DTYPE_CRYPT,     DTYPE_CATHEDRAL, TMUSIC_L5,              10,      10, "Crypt 2",
						"NLevels\\L5Data\\L5.AMP", "NLevels\\L5Data\\L5.SOL",    "NLevels\\L5Data\\L5.TMI",     "NLevels\\L5Data\\L5.CEL",     "NLevels\\L5Data\\L5.TIL",     "NLevels\\L5Data\\L5.MIN",     "NLevels\\L5Data\\L5S.CEL",    "NLevels\\L5Data\\L5Base.PAL",   "Nlevels\\Cutl5.CEL",    "Nlevels\\Cutl5.pal",            FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_BIGFALL, MT_TOMBRAT, MT_FIREBAT, MT_SKLWING, MT_LICH, MT_CRYPTDMN, MT_INVALID }, ALIGN32
					},
/*DLV_CRYPT3*/		{      30, FALSE,   DTYPE_CRYPT,     DTYPE_CATHEDRAL, TMUSIC_L5,              10,      10, "Crypt 3",
						"NLevels\\L5Data\\L5.AMP", "NLevels\\L5Data\\L5.SOL",    "NLevels\\L5Data\\L5.TMI",     "NLevels\\L5Data\\L5.CEL",     "NLevels\\L5Data\\L5.TIL",     "NLevels\\L5Data\\L5.MIN",     "NLevels\\L5Data\\L5S.CEL",    "NLevels\\L5Data\\L5Base.PAL",   "Nlevels\\Cutl5.CEL",    "Nlevels\\Cutl5.pal",            FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_CRYPTDMN, MT_HELLBAT, MT_BONEDEMN, MT_ARCHLICH, MT_BICLOPS, MT_FLESTHNG, MT_REAPER, MT_INVALID }, ALIGN32
					},
/*DLV_CRYPT4*/		{      32, FALSE,   DTYPE_CRYPT,     DTYPE_CATHEDRAL, TMUSIC_L5,              10,      10, "Crypt 4",
						"NLevels\\L5Data\\L5.AMP", "NLevels\\L5Data\\L5.SOL",    "NLevels\\L5Data\\L5.TMI",     "NLevels\\L5Data\\L5.CEL",     "NLevels\\L5Data\\L5.TIL",     "NLevels\\L5Data\\L5.MIN",     "NLevels\\L5Data\\L5S.CEL",    "NLevels\\L5Data\\L5Base.PAL",   "Nlevels\\Cutl5.CEL",    "Nlevels\\Cutl5.pal",            FALSE,            43, 34,          32,          0,             0,                        WRPT_NONE,    SPT_NONE,
						{ MT_HELLBAT, MT_BICLOPS, MT_FLESTHNG, MT_REAPER, MT_INVALID }, /* MT_ARCHLICH, MT_NAKRUL */ ALIGN32
					},
#endif
					// dLevel, dSetLvl, dType,           dDunType,        dMusic,      dMicroTileLen, dBlocks, dLevelName,
/*SL_BUTCHCHAMB*//*	{       0, TRUE,    DTYPE_CATHEDRAL, DTYPE_CATHEDRAL, TMUSIC_TOWN,            16,      16, "",
					//  dAutomapData,             dSolidTable,                   dMicroFlags,                   dMicroCels,                    dMegaTiles,                    dMiniTiles,                    dSpecCels,                     dPalName,                        dLoadCels,               dLoadPal,                dLoadBarOnTop, dLoadBarColor, dMonDensity, dObjDensity, dSetLvlDunX,   dSetLvlDunY,              dSetLvlWarp,  dSetLvlPiece,
						NULL,                     "Levels\\TownData\\Town.SOL",  "Levels\\TownData\\Town.TMI",  "Levels\\TownData\\Town.CEL",  "Levels\\TownData\\Town.TIL",  "Levels\\TownData\\Town.MIN",  "Levels\\TownData\\TownS.CEL", "",                              "Gendata\\Cutl1d.CEL",   "Gendata\\Cutl1d.pal",            TRUE,           138, 0,           0,           0,             0,                        WRPT_NONE,    SPT_NONE,
					//  dMonTypes,
						{ MT_INVALID }, ALIGN32
					},*/
/*SL_SKELKING*/		{       6, TRUE,    DTYPE_CATHEDRAL, DTYPE_CATHEDRAL, TMUSIC_L1,              10,      10, "Skeleton King's Lair",
						"Levels\\L1Data\\L1.AMP", "Levels\\L1Data\\L1.SOL",      "Levels\\L1Data\\L1.TMI",      "Levels\\L1Data\\L1.CEL",      "Levels\\L1Data\\L1.TIL",      "Levels\\L1Data\\L1.MIN",      "Levels\\L1Data\\L1S.CEL",     "Levels\\L1Data\\L1_2.pal",      "Gendata\\Cutl1d.CEL",   "Gendata\\Cutl1d.pal",            TRUE,           138, 60,          0,           DBORDERX + 2 * 33, DBORDERY + 2 * 13,    WRPT_L1_UP,   SPT_LVL_SKELKING,
						{ MT_RSKELAX, MT_TSKELBW, MT_RSKELBW, MT_XSKELBW, MT_RSKELSD, MT_INVALID }, /* MT_RSKELAX, MT_TSKELBW, MT_RSKELBW, MT_XSKELBW, MT_RSKELSD, */ ALIGN32
					},
/*SL_BONECHAMB*/	{      12, TRUE,    DTYPE_CATACOMBS, DTYPE_CATACOMBS, TMUSIC_L2,              10,      10, "Chamber of Bone",
						"Levels\\L2Data\\L2.AMP", "Levels\\L2Data\\L2.SOL",      "Levels\\L2Data\\L2.TMI",      "Levels\\L2Data\\L2.CEL",      "Levels\\L2Data\\L2.TIL",      "Levels\\L2Data\\L2.MIN",      "Levels\\L2Data\\L2S.CEL",     "Levels\\L2Data\\L2_2.pal",      "Gendata\\Cut2.CEL",     "Gendata\\Cut2.pal",              TRUE,           254, 0,           0,           DBORDERX + 2 * 27, DBORDERY + 2 * 11,    WRPT_L2_DOWN, SPT_LVL_BCHAMB,
						{ MT_INVALID }, /* MT_XSKELSD, MT_BSNEAK, MT_NRHINO */ ALIGN32
					},
/*SL_MAZE*//*		{       0, TRUE,    DTYPE_CATHEDRAL, DTYPE_CATHEDRAL, TMUSIC_TOWN,            16,      16, "Maze",
						NULL,                     "Levels\\TownData\\Town.SOL",  "Levels\\TownData\\Town.TMI",  "Levels\\TownData\\Town.CEL",  "Levels\\TownData\\Town.TIL",  "Levels\\TownData\\Town.MIN",  "Levels\\TownData\\TownS.CEL", "Levels\\L1Data\\L1_5.pal",      "Gendata\\Cutl1d.CEL",   "Gendata\\Cutl1d.pal",            TRUE,           138, 0,           0,           DBORDERX +  4, DBORDERY + 34,            WRPT_NONE,    SPT_MAZE,
						{ MT_INVALID }, / * MT_XRHINO * / ALIGN32
					},*/
/*SL_POISONWATER*/	{       4, TRUE,    DTYPE_CAVES,     DTYPE_CAVES,     TMUSIC_L3,              10,      10, "Poisoned Water Supply",
						"Levels\\L3Data\\L3.AMP", "Levels\\L3Data\\L3.SOL",      "Levels\\L3Data\\L3.TMI",      "Levels\\L3Data\\L3.CEL",      "Levels\\L3Data\\L3.TIL",      "Levels\\L3Data\\L3.MIN",      "Levels\\L1Data\\L1S.CEL",     "Levels\\L3Data\\L3pfoul.pal",   "Gendata\\Cutl1d.CEL",   "Gendata\\Cutl1d.pal",            TRUE,           138, 0,           0,           DBORDERX + 2 * 7, DBORDERY + 2 * 33,     WRPT_L3_DOWN, SPT_LVL_PWATER,
						{ MT_INVALID }, /* MT_DFALLSP, MT_YFALLSD, MT_NGOATMC, MT_NGOATBW */ ALIGN32
					},
/*SL_VILEBETRAYER*/	{      30, TRUE,    DTYPE_CATHEDRAL, DTYPE_CATHEDRAL, TMUSIC_L1,              10,      10, "Archbishop Lazarus' Lair",
						"Levels\\L1Data\\L1.AMP", "Levels\\L1Data\\L1.SOL",      "Levels\\L1Data\\L1.TMI",      "Levels\\L1Data\\L1.CEL",      "Levels\\L1Data\\L1.TIL",      "Levels\\L1Data\\L1.MIN",      "Levels\\L1Data\\L1S.CEL",     "Levels\\L1Data\\L1_2.pal",      "Gendata\\Cutportr.CEL", "Gendata\\Cutportr.pal",         FALSE,            43, 0,           0,           DBORDERX + 2 * 9 + 1, DBORDERY + 2 * 10, WRPT_CIRCLE,  SPT_LVL_BETRAYER,
						{ MT_INVALID }, /* MT_RSUCC, MT_BMAGE */ ALIGN32
					},
	// clang-format on
};

/** Contains the data related to each quest_id. */
const QuestData questlist[NUM_QUESTS] = {
	// clang-format off
	          // _qdlvl,       _qslvl,           _qdmsg,        _qlstr
/*Q_BUTCHER*/ { DLV_CATHEDRAL2, 0,               TEXT_BUTCH9,   "The Butcher"              },
/*Q_PWATER*/  { DLV_CATHEDRAL2, SL_POISONWATER,  TEXT_POISON3,  "Poisoned Water Supply"    },
/*Q_SKELKING*/{ DLV_CATHEDRAL3, SL_SKELKING,     TEXT_KING2,    "The Curse of King Leoric" },
/*Q_BANNER*/  { DLV_CATHEDRAL4, 0,               TEXT_BANNER2,  "Ogden's Sign"             },
/*Q_GARBUD*/  { DLV_CATHEDRAL4, 0,               TEXT_GARBUD1,  "Gharbad The Weak"         },
/*Q_ROCK*/    { DLV_CATACOMBS1, 0,               TEXT_INFRA5,   "The Magic Rock"           },
/*Q_BLOOD*/   { DLV_CATACOMBS1, 0,               TEXT_BLOODY,   "Valor"                    },
/*Q_BCHAMB*/  { DLV_CATACOMBS2, SL_BONECHAMB,    TEXT_BONER,    "The Chamber of Bone"      },
/*Q_BLIND*/   { DLV_CATACOMBS3, 0,               TEXT_BLINDING, "Halls of the Blind"       },
/*Q_ZHAR*/    { DLV_CATACOMBS4, 0,               TEXT_ZHAR1,    "Zhar the Mad"             },
/*Q_MUSHROOM*/{ DLV_CAVES1,     0,               TEXT_MUSH8,    "Black Mushroom"           },
/*Q_ANVIL*/   { DLV_CAVES2,     0,               TEXT_ANVIL5,   "Anvil of Fury"            },
/*Q_WARLORD*/ { DLV_HELL1,      0,               TEXT_BLOODWAR, "Warlord of Blood"         },
/*Q_VEIL*/    { DLV_HELL2,      0,               TEXT_VEIL9,    "Lachdanan"                },
/*Q_BETRAYER*/{ DLV_HELL3,      SL_VILEBETRAYER, TEXT_VILE1,    "Archbishop Lazarus"       },
/*Q_DIABLO*/  { DLV_HELL3,      0,               TEXT_VILE3,    "Diablo"                   },
#ifdef HELLFIRE
/*Q_JERSEY*/  { DLV_CAVES1,     0,               TEXT_JERSEY4,  "The Jersey's Jersey"      },
/*Q_FARMER*/  { DLV_CAVES1,     0,               TEXT_FARMER1,  "Farmer's Orchard"         },
/*Q_GIRL*/    { DLV_NEST1,      0,               TEXT_GIRL2,    "Little Girl"              },
/*Q_DEFILER*/ { DLV_NEST1,      0,               TEXT_DM_NEST,  "The Defiler"              },
/*Q_TRADER*///{ DLV_NEST3,      0,               TEXT_TRADER,   "Wandering Trader"         },
/*Q_GRAVE*/   { DLV_CRYPT1,     0,               TEXT_GRAVE8,   "Grave Matters"            },
/*Q_NAKRUL*/  { DLV_CRYPT4,     0,               TEXT_NONE,     "Na-Krul"                  },
#endif
	// clang-format on
};

const SetPieceData setpiecedata[NUM_SPT_TYPES] = {
	// clang-format off
//                     _spdDunFile,                   _spdPreDunFile
/*SPT_NONE*/         { NULL,                            NULL                           },
/*SPT_BANNER*/       { "Levels\\L1Data\\Banner1.DUN",   "Levels\\L1Data\\Banner2.DUN"  },
/*SPT_SKELKING*/     { "Levels\\L1Data\\SKngDO.DUN",    NULL                           },
/*SPT_BUTCHER*/      { "Levels\\L1Data\\Butcher.DUN",   NULL                           },
/*SPT_MAZE*///       { "Levels\\L1Data\\Lv1MazeA.DUN",  "Levels\\L1Data\\Lv1MazeB.DUN" },
/*SPT_BLIND*/        { "Levels\\L2Data\\Blind1.DUN",    "Levels\\L2Data\\Blind2.DUN"   },
/*SPT_BLOOD*/        { "Levels\\L2Data\\Blood1.DUN",    "Levels\\L2Data\\Blood2.DUN"   },
/*SPT_BCHAMB*/       { "Levels\\L2Data\\Bonestr2.DUN",  "Levels\\L2Data\\Bonestr1.DUN" },
/*SPT_ANVIL*/        { "Levels\\L3Data\\Anvil.DUN",     NULL                           },
/*SPT_BETRAY_S*/     { "Levels\\L4Data\\Viles.DUN",     NULL                           },
/*SPT_BETRAY_M*/     { "Levels\\L4Data\\Vile1.DUN",     NULL                           },
/*SPT_WARLORD*/      { "Levels\\L4Data\\Warlord.DUN",   "Levels\\L4Data\\Warlord2.DUN" },
/*SPT_DIAB_QUAD_1*/  { "Levels\\L4Data\\diab1.DUN",     NULL                           },
/*SPT_DIAB_QUAD_2*/  { "Levels\\L4Data\\diab2b.DUN",    "Levels\\L4Data\\diab2a.DUN"   },
/*SPT_DIAB_QUAD_3*/  { "Levels\\L4Data\\diab3b.DUN",    "Levels\\L4Data\\diab3a.DUN"   },
/*SPT_DIAB_QUAD_4*/  { "Levels\\L4Data\\diab4b.DUN",    "Levels\\L4Data\\diab4a.DUN"   },
/*SPT_LVL_SKELKING*/ { "Levels\\L1Data\\SklKng1.DUN",   "Levels\\L1Data\\SklKng2.DUN"  },
/*SPT_LVL_BCHAMB*/   { "Levels\\L2Data\\Bonecha2.DUN",  "Levels\\L2Data\\Bonecha1.DUN" },
/*SPT_LVL_PWATER*/   { "Levels\\L3Data\\Foulwatr.DUN",  NULL                           },
/*SPT_LVL_BETRAYER*/ { "Levels\\L1Data\\Vile1.DUN",     "Levels\\L1Data\\Vile2.DUN"    },
#ifdef HELLFIRE
/*SPT_NAKRUL*/       { "NLevels\\L5Data\\Nakrul2.DUN",  "NLevels\\L5Data\\Nakrul1.DUN" },
#endif
	// clang-format on
};

DEVILUTION_END_NAMESPACE
