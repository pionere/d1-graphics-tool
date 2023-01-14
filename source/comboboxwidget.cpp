#include "comboboxwidget.h"

#include <QCoreApplication>
#include <QFontMetrics>

ComboBoxWidget::ComboBoxWidget(QWidget *parent)
    : QComboBox(parent)
{
    this->lineEditWiget = new LineEditWiget(this);
    this->setLineEdit(this->lineEditWiget);

    // forward events of the lineEditWiget
    QObject::connect(this->lineEditWiget, SIGNAL(cancel_signal()), this, SIGNAL(cancel_signal()));
    QObject::connect(this->lineEditWiget, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));
}

void ComboBoxWidget::setCharWidth(int value)
{
    int maxWidth = this->fontMetrics().horizontalAdvance('w');

    maxWidth *= value;

    const QMargins margins = this->contentsMargins();

    maxWidth += margins.left() + margins.right();

    this->setMinimumWidth(maxWidth);
    this->setMaximumWidth(maxWidth);
}

/*void ComboBoxWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit cancel_signal();
    }

    QComboBox::keyPressEvent(event);
}

void ComboBoxWidget::focusOutEvent(QFocusEvent *event)
{
    if (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason /*|| event->reason() == Qt::ShortcutFocusReason* /) {
        // submit on tabpress
        QKeyEvent *kpEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
        QCoreApplication::postEvent(this, kpEvent);
    } else {
        // cancel otherwise
        emit cancel_signal();
    }

    QComboBox::focusOutEvent(event);
}*/