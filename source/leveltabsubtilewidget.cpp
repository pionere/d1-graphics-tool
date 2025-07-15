#include "leveltabsubtilewidget.h"

#include <QLineEdit>
#include <QStyle>

#include <vector>

#include "levelcelview.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_leveltabsubtilewidget.h"

#include "dungeon/all.h"

EditMinCommand::EditMinCommand(D1Min *m, int si, int idx, int fr)
    : QUndoCommand(nullptr)
    , min(m)
    , subtileIndex(si)
    , index(idx)
    , frameRef(fr)
{
}

void EditMinCommand::undo()
{
    if (this->min.isNull()) {
        this->setObsolete(true);
        return;
    }

    std::vector<unsigned> &frames = this->min->getFrameReferences(this->subtileIndex);
    if (frames.size() < (unsigned)this->index) {
        this->setObsolete(true);
        return;
    }
    int nf = this->frameRef;
    this->frameRef = frames[this->index];
    this->min->setFrameReference(this->subtileIndex, this->index, nf);

    emit this->modified();
}

void EditMinCommand::redo()
{
    this->undo();
}

EditSlaCommand::EditSlaCommand(D1Sla *s, int si, SLA_FIELD_TYPE f, int v)
    : QUndoCommand(nullptr)
    , sla(s)
    , subtileIndex(si)
    , field(f)
    , value(v)
{
}

void EditSlaCommand::undo()
{
    if (this->sla.isNull()) {
        this->setObsolete(true);
        return;
    }

    int subtileIdx = this->subtileIndex;
    switch (this->field) {
    case SLA_FIELD_TYPE::SOL_PROP: {
        quint8 nv = this->value;
        this->value = this->sla->getSubProperties(subtileIdx);
        this->sla->setSubProperties(subtileIdx, nv);
    } break;
    case SLA_FIELD_TYPE::LIGHT_PROP: {
        quint8 nv = this->value;
        this->value = this->sla->getLightRadius(subtileIdx);
        this->sla->setLightRadius(subtileIdx, nv);
    } break;
    case SLA_FIELD_TYPE::TRAP_PROP: {
        int nv = this->value;
        this->value = this->sla->getTrapProperty(subtileIdx);
        this->sla->setTrapProperty(subtileIdx, nv);
    } break;
    case SLA_FIELD_TYPE::SPEC_PROP: {
        int nv = this->value;
        this->value = this->sla->getSpecProperty(subtileIdx);
        this->sla->setSpecProperty(subtileIdx, nv);
    } break;
    case SLA_FIELD_TYPE::RENDER_PROP: {
        quint8 nv = this->value;
        this->value = this->sla->getRenderProperties(subtileIdx);
        this->sla->setRenderProperties(subtileIdx, nv);
    } break;
    case SLA_FIELD_TYPE::MAP_PROP: {
        quint8 nv = this->value;
        this->value = this->sla->getMapProperties(subtileIdx) | this->sla->getMapType(subtileIdx);
        this->sla->setMapType(subtileIdx, nv & MAT_TYPE);
        this->sla->setMapProperties(subtileIdx, nv & ~MAT_TYPE);
    } break;
    }

    emit this->modified();
}

void EditSlaCommand::redo()
{
    this->undo();
}

LevelTabSubtileWidget::LevelTabSubtileWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LevelTabSubtileWidget())
{
    ui->setupUi(this);

    QLayout *layout = this->ui->buttonsHorizontalLayout;
    this->clearButton = PushButtonWidget::addButton(this, layout, QStyle::SP_TitleBarMaxButton, tr("Reset flags"), this, &LevelTabSubtileWidget::on_clearPushButtonClicked);
    this->deleteButton = PushButtonWidget::addButton(this, layout, QStyle::SP_TrashIcon, tr("Delete the current subtile"), this, &LevelTabSubtileWidget::on_deletePushButtonClicked);

    for (int i = 0; i <= MAX_LIGHT_RAD; i++) {
        this->ui->lightComboBox->addItem(QString::number(i), QVariant::fromValue(i));
    }

    QObject::connect(ui->specCelComboBox->lineEdit(), &QLineEdit::returnPressed, this, &LevelTabSubtileWidget::on_specCelComboBox_returnPressed);
}

