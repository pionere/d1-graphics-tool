#pragma once

#include <QLineEdit>

class LineEditWidget : public QLineEdit {
    Q_OBJECT

public:
    explicit LineEditWidget(QWidget *parent);
    ~LineEditWidget() = default;

    void setCharWidth(int width);
};
