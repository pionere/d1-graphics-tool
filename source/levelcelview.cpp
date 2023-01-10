#include "levelcelview.h"

#include <algorithm>

#include <QAction>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>

#include "d1image.h"
#include "mainwindow.h"
#include "ui_levelcelview.h"

LevelCelScene::LevelCelScene(QWidget *v)
    : QGraphicsScene()
    , view(v)
{
}

void LevelCelScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    int x = event->scenePos().x();
    int y = event->scenePos().y();

    qDebug() << "Clicked: " << x << "," << y;

    emit this->framePixelClicked(x, y);
}

void LevelCelScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    this->dragMoveEvent(event);
}

void LevelCelScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (MainWindow::hasImageUrl(event->mimeData())) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void LevelCelScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    event->acceptProposedAction();

    QStringList filePaths;
    for (const QUrl &url : event->mimeData()->urls()) {
        filePaths.append(url.toLocalFile());
    }
    // try to insert as frames
    ((MainWindow *)this->view->window())->openImageFiles(IMAGE_FILE_MODE::AUTO, filePaths, false);
}

void LevelCelScene::contextMenuEvent(QContextMenuEvent *event)
{
    ((LevelCelView *)this->view)->ShowContextMenu(event->globalPos());
}

LevelCelView::LevelCelView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LevelCelView)
    , celScene(new LevelCelScene(this))
{
    ui->setupUi(this);
    ui->celGraphicsView->setScene(this->celScene);
    ui->zoomEdit->setText(QString::number(this->currentZoomFactor));
    ui->playDelayEdit->setText(QString::number(this->currentPlayDelay));
    ui->stopButton->setEnabled(false);
    this->playTimer.connect(&this->playTimer, SIGNAL(timeout()), this, SLOT(playGroup()));
    ui->tilesTabs->addTab(this->tabTileWidget, "Tile properties");
    ui->tilesTabs->addTab(this->tabSubTileWidget, "Subtile properties");
    ui->tilesTabs->addTab(this->tabFrameWidget, "Frame properties");

    // If a pixel of the frame, subtile or tile was clicked get pixel color index and notify the palette widgets
    QObject::connect(this->celScene, &LevelCelScene::framePixelClicked, this, &LevelCelView::framePixelClicked);

    // setup context menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));

    setAcceptDrops(true);
}

LevelCelView::~LevelCelView()
{
    delete ui;
    delete celScene;
    delete tabTileWidget;
    delete tabSubTileWidget;
    delete tabFrameWidget;
}

void LevelCelView::initialize(D1Gfx *g, D1Min *m, D1Til *t, D1Sol *s, D1Amp *a, D1Tmi *i)
{
    this->gfx = g;
    this->min = m;
    this->til = t;
    this->sol = s;
    this->amp = a;
    this->tmi = i;

    this->update();
}

