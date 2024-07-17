#include "dungeondecoratedialog.h"

#include <QMessageBox>

#include "d1dun.h"
#include "dungeon/interfac.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_dungeondecoratedialog.h"

DungeonDecorateDialog::DungeonDecorateDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DungeonDecorateDialog())
{
    this->ui->setupUi(this);

    QHBoxLayout *layout = this->ui->seedWithRefreshButtonLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_BrowserReload, tr("Generate"), this, &DungeonDecorateDialog::on_actionGenerateSeed_triggered);
    layout->addStretch();
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

void DungeonDecorateDialog::on_levelComboBox_activated(int index)
{
    bool fixLevel = (index + 1) < NUM_FIXLVLS;
    this->ui->levelLineEdit->setReadOnly(fixLevel);
    if (fixLevel) {
        this->ui->levelLineEdit->setText(QString::number(index + 1));
    }
}

void DungeonDecorateDialog::on_actionGenerateSeed_triggered()
{
    QRandomGenerator *gen = QRandomGenerator::global();
    this->ui->seedLineEdit->setText(QString::number((int)gen->generate()));
}

void DungeonDecorateDialog::on_decorateButton_clicked()
{
    DecorateDunParam params;
    params.levelIdx = this->ui->levelComboBox->currentIndex() + 1;
    params.levelNum = this->ui->levelLineEdit->text().toUShort();
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
