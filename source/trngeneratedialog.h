#pragma once

#include <vector>

#include <QDialog>
#include <QList>
#include <QPointer>

class D1Pal;

typedef struct GenerateTrnColor {
    int firstfixcolor;
    int lastfixcolor;
    bool shadefixcolor;
};

class GenerateTrnParam {
public:
    std::vector<GenerateTrnColor> colors;
    std::vector<D1Pal *> pals;
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

private slots:
    void on_actionAddRange_triggered();
    void on_actionAddPalette_triggered();

    void on_generateButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::TrnGenerateDialog *ui;

    QList<QPointer<D1Pal>> pals;
};
