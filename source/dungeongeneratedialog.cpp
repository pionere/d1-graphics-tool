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
}

DungeonGenerateDialog::~DungeonGenerateDialog()
{
    delete ui;
}

void DungeonGenerateDialog::initialize(D1Dun *d)
{
    this->dun = d;
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
    params.entryMode = this->ui->entryComboBox->currentIndex();

    this->close();

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

	LevelCelView * view = qobject_cast<LevelCelView*>(this->parentWidget());
	EnterGameLevel(this->dun, view, params);

    // Clear loading message from status bar
    ProgressDialog::done();
}

void DungeonGenerateDialog::on_cancelButton_clicked()
{
    this->close();
}
