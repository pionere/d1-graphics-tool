#include "levelcelview.h"

#include <algorithm>
#include <set>

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
#include "ui_levelcelview.h"
#include "upscaler.h"

LevelCelView::LevelCelView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LevelCelView())
{
    this->ui->setupUi(this);
    this->ui->celGraphicsView->setScene(&this->celScene);
    this->on_zoomEdit_escPressed();
    this->on_playDelayEdit_escPressed();
    this->ui->stopButton->setEnabled(false);
    this->playTimer.connect(&this->playTimer, SIGNAL(timeout()), this, SLOT(playGroup()));
    this->ui->tilesTabs->addTab(&this->tabTileWidget, tr("Tile properties"));
    this->ui->tilesTabs->addTab(&this->tabSubTileWidget, tr("Subtile properties"));
    this->ui->tilesTabs->addTab(&this->tabFrameWidget, tr("Frame properties"));

    // If a pixel of the frame, subtile or tile was clicked get pixel color index and notify the palette widgets
    QObject::connect(&this->celScene, &CelScene::framePixelClicked, this, &LevelCelView::framePixelClicked);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->frameIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_frameIndexEdit_escPressed()));
    QObject::connect(this->ui->subtileIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_subtileIndexEdit_escPressed()));
    QObject::connect(this->ui->tileIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_tileIndexEdit_escPressed()));
    QObject::connect(this->ui->minFrameWidthEdit, SIGNAL(cancel_signal()), this, SLOT(on_minFrameWidthEdit_escPressed()));
    QObject::connect(this->ui->minFrameHeightEdit, SIGNAL(cancel_signal()), this, SLOT(on_minFrameHeightEdit_escPressed()));
    QObject::connect(this->ui->zoomEdit, SIGNAL(cancel_signal()), this, SLOT(on_zoomEdit_escPressed()));
    QObject::connect(this->ui->playDelayEdit, SIGNAL(cancel_signal()), this, SLOT(on_playDelayEdit_escPressed()));

    // setup context menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));
    QObject::connect(&this->celScene, &CelScene::showContextMenu, this, &LevelCelView::ShowContextMenu);

    setAcceptDrops(true);
}

LevelCelView::~LevelCelView()
{
    delete ui;
}

void LevelCelView::initialize(D1Pal *p, D1Gfx *g, D1Min *m, D1Til *t, D1Sol *s, D1Amp *a, D1Tmi *i)
{
    this->pal = p;
    this->gfx = g;
    this->min = m;
    this->til = t;
    this->sol = s;
    this->amp = a;
    this->tmi = i;

    this->tabTileWidget.initialize(this, this->til, this->min, this->amp);
    this->tabSubTileWidget.initialize(this, this->gfx, this->min, this->sol, this->tmi);
    this->tabFrameWidget.initialize(this, this->gfx);

    this->update();
}

void LevelCelView::setPal(D1Pal *p)
{
    this->pal = p;
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

    this->ui->celLabel->setText(label);
}

void LevelCelView::update()
{
    this->updateLabel();

    ui->frameNumberEdit->setText(
        QString::number(this->gfx->getFrameCount()));

    ui->subtileNumberEdit->setText(
        QString::number(this->min->getSubtileCount()));

    ui->tileNumberEdit->setText(
        QString::number(this->til->getTileCount()));

    this->tabTileWidget.update();
    this->tabSubTileWidget.update();
    this->tabFrameWidget.update();
}

int LevelCelView::getCurrentFrameIndex()
{
    return this->currentFrameIndex;
}

int LevelCelView::getCurrentSubtileIndex()
{
    return this->currentSubtileIndex;
}

int LevelCelView::getCurrentTileIndex()
{
    return this->currentTileIndex;
}

void LevelCelView::changeColor(quint8 startColorIndex, quint8 endColorIndex, D1GfxPixel pixel, bool all)
{
    if (all || this->gfx->getFrameCount() == 0) {
        for (int i = 0; i < this->gfx->getFrameCount(); i++) {
            D1GfxFrame *frame = this->gfx->getFrame(i);
            frame->replacePixels(startColorIndex, endColorIndex, pixel);
        }
    } else {
        D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
        frame->replacePixels(startColorIndex, endColorIndex, pixel);
    }
    this->gfx->setModified();
    // update the view
    this->displayFrame();
}