void LevelCelView::update()
{
    // Displaying CEL file path information
    QFileInfo gfxFileInfo(this->gfx->getFilePath());
    QFileInfo minFileInfo(this->min->getFilePath());
    QFileInfo tilFileInfo(this->til->getFilePath());
    QFileInfo solFileInfo(this->sol->getFilePath());
    QFileInfo ampFileInfo(this->amp->getFilePath());
    QFileInfo tmiFileInfo(this->tmi->getFilePath());
    ui->celLabel->setText(gfxFileInfo.fileName() + ", " + minFileInfo.fileName() + ", " + tilFileInfo.fileName() + ", " + solFileInfo.fileName() + ", " + ampFileInfo.fileName() + ", " + tmiFileInfo.fileName());

    ui->frameNumberEdit->setText(
        QString::number(this->gfx->getFrameCount()));

    ui->subtileNumberEdit->setText(
        QString::number(this->min->getSubtileCount()));

    ui->tileNumberEdit->setText(
        QString::number(this->til->getTileCount()));

    this->tabTileWidget->initialize(this, this->til, this->min, this->amp);
    this->tabSubTileWidget->initialize(this, this->gfx, this->min, this->sol, this->tmi);
    this->tabFrameWidget->initialize(this, this->gfx);
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

void LevelCelView::framePixelClicked(unsigned x, unsigned y)
{
    quint8 index = 0;

    unsigned celFrameWidth = MICRO_WIDTH; // this->gfx->getFrameWidth(this->currentFrameIndex);
    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned tileWidth = subtileWidth * TILE_WIDTH;

    unsigned celFrameHeight = MICRO_HEIGHT; // this->gfx->getFrameHeight(this->currentFrameIndex);
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;
    unsigned subtileShiftY = subtileWidth / 4;
    unsigned tileHeight = subtileHeight + 2 * subtileShiftY;

    if (x >= CEL_SCENE_SPACING && x < (celFrameWidth + CEL_SCENE_SPACING)
        && y >= CEL_SCENE_SPACING && y < (celFrameHeight + CEL_SCENE_SPACING)
        && this->gfx->getFrameCount() != 0) {
        // If CEL frame color is clicked, select it in the palette widgets
        D1GfxFrame *frame = this->gfx->getFrame(this->currentFrameIndex);
        index = frame->getPixel(x - CEL_SCENE_SPACING, y - CEL_SCENE_SPACING).getPaletteIndex();

        emit this->colorIndexClicked(index);
    } else if (x >= (celFrameWidth + CEL_SCENE_SPACING * 2)
        && x < (celFrameWidth + subtileWidth + CEL_SCENE_SPACING * 2)
        && y >= CEL_SCENE_SPACING
        && y < (subtileHeight + CEL_SCENE_SPACING)
        && this->min->getSubtileCount() != 0) {
        // When a CEL frame is clicked in the subtile, display the corresponding CEL frame

        // Adjust coordinates
        unsigned stx = x - celFrameWidth - CEL_SCENE_SPACING * 2;
        unsigned sty = y - CEL_SCENE_SPACING;

        // qDebug() << "Subtile clicked: " << stx << "," << sty;

        int stFrame = (sty / MICRO_HEIGHT) * TILE_WIDTH + (stx / MICRO_WIDTH);
        QList<quint16> &minFrames = this->min->getCelFrameIndices(this->currentSubtileIndex);
        quint16 frameIndex = minFrames.count() > stFrame ? minFrames.at(stFrame) : 0;

        if (frameIndex > 0) {
            this->currentFrameIndex = frameIndex - 1;
            this->displayFrame();
        }
    } else if (x >= (celFrameWidth + subtileWidth + CEL_SCENE_SPACING * 3)
        && x < (celFrameWidth + subtileWidth + tileWidth + CEL_SCENE_SPACING * 3)
        && y >= CEL_SCENE_SPACING
        && y < tileHeight + CEL_SCENE_SPACING
        && this->til->getTileCount() != 0) {
        // When a subtile is clicked in the tile, display the corresponding subtile

        // Adjust coordinates
        unsigned tx = x - celFrameWidth - subtileWidth - CEL_SCENE_SPACING * 3;
        unsigned ty = y - CEL_SCENE_SPACING;

        // qDebug() << "Tile clicked" << tx << "," << ty;

        // Split the area based on the ground squares
        // f(x)\ 0 /
        //      \ /
        //   2   *   1
        //      / \  
        // g(x)/ 3 \
        //
        // f(x) = (tileHeight - 2 * subtileShiftY) + 0.5x
        unsigned ftx = (tileHeight - 2 * subtileShiftY) + tx / 2;
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
            this->displayFrame();
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
    QList<quint16> *frameIndices = nullptr;
    if (subtileIndex >= 0) {
        frameIndices = &this->min->getCelFrameIndices(subtileIndex);
        frameIndices->clear();
    }
    // TODO: merge with D1Min::insertSubtile ?
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
            if (frameIndices != nullptr) {
                frameIndices->append(hasColor ? frameIndex + 1 : 0);
            }
            if (!hasColor) {
                continue;
            }

            D1GfxFrame *frame = this->gfx->insertFrame(frameIndex, subImage);
            LevelTabFrameWidget::selectFrameType(frame);
            frameIndex++;
        }
    }
}

