#include "gfxcomponentdialog.h"

#include <QMessageBox>

#include "config.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_gfxcomponentdialog.h"

GfxComponentDialog::GfxComponentDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GfxComponentDialog())
{
    ui->setupUi(this);
    this->ui->celGraphicsView->setScene(&this->celScene);
    this->on_zoomEdit_escPressed();

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->labelEdit, SIGNAL(cancel_signal()), this, SLOT(on_labelEdit_escPressed()));    
    QObject::connect(this->ui->frameIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_frameIndexEdit_escPressed()));
    QObject::connect(this->ui->groupIndexEdit, SIGNAL(cancel_signal()), this, SLOT(on_groupIndexEdit_escPressed()));
    QObject::connect(this->ui->zorderEdit, SIGNAL(cancel_signal()), this, SLOT(on_zorderEdit_escPressed()));
    QObject::connect(this->ui->xOffsetEdit, SIGNAL(cancel_signal()), this, SLOT(on_xOffsetEdit_escPressed()));
    QObject::connect(this->ui->yOffsetEdit, SIGNAL(cancel_signal()), this, SLOT(on_yOffsetEdit_escPressed()));
    QObject::connect(this->ui->frameRefEdit, SIGNAL(cancel_signal()), this, SLOT(on_frameRefEdit_escPressed()));
    QObject::connect(this->ui->zoomEdit, SIGNAL(cancel_signal()), this, SLOT(on_zoomEdit_escPressed()));
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
    // Set the maximum group text
    this->ui->groupNumberEdit->setText(QString::number(g->getGroupCount()));
    // Set the maximum frame-reference
    this->ui->frameRefNumberEdit->setText(QString::number(gc->getGFX()->getFrameCount()));
    int compIdx = 0;
    for (int i = 0; i < g->getComponentCount(); i++) {
        if (g->getComponent(i) == gc) {
            compIdx = i;
        }
    }
    // TODO: use D1Gfx copy-constructor?
    this->newGfx = new D1Gfx();
    this->newGfx->setPalette(g->getPalette());
    // this->newGfx->setFilePath(g->getFilePath());
    this->newGfx->addGfx(g);
    this->newComp = this->newGfx->getComponent(compIdx);

    this->displayFrame();
}

void GfxComponentDialog::updateFields()
{
    int count;

    this->ui->labelEdit->setText(this->compLabel);
    // Set the current group text
    count = this->gfx->getGroupCount();
    this->ui->groupIndexEdit->setText(
        QString::number(count != 0 ? this->currentGroupIndex + 1 : 0));

    // Set current and maximum frame text
    int frameIndex = this->currentFrameIndex;
    if (this->ui->framesGroupCheckBox->isChecked() && count != 0) {
        std::pair<int, int> groupFrameIndices = this->gfx->getGroupFrameIndices(this->currentGroupIndex);
        frameIndex -= groupFrameIndices.first;
        count = groupFrameIndices.second - groupFrameIndices.first + 1;
    } else {
        count = this->gfx->getFrameCount();
    }
    this->ui->frameIndexEdit->setText(
        QString::number(count != 0 ? frameIndex + 1 : 0));
    this->ui->frameNumberEdit->setText(QString::number(count));

    // set the details
    if (count != 0) {
        D1GfxCompFrame *frame = this->newComp->getCompFrame(this->currentFrameIndex);
        this->ui->zorderEdit->setText(QString::number(frame->cfZOrder));
        this->ui->xOffsetEdit->setText(QString::number(frame->cfOffsetX));
        this->ui->yOffsetEdit->setText(QString::number(frame->cfOffsetY));
        this->ui->frameRefEdit->setText(QString::number(frame->cfFrameRef));
    } else {
        this->ui->zorderEdit->setText("");
        this->ui->xOffsetEdit->setText("");
        this->ui->yOffsetEdit->setText("");
        this->ui->frameRefEdit->setText("");
    }
}

void GfxComponentDialog::displayFrame()
{
    this->updateFields();
    this->celScene.clear();

    QImage celFrame = this->newGfx->getFrameCount() != 0 ? this->newGfx->getFrameImage(this->currentFrameIndex, -1) : QImage();

    this->celScene.setBackgroundBrush(QColor(Config::getGraphicsBackgroundColor()));

    // Building background of the width/height of the CEL frame
    QImage celFrameBackground = QImage(celFrame.width(), celFrame.height(), QImage::Format_ARGB32);
    celFrameBackground.fill(QColor(Config::getGraphicsTransparentColor()));

    // Resize the scene rectangle to include some padding around the CEL frame
    this->celScene.setSceneRect(0, 0,
        CEL_SCENE_MARGIN + celFrame.width() + CEL_SCENE_MARGIN,
        CEL_SCENE_MARGIN + celFrame.height() + CEL_SCENE_MARGIN);
    // ui->celGraphicsView->adjustSize();

    // Add the backgrond and CEL frame while aligning it in the center
    this->celScene.addPixmap(QPixmap::fromImage(celFrameBackground))
        ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
    this->celScene.addPixmap(QPixmap::fromImage(celFrame))
        ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
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
    this->compLabel = this->ui->labelEdit->text();

    this->on_labelEdit_escPressed();
}

