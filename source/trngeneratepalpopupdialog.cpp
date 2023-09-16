#include "trngeneratepalpopupdialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QStyle>

#include "d1trn.h"
#include "trngeneratedialog.h"
#include "ui_trngeneratepalpopupdialog.h"

TrnGeneratePalPopupDialog::TrnGeneratePalPopupDialog(TrnGenerateDialog *parent)
    : QDialog(parent)
    , ui(new Ui::TrnGeneratePalPopupDialog())
    , view(parent)
{
    this->ui->setupUi(this);
    this->ui->colorPickerView->setMouseTracking(true);
    this->ui->colorPickerView->setScene(&this->popupScene);

}

TrnGeneratePalPopupDialog::~TrnGeneratePalPopupDialog()
{
    delete ui;
}

void TrnGeneratePalPopupDialog::initialize(D1Pal *p, D1Trn *t, int i)
{
    this->trn = t;
    this->trnIndex = i;

    this->popupScene.initialize(p, nullptr);
    this->popupScene.setSelectedIndex(t->getTranslation(i));

    this->popupScene.displayColors();
}

void TrnGeneratePalPopupDialog::on_colorDblClicked(int index)
{
    this->trn->setTranslation(this->trnIndex, index);

    this->view->updatePals();

    this->close();
}

/*void TrnGeneratePalPopupDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        this->close();
    }
}*/
