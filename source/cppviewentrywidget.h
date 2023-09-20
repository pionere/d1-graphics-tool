#pragma once

#include <QWidget>

class CppView;

namespace Ui {
class CppViewEntryWidget;
} // namespace Ui

class CppViewEntryWidget : public QWidget {
    Q_OBJECT

public:
    explicit CppViewEntryWidget(CppView *parent);
    ~CppViewEntryWidget();

    void initialize(D1CppTable *table, int rowNum, int columnNum);

private:
    Ui::CppViewEntryWidget *ui;
    CppView *view;
    D1CppTable *table;
    int rowNum;
    int columnNum;
};
