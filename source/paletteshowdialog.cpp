#include "paletteshowdialog.h"

#include "config.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "pushbuttonwidget.h"
#include "ui_paletteshowdialog.h"

static QImage *loadImageARGB32(const QString &path)
{
    QImage *image = new QImage(path);
    image->convertTo(QImage::Format_ARGB32_Premultiplied);
    return image;
}

PaletteShowDialog::PaletteShowDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PaletteShowDialog())
{
    this->ui->setupUi(this);
    this->ui->palGraphicsView->setScene(&this->palScene);
    this->on_zoomEdit_escPressed();

    QHBoxLayout *layout = this->ui->imageHBoxLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogOpenButton, tr("Open"), this, &PaletteShowDialog::on_openPushButtonClicked);
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogCloseButton, tr("Close"), this, &PaletteShowDialog::on_closePushButtonClicked);
    layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

    this->images[PaletteShowDialog::GRB_SQ_PATH] = loadImageARGB32(PaletteShowDialog::GRB_SQ_PATH);
    this->images[PaletteShowDialog::RGB_SQ_PATH] = loadImageARGB32(PaletteShowDialog::RGB_SQ_PATH);
    this->images[PaletteShowDialog::RGB_WH_PATH] = loadImageARGB32(PaletteShowDialog::RGB_WH_PATH);
    this->images[PaletteShowDialog::CIE_PATH] = loadImageARGB32(PaletteShowDialog::CIE_PATH);
    this->images[PaletteShowDialog::CIEXY_PATH] = loadImageARGB32(PaletteShowDialog::CIEXY_PATH);

    this->updatePathComboBoxOptions(this->images.keys(), PaletteShowDialog::GRB_SQ_PATH);

    // connect esc events of LineEditWidgets
    QObject::connect(this->ui->zoomEdit, SIGNAL(cancel_signal()), this, SLOT(on_zoomEdit_escPressed()));
}

PaletteShowDialog::~PaletteShowDialog()
{
    delete ui;
    qDeleteAll(this->images);
    this->images.clear();
}

void PaletteShowDialog::initialize(D1Pal *p)
{
    this->pal = p;

    this->displayFrame();
}

int getColorDistance(QColor colorA, QColor colorB)
{
    int currR = colorA.red() - colorB.red();
    int currG = colorA.green() - colorB.green();
    int currB = colorA.blue() - colorB.blue();
    return currR * currR + currG * currG + currB * currB;
}

void PaletteShowDialog::displayFrame()
{
    this->palScene.clear();

    QString path = this->ui->pathComboBox->currentData().value<QString>();
    const QImage *baseImage = this->images[path];
    QImage palFrame = baseImage->copy();

    // select pixels
    const QColor color = pal->getUndefinedColor();
    for (int i = 0; i < NUM_COLORS; i++) {
        QColor palColor = pal->getColor(i);
        if (palColor == color) {
            continue;
        }
        int pos = -1;
        int dist = 0;
        const QRgb *bits = reinterpret_cast<const QRgb *>(baseImage->bits());
        for (int n = 0; n < baseImage->width() * baseImage->height(); n++) {
            if (qAlpha(bits[n]) != 255) {
                continue; // ignore non-opaque pixels
            }
            int cd = getColorDistance(palColor, QColor(bits[n]));
            if (pos == -1 || cd < dist) {
                pos = n;
                dist = cd;
            }
        }
        if (pos == -1) {
            dProgressWarn() << tr("Non opaque pixels are ignored.");
            break; // only non-opaque pixels -> skip
        }
        // palFrame.setPixel(pos % baseImage->width(), pos / baseImage->width(), color);
        QRgb *destBits = reinterpret_cast<QRgb *>(palFrame.bits());
        destBits[pos] = color.rgba();
    }

    this->palScene.setBackgroundBrush(QColor(Config::getGraphicsTransparentColor()));

    // Resize the scene rectangle to include some padding around the CEL frame
    this->palScene.setSceneRect(0, 0,
        CEL_SCENE_MARGIN + palFrame.width() + CEL_SCENE_MARGIN,
        CEL_SCENE_MARGIN + palFrame.height() + CEL_SCENE_MARGIN);
    // ui->palGraphicsView->adjustSize();

    this->palScene.addPixmap(QPixmap::fromImage(palFrame))
        ->setPos(CEL_SCENE_MARGIN, CEL_SCENE_MARGIN);
}

