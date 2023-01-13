#pragma once

#include <QLineEdit>

class LineEditWidget : public QLineEdit {
    Q_OBJECT

public:
    void setCharWidth(int width);
};
