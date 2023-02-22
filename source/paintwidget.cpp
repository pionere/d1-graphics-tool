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

PaintWidget::PaintWidget(QWidget *parent, QUndoStack *us, CelView *cv, LevelCelView *lcv)
    : QFrame(parent)
    , ui(new Ui::PaintWidget())
    , undoStack(us)
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

    // assume the first color is selected on the palette-widget
    this->selectedColors.append(0);

    // listen to keyboard events
    dMainWindow().installEventFilter(this);
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
    // redraw...
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
    // this->setCursor(Qt::ClosedHandCursor);
}

bool PaintWidget::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            if (this->moving) {
                this->stopMove();
            } else {
                this->hide();
            }
            return true;
        }
    }
    if (event->type() == QEvent::MouseButtonPress && this->moving) {
        this->stopMove();
        return true;
    }
    return QFrame::eventFilter(object, event);
}

void PaintWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (this->moving) {
        QPoint currPos = QCursor::pos();
        QPoint wndPos = this->pos();
        wndPos += currPos - this->lastPos;
        this->lastPos = currPos;
        this->move(wndPos);
    }

    QFrame::mouseMoveEvent(event);
}
