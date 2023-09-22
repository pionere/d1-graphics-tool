#pragma once

#include <QWidget>

class CppView;
class D1CppTable;

namespace Ui {
class CppViewEntryWidget;
} // namespace Ui

class CppViewEntryWidget : public QWidget {
    Q_OBJECT

public:
    explicit CppViewEntryWidget(CppView *parent);
    ~CppViewEntryWidget();

    void initialize(D1CppTable *table, int rowNum, int columnNum, int width);

private slots:
    void on_headerButton_clicked();

    void on_entryLineEdit_returnPressed();
    void on_entryLineEdit_escPressed();

private:
    Ui::CppViewEntryWidget *ui;
    CppView *view;
    QWidget *widget;

    D1CppTable *table;
    int rowNum;
    int columnNum;
};