LevelTabSubtileWidget::~LevelTabSubtileWidget()
{
    delete ui;
}

void LevelTabSubtileWidget::initialize(LevelCelView *v, QUndoStack *us)
{
    this->levelCelView = v;
    this->undoStack = us;
}

void LevelTabSubtileWidget::setTileset(D1Tileset *ts)
{
    this->gfx = ts->gfx;
    this->min = ts->min;
    this->sla = ts->sla;

    this->ui->specCelComboBox->clear();
    int specNum = ts->cls->getFrameCount();
    for (int i = 0; i <= specNum; i++) {
        this->ui->specCelComboBox->addItem(QString::number(i), QVariant::fromValue(i));
    }
    // if (specNum != 0) {
        this->ui->specCelComboBox->addItem(tr("Any"), QVariant::fromValue(-1));
    // }
    // this->ui->specCelComboBox->setEnabled(specNum != 0);
}

void LevelTabSubtileWidget::setGfx(D1Gfx *g)
{
    this->gfx = g;
}

void LevelTabSubtileWidget::updateFields()
{
    this->onUpdate = true;

    bool hasSubtile = this->min->getSubtileCount() != 0;

    this->clearButton->setEnabled(hasSubtile);
    this->deleteButton->setEnabled(hasSubtile);

    this->ui->solSettingsGroupBox->setEnabled(hasSubtile);
    this->ui->sptSettingsGroupBox->setEnabled(hasSubtile);
    this->ui->tmiSettingsGroupBox->setEnabled(hasSubtile);
    this->ui->smpSettingsGroupBox->setEnabled(hasSubtile);

    this->ui->framesComboBox->setEnabled(hasSubtile);

    if (!hasSubtile) {
        this->ui->sol0->setChecked(false);
        this->ui->sol1->setChecked(false);
        this->ui->sol2->setChecked(false);

        this->ui->lightComboBox->setCurrentIndex(-1);
        this->ui->trapNoneRadioButton->setChecked(true);
        this->ui->specCelComboBox->setCurrentIndex(-1);

        this->ui->tmi0->setChecked(false);
        this->ui->tmi1->setChecked(false);
        this->ui->tmi2->setChecked(false);
        this->ui->tmi3->setChecked(false);
        this->ui->tmi4->setChecked(false);
        this->ui->tmi5->setChecked(false);
        this->ui->tmi6->setChecked(false);

        this->ui->smpTypeComboBox->setCurrentIndex(-1);
        this->ui->smp4->setChecked(false);
        this->ui->smp5->setChecked(false);
        this->ui->smp6->setChecked(false);
        this->ui->smp7->setChecked(false);

        this->ui->framesComboBox->setCurrentIndex(-1);
        this->ui->framesPrevButton->setEnabled(false);
        this->ui->framesNextButton->setEnabled(false);

        this->onUpdate = false;
        return;
    }

    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();
    quint8 solFlags = this->sla->getSubProperties(subtileIdx);
    quint8 lightRadius = this->sla->getLightRadius(subtileIdx);
    quint8 tmiFlags = this->sla->getRenderProperties(subtileIdx);
    int sptTrapFlags = this->sla->getTrapProperty(subtileIdx);
    int sptSpecCel = this->sla->getSpecProperty(subtileIdx);
    quint8 smpType = this->sla->getMapType(subtileIdx);
    quint8 smpFlags = this->sla->getMapProperties(subtileIdx);
    std::vector<unsigned> &frames = this->min->getFrameReferences(subtileIdx);

    this->ui->sol0->setChecked((solFlags & PSF_BLOCK_PATH) != 0);
    this->ui->sol1->setChecked((solFlags & PSF_BLOCK_LIGHT) != 0);
    this->ui->sol2->setChecked((solFlags & PSF_BLOCK_MISSILE) != 0);

    this->ui->lightComboBox->setCurrentIndex(lightRadius);
    if (sptTrapFlags == PST_NONE)
        this->ui->trapNoneRadioButton->setChecked(true);
    else if (sptTrapFlags == PST_LEFT)
        this->ui->trapLeftRadioButton->setChecked(true);
    else if (sptTrapFlags == PST_RIGHT)
        this->ui->trapRightRadioButton->setChecked(true);
    int specIdx = this->ui->specCelComboBox->findData(sptSpecCel);
    if (specIdx != -1) {
        this->ui->specCelComboBox->setCurrentIndex(specIdx);
    } else {
        this->ui->specCelComboBox->setEditText(QString::number(sptSpecCel));
    }

    this->ui->tmi0->setChecked((tmiFlags & TMIF_WALL_TRANS) != 0);
    this->ui->tmi1->setChecked((tmiFlags & TMIF_LEFT_REDRAW) != 0);
    this->ui->tmi2->setChecked((tmiFlags & TMIF_LEFT_FOLIAGE) != 0);
    this->ui->tmi3->setChecked((tmiFlags & TMIF_LEFT_WALL_TRANS) != 0);
    this->ui->tmi4->setChecked((tmiFlags & TMIF_RIGHT_REDRAW) != 0);
    this->ui->tmi5->setChecked((tmiFlags & TMIF_RIGHT_FOLIAGE) != 0);
    this->ui->tmi6->setChecked((tmiFlags & TMIF_RIGHT_WALL_TRANS) != 0);

    this->ui->smpTypeComboBox->setCurrentIndex(smpType);
    this->ui->smp4->setChecked((smpFlags & MAT_WALL_NW) != 0);
    this->ui->smp5->setChecked((smpFlags & MAT_WALL_NE) != 0);
    this->ui->smp6->setChecked((smpFlags & MAT_WALL_SW) != 0);
    this->ui->smp7->setChecked((smpFlags & MAT_WALL_SE) != 0);

    // update combo box of the frames
    for (int i = this->ui->framesComboBox->count() - frames.size(); i > 0; i--)
        this->ui->framesComboBox->removeItem(0);
    for (int i = frames.size() - this->ui->framesComboBox->count(); i > 0; i--)
        this->ui->framesComboBox->addItem("");
    for (unsigned i = 0; i < frames.size(); i++) {
        this->ui->framesComboBox->setItemText(i, QString::number(frames[i]));
    }
    if (this->lastSubtileIndex != subtileIdx || this->ui->framesComboBox->currentIndex() == -1) {
        this->lastSubtileIndex = subtileIdx;
        this->lastFrameEntryIndex = 0;
        this->ui->framesComboBox->setCurrentIndex(0);
    }
    this->updateFramesSelection(this->lastFrameEntryIndex);

    this->onUpdate = false;
}

