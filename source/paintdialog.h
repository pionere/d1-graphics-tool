#pragma once

#include <QDialog>
#include <QList>
#include <QMouseEvent>
#include <QPoint>

namespace Ui {
class PaintDialog;
} // namespace Ui

class D1Pal;
class D1Tileset;

class PaintDialog : public QDialog {
    Q_OBJECT

public:
    explicit PaintDialog(QWidget *parent);
    ~PaintDialog();

    void initialize(D1Tileset *tileset);
    void setPalette(D1Pal *pal);

    void show(); // override;
    void hide(); // override;

public slots:
    void palColorsSelected(const QList<quint8> &indices);
    void colorModified();

private slots:
    void on_closePushButtonClicked();
    void on_movePushButtonPressed();
    void on_movePushButtonReleased();
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    Ui::PaintDialog *ui;
    bool moving;
    QPoint lastPos;
    bool isTileset;
    D1Pal *pal;
    QList<quint8> selectedColors;
};
