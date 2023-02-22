#include "paintdialog.h"

#include <QCursor>
#include <QImage>

#include "config.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_paintdialog.h"

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

PaintDialog::PaintDialog(QWidget *parent, QUndoStack *us, CelView *cv, LevelCelView *lcv)
    : QDialog(parent, Qt::Tool | Qt::FramelessWindowHint)
    , ui(new Ui::PaintDialog())
    , moving(false)
    , undoStack(us)
    , celView(cv)
    , levelCelView(lcv)
{
    this->ui->setupUi(this);

    QLayout *layout = this->ui->centerButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogListView, tr("Move"), this, &PaintDialog::on_movePushButtonPressed, &PaintDialog::on_movePushButtonReleased);
    layout = this->ui->rightButtonsHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogCloseButton, tr("Close"), this, &PaintDialog::on_closePushButtonClicked);

    // assume the first color is selected on the palette-widget
    this->selectedColors.append(0);
}

PaintDialog::~PaintDialog()
{
    delete ui;
}

void PaintDialog::setPalette(D1Pal *p)
{
    this->pal = p;
}

void PaintDialog::show()
{
    QDialog::show();

    this->setCursor(Qt::CrossCursor);
    // update the view
    this->colorModified();
}

void PaintDialog::hide()
{
    QDialog::hide();

    this->unsetCursor();
}

D1GfxPixel PaintDialog::getCurrentColor(unsigned counter) const
{
    unsigned numColors = this->selectedColors.count();
    if (numColors == 0) {
        return D1GfxPixel::transparentPixel();
    }
    return D1GfxPixel::colorPixel(this->selectedColors[counter % numColors]);
}

void PaintDialog::frameClicked(D1GfxFrame *frame, const QPoint &pos, unsigned counter)
{
    D1GfxPixel pixel = this->getCurrentColor(counter);

    // Build frame editing command and connect it to the current main window widget
    // to update the palHits and CEL views when undo/redo is performed
    EditFrameCommand *command = new EditFrameCommand(frame, pos, pixel);
    QObject::connect(command, &EditFrameCommand::modified, &dMainWindow(), &MainWindow::frameModified);

    this->undoStack->push(command);
}

void PaintDialog::selectColor(const D1GfxPixel &pixel)
{
    this->selectedColors.clear();
    if (!pixel.isTransparent()) {
        this->selectedColors.append(pixel.getPaletteIndex());
    }
    // update the view
    this->colorModified();
}

void PaintDialog::palColorsSelected(const QList<quint8> &indices)
{
    this->selectedColors = indices;
    // update the view
    this->colorModified();
}

void PaintDialog::colorModified()
{
    if (!this->isVisible())
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

void PaintDialog::on_closePushButtonClicked()
{
    this->hide();
}

void PaintDialog::on_movePushButtonPressed()
{
    this->moving = true;
    this->lastPos = QCursor::pos();
    this->setCursor(Qt::ClosedHandCursor);
}

void PaintDialog::on_movePushButtonReleased()
{
    this->moving = false;
    this->setCursor(Qt::CrossCursor);
}

void PaintDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (this->moving) {
        QPoint currPos = QCursor::pos();
        QPoint wndPos = this->pos();
        wndPos += currPos - this->lastPos;
        this->lastPos = currPos;
        this->move(wndPos);
    }

    PaintDialog::mouseMoveEvent(event);
}
