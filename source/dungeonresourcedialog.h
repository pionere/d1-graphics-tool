#pragma once

#include <QComboBox>
#include <QDialog>

class D1Dun;

enum class DUN_ENTITY_TYPE {
    OBJECT,
    MONSTER,
    ITEM,
};

class AddResourceParam {
public:
    DUN_ENTITY_TYPE type;
    int index;
    QString name;
    QString path;
    int width;
    int frame;
    QString trnPath;
};

namespace Ui {
class DungeonResourceDialog;
}

class DungeonResourceDialog : public QDialog {
    Q_OBJECT

public:
    explicit DungeonResourceDialog(QWidget *parent);
    ~DungeonResourceDialog();

    void initialize(DUN_ENTITY_TYPE type, D1Dun *dun, QComboBox *comboBox, int currentValue);

private slots:
    void on_celFileBrowsePushButton_clicked();
    void on_trnFileBrowsePushButton_clicked();
    void on_trnFileClearPushButton_clicked();

    void on_addButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::DungeonResourceDialog *ui;

    DUN_ENTITY_TYPE type;
    D1Dun *dun;
    QComboBox *comboBox;
    int currentValue;
};
