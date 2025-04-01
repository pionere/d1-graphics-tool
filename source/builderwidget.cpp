#include "builderwidget.h"

#include <QCursor>
#include <QGraphicsView>
#include <QImage>
#include <QList>
#include <QMessageBox>
#include <QString>

#include "config.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_builderwidget.h"

DunPos::DunPos(int cx, int cy, int v)
    : cellX(cx)
    , cellY(cy)
    , value(v)
{
}

EditDungeonCommand::EditDungeonCommand(D1Dun *d, int cellX, int cellY, int value, int vt)
    : QUndoCommand(nullptr)
    , dun(d)
    , valueType(vt)
{
    this->modValues.push_back(DunPos(cellX, cellY, value));
}

EditDungeonCommand::EditDungeonCommand(D1Dun *d, const std::vector<DunPos> &mods, int vt)
    : QUndoCommand(nullptr)
    , dun(d)
    , valueType(vt)
    , modValues(mods)
{
}

void EditDungeonCommand::undo()
{
    if (this->dun.isNull()) {
        this->setObsolete(true);
        return;
    }

    for (DunPos &dp : this->modValues) {
        int currValue;
        switch (this->valueType) {
        case BEM_TILE:
            currValue = this->dun->getTileAt(dp.cellX, dp.cellY); // TODO: store subtiles as well
            this->dun->setTileAt(dp.cellX, dp.cellY, dp.value);
            dp.value = currValue;
            break;
        case BEM_TILE_PROTECTION:
            currValue = (int)this->dun->getTileProtectionAt(dp.cellX, dp.cellY);
            this->dun->setTileProtectionAt(dp.cellX, dp.cellY, (Qt::CheckState)dp.value);
            dp.value = currValue;
            break;
        case BEM_SUBTILE:
            currValue = (int)this->dun->getSubtileAt(dp.cellX, dp.cellY); // TODO: store tiles as well
            this->dun->setSubtileAt(dp.cellX, dp.cellY, dp.value);
            dp.value = currValue;
            break;
        case BEM_SUBTILE_PROTECTION:
            currValue = (this->dun->getSubtileMonProtectionAt(dp.cellX, dp.cellY) ? 1 : 0) | (this->dun->getSubtileObjProtectionAt(dp.cellX, dp.cellY) ? 2 : 0);
            this->dun->setSubtileMonProtectionAt(dp.cellX, dp.cellY, (dp.value & 1) != 0);
            this->dun->setSubtileObjProtectionAt(dp.cellX, dp.cellY, (dp.value & 2) != 0);
            dp.value = currValue;
            break;
        case BEM_OBJECT: {
            MapObject obj = this->dun->getObjectAt(dp.cellX, dp.cellY);
            currValue = obj.oType;
            obj.oType = dp.value;
            this->dun->setObjectAt(dp.cellX, dp.cellY, obj);
            dp.value = currValue;
        } break;
        case BEM_MONSTER: {
            MapMonster mon = this->dun->getMonsterAt(dp.cellX, dp.cellY);
            currValue = mon.moType.monIndex | (mon.moType.monUnique ? 1 << 31 : 0);
            mon.moType.monIndex = dp.value & INT32_MAX;
            mon.moType.monUnique = (dp.value & (1 << 31)) != 0;
            this->dun->setMonsterAt(dp.cellX, dp.cellY, mon);
            dp.value = currValue;
        } break;
        }
    }

    emit this->modified();
}

void EditDungeonCommand::redo()
{
    this->undo();
}

BuilderWidget::BuilderWidget(QWidget *parent, QUndoStack *us, D1Dun *d, LevelCelView *lcv, D1Tileset *ts)
    : QFrame(parent)
    , ui(new Ui::BuilderWidget())
    , undoStack(us)
    , dun(d)
    , levelCelView(lcv)
    , tileset(ts)
{
    this->ui->setupUi(this);

    QLayout *layout = this->ui->centerButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogListView, tr("Move"), this, &BuilderWidget::on_movePushButtonClicked);
    layout = this->ui->rightButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogCloseButton, tr("Close"), this, &BuilderWidget::on_closePushButtonClicked);

    // initialize the drop-down
    this->ui->builderModeComboBox->setCurrentIndex(this->mode);
    // initialize the edit fields
    this->on_tileLineEdit_escPressed();
    this->on_subtileLineEdit_escPressed();
    this->on_objectLineEdit_escPressed();
    this->on_monsterLineEdit_escPressed();

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->tileLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_tileLineEdit_escPressed()));
    QObject::connect(this->ui->subtileLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_subtileLineEdit_escPressed()));
    QObject::connect(this->ui->objectLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_objectLineEdit_escPressed()));
    QObject::connect(this->ui->monsterLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_monsterLineEdit_escPressed()));

    this->adjustSize(); // not sure why this is necessary...

    // cache the active graphics view
    QList<QGraphicsView *> views = lcv->getCelScene()->views();
    this->graphView = views[0];
}

