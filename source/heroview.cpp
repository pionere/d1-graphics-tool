#include "heroview.h"

#include <algorithm>

#include <QDebug>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QImageReader>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>

#include "config.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_heroview.h"

#include "dungeon/all.h"

HeroScene::HeroScene(QWidget *v)
    : QGraphicsScene(v)
{
}

void HeroScene::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control && !this->leftMousePressed) {
        this->panning = true;
        this->views()[0]->setDragMode(QGraphicsView::ScrollHandDrag);
        return;
    }
    QGraphicsScene::keyPressEvent(event);
}

void HeroScene::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control && !this->leftMousePressed) {
        this->panning = false;
        this->views()[0]->setDragMode(QGraphicsView::NoDrag);
        return;
    }
    QGraphicsScene::keyReleaseEvent(event);
}

void HeroScene::mouseEvent(QGraphicsSceneMouseEvent *event, int flags)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }

    if (this->panning) {
        if (!(flags & DOUBLE_CLICK)) {
            QGraphicsScene::mousePressEvent(event);
        }
        return;
    }

    this->leftMousePressed = true;

    QPointF scenePos = event->scenePos();
    QPoint currPos = QPoint(scenePos.x(), scenePos.y());
    // qDebug() << QStringLiteral("Mouse event at: %1:%2").arg(currPos.x()).arg(currPos.y());
    if (!(flags & FIRST_CLICK) && this->lastPos == currPos) {
        return;
    }
    this->lastPos = currPos;

    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        flags |= SHIFT_CLICK;
    }
    // emit this->framePixelClicked(this->lastPos, first);
    QObject *view = this->parent();
    HeroView *heroView = qobject_cast<HeroView *>(view);
    if (heroView != nullptr) {
        heroView->framePixelClicked(currPos, flags);
        return;
    }
}

void HeroScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    this->mouseEvent(event, FIRST_CLICK);
}

void HeroScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->leftMousePressed = false;
        if (this->panning) {
            this->panning = (event->modifiers() & Qt::ControlModifier) != 0;
            this->views()[0]->setDragMode(this->panning ? QGraphicsView::ScrollHandDrag : QGraphicsView::NoDrag);
        }
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

void HeroScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    this->mouseEvent(event, FIRST_CLICK | DOUBLE_CLICK);
}

void HeroScene::mouseHoverEvent(QGraphicsSceneMouseEvent *event)
{
    // emit this->framePixelHovered(this->lastPos);
    QPointF scenePos = event->scenePos();
    QPoint currPos = QPoint(scenePos.x(), scenePos.y());
    QObject *view = this->parent();
    HeroView *heroView = qobject_cast<HeroView *>(view);
    if (heroView != nullptr) {
        heroView->framePixelHovered(currPos);
        return;
    }
}

void HeroScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (this->panning) {
        QGraphicsScene::mouseMoveEvent(event);
        return;
    }
    if (event->buttons() != Qt::NoButton) {
        this->mouseEvent(event, false);
    }
    this->mouseHoverEvent(event);
}

/*void HeroScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    this->dragMoveEvent(event);
}

void HeroScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (MainWindow::hasHeroUrl(event->mimeData())) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void HeroScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    event->acceptProposedAction();

    QStringList filePaths;
    for (const QUrl &url : event->mimeData()->urls()) {
        filePaths.append(url.toLocalFile());
    }
    dMainWindow().openFiles(filePaths);
}*/

