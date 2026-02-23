#pragma once

#include <QDialog>

#include "celview.h"
#include "d1gfx.h"

namespace Ui {
class GfxComponentDialog;
}

class GfxComponentDialog : public QDialog {
    Q_OBJECT

public:
    explicit GfxComponentDialog(QWidget *parent);
    ~GfxComponentDialog();

    void initialize(D1Gfx *gfx, D1GfxComp *gfxComp);

private:
    void updateFields();
    void displayFrame();
    void zoomInOut(int dir);

    void setFrameIndex(int frameIndex);
    void updateGroupIndex();
    void setGroupIndex(int groupIndex);

    std::pair<int, int> getEditRange() const;

private slots:
    void on_labelEdit_returnPressed();
    void on_labelEdit_escPressed();

    void on_framesGroupCheckBox_clicked();
    void on_firstFrameButton_clicked();
    void on_previousFrameButton_clicked();
    void on_nextFrameButton_clicked();
    void on_lastFrameButton_clicked();
    void on_frameIndexEdit_returnPressed();
    void on_frameIndexEdit_escPressed();

    void on_firstGroupButton_clicked();
    void on_previousGroupButton_clicked();
    void on_groupIndexEdit_returnPressed();
    void on_groupIndexEdit_escPressed();
    void on_nextGroupButton_clicked();
    void on_lastGroupButton_clicked();

    void on_zorderDecButton_clicked();
    void on_zorderIncButton_clicked();
    void on_zorderEdit_returnPressed();
    void on_zorderEdit_escPressed();
    void on_xOffsetEdit_returnPressed();
    void on_xOffsetEdit_escPressed();
    void on_yOffsetEdit_returnPressed();
    void on_yOffsetEdit_escPressed();
    void on_prevRefButton_clicked();
    void on_nextRefButton_clicked();
    void on_frameRefEdit_returnPressed();
    void on_frameRefEdit_escPressed();

    void on_zoomOutButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomEdit_returnPressed();
    void on_zoomEdit_escPressed();

    void on_submitButton_clicked();
    void on_cancelButton_clicked();

    void wheelEvent(QWheelEvent *event) override;

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::GfxComponentDialog *ui;
    CelScene celScene = CelScene(this);

    D1Gfx *gfx;
    D1GfxComp *gfxComp;
    int currentGroupIndex = 0;
    int currentFrameIndex = 0;
    QString compLabel;
    D1Gfx *newGfx = nullptr;
    D1GfxComp *newComp;
};
