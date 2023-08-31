#include "trngeneratedialog.h"

#include <QMessageBox>

#include "d1trs.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_trngeneratedialog.h"

TrnGenerateDialog::TrnGenerateDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TrnGenerateDialog())
{
    this->ui->setupUi(this);

    QHBoxLayout *layout = this->ui->addRangeButtonLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogNewFolder, tr("Add"), this, &TrnGenerateDialog::on_actionAddRange_triggered);
    layout->addStretch();
    layout = this->ui->addPaletteButtonLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_FileDialogNewFolder, tr("Add"), this, &TrnGenerateDialog::on_actionAddPalette_triggered);
    layout->addStretch();

    this->ui->mainGridLayout->setRowStretch(1, 1);
    this->ui->mainGridLayout->setRowStretch(3, 1);
    this->ui->mainGridLayout->setColumnStretch(1, 1);

    this->on_actionAddRange_triggered();
}

TrnGenerateDialog::~TrnGenerateDialog()
{
    delete ui;
}

void TrnGenerateDialog::initialize(D1Pal *p)
{
    QList<TrnGeneratePalEntryWidget *> palWidgets = this->ui->palettesVBoxLayout->findChildren<TrnGeneratePalEntryWidget *>();
    if (palWidgets.empty()) {
        TrnGeneratePalEntryWidget *widget = new TrnGeneratePalEntryWidget(this, p, false);
        this->ui->palettesVBoxLayout->addWidget(widget, 0, Qt::AlignTop);
    }
}

void TrnGenerateDialog::on_actionAddRange_triggered()
{
    TrnGenerateColEntryWidget *widget = new TrnGenerateColEntryWidget(this);
    this->ui->fixColorsVBoxLayout->addWidget(widget, 0, Qt::AlignTop);
}

void TrnGenerateDialog::on_actionDelRange_triggered(TrnGenerateColEntryWidget *caller)
{
    this->ui->fixColorsVBoxLayout->removeWidget(caller);
    delete caller;
}

void TrnGenerateDialog::on_actionAddPalette_triggered()
{
    QStringList palFilePaths = dMainWindow().filesDialog(tr("Select Palette Files"), tr("PAL Files (*.pal *.PAL)"));

    for (const QString &path : palFilePaths) {
        D1Pal *newPal = new D1Pal();
        if (!newPal->load(path)) {
            delete newPal;
            dProgressErr() << tr("Failed loading PAL file: %1.").arg(QDir::toNativeSeparators(path));
            continue;
        }

        TrnGeneratePalEntryWidget *widget = new TrnGeneratePalEntryWidget(this, newPal, true);
        this->ui->palettesVBoxLayout->addWidget(widget, 0, Qt::AlignTop);
    }
}

void TrnGenerateDialog::on_actionDelPalette_triggered(TrnGeneratePalEntryWidget *caller)
{
    this->ui->palettesVBoxLayout->removeWidget(caller);
    delete caller;
}

void TrnGenerateDialog::on_generateButton_clicked()
{
    GenerateTrnParam params;
    QList<TrnGenerateColEntryWidget *> colWidgets = this->ui->fixColorsVBoxLayout->findChildren<TrnGenerateColEntryWidget *>();
    for (TrnGenerateColEntryWidget *colWidget : colWidgets) {
        params.colors.push_back(colWidget->getTrnColor());
    }

    QList<TrnGeneratePalEntryWidget *> palWidgets = this->ui->palettesVBoxLayout->findChildren<TrnGeneratePalEntryWidget *>();
    for (TrnGeneratePalEntryWidget *palWidget : palWidgets) {
        params.pals.push_back(palWidget->getPalette());
    }

    params.mode = this->ui->levelTypeComboBox->currentIndex();

    this->close();

    dMainWindow().generateTrn(params);
}

void TrnGenerateDialog::on_cancelButton_clicked()
{
    this->close();
}
