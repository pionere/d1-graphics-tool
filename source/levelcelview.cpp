#include "levelcelview.h"

#include <algorithm>
#include <set>
#include <vector>

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QImageReader>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPen>
#include <QRectF>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTimer>

#include "builderwidget.h"
#include "config.h"
#include "d1image.h"
#include "d1pcx.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "pushbuttonwidget.h"
#include "ui_levelcelview.h"
#include "upscaler.h"

#include "dungeon/all.h"

LevelCelView::LevelCelView(QWidget *parent, QUndoStack *us)
    : QWidget(parent)
    , ui(new Ui::LevelCelView())
    , undoStack(us)
{
    this->ui->setupUi(this);
    this->ui->celGraphicsView->setScene(&this->celScene);
    this->ui->celGraphicsView->setMouseTracking(true);
    this->on_zoomEdit_escPressed();
    this->on_playDelayEdit_escPressed();
    this->on_dunZoomEdit_escPressed();
    this->on_dunPlayDelayEdit_escPressed();
    this->ui->tilesTabs->addTab(&this->tabTileWidget, tr("Tile properties"));
    this->ui->tilesTabs->addTab(&this->tabSubtileWidget, tr("Subtile properties"));
    this->ui->tilesTabs->addTab(&this->tabFrameWidget, tr("Frame properties"));

    QLayout *layout = this->ui->tilesetButtonsHorizontalLayout;
    PushButtonWidget *btn = PushButtonWidget::addButton(this, layout, QStyle::SP_DialogResetButton, tr("Start drawing"), &dMainWindow(), &MainWindow::on_actionToggle_Painter_triggered);
    layout->setAlignment(btn, Qt::AlignRight);
    setTabOrder(this->ui->dunPlayComboBox, btn);
    this->viewBtn = PushButtonWidget::addButton(this, layout, QStyle::SP_ArrowRight, tr("Switch to dungeon view"), &dMainWindow(), &MainWindow::on_actionToggle_View_triggered);
    setTabOrder(btn, this->viewBtn);
    layout = this->ui->dunButtonsHorizontalLayout;
    btn = PushButtonWidget::addButton(this, layout, QStyle::SP_DialogResetButton, tr("Start building"), &dMainWindow(), &MainWindow::on_actionToggle_Builder_triggered);
    layout->setAlignment(btn, Qt::AlignRight);
    setTabOrder(this->ui->dunPlayComboBox, btn);
    PushButtonWidget *switchBtn = PushButtonWidget::addButton(this, layout, QStyle::SP_ArrowLeft, tr("Switch to tileset view"), &dMainWindow(), &MainWindow::on_actionToggle_View_triggered);
    setTabOrder(btn, switchBtn);

    QGridLayout *gridLayout = this->ui->dungeonLayout;
    btn = PushButtonWidget::addButton(this, gridLayout, 3, 10, QStyle::SP_DialogOkButton, tr("Center Dungeon"), this, &LevelCelView::scrollToCurrent);
    setTabOrder(this->ui->moveDownButton, btn);
    PushButtonWidget *monBtn = PushButtonWidget::addButton(this, gridLayout, 3, 11, QStyle::SP_FileDialogInfoView, tr("Show Subtile Info"), this, &LevelCelView::showSubtileInfo);
    setTabOrder(btn, monBtn);

    // If a pixel of the frame, subtile or tile was clicked get pixel color index and notify the palette widgets
    // QObject::connect(&this->celScene, &CelScene::framePixelClicked, this, &LevelCelView::framePixelClicked);
    // QObject::connect(&this->celScene, &CelScene::framePixelHovered, this, &LevelCelView::framePixelHovered);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->frameIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_frameIndexEdit_escPressed()));
    QObject::connect(this->ui->subtileIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_subtileIndexEdit_escPressed()));
    QObject::connect(this->ui->tileIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_tileIndexEdit_escPressed()));
    QObject::connect(this->ui->minFrameWidthEdit, SIGNAL(cancel_signal()), this, SLOT(on_minFrameWidthEdit_escPressed()));
    QObject::connect(this->ui->minFrameHeightEdit, SIGNAL(cancel_signal()), this, SLOT(on_minFrameHeightEdit_escPressed()));
    QObject::connect(this->ui->zoomEdit, SIGNAL(cancel_signal()), this, SLOT(on_zoomEdit_escPressed()));
    QObject::connect(this->ui->playDelayEdit, SIGNAL(cancel_signal()), this, SLOT(on_playDelayEdit_escPressed()));
    QObject::connect(this->ui->dunZoomEdit, SIGNAL(cancel_signal()), this, SLOT(on_dunZoomEdit_escPressed()));
    QObject::connect(this->ui->dunPlayDelayEdit, SIGNAL(cancel_signal()), this, SLOT(on_dunPlayDelayEdit_escPressed()));
    QObject::connect(this->ui->dungeonDefaultTileLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_dungeonDefaultTileLineEdit_escPressed()));
    QObject::connect(this->ui->dungeonPosLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_dungeonPosLineEdit_escPressed()));
    QObject::connect(this->ui->dunWidthEdit, SIGNAL(cancel_signal()), this, SLOT(on_dunWidthEdit_escPressed()));
    QObject::connect(this->ui->dunHeightEdit, SIGNAL(cancel_signal()), this, SLOT(on_dunHeightEdit_escPressed()));
    QObject::connect(this->ui->dungeonTileLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_dungeonTileLineEdit_escPressed()));
    QObject::connect(this->ui->dungeonSubtileLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_dungeonSubtileLineEdit_escPressed()));
    QObject::connect(this->ui->dungeonItemLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_dungeonItemLineEdit_escPressed()));
    QObject::connect(this->ui->dungeonMonsterLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_dungeonMonsterLineEdit_escPressed()));
    QObject::connect(this->ui->dungeonObjectLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_dungeonObjectLineEdit_escPressed()));
    QObject::connect(this->ui->dungeonRoomLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_dungeonRoomLineEdit_escPressed()));

    // setup context menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));

    setAcceptDrops(true);
}

LevelCelView::~LevelCelView()
{
    delete ui;
}

void LevelCelView::initialize(D1Pal *p, D1Tileset *ts, D1Dun *d, bool bottomPanelHidden)
{
    this->tabTileWidget.initialize(this, this->undoStack);
    this->tabSubtileWidget.initialize(this, this->undoStack);
    this->tabFrameWidget.initialize(this, this->undoStack);

    this->pal = p;
    this->setTileset(ts);
    this->dun = d;

    bool dunMode = d != nullptr;
    this->dunView = dunMode;
    this->viewBtn->setVisible(dunMode);
    // select gridlayout
    this->ui->tilesetWidget->setVisible(!dunMode && !bottomPanelHidden);
    this->ui->dungeonWidget->setVisible(dunMode && !bottomPanelHidden);

    if (dunMode) {
        // add buttons for the custom entities
        QGridLayout *layout = this->ui->dungeonGridLayout;
        PushButtonWidget::addButton(this, layout, 1, 3, QStyle::SP_FileDialogNewFolder, tr("Add Custom Object"), this, &LevelCelView::on_dungeonObjectAddButton_clicked);
        PushButtonWidget::addButton(this, layout, 1, 15, QStyle::SP_FileDialogNewFolder, tr("Add Custom Item"), this, &LevelCelView::on_dungeonItemAddButton_clicked);
        { // add monster button
            QLayout *layout = this->ui->dungeonMonsterHBoxLayout;
            PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogNewFolder, tr("Add Custom Monster"), this, &LevelCelView::on_dungeonMonsterAddButton_clicked);
        }
        // initialize the fields which are not updated
        this->on_dungeonDefaultTileLineEdit_escPressed();
        this->ui->levelTypeComboBox->setCurrentIndex(d->getLevelType());
        this->updateTilesetIcon();
        // prepare the entities
        this->updateEntityOptions();
    }
    this->updateFields();
}

void LevelCelView::setPal(D1Pal *p)
{
    this->pal = p;
}

void LevelCelView::setTileset(D1Tileset *ts)
{
    this->tileset = ts;
    this->gfx = ts->gfx;
    this->cls = ts->cls;
    this->min = ts->min;
    this->til = ts->til;
    this->sla = ts->sla;
    this->tla = ts->tla;

    if (this->currentFrameIndex >= this->gfx->getFrameCount()) {
        this->currentFrameIndex = 0;
    }
    if (this->currentSubtileIndex >= this->min->getSubtileCount()) {
        this->currentSubtileIndex = 0;
    }
    if (this->currentTileIndex >= this->til->getTileCount()) {
        this->currentTileIndex = 0;
    }

    this->tabTileWidget.setTileset(ts);
    this->tabSubtileWidget.setTileset(ts);
    this->tabFrameWidget.setTileset(ts);
}

void LevelCelView::setGfx(D1Gfx *g)
{
    this->gfx = g;

    if (this->currentFrameIndex >= this->gfx->getFrameCount()) {
        this->currentFrameIndex = 0;
    }

    this->tabTileWidget.setGfx(g);
    this->tabSubtileWidget.setGfx(g);
    this->tabFrameWidget.setGfx(g);
}

void LevelCelView::setDungeon(D1Dun *dun)
{
    // stop playback
    if (this->playTimer != 0) {
        this->on_playStopButton_clicked();
    }

    if (this->currentDunPosX >= dun->getWidth() || this->currentDunPosY >= dun->getHeight()) {
        this->currentDunPosX = 0;
        this->currentDunPosY = 0;
    }
    if (this->dun == nullptr) {
        this->initialize(this->pal, this->tileset, dun, this->ui->tilesetWidget->isHidden());
        // TODO: sync with on_actionToggle_View_triggered
        // preserve scroll value
        this->lastVertScrollValue = this->ui->celGraphicsView->verticalScrollBar()->value();
        this->lastHorizScrollValue = this->ui->celGraphicsView->horizontalScrollBar()->value();
        // update zoom
        QString zoom = this->ui->dunZoomEdit->text();
        this->celScene.setZoom(zoom);
    } else {
        this->dun = dun;
    }
}

void LevelCelView::updateTilesetIcon()
{
    // update icon of tileset
    QString tilPath = this->til->getFilePath();
    if (!tilPath.isEmpty()) {
        QFileInfo fileInfo = QFileInfo(tilPath);
        tilPath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName();
        this->ui->tilesetLoadPushButton->setToolTip(tilPath);
        QIcon icon = QApplication::style()->standardIcon(QStyle::SP_DriveCDIcon);
        this->ui->tilesetLoadPushButton->setIcon(icon);
        this->ui->tilesetLoadPushButton->setText("");
    } else {
        this->ui->tilesetLoadPushButton->setToolTip(tr("Select tileset"));
        QIcon icon;
        this->ui->tilesetLoadPushButton->setIcon(icon);
        this->ui->tilesetLoadPushButton->setText("...");
    }
}

void LevelCelView::updateEntityOptions()
{
    QComboBox *comboBox;

    // prepare the comboboxes
    // - objects
    comboBox = this->ui->dungeonObjectComboBox;
    comboBox->hide();
    comboBox->clear();
    comboBox->addItem("", 0);
    const std::vector<CustomObjectStruct> &customObjectTypes = this->dun->getCustomObjectTypes();
    if (!customObjectTypes.empty()) {
        for (const CustomObjectStruct &obj : customObjectTypes) {
            comboBox->addItem(obj.name, obj.type);
        }
        comboBox->insertSeparator(INT_MAX);
    }
    for (int i = 0; i < lengthof(DunObjConvTbl); i++) {
        const DunObjectStruct &obj = DunObjConvTbl[i];
        if (obj.name != nullptr) {
            // filter custom entries
            unsigned n = 0;
            for (; n < customObjectTypes.size(); n++) {
                if (customObjectTypes[n].type == i) {
                    break;
                }
            }
            if (n >= customObjectTypes.size()) {
                comboBox->addItem(obj.name, i);
            }
        }
    }
    comboBox->show();
    // - monsters
    comboBox = this->ui->dungeonMonsterComboBox;
    comboBox->hide();
    comboBox->clear();
    DunMonsterType monType = { 0, false, false };
    comboBox->addItem("", QVariant::fromValue(monType));
    //   - custom monsters
    const std::vector<CustomMonsterStruct> &customMonsterTypes = this->dun->getCustomMonsterTypes();
    if (!customMonsterTypes.empty()) {
        for (const CustomMonsterStruct &mon : customMonsterTypes) {
            comboBox->addItem(mon.name, QVariant::fromValue(mon.type));
        }
        comboBox->insertSeparator(INT_MAX);
    }
    //   - normal monsters
    for (int i = 0; i < lengthof(DunMonstConvTbl); i++) {
        const DunMonsterStruct &mon = DunMonstConvTbl[i];
        if (mon.name != nullptr) {
            DunMonsterType monType = { i, false, false };
            // filter custom entries
            unsigned n = 0;
            for (; n < customMonsterTypes.size(); n++) {
                if (customMonsterTypes[n].type == monType) {
                    break;
                }
            }
            if (n >= customMonsterTypes.size()) {
                comboBox->addItem(mon.name, QVariant::fromValue(monType));
            }
        }
    }
    comboBox->insertSeparator(INT_MAX);
    //   - unique monsters
    for (int i = 0;; i++) {
        const UniqMonData &mon = uniqMonData[i];
        if (mon.mtype != MT_INVALID) {
            DunMonsterType monType = { i + 1, true, false };
            // filter custom entries
            unsigned n = 0;
            for (; n < customMonsterTypes.size(); n++) {
                if (customMonsterTypes[n].type == monType) {
                    break;
                }
            }
            if (n >= customMonsterTypes.size()) {
                comboBox->addItem(mon.mName, QVariant::fromValue(monType));
            }
            continue;
        }
        break;
    }
    comboBox->show();
    // - items
    comboBox = this->ui->dungeonItemComboBox;
    comboBox->hide();
    comboBox->clear();
    comboBox->addItem("", 0);
    const std::vector<CustomItemStruct> &customItemTypes = this->dun->getCustomItemTypes();
    for (const CustomItemStruct &item : customItemTypes) {
        comboBox->addItem(item.name, item.type);
    }
    comboBox->show();
    // - missiles
    comboBox = this->ui->dungeonMissileComboBox;
    comboBox->hide();
    comboBox->clear();
    comboBox->addItem("", 0);
    const std::vector<CustomMissileStruct> &customMissileTypes = this->dun->getCustomMissileTypes();
    if (!customMissileTypes.empty()) {
        for (const CustomMissileStruct &mis : customMissileTypes) {
            comboBox->addItem(mis.name, mis.type);
        }
        comboBox->insertSeparator(INT_MAX);
    }
    //   - normal missiles
    for (int i = 0; i < lengthof(DunMissConvTbl); i++) {
        const DunMissileStruct &mis = DunMissConvTbl[i];
        if (mis.name != nullptr) {
            // filter custom entries
            unsigned n = 0;
            for (; n < customMissileTypes.size(); n++) {
                if (customMissileTypes[n].type == i) {
                    break;
                }
            }
            if (n >= customMissileTypes.size()) {
                comboBox->addItem(mis.name, i);
            }
        }
    }
    comboBox->show();
    // update icon of assets
    QString assetPath = this->dun->getAssetPath();
    if (!assetPath.isEmpty()) {
        this->ui->assetLoadPushButton->setToolTip(assetPath);
        QIcon icon = QApplication::style()->standardIcon(QStyle::SP_DriveCDIcon);
        this->ui->assetLoadPushButton->setIcon(icon);
        this->ui->assetLoadPushButton->setText("");
    } else {
        this->ui->assetLoadPushButton->setToolTip(tr("Select asset folder of the entites"));
        QIcon icon;
        this->ui->assetLoadPushButton->setIcon(icon);
        this->ui->assetLoadPushButton->setText("...");
    }

    emit this->dunResourcesModified();
}

// Displaying CEL file path information
void LevelCelView::updateLabel()
{
    bool hasDun = this->dun != nullptr;

    QHBoxLayout *layout = this->ui->celLabelsHorizontalLayout;
    const int labelCount = hasDun ? 7 : 6;
    while (layout->count() != labelCount + 1) {
        layout->insertWidget(0, new QLabel(""), 0, Qt::AlignLeft);
    }

    CelView::setLabelContent(qobject_cast<QLabel *>(layout->itemAt(0)->widget()), this->gfx->getFilePath(), this->gfx->isModified());
    CelView::setLabelContent(qobject_cast<QLabel *>(layout->itemAt(1)->widget()), this->min->getFilePath(), this->min->isModified());
    CelView::setLabelContent(qobject_cast<QLabel *>(layout->itemAt(2)->widget()), this->til->getFilePath(), this->til->isModified());
    CelView::setLabelContent(qobject_cast<QLabel *>(layout->itemAt(3)->widget()), this->sla->getFilePath(), this->sla->isModified());
    CelView::setLabelContent(qobject_cast<QLabel *>(layout->itemAt(4)->widget()), this->tla->getFilePath(), this->tla->isModified());
    CelView::setLabelContent(qobject_cast<QLabel *>(layout->itemAt(5)->widget()), this->cls->getFilePath(), this->cls->isModified());

    if (hasDun) {
        CelView::setLabelContent(qobject_cast<QLabel *>(layout->itemAt(6)->widget()), this->dun->getFilePath(), this->dun->isModified());
    }
}

int LevelCelView::findMonType(const QComboBox *comboBox, const DunMonsterType &value)
{
    // check if there are custom entries
    int num = 0;
    for (int index = 0; index < comboBox->count(); index++) {
        QVariant entry = comboBox->itemData(index);
        if (!entry.isValid()) {
            num++;
        }
    }
    num = num == 2 ? 1 : 0;
    for (int index = 0; index < comboBox->count(); index++) {
        QVariant entry = comboBox->itemData(index);
        if (!entry.isValid()) {
            num = 0;
            continue;
        }
        const DunMonsterType monType = entry.value<DunMonsterType>();
        // custom entries must match exactly, match standard entries ignoring the monDeadFlag
        if (monType == value || (num == 0 && monType.monIndex == value.monIndex && monType.monUnique == value.monUnique)) {
            return index;
        }
    }
    return -1;
}

void LevelCelView::updateFields()
{
    int count;

    this->updateLabel();

    if (this->dunView) {
        // Set dungeon width and height
        this->ui->dunWidthEdit->setText(QString::number(this->dun->getWidth()));
        this->ui->dunHeightEdit->setText(QString::number(this->dun->getHeight()));

        int posx = this->currentDunPosX;
        int posy = this->currentDunPosY;
        // Set dungeon location
        this->ui->dungeonPosLineEdit->setText(QString::number(posx) + ":" + QString::number(posy));
        int tileRef = this->dun->getTileAt(posx, posy);
        this->ui->dungeonTileLineEdit->setText(tileRef == UNDEF_TILE ? QStringLiteral("?") : QString::number(tileRef));
        Qt::CheckState tps = this->dun->getTileProtectionAt(posx, posy);
        this->ui->dungeonTileProtectionCheckBox->setCheckState(tps);
        this->ui->dungeonTileProtectionCheckBox->setToolTip(tps == Qt::Unchecked ? tr("Tile might be replaced in the game") : (tps == Qt::PartiallyChecked ? tr("Tile might be decorated in the game") : tr("Tile is used as is in the game")));
        int subtileRef = this->dun->getSubtileAt(posx, posy);
        this->ui->dungeonSubtileLineEdit->setText(subtileRef == UNDEF_SUBTILE ? QStringLiteral("?") : QString::number(subtileRef));
        bool smps = this->dun->getSubtileMonProtectionAt(posx, posy);
        this->ui->dungeonSubtileMonProtectionCheckBox->setChecked(smps);
        this->ui->dungeonSubtileMonProtectionCheckBox->setToolTip(smps ? tr("No monster might be placed by the game on this subtile") : tr("Monster might be placed by the game on this subtile"));
        bool sops = this->dun->getSubtileObjProtectionAt(posx, posy);
        this->ui->dungeonSubtileObjProtectionCheckBox->setChecked(sops);
        this->ui->dungeonSubtileObjProtectionCheckBox->setToolTip(sops ? tr("No object might be placed by the game on this subtile") : tr("Object might be placed by the game on this subtile"));
        int itemIndex = this->dun->getItemAt(posx, posy);
        this->ui->dungeonItemLineEdit->setText(QString::number(itemIndex));
        this->ui->dungeonItemComboBox->setCurrentIndex(this->ui->dungeonItemComboBox->findData(itemIndex));
        MapMonster mon = this->dun->getMonsterAt(posx, posy);
        this->ui->dungeonMonsterLineEdit->setText(QString::number(mon.moType.monIndex));
        this->ui->dungeonMonsterCheckBox->setChecked(mon.moType.monUnique);
        this->ui->dungeonMonsterDeadCheckBox->setChecked(mon.moType.monDeadFlag);
        this->ui->dungeonMonsterComboBox->setCurrentIndex(LevelCelView::findMonType(this->ui->dungeonMonsterComboBox, mon.moType));
        {
            const int limit = this->min->getSubtileWidth() * MICRO_WIDTH;
            const QSignalBlocker blocker(this->ui->dungeonMonsterXOffSpinBox);
            this->ui->dungeonMonsterXOffSpinBox->setRange(-limit, limit);
            this->ui->dungeonMonsterXOffSpinBox->setValue(mon.mox);
        }
        {
            const int limit = this->min->getSubtileWidth() * MICRO_HEIGHT / 2;
            const QSignalBlocker blocker(this->ui->dungeonMonsterYOffSpinBox);
            this->ui->dungeonMonsterYOffSpinBox->setRange(-limit, limit);
            this->ui->dungeonMonsterYOffSpinBox->setValue(mon.moy);
        }
        MapObject oo = this->dun->getObjectAt(posx, posy);
        this->ui->dungeonObjectLineEdit->setText(QString::number(oo.oType));
        this->ui->dungeonObjectComboBox->setCurrentIndex(this->ui->dungeonObjectComboBox->findData(oo.oType));
        this->ui->dungeonObjectPreCheckBox->setChecked(oo.oPreFlag);
        MapMissile mi = this->dun->getMissileAt(posx, posy);
        this->ui->dungeonMissileLineEdit->setText(QString::number(mi.miType));
        this->ui->dungeonMissileComboBox->setCurrentIndex(this->ui->dungeonObjectComboBox->findData(mi.miType));
        this->ui->dungeonMissilePreCheckBox->setChecked(mi.miPreFlag);
        {
            const int limit = this->min->getSubtileWidth() * MICRO_WIDTH;
            const QSignalBlocker blocker(this->ui->dungeonMissileXOffSpinBox);
            this->ui->dungeonMissileXOffSpinBox->setRange(-limit, limit);
            this->ui->dungeonMissileXOffSpinBox->setValue(mi.mix);
        }
        {
            const int limit = this->min->getSubtileWidth() * MICRO_HEIGHT / 2;
            const QSignalBlocker blocker(this->ui->dungeonMissileYOffSpinBox);
            this->ui->dungeonMissileYOffSpinBox->setRange(-limit, limit);
            this->ui->dungeonMissileYOffSpinBox->setValue(mi.miy);
        }
        this->ui->dungeonRoomLineEdit->setText(QString::number(this->dun->getRoomAt(posx, posy)));
        // update modal window
        if (this->dungeonSubtileWidget != nullptr) {
            this->dungeonSubtileWidget->initialize(posx, posy);
        }
    } else {
        // Set current and maximum frame text
        count = this->gfx->getFrameCount();
        this->ui->frameIndexEdit->setText(
            QString::number(count != 0 ? this->currentFrameIndex + 1 : 0));
        this->ui->frameNumberEdit->setText(QString::number(count));

        // Set current and maximum subtile text
        count = this->min->getSubtileCount();
        this->ui->subtileIndexEdit->setText(
            QString::number(count != 0 ? this->currentSubtileIndex + 1 : 0));
        this->ui->subtileNumberEdit->setText(QString::number(count));

        // Set current and maximum tile text
        count = this->til->getTileCount();
        this->ui->tileIndexEdit->setText(
            QString::number(count != 0 ? this->currentTileIndex + 1 : 0));
        this->ui->tileNumberEdit->setText(QString::number(count));

        this->tabTileWidget.updateFields();
        this->tabSubtileWidget.updateFields();
        this->tabFrameWidget.updateFields();
    }
}

