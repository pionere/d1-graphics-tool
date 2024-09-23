#include "heroskillscombobox.h"

#include <QComboBox>
#include <QMessageBox>

#include "d1hro.h"

#include "dungeon/all.h"

HeroSkillsComboBox::HeroSkillsComboBox(QWidget *parent)
    : QComboBox(parent)
{
}

void HeroSkillsComboBox::setHero(D1Hero *h)
{
    this->hero = h;
}

int HeroSkillsComboBox::update()
{
    int mi, numspells;
    mi = this->currentData().value<int>();
    // QMessageBox::critical(nullptr, "Error", QApplication::tr("Selected skill %1.").arg(mi));
    this->clear();
    numspells = (IsHellfireGame && this->hero->isHellfire()) ? NUM_SPELLS : NUM_SPELLS_DIABLO;
    // QMessageBox::critical(nullptr, "Error", tr("Selected skill %1 of %2.").arg(mi).arg(numspells));
    for (int sn = 0; sn < numspells; sn++) {
        if ((spelldata[sn].sUseFlags & this->hero->getSkillFlags()) != spelldata[sn].sUseFlags) continue;
        // if (sn != SPL_ATTACK) {
            if (!HasSkillDamage(sn)) continue;
            /*if (spelldata[sn].sBookLvl == SPELL_NA && spelldata[sn].sStaffLvl == SPELL_NA && !SPELL_RUNE(sn)) {
                continue;
            }*/
            if (this->hero->getSkillLvl(sn) == 0 && !(this->hero->getFixedSkills() & SPELL_MASK(sn)) && !SPELL_RUNE(sn)) {
                continue;
            }
        // }
        GetSkillName(sn);
        this->addItem(infostr, QVariant::fromValue(sn));
    }
    // QMessageBox::critical(nullptr, "Error", tr("Added %1 skills.").arg(this->count()));
    mi = this->findData(mi);
    // QMessageBox::critical(nullptr, "Error", QApplication::tr("Skill index %1.").arg(mi));
    if (mi < 0) mi = 0;
    this->setCurrentIndex(mi);

    return this->currentData().value<int>();
}
