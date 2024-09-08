#pragma once

#include <QFile>
#include <QPainter>
#include <QString>

#include "d1pal.h"
#include "openasdialog.h"
#include "saveasdialog.h"

struct ItemStruct;

class D1Hero : public QObject {
    Q_OBJECT

public:
    ~D1Hero();

    static D1Hero* instance();

    bool load(const QString &filePath, const OpenAsParam &params);
    bool save(const SaveAsParam &params);
    void create(unsigned index);

    D1Pal *getPalette() const;
    void setPalette(D1Pal *pal);

    QImage getEquipmentImage() const;
    const ItemStruct *item(int ii) const;

    void compareTo(const D1Hero *hero, QString header) const;

    QString getFilePath() const;
    void setFilePath(const QString &filePath);
    bool isModified() const;
    void setModified(bool modified = true);

    const char* getName() const;
    void setName(const QString &name);

    int getClass() const;
    void setClass(int cls);

    int getLevel() const;
    void setLevel(int level);
    int getStatPoints() const;

    int getStrength() const;
    int getBaseStrength() const;
    void addStrength();
    void subStrength();
    int getDexterity() const;
    int getBaseDexterity() const;
    void addDexterity();
    void subDexterity();
    int getMagic() const;
    int getBaseMagic() const;
    void addMagic();
    void subMagic();
    int getVitality() const;
    int getBaseVitality() const;
    void addVitality();
    void subVitality();

    int getLife() const;
    int getBaseLife() const;
    void decLife();
    void restoreLife();
    int getMana() const;
    int getBaseMana() const;

    int getMagicResist() const;
    int getFireResist() const;
    int getLightningResist() const;
    int getAcidResist() const;

	/*
	BYTE _pRank;
	BYTE _pLightRad;
	unsigned _pExperience;
	unsigned _pNextExper;
	BYTE _pAtkSkill;         // the selected attack skill for the primary action
	BYTE _pAtkSkillType;     // the (RSPLTYPE_)type of the attack skill for the primary action
	BYTE _pMoveSkill;        // the selected movement skill for the primary action
	BYTE _pMoveSkillType;    // the (RSPLTYPE_)type of the movement skill for the primary action
	BYTE _pAltAtkSkill;      // the selected attack skill for the secondary action
	BYTE _pAltAtkSkillType;  // the (RSPLTYPE_)type of the attack skill for the secondary action
	BYTE _pAltMoveSkill;     // the selected movement skill for the secondary action
	BYTE _pAltMoveSkillType; // the (RSPLTYPE_)type of the movement skill for the secondary action
	BYTE _pSkillLvlBase[64]; // the skill levels of the player if they would not wear an item
	BYTE _pSkillActivity[64];
	unsigned _pSkillExp[64];
	uint64_t _pMemSkills;  // Bitmask of learned skills
	uint64_t _pAblSkills;  // Bitmask of abilities
	uint64_t _pInvSkills;  // Bitmask of skills available via items in inventory (scrolls or runes)
	ItemStruct _pInvBody[NUM_INVLOC];
	ItemStruct _pInvList[NUM_INV_GRID_ELEM];
	BYTE _pSkillLvl[64]; // the skill levels of the player
	BYTE _pAlign_B0;
	int _pISlMinDam; // min slash-damage (swords, axes)
	int _pISlMaxDam; // max slash-damage (swords, axes)
	int _pIBlMinDam; // min blunt-damage (maces, axes)
	int _pIBlMaxDam; // max blunt-damage (maces, axes)
	int _pIPcMinDam; // min puncture-damage (bows, daggers)
	int _pIPcMaxDam; // max puncture-damage (bows, daggers)
	int _pIChMinDam; // min charge-damage (shield charge)
	int _pIChMaxDam; // max charge-damage (shield charge)
	int _pIEvasion;
	int _pIAC;
	int _pIHitChance;
	BYTE _pSkillFlags;    // Bitmask of allowed skill-types (SFLAG_*)
	BYTE _pIBaseHitBonus; // indicator whether the base BonusToHit of the items is positive/negative/neutral
	BYTE _pICritChance; // 200 == 100%
	BYTE _pIBlockChance;
	uint64_t _pISpells; // Bitmask of skills available via equipped items (staff)
	unsigned _pIFlags;
	BYTE _pIWalkSpeed;
	BYTE _pIRecoverySpeed;
	BYTE _pIBaseCastSpeed;
	int _pIGetHit;
	BYTE _pIBaseAttackSpeed;
	int8_t _pIArrowVelBonus; // _pISplCost in vanilla code
	BYTE _pILifeSteal;
	BYTE _pIManaSteal;
	int _pIFMinDam; // min fire damage (item's added fire damage)
	int _pIFMaxDam; // max fire damage (item's added fire damage)
	int _pILMinDam; // min lightning damage (item's added lightning damage)
	int _pILMaxDam; // max lightning damage (item's added lightning damage)
	int _pIMMinDam; // min magic damage (item's added magic damage)
	int _pIMMaxDam; // max magic damage (item's added magic damage)
	int _pIAMinDam; // min acid damage (item's added acid damage)
	int _pIAMaxDam; // max acid damage (item's added acid damage)*/

private:
    D1Hero() = default;
    void rebalance();

    int pnum;
    QString filePath;
    bool modified = false;
    D1Pal *palette = nullptr;
};
