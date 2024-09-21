#include "sliderwidget.h"

SliderWidget::SliderWidget(QWidget *parent)
    : QSlider(parent)
{
}

void SliderWidget::changeValue(int value)
{
    this->blockSignals(true);
    this->setValue(value);
    this->blockSignals(false);
}