void LevelCelView::framePixelClicked(unsigned x, unsigned y, unsigned counter)
{
    unsigned celFrameWidth = MICRO_WIDTH; // this->gfx->getFrameWidth(this->currentFrameIndex);
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned tileWidth = subtileWidth * TILE_WIDTH;

    unsigned celFrameHeight = MICRO_HEIGHT; // this->gfx->getFrameHeight(this->currentFrameIndex);
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;
    unsigned subtileShiftY = subtileWidth / 4;
    unsigned tileHeight = subtileHeight + 2 * subtileShiftY;

    if (x >= (celFrameWidth + CEL_SCENE_SPACING * 2)
        && x < (celFrameWidth + subtileWidth + CEL_SCENE_SPACING * 2)
        && y >= CEL_SCENE_SPACING
        && y < (subtileHeight + CEL_SCENE_SPACING)
        && this->min->getSubtileCount() != 0) {
        // When a CEL frame is clicked in the subtile, display the corresponding CEL frame

        // Adjust coordinates
        unsigned stx = x - (celFrameWidth + CEL_SCENE_SPACING * 2);
        unsigned sty = y - CEL_SCENE_SPACING;

        // qDebug() << "Subtile clicked: " << stx << "," << sty;

        stx /= MICRO_WIDTH;
        sty /= MICRO_HEIGHT;

        int stFrame = sty * subtileWidth / MICRO_WIDTH + stx;
        QList<quint16> &minFrames = this->min->getFrameReferences(this->currentSubtileIndex);
        quint16 frameRef = 0;
        if (minFrames.count() > stFrame) {
            frameRef = minFrames.at(stFrame);
            this->tabSubTileWidget.selectFrame(stFrame);
        }

        if (frameRef > 0) {
            this->currentFrameIndex = frameRef - 1;
            this->displayFrame();
        }
        // highlight selection
        QColor borderColor = QColor(Config::getPaletteSelectionBorderColor());
        QPen pen(borderColor);
        pen.setWidth(PALETTE_SELECTION_WIDTH);
        QRectF coordinates = QRectF(CEL_SCENE_SPACING + celFrameWidth + CEL_SCENE_SPACING + stx * MICRO_WIDTH, CEL_SCENE_SPACING + sty * MICRO_HEIGHT, MICRO_WIDTH, MICRO_HEIGHT);
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
    } else if (x >= (celFrameWidth + subtileWidth + CEL_SCENE_SPACING * 3)
        && x < (celFrameWidth + subtileWidth + tileWidth + CEL_SCENE_SPACING * 3)
        && y >= CEL_SCENE_SPACING
        && y < (tileHeight + CEL_SCENE_SPACING)
        && this->til->getTileCount() != 0) {
        // When a subtile is clicked in the tile, display the corresponding subtile

        // Adjust coordinates
        unsigned tx = x - (celFrameWidth + subtileWidth + CEL_SCENE_SPACING * 3);
        unsigned ty = y - CEL_SCENE_SPACING;

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
        int tSubtile = 0;
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

        QList<quint16> &tilSubtiles = this->til->getSubtileIndices(this->currentTileIndex);
        if (tilSubtiles.count() > tSubtile) {
            this->currentSubtileIndex = tilSubtiles.at(tSubtile);
            this->tabTileWidget.selectSubtile(tSubtile);
            this->displayFrame();
        }
        return;
    }
    // otherwise emit frame-click event
    D1GfxFrame *frame = nullptr;
    if (this->gfx->getFrameCount() != 0) {
        frame = this->gfx->getFrame(this->currentFrameIndex);
    }
    emit this->frameClicked(frame, x - CEL_SCENE_SPACING, y - CEL_SCENE_SPACING, counter);
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
    QList<quint16> frameReferencesList;

    // TODO: merge with LevelCelView::insertSubtile ?
    QImage subImage = QImage(MICRO_WIDTH, MICRO_HEIGHT, QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); y += MICRO_HEIGHT) {
        for (int x = 0; x < image.width(); x += MICRO_WIDTH) {
            // subImage.fill(Qt::transparent);

            bool hasColor = false;
            for (int j = 0; j < MICRO_HEIGHT; j++) {
                for (int i = 0; i < MICRO_WIDTH; i++) {
                    const QColor color = image.pixelColor(x + i, y + j);
                    if (color.alpha() >= COLOR_ALPHA_LIMIT) {
                        hasColor = true;
                    }
                    subImage.setPixelColor(i, j, color);
                }
            }
            frameReferencesList.append(hasColor ? frameIndex + 1 : 0);
            if (!hasColor) {
                continue;
            }

            D1GfxFrame *frame = this->gfx->insertFrame(frameIndex, subImage);
            LevelTabFrameWidget::selectFrameType(frame);
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
    QList<quint16> frameReferencesList;

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
            frameReferencesList.append(hasColor ? frameIndex + 1 : 0);
            if (!hasColor) {
                continue;
            }

            bool clipped;
            D1GfxFrame *subFrame = this->gfx->insertFrame(frameIndex, &clipped);
            for (int j = 0; j < MICRO_HEIGHT; j++) {
                QList<D1GfxPixel> pixelLine;
                for (int i = 0; i < MICRO_WIDTH; i++) {
                    pixelLine.append(frame.getPixel(x + i, y + j));
                }
                subFrame->addPixelLine(pixelLine);
            }
            LevelTabFrameWidget::selectFrameType(subFrame);
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
            dProgressFail() << tr("Failed to load file: %1.").arg(imagefilePath);
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
        QString msg = tr("Failed to read file: %1.").arg(imagefilePath);
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
            QList<quint16> &frameReferences = this->min->getFrameReferences(i);
            for (int n = 0; n < frameReferences.count(); n++) {
                if (frameReferences[n] >= frameRef) {
                    frameReferences[n] += deltaFrameCount;
                }
            }
        }
    }
    // update the view
    this->displayFrame();
}

void LevelCelView::assignSubtiles(const QImage &image, int tileIndex, int subtileIndex)
{
    QList<quint16> *subtileIndices = nullptr;
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
        for (int x = 0; x < image.width(); x += subtileWidth) {
            // subImage.fill(Qt::transparent);

            bool hasColor = false;
            for (unsigned j = 0; j < subtileHeight; j++) {
                for (unsigned i = 0; i < subtileWidth; i++) {
                    const QColor color = image.pixelColor(x + i, y + j);
                    if (color.alpha() >= COLOR_ALPHA_LIMIT) {
                        hasColor = true;
                    }
                    subImage.setPixelColor(i, j, color);
                }
            }

            if (subtileIndices != nullptr) {
                subtileIndices->append(subtileIndex);
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
    QList<quint16> *subtileIndices = nullptr;
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
                QList<D1GfxPixel> pixelLine;
                for (unsigned i = 0; i < subtileWidth; i++) {
                    D1GfxPixel pixel = frame.getPixel(x + i, y + j);
                    if (!pixel.isTransparent()) {
                        hasColor = true;
                    }
                    pixelLine.append(pixel);
                }
                subFrame.addPixelLine(pixelLine);
            }
            if (!hasColor) {
                continue;
            }

            if (subtileIndices != nullptr) {
                subtileIndices->append(subtileIndex);
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
            dProgressFail() << tr("Failed to load file: %1.").arg(imagefilePath);
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
        QString msg = tr("Failed to read file: %1.").arg(imagefilePath);
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
        unsigned refIndex = this->currentSubtileIndex;
        // shift subtile indices of the tiles
        for (int i = 0; i < this->til->getTileCount(); i++) {
            QList<quint16> &subtileIndices = this->til->getSubtileIndices(i);
            for (int n = 0; n < subtileIndices.count(); n++) {
                if (subtileIndices[n] >= refIndex) {
                    subtileIndices[n] += deltaSubtileCount;
                    this->til->setModified();
                }
            }
        }
    }
    // update the view
    this->displayFrame();
}

void LevelCelView::insertSubtile(int subtileIndex, const QImage &image)
{
    QList<quint16> frameReferencesList;

    int frameIndex = this->gfx->getFrameCount();
    QImage subImage = QImage(MICRO_WIDTH, MICRO_HEIGHT, QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); y += MICRO_HEIGHT) {
        for (int x = 0; x < image.width(); x += MICRO_WIDTH) {
            // subImage.fill(Qt::transparent);

            bool hasColor = false;
            for (int j = 0; j < MICRO_HEIGHT; j++) {
                for (int i = 0; i < MICRO_WIDTH; i++) {
                    const QColor color = image.pixelColor(x + i, y + j);
                    if (color.alpha() >= COLOR_ALPHA_LIMIT) {
                        hasColor = true;
                    }
                    subImage.setPixelColor(i, j, color);
                }
            }

            frameReferencesList.append(hasColor ? frameIndex + 1 : 0);

            if (!hasColor) {
                continue;
            }

            D1GfxFrame *frame = this->gfx->insertFrame(frameIndex, subImage);
            LevelTabFrameWidget::selectFrameType(frame);
            frameIndex++;
        }
    }
    this->min->insertSubtile(subtileIndex, frameReferencesList);
    this->sol->insertSubtile(subtileIndex, 0);
    this->tmi->insertSubtile(subtileIndex, 0);
}

