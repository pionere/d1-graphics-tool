#include "dungeongeneratedialog.h"

#include <QMessageBox>

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

    QHBoxLayout *layout = this->ui->seedWithRefreshButtonLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_BrowserReload, tr("Generate"), this, &DungeonGenerateDialog::on_actionGenerateSeed_triggered);
    layout->addStretch();
    layout = this->ui->questSeedWithRefreshButtonLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_BrowserReload, tr("Generate"), this, &DungeonGenerateDialog::on_actionGenerateQuestSeed_triggered);
    layout->addStretch();
}

DungeonGenerateDialog::~DungeonGenerateDialog()
{
    delete ui;
}

void DungeonGenerateDialog::initialize(D1Dun *d, D1Tileset *ts)
{
    this->dun = d;
    this->tileset = ts;
}

void DungeonGenerateDialog::on_actionGenerateSeed_triggered()
{
    QRandomGenerator *gen = QRandomGenerator::global();
    this->ui->seedLineEdit->setText(QString::number((int)gen->generate()));
}

void DungeonGenerateDialog::on_actionGenerateQuestSeed_triggered()
{
    QRandomGenerator *gen = QRandomGenerator::global();
    this->ui->questSeedLineEdit->setText(QString::number((int)gen->generate()));
}

void DungeonGenerateDialog::on_generateButton_clicked()
{
    GenerateDunParam params;
    params.level = this->ui->levelComboBox->currentIndex() + 1;
    params.difficulty = this->ui->difficultyComboBox->currentIndex();
    params.isMulti = this->ui->multiCheckBox->isChecked();
    params.isHellfire = this->ui->hellfireCheckBox->isChecked();
    params.useTileset = this->ui->tilesetCheckBox->isChecked();
    params.patchDunFiles = this->ui->patchDunCheckBox->isChecked();
    bool ok;
    QString seedTxt = this->ui->seedLineEdit->text();
    params.seed = seedTxt.toInt(&ok);
    if (!ok && !seedTxt.isEmpty()) {
        QMessageBox::critical(this, "Error", "Failed to parse the seed to a 32-bit integer.");
        return;
    }
    seedTxt = this->ui->questSeedLineEdit->text();
    params.seedQuest = seedTxt.toInt(&ok);
    if (!ok && !seedTxt.isEmpty()) {
        QMessageBox::critical(this, "Error", "Failed to parse the quest-seed to a 32-bit integer.");
        return;
    }
    params.entryMode = this->ui->entryComboBox->currentData().value<int>();
    params.extraRounds = this->ui->extraRoundsLineEdit->text().toInt();

    LevelCelView *view = qobject_cast<LevelCelView *>(this->parentWidget());

    this->close();

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    EnterGameLevel(this->dun, this->tileset, view, params);

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
