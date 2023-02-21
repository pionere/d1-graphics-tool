#include "paintdialog.h"

#include <QCursor>
#include <QImage>

#include "config.h"
#include "d1pal.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_paintdialog.h"

PaintDialog::PaintDialog(QWidget *parent)
    : QDialog(parent, Qt::SubWindow | Qt::FramelessWindowHint)
    , ui(new Ui::PaintDialog())
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

void PaintDialog::initialize(D1Tileset *ts)
{
    this->moving = false;
    this->isTileset = ts != nullptr;
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
