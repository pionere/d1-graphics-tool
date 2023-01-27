#include "leveltabsubtilewidget.h"

#include "levelcelview.h"
#include "mainwindow.h"
#include "ui_leveltabsubtilewidget.h"

QPushButton *LevelTabSubTileWidget::addButton(QStyle::StandardPixmap type, QString tooltip, void (LevelTabSubTileWidget::*callback)(void))
{
    QPushButton *button = new QPushButton(this->style()->standardIcon(type), "", nullptr);
    constexpr int iconSize = 16;
    button->setToolTip(tooltip);
    button->setIconSize(QSize(iconSize, iconSize));
    button->setMinimumSize(iconSize, iconSize);
    button->setMaximumSize(iconSize, iconSize);
    this->ui->buttonsHorizontalLayout->addWidget(button);

    QObject::connect(button, &QPushButton::clicked, this, callback);
    return button;
}

LevelTabSubTileWidget::LevelTabSubTileWidget()
    : QWidget(nullptr)
    , ui(new Ui::LevelTabSubTileWidget)
{
    ui->setupUi(this);

    this->addButton(QStyle::SP_DialogResetButton, tr("Reset flags"), &LevelTabSubTileWidget::on_clearPushButtonClicked);
    this->addButton(QStyle::SP_TrashIcon, tr("Delete the current subtile"), &LevelTabSubTileWidget::on_deletePushButtonClicked);
}

LevelTabSubTileWidget::~LevelTabSubTileWidget()
{
    delete ui;
}

void LevelTabSubTileWidget::initialize(LevelCelView *v, D1Gfx *g, D1Min *m, D1Sol *s, D1Tmi *t)
{
    this->levelCelView = v;
    this->gfx = g;
    this->min = m;
    this->sol = s;
    this->tmi = t;
}

