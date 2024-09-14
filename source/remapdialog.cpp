#include "remapdialog.h"

#include <QMessageBox>

#include "mainwindow.h"
#include "ui_remapdialog.h"

#define UNSET_VALUE INT_MIN

RemapDialog::RemapDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RemapDialog())
{
    ui->setupUi(this);

    QObject::connect(this->ui->colorFrom0LineEdit, SIGNAL(cancel_signal()), this, SLOT(on_colorFrom0LineEdit_escPressed()));
    QObject::connect(this->ui->colorFrom1LineEdit, SIGNAL(cancel_signal()), this, SLOT(on_colorFrom1LineEdit_escPressed()));
    QObject::connect(this->ui->colorTo0LineEdit, SIGNAL(cancel_signal()), this, SLOT(on_colorTo0LineEdit_escPressed()));
    QObject::connect(this->ui->colorTo1LineEdit, SIGNAL(cancel_signal()), this, SLOT(on_colorTo1LineEdit_escPressed()));
    QObject::connect(this->ui->range0LineEdit, SIGNAL(cancel_signal()), this, SLOT(on_range0LineEdit_escPressed()));
    QObject::connect(this->ui->range1LineEdit, SIGNAL(cancel_signal()), this, SLOT(on_range1LineEdit_escPressed()));

    this->range = { UNSET_VALUE, UNSET_VALUE };
    this->colorTo = { UNSET_VALUE, UNSET_VALUE };
}

RemapDialog::~RemapDialog()
{
    delete ui;
}

void RemapDialog::initialize(const PaletteWidget *palWidget)
{
    this->colorFrom = palWidget->getCurrentSelection();
    if ((unsigned)this->colorFrom.first > D1PAL_COLORS || (unsigned)this->colorFrom.second > D1PAL_COLORS)
        this->colorFrom = { D1PAL_COLORS, D1PAL_COLORS };

    // this->range = { UNSET_VALUE, UNSET_VALUE };
    // this->colorTo = { UNSET_VALUE, UNSET_VALUE };

    this->updateFields();
}

static void displayOptionalIntValue(LineEditWidget *w, int value)
{
    if (value != UNSET_VALUE)
        w->setText(QString::number(value));
    else
        w->setText("");
}

static int getOptionalIntValue(LineEditWidget *w)
{
    bool ok;
    int value = w->text().toInt(&ok);
    if (!ok)
        value = UNSET_VALUE;
    return value;
}

void RemapDialog::updateFields()
{
    if (this->colorTo.first != UNSET_VALUE && this->colorTo.second != UNSET_VALUE
     && this->colorTo.first != this->colorTo.second) {
        if (this->colorTo.first < this->colorTo.second) {
            this->colorTo.second = this->colorTo.first + (this->colorFrom.second - this->colorFrom.first);
        } else {
            this->colorTo.second = this->colorTo.first - (this->colorFrom.second - this->colorFrom.first);
        }
    }

    this->ui->colorFrom0LineEdit->setText(QString::number(this->colorFrom.first));
    this->ui->colorFrom1LineEdit->setText(QString::number(this->colorFrom.second));
    displayOptionalIntValue(this->ui->colorTo0LineEdit, this->colorTo.first);
    displayOptionalIntValue(this->ui->colorTo1LineEdit, this->colorTo.second);

    displayOptionalIntValue(this->ui->range0LineEdit, this->range.first);
    displayOptionalIntValue(this->ui->range1LineEdit, this->range.second);
}

void RemapDialog::on_colorFrom0LineEdit_returnPressed()
{
    bool ok;
    this->colorFrom.first = this->ui->colorFrom0LineEdit->text().toInt(&ok);
    if (!ok)
        this->colorFrom.first = this->colorFrom.second;
    else if (this->colorFrom.first < 0)
        this->colorFrom.first = 0;
    else if (this->colorFrom.first > D1PAL_COLORS)
        this->colorFrom.first = D1PAL_COLORS;

    this->on_colorFrom0LineEdit_escPressed();
}

void RemapDialog::on_colorFrom0LineEdit_escPressed()
{
    // update colorFrom0LineEdit
    this->updateFields();
    this->ui->colorFrom0LineEdit->clearFocus();
}

void RemapDialog::on_colorFrom1LineEdit_returnPressed()
{
    bool ok;
    this->colorFrom.second = this->ui->colorFrom1LineEdit->text().toInt(&ok);
    if (!ok)
        this->colorFrom.second = this->colorFrom.first;
    else if (this->colorFrom.second < 0)
        this->colorFrom.second = 0;
    else if (this->colorFrom.second > D1PAL_COLORS)
        this->colorFrom.second = D1PAL_COLORS;

    this->on_colorFrom1LineEdit_escPressed();
}

void RemapDialog::on_colorFrom1LineEdit_escPressed()
{
    // update colorFrom1LineEdit
    this->updateFields();
    this->ui->colorFrom1LineEdit->clearFocus();
}

void RemapDialog::on_colorTo0LineEdit_returnPressed()
{
    this->colorTo.first = getOptionalIntValue(this->ui->colorTo0LineEdit);
    /*if (this->colorTo.first != UNSET_VALUE && this->colorTo.second != UNSET_VALUE
     && this->colorTo.first != this->colorTo.second) {
        if (this->colorTo.first < this->colorTo.second) {
            this->colorTo.second = this->colorTo.first + (this->colorFrom.second - this->colorFrom.first);
        } else {
            this->colorTo.second = this->colorTo.first - (this->colorFrom.second - this->colorFrom.first);
        }
    }*/

    this->on_colorTo0LineEdit_escPressed();
}

void RemapDialog::on_colorTo0LineEdit_escPressed()
{
    // update colorTo0LineEdit
    this->updateFields();
    this->ui->colorTo0LineEdit->clearFocus();
}

