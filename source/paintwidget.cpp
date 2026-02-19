#include "paintwidget.h"

#include <queue>

#include <QCursor>
#include <QGraphicsView>
#include <QImage>
#include <QList>
#include <QMessageBox>
#include <QString>

#include "config.h"
#include "d1image.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_paintwidget.h"

Q_DECLARE_METATYPE(D1CEL_FRAME_TYPE)

FramePixel::FramePixel(const QPoint &p, D1GfxPixel px)
    : pos(p)
    , pixel(px)
{
}

EditFrameCommand::EditFrameCommand(D1GfxFrame *f, const QPoint &pos, D1GfxPixel newPixel)
    : QUndoCommand(nullptr)
    , frame(f)
{
    this->modPixels.push_back(FramePixel(pos, newPixel));
}

EditFrameCommand::EditFrameCommand(D1GfxFrame *f, const std::vector<FramePixel> &pixels)
    : QUndoCommand(nullptr)
    , frame(f)
    , modPixels(pixels)
{
}

void EditFrameCommand::undo()
{
    if (this->frame.isNull()) {
        this->setObsolete(true);
        return;
    }

    for (FramePixel &fp : this->modPixels) {
        D1GfxPixel pixel = this->frame->getPixel(fp.pos.x(), fp.pos.y());
        if (pixel != fp.pixel) {
            this->frame->setPixel(fp.pos.x(), fp.pos.y(), fp.pixel);
            fp.pixel = pixel;
        }
    }

    dMainWindow().frameModified(this->frame);
}

void EditFrameCommand::redo()
{
    this->undo();
}

PaintWidget::PaintWidget(QWidget *parent, QUndoStack *us, D1Gfx *g, CelView *cv, LevelCelView *lcv, GfxsetView *gsv)
    : QFrame(parent)
    , ui(new Ui::PaintWidget())
    , undoStack(us)
    , gfx(g)
    , celView(cv)
    , levelCelView(lcv)
    , gfxsetView(gsv)
    , rubberBand(nullptr)
    , moving(false)
    , moved(false)
    , lastMoveCmd(nullptr)
{
    this->ui->setupUi(this);

    QLayout *layout = this->ui->centerButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogListView, tr("Move"), this, &PaintWidget::on_movePushButtonClicked);
    layout = this->ui->rightButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogCloseButton, tr("Close"), this, &PaintWidget::on_closePushButtonClicked);

    // initialize the edit fields
    this->on_brushWidthLineEdit_escPressed();
    this->on_brushLengthLineEdit_escPressed();
    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->brushWidthLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_brushWidthLineEdit_escPressed()));
    QObject::connect(this->ui->brushLengthLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_brushLengthLineEdit_escPressed()));

    // prepare combobox of the masks
    if (lcv != nullptr) {
        this->ui->tilesetMaskWidget->setVisible(true);
        this->ui->tilesetMaskComboBox->addItem(tr("Square"), QVariant::fromValue(D1CEL_FRAME_TYPE::Square));
        this->ui->tilesetMaskComboBox->addItem(tr("Left Triangle"), QVariant::fromValue(D1CEL_FRAME_TYPE::LeftTriangle));
        this->ui->tilesetMaskComboBox->addItem(tr("Right Triangle"), QVariant::fromValue(D1CEL_FRAME_TYPE::RightTriangle));
        this->ui->tilesetMaskComboBox->addItem(tr("Left Trapezoid"), QVariant::fromValue(D1CEL_FRAME_TYPE::LeftTrapezoid));
        this->ui->tilesetMaskComboBox->addItem(tr("Right Trapezoid"), QVariant::fromValue(D1CEL_FRAME_TYPE::RightTrapezoid));
    }
    this->adjustSize(); // not sure why this is necessary if lcv is nullptr...

    // assume the first color is selected on the palette-widget
    this->selectedColors.push_back(0);

    // cache the active graphics view
    QList<QGraphicsView *> views;
    if (cv != nullptr) {
        views = cv->getCelScene()->views();
    }
    if (lcv != nullptr) {
        views = lcv->getCelScene()->views();
    }
    if (gsv != nullptr) {
        views = gsv->getCelScene()->views();
    }
    this->graphView = views[0];
}

