#include "leveltabtilewidget.h"

#include "d1amp.h"
#include "d1min.h"
#include "d1til.h"
#include "levelcelview.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_leveltabtilewidget.h"

LevelTabTileWidget::LevelTabTileWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LevelTabTileWidget())
{
    ui->setupUi(this);

    QLayout *layout = this->ui->buttonsHorizontalLayout;
    this->clearButton = PushButtonWidget::addButton(this, layout, QStyle::SP_TitleBarMaxButton, tr("Reset flags"), this, &LevelTabTileWidget::on_clearPushButtonClicked);
    this->deleteButton = PushButtonWidget::addButton(this, layout, QStyle::SP_TrashIcon, tr("Delete the current tile"), this, &LevelTabTileWidget::on_deletePushButtonClicked);
}

LevelTabTileWidget::~LevelTabTileWidget()
{
    delete ui;
}

void LevelTabTileWidget::initialize(LevelCelView *v, D1Til *t, D1Min *m, D1Amp *a)
{
    this->levelCelView = v;
    this->til = t;
    this->min = m;
    this->amp = a;
}

void LevelTabTileWidget::update()
{
    this->onUpdate = true;

    bool hasTile = this->til->getTileCount() != 0;

    this->clearButton->setEnabled(hasTile);
    this->deleteButton->setEnabled(hasTile);

    this->ui->ampTypeComboBox->setEnabled(hasTile);

    this->ui->amp0->setEnabled(hasTile);
    this->ui->amp1->setEnabled(hasTile);
    this->ui->amp2->setEnabled(hasTile);
    this->ui->amp3->setEnabled(hasTile);
    this->ui->amp4->setEnabled(hasTile);
    this->ui->amp5->setEnabled(hasTile);
    this->ui->amp6->setEnabled(hasTile);
    this->ui->amp7->setEnabled(hasTile);

    this->ui->subtilesComboBox->setEnabled(hasTile);

    if (!hasTile) {
        this->ui->ampTypeComboBox->setCurrentIndex(-1);

        this->ui->amp0->setChecked(false);
        this->ui->amp1->setChecked(false);
        this->ui->amp2->setChecked(false);
        this->ui->amp3->setChecked(false);
        this->ui->amp4->setChecked(false);
        this->ui->amp5->setChecked(false);
        this->ui->amp6->setChecked(false);
        this->ui->amp7->setChecked(false);

        this->ui->subtilesComboBox->setCurrentIndex(-1);
        this->ui->subtilesPrevButton->setEnabled(false);
        this->ui->subtilesNextButton->setEnabled(false);

        this->onUpdate = false;
        return;
    }

    int tileIdx = this->levelCelView->getCurrentTileIndex();
    quint8 ampType = this->amp->getTileType(tileIdx);
    quint8 ampProperty = this->amp->getTileProperties(tileIdx);
    std::vector<int> &subtiles = this->til->getSubtileIndices(tileIdx);

    // update the combo box of the amp-type
    this->ui->ampTypeComboBox->setCurrentIndex(ampType);
    // update the checkboxes
    this->ui->amp0->setChecked((ampProperty & (MAPFLAG_VERTDOOR >> 8)) != 0);
    this->ui->amp1->setChecked((ampProperty & (MAPFLAG_HORZDOOR >> 8)) != 0);
    this->ui->amp2->setChecked((ampProperty & (MAPFLAG_VERTARCH >> 8)) != 0);
    this->ui->amp3->setChecked((ampProperty & (MAPFLAG_HORZARCH >> 8)) != 0);
    this->ui->amp4->setChecked((ampProperty & (MAPFLAG_VERTGRATE >> 8)) != 0);
    this->ui->amp5->setChecked((ampProperty & (MAPFLAG_HORZGRATE >> 8)) != 0);
    this->ui->amp6->setChecked((ampProperty & (MAPFLAG_DIRT >> 8)) != 0);
    this->ui->amp7->setChecked((ampProperty & (MAPFLAG_STAIRS >> 8)) != 0);
    // update combo box of the subtiles
    for (int i = this->ui->subtilesComboBox->count() - subtiles.size(); i > 0; i--)
        this->ui->subtilesComboBox->removeItem(0);
    for (int i = subtiles.size() - this->ui->subtilesComboBox->count(); i > 0; i--)
        this->ui->subtilesComboBox->addItem("");
    for (unsigned i = 0; i < subtiles.size(); i++) {
        this->ui->subtilesComboBox->setItemText(i, QString::number(subtiles[i] + 1));
    }
    if (this->lastTileIndex != tileIdx || this->ui->subtilesComboBox->currentIndex() == -1) {
        this->lastTileIndex = tileIdx;
        this->lastSubtileEntryIndex = 0;
        this->ui->subtilesComboBox->setCurrentIndex(0);
    }
    this->updateSubtilesSelection(this->lastSubtileEntryIndex);

    this->onUpdate = false;
}

void LevelTabTileWidget::selectSubtile(int index)
{
    this->ui->subtilesComboBox->setCurrentIndex(index);
    this->updateSubtilesSelection(index);
}

void LevelTabTileWidget::updateSubtilesSelection(int index)
{
    this->lastSubtileEntryIndex = index;
    int subtileIdx = this->ui->subtilesComboBox->currentText().toInt() - 1;

    this->ui->subtilesPrevButton->setEnabled(subtileIdx > 0);
    this->ui->subtilesNextButton->setEnabled(subtileIdx < this->min->getSubtileCount() - 1);
}