void LevelCelView::insertFrame(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath)
{
    QImage image = QImage(imagefilePath);
    if (image.isNull()) {
        if (mode != IMAGE_FILE_MODE::AUTO) {
            QMessageBox::critical(nullptr, "Error", "Failed open image file: " + imagefilePath);
        }
        return;
    }

    if ((image.width() % MICRO_WIDTH) != 0 || (image.height() % MICRO_HEIGHT) != 0) {
        return;
    }

    if (mode == IMAGE_FILE_MODE::AUTO
        && image.width() != MICRO_WIDTH && image.height() != MICRO_HEIGHT && image.width() != MICRO_WIDTH * EXPORT_LVLFRAMES_PER_LINE) {
        // not a column of micros
        // not a row or micros
        // not a grouped micros from an export -> ignore
        return;
    }

    this->assignFrames(image, -1, index);
}

void LevelCelView::insertFrames(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append)
{
    int prevFrameCount = this->gfx->getFrameCount();

    if (append) {
        // append the frame(s)
        for (int i = 0; i < imagefilePaths.count(); i++) {
            this->insertFrame(mode, this->gfx->getFrameCount(), imagefilePaths[i]);
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
            this->insertFrame(mode, this->currentFrameIndex, imagefilePaths[i]);
        }
        int deltaFrameCount = this->gfx->getFrameCount() - prevFrameCount;
        if (deltaFrameCount == 0) {
            return; // no new frame -> done
        }
        // shift references
        unsigned refIndex = this->currentFrameIndex + 1;
        // shift frame indices of the subtiles
        for (int i = 0; i < this->min->getSubtileCount(); i++) {
            QList<quint16> &frameIndices = this->min->getCelFrameIndices(i);
            for (int n = 0; n < frameIndices.count(); n++) {
                if (frameIndices[n] >= refIndex) {
                    frameIndices[n] += deltaFrameCount;
                }
            }
        }
    }
    // update the view
    this->update();
    this->displayFrame();
}

void LevelCelView::assignSubtiles(const QImage &image, int tileIndex, int subtileIndex)
{
    QList<quint16> *subtileIndices = nullptr;
    if (tileIndex >= 0) {
        subtileIndices = &this->til->getSubtileIndices(tileIndex);
        subtileIndices->clear();
    }
    // TODO: merge with D1Til::insertTile ?
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

            this->min->insertSubtile(subtileIndex, subImage);
            subtileIndex++;
        }
    }
}

void LevelCelView::insertSubtile(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath)
{
    QImage image = QImage(imagefilePath);
    if (image.isNull()) {
        if (mode != IMAGE_FILE_MODE::AUTO) {
            QMessageBox::critical(nullptr, "Error", "Failed open image file: " + imagefilePath);
        }
        return;
    }

    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if ((image.width() % subtileWidth) != 0 || (image.height() % subtileHeight) != 0) {
        return;
    }

    if (mode == IMAGE_FILE_MODE::AUTO
        && image.width() != subtileWidth && image.height() != subtileHeight && image.width() != subtileWidth * EXPORT_SUBTILES_PER_LINE) {
        // not a column of subtiles
        // not a row or subtiles
        // not a grouped subtiles from an export -> ignore
        return;
    }

    this->assignSubtiles(image, -1, index);
}

void LevelCelView::insertSubtiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append)
{
    int prevSubtileCount = this->min->getSubtileCount();

    if (append) {
        // append the subtile(s)
        for (int i = 0; i < imagefilePaths.count(); i++) {
            this->insertSubtile(mode, this->min->getSubtileCount(), imagefilePaths[i]);
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
            this->insertSubtile(mode, this->currentSubtileIndex, imagefilePaths[i]);
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
                }
            }
        }
    }
    // update the view
    this->update();
    this->displayFrame();
}

