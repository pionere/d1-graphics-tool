#include "paintwidget.h"

#include <QCursor>
#include <QGraphicsView>
#include <QImage>
#include <QList>
#include <QMessageBox>

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
        this->frame->setPixel(fp.pos.x(), fp.pos.y(), fp.pixel);
        fp.pixel = pixel;
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

    // prepare combobox of the masks
    if (lcv != nullptr) {
        this->ui->tilesetMaskWidget->setVisible(true);
        this->ui->tilesetMaskComboBox->addItem(tr("Square"), QVariant::fromValue(D1CEL_FRAME_TYPE::Square));
        this->ui->tilesetMaskComboBox->addItem(tr("Left Triangle"), QVariant::fromValue(D1CEL_FRAME_TYPE::LeftTriangle));
        this->ui->tilesetMaskComboBox->addItem(tr("Right Triangle"), QVariant::fromValue(D1CEL_FRAME_TYPE::RightTriangle));
        this->ui->tilesetMaskComboBox->addItem(tr("Left Trapezoid"), QVariant::fromValue(D1CEL_FRAME_TYPE::LeftTrapezoid));
        this->ui->tilesetMaskComboBox->addItem(tr("Right Trapezoid"), QVariant::fromValue(D1CEL_FRAME_TYPE::RightTrapezoid));
        this->adjustSize();
    }

    // assume the first color is selected on the palette-widget
    this->selectedColors.push_back(0);
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
    unsigned numColors = this->selectedColors.size();
    if (numColors == 0) {
        return D1GfxPixel::transparentPixel();
    }
    return D1GfxPixel::colorPixel(this->selectedColors[counter % numColors]);
}

void PaintWidget::frameClicked(D1GfxFrame *frame, const QPoint &pos, unsigned counter)
{
    D1GfxPixel pixel = this->getCurrentColor(counter);

    if (counter == 0 && pixel == frame->getPixel(pos.x(), pos.y())) {
        return;
    }

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
