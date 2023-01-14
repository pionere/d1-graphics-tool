#include "comboboxwidget.h"

#include <QCoreApplication>
#include <QFontMetrics>

ComboBoxWidget::ComboBoxWidget(QWidget *parent)
    : QComboBox(parent)
{
}

void ComboBoxWidget::setCharWidth(int value)
{
    int maxWidth = this->fontMetrics().horizontalAdvance('w');

    maxWidth *= value;

    QStyleOptionComboBox opt;
    opt.initFrom(this);
    const QRect rc = this->style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, this);
    maxWidth += rc.width();

    const QMargins margins = this->contentsMargins();

    maxWidth += margins.left() + margins.right();

    this->setMinimumWidth(maxWidth);
    this->setMaximumWidth(maxWidth);
}
