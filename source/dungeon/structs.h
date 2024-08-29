/**
 * @file structs.h
 *
 * Various global structures.
 */

typedef uint8_t BYTE;
typedef uint8_t BOOLEAN;
typedef uint32_t BOOL;

//////////////////////////////////////////////////
// control
//////////////////////////////////////////////////

typedef struct POS32 {
	int x;
	int y;
} POS32;

typedef struct AREA32 {
	int w;
	int h;
} AREA32;

typedef struct RECT32 {
	int x;
	int y;
	int w;
	int h;
} RECT32;

typedef struct RECT_AREA32 {
	int x1;
	int y1;
	int x2;
	int y2;
} RECT_AREA32;

//////////////////////////////////////////////////
// items
//////////////////////////////////////////////////

typedef struct RANGE {
	BYTE from;
	BYTE to;
} RANGE;

typedef struct AffixData {
	BYTE PLPower; // item_effect_type
	int PLParam1;
	int PLParam2;
	RANGE PLRanges[NUM_IARS];
	int PLIType; // affix_item_type
	BOOLEAN PLDouble;
	BOOLEAN PLOk;
	int PLMinVal;
	int PLMaxVal;
	int PLMultVal;
} AffixData;

typedef struct UniqItemData {
	const char* UIName;
	BYTE UIUniqType; // unique_item_type
	BYTE UIMinLvl;
	uint16_t UICurs;
	int UIValue;
	BYTE UIPower1; // item_effect_type
	int UIParam1a;
	int UIParam1b;
	BYTE UIPower2; // item_effect_type
	int UIParam2a;
	int UIParam2b;
	BYTE UIPower3; // item_effect_type
	int UIParam3a;
	int UIParam3b;
	BYTE UIPower4; // item_effect_type
	int UIParam4a;
	int UIParam4b;
	BYTE UIPower5; // item_effect_type
	int UIParam5a;
	int UIParam5b;
	BYTE UIPower6; // item_effect_type
	int UIParam6a;
	int UIParam6b;
} UniqItemData;

typedef struct ItemFileData {
	const char* ifName; // Map of item type .cel file names.
	int idSFX;          // sounds effect of dropping the item on ground (_sfx_id).
	int iiSFX;          // sounds effect of placing the item in the inventory (_sfx_id).
	int iAnimLen;       // item drop animation length
} ItemFileData;

typedef struct ItemData {
	const char* iName;
	BYTE iRnd;
	BYTE iMinMLvl;
	BYTE iUniqType; // unique_item_type
	int iCurs;      // item_cursor_graphic
	int itype;      // item_type
	int iMiscId;    // item_misc_id
	int iSpell;     // spell_id
	BYTE iClass;    // item_class
	BYTE iLoc;      // item_equip_type
	BYTE iDamType;  // item_damage_type
	BYTE iMinDam;
	BYTE iMaxDam;
	BYTE iBaseCrit;
	BYTE iMinStr;
	BYTE iMinMag;
	BYTE iMinDex;
	BOOLEAN iUsable;
	BYTE iMinAC;
	BYTE iMaxAC;
	BYTE iDurability;
	int iValue;
} ItemData;

