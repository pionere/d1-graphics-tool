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

void DungeonResourceDialog::initialize(DUN_ENTITY_TYPE t, int index, bool unique, D1Dun *d)
{
    this->dun = d;

    if (this->type != (int)t) {
        this->type = (int)t;
        // reset the fields
        this->ui->indexLineEdit->setText("");
        this->ui->nameLineEdit->setText("");
        this->ui->celFileLineEdit->setText("");
        this->ui->widthLineEdit->setText("");
        this->ui->frameLineEdit->setText("");
        this->ui->baseTrnFileLineEdit->setText("");
        this->ui->uniqueTrnFileLineEdit->setText("");
        this->ui->uniqueMonCheckBox->setChecked(false);
        this->ui->preFlagCheckBox->setChecked(false);
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
        case DUN_ENTITY_TYPE::MISSILE:
            title = tr("Missile parameters");
            break;
        }
        this->setWindowTitle(title);

        this->ui->baseTrnFileLabel->setVisible(t == DUN_ENTITY_TYPE::MONSTER || t == DUN_ENTITY_TYPE::MISSILE);
        this->ui->baseTrnFileLineEdit->setVisible(t == DUN_ENTITY_TYPE::MONSTER || t == DUN_ENTITY_TYPE::MISSILE);
        this->ui->baseTrnFileBrowsePushButton->setVisible(t == DUN_ENTITY_TYPE::MONSTER || t == DUN_ENTITY_TYPE::MISSILE);
        this->ui->baseTrnFileClearPushButton->setVisible(t == DUN_ENTITY_TYPE::MONSTER || t == DUN_ENTITY_TYPE::MISSILE);

        this->ui->uniqueTrnFileLabel->setVisible(t == DUN_ENTITY_TYPE::MONSTER);
        this->ui->uniqueTrnFileLineEdit->setVisible(t == DUN_ENTITY_TYPE::MONSTER);
        this->ui->uniqueTrnFileBrowsePushButton->setVisible(t == DUN_ENTITY_TYPE::MONSTER);
        this->ui->uniqueTrnFileClearPushButton->setVisible(t == DUN_ENTITY_TYPE::MONSTER);

        this->ui->uniqueMonCheckBox->setVisible(t == DUN_ENTITY_TYPE::MONSTER);

        this->ui->frameLabel->setVisible(t == DUN_ENTITY_TYPE::OBJECT);
        this->ui->frameLineEdit->setVisible(t == DUN_ENTITY_TYPE::OBJECT);

        this->ui->preFlagCheckBox->setVisible(t != DUN_ENTITY_TYPE::ITEM);
        this->ui->preFlagCheckBox->setToolTip(t == DUN_ENTITY_TYPE::MONSTER ? tr("Dead monster") : tr("On the floor"));

        this->adjustSize();
    }

    if (this->prevIndex != index || this->prevUnique != unique) {
        this->prevIndex = index;
        this->prevUnique = unique;
        if (index != 0) {
            this->ui->indexLineEdit->setText(QString::number(index));
            this->ui->uniqueMonCheckBox->setChecked(unique);
        }
    }
}

void DungeonResourceDialog::on_celFileBrowsePushButton_clicked()
{
    QString title = this->type == (int)DUN_ENTITY_TYPE::MONSTER ? tr("Select CL2 file") : tr("Select CEL file");
    QString filter = this->type == (int)DUN_ENTITY_TYPE::MONSTER ? tr("CL2 Files (*.cl2 *.CL2)") : tr("CEL Files (*.cel *.CEL)");
    QString filePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, title, filter);
    if (filePath.isEmpty()) {
        return;
    }
    this->ui->celFileLineEdit->setText(filePath);
}

void DungeonResourceDialog::on_baseTrnFileBrowsePushButton_clicked()
{
    QString filePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Base Translation File"), tr("TRN Files (*.trn *.TRN)"));
    if (filePath.isEmpty()) {
        return;
    }
    this->ui->baseTrnFileLineEdit->setText(filePath);
}

void DungeonResourceDialog::on_baseTrnFileClearPushButton_clicked()
{
    this->ui->baseTrnFileLineEdit->setText("");
}

void DungeonResourceDialog::on_uniqueTrnFileBrowsePushButton_clicked()
{
    QString filePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Unique Translation File"), tr("TRN Files (*.trn *.TRN)"));
    if (filePath.isEmpty()) {
        return;
    }
    this->ui->uniqueTrnFileLineEdit->setText(filePath);
    this->ui->uniqueMonCheckBox->setChecked(true);
}

void DungeonResourceDialog::on_uniqueTrnFileClearPushButton_clicked()
{
    this->ui->uniqueTrnFileLineEdit->setText("");
}

void DungeonResourceDialog::on_addButton_clicked()
{
    AddResourceParam params;
    params.type = (DUN_ENTITY_TYPE)this->type;
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
    params.baseTrnPath = this->ui->baseTrnFileLineEdit->text();
    params.uniqueTrnPath = this->ui->uniqueTrnFileLineEdit->text();
    params.uniqueMon = this->ui->uniqueMonCheckBox->isChecked();
    params.preFlag = this->ui->preFlagCheckBox->isChecked();
    if (params.frame < 0 && params.type == DUN_ENTITY_TYPE::OBJECT) {
        return;
    }

    this->close();

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    if (this->dun->addResource(params)) {
        LevelCelView *view = qobject_cast<LevelCelView *>(this->parentWidget());
        view->updateEntityOptions();
    }

    // Clear loading message from status bar
    ProgressDialog::done();
}

void DungeonResourceDialog::on_cancelButton_clicked()
{
    this->close();
}
