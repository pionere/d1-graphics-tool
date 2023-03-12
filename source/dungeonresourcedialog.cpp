#include "dungeonresourcedialog.h"

#include "d1dun.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_dungeonresourcedialog.h"

DungeonResourceDialog::DungeonResourceDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DungeonResourceDialog())
{
    this->ui->setupUi(this);
}

DungeonResourceDialog::~DungeonResourceDialog()
{
    delete ui;
}

void DungeonResourceDialog::initialize(DUN_ENTITY_TYPE t, D1Dun *d, QComboBox *cb)
{
    this->dun = d;
    this->comboBox = cb;

    if (this->type != t) {
        this->type = t;
        // reset the fields
        this->ui->indexLineEdit->setText("");
        this->ui->nameLineEdit->setText("");
        this->ui->celFileLineEdit->setText("");
        this->ui->widthLineEdit->setText("");
        this->ui->frameLineEdit->setText("");
        this->ui->trnFileLineEdit->setText("");
        // select window title
        QString title;
        switch (t) {
        case DUN_ENTITY_TYPE::OBJECT:
            title = tr("Object parameters");
            break;
        case DUN_ENTITY_TYPE::MONSTER:
            title = tr("Monster parameters");
            break;
        case DUN_ENTITY_TYPE::ITEM:
            title = tr("Item parameters");
            break;
        }
        this->setWindowTitle(title);

        this->ui->trnFileLabel->setVisible(t == DUN_ENTITY_TYPE::MONSTER);
        this->ui->trnFileLineEdit->setVisible(t == DUN_ENTITY_TYPE::MONSTER);
        this->ui->trnFileBrowsePushButton->setVisible(t == DUN_ENTITY_TYPE::MONSTER);

        this->ui->frameLabel->setVisible(t == DUN_ENTITY_TYPE::OBJECT);
        this->ui->frameLineEdit->setVisible(t == DUN_ENTITY_TYPE::OBJECT);

        this->adjustSize();
    }
}

void DungeonResourceDialog::on_celFileBrowsePushButton_clicked()
{
    QString title = this->type == DUN_ENTITY_TYPE::MONSTER ? tr("Select CL2 file") : tr("Select CEL file");
    QString filter = this->type == DUN_ENTITY_TYPE::MONSTER ? tr("CL2 Files (*.cl2 *.CL2)") : tr("CEL Files (*.cel *.CEL)");
    QString filePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, title, filter);

    if (filePath.isEmpty())
        return;

    this->ui->celFileLineEdit->setText(filePath);
}

void DungeonResourceDialog::on_trnFileBrowsePushButton_clicked()
{
    QString filePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select TRN file"), tr("TRN Files (*.trn *.TRN)"));

    if (filePath.isEmpty())
        return;

    this->ui->trnFileLineEdit->setText(filePath);
}

void DungeonResourceDialog::on_trnFileClearPushButton_clicked()
{
    this->ui->trnFileLineEdit->setText("");
}

void DungeonResourceDialog::on_addButton_clicked()
{
    AddResourceParam params;
    params.type = this->type;
    params.index = this->ui->indexLineEdit->text().toInt();
    if (params.index <= 0) {
        return;
    }
    params.name = this->ui->nameLineEdit->text();
    if (params.name.isEmpty()) {
        return;
    }
    params.path = this->ui->celFileLineEdit->text();
    if (params.path.isEmpty()) {
        return;
    }
    params.width = this->ui->widthLineEdit->text().toInt();
    params.frame = this->ui->frameLineEdit->text().toInt();
    params.trnPath = this->ui->trnFileLineEdit->text();
    if (params.frame < 0 && params.type == DUN_ENTITY_TYPE::OBJECT) {
        return;
    }

    this->close();

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    if (this->dun->addResource(params)) {
        for (int i = this->comboBox->count() - 1; i >= 0; i--) {
            if (this->comboBox->itemData(i).value<int>() == params.index) {
                this->comboBox->removeItem(i);
            }
        }
        this->comboBox->addItem(params.name, params.index);
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void DungeonResourceDialog::on_cancelButton_clicked()
{
    this->close();
}
