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
#include <QTimer>

#include "config.h"
#include "d1image.h"
#include "d1pcx.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "pushbuttonwidget.h"
#include "ui_levelcelview.h"
#include "upscaler.h"

#include "dungeon/all.h"

Q_DECLARE_METATYPE(DunMonsterType);

LevelCelView::LevelCelView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LevelCelView())
{
    this->ui->setupUi(this);
    this->ui->celGraphicsView->setScene(&this->celScene);
    this->on_zoomEdit_escPressed();
    this->on_playDelayEdit_escPressed();
    this->ui->stopButton->setEnabled(false);
    this->on_dunZoomEdit_escPressed();
    this->on_dunPlayDelayEdit_escPressed();
    this->ui->dunStopButton->setEnabled(false);
    this->ui->tilesTabs->addTab(&this->tabTileWidget, tr("Tile properties"));
    this->ui->tilesTabs->addTab(&this->tabSubtileWidget, tr("Subtile properties"));
    this->ui->tilesTabs->addTab(&this->tabFrameWidget, tr("Frame properties"));
    QLayout *layout = this->ui->tilesetButtonsHorizontalLayout;
    PushButtonWidget *btn = PushButtonWidget::addButton(this, layout, QStyle::SP_DialogResetButton, tr("Start drawing"), &dMainWindow(), &MainWindow::on_actionToggle_Painter_triggered);
    layout->setAlignment(btn, Qt::AlignRight);
    this->viewBtn = PushButtonWidget::addButton(this, layout, QStyle::SP_ArrowRight, tr("Switch to dungeon view"), &dMainWindow(), &MainWindow::on_actionToggle_View_triggered);
    layout = this->ui->dunButtonsHorizontalLayout;
    btn = PushButtonWidget::addButton(this, layout, QStyle::SP_DialogResetButton, tr("Start building"), &dMainWindow(), &MainWindow::on_actionToggle_Builder_triggered);
    layout->setAlignment(btn, Qt::AlignRight);
    btn = PushButtonWidget::addButton(this, layout, QStyle::SP_ArrowLeft, tr("Switch to tileset view"), &dMainWindow(), &MainWindow::on_actionToggle_View_triggered);

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
    QObject::connect(this->ui->dungeonPosXLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_dungeonPosXLineEdit_escPressed()));
    QObject::connect(this->ui->dungeonPosYLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_dungeonPosYLineEdit_escPressed()));
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
    this->pal = p;
    this->tileset = ts;
    this->gfx = ts->gfx;
    this->min = ts->min;
    this->til = ts->til;
    this->sol = ts->sol;
    this->amp = ts->amp;
    this->tmi = ts->tmi;
    this->dun = d;

    this->tabTileWidget.initialize(this, this->til, this->min, this->amp);
    this->tabSubtileWidget.initialize(this, this->gfx, this->min, this->sol, this->tmi);
    this->tabFrameWidget.initialize(this, this->gfx);

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
        PushButtonWidget::addButton(this, layout, 1, 9, QStyle::SP_FileDialogNewFolder, tr("Add Custom Monster"), this, &LevelCelView::on_dungeonMonsterAddButton_clicked);
        PushButtonWidget::addButton(this, layout, 1, 14, QStyle::SP_FileDialogNewFolder, tr("Add Custom Item"), this, &LevelCelView::on_dungeonItemAddButton_clicked);
        // initialize the fields which are not updated
        this->on_dungeonDefaultTileLineEdit_escPressed();
        this->ui->levelTypeComboBox->setCurrentIndex(d->getLevelType());
        this->updateTilesetIcon();
        // prepare the entities
        this->updateEntityOptions();
    }
    // this->update();
}

void LevelCelView::setPal(D1Pal *p)
{
    this->pal = p;
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
    DunMonsterType monType = { 0, false };
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
            DunMonsterType monType = { i, false };
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
            DunMonsterType monType = { i + 1, true };
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
    QFileInfo gfxFileInfo(this->gfx->getFilePath());
    QFileInfo minFileInfo(this->min->getFilePath());
    QFileInfo tilFileInfo(this->til->getFilePath());
    QFileInfo solFileInfo(this->sol->getFilePath());
    QFileInfo ampFileInfo(this->amp->getFilePath());
    QFileInfo tmiFileInfo(this->tmi->getFilePath());

    QString label = gfxFileInfo.fileName();
    if (this->gfx->isModified()) {
        label += "*";
    }
    label += ", ";
    label += minFileInfo.fileName();
    if (this->min->isModified()) {
        label += "*";
    }
    label += ", ";
    label += tilFileInfo.fileName();
    if (this->til->isModified()) {
        label += "*";
    }
    label += ", ";
    label += solFileInfo.fileName();
    if (this->sol->isModified()) {
        label += "*";
    }
    label += ", ";
    label += ampFileInfo.fileName();
    if (this->amp->isModified()) {
        label += "*";
    }
    label += ", ";
    label += tmiFileInfo.fileName();
    if (this->tmi->isModified()) {
        label += "*";
    }

    if (this->dun != nullptr) {
        const D1Gfx *specGfx = this->dun->getSpecGfx();
        if (specGfx != nullptr) {
            QFileInfo specFileInfo(specGfx->getFilePath());
            label += ", ";
            label += specFileInfo.fileName();
            if (specGfx->isModified()) {
                label += "*";
            }
        }
        QFileInfo dunFileInfo(this->dun->getFilePath());
        label += ", ";
        label += dunFileInfo.fileName();
        if (this->dun->isModified()) {
            label += "*";
        }
    }

    this->ui->celLabel->setText(label);
}

void LevelCelView::update()
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
        this->ui->dungeonPosXLineEdit->setText(QString::number(posx));
        this->ui->dungeonPosYLineEdit->setText(QString::number(posy));
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
        DunMonsterType monType = this->dun->getMonsterAt(posx, posy);
        this->ui->dungeonMonsterLineEdit->setText(QString::number(monType.first));
        this->ui->dungeonMonsterCheckBox->setChecked(monType.second);
        this->ui->dungeonMonsterComboBox->setCurrentIndex(this->ui->dungeonMonsterComboBox->findData(QVariant::fromValue(monType)));
        int objectIndex = this->dun->getObjectAt(posx, posy);
        this->ui->dungeonObjectLineEdit->setText(QString::number(objectIndex));
        this->ui->dungeonObjectComboBox->setCurrentIndex(this->ui->dungeonObjectComboBox->findData(objectIndex));
        this->ui->dungeonRoomLineEdit->setText(QString::number(this->dun->getRoomAt(posx, posy)));
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

        this->tabTileWidget.update();
        this->tabSubtileWidget.update();
        this->tabFrameWidget.update();
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

