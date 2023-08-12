#include "leveltabtilewidget.h"

#include <QStyle>

#include "d1amp.h"
#include "d1min.h"
#include "d1til.h"
#include "levelcelview.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_leveltabtilewidget.h"

EditTileCommand::EditTileCommand(D1Til *t, int ti, int idx, int si)
    : QUndoCommand(nullptr)
    , til(t)
    , tileIndex(ti)
    , index(idx)
    , subtileIndex(si)
{
}

void EditTileCommand::undo()
{
    if (this->til.isNull()) {
        this->setObsolete(true);
        return;
    }

    std::vector<int> &subtileIndices = this->til->getSubtileIndices(this->tileIndex);
    if (subtileIndices.size() < (unsigned)this->index) {
        this->setObsolete(true);
        return;
    }
    int nsi = this->subtileIndex;
    this->subtileIndex = subtileIndices[this->index];
    this->til->setSubtileIndex(this->tileIndex, this->index, nsi);

    emit this->modified();
}

void EditTileCommand::redo()
{
    this->undo();
}

EditAmpCommand::EditAmpCommand(D1Amp *a, int ti, int v, bool t)
    : QUndoCommand(nullptr)
    , amp(a)
    , tileIndex(ti)
    , value(v)
    , type(t)
{
}

void EditAmpCommand::undo()
{
    if (this->amp.isNull()) {
        this->setObsolete(true);
        return;
    }

    int nv = this->value;
    if (this->type) {
        this->value = this->amp->getTileType(this->tileIndex);
        this->amp->setTileType(this->tileIndex, nv);
    } else {
        this->value = this->amp->getTileProperties(this->tileIndex);
        this->amp->setTileProperties(this->tileIndex, nv);
    }
    emit this->modified();
}

void EditAmpCommand::redo()
{
    this->undo();
}

EditTlaCommand::EditTlaCommand(D1Tla *tt, int ti, int v)
    : QUndoCommand(nullptr)
    , tla(tt)
    , tileIndex(ti)
    , value(v)
{
}

void EditTlaCommand::undo()
{
    if (this->tla.isNull()) {
        this->setObsolete(true);
        return;
    }

    int nv = this->value;
    this->value = this->tla->getTileProperties(this->tileIndex);
    this->tla->setTileProperties(this->tileIndex, nv);

    emit this->modified();
}

void EditTlaCommand::redo()
{
    this->undo();
}

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

void LevelTabTileWidget::initialize(LevelCelView *v, QUndoStack *us, D1Til *t, D1Min *m, D1Amp *a, D1Tla *tt)
{
    this->levelCelView = v;
    this->undoStack = us;
    this->til = t;
    this->min = m;
    this->amp = a;
    this->tla = tt;
}

void LevelTabTileWidget::updateFields()
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

    this->ui->tlaF00->setEnabled(hasTile);
    this->ui->tlaF01->setEnabled(hasTile);
    this->ui->tlaF10->setEnabled(hasTile);
    this->ui->tlaF11->setEnabled(hasTile);
    this->ui->tlaS00->setEnabled(hasTile);
    this->ui->tlaS01->setEnabled(hasTile);
    this->ui->tlaS02->setEnabled(hasTile);
    this->ui->tlaS03->setEnabled(hasTile);

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

        this->ui->tlaF00->setChecked(false);
        this->ui->tlaF01->setChecked(false);
        this->ui->tlaF10->setChecked(false);
        this->ui->tlaF11->setChecked(false);
        this->ui->tlaS00->setChecked(false);
        this->ui->tlaS01->setChecked(false);
        this->ui->tlaS02->setChecked(false);
        this->ui->tlaS03->setChecked(false);

        this->ui->subtilesComboBox->setCurrentIndex(-1);
        this->ui->subtilesPrevButton->setEnabled(false);
        this->ui->subtilesNextButton->setEnabled(false);

        this->onUpdate = false;
        return;
    }

    int tileIdx = this->levelCelView->getCurrentTileIndex();
    quint8 ampType = this->amp->getTileType(tileIdx);
    quint8 ampProperty = this->amp->getTileProperties(tileIdx);
    quint8 tlaProperty = this->tla->getTileProperties(tileIdx);
    std::vector<int> &subtiles = this->til->getSubtileIndices(tileIdx);

    // update the combo box of the amp-type
    this->ui->ampTypeComboBox->setCurrentIndex(ampType);
    // update the checkboxes
    this->ui->amp0->setChecked((ampProperty & (MAF_WEST_DOOR >> 8)) != 0);
    this->ui->amp1->setChecked((ampProperty & (MAF_EAST_DOOR >> 8)) != 0);
    this->ui->amp2->setChecked((ampProperty & (MAF_WEST_ARCH >> 8)) != 0);
    this->ui->amp3->setChecked((ampProperty & (MAF_EAST_ARCH >> 8)) != 0);
    this->ui->amp4->setChecked((ampProperty & (MAF_WEST_GRATE >> 8)) != 0);
    this->ui->amp5->setChecked((ampProperty & (MAF_EAST_GRATE >> 8)) != 0);
    this->ui->amp6->setChecked((ampProperty & (MAF_EXTERN >> 8)) != 0);
    this->ui->amp7->setChecked((ampProperty & (MAF_STAIRS >> 8)) != 0);

    this->ui->tlaF00->setChecked((tlaProperty & TIF_FLOOR_00) != 0);
    this->ui->tlaF01->setChecked((tlaProperty & TIF_FLOOR_01) != 0);
    this->ui->tlaF10->setChecked((tlaProperty & TIF_FLOOR_10) != 0);
    this->ui->tlaF11->setChecked((tlaProperty & TIF_FLOOR_11) != 0);
    this->ui->tlaS00->setChecked((tlaProperty & TIF_SHADOW_00) != 0);
    this->ui->tlaS01->setChecked((tlaProperty & TIF_SHADOW_01) != 0);
    this->ui->tlaS02->setChecked((tlaProperty & TIF_SHADOW_02) != 0);
    this->ui->tlaS03->setChecked((tlaProperty & TIF_SHADOW_03) != 0);

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

    // Build amp editing command and connect it to the views widget
    // to update the label when undo/redo is performed
    EditAmpCommand *command = new EditAmpCommand(this->amp, tileIdx, flags, false);
    QObject::connect(command, &EditAmpCommand::modified, this->levelCelView, &LevelCelView::updateFields);

    this->undoStack->push(command);
}