PaintWidget::~PaintWidget()
{
    delete ui;
    delete rubberBand;
}

void PaintWidget::setPalette(D1Pal *p)
{
    this->pal = p;
}

void PaintWidget::setGfx(D1Gfx *g)
{
    this->gfx = g;
}

void PaintWidget::show()
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
    QFrame::show();

    this->setFocus(); // otherwise the widget does not receive keypress events...
    this->graphView->setCursor(Qt::CrossCursor);
    // update the view
    this->colorModified();
}

void PaintWidget::hide()
{
    if (this->rubberBand) {
        MemFree(this->rubberBand);
    }
    if (this->moving) {
        this->stopMove();
    }
    this->graphView->unsetCursor();

    QFrame::hide();
}

static QRect getArea(const QPoint &pos1, const QPoint &pos2)
{
    return QRect(pos1, pos2).normalized();
}

D1GfxFrame *PaintWidget::getCurrentFrame() const
{
    int frameIndex = 0;
    if (this->levelCelView != nullptr) {
        frameIndex = this->levelCelView->getCurrentFrameIndex();
    } else if (this->celView != nullptr) {
        frameIndex = this->celView->getCurrentFrameIndex();
    } else if (this->gfxsetView != nullptr) {
        frameIndex = this->gfxsetView->getCurrentFrameIndex();
    }
    return this->gfx->getFrameCount() > frameIndex ? const_cast<D1GfxFrame *>(this->gfx->getFrame(frameIndex)) : nullptr;
}

QRect PaintWidget::getSelectArea(const D1GfxFrame *frame) const
{
    QRect area = getArea(this->currPos, this->lastPos);
    if (area.left() < 0) {
        area.setLeft(0);
    }
    if (area.right() >= frame->getWidth()) {
        area.setRight(frame->getWidth() - 1);
    }
    if (area.top() < 0) {
        area.setTop(0);
    }
    if (area.bottom() >= frame->getHeight()) {
        area.setBottom(frame->getHeight() - 1);
    }
    return area;
}

void PaintWidget::pasteCurrentFrame(const D1GfxFrame &srcFrame)
{
    // select frame
    D1GfxFrame *frame = this->getCurrentFrame();
    if (frame == nullptr) {
        return;
    }
    // select starting position + select the destination rectangle
    QPoint destPos = QPoint(0, 0);
    if (this->rubberBand != nullptr) {
        QRect area = getArea(this->currPos, this->lastPos);
        destPos = area.topLeft() + this->lastDelta;
    } else {
        this->rubberBand = new QRubberBand(QRubberBand::Rectangle, this->parentWidget());
    }
    this->currPos = destPos;
    const int width = srcFrame.getWidth();
    const int height = srcFrame.getHeight();
    QRect area = QRect(destPos, QSize(width, height));
    this->lastPos = area.bottomRight();
    this->selectArea(area);
    this->selectionMoveMode = 0;

    // copy to the destination
    std::vector<FramePixel> pixels;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            D1GfxPixel pixel = srcFrame.getPixel(x, y);
            if (pixel.isTransparent()) {
                continue;
            }
            QPoint tp = destPos + QPoint(x, y);
            if (tp.x() < 0 || tp.x() >= frame->getWidth()) {
                continue;
            }
            if (tp.y() < 0 || tp.y() >= frame->getHeight()) {
                continue;
            }
            pixels.push_back(FramePixel(tp, pixel));
        }
    }
    if (!pixels.empty()) {
        // Build frame editing command and connect it to the current main window widget
        // to update the palHits and CEL views when undo/redo is performed
        EditFrameCommand *command = new EditFrameCommand(frame, pixels);

        this->undoStack->push(command);
    }
}

