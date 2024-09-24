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
    static bool isStandardClass(int hc);

    bool load(const QString &filePath, const OpenAsParam &params);
    bool save(const SaveAsParam &params);
    void create(unsigned index);

    D1Pal *getPalette() const;
    void setPalette(D1Pal *pal);
    void update();

    QImage getEquipmentImage(int ii) const;
    const ItemStruct *item(int ii) const;
    bool addInvItem(int dst_ii, ItemStruct *is);
    bool swapInvItem(int dst_ii, int src_ii);
    void renameItem(int ii, QString &text);

    void compareTo(const D1Hero *hero, QString header) const;

    QString getFilePath() const;
    void setFilePath(const QString &filePath);
    bool isModified() const;
    void setModified(bool modified = true);
    bool isHellfire() const;
    void setHellfire(bool hellfire);
    bool isMulti() const;
    void setMulti(bool multi);

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
    int getWalkSpeedInTicks() const;
    int getChargeSpeed() const;
    int getBaseAttackSpeed() const;
    int getAttackSpeedInTicks(int sn) const;
    int getBaseCastSpeed() const;
    int getCastSpeedInTicks(int sn) const;
    int getRecoverySpeed() const;
    int getRecoverySpeedInTicks() const;
    int getLightRad() const;
    int getEvasion() const;
    int getAC() const;
    int getBlockChance() const;
    int getGetHit() const;
    int getLifeSteal() const;
    int getManaSteal() const;
    int getArrowVelBonus() const;
    int getArrowVelocity() const;
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
    void getMonDamage(int sn, int sl, const MonsterStruct *mon, int *mindam, int *maxdam) const;
    void getPlrDamage(int sn, int sl, const D1Hero *hero, int *mindam, int *maxdam) const;
    void getMonSkillDamage(int sn, int sl, int dist, const MonsterStruct *mon, int *mindam, int *maxdam) const;
    void getPlrSkillDamage(int sn, int sl, int dist, const D1Hero *target, int *mindam, int *maxdam) const;
    int calcPlrDam(unsigned mRes, unsigned damage) const;

    int getSkillFlags() const;
    int getItemFlags() const;
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
    void calcInv();

    int pnum;
    QString filePath;
    bool modified = false;
    bool hellfire = false;
    bool multi = false;
    D1Pal *palette = nullptr;
};
