#include "dungeondecoratedialog.h"

#include <QMessageBox>

#include "d1dun.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_dungeondecoratedialog.h"

#include "dungeon/all.h"

DungeonDecorateDialog::DungeonDecorateDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DungeonDecorateDialog())
{
    this->ui->setupUi(this);

    QHBoxLayout *layout = this->ui->seedWithRefreshButtonLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_BrowserReload, tr("Generate"), this, &DungeonDecorateDialog::on_actionGenerateSeed_triggered);
    layout->addStretch();

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->lvlLineEdit, SIGNAL(cancel_signal()), this, SLOT(on_lvlLineEdit_escPressed()));
}

DungeonDecorateDialog::~DungeonDecorateDialog()
{
    delete ui;
}

void DungeonDecorateDialog::initialize(D1Dun *d, D1Tileset *ts)
{
    this->dun = d;
    this->tileset = ts;
}

void DungeonDecorateDialog::on_lvlComboBox_activated(int index)
{
    bool fixLevel = (index + 1) < NUM_FIXLVLS;
    LineEditWidget *lew = this->ui->lvlLineEdit;
    QComboBox *ltc = this->ui->lvlTypeComboBox;
    lew->setReadOnly(fixLevel);
    ltc->setDisabled(fixLevel);
    if (fixLevel) {
        lew->setText(QString::number(index + 1));
        static_assert(DTYPE_TOWN == 0, "DungeonDecorateDialog has hardcoded enum values I.");
        static_assert(DTYPE_CATHEDRAL == 1, "DungeonDecorateDialog has hardcoded enum values II.");
        static_assert(DTYPE_CATACOMBS == 2, "DungeonDecorateDialog has hardcoded enum values III.");
        static_assert(DTYPE_CAVES == 3, "DungeonDecorateDialog has hardcoded enum values IV.");
        static_assert(DTYPE_HELL == 4, "DungeonDecorateDialog has hardcoded enum values V.");
        static_assert(DTYPE_CRYPT == 5, "DungeonDecorateDialog has hardcoded enum values VI.");
        static_assert(DTYPE_NEST == 6, "DungeonDecorateDialog has hardcoded enum values VII.");
        ltc->setCurrentIndex((int)AllLevels[index + 1].dType);
    }
}

void DungeonDecorateDialog::on_lvlTypeComboBox_activated(int index)
{
    LineEditWidget *lew = this->ui->lvlLineEdit;
    int levelNum = lew->text().toUShort();
    if (index != 0) {
        int from = INT_MAX;
        for (int i = 0; i < NUM_FIXLVLS; i++) {
            if (AllLevels[i].dType == index) {
                int lvl = AllLevels[i].dLevel;
                if (from > lvl)
                    from = lvl;
            }
        }
        if (levelNum < from) {
            lew->setText(QString::number(from));
        }
    }
}

void DungeonDecorateDialog::on_lvlLineEdit_returnPressed()
{
    this->on_lvlLineEdit_escPressed();
}

void DungeonDecorateDialog::on_lvlLineEdit_escPressed()
{
    this->ui->lvlLineEdit->clearFocus();
    this->on_lvlTypeComboBox_activated(this->ui->lvlTypeComboBox->currentIndex());
}

void DungeonDecorateDialog::on_actionGenerateSeed_triggered()
{
    QRandomGenerator *gen = QRandomGenerator::global();
    this->ui->seedLineEdit->setText(QString::number((int)gen->generate()));
}

void DungeonDecorateDialog::on_decorateButton_clicked()
{
    DecorateDunParam params;
    params.levelIdx = this->ui->lvlComboBox->currentIndex() + 1;
    params.levelType = this->ui->lvlTypeComboBox->currentIndex();
    params.levelNum = this->ui->lvlLineEdit->text().toUShort();
    params.difficulty = this->ui->difficultyComboBox->currentIndex();
    int numPlayers = this->ui->plrCountLineEdit->text().toUShort();
    params.numPlayers = numPlayers == 0 ? 1 : numPlayers;
    params.isMulti = this->ui->multiCheckBox->isChecked();
    params.isHellfire = this->ui->hellfireCheckBox->isChecked();
    params.useTileset = this->ui->tilesetCheckBox->isChecked();
    params.resetMonsters = this->ui->resetMonstersCheckBox->isChecked();
    params.resetObjects = this->ui->resetObjectsCheckBox->isChecked();
    params.resetItems = this->ui->resetItemsCheckBox->isChecked();
    params.resetRooms = this->ui->resetRoomsCheckBox->isChecked();
    params.resetMissiles = this->ui->resetMissilesCheckBox->isChecked();
    params.addTiles = this->ui->addTilesCheckBox->isChecked();
    params.addShadows = this->ui->addShadowsCheckBox->isChecked();
    params.addMonsters = this->ui->addMonstersCheckBox->isChecked();
    params.addObjects = this->ui->addObjectsCheckBox->isChecked();
    params.addItems = this->ui->addItemsCheckBox->isChecked();
    params.addRooms = this->ui->addRoomsCheckBox->isChecked();
    bool ok;
    QString seedTxt = this->ui->seedLineEdit->text();
    params.seed = seedTxt.toInt(&ok);
    if (!ok && !seedTxt.isEmpty()) {
        QMessageBox::critical(this, "Error", "Failed to parse the seed to a 32-bit integer.");
        return;
    }
    params.extraRounds = this->ui->extraRoundsLineEdit->text().toInt();

    LevelCelView *view = qobject_cast<LevelCelView *>(this->parentWidget());

    this->close();

    ProgressDialog::start(PROGRESS_DIALOG_STATE::BACKGROUND, tr("Processing..."), 1, PAF_UPDATE_WINDOW);

    DecorateGameLevel(this->dun, this->tileset, view, params);

    // Clear loading message from status bar
    ProgressDialog::done();
    /*std::function<void()> func = [this, view, params]() {
        EnterGameLevel(this->dun, view, params);
    };
    ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::ACTIVE, tr("Processing..."), 1, PAF_UPDATE_WINDOW, std::move(func));*/
}

void DungeonDecorateDialog::on_cancelButton_clicked()
{
    this->close();
}
