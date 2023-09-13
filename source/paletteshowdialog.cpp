#include "paletteshowdialog.h"

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


    QHBoxLayout *layout = this->ui->imageHBoxLayout;
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogOpenButton, tr("Open"), this, &PaletteShowDialog::on_openPushButtonClicked);
    PushButtonWidget::addButton(this, layout, QStyle::SP_DialogCloseButton, tr("Close"), this, &PaletteShowDialog::on_closePushButtonClicked);
    layout->addSpacerItem(new QSpacerItem(0, 0));

    this->images[PaletteShowDialog::WHEEL_PATH] = loadImageARGB32(PaletteShowDialog::WHEEL_PATH);
    this->images[PaletteShowDialog::CIE_PATH] = loadImageARGB32(PaletteShowDialog::CIE_PATH);
    this->images[PaletteShowDialog::CIEXY_PATH] = loadImageARGB32(PaletteShowDialog::CIEXY_PATH);

    this->updatePathComboBoxOptions(this->images.keys(), PaletteShowDialog::WHEEL_PATH);
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
        for (int n = 0; i < baseImage->width() * baseImage->height(); i++) {
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
			QColor c0 = baseImage->pixelColor(baseImage->width() / 2, baseImage->height() / 2);
			QColor c1 = QColor(bits[baseImage->width() * baseImage->height() / 2 + baseImage->width() / 2]);
            dProgressWarn() << tr("Non opaque pixels are ignored. Width %1 Height %2. Alpha %3 vs %4 name: %5 vs %6").arg(baseImage->width()).arg(baseImage->height()).arg(c0.alpha()).arg(c1.alpha()).arg(c0.name()).arg(c1.name());
            break; // only non-opaque pixels -> skip
        }
        palFrame.bits()[pos] = color.rgba();
    }

    this->palScene.setBackgroundBrush(QColor(Qt::transparent));

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
        if (option == PaletteShowDialog::WHEEL_PATH) {
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

    this->updatePathComboBoxOptions(this->images.keys(), PaletteShowDialog::WHEEL_PATH);
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
