#include "pushbuttonwidget.h"

#include <QApplication>
#include <QColor>
#include <QPainter>

#include "config.h"

#define ICON_SIZE 16
#define SELECTION_WIDTH 1

PushButtonWidget::PushButtonWidget(QWidget *parent, QStyle::StandardPixmap type, const QString &tooltip)
    : QPushButton(QApplication::style()->standardIcon(type), "", parent)
{
    this->setToolTip(tooltip);
    this->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    // this->setMinimumSize(ICON_SIZE + SELECTION_WIDTH * 2, ICON_SIZE + SELECTION_WIDTH * 2);
    // this->setMaximumSize(ICON_SIZE + SELECTION_WIDTH * 2, ICON_SIZE + SELECTION_WIDTH * 2);
    this->setMinimumSize(ICON_SIZE, ICON_SIZE);
    this->setMaximumSize(ICON_SIZE, ICON_SIZE);
}

void PushButtonWidget::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);

    if (this->focusFlags != 0) {
        QPainter painter(this);
        QColor borderColor = QColor(Config::getPaletteSelectionBorderColor());
        QPen pen(borderColor);
        pen.setWidth(SELECTION_WIDTH);
        painter.setPen(pen);

        QSize size = this->size();
        QRect rect = QRect(0, 0, size.width(), size.height());
        // rect.adjust(0, 0, -SELECTION_WIDTH, -SELECTION_WIDTH);
        // - top line
        painter.drawLine(rect.left(), rect.top(), rect.right(), rect.top());
        // - bottom line
        painter.drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());
        // - left side
        painter.drawLine(rect.left(), rect.top(), rect.left(), rect.bottom());
        // - right side
        painter.drawLine(rect.right(), rect.top(), rect.right(), rect.bottom());
    }
}

void PushButtonWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QPushButton::mouseReleaseEvent(event);

    this->clearFocus();
}

void PushButtonWidget::focusInEvent(QFocusEvent *event)
{
    this->focusFlags |= 2;

    QPushButton::focusInEvent(event);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void PushButtonWidget::enterEvent(QEnterEvent *event)
#else
void PushButtonWidget::enterEvent(QEvent *event)
#endif
{
    this->focusFlags |= 1;

    QPushButton::enterEvent(event);
}

void PushButtonWidget::focusOutEvent(QFocusEvent *event)
{
    this->focusFlags &= ~2;

    QPushButton::focusOutEvent(event);
}

void PushButtonWidget::leaveEvent(QEvent *event)
{
    this->focusFlags &= ~1;

    QPushButton::leaveEvent(event);
}
