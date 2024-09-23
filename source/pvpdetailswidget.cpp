#include "pvpdetailswidget.h"

#include <QApplication>
#include <QMessageBox>

#include "d1hro.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_pvpdetailswidget.h"

#include "dungeon/all.h"

PvPDetailsWidget::PvPDetailsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PvPDetailsWidget())
{
    ui->setupUi(this);

    this->ui->pvpHeroDetails->setReadOnly();
}

PvPDetailsWidget::~PvPDetailsWidget()
{
    delete ui;
}

void PvPDetailsWidget::initialize(D1Hero *h)
{
    this->hero = h;

    // LogErrorF("PvPDetailsWidget init 5");
    this->updateFields();
    // LogErrorF("PvPDetailsWidget init 6");
    this->setVisible(true);
}

void PvPDetailsWidget::displayFrame()
{
    // this->updateFields();

    if (this->ui->pvpHeroDetails->isVisible())
        this->ui->pvpHeroDetails->displayFrame();
}

void PvPDetailsWidget::updateFields()
{
    this->ui->discardHeroButton->setEnabled(!this->heros.isEmpty());
    this->ui->pvpHeroDetails->setVisible(!this->heros.isEmpty());

    D1Hero *newhero = D1Hero::instance();
    this->ui->addHeroButton->setEnabled(newhero != nullptr);
    delete newhero;
}

void PvPDetailsWidget::on_pvpHerosComboBox_activated(int index)
{
    D1Hero *currhero = this->heros[index];

    this->ui->pvpHeroDetails->initialize(currhero);

    this->displayFrame();
}

void PvPDetailsWidget::on_discardHeroButton_clicked()
{
    int index = this->ui->pvpHerosComboBox->currentIndex();

    this->ui->pvpHerosComboBox->removeItem(index);
    D1Hero *currhero = this->heros.takeAt(index);
    delete currhero;

    if (this->heros.count() <= index) {
        index--;
        if (index < 0) {
            this->updateFields();
            return;
        }
    }
    this->on_pvpHerosComboBox_activated(index);
}

void PvPDetailsWidget::on_addHeroButton_clicked()
{
    QString filePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Hero"), tr("hro Files (*.hro *.HRO)"));

    if (!filePath.isEmpty()) {
        OpenAsParam params = OpenAsParam();
        params.filePath = filePath;

        D1Hero *newhero = D1Hero::instance();
        // newhero->setPalette(this->trnBase->getResultingPalette());
        if (newhero->load(filePath, params)) {
            this->ui->pvpHerosComboBox->addItem(newhero->getName());
            this->heros.append(newhero);

            this->on_pvpHerosComboBox_activated(this->heros.count() - 1);
        } else {
            delete newhero;
            dProgressFail() << tr("Failed loading HRO file: %1.").arg(QDir::toNativeSeparators(filePath));
        }
    }
}

void PvPDetailsWidget::on_closeButton_clicked()
{
    this->setVisible(false);
}
