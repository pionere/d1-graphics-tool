#include "tilesetlightentrywidget.h"

#include <QApplication>
#include <QStyle>

#include "tilesetlightdialog.h"
#include "pushbuttonwidget.h"
#include "ui_tilesetlightentrywidget.h"

TilesetLightEntryWidget::TilesetLightEntryWidget(TrnGenerateDialog *parent)
    : QWidget(parent)
    , ui(new Ui::TilesetLightEntryWidget())
    , view(parent)
{
    ui->setupUi(this);

    QLayout *layout = this->ui->entryHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_TitleBarCloseButton, tr("Remove"), this, &TilesetLightEntryWidget::on_deletePushButtonClicked);
}

TilesetLightEntryWidget::~TilesetLightEntryWidget()
{
    delete ui;
}

void TilesetLightEntryWidget::initialize(const SubtileLight &lightRange)
{
    this->ui->firstSubtileLineEdit->setText(QString::number(lightRange.firstsubtile));
    this->ui->lastSubtileLineEdit->setText(QString::number(lightRange.lastsubtile));
    this->setLightRadius(lightRange.radius);
}

SubtileLight TilesetLightEntryWidget::getSubtileRange() const
{
    SubtileLight lightRange;
    bool firstOk, lastOk;
    lightRange.firstsubtile = this->ui->firstSubtileLineEdit->text().toInt(&firstOk);
    lightRange.lastsubtile = this->ui->lastSubtileLineEdit->text().toInt(&lastOk);
    if (!lastOk) {
        if (!firstOk) {
            lightRange.lastsubtile = -1;
        } else {
            lightRange.lastsubtile = INT_MAX;
        }
    }
    if (lightRange.firstsubtile < 0) {
        lightRange.firstsubtile = 0;
    }

    lightRange.radius = this->getLightRadius();

    return color;
}

int TilesetLightEntryWidget::getLightRadius() const
{
    return this->ui->lightLineEdit->text().toInt();
}

void TilesetLightEntryWidget::setLightRadius(int radius)
{
    this->ui->lightLineEdit->setText(QString::number(radius));
}

void TilesetLightEntryWidget::on_deletePushButtonClicked()
{
    this->view->on_actionDelRange_triggered(this);
}
