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

int LineEditWidget::nonNegInt() const
{
    int result = this->text().toInt();
    if (result < 0) {
        result = 0;
    }
    return result;
}

std::pair<int, int> LineEditWidget::nonNegRange() const
{
    QStringList parts = this->text().split('-', Qt::SkipEmptyParts);
    std::pair<int, int> result = { 0, 0 };
    if (parts.size() == 1) {
        result.first = parts[0].asInt();
        result.second = result.first;
    } else if (parts.size() == 2) {
        result.first = parts[0].asInt();
        result.second = parts[1].asInt();
    }
    if (result.first < 0) {
        result.first = 0;
    }
    if (result.second < 0) {
        result.second = 0;
    }
    return result;
}

void LineEditWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit cancel_signal();
        return;
    }

    QLineEdit::keyPressEvent(event);
}

void LineEditWidget::focusOutEvent(QFocusEvent *event)
{
    if (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason /*|| event->reason() == Qt::ShortcutFocusReason*/) {
        // submit on tabpress
        emit returnPressed();
    } else {
        // cancel otherwise
        emit cancel_signal();
    }

    QLineEdit::focusOutEvent(event);
}