void LevelTabTileWidget::setTlaProperty(quint8 flags)
{
    int tileIdx = this->levelCelView->getCurrentTileIndex();

    // Build amp editing command and connect it to the views widget
    // to update the label when undo/redo is performed
    EditTlaCommand *command = new EditTlaCommand(this->tla, tileIdx, flags);
    QObject::connect(command, &EditTlaCommand::modified, this->levelCelView, &LevelCelView::updateFields);

    this->undoStack->push(command);
}

void LevelTabTileWidget::updateAmpProperty()
{
    quint8 flags = 0;
    if (this->ui->amp0->checkState())
        flags |= (MAF_WEST_DOOR >> 8);
    if (this->ui->amp1->checkState())
        flags |= (MAF_EAST_DOOR >> 8);
    if (this->ui->amp2->checkState())
        flags |= (MAF_WEST_ARCH >> 8);
    if (this->ui->amp3->checkState())
        flags |= (MAF_EAST_ARCH >> 8);
    if (this->ui->amp4->checkState())
        flags |= (MAF_WEST_GRATE >> 8);
    if (this->ui->amp5->checkState())
        flags |= (MAF_EAST_GRATE >> 8);
    if (this->ui->amp6->checkState())
        flags |= (MAF_EXTERN >> 8);
    if (this->ui->amp7->checkState())
        flags |= (MAF_STAIRS >> 8);

    this->setAmpProperty(flags);
}

void LevelTabTileWidget::updateTlaProperty()
{
    quint8 flags = 0;
    if (this->ui->tlaF00->checkState())
        flags |= TIF_FLOOR_00;
    if (this->ui->tlaF01->checkState())
        flags |= TIF_FLOOR_01;
    if (this->ui->tlaF10->checkState())
        flags |= TIF_FLOOR_10;
    if (this->ui->tlaF11->checkState())
        flags |= TIF_FLOOR_11;
    if (this->ui->tlaS00->checkState())
        flags |= TIF_SHADOW_00;
    if (this->ui->tlaS01->checkState())
        flags |= TIF_SHADOW_01;
    if (this->ui->tlaS02->checkState())
        flags |= TIF_SHADOW_02;
    if (this->ui->tlaS03->checkState())
        flags |= TIF_SHADOW_03;

    this->setTlaProperty(flags);
}

void LevelTabTileWidget::on_clearPushButtonClicked()
{
    this->setAmpProperty(0);
    this->setTlaProperty(0);
    this->updateFields();
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

    // Build amp editing command and connect it to the views widget
    // to update the label when undo/redo is performed
    EditAmpCommand *command = new EditAmpCommand(this->amp, tileIdx, index, true);
    QObject::connect(command, &EditAmpCommand::modified, this->levelCelView, &LevelCelView::updateFields);

    this->undoStack->push(command);
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

void LevelTabTileWidget::on_tlaF00_clicked()
{
    this->updateTlaProperty();
}

void LevelTabTileWidget::on_tlaF01_clicked()
{
    this->updateTlaProperty();
}

void LevelTabTileWidget::on_tlaF10_clicked()
{
    this->updateTlaProperty();
}

void LevelTabTileWidget::on_tlaF11_clicked()
{
    this->updateTlaProperty();
}

void LevelTabTileWidget::on_tlaS00_clicked()
{
    this->updateTlaProperty();
}

void LevelTabTileWidget::on_tlaS01_clicked()
{
    this->updateTlaProperty();
}

void LevelTabTileWidget::on_tlaS02_clicked()
{
    this->updateTlaProperty();
}

void LevelTabTileWidget::on_tlaS03_clicked()
{
    this->updateTlaProperty();
}

void LevelTabTileWidget::setSubtileIndex(int tileIndex, int index, int subtileIndex)
{
    // Build tile editing command and connect it to the views widget
    // to update the label and refresh the view when undo/redo is performed
    EditTileCommand *command = new EditTileCommand(this->til, tileIndex, index, subtileIndex);
    QObject::connect(command, &EditTileCommand::modified, this->levelCelView, &LevelCelView::on_tilModified);

    this->undoStack->push(command);
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
    this->setSubtileIndex(tileIdx, index, subtileIdx);
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
    this->setSubtileIndex(tileIdx, index, subtileIdx);
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
    this->setSubtileIndex(tileIdx, index, subtileIdx);
}