BuilderWidget::~BuilderWidget()
{
    delete ui;
}

void BuilderWidget::setDungeon(D1Dun *d)
{
    this->dun = d;

    this->colorModified();
}

void BuilderWidget::show()
{
    this->resetPos();
    QFrame::show();

    this->setFocus(); // otherwise the widget does not receive keypress events...
    this->graphView->setCursor(Qt::BlankCursor);
    // update the view
    this->dunResourcesModified();
    this->colorModified();
}

void BuilderWidget::hide()
{
    if (this->moving) {
        this->stopMove();
    }
    this->graphView->unsetCursor();

    QFrame::hide();
}

int BuilderWidget::getOverlayType() const
{
    return (this->isHidden() || this->mode == BEM_SELECT) ? -1 : this->mode;
}

bool BuilderWidget::dunClicked(const QPoint &cellClick, int flags)
{
    if (this->isHidden() || this->mode == BEM_SELECT) {
        return false;
    }

    std::vector<DunPos> modValues;
    if (!(flags & FIRST_CLICK)) {
        if (this->lastPos == cellClick) {
            return true;
        }
        int fcx = cellClick.x();
        int lcx = this->lastPos.x();
        int fcy = cellClick.y();
        int lcy = this->lastPos.y();

        // collect locations
        if (fcx > lcx) {
            std::swap(fcx, lcx);
        }
        if (fcy > lcy) {
            std::swap(fcy, lcy);
        }
        for (int x = fcx; x <= lcx; x++) {
            for (int y = fcy; y <= lcy; y++) {
                modValues.push_back(DunPos(x, y, 0));
            }
        }

        // rollback previous change
        this->undoStack->undo();
    } else {
        this->lastPos = cellClick;
        // collect locations
        modValues.push_back(DunPos(cellClick.x(), cellClick.y(), 0));
    }

    // calculate the value (reset value if it is the same as before)
    const QPoint cell = this->lastPos;
    int value = 0, v;
    if (cell.x() >= 0 && cell.x() < this->dun->getWidth() && cell.y() >= 0 && cell.y() < this->dun->getHeight()) {
    switch (this->mode) {
    case BEM_TILE:
        value = this->currentTileIndex; // this->ui->tileLineEdit->text().toInt();
        if (value == this->dun->getTileAt(cell.x(), cell.y())) {
            value = 0;
        }
        break;
    case BEM_TILE_PROTECTION:
        value = this->ui->tileProtectionModeComboBox->currentIndex();
        value = (int)(value == 0 ? Qt::Checked : (value == 1 ? Qt::PartiallyChecked : Qt::Unchecked));
        if (value == (int)this->dun->getTileProtectionAt(cell.x(), cell.y())) {
            value = (int)Qt::Unchecked;
        }
        break;
    case BEM_SUBTILE:
        value = this->currentSubtileIndex; // this->ui->subtileLineEdit->text().toInt();
        if (value == this->dun->getSubtileAt(cell.x(), cell.y())) {
            value = 0;
        }
        break;
    case BEM_SUBTILE_PROTECTION:
        value = this->ui->subtileProtectionModeComboBox->currentIndex();
        v = (this->dun->getSubtileMonProtectionAt(cell.x(), cell.y()) ? 1 : 0) | (this->dun->getSubtileObjProtectionAt(cell.x(), cell.y()) ? 2 : 0);
        if (value == 0) {
            if (v == 3) {
                value = 0;
            } else {
                value = 3;
            }
        } else if (value == 1) {
            if (v & 1) {
                v = -1;
            }
        } else if (value == 2) {
            if (v & 2) {
                v = -1;
            }
        } else {
            value = 0;
        }
        break;
    case BEM_OBJECT:
        value = this->currentObjectIndex; // this->ui->objectLineEdit->text().toInt();
        if (value == this->dun->getObjectAt(cell.x(), cell.y()).oType) {
            value = 0;
        }
        break;
    case BEM_MONSTER:
        value = this->currentMonsterType.monIndex; // this->ui->monsterLineEdit->text().toInt();
        value |= this->currentMonsterType.monUnique /*this->ui->monsterCheckBox->isChecked()*/ ? 1 << 31 : 0;
        if (this->currentMonsterType == this->dun->getMonsterAt(cell.x(), cell.y()).moType) {
            value = 0;
        }
        break;
    }
    }

    // filter the positions
    for (auto it = modValues.begin(); it != modValues.end(); ) {
        const DunPos &dp = *it;
        if (dp.cellX >= 0 && dp.cellX < this->dun->getWidth() && dp.cellY >= 0 && dp.cellY < this->dun->getHeight()) {
            it++;
        } else {
            it = modValues.erase(it);
        }
    }

    // set values
    if (this->mode != BEM_SUBTILE_PROTECTION || value == 0 || value == 3) {
        for (DunPos &dp : modValues) {
            dp.value = value;
        }
    } else {
        if (value == 1) {
            // toggle monster protection
            v = v < 0 ? 0 : 1;
            for (DunPos &dp : modValues) {
                value = v | (this->dun->getSubtileObjProtectionAt(dp.cellX, dp.cellY) ? 2 : 0);
                dp.value = value;
            }
        } else {
            // toggle object protection
            v = v < 0 ? 0 : 2;
            for (DunPos &dp : modValues) {
                value = v | (this->dun->getSubtileMonProtectionAt(dp.cellX, dp.cellY) ? 1 : 0);
                dp.value = value;
            }
        }
    }

    // Build frame editing command and connect it to the current main window widget
    // to update the palHits and CEL views when undo/redo is performed
    EditDungeonCommand *command = new EditDungeonCommand(this->dun, modValues, this->mode);
    QObject::connect(command, &EditDungeonCommand::modified, &dMainWindow(), &MainWindow::updateWindow);

    this->undoStack->push(command);
    return true;
}

