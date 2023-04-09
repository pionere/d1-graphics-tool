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

BuilderWidget::BuilderWidget(QWidget *parent, QUndoStack *us, D1Dun *d, LevelCelView *lcv)
    : QFrame(parent)
    , ui(new Ui::BuilderWidget())
    , undoStack(us)
    , dun(d)
    , levelCelView(lcv)
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
    this->graphView->setCursor(Qt::CrossCursor);
    // update the view
    this->dunResourcesModified();
    this->colorModified();
}

void BuilderWidget::hide()
{
    this->stopMove();
    this->graphView->unsetCursor();

    QFrame::hide();
}

bool BuilderWidget::dunClicked(int cellX, int cellY, bool first)
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
        int fcx = cellX;
        int lcx = this->lastPos.x();
        int fcy = cellY;
        int lcy = this->lastPos.y();

        if (fcx == lcx && fcy == lcy) {
            return true;
        }

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
        this->lastPos.setX(cellX);
        this->lastPos.setY(cellY);
        // collect locations
        modValues.push_back(DunPos(cellX, cellY, 0));

        // reset value if it is the same as before
        switch (this->mode) {
        case BEM_TILE:
            if (value == this->dun->getTileAt(cellX, cellY)) {
                value = 0;
            }
            break;
        case BEM_TILE_PROTECTION:
            if (value == (int)this->dun->getTileProtectionAt(cellX, cellY)) {
                value = (int)Qt::Unchecked;
            }
            break;
        case BEM_SUBTILE:
            if (value == this->dun->getSubtileAt(cellX, cellY)) {
                value = 0;
            }
            break;
        case BEM_SUBTILE_PROTECTION:
            if (value == (int)this->dun->getSubtileProtectionAt(cellX, cellY)) {
                value = 0;
            }
            break;
        case BEM_OBJECT:
            if (value == this->dun->getObjectAt(cellX, cellY)) {
                value = 0;
            }
            break;
        case BEM_MONSTER:
            if (this->currentMonsterType == this->dun->getMonsterAt(cellX, cellY)) {
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

void BuilderWidget::dunResourcesModified()
{
    if (this->isHidden())
        return;

    QComboBox *comboBox;

    // prepare the comboboxes
    // - objects
    comboBox = this->ui->objectComboBox;
    comboBox->hide();
    comboBox->clear();
    const QComboBox *objSrc = this->levelCelView->getObjects();
    for (int i = 0; i < objSrc->count(); i++) {
        comboBox->addItem(objSrc->itemText(i), objSrc->itemData(i));
    }
    comboBox->setCurrentIndex(comboBox->findData(this->currentObjectIndex));
    comboBox->show();

    // - monsters
    comboBox = this->ui->monsterComboBox;
    comboBox->hide();
    comboBox->clear();
    const QComboBox *monSrc = this->levelCelView->getMonsters();
    for (int i = 0; i < monSrc->count(); i++) {
        comboBox->addItem(monSrc->itemText(i), monSrc->itemData(i));
    }
    comboBox->setCurrentIndex(comboBox->findData(QVariant::fromValue(this->currentMonsterType)));
    comboBox->show();
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
        QPoint destPos = this->mapFromGlobal(targetPos);
        QMessageBox::critical(nullptr, "Error", QString("Moving v %1:%2 vr %3:%4 s%5:%6 tp %7:%8 dp %9%10").arg(viewSize.width()).arg(viewSize.height()).arg(viewBottomRight.x()).arg(viewBottomRight.y()).arg(mySize.width()).arg(mySize.height()).arg(destPos.x()).arg(destPos.y()));
        this->move(destPos);
    }
}

void BuilderWidget::stopMove()
{
    this->setMouseTracking(false);
    this->moving = false;
    this->moved = true;
    this->releaseMouse();
    // this->setCursor(Qt::CrossCursor);
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
}

void BuilderWidget::setTileIndex(int tileIndex)
{
    this->currentTileIndex = tileIndex;
    this->ui->tileLineEdit->setText(QString::number(tileIndex));
}

void BuilderWidget::setSubtileIndex(int subtileIndex)
{
    this->currentSubtileIndex = subtileIndex;
    this->ui->subtileLineEdit->setText(QString::number(subtileIndex));
}

void BuilderWidget::setObjectIndex(int objectIndex)
{
    this->currentObjectIndex = objectIndex;
    this->ui->objectLineEdit->setText(QString::number(objectIndex));
}

void BuilderWidget::setMonsterType(DunMonsterType monType)
{
    this->currentMonsterType = monType;
    this->ui->monsterLineEdit->setText(QString::number(monType.first));
    this->ui->monsterCheckBox->setChecked(monType.second);
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
    DunMonsterType monType;
    monType.first = this->ui->monsterLineEdit->text().toInt();
    monType.second = this->ui->monsterCheckBox->isChecked();

    this->setMonsterType(monType);

    this->on_monsterLineEdit_escPressed();
}

void BuilderWidget::on_monsterLineEdit_escPressed()
{
    this->ui->monsterLineEdit->setText(QString::number(this->currentMonsterType.first));
    this->ui->monsterLineEdit->clearFocus();
}

void BuilderWidget::on_monsterComboBox_activated(int index)
{
    if (index < 0) {
        return;
    }
    DunMonsterType monType = this->ui->monsterComboBox->itemData(index).value<DunMonsterType>();

    this->setMonsterType(monType);
}