void LevelTabTileWidget::setAmpProperty(quint8 flags)
{
    int tileIdx = this->levelCelView->getCurrentTileIndex();

    if (this->amp->setTileProperties(tileIdx, flags)) {
        this->levelCelView->updateLabel();
    }
}

void LevelTabTileWidget::updateAmpProperty()
{
    quint8 flags = 0;
    if (this->ui->amp0->checkState())
        flags |= (MAPFLAG_VERTDOOR >> 8);
    if (this->ui->amp1->checkState())
        flags |= (MAPFLAG_HORZDOOR >> 8);
    if (this->ui->amp2->checkState())
        flags |= (MAPFLAG_VERTARCH >> 8);
    if (this->ui->amp3->checkState())
        flags |= (MAPFLAG_HORZARCH >> 8);
    if (this->ui->amp4->checkState())
        flags |= (MAPFLAG_VERTGRATE >> 8);
    if (this->ui->amp5->checkState())
        flags |= (MAPFLAG_HORZGRATE >> 8);
    if (this->ui->amp6->checkState())
        flags |= (MAPFLAG_DIRT >> 8);
    if (this->ui->amp7->checkState())
        flags |= (MAPFLAG_STAIRS >> 8);

    this->setAmpProperty(flags);
}

void LevelTabTileWidget::on_clearPushButtonClicked()
{
    this->setAmpProperty(0);
    this->update();
}

void LevelTabTileWidget::on_deletePushButtonClicked()
{
    dMainWindow().on_actionDel_Tile_triggered();
}

void LevelTabTileWidget::on_ampTypeComboBox_activated(int index)
{
    int tileIdx = this->levelCelView->getCurrentTileIndex();

    if (this->onUpdate) {
        return;
    }

    if (this->amp->setTileType(tileIdx, index)) {
        this->levelCelView->updateLabel();
    }
}

void LevelTabTileWidget::on_amp0_clicked()
{
    this->updateAmpProperty();
}

void LevelTabTileWidget::on_amp1_clicked()
{
    this->updateAmpProperty();
}

void LevelTabTileWidget::on_amp2_clicked()
{
    this->updateAmpProperty();
}

void LevelTabTileWidget::on_amp3_clicked()
{
    this->updateAmpProperty();
}

void LevelTabTileWidget::on_amp4_clicked()
{
    this->updateAmpProperty();
}

void LevelTabTileWidget::on_amp5_clicked()
{
    this->updateAmpProperty();
}

void LevelTabTileWidget::on_amp6_clicked()
{
    this->updateAmpProperty();
}

void LevelTabTileWidget::on_amp7_clicked()
{
    this->updateAmpProperty();
}

void LevelTabTileWidget::on_subtilesPrevButton_clicked()
{
    int index = this->ui->subtilesComboBox->currentIndex();
    int tileIdx = this->levelCelView->getCurrentTileIndex();
    std::vector<int> &subtileIndices = this->til->getSubtileIndices(tileIdx);
    int subtileIdx = subtileIndices[index] - 1;

    if (subtileIdx < 0) {
        subtileIdx = 0;
    }

    if (this->til->setSubtileIndex(tileIdx, index, subtileIdx)) {
        // this->ui->subtilesComboBox->setItemText(index, QString::number(subtileIdx + 1));
        // this->updateSubtilesSelection(index);

        this->levelCelView->updateLabel();
        this->levelCelView->displayFrame();
    }
}

void LevelTabTileWidget::on_subtilesComboBox_activated(int index)
{
    if (!this->onUpdate) {
        this->updateSubtilesSelection(index);
    }
}

void LevelTabTileWidget::on_subtilesComboBox_currentTextChanged(const QString &arg1)
{
    int index = this->lastSubtileEntryIndex;

    if (this->onUpdate || this->ui->subtilesComboBox->currentIndex() != index)
        return; // on update or side effect of combobox activated -> ignore

    int subtileIdx = this->ui->subtilesComboBox->currentText().toInt() - 1;
    if (subtileIdx < 0 || subtileIdx >= this->min->getSubtileCount())
        return; // invalid value -> ignore

    int tileIdx = this->levelCelView->getCurrentTileIndex();
    if (this->til->setSubtileIndex(tileIdx, index, subtileIdx)) {
        // this->ui->subtilesComboBox->setItemText(index, QString::number(subtileIdx + 1));
        // this->updateSubtilesSelection(index);

        this->levelCelView->updateLabel();
        this->levelCelView->displayFrame();
    }
}

void LevelTabTileWidget::on_subtilesNextButton_clicked()
{
    int index = this->ui->subtilesComboBox->currentIndex();
    int tileIdx = this->levelCelView->getCurrentTileIndex();
    std::vector<int> &subtileIndices = this->til->getSubtileIndices(tileIdx);
    int subtileIdx = subtileIndices[index] + 1;

    if (subtileIdx > this->min->getSubtileCount() - 1) {
        subtileIdx = this->min->getSubtileCount() - 1;
    }

    if (this->til->setSubtileIndex(tileIdx, index, subtileIdx)) {
        // this->ui->subtilesComboBox->setItemText(index, QString::number(subtileIdx + 1));
        // this->updateSubtilesSelection(index);

        this->levelCelView->updateLabel();
        this->levelCelView->displayFrame();
    }
}
