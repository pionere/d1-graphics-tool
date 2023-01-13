#pragma once

#include <QKeyEvent>
#include <QLineEdit>

class LineEditWidget : public QLineEdit {
    Q_OBJECT

public:
    explicit LineEditWidget(QWidget *parent);
    ~LineEditWidget() = default;

    void setCharWidth(int width);

signals:
    void cancel_signal();

protected:
    void keyPressEvent(QKeyEvent *event);
};
