#pragma once

#include <QDialog>
#include <QMap>

#include "celview.h"
#include "d1pal.h"

namespace Ui {
class PaletteShowDialog;
}

class PaletteShowDialog : public QDialog {
    Q_OBJECT

public:
    static constexpr const char *GRB_SQ_PATH = ":/GRBSquare.png";
    static constexpr const char *RGB_SQ_PATH = ":/RGBSquare.png";
    static constexpr const char *RGB_WH_PATH = ":/RGBWheel.png";
    static constexpr const char *CIE_PATH = ":/CIE1976.png";
    static constexpr const char *CIEXY_PATH = ":/CIExy1931.png";

    explicit PaletteShowDialog(QWidget *parent);
    ~PaletteShowDialog();

    void initialize(D1Pal *pal);

private:
    void displayFrame();
    void updatePathComboBoxOptions(const QList<QString> &options, const QString &selectedOption);

private slots:
    void on_openPushButtonClicked();
    void on_closePushButtonClicked();
    void on_pathComboBox_activated(int index);

    void on_zoomOutButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomEdit_returnPressed();
    void on_zoomEdit_escPressed();

    void on_closeButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::PaletteShowDialog *ui;
    CelScene palScene = CelScene(this);

    D1Pal *pal;
    QMap<QString, QImage *> images;
};
