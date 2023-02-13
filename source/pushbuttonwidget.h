#pragma once

#include <QEvent>
#include <QFocusEvent>
#include <QLayout>
#include <QObject>
#include <QPaintEvent>
#include <QPushButton>
#include <QStyle>

class PushButtonWidget : public QPushButton {
    Q_OBJECT

    explicit PushButtonWidget(QLayout *parent, QStyle::StandardPixmap type, const QString &tooltip);

public:
    ~PushButtonWidget() = default;

    template <typename Object, typename PointerToMemberFunction>
    static PushButtonWidget *addButton(QLayout *parent, QStyle::StandardPixmap type, const QString &tooltip, const Object receiver, PointerToMemberFunction method)
    {
        PushButtonWidget *widget = new PushButtonWidget(parent, type, tooltip);
        QObject::connect(widget, &QPushButton::clicked, receiver, method);
        return widget;
    }

protected:
    void paintEvent(QPaintEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void focusInEvent(QFocusEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    int focusFlags = 0;
};
