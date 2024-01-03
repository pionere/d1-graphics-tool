#include "dungeonsubtilewidget.h"

#include <QApplication>
#include <QCursor>
#include <QGraphicsView>
#include <QImage>
#include <QList>
#include <QMessageBox>
#include <QString>

#include "config.h"
#include "levelcelview.h"
#include "ui_dungeonsubtilewidget.h"

#include "dungeon/all.h"

DungeonSubtileWidget::DungeonSubtileWidget(LevelCelView *parent)
    : QFrame(parent)
    , ui(new Ui::DungeonSubtileWidget())
{
    this->ui->setupUi(this);

    QLayout *layout = this->ui->centerButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogListView, tr("Move"), this, &DungeonSubtileWidget::on_movePushButtonClicked);
    layout = this->ui->rightButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogCloseButton, tr("Close"), this, &DungeonSubtileWidget::on_closePushButtonClicked);

    // cache the active graphics view
    QList<QGraphicsView *> views = parent->getCelScene()->views();
    this->graphView = views[0];
}

DungeonSubtileWidget::~DungeonSubtileWidget()
{
    delete ui;
}

void DungeonSubtileWidget::initialize(int x, int y)
{
    this->dunX = x;
    this->dunY = y;

    this->dungeonModified();
}

void DungeonSubtileWidget::show()
{
    this->resetPos();
    QFrame::show();

    this->setFocus(); // otherwise the widget does not receive keypress events...
    // update the view
    this->dungeonModified();
}

void DungeonSubtileWidget::hide()
{
    if (this->moving) {
        this->stopMove();
    }

    QFrame::hide();
}

static QString MonLeaderText(BYTE flag, BYTE packsize)
{
    QString result;
    switch (flag) {
    case MLEADER_NONE:
        result = "-";
        break;
    case MLEADER_PRESENT:
        result = QApplication::tr("Minion");
        break;
    case MLEADER_AWAY:
        result = QApplication::tr("Lost");
        break;
    case MLEADER_SELF:
        // result = QApplication::tr("Leader(%1)").arg(packsize);
        result = QApplication::tr("Leader");
        break;
    default:
        result = "???";
        break;
    }
    return result;
}

static const char* MonResistText(unsigned resist, unsigned idx)
{
    const char* result;
    switch ((resist >> idx) & 3) {
    case MORT_NONE:
        result = "-";
        break;
    case MORT_PROTECTED:
        result = "Protected";
        break;
    case MORT_RESIST:
        result = "Resist";
        break;
    case MORT_IMMUNE:
        result = "Immune";
        break;
    default:
        result = "???";
        break;
    }
    return result;
}

