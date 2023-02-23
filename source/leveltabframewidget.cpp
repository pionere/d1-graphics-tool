#include "leveltabframewidget.h"

#include <QApplication>

#include "d1gfx.h"
#include "levelcelview.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_leveltabframewidget.h"

LevelTabFrameWidget::LevelTabFrameWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LevelTabFrameWidget())
{
    ui->setupUi(this);

    QLayout *layout = this->ui->buttonsHorizontalLayout;
    this->deleteButton = PushButtonWidget::addButton(this, layout, QStyle::SP_TrashIcon, tr("Delete the current frame"), this, &LevelTabFrameWidget::on_deletePushButtonClicked);
}

LevelTabFrameWidget::~LevelTabFrameWidget()
{
    delete ui;
}

void LevelTabFrameWidget::initialize(LevelCelView *v, D1Gfx *g)
{
    this->levelCelView = v;
    this->gfx = g;
}

void LevelTabFrameWidget::update()
{
    bool hasFrame = this->gfx->getFrameCount() != 0;

    this->deleteButton->setEnabled(hasFrame);

    this->ui->frameTypeComboBox->setEnabled(hasFrame);

    if (!hasFrame) {
        this->ui->frameTypeComboBox->setCurrentIndex(-1);
        return;
    }

    int frameIdx = this->levelCelView->getCurrentFrameIndex();
    D1GfxFrame *frame = this->gfx->getFrame(frameIdx);
    D1CEL_FRAME_TYPE frameType = frame->getFrameType();
    this->ui->frameTypeComboBox->setCurrentIndex((int)frameType);

    this->validate();
}

void LevelTabFrameWidget::on_deletePushButtonClicked()
{
    dMainWindow().on_actionDel_Frame_triggered();
}


void LevelTabFrameWidget::validate()
{
    int frameIdx = this->levelCelView->getCurrentFrameIndex();
    D1GfxFrame *frame = this->gfx->getFrame(frameIdx);

    QString error, warning;
    D1CelTilesetFrame::validate(frame, error, warning);

    if (!error.isEmpty()) {
        this->ui->frameTypeMsgLabel->setText(error);
        this->ui->frameTypeMsgLabel->setStyleSheet("color: rgb(255, 0, 0);");
    } else if (!warning.isEmpty()) {
        this->ui->frameTypeMsgLabel->setText(warning);
        this->ui->frameTypeMsgLabel->setStyleSheet("color: rgb(0, 255, 0);");
    } else {
        this->ui->frameTypeMsgLabel->setText("");
    }
}

void LevelTabFrameWidget::on_frameTypeComboBox_activated(int index)
{
    int frameIdx = this->levelCelView->getCurrentFrameIndex();

    if (this->gfx->setFrameType(frameIdx, (D1CEL_FRAME_TYPE)index)) {
        this->validate();
        this->levelCelView->updateLabel();
    }
}
