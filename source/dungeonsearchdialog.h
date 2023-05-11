#pragma once

#include <QDialog>
#include <QRect>

enum class DUN_SEARCH_TYPE {
    Tile,
    Subtile,
    Tile_Protection,
    Monster_Protection,
    Object_Protection,
    Room,
    Monster,
    Object,
    Item,
};

class DungeonSearchParam {
public:
    DUN_SEARCH_TYPE type;
    int index;
    bool special;
    bool scrollTo;
    QRect area;
};

class D1Dun;

namespace Ui {
class DungeonSearchDialog;
}

class DungeonSearchDialog : public QDialog {
    Q_OBJECT

public:
    explicit DungeonSearchDialog(QWidget *parent);
    ~DungeonSearchDialog();

    void initialize(D1Dun *dun);

private slots:
    void on_searchButton_clicked();
    void on_searchCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::DungeonSearchDialog *ui;

    D1Dun *dun;
};