CelScene *LevelCelView::getCelScene() const
{
    return const_cast<CelScene *>(&this->celScene);
}

int LevelCelView::getCurrentFrameIndex() const
{
    return this->currentFrameIndex;
}

int LevelCelView::getCurrentSubtileIndex() const
{
    return this->currentSubtileIndex;
}

int LevelCelView::getCurrentTileIndex() const
{
    return this->currentTileIndex;
}

const QComboBox *LevelCelView::getObjects() const
{
    return this->ui->dungeonObjectComboBox;
}

const QComboBox *LevelCelView::getMonsters() const
{
    return this->ui->dungeonMonsterComboBox;
}

void LevelCelView::showSubtileInfo()
{
    if (this->dungeonSubtileWidget == nullptr) {
        this->dungeonSubtileWidget = new DungeonSubtileWidget(this);
        this->dungeonSubtileWidget->initialize(this->currentDunPosX, this->currentDunPosY);
    }
    this->dungeonSubtileWidget->show();
}

QPoint LevelCelView::getCellPos(const QPoint &pos) const
{
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    int cellWidth = subtileWidth;
    int cellHeight = subtileWidth / 2;
    // move to 0;0
    int cX = pos.x() - this->celScene.sceneRect().width() / 2;
    int cY = pos.y() - (CEL_SCENE_MARGIN + subtileHeight - cellHeight);
    int offX = (this->dun->getWidth() - this->dun->getHeight()) * (cellWidth / 2);
    cX += offX;

    // switch unit
    int dunX = cX / cellWidth;
    int dunY = cY / cellHeight;
    int remX = cX % cellWidth;
    int remY = cY % cellHeight;
    // SHIFT_GRID
    int cellX = dunX + dunY;
    int cellY = dunY - dunX;

    // Shift position to match diamond grid aligment
    bool bottomLeft = remY >= cellHeight + (remX / 2);
    bool bottomRight = remY >= cellHeight - (remX / 2);
    if (bottomLeft) {
        cellY++;
    }
    if (bottomRight) {
        cellX++;
    }
    bool topRight = remY < (remX / 2);
    bool topLeft = remY < -(remX / 2);
    if (topRight) {
        cellY--;
    }
    if (topLeft) {
        cellX--;
    }
    return QPoint(cellX, cellY);
}

bool LevelCelView::subtilePos(QPoint &pos, QPoint &rpos) const
{
    unsigned celFrameWidth = MICRO_WIDTH; // this->gfx->getFrameWidth(this->currentFrameIndex);
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;

    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if (pos.x() >= (int)(CEL_SCENE_MARGIN + celFrameWidth + CEL_SCENE_SPACING)
        && pos.x() < (int)(CEL_SCENE_MARGIN + celFrameWidth + CEL_SCENE_SPACING + subtileWidth)
        && pos.y() >= CEL_SCENE_MARGIN
        && pos.y() < (int)(subtileHeight + CEL_SCENE_MARGIN)
        && this->min->getSubtileCount() != 0) {
        // Adjust coordinates
        unsigned stx = pos.x() - (CEL_SCENE_MARGIN + celFrameWidth + CEL_SCENE_SPACING);
        unsigned sty = pos.y() - CEL_SCENE_MARGIN;

        // qDebug() << "Subtile clicked: " << stx << "," << sty;

        rpos.setX(stx % MICRO_WIDTH);
        rpos.setY(sty % MICRO_HEIGHT);

        stx /= MICRO_WIDTH;
        sty /= MICRO_HEIGHT;

        pos.setX(stx);
        pos.setY(sty);
        return true;
    }

    return false;
}

bool LevelCelView::tilePos(QPoint &pos, QPoint &rpos) const
{
    unsigned celFrameWidth = MICRO_WIDTH; // this->gfx->getFrameWidth(this->currentFrameIndex);
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned tileWidth = subtileWidth * TILE_WIDTH;

    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;
    unsigned subtileShiftY = subtileWidth / 4;
    unsigned tileHeight = subtileHeight + 2 * subtileShiftY;

    if (pos.x() >= (int)(CEL_SCENE_MARGIN + celFrameWidth + CEL_SCENE_SPACING + subtileWidth + CEL_SCENE_SPACING)
        && pos.x() < (int)(CEL_SCENE_MARGIN + celFrameWidth + CEL_SCENE_SPACING + subtileWidth + CEL_SCENE_SPACING + tileWidth)
        && pos.y() >= CEL_SCENE_MARGIN
        && pos.y() < (int)(CEL_SCENE_MARGIN + tileHeight)
        && this->til->getTileCount() != 0) {

        // Adjust coordinates
        unsigned tx = pos.x() - (CEL_SCENE_MARGIN + celFrameWidth + CEL_SCENE_SPACING + subtileWidth + CEL_SCENE_SPACING);
        unsigned ty = pos.y() - CEL_SCENE_MARGIN;

        // qDebug() << "Tile clicked" << tx << "," << ty;

        // Split the area based on the ground squares
        // f(x)\ 0 /
        //      \ /
        //   2   *   1
        //      / \  
        // g(x)/ 3 \
        //
        // f(x) = (tileHeight - tileWidth / 2) + 0.5x
        unsigned ftx = (tileHeight - tileWidth / 2) + tx / 2;
        // g(tx) = tileHeight - 0.5x
        unsigned gtx = tileHeight - tx / 2;
        // qDebug() << "fx=" << ftx << ", gx=" << gtx;
        unsigned tSubtile = 0;
        if (ty < ftx) {
            if (ty < gtx) {
                // limit the area of 0 horizontally
                if (tx < subtileWidth / 2)
                    tSubtile = 2;
                else if (tx >= (subtileWidth / 2 + subtileWidth))
                    tSubtile = 1;
                else
                    tSubtile = 0;
            } else {
                tSubtile = 1;
            }
        } else {
            if (ty < gtx)
                tSubtile = 2;
            else
                tSubtile = 3;
        }
        switch (tSubtile) {
        case 0: tx -= subtileWidth / 2; ty -= 0; break;
        case 1: tx -= subtileWidth; ty -= subtileShiftY; break;
        case 2: tx -= 0; ty -= subtileShiftY; break;
        case 3: tx -= subtileWidth / 2; ty -= 2 * subtileShiftY; break;
        }
        rpos.setX(tx);
        rpos.setY(ty);

        pos.setX(tSubtile);
        pos.setY(UINT_MAX);
        return true;
    }
    return false;
}

bool LevelCelView::framePos(QPoint &pos) const
{
    if (this->gfx->getFrameCount() != 0) {
        D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
        pos -= QPoint(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
        return pos.x() >= 0 && pos.x() < frame->getWidth() && pos.y() >= 0 && pos.y() < frame->getHeight();
    }

    return false;
}

void LevelCelView::framePixelClicked(const QPoint &pos, int flags)
{
    unsigned celFrameWidth = MICRO_WIDTH; // this->gfx->getFrameWidth(this->currentFrameIndex);
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned tileWidth = subtileWidth * TILE_WIDTH;

    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;
    unsigned subtileShiftY = subtileWidth / 4;
    unsigned tileHeight = subtileHeight + 2 * subtileShiftY;

    if (this->dunView) {
        QPoint cellPos = this->getCellPos(pos);
        dMainWindow().dunClicked(cellPos, flags);
        return;
    }
    QPoint tpos = pos;
    QPoint rpos;
    if (this->subtilePos(tpos, rpos)) {
        unsigned stx = tpos.x();
        unsigned sty = tpos.y();

        unsigned stFrame = sty * subtileWidth / MICRO_WIDTH + stx;
        std::vector<unsigned> &minFrames = this->min->getFrameReferences(this->currentSubtileIndex);
        unsigned frameRef = 0;
        bool frameChanged = false, drawn = false;
        if (minFrames.size() > stFrame) {
            frameRef = minFrames.at(stFrame);
            frameChanged = this->tabSubtileWidget.selectFrame(stFrame);
        }
        if (frameRef > 0) {
            this->currentFrameIndex = frameRef - 1;
        }
        if ((dMainWindow().isPainting() && !(QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier)) && (!frameChanged || this->ui->drawToSpecCheckBox->isChecked())) {
            if (!this->ui->drawToSpecCheckBox->isChecked()) {
                // draw to the micro
                if (frameRef > 0 && frameRef <= (unsigned)this->gfx->getFrameCount()) {
                    D1GfxFrame *frame = this->gfx->getFrame(frameRef - 1);
                    dMainWindow().frameClicked(frame, rpos, flags);
                    drawn = true;
                }
            } else {
                // draw to the special frame of the subtile
                int specFrame = this->sla->getSpecProperty(this->currentSubtileIndex);
                if (specFrame > 0 && specFrame <= this->cls->getFrameCount()) {
                    D1GfxFrame *frame = this->cls->getFrame(specFrame - 1);
                    rpos.rx() += stx * MICRO_WIDTH;
                    rpos.ry() += sty * MICRO_HEIGHT;
                    dMainWindow().frameClicked(frame, rpos, flags);
                    drawn = true;
                }
            }
        }
        // if (frameChanged || drawn) {
            this->displayFrame();
        // }
        // highlight selection
        if (!drawn) {
            QColor borderColor = QColor(Config::getPaletteSelectionBorderColor());
            QPen pen(borderColor);
            pen.setWidth(PALETTE_SELECTION_WIDTH);
            QRectF coordinates = QRectF(CEL_SCENE_MARGIN + celFrameWidth + CEL_SCENE_SPACING + stx * MICRO_WIDTH, CEL_SCENE_MARGIN + sty * MICRO_HEIGHT, MICRO_WIDTH, MICRO_HEIGHT);
            int a = PALETTE_SELECTION_WIDTH / 2;
            coordinates.adjust(-a, -a, 0, 0);
            // - top line
            this->celScene.addLine(coordinates.left(), coordinates.top(), coordinates.right(), coordinates.top(), pen);
            // - bottom line
            this->celScene.addLine(coordinates.left(), coordinates.bottom(), coordinates.right(), coordinates.bottom(), pen);
            // - left side
            this->celScene.addLine(coordinates.left(), coordinates.top(), coordinates.left(), coordinates.bottom(), pen);
            // - right side
            this->celScene.addLine(coordinates.right(), coordinates.top(), coordinates.right(), coordinates.bottom(), pen);
            // clear after some time
            QTimer *timer = new QTimer();
            QObject::connect(timer, &QTimer::timeout, [this, timer]() {
                this->displayFrame();
                timer->deleteLater();
            });
            timer->start(500);
        }
        return;
    }
    if (this->tilePos(tpos, rpos)) {
        unsigned tSubtile = tpos.x();

        std::vector<int> &tilSubtiles = this->til->getSubtileIndices(this->currentTileIndex);
        if (tilSubtiles.size() > tSubtile) {
            this->currentSubtileIndex = tilSubtiles.at(tSubtile);
            bool tileChanged = this->tabTileWidget.selectSubtile(tSubtile);
            if ((dMainWindow().isPainting() && !(QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier)) && (!tileChanged || this->ui->drawToSpecCheckBox->isChecked())) {
                if (!this->ui->drawToSpecCheckBox->isChecked()) {
                    // draw to the micro
                    unsigned stx = rpos.x();
                    unsigned sty = rpos.y();

                    rpos.setX(stx % MICRO_WIDTH);
                    rpos.setY(sty % MICRO_HEIGHT);

                    stx /= MICRO_WIDTH;
                    sty /= MICRO_HEIGHT;

                    unsigned stFrame = sty * subtileWidth / MICRO_WIDTH + stx;
                    std::vector<unsigned> &minFrames = this->min->getFrameReferences(this->currentSubtileIndex);
                    unsigned frameRef = 0;
                    if (minFrames.size() > stFrame) {
                        frameRef = minFrames.at(stFrame);
                        if (frameRef > 0 && frameRef <= (unsigned)this->gfx->getFrameCount()) {
                            D1GfxFrame *frame = this->gfx->getFrame(frameRef - 1);
                            dMainWindow().frameClicked(frame, rpos, flags);
                        }
                    }
                } else {
                    // draw to the special frame of the subtile
                    int specFrame = this->sla->getSpecProperty(this->currentSubtileIndex);
                    if (specFrame > 0 && specFrame <= this->cls->getFrameCount()) {
                        D1GfxFrame *frame = this->cls->getFrame(specFrame - 1);
                        dMainWindow().frameClicked(frame, rpos, flags);
                    }
                }
            }
            // if (tileChanged || drawn) {
                this->displayFrame();
            // }
        }
        return;
    }

    // otherwise emit frame-click event
    if (this->gfx->getFrameCount() == 0) {
        return;
    }
    this->framePos(tpos);
    D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
    dMainWindow().frameClicked(frame, tpos, flags);
}

void LevelCelView::framePixelHovered(const QPoint &pos) const
{
    if (this->dunView) {
        QPoint cellPos = this->getCellPos(pos);
        dMainWindow().dunHovered(cellPos);
        return;
    }
    QPoint tpos = pos;
    QPoint rpos;
    if (this->subtilePos(tpos, rpos)) {
        unsigned stx = tpos.x();
        unsigned sty = tpos.y();
        unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;

        unsigned stFrame = sty * subtileWidth / MICRO_WIDTH + stx;
        tpos.setX(stFrame);
        tpos.setY(UINT_MAX);

        dMainWindow().pointHovered(tpos);
        return;
    }
    if (this->tilePos(tpos, rpos)) {
        dMainWindow().pointHovered(tpos);
        return;
    }
    if (this->framePos(tpos)) {
        dMainWindow().pointHovered(tpos);
        return;
    }

    tpos.setX(UINT_MAX);
    tpos.setY(UINT_MAX);
    dMainWindow().pointHovered(tpos);
}

void LevelCelView::scrollTo(int posx, int posy)
{
    this->currentDunPosX = posx;
    this->currentDunPosY = posy;
    this->isScrolling = true;
}

void LevelCelView::scrollToCurrent()
{
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    int cellWidth = subtileWidth;
    int cellHeight = subtileWidth / 2;
    // move to 0;0
    int cX = this->celScene.sceneRect().width() / 2;
    int cY = CEL_SCENE_MARGIN + subtileHeight - cellHeight / 2;
    int bX = cX, bY = cY;
    int offX = (this->dun->getWidth() - this->dun->getHeight()) * (cellWidth / 2);
    cX -= offX;

    int dunX = this->currentDunPosX;
    int dunY = this->currentDunPosY;
    // SHIFT_GRID
    int cellX = dunX - dunY;
    int cellY = dunY + dunX;
    // switch unit
    cellX *= cellWidth / 2;
    cellY *= cellHeight / 2;

    cX += cellX;
    cY += cellY;
    this->ui->celGraphicsView->centerOn(cX, cY);
}

void LevelCelView::selectPos(const QPoint &cell, int flags)
{
    this->currentDunPosX = cell.x();
    this->currentDunPosY = cell.y();
    // update the view
    this->updateFields();

    if (flags & DOUBLE_CLICK) {
        this->scrollToCurrent();
    }
    if (flags & SHIFT_CLICK) {
        int tileRef = this->dun->getTileAt(cell.x(), cell.y());
        int subtileRef = this->dun->getSubtileAt(cell.x(), cell.y());
        bool switchView = false;
        if (tileRef != UNDEF_TILE && this->til->getTileCount() >= tileRef) {
            this->currentTileIndex = tileRef - 1;
            switchView = true;
        }
        if (subtileRef != UNDEF_SUBTILE && this->sla->getSubtileCount() >= subtileRef) {
            this->currentSubtileIndex = subtileRef - 1;
            switchView = true;
        }
        if (switchView) {
            dMainWindow().on_actionToggle_View_triggered();
        }
    }
}

void LevelCelView::insertImageFiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append)
{
    if (mode == IMAGE_FILE_MODE::FRAME || mode == IMAGE_FILE_MODE::AUTO) {
        this->insertFrames(mode, imagefilePaths, append);
    }
    if (mode == IMAGE_FILE_MODE::SUBTILE || mode == IMAGE_FILE_MODE::AUTO) {
        this->insertSubtiles(mode, imagefilePaths, append);
    }
    if (mode == IMAGE_FILE_MODE::TILE || mode == IMAGE_FILE_MODE::AUTO) {
        this->insertTiles(mode, imagefilePaths, append);
    }
}

void LevelCelView::assignFrames(const QImage &image, int subtileIndex, int frameIndex)
{
    std::vector<unsigned> frameReferencesList;

    // TODO: merge with LevelCelView::insertSubtile ?
    QImage subImage = QImage(MICRO_WIDTH, MICRO_HEIGHT, QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); y += MICRO_HEIGHT) {
        const QRgb *imageBits = reinterpret_cast<const QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); x += MICRO_WIDTH, imageBits += MICRO_WIDTH) {
            // subImage.fill(Qt::transparent);

            bool hasColor = false;
            QRgb *destBits = reinterpret_cast<QRgb *>(subImage.bits());
            const QRgb *srcBits = imageBits;
            for (int j = 0; j < MICRO_HEIGHT; j++) {
                for (int i = 0; i < MICRO_WIDTH; i++, srcBits++, destBits++) {
                    // const QColor color = image.pixelColor(x + i, y + j);
                    // if (color.alpha() >= COLOR_ALPHA_LIMIT) {
                    if (qAlpha(*srcBits) >= COLOR_ALPHA_LIMIT) {
                        hasColor = true;
                    }
                    // subImage.setPixelColor(i, j, color);
                    *destBits = *srcBits;
                }
                srcBits += image.width() - MICRO_WIDTH;
            }
            frameReferencesList.push_back(hasColor ? frameIndex + 1 : 0);
            if (!hasColor) {
                continue;
            }

            D1GfxFrame *frame = this->gfx->insertFrame(frameIndex, subImage);
            D1CelTilesetFrame::selectFrameType(frame);
            frameIndex++;
        }
    }

    if (subtileIndex >= 0) {
        this->min->getFrameReferences(subtileIndex).swap(frameReferencesList);
        this->min->setModified();
        this->tileset->resetSubtileFlags(subtileIndex);
    }
}

void LevelCelView::assignFrames(const D1GfxFrame &frame, int subtileIndex, int frameIndex)
{
    std::vector<unsigned> frameReferencesList;

    // TODO: merge with LevelCelView::insertSubtile ?
    for (int y = 0; y < frame.getHeight(); y += MICRO_HEIGHT) {
        for (int x = 0; x < frame.getWidth(); x += MICRO_WIDTH) {
            bool hasColor = false;
            for (int j = 0; j < MICRO_HEIGHT; j++) {
                for (int i = 0; i < MICRO_WIDTH; i++) {
                    if (!frame.getPixel(x + i, y + j).isTransparent()) {
                        hasColor = true;
                    }
                }
            }
            frameReferencesList.push_back(hasColor ? frameIndex + 1 : 0);
            if (!hasColor) {
                continue;
            }

            D1GfxFrame *subFrame = this->gfx->insertFrame(frameIndex);
            for (int j = 0; j < MICRO_HEIGHT; j++) {
                std::vector<D1GfxPixel> pixelLine;
                for (int i = 0; i < MICRO_WIDTH; i++) {
                    pixelLine.push_back(frame.getPixel(x + i, y + j));
                }
                subFrame->addPixelLine(std::move(pixelLine));
            }
            D1CelTilesetFrame::selectFrameType(subFrame);
            frameIndex++;
        }
    }

    if (subtileIndex >= 0) {
        this->min->getFrameReferences(subtileIndex).swap(frameReferencesList);
        this->min->setModified();
        this->tileset->resetSubtileFlags(subtileIndex);
    }
}

