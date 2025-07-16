#include "gfxcomponentdialog.h"

#include <QMessageBox>

#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_gfxcomponentdialog.h"

GfxComponentDialog::GfxComponentDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GfxComponentDialog())
{
    ui->setupUi(this);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->labelEdit, SIGNAL(cancel_signal()), this, SLOT(on_labelEdit_escPressed()));    
    QObject::connect(this->ui->frameIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_frameIndexEdit_escPressed()));
    QObject::connect(this->ui->groupIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_groupIndexEdit_escPressed()));
    QObject::connect(this->ui->zorderEdit, SIGNAL(cancel_signal()), this, SLOT(on_zorderEdit_escPressed()));
    QObject::connect(this->ui->xOffsetEdit, SIGNAL(cancel_signal()), this, SLOT(on_xOffsetEdit_escPressed()));
    QObject::connect(this->ui->yOffsetEdit, SIGNAL(cancel_signal()), this, SLOT(on_yOffsetEdit_escPressed()));
    QObject::connect(this->ui->frameRefEdit, SIGNAL(cancel_signal()), this, SLOT(on_frameRefEdit_escPressed()));
}

GfxComponentDialog::~GfxComponentDialog()
{
    delete ui;
}

void GfxComponentDialog::initialize(D1Gfx* g, D1GfxComp *gc)
{
    this->gfx = g;
    this->gfxComp = gc;
    this->compLabel = gc->getLabel();

    if (this->currentFrameIndex >= this->gfx->getFrameCount()) {
        this->currentFrameIndex = 0;
    }
    this->updateGroupIndex();
    this->ui->frameNumberEdit->setText(QString::number(gc->getCompFrameCount()));
    for (int i = 0; i < gc->getCompFrameCount(); i++) {
        D1GfxCompFrame *frame = gc->getCompFrame(i);
        this->compFrames.push_back(*gc);
    }

    this->updateFields();
}

void GfxComponentDialog::updateFields()
{
    this->ui->labelLineEdit->setText(this->compLabel);
    this->ui->frameIndexEdit->setText(QString::number(this->currentFrameIndex + 1));
    D1GfxCompFrame *frame = this->gfxComp->getCompFrame(this->currentFrameIndex);
    this->ui->zorderLineEdit->setText(QString::number(this->frame->cfZOrder));
    this->ui->xOffsetLineEdit->setText(QString::number(this->frame->cfOffsetX));
    this->ui->yOffsetLineEdit->setText(QString::number(this->frame->cfOffsetY));
    this->ui->frameRefLineEdit->setText(QString::number(this->frame->cfFrameRef));
}

void GfxComponentDialog::displayFrame()
{
    this->updateFields();
}

void GfxComponentDialog::setFrameIndex(int frameIndex)
{
    const int frameCount = this->gfx->getFrameCount();
    if (frameCount == 0) {
        // this->currentFrameIndex = 0;
        // this->currentGroupIndex = 0;
        // this->displayFrame();
        return;
    }
    if (frameIndex >= frameCount) {
        frameIndex = frameCount - 1;
    } else if (frameIndex < 0) {
        frameIndex = 0;
    }
    this->currentFrameIndex = frameIndex;
    this->updateGroupIndex();

    this->displayFrame();
}

void GfxComponentDialog::updateGroupIndex()
{
    int i = 0;

    for (; i < this->gfx->getGroupCount(); i++) {
        std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(i);

        if (this->currentFrameIndex >= groupFrameIndices.first
            && this->currentFrameIndex <= groupFrameIndices.second) {
            break;
        }
    }
    this->currentGroupIndex = i;
}