void DungeonSubtileWidget::dungeonModified()
{
    if (this->isHidden())
        return;
    MonsterStruct *mon = GetMonsterAt(this->dunX, this->dunY);
    if (mon != nullptr) {
        const char* color = "black";
        if (mon->_mNameColor == COL_BLUE)
            color = "blue";
        else if (mon->_mNameColor == COL_GOLD)
            color = "orange";
        this->ui->monsterName->setText(QString("<u><font color='%1'>%2</font></u>").arg(color).arg(mon->_mName));
        this->ui->monsterLevel->setText(tr("(Level %1)").arg(mon->_mLevel));
        this->ui->monsterExp->setText(QString::number(mon->_mExp));
        this->ui->monsterStatus->setText(MonLeaderText(mon->_mleaderflag, mon->_mpacksize));

        // MonsterAI _mAI;
        this->ui->monsterInt->setText(QString::number(mon->_mAI.aiInt));

        this->ui->monsterHit->setText(QString::number(mon->_mHit));
        this->ui->monsterDamage->setText(QString("%1 - %2").arg(mon->_mMinDamage).arg(mon->_mMaxDamage));
        this->ui->monsterHit2->setText(QString::number(mon->_mHit2));
        this->ui->monsterDamage2->setText(QString("%1 - %2").arg(mon->_mMinDamage2).arg(mon->_mMaxDamage2));
        this->ui->monsterMagic->setText(QString::number(mon->_mMagic));

        this->ui->monsterHp->setText(QString::number(mon->_mmaxhp >> 6));
        this->ui->monsterArmorClass->setText(QString::number(mon->_mArmorClass));
        this->ui->monsterEvasion->setText(QString::number(mon->_mEvasion));

        unsigned res = mon->_mMagicRes;
        this->ui->monsterResSlash->setText(MonResistText(res, MORS_IDX_SLASH));
        this->ui->monsterResBlunt->setText(MonResistText(res, MORS_IDX_BLUNT));
        this->ui->monsterResPunct->setText(MonResistText(res, MORS_IDX_PUNCTURE));
        this->ui->monsterResFire->setText(MonResistText(res, MORS_IDX_FIRE));
        this->ui->monsterResLight->setText(MonResistText(res, MORS_IDX_LIGHTNING));
        this->ui->monsterResMagic->setText(MonResistText(res, MORS_IDX_MAGIC));
        this->ui->monsterResAcid->setText(MonResistText(res, MORS_IDX_ACID));

        unsigned flags = mon->_mFlags;
        this->ui->monsterHiddenCheckBox->setChecked((flags & MFLAG_HIDDEN) != 0);
        this->ui->monsterGargCheckBox->setChecked((flags & MFLAG_GARG_STONE) != 0);
        this->ui->monsterStealCheckBox->setChecked((flags & MFLAG_LIFESTEAL) != 0);
        this->ui->monsterOpenCheckBox->setChecked((flags & MFLAG_CAN_OPEN_DOOR) != 0);
        this->ui->monsterSearchCheckBox->setChecked((flags & MFLAG_SEARCH) != 0);
        this->ui->monsterNoStoneCheckBox->setChecked((flags & MFLAG_NOSTONE) != 0);
        this->ui->monsterBleedCheckBox->setChecked((flags & MFLAG_CAN_BLEED) != 0);
        this->ui->monsterNoDropCheckBox->setChecked((flags & MFLAG_NODROP) != 0);
        this->ui->monsterKnockbackCheckBox->setChecked((flags & MFLAG_KNOCKBACK) != 0);

        this->adjustSize(); // not sure why this is necessary...
    } else {
        this->ui->monsterName->setText("");
        this->ui->monsterLevel->setText("");
        this->ui->monsterExp->setText("");
        this->ui->monsterStatus->setText("");

        this->ui->monsterInt->setText("");

        this->ui->monsterHit->setText("");
        this->ui->monsterDamage->setText("");
        this->ui->monsterHit2->setText("");
        this->ui->monsterDamage2->setText("");
        this->ui->monsterMagic->setText("");

        this->ui->monsterHp->setText("");
        this->ui->monsterArmorClass->setText("");
        this->ui->monsterEvasion->setText("");

        this->ui->monsterResSlash->setText("");
        this->ui->monsterResBlunt->setText("");
        this->ui->monsterResPunct->setText("");
        this->ui->monsterResFire->setText("");
        this->ui->monsterResLight->setText("");
        this->ui->monsterResMagic->setText("");
        this->ui->monsterResAcid->setText("");

        this->ui->monsterHiddenCheckBox->setChecked(false);
        this->ui->monsterGargCheckBox->setChecked(false);
        this->ui->monsterStealCheckBox->setChecked(false);
        this->ui->monsterOpenCheckBox->setChecked(false);
        this->ui->monsterSearchCheckBox->setChecked(false);
        this->ui->monsterNoStoneCheckBox->setChecked(false);
        this->ui->monsterBleedCheckBox->setChecked(false);
        this->ui->monsterNoDropCheckBox->setChecked(false);
        this->ui->monsterKnockbackCheckBox->setChecked(false);
    }
}

void DungeonSubtileWidget::on_closePushButtonClicked()
{
    this->hide();
}

void DungeonSubtileWidget::resetPos()
{
    if (!this->moved) {
        QSize viewSize = this->graphView->frameSize();
        QPoint viewBottomRight = this->graphView->mapToGlobal(QPoint(viewSize.width(), viewSize.height()));
        QSize mySize = this->frameSize();
        QPoint targetPos = viewBottomRight - QPoint(mySize.width(), mySize.height());
        QPoint relPos = this->mapFromGlobal(targetPos);
        QPoint destPos = relPos + this->pos();
        this->move(destPos);
    }
}

void DungeonSubtileWidget::stopMove()
{
    this->setMouseTracking(false);
    this->moving = false;
    this->moved = true;
    this->releaseMouse();
    // this->setCursor(Qt::BlankCursor);
}

void DungeonSubtileWidget::on_movePushButtonClicked()
{
    if (this->moving) { // this->hasMouseTracking()
        this->stopMove();
        return;
    }
    this->grabMouse(QCursor(Qt::ClosedHandCursor));
    this->moving = true;
    this->setMouseTracking(true);
    this->lastPos = QCursor::pos();
    this->setFocus();
    // this->setCursor(Qt::ClosedHandCursor);
}

void DungeonSubtileWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (this->moving) {
            this->stopMove();
        } else {
            this->hide();
        }
        return;
    }

    QFrame::keyPressEvent(event);
}

void DungeonSubtileWidget::mousePressEvent(QMouseEvent *event)
{
    if (this->moving) {
        this->stopMove();
        return;
    }

    QFrame::mouseMoveEvent(event);
}

void DungeonSubtileWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (this->moving) {
        QPoint currPos = QCursor::pos();
        QPoint wndPos = this->pos();
        wndPos += currPos - this->lastPos;
        this->lastPos = currPos;
        this->move(wndPos);
        // return;
    }

    QFrame::mouseMoveEvent(event);
}