void LevelCelView::insertFrames(IMAGE_FILE_MODE mode, int index, const QImage &image)
{
    if ((image.width() % MICRO_WIDTH) != 0 || (image.height() % MICRO_HEIGHT) != 0) {
        QString msg = tr("The image must contain 32px * 32px blocks to be used as a frame.");
        if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << msg;
        } else {
            dProgressErr() << msg;
        }
        return;
    }

    if (mode == IMAGE_FILE_MODE::AUTO) {
        // check for subtile dimensions to be more lenient than EXPORT_LVLFRAMES_PER_LINE
        unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
        unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

        if ((image.width() % subtileWidth) == 0 && (image.height() % subtileHeight) == 0) {
            return; // this is a subtile or a tile (or subtiles or tiles) -> ignore
        }
    }

    this->assignFrames(image, -1, index);
}

bool LevelCelView::insertFrames(IMAGE_FILE_MODE mode, int index, const D1GfxFrame &frame)
{
    if ((frame.getWidth() % MICRO_WIDTH) != 0 || (frame.getHeight() % MICRO_HEIGHT) != 0) {
        QString msg = tr("The image must contain 32px * 32px blocks to be used as a frame.");
        if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << msg;
        } else {
            dProgressErr() << msg;
        }
        return false;
    }

    if (mode == IMAGE_FILE_MODE::AUTO) {
        // check for subtile dimensions to be more lenient than EXPORT_LVLFRAMES_PER_LINE
        unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
        unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

        if ((frame.getWidth() % subtileWidth) == 0 && (frame.getHeight() % subtileHeight) == 0) {
            return false; // this is a subtile or a tile (or subtiles or tiles) -> ignore
        }
    }

    this->assignFrames(frame, -1, index);
    return true;
}

void LevelCelView::insertFrames(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, &basePal, this->gfx->getPalette(), &palMod);
        if (success) {
            success = this->insertFrames(mode, index, frame);
        } else if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        }
        if (success && palMod) {
            // update the palette
            this->pal->updateColors(basePal);
            emit this->palModified();
        }
        return;
    }

    QImageReader reader = QImageReader(imagefilePath);
    if (!reader.canRead()) {
        QString msg = tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << msg;
        } else {
            dProgressErr() << msg;
        }
        return;
    }
    int prevFrameCount = this->gfx->getFrameCount();
    while (true) {
        QImage image = reader.read();
        if (image.isNull()) {
            break;
        }
        this->insertFrames(mode, index, image);
        // update index
        int newFrameCount = this->gfx->getFrameCount();
        index += newFrameCount - prevFrameCount;
        prevFrameCount = newFrameCount;
    }
}

void LevelCelView::insertFrames(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append)
{
    int prevFrameCount = this->gfx->getFrameCount();

    if (append) {
        // append the frame(s)
        for (int i = 0; i < imagefilePaths.count(); i++) {
            this->insertFrames(mode, this->gfx->getFrameCount(), imagefilePaths[i]);
        }
        int deltaFrameCount = this->gfx->getFrameCount() - prevFrameCount;
        if (deltaFrameCount == 0) {
            return; // no new frame -> done
        }
        // jump to the first appended frame
        this->currentFrameIndex = prevFrameCount;
    } else {
        // insert the frame(s)
        for (int i = imagefilePaths.count() - 1; i >= 0; i--) {
            this->insertFrames(mode, this->currentFrameIndex, imagefilePaths[i]);
        }
        int deltaFrameCount = this->gfx->getFrameCount() - prevFrameCount;
        if (deltaFrameCount == 0) {
            return; // no new frame -> done
        }
        // shift references
        unsigned frameRef = this->currentFrameIndex + 1;
        // shift frame indices of the subtiles
        for (int i = 0; i < this->min->getSubtileCount(); i++) {
            std::vector<unsigned> &frameReferences = this->min->getFrameReferences(i);
            for (unsigned n = 0; n < frameReferences.size(); n++) {
                if (frameReferences[n] >= frameRef) {
                    frameReferences[n] += deltaFrameCount;
                }
            }
        }
    }
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::assignSubtiles(const QImage &image, int tileIndex, int subtileIndex)
{
    std::vector<int> *subtileIndices = nullptr;
    if (tileIndex >= 0) {
        subtileIndices = &this->til->getSubtileIndices(tileIndex);
        subtileIndices->clear();
        this->til->setModified();
    }
    // TODO: merge with LevelCelView::insertTile ?
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    QImage subImage = QImage(subtileWidth, subtileHeight, QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); y += subtileHeight) {
        const QRgb *imageBits = reinterpret_cast<const QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); x += subtileWidth, imageBits += subtileWidth) {
            // subImage.fill(Qt::transparent);

            bool hasColor = false;
            QRgb *destBits = reinterpret_cast<QRgb *>(subImage.bits());
            const QRgb *srcBits = imageBits;
            for (unsigned j = 0; j < subtileHeight; j++) {
                for (unsigned i = 0; i < subtileWidth; i++, srcBits++, destBits++) {
                    // const QColor color = image.pixelColor(x + i, y + j);
                    // if (color.alpha() >= COLOR_ALPHA_LIMIT) {
                    if (qAlpha(*srcBits) >= COLOR_ALPHA_LIMIT) {
                        hasColor = true;
                    }
                    // subImage.setPixelColor(i, j, color);
                    *destBits = *srcBits;
                }
                srcBits += image.width() - subtileWidth;
            }

            if (subtileIndices != nullptr) {
                subtileIndices->push_back(subtileIndex);
            } else if (!hasColor) {
                continue;
            }

            this->insertSubtile(subtileIndex, subImage);
            subtileIndex++;
        }
    }
}

void LevelCelView::assignSubtiles(const D1GfxFrame &frame, int tileIndex, int subtileIndex)
{
    std::vector<int> *subtileIndices = nullptr;
    if (tileIndex >= 0) {
        subtileIndices = &this->til->getSubtileIndices(tileIndex);
        subtileIndices->clear();
        this->til->setModified();
    }
    // TODO: merge with LevelCelView::insertTile ?
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    for (int y = 0; y < frame.getHeight(); y += subtileHeight) {
        for (int x = 0; x < frame.getWidth(); x += subtileWidth) {
            D1GfxFrame subFrame;
            bool hasColor = false;
            for (unsigned j = 0; j < subtileHeight; j++) {
                std::vector<D1GfxPixel> pixelLine;
                for (unsigned i = 0; i < subtileWidth; i++) {
                    D1GfxPixel pixel = frame.getPixel(x + i, y + j);
                    if (!pixel.isTransparent()) {
                        hasColor = true;
                    }
                    pixelLine.push_back(pixel);
                }
                subFrame.addPixelLine(std::move(pixelLine));
            }
            if (!hasColor) {
                continue;
            }

            if (subtileIndices != nullptr) {
                subtileIndices->push_back(subtileIndex);
            } else if (!hasColor) {
                continue;
            }

            this->insertSubtile(subtileIndex, subFrame);
            subtileIndex++;
        }
    }
}

void LevelCelView::insertSubtiles(IMAGE_FILE_MODE mode, int index, const QImage &image)
{
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if ((image.width() % subtileWidth) != 0 || (image.height() % subtileHeight) != 0) {
        if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << tr("The image must contain %1px * %2px blocks to be used as a subtile.").arg(subtileWidth).arg(subtileWidth);
        }
        return;
    }

    if (mode == IMAGE_FILE_MODE::AUTO) {
        // check for tile dimensions to be more lenient than EXPORT_SUBTILES_PER_LINE
        unsigned tileWidth = subtileWidth * TILE_WIDTH * TILE_HEIGHT;
        unsigned tileHeight = subtileHeight;

        if ((image.width() % tileWidth) == 0 && (image.height() % tileHeight) == 0) {
            return; // this is a tile (or tiles) -> ignore
        }
    }

    this->assignSubtiles(image, -1, index);
}

bool LevelCelView::insertSubtiles(IMAGE_FILE_MODE mode, int index, const D1GfxFrame &frame)
{
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if ((frame.getWidth() % subtileWidth) != 0 || (frame.getHeight() % subtileHeight) != 0) {
        if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << tr("The image must contain %1px * %2px blocks to be used as a subtile.").arg(subtileWidth).arg(subtileWidth);
        }
        return false;
    }

    if (mode == IMAGE_FILE_MODE::AUTO) {
        // check for tile dimensions to be more lenient than EXPORT_SUBTILES_PER_LINE
        unsigned tileWidth = subtileWidth * TILE_WIDTH * TILE_HEIGHT;
        unsigned tileHeight = subtileHeight;

        if ((frame.getWidth() % tileWidth) == 0 && (frame.getHeight() % tileHeight) == 0) {
            return false; // this is a tile (or tiles) -> ignore
        }
    }

    this->assignSubtiles(frame, -1, index);
    return true;
}

void LevelCelView::insertSubtiles(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, &basePal, this->gfx->getPalette(), &palMod);
        if (success) {
            success = this->insertSubtiles(mode, index, frame);
        } else if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        }
        if (success && palMod) {
            // update the palette
            this->pal->updateColors(basePal);
            emit this->palModified();
        }
        return;
    }

    QImageReader reader = QImageReader(imagefilePath);
    if (!reader.canRead()) {
        QString msg = tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << msg;
        } else {
            dProgressErr() << msg;
        }
        return;
    }
    int prevSubtileCount = this->min->getSubtileCount();
    while (true) {
        QImage image = reader.read();
        if (image.isNull()) {
            break;
        }
        this->insertSubtiles(mode, index, image);
        // update index
        int newSubtileCount = this->min->getSubtileCount();
        index += newSubtileCount - prevSubtileCount;
        prevSubtileCount = newSubtileCount;
    }
}

void LevelCelView::insertSubtiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append)
{
    int prevSubtileCount = this->min->getSubtileCount();

    if (append) {
        // append the subtile(s)
        for (int i = 0; i < imagefilePaths.count(); i++) {
            this->insertSubtiles(mode, this->min->getSubtileCount(), imagefilePaths[i]);
        }
        int deltaSubtileCount = this->min->getSubtileCount() - prevSubtileCount;
        if (deltaSubtileCount == 0) {
            return; // no new subtile -> done
        }
        // jump to the first appended subtile
        this->currentSubtileIndex = prevSubtileCount;
    } else {
        // insert the subtile(s)
        for (int i = imagefilePaths.count() - 1; i >= 0; i--) {
            this->insertSubtiles(mode, this->currentSubtileIndex, imagefilePaths[i]);
        }
        int deltaSubtileCount = this->min->getSubtileCount() - prevSubtileCount;
        if (deltaSubtileCount == 0) {
            return; // no new subtile -> done
        }
        // shift references
        int refIndex = this->currentSubtileIndex;
        // shift subtile indices of the tiles
        for (int i = 0; i < this->til->getTileCount(); i++) {
            std::vector<int> &subtileIndices = this->til->getSubtileIndices(i);
            for (unsigned n = 0; n < subtileIndices.size(); n++) {
                if (subtileIndices[n] >= refIndex) {
                    subtileIndices[n] += deltaSubtileCount;
                    this->til->setModified();
                }
            }
        }
    }
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::insertSubtile(int subtileIndex, const std::vector<unsigned> &frameReferencesList)
{
    this->tileset->insertSubtile(subtileIndex, frameReferencesList);
    if (this->dun != nullptr) {
        this->dun->subtileInserted(subtileIndex);
    }
}

void LevelCelView::insertSubtile(int subtileIndex, const QImage &image)
{
    std::vector<unsigned> frameReferencesList;

    int frameIndex = this->gfx->getFrameCount();
    QImage subImage = QImage(MICRO_WIDTH, MICRO_HEIGHT, QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); y += MICRO_HEIGHT) {
        const QRgb *imageBits = reinterpret_cast<const QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); x += MICRO_WIDTH, imageBits += MICRO_WIDTH) {
            // subImage.fill(Qt::transparent);

            bool hasColor = false;
            QRgb *destBits = reinterpret_cast<QRgb *>(subImage.bits());
            const QRgb *srcBits = imageBits;
            for (int j = 0; j < MICRO_HEIGHT; j++) {
                for (int i = 0; i < MICRO_WIDTH; i++, srcBits++, destBits++) {
                    // const QColor color = image.pixelColor(x + i, y + j);
                    // if (color.alpha() >= COLOR_ALPHA_LIMIT) {
                    if (qAlpha(*srcBits) >= COLOR_ALPHA_LIMIT) {
                        hasColor = true;
                    }
                    // subImage.setPixelColor(i, j, color);
                    *destBits = *srcBits;
                }
                srcBits += image.width() - MICRO_WIDTH;
            }

            frameReferencesList.push_back(hasColor ? frameIndex + 1 : 0);

            if (!hasColor) {
                continue;
            }

            D1GfxFrame *frame = this->gfx->insertFrame(frameIndex, subImage);
            D1CelTilesetFrame::selectFrameType(frame);
            frameIndex++;
        }
    }
    this->insertSubtile(subtileIndex, frameReferencesList);
}

void LevelCelView::insertSubtile(int subtileIndex, const D1GfxFrame &frame)
{
    std::vector<unsigned> frameReferencesList;

    int frameIndex = this->gfx->getFrameCount();
    for (int y = 0; y < frame.getHeight(); y += MICRO_HEIGHT) {
        for (int x = 0; x < frame.getWidth(); x += MICRO_WIDTH) {
            bool hasColor = false;
            for (int j = 0; j < MICRO_HEIGHT; j++) {
                for (int i = 0; i < MICRO_WIDTH; i++) {
                    if (!frame.getPixel(x + i, y + j).isTransparent()) {
                        hasColor = true;
                    }
                }
            }

            frameReferencesList.push_back(hasColor ? frameIndex + 1 : 0);

            if (!hasColor) {
                continue;
            }

            D1GfxFrame *subFrame = this->gfx->insertFrame(frameIndex);
            for (int j = 0; j < MICRO_HEIGHT; j++) {
                std::vector<D1GfxPixel> pixelLine;
                for (int i = 0; i < MICRO_WIDTH; i++) {
                    pixelLine.push_back(frame.getPixel(x + i, y + j));
                }
                subFrame->addPixelLine(std::move(pixelLine));
            }
            D1CelTilesetFrame::selectFrameType(subFrame);
            frameIndex++;
        }
    }
    this->insertSubtile(subtileIndex, frameReferencesList);
}

void LevelCelView::insertTile(int tileIndex, const std::vector<int> &subtileIndices)
{
    this->tileset->insertTile(tileIndex, subtileIndices);
    if (this->dun != nullptr) {
        this->dun->tileInserted(tileIndex);
    }
}

void LevelCelView::insertTile(int tileIndex, const QImage &image)
{
    std::vector<int> subtileIndices;

    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    QImage subImage = QImage(subtileWidth, subtileHeight, QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); y += subtileHeight) {
        const QRgb *imageBits = reinterpret_cast<const QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); x += subtileWidth, imageBits += subtileWidth) {
            // subImage.fill(Qt::transparent);

            // bool hasColor = false;
            QRgb *destBits = reinterpret_cast<QRgb *>(subImage.bits());
            const QRgb *srcBits = imageBits;
            for (unsigned j = 0; j < subtileHeight; j++) {
                for (unsigned i = 0; i < subtileWidth; i++, srcBits++, destBits++) {
                    // const QColor color = image.pixelColor(x + i, y + j);
                    // if (color.alpha() >= COLOR_ALPHA_LIMIT) {
                    // if (qAlpha(*srcBits) >= COLOR_ALPHA_LIMIT) {
                    //    hasColor = true;
                    // }
                    // subImage.setPixelColor(i, j, color);
                    *destBits = *srcBits;
                }
                srcBits += image.width() - subtileWidth;
            }

            int index = this->min->getSubtileCount();
            subtileIndices.push_back(index);
            this->insertSubtile(index, subImage);
        }
    }
    this->insertTile(tileIndex, subtileIndices);
}

void LevelCelView::insertTile(int tileIndex, const D1GfxFrame &frame)
{
    std::vector<int> subtileIndices;

    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    for (int y = 0; y < frame.getHeight(); y += subtileHeight) {
        for (int x = 0; x < frame.getWidth(); x += subtileWidth) {
            D1GfxFrame subFrame;
            for (unsigned j = 0; j < subtileHeight; j++) {
                std::vector<D1GfxPixel> pixelLine;
                for (unsigned i = 0; i < subtileWidth; i++) {
                    pixelLine.push_back(frame.getPixel(x + i, y + j));
                }
                subFrame.addPixelLine(std::move(pixelLine));
            }

            int index = this->min->getSubtileCount();
            subtileIndices.push_back(index);
            this->insertSubtile(index, subFrame);
        }
    }
    this->insertTile(tileIndex, subtileIndices);
}

void LevelCelView::insertTiles(IMAGE_FILE_MODE mode, int index, const QImage &image)
{
    unsigned tileWidth = this->min->getSubtileWidth() * MICRO_WIDTH * TILE_WIDTH * TILE_HEIGHT;
    unsigned tileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if ((image.width() % tileWidth) != 0 || (image.height() % tileHeight) != 0) {
        if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << tr("The image must contain %1px * %2px blocks to be used as a tile.").arg(tileWidth).arg(tileHeight);
        }
        return;
    }

    /*if (mode == IMAGE_FILE_MODE::AUTO
        && (image.width() != subtileWidth || image.height() != subtileHeight) && image.width() != subtileWidth * EXPORT_TILES_PER_LINE) {
        // not a column of tiles
        // not a row or tiles
        // not a grouped tiles from an export -> ignore
        return;
    }*/

    QImage subImage = QImage(tileWidth, tileHeight, QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); y += tileHeight) {
        const QRgb *imageBits = reinterpret_cast<const QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); x += tileWidth, imageBits += tileWidth) {
            // subImage.fill(Qt::transparent);

            bool hasColor = false;
            QRgb *destBits = reinterpret_cast<QRgb *>(subImage.bits());
            const QRgb *srcBits = imageBits;
            for (unsigned j = 0; j < tileHeight; j++) {
                for (unsigned i = 0; i < tileWidth; i++, srcBits++, destBits++) {
                    // const QColor color = image.pixelColor(x + i, y + j);
                    // if (color.alpha() >= COLOR_ALPHA_LIMIT) {
                    if (qAlpha(*srcBits) >= COLOR_ALPHA_LIMIT) {
                        hasColor = true;
                    }
                    // subImage.setPixelColor(i, j, color);
                    *destBits = *srcBits;
                }
                srcBits += image.width() - tileWidth;
            }

            if (!hasColor) {
                continue;
            }

            this->insertTile(index, subImage);
            index++;
        }
    }
}

bool LevelCelView::insertTiles(IMAGE_FILE_MODE mode, int index, const D1GfxFrame &frame)
{
    unsigned tileWidth = this->min->getSubtileWidth() * MICRO_WIDTH * TILE_WIDTH * TILE_HEIGHT;
    unsigned tileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if ((frame.getWidth() % tileWidth) != 0 || (frame.getHeight() % tileHeight) != 0) {
        if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << tr("The image must contain %1px * %2px blocks to be used as a tile.").arg(tileWidth).arg(tileHeight);
        }
        return false;
    }

    /*if (mode == IMAGE_FILE_MODE::AUTO
        && (frame.getWidth() != subtileWidth || frame.getHeight() != subtileHeight) && frame.getWidth() != subtileWidth * EXPORT_TILES_PER_LINE) {
        // not a column of tiles
        // not a row or tiles
        // not a grouped tiles from an export -> ignore
        return false;
    }*/

    for (int y = 0; y < frame.getHeight(); y += tileHeight) {
        for (int x = 0; x < frame.getWidth(); x += tileWidth) {
            D1GfxFrame subFrame;
            bool hasColor = false;
            for (unsigned j = 0; j < tileHeight; j++) {
                std::vector<D1GfxPixel> pixelLine;
                for (unsigned i = 0; i < tileWidth; i++) {
                    D1GfxPixel pixel = frame.getPixel(x + i, y + j);
                    if (!pixel.isTransparent()) {
                        hasColor = true;
                    }
                    pixelLine.push_back(pixel);
                }
                subFrame.addPixelLine(std::move(pixelLine));
            }
            if (!hasColor) {
                continue;
            }

            this->insertTile(index, subFrame);
            index++;
        }
    }
    return true;
}

void LevelCelView::insertTiles(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, &basePal, this->gfx->getPalette(), &palMod);
        if (success) {
            success = this->insertTiles(mode, index, frame);
        } else if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        }
        if (success && palMod) {
            // update the palette
            this->pal->updateColors(basePal);
            emit this->palModified();
        }
        return;
    }

    QImageReader reader = QImageReader(imagefilePath);
    if (!reader.canRead()) {
        QString msg = tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        if (mode != IMAGE_FILE_MODE::AUTO) {
            dProgressFail() << msg;
        } else {
            dProgressErr() << msg;
        }
        return;
    }
    int prevTileCount = this->til->getTileCount();
    while (true) {
        QImage image = reader.read();
        if (image.isNull()) {
            break;
        }
        this->insertTiles(mode, index, image);
        // update index
        int newTileCount = this->til->getTileCount();
        index += newTileCount - prevTileCount;
        prevTileCount = newTileCount;
    }
}

