#include "dungeongeneratedialog.h"

#include "d1dun.h"
#include "dungeon/interfac.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_dungeongeneratedialog.h"

DungeonGenerateDialog::DungeonGenerateDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DungeonGenerateDialog())
{
    this->ui->setupUi(this);

    this->ui->entryComboBox->addItem(tr("Entrance"), QVariant(ENTRY_MAIN));
    this->ui->entryComboBox->addItem(tr("Exit"), QVariant(ENTRY_PREV));
    this->ui->entryComboBox->addItem(tr("Town"), QVariant(ENTRY_TWARPDN));
    this->ui->entryComboBox->addItem(tr("Return"), QVariant(ENTRY_RTNLVL));
}

DungeonGenerateDialog::~DungeonGenerateDialog()
{
    delete ui;
}

void DungeonGenerateDialog::initialize(D1Dun *d)
{
    this->dun = d;
    this->ui->seedLineEdit->setText(QString::number(812323448));
}

void DungeonGenerateDialog::on_generateButton_clicked()
{
    GenerateDunParam params;
    params.level = this->ui->levelComboBox->currentIndex() + 1;
    params.difficulty = this->ui->difficultyComboBox->currentIndex();
    params.isMulti = this->ui->multiCheckBox->isChecked();
    params.isHellfire = this->ui->hellfireCheckBox->isChecked();
    params.seed = this->ui->seedLineEdit->text().toInt();
    params.seedQuest = this->ui->questSeedLineEdit->text().toInt();
    params.entryMode = this->ui->entryComboBox->currentData().value<int>();
    params.extraRounds = this->ui->extraRoundsLineEdit->text().toInt();

    LevelCelView *view = qobject_cast<LevelCelView *>(this->parentWidget());

    this->close();

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    EnterGameLevel(this->dun, view, params);

    // Clear loading message from status bar
    ProgressDialog::done();
    /*std::function<void()> func = [this, view, params]() {
        EnterGameLevel(this->dun, view, params);
    };
    ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::ACTIVE, tr("Processing..."), 1, PAF_UPDATE_WINDOW, std::move(func));*/
}

void DungeonGenerateDialog::on_cancelButton_clicked()
{
    this->close();
}