void LevelCelView::framePixelClicked(const QPoint &pos, bool first)
{
    unsigned celFrameWidth = MICRO_WIDTH; // this->gfx->getFrameWidth(this->currentFrameIndex);
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned tileWidth = subtileWidth * TILE_WIDTH;

    unsigned celFrameHeight = MICRO_HEIGHT; // this->gfx->getFrameHeight(this->currentFrameIndex);
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;
    unsigned subtileShiftY = subtileWidth / 4;
    unsigned tileHeight = subtileHeight + 2 * subtileShiftY;

    if (this->dunView) {
        QPoint cellPos = this->getCellPos(pos);
        dMainWindow().dunClicked(cellPos, first);
        return;
    }
    if (pos.x() >= (int)(CEL_SCENE_MARGIN + celFrameWidth + CEL_SCENE_SPACING)
        && pos.x() < (int)(CEL_SCENE_MARGIN + celFrameWidth + CEL_SCENE_SPACING + subtileWidth)
        && pos.y() >= CEL_SCENE_MARGIN
        && pos.y() < (int)(subtileHeight + CEL_SCENE_MARGIN)
        && this->min->getSubtileCount() != 0) {
        // When a CEL frame is clicked in the subtile, display the corresponding CEL frame

        // Adjust coordinates
        unsigned stx = pos.x() - (CEL_SCENE_MARGIN + celFrameWidth + CEL_SCENE_SPACING);
        unsigned sty = pos.y() - CEL_SCENE_MARGIN;

        // qDebug() << "Subtile clicked: " << stx << "," << sty;

        stx /= MICRO_WIDTH;
        sty /= MICRO_HEIGHT;

        unsigned stFrame = sty * subtileWidth / MICRO_WIDTH + stx;
        std::vector<unsigned> &minFrames = this->min->getFrameReferences(this->currentSubtileIndex);
        unsigned frameRef = 0;
        if (minFrames.size() > stFrame) {
            frameRef = minFrames.at(stFrame);
            this->tabSubtileWidget.selectFrame(stFrame);
        }

        if (frameRef > 0) {
            this->currentFrameIndex = frameRef - 1;
            this->displayFrame();
        }
        // highlight selection
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
        return;
    } else if (pos.x() >= (int)(CEL_SCENE_MARGIN + celFrameWidth + CEL_SCENE_SPACING + subtileWidth + CEL_SCENE_SPACING)
        && pos.x() < (int)(CEL_SCENE_MARGIN + celFrameWidth + CEL_SCENE_SPACING + subtileWidth + CEL_SCENE_SPACING + tileWidth)
        && pos.y() >= CEL_SCENE_MARGIN
        && pos.y() < (int)(CEL_SCENE_MARGIN + tileHeight)
        && this->til->getTileCount() != 0) {
        // When a subtile is clicked in the tile, display the corresponding subtile

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

        std::vector<int> &tilSubtiles = this->til->getSubtileIndices(this->currentTileIndex);
        if (tilSubtiles.size() > tSubtile) {
            this->currentSubtileIndex = tilSubtiles.at(tSubtile);
            this->tabTileWidget.selectSubtile(tSubtile);
            this->displayFrame();
        }
        return;
    }
    // otherwise emit frame-click event
    if (this->gfx->getFrameCount() == 0) {
        return;
    }
    D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
    QPoint p = pos;
    p -= QPoint(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    dMainWindow().frameClicked(frame, p, first);
}

void LevelCelView::framePixelHovered(const QPoint &pos)
{
    if (this->dunView) {
        QPoint cellPos = this->getCellPos(pos);
        dMainWindow().dunHovered(cellPos);
    }
}

void LevelCelView::scrollTo(int posx, int posy)
{
    this->currentDunPosX = posx;
    this->currentDunPosY = posy;
    this->isScrolling = true;
}

void LevelCelView::selectPos(const QPoint &cell)
{
    this->currentDunPosX = cell.x();
    this->currentDunPosY = cell.y();
    // update the view
    this->update();
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
        // reset subtile flags
        this->sol->setSubtileProperties(subtileIndex, 0);
        this->tmi->setSubtileProperties(subtileIndex, 0);
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

            bool clipped;
            D1GfxFrame *subFrame = this->gfx->insertFrame(frameIndex, &clipped);
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
        // reset subtile flags
        this->sol->setSubtileProperties(subtileIndex, 0);
        this->tmi->setSubtileProperties(subtileIndex, 0);
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
        bool clipped = false, palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, clipped, &basePal, this->gfx->getPalette(), &palMod);
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
        bool clipped = false, palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, clipped, &basePal, this->gfx->getPalette(), &palMod);
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
    this->min->insertSubtile(subtileIndex, frameReferencesList);
    this->sol->insertSubtile(subtileIndex, 0);
    this->tmi->insertSubtile(subtileIndex, 0);
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

            bool clipped;
            D1GfxFrame *subFrame = this->gfx->insertFrame(frameIndex, &clipped);
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
    this->min->insertSubtile(subtileIndex, frameReferencesList);
    this->sol->insertSubtile(subtileIndex, 0);
    this->tmi->insertSubtile(subtileIndex, 0);
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

    this->til->insertTile(tileIndex, subtileIndices);
    this->amp->insertTile(tileIndex);
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

    this->til->insertTile(tileIndex, subtileIndices);
    this->amp->insertTile(tileIndex);
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
        bool clipped = false, palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, clipped, &basePal, this->gfx->getPalette(), &palMod);
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
        bool clipped = false, palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, clipped, &basePal, this->gfx->getPalette(), &palMod);
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

void LevelCelView::replaceCurrentFrame(const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool clipped = false, palMod;
        D1GfxFrame *frame = new D1GfxFrame();
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(*frame, imagefilePath, clipped, &basePal, this->gfx->getPalette(), &palMod);
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
    this->min->removeFrame(frameIndex, 0);
    // update frame index if necessary
    if (frameIndex < this->currentFrameIndex || this->currentFrameIndex == this->gfx->getFrameCount()) {
        this->currentFrameIndex = std::max(0, this->currentFrameIndex - 1);
    }
}

void LevelCelView::removeCurrentFrame()
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

