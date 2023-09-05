#pragma once

#include <QWidget>

class TilesetLightDialog;
struct SubtileLight;

namespace Ui {
class TilesetLightEntryWidget;
} // namespace Ui

class TilesetLightEntryWidget : public QWidget {
    Q_OBJECT

public:
    explicit TilesetLightEntryWidget(TilesetLightDialog *parent);
    ~TilesetLightEntryWidget();

    void initialize(const SubtileLight &lightRange);

    SubtileLight getSubtileRange() const;
    int getLightRadius() const;
    void setLightRadius(int lightrange);

private slots:
    void on_deletePushButtonClicked();

private:
    Ui::TilesetLightEntryWidget *ui;
    TilesetLightDialog *view;
};
