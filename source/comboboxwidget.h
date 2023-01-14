#pragma once

#include <QComboBox>
#include <QFocusEvent>
#include <QKeyEvent>

#include "lineeditwidget.h"

class ComboBoxWidget : public QComboBox {
    Q_OBJECT

public:
    explicit ComboBoxWidget(QWidget *parent);
    ~ComboBoxWidget() = default;

    void setCharWidth(int width);
};