void LevelCelView::createSubtile()
{
    this->tileset->createSubtile();
    // jump to the new subtile
    this->currentSubtileIndex = this->min->getSubtileCount() - 1;
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::replaceCurrentSubtile(const QString &imagefilePath)
{
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool clipped = false, palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, clipped, &basePal, this->gfx->getPalette(), &palMod);
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
        // reset subtile flags
        this->sol->setSubtileProperties(subtileIndex, 0);
        this->tmi->setSubtileProperties(subtileIndex, 0);
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

    // reset subtile flags
    this->sol->setSubtileProperties(subtileIndex, 0);
    this->tmi->setSubtileProperties(subtileIndex, 0);

    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::removeSubtile(int subtileIndex)
{
    this->tileset->removeSubtile(subtileIndex, 0);

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

void LevelCelView::createTile()
{
    this->til->createTile();
    this->amp->createTile();
    // jump to the new tile
    this->currentTileIndex = this->til->getTileCount() - 1;
    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::replaceCurrentTile(const QString &imagefilePath)
{
    unsigned tileWidth = this->min->getSubtileWidth() * MICRO_WIDTH * TILE_WIDTH * TILE_HEIGHT;
    unsigned tileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool clipped = false, palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, clipped, &basePal, this->gfx->getPalette(), &palMod);
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
        this->amp->setTileProperties(tileIndex, 0);
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
    this->amp->setTileProperties(tileIndex, 0);

    // update the view - done by the caller
    // this->displayFrame();
}

void LevelCelView::removeCurrentTile()
{
    int tileIndex = this->currentTileIndex;

    this->til->removeTile(tileIndex);
    this->amp->removeTile(tileIndex);
    // update tile index if necessary
    if (/*tileIndex < this->currentTileIndex ||*/ this->currentTileIndex == this->til->getTileCount()) {
        this->currentTileIndex = std::max(0, this->currentTileIndex - 1);
    }
    // update the view - done by the caller
    // this->displayFrame();
}

QImage LevelCelView::copyCurrent() const
{
    /*if (this->ui->tilesTabs->currentIndex() == 1)
        return this->min->getSubtileImage(this->currentSubtileIndex);*/
    if (this->gfx->getFrameCount() == 0) {
        return QImage();
    }
    return this->gfx->getFrameImage(this->currentFrameIndex);
}

void LevelCelView::pasteCurrent(const QImage &image)
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

        // reset subtile flags
        this->sol->setSubtileProperties(subtileIndex, 0);
        this->tmi->setSubtileProperties(subtileIndex, 0);
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
    }

    if (!hasFrame && !hasSubtile) {
        dProgress() << tr("The tileset is empty.");
    }

    ProgressDialog::decBar();
}

static int countCycledPixels(const std::vector<std::vector<D1GfxPixel>> &pixelImage, int cycleColors)
{
    int result = 0;
    for (auto &pixelLine : pixelImage) {
        for (auto &pixel : pixelLine) {
            if (!pixel.isTransparent() && pixel.getPaletteIndex() != 0 && pixel.getPaletteIndex() < cycleColors) {
                result++;
            }
        }
    }
    return result;
}

