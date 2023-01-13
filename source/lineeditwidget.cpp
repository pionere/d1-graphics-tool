#include "lineeditwidget.h"

#include <QFontMetrics>

static QJsonObject theConfig;

void LineEditWidget::setCharWidth(int value)
{
    int maxWidth = QFontMetrics(this->font()).maxWidth();

    maxWidth *= value;
    this->setMinimumWidth(maxWidth);
    this->setMaximumWidth(maxWidth);
}
