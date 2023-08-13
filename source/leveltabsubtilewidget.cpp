#include "leveltabsubtilewidget.h"

#include <QStyle>

#include <vector>

#include "levelcelview.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_leveltabsubtilewidget.h"

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

EditSolCommand::EditSolCommand(D1Sol *s, int si, quint8 f)
    : QUndoCommand(nullptr)
    , sol(s)
    , subtileIndex(si)
    , flags(f)
{
}

void EditSolCommand::undo()
{
    if (this->sol.isNull()) {
        this->setObsolete(true);
        return;
    }

    quint8 nf = this->flags;
    this->flags = this->sol->getSubtileProperties(this->subtileIndex);
    this->sol->setSubtileProperties(this->subtileIndex, nf);

    emit this->modified();
}

void EditSolCommand::redo()
{
    this->undo();
}

EditSptCommand::EditSptCommand(D1Spt *s, int si, int v, bool t)
    : QUndoCommand(nullptr)
    , spt(s)
    , subtileIndex(si)
    , value(v)
    , trap(t)
{
}

void EditSptCommand::undo()
{
    if (this->spt.isNull()) {
        this->setObsolete(true);
        return;
    }

    int nv = this->value;
    if (this->trap) {
        this->value = this->spt->getSubtileTrapProperty(this->subtileIndex);
        this->spt->setSubtileTrapProperty(this->subtileIndex, nv);

        emit this->trapModified();
    } else {
        this->value = this->spt->getSubtileSpecProperty(this->subtileIndex);
        this->spt->setSubtileSpecProperty(this->subtileIndex, nv);

        emit this->specModified();
    }
}

void EditSptCommand::redo()
{
    this->undo();
}

EditTmiCommand::EditTmiCommand(D1Tmi *t, int si, quint8 f)
    : QUndoCommand(nullptr)
    , tmi(t)
    , subtileIndex(si)
    , flags(f)
{
}

void EditTmiCommand::undo()
{
    if (this->tmi.isNull()) {
        this->setObsolete(true);
        return;
    }

    quint8 nf = this->flags;
    this->flags = this->tmi->getSubtileProperties(this->subtileIndex);
    this->tmi->setSubtileProperties(this->subtileIndex, nf);

    emit this->modified();
}

void EditTmiCommand::redo()
{
    this->undo();
}

EditSmpCommand::EditSmpCommand(D1Smp *s, int si, int v, int t)
    : QUndoCommand(nullptr)
    , smp(s)
    , subtileIndex(si)
    , value(v)
    , typeProp(t)
{
}

void EditSmpCommand::undo()
{
    if (this->smp.isNull()) {
        this->setObsolete(true);
        return;
    }

    int nv = this->value;
    if (this->typeProp & 1) {
        this->value = this->smp->getSubtileType(this->subtileIndex);
        this->smp->setSubtileType(this->subtileIndex, nv & MAT_TYPE);
    }
    if (this->typeProp & 2) {
        this->value = this->smp->getSubtileProperties(this->subtileIndex);
        this->smp->setSubtileProperties(this->subtileIndex, nv & ~MAT_TYPE);

    }
    emit this->modified();
}

void EditSmpCommand::redo()
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
}

LevelTabSubtileWidget::~LevelTabSubtileWidget()
{
    delete ui;
}