void LevelCelView::activeSubtiles() const
{
    ProgressDialog::incBar(tr("Checking subtiles..."), 1);
    QComboBox *cycleBox = this->dunView ? this->ui->dunPlayComboBox : this->ui->playComboBox;
    QString cycleTypeTxt = cycleBox->currentText();
    int cycleType = cycleBox->currentIndex();
    if (cycleType != 0) {
        int cycleColors = D1Pal::getCycleColors((D1PAL_CYCLE_TYPE)(cycleType - 1));
        bool result = false;

        QPair<int, QString> progress;
        progress.first = -1;
        progress.second = tr("Active subtiles (using '%1' playback mode):").arg(cycleTypeTxt);

        dProgress() << progress;
        for (int i = 0; i < this->min->getSubtileCount(); i++) {
            const std::vector<std::vector<D1GfxPixel>> pixelImage = this->min->getSubtilePixelImage(i);
            int numPixels = countCycledPixels(pixelImage, cycleColors);
            if (numPixels != 0) {
                dProgress() << tr("Subtile %1 has %2 affected pixels.", "", numPixels).arg(i + 1).arg(numPixels);
                result = true;
            }
        }

        if (!result) {
            progress.second = tr("None of the subtiles are active in '%1' playback mode.").arg(cycleTypeTxt);
            dProgress() << progress;
        }
    } else {
        dProgress() << tr("Colors are not affected if the playback mode is '%1'.").arg(cycleTypeTxt);
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
        bool result = false;

        QPair<int, QString> progress;
        progress.first = -1;
        progress.second = tr("Active tiles (using '%1' playback mode):").arg(cycleTypeTxt);

        dProgress() << progress;
        for (int i = 0; i < this->til->getTileCount(); i++) {
            const std::vector<std::vector<D1GfxPixel>> pixelImage = this->til->getTilePixelImage(i);
            int numPixels = countCycledPixels(pixelImage, cycleColors);
            if (numPixels != 0) {
                dProgress() << tr("Tile %1 has %2 affected pixels.", "", numPixels).arg(i + 1).arg(numPixels);
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

static QString getFrameTypeName(D1CEL_FRAME_TYPE type)
{
    switch (type) {
    case D1CEL_FRAME_TYPE::Square:
        return QApplication::tr("Square");
    case D1CEL_FRAME_TYPE::TransparentSquare:
        return QApplication::tr("Transparent square");
    case D1CEL_FRAME_TYPE::LeftTriangle:
        return QApplication::tr("Left Triangle");
    case D1CEL_FRAME_TYPE::RightTriangle:
        return QApplication::tr("Right Triangle");
    case D1CEL_FRAME_TYPE::LeftTrapezoid:
        return QApplication::tr("Left Trapezoid");
    case D1CEL_FRAME_TYPE::RightTrapezoid:
        return QApplication::tr("Right Trapezoid");
    case D1CEL_FRAME_TYPE::Empty:
        return QApplication::tr("Empty");
    default:
        return QApplication::tr("Unknown");
    }
}

void LevelCelView::inefficientFrames() const
{
    ProgressDialog::incBar(tr("Scanning frames..."), 1);

    int limit = 10;
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
            dProgress() << tr("Frame %1 could be '%2' by changing %n pixel(s).", "", diff).arg(i + 1).arg(getFrameTypeName(effType));
            result = true;
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

    for (int i = 0; i < 16; i++) {
        QString colors;
        for (int j = 0; j < 16; j++) {
            QColor color = this->pal->getColor(i * 16 + j);
            int lmin = std::min(std::min(color.red(), color.green()), color.blue());
            int lmax = std::max(std::max(color.red(), color.green()), color.blue());
            int lightness = (lmax + lmin) / 2;
            colors += QString("%1, ").arg(lightness);
        }
        dProgress() << QString("Color line %1: %2.").arg(i).arg(colors);
    }

    bool result = false;
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        D1GfxFrame *frame = this->gfx->getFrame(i);
        D1CEL_FRAME_TYPE prevType = frame->getFrameType();
        D1CelTilesetFrame::selectFrameType(frame);
        D1CEL_FRAME_TYPE newType = frame->getFrameType();
        if (prevType != newType) {
            dProgress() << tr("Changed Frame %1 from '%2' to '%3'.").arg(i + 1).arg(getFrameTypeName(prevType)).arg(getFrameTypeName(newType));
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
        // this->tabFrameWidget.update();
        this->update();
    }

    ProgressDialog::decBar();
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

void LevelCelView::checkSubtileFlags() const
{
    ProgressDialog::incBar(tr("Checking SOL flags..."), 1);
    bool result = false;
    unsigned floorMicros = this->min->getSubtileWidth() * this->min->getSubtileWidth() / 2;

    // SOL:
    QPair<int, QString> progress;
    progress.first = -1;
    progress.second = tr("SOL inconsistencies:");
    dProgress() << progress;
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        const std::vector<unsigned> &frameRefs = this->min->getFrameReferences(i);
        quint8 solFlags = this->sol->getSubtileProperties(i);
        if (solFlags & (1 << 1)) {
            // block light
            if (!(solFlags & (1 << 0))) {
                dProgressWarn() << tr("Subtile %1 blocks the light, but still passable (not solid).").arg(i + 1);
                result = true;
            }
            if (!(solFlags & (1 << 2))) {
                dProgressWarn() << tr("Subtile %1 blocks the light, but it does not block missiles.").arg(i + 1);
                result = true;
            }
        }
        if (solFlags & (1 << 2)) {
            // block missile
            if (!(solFlags & (1 << 0))) {
                dProgressWarn() << tr("Subtile %1 blocks missiles, but still passable (not solid).").arg(i + 1);
                result = true;
            }
        }
        if (solFlags & (1 << 4)) {
            // left transparency
            if (!(solFlags & (1 << 3))) {
                dProgressWarn() << tr("Subtile %1 has left transparency enabled, but transparency is not enabled.").arg(i + 1);
                result = true;
            }
        }
        if (solFlags & (1 << 5)) {
            // right transparency
            if (!(solFlags & (1 << 3))) {
                dProgressWarn() << tr("Subtile %1 has right transparency enabled, but transparency is not enabled.").arg(i + 1);
                result = true;
            }
        }
        if (solFlags & (1 << 7)) {
            // trap
            if (!(solFlags & (1 << 0))) {
                dProgressWarn() << tr("Subtile %1 is for traps, but still passable (not solid).").arg(i + 1);
                result = true;
            }
            if (!(solFlags & (1 << 1))) {
                dProgressWarn() << tr("Subtile %1 is for traps, but it does not block light.").arg(i + 1);
                result = true;
            }
            if (!(solFlags & (1 << 2))) {
                dProgressWarn() << tr("Subtile %1 is for traps, but it does not block missiles.").arg(i + 1);
                result = true;
            }
        }
        // checks for non-upscaled tilesets
        if (!this->gfx->isUpscaled()) {
            if (solFlags & ((1 << 1) | (1 << 2))) {
                // block light or missile
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
                    dProgressWarn() << tr("Subtile %1 blocks the light or missiles, but it is completely transparent above the floor.").arg(i + 1);
                    result = true;
                }
            }
            if (solFlags & (1 << 7) && this->min->getSubtileHeight() > 1) {
                // trap
                // - one above the floor is square (left or right)
                unsigned frameRefLeft = frameRefs[frameRefs.size() - 2 * floorMicros];
                unsigned frameRefRight = frameRefs[frameRefs.size() - (floorMicros + 1)];
                bool trapLeft = frameRefLeft != 0 && this->gfx->getFrame(frameRefLeft - 1)->getFrameType() == D1CEL_FRAME_TYPE::Square;
                bool trapRight = frameRefRight != 0 && this->gfx->getFrame(frameRefRight - 1)->getFrameType() == D1CEL_FRAME_TYPE::Square;
                if (!trapLeft && !trapRight) {
                    dProgressWarn() << tr("Subtile %1 is for traps, but the frames above the floor is not square on either side.").arg(i + 1);
                    result = true;
                }
            }
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected in the SOL flags.");
        dProgress() << progress;
    }

    ProgressDialog::decBar();
    ProgressDialog::incBar(tr("Checking TMI flags..."), 1);

    // TMI:
    result = false;
    progress.first = -1;
    progress.second = tr("TMI inconsistencies:");
    dProgress() << progress;
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        const std::vector<unsigned> &frameRefs = this->min->getFrameReferences(i);
        quint8 tmiFlags = this->tmi->getSubtileProperties(i);
        if (tmiFlags & (1 << 0)) {
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
                dProgressWarn() << tr("Subtile %1 has wall transparency set, but it is completely transparent above the floor.").arg(i + 1);
                result = true;
            }
        }
        /*if (tmiFlags & (1 << 1)) {
            // left second pass
            if (tmiFlags & (1 << 2)) {
                dProgressWarn() << tr("Subtile %1 has both second pass and foliage enabled on the left side.").arg(i + 1);
                result = true;
            }
        }*/
        /*if (tmiFlags & (1 << 2)) {
            // left foliage
            if (tmiFlags & (1 << 3)) {
                dProgressWarn() << tr("Subtile %1 has both foliage and floor transparency enabled on the left side.").arg(i + 1);
                result = true;
            }
        }*/
        if (tmiFlags & (1 << 3)) {
            // left transparency
            // - wall transparency must be set
            if (!(tmiFlags & (1 << 0))) {
                dProgressWarn() << tr("Subtile %1 has floor transparency on the left side, but no wall transparency.").arg(i + 1);
                result = true;
            }
        }
        /*if (tmiFlags & (1 << 4)) {
            // right second pass
            if (tmiFlags & (1 << 5)) {
                dProgressWarn() << tr("Subtile %1 has both second pass and foliage enabled on the right side.").arg(i + 1);
                result = true;
            }
        }*/
        /*if (tmiFlags & (1 << 5)) {
            // right foliage
            if (tmiFlags & (1 << 6)) {
                dProgressWarn() << tr("Subtile %1 has both foliage and floor transparency enabled on the right side.").arg(i + 1);
                result = true;
            }
        }*/
        if (tmiFlags & (1 << 6)) {
            // right transparency
            // - wall transparency must be set
            if (!(tmiFlags & (1 << 0))) {
                dProgressWarn() << tr("Subtile %1 has floor transparency on the right side, but no wall transparency.").arg(i + 1);
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
            if (tmiFlags & (1 << 0)) {
                // transp.wall
                // - trapezoid floor on the left without transparency
                if ((/*(leftType == D1CEL_FRAME_TYPE::LeftTrapezoid || */ (leftAbove && leftPixels > 30)) && !(tmiFlags & (1 << 3))) {
                    dProgressWarn() << tr("Subtile %1 has transparency on the wall while the frames above the left floor are not empty, but the left floor with many (%2) foliage pixels does not have transparency.").arg(i + 1).arg(leftPixels);
                    result = true;
                }
                // - trapezoid floor on the right without transparency
                if ((/*(rightType == D1CEL_FRAME_TYPE::RightTrapezoid || */ (rightAbove && rightPixels > 30)) && !(tmiFlags & (1 << 6))) {
                    dProgressWarn() << tr("Subtile %1 has transparency on the wall while the frames above the right floor are not empty, but the right floor with many (%2) foliage pixels does not have transparency.").arg(i + 1).arg(rightPixels);
                    result = true;
                }
            }
            if ((tmiFlags & (1 << 1)) && (tmiFlags & (1 << 4))) {
                // left&right second pass
                // - at least one not transparent frame above the floor or left floor with foliage or right floor with foliage
                bool hasColor = leftPixels != 0 || rightPixels != 0 || leftAbove || rightAbove;
                if (!hasColor) {
                    dProgressWarn() << tr("Subtile %1 has second pass set on both sides, but it is completely transparent or just a left/right triangle on the floor.").arg(i + 1);
                    result = true;
                }
            }
            if ((tmiFlags & (1 << 1)) && !(tmiFlags & (1 << 4))) {
                // left second pass without right
                // - at least one not transparent frame above the floor or left floor is not triangle
                bool hasColor = leftPixels != 0 || leftAbove;
                if (!hasColor) {
                    dProgressWarn() << tr("Subtile %1 has second pass set only on the left, but it is completely transparent or just a left triangle on the left-side.").arg(i + 1);
                    result = true;
                }
            }
            if (tmiFlags & (1 << 2)) {
                // left foliage
                // - left floor has a foliage pixel
                if (leftPixels == 0) {
                    dProgressWarn() << tr("Subtile %1 has foliage set on the left, but no foliage pixel on the (left-)floor.").arg(i + 1);
                    result = true;
                }
            }
            if (tmiFlags & (1 << 3)) {
                // left floor transparency
                // - must have non-transparent bits above the (left-)floor
                if (!leftAbove) {
                    dProgressWarn() << tr("Subtile %1 has left floor transparency set, but the left side is completely transparent above the floor.").arg(i + 1);
                    result = true;
                }
                // - must have wall transparency set if there are non-transparent bits above the floor
                if (leftAbove && !(tmiFlags & (1 << 0))) {
                    dProgressWarn() << tr("Subtile %1 has left floor transparency set, but the parts above the floor are not going to be transparent (wall transparency not set).").arg(i + 1);
                    result = true;
                }
            }
            if (tmiFlags & (1 << 4) && !(tmiFlags & (1 << 1))) {
                // right second pass without left
                // - at least one not transparent frame above the floor or right floor is not triangle
                bool hasColor = rightPixels != 0 || rightAbove;
                if (!hasColor) {
                    dProgressWarn() << tr("Subtile %1 has second pass set only on the right, but it is completely transparent or just a right triangle on the right-side.").arg(i + 1);
                    result = true;
                }
            }
            if (tmiFlags & (1 << 5)) {
                // right foliage
                // - right floor has a foliage pixel
                if (rightPixels == 0) {
                    dProgressWarn() << tr("Subtile %1 has foliage set on the right, but no foliage pixel on the (right-)floor.").arg(i + 1);
                    result = true;
                }
            }
            if (tmiFlags & (1 << 6)) {
                // right floor transparency
                // - must have non-transparent bits above the (right-)floor
                if (!rightAbove) {
                    dProgressWarn() << tr("Subtile %1 has right floor transparency set, but the right side is completely transparent above the floor.").arg(i + 1);
                    result = true;
                }
                // - must have wall transparency set if there are non-transparent bits above the floor
                if (rightAbove && !(tmiFlags & (1 << 0))) {
                    dProgressWarn() << tr("Subtile %1 has right floor transparency set, but the parts above the floor are not going to be transparent (wall transparency not set).").arg(i + 1);
                    result = true;
                }
            }
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected in the TMI flags.");
        dProgress() << progress;
    }

    ProgressDialog::decBar();
}

void LevelCelView::checkTileFlags() const
{
    ProgressDialog::incBar(tr("Checking AMP flags..."), 1);
    bool result = false;

    // AMP:
    QPair<int, QString> progress;
    progress.first = -1;
    progress.second = tr("AMP inconsistencies:");
    dProgress() << progress;
    for (int i = 0; i < this->til->getTileCount(); i++) {
        quint8 ampType = this->amp->getTileType(i);
        quint8 ampFlags = this->amp->getTileProperties(i);
        if (ampFlags & (1 << 0)) {
            // western door
            if (ampFlags & (1 << 2)) {
                dProgressWarn() << tr("Tile %1 has both west-door and west-arch flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 4)) {
                dProgressWarn() << tr("Tile %1 has both west-door and west-grate flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 6)) {
                dProgressWarn() << tr("Tile %1 has both west-door and external flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 7)) {
                dProgressWarn() << tr("Tile %1 has both west-door and stairs flags set.").arg(i + 1);
                result = true;
            }
            if (ampType != 2 && ampType != 4 && ampType != 5 && ampType != 10) {
                dProgressWarn() << tr("Tile %1 has a west-door but neither a wall (north-west), a wall intersection (north), a wall ending (north-west) nor a wall (south-west).").arg(i + 1);
                result = true;
            }
        }
        if (ampFlags & (1 << 1)) {
            // eastern door
            if (ampFlags & (1 << 3)) {
                dProgressWarn() << tr("Tile %1 has both east-door and east-arch flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 5)) {
                dProgressWarn() << tr("Tile %1 has both east-door and east-grate flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 6)) {
                dProgressWarn() << tr("Tile %1 has both east-door and external flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 7)) {
                dProgressWarn() << tr("Tile %1 has both east-door and stairs flags set.").arg(i + 1);
                result = true;
            }
            if (ampType != 3 && ampType != 4 && ampType != 6 && ampType != 11) {
                dProgressWarn() << tr("Tile %1 has an east-door but neither a wall (north-east), a wall intersection (north), a wall ending (north-east) nor a wall (south-east).").arg(i + 1);
                result = true;
            }
        }
        if (ampFlags & (1 << 2)) {
            // western arch
            if (ampFlags & (1 << 4)) {
                dProgressWarn() << tr("Tile %1 has both west-arch and west-grate flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 6)) {
                dProgressWarn() << tr("Tile %1 has both west-arch and external flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 7)) {
                dProgressWarn() << tr("Tile %1 has both west-arch and stairs flags set.").arg(i + 1);
                result = true;
            }
            if (ampType != 2 && ampType != 4 && ampType != 5) {
                dProgressWarn() << tr("Tile %1 has a west-arch but neither a wall (north-west), a wall intersection (north) nor a wall ending (north-west).").arg(i + 1);
                result = true;
            }
        }
        if (ampFlags & (1 << 3)) {
            // eastern arch
            if (ampFlags & (1 << 5)) {
                dProgressWarn() << tr("Tile %1 has both east-arch and east-grate flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 6)) {
                dProgressWarn() << tr("Tile %1 has both east-arch and external flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 7)) {
                dProgressWarn() << tr("Tile %1 has both east-arch and stairs flags set.").arg(i + 1);
                result = true;
            }
            if (ampType != 3 && ampType != 4 && ampType != 6) {
                dProgressWarn() << tr("Tile %1 has an east-arch but neither a wall (north-east), a wall intersection (north) nor a wall ending (north-east).").arg(i + 1);
                result = true;
            }
        }
        if (ampFlags & (1 << 4)) {
            // western grate
            if (ampFlags & (1 << 6)) {
                dProgressWarn() << tr("Tile %1 has both west-grate and external flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 7)) {
                dProgressWarn() << tr("Tile %1 has both west-grate and stairs flags set.").arg(i + 1);
                result = true;
            }
            if (ampType != 2 && ampType != 4 && ampType != 5) {
                dProgressWarn() << tr("Tile %1 has a west-grate but neither a wall (north-west), a wall intersection (north) nor a wall ending (north-west).").arg(i + 1);
                result = true;
            }
        }
        if (ampFlags & (1 << 5)) {
            // eastern grate
            if (ampFlags & (1 << 6)) {
                dProgressWarn() << tr("Tile %1 has both east-grate and external flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 7)) {
                dProgressWarn() << tr("Tile %1 has both east-grate and stairs flags set.").arg(i + 1);
                result = true;
            }
            if (ampType != 3 && ampType != 4 && ampType != 6) {
                dProgressWarn() << tr("Tile %1 has an east-grate but neither a wall (north-east), a wall intersection (north) nor a wall ending (north-east).").arg(i + 1);
                result = true;
            }
        }
        if (ampFlags & (1 << 6)) {
            // external
            if (ampFlags & (1 << 7)) {
                dProgressWarn() << tr("Tile %1 has both external and stairs flags set.").arg(i + 1);
                result = true;
            }
            if (ampType == 1) {
                dProgressWarn() << tr("Tile %1 is external but also a pillar.").arg(i + 1);
                result = true;
            }
        }
        if (ampFlags & (1 << 7)) {
            // stairs
            if (ampType != 0) {
                dProgressWarn() << tr("Tile %1 is stairs but its type is also set (not None).").arg(i + 1);
                result = true;
            }
        }
    }
    if (!result) {
        progress.second = tr("No inconsistency detected in the AMP flags.");
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
            dProgress() << tr("Removed frame %1.").arg(i);
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
            dProgress() << tr("Removed subtile %1.").arg(i);
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
    std::set<int> removedIndices;

    bool result = this->tileset->reuseSubtiles(removedIndices);

    // update subtile index if necessary
    auto it = removedIndices.lower_bound(this->currentSubtileIndex);
    if (it != removedIndices.begin()) {
        if (*it == this->currentSubtileIndex)
            it--;
        this->currentSubtileIndex -= std::distance(removedIndices.begin(), it);
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
    ProgressDialog::incBar(tr("Compressing tileset..."), 2);

    bool reusedFrame = this->reuseFrames();

    if (reusedFrame) {
        dProgress() << "\n";
    }

    ProgressDialog::incValue();

    bool reusedSubtile = this->reuseSubtiles();

    if (reusedFrame || reusedSubtile) {
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
    this->min->remapSubtiles(backmap);
    this->sol->remapSubtiles(backmap);
    this->tmi->remapSubtiles(backmap);
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
    ProgressDialog::incBar(tr("Scanning..."), 4);

    std::pair<int, int> space = this->dun->collectSpace(this->sol);

    if (space.first != 0) {
        dProgress() << tr("There are %1 subtiles in the dungeon for monsters.", "", space.first).arg(space.first);
        dProgress() << "\n";
    }
    if (space.second != 0) {
        dProgress() << tr("There are %1 subtiles in the dungeon for objects.", "", space.first).arg(space.first);
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
    this->dun->checkItems(this->sol);
}

void LevelCelView::checkMonsters() const
{
    this->dun->checkMonsters(this->sol);
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

static bool dimensionMatch(const D1Dun *dun1, const D1Dun *dun2)
{
    if (dun1->getWidth() == dun2->getWidth() && dun1->getHeight() == dun2->getHeight()) {
        return true;
    }
    QMessageBox::critical(nullptr, QApplication::tr("Error"), QApplication::tr("Mismatching dungeons (Dimensions are %1:%2 vs %3:%4).").arg(dun1->getWidth()).arg(dun1->getHeight()).arg(dun2->getHeight()).arg(dun2->getWidth()));
    return false;
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
    this->dungeonGenerateDialog.initialize(this->dun);
    this->dungeonGenerateDialog.show();
}

void LevelCelView::upscale(const UpscaleParam &params)
{
    if (Upscaler::upscaleTileset(this->gfx, this->min, params, false)) {
        // std::set<int> removedIndices;
        // this->tileset->reuseFrames(removedIndices, true);
        // update the view - done by the caller
        // this->displayFrame();
    }
}

void LevelCelView::displayFrame()
{
    this->update();

    this->celScene.clear();
    this->celScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    if (this->dunView) {
        DunDrawParam params;
        params.tileState = this->ui->showTilesRadioButton->isChecked() ? Qt::Checked : (this->ui->showFloorRadioButton->isChecked() ? Qt::PartiallyChecked : Qt::Unchecked);
        params.showRooms = this->ui->showRoomsMetaRadioButton->isChecked();
        params.showTileProtections = this->ui->showTileMetaRadioButton->isChecked();
        params.showSubtileProtections = this->ui->showSubtileMetaRadioButton->isChecked();
        params.showItems = this->ui->showItemsCheckBox->isChecked();
        params.showMonsters = this->ui->showMonstersCheckBox->isChecked();
        params.showObjects = this->ui->showObjectsCheckBox->isChecked();
        params.time = this->playCounter;
        QImage dunFrame = this->dun->getImage(params);

        this->celScene.setSceneRect(0, 0,
            CEL_SCENE_MARGIN + dunFrame.width() + CEL_SCENE_MARGIN,
            CEL_SCENE_MARGIN + dunFrame.height() + CEL_SCENE_MARGIN);

        // this->celScene.addPixmap(QPixmap::fromImage(dunFrame))
        //    ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
        QGraphicsPixmapItem *item;
        item = this->celScene.addItem(QPixmap::fromImage(dunFrame));
        item->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
        item->setAcceptHoverEvents(true);
        // scroll to the current position
        if (this->isScrolling) {
            this->isScrolling = false;
            unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
            unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

            int cellWidth = subtileWidth;
            int cellHeight = subtileWidth / 2;
            // move to 0;0
            int cX = this->celScene.sceneRect().width() / 2;
            int cY = CEL_SCENE_MARGIN + subtileHeight - cellHeight / 2;
            int bX = cX, bY = cY;
            int offX = (this->dun->getWidth() - this->dun->getHeight()) * (cellWidth / 2);
            cX += offX;

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
        // Notify PalView that the frame changed (used to refresh palette widget)
        emit frameRefreshed();
        return;
    }

    // Getting the current frame/sub-tile/tile to display
    QImage celFrame = this->gfx->getFrameCount() != 0 ? this->gfx->getFrameImage(this->currentFrameIndex) : QImage();
    QImage subtile = this->min->getSubtileCount() != 0 ? this->min->getSubtileImage(this->currentSubtileIndex) : QImage();
    QImage tile = this->til->getTileCount() != 0 ? this->til->getTileImage(this->currentTileIndex) : QImage();

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
        return;
    }
    MainWindow *mw = &dMainWindow();
    QAction actions[15];
    QMenu contextMenu(this);

    QMenu frameMenu(tr("Frame"), this);
    frameMenu.setToolTipsVisible(true);

    int cursor = 0;
    actions[cursor].setText(tr("Add Layer"));
    actions[cursor].setToolTip(tr("Add the content of an image to the current frame"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAddTo_Frame_triggered()));
    frameMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Insert"));
    actions[cursor].setToolTip(tr("Add new frames before the current one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionInsert_Frame_triggered()));
    frameMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Append"));
    actions[cursor].setToolTip(tr("Append new frames at the end"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAdd_Frame_triggered()));
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
    actions[cursor].setToolTip(tr("Add new subtiles before the current one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionInsert_Subtile_triggered()));
    subtileMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Append"));
    actions[cursor].setToolTip(tr("Append new subtiles at the end"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAdd_Subtile_triggered()));
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
    actions[cursor].setToolTip(tr("Add new tiles before the current one"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionInsert_Tile_triggered()));
    tileMenu.addAction(&actions[cursor]);

    cursor++;
    actions[cursor].setText(tr("Append"));
    actions[cursor].setToolTip(tr("Append new tiles at the end"));
    QObject::connect(&actions[cursor], SIGNAL(triggered()), mw, SLOT(on_actionAdd_Tile_triggered()));
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

void LevelCelView::on_firstFrameButton_clicked()
{
    this->setFrameIndex(0);
}

void LevelCelView::on_previousFrameButton_clicked()
{
    this->setFrameIndex(this->currentFrameIndex - 1);
}

void LevelCelView::on_nextFrameButton_clicked()
{
    this->setFrameIndex(this->currentFrameIndex + 1);
}

void LevelCelView::on_lastFrameButton_clicked()
{
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

void LevelCelView::on_firstSubtileButton_clicked()
{
    this->setSubtileIndex(0);
}

void LevelCelView::on_previousSubtileButton_clicked()
{
    this->setSubtileIndex(this->currentSubtileIndex - 1);
}

void LevelCelView::on_nextSubtileButton_clicked()
{
    this->setSubtileIndex(this->currentSubtileIndex + 1);
}

void LevelCelView::on_lastSubtileButton_clicked()
{
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

void LevelCelView::on_firstTileButton_clicked()
{
    this->setTileIndex(0);
}

void LevelCelView::on_previousTileButton_clicked()
{
    this->setTileIndex(this->currentTileIndex - 1);
}

void LevelCelView::on_nextTileButton_clicked()
{
    this->setTileIndex(this->currentTileIndex + 1);
}

void LevelCelView::on_lastTileButton_clicked()
{
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

void LevelCelView::on_playButton_clicked()
{
    if (this->playTimer != 0) {
        return;
    }

    // disable the related fields
    this->ui->playButton->setEnabled(false);
    this->ui->playDelayEdit->setReadOnly(true);
    this->ui->playComboBox->setEnabled(false);
    this->ui->dunPlayButton->setEnabled(false);
    this->ui->dunPlayDelayEdit->setReadOnly(true);
    this->ui->dunPlayComboBox->setEnabled(false);
    // enable the stop button
    this->ui->stopButton->setEnabled(true);
    this->ui->dunStopButton->setEnabled(true);
    // preserve the palette
    dMainWindow().initPaletteCycle();

    this->playTimer = this->startTimer(this->dunView ? this->dunviewPlayDelay : this->tilesetPlayDelay);
}

void LevelCelView::on_stopButton_clicked()
{
    this->killTimer(this->playTimer);
    this->playTimer = 0;
    this->playCounter = 0;

    // restore palette
    dMainWindow().resetPaletteCycle();
    // disable the stop button
    this->ui->stopButton->setEnabled(false);
    this->ui->dunStopButton->setEnabled(false);
    // enable the related fields
    this->ui->playButton->setEnabled(true);
    this->ui->playDelayEdit->setReadOnly(false);
    this->ui->playComboBox->setEnabled(true);
    this->ui->dunPlayButton->setEnabled(true);
    this->ui->dunPlayDelayEdit->setReadOnly(false);
    this->ui->dunPlayComboBox->setEnabled(true);
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

void LevelCelView::on_actionToggle_View_triggered()
{
    // stop playback
    if (this->playTimer != 0) {
        this->on_stopButton_clicked();
    }

    bool dunMode = !this->dunView;
    this->dunView = dunMode;
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
}

void LevelCelView::setPositionX(int posx)
{
    if (posx < 0) {
        posx = 0;
    }
    if (posx >= this->dun->getWidth()) {
        posx = this->dun->getWidth() - 1;
    }
    bool change = this->currentDunPosX != posx;
    this->currentDunPosX = posx;
    this->on_dungeonPosXLineEdit_escPressed();
    if (change) {
        // update the view
        this->update();
    }
}

void LevelCelView::setPositionY(int posy)
{
    if (posy < 0) {
        posy = 0;
    }
    if (posy >= this->dun->getHeight()) {
        posy = this->dun->getHeight() - 1;
    }
    bool change = this->currentDunPosY != posy;
    this->currentDunPosY = posy;
    this->on_dungeonPosYLineEdit_escPressed();
    if (change) {
        // update the view
        this->update();
    }
}

void LevelCelView::on_moveLeftButton_clicked()
{
    this->setPositionX(this->currentDunPosX - 1);
}

void LevelCelView::on_moveRightButton_clicked()
{
    this->setPositionX(this->currentDunPosX + 1);
}

void LevelCelView::on_moveUpButton_clicked()
{
    this->setPositionY(this->currentDunPosY - 1);
}

void LevelCelView::on_moveDownButton_clicked()
{
    this->setPositionY(this->currentDunPosY + 1);
}

void LevelCelView::on_dungeonPosXLineEdit_returnPressed()
{
    int posx = this->ui->dungeonPosXLineEdit->text().toInt();
    this->setPositionX(posx);
}

void LevelCelView::on_dungeonPosXLineEdit_escPressed()
{
    int posx = this->currentDunPosX;
    this->ui->dungeonPosXLineEdit->setText(QString::number(posx));
    this->ui->dungeonPosXLineEdit->clearFocus();
}

void LevelCelView::on_dungeonPosYLineEdit_returnPressed()
{
    int posy = this->ui->dungeonPosYLineEdit->text().toInt();
    this->setPositionY(posy);
}

void LevelCelView::on_dungeonPosYLineEdit_escPressed()
{
    int posy = this->currentDunPosY;
    this->ui->dungeonPosYLineEdit->setText(QString::number(posy));
    this->ui->dungeonPosYLineEdit->clearFocus();
}

void LevelCelView::on_dunWidthEdit_returnPressed()
{
    int newWidth = this->ui->dunWidthEdit->text().toUShort();

    bool change = this->dun->setWidth(newWidth, false);
    this->on_dunWidthEdit_escPressed();
    if (change) {
        if (this->currentDunPosX >= newWidth) {
            this->currentDunPosX = newWidth - 1;
            this->on_dungeonPosXLineEdit_escPressed();
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
            this->on_dungeonPosYLineEdit_escPressed();
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
    int defaultTile = this->ui->dungeonDefaultTileLineEdit->text().toInt();

    bool change = this->dun->setDefaultTile(defaultTile);
    this->on_dungeonDefaultTileLineEdit_escPressed();
    if (change) {
        this->ui->levelTypeComboBox->setCurrentIndex(-1);
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

void LevelCelView::on_showMetaButtonGroup_idClicked()
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

void LevelCelView::on_dungeonObjectLineEdit_returnPressed()
{
    int objectIndex = this->ui->dungeonObjectLineEdit->text().toUShort();

    bool change = this->dun->setObjectAt(this->currentDunPosX, this->currentDunPosY, objectIndex);
    this->on_dungeonObjectLineEdit_escPressed();
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonObjectLineEdit_escPressed()
{
    int objectIndex = this->dun->getObjectAt(this->currentDunPosX, this->currentDunPosY);

    this->ui->dungeonObjectLineEdit->setText(QString::number(objectIndex));
    this->ui->dungeonObjectLineEdit->clearFocus();
}

void LevelCelView::on_dungeonObjectComboBox_activated(int index)
{
    if (index < 0) {
        return;
    }
    int objectIndex = this->ui->dungeonObjectComboBox->itemData(index).value<int>();

    bool change = this->dun->setObjectAt(this->currentDunPosX, this->currentDunPosY, objectIndex);
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonObjectAddButton_clicked()
{
    this->dungeonResourceDialog.initialize(DUN_ENTITY_TYPE::OBJECT, this->dun);
    this->dungeonResourceDialog.show();
}

void LevelCelView::on_dungeonMonsterLineEdit_returnPressed()
{
    int monsterIndex = this->ui->dungeonMonsterLineEdit->text().toUShort();
    bool monsterUnique = this->ui->dungeonMonsterCheckBox->isChecked();
    DunMonsterType monType = { monsterIndex, monsterUnique };

    bool change = this->dun->setMonsterAt(this->currentDunPosX, this->currentDunPosY, monType);
    this->on_dungeonMonsterLineEdit_escPressed();
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonMonsterLineEdit_escPressed()
{
    DunMonsterType monType = this->dun->getMonsterAt(this->currentDunPosX, this->currentDunPosY);

    this->ui->dungeonMonsterLineEdit->setText(QString::number(monType.first));
    this->ui->dungeonMonsterLineEdit->clearFocus();
}

void LevelCelView::on_dungeonMonsterCheckBox_clicked()
{
    int monsterIndex = this->ui->dungeonMonsterLineEdit->text().toUShort();
    bool monsterUnique = this->ui->dungeonMonsterCheckBox->isChecked();
    DunMonsterType monType = { monsterIndex, monsterUnique };

    bool change = this->dun->setMonsterAt(this->currentDunPosX, this->currentDunPosY, monType);
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonMonsterComboBox_activated(int index)
{
    if (index < 0) {
        return;
    }
    DunMonsterType monType = this->ui->dungeonMonsterComboBox->itemData(index).value<DunMonsterType>();

    bool change = this->dun->setMonsterAt(this->currentDunPosX, this->currentDunPosY, monType);
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::on_dungeonMonsterAddButton_clicked()
{
    this->dungeonResourceDialog.initialize(DUN_ENTITY_TYPE::MONSTER, this->dun);
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
    this->dungeonResourceDialog.initialize(DUN_ENTITY_TYPE::ITEM, this->dun);
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

void LevelCelView::on_dunPlayButton_clicked()
{
    this->on_playButton_clicked();
}

void LevelCelView::on_dunStopButton_clicked()
{
    this->on_stopButton_clicked();
}
