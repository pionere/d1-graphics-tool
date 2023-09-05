#include "tilesetlightdialog.h"

#include <vector>

#include <QMessageBox>

#include "d1dun.h"
#include "levelcelview.h"
#include "progressdialog.h"
#include "ui_tilesetlightdialog.h"

#include "dungeon/all.h"

TilesetLightDialog::TilesetLightDialog(LevelCelView *v)
    : QDialog(v)
    , ui(new Ui::TilesetLightDialog())
    , view(v)
{
    ui->setupUi(this);

    QHBoxLayout *layout = this->ui->addRangeButtonLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogNewFolder, tr("Add"), this, &TilesetLightDialog::on_actionAddRange_triggered);
    layout->addStretch();

    this->ui->mainGridLayout->setRowStretch(1, 1);
    this->ui->mainGridLayout->setRowStretch(3, 1);
    this->ui->mainGridLayout->setColumnStretch(1, 1);
}

TilesetLightDialog::~TilesetLightDialog()
{
    delete ui;
}

void TilesetLightDialog::initialize(D1Tileset *ts)
{
    this->tileset = ts;

    std::vector<SubtileLight> ranges;
    for (int i = 0; i < ts->sla->getSubtileCount(); i++) {
        int radius = ts->sla->getLightRadius(i);
        if (radius != 0) {
            if (ranges.empty() || ranges.back().radius != radius || ranges.back().lastsubtile != i - 1) {
                SubtileLight range;
                range.firstsubtile = i;
                range.lastsubtile = i;
                range.radius = radius;
                ranges.push_back(range);
            } else {
                ranges.back().lastsubtile = i;
            }
        }
    }

    QList<TilesetLightEntryWidget *> rangeWidgets = this->ui->rangesVBoxLayout->parentWidget()->findChildren<TilesetLightEntryWidget *>();
    for (int i = ranges.size(); i < rangeWidgets.count(); i++) {
        this->on_actionDelRange_triggered(rangeWidgets[i]);
    }
    for (unsigned i = rangeWidgets.count(); i < ranges.size(); i++) {
        this->on_actionAddRange_triggered();
    }

    rangeWidgets = this->ui->rangesVBoxLayout->parentWidget()->findChildren<TilesetLightEntryWidget *>();
    for (int i = 0; i < rangeWidgets.count(); i++) {
        rangeWidgets[i]->initialize(ranges[i]);
    }
}

void TilesetLightDialog::on_actionAddRange_triggered()
{
    TilesetLightEntryWidget *widget = new TilesetLightEntryWidget(this);
    this->ui->rangesVBoxLayout->addWidget(widget, 0, Qt::AlignTop);
}

void TilesetLightDialog::on_actionDelRange_triggered(TilesetLightEntryWidget *caller)
{
    this->ui->rangesVBoxLayout->removeWidget(caller);
    delete caller;
    // this->adjustSize();
}

void TilesetLightDialog::on_lightDecButton_clicked()
{
    QList<TilesetLightEntryWidget *> ranges = this->ui->rangesVBoxLayout->parentWidget()->findChildren<TilesetLightEntryWidget *>();
    for (int i = 0; i < ranges.count(); i++) {
        TilesetLightEntryWidget *w = ranges[i];
        int radius = w->getLightRadius() - 1;
        if (radius < 0) {
            radius = 0;
        }
        w->setLightRadius(radius);
    }
}

void TilesetLightDialog::on_lightIncButton_clicked()
{
    QList<TilesetLightEntryWidget *> ranges = this->ui->rangesVBoxLayout->parentWidget()->findChildren<TilesetLightEntryWidget *>();
    for (int i = 0; i < ranges.count(); i++) {
        TilesetLightEntryWidget *w = ranges[i];
        int radius = w->getLightRadius() + 1;
        if (radius > MAX_LIGHT_RAD) {
            radius = MAX_LIGHT_RAD;
        }
        w->setLightRadius(radius);
    }
}

void TilesetLightDialog::on_lightAcceptButton_clicked()
{
    QList<TilesetLightEntryWidget *> ranges = this->ui->rangesVBoxLayout->parentWidget()->findChildren<TilesetLightEntryWidget *>();
    for (int i = 0; i < this->tileset->sla->getSubtileCount(); i++) {
        int radius = 0;
        for (int n = ranges.count() - 1; n >= 0; n--) {
            TilesetLightEntryWidget *w = ranges[n];
            SubtileLight light = w->getSubtileRange();
            if (light.firstsubtile <= i && light.lastsubtile >= i) {
                radius = light.radius;
                break;
            }
        }
        this->tileset->sla->setLightRadius(i, radius);
    }

    this->view->updateFields();
    this->close();
}

void TilesetLightDialog::on_lightCancelButton_clicked()
{
    this->close();
}

void TilesetLightDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
