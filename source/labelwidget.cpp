#include "labelwidget.h"

#include <QFontMetrics>

LabelWidget::LabelWidget(QWidget *parent)
    : QLabel(parent)
{
}

LabelWidget::LabelWidget(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
}

void LabelWidget::setCharWidth(int value)
{
    int maxWidth = this->fontMetrics().horizontalAdvance('w');

    maxWidth *= value;

    const QMargins margins = this->contentsMargins();

    maxWidth += margins.left() + margins.right();

    this->setFixedWidth(maxWidth);
}
