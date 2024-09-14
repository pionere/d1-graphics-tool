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

private:
    void updateFields();

private slots:
    void on_remapButton_clicked();
    void on_remapCancelButton_clicked();

    void on_colorFrom0LineEdit_returnPressed();
    void on_colorFrom0LineEdit_escPressed();
    void on_colorFrom1LineEdit_returnPressed();
    void on_colorFrom1LineEdit_escPressed();
    void on_colorTo0LineEdit_returnPressed();
    void on_colorTo0LineEdit_escPressed();
    void on_colorTo1LineEdit_returnPressed();
    void on_colorTo1LineEdit_escPressed();
    void on_range0LineEdit_returnPressed();
    void on_range0LineEdit_escPressed();
    void on_range1LineEdit_returnPressed();
    void on_range1LineEdit_escPressed();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::RemapDialog *ui;

    std::pair<int, int> colorFrom;
    std::pair<int, int> colorTo;

    std::pair<int, int> range;
};