bool LevelTabSubtileWidget::selectFrame(int index)
{
    if (this->lastFrameEntryIndex == index) {
        return false;
    }
    this->ui->framesComboBox->setCurrentIndex(index);
    this->updateFramesSelection(index);
    return true;
}

void LevelTabSubtileWidget::updateFramesSelection(int index)
{
    this->lastFrameEntryIndex = index;
    int frameRef = this->ui->framesComboBox->currentText().toInt();

    this->ui->framesPrevButton->setEnabled(frameRef > 0);
    this->ui->framesNextButton->setEnabled(frameRef < this->gfx->getFrameCount());
}

void LevelTabSubtileWidget::setSolProperty(quint8 flags)
{
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();

    // Build sla editing command and connect it to the views widget
    // to update the label when undo/redo is performed
    EditSlaCommand *command = new EditSlaCommand(this->sla, subtileIdx, SLA_FIELD_TYPE::SOL_PROP, flags);
    QObject::connect(command, &EditSlaCommand::modified, this->levelCelView, &LevelCelView::updateFields);

    this->undoStack->push(command);
}

void LevelTabSubtileWidget::updateSolProperty()
{
    quint8 flags = 0;
    if (this->ui->sol0->checkState())
        flags |= PSF_BLOCK_PATH;
    if (this->ui->sol1->checkState())
        flags |= PSF_BLOCK_LIGHT;
    if (this->ui->sol2->checkState())
        flags |= PSF_BLOCK_MISSILE;

    this->setSolProperty(flags);
}

