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
        DunMonsterType dmt;
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
            currValue = (int)this->dun->getSubtileProtectionAt(dp.cellX, dp.cellY);
            this->dun->setSubtileProtectionAt(dp.cellX, dp.cellY, (bool)dp.value);
            dp.value = currValue;
            break;
        case BEM_OBJECT:
            currValue = this->dun->getObjectAt(dp.cellX, dp.cellY);
            this->dun->setObjectAt(dp.cellX, dp.cellY, dp.value);
            dp.value = currValue;
            break;
        case BEM_MONSTER:
            dmt = this->dun->getMonsterAt(dp.cellX, dp.cellY);
            currValue = dmt.first | (dmt.second ? 1 << 31 : 0);
            dmt.first = dp.value & INT32_MAX;
            dmt.second = (dp.value & (1 << 31)) != 0;
            this->dun->setMonsterAt(dp.cellX, dp.cellY, dmt);
            dp.value = currValue;
            break;
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

bool BuilderWidget::dunClicked(const QPoint &pos, bool first)
{
    if (this->isHidden()) {
        return false;
    }

    // calculate the value
    int value;
    switch (this->mode) {
    case BEM_TILE:
        value = this->currentTileIndex; // this->ui->tileLineEdit->text().toInt();
        break;
    case BEM_TILE_PROTECTION:
        value = this->ui->tileProtectionModeComboBox->currentIndex();
        value = (int)(value == 0 ? Qt::Unchecked : (value == 1 ? Qt::PartiallyChecked : Qt::Checked));
        break;
    case BEM_SUBTILE:
        value = this->currentSubtileIndex; // this->ui->subtileLineEdit->text().toInt();
        break;
    case BEM_SUBTILE_PROTECTION:
        value = (int)(this->ui->subtileProtectionModeComboBox->currentIndex() == 1);
        break;
    case BEM_OBJECT:
        value = this->currentObjectIndex; // this->ui->objectLineEdit->text().toInt();
        break;
    case BEM_MONSTER:
        value = this->currentMonsterType.first; // this->ui->monsterLineEdit->text().toInt();
        value |= this->currentMonsterType.second /*this->ui->monsterCheckBox->isChecked()*/ ? 1 << 31 : 0;
        break;
    }

    std::vector<DunPos> modValues;
    if (!first) {
        if (this->lastPos == pos) {
            return true;
        }
        int fcx = pos.x();
        int lcx = this->lastPos.x();
        int fcy = pos.y();
        int lcy = this->lastPos.y();

        // collect locations
        if (fcx > lcx) {
            std::swap(fcx, lcx);
        }
        if (fcy > lcy) {
            std::swap(fcx, lcx);
        }
        for (int x = fcx; x <= lcx; x++) {
            for (int y = fcy; y <= lcy; y++) {
                modValues.push_back(DunPos(x, y, 0));
            }
        }

        // rollback previous change
        this->undoStack->undo();
    } else {
        this->lastPos = pos;
        // collect locations
        modValues.push_back(DunPos(pos.x(), pos.y(), 0));

        // reset value if it is the same as before
        switch (this->mode) {
        case BEM_TILE:
            if (value == this->dun->getTileAt(pos.x(), pos.y())) {
                value = 0;
            }
            break;
        case BEM_TILE_PROTECTION:
            if (value == (int)this->dun->getTileProtectionAt(pos.x(), pos.y())) {
                value = (int)Qt::Unchecked;
            }
            break;
        case BEM_SUBTILE:
            if (value == this->dun->getSubtileAt(pos.x(), pos.y())) {
                value = 0;
            }
            break;
        case BEM_SUBTILE_PROTECTION:
            if (value == (int)this->dun->getSubtileProtectionAt(pos.x(), pos.y())) {
                value = 0;
            }
            break;
        case BEM_OBJECT:
            if (value == this->dun->getObjectAt(pos.x(), pos.y())) {
                value = 0;
            }
            break;
        case BEM_MONSTER:
            if (this->currentMonsterType == this->dun->getMonsterAt(pos.x(), pos.y())) {
                value = 0;
            }
            break;
        }
    }

    // set values
    for (DunPos &dp : modValues) {
        dp.value = value;
    }

    // Build frame editing command and connect it to the current main window widget
    // to update the palHits and CEL views when undo/redo is performed
    EditDungeonCommand *command = new EditDungeonCommand(this->dun, modValues, this->mode);
    QObject::connect(command, &EditDungeonCommand::modified, &dMainWindow(), &MainWindow::updateWindow);

    this->undoStack->push(command);
    return true;
}