void RemapDialog::on_colorTo1LineEdit_returnPressed()
{
    this->colorTo.second = getOptionalIntValue(this->ui->colorTo1LineEdit);
    if (this->colorTo.first != UNSET_VALUE && this->colorTo.second != UNSET_VALUE
     && this->colorTo.first != this->colorTo.second) {
        if (this->colorTo.first < this->colorTo.second) {
            this->colorTo.first = this->colorTo.second - (this->colorFrom.second - this->colorFrom.first);
        } else {
            this->colorTo.first = this->colorTo.second + (this->colorFrom.second - this->colorFrom.first);
        }
    }

    this->on_colorTo1LineEdit_escPressed();
}

void RemapDialog::on_colorTo1LineEdit_escPressed()
{
    // update colorTo1LineEdit
    this->updateFields();
    this->ui->colorTo1LineEdit->clearFocus();
}

void RemapDialog::on_range0LineEdit_returnPressed()
{
    this->range.first = getOptionalIntValue(this->ui->range0LineEdit);
    if (this->range.second != UNSET_VALUE && this->range.first != UNSET_VALUE && this->range.second < this->range.first)
        this->range.second = this->range.first;

    this->on_range0LineEdit_escPressed();
}

void RemapDialog::on_range0LineEdit_escPressed()
{
    // update range0LineEdit
    this->updateFields();
    this->ui->range0LineEdit->clearFocus();
}

void RemapDialog::on_range1LineEdit_returnPressed()
{
    this->range.second = getOptionalIntValue(this->ui->range1LineEdit);
    if (this->range.second != UNSET_VALUE && this->range.first != UNSET_VALUE && this->range.second < this->range.first)
        this->range.first = this->range.second;

    this->on_range1LineEdit_escPressed();
}

void RemapDialog::on_range1LineEdit_escPressed()
{
    // update range1LineEdit
    this->updateFields();
    this->ui->range1LineEdit->clearFocus();
}

void RemapDialog::on_remapButton_clicked()
{
    QString line;
    RemapParam params;

    params.colorFrom = this->colorFrom;
    // assert((unsigned)params.colorFrom.first <= D1PAL_COLORS && (unsigned)params.colorFrom.second <= D1PAL_COLORS);
    params.colorTo = this->colorTo;
    if (params.colorTo.first == UNSET_VALUE) {
        if (params.colorTo.second == UNSET_VALUE) {
            // [x, y] -> [-, -]
            params.colorTo.first = params.colorTo.second = D1PAL_COLORS;
        } else {
            // [x, y] -> [-, j]
            params.colorTo.first = params.colorTo.second; // +params.colorFrom.first - params.colorFrom.second;
        }
    } else {
        if (params.colorTo.second == UNSET_VALUE) {
            // [x, y] -> [i, -]
            params.colorTo.second = params.colorTo.first; // + params.colorFrom.second - params.colorFrom.first;
        } else {
            // [x, y] -> [i, j]
            // assert(params.colorFrom.second - params.colorFrom.first == params.colorTo.second - params.colorTo.first);
        }
    }

    params.frames = this->range;
    if (params.frames.first == UNSET_VALUE)
        params.frames.first = 0;
    if (params.frames.second == UNSET_VALUE)
        params.frames.second = 0;

    /*line = this->ui->colorFrom0LineEdit->text();
    params.colorFrom.first = line.toShort();
    line = this->ui->colorFrom1LineEdit->text();
    if (line.isEmpty()) {
        params.colorFrom.second = params.colorFrom.first;
    } else {
        params.colorFrom.second = line.toShort();
    }
    line = this->ui->colorTo0LineEdit->text();
    params.colorTo.first = line.toShort();
    line = this->ui->colorTo1LineEdit->text();
    if (line.isEmpty()) {
        params.colorTo.second = params.colorTo.first;
    } else {
        params.colorTo.second = line.toShort();
    }
    params.frames.first = this->ui->range0LineEdit->nonNegInt();
    params.frames.second = this->ui->range1LineEdit->nonNegInt();

    if (params.colorFrom.first > D1PAL_COLORS) {
        params.colorFrom.first = D1PAL_COLORS;
    }
    if (params.colorFrom.second > D1PAL_COLORS) {
        params.colorFrom.second = D1PAL_COLORS;
    }
    if (params.colorTo.first > D1PAL_COLORS) {
        params.colorTo.first = D1PAL_COLORS;
    }
    if (params.colorTo.second > D1PAL_COLORS) {
        params.colorTo.second = D1PAL_COLORS;
    }*/

    if (params.colorFrom.first > params.colorFrom.second) {
        std::swap(params.colorFrom.first, params.colorFrom.second);
        std::swap(params.colorTo.first, params.colorTo.second);
    }

    /*if (params.colorTo.first != params.colorTo.second && abs(params.colorTo.second - params.colorTo.first) != params.colorFrom.second - params.colorFrom.first) {
        this->ui->colorFrom0LineEdit->setText(QString::number(params.colorFrom.first));
        this->ui->colorFrom1LineEdit->setText(QString::number(params.colorFrom.second));
        this->ui->colorTo0LineEdit->setText(QString::number(params.colorTo.first));
        this->ui->colorTo1LineEdit->setText(QString::number(params.colorTo.second));
        this->ui->range0LineEdit->setText(QString::number(params.frames.first));
        this->ui->range1LineEdit->setText(QString::number(params.frames.second));

        QMessageBox::warning(this, tr("Warning"), tr("Source and target selection length do not match."));
        return;
    }*/

    this->close();

    dMainWindow().remapColors(params);
}

void RemapDialog::on_remapCancelButton_clicked()
{
    this->close();
}

void RemapDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
