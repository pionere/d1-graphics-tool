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

TblView::TblView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TblView())
{
    this->ui->setupUi(this);
    this->ui->tblGraphicsView->setScene(&this->tblScene);
    // this->on_radiusLineEdit_escPressed();
    this->on_zoomEdit_escPressed();
    this->on_playDelayEdit_escPressed();
    this->ui->stopButton->setEnabled(false);

    // If a pixel of the frame was clicked get pixel color index and notify the palette widgets
    // QObject::connect(&this->tblScene, &CelScene::framePixelClicked, this, &TblView::framePixelClicked);
    // QObject::connect(&this->tblScene, &CelScene::framePixelHovered, this, &TblView::framePixelHovered);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->radiusLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_radiusLineEdit_escPressed()));
    QObject::connect(this->ui->zoomEdit, SIGNAL(cancel_signal()), this, SLOT(on_zoomEdit_escPressed()));
    QObject::connect(this->ui->playDelayEdit, SIGNAL(cancel_signal()), this, SLOT(on_playDelayEdit_escPressed()));
}

TblView::~TblView()
{
    delete ui;
}

void TblView::initialize(D1Pal *p, D1Tbl *t, bool bottomPanelHidden)
{
    this->pal = p;
    this->tbl = t;

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
    QFileInfo tblFileInfo(this->tbl->getFilePath());
    QString label = tblFileInfo.fileName();
    if (this->tbl->isModified()) {
        label += "*";
    }
    ui->tblLabel->setText(label);
}

void TblView::update()
{
    this->updateLabel();

    // Set current radius
    this->ui->radiusLineEdit->setText(QString::number(this->currentLightRadius));
}

void TblView::framePixelClicked(const QPoint &pos, bool first)
{
    /*if (this->gfx->getFrameCount() == 0) {
        return;
    }
    D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
    QPoint p = pos;
    p -= QPoint(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    dMainWindow().frameClicked(frame, p, first);*/
}

void TblView::framePixelHovered(const QPoint &pos)
{
    int tableImageWidth = D1Tbl::getTableImageWidth();
    int tableImageHeight = D1Tbl::getTableImageHeight();
    int darkImageWidth = D1Tbl::getDarkImageWidth();
    int darkImageHeight = D1Tbl::getDarkImageHeight();

    if (pos.x() < CEL_SCENE_MARGIN || pos.y() < CEL_SCENE_MARGIN) {
        return;
    }
    if (pos.y() < CEL_SCENE_MARGIN + tableImageHeight) {
        if (pos.x() >= tableImageWidth) {
            return;
        }

        QPoint valuePos = pos - QPoint(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
        int value = this->tbl->getTableValueAt(valuePos.x(), valuePos.y());
        this->ui->valueLineEdit->setText(QString::number(value));
    } else if (pos.y() >= CEL_SCENE_MARGIN + tableImageHeight + CEL_SCENE_SPACING && pos.y() < CEL_SCENE_MARGIN + tableImageHeight + CEL_SCENE_SPACING + darkImageHeight) {
        if (pos.x() >= darkImageWidth) {
            return;
        }

        QPoint valuePos = pos - QPoint(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN + tableImageHeight + CEL_SCENE_SPACING);
        int value = this->tbl->getDarkValueAt(valuePos.x(), this->currentLightRadius);
        this->ui->valueLineEdit->setText(QString::number(value));
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
    QImage tblFrame = this->tbl->getTableImage(this->currentLightRadius, this->ui->levelTypeComboBox->currentIndex(), this->currentColor);
    QImage lightImage = this->tbl->getLightImage(this->currentColor);
    QImage darkImage = this->tbl->getDarkImage(this->currentLightRadius);

    this->tblScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    // Building background of the width/height of the CEL frame
    // QImage tblFrameBackground = QImage(tblFrame.width(), tblFrame.height(), QImage::Format_ARGB32);
    // tblFrameBackground.fill(QColor(Config::getGraphicsTransparentColor()));

    // Resize the scene rectangle to include some padding around the CEL frame
    this->tblScene.setSceneRect(0, 0,
        CEL_SCENE_MARGIN + std::max(D1Tbl::getTableImageWidth() + CEL_SCENE_SPACING + D1Tbl::getLightImageWidth(), D1Tbl::getDarkImageWidth()) + CEL_SCENE_MARGIN,
        CEL_SCENE_MARGIN + D1Tbl::getTableImageHeight() + CEL_SCENE_SPACING + D1Tbl::getDarkImageHeight() + CEL_SCENE_MARGIN);
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
    // add darkness equalizer frame
    item = this->tblScene.addPixmap(QPixmap::fromImage(darkImage))
    item->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN + D1Tbl::getTableImageHeight() + CEL_SCENE_SPACING);
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
    // update radiusLineEdit
    this->update();
    this->ui->radiusLineEdit->clearFocus();
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

void TblView::on_playButton_clicked()
{
    if (this->playTimer != 0) {
        return;
    }
    // disable the related fields
    this->ui->playButton->setEnabled(false);
    this->ui->playDelayEdit->setReadOnly(true);
    this->ui->playComboBox->setEnabled(false);
    // enable the stop button
    this->ui->stopButton->setEnabled(true);
    // preserve the palette
    dMainWindow().initPaletteCycle();

    this->playTimer = this->startTimer(this->currentPlayDelay);
}

void TblView::on_stopButton_clicked()
{
    this->killTimer(this->playTimer);
    this->playTimer = 0;

    // restore palette
    dMainWindow().resetPaletteCycle();
    // disable the stop button
    this->ui->stopButton->setEnabled(false);
    // enable the related fields
    this->ui->playButton->setEnabled(true);
    this->ui->playDelayEdit->setReadOnly(false);
    this->ui->playComboBox->setEnabled(true);
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
