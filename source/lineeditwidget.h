#pragma once

#include <utility>

#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineEdit>

class LineEditWidget : public QLineEdit {
    Q_OBJECT

public:
    LineEditWidget(QWidget *parent);
    LineEditWidget(const QString &contents, QWidget *parent);
    ~LineEditWidget() = default;

    void setCharWidth(int width);

    int nonNegInt() const;
    std::pair<int, int> nonNegRange() const;

    void setReadOnly(bool ro);

signals:
    void cancel_signal();
    void focus_gain_signal();
    void focus_lost_signal();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
};
