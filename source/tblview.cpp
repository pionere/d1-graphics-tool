#include "tblview.h"

#include <algorithm>

#include <QAction>
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
#include "ui_tblview.h"

#include "dungeon/all.h"

TableValue::TableValue(int x, int y, int v)
    : tblX(x)
    , tblY(y)
    , value(v)
{
}

EditTableCommand::EditTableCommand(D1Tbl *t, const std::vector<TableValue> &values)
    : QUndoCommand(nullptr)
    , table(t)
    , modValues(values)
{
}

void EditTableCommand::undo()
{
    if (this->table.isNull()) {
        this->setObsolete(true);
        return;
    }
    int num = 0;
    for (TableValue &tv : this->modValues) {
        int value = D1Tbl::getDarkValueAt(tv.tblX, tv.tblY);
        if (value != tv.value) {
            this->table->setDarkValueAt(tv.tblX, tv.tblY, tv.value);
            tv.value = value;
            num++;
        }
    }

    emit this->modified();
}

void EditTableCommand::redo()
{
    this->undo();
}

TblView::TblView(QWidget *parent, QUndoStack *us)
    : QWidget(parent)
    , ui(new Ui::TblView())
    , undoStack(us)
{
    this->ui->setupUi(this);
    this->ui->tblGraphicsView->setScene(&this->tblScene);
    this->on_radiusLineEdit_escPressed();
    this->on_offsetXYLineEdit_escPressed();
    this->on_zoomEdit_escPressed();
    this->on_playDelayEdit_escPressed();

    // If a pixel of the frame was clicked get pixel color index and notify the palette widgets
    // QObject::connect(&this->tblScene, &CelScene::framePixelClicked, this, &TblView::framePixelClicked);
    // QObject::connect(&this->tblScene, &CelScene::framePixelHovered, this, &TblView::framePixelHovered);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->radiusLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_radiusLineEdit_escPressed()));
    QObject::connect(this->ui->offsetXYLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_offsetXYLineEdit_escPressed()));
    QObject::connect(this->ui->zoomEdit, SIGNAL(cancel_signal()), this, SLOT(on_zoomEdit_escPressed()));
    QObject::connect(this->ui->playDelayEdit, SIGNAL(cancel_signal()), this, SLOT(on_playDelayEdit_escPressed()));
}

TblView::~TblView()
{
    delete ui;
}

void TblView::initialize(D1Pal *p, D1Tableset *t, bool bottomPanelHidden)
{
    this->pal = p;
    this->tableset = t;

    this->ui->bottomPanel->setVisible(!bottomPanelHidden);

    // this->update();
}

void TblView::setPal(D1Pal *p)
{
    this->pal = p;
}

// Displaying CEL file path information
void TblView::updateLabel()
{
    QFileInfo distTblFileInfo(this->tableset->distTbl->getFilePath());
    QFileInfo darkTblFileInfo(this->tableset->darkTbl->getFilePath());

    QString label = distTblFileInfo.fileName();
    if (this->tableset->distTbl->isModified()) {
        label += "*";
    }
    label += ", ";
    label += darkTblFileInfo.fileName();
    if (this->tableset->darkTbl->isModified()) {
        label += "*";
    }

    ui->tblLabel->setText(label);
}

void TblView::update()
{
    this->updateLabel();

    // Set current radius
    this->ui->radiusLineEdit->setText(QString::number(this->currentLightRadius));
    // Set current offset
    this->ui->offsetXYLineEdit->setText(QString("%1:%2").arg(this->currentXOffset).arg(this->currentYOffset));
}