void LevelCelView::insertSubtile(int subtileIndex, const D1GfxFrame &frame)
{
    QList<quint16> frameReferencesList;

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

            frameReferencesList.append(hasColor ? frameIndex + 1 : 0);

            if (!hasColor) {
                continue;
            }

            bool clipped;
            D1GfxFrame *subFrame = this->gfx->insertFrame(frameIndex, &clipped);
            for (int j = 0; j < MICRO_HEIGHT; j++) {
                QList<D1GfxPixel> pixelLine;
                for (int i = 0; i < MICRO_WIDTH; i++) {
                    pixelLine.append(frame.getPixel(x + i, y + j));
                }
                subFrame->addPixelLine(pixelLine);
            }
            LevelTabFrameWidget::selectFrameType(subFrame);
            frameIndex++;
        }
    }
    this->min->insertSubtile(subtileIndex, frameReferencesList);
    this->sol->insertSubtile(subtileIndex, 0);
    this->tmi->insertSubtile(subtileIndex, 0);
}

void LevelCelView::insertTile(int tileIndex, const QImage &image)
{
    QList<quint16> subtileIndices;

    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    QImage subImage = QImage(subtileWidth, subtileHeight, QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); y += subtileHeight) {
        for (int x = 0; x < image.width(); x += subtileWidth) {
            // subImage.fill(Qt::transparent);

            // bool hasColor = false;
            for (unsigned j = 0; j < subtileHeight; j++) {
                for (unsigned i = 0; i < subtileWidth; i++) {
                    const QColor color = image.pixelColor(x + i, y + j);
                    // if (color.alpha() >= COLOR_ALPHA_LIMIT) {
                    //    hasColor = true;
                    // }
                    subImage.setPixelColor(i, j, color);
                }
            }

            int index = this->min->getSubtileCount();
            subtileIndices.append(index);
            this->insertSubtile(index, subImage);
        }
    }

    this->til->insertTile(tileIndex, subtileIndices);
}

void LevelCelView::insertTile(int tileIndex, const D1GfxFrame &frame)
{
    QList<quint16> subtileIndices;

    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    for (int y = 0; y < frame.getHeight(); y += subtileHeight) {
        for (int x = 0; x < frame.getWidth(); x += subtileWidth) {
            D1GfxFrame subFrame;
            for (unsigned j = 0; j < subtileHeight; j++) {
                QList<D1GfxPixel> pixelLine;
                for (unsigned i = 0; i < subtileWidth; i++) {
                    pixelLine.append(frame.getPixel(x + i, y + j));
                }
                subFrame.addPixelLine(pixelLine);
            }

            int index = this->min->getSubtileCount();
            subtileIndices.append(index);
            this->insertSubtile(index, subFrame);
        }
    }

    this->til->insertTile(tileIndex, subtileIndices);
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
        for (int x = 0; x < image.width(); x += tileWidth) {
            // subImage.fill(Qt::transparent);

            bool hasColor = false;
            for (unsigned j = 0; j < tileHeight; j++) {
                for (unsigned i = 0; i < tileWidth; i++) {
                    const QColor color = image.pixelColor(x + i, y + j);
                    if (color.alpha() >= COLOR_ALPHA_LIMIT) {
                        hasColor = true;
                    }
                    subImage.setPixelColor(i, j, color);
                }
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
                QList<D1GfxPixel> pixelLine;
                for (unsigned i = 0; i < tileWidth; i++) {
                    D1GfxPixel pixel = frame.getPixel(x + i, y + j);
                    if (!pixel.isTransparent()) {
                        hasColor = true;
                    }
                    pixelLine.append(pixel);
                }
                subFrame.addPixelLine(pixelLine);
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
            dProgressFail() << tr("Failed to load file: %1.").arg(imagefilePath);
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
        QString msg = tr("Failed to read file: %1.").arg(imagefilePath);
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
    // update the view
    this->displayFrame();
}

void LevelCelView::addToCurrentFrame(const QString &imagefilePath)
{
    if (imagefilePath.toLower().endsWith(".pcx")) {
        bool clipped = false, palMod;
        D1GfxFrame frame;
        D1Pal basePal = D1Pal(*this->pal);
        bool success = D1Pcx::load(frame, imagefilePath, clipped, &basePal, this->gfx->getPalette(), &palMod);
        if (!success) {
            dProgressFail() << tr("Failed to load file: %1.").arg(imagefilePath);
            return;
        }
        D1GfxFrame *resFrame = this->gfx->addToFrame(this->currentFrameIndex, frame);
        if (resFrame == nullptr) {
            return; // error set by gfx->addToFrame
        }
        LevelTabFrameWidget::selectFrameType(resFrame);
        if (palMod) {
            // update the palette
            this->pal->updateColors(basePal);
            emit this->palModified();
        }
        // update the view
        this->displayFrame();
        return;
    }

    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        dProgressFail() << tr("Failed to read file: %1.").arg(imagefilePath);
        return;
    }

    D1GfxFrame *frame = this->gfx->addToFrame(this->currentFrameIndex, image);
    if (frame == nullptr) {
        return; // error set by gfx->addToFrame
    }

    LevelTabFrameWidget::selectFrameType(frame);
    // update the view
    this->displayFrame();
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
            dProgressFail() << tr("Failed to load file: %1.").arg(imagefilePath);
            return;
        }
        if (frame->getWidth() != MICRO_WIDTH || frame->getHeight() != MICRO_HEIGHT) {
            delete frame;
            dProgressFail() << tr("The image must be 32px * 32px to be used as a frame.");
            return;
        }
        LevelTabFrameWidget::selectFrameType(frame);
        this->gfx->setFrame(this->currentFrameIndex, frame);
        if (palMod) {
            // update the palette
            this->pal->updateColors(basePal);
            emit this->palModified();
        }
        // update the view
        this->displayFrame();
        return;
    }

    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        dProgressFail() << tr("Failed to read file: %1.").arg(imagefilePath);
        return;
    }

    if (image.width() != MICRO_WIDTH || image.height() != MICRO_HEIGHT) {
        dProgressFail() << tr("The image must be 32px * 32px to be used as a frame.");
        return;
    }

    D1GfxFrame *frame = this->gfx->replaceFrame(this->currentFrameIndex, image);
    LevelTabFrameWidget::selectFrameType(frame);
    // update the view
    this->displayFrame();
}