QString PaintWidget::copyCurrentPixels(bool values) const
{
    const D1GfxFrame *frame = this->getCurrentFrame();
    if (this->rubberBand == nullptr || frame == nullptr) {
        return QString();
    }
    QRect area = this->getSelectArea(frame);
    QString pixels;
    D1Pal *pal = values ? nullptr : this->pal;
    for (int y = area.top(); y <= area.bottom(); y++) {
        for (int x = area.left(); x <= area.right(); x++) {
            D1GfxPixel d1pix = frame->getPixel(x, y);

            QString colorTxt = d1pix.colorText(pal);
            pixels.append(colorTxt);
        }
        pixels.append('\n');
    }
    pixels = pixels.trimmed();
    return pixels;
}

void PaintWidget::pasteCurrentPixels(const QString &pixels)
{
    // load the image
    D1GfxFrame srcFrame;
    D1ImageFrame::load(srcFrame, pixels, this->pal);

    this->pasteCurrentFrame(srcFrame);
}

QImage PaintWidget::copyCurrentImage() const
{
    D1GfxFrame *frame = this->getCurrentFrame();
    if (this->rubberBand == nullptr || frame == nullptr) {
        return QImage();
    }
    QRect area = this->getSelectArea(frame);
    QImage image = QImage(area.size(), QImage::Format_ARGB32);
    QRgb *destBits = reinterpret_cast<QRgb *>(image.bits());
    for (int y = area.top(); y <= area.bottom(); y++) {
        for (int x = area.left(); x <= area.right(); x++, destBits++) {
            D1GfxPixel d1pix = frame->getPixel(x, y);

            QColor color;
            if (d1pix.isTransparent())
                color = QColor(Qt::transparent);
            else
                color = this->pal->getColor(d1pix.getPaletteIndex());

            *destBits = color.rgba();
        }
    }
    return image;
}

void PaintWidget::pasteCurrentImage(const QImage &image)
{
    // load the image
    D1GfxFrame srcFrame;
    D1ImageFrame::load(srcFrame, image, false, this->pal);

    this->pasteCurrentFrame(srcFrame);
}

void PaintWidget::deleteCurrent()
{
    // select frame
    D1GfxFrame *frame = this->getCurrentFrame();
    if (this->rubberBand == nullptr || frame == nullptr) {
        return;
    }

    QRect area = getArea(this->currPos, this->lastPos);
    std::vector<FramePixel> pixels;
    for (int y = area.top(); y <= area.bottom(); y++) {
        for (int x = area.left(); x <= area.right(); x++) {
            if (x < 0 || x >= frame->getWidth()) {
                continue;
            }
            if (y < 0 || y >= frame->getHeight()) {
                continue;
            }
            if (!frame->getPixel(x, y).isTransparent()) {
                pixels.push_back(FramePixel(QPoint(x, y), D1GfxPixel::transparentPixel()));
            }
        }
    }
    if (!pixels.empty()) {
        // Build frame editing command and connect it to the current main window widget
        // to update the palHits and CEL views when undo/redo is performed
        EditFrameCommand *command = new EditFrameCommand(frame, pixels);

        this->undoStack->push(command);
    }
}

void PaintWidget::toggleMode()
{
    if (this->ui->selectModeRadioButton->isChecked()) {
        if (this->prevmode == 0)
            this->ui->drawModeRadioButton->setChecked(true);
        else
            this->ui->fillModeRadioButton->setChecked(true);
    } else {
        this->prevmode = this->ui->fillModeRadioButton->isChecked() ? 1 : 0;
        this->ui->selectModeRadioButton->setChecked(true);
    }
}

void PaintWidget::adjustBrush(int dir)
{
    const bool size = QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier;
    if (size) {
        if (dir >= 0)
            this->on_brushWidthIncButton_clicked();
        else
            this->on_brushWidthDecButton_clicked();
    } else {
        if (dir >= 0)
            this->on_brushLengthIncButton_clicked();
        else
            this->on_brushLengthDecButton_clicked();
    }
}

D1GfxPixel PaintWidget::getCurrentColor(unsigned counter) const
{
    unsigned numColors = this->selectedColors.size();
    if (numColors == 0) {
        return D1GfxPixel::transparentPixel();
    }
    return D1GfxPixel::colorPixel(this->selectedColors[(counter / this->brushLength) % numColors]);
}