HeroView::HeroView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HeroView())
{
    this->ui->setupUi(this);
    this->ui->heroGraphicsView->setScene(&this->heroScene);
    this->ui->heroGraphicsView->setMouseTracking(true);

    // this->mainHeroDetails = new HeroDetailsWidget(this);
    // this->ui->heroVBoxLayout->addWidget(this->mainHeroDetails);

    // If a pixel of the frame was clicked get pixel color index and notify the palette widgets
    // QObject::connect(&this->heroScene, &HeroScene::framePixelClicked, this, &HeroView::framePixelClicked);
    // QObject::connect(&this->heroScene, &HeroScene::framePixelHovered, this, &HeroView::framePixelHovered);

    // connect esc events of LineEditWidgets
    // QObject::connect(this->ui->heroNameEdit, SIGNAL(cancel_signal()), this, SLOT(on_heroNameEdit_escPressed()));
    // QObject::connect(this->ui->heroLevelEdit, SIGNAL(cancel_signal()), this, SLOT(on_heroLevelEdit_escPressed()));
    // QObject::connect(this->ui->heroRankEdit, SIGNAL(cancel_signal()), this, SLOT(on_heroRankEdit_escPressed()));

    // setup context menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));

    setAcceptDrops(true);
}

HeroView::~HeroView()
{
    delete ui;
}

void HeroView::initialize(D1Pal *p, D1Hero *h, bool bottomPanelHidden)
{
    this->pal = p;
    // this->hero = h;
    this->hoverItem = INVITEM_NONE;

    this->ui->bottomPanel->setVisible(!bottomPanelHidden);

    // this->ui->mainHeroDetails->initialize(h);
    this->setHero(h);
    // this->updateFields();
}

void HeroView::setPal(D1Pal *p)
{
    this->pal = p;
}

void HeroView::setHero(D1Hero *h)
{
    this->hero = h;
    this->ui->mainHeroDetails->initialize(h);
}

void HeroView::setLabelContent(QLabel *label, const QString &filePath, bool modified)
{
    label->setToolTip(filePath);

    QFileInfo fileInfo(filePath);
    QString labelText = fileInfo.fileName();
    if (modified) {
        labelText += "*";
    }
    label->setText(labelText);
}

// Displaying CEL file path information
void HeroView::updateLabel()
{
    HeroView::setLabelContent(this->ui->heroLabel, this->hero->getFilePath(), this->hero->isModified());
}

/*static void displayDamage(QLabel *label, int minDam, int maxDam)
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

/*void HeroView::updateFields()
{
    int hc, bv;
    QLabel *label;
    this->updateLabel();

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
}*/

HeroScene *HeroView::getHeroScene() const
{
    return const_cast<HeroScene *>(&this->heroScene);
}