void GfxComponentDialog::setGroupIndex(int groupIndex)
{
    const int groupCount = this->gfx->getGroupCount();
    if (groupCount == 0) {
        // this->currentFrameIndex = 0;
        // this->currentGroupIndex = 0;
        // this->displayFrame();
        return;
    }
    if (groupIndex >= groupCount) {
        groupIndex = groupCount - 1;
    } else if (groupIndex < 0) {
        groupIndex = 0;
    }
    std::pair<int, int> prevGroupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);
    int frameIndex = this->currentFrameIndex - prevGroupFrameIndices.first;
    std::pair<int, int> newGroupFrameIndices = this->gfx->getGroupFrameIndices(groupIndex);
    this->currentGroupIndex = groupIndex;
    this->currentFrameIndex = std::min(newGroupFrameIndices.first + frameIndex, newGroupFrameIndices.second);

    this->displayFrame();
}

void GfxComponentDialog::on_labelEdit_returnPressed()
{
    this->compLabel = this->ui->frameIndexEdit->text();

    this->on_labelEdit_escPressed();
}

void GfxComponentDialog::on_labelEdit_escPressed()
{
    this->updateFields();
}

void GfxComponentDialog::on_framesGroupCheckBox_clicked()
{
    // update frameIndexEdit and frameNumberEdit
    this->updateFields();
}

void GfxComponentDialog::on_firstFrameButton_clicked()
{
    int nextFrameIndex = 0;
    const bool moveFrame = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).first;
        if (moveFrame) {
            for (int i = this->currentFrameIndex; i > nextFrameIndex; i--) {
                this->gfx->swapFrames(i, i - 1);
            }
        }
    } else {
        if (moveFrame) {
            this->gfx->swapFrames(UINT_MAX, this->currentFrameIndex);
        }
    }
    this->setFrameIndex(nextFrameIndex);
}

void GfxComponentDialog::on_previousFrameButton_clicked()
{
    int nextFrameIndex = this->currentFrameIndex - 1;
    const bool moveFrame = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = std::max(nextFrameIndex, this->gfx->getGroupFrameIndices(this->currentGroupIndex).first);
    }
    if (moveFrame) {
        this->gfx->swapFrames(nextFrameIndex, this->currentFrameIndex);
    }
    this->setFrameIndex(nextFrameIndex);
}

void GfxComponentDialog::on_nextFrameButton_clicked()
{
    int nextFrameIndex = this->currentFrameIndex + 1;
    const bool moveFrame = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = std::min(nextFrameIndex, this->gfx->getGroupFrameIndices(this->currentGroupIndex).second);
    }
    if (moveFrame) {
        this->gfx->swapFrames(this->currentFrameIndex, nextFrameIndex);
    }
    this->setFrameIndex(nextFrameIndex);
}

void GfxComponentDialog::on_lastFrameButton_clicked()
{
    int nextFrameIndex = this->gfx->getFrameCount() - 1;
    const bool moveFrame = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        nextFrameIndex = this->gfx->getGroupFrameIndices(this->currentGroupIndex).second;
        if (moveFrame) {
            for (int i = this->currentFrameIndex; i < nextFrameIndex; i++) {
                this->gfx->swapFrames(i, i + 1);
            }
        }
    } else {
        if (moveFrame) {
            this->gfx->swapFrames(this->currentFrameIndex, UINT_MAX);
        }
    }
    this->setFrameIndex(nextFrameIndex);
}

void GfxComponentDialog::on_frameIndexEdit_returnPressed()
{
    int nextFrameIndex = this->ui->frameIndexEdit->text().toInt() - 1;

    if (this->ui->framesGroupCheckBox->isChecked() && this->gfx->getGroupCount() != 0) {
        std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);
        nextFrameIndex += groupFrameIndices.first;
        nextFrameIndex = std::max(nextFrameIndex, groupFrameIndices.first);
        nextFrameIndex = std::min(nextFrameIndex, groupFrameIndices.second);
    }
    this->setFrameIndex(nextFrameIndex);

    this->on_frameIndexEdit_escPressed();
}

void GfxComponentDialog::on_firstGroupButton_clicked()
{
    this->setGroupIndex(0);
}

