#pragma once

#include <QEvent>
#include <QFocusEvent>
#include <QLayout>
#include <QPaintEvent>
#include <QPushButton>
#include <QStyle>

class PushButtonWidget : public QPushButton {
    Q_OBJECT

    explicit PushButtonWidget(QLayout *parent, QStyle::StandardPixmap type, const QString &tooltip, const QObject *receiver, PointerToMemberFunction method);
public:
    ~PushButtonWidget() = default;

    static PushButtonWidget *addButton(QLayout *parent, QStyle::StandardPixmap type, const QString &tooltip, const QObject *receiver, PointerToMemberFunction method);
signals:
    void cancel_signal();

protected:
    void paintEvent(QPaintEvent *event);
    void enterEvent(QEvent *event);
    void focusInEvent(QFocusEvent *event);
    void leaveEvent(QEvent *event);
    void focusOutEvent(QFocusEvent *event);

private:
    bool inFocus = false;
};