void LevelCelView::insertTiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append)
{
    int prevTileCount = this->til->getTileCount();

    if (append) {
        // append the tile(s)
        for (int i = 0; i < imagefilePaths.count(); i++) {
            this->insertTiles(mode, this->til->getTileCount(), imagefilePaths[i]);
        }
        int deltaTileCount = this->til->getTileCount() - prevTileCount;
        if (deltaTileCount == 0) {
            return; // no new tile -> done
        }
        // jump to the first appended tile
        this->currentTileIndex = prevTileCount;
    } else {
        // insert the tile(s)
        for (int i = imagefilePaths.count() - 1; i >= 0; i--) {
            this->insertTiles(mode, this->currentTileIndex, imagefilePaths[i]);
        }
        int deltaTileCount = this->til->getTileCount() - prevTileCount;
        if (deltaTileCount == 0) {
            return; // no new tile -> done
        }
    }
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::addToCurrentFrame(const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, &basePal, this->gfx->getPalette(), &palMod);
        if (!success) {
            dProgressFail() << tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
            return;
        }
        D1GfxFrame *resFrame = this->gfx->addToFrame(this->currentFrameIndex, frame);
        if (resFrame == nullptr) {
            return; // error set by gfx->addToFrame
        }
        D1CelTilesetFrame::selectFrameType(resFrame);
        if (palMod) {
            // update the palette
            this->pal->updateColors(basePal);
            emit this->palModified();
        }
        // update the view - done by the caller
        // this->displayFrame();
        return;
    }

    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        dProgressFail() << tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        return;
    }

    D1GfxFrame *frame = this->gfx->addToFrame(this->currentFrameIndex, image);
    if (frame == nullptr) {
        return; // error set by gfx->addToFrame
    }

    D1CelTilesetFrame::selectFrameType(frame);
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::createFrame(bool append)
{
    int newFrameIndex = append ? this->gfx->getFrameCount() : this->currentFrameIndex;
    this->tileset->createFrame(newFrameIndex);
    // jump to the new frame
    this->currentFrameIndex = newFrameIndex;
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::duplicateCurrentFrame(bool wholeGroup)
{
    this->currentFrameIndex = this->gfx->duplicateFrame(this->currentFrameIndex, false);

    this->updateFields();
}

void LevelCelView::replaceCurrentFrame(const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool palMod;
        D1GfxFrame *frame = new D1GfxFrame();
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(*frame, imagefilePath, &basePal, this->gfx->getPalette(), &palMod);
        if (!success) {
            delete frame;
            dProgressFail() << tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
            return;
        }
        if (frame->getWidth() != MICRO_WIDTH || frame->getHeight() != MICRO_HEIGHT) {
            delete frame;
            dProgressFail() << tr("The image must be 32px * 32px to be used as a frame.");
            return;
        }
        D1CelTilesetFrame::selectFrameType(frame);
        this->gfx->setFrame(this->currentFrameIndex, frame);
        if (palMod) {
            // update the palette
            this->pal->updateColors(basePal);
            emit this->palModified();
        }
        // update the view - done by the caller
        // this->displayFrame();
        return;
    }

    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        dProgressFail() << tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        return;
    }

    if (image.width() != MICRO_WIDTH || image.height() != MICRO_HEIGHT) {
        dProgressFail() << tr("The image must be 32px * 32px to be used as a frame.");
        return;
    }

    D1GfxFrame *frame = this->gfx->replaceFrame(this->currentFrameIndex, image);
    D1CelTilesetFrame::selectFrameType(frame);
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::removeFrame(int frameIndex)
{
    // remove the frame
    this->tileset->removeFrame(frameIndex, 0);
    // update frame index if necessary
    if (frameIndex < this->currentFrameIndex || this->currentFrameIndex == this->gfx->getFrameCount()) {
        this->currentFrameIndex = std::max(0, this->currentFrameIndex - 1);
    }
}

void LevelCelView::removeCurrentFrame(bool wholeGroup)
{
    // check if the current frame is used
    std::vector<int> frameUsers;

    this->collectFrameUsers(this->currentFrameIndex, frameUsers);

    if (!frameUsers.empty()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("The frame is used by subtile %1 (and maybe others). Are you sure you want to proceed?").arg(frameUsers.front() + 1), QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }
    // remove the current frame
    this->removeFrame(this->currentFrameIndex);
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::flipHorizontalCurrentFrame(bool wholeGroup)
{
    // flip the frame
    this->gfx->flipHorizontalFrame(this->currentFrameIndex, wholeGroup);

    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::flipVerticalCurrentFrame(bool wholeGroup)
{
    // flip the frame
    this->gfx->flipVerticalFrame(this->currentFrameIndex, wholeGroup);

    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::mergeFrames(const MergeFramesParam &params)
{
    int firstFrameIdx = params.rangeFrom;
    int lastFrameIdx = params.rangeTo;
    // assert(firstFrameIdx >= 0 && firstFrameIdx < lastFrameIdx && lastFrameIdx < this->gfx->getFrameCount());
    this->gfx->mergeFrames(firstFrameIdx, lastFrameIdx);
    if (this->currentFrameIndex > firstFrameIdx) {
        if (this->currentFrameIndex <= lastFrameIdx) {
            this->currentFrameIndex = firstFrameIdx;
        } else {
            this->currentFrameIndex -= lastFrameIdx - firstFrameIdx;
        }
    }
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::createSubtile(bool append)
{
    int newSubtileIndex = append ? this->min->getSubtileCount() : this->currentSubtileIndex;
    this->tileset->createSubtile(newSubtileIndex);
    // jump to the new subtile
    this->currentSubtileIndex = newSubtileIndex;
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::duplicateCurrentSubtile(bool deepCopy)
{
    this->currentSubtileIndex = this->tileset->duplicateSubtile(this->currentSubtileIndex, deepCopy);

    this->updateFields();
}

void LevelCelView::replaceCurrentSubtile(const QString &imagefilePath)
{
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, &basePal, this->gfx->getPalette(), &palMod);
        if (!success) {
            dProgressFail() << tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
            return;
        }
        if (frame.getWidth() != subtileWidth || frame.getHeight() != subtileHeight) {
            dProgressFail() << tr("The image must be %1px * %2px to be used as a subtile.").arg(subtileWidth).arg(subtileHeight);
            return;
        }
        int subtileIndex = this->currentSubtileIndex;
        this->assignFrames(frame, subtileIndex, this->gfx->getFrameCount());
        if (palMod) {
            // update the palette
            this->pal->updateColors(basePal);
            emit this->palModified();
        }
        this->tileset->resetSubtileFlags(subtileIndex);
        // update the view - done by the caller
        // this->displayFrame();
        return;
    }

    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        dProgressFail() << tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        return;
    }

    if (image.width() != subtileWidth || image.height() != subtileHeight) {
        dProgressFail() << tr("The image must be %1px * %2px to be used as a subtile.").arg(subtileWidth).arg(subtileHeight);
        return;
    }

    int subtileIndex = this->currentSubtileIndex;
    this->assignFrames(image, subtileIndex, this->gfx->getFrameCount());

    this->tileset->resetSubtileFlags(subtileIndex);

    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::removeSubtile(int subtileIndex)
{
    this->tileset->removeSubtile(subtileIndex, 0);
    if (this->dun != nullptr) {
        this->dun->subtileRemoved(subtileIndex);
    }

    // update subtile index if necessary
    if (subtileIndex < this->currentSubtileIndex || this->currentSubtileIndex == this->min->getSubtileCount()) {
        this->currentSubtileIndex = std::max(0, this->currentSubtileIndex - 1);
    }
}

void LevelCelView::removeCurrentSubtile()
{
    // check if the current subtile is used
    std::vector<int> subtileUsers;

    this->collectSubtileUsers(this->currentSubtileIndex, subtileUsers);

    if (!subtileUsers.empty()) {
        QMessageBox::critical(nullptr, tr("Error"), tr("The subtile is used by tile %1 (and maybe others).").arg(subtileUsers.front() + 1));
        return;
    }
    // remove the current subtile
    this->removeSubtile(this->currentSubtileIndex);
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::createTile(bool append)
{
    int newTileIndex = append ? this->til->getTileCount() : this->currentTileIndex;
    this->tileset->createTile(newTileIndex);
    // jump to the new tile
    this->currentTileIndex = newTileIndex;
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::duplicateCurrentTile(bool deepCopy)
{
    this->currentTileIndex = this->tileset->duplicateTile(this->currentTileIndex, deepCopy);

    this->updateFields();
}

void LevelCelView::replaceCurrentTile(const QString &imagefilePath)
{
    unsigned tileWidth = this->min->getSubtileWidth() * MICRO_WIDTH * TILE_WIDTH * TILE_HEIGHT;
    unsigned tileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, &basePal, this->gfx->getPalette(), &palMod);
        if (!success) {
            dProgressFail() << tr("Failed to load file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
            return;
        }
        if (frame.getWidth() != tileWidth || frame.getHeight() != tileHeight) {
            dProgressFail() << tr("The image must be %1px * %2px to be used as a tile.").arg(tileWidth).arg(tileHeight);
            return;
        }
        int tileIndex = this->currentTileIndex;
        this->assignSubtiles(frame, tileIndex, this->min->getSubtileCount());
        // reset tile flags
        this->tla->setTileProperties(tileIndex, 0);
        if (palMod) {
            // update the palette
            this->pal->updateColors(basePal);
            emit this->palModified();
        }
        // update the view - done by the caller
        // this->displayFrame();
        return;
    }

    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        dProgressFail() << tr("Failed to read file: %1.").arg(QDir::toNativeSeparators(imagefilePath));
        return;
    }

    if (image.width() != tileWidth || image.height() != tileHeight) {
        dProgressFail() << tr("The image must be %1px * %2px to be used as a tile.").arg(tileWidth).arg(tileHeight);
        return;
    }

    int tileIndex = this->currentTileIndex;
    this->assignSubtiles(image, tileIndex, this->min->getSubtileCount());

    // reset tile flags
    this->tla->setTileProperties(tileIndex, 0);

    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::removeCurrentTile()
{
    int tileIndex = this->currentTileIndex;

    this->tileset->removeTile(tileIndex);
    if (this->dun != nullptr) {
        this->dun->tileRemoved(tileIndex);
    }
    // update tile index if necessary
    if (/*tileIndex < this->currentTileIndex ||*/ this->currentTileIndex == this->til->getTileCount()) {
        this->currentTileIndex = std::max(0, this->currentTileIndex - 1);
    }
    // update the view - done by the caller
    // this->displayFrame();
}

QString LevelCelView::copyCurrentPixels(bool values) const
{
    if (this->gfx->getFrameCount() == 0) {
        return QString();
    }
    return this->gfx->getFramePixels(this->currentFrameIndex, values);
}

void LevelCelView::pasteCurrentPixels(const QString &pixels)
{
    if (this->gfx->getFrameCount() != 0) {
        this->gfx->replaceFrame(this->currentFrameIndex, pixels);
    } else {
        this->gfx->insertFrame(this->currentFrameIndex, pixels);
    }
    // update the view - done by the caller
    // this->displayFrame();
}

QImage LevelCelView::copyCurrentImage() const
{
    /*if (this->ui->tilesTabs->currentIndex() == 1)
        return this->min->getSubtileImage(this->currentSubtileIndex);*/
    if (this->gfx->getFrameCount() == 0) {
        return QImage();
    }
    return this->gfx->getFrameImage(this->currentFrameIndex);
}

void LevelCelView::pasteCurrentImage(const QImage &image)
{
    unsigned imageWidth = image.width();
    unsigned imageHeight = image.height();

    if (imageWidth == MICRO_WIDTH && imageHeight == MICRO_HEIGHT) {
        D1GfxFrame *frame;
        if (this->gfx->getFrameCount() != 0) {
            frame = this->gfx->replaceFrame(this->currentFrameIndex, image);
        } else {
            frame = this->gfx->insertFrame(this->currentFrameIndex, image);
        }
        D1CelTilesetFrame::selectFrameType(frame);
        // update the view - done by the caller
        // this->displayFrame();
        return;
    }
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if (imageWidth == subtileWidth && imageHeight == subtileHeight) {
        int subtileIndex = this->currentSubtileIndex;
        if (this->min->getSubtileCount() != 0) {
            this->assignFrames(image, subtileIndex, this->gfx->getFrameCount());
        } else {
            this->assignSubtiles(image, -1, subtileIndex);
        }

        this->tileset->resetSubtileFlags(subtileIndex);
        // update the view - done by the caller
        // this->displayFrame();
        return;
    }

    dProgressFail() << tr("The image can not be used as a frame or as a subtile.");
}

void LevelCelView::collectFrameUsers(int frameIndex, std::vector<int> &users) const
{
    unsigned frameRef = frameIndex + 1;

    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        const std::vector<unsigned> &frameReferences = this->min->getFrameReferences(i);
        for (unsigned reference : frameReferences) {
            if (reference == frameRef) {
                users.push_back(i);
                break;
            }
        }
    }
}

void LevelCelView::collectSubtileUsers(int subtileIndex, std::vector<int> &users) const
{
    for (int i = 0; i < this->til->getTileCount(); i++) {
        const std::vector<int> &subtileIndices = this->til->getSubtileIndices(i);
        for (int index : subtileIndices) {
            if (index == subtileIndex) {
                users.push_back(i);
                break;
            }
        }
    }
}

void LevelCelView::reportUsage() const
{
    ProgressDialog::incBar(tr("Scanning..."), 2);

    bool hasFrame = this->gfx->getFrameCount() > this->currentFrameIndex;
    if (hasFrame) {
        // collect users of the current frame
        std::vector<int> frameUsers;
        this->collectFrameUsers(this->currentFrameIndex, frameUsers);

        if (frameUsers.size() == 0) {
            dProgress() << tr("Frame %1 is not used by any subtile.").arg(this->currentFrameIndex + 1);
        } else {
            QString frameUses;
            for (int user : frameUsers) {
                frameUses += QString::number(user + 1) + ", ";
            }
            frameUses.chop(2);
            dProgress() << tr("Frame %1 is used by subtile %2.", "", frameUsers.size()).arg(this->currentFrameIndex + 1).arg(frameUses);
        }

        dProgress() << "\n";
    }

    ProgressDialog::incValue();

    bool hasSubtile = this->min->getSubtileCount() > this->currentSubtileIndex;
    if (hasSubtile) {
        // collect users of the current subtile
        std::vector<int> subtileUsers;
        this->collectSubtileUsers(this->currentSubtileIndex, subtileUsers);

        if (subtileUsers.size() == 0) {
            dProgress() << tr("Subtile %1 is not used by any tile.").arg(this->currentSubtileIndex + 1);
        } else {
            QString subtileUses;
            for (int user : subtileUsers) {
                subtileUses += QString::number(user + 1) + ", ";
            }
            subtileUses.chop(2);
            dProgress() << tr("Subtile %1 is used by tile %2.", "", subtileUsers.size()).arg(this->currentSubtileIndex + 1).arg(subtileUses);
        }

        dProgress() << "\n";

        // collect users of the special frame(s)
        int specType = this->sla->getSpecProperty(this->currentSubtileIndex);
        if (specType != 0) {
            if (specType < 0) {
                // collect users of any special frame
                std::map<int, std::vector<int>> specUsers;
                for (int i = 0; i < this->min->getSubtileCount(); i++) {
                    specType = this->sla->getSpecProperty(i);
                    if (specType > 0) {
                        specUsers[specType].push_back(i);
                    }
                }
                if (specUsers.empty()) {
                    dProgress() << tr("No valid special frame use in the tileset.");
                } else {
                    for (auto it = specUsers.cbegin(); it != specUsers.end(); it++) {
                        QString specUses;
                        for (int user : it->second) {
                            specUses += QString::number(user + 1) + ", ";
                        }
                        specUses.chop(2);
                        dProgress() << tr("Special-Frame %1 is used by subtile %2.", "", it->second.size()).arg(it->first).arg(specUses);
                    }
                }
            } else {
                // collect users of the 'current' special frame
                std::vector<int> specUsers;
                for (int i = 0; i < this->min->getSubtileCount(); i++) {
                    if (this->sla->getSpecProperty(i) == specType) {
                        specUsers.push_back(i);
                    }
                }
                QString specUses;
                for (int user : specUsers) {
                    specUses += QString::number(user + 1) + ", ";
                }
                specUses.chop(2);
                dProgress() << tr("Special-Frame %1 is used by subtile %2.", "", specUsers.size()).arg(specType).arg(specUses);
            }
        }
    }

    if (!hasFrame && !hasSubtile) {
        dProgress() << tr("The tileset is empty.");
    }

    ProgressDialog::decBar();
}

void LevelCelView::coloredFrames(const std::pair<int, int>& colors) const
{
    ProgressDialog::incBar(tr("Checking frames..."), 1);
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    if ((unsigned)colors.first >= D1PAL_COLORS) {
        progress.second = tr("Frames with transparent pixels:");
    } else {
        progress.second = tr("Frames with pixels in the [%1..%2] color range:").arg(colors.first).arg(colors.second);
    }

    dProgress() << progress;
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        const std::vector<std::vector<D1GfxPixel>> pixelImage = this->gfx->getFramePixelImage(i);
        int numPixels = D1GfxPixel::countAffectedPixels(pixelImage, colors);
        if (numPixels != 0) {
            dProgress() << tr("Frame %1 has %n affected pixels.", "", numPixels).arg(i + 1);
            result = true;
        }
    }

    if (!result) {
        if ((unsigned)colors.first >= D1PAL_COLORS) {
            progress.second = tr("None of the frames have transparent pixel.");
        } else {
            progress.second = tr("None of the frames are using the colors [%1..%2].").arg(colors.first).arg(colors.second);
        }
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

void LevelCelView::coloredSubtiles(const std::pair<int, int>& colors) const
{
    ProgressDialog::incBar(tr("Checking subtiles..."), 1);
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    if ((unsigned)colors.first >= D1PAL_COLORS) {
        progress.second = tr("Subtiles with transparent pixels:");
    } else {
        progress.second = tr("Subtiles with pixels in the [%1..%2] color range:").arg(colors.first).arg(colors.second);
    }

    dProgress() << progress;
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        const std::vector<std::vector<D1GfxPixel>> pixelImage = this->min->getSubtilePixelImage(i);
        int numPixels = D1GfxPixel::countAffectedPixels(pixelImage, colors);
        if (numPixels != 0) {
            dProgress() << tr("Subtile %1 has %n affected pixels.", "", numPixels).arg(i + 1);
            result = true;
        }
    }

    if (!result) {
        if ((unsigned)colors.first >= D1PAL_COLORS) {
            progress.second = tr("None of the subtiles have transparent pixel.");
        } else {
            progress.second = tr("None of the subtiles are using the colors [%1..%2].").arg(colors.first).arg(colors.second);
        }
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

void LevelCelView::coloredTiles(const std::pair<int, int>& colors) const
{
    ProgressDialog::incBar(tr("Checking tiles..."), 1);
    bool result = false;

    QPair<int, QString> progress;
    progress.first = -1;
    if ((unsigned)colors.first >= D1PAL_COLORS) {
        progress.second = tr("Tiles with transparent pixels:");
    } else {
        progress.second = tr("Tiles with pixels in the [%1..%2] color range:").arg(colors.first).arg(colors.second);
    }

    dProgress() << progress;
    for (int i = 0; i < this->til->getTileCount(); i++) {
        const std::vector<std::vector<D1GfxPixel>> pixelImage = this->til->getTilePixelImage(i);
        int numPixels = D1GfxPixel::countAffectedPixels(pixelImage, colors);
        if (numPixels != 0) {
            dProgress() << tr("Tile %1 has %n affected pixels.", "", numPixels).arg(i + 1);
            result = true;
        }
    }

    if (!result) {
        if ((unsigned)colors.first >= D1PAL_COLORS) {
            progress.second = tr("None of the tiles have transparent pixel.");
        } else {
            progress.second = tr("None of the tiles are using the colors [%1..%2].").arg(colors.first).arg(colors.second);
        }
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

void LevelCelView::activeFrames() const
{
    ProgressDialog::incBar(tr("Checking frames..."), 1);
    QComboBox *cycleBox = this->dunView ? this->ui->dunPlayComboBox : this->ui->playComboBox;
    QString cycleTypeTxt = cycleBox->currentText();
    int cycleType = cycleBox->currentIndex();
    if (cycleType != 0) {
        int cycleColors = D1Pal::getCycleColors((D1PAL_CYCLE_TYPE)(cycleType - 1));
        const std::pair<int, int> colors = { 1, cycleColors - 1 };
        bool result = false;

        QPair<int, QString> progress;
        progress.first = -1;
        progress.second = tr("Active frames (using '%1' playback mode):").arg(cycleTypeTxt);

        dProgress() << progress;
        for (int i = 0; i < this->gfx->getFrameCount(); i++) {
            const std::vector<std::vector<D1GfxPixel>> pixelImage = this->gfx->getFramePixelImage(i);
            int numPixels = D1GfxPixel::countAffectedPixels(pixelImage, colors);
            if (numPixels != 0) {
                dProgress() << tr("Frame %1 has %n affected pixels.", "", numPixels).arg(i + 1);
                result = true;
            }
        }

        if (!result) {
            progress.second = tr("None of the frames are active in '%1' playback mode.").arg(cycleTypeTxt);
            dProgress() << progress;
        }
    } else {
        dProgress() << tr("Colors are not affected if the playback mode is '%1'.").arg(cycleTypeTxt);
    }

    ProgressDialog::decBar();
}

void LevelCelView::activeSubtiles() const
{
    ProgressDialog::incBar(tr("Checking subtiles..."), 1);
    QComboBox *cycleBox = this->dunView ? this->ui->dunPlayComboBox : this->ui->playComboBox;
    QString cycleTypeTxt = cycleBox->currentText();
    int cycleType = cycleBox->currentIndex();
    if (cycleType != 0) {
        int cycleColors = D1Pal::getCycleColors((D1PAL_CYCLE_TYPE)(cycleType - 1));
        const std::pair<int, int> colors = { 1, cycleColors - 1 };
        bool result = false;

        QPair<int, QString> progress;
        progress.first = -1;
        progress.second = tr("Active subtiles (using '%1' playback mode):").arg(cycleTypeTxt);

        dProgress() << progress;
        for (int i = 0; i < this->min->getSubtileCount(); i++) {
            const std::vector<std::vector<D1GfxPixel>> pixelImage = this->min->getSubtilePixelImage(i);
            int numPixels = D1GfxPixel::countAffectedPixels(pixelImage, colors);
            if (numPixels != 0) {
                QString msg = tr("Subtile %1 has %n affected pixels.", "", numPixels).arg(i + 1);
                if (this->sla->getLightRadius(i) == 0) {
                    dProgressWarn() << msg.append(tr(" The subtile is not lit."));
                } else {
                    dProgress() << msg.append(tr(" The subtile is lit."));
                }
                result = true;
            }
        }

        if (!result) {
            progress.second = tr("None of the subtiles are active in '%1' playback mode.").arg(cycleTypeTxt);
            dProgress() << progress;
        }
    } else {
        dProgress() << tr("Colors are not affected if the playback mode is '%1'.").arg(cycleTypeTxt);

        QPair<int, QString> progress;
        progress.first = -1;
        progress.second = tr("Lit subtiles:");

        dProgress() << progress;
        std::map<int, std::set<int>> radii;
        for (int i = 0; i < this->sla->getSubtileCount(); i++) {
            int radius = this->sla->getLightRadius(i);
            radii[radius].insert(i);
        }
        radii.erase(0);
        if (!radii.empty()) {
            for (auto it = radii.cbegin(); it != radii.cend(); it++) {
                QString msg = tr("Radius %1: ").arg(it->first);
                for (auto sit = it->second.cbegin(); sit != it->second.cend(); sit++) {
                    msg = msg.append("%1, ").arg((*sit) + 1);
                }
                msg.chop(2);
                dProgress() << msg;
            }
        } else {
            progress.second = tr("None of the subtiles are lit.");
            dProgress() << progress;
        }
    }

    ProgressDialog::decBar();
}

void LevelCelView::activeTiles() const
{
    ProgressDialog::incBar(tr("Checking tiles..."), 1);
    QComboBox *cycleBox = this->dunView ? this->ui->dunPlayComboBox : this->ui->playComboBox;
    QString cycleTypeTxt = cycleBox->currentText();
    int cycleType = cycleBox->currentIndex();
    if (cycleType != 0) {
        int cycleColors = D1Pal::getCycleColors((D1PAL_CYCLE_TYPE)(cycleType - 1));
        const std::pair<int, int> colors = { 1, cycleColors - 1 };
        bool result = false;

        QPair<int, QString> progress;
        progress.first = -1;
        progress.second = tr("Active tiles (using '%1' playback mode):").arg(cycleTypeTxt);

        dProgress() << progress;
        for (int i = 0; i < this->til->getTileCount(); i++) {
            const std::vector<std::vector<D1GfxPixel>> pixelImage = this->til->getTilePixelImage(i);
            int numPixels = D1GfxPixel::countAffectedPixels(pixelImage, colors);
            if (numPixels != 0) {
                dProgress() << tr("Tile %1 has %n affected pixels.", "", numPixels).arg(i + 1);
                result = true;
            }
        }

        if (!result) {
            progress.second = tr("None of the tiles are active in '%1' playback mode.").arg(cycleTypeTxt);
            dProgress() << progress;
        }
    } else {
        dProgress() << tr("Colors are not affected if the playback mode is '%1'.").arg(cycleTypeTxt);
    }

    ProgressDialog::decBar();
}

void LevelCelView::inefficientFrames() const
{
    ProgressDialog::incBar(tr("Scanning frames..."), 1);

    const int limit = 10;
    bool result = false;
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        const D1GfxFrame *frame = this->gfx->getFrame(i);
        if (frame->getFrameType() != D1CEL_FRAME_TYPE::TransparentSquare) {
            continue;
        }
        int diff = limit;
        D1CEL_FRAME_TYPE effType = D1CelTilesetFrame::altFrameType(frame, &diff);
        if (effType != D1CEL_FRAME_TYPE::TransparentSquare) {
            diff = limit - diff;
            dProgress() << tr("Frame %1 could be '%2' by changing %n pixel(s).", "", diff).arg(i + 1).arg(D1GfxFrame::frameTypeToStr(effType));
            result = true;
        }
    }
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        const D1GfxFrame *frame1 = this->gfx->getFrame(i);
        for (int j = i + 1; j < this->gfx->getFrameCount(); j++) {
            int diff = limit;
            const D1GfxFrame *frame2 = this->gfx->getFrame(j);
            if (D1CelTilesetFrame::altFrame(frame1, frame2, &diff)) {
                diff = limit - diff;
                dProgress() << tr("The difference between Frame %1 and Frame %2 is only %n pixel(s).", "", diff).arg(i + 1).arg(j + 1);
                result = true;
            }
        }
    }
    if (!result) {
        dProgress() << tr("The frames are optimal.");
    }

    ProgressDialog::decBar();
}

void LevelCelView::resetFrameTypes()
{
    ProgressDialog::incBar(tr("Checking frames..."), 1);

    bool result = false;
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        D1GfxFrame *frame = this->gfx->getFrame(i);
        D1CEL_FRAME_TYPE prevType = frame->getFrameType();
        D1CelTilesetFrame::selectFrameType(frame);
        D1CEL_FRAME_TYPE newType = frame->getFrameType();
        if (prevType != newType) {
            dProgress() << tr("Changed Frame %1 from '%2' to '%3'.").arg(i + 1).arg(D1GfxFrame::frameTypeToStr(prevType)).arg(D1GfxFrame::frameTypeToStr(newType));
            result = true;
        }
    }
    if (!result) {
        dProgress() << tr("No change was necessary.");
    } else {
        if (this->gfx->isUpscaled()) {
            this->gfx->setModified();
        } else {
            this->min->setModified();
        }
        // update the view
        // this->updateLabel();
        // this->tabFrameWidget.updateFields();
        this->updateFields();
    }

    ProgressDialog::decBar();
}

void LevelCelView::lightSubtiles()
{
    this->tilesetLightDialog.initialize(this->tileset);
    this->tilesetLightDialog.show();
}

static int leftFoliagePixels(const D1GfxFrame *frame)
{
    int result = 0;
    for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            if (!frame->getPixel(x, y).isTransparent()) {
                if (x < (MICRO_WIDTH - y * 2)) {
                    result++;
                }
            }
        }
    }
    for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            if (!frame->getPixel(x, y).isTransparent()) {
                if (x < (y * 2 - MICRO_WIDTH)) {
                    result++;
                }
            }
        }
    }
    return result;
}