static void drawHollowDiamond(QImage &image, unsigned width, const QColor &color)
{
    unsigned y = 1;
    QRgb srcBit = color.rgba();
    QRgb *destBits = reinterpret_cast<QRgb *>(image.scanLine(0 + y));
    // draw the upper right line
    destBits += width / 2;
    for (unsigned x = 0; x < width / 4; x++) {
        destBits[0] = srcBit;
        destBits[1] = srcBit;
        destBits += width + 2;
    }
    // draw the lower right line
    destBits -= 4;
    for (unsigned x = 0; x < width / 4 - 1; x++) {
        destBits[0] = srcBit;
        destBits[1] = srcBit;
        destBits += width - 2;
    }
    // draw the lower left line
    destBits -= width;
    for (unsigned x = 0; x < width / 4; x++) {
        destBits[0] = srcBit;
        destBits[1] = srcBit;
        destBits -= width + 2;
    }
    // draw the upper left line
    destBits += 4;
    for (unsigned x = 0; x < width / 4 - 1; x++) {
        destBits[0] = srcBit;
        destBits[1] = srcBit;
        destBits -= width - 2;
    }
}

void BuilderWidget::dunHovered(const QPoint &cell)
{
    this->lastHoverPos = cell;

    this->redrawOverlay(false);
}