typedef struct ItemStruct {
	int32_t _iSeed;
	uint16_t _iIdx;        // item_indexes
	uint16_t _iCreateInfo; // icreateinfo_flag
	int _ix;
	int _iy;
	int _iCurs;   // item_cursor_graphic
	int _itype;   // item_type
	int _iMiscId; // item_misc_id
	int _iSpell;  // spell_id
	BYTE _iClass; // item_class enum
	BYTE _iLoc;   // item_equip_type
	BYTE _iDamType; // item_damage_type
	BYTE _iMinDam;
	BYTE _iMaxDam;
	BYTE _iBaseCrit;
	BYTE _iMinStr;
	BYTE _iMinMag;
	BYTE _iMinDex;
	BOOLEAN _iUsable; // can be placed in belt, can be consumed/used or stacked (if max durability is not 1)
	BYTE _iPrePower; // item_effect_type
	BYTE _iSufPower; // item_effect_type
	BYTE _iMagical;	// item_quality
	BYTE _iSelFlag;
	BOOLEAN _iFloorFlag;
	BOOLEAN _iAnimFlag;
	BYTE* _iAnimData;        // PSX name -> ItemFrame
	unsigned _iAnimFrameLen; // Tick length of each frame in the current animation
	unsigned _iAnimCnt;      // Increases by one each game tick, counting how close we are to _iAnimFrameLen
	unsigned _iAnimLen;      // Number of frames in current animation
	unsigned _iAnimFrame;    // Current frame of animation.
	//int _iAnimWidth;
	//int _iAnimXOffset;
	BOOL _iPostDraw; // should be drawn during the post-phase (magic rock on the stand) -- unused
	char _iName[32];
	int _ivalue;
	int _iIvalue;
	int _iAC;
	int _iFlags;	// item_special_effect
	int _iCharges;
	int _iMaxCharges;
	int _iDurability;
	int _iMaxDur;
	int _iPLDam;
	int _iPLToHit;
	int _iPLAC;
	int _iPLStr;
	int _iPLMag;
	int _iPLDex;
	int _iPLVit;
	int _iPLFR;
	int _iPLLR;
	int _iPLMR;
	int _iPLAR;
	int _iPLMana;
	int _iPLHP;
	int _iPLDamMod;
	int _iPLGetHit;
	int8_t _iPLLight;
	int8_t _iPLSkillLevels;
	BYTE _iPLSkill;
	int8_t _iPLSkillLvl;
	BYTE _iPLManaSteal;
	BYTE _iPLLifeSteal;
	BYTE _iPLCrit;
	int _iUid; // unique_item_indexes
	BYTE _iPLFMinDam;
	BYTE _iPLFMaxDam;
	BYTE _iPLLMinDam;
	BYTE _iPLLMaxDam;
	BYTE _iPLMMinDam;
	BYTE _iPLMMaxDam;
	BYTE _iPLAMinDam;
	BYTE _iPLAMaxDam;
	int _iVAdd;
	int _iVMult;
} ItemStruct;

//////////////////////////////////////////////////
// player
//////////////////////////////////////////////////

typedef struct PlrAnimType {
	char patTxt[4]; // suffix to select the player animation CL2
	int patGfxIdx;  // player_graphic_idx
} PlrAnimType;

//////////////////////////////////////////////////
// monster
//////////////////////////////////////////////////

typedef struct MonAnimStruct {
	int maFrames;
	int maFrameLen;
} MonAnimStruct;

typedef struct MonsterAI {
	BYTE aiType; // _monster_ai
	BYTE aiInt;
	BYTE aiParam1;
	BYTE aiParam2;
} MonsterAI;

typedef struct MonsterData {
	uint16_t moFileNum; // _monster_gfx_id
	BYTE mLevel;
	BYTE mSelFlag;
	const char* mTransFile;
	const char* mName;
	MonsterAI mAI;
	uint16_t mMinHP;
	uint16_t mMaxHP;
	unsigned mFlags;  // _monster_flag
	uint16_t mHit;    // hit chance (melee+projectile)
	BYTE mMinDamage;
	BYTE mMaxDamage;
	uint16_t mHit2;   // hit chance of special melee attacks
	BYTE mMinDamage2;
	BYTE mMaxDamage2;
	BYTE mMagic;      // hit chance of magic-projectile
	BYTE mMagic2;     // unused
	BYTE mArmorClass; // AC+evasion: used against physical-hit (melee+projectile)
	BYTE mEvasion;    // evasion: used against magic-projectile
	uint16_t mMagicRes;  // resistances in normal and nightmare difficulties (_monster_resistance)
	uint16_t mMagicRes2; // resistances in hell difficulty (_monster_resistance)
	uint16_t mExp;
} MonsterData;

typedef struct MonFileData {
	int moImage;
	const char* moGfxFile;
	const char* moSndFile;
	int moAnimFrames[NUM_MON_ANIM];
	int moAnimFrameLen[NUM_MON_ANIM];
	BYTE moWidth;
	BOOLEAN moSndSpecial;
	BYTE moAFNum;
	BYTE moAFNum2;
} MonFileData;

#pragma pack(push, 1)
typedef struct MapMonData {
	int cmType;
	BOOL cmPlaceScatter;
	MonAnimStruct cmAnims[NUM_MON_ANIM];
	const char* cmName;
	uint16_t cmFileNum;
	BYTE cmLevel;
	BYTE cmSelFlag;
	MonsterAI cmAI;
	unsigned cmFlags;    // _monster_flag
	int cmHit;           // hit chance (melee+projectile)
	int cmMinDamage;
	int cmMaxDamage;
	int cmHit2;          // hit chance of special melee attacks
	int cmMinDamage2;
	int cmMaxDamage2;
	int cmMagic;         // hit chance of magic-projectile
	int cmArmorClass;    // AC+evasion: used against physical-hit (melee+projectile)
	int cmEvasion;       // evasion: used against magic-projectile
	unsigned cmMagicRes; // resistances of the monster (_monster_resistance)
	unsigned cmExp;
	// int cmWidth;
	// int cmXOffset;
	// BYTE cmAFNum;
	// BYTE cmAFNum2;
	// uint16_t cmAlign_0; // unused
	int cmMinHP;
	int cmMaxHP;
} MapMonData;

