#pragma once

#include <QWidget>

struct ItemStruct;

namespace Ui {
class ItemPropertiesWidget;
} // namespace Ui

class ItemPropertiesWidget : public QWidget {
    Q_OBJECT

public:
    explicit ItemPropertiesWidget(QWidget *parent);
    ~ItemPropertiesWidget();

    void initialize(const ItemStruct *is);

private:
    Ui::ItemPropertiesWidget *ui;
};