void LevelCelView::removeFrame(int frameIndex)
{
    // remove the frame
    this->gfx->removeFrame(frameIndex);
    if (this->gfx->getFrameCount() == this->currentFrameIndex) {
        this->currentFrameIndex = std::max(0, this->currentFrameIndex - 1);
    }
    // shift references
    // - shift frame indices of the subtiles
    unsigned refIndex = frameIndex + 1;
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        QList<quint16> &frameReferences = this->min->getFrameReferences(i);
        for (int n = 0; n < frameReferences.count(); n++) {
            if (frameReferences[n] >= refIndex) {
                if (frameReferences[n] == refIndex) {
                    frameReferences[n] = 0;
                } else {
                    frameReferences[n] -= 1;
                }
                this->min->setModified();
            }
        }
    }
}

void LevelCelView::removeCurrentFrame()
{
    // check if the current frame is used
    QList<int> frameUsers;

    this->collectFrameUsers(this->currentFrameIndex, frameUsers);

    if (!frameUsers.isEmpty()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, tr("Confirmation"), tr("The frame is used by subtile %1 (and maybe others). Are you sure you want to proceed?").arg(frameUsers.first() + 1), QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }
    // remove the current frame
    this->removeFrame(this->currentFrameIndex);
    // update the view
    this->displayFrame();
}

void LevelCelView::createSubtile()
{
    this->min->createSubtile();
    this->sol->createSubtile();
    this->tmi->createSubtile();
    // jump to the new subtile
    this->currentSubtileIndex = this->min->getSubtileCount() - 1;
    // update the view
    this->displayFrame();
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
            dProgressFail() << tr("Failed to load file: %1.").arg(imagefilePath);
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
        // update the view
        this->displayFrame();
        return;
    }

    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        dProgressFail() << tr("Failed to read file: %1.").arg(imagefilePath);
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

    // update the view
    this->displayFrame();
}

void LevelCelView::removeSubtile(int subtileIndex)
{
    this->min->removeSubtile(subtileIndex);
    this->sol->removeSubtile(subtileIndex);
    this->tmi->removeSubtile(subtileIndex);
    // update subtile index if necessary
    if (this->currentSubtileIndex == this->min->getSubtileCount()) {
        this->currentSubtileIndex = std::max(0, this->currentSubtileIndex - 1);
    }
    // shift references
    // - shift subtile indices of the tiles
    unsigned refIndex = subtileIndex;
    for (int i = 0; i < this->til->getTileCount(); i++) {
        QList<quint16> &subtileIndices = this->til->getSubtileIndices(i);
        for (int n = 0; n < subtileIndices.count(); n++) {
            if (subtileIndices[n] >= refIndex) {
                // assert(subtileIndices[n] != refIndex);
                subtileIndices[n] -= 1;
                this->til->setModified();
            }
        }
    }
}

void LevelCelView::removeCurrentSubtile()
{
    // check if the current subtile is used
    QList<int> subtileUsers;

    this->collectSubtileUsers(this->currentSubtileIndex, subtileUsers);

    if (!subtileUsers.isEmpty()) {
        QMessageBox::critical(nullptr, tr("Error"), tr("The subtile is used by tile %1 (and maybe others).").arg(subtileUsers.first() + 1));
        return;
    }
    // remove the current subtile
    this->removeSubtile(this->currentSubtileIndex);
    // update the view
    this->displayFrame();
}

void LevelCelView::createTile()
{
    this->til->createTile();
    this->amp->createTile();
    // jump to the new tile
    this->currentTileIndex = this->til->getTileCount() - 1;
    // update the view
    this->displayFrame();
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
            dProgressFail() << tr("Failed to load file: %1.").arg(imagefilePath);
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
        // update the view
        this->displayFrame();
        return;
    }

    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        dProgressFail() << tr("Failed to read file: %1.").arg(imagefilePath);
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

    // update the view
    this->displayFrame();
}

void LevelCelView::removeCurrentTile()
{
    this->til->removeTile(this->currentTileIndex);
    this->amp->removeTile(this->currentTileIndex);
    // update tile index if necessary
    if (this->currentTileIndex == this->til->getTileCount()) {
        this->currentTileIndex = std::max(0, this->currentTileIndex - 1);
    }
    // update the view
    this->displayFrame();
}

void LevelCelView::collectFrameUsers(int frameIndex, QList<int> &users) const
{
    unsigned frameRef = frameIndex + 1;

    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        const QList<quint16> &frameReferences = this->min->getFrameReferences(i);
        for (quint16 reference : frameReferences) {
            if (reference == frameRef) {
                users.append(i);
                break;
            }
        }
    }
}

void LevelCelView::collectSubtileUsers(int subtileIndex, QList<int> &users) const
{
    for (int i = 0; i < this->til->getTileCount(); i++) {
        const QList<quint16> &subtileIndices = this->til->getSubtileIndices(i);
        for (quint16 index : subtileIndices) {
            if (index == subtileIndex) {
                users.append(i);
                break;
            }
        }
    }
}

void LevelCelView::reportUsage()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Scanning..."), 2);

    bool hasFrame = this->gfx->getFrameCount() > this->currentFrameIndex;
    if (hasFrame) {
        QList<int> frameUsers;
        this->collectFrameUsers(this->currentFrameIndex, frameUsers);

        if (frameUsers.count() == 0) {
            dProgress() << tr("Frame %1 is not used by any subtile.").arg(this->currentFrameIndex + 1);
        } else {
            QString frameUses;
            for (int user : frameUsers) {
                frameUses += QString::number(user + 1) + ", ";
            }
            frameUses.chop(2);
            dProgress() << tr("Frame %1 is used by subtile %2.", "", frameUsers.count()).arg(this->currentFrameIndex + 1).arg(frameUses);
        }

        dProgress() << "\n";
    }

    ProgressDialog::incValue();

    bool hasSubtile = this->min->getSubtileCount() > this->currentSubtileIndex;
    if (hasSubtile) {
        QList<int> subtileUsers;
        this->collectSubtileUsers(this->currentSubtileIndex, subtileUsers);

        if (subtileUsers.count() == 0) {
            dProgress() << tr("Subtile %1 is not used by any tile.").arg(this->currentSubtileIndex + 1);
        } else {
            QString subtileUses;
            for (int user : subtileUsers) {
                subtileUses += QString::number(user + 1) + ", ";
            }
            subtileUses.chop(2);
            dProgress() << tr("Subtile %1 is used by tile %2.", "", subtileUsers.count()).arg(this->currentSubtileIndex + 1).arg(subtileUses);
        }
    }

    if (!hasFrame && !hasSubtile) {
        dProgress() << tr("The tileset is empty.");
    }

    ProgressDialog::done(true);
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

