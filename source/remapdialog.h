#pragma once

#include <QDialog>

class PaletteWidget;

class RemapParam {
public:
    std::pair<int, int> colorFrom;
    std::pair<int, int> colorTo;
    std::pair<int, int> frames;
};

namespace Ui {
class RemapDialog;
}

class RemapDialog : public QDialog {
    Q_OBJECT

public:
    explicit RemapDialog(QWidget *parent);
    ~RemapDialog();

    void initialize(const PaletteWidget *palWidget);

private slots:
    void on_remapButton_clicked();
    void on_remapCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::RemapDialog *ui;
};