void TblView::framePixelClicked(const QPoint &pos, bool first)
{
    QRect darkImageRect = QRect(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN + std::max(D1Tbl::getTableImageHeight(), D1Tbl::getLightImageHeight() + CEL_SCENE_SPACING + D1Tbl::getLumImageHeight()) + CEL_SCENE_SPACING, D1Tbl::getDarkImageWidth(), D1Tbl::getDarkImageHeight());
    darkImageRect.adjust(4, 4, -4, -4); // DARK_BORDER_WIDTH

    if (first) {
        this->lastPos = this->firstPos = pos;
        return;
    }

    if (darkImageRect.contains(this->firstPos)) {
        int firstValue = this->firstPos.y() - (darkImageRect.y() + 4); // DARK_BORDER_WIDTH
        if (firstValue < 0) {
            return; // out of the table -> skip
        }
        firstValue /= 32; // DARK_COLUMN_HEIGHT_UNIT
        firstValue = MAXDARKNESS - firstValue;
        if (firstValue < 0) {
            return; // out of the table -> skip
        }

        int lastValue = this->lastPos.y() - (darkImageRect.y() + 4); // DARK_BORDER_WIDTH
        if (lastValue < 0) {
            lastValue = 0;
        }
        lastValue /= 32; // DARK_COLUMN_HEIGHT_UNIT
        lastValue = MAXDARKNESS - lastValue;
        if (lastValue < 0) {
            lastValue = 0;
        }

        int value = pos.y() - (darkImageRect.y() + 4); // DARK_BORDER_WIDTH
        if (value < 0) {
            value = 0;
        }
        value /= 32; // DARK_COLUMN_HEIGHT_UNIT
        value = MAXDARKNESS - value;
        if (value < 0) {
            value = 0;
        }

        if (lastValue == value) {
            return; // same value as before -> skip
        }

        this->lastPos = pos;

        if (firstValue != lastValue) {
            // rollback previous change
            this->undoStack->undo();
        }

        int deltaValue = value - firstValue;
        if (deltaValue == 0) {
            return;
        }

        QPoint valuePos = pos - darkImageRect.topLeft();
        value = D1Tbl::getDarkValueAt(valuePos.x(), this->currentLightRadius) + deltaValue;
        if (value < 0) {
            value = 0;
        }
        if (value > MAXDARKNESS) {
            value = MAXDARKNESS;
        }

        std::vector<TableValue> modValues;
        if (deltaValue < 0) {
            for (int i = valuePos.x(); i < darkImageRect.width(); i += 8) { // DARK_COLUMN_WIDTH
                int v = D1Tbl::getDarkValueAt(i, this->currentLightRadius);
                if (v >= value) {
                    modValues.push_back(TableValue(i, this->currentLightRadius, value));
                } else {
                    break;
                }
            }
        } else {
            for (int i = valuePos.x(); i >= 0; i -= 8) { // DARK_COLUMN_WIDTH
                int v = D1Tbl::getDarkValueAt(i, this->currentLightRadius);
                if (v <= value) {
                    modValues.push_back(TableValue(i, this->currentLightRadius, value));
                } else {
                    break;
                }
            }
        }

        // Build frame editing command and connect it to the current main window widget
        // to update the palHits and CEL views when undo/redo is performed
        EditTableCommand *command = new EditTableCommand(this->tableset->darkTbl, modValues);
        QObject::connect(command, &EditTableCommand::modified, this, &TblView::displayFrame);

        this->undoStack->push(command);
        return;
    }
}

