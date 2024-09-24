#include "pvpdetailswidget.h"

#include <QApplication>
#include <QMessageBox>

#include "d1hro.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_pvpdetailswidget.h"

#include "dungeon/all.h"

PvPDetailsWidget::PvPDetailsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PvPDetailsWidget())
{
    ui->setupUi(this);

    this->ui->pvpHeroDetails->setReadOnly();
}

PvPDetailsWidget::~PvPDetailsWidget()
{
    delete ui;

    qDeleteAll(this->heros);
}

void PvPDetailsWidget::initialize(D1Hero *h)
{
    this->hero = h;
    this->ui->offHeroSkillsComboBox->setHero(h);

    // LogErrorF("PvPDetailsWidget init 5");
    this->updateFields();
    // LogErrorF("PvPDetailsWidget init 6");
    this->setVisible(true);
}

void PvPDetailsWidget::displayFrame()
{
    this->updateFields();

    if (this->ui->pvpHeroDetails->isVisible())
        this->ui->pvpHeroDetails->displayFrame();
}

static void displayDamage(QLabel *label, int minDam, int maxDam)
{
    if (maxDam != 0) {
        if (minDam != maxDam)
            label->setText(QString("%1 - %2").arg(minDam).arg(maxDam));
        else
            label->setText(QString("%1").arg(minDam));
    } else {
        label->setText(QString("-"));
    }
}

typedef struct PlayerDamage {
    bool hth;
    bool mis;

    int minHth;
    int maxHth;
    int chanceHth;

    int minMis;
    int maxMis;
    int chanceMis;
    BYTE resMis;
    bool blockMis;

} PlayerDamage;

static PlayerDamage GetPlayerDamage(const D1Hero *offHero, int sn, int dist, const D1Hero *defHero)
{
    int sl = offHero->getSkillLvl(sn);
    bool mis = spelldata[sn].sType != STYPE_NONE || (spelldata[sn].sUseFlags & SFLAG_RANGED);
    bool hth = !mis;
    PlayerDamage result = { 0 };
    if (mis) {
        result.mis = true;
        int mtype = spelldata[sn].sMissile;
        mtype = GetBaseMissile(mtype);
        result.resMis = GetMissileElement(mtype);
        result.blockMis = !(missiledata[mtype].mdFlags & MIF_NOBLOCK);
        int mindam, maxdam;
        offHero->getPlrSkillDamage(sn, sl, dist, defHero, &mindam, &maxdam);
        result.minMis = mindam;
        result.maxMis = maxdam;
        result.chanceMis = MissPlrHitByPlrChance(mtype, dist, offHero, defHero);
        if (sn == SPL_CHARGE)
            result.chanceMis = sl * 16 - defHero->getAC();
    }

    if (hth) {
        result.hth = true;
        result.chanceHth = offHero->getHitChance() - defHero->getAC();
        if (sn == SPL_SWIPE) {
            result.chanceHth -= 30 - sl * 2;
        }
        int mindam, maxdam;
        offHero->getPlrDamage(sn, sl, defHero, &mindam, &maxdam);
        result.minHth = mindam;
        result.maxHth = maxdam;
    }
    return result;
}

