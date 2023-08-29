#include "leveltabframewidget.h"

#include <QApplication>
#include <QStyle>

#include "d1gfx.h"
#include "levelcelview.h"
#include "mainwindow.h"
#include "pushbuttonwidget.h"
#include "ui_leveltabframewidget.h"

EditFrameTypeCommand::EditFrameTypeCommand(D1Gfx *g, int fi, D1CEL_FRAME_TYPE t)
    : QUndoCommand(nullptr)
    , gfx(g)
    , frameIndex(fi)
    , type(t)
{
}

void EditFrameTypeCommand::undo()
{
    if (this->gfx.isNull()) {
        this->setObsolete(true);
        return;
    }

    D1GfxFrame *frame = this->gfx->getFrame(this->frameIndex);
    D1CEL_FRAME_TYPE nt = this->type;
    this->type = frame->getFrameType();
    frame->setFrameType(nt);

    emit this->modified();
}

void EditFrameTypeCommand::redo()
{
    this->undo();
}

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

void LevelTabFrameWidget::initialize(LevelCelView *v, QUndoStack *us)
{
    this->levelCelView = v;
    this->undoStack = us;
}

void LevelTabFrameWidget::setTileset(D1Tileset *ts)
{
    this->gfx = ts->gfx;
}

void LevelTabFrameWidget::updateFields()
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

    // Build frame editing command and connect it to the current widget
    // to update the label and validate when undo/redo is performed
    EditFrameTypeCommand *command = new EditFrameTypeCommand(this->gfx, frameIdx, (D1CEL_FRAME_TYPE)index);
    QObject::connect(command, &EditFrameTypeCommand::modified, this, &LevelTabFrameWidget::updateTab);

    this->undoStack->push(command);
}

void LevelTabFrameWidget::updateTab()
{
    this->updateFields();
    this->levelCelView->updateLabel();
}