void LevelTabSubtileWidget::setTmiProperty(quint8 flags)
{
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();

    // Build sla editing command and connect it to the views widget
    // to update the label when undo/redo is performed
    EditSlaCommand *command = new EditSlaCommand(this->sla, subtileIdx, SLA_FIELD_TYPE::RENDER_PROP, flags);
    QObject::connect(command, &EditSlaCommand::modified, this->levelCelView, &LevelCelView::updateFields);

    this->undoStack->push(command);
}

void LevelTabSubtileWidget::updateTmiProperty()
{
    quint8 flags = 0;
    if (this->ui->tmi0->checkState())
        flags |= TMIF_WALL_TRANS;
    if (this->ui->tmi1->checkState())
        flags |= TMIF_LEFT_REDRAW;
    if (this->ui->tmi2->checkState())
        flags |= TMIF_LEFT_FOLIAGE;
    if (this->ui->tmi3->checkState())
        flags |= TMIF_LEFT_WALL_TRANS;
    if (this->ui->tmi4->checkState())
        flags |= TMIF_RIGHT_REDRAW;
    if (this->ui->tmi5->checkState())
        flags |= TMIF_RIGHT_FOLIAGE;
    if (this->ui->tmi6->checkState())
        flags |= TMIF_RIGHT_WALL_TRANS;

    this->setTmiProperty(flags);
}

void LevelTabSubtileWidget::setSmpProperty(quint8 flags)
{
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();

    // Build smp editing command and connect it to the views widget
    // to update the label when undo/redo is performed
    EditSlaCommand *command = new EditSlaCommand(this->sla, subtileIdx, SLA_FIELD_TYPE::MAP_PROP, flags);
    QObject::connect(command, &EditSlaCommand::modified, this->levelCelView, &LevelCelView::updateFields);

    this->undoStack->push(command);
}

void LevelTabSubtileWidget::updateSmpProperty()
{
    quint8 flags = this->ui->smpTypeComboBox->currentIndex();
    if (this->ui->smp4->checkState())
        flags |= MAT_WALL_NW;
    if (this->ui->smp5->checkState())
        flags |= MAT_WALL_NE;
    if (this->ui->smp6->checkState())
        flags |= MAT_WALL_SW;
    if (this->ui->smp7->checkState())
        flags |= MAT_WALL_SE;

    this->setSmpProperty(flags);
}

void LevelTabSubtileWidget::on_clearPushButtonClicked()
{
    this->setSolProperty(0);
    this->setLightRadius(0);
    this->setSpecProperty(0);
    this->setTrapProperty(PST_NONE);
    this->setTmiProperty(0);
    this->setSmpProperty(MAT_NONE);
}

void LevelTabSubtileWidget::on_deletePushButtonClicked()
{
    dMainWindow().on_actionDel_Subtile_triggered();
}

void LevelTabSubtileWidget::on_sol0_clicked()
{
    this->updateSolProperty();
}

void LevelTabSubtileWidget::on_sol1_clicked()
{
    this->updateSolProperty();
}

void LevelTabSubtileWidget::on_sol2_clicked()
{
    this->updateSolProperty();
}

void LevelTabSubtileWidget::setLightRadius(quint8 radius)
{
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();

    // Build sla editing command and connect it to the views widget
    // to update the label when undo/redo is performed
    EditSlaCommand *command = new EditSlaCommand(this->sla, subtileIdx, SLA_FIELD_TYPE::LIGHT_PROP, radius);
    QObject::connect(command, &EditSlaCommand::modified, this->levelCelView, &LevelCelView::updateFields);

    this->undoStack->push(command);
}

void LevelTabSubtileWidget::on_lightComboBox_activated(int index)
{
    this->setLightRadius(index);
}

void LevelTabSubtileWidget::setTrapProperty(int trap)
{
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();

    // Build sla editing command and connect it to the views widget
    // to update the label when undo/redo is performed
    EditSlaCommand *command = new EditSlaCommand(this->sla, subtileIdx, SLA_FIELD_TYPE::TRAP_PROP, trap);
    QObject::connect(command, &EditSlaCommand::modified, this->levelCelView, &LevelCelView::updateFields);

    this->undoStack->push(command);
}