void PaintWidget::collectPixels(const D1GfxFrame *frame, const QPoint &startPos, std::vector<FramePixel> &pixels)
{
    std::queue<std::pair<QPoint, int>> posQueue;
    posQueue.push(std::pair<QPoint, int>(startPos, 0));

    const D1GfxPixel pixel = frame->getPixel(startPos.x(), startPos.y());
    const bool squareShape = this->ui->squareShapeRadioButton->isChecked();

    while (!posQueue.empty()) {
        QPoint currPos = posQueue.front().first;
        int dist = posQueue.front().second;
        posQueue.pop();

        unsigned n = 0;
        for (; n < pixels.size(); n++) {
            if (pixels[n].pos == currPos) {
                break;
            }
        }
        if (n < pixels.size()) {
            continue;
        }
        pixels.push_back(FramePixel(currPos, this->getCurrentColor(dist)));
        dist++;

        unsigned numOffsets;
        const std::pair<int, int> *offsets;
        if (squareShape) {
            const std::pair<int, int> squareOffsets[8] = { { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 1 }, { 0, 1 }, { 1, 1 }, { -1, 0 }, { 1, 0 } };
            numOffsets = 8;
            offsets = squareOffsets;
        } else {
            const std::pair<int, int> roundOffsets[4] = { { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } };
            numOffsets = 4;
            offsets = roundOffsets;
        }

        for (unsigned i = 0; i < numOffsets; i++) {
            QPoint pos = QPoint(currPos.x() + offsets[i].first, currPos.y() + offsets[i].second);
            if (pos.x() < 0 || pos.x() >= frame->getWidth()) {
                continue;
            }
            if (pos.y() < 0 || pos.y() >= frame->getHeight()) {
                continue;
            }
            if (pixel != frame->getPixel(pos.x(), pos.y())) {
                continue;
            }
            posQueue.push(std::pair<QPoint, int>(pos, dist));
        }
    }
}

void PaintWidget::collectPixelsSquare(int baseX, int baseY, int baseDist, std::vector<FramePixel> &pixels)
{
    const int width = this->brushWidth;
    int x0 = baseX - (width - 1) / 2;
    int y0 = baseY - (width - 1) / 2;
    for (int y = y0; y < y0 + width; y++) {
        for (int x = x0; x < x0 + width; x++) {
            QPoint pos = QPoint(x, y);
            unsigned n = 0;
            for (; n < pixels.size(); n++) {
                if (pixels[n].pos == pos) {
                    break;
                }
            }
            if (n < pixels.size()) {
                continue;
            }
            int dist = baseDist + std::max(abs(x - baseX), abs(abs(y - baseY)));
            pixels.push_back(FramePixel(pos, this->getCurrentColor(dist)));
        }
    }
}

void PaintWidget::collectPixelsRound(int baseX, int baseY, int baseDist, std::vector<FramePixel> &pixels)
{
    const int width = this->brushWidth;
    const int r = (width + 1) / 2;
    const double rd = width / 2.0;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            QPoint pos = QPoint(baseX + dx, baseY + dy);
            unsigned n = 0;
            for (; n < pixels.size(); n++) {
                if (pixels[n].pos == pos) {
                    break;
                }
            }
            if (n < pixels.size()) {
                continue;
            }
            double dd = sqrt(dx * dx + dy * dy);
            if (rd < dd) {
                continue;
            }
            int dist = baseDist + (int)(dd + 0.5);
            pixels.push_back(FramePixel(pos, this->getCurrentColor(dist)));
        }
    }
}

/**
 * Collect pixel-locations in the (startPos:destPos] range.
 *
 * @param startPos: the starting position
 * @param destPos: the destination
 * @param pixels: the container for the collected locations.
 */