void LevelCelView::resetFrameTypes()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Checking frames..."), 1);

    bool result = false;
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        D1GfxFrame *frame = this->gfx->getFrame(i);
        D1CEL_FRAME_TYPE prevType = frame->getFrameType();
        LevelTabFrameWidget::selectFrameType(frame);
        D1CEL_FRAME_TYPE newType = frame->getFrameType();
        if (prevType != newType) {
            dProgress() << tr("Changed Frame %1 from '%2' to '%3'.").arg(i + 1).arg(getFrameTypeName(prevType)).arg(getFrameTypeName(newType));
            result = true;
        }
    }
    if (!result) {
        dProgress() << tr("No change was necessary.");
    } else {
        this->gfx->setModified();
        // update the view
        this->update();
    }

    ProgressDialog::done(true);
}

void LevelCelView::inefficientFrames()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Scanning frames..."), 1);

    int limit = 10;
    bool result = false;
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        D1GfxFrame *frame = this->gfx->getFrame(i);
        if (frame->getFrameType() != D1CEL_FRAME_TYPE::TransparentSquare) {
            continue;
        }
        int diff = limit;
        D1CEL_FRAME_TYPE effType = LevelTabFrameWidget::altFrameType(frame, &diff);
        if (effType != D1CEL_FRAME_TYPE::TransparentSquare) {
            diff = limit - diff;
            dProgress() << tr("Frame %1 could be '%2' by changing %n pixel(s).", "", diff).arg(i + 1).arg(getFrameTypeName(effType));
            result = true;
        }
    }
    if (!result) {
        dProgress() << tr("The frames are optimal.");
    }

    ProgressDialog::done(true);
}

void LevelCelView::checkSubtileFlags()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Checking SOL and TMI flags..."), 2);
    bool result = false;
    // SOL:
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
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
            // TODO: at least one not transparent frame above the floor
        }
        if (solFlags & (1 << 2)) {
            // block missile
            if (!(solFlags & (1 << 0))) {
                dProgressWarn() << tr("Subtile %1 blocks missiles, but still passable (not solid).").arg(i + 1);
                result = true;
            }
            // TODO: at least one not transparent frame above the floor
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
            // TODO: one above the floor is square (left or right)
        }
    }

    ProgressDialog::incValue();

    // TMI:
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        quint8 tmiFlags = this->tmi->getSubtileProperties(i);
        if (tmiFlags & (1 << 0)) {
            // transp.wall
            // TODO: at least one not transparent frame above the floor
            // TODO: no left/right triangle/trapezoid frame above the floor
        }
        /*if (tmiFlags & (1 << 1)) {
            // left second pass
            if (tmiFlags & (1 << 2)) {
                dProgressWarn() << tr("Subtile %1 has both second pass and foliage enabled on the left side.").arg(i + 1);
                result = true;
            }
        }*/
        if (tmiFlags & (1 << 2)) {
            // left foliage
            if (tmiFlags & (1 << 3)) {
                dProgressWarn() << tr("Subtile %1 has both foliage and floor transparency enabled on the left side.").arg(i + 1);
                result = true;
            }
        }
        if (tmiFlags & (1 << 2)) {
            // left floor transparency
            // TODO: must be left trapezoid
        }
        /*if (tmiFlags & (1 << 4)) {
            // right second pass
            if (tmiFlags & (1 << 5)) {
                dProgressWarn() << tr("Subtile %1 has both second pass and foliage enabled on the right side.").arg(i + 1);
                result = true;
            }
        }*/
        if (tmiFlags & (1 << 5)) {
            // right foliage
            if (tmiFlags & (1 << 6)) {
                dProgressWarn() << tr("Subtile %1 has both foliage and floor transparency enabled on the right side.").arg(i + 1);
                result = true;
            }
        }
        if (tmiFlags & (1 << 6)) {
            // right floor transparency
            // TODO: must be right trapezoid
        }
    }
    if (!result) {
        dProgress() << tr("No inconsistency detected.");
    }

    ProgressDialog::done(true);
}

void LevelCelView::checkTileFlags()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Checking AMP flags..."), 1);
    // AMP:
    bool result = false;
    for (int i = 0; i < this->til->getTileCount(); i++) {
        quint8 ampType = this->amp->getTileType(i);
        quint8 ampFlags = this->amp->getTileProperties(i);
        if (ampFlags & (1 << 0)) {
            // western door
            if (ampFlags & (1 << 2)) {
                dProgressWarn() << tr("Tile %1 has both west-door and west arch flags set.").arg(i + 1);
                result = true;
            }
            if (ampFlags & (1 << 4)) {
                dProgressWarn() << tr("Tile %1 has both west-door and west grate flags set.").arg(i + 1);
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
        dProgress() << tr("No inconsistency detected.");
    }

    ProgressDialog::done(true);
}

bool LevelCelView::removeUnusedFrames()
{
    // collect every frame uses
    QList<bool> frameUsed;
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        frameUsed.append(false);
    }
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        const QList<quint16> &frameReferences = this->min->getFrameReferences(i);
        for (quint16 frameRef : frameReferences) {
            if (frameRef != 0) {
                frameUsed[frameRef - 1] = true;
            }
        }
    }
    // remove the unused frames
    bool result = false;
    for (int i = this->gfx->getFrameCount() - 1; i >= 0; i--) {
        if (!frameUsed[i]) {
            this->removeFrame(i);
            dProgress() << tr("Removed frame %1.").arg(i);
            result = true;
        }
    }
    return result;
}

