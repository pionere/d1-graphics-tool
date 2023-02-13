#include "pushbuttonwidget.h"

#include <QColor>
#include <QPainter>

#include "config.h"

#define ICON_SIZE 16
#define SELECTION_WIDTH 1

PushButtonWidget::PushButtonWidget(QLayout *parent, QStyle::StandardPixmap type, const QString &tooltip, const QObject *receiver, PointerToMemberFunction callback)
    : QPushButton("", nullptr)
{
    this->setIcon(this->style()->standardIcon(type));
    this->setToolTip(tooltip);
    this->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    this->setMinimumSize(ICON_SIZE + SELECTION_WIDTH * 2, ICON_SIZE + SELECTION_WIDTH * 2);
    this->setMaximumSize(ICON_SIZE + SELECTION_WIDTH * 2, ICON_SIZE + SELECTION_WIDTH * 2);
    parent->addWidget(this);

    QObject::connect(this, &QPushButton::clicked, receiver, callback);
}

void PushButtonWidget::addButton(QLayout *parent, QStyle::StandardPixmap type, const QString &tooltip, const QObject *receiver, PointerToMemberFunction callback)
{
    return new PushButtonWidget(parent, type, tooltip, receiver, callback);
}

void PushButtonWidget::paintEvent(QPaintEvent *event)
{
    if (this->inFocus) {
        QPainter painter(this);
        QColor borderColor = QColor(Config::getPaletteSelectionBorderColor());

        int width = this->size().width();
        int height = this->size().height();

        painter.fillRect(0, 0, width, height, borderColor);
    }

    PushButtonWidget::paintEvent(event);
}

void PushButtonWidget::focusInEvent(QFocusEvent *event)
{
    this->inFocus = true;

    PushButtonWidget::focusInEvent(event);
}

void PushButtonWidget::enterEvent(QEvent *event)
{
    this->inFocus = true;

    PushButtonWidget::enterEvent(event);
}

void PushButtonWidget::leaveEvent(QEvent *event)
{
    this->inFocus = false;

    PushButtonWidget::leaveEvent(event);
}

void PushButtonWidget::focusOutEvent(QEvent *event)
{
    this->inFocus = false;

    PushButtonWidget::focusOutEvent(event);
}
