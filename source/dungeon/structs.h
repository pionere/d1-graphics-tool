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

typedef struct RECT32 {
	int x;
	int y;
	int w;
	int h;
} RECT32;

//////////////////////////////////////////////////
// items
//////////////////////////////////////////////////

typedef struct AffixData {
	BYTE PLPower; // item_effect_type
	int PLParam1;
	int PLParam2;
	BYTE PLMinLvl;
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
	int _iSeed;
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
	BOOL _iPostDraw; // should be drawn during the post-phase (magic rock on the stand)
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
	int mFlags;       // _monster_flag
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
	uint16_t mTreasure;  // unique drops of monsters + no-drop flag (unique_item_indexes + _monster_treasure)
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
	int cmFlags;
	uint16_t cmHit;    // hit chance (melee+projectile)
	BYTE cmMinDamage;
	BYTE cmMaxDamage;
	uint16_t cmHit2;   // hit chance of special melee attacks
	BYTE cmMinDamage2;
	BYTE cmMaxDamage2;
	BYTE cmMagic;      // hit chance of magic-projectile
	BYTE cmMagic2;     // unused
	BYTE cmArmorClass; // AC+evasion: used against physical-hit (melee+projectile)
	BYTE cmEvasion;    // evasion: used against magic-projectile
	uint16_t cmMagicRes;  // resistances of the monster
	uint16_t cmTreasure;  // unique drops of monsters + no-drop flag
	unsigned cmExp;
	int cmWidth;
	int cmXOffset;
	BYTE cmAFNum;
	BYTE cmAFNum2;
	uint16_t cmAlign_0; // unused
	uint16_t cmMinHP;
	uint16_t cmMaxHP;
} MapMonData;

#pragma pack(pop)
typedef struct MonsterStruct {
	int _mmode; /* MON_MODE */
	BYTE _mMTidx;
	int _mx;                // Tile X-position of monster
	int _my;                // Tile Y-position of monster
	int _mdir;              // Direction faced by monster (direction enum)
	int _mAnimFrameLen; // Tick length of each frame in the current animation
	int _mAnimCnt;   // Increases by one each game tick, counting how close we are to _mAnimFrameLen
	int _mAnimLen;   // Number of frames in current animation
	int _mAnimFrame; // Current frame of animation.
	int _mmaxhp;
	int _mRndSeed;
	BYTE _muniqtype;
	BYTE _muniqtrans;
	BYTE _mNameColor; // color of the tooltip. white: normal, blue: pack; gold: unique. (text_color)
	BYTE _mlid; // light id of the monster
	BYTE _mleader; // the leader of the monster
	BYTE _mleaderflag; // the status of the monster's leader
	BYTE _mpacksize; // the number of 'pack'-monsters close to their leader
	BYTE _mvid; // vision id of the monster (for minions only)
	const char* _mName;
	BYTE _mLevel;
	MonsterAI _mAI;
	int _mFlags;       // _monster_flag
	int _mType; // _monster_id
	MonAnimStruct* _mAnims;
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
	uint16_t mMagicRes;  // _monster_resistance
	uint16_t mMagicRes2; // _monster_resistance
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
	BYTE oSetLvlType; // dungeon_type
	BYTE otheme;      // theme_id
	BYTE oquest;      // quest_id
	BYTE oProc;       // object_proc_func
	//BYTE oAnimFlag;
	BYTE oAnimBaseFrame; // The starting/base frame of (initially) non-animated objects
	//int oAnimFrameLen; // Tick length of each frame in the current animation
	//int oAnimLen;   // Number of frames in current animation
	//int oAnimWidth;
	//int oSFX;
	//BYTE oSFXCnt;
	//BOOL oSolidFlag;
	//BOOL oMissFlag;
	//BOOL oLightFlag;
	//BYTE oBreak;
	BYTE oDoorFlag;   // object_door_type
	BYTE oSelFlag;
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
	BOOLEAN oMissFlag;
	BOOLEAN oLightFlag;
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
	BOOLEAN _oMissFlag;
	BOOLEAN _oLightFlag;
	BYTE _oBreak; // object_break_mode
	BYTE _oDoorFlag; // object_door_type
	BYTE _oSelFlag;
	BYTE _oTrapChance;
	BOOLEAN _oPreFlag;
	int _oRndSeed;
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
	BYTE _dLevelIdx;  // index in AllLevels
	BOOLEAN _dSetLvl; // cached flag if the level is a set-level
	BYTE _dLevel;     // cached difficulty value of the level
	BYTE _dType;      // cached type of the level
	BYTE _dDunType;   // cached type of the dungeon
} LevelStruct;

typedef struct LevelData {
	BYTE dLevel;
	BOOLEAN dSetLvl;
	BYTE dType;
	BYTE dDunType;
	BYTE dMusic;
	BYTE dMicroTileLen;
	BYTE dBlocks;
	const char* dLevelName;
	const char* dAutomapData;
	const char* dSolidTable;
	const char* dMicroFlags;
	const char* dMicroCels;
	const char* dMegaTiles;
	const char* dMiniTiles;
	const char* dSpecCels;
	const char* dPalName;
	const char* dLoadCels;
	const char* dLoadPal;
	BOOLEAN dLoadBarOnTop;
	BYTE dLoadBarColor;
	const char* dSetLvlPreDun;
	const char* dSetLvlDun;
	BYTE dSetLvlDunX;
	BYTE dSetLvlDunY;
	BYTE dMonTypes[32];
} LevelData;

typedef struct WarpStruct {
	int _wx;
	int _wy;
} WarpStruct;

//////////////////////////////////////////////////
// quests
//////////////////////////////////////////////////

typedef struct QuestStruct {
	BYTE _qactive;
	BYTE _qvar1; // quest parameter which is synchronized with the other players
	BYTE _qvar2; // quest parameter which is NOT synchronized with the other players
	BOOLEAN _qlog;
	unsigned _qmsg;
} QuestStruct;

typedef struct QuestData {
	BYTE _qdlvl;
	BYTE _qslvl;
	int _qdmsg;
	const char* _qlstr;
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

typedef struct THEME_LOC {
	int x;
	int y;
	BYTE ttval;
	int width;
	int height;
} THEME_LOC;

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

//////////////////////////////////////////////////
// themes
//////////////////////////////////////////////////

typedef struct ThemeStruct {
	BYTE ttype;
	BYTE ttval;
} ThemeStruct;

//////////////////////////////////////////////////
// trigs
//////////////////////////////////////////////////

typedef struct TriggerStruct {
	int _tx;
	int _ty;
	int _tmsg;
	int _tlvl;
} TriggerStruct;