bool LevelCelView::removeUnusedSubtiles()
{
    // collect every subtile uses
    QList<bool> subtileUsed;
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        subtileUsed.append(false);
    }
    for (int i = 0; i < this->til->getTileCount(); i++) {
        const QList<quint16> &subtileIndices = this->til->getSubtileIndices(i);
        for (quint16 subtileIndex : subtileIndices) {
            subtileUsed[subtileIndex] = true;
        }
    }
    // remove the unused subtiles
    bool result = false;
    for (int i = this->min->getSubtileCount() - 1; i >= 0; i--) {
        if (!subtileUsed[i]) {
            this->removeSubtile(i);
            dProgress() << tr("Removed subtile %1.").arg(i);
            result = true;
        }
    }
    return result;
}

void LevelCelView::cleanupFrames()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Scanning frames..."), 1);

    if (this->removeUnusedFrames()) {
        // update the view
        this->displayFrame();
    } else {
        dProgress() << tr("Every frame is used.");
    }

    ProgressDialog::done(true);
}

void LevelCelView::cleanupSubtiles()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Scanning subtiles..."), 1);

    if (this->removeUnusedSubtiles()) {
        // update the view
        this->displayFrame();
    } else {
        dProgress() << tr("Every subtile is used.");
    }

    ProgressDialog::done(true);
}

void LevelCelView::cleanupTileset()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Scanning tileset..."), 2);

    bool removedSubtile = this->removeUnusedSubtiles();

    if (removedSubtile) {
        dProgress() << "\n";
    }

    ProgressDialog::incValue();

    bool removedFrame = this->removeUnusedFrames();

    if (removedSubtile || removedFrame) {
        // update the view
        this->displayFrame();
    } else {
        dProgress() << tr("Every subtile and frame are used.");
    }

    ProgressDialog::done(true);
}

bool LevelCelView::reuseFrames()
{
    std::set<int> removedIndices;
    bool result = false;
    for (int i = 0; i < this->gfx->getFrameCount(); i++) {
        for (int j = i + 1; j < this->gfx->getFrameCount(); j++) {
            D1GfxFrame *frame0 = this->gfx->getFrame(i);
            D1GfxFrame *frame1 = this->gfx->getFrame(j);
            int width = frame0->getWidth();
            int height = frame0->getHeight();
            if (width != frame1->getWidth() || height != frame1->getHeight()) {
                continue; // should not happen, but better safe than sorry
            }
            bool match = true;
            for (int y = 0; y < height && match; y++) {
                for (int x = 0; x < width; x++) {
                    if (frame0->getPixel(x, y) == frame1->getPixel(x, y)) {
                        continue;
                    }
                    match = false;
                    break;
                }
            }
            if (!match) {
                continue;
            }
            // reuse frame0 instead of frame1
            const unsigned frameRef = j + 1;
            for (int n = 0; n < this->min->getSubtileCount(); n++) {
                QList<quint16> &frameReferences = this->min->getFrameReferences(n);
                for (auto iter = frameReferences.begin(); iter != frameReferences.end(); iter++) {
                    if (*iter == frameRef) {
                        *iter = i + 1;
                        this->min->setModified();
                    }
                }
            }
            // eliminate frame1
            this->removeFrame(j);
            // calculate the original indices
            int originalIndexI = i;
            for (auto iter = removedIndices.cbegin(); iter != removedIndices.cend(); ++iter) {
                if (*iter <= originalIndexI) {
                    originalIndexI++;
                    continue;
                }
                break;
            }
            int originalIndexJ = j;
            for (auto iter = removedIndices.cbegin(); iter != removedIndices.cend(); ++iter) {
                if (*iter <= originalIndexJ) {
                    originalIndexJ++;
                    continue;
                }
                break;
            }
            removedIndices.insert(originalIndexJ);
            dProgress() << tr("Using frame %1 instead of %2.").arg(originalIndexI + 1).arg(originalIndexJ + 1);
            result = true;
            j--;
        }
    }
    return result;
}

bool LevelCelView::reuseSubtiles()
{
    std::set<int> removedIndices;
    bool result = false;
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        for (int j = i + 1; j < this->min->getSubtileCount(); j++) {
            QList<quint16> &frameReferences0 = this->min->getFrameReferences(i);
            QList<quint16> &frameReferences1 = this->min->getFrameReferences(j);
            if (frameReferences0.count() != frameReferences1.count()) {
                continue; // should not happen, but better safe than sorry
            }
            bool match = true;
            for (int x = 0; x < frameReferences0.count(); x++) {
                if (frameReferences0[x] == frameReferences1[x]) {
                    continue;
                }
                match = false;
                break;
            }
            if (!match) {
                continue;
            }
            // use subtile 'i' instead of subtile 'j'
            const unsigned refIndex = j;
            for (int n = 0; n < this->til->getTileCount(); n++) {
                QList<quint16> &subtileIndices = this->til->getSubtileIndices(n);
                for (auto iter = subtileIndices.begin(); iter != subtileIndices.end(); iter++) {
                    if (*iter == refIndex) {
                        *iter = i;
                        this->til->setModified();
                    }
                }
            }
            // eliminate subtile 'j'
            this->removeSubtile(j);
            // calculate the original indices
            int originalIndexI = i;
            for (auto iter = removedIndices.cbegin(); iter != removedIndices.cend(); ++iter) {
                if (*iter <= originalIndexI) {
                    originalIndexI++;
                    continue;
                }
                break;
            }
            int originalIndexJ = j;
            for (auto iter = removedIndices.cbegin(); iter != removedIndices.cend(); ++iter) {
                if (*iter <= originalIndexJ) {
                    originalIndexJ++;
                    continue;
                }
                break;
            }
            removedIndices.insert(originalIndexJ);
            dProgress() << tr("Using subtile %1 instead of %2.").arg(originalIndexI + 1).arg(originalIndexJ + 1);
            result = true;
            j--;
        }
    }
    return result;
}

void LevelCelView::compressSubtiles()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Compressing subtiles..."), 1);

    if (this->reuseFrames()) {
        // update the view
        this->displayFrame();
    } else {
        dProgress() << tr("All frames are unique.");
    }

    ProgressDialog::done(true);
}

void LevelCelView::compressTiles()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Compressing tiles..."), 1);

    if (this->reuseSubtiles()) {
        // update the view
        this->displayFrame();
    } else {
        dProgress() << tr("All subtiles are unique.");
    }

    ProgressDialog::done(true);
}

void LevelCelView::compressTileset()
{
    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Compressing tileset..."), 2);

    bool reusedFrame = this->reuseFrames();

    if (reusedFrame) {
        dProgress() << "\n";
    }

    ProgressDialog::incValue();

    bool reusedSubtile = this->reuseSubtiles();

    if (reusedFrame || reusedSubtile) {
        // update the view
        this->displayFrame();
    } else {
        dProgress() << tr("Every subtile and frame are unique.");
    }

    ProgressDialog::done(true);
}

