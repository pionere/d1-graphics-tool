#include "trngeneratepalentrywidget.h"

#include <QApplication>
#include <QMessageBox>
#include <QStyle>

#include "mainwindow.h"
#include "trngeneratedialog.h"
#include "pushbuttonwidget.h"
#include "ui_trngeneratepalentrywidget.h"

TrnGeneratePalEntryWidget::TrnGeneratePalEntryWidget(TrnGenerateDialog *parent, QButtonGroup *btnGroup, D1Pal *p, bool dp)
    : QWidget(parent)
    , ui(new Ui::TrnGeneratePalEntryWidget())
    , view(parent)
    , pal(p)
    , delPal(dp)
{
    ui->setupUi(this);
    btnGroup->addButton(this->ui->selectRadioButton);

    QLayout *layout = this->ui->entryHorizontalLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_TitleBarCloseButton, tr("Remove"), this, &TrnGeneratePalEntryWidget::on_deletePushButtonClicked);

    this->ui->paletteFileEdit->setText(p->getFilePath());
}

TrnGeneratePalEntryWidget::~TrnGeneratePalEntryWidget()
{
    QButtonGroup *group = this->ui->selectRadioButton->group();
    if (group != nullptr) {
        group->removeButton(this->ui->selectRadioButton);
    }
    delete ui;
    if (this->delPal) {
        delete this->pal;
    }
}

D1Pal *TrnGeneratePalEntryWidget::getPalette() const
{
    return const_cast<D1Pal *>(this->pal);
}

void TrnGeneratePalEntryWidget::setPalette(D1Pal *p)
{
    // assert(!this->delPal);
    this->pal = p;
}

bool TrnGeneratePalEntryWidget::ownsPalette() const
{
    return this->delPal;
}

bool TrnGeneratePalEntryWidget::isSelected() const
{
    return this->ui->selectRadioButton->isChecked();
}

void TrnGeneratePalEntryWidget::setSelected(bool selected)
{
    this->ui->selectRadioButton->setChecked(selected);
}

void TrnGeneratePalEntryWidget::on_paletteFileBrowseButton_clicked()
{
    // start file-dialog
    QString palFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Palette File"), tr("PAL File (*.pal *.PAL)"));
    if (palFilePath.isEmpty()) {
        return;
    }
    // load the palette
    D1Pal *newPal = new D1Pal();
    if (!newPal->load(palFilePath)) {
        delete newPal;
        QMessageBox::critical(this, tr("Error"), tr("Failed loading PAL file: %1.").arg(QDir::toNativeSeparators(palFilePath)));
        return;
    }
    // update the widget
    if (this->delPal) {
        delete this->pal;
    }
    this->pal = newPal;
    this->delPal = true;
    this->ui->paletteFileEdit->setText(newPal->getFilePath());
}

void TrnGeneratePalEntryWidget::on_deletePushButtonClicked()
{
    this->view->on_actionDelPalette_triggered(this);
}
