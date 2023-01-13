#include "lineeditwidget.h"

#include <QFontMetrics>

LineEditWidget::LineEditWidget(QWidget *parent)
    : QLineEdit(parent)
{
}

void LineEditWidget::setCharWidth(int value)
{
    int maxWidth = this->fontMetrics().horizontalAdvance('w');

    maxWidth *= value;

    const QMargins margins = this->contentsMargins();

    maxWidth += margins.left() + margins.right();

    this->setMinimumWidth(maxWidth);
    this->setMaximumWidth(maxWidth);
}

void LineEditWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit cancel_signal();
    }

    QLineEdit::keyPressEvent(event);
}

void YourButton::focusOutEvent(QFocusEvent *event)
{
    if (e->reason() != Qt::OtherFocusReason) {
        if (e->reason() == Qt::TabFocusReason) {
            // submit on tabpress
            QKeyEvent *kpEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter);
            QCoreApplication::postEvent(this, kpEvent);
        } else {
            // cancel otherwise
            emit cancel_signal();
        }
    }

    QLineEdit::focusOutEvent(e);
}