void PaintWidget::traceClick(const QPoint &startPos, const QPoint &destPos, std::vector<FramePixel> &pixels, void (PaintWidget::*collectorFunc)(int, int, int, std::vector<FramePixel> &))
{
    int x1 = startPos.x();
    int y1 = startPos.y();
    int x2 = destPos.x();
    int y2 = destPos.y();
    int dx = x2 - x1;
    int dy = y2 - y1;
    int xinc, yinc, d;
    int dist = this->distance;
    // find out step size and direction on the y coordinate
    xinc = dx < 0 ? -1 : 1;
    yinc = dy < 0 ? -1 : 1;

    dx = abs(dx);
    dy = abs(dy);
    if (dx >= dy) {
        if (dx == 0) {
            return;
        }

        // multiply by 2 so we round up
        // dy *= 2;
        d = 0;
        while (true) {
            d += dy;
            if (d >= dx) {
                // d -= 2 * dx; // multiply by 2 to support rounding
                d -= dx;
                y1 += yinc;
            }
            x1 += xinc;
            dist++;
            (this->*collectorFunc)(x1, y1, dist, pixels);
            if (x1 == x2)
                break;
        }
    } else {
        // multiply by 2 so we round up
        // dx *= 2;
        d = 0;
        while (true) {
            d += dx;
            if (d >= dy) {
                // d -= 2 * dy; // multiply by 2 to support rounding
                d -= dy;
                x1 += xinc;
            }
            y1 += yinc;
            dist++;
            (this->*collectorFunc)(x1, y1, dist, pixels);
            if (y1 == y2)
                break;
        }
    }
}