void LevelTabSubtileWidget::initialize(LevelCelView *v, QUndoStack *us, D1Gfx *g, D1Min *m, D1Sol *s, D1Spt *p, D1Tmi *t, D1Smp *mp)
{
    this->levelCelView = v;
    this->undoStack = us;
    this->gfx = g;
    this->min = m;
    this->sol = s;
    this->spt = p;
    this->tmi = t;
    this->smp = mp;
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

        this->ui->trapNoneRadioButton->setChecked(true);
        this->ui->specCelLineEdit->setText("");

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
    quint8 solFlags = this->sol->getSubtileProperties(subtileIdx);
    quint8 tmiFlags = this->tmi->getSubtileProperties(subtileIdx);
    int sptTrapFlags = this->spt->getSubtileTrapProperty(subtileIdx);
    int sptSpecCel = this->spt->getSubtileSpecProperty(subtileIdx);
    quint8 smpType = this->smp->getSubtileType(subtileIdx);
    quint8 smpFlags = this->smp->getSubtileProperties(subtileIdx);
    std::vector<unsigned> &frames = this->min->getFrameReferences(subtileIdx);

    this->ui->sol0->setChecked((solFlags & PFLAG_BLOCK_PATH) != 0);
    this->ui->sol1->setChecked((solFlags & PFLAG_BLOCK_LIGHT) != 0);
    this->ui->sol2->setChecked((solFlags & PFLAG_BLOCK_MISSILE) != 0);

    if (sptTrapFlags == PTT_NONE)
        this->ui->trapNoneRadioButton->setChecked(true);
    else if (sptTrapFlags == PTT_LEFT)
        this->ui->trapLeftRadioButton->setChecked(true);
    else if (sptTrapFlags == PTT_RIGHT)
        this->ui->trapRightRadioButton->setChecked(true);
    this->ui->specCelLineEdit->setText(QString::number(sptSpecCel));

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

void LevelTabSubtileWidget::selectFrame(int index)
{
    this->ui->framesComboBox->setCurrentIndex(index);
    this->updateFramesSelection(index);
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

    // Build sol editing command and connect it to the views widget
    // to update the label when undo/redo is performed
    EditSolCommand *command = new EditSolCommand(this->sol, subtileIdx, flags);
    QObject::connect(command, &EditSolCommand::modified, this->levelCelView, &LevelCelView::updateFields);

    this->undoStack->push(command);
}

void LevelTabSubtileWidget::updateSolProperty()
{
    quint8 flags = 0;
    if (this->ui->sol0->checkState())
        flags |= PFLAG_BLOCK_PATH;
    if (this->ui->sol1->checkState())
        flags |= PFLAG_BLOCK_LIGHT;
    if (this->ui->sol2->checkState())
        flags |= PFLAG_BLOCK_MISSILE;

    this->setSolProperty(flags);
}

void LevelTabSubtileWidget::setTmiProperty(quint8 flags)
{
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();

    // Build tmi editing command and connect it to the views widget
    // to update the label when undo/redo is performed
    EditTmiCommand *command = new EditTmiCommand(this->tmi, subtileIdx, flags);
    QObject::connect(command, &EditTmiCommand::modified, this->levelCelView, &LevelCelView::updateFields);

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
    EditSmpCommand *command = new EditSmpCommand(this->smp, subtileIdx, flags, 3);
    QObject::connect(command, &EditSmpCommand::modified, this->levelCelView, &LevelCelView::updateFields);

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
    this->setTrapProperty(PTT_NONE);
    this->setTmiProperty(0);
    this->setSmpProperty(MAT_NONE);
    this->updateFields();
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

void LevelTabSubtileWidget::setTrapProperty(int trap)
{
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();

    // Build spt editing command and connect it to the views widget
    // to update the label when undo/redo is performed
    EditSptCommand *command = new EditSptCommand(this->spt, subtileIdx, trap, true);
    QObject::connect(command, &EditSptCommand::trapModified, this->levelCelView, &LevelCelView::updateFields);

    this->undoStack->push(command);
}

void LevelTabSubtileWidget::on_trapNoneRadioButton_clicked()
{
    this->setTrapProperty(PTT_NONE);
}

void LevelTabSubtileWidget::on_trapLeftRadioButton_clicked()
{
    this->setTrapProperty(PTT_LEFT);
}

void LevelTabSubtileWidget::on_trapRightRadioButton_clicked()
{
    this->setTrapProperty(PTT_RIGHT);
}

void LevelTabSubtileWidget::on_specCelLineEdit_returnPressed()
{
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();
    int sptSpecCel = this->ui->specCelLineEdit->text().toInt();

    // Build spt editing command and connect it to the views widget
    // to update the label and refresh the view when undo/redo is performed
    EditSptCommand *command = new EditSptCommand(this->spt, subtileIdx, sptSpecCel, false);
    QObject::connect(command, &EditSptCommand::specModified, this->levelCelView, &LevelCelView::displayFrame);

    this->undoStack->push(command);

    this->on_specCelLineEdit_escPressed();
}

void LevelTabSubtileWidget::on_specCelLineEdit_escPressed()
{
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();
    int sptSpecCel = this->spt->getSubtileSpecProperty(subtileIdx);

    this->ui->specCelLineEdit->setText(QString::number(sptSpecCel));
    this->ui->specCelLineEdit->clearFocus();
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

    if (frameRef < 0) {
        frameRef = 0;
    }
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
    if (frameRef < 0 || frameRef > this->gfx->getFrameCount())
        return; // invalid value -> ignore

    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();
    this->setFrameReference(subtileIdx, index, frameRef);
}

void LevelTabSubtileWidget::on_framesNextButton_clicked()
{
    int index = this->ui->framesComboBox->currentIndex();
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();
    std::vector<unsigned> &frameReferences = this->min->getFrameReferences(subtileIdx);
    int frameRef = frameReferences[index] + 1;

    if (frameRef > this->gfx->getFrameCount()) {
        frameRef = this->gfx->getFrameCount();
    }
    this->setFrameReference(subtileIdx, index, frameRef);
}
