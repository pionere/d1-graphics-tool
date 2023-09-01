#pragma once

#include <vector>

#include <QDialog>
#include <QList>
#include <QPointer>

#include "trngeneratecolentrywidget.h"
#include "trngeneratepalentrywidget.h"

class D1Pal;

typedef struct GenerateTrnColor {
    int firstcolor;
    int lastcolor;
    bool shadecolor;
} GenerateTrnColor;

class GenerateTrnParam {
public:
    std::vector<GenerateTrnColor> colors;
    std::vector<D1Pal *> pals;
    int mode;
};

namespace Ui {
class TrnGenerateDialog;
}

class TrnGenerateDialog : public QDialog {
    Q_OBJECT

public:
    explicit TrnGenerateDialog(QWidget *parent);
    ~TrnGenerateDialog();

    void initialize(D1Pal *pal);

    void on_actionDelRange_triggered(TrnGenerateColEntryWidget *caller);
    void on_actionDelPalette_triggered(TrnGeneratePalEntryWidget *caller);

private slots:
    void on_actionAddRange_triggered();
    void on_actionAddPalette_triggered();

    void on_generateButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::TrnGenerateDialog *ui;

    QList<QPointer<D1Pal>> pals;
};