void PaintWidget::selectArea(const QRect &area)
{
    QRect sceneRect = area;
    if (!sceneRect.isValid()) {
        sceneRect = QRect();
        D1GfxFrame *frame = this->getCurrentFrame();
        if (frame != nullptr) {
            sceneRect.setWidth(frame->getWidth());
            sceneRect.setHeight(frame->getHeight());
        }
    }
    sceneRect.adjust(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN, CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    QPolygon poly = this->graphView->mapFromScene(sceneRect);
    QRect vpRect = poly.boundingRect();
    QPoint globalTopLeft = this->graphView->viewport()->mapToGlobal(vpRect.topLeft());
    QPoint globalBottomRight = this->graphView->viewport()->mapToGlobal(vpRect.bottomRight());
    QPoint topLeft = this->parentWidget()->mapFromGlobal(globalTopLeft);
    QPoint bottomRight = this->parentWidget()->mapFromGlobal(globalBottomRight);
    this->rubberBand->setGeometry(QRect(topLeft, bottomRight));
    this->rubberBand->show();
}

bool PaintWidget::frameClicked(D1GfxFrame *frame, const QPoint &pos, int flags)
{
    if (this->isHidden()) {
        return false;
    }

    this->ui->currPosLabel->setText(QString("(%1:%2)").arg(pos.x()).arg(pos.y()));

    if (this->ui->selectModeRadioButton->isChecked()) {
        // select mode
        if (flags & FIRST_CLICK) {
            if (this->rubberBand && this->selectionMoveMode != 1) {
                QPoint globalCursorPos = QCursor::pos();
                QRect rubberBandRect = this->rubberBand->geometry();
                QPoint globalTopLeft = this->parentWidget()->mapToGlobal(rubberBandRect.topLeft());
                QPoint globalBottomRight = this->parentWidget()->mapToGlobal(rubberBandRect.bottomRight());
                QRect globalRubberBandRect = QRect(globalTopLeft, globalBottomRight);
                if (globalRubberBandRect.contains(globalCursorPos)) {
                    if (this->selectionMoveMode == 0) {
                        this->movePos = pos;
                        this->lastMoveCmd = nullptr;
                        this->lastDelta = QPoint(0, 0);
                    }
                    this->selectionMoveMode = 1;
                    return false;
                }
            }
            this->lastPos = pos;
            MemFree(this->rubberBand);
            this->lastMoveCmd = nullptr;
            this->lastDelta = QPoint(0, 0);
            this->selectionMoveMode = 0;
            return false;
        }
        if (this->selectionMoveMode != 0) {
            if (this->lastMoveCmd != nullptr && this->undoStack->count() != 0 && this->undoStack->command(this->undoStack->count() - 1) == this->lastMoveCmd) {
                this->undoStack->undo();
                this->lastMoveCmd->setObsolete(true);
            }
            this->selectionMoveMode = 2;

            QRect area = this->getSelectArea(frame);

            std::vector<FramePixel> pixels;
            for (int x = area.left(); x <= area.right(); x++) {
                for (int y = area.top(); y <= area.bottom(); y++) {
                    pixels.push_back(FramePixel(QPoint(x, y), D1GfxPixel::transparentPixel()));
                }
            }
            QPoint delta = pos - this->movePos;
            for (int x = area.left(); x <= area.right(); x++) {
                for (int y = area.top(); y <= area.bottom(); y++) {
                    QPoint tp = QPoint(x, y) + delta;
                    if (tp.x() < 0 || tp.x() >= frame->getWidth()) {
                        continue;
                    }
                    if (tp.y() < 0 || tp.y() >= frame->getHeight()) {
                        continue;
                    }
                    D1GfxPixel pixel = frame->getPixel(x, y);
                    if (pixel.isTransparent()) {
                        continue;
                    }

                    unsigned n = 0;
                    for (; n < pixels.size(); n++) {
                        if (pixels[n].pos == tp) {
                            pixels[n].pixel = pixel;
                            break;
                        }
                    }
                    if (n < pixels.size()) {
                        continue;
                    }
                    pixels.push_back(FramePixel(tp, pixel));
                }
            }
            this->lastDelta = delta;
            area.translate(delta);
            this->selectArea(area);

            // Build frame editing command and connect it to the current main window widget
            // to update the palHits and CEL views when undo/redo is performed
            EditFrameCommand *command = new EditFrameCommand(frame, pixels);

            this->lastMoveCmd = command;
            this->undoStack->push(command);
            return true;
        }

        if (this->rubberBand == nullptr) {
            this->rubberBand = new QRubberBand(QRubberBand::Rectangle, this->parentWidget());
        }
        this->currPos = pos;
        this->selectArea(getArea(this->currPos, this->lastPos));
        return true;
    }
    MemFree(this->rubberBand);
    this->lastMoveCmd = nullptr;
    this->lastDelta = QPoint(0, 0);
    this->selectionMoveMode = 0;

    QPoint destPos = pos;
    std::vector<FramePixel> allPixels;
    if (this->ui->fillModeRadioButton->isChecked()) {
        // fill mode
        this->collectPixels(frame, destPos, allPixels);
    } else {
        // draw mode
        void (PaintWidget::*squareCollectorFunc)(int, int, int, std::vector<FramePixel> &) = &PaintWidget::collectPixelsSquare;
        void (PaintWidget::*roundCollectorFunc)(int, int, int, std::vector<FramePixel> &) = &PaintWidget::collectPixelsRound;
        auto collectorFunc = this->ui->squareShapeRadioButton->isChecked() ? squareCollectorFunc : roundCollectorFunc;

        if (flags & FIRST_CLICK) {
            this->distance = 0;
            (this->*collectorFunc)(destPos.x(), destPos.y(), this->distance, allPixels);
        } else {
            // adjust destination if gradient is set
            QString xGradient = this->ui->gradientXLineEdit->text();
            QString yGradient = this->ui->gradientYLineEdit->text();
            if (!xGradient.isEmpty() || !yGradient.isEmpty()) {
                int gx = xGradient.toInt();
                int gy = yGradient.toInt();
                QPoint dPos = pos - this->currPos;
                bool sx = (gx < 0) == (dPos.x() < 0);
                bool sy = (gy < 0) == (dPos.y() < 0);
                if (gx == 0) {
                    if (gy == 0) {
                        return true; // lock if gradient is set to 0:0
                    }
                    // check for exact match if gradient is completely set
                    if (!xGradient.isEmpty() && !sy) {
                        return true;
                    }
                    dPos.setX(0);
                    int dy = (dPos.y() / gy) * gy;
                    dPos.setY(dy);
                    // 'step back' to let the user do equal advances
                    if (this->distance == 0 && dy != 0 && abs(gy) > 1) {
                        this->currPos.ry() += dy < 0 ? 1 : -1;
                        this->distance = -1;
                    }
                } else if (gy == 0) {
                    // check for exact match if gradient is completely set
                    if (!yGradient.isEmpty() && !sx) {
                        return true;
                    }
                    dPos.setY(0);
                    int dx = (dPos.x() / gx) * gx;
                    dPos.setX(dx);
                    // 'step back' to let the user do equal advances
                    if (this->distance == 0 && dx != 0 && abs(gx) > 1) {
                        this->currPos.rx() += dx < 0 ? 1 : -1;
                        this->distance = -1;
                    }
                } else {
                    // if (sx != sy) {
                    if (!sx || !sy) {
                        return true;
                    }
                    int nx = dPos.x() / gx;
                    int ny = dPos.y() / gy;
                    if (sx) {
                        nx = std::min(nx, ny);
                    } else {
                        nx = std::max(nx, ny);
                    }
                    int dx = nx * gx;
                    int dy = nx * gy;
                    dPos.setX(dx);
                    dPos.setY(dy);
                    // TODO: 'step back' to let the user do equal advances?
                }

                destPos = this->currPos + dPos;
            }

            (this->*collectorFunc)(this->currPos.x(), this->currPos.y(), 0, allPixels);
            unsigned n = allPixels.size();

            traceClick(this->currPos, destPos, allPixels, collectorFunc);

            allPixels.erase(allPixels.begin(), allPixels.begin() + n);

            QPoint dPos = destPos - this->currPos;
            this->distance += std::max(abs(dPos.x()), abs(dPos.y()));
        }
    }
    this->currPos = destPos;
    // filter pixels
    std::vector<FramePixel> pixels;
    for (const FramePixel &framePixel : allPixels) {
        if (framePixel.pos.x() < 0 || framePixel.pos.x() >= frame->getWidth()) {
            continue;
        }
        if (framePixel.pos.y() < 0 || framePixel.pos.y() >= frame->getHeight()) {
            continue;
        }
        if (framePixel.pixel == frame->getPixel(framePixel.pos.x(), framePixel.pos.y())) {
            continue;
        }
        pixels.push_back(framePixel);
    }

    if (pixels.empty()) {
        return true;
    }

    // Build frame editing command and connect it to the current main window widget
    // to update the palHits and CEL views when undo/redo is performed
    EditFrameCommand *command = new EditFrameCommand(frame, pixels);

    this->undoStack->push(command);
    return true;
}

void PaintWidget::selectColor(const D1GfxPixel &pixel)
{
    this->selectedColors.clear();
    if (!pixel.isTransparent()) {
        this->selectedColors.push_back(pixel.getPaletteIndex());
    }
    // update the view
    this->colorModified();
}

void PaintWidget::palColorsSelected(const std::vector<quint8> &indices)
{
    this->selectedColors = indices;
    // update the view
    this->colorModified();
}

void PaintWidget::colorModified()
{
    if (this->isHidden())
        return;
    // update the color-icon
    QSize imageSize = this->ui->imageLabel->size();
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
    this->ui->imageLabel->setPixmap(pixmap);
}

void PaintWidget::on_closePushButtonClicked()
{
    this->hide();
}

void PaintWidget::stopMove()
{
    this->setMouseTracking(false);
    this->moving = false;
    this->moved = true;
    this->releaseMouse();
    // this->setCursor(Qt::CrossCursor);
}

void PaintWidget::on_movePushButtonClicked()
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

void PaintWidget::keyPressEvent(QKeyEvent *event)
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

void PaintWidget::mousePressEvent(QMouseEvent *event)
{
    if (this->moving) {
        this->stopMove();
        return;
    }

    QFrame::mouseMoveEvent(event);
}

void PaintWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (this->moving) {
        QPoint cursorPos = QCursor::pos();
        QPoint wndPos = this->pos();
        wndPos += cursorPos - this->lastPos;
        this->lastPos = cursorPos;
        this->move(wndPos);
        // return;
    }

    QFrame::mouseMoveEvent(event);
}

