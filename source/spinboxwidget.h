#pragma once

#include <QSpinBox>

class SpinBoxWidget : public QSpinBox {
    Q_OBJECT

public:
    SpinBoxWidget(QWidget *parent = nullptr);
    ~SpinBoxWidget() = default;

    void changeValue(int value);
};
