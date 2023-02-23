#include "paintwidget.h"

#include <QCursor>
#include <QGraphicsView>
#include <QImage>

#include "config.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_paintwidget.h"

FramePixel::FramePixel(const QPoint &p, D1GfxPixel px)
    : pos(p)
    , pixel(px)
{
}

EditFrameCommand::EditFrameCommand(D1GfxFrame *f, const QPoint &pos, D1GfxPixel newPixel)
    : QUndoCommand(nullptr)
    , frame(f)
{
    FramePixel fp(pos, newPixel);

    this->modPixels.append(fp);
}

EditFrameCommand::EditFrameCommand(D1GfxFrame *f, const std::vector<FramePixel> &pixels)
    : QUndoCommand(nullptr)
    , frame(f)
{
    for (unsigned i = 0; i < pixels.size(); i++) {
        this->modPixels.append(pixels[i]);
    }
}

void EditFrameCommand::undo()
{
    if (this->frame.isNull()) {
        this->setObsolete(true);
        return;
    }

    bool change = false;
    for (int i = 0; i < this->modPixels.count(); i++) {
        FramePixel &fp = this->modPixels[i];
        D1GfxPixel pixel = this->frame->getPixel(fp.pos.x(), fp.pos.y());
        if (pixel != fp.pixel) {
            this->frame->setPixel(fp.pos.x(), fp.pos.y(), fp.pixel);
            fp.pixel = pixel;
            change = true;
        }
    }

    if (change) {
        emit this->modified();
    }
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

    // prepare combobox of the masks
    this->ui->tilesetMaskHorizontalLayout->setVisible(lcv != nullptr);
    this->ui->tilesetMaskComboBox->addItem(tr("Square"), QVariant((int)D1CEL_FRAME_TYPE::Square));
    this->ui->tilesetMaskComboBox->addItem(tr("Left Triangle"), QVariant((int)D1CEL_FRAME_TYPE::LeftTriangle));
    this->ui->tilesetMaskComboBox->addItem(tr("Right Triangle"), QVariant((int)D1CEL_FRAME_TYPE::RightTriangle));
    this->ui->tilesetMaskComboBox->addItem(tr("Left Trapezoid"), QVariant((int)D1CEL_FRAME_TYPE::LeftTrapezoid));
    this->ui->tilesetMaskComboBox->addItem(tr("Right Trapezoid"), QVariant((int)D1CEL_FRAME_TYPE::RightTrapezoid));

    // assume the first color is selected on the palette-widget
    this->selectedColors.append(0);
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
        QGraphicsView *view;
        QList<QGraphicsView *> views;
        if (this->celView != nullptr) {
            views = this->celView->getCelScene()->views();
        }
        if (this->levelCelView != nullptr) {
            views = this->levelCelView->getCelScene()->views();
        }
        view = views[0];
        QSize viewSize = view->frameSize();
        QPoint viewBottomRight = view->mapToGlobal(QPoint(viewSize.width(), viewSize.height()));
        QSize mySize = this->frameSize();
        QPoint targetPos = viewBottomRight - QPoint(mySize.width(), mySize.height());
        this->move(this->mapFromGlobal(targetPos));
    }
    QFrame::show();

    this->setFocus(); // otherwise the widget does not receive keypress events...
    this->setCursor(Qt::CrossCursor);
    // update the view
    this->colorModified();
}

void PaintWidget::hide()
{
    this->stopMove();
    this->unsetCursor();

    QFrame::hide();
}

D1GfxPixel PaintWidget::getCurrentColor(unsigned counter) const
{
    unsigned numColors = this->selectedColors.count();
    if (numColors == 0) {
        return D1GfxPixel::transparentPixel();
    }
    return D1GfxPixel::colorPixel(this->selectedColors[counter % numColors]);
}

void PaintWidget::frameClicked(D1GfxFrame *frame, const QPoint &pos, unsigned counter)
{
    D1GfxPixel pixel = this->getCurrentColor(counter);

    // Build frame editing command and connect it to the current main window widget
    // to update the palHits and CEL views when undo/redo is performed
    EditFrameCommand *command = new EditFrameCommand(frame, pos, pixel);
    QObject::connect(command, &EditFrameCommand::modified, &dMainWindow(), &MainWindow::frameModified);

    this->undoStack->push(command);
}

void PaintWidget::selectColor(const D1GfxPixel &pixel)
{
    this->selectedColors.clear();
    if (!pixel.isTransparent()) {
        this->selectedColors.append(pixel.getPaletteIndex());
    }
    // update the view
    this->colorModified();
}

void PaintWidget::palColorsSelected(const QList<quint8> &indices)
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

    int numColors = this->selectedColors.count();
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

void PaintWidget::on_tilesetMaskPushButton_clicked()
{
    D1CEL_FRAME_TYPE maskType = this->ui->tilesetMaskComboBox->currentData().value<D1CEL_FRAME_TYPE>();
    int frameIndex = this->levelCelView->getCurrentFrameIndex();
    if (this->gfx->getFrameCount() == 0) {
        return;
    }
    D1GfxFrame *frame = this->gfx->getFrame(frameIndex);
    // collect the pixels which need to be replaced
    std::vector<FramePixel> pixels;
    D1CelTilesetFrame::collectPixels(frame, maskType, pixels);

    if (pixels.empty()) {
        return;
    }

    // select replacement color if necessary
    bool needsColor = false;
    for (const FramePixel framePixel : pixels) {
        if (framePixel.pixel.isTransparent()) {
            needsColor = true;
            break;
        }

        break;
    }
    quint8 paletteIndex = 0;
    if (needsColor) {
        if (this->selectedColors.count() != 1) {
            QMessageBox::warning(this, tr("Warning"), tr("Select a single color-index from the palette to use."));
            return;
        }
        paletteIndex = this->selectedColors[0];
    }

    // replace the pixels
    for (FramePixel &framePixel : pixels) {
        if (framePixel.pixel.isTransparent()) {
            framePixel.pixel = D1GfxPixel::colorPixel(paletteIndex);
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