void PaintWidget::on_brushWidthDecButton_clicked()
{
    if (this->brushWidth > 1) {
        this->brushWidth = (this->brushWidth - 2) | 1;
        this->on_brushWidthLineEdit_escPressed();
    }
}

void PaintWidget::on_brushWidthIncButton_clicked()
{
    this->brushWidth = (this->brushWidth + 1) | 1;
    this->on_brushWidthLineEdit_escPressed();
}

void PaintWidget::on_brushWidthLineEdit_returnPressed()
{
    this->brushWidth = this->ui->brushWidthLineEdit->text().toUInt();
    if (this->brushWidth == 0) {
        this->brushWidth = 1;
    }
    this->on_brushWidthLineEdit_escPressed();
}

void PaintWidget::on_brushWidthLineEdit_escPressed()
{
    this->ui->brushWidthLineEdit->setText(QString::number(this->brushWidth));
    this->ui->brushWidthLineEdit->clearFocus();
}

void PaintWidget::on_brushLengthDecButton_clicked()
{
    if (this->brushLength > 1) {
        this->brushLength--;
        this->on_brushLengthLineEdit_escPressed();
    }
}

void PaintWidget::on_brushLengthIncButton_clicked()
{
    this->brushLength++;
    this->on_brushLengthLineEdit_escPressed();
}

