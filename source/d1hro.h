#pragma once

#include <QFile>
#include <QPainter>
#include <QString>

#include "d1pal.h"
#include "openasdialog.h"
#include "saveasdialog.h"

struct ItemStruct;
struct MonsterStruct;

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
    void update();

    QImage getEquipmentImage(int ii) const;
    const ItemStruct *item(int ii) const;
    bool addItem(int dst_ii, ItemStruct *is);
    void swapItem(int dst_ii, int src_ii);
    void renameItem(int ii, QString &text);

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
    int getRank() const;
    void setRank(int rank);
    int getStatPoints() const;

    int getStrength() const;
    void setStrength(int value);
    int getBaseStrength() const;
    void addStrength();
    void subStrength();
    int getDexterity() const;
    void setDexterity(int value);
    int getBaseDexterity() const;
    void addDexterity();
    void subDexterity();
    int getMagic() const;
    void setMagic(int value);
    int getBaseMagic() const;
    void addMagic();
    void subMagic();
    int getVitality() const;
    void setVitality(int value);
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

    int getSkillLvl(int sn) const;
    int getSkillLvlBase(int sn) const;
    void setSkillLvlBase(int sn, int level);
    uint64_t getSkills() const;
    uint64_t getFixedSkills() const;
    int getSkillSources(int sn) const;

    int getWalkSpeed() const;
    int getBaseAttackSpeed() const;
    int getBaseCastSpeed() const;
    int getRecoverySpeed() const;
    int getLightRad() const;
    int getEvasion() const;
    int getAC() const;
    int getBlockChance() const;
    int getGetHit() const;
    int getLifeSteal() const;
    int getManaSteal() const;
    int getArrowVelBonus() const;
    int getHitChance() const;
    int getCritChance() const;
    int getSlMinDam() const;
    int getSlMaxDam() const;
    int getBlMinDam() const;
    int getBlMaxDam() const;
    int getPcMinDam() const;
    int getPcMaxDam() const;
    int getChMinDam() const;
    int getChMaxDam() const;
    int getFMinDam() const;
    int getFMaxDam() const;
    int getLMinDam() const;
    int getLMaxDam() const;
    int getMMinDam() const;
    int getMMaxDam() const;
    int getAMinDam() const;
    int getAMaxDam() const;
    int getTotalMinDam() const;
    int getTotalMaxDam() const;
    int getTotalMinDam(const MonsterStruct *mon) const;
    int getTotalMaxDam(const MonsterStruct *mon) const;

    int getItemFlags() const;
    int getSkillCost(int sn) const;
	/*
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
	BYTE _pSkillActivity[64];*/

private:
    D1Hero() = default;
    void rebalance();

    int pnum;
    QString filePath;
    bool modified = false;
    D1Pal *palette = nullptr;
};
