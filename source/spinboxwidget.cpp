#include "spinboxwidget.h"

SpinBoxWidget::SpinBoxWidget(QWidget *parent)
    : QSpinBox(parent)
{
}

void SpinBoxWidget::changeValue(int value)
{
    this->blockSignals(true);
    this->setValue(value);
    this->blockSignals(false);
}