void PaintWidget::on_brushLengthLineEdit_returnPressed()
{
    this->brushLength = this->ui->brushLengthLineEdit->text().toUInt();
    if (this->brushLength == 0) {
        this->brushLength = 1;
    }
    this->on_brushLengthLineEdit_escPressed();
}

void PaintWidget::on_brushLengthLineEdit_escPressed()
{
    this->ui->brushLengthLineEdit->setText(QString::number(this->brushLength));
    this->ui->brushLengthLineEdit->clearFocus();
}

void PaintWidget::on_gradientClearPushButton_clicked()
{
    this->ui->gradientXLineEdit->clear();
    this->ui->gradientYLineEdit->clear();
}

void PaintWidget::on_tilesetMaskPushButton_clicked()
{
    D1CEL_FRAME_TYPE maskType = this->ui->tilesetMaskComboBox->currentData().value<D1CEL_FRAME_TYPE>();
    int frameIndex = this->levelCelView->getCurrentFrameIndex();
    if (this->gfx->getFrameCount() <= frameIndex) {
        return;
    }
    D1GfxFrame *frame = this->gfx->getFrame(frameIndex);
    if (frame->getWidth() != MICRO_WIDTH) {
        static_assert(MICRO_WIDTH == 32, "PaintWidget::on_tilesetMaskPushButton_clicked uses hardcoded width.");
        QMessageBox::critical(this, tr("Error"), tr("Frame width is not 32px."));
        return;
    }
    if (frame->getHeight() != MICRO_HEIGHT) {
        static_assert(MICRO_HEIGHT == 32, "PaintWidget::on_tilesetMaskPushButton_clicked uses hardcoded height.");
        QMessageBox::critical(this, tr("Error"), tr("Frame height is not 32px."));
        return;
    }
    // collect the pixels which need to be replaced
    std::vector<FramePixel> pixels;
    D1CelTilesetFrame::collectPixels(frame, maskType, pixels);

    if (pixels.empty()) {
        return;
    }

    // check if replacement color is necessary
    bool needsColor = false;
    for (const FramePixel framePixel : pixels) {
        if (framePixel.pixel.isTransparent()) {
            needsColor = true;
            break;
        }
    }
    unsigned numColors = this->selectedColors.size();
    if (needsColor && numColors == 0) {
        QMessageBox::warning(this, tr("Warning"), tr("Select a color-index from the palette to use."));
        return;
    }

    // replace the pixels
    unsigned cursor = 0;
    for (FramePixel &framePixel : pixels) {
        if (framePixel.pixel.isTransparent()) {
            framePixel.pixel = D1GfxPixel::colorPixel(this->selectedColors[cursor]);
            cursor = (cursor + 1) % numColors;
        } else {
            framePixel.pixel = D1GfxPixel::transparentPixel();
        }
    }

    // Build frame editing command and connect it to the current main window widget
    // to update the palHits and CEL views when undo/redo is performed
    EditFrameCommand *command = new EditFrameCommand(frame, pixels);

    this->undoStack->push(command);

    // change the frame-type
    frame->setFrameType(maskType);
}
