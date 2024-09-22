#pragma once

#include "sliderwidget.h"

class AffixSliderWidget : public SliderWidget {
    Q_OBJECT

public:
    AffixSliderWidget(QWidget *parent = nullptr);
    ~AffixSliderWidget() = default;

    void changeValue(int value);
    void setLimitMode(int mode);

private slots:
    void on_valueChanged(int value);

private:
    void updateToolTip();

    int limitMode = 0;
};