#pragma pack(pop)
typedef struct MonsterStruct {
	int _mmode; // MON_MODE
	BYTE _mMTidx;
	int _mx;           // Tile X-position where the monster should be drawn
	int _my;           // Tile Y-position where the monster should be drawn
	int _mdir;         // Direction faced by monster (direction enum)
	int _mAnimFrameLen; // Tick length of each frame in the current animation
	int _mAnimCnt;   // Increases by one each game tick, counting how close we are to _mAnimFrameLen
	int _mAnimLen;   // Number of frames in current animation
	int _mAnimFrame; // Current frame of animation.
	int _mmaxhp;
	int32_t _mRndSeed;
	BYTE _muniqtype;
	BYTE _muniqtrans;
	BYTE _mNameColor;  // color of the tooltip. white: normal, blue: pack; gold: unique. (text_color)
	BYTE _mlid;        // light id of the monster
	BYTE _mleader;     // the leader of the monster
	BYTE _mleaderflag; // the status of the monster's leader
	BYTE _mpacksize;   // the number of 'pack'-monsters close to their leader
	BYTE _mvid;        // vision id of the monster (for minions only)
	const char* _mName;
	BYTE _mLevel;
	BYTE _mSelFlag;
	MonsterAI _mAI;
	unsigned _mFlags;    // _monster_flag
	int _mHit;           // hit chance (melee+projectile)
	int _mMinDamage;
	int _mMaxDamage;
	int _mHit2;          // hit chance of special melee attacks
	int _mMinDamage2;
	int _mMaxDamage2;
	int _mMagic;         // hit chance of magic-projectile
	int _mArmorClass;    // AC+evasion: used against physical-hit (melee+projectile)
	int _mEvasion;       // evasion: used against magic-projectile
	unsigned _mMagicRes; // resistances of the monster (_monster_resistance)
	unsigned _mExp;
} MonsterStruct;

typedef struct UniqMonData {
	int mtype; // _monster_id
	const char* mName;
	const char* mTrnName;
	BYTE muLevelIdx; // level-index to place the monster (dungeon_level)
	BYTE muLevel;    // difficulty level of the monster
	uint16_t mmaxhp;
	MonsterAI mAI;
	BYTE mMinDamage;
	BYTE mMaxDamage;
	BYTE mMinDamage2;
	BYTE mMaxDamage2;
	uint16_t mMagicRes;  // resistances in normal and nightmare difficulties (_monster_resistance)
	uint16_t mMagicRes2; // resistances in hell difficulty (_monster_resistance)
	BYTE mUnqFlags;// _uniq_monster_flag
	BYTE mUnqHit;  // to-hit (melee+projectile) bonus
	BYTE mUnqHit2; // to-hit (special melee attacks) bonus
	BYTE mUnqMag;  // to-hit (magic-projectile) bonus
	BYTE mUnqEva;  // evasion bonus
	BYTE mUnqAC;   // armor class bonus
	BYTE mQuestId; // quest_id
	int mtalkmsg;  // _speech_id
} UniqMonData;

//////////////////////////////////////////////////
// objects
//////////////////////////////////////////////////

typedef struct ObjectData {
	BYTE ofindex;     // object_graphic_id
	BYTE oLvlTypes;   // dungeon_type_mask
	BYTE otheme;      // theme_id
	BYTE oquest;      // quest_id
	//BYTE oAnimFlag;
	BYTE oAnimBaseFrame; // The starting/base frame of (initially) non-animated objects
	//int oAnimFrameLen; // Tick length of each frame in the current animation
	//int oAnimLen;   // Number of frames in current animation
	//int oAnimWidth;
	//int oSFX;
	//BYTE oSFXCnt;
	BYTE oLightRadius;
	int8_t oLightOffX;
	int8_t oLightOffY;
	BYTE oProc;       // object_proc_func
	BYTE oModeFlags;  // object_mode_flags
	//BOOL oSolidFlag;
	//BYTE oBreak;
	BOOLEAN oMissFlag;
	BYTE oDoorFlag;   // object_door_type
	BYTE oSelFlag;
	BYTE oPreFlag;
	BOOLEAN oTrapFlag;
} ObjectData;

