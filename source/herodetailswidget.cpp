#include "herodetailswidget.h"

#include <QApplication>
#include <QMessageBox>
#include <QString>

#include "celview.h"
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
}

HeroDetailsWidget::~HeroDetailsWidget()
{
    delete ui;
}

void HeroDetailsWidget::initialize(D1Hero *h)
{
    this->hero = h;

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
            label->setText(QString("%1-%2").arg(minDam).arg(maxDam));
        else
            label->setText(QString("%1").arg(minDam));
    } else {
        label->setText(QString("-"));
    }
}

void HeroDetailsWidget::updateFields()
{
    int hc, bv;
    QLabel *label;

    QObject *view = this->parent();
    CelView *celView = qobject_cast<CelView *>(view);
    if (celView != nullptr) {
        celView->updateLabel();
    }
    // this->updateLabel();

    // set texts
    this->ui->heroNameEdit->setText(this->hero->getName());

    hc = this->hero->getClass();
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

    label = this->ui->heroStrengthLabel;
    label->setText(QString::number(this->hero->getStrength()));
    bv = this->hero->getBaseStrength();
    label->setToolTip(QString::number(bv));
    this->ui->heroSubStrengthButton->setEnabled(bv > StrengthTbl[hc]);
    label = this->ui->heroDexterityLabel;
    label->setText(QString::number(this->hero->getDexterity()));
    bv = this->hero->getBaseDexterity();
    label->setToolTip(QString::number(bv));
    this->ui->heroSubDexterityButton->setEnabled(bv > DexterityTbl[hc]);
    label = this->ui->heroMagicLabel;
    label->setText(QString::number(this->hero->getMagic()));
    bv = this->hero->getBaseMagic();
    label->setToolTip(QString::number(bv));
    this->ui->heroSubMagicButton->setEnabled(bv > MagicTbl[hc]);
    label = this->ui->heroVitalityLabel;
    label->setText(QString::number(this->hero->getVitality()));
    bv = this->hero->getBaseVitality();
    label->setToolTip(QString::number(bv));
    this->ui->heroSubVitalityButton->setEnabled(bv > VitalityTbl[hc]);

    label = this->ui->heroLifeLabel;
    label->setText(QString::number(this->hero->getLife()));
    label->setToolTip(QString::number(this->hero->getBaseLife()));
    label = this->ui->heroManaLabel;
    label->setText(QString::number(this->hero->getMana()));
    label->setToolTip(QString::number(this->hero->getBaseMana()));

    this->ui->heroMagicResistLabel->setText(QString("%1%").arg(this->hero->getMagicResist()));
    this->ui->heroFireResistLabel->setText(QString("%1%").arg(this->hero->getFireResist()));
    this->ui->heroLightningResistLabel->setText(QString("%1%").arg(this->hero->getLightningResist()));
    this->ui->heroAcidResistLabel->setText(QString("%1%").arg(this->hero->getAcidResist()));

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
    displayDamage(this->ui->heroSlashDamLabel, this->hero->getSlMaxDam(), this->hero->getSlMaxDam());
    displayDamage(this->ui->heroBluntDamLabel, this->hero->getBlMinDam(), this->hero->getBlMaxDam());
    displayDamage(this->ui->heroPierceDamLabel, this->hero->getPcMinDam(), this->hero->getPcMaxDam());
    displayDamage(this->ui->heroChargeDamLabel, this->hero->getChMinDam(), this->hero->getChMaxDam());
    displayDamage(this->ui->heroFireDamLabel, this->hero->getFMinDam(), this->hero->getFMaxDam());
    displayDamage(this->ui->heroLightningDamLabel, this->hero->getLMinDam(), this->hero->getLMaxDam());
    displayDamage(this->ui->heroMagicDamLabel, this->hero->getMMinDam(), this->hero->getMMaxDam());
    displayDamage(this->ui->heroAcidDamLabel, this->hero->getAMinDam(), this->hero->getAMaxDam());
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
    this->updateFields();
}

void HeroDetailsWidget::on_heroDecLevelButton_clicked()
{
    this->hero->setLevel(this->hero->getLevel() - 1);
    this->updateFields();
}

void HeroDetailsWidget::on_heroIncLevelButton_clicked()
{
    this->hero->setLevel(this->hero->getLevel() + 1);
    this->updateFields();
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
    this->updateFields();
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
    this->updateFields();
}

void HeroDetailsWidget::on_heroRestoreLifeButton_clicked()
{
    this->hero->restoreLife();
    this->updateFields();
}

void HeroDetailsWidget::on_heroAddStrengthButton_clicked()
{
    this->hero->addStrength();
    this->updateFields();
}

void HeroDetailsWidget::on_heroAddDexterityButton_clicked()
{
    this->hero->addDexterity();
    this->updateFields();
}

void HeroDetailsWidget::on_heroAddMagicButton_clicked()
{
    this->hero->addMagic();
    this->updateFields();
}

void HeroDetailsWidget::on_heroAddVitalityButton_clicked()
{
    this->hero->addVitality();
    this->updateFields();
}

void HeroDetailsWidget::on_heroSubStrengthButton_clicked()
{
    this->hero->subStrength();
    this->updateFields();
}

void HeroDetailsWidget::on_heroSubDexterityButton_clicked()
{
    this->hero->subDexterity();
    this->updateFields();
}

void HeroDetailsWidget::on_heroSubMagicButton_clicked()
{
    this->hero->subMagic();
    this->updateFields();
}

void HeroDetailsWidget::on_heroSubVitalityButton_clicked()
{
    this->hero->subVitality();
    this->updateFields();
}
