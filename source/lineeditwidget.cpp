#include "lineeditwidget.h"

#include <QCoreApplication>
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

void LineEditWidget::focusOutEvent(QFocusEvent *event)
{
    if (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason /*|| event->reason() == Qt::ShortcutFocusReason*/) {
        // submit on tabpress
        QKeyEvent *kpEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
        QCoreApplication::postEvent(this, kpEvent);
    } else {
        // cancel otherwise
        emit cancel_signal();
    }

    QLineEdit::focusOutEvent(event);
}