typedef struct ObjFileData {
	const char* ofName;
	int oSFX; // _sfx_id
	BYTE oSFXCnt;
	BYTE oAnimFlag; // object_anim_mode
	int oAnimFrameLen; // Tick length of each frame in the current animation
	int oAnimLen;   // Number of frames in current animation
	int oAnimWidth;
	BOOLEAN oSolidFlag;
	BYTE oBreak; // object_break_mode
} ObjFileData;

typedef struct ObjectStruct {
	int _otype; // _object_id
	int _ox;    // Tile X-position of the object
	int _oy;    // Tile Y-position of the object
	BYTE _oSFXCnt;
	BYTE _oAnimFlag;  // object_anim_mode
	BYTE _oProc;      // object_proc_func
	BYTE _oModeFlags; // object_mode_flags
	int _oAnimFrameLen; // Tick length of each frame in the current animation
	int _oAnimCnt;   // Increases by one each game tick, counting how close we are to _oAnimFrameLen
	int _oAnimLen;   // Number of frames in current animation
	int _oAnimFrame; // Current frame of animation.
	BOOLEAN _oSolidFlag;
	BYTE _oBreak; // object_break_mode
	BYTE _oTrapChance;
	BYTE _oAlign;
	BOOLEAN _oMissFlag;
	BYTE _oDoorFlag; // object_door_type
	BYTE _oSelFlag;
	BOOLEAN _oPreFlag;
	int32_t _oRndSeed;
	int _oVar1;
	int _oVar2;
	int _oVar3;
	int _oVar4;
	int _oVar5;
	int _oVar6;
	int _oVar7;
	int _oVar8;
} ObjectStruct;

//////////////////////////////////////////////////
// levels
//////////////////////////////////////////////////

typedef struct LevelStruct {
	int _dLevelIdx;   // dungeon_level / NUM_LEVELS
	int _dLevelNum;   // index in AllLevels (dungeon_level / NUM_FIXLVLS)
	bool _dSetLvl;    // cached flag if the level is a set-level
	bool _dDynLvl;    // cached flag if the level is a dynamic-level
	int _dLevel;      // cached difficulty value of the level
	int _dType;       // cached type of the level (dungeon_type)
	int _dDunType;    // cached type of the dungeon (dungeon_gen_type)
	int _dLevelPlyrs; // cached number of players when the level was 'initialized'
	int _dLevelBonus; // cached level bonus
} LevelStruct;

typedef struct LevelFileData {
	const char* dSubtileSettings;
	const char* dTileFlags;
	const char* dMicroCels;
	const char* dMegaTiles;
	const char* dMiniTiles;
	const char* dSpecCels;
	const char* dLightTrns;
} LevelFileData;

typedef struct LevelData {
	BYTE dLevel;
	BOOLEAN dSetLvl;
	BYTE dType;    // dungeon_type
	BYTE dDunType; // dungeon_gen_type
	BYTE dMusic;   // _music_id
	BYTE dfindex;  // level_graphic_id
	BYTE dMicroTileLen;
	BYTE dBlocks;
	const char* dLevelName;
	const char* dPalName;
	const char* dLoadCels;
	const char* dLoadPal;
	BOOLEAN dLoadBarOnTop;
	BYTE dLoadBarColor;
	BYTE dMonDensity;
	BYTE dObjDensity;
	BYTE dSetLvlDunX;
	BYTE dSetLvlDunY;
	BYTE dSetLvlWarp; // dungeon_warp_type
	BYTE dSetLvlPiece; // _setpiece_type
	BYTE dMonTypes[32];
} LevelData;

typedef struct WarpStruct {
	int _wx;
	int _wy;
	int _wtype; // dungeon_warp_type
	int _wlvl;  // dungeon_level / _setlevels
} WarpStruct;

typedef struct SetPieceStruct {
	BYTE* _spData;
	int _spx;
	int _spy;
	int _sptype; // _setpiece_type
} SetPieceStruct;

typedef struct SetPieceData {
	const char* _spdDunFile;
	const char* _spdPreDunFile;
} SetPieceData;

//////////////////////////////////////////////////
// quests
//////////////////////////////////////////////////

typedef struct QuestStruct {
	BYTE _qactive; // quest_state
	BYTE _qvar1; // quest parameter which is synchronized with the other players
	BYTE _qvar2; // quest parameter which is NOT synchronized with the other players
	BOOLEAN _qlog;
	unsigned _qmsg;
} QuestStruct;

typedef struct QuestData {
	BYTE _qdlvl; // dungeon_level
	BYTE _qslvl; // _setlevels
	int _qdmsg;  // _speech_id
	const char* _qlstr; // quest title
} QuestData;