void LevelTabSubtileWidget::on_trapNoneRadioButton_clicked()
{
    this->setTrapProperty(PST_NONE);
}

void LevelTabSubtileWidget::on_trapLeftRadioButton_clicked()
{
    this->setTrapProperty(PST_LEFT);
}

void LevelTabSubtileWidget::on_trapRightRadioButton_clicked()
{
    this->setTrapProperty(PST_RIGHT);
}

void LevelTabSubtileWidget::setSpecProperty(int spec)
{
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();

    // Build sla editing command and connect it to the views widget
    // to update the label and refresh the view when undo/redo is performed
    EditSlaCommand *command = new EditSlaCommand(this->sla, subtileIdx, SLA_FIELD_TYPE::SPEC_PROP, spec);
    QObject::connect(command, &EditSlaCommand::modified, this->levelCelView, &LevelCelView::displayFrame);

    this->undoStack->push(command);
}

void LevelTabSubtileWidget::on_specCelComboBox_activated(int index)
{
    this->setSpecProperty(this->ui->specCelComboBox->currentData().value<int>());
}

void LevelTabSubtileWidget::on_specCelComboBox_returnPressed()
{
    this->setSpecProperty(this->ui->specCelComboBox->currentText().toInt());
}

void LevelTabSubtileWidget::on_tmi0_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubtileWidget::on_tmi1_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubtileWidget::on_tmi2_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubtileWidget::on_tmi3_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubtileWidget::on_tmi4_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubtileWidget::on_tmi5_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubtileWidget::on_tmi6_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubtileWidget::on_smpTypeComboBox_activated(int index)
{
    this->updateSmpProperty();
}

void LevelTabSubtileWidget::on_smp4_clicked()
{
    this->updateSmpProperty();
}

void LevelTabSubtileWidget::on_smp5_clicked()
{
    this->updateSmpProperty();
}

void LevelTabSubtileWidget::on_smp6_clicked()
{
    this->updateSmpProperty();
}

void LevelTabSubtileWidget::on_smp7_clicked()
{
    this->updateSmpProperty();
}

void LevelTabSubtileWidget::setFrameReference(int subtileIndex, int index, int frameRef)
{
    if (frameRef < 0)
        frameRef = 0;
    if (frameRef > this->gfx->getFrameCount())
        frameRef = this->gfx->getFrameCount();

    // Build min editing command and connect it to the views widget
    // to update the label and refresh the view when undo/redo is performed
    EditMinCommand *command = new EditMinCommand(this->min, subtileIndex, index, frameRef);
    QObject::connect(command, &EditMinCommand::modified, this->levelCelView, &LevelCelView::displayFrame);

    this->undoStack->push(command);
}

void LevelTabSubtileWidget::on_framesPrevButton_clicked()
{
    int index = this->ui->framesComboBox->currentIndex();
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();
    std::vector<unsigned> &frameReferences = this->min->getFrameReferences(subtileIdx);
    int frameRef = frameReferences[index] - 1;

    this->setFrameReference(subtileIdx, index, frameRef);
}

void LevelTabSubtileWidget::on_framesComboBox_activated(int index)
{
    if (!this->onUpdate) {
        this->updateFramesSelection(index);
    }
}

void LevelTabSubtileWidget::on_framesComboBox_currentTextChanged(const QString &arg1)
{
    int index = this->lastFrameEntryIndex;

    if (this->onUpdate || this->ui->framesComboBox->currentIndex() != index)
        return; // on update or side effect of combobox activated -> ignore

    int frameRef = this->ui->framesComboBox->currentText().toInt();

    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();
    this->setFrameReference(subtileIdx, index, frameRef);
}

void LevelTabSubtileWidget::on_framesNextButton_clicked()
{
    int index = this->ui->framesComboBox->currentIndex();
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();
    std::vector<unsigned> &frameReferences = this->min->getFrameReferences(subtileIdx);
    int frameRef = frameReferences[index] + 1;

    this->setFrameReference(subtileIdx, index, frameRef);
}
