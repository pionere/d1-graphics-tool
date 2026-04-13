#include "resizedialog.h"

#include "d1gfxset.h"
#include "mainwindow.h"
#include "ui_resizedialog.h"

ResizeDialog::ResizeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ResizeDialog())
{
    ui->setupUi(this);

    this->ui->placementButtonGroup->setId(this->ui->topLeftPlacementRadioButton, (int)RESIZE_PLACEMENT::TOP_LEFT);
    this->ui->placementButtonGroup->setId(this->ui->topPlacementRadioButton, (int)RESIZE_PLACEMENT::TOP);
    this->ui->placementButtonGroup->setId(this->ui->topRightPlacementRadioButton, (int)RESIZE_PLACEMENT::TOP_RIGHT);
    this->ui->placementButtonGroup->setId(this->ui->centerLeftPlacementRadioButton, (int)RESIZE_PLACEMENT::CENTER_LEFT);
    this->ui->placementButtonGroup->setId(this->ui->centerPlacementRadioButton, (int)RESIZE_PLACEMENT::CENTER);
    this->ui->placementButtonGroup->setId(this->ui->centerRightPlacementRadioButton, (int)RESIZE_PLACEMENT::CENTER_RIGHT);
    this->ui->placementButtonGroup->setId(this->ui->bottomLeftPlacementRadioButton, (int)RESIZE_PLACEMENT::BOTTOM_LEFT);
    this->ui->placementButtonGroup->setId(this->ui->bottomPlacementRadioButton, (int)RESIZE_PLACEMENT::BOTTOM);
    this->ui->placementButtonGroup->setId(this->ui->bottomRightPlacementRadioButton, (int)RESIZE_PLACEMENT::BOTTOM_RIGHT);
}

ResizeDialog::~ResizeDialog()
{
    delete ui;
}

void ResizeDialog::initialize(D1Gfx *gfx, D1Gfxset *gfxset)
{
    this->gfx = gfx;
    this->gfxset = gfxset;

    this->ui->resizeAllCheckBox->setVisible(gfxset != nullptr);
    if (this->ui->backColorLineEdit->text().isEmpty()) {
        this->ui->backColorLineEdit->setText("256");
    }
    this->ui->widthLineEdit->setText("");
    this->ui->heightLineEdit->setText("");
    this->ui->rangeFromLineEdit->setText("");
    this->ui->rangeToLineEdit->setText("");
    this->ui->centerPlacementRadioButton->setChecked(true);
}

void ResizeDialog::setMinSize(bool width)
{
    const bool gfxOnly = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
    QRect rect;
    QSize frameSize;
    if (this->gfxset != nullptr && !gfxOnly) {
        rect = this->gfxset->getBoundary();
        frameSize = this->gfxset->getFrameSize();
    } else {
        rect = this->gfx->getBoundary();
        frameSize = this->gfx->getFrameSize();
    }
    if (!frameSize.isValid()) {
        QMessageBox::critical(this, tr("Error"), tr("Framesize is not constant"));
        return;
    }
    RESIZE_PLACEMENT placement = (RESIZE_PLACEMENT)this->ui->placementButtonGroup->checkedId();

    int vert = 0, horz = 0;
    switch (placement) {
    case RESIZE_PLACEMENT::TOP_LEFT:     vert = 0; horz = 0; break;
    case RESIZE_PLACEMENT::TOP:          vert = 0; horz = 1; break;
    case RESIZE_PLACEMENT::TOP_RIGHT:    vert = 0; horz = 2; break;
    case RESIZE_PLACEMENT::CENTER_LEFT:  vert = 1; horz = 0; break;
    case RESIZE_PLACEMENT::CENTER:       vert = 1; horz = 1; break;
    case RESIZE_PLACEMENT::CENTER_RIGHT: vert = 1; horz = 2; break;
    case RESIZE_PLACEMENT::BOTTOM_LEFT:  vert = 2; horz = 0; break;
    case RESIZE_PLACEMENT::BOTTOM:       vert = 2; horz = 1; break;
    case RESIZE_PLACEMENT::BOTTOM_RIGHT: vert = 2; horz = 2; break;
    }
    getFrameSize
    if (width) {
        int w = rect.width();
        switch (horz) {
        case 0: w += rect.x(); break;
        case 1: w = frameSize.width() - 2 * std::min(rect.x(), frameSize.width() - (w + rect.x())); break;
        case 2: w = frameSize.width() - rect.x(); break;
        }
        this->ui->widthLineEdit->setText(QString::number(w));
    } else {
        int h = rect.height();
        switch (vert) {
        case 0: h += rect.y(); break;
        case 1: h = frameSize.height() - 2 * std::min(rect.y(), frameSize.height() - (h + rect.y())); break;
        case 2: h = frameSize.height() - rect.y()); break;
        }
        this->ui->heightLineEdit->setText(QString::number(w));
    }
}

void ResizeDialog::on_minWidthButton_clicked()
{
    this->setMinSize(true);
}

void ResizeDialog::on_minHeightButton_clicked()
{
    this->setMinSize(false);
}

void ResizeDialog::on_resizeButton_clicked()
{
    ResizeParam params;

    params.width = this->ui->widthLineEdit->nonNegInt();
    params.height = this->ui->heightLineEdit->nonNegInt();
    params.backcolor = this->ui->backColorLineEdit->text().toUShort();
    params.rangeFrom = this->ui->rangeFromLineEdit->nonNegInt();
    params.rangeTo = this->ui->rangeToLineEdit->nonNegInt();
    params.resizeAll = this->ui->resizeAllCheckBox->isChecked();
    params.placement = (RESIZE_PLACEMENT)this->ui->placementButtonGroup->checkedId();

    if (params.backcolor > D1PAL_COLORS) {
        params.backcolor = D1PAL_COLORS;
    }

    this->close();

    dMainWindow().resize(params);
}

void ResizeDialog::on_resizeCancelButton_clicked()
{
    this->close();
}

void ResizeDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
