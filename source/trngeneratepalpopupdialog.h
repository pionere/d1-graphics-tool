#pragma once

#include <QDialog>

#include "trngeneratedialog.h"

class D1Pal;
class D1Trn;
class TrnGenerateDialog;

namespace Ui {
class TrnGeneratePalPopupDialog;
} // namespace Ui

class TrnGeneratePalPopupDialog : public QDialog {
    Q_OBJECT

public:
    explicit TrnGeneratePalPopupDialog(TrnGenerateDialog *view);
    ~TrnGeneratePalPopupDialog();

    void initialize(D1Pal *pal, D1Trn *trn, int trnIndex);
    void on_colorDblClicked(int colorIndex);

private slots:
    void keyPressEvent(QKeyEvent *event) override;

private:
    Ui::TrnGeneratePalPopupDialog *ui;
    PalScene popupScene = PalScene(this);

    TrnGenerateDialog *view;
    D1Trn *trn;
    int trnIndex;
};