void TblView::framePixelHovered(const QPoint &pos)
{
    QRect tableImageRect = QRect(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN, D1Tbl::getTableImageWidth(), D1Tbl::getTableImageHeight());
    QRect lightImageRect = QRect(CEL_SCENE_MARGIN + D1Tbl::getTableImageWidth() + CEL_SCENE_SPACING, CEL_SCENE_MARGIN, D1Tbl::getLightImageWidth(), D1Tbl::getLightImageHeight());
    lightImageRect.adjust(4, 4, -4, -4); // LIGHT_BORDER_WIDTH
    QRect lumImageRect = QRect(CEL_SCENE_MARGIN + D1Tbl::getTableImageWidth() + CEL_SCENE_SPACING, CEL_SCENE_MARGIN + D1Tbl::getLightImageHeight() + CEL_SCENE_SPACING, D1Tbl::getLumImageWidth(), D1Tbl::getLumImageHeight());
    lumImageRect.adjust(4, 4, -4, -4); // LUM_BORDER_WIDTH
    QRect darkImageRect = QRect(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN + std::max(D1Tbl::getTableImageHeight(), D1Tbl::getLightImageHeight() + CEL_SCENE_SPACING + D1Tbl::getLumImageHeight()) + CEL_SCENE_SPACING, D1Tbl::getDarkImageWidth(), D1Tbl::getDarkImageHeight());
    darkImageRect.adjust(4, 4, -4, -4); // DARK_BORDER_WIDTH

    if (tableImageRect.contains(pos)) {
        QPoint valuePos = pos - tableImageRect.topLeft();
        int value = D1Tbl::getTableValueAt(valuePos.x(), valuePos.y());
        this->ui->valueLineEdit->setText(QString::number(value));
        this->ui->valueAtLineEdit->setText(QString("%1:%2").arg(valuePos.x() / 32).arg(valuePos.y() / 32)); // TABLE_TILE_SIZE
        return;
    }
    if (lightImageRect.contains(pos)) {
        QPoint valuePos = pos - lightImageRect.topLeft();
        int value = D1Tbl::getLightValueAt(this->pal, valuePos.x(), this->currentColor);
        this->ui->valueLineEdit->setText(QString::number(value));
        this->ui->valueAtLineEdit->setText(QString::number(valuePos.x() / 32)); // LIGHT_COLUMN_WIDTH
        return;
    }
    if (lumImageRect.contains(pos)) {
        QPoint valuePos = pos - lumImageRect.topLeft();
        int value = D1Tbl::getLumValueAt(this->pal, valuePos.x(), this->currentColor);
        this->ui->valueLineEdit->setText(QString::number(value));
        this->ui->valueAtLineEdit->setText(QString::number(valuePos.x() / 32)); // LUM_COLUMN_WIDTH
        return;
    }
    if (darkImageRect.contains(pos)) {
        QPoint valuePos = pos - darkImageRect.topLeft();
        int value = D1Tbl::getDarkValueAt(valuePos.x(), this->currentLightRadius);
        this->ui->valueLineEdit->setText(QString::number(value));
        this->ui->valueAtLineEdit->setText(QString::number(valuePos.x() / 8)); // DARK_COLUMN_WIDTH
        return;
    }
}

void TblView::palColorsSelected(const std::vector<quint8> &indices)
{
    this->currentColor = indices.empty() ? 0 : indices[0];
    // update the view
    this->displayFrame();
}

