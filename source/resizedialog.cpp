#include "resizedialog.h"

#include "d1gfx.h"
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

void ResizeDialog::initialize()
{
    if (this->ui->backColorLineEdit->text().isEmpty()) {
        this->ui->backColorLineEdit->setText("256");
    }
    this->ui->widthLineEdit->setText("");
    this->ui->heightLineEdit->setText("");
    this->ui->rangeFromLineEdit->setText("");
    this->ui->rangeToLineEdit->setText("");
    this->ui->centerPlacementRadioButton->setChecked(true);
}

void ResizeDialog::on_resizeButton_clicked()
{
    ResizeParam params;

    params.width = this->ui->widthLineEdit->nonNegInt();
    params.height = this->ui->heightLineEdit->nonNegInt();
    params.backcolor = this->ui->backColorLineEdit->text().toUShort();
    params.rangeFrom = this->ui->rangeFromLineEdit->nonNegInt();
    params.rangeTo = this->ui->rangeToLineEdit->nonNegInt();
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
