#include "paintwidget.h"

#include <QCursor>
#include <QGraphicsView>
#include <QImage>
#include <QList>
#include <QMessageBox>
#include <QString>

#include "config.h"
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

    emit this->modified();
}

void EditFrameCommand::redo()
{
    this->undo();
}

PaintWidget::PaintWidget(QWidget *parent, QUndoStack *us, D1Gfx *g, CelView *cv, LevelCelView *lcv)
    : QFrame(parent)
    , ui(new Ui::PaintWidget())
    , undoStack(us)
    , gfx(g)
    , celView(cv)
    , levelCelView(lcv)
    , moving(false)
    , moved(false)
{
    this->ui->setupUi(this);

    QLayout *layout = this->ui->centerButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogListView, tr("Move"), this, &PaintWidget::on_movePushButtonClicked);
    layout = this->ui->rightButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogCloseButton, tr("Close"), this, &PaintWidget::on_closePushButtonClicked);

    // initialize the edit fields
    this->on_brushWidthLineEdit_escPressed();
    this->on_brushLengthLineEdit_escPressed();

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
    this->graphView = views[0];
}

PaintWidget::~PaintWidget()
{
    delete ui;
}

void PaintWidget::setPalette(D1Pal *p)
{
    this->pal = p;
}

void PaintWidget::show()
{
    if (!this->moved) {
        QSize viewSize = this->graphView->frameSize();
        QPoint viewBottomRight = this->graphView->mapToGlobal(QPoint(viewSize.width(), viewSize.height()));
        QSize mySize = this->frameSize();
        QPoint targetPos = viewBottomRight - QPoint(mySize.width(), mySize.height());
        this->move(this->mapFromGlobal(targetPos));
    }
    QFrame::show();

    this->setFocus(); // otherwise the widget does not receive keypress events...
    this->graphView->setCursor(Qt::CrossCursor);
    // update the view
    this->colorModified();
}

void PaintWidget::hide()
{
    this->stopMove();
    this->graphView->unsetCursor();

    QFrame::hide();
}

D1GfxPixel PaintWidget::getCurrentColor(unsigned counter) const
{
    unsigned numColors = this->selectedColors.size();
    if (numColors == 0) {
        return D1GfxPixel::transparentPixel();
    }
    return D1GfxPixel::colorPixel(this->selectedColors[(counter / this->brushLength) % numColors]);
}

void PaintWidget::collectPixels(int baseX, int baseY, int baseDist, std::vector<FramePixel> &pixels)
{
    int width = this->brushWidth;
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

/**
 * Collect pixel-locations in the (startPos:destPos] range.
 *
 * @param startPos: the starting position
 * @param destPos: the destination
 * @param pixels: the container for the collected locations.
 */
void PaintWidget::traceClick(const QPoint &startPos, const QPoint &destPos, std::vector<FramePixel> &pixels)
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
            this->collectPixels(x1, y1, dist, pixels);
            // pixels.push_back(FramePixel(QPoint(x1, y1), this->getCurrentColor(dist)));
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
            this->collectPixels(x1, y1, dist, pixels);
            // pixels.push_back(FramePixel(QPoint(x1, y1), this->getCurrentColor(dist)));
            if (y1 == y2)
                break;
        }
    }
}

bool PaintWidget::frameClicked(D1GfxFrame *frame, const QPoint &pos, unsigned counter)
{
    if (this->isHidden() || this->ui->pickModeRadioButton->isChecked()) {
        return false;
    }

    QPoint destPos = pos;
    std::vector<FramePixel> allPixels;
    if (counter == 0) {
        this->distance = 0;
        this->collectPixels(destPos.x(), destPos.y(), this->distance, allPixels);
        // allPixels.push_back(FramePixel(destPos, this->getCurrentColor(this->distance)));
    } else {
        // adjust destination if gradient is set
        QString xGradient = this->ui->gradientXLineEdit->text();
        QString yGradient = this->ui->gradientYLineEdit->text();
        if (!xGradient.isEmpty() || !yGradient.isEmpty()) {
            int gx = xGradient.toInt();
            int gy = yGradient.toInt();
            QPoint dPos = pos - this->lastPos;
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
                    this->lastPos.ry() += dy < 0 ? 1 : -1;
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
                    this->lastPos.rx() += dx < 0 ? 1 : -1;
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

            destPos = this->lastPos + dPos;
        }

        this->collectPixels(this->lastPos.x(), this->lastPos.y(), 0, allPixels);
        unsigned n = allPixels.size();

        traceClick(this->lastPos, destPos, allPixels);

        allPixels.erase(allPixels.begin(), allPixels.begin() + n);

        QPoint dPos = destPos - this->lastPos;
        this->distance += std::max(abs(dPos.x()), abs(dPos.y()));
    }
    this->lastPos = destPos;
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
    QObject::connect(command, &EditFrameCommand::modified, &dMainWindow(), &MainWindow::frameModified);

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
                image.setPixelColor(x, y, color);
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
        QPoint currPos = QCursor::pos();
        QPoint wndPos = this->pos();
        wndPos += currPos - this->lastPos;
        this->lastPos = currPos;
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
    if (this->gfx->getFrameCount() == 0) {
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
    QObject::connect(command, &EditFrameCommand::modified, &dMainWindow(), &MainWindow::frameModified);

    this->undoStack->push(command);

    // change the frame-type
    frame->setFrameType(maskType);
}
