#include "affixsliderwidget.h"

#include <QToolTip>

#include "dungeon/all.h"

AffixSliderWidget::AffixSliderWidget(QWidget *parent)
    : SliderWidget(parent)
{
    QObject::connect(this, SIGNAL(valueChanged(int)), this, SLOT(on_valueChanged(int)));
}

void AffixSliderWidget::on_valueChanged(int value)
{
    this->updateToolTip();
    QToolTip::showText(this->mapToGlobal(QPoint(0, 0)), this->toolTip());
}

void AffixSliderWidget::changeValue(int value)
{
    SliderWidget::changeValue(value);
    this->updateToolTip();
}

void AffixSliderWidget::setLimitMode(int mode)
{
    this->limitMode = mode;
    this->setEnabled(mode >= 0);
    if (mode < 0) {
        int minval = this->minimum();
        this->changeValue(minval);
    } else {
        this->updateToolTip();
    }
}

void AffixSliderWidget::updateToolTip()
{
    int val = this->value();
    QString text;
    if (this->limitMode == 3) {
        text = spelldata[GetItemSpell(val)].sNameText;
    } else if (this->limitMode >= 0) {
        text = QString::number(val);
    }
    this->setToolTip(text);
}
