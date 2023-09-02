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

void TrnGenerateDialog::on_levelTypeComboBox_activated(int index)
{
    std::vector<GenerateTrnColor> colors;
    {
        GenerateTrnColor black;
        black.firstcolor = 0;
        black.lastcolor = 0;
        black.shadecolor = false;
        colors.push_back(black);
    }
    for (int i = 0; i < 8; i++) {
        GenerateTrnColor levelColor;
        levelColor.firstcolor = i == 0 ? 1 : i * 16;
        levelColor.lastcolor = (i + 1) * 16 - 1;
        levelColor.shadecolor = true;
        colors.push_back(levelColor);
    }
    for (int i = 0; i < 4; i++) {
        GenerateTrnColor stdColor;
        stdColor.firstcolor = 16 * 8 + i * 8;
        stdColor.lastcolor = stdColor.firstcolor + 8 - 1;
        stdColor.shadecolor = true;
        colors.push_back(stdColor);
    }
    for (int i = 0; i < 6; i++) {
        GenerateTrnColor stdColor;
        stdColor.firstcolor = 16 * 8 + 8 * 4 + i * 16;
        stdColor.lastcolor = i == 5 ? 254 : (stdColor.firstcolor + 15);
        stdColor.shadecolor = true;
        colors.push_back(stdColor);
    }
    {
        GenerateTrnColor white;
        white.firstcolor = 255;
        white.lastcolor = 255;
        white.shadecolor = false;
        colors.push_back(white);
    }
    switch (index) {
    case DTYPE_TOWN:
        break;
    case DTYPE_CAVES:
        colors.erase(colors.begin() + 2);
        colors[1].lastcolor = 31;
        colors[1].shadecolor = false;
        // FIXME: maxdarkness?
        break;
    case DTYPE_HELL:
        break;
    case DTYPE_NEST:
        colors[1].shadecolor = false;
        // FIXME: maxdarkness?
        break;
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
