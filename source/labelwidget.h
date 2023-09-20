#pragma once

#include <QLabel>

class LabelWidget : public QLabel {
    Q_OBJECT

public:
    explicit LabelWidget(QWidget *parent);
    ~LabelWidget() = default;

    void setCharWidth(int width);
};