int HeroView::invItemIdx(QPoint &pos) const
{
    constexpr int gnWndInvX = 0;
    constexpr int gnWndInvY = 0;
    pos -= QPoint(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    int i = pos.x(), j = pos.y(), r;

    for (r = 0; r < SLOTXY_CHEST_LAST; r++) {
        if (POS_IN_RECT(i, j,
            gnWndInvX + InvRect[r].X, gnWndInvY + InvRect[r].Y - INV_SLOT_SIZE_PX,
            INV_SLOT_SIZE_PX + 1, INV_SLOT_SIZE_PX + 1)) {
            static_assert((int)SLOT_HEAD == (int)INVITEM_HEAD, "SLOT - INVITEM match is necessary in framePixelClicked I.");
            static_assert((int)SLOT_RING_LEFT == (int)INVITEM_RING_LEFT, "SLOT - INVITEM match is necessary in framePixelClicked II.");
            static_assert((int)SLOT_RING_RIGHT == (int)INVITEM_RING_RIGHT, "SLOT - INVITEM match is necessary in framePixelClicked III.");
            static_assert((int)SLOT_AMULET == (int)INVITEM_AMULET, "SLOT - INVITEM match is necessary in framePixelClicked IV.");
            static_assert((int)SLOT_HAND_LEFT == (int)INVITEM_HAND_LEFT, "SLOT - INVITEM match is necessary in framePixelClicked V.");
            static_assert((int)SLOT_HAND_RIGHT == (int)INVITEM_HAND_RIGHT, "SLOT - INVITEM match is necessary in framePixelClicked VI.");
            static_assert((int)SLOT_CHEST == (int)INVITEM_CHEST, "SLOT - INVITEM match is necessary in framePixelClicked VII.");
            return InvSlotTbl[r];
        }
    }
    return INVITEM_NONE;
}

void HeroView::framePixelClicked(const QPoint &pos, int flags)
{
    QPoint tpos = pos;
    int idx = this->invItemIdx(tpos);
    if (idx != INVITEM_NONE) {
        dMainWindow().heroItemClicked(idx);
    }
}

bool HeroView::framePos(const QPoint &p) const
{
    constexpr int INV_WIDTH = SPANEL_WIDTH; // same as D1Hero::getEquipmentImage
    constexpr int INV_HEIGHT = 178;

    if (p.x() < 0 || p.x() >= INV_WIDTH)
        return false;
    if (p.y() < 0 || p.y() >= INV_HEIGHT)
        return false;
    return true;
}

void HeroView::framePixelHovered(const QPoint &pos)
{
    QPoint tpos = pos;
    int idx = this->invItemIdx(tpos);
    if (idx != this->hoverItem) {
        this->hoverItem = idx;
        this->displayFrame();
    }
    if (this->framePos(tpos)) {
        dMainWindow().pointHovered(tpos);
        return;
    }

    tpos.setX(UINT_MAX);
    tpos.setY(UINT_MAX);
    dMainWindow().pointHovered(tpos);
}

void HeroView::displayFrame()
{
    // set context-fields
    this->ui->gameHellfireCheckBox->setChecked(this->hero->isHellfire());
    this->ui->gameHellfireCheckBox->setEnabled(D1Hero::isStandardClass(this->hero->getClass()));
    this->ui->gameMultiCheckBox->setChecked(this->hero->isMulti());
    this->ui->gameDifficultyComboBox->setCurrentIndex(gnDifficulty);

    // LogErrorF("HeroView::displayFrame 0");
    // this->updateFields();
    this->updateLabel();
    this->ui->mainHeroDetails->displayFrame();
    // LogErrorF("HeroView::displayFrame 1");
    this->heroScene.clear();
    // LogErrorF("HeroView::displayFrame 2");
    // Getting the current frame to display
    QImage invFrame = this->hero->getEquipmentImage(this->hoverItem);
    // LogErrorF("HeroView::displayFrame 3 %dx%d", invFrame.width(), invFrame.height());
    this->heroScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    // Building background of the width/height of the CEL frame
    QImage invFrameBackground = QImage(invFrame.width(), invFrame.height(), QImage::Format_ARGB32);
    invFrameBackground.fill(QColor(Config::getGraphicsTransparentColor()));

    // Resize the scene rectangle to include some padding around the CEL frame
    this->heroScene.setSceneRect(0, 0,
        CEL_SCENE_MARGIN + invFrame.width() + CEL_SCENE_MARGIN,
        CEL_SCENE_MARGIN + invFrame.height() + CEL_SCENE_MARGIN);
    // ui->heroGraphicsView->adjustSize();

    // Add the backgrond and CEL frame while aligning it in the center
    this->heroScene.addPixmap(QPixmap::fromImage(invFrameBackground))
        ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    this->heroScene.addPixmap(QPixmap::fromImage(invFrame))
        ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    // LogErrorF("HeroView::displayFrame 4");
    // Notify PalView that the frame changed (used to refresh palette widget)
    // emit this->frameRefreshed();
}

void HeroView::toggleBottomPanel()
{
    this->ui->bottomPanel->setVisible(this->ui->bottomPanel->isHidden());
}

ItemAction::ItemAction(D1Hero *h, int ii, const QString &text, int pi)
    : QAction()
    , hero(h)
    , ii(ii)
    , pi(pi)
{
    setText(text);
    QObject::connect(this, SIGNAL(triggered()), this, SLOT(on_action_triggered()));
}

void ItemAction::on_action_triggered()
{
    this->hero->swapInvItem(this->ii, this->pi);

    dMainWindow().updateWindow();
}

void HeroView::on_gameHellfireCheckBox_clicked()
{
    IsHellfireGame = this->ui->gameHellfireCheckBox->isChecked();

    this->hero->setHellfire(IsHellfireGame);

    dMainWindow().updateWindow();
}

void HeroView::on_gameMultiCheckBox_clicked()
{
    IsMultiGame = this->ui->gameMultiCheckBox->isChecked();

    this->hero->setMulti(IsMultiGame);

    dMainWindow().updateWindow();
}

void HeroView::on_gameSpeedComboBox_activated(int index)
{
    switch (index) {
    case 0: gnTicksRate = SPEED_NORMAL;  break;
    case 1: gnTicksRate = SPEED_FAST;    break;
    case 2: gnTicksRate = SPEED_FASTER;  break;
    case 3: gnTicksRate = SPEED_FASTEST; break;
    }
    dMainWindow().updateWindow();
}

void HeroView::on_gameDifficultyComboBox_activated(int index)
{
    gnDifficulty = index;

    this->hero->update(); // update resists

    dMainWindow().updateWindow();
}

void HeroView::on_heroSkillsButton_clicked()
{
    dMainWindow().heroSkillsClicked();
}

void HeroView::on_heroMonstersButton_clicked()
{
    dMainWindow().heroMonstersClicked();
}

void HeroView::on_heroPvPButton_clicked()
{
    dMainWindow().heroPvPClicked();
}

void HeroView::ShowContextMenu(const QPoint &pos)
{
    int ii = this->hoverItem;
    if (ii != INVITEM_NONE) {
        QList<QAction*> actions;
        ItemAction *action;
        const ItemStruct* is;

        is = this->hero->item(ii);

        action = new ItemAction(this->hero, ii, tr("None"), INVITEM_NONE);
        actions.append(action);
        if (is->_itype == ITYPE_NONE) {
            action->setChecked(true);
            action->setDisabled(true);
        } else {
            action = new ItemAction(this->hero, ii, ItemName(is), ii);
            action->setChecked(true);
            action->setDisabled(true);
            actions.append(action);
        }

        for (int i = INVITEM_INV_FIRST; i < NUM_INVELEM; i++) {
            is = this->hero->item(i);
            if (is->_itype == ITYPE_NONE || is->_itype == ITYPE_PLACEHOLDER) {
                continue;
            }
            switch (ii) {
            case INVITEM_HEAD:
                if (is->_iLoc != ILOC_HELM)
                    continue;
                break;
            case INVITEM_RING_LEFT:
            case INVITEM_RING_RIGHT:
                if (is->_iLoc != ILOC_RING)
                    continue;
                break;
            case INVITEM_AMULET:
                if (is->_iLoc != ILOC_AMULET)
                    continue;
                break;
            case INVITEM_HAND_LEFT:
                if (is->_iLoc != ILOC_ONEHAND && is->_iLoc != ILOC_TWOHAND)
                    continue;
                break;
            case INVITEM_HAND_RIGHT:
                if (is->_iLoc != ILOC_ONEHAND)
                    continue;
                break;
            case INVITEM_CHEST:
                if (is->_iLoc != ILOC_ARMOR)
                    continue;
                break;
            }
            action = new ItemAction(this->hero, ii, ItemName(is), i);
            actions.append(action);
        }
        // if (!actions.isEmpty()) {
            QMenu contextMenu(this);
            // contextMenu.setToolTipsVisible(true);
            contextMenu.addActions(actions);
            contextMenu.exec(mapToGlobal(pos));
        // }

        qDeleteAll(actions);

        this->hoverItem = INVITEM_NONE;
        this->displayFrame();
    }
}


/*void HeroView::dragEnterEvent(QDragEnterEvent *event)
{
    this->dragMoveEvent(event);
}

void HeroView::dragMoveEvent(QDragMoveEvent *event)
{
    if (MainWindow::hasImageUrl(event->mimeData())) {
        event->acceptProposedAction();
    }
}

void HeroView::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    QStringList filePaths;
    for (const QUrl &url : event->mimeData()->urls()) {
        filePaths.append(url.toLocalFile());
    }
    // try to insert as frames
    // dMainWindow().openImageFiles(IMAGE_FILE_MODE::AUTO, filePaths, false);
}*/
