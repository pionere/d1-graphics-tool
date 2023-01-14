#pragma once

#include <QComboBox>
#include <QFocusEvent>
#include <QKeyEvent>

class ComboBoxWidget : public QComboBox {
    Q_OBJECT

public:
    explicit ComboBoxWidget(QWidget *parent);
    ~ComboBoxWidget() = default;

    void setCharWidth(int width);

signals:
    void cancel_signal();

protected:
    void keyPressEvent(QKeyEvent *event);
    void focusOutEvent(QFocusEvent *event);
};