void TblView::displayFrame()
{
    this->update();
    this->tblScene.clear();

    // Getting the current frame to display
    QImage tblFrame = this->tableset->darkTbl->getTableImage(this->pal, this->currentLightRadius, this->currentXOffset, this->currentYOffset, this->ui->levelTypeComboBox->currentIndex(), this->currentColor);
    QImage lightImage = D1Tbl::getLightImage(this->pal, this->currentColor);
    QImage lumImage = D1Tbl::getLumImage(this->pal, this->currentColor);
    QImage darkImage = D1Tbl::getDarkImage(this->currentLightRadius);

    this->tblScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    // Building background of the width/height of the CEL frame
    // QImage tblFrameBackground = QImage(tblFrame.width(), tblFrame.height(), QImage::Format_ARGB32);
    // tblFrameBackground.fill(QColor(Config::getGraphicsTransparentColor()));

    // Resize the scene rectangle to include some padding around the CEL frame
    this->tblScene.setSceneRect(0, 0,
        CEL_SCENE_MARGIN + std::max(D1Tbl::getTableImageWidth() + CEL_SCENE_SPACING + std::max(D1Tbl::getLightImageWidth(), D1Tbl::getLumImageWidth()), D1Tbl::getDarkImageWidth()) + CEL_SCENE_MARGIN,
        CEL_SCENE_MARGIN + std::max(D1Tbl::getTableImageHeight(), D1Tbl::getLightImageHeight() + CEL_SCENE_SPACING + D1Tbl::getLumImageHeight()) + CEL_SCENE_SPACING + D1Tbl::getDarkImageHeight() + CEL_SCENE_MARGIN);
    // ui->celGraphicsView->adjustSize();

    // Add the backgrond and CEL frame while aligning it in the center
    // this->tblScene.addPixmap(QPixmap::fromImage(tblFrameBackground))
    //    ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    QGraphicsPixmapItem *item;
    // add table frame
    item = this->tblScene.addPixmap(QPixmap::fromImage(tblFrame));
    item->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    item->setAcceptHoverEvents(true);
    // add light-plot frame
    item = this->tblScene.addPixmap(QPixmap::fromImage(lightImage));
    item->setPos(CEL_SCENE_MARGIN + D1Tbl::getTableImageWidth() + CEL_SCENE_SPACING, CEL_SCENE_MARGIN);
    item->setAcceptHoverEvents(true);
    // add lum-plot frame
    item = this->tblScene.addPixmap(QPixmap::fromImage(lumImage));
    item->setPos(CEL_SCENE_MARGIN + D1Tbl::getTableImageWidth() + CEL_SCENE_SPACING, CEL_SCENE_MARGIN + D1Tbl::getLightImageHeight() + CEL_SCENE_SPACING);
    item->setAcceptHoverEvents(true);
    // add darkness equalizer frame
    item = this->tblScene.addPixmap(QPixmap::fromImage(darkImage));
    item->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN + std::max(D1Tbl::getTableImageHeight(), D1Tbl::getLightImageHeight() + CEL_SCENE_SPACING + D1Tbl::getLumImageHeight()) + CEL_SCENE_SPACING);
    item->setAcceptHoverEvents(true);

    // Notify PalView that the frame changed (used to refresh palette widget)
    emit this->frameRefreshed();
}

void TblView::toggleBottomPanel()
{
    this->ui->bottomPanel->setVisible(this->ui->bottomPanel->isHidden());
}

void TblView::on_levelTypeComboBox_activated(int index)
{
    // update the view
    this->displayFrame();
}

void TblView::setRadius(int nextRadius)
{
    if (nextRadius < 0) {
        nextRadius = 0;
    }
    if (nextRadius > MAX_LIGHT_RAD) {
        nextRadius = MAX_LIGHT_RAD;
    }
    this->currentLightRadius = nextRadius;
    // update the view
    this->displayFrame();
}

void TblView::on_radiusDecButton_clicked()
{
    this->setRadius(this->currentLightRadius - 1);
}

void TblView::on_radiusIncButton_clicked()
{
    this->setRadius(this->currentLightRadius + 1);
}

void TblView::on_radiusLineEdit_returnPressed()
{
    int nextRadius = this->ui->radiusLineEdit->text().toInt();
    this->setRadius(nextRadius);
    this->on_radiusLineEdit_escPressed();
}

void TblView::on_radiusLineEdit_escPressed()
{
    this->ui->radiusLineEdit->setText(QString::number(this->currentLightRadius));
    this->ui->radiusLineEdit->clearFocus();
}

void TblView::setOffset(int xoff, int yoff)
{
    if (xoff < -MAX_OFFSET) {
        xoff = -MAX_OFFSET;
    }
    if (xoff > MAX_OFFSET) {
        xoff = MAX_OFFSET;
    }
    if (yoff < -MAX_OFFSET) {
        yoff = -MAX_OFFSET;
    }
    if (yoff > MAX_OFFSET) {
        yoff = MAX_OFFSET;
    }
    this->currentXOffset = xoff;
    this->currentYOffset = yoff;
    // update the view
    this->displayFrame();
}

void TblView::on_moveNWButton_clicked()
{
    this->setOffset(this->currentXOffset - 1, this->currentYOffset - 1);
}

void TblView::on_moveNButton_clicked()
{
    this->setOffset(this->currentXOffset, this->currentYOffset - 1);
}

void TblView::on_moveNEButton_clicked()
{
    this->setOffset(this->currentXOffset + 1, this->currentYOffset - 1);
}