void BuilderWidget::redrawOverlay(bool forceRedraw)
{
    unsigned subtileWidth = this->tileset->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->tileset->min->getSubtileHeight() * MICRO_HEIGHT;

    int cellWidth = subtileWidth;
    int cellHeight = subtileWidth / 2;

    QGraphicsScene *scene = this->graphView->scene(); // this->levelCelView->getCelScene();
    QList<QGraphicsItem *> items = scene->items();
    QGraphicsPixmapItem *overlay = nullptr;
    if (items.count() == 2) {
        overlay = reinterpret_cast<QGraphicsPixmapItem *>(items[0]);
    }
    int overlayType = this->getOverlayType();
    bool change = this->overlayType != overlayType || overlay == nullptr || forceRedraw;
    if (change) {
        this->overlayType = overlayType;
        QImage image;
        QColor color = QColorConstants::Svg::darkcyan;
        int value;
        switch (overlayType) {
        case BEM_TILE:
            if (this->currentTileIndex != 0) {
                if (this->currentTileIndex > 0 && this->currentTileIndex <= this->tileset->til->getTileCount()) {
                    image = this->tileset->til->getTileImage(this->currentTileIndex - 1);
                }
                color = QColorConstants::Svg::magenta;
            }
            break;
        case BEM_TILE_PROTECTION:
            value = this->ui->tileProtectionModeComboBox->currentIndex();
            color = value == 0 ? QColorConstants::Svg::orangered : (value == 1 ? QColorConstants::Svg::plum : QColorConstants::Svg::darkcyan);
            break;
        case BEM_SUBTILE:
            if (this->currentSubtileIndex != 0) {
                if (this->currentSubtileIndex > 0 && this->currentSubtileIndex <= this->tileset->min->getSubtileCount()) {
                    image = this->tileset->min->getSubtileImage(this->currentSubtileIndex - 1);
                }
                color = QColorConstants::Svg::magenta;
            }
            break;
        case BEM_SUBTILE_PROTECTION:
            value = this->ui->subtileProtectionModeComboBox->currentIndex();
            color = value == 0 ? QColorConstants::Svg::crimson : (value == 1 ? QColorConstants::Svg::lightcoral : (value == 2 ? QColorConstants::Svg::peru : QColorConstants::Svg::darkcyan));
            break;
        case BEM_OBJECT:
            if (this->currentObjectIndex != 0) {
                MapObject obj = MapObject();
                obj.oType = this->currentObjectIndex;
                image = this->dun->getObjectImage(obj);
                color = QColorConstants::Svg::magenta;
            }
            break;
        case BEM_MONSTER:
            if (this->currentMonsterType.monIndex != 0 || this->currentMonsterType.monUnique) {
                MapMonster mon = MapMonster();
                mon.moType = this->currentMonsterType;
                image = this->dun->getMonsterImage(mon);
                color = QColorConstants::Svg::magenta;
            }
            break;
        }
        if (image.isNull()) {
            int mpl = (this->overlayType == BEM_TILE || this->overlayType == BEM_TILE_PROTECTION) ? 2 : 1;
            image = QImage(cellWidth * mpl, cellHeight * mpl, QImage::Format_ARGB32);
            image.fill(Qt::transparent);
            drawHollowDiamond(image, cellWidth * mpl, color);
        }

        QPixmap pixmap = QPixmap::fromImage(image);
        if (overlay == nullptr) {
            overlay = scene->addPixmap(pixmap);
        } else {
            overlay->setPixmap(pixmap);
        }
    }

    QPoint op;
    int cellX = this->lastHoverPos.x();
    int cellY = this->lastHoverPos.y();
    // SHIFT_GRID
    int dunX = cellX - cellY;
    int dunY = cellX + cellY;

    // switch unit
    int cX = dunX * (cellWidth / 2);
    int cY = dunY * (cellHeight / 2);

    // move to 0;0
    cX += scene->sceneRect().width() / 2 - (this->dun->getWidth() - this->dun->getHeight()) * (cellWidth / 2);
    cY += CEL_SCENE_MARGIN + subtileHeight;

    // center the image
    cX -= overlay->pixmap().width() / 2;
    cY -= overlay->pixmap().height();

    if (this->overlayType == BEM_TILE || this->overlayType == BEM_TILE_PROTECTION) {
        cY += cellHeight;
        if (cellX & 1) {
            cX -= cellWidth / 2;
            cY -= cellHeight / 2;
        }
        if (cellY & 1) {
            cX += cellWidth / 2;
            cY -= cellHeight / 2;
        }
    }

    op = QPoint(cX, cY);

    if (change || overlay->pos() != op) {
        overlay->setPos(op);
        scene->update();
    }    
}

void BuilderWidget::colorModified()
{
    if (this->isHidden())
        return;
    this->redrawOverlay(true);
}

static void copyComboBox(QComboBox *cmbDst, const QComboBox *cmbSrc)
{
    cmbDst->hide();
    cmbDst->clear();
    for (int i = 0; i < cmbSrc->count(); i++) {
        QVariant data = cmbSrc->itemData(i);
        if (data.isValid()) {
            cmbDst->addItem(cmbSrc->itemText(i), data);
        } else {
            cmbDst->insertSeparator(i);
        }
    }
    cmbDst->show();
}