//////////////////////////////////////////////////
// spells
//////////////////////////////////////////////////

typedef struct SpellData {
	BYTE sManaCost;
	BYTE sType;
	BYTE sIcon;
	const char* sNameText;
	BYTE sBookLvl;   // minimum level for books
	BYTE sStaffLvl;  // minimum level for staves
	BYTE sScrollLvl; // minimum level for scrolls/runes
	BYTE sSkillFlags; // flags (SDFLAG*) of the skill
	BYTE scCurs; // cursor for scrolls/runes
	BYTE spCurs; // cursor for spells
	BYTE sUseFlags; // the required flags(SFLAG*) to use the skill
	BYTE sMinInt;
	BYTE sSFX;
	BYTE sMissile;
	BYTE sManaAdj;
	BYTE sMinMana;
	uint16_t sStaffMin;
	uint16_t sStaffMax;
	int sBookCost;
	int sStaffCost; // == sScrollCost == sRuneCost
} SpellData;

//////////////////////////////////////////////////
// gendung
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// drlg
//////////////////////////////////////////////////

typedef struct ShadowPattern {
	union {
		struct {
			BYTE sh11;
			BYTE sh01;
			BYTE sh10;
			BYTE sh00;
		};
		uint32_t asUInt32;
	};
} ShadowPattern;

typedef struct ShadowStruct {
	ShadowPattern shPattern;
	ShadowPattern shMask;
	BYTE nv1;
	BYTE nv2;
	BYTE nv3;
} ShadowStruct;

typedef struct ROOMHALLNODE {
	int nRoomParent;
	int nRoomx1;
	int nRoomy1;
	int nRoomx2;
	int nRoomy2;
	int nHallx1;
	int nHally1;
	int nHallx2;
	int nHally2;
	int nHalldir;
} ROOMHALLNODE;

typedef struct L1ROOM {
	int lrx;
	int lry;
	int lrw;
	int lrh;
} L1ROOM;

typedef struct ThemePosDir {
	int tpdx;
	int tpdy;
	int tpdvar1;
	int tpdvar2; // unused
} ThemePosDir;

/** The number of generated rooms in cathedral. */
#define L1_MAXROOMS ((DSIZEX * DSIZEY) / sizeof(L1ROOM))
/** The number of generated rooms in catacombs. */
#define L2_MAXROOMS 32
/** Possible matching locations in a theme room. */
#define THEME_LOCS ((DSIZEX * DSIZEY) / sizeof(ThemePosDir))

typedef struct DrlgMem {
	union {
		L1ROOM L1RoomList[L1_MAXROOMS];     // drlg_l1
		ROOMHALLNODE RoomList[L2_MAXROOMS]; // drlg_l2
		BYTE transvalMap[DMAXX][DMAXY];     // drlg_l1, drlg_l2, drlg_l3, drlg_l4
		BYTE transDirMap[DSIZEX][DSIZEY];   // drlg_l1, drlg_l2, drlg_l3, drlg_l4 (gendung)
		BYTE lockoutMap[DMAXX][DMAXY];      // drlg_l3
		BYTE dungBlock[L4BLOCKX][L4BLOCKY]; // drlg_l4
		ThemePosDir thLocs[THEME_LOCS];     // themes
	};
} DrlgMem;

//////////////////////////////////////////////////
// themes
//////////////////////////////////////////////////

typedef struct ThemeStruct {
	int _tsx1; // top-left corner of the theme-room
	int _tsy1;
	int _tsx2; // bottom-right corner of the theme-room
	int _tsy2;
	BYTE _tsType;
	BYTE _tsTransVal;
	BYTE _tsObjVar1;
	BYTE _tsObjVar2; // unused
	int _tsObjX;
	int _tsObjY;
} ThemeStruct;

//////////////////////////////////////////////////
// lighting
//////////////////////////////////////////////////

typedef struct LightListStruct {
	int _lx;
	int _ly;
	int _lunx;
	int _luny;
	BYTE _lradius;
	BYTE _lunr;
	int8_t _lunxoff;
	int8_t _lunyoff;
	BOOLEAN _ldel;
	BOOLEAN _lunflag;
	BOOLEAN _lmine;
	BYTE _lAlign2;
	int _lxoff;
	int _lyoff;
} LightListStruct;

//////////////////////////////////////////////////
// trigs
//////////////////////////////////////////////////

typedef struct TriggerStruct {
	int _tx;
	int _ty;
	int _ttype; // dungeon_warp_type
	int _tlvl;  // dungeon_level
	int _tmsg;  // window_messages
} TriggerStruct;
