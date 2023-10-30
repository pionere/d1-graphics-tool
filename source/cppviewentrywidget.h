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

    static int baseHorizontalMargin();

public slots:
    void on_toggleInfoButton();

private slots:
    void on_headerButton_clicked();
    void on_infoButton_clicked();
    void ShowHeaderContextMenu(const QPoint &pos);
    void ShowRowContextMenu(const QPoint &pos);

    void on_actionInsertColumn_triggered();
    void on_actionDelColumn_triggered();
    void on_actionHideColumn_triggered();
    void on_actionInsertRow_triggered();
    void on_actionDelRow_triggered();
    void on_actionHideRow_triggered();

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