static int rightFoliagePixels(const D1GfxFrame *frame)
{
    int result = 0;
    for (int y = 0; y < MICRO_HEIGHT / 2; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            if (!frame->getPixel(x, y).isTransparent()) {
                if (x >= y * 2) {
                    result++;
                }
            }
        }
    }
    for (int y = MICRO_HEIGHT / 2; y < MICRO_HEIGHT; y++) {
        for (int x = 0; x < MICRO_WIDTH; x++) {
            if (!frame->getPixel(x, y).isTransparent()) {
                if (x >= (2 * MICRO_WIDTH - y * 2)) {
                    result++;
                }
            }
        }
    }
    return result;
}

void LevelCelView::warnOrReportSubtile(const QString &msg, int subtileIndex) const
{
    std::vector<int> subtileUsers;
    this->collectSubtileUsers(subtileIndex, subtileUsers);
    bool unused = subtileUsers.empty();
    if (unused) {
        dProgress() << msg;
    } else {
        dProgressWarn() << msg;
    }
}

void LevelCelView::checkSubtileFlags() const
{
    ProgressDialog::incBar(tr("Checking Collision settings..."), 1);
    bool result = false;
    unsigned floorMicros = this->min->getSubtileWidth() * this->min->getSubtileWidth() / 2;

    // SOL:
    QPair<int, QString> progress;
    progress.first = -1;
    progress.second = tr("Inconsistencies in the Collision settings:");
    dProgress() << progress;
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        const std::vector<unsigned> &frameRefs = this->min->getFrameReferences(i);
        quint8 solFlags = this->sla->getSubProperties(i);
        std::vector<int> subtileUsers;
        this->collectSubtileUsers(i, subtileUsers);
        bool unused = subtileUsers.empty();
        if (solFlags & PSF_BLOCK_LIGHT) {
            // block light
            if (!(solFlags & PSF_BLOCK_PATH)) {
                this->warnOrReportSubtile(tr("Subtile %1 blocks the light, but still passable (not solid).").arg(i + 1), i);
                result = true;
            }
            if (!(solFlags & PSF_BLOCK_MISSILE)) {
                this->warnOrReportSubtile(tr("Subtile %1 blocks the light, but it does not block missiles.").arg(i + 1), i);
                result = true;
            }
        }
        if (solFlags & PSF_BLOCK_MISSILE) {
            // block missile
            if (!(solFlags & PSF_BLOCK_PATH)) {
                this->warnOrReportSubtile(tr("Subtile %1 blocks missiles, but still passable (not solid).").arg(i + 1), i);
                result = true;
            }
        }
        // checks for non-upscaled tilesets
        if (!this->gfx->isUpscaled()) {
            if (solFlags & (PSF_BLOCK_LIGHT | PSF_BLOCK_MISSILE)) {
                // block light or missile
                // - at least one not transparent frame above the floor
                bool hasColor = this->sla->getSpecProperty(i) != 0;
                for (unsigned n = 0; n < frameRefs.size() - floorMicros; n++) {
                    unsigned frameRef = frameRefs[n];
                    if (frameRef == 0) {
                        continue;
                    }
                    hasColor = true;
                    break;
                }
                if (!hasColor) {
                    this->warnOrReportSubtile(tr("Subtile %1 blocks the light or missiles, but it is completely transparent above the floor.").arg(i + 1), i);
                    result = true;
                }
            }
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected in the Collision settings.");
        dProgress() << progress;
    }

    ProgressDialog::decBar();
    ProgressDialog::incBar(tr("Checking Special settings..."), 1);

    // SPT:
    result = false;
    progress.first = -1;
    progress.second = tr("Inconsistencies in the Special settings:");
    dProgress() << progress;
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        int trapFlags = this->sla->getTrapProperty(i);
        if (trapFlags != PST_NONE) {
            const std::vector<unsigned> &frameRefs = this->min->getFrameReferences(i);
            if (trapFlags == PST_LEFT && this->min->getSubtileHeight() > 1) {
                // - one above the floor is square (left or right)
                unsigned frameRefLeft = frameRefs[frameRefs.size() - 2 * floorMicros];
                bool trapLeft = frameRefLeft != 0 && this->gfx->getFrame(frameRefLeft - 1)->getFrameType() == D1CEL_FRAME_TYPE::Square;
                if (!trapLeft) {
                    this->warnOrReportSubtile(tr("Subtile %1 is for traps, but the frames above the floor is not square on the left side.").arg(i + 1), i);
                    result = true;
                }
            } else if (trapFlags == PST_RIGHT && this->min->getSubtileHeight() > 1) {
                // - one above the floor is square (left or right)
                unsigned frameRefRight = frameRefs[frameRefs.size() - (floorMicros + 1)];
                bool trapRight = frameRefRight != 0 && this->gfx->getFrame(frameRefRight - 1)->getFrameType() == D1CEL_FRAME_TYPE::Square;
                if (!trapRight) {
                    this->warnOrReportSubtile(tr("Subtile %1 is for traps, but the frames above the floor is not square on the right side.").arg(i + 1), i);
                    result = true;
                }
            } else {
                dProgressErr() << tr("Subtile %1 has an invalid trap-setting [%2:%3].").arg(i + 1).arg((trapFlags & PST_LEFT) != 0).arg((trapFlags & PST_RIGHT) != 0);
                result = true;
            }
        }
        int specFrame = this->sla->getSpecProperty(i);
        if ((unsigned)specFrame > PST_SPEC_TYPE) {
            dProgressErr() << tr("Subtile %1 has a too high special cel-frame setting: %2. Limit it %3").arg(i + 1).arg(specFrame).arg(PST_SPEC_TYPE);
            result = true;
        } else if (specFrame != 0 && this->cls->getFrameCount() < specFrame) {
            dProgressErr() << tr("The special cel-frame (%1) referenced by Subtile %2 does not exist.").arg(specFrame).arg(i + 1);
            result = true;
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected in the Special settings.");
        dProgress() << progress;
    }

    ProgressDialog::decBar();
    ProgressDialog::incBar(tr("Checking Render settings..."), 1);

    // TMI:
    result = false;
    progress.first = -1;
    progress.second = tr("Inconsistencies in the Render settings:");
    dProgress() << progress;
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        const std::vector<unsigned> &frameRefs = this->min->getFrameReferences(i);
        quint8 tmiFlags = this->sla->getRenderProperties(i);
        int specFrame = this->sla->getSpecProperty(i);
        if (tmiFlags & TMIF_WALL_TRANS) {
            // transp.wall
            // - at least one not transparent frame above the floor
            bool hasColor = false;
            for (unsigned n = 0; n < frameRefs.size() - floorMicros; n++) {
                unsigned frameRef = frameRefs[n];
                if (frameRef == 0) {
                    continue;
                }
                hasColor = true;
                break;
            }
            if (!hasColor) {
                this->warnOrReportSubtile(tr("Subtile %1 has wall transparency set, but it is completely transparent above the floor.").arg(i + 1), i);
                result = true;
            }
        }
        if (tmiFlags & TMIF_LEFT_WALL_TRANS) {
            // left transparency
            // - wall transparency must be set
            if (!(tmiFlags & TMIF_WALL_TRANS)) {
                this->warnOrReportSubtile(tr("Subtile %1 has floor transparency on the left side, but no wall transparency.").arg(i + 1), i);
                result = true;
            }
        }
        if (tmiFlags & TMIF_RIGHT_WALL_TRANS) {
            // right transparency
            // - wall transparency must be set
            if (!(tmiFlags & TMIF_WALL_TRANS)) {
                this->warnOrReportSubtile(tr("Subtile %1 has floor transparency on the right side, but no wall transparency.").arg(i + 1), i);
                result = true;
            }
        }
        // checks for non-upscaled tilesets
        if (!this->gfx->isUpscaled() && floorMicros == 2) {
            unsigned frameRefLeft = frameRefs[frameRefs.size() - 2];
            unsigned frameRefRight = frameRefs[frameRefs.size() - 1];
            D1CEL_FRAME_TYPE leftType = D1CEL_FRAME_TYPE::Empty;
            D1CEL_FRAME_TYPE rightType = D1CEL_FRAME_TYPE::Empty;
            int leftPixels = 0;
            int rightPixels = 0;
            if (frameRefLeft != 0) {
                leftPixels = leftFoliagePixels(this->gfx->getFrame(frameRefLeft - 1));
                leftType = this->gfx->getFrame(frameRefLeft - 1)->getFrameType();
            }
            if (frameRefRight != 0) {
                rightPixels = rightFoliagePixels(this->gfx->getFrame(frameRefRight - 1));
                rightType = this->gfx->getFrame(frameRefRight - 1)->getFrameType();
            }
            bool leftAbove = false;
            bool rightAbove = false;
            for (unsigned n = 0; n < frameRefs.size() - floorMicros; n++) {
                unsigned frameRef = frameRefs[n];
                if (frameRef == 0) {
                    continue;
                }
                if (n & 1)
                    rightAbove = true;
                else
                    leftAbove = true;
            }
            if (tmiFlags & TMIF_WALL_TRANS) {
                // transp.wall
                // - trapezoid floor on the left without transparency
                if ((/*(leftType == D1CEL_FRAME_TYPE::LeftTrapezoid || */ (leftAbove && leftPixels > 30)) && !(tmiFlags & TMIF_LEFT_WALL_TRANS)) {
                    this->warnOrReportSubtile(tr("Subtile %1 has transparency on the wall while the frames above the left floor are not empty, but the left floor with many (%2) foliage pixels does not have transparency.").arg(i + 1).arg(leftPixels), i);
                    result = true;
                }
                // - trapezoid floor on the right without transparency
                if ((/*(rightType == D1CEL_FRAME_TYPE::RightTrapezoid || */ (rightAbove && rightPixels > 30)) && !(tmiFlags & TMIF_RIGHT_WALL_TRANS)) {
                    this->warnOrReportSubtile(tr("Subtile %1 has transparency on the wall while the frames above the right floor are not empty, but the right floor with many (%2) foliage pixels does not have transparency.").arg(i + 1).arg(rightPixels), i);
                    result = true;
                }
            }
            if (tmiFlags & TMIF_LEFT_REDRAW) {
                if (leftType == D1CEL_FRAME_TYPE::Empty) {
                    this->warnOrReportSubtile(tr("Subtile %1 has second pass set on the left side, but the floor-frame is empty.").arg(i + 1), i);
                    result = true;
                } else if (leftPixels == 0 && !leftAbove && specFrame == 0 && !(tmiFlags & TMIF_RIGHT_REDRAW)) {
                    this->warnOrReportSubtile(tr("Subtile %1 has second pass set on the left side, but it is just a left triangle on the floor and the right side is not redrawn.").arg(i + 1), i);
                    result = true;
                }
            }
            if (tmiFlags & TMIF_RIGHT_REDRAW) {
                if (rightType == D1CEL_FRAME_TYPE::Empty) {
                    this->warnOrReportSubtile(tr("Subtile %1 has second pass set on the right side, but the floor-frame is empty.").arg(i + 1), i);
                    result = true;
                } else if (rightPixels == 0 && !rightAbove && specFrame == 0 && !(tmiFlags & TMIF_LEFT_REDRAW)) {
                    this->warnOrReportSubtile(tr("Subtile %1 has second pass set on the right side, but it is just a right triangle on the floor and the left side is not redrawn.").arg(i + 1), i);
                    result = true;
                }
            }
            if (tmiFlags & TMIF_LEFT_FOLIAGE) {
                // left foliage
                // - must have second pass set
                if (!(tmiFlags & TMIF_LEFT_REDRAW)) {
                    this->warnOrReportSubtile(tr("Subtile %1 has foliage set on the left, but not second pass.").arg(i + 1), i);
                    result = true;
                }
                // - left floor has a foliage pixel
                if (leftPixels == 0) {
                    this->warnOrReportSubtile(tr("Subtile %1 has foliage set on the left, but no foliage pixel on the (left-)floor.").arg(i + 1), i);
                    result = true;
                }
            }
            if (tmiFlags & TMIF_LEFT_WALL_TRANS) {
                // left floor transparency
                // - must have non-transparent bits above the (left-)floor
                if (!leftAbove) {
                    this->warnOrReportSubtile(tr("Subtile %1 has left floor transparency set, but the left side is completely transparent above the floor.").arg(i + 1), i);
                    result = true;
                }
                // - must have wall transparency set if there are non-transparent bits above the floor
                if (leftAbove && !(tmiFlags & TMIF_WALL_TRANS)) {
                    this->warnOrReportSubtile(tr("Subtile %1 has left floor transparency set, but the parts above the floor are not going to be transparent (wall transparency not set).").arg(i + 1), i);
                    result = true;
                }
            }
            if (tmiFlags & TMIF_RIGHT_FOLIAGE) {
                // right foliage
                // - must have second pass set
                if (!(tmiFlags & TMIF_RIGHT_REDRAW)) {
                    this->warnOrReportSubtile(tr("Subtile %1 has foliage set on the right, but not second pass.").arg(i + 1), i);
                    result = true;
                }
                // - right floor has a foliage pixel
                if (rightPixels == 0) {
                    this->warnOrReportSubtile(tr("Subtile %1 has foliage set on the right, but no foliage pixel on the (right-)floor.").arg(i + 1), i);
                    result = true;
                }
            }
            if (tmiFlags & TMIF_RIGHT_WALL_TRANS) {
                // right floor transparency
                // - must have non-transparent bits above the (right-)floor
                if (!rightAbove) {
                    this->warnOrReportSubtile(tr("Subtile %1 has right floor transparency set, but the right side is completely transparent above the floor.").arg(i + 1), i);
                    result = true;
                }
                // - must have wall transparency set if there are non-transparent bits above the floor
                if (rightAbove && !(tmiFlags & TMIF_WALL_TRANS)) {
                    this->warnOrReportSubtile(tr("Subtile %1 has right floor transparency set, but the parts above the floor are not going to be transparent (wall transparency not set).").arg(i + 1), i);
                    result = true;
                }
            }
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected in the Render settings.");
        dProgress() << progress;
    }

    ProgressDialog::decBar();
    ProgressDialog::incBar(tr("Checking Map settings..."), 1);

    // SMP:
    result = false;
    progress.first = -1;
    progress.second = tr("Inconsistencies in the Map settings:");
    dProgress() << progress;
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        int mapType = this->sla->getMapType(i);
        if (mapType == MAT_NONE) {
            if ((this->sla->getSubProperties(i) & PSF_BLOCK_LIGHT) && this->sla->getMapProperties(i) == 0) {
                this->warnOrReportSubtile(tr("Subtile %1 is plain light blocker, but it has no walls.").arg(i + 1), i);
                result = true;
            }
        }
        if (mapType == MAT_EXTERN) {
            if (!(this->sla->getSubProperties(i) & PSF_BLOCK_LIGHT)) {
                this->warnOrReportSubtile(tr("Subtile %1 is marked extern, but it does not block light.").arg(i + 1), i);
                result = true;
            }
        }
        if (mapType == MAT_DOOR_EAST || mapType == MAT_DOOR_WEST) {
            if (this->sla->getMapProperties(i) != 0) {
                this->warnOrReportSubtile(tr("Subtile %1 is for doors, but it has also walls.").arg(i + 1), i);
                result = true;
            }

            if (!(this->sla->getSubProperties(i) & PSF_BLOCK_LIGHT)) {
                this->warnOrReportSubtile(tr("Subtile %1 is for doors, but it does not block light.").arg(i + 1), i);
                result = true;
            }

            if (i == 0 || this->sla->getMapType(i - 1) != mapType) {
                int nl = i;
                while (true) {
                    nl++;
                    if (nl >= this->min->getSubtileCount() || this->sla->getMapType(nl) != mapType) {
                        break;
                    }
                }
                nl--;
                for (int n = i; n <= nl; n++) {
                    if (this->sla->getSubProperties(n) & PSF_BLOCK_PATH) {
                        if (n == i) {
                            if (mapType == MAT_DOOR_EAST) {
                                this->warnOrReportSubtile(tr("Subtile %1 is for closed east-doors, but Subtile %2 is not for open east-doors.").arg(n + 1).arg(n), n);
                                result = true;
                            } else {
                                this->warnOrReportSubtile(tr("Subtile %1 is for closed west-doors, but Subtile %2 is not for open west-doors.").arg(n + 1).arg(n), n);
                                result = true;
                            }
                        } else {
                            if (this->sla->getSubProperties(n - 1) & PSF_BLOCK_PATH) {
                                this->warnOrReportSubtile(tr("Subtile %1 is for closed doors, but Subtile %2 is a path-blocker.").arg(n + 1).arg(n), n);
                                result = true;
                            }
                        }
                    } else {
                        if (n == nl) {
                            if (mapType == MAT_DOOR_EAST) {
                                this->warnOrReportSubtile(tr("Subtile %1 is for open east-doors, but Subtile %2 is not for closed east-doors.").arg(n + 1).arg(n + 2), n);
                                result = true;
                            } else {
                                this->warnOrReportSubtile(tr("Subtile %1 is for open west-doors, but Subtile %2 is not for closed west-doors.").arg(n + 1).arg(n + 2), n);
                                result = true;
                            }
                        } else {
                            if (!(this->sla->getSubProperties(n + 1) & PSF_BLOCK_PATH)) {
                                this->warnOrReportSubtile(tr("Subtile %1 is for open doors, but Subtile %2 is not a path-blocker.").arg(n + 1).arg(n + 2), n);
                                result = true;
                            }
                        }
                    }
                }
            }
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected in the Map settings.");
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

void LevelCelView::checkTileFlags() const
{
    ProgressDialog::incBar(tr("Checking TLA flags..."), 1);
    bool result = false;

    // TLA:
    QPair<int, QString> progress;
    progress.first = -1;
    progress.second = tr("TLA inconsistencies:");
    dProgress() << progress;
    for (int i = 0; i < this->til->getTileCount(); i++) {
        quint8 tlaFlags = this->tla->getTileProperties(i);
        const std::vector<int> &subtileIndices = this->til->getSubtileIndices(i);

        if (subtileIndices.size() > 0) {
            int subtileIdx = subtileIndices[0];
            if (tlaFlags & TIF_FLOOR_00) {
                if (subtileIdx != UNDEF_SUBTILE && (this->sla->getSubProperties(subtileIdx) & PSF_BLOCK_PATH) != 0) {
                    dProgressWarn() << tr("Unreachable Subtile %1 in Tile %2 propagates the room-index.").arg(subtileIdx + 1).arg(i + 1);
                    result = true;
                }
            } else {
                if (subtileIdx != UNDEF_SUBTILE && (this->sla->getSubProperties(subtileIdx) & PSF_BLOCK_PATH) == 0 && this->sla->getSpecProperty(subtileIdx) == 0) {
                    dProgressWarn() << tr("Walkable Subtile %1 in Tile %2 does not propagate the room-index.").arg(subtileIdx + 1).arg(i + 1);
                    result = true;
                }
            }
        }
        if (subtileIndices.size() > 1) {
            int subtileIdx = subtileIndices[1];
            if (tlaFlags & TIF_FLOOR_01) {
                if (subtileIdx != UNDEF_SUBTILE && (this->sla->getSubProperties(subtileIdx) & PSF_BLOCK_PATH) != 0) {
                    dProgressWarn() << tr("Unreachable Subtile %1 in Tile %2 propagates the room-index.").arg(subtileIdx + 1).arg(i + 1);
                    result = true;
                }
            } else {
                if (subtileIdx != UNDEF_SUBTILE && (this->sla->getSubProperties(subtileIdx) & PSF_BLOCK_PATH) == 0 && this->sla->getSpecProperty(subtileIdx) == 0) {
                    dProgressWarn() << tr("Walkable Subtile %1 in Tile %2 does not propagate the room-index.").arg(subtileIdx + 1).arg(i + 1);
                    result = true;
                }
            }
        }
        if (subtileIndices.size() > 2) {
            int subtileIdx = subtileIndices[2];
            if (tlaFlags & TIF_FLOOR_10) {
                if (subtileIdx != UNDEF_SUBTILE && (this->sla->getSubProperties(subtileIdx) & PSF_BLOCK_PATH) != 0) {
                    dProgressWarn() << tr("Unreachable Subtile %1 in Tile %2 propagates the room-index.").arg(subtileIdx + 1).arg(i + 1);
                    result = true;
                }
            } else {
                if (subtileIdx != UNDEF_SUBTILE && (this->sla->getSubProperties(subtileIdx) & PSF_BLOCK_PATH) == 0 && this->sla->getSpecProperty(subtileIdx) == 0) {
                    dProgressWarn() << tr("Walkable Subtile %1 in Tile %2 does not propagate the room-index.").arg(subtileIdx + 1).arg(i + 1);
                    result = true;
                }
            }
        }
        if (subtileIndices.size() > 3) {
            int subtileIdx = subtileIndices[3];
            if (tlaFlags & TIF_FLOOR_11) {
                if (subtileIdx != UNDEF_SUBTILE && (this->sla->getSubProperties(subtileIdx) & PSF_BLOCK_PATH) != 0) {
                    dProgressWarn() << tr("Unreachable Subtile %1 in Tile %2 propagates the room-index.").arg(subtileIdx + 1).arg(i + 1);
                    result = true;
                }
            } else {
                if (subtileIdx != UNDEF_SUBTILE && (this->sla->getSubProperties(subtileIdx) & PSF_BLOCK_PATH) == 0 && this->sla->getSpecProperty(subtileIdx) == 0) {
                    dProgressWarn() << tr("Walkable Subtile %1 in Tile %2 does not propagate the room-index.").arg(subtileIdx + 1).arg(i + 1);
                    result = true;
                }
            }
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected in the TLA flags.");
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

void LevelCelView::checkTilesetFlags() const
{
    this->checkTileFlags();
    this->checkSubtileFlags();
}

bool LevelCelView::removeUnusedFrames()
{
    ProgressDialog::incBar(tr("Removing unused frames..."), this->gfx->getFrameCount());
    // collect every frame uses
    std::vector<bool> frameUsed;
    this->min->getFrameUses(frameUsed);
    // remove the unused frames
    int result = 0;
    for (int i = this->gfx->getFrameCount() - 1; i >= 0; i--) {
        if (ProgressDialog::wasCanceled()) {
            result |= 2;
            break;
        }
        if (!frameUsed[i]) {
            this->removeFrame(i);
            dProgress() << tr("Removed frame %1.").arg(i + 1);
            result = 1;
        }
        if (!ProgressDialog::incValue()) {
            result |= 2;
            break;
        }
    }
    ProgressDialog::decBar();
    return result != 0;
}

bool LevelCelView::removeUnusedSubtiles()
{
    ProgressDialog::incBar(tr("Removing unused subtiles..."), this->min->getSubtileCount());
    // collect every subtile uses
    std::vector<bool> subtileUsed = std::vector<bool>(this->min->getSubtileCount());
    for (int i = 0; i < this->til->getTileCount(); i++) {
        const std::vector<int> &subtileIndices = this->til->getSubtileIndices(i);
        for (int subtileIndex : subtileIndices) {
            subtileUsed[subtileIndex] = true;
        }
    }
    // remove the unused subtiles
    int result = 0;
    for (int i = this->min->getSubtileCount() - 1; i >= 0; i--) {
        if (ProgressDialog::wasCanceled()) {
            result |= 2;
            break;
        }
        if (!subtileUsed[i]) {
            this->removeSubtile(i);
            dProgress() << tr("Removed subtile %1.").arg(i + 1);
            result = 1;
        }
        if (!ProgressDialog::incValue()) {
            result |= 2;
            break;
        }
    }
    ProgressDialog::decBar();
    return result != 0;
}

void LevelCelView::cleanupFrames()
{
    if (this->removeUnusedFrames()) {
        // update the view - done by the caller
        // this->displayFrame();
    } else {
        dProgress() << tr("Every frame is used.");
    }
}

void LevelCelView::cleanupSubtiles()
{
    if (this->removeUnusedSubtiles()) {
        if (this->dun != nullptr) {
            this->dun->refreshSubtiles();
        }
        // update the view - done by the caller
        // this->displayFrame();
    } else {
        dProgress() << tr("Every subtile is used.");
    }
}

void LevelCelView::cleanupTileset()
{
    ProgressDialog::incBar(tr("Scanning tileset..."), 2);

    bool removedSubtile = this->removeUnusedSubtiles();

    if (removedSubtile) {
        dProgress() << "\n";
    }

    ProgressDialog::incValue();

    bool removedFrame = this->removeUnusedFrames();

    if (removedSubtile || removedFrame) {
        if (removedSubtile && this->dun != nullptr) {
            this->dun->refreshSubtiles();
        }
        // update the view - done by the caller
        // this->displayFrame();
    } else {
        dProgress() << tr("Every subtile and frame are used.");
    }

    ProgressDialog::decBar();
}

bool LevelCelView::reuseFrames()
{
    std::set<int> removedIndices;

    bool result = this->tileset->reuseFrames(removedIndices, false);

    // update frame index if necessary
    auto it = removedIndices.lower_bound(this->currentFrameIndex);
    if (it != removedIndices.begin()) {
        if (*it == this->currentFrameIndex)
            it--;
        this->currentFrameIndex -= std::distance(removedIndices.begin(), it);
    }

    return result;
}

bool LevelCelView::reuseSubtiles()
{
    std::map<unsigned, unsigned> removedIndices;
    bool result = this->tileset->reuseSubtiles(removedIndices);
    if (this->dun != nullptr) {
        this->dun->subtilesRemapped(removedIndices);
    }

    // update subtile index if necessary
    auto it = removedIndices.lower_bound((unsigned)this->currentSubtileIndex);
    if (it != removedIndices.begin()) {
        if (it->first == (unsigned)this->currentSubtileIndex)
            it--;
        this->currentSubtileIndex -= std::distance(removedIndices.begin(), it);
    }

    return result;
}

bool LevelCelView::reuseTiles()
{
    std::map<unsigned, unsigned> removedIndices;
    bool result = this->tileset->reuseTiles(removedIndices);
    if (this->dun != nullptr) {
        this->dun->tilesRemapped(removedIndices);
    }

    // update tile index if necessary
    auto it = removedIndices.lower_bound((unsigned)this->currentTileIndex);
    if (it != removedIndices.begin()) {
        if (it->first == (unsigned)this->currentTileIndex)
            it--;
        this->currentTileIndex -= std::distance(removedIndices.begin(), it);
    }

    return result;
}

void LevelCelView::compressSubtiles()
{
    if (this->reuseFrames()) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::compressTiles()
{
    if (this->reuseSubtiles()) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::compressTileset()
{
    ProgressDialog::incBar(tr("Compressing tileset..."), 3);

    bool reusedFrame = this->reuseFrames();

    if (reusedFrame) {
        dProgress() << "\n";
    }

    ProgressDialog::incValue();

    bool reusedSubtile = this->reuseSubtiles();

    ProgressDialog::incValue();

    bool reusedTile = this->reuseTiles();

    if (reusedFrame || reusedSubtile || reusedTile) {
        // update the view - done by the caller
        // this->displayFrame();
    }

    ProgressDialog::decBar();
}

bool LevelCelView::sortFrames_impl()
{
    std::map<unsigned, unsigned> remap;
    bool change = false;
    unsigned idx = 1;

    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        std::vector<unsigned> &frameReferences = this->min->getFrameReferences(i);
        for (auto sit = frameReferences.begin(); sit != frameReferences.end(); ++sit) {
            if (*sit == 0) {
                continue;
            }
            auto mit = remap.find(*sit);
            if (mit != remap.end()) {
                *sit = mit->second;
            } else {
                remap[*sit] = idx;
                change |= *sit != idx;
                *sit = idx;
                idx++;
            }
        }
    }
    if (!change) {
        return false;
    }
    this->min->setModified();
    std::map<unsigned, unsigned> backmap;
    for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
        backmap[iter->second] = iter->first;
    }
    this->gfx->remapFrames(backmap);
    return true;
}

bool LevelCelView::sortSubtiles_impl()
{
    std::map<unsigned, unsigned> remap;
    bool change = false;
    int idx = 0;

    for (int i = 0; i < this->til->getTileCount(); i++) {
        std::vector<int> &subtileIndices = this->til->getSubtileIndices(i);
        for (auto sit = subtileIndices.begin(); sit != subtileIndices.end(); ++sit) {
            auto mit = remap.find(*sit);
            if (mit != remap.end()) {
                *sit = mit->second;
            } else {
                remap[*sit] = idx;
                change |= *sit != idx;
                *sit = idx;
                idx++;
            }
        }
    }
    if (!change) {
        return false;
    }
    this->til->setModified();
    std::map<unsigned, unsigned> backmap;
    for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
        backmap[iter->second] = iter->first;
    }
    this->tileset->remapSubtiles(backmap);
    if (this->dun != nullptr) {
        this->dun->subtilesRemapped(backmap);
    }
    return true;
}

void LevelCelView::sortFrames()
{
    if (this->sortFrames_impl()) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::sortSubtiles()
{
    if (this->sortSubtiles_impl()) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::sortTileset()
{
    bool change = false;

    change |= this->sortSubtiles_impl();
    change |= this->sortFrames_impl();
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::reportDungeonUsage() const
{
    ProgressDialog::incBar(tr("Scanning..."), 5);

    std::pair<int, int> space = this->dun->collectSpace();

    if (space.first != 0) {
        dProgress() << tr("There are %n subtiles in the dungeon for monsters.", "", space.first);
        dProgress() << "\n";
    }
    if (space.second != 0) {
        dProgress() << tr("There are %n subtiles in the dungeon for objects.", "", space.first);
        dProgress() << "\n";
    }
    if (space.first == 0 && space.second == 0) {
        dProgress() << tr("There is no available space in the dungeon to generate monsters or objects.");
        dProgress() << "\n";
    }

    ProgressDialog::incValue();

    std::vector<std::pair<int, int>> items;
    this->dun->collectItems(items);

    if (items.empty()) {
        dProgress() << tr("There are no items in the dungeon.");
    } else {
        QString itemUses;
        int totalCount = 0;
        for (std::pair<int, int> &item : items) {
            totalCount += item.second;
            QString itemName = this->dun->getItemName(item.first);
            itemUses += tr("%1 %2").arg(item.second).arg(itemName) + ", ";
        }
        itemUses.chop(2);
        dProgress() << tr("There are %1 in the dungeon.", "", totalCount).arg(itemUses);
    }

    dProgress() << "\n";

    ProgressDialog::incValue();

    std::vector<std::pair<DunMonsterType, int>> monsters;
    this->dun->collectMonsters(monsters);

    if (monsters.empty()) {
        dProgress() << tr("There are no monsters in the dungeon.");
    } else {
        QString monsterUses;
        int totalCount = 0;
        for (std::pair<DunMonsterType, int> &monster : monsters) {
            totalCount += monster.second;
            QString monsterName = this->dun->getMonsterName(monster.first);
            monsterUses += tr("%1 %2").arg(monster.second).arg(monsterName) + ", ";
        }
        monsterUses.chop(2);
        dProgress() << tr("There are %1 in the dungeon.", "", totalCount).arg(monsterUses);
    }

    dProgress() << "\n";

    ProgressDialog::incValue();

    std::vector<std::pair<int, int>> objects;
    this->dun->collectObjects(objects);

    if (objects.empty()) {
        dProgress() << tr("There are no objects in the dungeon.");
    } else {
        QString objectUses;
        int totalCount = 0;
        for (std::pair<int, int> &object : objects) {
            totalCount += object.second;
            QString objectName = this->dun->getObjectName(object.first);
            objectUses += tr("%1 %2").arg(object.second).arg(objectName) + ", ";
        }
        objectUses.chop(2);
        dProgress() << tr("There are %1 in the dungeon.", "", totalCount).arg(objectUses);
    }

    dProgress() << "\n";

    ProgressDialog::incValue();

    std::map<int, int> subtiles;
    for (int y = 0; y < this->dun->getHeight(); y++) {
        for (int x = 0; x < this->dun->getWidth(); x++) {
            subtiles[this->dun->getSubtileAt(x, y)]++;
        }
    }
    subtiles.erase(UNDEF_SUBTILE);
    subtiles.erase(0);
    dProgress() << tr("Subtiles in the dungeon:");
    if (subtiles.empty()) {
        dProgress() << tr("   None.");
    } else {
        for (auto subtile : subtiles) {
            dProgress() << tr("    %1: %2").arg(subtile.first).arg(subtile.second);
        }
    }

    ProgressDialog::decBar();
}

void LevelCelView::resetDungeonTiles()
{
    bool change = this->dun->resetTiles();
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::resetDungeonSubtiles()
{
    bool change = this->dun->resetSubtiles();
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::maskDungeonTiles(const D1Dun *srcDun)
{
    bool change = this->dun->maskTilesFrom(srcDun);
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::protectDungeonTiles()
{
    bool change = this->dun->protectTiles();
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::protectDungeonTilesFrom(const D1Dun *srcDun)
{
    bool change = this->dun->protectTilesFrom(srcDun);
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::protectDungeonSubtiles()
{
    bool change = this->dun->protectSubtiles();
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::checkTiles() const
{
    this->dun->checkTiles();
}

void LevelCelView::checkProtections() const
{
    this->dun->checkProtections();
}

void LevelCelView::checkItems() const
{
    this->dun->checkItems();
}

void LevelCelView::checkMonsters() const
{
    this->dun->checkMonsters();
}

void LevelCelView::checkObjects() const
{
    this->dun->checkObjects();
}

void LevelCelView::checkEntities() const
{
    this->checkTiles();
    this->checkProtections();
    this->checkItems();
    this->checkMonsters();
    this->checkObjects();
}

void LevelCelView::removeTiles()
{
    bool change = this->dun->removeTiles();
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::removeProtections()
{
    bool change = this->dun->removeProtections();
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::removeItems()
{
    bool change = this->dun->removeItems();
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::removeMonsters()
{
    bool change = this->dun->removeMonsters();
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::removeObjects()
{
    bool change = this->dun->removeObjects();
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::removeMissiles()
{
    bool change = this->dun->removeMissiles();
    if (change) {
        // update the view - done by the caller
        // this->displayFrame();
    }
}

static bool dimensionMatch(const D1Dun *dun1, const D1Dun *dun2)
{
    // if (dun1->getWidth() == dun2->getWidth() && dun1->getHeight() == dun2->getHeight()) {
         return true;
    // }
    // QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Mismatching dungeons (Dimensions are %1:%2 vs %3:%4).").arg(dun1->getWidth()).arg(dun1->getHeight()).arg(dun2->getHeight()).arg(dun2->getWidth()));
    // return false;
}

void LevelCelView::loadTiles(const D1Dun *srcDun)
{
    if (!dimensionMatch(this->dun, srcDun)) {
        return;
    }
    this->dun->loadTiles(srcDun);
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::loadProtections(const D1Dun *srcDun)
{
    if (!dimensionMatch(this->dun, srcDun)) {
        return;
    }
    this->dun->loadProtections(srcDun);
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::loadItems(const D1Dun *srcDun)
{
    if (!dimensionMatch(this->dun, srcDun)) {
        return;
    }
    this->dun->loadItems(srcDun);
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::loadMonsters(const D1Dun *srcDun)
{
    if (!dimensionMatch(this->dun, srcDun)) {
        return;
    }
    this->dun->loadMonsters(srcDun);
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::loadObjects(const D1Dun *srcDun)
{
    if (!dimensionMatch(this->dun, srcDun)) {
        return;
    }
    this->dun->loadObjects(srcDun);
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::generateDungeon()
{
    this->dungeonGenerateDialog.initialize(this->dun, this->tileset);
    this->dungeonGenerateDialog.show();
}

void LevelCelView::decorateDungeon()
{
    this->dungeonDecorateDialog.initialize(this->dun, this->tileset);
    this->dungeonDecorateDialog.show();
}

void LevelCelView::searchDungeon()
{
    this->dungeonSearchDialog.initialize(this->dun);
    this->dungeonSearchDialog.show();
}

void LevelCelView::upscale(const UpscaleParam &params)
{
    bool change = Upscaler::upscaleTileset(this->gfx, this->min, params, false);
    if (this->tileset->cls->getFrameCount() != 0) {
        change |= Upscaler::upscaleGfx(this->tileset->cls, params, false);
    }
    if (change) {
        // std::set<int> removedIndices;
        // this->tileset->reuseFrames(removedIndices, true);
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::displayFrame()
{
    this->updateFields();

    this->celScene.clear();
    this->celScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    if (this->dunView) {
        if (this->playTimer != 0) {
            this->dun->game_logic();
        }
        DunDrawParam params;
        params.tileState = this->ui->showTilesRadioButton->isChecked() ? Qt::Checked : (this->ui->showFloorRadioButton->isChecked() ? Qt::PartiallyChecked : Qt::Unchecked);
        params.overlayType = this->ui->dunOverlayComboBox->currentIndex();
        params.showItems = this->ui->showItemsCheckBox->isChecked();
        params.showMonsters = this->ui->showMonstersCheckBox->isChecked();
        params.showObjects = this->ui->showObjectsCheckBox->isChecked();
        params.showMissiles = this->ui->showMissilesCheckBox->isChecked();
        // params.time = this->playCounter;
        QImage dunFrame = this->dun->getImage(params);

        this->celScene.setSceneRect(0, 0,
            CEL_SCENE_MARGIN + dunFrame.width() + CEL_SCENE_MARGIN,
            CEL_SCENE_MARGIN + dunFrame.height() + CEL_SCENE_MARGIN);

        // this->celScene.addPixmap(QPixmap::fromImage(dunFrame))
        //    ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
        QGraphicsPixmapItem *item;
        item = this->celScene.addPixmap(QPixmap::fromImage(dunFrame));
        item->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
        // scroll to the current position
        if (this->isScrolling) {
            this->isScrolling = false;
            this->scrollToCurrent();
        }
        return;
    }

    // Getting the current frame/sub-tile/tile to display
    QImage celFrame = this->gfx->getFrameCount() != 0 ? this->gfx->getFrameImage(this->currentFrameIndex) : QImage();
    QImage subtile = this->min->getSubtileCount() != 0 ? (this->ui->showSpecSubtileCheckBox->isChecked() ? this->min->getSpecSubtileImage(this->currentSubtileIndex) : this->min->getSubtileImage(this->currentSubtileIndex)) : QImage();
    QImage tile = this->til->getTileCount() != 0 ? (this->ui->showSpecTileCheckBox->isChecked() ? this->til->getSpecTileImage(this->currentTileIndex) : this->til->getTileImage(this->currentTileIndex)) : QImage();

    QColor backColor = QColor(Config::getGraphicsTransparentColor());
    // Building a gray background of the width/height of the CEL frame
    QImage celFrameBackground = QImage(celFrame.width(), celFrame.height(), QImage::Format_ARGB32_Premultiplied); // QImage::Format_ARGB32
    celFrameBackground.fill(backColor);
    // Building a gray background of the width/height of the MIN subtile
    QImage subtileBackground = QImage(subtile.width(), subtile.height(), QImage::Format_ARGB32_Premultiplied); // QImage::Format_ARGB32
    subtileBackground.fill(backColor);
    // Building a gray background of the width/height of the MIN subtile
    QImage tileBackground = QImage(tile.width(), tile.height(), QImage::Format_ARGB32_Premultiplied); // QImage::Format_ARGB32
    tileBackground.fill(backColor);

    // Resize the scene rectangle to include some padding around the CEL frame
    // the MIN subtile and the TIL tile
    this->celScene.setSceneRect(0, 0,
        CEL_SCENE_MARGIN + celFrame.width() + CEL_SCENE_SPACING + subtile.width() + CEL_SCENE_SPACING + tile.width() + CEL_SCENE_MARGIN,
        CEL_SCENE_MARGIN + tile.height() + CEL_SCENE_MARGIN);

    // Add the backgrond and CEL frame while aligning it in the center
    this->celScene.addPixmap(QPixmap::fromImage(celFrameBackground))
        ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    this->celScene.addPixmap(QPixmap::fromImage(celFrame))
        ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);

    // Set current frame width and height
    this->ui->celFrameWidthEdit->setText(QString::number(celFrame.width()) + " px");
    this->ui->celFrameHeightEdit->setText(QString::number(celFrame.height()) + " px");

    // MIN
    int minPosX = CEL_SCENE_MARGIN + celFrame.width() + CEL_SCENE_SPACING;
    this->celScene.addPixmap(QPixmap::fromImage(subtileBackground))
        ->setPos(minPosX, CEL_SCENE_MARGIN);
    this->celScene.addPixmap(QPixmap::fromImage(subtile))
        ->setPos(minPosX, CEL_SCENE_MARGIN);

    // Set current frame width and height
    this->ui->minFrameWidthEdit->setText(QString::number(this->min->getSubtileWidth()));
    this->ui->minFrameWidthEdit->setToolTip(QString::number(subtile.width()) + " px");
    this->ui->minFrameHeightEdit->setText(QString::number(this->min->getSubtileHeight()));
    this->ui->minFrameHeightEdit->setToolTip(QString::number(subtile.height()) + " px");

    // TIL
    int tilPosX = minPosX + subtile.width() + CEL_SCENE_SPACING;
    this->celScene.addPixmap(QPixmap::fromImage(tileBackground))
        ->setPos(tilPosX, CEL_SCENE_MARGIN);
    this->celScene.addPixmap(QPixmap::fromImage(tile))
        ->setPos(tilPosX, CEL_SCENE_MARGIN);

    // Set current frame width and height
    this->ui->tilFrameWidthEdit->setText(QString::number(TILE_WIDTH));
    this->ui->tilFrameWidthEdit->setToolTip(QString::number(tile.width()) + " px");
    this->ui->tilFrameHeightEdit->setText(QString::number(TILE_HEIGHT));
    this->ui->tilFrameHeightEdit->setToolTip(QString::number(tile.height()) + " px");

    // Notify PalView that the frame changed (used to refresh palette widget)
    emit frameRefreshed();
}

void LevelCelView::toggleBottomPanel()
{
    QWidget *layout = this->dunView ? this->ui->dungeonWidget : this->ui->tilesetWidget;
    layout->setVisible(layout->isHidden());
}

void LevelCelView::setFrameIndex(int frameIndex)
{
    const int frameCount = this->gfx->getFrameCount();
    if (frameIndex >= frameCount) {
        frameIndex = frameCount - 1;
    }
    if (frameIndex < 0) {
        frameIndex = 0;
    }
    this->currentFrameIndex = frameIndex;
    this->displayFrame();
}

void LevelCelView::setSubtileIndex(int subtileIndex)
{
    const int subtileCount = this->min->getSubtileCount();
    if (subtileIndex >= subtileCount) {
        subtileIndex = subtileCount - 1;
    }
    if (subtileIndex < 0) {
        subtileIndex = 0;
    }
    this->currentSubtileIndex = subtileIndex;
    this->displayFrame();
}

void LevelCelView::setTileIndex(int tileIndex)
{
    const int tileCount = this->til->getTileCount();
    if (tileIndex >= tileCount) {
        tileIndex = tileCount - 1;
    }
    if (tileIndex < 0) {
        tileIndex = 0;
    }
    this->currentTileIndex = tileIndex;
    this->displayFrame();
}

void LevelCelView::ShowContextMenu(const QPoint &pos)
{
    if (this->dunView) {
        QAction actions[4];
        QMenu contextMenu(this);

        int cursor = 0;
        actions[cursor].setText(tr("Insert Tile-Row"));
        actions[cursor].setToolTip(tr("Add a tile-row before the one at the current position"));
        QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionInsert_DunTileRow_triggered()));
        contextMenu.addAction(&actions[cursor]);

        cursor++;
        actions[cursor].setText(tr("Insert Tile-Column"));
        actions[cursor].setToolTip(tr("Add a tile-column before the one at the current position"));
        QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionInsert_DunTileColumn_triggered()));
        contextMenu.addAction(&actions[cursor]);

        cursor++;
        actions[cursor].setText(tr("Delete Tile-Row"));
        actions[cursor].setToolTip(tr("Delete the tile-row at the current position"));
        QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionDel_DunTileRow_triggered()));
        actions[cursor].setEnabled(this->dun->getHeight() > TILE_HEIGHT);
        contextMenu.addAction(&actions[cursor]);

        cursor++;
        actions[cursor].setText(tr("Delete Tile-Column"));
        actions[cursor].setToolTip(tr("Delete the tile-column at the current position"));
        QObject::connect(&actions[cursor], SIGNAL(triggered()), this, SLOT(on_actionDel_DunTileColumn_triggered()));
        actions[cursor].setEnabled(this->dun->getWidth() > TILE_WIDTH);
        contextMenu.addAction(&actions[cursor]);

        contextMenu.exec(mapToGlobal(pos));
        return;
    }
    MainWindow *mw = &dMainWindow();
    QAction actions[18];
    QMenu contextMenu(this);

    QMenu frameMenu(tr("Frame"), this);
    frameMenu.setToolTipsVisible(true);

    int cursor = 0;
    actions[cursor].setText(tr("Add Layer"));
    actions[cursor].setToolTip(tr("Add the content of an image to the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAddTo_Frame_triggered()));
    frameMenu.addAction(&actions[cursor]);

    frameMenu.addSeparator();

    cursor++;
    actions[cursor].setText(tr("Create"));
    actions[cursor].setToolTip(tr("Create a new frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionCreate_Frame_triggered()));
    frameMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Insert"));
    actions[cursor].setToolTip(tr("Add a new frame before the current one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionInsert_Frame_triggered()));
    frameMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Duplicate"));
    actions[cursor].setToolTip(tr("Duplicate the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDuplicate_Frame_triggered()));
    frameMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Replace"));
    actions[cursor].setToolTip(tr("Replace the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionReplace_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    frameMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Delete"));
    actions[cursor].setToolTip(tr("Delete the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDel_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    frameMenu.addAction(&actions[cursor]);

    frameMenu.addSeparator();

    cursor++;
    actions[cursor].setText(tr("Horizontal Flip"));
    actions[cursor].setToolTip(tr("Flip the current frame horizontally"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionFlipHorizontal_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    frameMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Vertical Flip"));
    actions[cursor].setToolTip(tr("Flip the current frame vertically"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionFlipVertical_Frame_triggered()));
    actions[cursor].setEnabled(this->gfx->getFrameCount() != 0);
    frameMenu.addAction(&actions[cursor]);

    contextMenu.addMenu(&frameMenu);

    QMenu subtileMenu(tr("Subtile"), this);
    subtileMenu.setToolTipsVisible(true);

    cursor++;
    actions[cursor].setText(tr("Create"));
    actions[cursor].setToolTip(tr("Create a new subtile"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionCreate_Subtile_triggered()));
    subtileMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Insert"));
    actions[cursor].setToolTip(tr("Add a new subtile before the current one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionInsert_Subtile_triggered()));
    subtileMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Duplicate"));
    actions[cursor].setToolTip(tr("Duplicate the current subtile"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDuplicate_Subtile_triggered()));
    subtileMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Replace"));
    actions[cursor].setToolTip(tr("Replace the current subtile"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionReplace_Subtile_triggered()));
    actions[cursor].setEnabled(this->min->getSubtileCount() != 0);
    subtileMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Delete"));
    actions[cursor].setToolTip(tr("Delete the current subtile"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDel_Subtile_triggered()));
    actions[cursor].setEnabled(this->min->getSubtileCount() != 0);
    subtileMenu.addAction(&actions[cursor]);

    contextMenu.addMenu(&subtileMenu);

    QMenu tileMenu(tr("Tile"), this);
    tileMenu.setToolTipsVisible(true);

    cursor++;
    actions[cursor].setText(tr("Create"));
    actions[cursor].setToolTip(tr("Create a new tile"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionCreate_Tile_triggered()));
    actions[cursor].setEnabled(this->min->getSubtileCount() != 0);
    tileMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Insert"));
    actions[cursor].setToolTip(tr("Add a new tile before the current one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionInsert_Tile_triggered()));
    tileMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Duplicate"));
    actions[cursor].setToolTip(tr("Duplicate the current tile"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDuplicate_Tile_triggered()));
    tileMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Replace"));
    actions[cursor].setToolTip(tr("Replace the current tile"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionReplace_Tile_triggered()));
    actions[cursor].setEnabled(this->til->getTileCount() != 0);
    tileMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Delete"));
    actions[cursor].setToolTip(tr("Delete the current tile"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionDel_Tile_triggered()));
    actions[cursor].setEnabled(this->til->getTileCount() != 0);
    tileMenu.addAction(&actions[cursor]);

    contextMenu.addMenu(&tileMenu);

    contextMenu.exec(mapToGlobal(pos));
}

void LevelCelView::on_showSpecTileCheckBox_clicked()
{
    // update the view
    this->displayFrame();
}

void LevelCelView::on_showSpecSubtileCheckBox_clicked()
{
    // update the view
    this->displayFrame();
}

void LevelCelView::on_firstFrameButton_clicked()
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        this->tileset->swapFrames(UINT_MAX, this->currentFrameIndex);
    }
    this->setFrameIndex(0);
}

void LevelCelView::on_previousFrameButton_clicked()
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        this->tileset->swapFrames(this->currentFrameIndex - 1, this->currentFrameIndex);
    }
    this->setFrameIndex(this->currentFrameIndex - 1);
}

void LevelCelView::on_nextFrameButton_clicked()
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        this->tileset->swapFrames(this->currentFrameIndex, this->currentFrameIndex + 1);
    }
    this->setFrameIndex(this->currentFrameIndex + 1);
}

void LevelCelView::on_lastFrameButton_clicked()
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        this->tileset->swapFrames(this->currentFrameIndex, UINT_MAX);
    }
    this->setFrameIndex(this->gfx->getFrameCount() - 1);
}

void LevelCelView::on_frameIndexEdit_returnPressed()
{
    int frameIndex = this->ui->frameIndexEdit->text().toInt() - 1;

    this->setFrameIndex(frameIndex);

    this->on_frameIndexEdit_escPressed();
}

void LevelCelView::on_frameIndexEdit_escPressed()
{
    this->ui->frameIndexEdit->setText(QString::number(this->gfx->getFrameCount() != 0 ? this->currentFrameIndex + 1 : 0));
    this->ui->frameIndexEdit->clearFocus();
}

void LevelCelView::swapSubtiles(unsigned subtileIndex0, unsigned subtileIndex1)
{
    this->tileset->swapSubtiles(subtileIndex0, subtileIndex1);
    if (this->dun != nullptr) {
        this->dun->subtilesSwapped(subtileIndex0, subtileIndex1);
    }
}

void LevelCelView::on_firstSubtileButton_clicked()
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        this->swapSubtiles(UINT_MAX, this->currentSubtileIndex);
    }
    this->setSubtileIndex(0);
}

void LevelCelView::on_previousSubtileButton_clicked()
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        this->swapSubtiles(this->currentSubtileIndex - 1, this->currentSubtileIndex);
    }
    this->setSubtileIndex(this->currentSubtileIndex - 1);
}

void LevelCelView::on_nextSubtileButton_clicked()
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        this->swapSubtiles(this->currentSubtileIndex, this->currentSubtileIndex + 1);
    }
    this->setSubtileIndex(this->currentSubtileIndex + 1);
}

void LevelCelView::on_lastSubtileButton_clicked()
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        this->swapSubtiles(this->currentSubtileIndex, UINT_MAX);
    }
    this->setSubtileIndex(this->min->getSubtileCount() - 1);
}

void LevelCelView::on_subtileIndexEdit_returnPressed()
{
    int subtileIndex = this->ui->subtileIndexEdit->text().toInt() - 1;

    this->setSubtileIndex(subtileIndex);

    this->on_subtileIndexEdit_escPressed();
}

void LevelCelView::on_subtileIndexEdit_escPressed()
{
    this->ui->subtileIndexEdit->setText(QString::number(this->min->getSubtileCount() != 0 ? this->currentSubtileIndex + 1 : 0));
    this->ui->subtileIndexEdit->clearFocus();
}

void LevelCelView::swapTiles(unsigned tileIndex0, unsigned tileIndex1)
{
    this->tileset->swapTiles(tileIndex0, tileIndex1);
    if (this->dun != nullptr) {
        this->dun->tilesSwapped(tileIndex0, tileIndex1);
    }
}

void LevelCelView::on_firstTileButton_clicked()
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        this->swapTiles(UINT_MAX, this->currentTileIndex);
    }
    this->setTileIndex(0);
}

void LevelCelView::on_previousTileButton_clicked()
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        this->swapTiles(this->currentTileIndex - 1, this->currentTileIndex);
    }
    this->setTileIndex(this->currentTileIndex - 1);
}

void LevelCelView::on_nextTileButton_clicked()
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        this->swapTiles(this->currentTileIndex, this->currentTileIndex + 1);
    }
    this->setTileIndex(this->currentTileIndex + 1);
}

void LevelCelView::on_lastTileButton_clicked()
{
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        this->swapTiles(this->currentTileIndex, UINT_MAX);
    }
    this->setTileIndex(this->til->getTileCount() - 1);
}

void LevelCelView::on_tileIndexEdit_returnPressed()
{
    int tileIndex = this->ui->tileIndexEdit->text().toInt() - 1;

    this->setTileIndex(tileIndex);

    this->on_tileIndexEdit_escPressed();
}

void LevelCelView::on_tileIndexEdit_escPressed()
{
    this->ui->tileIndexEdit->setText(QString::number(this->til->getTileCount() != 0 ? this->currentTileIndex + 1 : 0));
    this->ui->tileIndexEdit->clearFocus();
}

void LevelCelView::on_minFrameWidthEdit_returnPressed()
{
    int width = this->ui->minFrameWidthEdit->nonNegInt();

    this->min->setSubtileWidth(width);
    // update the view
    this->displayFrame();

    this->on_minFrameWidthEdit_escPressed();
}

void LevelCelView::on_minFrameWidthEdit_escPressed()
{
    this->ui->minFrameWidthEdit->setText(QString::number(this->min->getSubtileWidth()));
    this->ui->minFrameWidthEdit->clearFocus();
}

void LevelCelView::on_minFrameHeightEdit_returnPressed()
{
    int height = this->ui->minFrameHeightEdit->nonNegInt();

    this->min->setSubtileHeight(height);
    // update the view
    this->displayFrame();

    this->on_minFrameHeightEdit_escPressed();
}

void LevelCelView::on_minFrameHeightEdit_escPressed()
{
    this->ui->minFrameHeightEdit->setText(QString::number(this->min->getSubtileHeight()));
    this->ui->minFrameHeightEdit->clearFocus();
}

void LevelCelView::on_zoomOutButton_clicked()
{
    this->celScene.zoomOut();
    this->on_zoomEdit_escPressed();
}

void LevelCelView::on_zoomInButton_clicked()
{
    this->celScene.zoomIn();
    this->on_zoomEdit_escPressed();
}

void LevelCelView::on_zoomEdit_returnPressed()
{
    QString zoom = this->ui->zoomEdit->text();

    this->celScene.setZoom(zoom);

    this->on_zoomEdit_escPressed();
}

void LevelCelView::on_zoomEdit_escPressed()
{
    this->ui->zoomEdit->setText(this->celScene.zoomText());
    this->ui->zoomEdit->clearFocus();
}

void LevelCelView::on_playDelayEdit_returnPressed()
{
    quint16 playDelay = this->ui->playDelayEdit->text().toUInt();

    if (playDelay != 0)
        this->tilesetPlayDelay = playDelay;

    this->on_playDelayEdit_escPressed();
}

void LevelCelView::on_playDelayEdit_escPressed()
{
    this->ui->playDelayEdit->setText(QString::number(this->tilesetPlayDelay));
    this->ui->playDelayEdit->clearFocus();
}

void LevelCelView::on_playStopButton_clicked()
{
    if (this->playTimer != 0) {
        this->killTimer(this->playTimer);
        this->playTimer = 0;
        this->playCounter = 0;

        // restore palette
        dMainWindow().resetPaletteCycle();
        // change the label of the button
        this->ui->playStopButton->setText(tr("Play"));
        this->ui->dunPlayStopButton->setText(tr("Play"));
        // enable the related fields
        this->ui->playDelayEdit->setReadOnly(false);
        this->ui->playComboBox->setEnabled(true);
        this->ui->dunPlayDelayEdit->setReadOnly(false);
        this->ui->dunPlayComboBox->setEnabled(true);
        return;
    }

    // disable the related fields
    this->ui->playDelayEdit->setReadOnly(true);
    this->ui->playComboBox->setEnabled(false);
    this->ui->dunPlayDelayEdit->setReadOnly(true);
    this->ui->dunPlayComboBox->setEnabled(false);
    // change the label of the button
    this->ui->playStopButton->setText(tr("Stop"));
    this->ui->dunPlayStopButton->setText(tr("Stop"));
    // preserve the palette
    dMainWindow().initPaletteCycle();

    this->playTimer = this->startTimer((this->dunView ? this->dunviewPlayDelay : this->tilesetPlayDelay) / 1000, Qt::PreciseTimer);
}

void LevelCelView::timerEvent(QTimerEvent *event)
{
    this->playCounter++;
    QComboBox *cycleBox = this->dunView ? this->ui->dunPlayComboBox : this->ui->playComboBox;
    int cycleType = cycleBox->currentIndex();
    if (cycleType == 0) {
        // normal playback
        this->displayFrame();
    } else {
        dMainWindow().nextPaletteCycle((D1PAL_CYCLE_TYPE)(cycleType - 1));
        // this->displayFrame();
    }
}

void LevelCelView::dragEnterEvent(QDragEnterEvent *event)
{
    this->dragMoveEvent(event);
}

void LevelCelView::dragMoveEvent(QDragMoveEvent *event)
{
    if (MainWindow::hasImageUrl(event->mimeData())) {
        event->acceptProposedAction();
    }
}

void LevelCelView::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    QStringList filePaths;
    for (const QUrl &url : event->mimeData()->urls()) {
        filePaths.append(url.toLocalFile());
    }
    // try to insert as frames
    dMainWindow().openImageFiles(IMAGE_FILE_MODE::AUTO, filePaths, false);
}

void LevelCelView::on_tilModified()
{
    if (this->dun != nullptr) {
        this->dun->refreshSubtiles();
    }
    this->displayFrame();
}

void LevelCelView::on_actionToggle_View_triggered()
{
    // stop playback
    if (this->playTimer != 0) {
        this->on_playStopButton_clicked();
    }

    bool dunMode = !this->dunView;
    this->dunView = dunMode;
    // preserve scroll value
    int horizScrollValue = this->lastHorizScrollValue;
    int vertScrollValue = this->lastVertScrollValue;
    this->lastVertScrollValue = this->ui->celGraphicsView->verticalScrollBar()->value();
    this->lastHorizScrollValue = this->ui->celGraphicsView->horizontalScrollBar()->value();
    // select gridlayout
    if (dunMode) {
        bool hidden = this->ui->tilesetWidget->isHidden();
        this->ui->tilesetWidget->setVisible(false);
        this->ui->dungeonWidget->setVisible(!hidden);
    } else {
        bool hidden = this->ui->dungeonWidget->isHidden();
        this->ui->dungeonWidget->setVisible(false);
        this->ui->tilesetWidget->setVisible(!hidden);
    }
    // update zoom
    QLineEdit *zoomField;
    if (dunMode) {
        zoomField = this->ui->dunZoomEdit;
    } else {
        zoomField = this->ui->zoomEdit;
    }
    QString zoomText = zoomField->text();
    this->celScene.setZoom(zoomText);
    // update the view
    this->displayFrame();
    // restore scroll value
    this->ui->celGraphicsView->verticalScrollBar()->setValue(vertScrollValue);
    this->ui->celGraphicsView->horizontalScrollBar()->setValue(horizScrollValue);
}

void LevelCelView::on_actionInsert_DunTileRow_triggered()
{
    int row = this->currentDunPosY;

    this->dun->insertTileRow(row);

    // update the view
    this->displayFrame();
}

void LevelCelView::on_actionInsert_DunTileColumn_triggered()
{
    int column = this->currentDunPosX;

    this->dun->insertTileColumn(column);

    // update the view
    this->displayFrame();
}

void LevelCelView::on_actionDel_DunTileRow_triggered()
{
    int row = this->currentDunPosY;

    this->dun->removeTileRow(row);

    if (row >= this->dun->getHeight()) {
        this->currentDunPosY = this->dun->getHeight() - 1;
    }

    // update the view
    this->displayFrame();
}

void LevelCelView::on_actionDel_DunTileColumn_triggered()
{
    int column = this->currentDunPosX;

    this->dun->removeTileColumn(column);
    if (column >= this->dun->getWidth()) {
        this->currentDunPosX = this->dun->getWidth() - 1;
    }

    // update the view
    this->displayFrame();
}

void LevelCelView::setPosition(int posx, int posy)
{
    if (posx < 0) {
        posx = 0;
    }
    if (posx >= this->dun->getWidth()) {
        posx = this->dun->getWidth() - 1;
    }
    if (posy < 0) {
        posy = 0;
    }
    if (posy >= this->dun->getHeight()) {
        posy = this->dun->getHeight() - 1;
    }
    bool change = this->currentDunPosX != posx || this->currentDunPosY != posy;
    this->currentDunPosX = posx;
    this->currentDunPosY = posy;
    this->on_dungeonPosLineEdit_escPressed();
    if (change) {
        // update the view
        this->updateFields();
    }
}

void LevelCelView::shiftPosition(int dx, int dy)
{
    int posx0 = this->currentDunPosX, posy0 = this->currentDunPosY;
    int posx1 = this->currentDunPosX + dx, posy1 = this->currentDunPosY + dy;
    const bool movePos = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;

    const int mode = dMainWindow().getDunBuilderMode();
    if (mode == BEM_TILE || mode == BEM_TILE_PROTECTION || (mode == -1 && movePos)) {
        posx1 += dx;
        posy1 += dy;
    }

    this->setPosition(posx1, posy1);
    if (movePos && this->dun->swapPositions(mode, posx0, posy0, posx1, posy1)) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_moveLeftButton_clicked()
{
    this->shiftPosition(-1, 0);
}

void LevelCelView::on_moveRightButton_clicked()
{
    this->shiftPosition(1, 0);
}

void LevelCelView::on_moveUpButton_clicked()
{
    this->shiftPosition(0, -1);
}

void LevelCelView::on_moveDownButton_clicked()
{
    this->shiftPosition(0, 1);
}

void LevelCelView::on_dungeonPosLineEdit_returnPressed()
{
    QString posText = this->ui->dungeonPosLineEdit->text();
    int sepIdx = posText.indexOf(":");

    int posx, posy;
    if (sepIdx >= 0) {
        if (sepIdx == 0) {
            posx = 0;
            posy = posText.mid(1).toUShort();
        } else if (sepIdx == posText.length() - 1) {
            posText.chop(1);
            posx = posText.toUShort();
            posy = 0;
        } else {
            posx = posText.mid(0, sepIdx).toUShort();
            posy = posText.mid(sepIdx + 1).toUShort();
        }
    } else {
        posx = posText.toUShort();
        posy = 0;
    }
    this->setPosition(posx, posy);
}

void LevelCelView::on_dungeonPosLineEdit_escPressed()
{
    this->ui->dungeonPosLineEdit->setText(QString::number(this->currentDunPosX) + ":" + QString::number(this->currentDunPosY));
    this->ui->dungeonPosLineEdit->clearFocus();
}

void LevelCelView::on_dunWidthEdit_returnPressed()
{
    int newWidth = this->ui->dunWidthEdit->text().toUShort();

    bool change = this->dun->setWidth(newWidth, false);
    this->on_dunWidthEdit_escPressed();
    if (change) {
        if (this->currentDunPosX >= newWidth) {
            this->currentDunPosX = newWidth - 1;
            this->on_dungeonPosLineEdit_escPressed();
        }
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dunWidthEdit_escPressed()
{
    int width = this->dun->getWidth();

    this->ui->dunWidthEdit->setText(QString::number(width));
    this->ui->dunWidthEdit->clearFocus();
}

void LevelCelView::on_dunHeightEdit_returnPressed()
{
    int newHeight = this->ui->dunHeightEdit->text().toUShort();

    bool change = this->dun->setHeight(newHeight, false);
    this->on_dunHeightEdit_escPressed();
    if (change) {
        if (this->currentDunPosY >= newHeight) {
            this->currentDunPosY = newHeight - 1;
            this->on_dungeonPosLineEdit_escPressed();
        }
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dunHeightEdit_escPressed()
{
    int height = this->dun->getHeight();

    this->ui->dunHeightEdit->setText(QString::number(height));
    this->ui->dunHeightEdit->clearFocus();
}

void LevelCelView::on_levelTypeComboBox_activated(int index)
{
    if (index < 0) {
        return;
    }
    bool change = this->dun->setLevelType(index);
    this->on_dungeonDefaultTileLineEdit_escPressed();
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonDefaultTileLineEdit_returnPressed()
{
    QString tileText = this->ui->dungeonDefaultTileLineEdit->text();
    int defaultTile = tileText.isEmpty() ? UNDEF_TILE : tileText.toInt();

    bool change = this->dun->setDefaultTile(defaultTile);
    this->on_dungeonDefaultTileLineEdit_escPressed();
    if (change) {
        this->ui->levelTypeComboBox->setCurrentIndex(DTYPE_NONE);
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonDefaultTileLineEdit_escPressed()
{
    QString defaultText;
    int defaultTile = this->dun->getDefaultTile();
    if (defaultTile != UNDEF_TILE) {
        defaultText = QString::number(defaultTile);
    }
    this->ui->dungeonDefaultTileLineEdit->setText(defaultText);
    this->ui->dungeonDefaultTileLineEdit->clearFocus();
}

void LevelCelView::on_showButtonGroup_idClicked()
{
    // update the view
    this->displayFrame();
}

void LevelCelView::on_showItemsCheckBox_clicked()
{
    // update the view
    this->displayFrame();
}

void LevelCelView::on_showMonstersCheckBox_clicked()
{
    // update the view
    this->displayFrame();
}

void LevelCelView::on_showObjectsCheckBox_clicked()
{
    // update the view
    this->displayFrame();
}

void LevelCelView::on_showMissilesCheckBox_clicked()
{
    // update the view
    this->displayFrame();
}

void LevelCelView::on_dunOverlayComboBox_activated(int index)
{
    // update the view
    this->displayFrame();
}

void LevelCelView::on_dungeonTileLineEdit_returnPressed()
{
    bool ok;
    int tileRef = this->ui->dungeonTileLineEdit->text().toInt(&ok);
    if (tileRef < 0 || !ok) {
        tileRef = UNDEF_TILE;
    }

    bool change = this->dun->setTileAt(this->currentDunPosX, this->currentDunPosY, tileRef);
    this->on_dungeonTileLineEdit_escPressed();
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonTileLineEdit_escPressed()
{
    int tileRef = this->dun->getTileAt(this->currentDunPosX, this->currentDunPosY);

    this->ui->dungeonTileLineEdit->setText(tileRef == UNDEF_TILE ? QStringLiteral("?") : QString::number(tileRef));
    this->ui->dungeonTileLineEdit->clearFocus();
}

void LevelCelView::on_dungeonTileProtectionCheckBox_clicked()
{
    Qt::CheckState state = this->ui->dungeonTileProtectionCheckBox->checkState();

    bool change = this->dun->setTileProtectionAt(this->currentDunPosX, this->currentDunPosY, state);
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::selectTilesetPath(QString path)
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 1, PAF_UPDATE_WINDOW);

    if (this->dun->reloadTileset(path)) {
        this->updateTilesetIcon();
        // update the view
        // this->displayFrame();
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void LevelCelView::on_tilesetLoadPushButton_clicked()
{
    QString celFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select tileset"), tr("CEL Files (*.cel *.CEL)"));

    if (!celFilePath.isEmpty()) {
        this->selectTilesetPath(celFilePath);
    }
}

void LevelCelView::on_tilesetClearPushButton_clicked()
{
    this->selectTilesetPath("");
}

void LevelCelView::selectAssetPath(QString path)
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Loading..."), 1, PAF_UPDATE_WINDOW);

    if (this->dun->setAssetPath(path)) {
        this->updateEntityOptions();
        // update the view
        // this->displayFrame();
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void LevelCelView::on_assetLoadPushButton_clicked()
{
    QString dirPath = dMainWindow().folderDialog(tr("Select Assets Folder"));

    if (!dirPath.isEmpty()) {
        this->selectAssetPath(dirPath);
    }
}

void LevelCelView::on_assetClearPushButton_clicked()
{
    this->selectAssetPath("");
}

void LevelCelView::on_dungeonSubtileLineEdit_returnPressed()
{
    bool ok;
    int subtileRef = this->ui->dungeonSubtileLineEdit->text().toInt(&ok);
    if (subtileRef < 0 || !ok) {
        subtileRef = UNDEF_SUBTILE;
    }

    bool change = this->dun->setSubtileAt(this->currentDunPosX, this->currentDunPosY, subtileRef);
    this->on_dungeonSubtileLineEdit_escPressed();
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonSubtileLineEdit_escPressed()
{
    int subtileRef = this->dun->getSubtileAt(this->currentDunPosX, this->currentDunPosY);

    this->ui->dungeonSubtileLineEdit->setText(subtileRef == UNDEF_SUBTILE ? QStringLiteral("?") : QString::number(subtileRef));
    this->ui->dungeonSubtileLineEdit->clearFocus();
}

void LevelCelView::on_dungeonSubtileMonProtectionCheckBox_clicked()
{
    bool checked = this->ui->dungeonSubtileMonProtectionCheckBox->isChecked();

    bool change = this->dun->setSubtileMonProtectionAt(this->currentDunPosX, this->currentDunPosY, checked);
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonSubtileObjProtectionCheckBox_clicked()
{
    bool checked = this->ui->dungeonSubtileObjProtectionCheckBox->isChecked();

    bool change = this->dun->setSubtileObjProtectionAt(this->currentDunPosX, this->currentDunPosY, checked);
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::setObjectIndex(int objectIndex)
{
    MapObject obj = this->dun->getObjectAt(this->currentDunPosX, this->currentDunPosY);
    obj.oType = objectIndex;

    bool change = this->dun->setObjectAt(this->currentDunPosX, this->currentDunPosY, obj);

    // update the view
    if (change) {
        this->displayFrame();
    } else {
        this->updateFields();
    }
}

void LevelCelView::on_dungeonObjectLineEdit_returnPressed()
{
    int objectIndex = this->ui->dungeonObjectLineEdit->text().toUShort();

    // this->on_dungeonObjectLineEdit_escPressed();
    this->ui->dungeonObjectLineEdit->clearFocus();

    this->setObjectIndex(objectIndex);
}

void LevelCelView::on_dungeonObjectLineEdit_escPressed()
{
    MapObject obj = this->dun->getObjectAt(this->currentDunPosX, this->currentDunPosY);

    this->ui->dungeonObjectLineEdit->setText(QString::number(obj.oType));
    this->ui->dungeonObjectLineEdit->clearFocus();
}

void LevelCelView::on_dungeonObjectComboBox_activated(int index)
{
    if (index < 0) {
        return;
    }
    int objectIndex = this->ui->dungeonObjectComboBox->itemData(index).value<int>();
    this->setObjectIndex(objectIndex);
}

void LevelCelView::on_dungeonObjectPreCheckBox_clicked()
{
    MapObject obj = this->dun->getObjectAt(this->currentDunPosX, this->currentDunPosY);
    obj.oPreFlag = this->ui->dungeonObjectPreCheckBox->isChecked();

    bool change = this->dun->setObjectAt(this->currentDunPosX, this->currentDunPosY, obj);

    // update the view
    if (change) {
        this->displayFrame();
    } else {
        this->updateFields();
    }
}

void LevelCelView::on_dungeonObjectAddButton_clicked()
{
    int objectIndex = this->ui->dungeonObjectLineEdit->text().toUShort();
    this->dungeonResourceDialog.initialize(DUN_ENTITY_TYPE::OBJECT, objectIndex, false, this->dun);
    this->dungeonResourceDialog.show();
}

void LevelCelView::setMonsterType(int monsterIndex, bool monsterUnique, bool monsterDead)
{
    MapMonster mon = this->dun->getMonsterAt(this->currentDunPosX, this->currentDunPosY);
    mon.moType = { monsterIndex, monsterUnique, monsterDead };
    this->setCurrentMonster(mon);
}

void LevelCelView::on_dungeonMonsterLineEdit_returnPressed()
{
    /*int monsterIndex = this->ui->dungeonMonsterLineEdit->text().toUShort();
    bool monsterUnique = this->ui->dungeonMonsterCheckBox->isChecked();
    bool monsterDead = this->ui->dungeonMonsterDeadCheckBox->isChecked();

    this->setMonsterType(monsterIndex, monsterUnique, monsterDead);*/
    this->on_dungeonMonsterCheckBox_clicked();

    this->on_dungeonMonsterLineEdit_escPressed();
}

void LevelCelView::on_dungeonMonsterLineEdit_escPressed()
{
    MapMonster mon = this->dun->getMonsterAt(this->currentDunPosX, this->currentDunPosY);

    this->ui->dungeonMonsterLineEdit->setText(QString::number(mon.moType.monIndex));
    this->ui->dungeonMonsterLineEdit->clearFocus();
}

void LevelCelView::on_dungeonMonsterCheckBox_clicked()
{
    int monsterIndex = this->ui->dungeonMonsterLineEdit->text().toUShort();
    bool monsterUnique = this->ui->dungeonMonsterCheckBox->isChecked();
    bool monsterDead = this->ui->dungeonMonsterDeadCheckBox->isChecked();

    this->setMonsterType(monsterIndex, monsterUnique, monsterDead);
}

void LevelCelView::on_dungeonMonsterComboBox_activated(int index)
{
    if (index < 0) {
        return;
    }
    DunMonsterType monType = this->ui->dungeonMonsterComboBox->itemData(index).value<DunMonsterType>();

    this->setMonsterType(monType.monIndex, monType.monUnique, monType.monDeadFlag);
}

void LevelCelView::on_dungeonMonsterDeadCheckBox_clicked()
{
    /*bool monsterDead = this->ui->dungeonMonsterDeadCheckBox->isChecked();
    MapMonster mon = this->dun->getMonsterAt(this->currentDunPosX, this->currentDunPosY);

    mon.moType.monDeadFlag = monsterDead;
    this->setCurrentMonster(mon);*/
    this->on_dungeonMonsterCheckBox_clicked();
}

void LevelCelView::on_dungeonMonsterAddButton_clicked()
{
    int monsterIndex = this->ui->dungeonMonsterLineEdit->text().toUShort();
    bool monsterUnique = this->ui->dungeonMonsterCheckBox->isChecked();
    this->dungeonResourceDialog.initialize(DUN_ENTITY_TYPE::MONSTER, monsterIndex, monsterUnique, this->dun);
    this->dungeonResourceDialog.show();
}

void LevelCelView::setCurrentMonster(const MapMonster &mon)
{
    bool change = this->dun->setMonsterAt(this->currentDunPosX, this->currentDunPosY, mon);
    // update the view
    if (change) {
        this->displayFrame();
    } else {
        this->updateFields();
    }
}

static int getSpinValue(QSpinBox *spinBox, int value, int curVal)
{
    const int minVal = spinBox->minimum();
    const int maxVal = spinBox->maximum();
    if (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier) {
        if (value < curVal) {
            if (value < 0) {
                value = minVal;
            } else {
                value = 0;
            }
        } else {
            if (value <= 0) {
                value = 0;
            } else {
                value = maxVal;
            }
        }
    }
    if (value < minVal) {
        value = minVal;
    }
    if (value > maxVal) {
        value = maxVal;
    }
    return value;
}

void LevelCelView::on_dungeonMonsterXOffSpinBox_valueChanged(int value)
{
    MapMonster mon = this->dun->getMonsterAt(this->currentDunPosX, this->currentDunPosY);
    mon.mox = getSpinValue(this->ui->dungeonMonsterXOffSpinBox, value, mon.mox);
    this->setCurrentMonster(mon);
}

void LevelCelView::on_dungeonMonsterYOffSpinBox_valueChanged(int value)
{
    MapMonster mon = this->dun->getMonsterAt(this->currentDunPosX, this->currentDunPosY);
    mon.moy = getSpinValue(this->ui->dungeonMonsterYOffSpinBox, value, mon.moy);
    this->setCurrentMonster(mon);
}

void LevelCelView::setCurrentMissile(const MapMissile &mis)
{
    bool change = this->dun->setMissileAt(this->currentDunPosX, this->currentDunPosY, mis);
    // update the view
    if (change) {
        this->displayFrame();
    } else {
        this->updateFields();
    }
}

void LevelCelView::on_dungeonMissileLineEdit_returnPressed()
{
    int misIndex = this->ui->dungeonMissileLineEdit->text().toUShort();
    // this->on_dungeonMissileLineEdit_escPressed();
    this->ui->dungeonMissileLineEdit->clearFocus();

    MapMissile mis = this->dun->getMissileAt(this->currentDunPosX, this->currentDunPosY);
    mis.miType = misIndex;

    this->setCurrentMissile(mis);
}

void LevelCelView::on_dungeonMissileLineEdit_escPressed()
{
    MapMissile mis = this->dun->getMissileAt(this->currentDunPosX, this->currentDunPosY);

    this->ui->dungeonMissileLineEdit->setText(QString::number(mis.miType));
    this->ui->dungeonMissileLineEdit->clearFocus();
}

void LevelCelView::on_dungeonMissileComboBox_activated(int index)
{
    if (index < 0) {
        return;
    }
    int misIndex = this->ui->dungeonMissileComboBox->itemData(index).value<int>();
    MapMissile mis = this->dun->getMissileAt(this->currentDunPosX, this->currentDunPosY);
    mis.miType = misIndex;
    this->setCurrentMissile(mis);
}

void LevelCelView::on_dungeonMissileXOffSpinBox_valueChanged(int value)
{
    MapMissile mis = this->dun->getMissileAt(this->currentDunPosX, this->currentDunPosY);
    mis.mix = getSpinValue(this->ui->dungeonMissileXOffSpinBox, value, mis.mix);
    this->setCurrentMissile(mis);
}

void LevelCelView::on_dungeonMissileYOffSpinBox_valueChanged(int value)
{
    MapMissile mis = this->dun->getMissileAt(this->currentDunPosX, this->currentDunPosY);
    mis.miy = getSpinValue(this->ui->dungeonMissileYOffSpinBox, value, mis.miy);
    this->setCurrentMissile(mis);
}

void LevelCelView::on_dungeonMissilePreCheckBox_clicked()
{
    MapMissile mis = this->dun->getMissileAt(this->currentDunPosX, this->currentDunPosY);
    mis.miPreFlag = this->ui->dungeonMissilePreCheckBox->isChecked();
    this->setCurrentMissile(mis);
}

void LevelCelView::on_dungeonMissileAddButton_clicked()
{
    int misIndex = this->ui->dungeonMissileLineEdit->text().toUShort();
    this->dungeonResourceDialog.initialize(DUN_ENTITY_TYPE::MISSILE, misIndex, false, this->dun);
    this->dungeonResourceDialog.show();
}

void LevelCelView::on_dungeonItemLineEdit_returnPressed()
{
    int itemIndex = this->ui->dungeonItemLineEdit->text().toUShort();
    int posx = this->currentDunPosX;
    int posy = this->currentDunPosY;

    bool change = this->dun->setItemAt(posx, posy, itemIndex);
    this->on_dungeonItemLineEdit_escPressed();
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonItemLineEdit_escPressed()
{
    int itemIndex = this->dun->getItemAt(this->currentDunPosX, this->currentDunPosY);

    this->ui->dungeonItemLineEdit->setText(QString::number(itemIndex));
    this->ui->dungeonItemLineEdit->clearFocus();
}

void LevelCelView::on_dungeonItemComboBox_activated(int index)
{
    if (index < 0) {
        return;
    }
    int itemIndex = this->ui->dungeonItemComboBox->itemData(index).value<int>();

    bool change = this->dun->setItemAt(this->currentDunPosX, this->currentDunPosY, itemIndex);
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonItemAddButton_clicked()
{
    int itemIndex = this->ui->dungeonItemLineEdit->text().toUShort();
    this->dungeonResourceDialog.initialize(DUN_ENTITY_TYPE::ITEM, itemIndex, false, this->dun);
    this->dungeonResourceDialog.show();
}

void LevelCelView::on_dungeonRoomLineEdit_returnPressed()
{
    int roomIndex = this->ui->dungeonRoomLineEdit->text().toUShort();

    bool change = this->dun->setRoomAt(this->currentDunPosX, this->currentDunPosY, roomIndex);
    this->on_dungeonRoomLineEdit_escPressed();
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonRoomLineEdit_escPressed()
{
    int roomIndex = this->dun->getRoomAt(this->currentDunPosX, this->currentDunPosY);

    this->ui->dungeonRoomLineEdit->setText(QString::number(roomIndex));
    this->ui->dungeonRoomLineEdit->clearFocus();
}

void LevelCelView::on_dunZoomOutButton_clicked()
{
    this->celScene.zoomOut();
    this->on_dunZoomEdit_escPressed();
}

void LevelCelView::on_dunZoomInButton_clicked()
{
    this->celScene.zoomIn();
    this->on_dunZoomEdit_escPressed();
}

void LevelCelView::on_dunZoomEdit_returnPressed()
{
    QString zoom = this->ui->dunZoomEdit->text();

    this->celScene.setZoom(zoom);

    this->on_dunZoomEdit_escPressed();
}

void LevelCelView::on_dunZoomEdit_escPressed()
{
    this->ui->dunZoomEdit->setText(this->celScene.zoomText());
    this->ui->dunZoomEdit->clearFocus();
}

void LevelCelView::on_dunPlayDelayEdit_returnPressed()
{
    quint16 playDelay = this->ui->dunPlayDelayEdit->text().toUInt();

    if (playDelay != 0)
        this->dunviewPlayDelay = playDelay;

    this->on_dunPlayDelayEdit_escPressed();
}

void LevelCelView::on_dunPlayDelayEdit_escPressed()
{
    this->ui->dunPlayDelayEdit->setText(QString::number(this->dunviewPlayDelay));
    this->ui->dunPlayDelayEdit->clearFocus();
}

void LevelCelView::on_dunPlayStopButton_clicked()
{
    this->on_playStopButton_clicked();
}
