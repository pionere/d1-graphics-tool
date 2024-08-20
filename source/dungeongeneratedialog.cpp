#include "dungeongeneratedialog.h"

#include <QMessageBox>

#include "d1dun.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_dungeongeneratedialog.h"

#include "dungeon/all.h"

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

void DungeonGenerateDialog::on_levelComboBox_activated(int index)
{
    bool fixLevel = (index + 1) < NUM_FIXLVLS;
    LineEditWidget *lew = this->ui->levelLineEdit;
    QComboBox *ltc = this->ui->lvlTypeComboBox;
    lew->setReadOnly(fixLevel);
    ltc->setDisabled(fixLevel);
    if (fixLevel) {
        lew->setText(QString::number(index + 1));
        static_assert(DLV_TOWN == 0, "DungeonGenerateDialog has hardcoded enum values I.");
        static_assert(DTYPE_CATHEDRAL == 1, "DungeonGenerateDialog has hardcoded enum values II.");
        static_assert(DTYPE_CATACOMBS == 2, "DungeonGenerateDialog has hardcoded enum values III.");
        static_assert(DTYPE_CAVES == 3, "DungeonGenerateDialog has hardcoded enum values IV.");
        static_assert(DTYPE_HELL == 4, "DungeonGenerateDialog has hardcoded enum values V.");
        static_assert(DTYPE_CRYPT == 5, "DungeonGenerateDialog has hardcoded enum values VI.");
        static_assert(DTYPE_NEST == 6, "DungeonGenerateDialog has hardcoded enum values VII.");
        ltc->setCurrentIndex((int)AllLevels[index + 1].dType);
    }
    // update the lineedit widget (thanks qt...)
    lew->style()->unpolish(lew);
    lew->style()->polish(lew);
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
    params.levelIdx = this->ui->levelComboBox->currentIndex() + 1;
    params.levelNum = this->ui->levelLineEdit->text().toUShort();
    params.levelType = this->ui->lvlTypeComboBox->currentIndex();
    params.difficulty = this->ui->difficultyComboBox->currentIndex();
    int numPlayers = this->ui->plrCountLineEdit->text().toUShort();
    params.numPlayers = numPlayers == 0 ? 1 : numPlayers;
    params.isMulti = this->ui->multiCheckBox->isChecked();
    params.isHellfire = this->ui->hellfireCheckBox->isChecked();
    params.useTileset = this->ui->tilesetCheckBox->isChecked();
    params.patchDunFiles = this->ui->patchDunCheckBox->isChecked();
    params.extraQuestRnd = this->ui->extraRndCheckBox->isChecked();	
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