#define CELL_BORDER 0

static void drawHollowDiamond(QImage &image, unsigned width, const QColor &color)
{
    unsigned len = 0;
    unsigned y = 1;
    QRgb *destBits = reinterpret_cast<QRgb *>(image.scanLine(0 + CELL_BORDER + y));
    destBits += 0;
    QRgb srcBit = color.rgba();
    for (; y <= width / 4; y++) {
        len += 2;
        for (unsigned x = width / 2 - len - CELL_BORDER - 1; x <= width / 2 - len; x++) {
            // image.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, color);
            destBits[x + CELL_BORDER] = srcBit;
        }
        for (unsigned x = width / 2 + len - 1; x <= width / 2 + len + CELL_BORDER; x++) {
            // image.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, color);
            destBits[x + CELL_BORDER] = srcBit;
        }
        destBits += width + 2 * CELL_BORDER; // image.width();
    }
    for (; y < width / 2; y++) {
        len -= 2;
        for (unsigned x = width / 2 - len - CELL_BORDER - 1; x <= width / 2 - len; x++) {
            // image.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, color);
            destBits[x + CELL_BORDER] = srcBit;
        }
        for (unsigned x = width / 2 + len - 1; x <= width / 2 + len + CELL_BORDER; x++) {
            // image.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, color);
            destBits[x + CELL_BORDER] = srcBit;
        }
        destBits += width + 2 * CELL_BORDER; // image.width();
    }
}

static void maskImage(QImage &image)
{
    int y = 0;
    unsigned width = image.width();
    QRgb *destBits = reinterpret_cast<QRgb *>(image.scanLine(0 + CELL_BORDER + y));
    QRgb srcBit = QColor(Qt::transparent).rgba();
    for (y = image.height() - 1; y >= 0; y--) {
        for (int x = y & 1; x < width; x += 2) {
            // image.setPixelColor(x + CELL_BORDER, y + CELL_BORDER, QColor(Qt::transparent));
            destBits[x + CELL_BORDER] = srcBit;
        }
        destBits += width + 2 * CELL_BORDER; // image.width();
    }
}

void BuilderWidget::dunHovered(const QPoint &pos)
{
    unsigned subtileWidth = this->tileset->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->tileset->min->getSubtileHeight() * MICRO_HEIGHT;

    int cellWidth = subtileWidth;
    int cellHeight = subtileWidth / 2;

    QGraphicsScene *scene = this->graphView->scene(); // this->levelCelView->getCelScene();
    QList<QGraphicsItem *> items = scene->items();
    QGraphicsPixmapItem *overlay;

    int overlayType = this->isHidden() ? -1 : this->mode;
    if (this->overlayType != overlayType || items.size() < 2) {
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
            if (image.isNull()) {
                image = QImage(cellWidth * TILE_WIDTH, cellHeight * TILE_HEIGHT, QImage::Format_ARGB32);
                image.fill(Qt::transparent);
                drawHollowDiamond(image, cellWidth * TILE_WIDTH, color);
            }
            break;
        case BEM_TILE_PROTECTION:
            value = this->ui->tileProtectionModeComboBox->currentIndex();
            color = value == 0 ? QColorConstants::Svg::darkcyan : (value == 1 ? QColorConstants::Svg::plum : QColorConstants::Svg::orchid);
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
            color = value == 0 ? QColorConstants::Svg::darkcyan : QColorConstants::Svg::pink;
            break;
        case BEM_OBJECT:
            if (this->currentObjectIndex != 0) {
                image = this->dun->getObjectImage(this->currentObjectIndex, 0);
                color = QColorConstants::Svg::magenta;
            }
            break;
        case BEM_MONSTER:
            if (this->currentMonsterType.first != 0) {
                image = this->dun->getMonsterImage(this->currentMonsterType, 0);
                color = QColorConstants::Svg::magenta;
            }
            break;
        }
        if (image.isNull()) {
            image = QImage(cellWidth, cellHeight, QImage::Format_ARGB32);
            image.fill(Qt::transparent);
            drawHollowDiamond(image, cellWidth, color);
        } else {
            ; // maskImage(image);
        }

        QPixmap pixmap = QPixmap::fromImage(image);
        if (items.size() < 2) {
            overlay = scene->addPixmap(pixmap);
        } else {
            overlay = reinterpret_cast<QGraphicsPixmapItem *>(items[0]);
            overlay->setPixmap(pixmap);
        }
    } else {
        overlay = reinterpret_cast<QGraphicsPixmapItem *>(items[0]);
    }

    QPoint op;
    int cellX = pos.x();
    int cellY = pos.y();
    if (cellX >= 0 && cellX < this->dun->getWidth() && cellY >= 0 && cellY < this->dun->getHeight()) {
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

        if (this->overlayType == BEM_TILE) {
            cY += cellHeight;
            if (pos.x() & 1) {
                cX -= cellWidth / 2;
                cY -= cellHeight / 2;
            }
            if (pos.y() & 1) {
                cX += cellWidth / 2;
                cY -= cellHeight / 2;
            }
        }

        op = QPoint(cX, cY);
    } else {
        op = QCursor::pos();
        op = this->graphView->mapFromGlobal(op);
    }

    overlay->setPos(op);
}