void PaletteShowDialog::updatePathComboBoxOptions(const QList<QString> &options, const QString &selectedOption)
{
    QComboBox *pcb = this->ui->pathComboBox;

    pcb->clear();
    int idx = 0;
    // add built-in options
    for (const QString &option : options) {
        if (!MainWindow::isResourcePath(option))
            continue;
        QString name;
        if (option == PaletteShowDialog::GRB_SQ_PATH) {
            name = tr("GRB Square");
        } else if (option ==  PaletteShowDialog::RGB_SQ_PATH) {
            name = tr("RGB Square");
        } else if (option ==  PaletteShowDialog::RGB_WH_PATH) {
            name = tr("RGB Wheel");
        } else if (option ==  PaletteShowDialog::CIE_PATH) {
            name = tr("CIE Chromaticity");
        } else {
            name = tr("CIExy Chromaticity"); // TODO: check if PaletteShowDialog::CIEXY_PATH?
        }
        pcb->addItem(name, option);
        if (selectedOption == option) {
            pcb->setCurrentIndex(idx);
            pcb->setToolTip(option);
        }
        idx++;
    }
    // add user-specific options
    for (const QString &option : options) {
        if (MainWindow::isResourcePath(option))
            continue;
        QFileInfo fileInfo(option);
        QString name = fileInfo.fileName();
        pcb->addItem(name, option);
        if (selectedOption == option) {
            pcb->setCurrentIndex(idx);
            pcb->setToolTip(option);
        }
        idx++;
    }
}

void PaletteShowDialog::on_openPushButtonClicked()
{
    QStringList allSupportedFormats;
    MainWindow::supportedImageFormats(allSupportedFormats);

    QString filter = tr("Image files (%1)").arg(allSupportedFormats.join(' '));
    QString imageFilePath = dMainWindow().fileDialog(FILE_DIALOG_MODE::OPEN, tr("Select Image File"), filter.toLatin1().data());

    if (imageFilePath.isEmpty()) {
        return;
    }

    QImage *image = loadImageARGB32(imageFilePath);
    if (image->isNull()) {
        dProgressErr() << tr("Failed loading image file: %1.").arg(QDir::toNativeSeparators(imageFilePath));
        return;
    }
    if (this->images.contains(imageFilePath)) {
        QImage *prevImage = this->images.take(imageFilePath);
        delete prevImage;
    }
    this->images[imageFilePath] = image;
    this->updatePathComboBoxOptions(this->images.keys(), imageFilePath);
    // update the view
    this->displayFrame();
}

void PaletteShowDialog::on_closePushButtonClicked()
{
    QString path = this->ui->pathComboBox->currentData().value<QString>();
    if (MainWindow::isResourcePath(path)) {
        return;
    }

    QImage *image = this->images.take(path);
    delete image;

    this->updatePathComboBoxOptions(this->images.keys(), PaletteShowDialog::GRB_SQ_PATH);
    // update the view
    this->displayFrame();
}

void PaletteShowDialog::on_pathComboBox_activated(int index)
{
    const QList<QString> &options = this->images.keys();
    this->updatePathComboBoxOptions(options, options[index]);
    // update the view
    this->displayFrame();
}

void PaletteShowDialog::on_zoomOutButton_clicked()
{
    this->palScene.zoomOut();
    this->on_zoomEdit_escPressed();
}

void PaletteShowDialog::on_zoomInButton_clicked()
{
    this->palScene.zoomIn();
    this->on_zoomEdit_escPressed();
}

void PaletteShowDialog::on_zoomEdit_returnPressed()
{
    QString zoom = this->ui->zoomEdit->text();

    this->palScene.setZoom(zoom);

    this->on_zoomEdit_escPressed();
}

void PaletteShowDialog::on_zoomEdit_escPressed()
{
    this->ui->zoomEdit->setText(this->palScene.zoomText());
    this->ui->zoomEdit->clearFocus();
}

void PaletteShowDialog::on_closeButton_clicked()
{
    this->close();
}

void PaletteShowDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
