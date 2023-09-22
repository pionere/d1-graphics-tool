#pragma once

#include <QLabel>

class LabelWidget : public QLabel {
    Q_OBJECT

public:
    LabelWidget(QWidget *parent = nullptr);
    LabelWidget(const QString &text, QWidget *parent = nullptr);
    ~LabelWidget() = default;

    void setCharWidth(int width);
};