void BuilderWidget::colorModified()
{
    if (this->isHidden())
        return;
    // update the color-icon
    /*QSize imageSize = this->ui->imageLabel->size();
    QImage image = QImage(imageSize, QImage::Format_ARGB32);
    image.fill(QColor(Config::getGraphicsTransparentColor()));

    unsigned numColors = this->selectedColors.size();
    if (numColors != 0) {
        for (int x = 0; x < imageSize.width(); x++) {
            QColor color = this->pal->getColor(this->selectedColors[x * numColors / imageSize.width()]);
            for (int y = 0; y < imageSize.height(); y++) {
                // image.setPixelColor(x, y, color);
                QRgb *destBits = reinterpret_cast<QRgb *>(image.scanLine(y));
                destBits[x] = color.rgba();
            }
        }
    }
    QPixmap pixmap = QPixmap::fromImage(std::move(image));
    this->ui->imageLabel->setPixmap(pixmap);*/
}

static void copyComboBox(QComboBox *cmbDst, const QComboBox *cmbSrc)
{
    cmbDst->hide();
    cmbDst->clear();
    for (int i = 0; i < cmbSrc->count(); i++) {
        QVariant data = cmbSrc->itemData(i);
        if (data != QVariant::Invalid) {
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
    comboBox->setCurrentIndex(comboBox->findData(QVariant::fromValue(this->currentMonsterType)));
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
    layout->setVisible(false);
    this->mode = index;
    switch (this->mode) {
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
    layout->setVisible(true);

    this->adjustSize(); // not sure why this is necessary...
    this->resetPos();
    this->overlayType = -1;
}

void BuilderWidget::setTileIndex(int tileIndex)
{
    this->currentTileIndex = tileIndex;
    this->ui->tileLineEdit->setText(QString::number(tileIndex));
    this->overlayType = -1;
}

void BuilderWidget::setSubtileIndex(int subtileIndex)
{
    this->currentSubtileIndex = subtileIndex;
    this->ui->subtileLineEdit->setText(QString::number(subtileIndex));
    this->overlayType = -1;
}

void BuilderWidget::setObjectIndex(int objectIndex)
{
    this->currentObjectIndex = objectIndex;
    this->ui->objectLineEdit->setText(QString::number(objectIndex));
    this->ui->objectComboBox->setCurrentIndex(this->ui->objectComboBox->findData(objectIndex));
    this->overlayType = -1;
}

void BuilderWidget::setMonsterType(DunMonsterType monType)
{
    this->currentMonsterType = monType;
    this->ui->monsterLineEdit->setText(QString::number(monType.first));
    this->ui->monsterCheckBox->setChecked(monType.second);
    this->ui->monsterComboBox->setCurrentIndex(this->ui->monsterComboBox->findData(QVariant::fromValue(monType)));
    this->overlayType = -1;
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
    this->overlayType = -1;
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
    this->overlayType = -1;
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
    this->ui->monsterLineEdit->setText(QString::number(this->currentMonsterType.first));
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
