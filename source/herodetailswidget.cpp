#include "herodetailswidget.h"

#include <QApplication>
#include <QMessageBox>
#include <QString>

#include "celview.h"
#include "mainwindow.h"
#include "ui_herodetailswidget.h"

#include "dungeon/all.h"

HeroDetailsWidget::HeroDetailsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HeroDetailsWidget())
{
    ui->setupUi(this);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->heroNameEdit, SIGNAL(cancel_signal()), this, SLOT(on_heroNameEdit_escPressed()));
    QObject::connect(this->ui->heroLevelEdit, SIGNAL(cancel_signal()), this, SLOT(on_heroLevelEdit_escPressed()));
    QObject::connect(this->ui->heroRankEdit, SIGNAL(cancel_signal()), this, SLOT(on_heroRankEdit_escPressed()));

    QObject::connect(this->ui->heroStrengthEdit, SIGNAL(cancel_signal()), this, SLOT(on_heroStrengthEdit_escPressed()));
    QObject::connect(this->ui->heroDexterityEdit, SIGNAL(cancel_signal()), this, SLOT(on_heroDexterityEdit_escPressed()));
    QObject::connect(this->ui->heroMagicEdit, SIGNAL(cancel_signal()), this, SLOT(on_heroMagicEdit_escPressed()));
    QObject::connect(this->ui->heroVitalityEdit, SIGNAL(cancel_signal()), this, SLOT(on_heroVitalityEdit_escPressed()));
}

HeroDetailsWidget::~HeroDetailsWidget()
{
    delete ui;
}

void HeroDetailsWidget::initialize(D1Hero *h)
{
    this->hero = h;
    // IsHellfireGame = !isHeroStandardClass(h->isHellfire());
    // gnDifficulty = h->getRank();

    this->updateFields();
}