void GfxComponentDialog::on_labelEdit_escPressed()
{
    this->updateFields();
    this->ui->labelEdit->clearFocus();
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

void GfxComponentDialog::on_frameIndexEdit_escPressed()
{
    // update frameIndexEdit
    this->updateFields();
    this->ui->frameIndexEdit->clearFocus();
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

void GfxComponentDialog::on_zorderDecButton_clicked()
{
    D1GfxCompFrame *frame = this->newComp->getCompFrame(this->currentFrameIndex);
    frame->cfZOrder--;

    this->displayFrame();
}

void GfxComponentDialog::on_zorderIncButton_clicked()
{
    D1GfxCompFrame *frame = this->newComp->getCompFrame(this->currentFrameIndex);
    frame->cfZOrder++;

    this->displayFrame();
}

void GfxComponentDialog::on_zorderEdit_returnPressed()
{
    int zorder = this->ui->zorderEdit->text().toInt();

    D1GfxCompFrame *frame = this->newComp->getCompFrame(this->currentFrameIndex);
    frame->cfZOrder = zorder;

    this->displayFrame();

    this->on_zorderEdit_escPressed();
}

void GfxComponentDialog::on_zorderEdit_escPressed()
{
    this->updateFields();
    this->ui->zorderEdit->clearFocus();
}

void GfxComponentDialog::on_xOffsetEdit_returnPressed()
{
    int offset = this->ui->xOffsetEdit->text().toInt();

    D1GfxCompFrame *frame = this->newComp->getCompFrame(this->currentFrameIndex);
    frame->cfOffsetX = offset;

    this->displayFrame();

    this->on_xOffsetEdit_escPressed();
}

void GfxComponentDialog::on_xOffsetEdit_escPressed()
{
    this->updateFields();
    this->ui->xOffsetEdit->clearFocus();
}

void GfxComponentDialog::on_yOffsetEdit_returnPressed()
{
    int offset = this->ui->yOffsetEdit->text().toInt();

    D1GfxCompFrame *frame = this->newComp->getCompFrame(this->currentFrameIndex);
    frame->cfOffsetY = offset;

    this->displayFrame();

    this->on_yOffsetEdit_escPressed();
}

void GfxComponentDialog::on_yOffsetEdit_escPressed()
{
    this->updateFields();
    this->ui->yOffsetEdit->clearFocus();
}

void GfxComponentDialog::on_prevRefButton_clicked()
{
    D1GfxCompFrame *frame = this->newComp->getCompFrame(this->currentFrameIndex);
    if (frame->cfFrameRef > 0) {
        frame->cfFrameRef--;
    }

    this->displayFrame();
}

void GfxComponentDialog::on_nextRefButton_clicked()
{
    D1GfxCompFrame *frame = this->newComp->getCompFrame(this->currentFrameIndex);
    if (frame->cfFrameRef < (unsigned)this->newComp->getGFX()->getFrameCount()) {
        frame->cfFrameRef++;
    }

    this->displayFrame();
}

void GfxComponentDialog::on_frameRefEdit_returnPressed()
{
    int frameRef = this->ui->frameRefEdit->text().toInt();

    D1GfxCompFrame *frame = this->newComp->getCompFrame(this->currentFrameIndex);
    frame->cfFrameRef = frameRef;

    this->displayFrame();

    this->on_frameRefEdit_escPressed();
}

void GfxComponentDialog::on_frameRefEdit_escPressed()
{
    this->updateFields();
    this->ui->frameRefEdit->clearFocus();
}

void GfxComponentDialog::on_zoomOutButton_clicked()
{
    this->celScene.zoomOut();
    this->on_zoomEdit_escPressed();
}

void GfxComponentDialog::on_zoomInButton_clicked()
{
    this->celScene.zoomIn();
    this->on_zoomEdit_escPressed();
}

void GfxComponentDialog::on_zoomEdit_returnPressed()
{
    QString zoom = this->ui->zoomEdit->text();

    this->celScene.setZoom(zoom);

    this->on_zoomEdit_escPressed();
}

void GfxComponentDialog::on_zoomEdit_escPressed()
{
    this->ui->zoomEdit->setText(this->celScene.zoomText());
    this->ui->zoomEdit->clearFocus();
}

void GfxComponentDialog::on_submitButton_clicked()
{
    D1GfxComp *gc = this->gfxComp;
    gc->setLabel(this->compLabel);

    bool change = false;
    for (int i = 0; i < gc->getCompFrameCount(); i++) {
        D1GfxCompFrame *frame = gc->getCompFrame(i);
        D1GfxCompFrame *newFrame = this->newComp->getCompFrame(i);
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

    this->on_cancelButton_clicked();

    dMainWindow().updateWindow();
}

void GfxComponentDialog::on_cancelButton_clicked()
{
    this->close();

    delete this->newGfx;
    this->newGfx = nullptr;
}

void GfxComponentDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
