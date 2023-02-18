#pragma once

#include <utility>

#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineEdit>

class LineEditWidget : public QLineEdit {
    Q_OBJECT

public:
    explicit LineEditWidget(QWidget *parent);
    ~LineEditWidget() = default;

    void setCharWidth(int width);

    int nonNegInt() const;
    std::pair<int, int> nonNegRange() const;

signals:
    void cancel_signal();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
};