void LevelCelView::insertTile(IMAGE_FILE_MODE mode, int index, const QString &imagefilePath)
{
    QImage image = QImage(imagefilePath);
    if (image.isNull()) {
        if (mode != IMAGE_FILE_MODE::AUTO) {
            QMessageBox::critical(nullptr, "Error", "Failed open image file: " + imagefilePath);
        }
        return;
    }

    unsigned tileWidth = this->min->getSubtileWidth() * MICRO_WIDTH * TILE_WIDTH * TILE_HEIGHT;
    unsigned tileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if ((image.width() % tileWidth) != 0 || (image.height() % tileHeight) != 0) {
        return;
    }

    /*if (mode == IMAGE_FILE_MODE::AUTO
        && image.width() != subtileWidth && image.height() != subtileHeight && image.width() != subtileWidth * EXPORT_TILES_PER_LINE) {
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

            this->til->insertTile(index, subImage);
            index++;
        }
    }
}

void LevelCelView::insertTiles(IMAGE_FILE_MODE mode, const QStringList &imagefilePaths, bool append)
{
    int prevTileCount = this->til->getTileCount();

    if (append) {
        // append the tile(s)
        for (int i = 0; i < imagefilePaths.count(); i++) {
            this->insertTile(mode, this->til->getTileCount(), imagefilePaths[i]);
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
            this->insertTile(mode, this->currentTileIndex, imagefilePaths[i]);
        }
        int deltaTileCount = this->til->getTileCount() - prevTileCount;
        if (deltaTileCount == 0) {
            return; // no new tile -> done
        }
    }
    // update the view
    this->update();
    this->displayFrame();
}

void LevelCelView::replaceCurrentFrame(const QString &imagefilePath)
{
    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        QMessageBox::critical(nullptr, "Error", "Failed open image file: " + imagefilePath);
        return;
    }

    if (image.width() != MICRO_WIDTH || image.height() != MICRO_HEIGHT) {
        QMessageBox::warning(this, "Warning", "The image must be 32px * 32px to be used as a level-frame.");
        return;
    }

    D1GfxFrame *frame = this->gfx->replaceFrame(this->currentFrameIndex, image);

    if (frame != nullptr) {
        LevelTabFrameWidget::selectFrameType(frame);
        // update the view
        this->update();
        this->displayFrame();
    }
}

void LevelCelView::removeCurrentFrame()
{
    // check if the frame is used
    int user = -1;
    unsigned refIndex = this->currentFrameIndex + 1;
    for (int i = 0; i < this->min->getSubtileCount() && user < 0; i++) {
        QList<quint16> &frameIndices = this->min->getCelFrameIndices(i);
        for (quint16 frameIdx : frameIndices) {
            if (frameIdx == refIndex) {
                user = i;
                break;
            }
        }
    }
    if (user >= 0) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, "Confirmation", "The frame is used by Subtile " + QString::number(user + 1) + " (and maybe others). Are you sure you want to proceed?", QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }
    // remove the frame
    this->gfx->removeFrame(this->currentFrameIndex);
    if (this->gfx->getFrameCount() == this->currentFrameIndex) {
        this->currentFrameIndex = std::max(0, this->currentFrameIndex - 1);
    }
    // shift references
    // - shift frame indices of the subtiles
    for (int i = 0; i < this->min->getSubtileCount(); i++) {
        QList<quint16> &frameIndices = this->min->getCelFrameIndices(i);
        for (int n = 0; n < frameIndices.count(); n++) {
            if (frameIndices[n] >= refIndex) {
                if (frameIndices[n] == refIndex) {
                    frameIndices[n] = 0;
                } else {
                    frameIndices[n] -= 1;
                }
            }
        }
    }
    // update the view
    this->update();
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
    this->update();
    this->displayFrame();
}

void LevelCelView::replaceCurrentSubtile(const QString &imagefilePath)
{
    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        QMessageBox::critical(nullptr, "Error", "Failed open image file: " + imagefilePath);
        return;
    }

    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if (image.width() != subtileWidth || image.height() != subtileHeight) {
        QMessageBox::warning(this, "Warning", "The image must be " + QString::number(subtileWidth) + "px * " + QString::number(subtileHeight) + "px to be used as a subtile.");
        return;
    }

    int subtileIndex = this->currentSubtileIndex;
    this->assignFrames(image, subtileIndex, this->gfx->getFrameCount());

    // reset subtile flags
    this->sol->setSubtileProperties(subtileIndex, 0);
    this->tmi->setSubtileProperties(subtileIndex, 0);

    // update the view
    this->update();
    this->displayFrame();
}

void LevelCelView::removeCurrentSubtile()
{
    // check if the subtile is used
    int user = -1;
    unsigned refIndex = this->currentSubtileIndex;
    for (int i = 0; i < this->til->getTileCount() && user < 0; i++) {
        QList<quint16> &subtileIndices = this->til->getSubtileIndices(i);
        for (quint16 subtileIdx : subtileIndices) {
            if (subtileIdx == refIndex) {
                user = i;
                break;
            }
        }
    }
    if (user >= 0) {
        QMessageBox::critical(nullptr, "Error", "The subtile is used by Tile " + QString::number(user + 1) + " (and maybe others).");
        return;
    }
    this->min->removeSubtile(this->currentSubtileIndex);
    this->sol->removeSubtile(this->currentSubtileIndex);
    this->tmi->removeSubtile(this->currentSubtileIndex);
    // update subtile index if necessary
    if (this->currentSubtileIndex == this->min->getSubtileCount()) {
        this->currentSubtileIndex = std::max(0, this->currentSubtileIndex - 1);
    }
    // shift references
    // - shift subtile indices of the tiles
    for (int i = 0; i < this->til->getTileCount(); i++) {
        QList<quint16> &subtileIndices = this->til->getSubtileIndices(i);
        for (int n = 0; n < subtileIndices.count(); n++) {
            if (subtileIndices[n] >= refIndex) {
                // assert(subtileIndices[n] != refIndex);
                subtileIndices[n] -= 1;
            }
        }
    }
    // update the view
    this->update();
    this->displayFrame();
}

void LevelCelView::createTile()
{
    this->til->createTile();
    this->amp->createTile();
    // jump to the new tile
    this->currentTileIndex = this->til->getTileCount() - 1;
    // update the view
    this->update();
    this->displayFrame();
}

void LevelCelView::replaceCurrentTile(const QString &imagefilePath)
{
    QImage image = QImage(imagefilePath);

    if (image.isNull()) {
        QMessageBox::critical(nullptr, "Error", "Failed open image file: " + imagefilePath);
        return;
    }

    unsigned tileWidth = this->min->getSubtileWidth() * MICRO_WIDTH * TILE_WIDTH * TILE_HEIGHT;
    unsigned tileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    if (image.width() != tileWidth || image.height() != tileHeight) {
        QMessageBox::warning(this, "Warning", "The image must be " + QString::number(tileWidth) + "px * " + QString::number(tileHeight) + "px to be used as a tile.");
        return;
    }

    int tileIndex = this->currentTileIndex;
    this->assignSubtiles(image, tileIndex, this->min->getSubtileCount());

    // reset tile flags
    this->amp->setTileProperties(tileIndex, 0);

    // update the view
    this->update();
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
    this->update();
    this->displayFrame();
}

void LevelCelView::displayFrame()
{
    quint16 minPosX = 0;
    quint16 tilPosX = 0;

    this->celScene->clear();

    // Getting the current frame/sub-tile/tile to display
    QImage celFrame = this->gfx->getFrameImage(this->currentFrameIndex);
    QImage subtile = this->min->getSubtileImage(this->currentSubtileIndex);
    QImage tile = this->til->getTileImage(this->currentTileIndex);

    this->tabSubTileWidget->update();
    this->tabTileWidget->update();
    this->tabFrameWidget->update();

    // Building a gray background of the width/height of the CEL frame
    QImage celFrameBackground = QImage(celFrame.width(), celFrame.height(), QImage::Format_ARGB32);
    celFrameBackground.fill(Qt::gray);
    // Building a gray background of the width/height of the MIN subtile
    QImage subtileBackground = QImage(subtile.width(), subtile.height(), QImage::Format_ARGB32);
    subtileBackground.fill(Qt::gray);
    // Building a gray background of the width/height of the MIN subtile
    QImage tileBackground = QImage(tile.width(), tile.height(), QImage::Format_ARGB32);
    tileBackground.fill(Qt::gray);

    // Resize the scene rectangle to include some padding around the CEL frame
    // the MIN subtile and the TIL tile
    this->celScene->setSceneRect(0, 0,
        celFrame.width() + subtile.width() + tile.width() + CEL_SCENE_SPACING * 4,
        tile.height() + CEL_SCENE_SPACING * 2);

    // Add the backgrond and CEL frame while aligning it in the center
    this->celScene->addPixmap(QPixmap::fromImage(celFrameBackground))
        ->setPos(CEL_SCENE_SPACING, CEL_SCENE_SPACING);
    this->celScene->addPixmap(QPixmap::fromImage(celFrame))
        ->setPos(CEL_SCENE_SPACING, CEL_SCENE_SPACING);

    // Set current frame width and height
    this->ui->celFrameWidthEdit->setText(QString::number(celFrame.width()) + " px");
    this->ui->celFrameHeightEdit->setText(QString::number(celFrame.height()) + " px");

    // Set current frame text
    this->ui->frameIndexEdit->setText(
        QString::number(this->gfx->getFrameCount() != 0 ? this->currentFrameIndex + 1 : 0));

    // MIN
    minPosX = celFrame.width() + CEL_SCENE_SPACING * 2;
    this->celScene->addPixmap(QPixmap::fromImage(subtileBackground))
        ->setPos(minPosX, CEL_SCENE_SPACING);
    this->celScene->addPixmap(QPixmap::fromImage(subtile))
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
    tilPosX = minPosX + subtile.width() + CEL_SCENE_SPACING;
    this->celScene->addPixmap(QPixmap::fromImage(tileBackground))
        ->setPos(tilPosX, CEL_SCENE_SPACING);
    this->celScene->addPixmap(QPixmap::fromImage(tile))
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

    switch (this->ui->playComboBox->currentIndex()) {
    case 0: // caves
        mw->nextPaletteCycle(D1PAL_CYCLE_TYPE::CAVES);
        break;
    case 1: // nest
        mw->nextPaletteCycle(D1PAL_CYCLE_TYPE::NEST);
        break;
    case 2: // crypt
        mw->nextPaletteCycle(D1PAL_CYCLE_TYPE::CRYPT);
        break;
    }

    // this->displayFrame();
}

void LevelCelView::ShowContextMenu(const QPoint &pos)
{
    MainWindow *mw = (MainWindow *)this->window();

    QMenu contextMenu(tr("Context menu"), this);

    QMenu frameMenu(tr("Frame"), this);
    frameMenu.setToolTipsVisible(true);

    QAction action0("Insert", this);
    action0.setToolTip("Add new frames before the current one");
    QObject::connect(&action0, SIGNAL(triggered()), mw, SLOT(on_actionInsert_Frame_triggered()));
    frameMenu.addAction(&action0);

    QAction action1("Add", this);
    action1.setToolTip("Add new frames at the end");
    QObject::connect(&action1, SIGNAL(triggered()), mw, SLOT(on_actionAdd_Frame_triggered()));
    frameMenu.addAction(&action1);

    QAction action2("Replace", this);
    action2.setToolTip("Replace the current frame");
    QObject::connect(&action2, SIGNAL(triggered()), mw, SLOT(on_actionReplace_Frame_triggered()));
    if (this->gfx->getFrameCount() == 0) {
        action2.setEnabled(false);
    }
    frameMenu.addAction(&action2);

    QAction action3("Delete", this);
    action3.setToolTip("Delete the current frame");
    QObject::connect(&action3, SIGNAL(triggered()), mw, SLOT(on_actionDel_Frame_triggered()));
    if (this->gfx->getFrameCount() == 0) {
        action3.setEnabled(false);
    }
    frameMenu.addAction(&action3);

    contextMenu.addMenu(&frameMenu);

    QMenu subtileMenu(tr("Subtile"), this);
    subtileMenu.setToolTipsVisible(true);

    QAction action4("Create", this);
    action4.setToolTip("Create a new subtile");
    QObject::connect(&action4, SIGNAL(triggered()), mw, SLOT(on_actionCreate_Subtile_triggered()));
    subtileMenu.addAction(&action4);

    QAction action5("Insert", this);
    action5.setToolTip("Add new subtiles before the current one");
    QObject::connect(&action5, SIGNAL(triggered()), mw, SLOT(on_actionInsert_Subtile_triggered()));
    subtileMenu.addAction(&action5);

    QAction action6("Add", this);
    action6.setToolTip("Add new subtiles at the end");
    QObject::connect(&action6, SIGNAL(triggered()), mw, SLOT(on_actionAdd_Subtile_triggered()));
    subtileMenu.addAction(&action6);

    QAction action7("Replace", this);
    action7.setToolTip("Replace the current subtile");
    QObject::connect(&action7, SIGNAL(triggered()), mw, SLOT(on_actionReplace_Subtile_triggered()));
    if (this->min->getSubtileCount() == 0) {
        action7.setEnabled(false);
    }
    subtileMenu.addAction(&action7);

    QAction action8("Delete", this);
    action8.setToolTip("Delete the current subtile");
    QObject::connect(&action8, SIGNAL(triggered()), mw, SLOT(on_actionDel_Subtile_triggered()));
    if (this->min->getSubtileCount() == 0) {
        action8.setEnabled(false);
    }
    subtileMenu.addAction(&action8);

    contextMenu.addMenu(&subtileMenu);

    QMenu tileMenu(tr("Tile"), this);
    tileMenu.setToolTipsVisible(true);

    QAction action9("Create", this);
    action9.setToolTip("Create a new tile");
    QObject::connect(&action9, SIGNAL(triggered()), mw, SLOT(on_actionCreate_Tile_triggered()));
    if (this->min->getSubtileCount() == 0) {
        action9.setEnabled(false);
    }
    tileMenu.addAction(&action9);

    QAction action10("Insert", this);
    action10.setToolTip("Add new tiles before the current one");
    QObject::connect(&action10, SIGNAL(triggered()), mw, SLOT(on_actionInsert_Tile_triggered()));
    tileMenu.addAction(&action10);

    QAction action11("Add", this);
    action11.setToolTip("Add new tiles at the end");
    QObject::connect(&action11, SIGNAL(triggered()), mw, SLOT(on_actionAdd_Tile_triggered()));
    tileMenu.addAction(&action11);

    QAction action12("Replace", this);
    action12.setToolTip("Replace the current tile");
    QObject::connect(&action12, SIGNAL(triggered()), mw, SLOT(on_actionReplace_Tile_triggered()));
    if (this->til->getTileCount() == 0) {
        action12.setEnabled(false);
    }
    tileMenu.addAction(&action12);

    QAction action13("Delete", this);
    action13.setToolTip("Delete the current tile");
    QObject::connect(&action13, SIGNAL(triggered()), mw, SLOT(on_actionDel_Tile_triggered()));
    if (this->til->getTileCount() == 0) {
        action13.setEnabled(false);
    }
    tileMenu.addAction(&action13);

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
}

void LevelCelView::on_minFrameWidthEdit_returnPressed()
{
    unsigned width = this->ui->minFrameWidthEdit->text().toUInt();

    this->min->setSubtileWidth(width);
    this->displayFrame();
}

void LevelCelView::on_minFrameHeightEdit_returnPressed()
{
    unsigned height = this->ui->minFrameHeightEdit->text().toUInt();

    this->min->setSubtileHeight(height);
    this->displayFrame();
}

void LevelCelView::on_zoomOutButton_clicked()
{
    if (this->currentZoomFactor - 1 >= 1)
        this->currentZoomFactor -= 1;
    ui->celGraphicsView->resetTransform();
    ui->celGraphicsView->scale(this->currentZoomFactor, this->currentZoomFactor);
    ui->celGraphicsView->show();
    ui->zoomEdit->setText(QString::number(this->currentZoomFactor));
}

void LevelCelView::on_zoomInButton_clicked()
{
    if (this->currentZoomFactor + 1 <= 10)
        this->currentZoomFactor += 1;
    ui->celGraphicsView->resetTransform();
    ui->celGraphicsView->scale(this->currentZoomFactor, this->currentZoomFactor);
    ui->celGraphicsView->show();
    ui->zoomEdit->setText(QString::number(this->currentZoomFactor));
}

void LevelCelView::on_zoomEdit_returnPressed()
{
    quint8 zoom = this->ui->zoomEdit->text().toUShort();

    if (zoom >= 1 && zoom <= 10)
        this->currentZoomFactor = zoom;
    ui->celGraphicsView->resetTransform();
    ui->celGraphicsView->scale(this->currentZoomFactor, this->currentZoomFactor);
    ui->celGraphicsView->show();
    ui->zoomEdit->setText(QString::number(this->currentZoomFactor));
}

void LevelCelView::on_playDelayEdit_textChanged(const QString &text)
{
    quint16 playDelay = text.toUInt();

    if (playDelay != 0)
        this->currentPlayDelay = playDelay;
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
