#include "lineeditwidget.h"

#include <QFontMetrics>

#include "dungeon/interfac.h"

LineEditWidget::LineEditWidget(QWidget *parent)
    : QLineEdit(parent)
{
}

LineEditWidget::LineEditWidget(const QString &contents, QWidget *parent)
    : QLineEdit(contents, parent)
{
}

static bool done = false;
void LineEditWidget::setCharWidth(int value)
{
    int maxWidth = this->fontMetrics().horizontalAdvance('w');

    maxWidth *= value;

    const QMargins margins = this->contentsMargins();

    maxWidth += margins.left() + margins.right();

    const QMargins textmargins = this->textMargins();

    maxWidth += textmargins.left() + textmargins.right();

    maxWidth += this->frameSize().width() - this->width();

    if (!done) {
        done = true;
        LogErrorF("crx:%d crw:%d w:%d fw:%d m (%d:%d, %d:%d)", this->cursorRect().x(), this->cursorRect().width(), this->width(), this->frameSize().width(), margins.left(), margins.right(), textmargins.left(), textmargins.right());
        LogErrorF("grx:%d grw:%d rx:%d rw:%d hrx:%d hrw:%d orx:%d orw:%d", this->normalGeometry().x(), this->normalGeometry().width(), this->rect().x(), this->rect().width(), this->childrenRect().x(), this->childrenRect().width(), this->contentsRect().x(), this->contentsRect().width());
    }
    maxWidth += 2 * 4;

    this->setMinimumWidth(maxWidth);
    this->setMaximumWidth(maxWidth);
}

int LineEditWidget::nonNegInt() const
{
    int result = this->text().toInt();
    if (result < 0) {
        result = 0;
    }
    return result;
}

std::pair<int, int> LineEditWidget::nonNegRange() const
{
    QStringList parts = this->text().split('-', Qt::SkipEmptyParts);
    std::pair<int, int> result = { 0, 0 };
    if (parts.size() == 1) {
        result.first = parts[0].toInt();
        result.second = result.first;
    } else if (parts.size() == 2) {
        result.first = parts[0].toInt();
        result.second = parts[1].toInt();
    }
    if (result.first < 0) {
        result.first = 0;
    }
    if (result.second < 0) {
        result.second = 0;
    }
    return result;
}

void LineEditWidget::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    if (key == Qt::Key_Escape) {
        emit cancel_signal();
        return;
    }
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        emit returnPressed();
        return;
    }

    QLineEdit::keyPressEvent(event);
}

void LineEditWidget::focusInEvent(QFocusEvent *event)
{
    emit focus_gain_signal();

    QLineEdit::focusInEvent(event);
}

void LineEditWidget::focusOutEvent(QFocusEvent *event)
{
    if (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason /*|| event->reason() == Qt::ShortcutFocusReason*/) {
        // submit on tabpress
        emit returnPressed();
    } else {
        // cancel otherwise
        emit cancel_signal();
    }

    emit focus_lost_signal();

    QLineEdit::focusOutEvent(event);
}