void GfxComponentDialog::on_previousGroupButton_clicked()
{
    this->setGroupIndex(this->currentGroupIndex - 1);
}

void GfxComponentDialog::on_groupIndexEdit_returnPressed()
{
    int groupIndex = this->ui->groupIndexEdit->text().toInt() - 1;

    this->setGroupIndex(groupIndex);

    this->on_groupIndexEdit_escPressed();
}

void GfxComponentDialog::on_groupIndexEdit_escPressed()
{
    // update groupIndexEdit
    this->updateFields();
    this->ui->groupIndexEdit->clearFocus();
}

void GfxComponentDialog::on_nextGroupButton_clicked()
{
    this->setGroupIndex(this->currentGroupIndex + 1);
}

void GfxComponentDialog::on_lastGroupButton_clicked()
{
    this->setGroupIndex(this->gfx->getGroupCount() - 1);
}

void GfxComponentDialog::on_zorderEdit_returnPressed()
{
    int zorder = this->ui->zorderEdit->text().toInt();

    D1GfxCompFrame *frame = this->gfxComp->getCompFrame(this->currentFrameIndex);
    frame->cfZOrder = zorder;

    this->on_zorderEdit_escPressed();
}

void GfxComponentDialog::on_zorderEdit_escPressed()
{
    this->updateFields();
}

void GfxComponentDialog::on_xOffsetEdit_returnPressed()
{
    int offset = this->ui->zorderEdit->text().toInt();

    D1GfxCompFrame *frame = this->gfxComp->getCompFrame(this->currentFrameIndex);
    frame->cfOffsetX = offset;

    this->on_xOffsetEdit_escPressed();
}

void GfxComponentDialog::on_xOffsetEdit_escPressed()
{
    this->updateFields();
}
void GfxComponentDialog::on_yOffsetEdit_returnPressed()
{
    int offset = this->ui->zorderEdit->text().toInt();

    D1GfxCompFrame *frame = this->gfxComp->getCompFrame(this->currentFrameIndex);
    frame->cfOffsetY = offset;

    this->on_yOffsetEdit_escPressed();
}
void GfxComponentDialog::on_yOffsetEdit_escPressed()
{
    this->updateFields();
}
void GfxComponentDialog::on_frameRefEdit_returnPressed()
{
    int frameRef = this->ui->zorderEdit->text().toInt();

    D1GfxCompFrame *frame = this->gfxComp->getCompFrame(this->currentFrameIndex);
    frame->cfFrameRef = frameRef;

    this->on_frameRefEdit_escPressed();
}
void GfxComponentDialog::on_frameRefEdit_escPressed()
{
    this->updateFields();
}

void GfxComponentDialog::on_submitButton_clicked()
{
    D1GfxComp * g = this->gfxComp;
    g->setLabel(this->compLabel);

    bool change = false;
    for (int i = 0; i < g->getCompFrameCount(); i++) {
        D1GfxCompFrame *frame = g->getCompFrame(i);
        D1GfxCompFrame *newFrame = &this->compFrames[i];
        if (frame->cfZOrder != newFrame->cfZOrder) {
            frame->cfZOrder = newFrame->cfZOrder;
            change = true;
        }
        if (frame->cfOffsetX != newFrame->cfOffsetX) {
            frame->cfOffsetX = newFrame->cfOffsetX;
            change = true;
        }
        if (frame->cfOffsetY != newFrame->cfOffsetY) {
            frame->cfOffsetY = newFrame->cfOffsetY;
            change = true;
        }
        if (frame->cfFrameRef != newFrame->cfFrameRef) {
            frame->cfFrameRef = newFrame->cfFrameRef;
            change = true;
        }
    }

    // TODO: report change?

    this->close();

    dMainWindow().updateWindow();
}

void GfxComponentDialog::on_cancelButton_clicked()
{
    this->close();
}

void GfxComponentDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
