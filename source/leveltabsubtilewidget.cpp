#include "leveltabsubtilewidget.h"

#include <vector>

#include "levelcelview.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_leveltabsubtilewidget.h"

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

void LevelTabSubtileWidget::initialize(LevelCelView *v, D1Gfx *g, D1Min *m, D1Sol *s, D1Spt *p, D1Tmi *t)
{
    this->levelCelView = v;
    this->gfx = g;
    this->min = m;
    this->sol = s;
    this->spt = p;
    this->tmi = t;
}

void LevelTabSubtileWidget::update()
{
    this->onUpdate = true;

    bool hasSubtile = this->min->getSubtileCount() != 0;

    this->clearButton->setEnabled(hasSubtile);
    this->deleteButton->setEnabled(hasSubtile);

    this->ui->solSettingsGroupBox->setEnabled(hasSubtile);
    this->ui->sptSettingsGroupBox->setEnabled(hasSubtile);
    this->ui->tmiSettingsGroupBox->setEnabled(hasSubtile);

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

        this->ui->framesComboBox->setCurrentIndex(-1);
        this->ui->framesComboBox->setEnabled(false);
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

    if (this->sol->setSubtileProperties(subtileIdx, flags)) {
        this->levelCelView->updateLabel();
    }
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

    if (this->tmi->setSubtileProperties(subtileIdx, flags)) {
        this->levelCelView->updateLabel();
    }
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

void LevelTabSubtileWidget::on_clearPushButtonClicked()
{
    this->setSolProperty(0);
    this->setTrapProperty(PTT_NONE);
    this->setTmiProperty(0);
    this->update();
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

    if (this->spt->setSubtileTrapProperty(subtileIdx, trap)) {
        this->levelCelView->updateLabel();
    }
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

    if (this->spt->setSubtileSpecProperty(subtileIdx, sptSpecCel)) {
        // this->levelCelView->updateLabel();
        this->levelCelView->displayFrame();
    }
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

void LevelTabSubtileWidget::on_framesPrevButton_clicked()
{
    int index = this->ui->framesComboBox->currentIndex();
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();
    std::vector<unsigned> &frameReferences = this->min->getFrameReferences(subtileIdx);
    int frameRef = frameReferences[index] - 1;

    if (frameRef < 0) {
        frameRef = 0;
    }

    if (this->min->setFrameReference(subtileIdx, index, frameRef)) {
        // this->ui->subtilesComboBox->setItemText(index, QString::number(frameRef));
        // this->updateFramesSelection(index);

        // this->levelCelView->updateLabel();
        this->levelCelView->displayFrame();
    }
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
    if (this->min->setFrameReference(subtileIdx, index, frameRef)) {
        // this->ui->subtilesComboBox->setItemText(index, QString::number(frameRef));
        // this->updateFramesSelection(index);

        // this->levelCelView->updateLabel();
        this->levelCelView->displayFrame();
    }
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

    if (this->min->setFrameReference(subtileIdx, index, frameRef)) {
        // this->ui->subtilesComboBox->setItemText(index, QString::number(frameRef));
        // this->updateFramesSelection(index);

        // this->levelCelView->updateLabel();
        this->levelCelView->displayFrame();
    }
}
