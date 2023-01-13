#include "lineeditwidget.h"

#include <QFontMetrics>

LineEditWidget::LineEditWidget(QWidget *parent)
    : QLineEdit(parent)
{
}

void LineEditWidget::setCharWidth(int value)
{
    const int maxWidth = QFontMetrics(this->font()).maxWidth();

    maxWidth *= value;

    const QMargins margins = this->contentsMargins();

    maxWidth += margins.left() + margins.right();

    this->setMinimumWidth(maxWidth);
    this->setMaximumWidth(maxWidth);
}
