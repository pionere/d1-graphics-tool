#pragma once

#include <QComboBox>
#include <QDialog>

class D1Dun;

enum class DUN_ENTITY_TYPE {
    OBJECT,
    MONSTER,
    ITEM,
    MISSILE,
};

class AddResourceParam {
public:
    DUN_ENTITY_TYPE type;
    int index;
    QString name;
    QString path;
    int width;
    int frame;
    QString baseTrnPath;
    QString uniqueTrnPath;
    bool uniqueMon;
    bool preFlag;
};

namespace Ui {
class DungeonResourceDialog;
}

class DungeonResourceDialog : public QDialog {
    Q_OBJECT

public:
    explicit DungeonResourceDialog(QWidget *view);
    ~DungeonResourceDialog();

    void initialize(DUN_ENTITY_TYPE type, int index, bool unique, D1Dun *dun);

private slots:
    void on_celFileBrowsePushButton_clicked();
    void on_baseTrnFileBrowsePushButton_clicked();
    void on_baseTrnFileClearPushButton_clicked();
    void on_uniqueTrnFileBrowsePushButton_clicked();
    void on_uniqueTrnFileClearPushButton_clicked();

    void on_addButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::DungeonResourceDialog *ui;

    int type = -1;
    int prevIndex = -1;
    bool prevUnique = false;
    D1Dun *dun;
};
