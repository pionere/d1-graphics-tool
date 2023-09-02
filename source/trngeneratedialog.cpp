#include "trngeneratedialog.h"

#include <QMessageBox>

#include "d1trs.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_trngeneratedialog.h"

#include "dungeon/all.h"

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

    this->ui->levelTypeComboBox->setCurrentIndex(DTYPE_NONE);
}

TrnGenerateDialog::~TrnGenerateDialog()
{
    delete ui;
}

void TrnGenerateDialog::initialize(D1Pal *p)
{
    QList<TrnGeneratePalEntryWidget *> palWidgets = this->ui->palettesVBoxLayout->parentWidget()->findChildren<TrnGeneratePalEntryWidget *>();
    if (palWidgets.empty()) {
        TrnGeneratePalEntryWidget *widget = new TrnGeneratePalEntryWidget(this, p, false);
        this->ui->palettesVBoxLayout->addWidget(widget, 0, Qt::AlignTop);
    }
}

void TrnGenerateDialog::on_actionAddRange_triggered()
{
    TrnGenerateColEntryWidget *widget = new TrnGenerateColEntryWidget(this);
    this->ui->colorsVBoxLayout->addWidget(widget, 0, Qt::AlignTop);
}

void TrnGenerateDialog::on_actionDelRange_triggered(TrnGenerateColEntryWidget *caller)
{
    this->ui->colorsVBoxLayout->removeWidget(caller);
    delete caller;
    this->adjustSize();
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
    this->adjustSize();
}

void TrnGenerateDialog::on_levelTypeComboBox_activated(int index)
{
    std::vector<GenerateTrnColor> colors;
    {
        GenerateTrnColor black;
        black.firstcolor = 0;
        black.lastcolor = 0;
        black.shadesteps = 0;
        black.deltasteps = false;
        black.protcolor = false;
        colors.push_back(black);
    }
    for (int i = 0; i < 8; i++) {
        GenerateTrnColor levelColor;
        levelColor.firstcolor = i == 0 ? 1 : i * 16;
        levelColor.lastcolor = (i + 1) * 16 - 1;
        levelColor.shadesteps = 0;
        levelColor.deltasteps = false;
        levelColor.protcolor = false;
        colors.push_back(levelColor);
    }
    for (int i = 0; i < 4; i++) {
        GenerateTrnColor stdColor;
        stdColor.firstcolor = 16 * 8 + i * 8;
        stdColor.lastcolor = stdColor.firstcolor + 8 - 1;
        stdColor.shadesteps = 0;
        stdColor.deltasteps = false;
        stdColor.protcolor = false;
        colors.push_back(stdColor);
    }
    for (int i = 0; i < 6; i++) {
        GenerateTrnColor stdColor;
        stdColor.firstcolor = 16 * 8 + 8 * 4 + i * 16;
        stdColor.lastcolor = i == 5 ? 254 : (stdColor.firstcolor + 15);
        stdColor.shadesteps = 0;
        stdColor.deltasteps = false;
        stdColor.protcolor = false;
        colors.push_back(stdColor);
    }
    {
        GenerateTrnColor white;
        white.firstcolor = 255;
        white.lastcolor = 255;
        white.shadesteps = -1;
        white.deltasteps = false;
        white.protcolor = true;
        colors.push_back(white);
    }
    switch (index) {
    case DTYPE_TOWN:
        colors[1].lastcolor = 127;
        colors[1].shadesteps = 5;
        colors[1].deltasteps = true;

        colors.erase(colors.begin() + 2, colors.begin() + 1 + 8);
        break;
    case DTYPE_CAVES:
        colors[1].lastcolor = 31;
        colors[1].shadesteps = -1;
        colors[1].deltasteps = false;
        colors[1].protcolor = true;

        colors.erase(colors.begin() + 2);
        break;
    case DTYPE_HELL:
    case DTYPE_CRYPT:
        colors[1].protcolor = true;
        colors[2].protcolor = true;
        break;
    case DTYPE_NEST: {
        colors[1].lastcolor = 7;
        colors[1].shadesteps = -1;
        colors[1].deltasteps = false;
        colors[1].protcolor = true;

        GenerateTrnColor stdColor;
        stdColor.firstcolor = 8;
        stdColor.lastcolor = 15;
        stdColor.shadesteps = -1;
        stdColor.deltasteps = false;
        stdColor.protcolor = true;
        colors.insert(colors.begin() + 2, stdColor);
    } break;
    case DTYPE_NONE:
        return;
    }

    QList<TrnGenerateColEntryWidget *> colWidgets = this->ui->colorsVBoxLayout->parentWidget()->findChildren<TrnGenerateColEntryWidget *>();
    for (int i = colors.size(); i < colWidgets.count(); i++) {
        this->on_actionDelRange_triggered(colWidgets[i]);
    }
    for (unsigned i = colWidgets.count(); i < colors.size(); i++) {
        this->on_actionAddRange_triggered();
    }
    colWidgets = this->ui->colorsVBoxLayout->parentWidget()->findChildren<TrnGenerateColEntryWidget *>();
    for (int i = 0; i < colWidgets.count(); i++) {
        colWidgets[i]->initialize(colors[i]);
    }
}

void TrnGenerateDialog::on_generateButton_clicked()
{
    GenerateTrnParam params;
    QList<TrnGenerateColEntryWidget *> colWidgets = this->ui->colorsVBoxLayout->parentWidget()->findChildren<TrnGenerateColEntryWidget *>();
    for (TrnGenerateColEntryWidget *colWidget : colWidgets) {
        params.colors.push_back(colWidget->getTrnColor());
    }

    QList<TrnGeneratePalEntryWidget *> palWidgets = this->ui->palettesVBoxLayout->parentWidget()->findChildren<TrnGeneratePalEntryWidget *>();
    for (TrnGeneratePalEntryWidget *palWidget : palWidgets) {
        params.pals.push_back(palWidget->getPalette());
    }
    if (params.pals.empty()) {
        QMessageBox::critical(nullptr, "Error", tr("At least one reference palette is necessary."));
        return;
    }

    params.mode = this->ui->levelTypeComboBox->currentIndex();

    this->close();

    dMainWindow().generateTrn(params);
}

void TrnGenerateDialog::on_cancelButton_clicked()
{
    this->close();
}