bool LevelCelView::sortFrames_impl()
{
    QMap<unsigned, unsigned> remap;
    bool change = false;
    unsigned idx = 1;

    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        QList<quint16> &frameReferences = this->min->getFrameReferences(i);
        for (auto sit = frameReferences.begin(); sit != frameReferences.end(); ++sit) {
            if (*sit == 0) {
                continue;
            }
            auto mit = remap.find(*sit);
            if (mit != remap.end()) {
                *sit = mit.value();
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
    QMap<unsigned, unsigned> backmap;
    for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
        backmap[iter.value()] = iter.key();
    }
    this->gfx->remapFrames(backmap);
    return true;
}

bool LevelCelView::sortSubtiles_impl()
{
    QMap<unsigned, unsigned> remap;
    bool change = false;
    unsigned idx = 0;

    for (int i = 0; i < this->til->getTileCount(); i++) {
        QList<quint16> &subtileIndices = this->til->getSubtileIndices(i);
        for (auto sit = subtileIndices.begin(); sit != subtileIndices.end(); ++sit) {
            auto mit = remap.find(*sit);
            if (mit != remap.end()) {
                *sit = mit.value();
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
    QMap<unsigned, unsigned> backmap;
    for (auto iter = remap.cbegin(); iter != remap.cend(); ++iter) {
        backmap[iter.value()] = iter.key();
    }
    this->min->remapSubtiles(backmap);
    this->sol->remapSubtiles(backmap);
    this->tmi->remapSubtiles(backmap);
    return true;
}

void LevelCelView::sortFrames()
{
    if (this->sortFrames_impl()) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::sortSubtiles()
{
    if (this->sortSubtiles_impl()) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::sortTileset()
{
    bool change = false;

    change |= this->sortSubtiles_impl();
    change |= this->sortFrames_impl();
    if (change) {
        // update the view
        this->displayFrame();
    }
}

void LevelCelView::upscale(const UpscaleParam &params)
{
    int amount = this->min->getSubtileCount();

    ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Upscaling..."), amount + 1);

    if (Upscaler::upscaleTileset(this->gfx, this->min, params)) {
        // update the view
        this->displayFrame();
    }

    ProgressDialog::done();
}

void LevelCelView::displayFrame()
{
    this->update();
    this->celScene.clear();

    // Getting the current frame/sub-tile/tile to display
    QImage celFrame = this->gfx->getFrameImage(this->currentFrameIndex);
    QImage subtile = this->min->getSubtileImage(this->currentSubtileIndex);
    QImage tile = this->til->getTileImage(this->currentTileIndex);

    this->celScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));
    QColor backColor = QColor(Config::getGraphicsTransparentColor());
    // Building a gray background of the width/height of the CEL frame
    QImage celFrameBackground = QImage(celFrame.width(), celFrame.height(), QImage::Format_ARGB32);
    celFrameBackground.fill(backColor);
    // Building a gray background of the width/height of the MIN subtile
    QImage subtileBackground = QImage(subtile.width(), subtile.height(), QImage::Format_ARGB32);
    subtileBackground.fill(backColor);
    // Building a gray background of the width/height of the MIN subtile
    QImage tileBackground = QImage(tile.width(), tile.height(), QImage::Format_ARGB32);
    tileBackground.fill(backColor);

    // Resize the scene rectangle to include some padding around the CEL frame
    // the MIN subtile and the TIL tile
    this->celScene.setSceneRect(0, 0,
        celFrame.width() + subtile.width() + tile.width() + CEL_SCENE_SPACING * 4,
        tile.height() + CEL_SCENE_SPACING * 2);

    // Add the backgrond and CEL frame while aligning it in the center
    this->celScene.addPixmap(QPixmap::fromImage(celFrameBackground))
        ->setPos(CEL_SCENE_SPACING, CEL_SCENE_SPACING);
    this->celScene.addPixmap(QPixmap::fromImage(celFrame))
        ->setPos(CEL_SCENE_SPACING, CEL_SCENE_SPACING);

    // Set current frame width and height
    this->ui->celFrameWidthEdit->setText(QString::number(celFrame.width()) + " px");
    this->ui->celFrameHeightEdit->setText(QString::number(celFrame.height()) + " px");

    // Set current frame text
    this->ui->frameIndexEdit->setText(
        QString::number(this->gfx->getFrameCount() != 0 ? this->currentFrameIndex + 1 : 0));

    // MIN
    int minPosX = celFrame.width() + CEL_SCENE_SPACING * 2;
    this->celScene.addPixmap(QPixmap::fromImage(subtileBackground))
        ->setPos(minPosX, CEL_SCENE_SPACING);
    this->celScene.addPixmap(QPixmap::fromImage(subtile))
        ->setPos(minPosX, CEL_SCENE_SPACING);

    // Set current frame width and height
    this->ui->minFrameWidthEdit->setText(QString::number(this->min->getSubtileWidth()));
    this->ui->minFrameWidthEdit->setToolTip(QString::number(subtile.width()) + " px");
    this->ui->minFrameHeightEdit->setText(QString::number(this->min->getSubtileHeight()));
    this->ui->minFrameHeightEdit->setToolTip(QString::number(subtile.height()) + " px");

    // Set current subtile text
    this->ui->subtileIndexEdit->setText(
        QString::number(this->min->getSubtileCount() != 0 ? this->currentSubtileIndex + 1 : 0));

    // TIL
    int tilPosX = minPosX + subtile.width() + CEL_SCENE_SPACING;
    this->celScene.addPixmap(QPixmap::fromImage(tileBackground))
        ->setPos(tilPosX, CEL_SCENE_SPACING);
    this->celScene.addPixmap(QPixmap::fromImage(tile))
        ->setPos(tilPosX, CEL_SCENE_SPACING);

    // Set current frame width and height
    this->ui->tilFrameWidthEdit->setText(QString::number(TILE_WIDTH));
    this->ui->tilFrameWidthEdit->setToolTip(QString::number(tile.width()) + " px");
    this->ui->tilFrameHeightEdit->setText(QString::number(TILE_HEIGHT));
    this->ui->tilFrameHeightEdit->setToolTip(QString::number(tile.height()) + " px");

    // Set current tile text
    this->ui->tileIndexEdit->setText(
        QString::number(this->til->getTileCount() != 0 ? this->currentTileIndex + 1 : 0));

    // Notify PalView that the frame changed (used to refresh palette hits)
    emit frameRefreshed();
}

void LevelCelView::playGroup()
{
    MainWindow *mw = (MainWindow *)this->window();

    mw->nextPaletteCycle((D1PAL_CYCLE_TYPE)this->ui->playComboBox->currentIndex());

    // this->displayFrame();
}

void LevelCelView::ShowContextMenu(const QPoint &pos)
{
    MainWindow *mw = (MainWindow *)this->window();
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
    this->currentFrameIndex = 0;
    this->displayFrame();
}

void LevelCelView::on_previousFrameButton_clicked()
{
    if (this->currentFrameIndex >= 1)
        this->currentFrameIndex--;
    else
        this->currentFrameIndex = std::max(0, this->gfx->getFrameCount() - 1);

    this->displayFrame();
}

void LevelCelView::on_nextFrameButton_clicked()
{
    if (this->currentFrameIndex < (this->gfx->getFrameCount() - 1))
        this->currentFrameIndex++;
    else
        this->currentFrameIndex = 0;

    this->displayFrame();
}

void LevelCelView::on_lastFrameButton_clicked()
{
    this->currentFrameIndex = std::max(0, this->gfx->getFrameCount() - 1);
    this->displayFrame();
}

void LevelCelView::on_frameIndexEdit_returnPressed()
{
    int frameIndex = this->ui->frameIndexEdit->text().toInt() - 1;

    if (frameIndex >= 0 && frameIndex < this->gfx->getFrameCount()) {
        this->currentFrameIndex = frameIndex;
        this->displayFrame();
    }
    this->on_frameIndexEdit_escPressed();
}

void LevelCelView::on_frameIndexEdit_escPressed()
{
    this->ui->frameIndexEdit->setText(QString::number(this->gfx->getFrameCount() != 0 ? this->currentFrameIndex + 1 : 0));
    this->ui->frameIndexEdit->clearFocus();
}

void LevelCelView::on_firstSubtileButton_clicked()
{
    this->currentSubtileIndex = 0;
    this->displayFrame();
}

void LevelCelView::on_previousSubtileButton_clicked()
{
    if (this->currentSubtileIndex >= 1)
        this->currentSubtileIndex--;
    else
        this->currentSubtileIndex = std::max(0, this->min->getSubtileCount() - 1);

    this->displayFrame();
}

void LevelCelView::on_nextSubtileButton_clicked()
{
    if (this->currentSubtileIndex < this->min->getSubtileCount() - 1)
        this->currentSubtileIndex++;
    else
        this->currentSubtileIndex = 0;

    this->displayFrame();
}

void LevelCelView::on_lastSubtileButton_clicked()
{
    this->currentSubtileIndex = std::max(0, this->min->getSubtileCount() - 1);
    this->displayFrame();
}

void LevelCelView::on_subtileIndexEdit_returnPressed()
{
    int subtileIndex = this->ui->subtileIndexEdit->text().toInt() - 1;

    if (subtileIndex >= 0 && subtileIndex < this->min->getSubtileCount()) {
        this->currentSubtileIndex = subtileIndex;
        this->displayFrame();
    }
    this->on_subtileIndexEdit_escPressed();
}

void LevelCelView::on_subtileIndexEdit_escPressed()
{
    this->ui->subtileIndexEdit->setText(QString::number(this->min->getSubtileCount() != 0 ? this->currentSubtileIndex + 1 : 0));
    this->ui->subtileIndexEdit->clearFocus();
}

void LevelCelView::on_firstTileButton_clicked()
{
    this->currentTileIndex = 0;
    this->displayFrame();
}

void LevelCelView::on_previousTileButton_clicked()
{
    if (this->currentTileIndex >= 1)
        this->currentTileIndex--;
    else
        this->currentTileIndex = std::max(0, this->til->getTileCount() - 1);

    this->displayFrame();
}

void LevelCelView::on_nextTileButton_clicked()
{
    if (this->currentTileIndex < this->til->getTileCount() - 1)
        this->currentTileIndex++;
    else
        this->currentTileIndex = 0;

    this->displayFrame();
}

void LevelCelView::on_lastTileButton_clicked()
{
    this->currentTileIndex = std::max(0, this->til->getTileCount() - 1);
    this->displayFrame();
}

void LevelCelView::on_tileIndexEdit_returnPressed()
{
    int tileIndex = this->ui->tileIndexEdit->text().toInt() - 1;

    if (tileIndex >= 0 && tileIndex < this->til->getTileCount()) {
        this->currentTileIndex = tileIndex;
        this->displayFrame();
    }
    this->on_tileIndexEdit_escPressed();
}

void LevelCelView::on_tileIndexEdit_escPressed()
{
    this->ui->tileIndexEdit->setText(QString::number(this->til->getTileCount() != 0 ? this->currentTileIndex + 1 : 0));
    this->ui->tileIndexEdit->clearFocus();
}

void LevelCelView::on_minFrameWidthEdit_returnPressed()
{
    unsigned width = this->ui->minFrameWidthEdit->text().toUInt();

    this->min->setSubtileWidth(width);
    // update view
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
    unsigned height = this->ui->minFrameHeightEdit->text().toUInt();

    this->min->setSubtileHeight(height);
    // update view
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
        this->currentPlayDelay = playDelay;

    this->on_playDelayEdit_escPressed();
}

void LevelCelView::on_playDelayEdit_escPressed()
{
    this->ui->playDelayEdit->setText(QString::number(this->currentPlayDelay));
    this->ui->playDelayEdit->clearFocus();
}

void LevelCelView::on_playButton_clicked()
{
    // disable the related fields
    this->ui->playButton->setEnabled(false);
    this->ui->playDelayEdit->setReadOnly(false);
    this->ui->playComboBox->setEnabled(false);
    // enable the stop button
    this->ui->stopButton->setEnabled(true);
    // preserve the palette
    ((MainWindow *)this->window())->initPaletteCycle();

    this->playTimer.start(this->currentPlayDelay);
}

void LevelCelView::on_stopButton_clicked()
{
    this->playTimer.stop();

    // restore palette
    ((MainWindow *)this->window())->resetPaletteCycle();
    // disable the stop button
    this->ui->stopButton->setEnabled(false);
    // enable the related fields
    this->ui->playButton->setEnabled(true);
    this->ui->playDelayEdit->setReadOnly(true);
    this->ui->playComboBox->setEnabled(true);
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
    ((MainWindow *)this->window())->openImageFiles(IMAGE_FILE_MODE::AUTO, filePaths, false);
}