void BuilderWidget::dunResourcesModified()
{
    if (this->isHidden())
        return;

    QComboBox *comboBox;

    // prepare the comboboxes
    // - objects
    comboBox = this->ui->objectComboBox;
    copyComboBox(comboBox, this->levelCelView->getObjects());
    comboBox->setCurrentIndex(comboBox->findData(this->currentObjectIndex));

    // - monsters
    comboBox = this->ui->monsterComboBox;
    copyComboBox(comboBox, this->levelCelView->getMonsters());
    comboBox->setCurrentIndex(LevelCelView::findMonType(comboBox, this->currentMonsterType));
}

void BuilderWidget::on_closePushButtonClicked()
{
    this->hide();
}

void BuilderWidget::resetPos()
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

void BuilderWidget::stopMove()
{
    this->setMouseTracking(false);
    this->moving = false;
    this->moved = true;
    this->releaseMouse();
    // this->setCursor(Qt::BlankCursor);
}

void BuilderWidget::on_movePushButtonClicked()
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

void BuilderWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Cancel)) {
        if (this->moving) {
            this->stopMove();
        } else {
            this->hide();
        }
        return;
    }

    QFrame::keyPressEvent(event);
}

void BuilderWidget::mousePressEvent(QMouseEvent *event)
{
    if (this->moving) {
        this->stopMove();
        return;
    }

    QFrame::mouseMoveEvent(event);
}

void BuilderWidget::mouseMoveEvent(QMouseEvent *event)
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

void BuilderWidget::on_builderModeComboBox_activated(int index)
{
    int prevMode = this->mode;
    QWidget *layout;
    switch (prevMode) {
    case BEM_SELECT:
        layout = nullptr;
        break;
    case BEM_TILE:
        layout = this->ui->tileModeWidget;
        break;
    case BEM_TILE_PROTECTION:
        layout = this->ui->tileProtectionModeWidget;
        break;
    case BEM_SUBTILE:
        layout = this->ui->subtileModeWidget;
        break;
    case BEM_SUBTILE_PROTECTION:
        layout = this->ui->subtileProtectionModeWidget;
        break;
    case BEM_OBJECT:
        layout = this->ui->objectModeWidget;
        break;
    case BEM_MONSTER:
        layout = this->ui->monsterModeWidget;
        break;
    }
    if (layout != nullptr) {
        layout->setVisible(false);
    }
    this->mode = index;
    switch (this->mode) {
    case BEM_SELECT:
        layout = nullptr;
        break;
    case BEM_TILE:
        layout = this->ui->tileModeWidget;
        break;
    case BEM_TILE_PROTECTION:
        layout = this->ui->tileProtectionModeWidget;
        break;
    case BEM_SUBTILE:
        layout = this->ui->subtileModeWidget;
        break;
    case BEM_SUBTILE_PROTECTION:
        layout = this->ui->subtileProtectionModeWidget;
        break;
    case BEM_OBJECT:
        layout = this->ui->objectModeWidget;
        break;
    case BEM_MONSTER:
        layout = this->ui->monsterModeWidget;
        break;
    }
    if (layout != nullptr) {
        layout->setVisible(true);
    }

    this->adjustSize(); // not sure why this is necessary...
    this->resetPos();
    this->redrawOverlay(false);
}

void BuilderWidget::setTileIndex(int tileIndex)
{
    this->currentTileIndex = tileIndex;
    this->ui->tileLineEdit->setText(QString::number(tileIndex));
    this->redrawOverlay(true);
}

void BuilderWidget::setSubtileIndex(int subtileIndex)
{
    this->currentSubtileIndex = subtileIndex;
    this->ui->subtileLineEdit->setText(QString::number(subtileIndex));
    this->redrawOverlay(true);
}

void BuilderWidget::setObjectIndex(int objectIndex)
{
    this->currentObjectIndex = objectIndex;
    this->ui->objectLineEdit->setText(QString::number(objectIndex));
    this->ui->objectComboBox->setCurrentIndex(this->ui->objectComboBox->findData(objectIndex));
    this->redrawOverlay(true);
}

void BuilderWidget::setMonsterType(DunMonsterType monType)
{
    this->currentMonsterType = monType;
    this->ui->monsterLineEdit->setText(QString::number(monType.monIndex));
    this->ui->monsterCheckBox->setChecked(monType.monUnique);
    this->ui->monsterComboBox->setCurrentIndex(LevelCelView::findMonType(this->ui->monsterComboBox, monType));
    this->redrawOverlay(true);
}

void BuilderWidget::on_firstTileButton_clicked()
{
    this->setTileIndex(0);
}

