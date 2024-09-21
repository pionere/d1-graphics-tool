#pragma once

#include <QSlider>

class SliderWidget : public QSlider {
    Q_OBJECT

public:
    SliderWidget(QWidget *parent = nullptr);
    ~SliderWidget() = default;

    void changeValue(int value);
};