void PvPDetailsWidget::updateFields()
{
    bool hasEnemy = !this->heros.isEmpty();
    this->ui->discardHeroButton->setEnabled(hasEnemy);
    this->ui->pvpHeroDetails->setVisible(hasEnemy);

    D1Hero *newhero = D1Hero::instance();
    this->ui->addHeroButton->setEnabled(newhero != nullptr);
    delete newhero;

    if (hasEnemy) {
        D1Hero *defHero = this->heros[this->ui->pvpHerosComboBox->currentIndex()];
        D1Hero *offHero = this->hero;

        int dist = this->ui->plrDistSpinBox->value();
        this->ui->plrDistSpinBox->setToolTip(tr("Distance to target in ticks. Charge distance to target: %1 / %2").arg(dist * offHero->getChargeSpeed()).arg(dist * defHero->getChargeSpeed()));

        int hper, mindam, maxdam;
        int offSn = this->ui->offHeroSkillsComboBox->update();
        PlayerDamage offDmg = GetPlayerDamage(offHero, offSn, dist, defHero);
        int defSn = this->ui->defHeroSkillsComboBox->update();
        PlayerDamage defDmg = GetPlayerDamage(defHero, defSn, dist, offHero);

        if (offDmg.hth) {
            hper = offDmg.chanceHth;
            hper = CheckHit(hper);
            this->ui->offPlrHitChance->setText(QString("%1%").arg(hper));
        } else {
            this->ui->offPlrHitChance->setText(QString("-"));
        }
        this->ui->offPlrHitChanceSep->setVisible(offDmg.mis);
        this->ui->offPlrHitChance2->setVisible(offDmg.mis);
        if (offDmg.mis) {
            hper = offDmg.chanceMis;
            hper = CheckHit(hper);
            this->ui->offPlrHitChance2->setText(QString("%1%").arg(hper));
        }

        this->ui->offPlrDamageSep->setVisible(offDmg.mis);
        this->ui->offPlrDamage2->setVisible(offDmg.mis);
        if (offDmg.mis) {
            mindam = offDmg.minMis;
            maxdam = offDmg.maxMis;
            displayDamage(this->ui->offPlrDamage2, mindam, maxdam);
            this->ui->offPlrDamage2->setStyleSheet(GetElementColor(offDmg.resMis));
        }
        mindam = offDmg.minHth;
        maxdam = offDmg.maxHth;
        displayDamage(this->ui->offPlrDamage, mindam, maxdam);

        if (defDmg.hth) {
            hper = defDmg.chanceHth;
            hper = CheckHit(hper);
            this->ui->defPlrHitChance->setText(QString("%1%").arg(hper));
        } else {
            this->ui->defPlrHitChance->setText(QString("-"));
        }
        this->ui->defPlrHitChanceSep->setVisible(defDmg.mis);
        this->ui->defPlrHitChance2->setVisible(defDmg.mis);
        if (defDmg.mis) {
            hper = defDmg.chanceMis;
            hper = CheckHit(hper);
            this->ui->defPlrHitChance2->setText(QString("%1%").arg(hper));
        }

        this->ui->defPlrDamageSep->setVisible(defDmg.mis);
        this->ui->defPlrDamage2->setVisible(defDmg.mis);
        if (defDmg.mis) {
            mindam = defDmg.minMis;
            maxdam = defDmg.maxMis;
            displayDamage(this->ui->defPlrDamage2, mindam, maxdam);
            this->ui->defPlrDamage2->setStyleSheet(GetElementColor(defDmg.resMis));
        }
        mindam = defDmg.minHth;
        maxdam = defDmg.maxHth;
        displayDamage(this->ui->defPlrDamage, mindam, maxdam);

        hper = -1;
        // if (hth || (mtype != -1 && !(missiledata[mtype].mdFlags & MIF_NOBLOCK))) {
        if ((offHero->getSkillFlags() & SFLAG_BLOCK) && (defDmg.hth || (defDmg.mis && defDmg.blockMis))) {
            hper = offHero->getBlockChance();
            if (hper != 0) {
                hper -= 2 * defHero->getLevel();
                hper = CheckHit(hper);
            }
        }
        if (hper < 0) {
            this->ui->offPlrBlockChance->setText(QString("-"));
        } else if (defDmg.hth && defDmg.mis && !defDmg.blockMis) {
            this->ui->offPlrBlockChance->setText(QString("%1% | -").arg(hper));
        } else {
            this->ui->offPlrBlockChance->setText(QString("%1%").arg(hper));
        }

        hper = -1;
        // if (hth || (mtype != -1 && !(missiledata[mtype].mdFlags & MIF_NOBLOCK))) {
        if ((defHero->getSkillFlags() & SFLAG_BLOCK) && (offDmg.hth || (offDmg.mis && offDmg.blockMis))) {
            hper = defHero->getBlockChance();
            if (hper != 0) {
                hper -= 2 * offHero->getLevel();
                hper = CheckHit(hper);
            }
        }
        if (hper < 0) {
            this->ui->defPlrBlockChance->setText(QString("-"));
        } else if (offDmg.hth && offDmg.mis && !offDmg.blockMis) {
            this->ui->defPlrBlockChance->setText(QString("%1% | -").arg(hper));
        } else {
            this->ui->defPlrBlockChance->setText(QString("%1%").arg(hper));
        }
    }
}

void PvPDetailsWidget::on_pvpHerosComboBox_activated(int index)
{
    D1Hero *currhero = this->heros[index];

    this->ui->pvpHeroDetails->initialize(currhero);
    this->ui->defHeroSkillsComboBox->setHero(currhero);

    this->displayFrame();
}

void PvPDetailsWidget::on_discardHeroButton_clicked()
{
    int index = this->ui->pvpHerosComboBox->currentIndex();

    this->ui->pvpHerosComboBox->removeItem(index);
    D1Hero *currhero = this->heros.takeAt(index);
    delete currhero;

    if (this->heros.count() <= index) {
        index--;
        if (index < 0) {
            this->updateFields();
            return;
        }
    }
    this->on_pvpHerosComboBox_activated(index);
}

void PvPDetailsWidget::on_addHeroButton_clicked()
{
    QString filePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Hero"), tr("HRO Files (*.hro *.HRO)"));

    if (!filePath.isEmpty()) {
        OpenAsParam params = OpenAsParam();
        params.filePath = filePath;

        D1Hero *newhero = D1Hero::instance();
        // newhero->setPalette(this->trnBase->getResultingPalette());
        if (newhero->load(filePath, params)) {
            this->ui->pvpHerosComboBox->addItem(newhero->getName());
            this->heros.append(newhero);

            // this->updateFields();
            this->on_pvpHerosComboBox_activated(this->heros.count() - 1);
        } else {
            delete newhero;
            dProgressFail() << tr("Failed loading HRO file: %1.").arg(QDir::toNativeSeparators(filePath));
        }
    }
}

void PvPDetailsWidget::on_offHeroSkillsComboBox_activated(int index)
{
    this->updateFields();
}

void PvPDetailsWidget::on_plrDistSpinBox_valueChanged(int value)
{
    this->updateFields();
}

void PvPDetailsWidget::on_defHeroSkillsComboBox_activated(int index)
{
    this->updateFields();
}

void PvPDetailsWidget::on_closeButton_clicked()
{
    this->setVisible(false);
}