void LevelTabSubTileWidget::update()
{
    this->onUpdate = true;

    bool hasSubtile = this->min->getSubtileCount() != 0;

    ((QPushButton *)this->ui->buttonsHorizontalLayout->children()[1])->setEnabled(hasSubtile); // Clear button
    ((QPushButton *)this->ui->buttonsHorizontalLayout->children()[2])->setEnabled(hasSubtile); // Delete button

    this->ui->sol0->setEnabled(hasSubtile);
    this->ui->sol1->setEnabled(hasSubtile);
    this->ui->sol2->setEnabled(hasSubtile);
    this->ui->sol3->setEnabled(hasSubtile);
    this->ui->sol4->setEnabled(hasSubtile);
    this->ui->sol5->setEnabled(hasSubtile);
    this->ui->sol7->setEnabled(hasSubtile);

    this->ui->tmi0->setEnabled(hasSubtile);
    this->ui->tmi1->setEnabled(hasSubtile);
    this->ui->tmi2->setEnabled(hasSubtile);
    this->ui->tmi3->setEnabled(hasSubtile);
    this->ui->tmi4->setEnabled(hasSubtile);
    this->ui->tmi5->setEnabled(hasSubtile);
    this->ui->tmi6->setEnabled(hasSubtile);

    this->ui->framesComboBox->setEnabled(hasSubtile);

    if (!hasSubtile) {
        this->ui->sol0->setChecked(false);
        this->ui->sol1->setChecked(false);
        this->ui->sol2->setChecked(false);
        this->ui->sol3->setChecked(false);
        this->ui->sol4->setChecked(false);
        this->ui->sol5->setChecked(false);
        this->ui->sol7->setChecked(false);

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
    QList<quint16> &frames = this->min->getFrameReferences(subtileIdx);

    this->ui->sol0->setChecked((solFlags & 1 << 0) != 0);
    this->ui->sol1->setChecked((solFlags & 1 << 1) != 0);
    this->ui->sol2->setChecked((solFlags & 1 << 2) != 0);
    this->ui->sol3->setChecked((solFlags & 1 << 3) != 0);
    this->ui->sol4->setChecked((solFlags & 1 << 4) != 0);
    this->ui->sol5->setChecked((solFlags & 1 << 5) != 0);
    this->ui->sol7->setChecked((solFlags & 1 << 7) != 0);

    this->ui->tmi0->setChecked((tmiFlags & 1 << 0) != 0);
    this->ui->tmi1->setChecked((tmiFlags & 1 << 1) != 0);
    this->ui->tmi2->setChecked((tmiFlags & 1 << 2) != 0);
    this->ui->tmi3->setChecked((tmiFlags & 1 << 3) != 0);
    this->ui->tmi4->setChecked((tmiFlags & 1 << 4) != 0);
    this->ui->tmi5->setChecked((tmiFlags & 1 << 5) != 0);
    this->ui->tmi6->setChecked((tmiFlags & 1 << 6) != 0);

    // update combo box of the frames
    while (this->ui->framesComboBox->count() > frames.count())
        this->ui->framesComboBox->removeItem(0);
    int i = 0;
    while (this->ui->framesComboBox->count() < frames.count())
        this->ui->framesComboBox->insertItem(0, QString::number(++i));
    for (i = 0; i < frames.count(); i++) {
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

void LevelTabSubTileWidget::selectFrame(int index)
{
    this->ui->framesComboBox->setCurrentIndex(index);
    this->updateFramesSelection(index);
}

void LevelTabSubTileWidget::updateFramesSelection(int index)
{
    this->lastFrameEntryIndex = index;
    int frameRef = this->ui->framesComboBox->currentText().toInt();

    this->ui->framesPrevButton->setEnabled(frameRef > 0);
    this->ui->framesNextButton->setEnabled(frameRef < this->gfx->getFrameCount());
}

void LevelTabSubTileWidget::setSolProperty(quint8 flags)
{
    int subTileIdx = this->levelCelView->getCurrentSubtileIndex();

    if (this->sol->setSubtileProperties(subTileIdx, flags)) {
        this->levelCelView->updateLabel();
    }
}

void LevelTabSubTileWidget::updateSolProperty()
{
    quint8 flags = 0;
    if (this->ui->sol0->checkState())
        flags |= 1 << 0;
    if (this->ui->sol1->checkState())
        flags |= 1 << 1;
    if (this->ui->sol2->checkState())
        flags |= 1 << 2;
    if (this->ui->sol3->checkState())
        flags |= 1 << 3;
    if (this->ui->sol4->checkState())
        flags |= 1 << 4;
    if (this->ui->sol5->checkState())
        flags |= 1 << 5;
    if (this->ui->sol7->checkState())
        flags |= 1 << 7;

    this->setSolProperty(flags);
}

void LevelTabSubTileWidget::setTmiProperty(quint8 flags)
{
    int subTileIdx = this->levelCelView->getCurrentSubtileIndex();

    if (this->tmi->setSubtileProperties(subTileIdx, flags)) {
        this->levelCelView->updateLabel();
    }
}

void LevelTabSubTileWidget::updateTmiProperty()
{
    quint8 flags = 0;
    if (this->ui->tmi0->checkState())
        flags |= 1 << 0;
    if (this->ui->tmi1->checkState())
        flags |= 1 << 1;
    if (this->ui->tmi2->checkState())
        flags |= 1 << 2;
    if (this->ui->tmi3->checkState())
        flags |= 1 << 3;
    if (this->ui->tmi4->checkState())
        flags |= 1 << 4;
    if (this->ui->tmi5->checkState())
        flags |= 1 << 5;
    if (this->ui->tmi6->checkState())
        flags |= 1 << 6;

    this->setTmiProperty(flags);
}

void LevelTabSubTileWidget::on_clearPushButtonClicked()
{
    this->setSolProperty(0);
    this->setTmiProperty(0);
    this->update();
}

void LevelTabSubTileWidget::on_deletePushButtonClicked()
{
    MainWindow *mw = (MainWindow *)this->window();
    mw->on_actionDel_Subtile_triggered();
}

void LevelTabSubTileWidget::on_sol0_clicked()
{
    this->updateSolProperty();
}

void LevelTabSubTileWidget::on_sol1_clicked()
{
    this->updateSolProperty();
}

void LevelTabSubTileWidget::on_sol2_clicked()
{
    this->updateSolProperty();
}

void LevelTabSubTileWidget::on_sol3_clicked()
{
    this->updateSolProperty();
}

void LevelTabSubTileWidget::on_sol4_clicked()
{
    this->updateSolProperty();
}

void LevelTabSubTileWidget::on_sol5_clicked()
{
    this->updateSolProperty();
}

void LevelTabSubTileWidget::on_sol7_clicked()
{
    this->updateSolProperty();
}

void LevelTabSubTileWidget::on_tmi0_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubTileWidget::on_tmi1_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubTileWidget::on_tmi2_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubTileWidget::on_tmi3_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubTileWidget::on_tmi4_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubTileWidget::on_tmi5_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubTileWidget::on_tmi6_clicked()
{
    this->updateTmiProperty();
}

void LevelTabSubTileWidget::on_framesPrevButton_clicked()
{
    int index = this->ui->framesComboBox->currentIndex();
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();
    QList<quint16> &frameReferences = this->min->getFrameReferences(subtileIdx);
    int frameRef = frameReferences[index] - 1;

    if (frameRef < 0) {
        frameRef = 0;
    }

    if (this->min->setFrameReference(subtileIdx, index, frameRef)) {
        // this->ui->subtilesComboBox->setItemText(index, QString::number(frameRef));
        // this->updateFramesSelection(index);

        this->levelCelView->updateLabel();
        this->levelCelView->displayFrame();
    }
}

void LevelTabSubTileWidget::on_framesComboBox_activated(int index)
{
    if (!this->onUpdate) {
        this->updateFramesSelection(index);
    }
}

void LevelTabSubTileWidget::on_framesComboBox_currentTextChanged(const QString &arg1)
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

        this->levelCelView->updateLabel();
        this->levelCelView->displayFrame();
    }
}

void LevelTabSubTileWidget::on_framesNextButton_clicked()
{
    int index = this->ui->framesComboBox->currentIndex();
    int subtileIdx = this->levelCelView->getCurrentSubtileIndex();
    QList<quint16> &frameReferences = this->min->getFrameReferences(subtileIdx);
    int frameRef = frameReferences[index] + 1;

    if (frameRef > this->gfx->getFrameCount()) {
        frameRef = this->gfx->getFrameCount();
    }

    if (this->min->setFrameReference(subtileIdx, index, frameRef)) {
        // this->ui->subtilesComboBox->setItemText(index, QString::number(frameRef));
        // this->updateFramesSelection(index);

        this->levelCelView->updateLabel();
        this->levelCelView->displayFrame();
    }
}