void TblView::on_moveWButton_clicked()
{
    this->setOffset(this->currentXOffset - 1, this->currentYOffset);
}

void TblView::on_moveEButton_clicked()
{
    this->setOffset(this->currentXOffset + 1, this->currentYOffset);
}

void TblView::on_moveSWButton_clicked()
{
    this->setOffset(this->currentXOffset - 1, this->currentYOffset + 1);
}

void TblView::on_moveSButton_clicked()
{
    this->setOffset(this->currentXOffset, this->currentYOffset + 1);
}

void TblView::on_moveSEButton_clicked()
{
    this->setOffset(this->currentXOffset + 1, this->currentYOffset + 1);
}

void TblView::on_offsetXYLineEdit_returnPressed()
{
    QString offset = this->ui->offsetXYLineEdit->text();
    int sepIdx = offset.indexOf(":");
    int xoff, yoff;

    if (sepIdx >= 0) {
        if (sepIdx == 0) {
            xoff = 0;
            yoff = offset.mid(1).toInt();
        } else if (sepIdx == offset.length() - 1) {
            offset.chop(1);
            xoff = offset.toInt();
            yoff = 0;
        } else {
            xoff = offset.mid(0, sepIdx).toInt();
            yoff = offset.mid(sepIdx + 1).toInt();
        }
    } else {
        xoff = offset.toInt();
        yoff = 0;
    }
    this->setOffset(xoff, yoff);
    this->on_offsetXYLineEdit_escPressed();
}

void TblView::on_offsetXYLineEdit_escPressed()
{
    this->ui->offsetXYLineEdit->setText(QString("%1:%2").arg(this->currentXOffset).arg(this->currentYOffset));
    this->ui->offsetXYLineEdit->clearFocus();
}

void TblView::on_zoomOutButton_clicked()
{
    this->tblScene.zoomOut();
    this->on_zoomEdit_escPressed();
}

void TblView::on_zoomInButton_clicked()
{
    this->tblScene.zoomIn();
    this->on_zoomEdit_escPressed();
}

void TblView::on_zoomEdit_returnPressed()
{
    QString zoom = this->ui->zoomEdit->text();

    this->tblScene.setZoom(zoom);

    this->on_zoomEdit_escPressed();
}

void TblView::on_zoomEdit_escPressed()
{
    this->ui->zoomEdit->setText(this->tblScene.zoomText());
    this->ui->zoomEdit->clearFocus();
}

void TblView::on_playDelayEdit_returnPressed()
{
    quint16 playDelay = this->ui->playDelayEdit->text().toUInt();

    if (playDelay != 0)
        this->currentPlayDelay = playDelay;

    this->on_playDelayEdit_escPressed();
}

void TblView::on_playDelayEdit_escPressed()
{
    this->ui->playDelayEdit->setText(QString::number(this->currentPlayDelay));
    this->ui->playDelayEdit->clearFocus();
}

void TblView::on_playStopButton_clicked()
{
    if (this->playTimer != 0) {
        this->killTimer(this->playTimer);
        this->playTimer = 0;

        // restore palette
        dMainWindow().resetPaletteCycle();
        // change the label of the button
        this->ui->playStopButton->setText(tr("Play"));
        // enable the related fields
        this->ui->playDelayEdit->setReadOnly(false);
        this->ui->playComboBox->setEnabled(true);
        return;
    }
    // disable the related fields
    this->ui->playDelayEdit->setReadOnly(true);
    this->ui->playComboBox->setEnabled(false);
    // change the label of the button
    this->ui->playStopButton->setText(tr("Stop"));
    // preserve the palette
    dMainWindow().initPaletteCycle();

    this->playTimer = this->startTimer(this->currentPlayDelay);
}

void TblView::timerEvent(QTimerEvent *event)
{
    int cycleType = this->ui->playComboBox->currentIndex();
    if (cycleType == 0) {
        // normal playback
        this->displayFrame();
    } else {
        dMainWindow().nextPaletteCycle((D1PAL_CYCLE_TYPE)(cycleType - 1));
        // this->displayFrame();
    }
}
