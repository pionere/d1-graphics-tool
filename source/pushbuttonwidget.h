#pragma once

#include <QEvent>
#include <QFocusEvent>
#include <QGridLayout>
#include <QLayout>
#include <QObject>
#include <QPaintEvent>
#include <QPushButton>
#include <QStyle>

class PushButtonWidget : public QPushButton {
    Q_OBJECT

    explicit PushButtonWidget(QWidget *parent, QStyle::StandardPixmap type, const QString &tooltip);

public:
    ~PushButtonWidget() = default;

    template <typename Object, typename PointerToMemberFunction>
    static PushButtonWidget *addButton(QWidget *parent, QStyle::StandardPixmap type, const QString &tooltip, const Object receiver, PointerToMemberFunction method)
    {
        PushButtonWidget *widget = new PushButtonWidget(parent, type, tooltip);
        QObject::connect(widget, &QPushButton::clicked, receiver, method);
        return widget;
    }
    template <typename Object, typename PointerToMemberFunction>
    static PushButtonWidget *addButton(QWidget *parent, QLayout *layout, QStyle::StandardPixmap type, const QString &tooltip, const Object receiver, PointerToMemberFunction method)
    {
        PushButtonWidget *widget = new PushButtonWidget(parent, type, tooltip);
        layout->addWidget(widget);
        QObject::connect(widget, &QPushButton::clicked, receiver, method);
        return widget;
    }
    template <typename Object, typename PointerToMemberFunction>
    static PushButtonWidget *addButton(QWidget *parent, QGridLayout *layout, int row, int column, QStyle::StandardPixmap type, const QString &tooltip, const Object receiver, PointerToMemberFunction method)
    {
        PushButtonWidget *widget = new PushButtonWidget(parent, type, tooltip);
        layout->addWidget(widget, row, column);
        QObject::connect(widget, &QPushButton::clicked, receiver, method);
        return widget;
    }

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
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
