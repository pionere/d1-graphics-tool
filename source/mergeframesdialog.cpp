#include "mergeframesdialog.h"

#include <QMessageBox>

#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_mergeframesdialog.h"

MergeFramesDialog::MergeFramesDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MergeFramesDialog())
{
    ui->setupUi(this);
}

MergeFramesDialog::~MergeFramesDialog()
{
    delete ui;
}

void MergeFramesDialog::initialize(D1Gfx *g)
{
    this->gfx = g;

    this->ui->rangeFromLineEdit->setText("");
    this->ui->rangeToLineEdit->setText("");
}

void MergeFramesDialog::on_submitButton_clicked()
{
    int firstFrameIdx = this->ui->rangeFromLineEdit->nonNegInt();
    int lastFrameIdx = this->ui->rangeToLineEdit->nonNegInt();
    int frameCount = this->gfx->getFrameCount();

    if (firstFrameIdx == 0) {
        firstFrameIdx = 1;
    }
    firstFrameIdx--;
    if (lastFrameIdx == 0) {
        lastFrameIdx = frameCount;
    }
    lastFrameIdx--;
    if (lastFrameIdx >= frameCount) {
        lastFrameIdx = frameCount - 1;
    }

    for (int frameIdx = firstFrameIdx + 1; frameIdx <= lastFrameIdx; frameIdx++) {
        D1GfxFrame* baseFrame = this->gfx->getFrame(firstFrameIdx);
        D1GfxFrame* currFrame = this->gfx->getFrame(frameIdx);
        if (baseFrame->getWidth() != currFrame->getWidth() || baseFrame->getHeight() != currFrame->getHeight()) {
            QMessageBox::critical(nullptr, tr("Error"), tr("Mismatching framesize (%1).").arg(frameIdx + 1));
            return;
        }
    }

    this->close();

    if (firstFrameIdx >= lastFrameIdx) {
        return;
    }

    MergeFramesParam params;

    params.rangeFrom = firstFrameIdx;
    params.rangeTo = lastFrameIdx;

    dMainWindow().mergeFrames(params);
}

void MergeFramesDialog::on_cancelButton_clicked()
{
    this->close();
}

void MergeFramesDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