void BuilderWidget::on_previousTileButton_clicked()
{
    int tileIndex = this->currentTileIndex - 1;
    if (tileIndex < 0) {
        tileIndex = 0;
    }
    this->setTileIndex(tileIndex);
}

void BuilderWidget::on_nextTileButton_clicked()
{
    int tileIndex = this->currentTileIndex + 1;
    int tileIndexLimit = this->tileset->til->getTileCount();
    if (tileIndex == tileIndexLimit) {
        tileIndex = tileIndexLimit - 1;
    }
    this->setTileIndex(tileIndex);
}

void BuilderWidget::on_lastTileButton_clicked()
{
    this->setTileIndex(this->tileset->til->getTileCount() - 1);
}

void BuilderWidget::on_tileLineEdit_returnPressed()
{
    int tileIndex = this->ui->tileLineEdit->text().toInt();

    this->setTileIndex(tileIndex);

    this->on_tileLineEdit_escPressed();
}

void BuilderWidget::on_tileLineEdit_escPressed()
{
    this->ui->tileLineEdit->setText(QString::number(this->currentTileIndex));
    this->ui->tileLineEdit->clearFocus();
}

void BuilderWidget::on_tileProtectionModeComboBox_activated(int index)
{
    this->redrawOverlay(true);
}

void BuilderWidget::on_firstSubtileButton_clicked()
{
    this->setSubtileIndex(0);
}

void BuilderWidget::on_previousSubtileButton_clicked()
{
    int subtileIndex = this->currentSubtileIndex - 1;
    if (subtileIndex < 0) {
        subtileIndex = 0;
    }
    this->setSubtileIndex(subtileIndex);
}

void BuilderWidget::on_nextSubtileButton_clicked()
{
    int subtileIndex = this->currentSubtileIndex + 1;
    int subtileIndexLimit = this->tileset->min->getSubtileCount();
    if (subtileIndex == subtileIndexLimit) {
        subtileIndex = subtileIndexLimit - 1;
    }
    this->setSubtileIndex(subtileIndex);
}

void BuilderWidget::on_lastSubtileButton_clicked()
{
    this->setSubtileIndex(this->tileset->min->getSubtileCount() - 1);
}

void BuilderWidget::on_subtileLineEdit_returnPressed()
{
    int subtileIndex = this->ui->subtileLineEdit->text().toInt();

    this->setSubtileIndex(subtileIndex);

    this->on_subtileLineEdit_escPressed();
}

void BuilderWidget::on_subtileLineEdit_escPressed()
{
    this->ui->subtileLineEdit->setText(QString::number(this->currentSubtileIndex));
    this->ui->subtileLineEdit->clearFocus();
}

void BuilderWidget::on_subtileProtectionModeComboBox_activated(int index)
{
    this->redrawOverlay(true);
}

void BuilderWidget::on_objectLineEdit_returnPressed()
{
    int objectIndex = this->ui->objectLineEdit->text().toInt();

    this->setObjectIndex(objectIndex);

    this->on_objectLineEdit_escPressed();
}

void BuilderWidget::on_objectLineEdit_escPressed()
{
    this->ui->objectLineEdit->setText(QString::number(this->currentObjectIndex));
    this->ui->objectLineEdit->clearFocus();
}

void BuilderWidget::on_objectComboBox_activated(int index)
{
    if (index < 0) {
        return;
    }
    int objectIndex = this->ui->objectComboBox->itemData(index).value<int>();

    this->setObjectIndex(objectIndex);
}

void BuilderWidget::on_monsterLineEdit_returnPressed()
{
    this->on_monsterCheckBox_clicked();

    this->on_monsterLineEdit_escPressed();
}

void BuilderWidget::on_monsterLineEdit_escPressed()
{
    this->ui->monsterLineEdit->setText(QString::number(this->currentMonsterType.monIndex));
    this->ui->monsterLineEdit->clearFocus();
}

void BuilderWidget::on_monsterCheckBox_clicked()
{
    int monsterIndex = this->ui->monsterLineEdit->text().toUShort();
    bool monsterUnique = this->ui->monsterCheckBox->isChecked();
    DunMonsterType monType = { monsterIndex, monsterUnique };

    this->setMonsterType(monType);
}

void BuilderWidget::on_monsterComboBox_activated(int index)
{
    if (index < 0) {
        return;
    }
    DunMonsterType monType = this->ui->monsterComboBox->itemData(index).value<DunMonsterType>();

    this->setMonsterType(monType);
}