void HeroDetailsWidget::displayFrame()
{
    this->updateFields();
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

static void HeroResistText(int misr, int res, QProgressBar *label)
{
    QString tooltip = QApplication::tr("Resistance to %1 damage");

    QString type;
    switch (misr) {
    case MISR_SLASH:     type = QApplication::tr("slash");     break;
    case MISR_BLUNT:     type = QApplication::tr("blunt");     break;
    case MISR_PUNCTURE:  type = QApplication::tr("puncture");  break;
    case MISR_FIRE:      type = QApplication::tr("fire");      break;
    case MISR_LIGHTNING: type = QApplication::tr("lightning"); break;
    case MISR_MAGIC:     type = QApplication::tr("magic");     break;
    case MISR_ACID:      type = QApplication::tr("acid");      break;
    }

    label->setValue(res);
    label->setToolTip(tooltip.arg(type));
}

void HeroDetailsWidget::updateFields()
{
    int hc, bv;
    QLabel *label;
    LineEditWidget *lineEdit;

    QObject *view = this->parent();
    CelView *celView = qobject_cast<CelView *>(view);
    if (celView != nullptr) {
        celView->updateLabel();
    }
    // this->updateLabel();

    hc = this->hero->getClass();
    // set context-fields
    this->ui->gameHellfireCheckBox->setChecked(this->hero->isHellfire());
    this->ui->gameHellfireCheckBox->setEnabled(D1Hero::isStandardClass(hc));
    this->ui->gameDifficultyComboBox->setCurrentIndex(gnDifficulty);

    // set hero-fields
    this->ui->heroNameEdit->setText(this->hero->getName());

    this->ui->heroClassComboBox->setCurrentIndex(hc);

    bv = this->hero->getLevel();
    this->ui->heroLevelEdit->setText(QString::number(bv));
    this->ui->heroDecLevelButton->setEnabled(bv > 1);
    this->ui->heroIncLevelButton->setEnabled(bv < MAXCHARLEVEL);
    this->ui->heroRankEdit->setText(QString::number(this->hero->getRank()));

    int statPts = this->hero->getStatPoints();
    this->ui->heroStatPtsLabel->setText(QString::number(statPts));
    this->ui->heroAddStrengthButton->setEnabled(statPts > 0);
    this->ui->heroAddDexterityButton->setEnabled(statPts > 0);
    this->ui->heroAddMagicButton->setEnabled(statPts > 0);
    this->ui->heroAddVitalityButton->setEnabled(statPts > 0);

    lineEdit = this->ui->heroStrengthEdit;
    lineEdit->setText(QString::number(this->hero->getStrength()));
    bv = this->hero->getBaseStrength();
    lineEdit->setToolTip(QString::number(bv));
    this->ui->heroSubStrengthButton->setEnabled(bv > StrengthTbl[hc]);
    lineEdit = this->ui->heroDexterityEdit;
    lineEdit->setText(QString::number(this->hero->getDexterity()));
    bv = this->hero->getBaseDexterity();
    lineEdit->setToolTip(QString::number(bv));
    this->ui->heroSubDexterityButton->setEnabled(bv > DexterityTbl[hc]);
    lineEdit = this->ui->heroMagicEdit;
    lineEdit->setText(QString::number(this->hero->getMagic()));
    bv = this->hero->getBaseMagic();
    lineEdit->setToolTip(QString::number(bv));
    this->ui->heroSubMagicButton->setEnabled(bv > MagicTbl[hc]);
    lineEdit = this->ui->heroVitalityEdit;
    lineEdit->setText(QString::number(this->hero->getVitality()));
    bv = this->hero->getBaseVitality();
    lineEdit->setToolTip(QString::number(bv));
    this->ui->heroSubVitalityButton->setEnabled(bv > VitalityTbl[hc]);

    label = this->ui->heroLifeLabel;
    label->setText(QString::number(this->hero->getLife()));
    label->setToolTip(QString::number(this->hero->getBaseLife()));
    label = this->ui->heroManaLabel;
    label->setText(QString::number(this->hero->getMana()));
    label->setToolTip(QString::number(this->hero->getBaseMana()));

    HeroResistText(MISR_MAGIC, this->hero->getMagicResist(), this->ui->heroMagicResist);
    HeroResistText(MISR_FIRE, this->hero->getFireResist(), this->ui->heroFireResist);
    HeroResistText(MISR_LIGHTNING, this->hero->getLightningResist(), this->ui->heroLightningResist);
    HeroResistText(MISR_ACID, this->hero->getAcidResist(), this->ui->heroAcidResist);

    this->ui->heroWalkSpeedLabel->setText(QString::number(this->hero->getWalkSpeed()));
    this->ui->heroBaseAttackSpeedLabel->setText(QString::number(this->hero->getBaseAttackSpeed()));
    this->ui->heroBaseCastSpeedLabel->setText(QString::number(this->hero->getBaseCastSpeed()));
    this->ui->heroRecoverySpeedLabel->setText(QString::number(this->hero->getRecoverySpeed()));
    this->ui->heroLightRadLabel->setText(QString::number(this->hero->getLightRad()));
    this->ui->heroEvasionLabel->setText(QString::number(this->hero->getEvasion()));
    this->ui->heroACLabel->setText(QString::number(this->hero->getAC()));
    this->ui->heroBlockChanceLabel->setText(QString("%1%").arg(this->hero->getBlockChance()));
    this->ui->heroGetHitLabel->setText(QString::number(this->hero->getGetHit()));
    this->ui->heroLifeStealLabel->setText(QString("%1%").arg((this->hero->getLifeSteal() * 100 + 64) >> 7));
    this->ui->heroManaStealLabel->setText(QString("%1%").arg((this->hero->getManaSteal() * 100 + 64) >> 7));
    this->ui->heroArrowVelBonusLabel->setText(QString::number(this->hero->getArrowVelBonus()));
    this->ui->heroHitChanceLabel->setText(QString("%1%").arg(this->hero->getHitChance()));
    this->ui->heroCritChanceLabel->setText(QString("%1%").arg(this->hero->getCritChance() * 100 / 200));

    displayDamage(this->ui->heroTotalDamLabel, this->hero->getTotalMinDam(), this->hero->getTotalMaxDam());
    displayDamage(this->ui->heroSlashDamLabel, this->hero->getSlMinDam(), this->hero->getSlMaxDam());
    displayDamage(this->ui->heroBluntDamLabel, this->hero->getBlMinDam(), this->hero->getBlMaxDam());
    displayDamage(this->ui->heroPierceDamLabel, this->hero->getPcMinDam(), this->hero->getPcMaxDam());
    displayDamage(this->ui->heroChargeDamLabel, this->hero->getChMinDam(), this->hero->getChMaxDam());
    displayDamage(this->ui->heroFireDamLabel, this->hero->getFMinDam(), this->hero->getFMaxDam());
    displayDamage(this->ui->heroLightningDamLabel, this->hero->getLMinDam(), this->hero->getLMaxDam());
    displayDamage(this->ui->heroMagicDamLabel, this->hero->getMMinDam(), this->hero->getMMaxDam());
    displayDamage(this->ui->heroAcidDamLabel, this->hero->getAMinDam(), this->hero->getAMaxDam());

    int flags = this->hero->getItemFlags();
    this->ui->heroDrainLineCheckBox->setChecked((flags & ISPL_DRAINLIFE) != 0);
    this->ui->heroKnockbackCheckBox->setChecked((flags & ISPL_KNOCKBACK) != 0);
    this->ui->heroPenPhysCheckBox->setChecked((flags & ISPL_PENETRATE_PHYS) != 0);
    this->ui->heroNoBleedCheckBox->setChecked((flags & ISPL_NO_BLEED) != 0);
    this->ui->heroBleedCheckBox->setChecked((flags & ISPL_BLEED) != 0);
    this->ui->heroStunCheckBox->setChecked((flags & ISPL_STUN) != 0);
    this->ui->heroNoManaCheckBox->setChecked((flags & ISPL_NOMANA) != 0);
}

void HeroDetailsWidget::on_gameHellfireCheckBox_clicked()
{
    IsHellfireGame = this->ui->gameHellfireCheckBox->isChecked();

    this->hero->setHellfire(IsHellfireGame);

    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_gameDifficultyComboBox_activated(int index)
{
    gnDifficulty = index;

    this->hero->update(); // update resists

    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_heroNameEdit_returnPressed()
{
    QString name = this->ui->heroNameEdit->text();

    this->hero->setName(name);
    this->ui->heroNameEdit->setText(this->hero->getName());

    this->on_heroNameEdit_escPressed();
}

void HeroDetailsWidget::on_heroNameEdit_escPressed()
{
    // update heroNameEdit
    this->updateFields();
    this->ui->heroNameEdit->clearFocus();
}

void HeroDetailsWidget::on_heroClassComboBox_activated(int index)
{
    this->hero->setClass(index);

    if (!IsHellfireGame && this->hero->isHellfire()) {
        IsHellfireGame = true;
    }

    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_heroDecLevelButton_clicked()
{
    this->hero->setLevel(this->hero->getLevel() - 1);
    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_heroIncLevelButton_clicked()
{
    this->hero->setLevel(this->hero->getLevel() + 1);
    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_heroLevelEdit_returnPressed()
{
    int level = this->ui->heroLevelEdit->text().toShort();

    this->hero->setLevel(level);

    this->on_heroLevelEdit_escPressed();
}

void HeroDetailsWidget::on_heroLevelEdit_escPressed()
{
    // update heroLevelEdit
    dMainWindow().updateWindow();
    this->ui->heroLevelEdit->clearFocus();
}

void HeroDetailsWidget::on_heroRankEdit_returnPressed()
{
    int rank = this->ui->heroRankEdit->text().toShort();

    if (rank >= 0 && rank <= 3) {
        this->hero->setRank(rank);
    }

    this->on_heroRankEdit_escPressed();
}

void HeroDetailsWidget::on_heroRankEdit_escPressed()
{
    // update heroRankEdit
    this->updateFields();
    this->ui->heroRankEdit->clearFocus();
}

void HeroDetailsWidget::on_heroSkillsButton_clicked()
{
    dMainWindow().heroSkillsClicked();
}

void HeroDetailsWidget::on_heroMonstersButton_clicked()
{
    dMainWindow().heroMonstersClicked();
}

void HeroDetailsWidget::on_heroDecLifeButton_clicked()
{
    this->hero->decLife();
    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_heroRestoreLifeButton_clicked()
{
    this->hero->restoreLife();
    this->updateFields();
}

void HeroDetailsWidget::on_heroSubStrengthButton_clicked()
{
    this->hero->subStrength();
    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_heroStrengthEdit_returnPressed()
{
    int str = this->ui->heroStrengthEdit->text().toInt();

    this->hero->setStrength(str);

    this->on_heroStrengthEdit_escPressed();
}

void HeroDetailsWidget::on_heroStrengthEdit_escPressed()
{
    // update heroStrengthEdit
    dMainWindow().updateWindow();
    this->ui->heroStrengthEdit->clearFocus();
}

void HeroDetailsWidget::on_heroAddStrengthButton_clicked()
{
    this->hero->addStrength();
    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_heroSubDexterityButton_clicked()
{
    this->hero->subDexterity();
    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_heroDexterityEdit_returnPressed()
{
    int dex = this->ui->heroDexterityEdit->text().toInt();

    this->hero->setDexterity(dex);

    this->on_heroDexterityEdit_escPressed();
}

void HeroDetailsWidget::on_heroDexterityEdit_escPressed()
{
    // update heroDexterityEdit
    dMainWindow().updateWindow();
    this->ui->heroDexterityEdit->clearFocus();
}

void HeroDetailsWidget::on_heroAddDexterityButton_clicked()
{
    this->hero->addDexterity();
    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_heroSubMagicButton_clicked()
{
    this->hero->subMagic();
    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_heroMagicEdit_returnPressed()
{
    int mag = this->ui->heroMagicEdit->text().toInt();

    this->hero->setMagic(mag);

    this->on_heroMagicEdit_escPressed();
}

void HeroDetailsWidget::on_heroMagicEdit_escPressed()
{
    // update heroMagicEdit
    dMainWindow().updateWindow();
    this->ui->heroMagicEdit->clearFocus();
}

void HeroDetailsWidget::on_heroAddMagicButton_clicked()
{
    this->hero->addMagic();
    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_heroSubVitalityButton_clicked()
{
    this->hero->subVitality();
    dMainWindow().updateWindow();
}

void HeroDetailsWidget::on_heroVitalityEdit_returnPressed()
{
    int vit = this->ui->heroVitalityEdit->text().toInt();

    this->hero->setVitality(vit);

    this->on_heroVitalityEdit_escPressed();
}

void HeroDetailsWidget::on_heroVitalityEdit_escPressed()
{
    // update heroVitalityEdit
    dMainWindow().updateWindow();
    this->ui->heroVitalityEdit->clearFocus();
}

void HeroDetailsWidget::on_heroAddVitalityButton_clicked()
{
    this->hero->addVitality();
    dMainWindow().updateWindow();
}
