#include "pushbuttonwidget.h"

#include <QApplication>
#include <QColor>
#include <QPainter>

#include "config.h"

#define ICON_SIZE 16
#define SELECTION_WIDTH 2

PushButtonWidget::PushButtonWidget(QLayout *parent, QStyle::StandardPixmap type, const QString &tooltip)
    : QPushButton(QApplication::style()->standardIcon(type), "", nullptr)
{
    this->setToolTip(tooltip);
    this->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    this->setMinimumSize(ICON_SIZE + SELECTION_WIDTH * 2, ICON_SIZE + SELECTION_WIDTH * 2);
    this->setMaximumSize(ICON_SIZE + SELECTION_WIDTH * 2, ICON_SIZE + SELECTION_WIDTH * 2);
    parent->addWidget(this);
}

void PushButtonWidget::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);

    if (this->inFocus) {
        QPainter painter(this);
        QColor borderColor = QColor(Config::getPaletteSelectionBorderColor());
        QPen pen(borderColor);
        pen.setWidth(SELECTION_WIDTH);
        painter.setPen(pen);

        QSize size = this->size();
        QRect rect = (0, 0, size.width(), size.height());
        rect.adjust(0, 0, -SELECTION_WIDTH, -SELECTION_WIDTH);
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

void PushButtonWidget::focusInEvent(QFocusEvent *event)
{
    this->inFocus = true;

    QPushButton::focusInEvent(event);
}

void PushButtonWidget::enterEvent(QEnterEvent *event)
{
    this->inFocus = true;

    QPushButton::enterEvent(event);
}

void PushButtonWidget::focusOutEvent(QFocusEvent *event)
{
    this->inFocus = false;

    QPushButton::focusOutEvent(event);
}

void PushButtonWidget::leaveEvent(QEvent *event)
{
    this->inFocus = false;

    QPushButton::leaveEvent(event);
}
