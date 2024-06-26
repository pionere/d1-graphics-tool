#include "exportdialog.h"

#include <algorithm>
#include <vector>

#include <QApplication>
#include <QColorDialog>
#include <QFileDialog>
#include <QImageWriter>
#include <QMessageBox>
#include <QPainter>
#include <QtConcurrent>

#include "d1image.h"
#include "d1pcx.h"
#include "d1smk.h"
#include "d1wav.h"
#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_exportdialog.h"

ExportDialog::ExportDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ExportDialog())
{
    ui->setupUi(this);

    // initialize the format combobox
    QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    QStringList formatTxts;
    // add PCX support
    formatTxts.append("PCX");
    for (QByteArray &format : formats) {
        QString fmt = format.toUpper();
        if (fmt != "PCX")
            formatTxts.append(fmt);
    }
    // preselect PNG
    int defaultIndex = formatTxts.indexOf("PNG");
    if (defaultIndex < 0)
        defaultIndex = 0;
    this->ui->formatComboBox->addItems(formatTxts);
    this->ui->formatComboBox->setCurrentIndex(defaultIndex);
    // 'disable' WAV by default
    this->hasWavFormat = false;
}

ExportDialog::~ExportDialog()
{
    delete ui;
}

void ExportDialog::initialize(D1Gfx *g, D1Tileset *ts)
{
    this->gfx = g;
    this->tileset = ts;
    bool isSmk = g->getType() == D1CEL_TYPE::SMK;

    // initialize format
    if (isSmk) {
        if (!this->hasWavFormat) {
            this->hasWavFormat = true;
            this->ui->formatComboBox->addItem("WAV");
        }
    } else {
        if (this->hasWavFormat) {
            this->hasWavFormat = false;
            this->ui->formatComboBox->removeItem(this->ui->formatComboBox->count() - 1);
        }
    }

    // initialize files count
    /*this->ui->filesCountComboBox->setEnabled(multiFrame);
    if (!multiFrame) {
        this->ui->filesCountComboBox->setCurrentIndex(0);
    }*/

    // initialize content type
    bool isTileset = this->tileset != nullptr;
    this->ui->contentTypeComboBox->setEnabled(isTileset);
    if (!isTileset) {
        this->ui->contentTypeComboBox->setCurrentIndex(0);
    }

    // initialize content placement
    /*this->ui->contentPlacementComboBox->setEnabled(multiFrame);
    if (!multiFrame) {
        this->ui->contentPlacementComboBox->setCurrentIndex(0);
    }*/

    // ensure the format is preselected and the depending fields are enabled/disabled
    int idx = this->ui->formatComboBox->currentIndex();
    this->on_formatComboBox_activated(idx == -1 ? 0 : idx);
}

void ExportDialog::on_formatComboBox_activated(int index)
{
    bool wavFormat = false;
    if (this->hasWavFormat) {
        wavFormat = index == this->ui->formatComboBox->count() - 1;

        this->ui->contentPlacementComboBox->setEnabled(!wavFormat);
        this->ui->contentCanvasIndexLineEdit->setEnabled(!wavFormat);
        this->ui->contentCanvasColorLineEdit->setEnabled(!wavFormat);
        this->ui->contentCanvasColorPushButton->setEnabled(!wavFormat);
    }
    this->ui->contentTypeComboBox->setItemText(0, wavFormat ? "Audio" : "Frame");
}

void ExportDialog::on_contentCanvasColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor();
    if (color.isValid()) {
        this->ui->contentCanvasColorLineEdit->setText(color.name());
    }
}

void ExportDialog::on_outputFolderBrowseButton_clicked()
{
    QString dirPath = dMainWindow().folderDialog(tr("Select Output Folder"));

    if (dirPath.isEmpty())
        return;

    this->ui->outputFolderEdit->setText(dirPath);
}

static void saveImage(const std::vector<std::vector<D1GfxPixel>> &pixels, const D1Pal *pal, const QString &fileName, const ExportParam &params)
{
    QString path = params.outFolder + "/" + fileName + params.outFileExtension;
    if (path.endsWith(".pcx")) {
        D1Pcx::save(pixels, pal, path, params);
        return;
    }

    QSize imageSize = D1PixelImage::getImageSize(pixels);
    QImage image = QImage(imageSize, QImage::Format_ARGB32);
    if (image.isNull()) {
        dProgressFail() << QApplication::tr("Failed to create image with (%1:%2) dimensions.").arg(imageSize.width()).arg(imageSize.height());
        return;
    }
    QRgb *destBits = reinterpret_cast<QRgb *>(image.bits());
    for (int y = 0; y < imageSize.height(); y++) {
        for (int x = 0; x < imageSize.width(); x++, destBits++) {
            const D1GfxPixel &d1pix = pixels[y][x];

            QColor color;
            if (d1pix.isTransparent()) {
                if (params.transparentIndex < D1PAL_COLORS) {
                    color = pal->getColor(params.transparentIndex);
                } else {
                    color = params.transparentColor;
                }
            } else {
                color = pal->getColor(d1pix.getPaletteIndex());
            }

            // image.setPixelColor(x, y, color);
            *destBits = color.rgba();
        }
    }

    if (image.save(path))
        dProgress() << QApplication::tr("%1 created.").arg(QDir::toNativeSeparators(path));
    else
        dProgressFail() << QApplication::tr("Failed to create %1.").arg(QDir::toNativeSeparators(path));
}

void ExportDialog::exportLevelTiles25D(const D1Til *til, const D1Gfx *gfx, const ExportParam &params)
{
    QString fileNameBase = QFileInfo(til->getFilePath()).fileName();

    fileNameBase.replace(".", "_25d_");

    int count = til->getTileCount();
    int tileFrom = params.rangeFrom;
    if (tileFrom != 0) {
        tileFrom--;
    }
    int tileTo = params.rangeTo;
    if (tileTo == 0 || tileTo > count) {
        tileTo = count;
    }
    tileTo--;
    int amount = tileTo - tileFrom + 1;
    // nothing to export
    if (amount == 0) {
        return;
    }
    ProgressDialog::incBar(tr("Exporting 2.5d tiles..."), amount);
    // single tile
    /*if (amount == 1 && tileFrom == 0) {
        // one file for the only tile (not indexed)
        QString &outputFileName = fileNameBase;
        saveImage(til->getTilePixelImage(0), gfx->getPalette(), outputFileName, params);
        return;
    }*/
    // multiple tiles
    if (amount == 1 || params.multi) {
        // one file for each tile (indexed)
        for (int i = tileFrom; i <= tileTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return;
            // }

            QString outputFileName = fileNameBase + QString("%1").arg(i, 4, 10, QChar('0'));
            saveImage(til->getTilePixelImage(0), gfx->getPalette(), outputFileName, params);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
        return;
    }
    // one file for all tiles
    if (tileFrom != 0 || tileTo < count - 1) {
        fileNameBase += QString::number(tileFrom + 1) + "_" + QString::number(tileTo + 1);
    }
    QString &outputFileName = fileNameBase;

    const unsigned tileWidth = til->getMin()->getSubtileWidth() * 2 * MICRO_WIDTH;
    const unsigned tileHeight = til->getMin()->getSubtileHeight() * MICRO_HEIGHT + 32;
    constexpr unsigned TILES_PER_LINE = 8;
    unsigned tempOutputImageWidth = 0;
    unsigned tempOutputImageHeight = 0;
    if (params.placement == 0) { // grouped
        tempOutputImageWidth = tileWidth * TILES_PER_LINE;
        tempOutputImageHeight = tileHeight * ((amount + (TILES_PER_LINE - 1)) / TILES_PER_LINE);
    } else if (params.placement == 2) { // tiles on one column
        tempOutputImageWidth = tileWidth;
        tempOutputImageHeight = tileHeight * amount;
    } else { // params.placement == 1 -- tiles on one line
        tempOutputImageWidth = tileWidth * amount;
        tempOutputImageHeight = tileHeight;
    }

    std::vector<std::vector<D1GfxPixel>> tempOutputPixels;
    D1PixelImage::createImage(tempOutputPixels, tempOutputImageWidth, tempOutputImageHeight);

    if (params.placement == 0) { // grouped
        unsigned dx = 0, dy = 0;
        for (int i = tileFrom; i <= tileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            const std::vector<std::vector<D1GfxPixel>> pixels = til->getTilePixelImage(i);
            D1PixelImage::drawImage(tempOutputPixels, dx, dy, pixels);

            const QSize imageSize = D1PixelImage::getImageSize(pixels);
            dx += imageSize.width();
            if (dx >= tempOutputImageWidth) {
                dx = 0;
                dy += imageSize.height();
            }
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
    } else {
        int cursor = 0;
        for (int i = tileFrom; i <= tileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            const std::vector<std::vector<D1GfxPixel>> pixels = til->getTilePixelImage(i);
            const QSize imageSize = D1PixelImage::getImageSize(pixels);
            if (params.placement == 2) { // tiles on one column
                D1PixelImage::drawImage(tempOutputPixels, 0, cursor, pixels);
                cursor += imageSize.height();
            } else { // params.placement == 1 -- tiles on one line
                D1PixelImage::drawImage(tempOutputPixels, cursor, 0, pixels);
                cursor += imageSize.width();
            }
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
    }

    saveImage(tempOutputPixels, gfx->getPalette(), outputFileName, params);
}

void ExportDialog::exportLevelTiles(const D1Til *til, const D1Gfx *gfx, const ExportParam &params)
{
    QString fileNameBase = QFileInfo(til->getFilePath()).fileName();

    fileNameBase.replace(".", "_flat_");

    int count = til->getTileCount();
    int tileFrom = params.rangeFrom;
    if (tileFrom != 0) {
        tileFrom--;
    }
    int tileTo = params.rangeTo;
    if (tileTo == 0 || tileTo > count) {
        tileTo = count;
    }
    tileTo--;
    int amount = tileTo - tileFrom + 1;
    // nothing to export
    if (amount <= 0) {
        return;
    }
    ProgressDialog::incBar(tr("Exporting flat tiles..."), amount);
    // single tile
    /*if (amount == 1 && tileFrom == 0) {
        // one file for the only tile (not indexed)
        QString &outputFileName = fileNameBase;
        saveImage(til->getFlatTilePixelImage(0), gfx->getPalette(), outputFileName, params);
        return;
    }*/
    // multiple tiles
    if (amount == 1 || params.multi) {
        // one file for each tile (indexed)
        for (int i = tileFrom; i <= tileTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return;
            // }
            QString outputFileName = fileNameBase + QString("%1").arg(i, 4, 10, QChar('0'));
            saveImage(til->getFlatTilePixelImage(0), gfx->getPalette(), outputFileName, params);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
        return;
    }
    // one file for all tiles
    if (tileFrom != 0 || tileTo < count - 1) {
        fileNameBase += QString::number(tileFrom + 1) + "_" + QString::number(tileTo + 1);
    }
    QString &outputFileName = fileNameBase;

    unsigned tileWidth = til->getMin()->getSubtileWidth() * MICRO_WIDTH * TILE_WIDTH * TILE_HEIGHT;
    unsigned tileHeight = til->getMin()->getSubtileHeight() * MICRO_HEIGHT;
    constexpr unsigned TILES_PER_LINE = 4;
    unsigned tempOutputImageWidth = 0;
    unsigned tempOutputImageHeight = 0;
    if (params.placement == 0) { // grouped
        tempOutputImageWidth = tileWidth * TILES_PER_LINE;
        tempOutputImageHeight = tileHeight * ((amount + (TILES_PER_LINE - 1)) / TILES_PER_LINE);
    } else if (params.placement == 2) { // tiles on one column
        tempOutputImageWidth = tileWidth;
        tempOutputImageHeight = tileHeight * amount;
    } else { // params.placement == 1 -- tiles on one line
        tempOutputImageWidth = tileWidth * amount;
        tempOutputImageHeight = tileHeight;
    }

    std::vector<std::vector<D1GfxPixel>> tempOutputPixels;
    D1PixelImage::createImage(tempOutputPixels, tempOutputImageWidth, tempOutputImageHeight);

    if (params.placement == 0) { // grouped
        unsigned dx = 0, dy = 0;
        for (int i = tileFrom; i <= tileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            const std::vector<std::vector<D1GfxPixel>> pixels = til->getFlatTilePixelImage(i);
            D1PixelImage::drawImage(tempOutputPixels, dx, dy, pixels);

            const QSize imageSize = D1PixelImage::getImageSize(pixels);
            dx += imageSize.width();
            if (dx >= tempOutputImageWidth) {
                dx = 0;
                dy += imageSize.height();
            }
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
    } else {
        int cursor = 0;
        for (int i = tileFrom; i <= tileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            const std::vector<std::vector<D1GfxPixel>> pixels = til->getFlatTilePixelImage(i);
            const QSize imageSize = D1PixelImage::getImageSize(pixels);
            if (params.placement == 2) { // tiles on one column
                D1PixelImage::drawImage(tempOutputPixels, 0, cursor, pixels);
                cursor += imageSize.height();
            } else { // params.placement == 1 -- tiles on one line
                D1PixelImage::drawImage(tempOutputPixels, cursor, 0, pixels);
                cursor += imageSize.width();
            }
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
    }

    saveImage(tempOutputPixels, gfx->getPalette(), outputFileName, params);
}

void ExportDialog::exportLevelSubtiles(const D1Min *min, const D1Gfx *gfx, const ExportParam &params)
{
    QString fileNameBase = QFileInfo(min->getFilePath()).fileName();

    fileNameBase.replace(".", "_");

    int count = min->getSubtileCount();
    int subtileFrom = params.rangeFrom;
    if (subtileFrom != 0) {
        subtileFrom--;
    }
    int subtileTo = params.rangeTo;
    if (subtileTo == 0 || subtileTo > count) {
        subtileTo = count;
    }
    subtileTo--;
    int amount = subtileTo - subtileFrom + 1;
    // nothing to export
    if (amount <= 0) {
        return;
    }
    ProgressDialog::incBar(tr("Exporting subtiles..."), amount);
    // single subtile
    /*if (amount == 1 && subtileFrom == 0) {
        // one file for the only subtile (not indexed)
        QString &outputFileName = fileNameBase;
        saveImage(min->getSubtilePixelImage(0), gfx->getPalette(), outputFileName, params);
        return;
    }*/
    // multiple subtiles
    if (amount == 1 || params.multi) {
        // one file for each subtile (indexed)
        for (int i = subtileFrom; i <= subtileTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return;
            // }
            QString outputFileName = fileNameBase + QString("%1").arg(i, 4, 10, QChar('0'));
            saveImage(min->getSubtilePixelImage(i), gfx->getPalette(), outputFileName, params);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
        return;
    }
    // one file for all subtiles
    if (subtileFrom != 0 || subtileTo < count - 1) {
        fileNameBase += QString::number(subtileFrom + 1) + "_" + QString::number(subtileTo + 1);
    }
    QString &outputFileName = fileNameBase;

    unsigned subtileWidth = min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = min->getSubtileHeight() * MICRO_HEIGHT;

    unsigned tempOutputImageWidth = 0;
    unsigned tempOutputImageHeight = 0;
    if (params.placement == 0) { // grouped
        tempOutputImageWidth = subtileWidth * EXPORT_SUBTILES_PER_LINE;
        tempOutputImageHeight = subtileHeight * ((amount + (EXPORT_SUBTILES_PER_LINE - 1)) / EXPORT_SUBTILES_PER_LINE);
    } else if (params.placement == 2) { // subtiles on one column
        tempOutputImageWidth = subtileWidth;
        tempOutputImageHeight = subtileHeight * amount;
    } else { // params.placement == 1 -- subtiles on one line
        tempOutputImageWidth = subtileWidth * amount;
        tempOutputImageHeight = subtileHeight;
        if ((amount % (TILE_WIDTH * TILE_HEIGHT)) == 0) {
            tempOutputImageWidth += subtileWidth; // add an extra subtile to ensure it is not recognized as a flat tile
        }
    }

    std::vector<std::vector<D1GfxPixel>> tempOutputPixels;
    D1PixelImage::createImage(tempOutputPixels, tempOutputImageWidth, tempOutputImageHeight);

    if (params.placement == 0) { // grouped
        unsigned dx = 0, dy = 0;
        for (int i = subtileFrom; i <= subtileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            const std::vector<std::vector<D1GfxPixel>> pixels = min->getSubtilePixelImage(i);
            D1PixelImage::drawImage(tempOutputPixels, dx, dy, pixels);

            const QSize imageSize = D1PixelImage::getImageSize(pixels);
            dx += imageSize.width();
            if (dx >= tempOutputImageWidth) {
                dx = 0;
                dy += imageSize.height();
            }
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
    } else {
        int cursor = 0;
        for (int i = subtileFrom; i <= subtileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            const std::vector<std::vector<D1GfxPixel>> pixels = min->getSubtilePixelImage(i);
            const QSize imageSize = D1PixelImage::getImageSize(pixels);
            if (params.placement == 2) { // subtiles on one column
                D1PixelImage::drawImage(tempOutputPixels, 0, cursor, pixels);
                cursor += imageSize.height();
            } else { // params.placement == 1 -- subtiles on one line
                D1PixelImage::drawImage(tempOutputPixels, cursor, 0, pixels);
                cursor += imageSize.width();
            }
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
    }

    saveImage(tempOutputPixels, gfx->getPalette(), outputFileName, params);
}

void ExportDialog::exportFrames(const D1Gfx *gfx, const ExportParam &params)
{
    QString fileNameBase = QFileInfo(gfx->getFilePath()).fileName();

    fileNameBase.replace(".", "_");

    int count = gfx->getFrameCount();
    int frameFrom = params.rangeFrom;
    if (frameFrom != 0) {
        frameFrom--;
    }
    int frameTo = params.rangeTo;
    if (frameTo == 0 || frameTo > count) {
        frameTo = count;
    }
    frameTo--;
    int amount = frameTo - frameFrom + 1;
    // nothing to export
    if (amount <= 0) {
        return;
    }
    ProgressDialog::incBar(tr("Exporting frames..."), amount);
    // single frame
    if (amount == 1 && frameFrom == 0) {
        // one file for the only frame (not indexed)
        QString &outputFileName = fileNameBase;
        saveImage(gfx->getFramePixelImage(0), gfx->getPalette(), outputFileName, params);
        return;
    }
    // multiple frames
    if (amount == 1 || params.multi) {
        // one file for each frame (indexed)
        for (int i = frameFrom; i <= frameTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return;
            // }
            QString outputFileName = fileNameBase + "_frame" + QString("%1").arg(i, 4, 10, QChar('0'));
            saveImage(gfx->getFramePixelImage(i), gfx->getPalette(), outputFileName, params);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
        return;
    }
    // one file for all frames
    if (frameFrom != 0 || frameTo < count - 1) {
        fileNameBase += QString::number(frameFrom + 1) + "_" + QString::number(frameTo + 1);
    }
    QString &outputFileName = fileNameBase;

    int tempOutputImageWidth = 0;
    int tempOutputImageHeight = 0;

    if (params.placement == 0) { // grouped
        if (gfx->getType() == D1CEL_TYPE::V1_LEVEL) {
            // artifical grouping of a tileset
            int groupImageWidth = 0;
            int groupImageHeight = 0;
            for (int i = frameFrom; i <= frameTo; i++) {
                if (((i - frameFrom) % EXPORT_LVLFRAMES_PER_LINE) == 0) {
                    tempOutputImageWidth = std::max(groupImageWidth, tempOutputImageWidth);
                    tempOutputImageHeight += groupImageHeight;
                    groupImageWidth = 0;
                    groupImageHeight = 0;
                }
                groupImageWidth += gfx->getFrameWidth(i);
                groupImageHeight = std::max(gfx->getFrameHeight(i), groupImageHeight);
            }
            tempOutputImageWidth = std::max(groupImageWidth, tempOutputImageWidth);
            tempOutputImageHeight += groupImageHeight;
        } else {
            for (int i = 0; i < gfx->getGroupCount(); i++) {
                int groupImageWidth = 0;
                int groupImageHeight = 0;
                for (int j = gfx->getGroupFrameIndices(i).first;
                     j <= gfx->getGroupFrameIndices(i).second; j++) {
                    if (j < frameFrom || j > frameTo) {
                        continue;
                    }
                    groupImageWidth += gfx->getFrameWidth(j);
                    groupImageHeight = std::max(gfx->getFrameHeight(j), groupImageHeight);
                }
                tempOutputImageWidth = std::max(groupImageWidth, tempOutputImageWidth);
                tempOutputImageHeight += groupImageHeight;
            }
        }
    } else if (params.placement == 2) { // frames on one column
        for (int i = frameFrom; i <= frameTo; i++) {
            tempOutputImageWidth = std::max(gfx->getFrameWidth(i), tempOutputImageWidth);
            tempOutputImageHeight += gfx->getFrameHeight(i);
        }
    } else { // params.placement == 1 -- frames on one line
        for (int i = frameFrom; i <= frameTo; i++) {
            tempOutputImageWidth += gfx->getFrameWidth(i);
            tempOutputImageHeight = std::max(gfx->getFrameHeight(i), tempOutputImageHeight);
        }
    }

    std::vector<std::vector<D1GfxPixel>> tempOutputPixels;
    D1PixelImage::createImage(tempOutputPixels, tempOutputImageWidth, tempOutputImageHeight);

    if (params.placement == 0) { // grouped
        if (gfx->getType() == D1CEL_TYPE::V1_LEVEL) {
            // artifical grouping of a tileset
            int cursorY = 0;
            int cursorX = 0;
            int groupImageHeight = 0;
            for (int i = frameFrom; i <= frameTo; i++) {
                if (ProgressDialog::wasCanceled()) {
                    return;
                }

                if (((i - frameFrom) % EXPORT_LVLFRAMES_PER_LINE) == 0) {
                    cursorY += groupImageHeight;
                    cursorX = 0;
                    groupImageHeight = 0;
                }

                const std::vector<std::vector<D1GfxPixel>> pixels = gfx->getFramePixelImage(i);
                D1PixelImage::drawImage(tempOutputPixels, cursorX, cursorY, pixels);

                const QSize imageSize = D1PixelImage::getImageSize(pixels);
                cursorX += imageSize.width();
                groupImageHeight = std::max(imageSize.height(), groupImageHeight);
                if (!ProgressDialog::incValue()) {
                    return;
                }
            }
        } else {
            int cursorY = 0;
            for (int i = 0; i < gfx->getGroupCount(); i++) {
                int cursorX = 0;
                int groupImageHeight = 0;
                for (int j = gfx->getGroupFrameIndices(i).first;
                     j <= gfx->getGroupFrameIndices(i).second; j++) {
                    if (j < frameFrom || j > frameTo) {
                        continue;
                    }
                    if (ProgressDialog::wasCanceled()) {
                        return;
                    }
                    const std::vector<std::vector<D1GfxPixel>> pixels = gfx->getFramePixelImage(j);
                    D1PixelImage::drawImage(tempOutputPixels, cursorX, cursorY, pixels);

                    const QSize imageSize = D1PixelImage::getImageSize(pixels);
                    cursorX += imageSize.width();
                    groupImageHeight = std::max(imageSize.height(), groupImageHeight);
                    if (!ProgressDialog::incValue()) {
                        return;
                    }
                }
                cursorY += groupImageHeight;
            }
        }
    } else {
        int cursor = 0;
        for (int i = frameFrom; i <= frameTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            const std::vector<std::vector<D1GfxPixel>> pixels = gfx->getFramePixelImage(i);
            const QSize imageSize = D1PixelImage::getImageSize(pixels);
            if (params.placement == 2) { // frames on one column
                D1PixelImage::drawImage(tempOutputPixels, 0, cursor, pixels);
                cursor += imageSize.height();
            } else { // params.placement == 1 -- frames on one line
                D1PixelImage::drawImage(tempOutputPixels, cursor, 0, pixels);
                cursor += imageSize.width();
            }
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
    }

    saveImage(tempOutputPixels, gfx->getPalette(), outputFileName, params);
}

static void saveAudio(D1SmkAudioData *tempAudioData, const QString &fileName, const ExportParam &params)
{
    unsigned long len;
    for (int n = 0; n < D1SMK_TRACKS; n++) {
        if (tempAudioData->getAudio(n, &len) != nullptr) {
            QString path = params.outFolder + "/" + fileName + QApplication::tr("track%1").arg(n + 1) + params.outFileExtension;
            // if (path.endsWith(".wav")) {
                if (D1Wav::save(tempAudioData, n, path))
                    dProgress() << QApplication::tr("%1 created.").arg(QDir::toNativeSeparators(path));
                else
                    dProgressFail() << QApplication::tr("Failed to create %1.").arg(QDir::toNativeSeparators(path));
            //  return;
            // }
        }
    }
}

void ExportDialog::exportAudio(const D1Gfx *gfx, const ExportParam &params)
{
    QString fileNameBase = QFileInfo(gfx->getFilePath()).fileName();

    fileNameBase.replace(".", "_");

    int count = gfx->getFrameCount();
    int frameFrom = params.rangeFrom;
    if (frameFrom != 0) {
        frameFrom--;
    }
    int frameTo = params.rangeTo;
    if (frameTo == 0 || frameTo > count) {
        frameTo = count;
    }
    frameTo--;
    int amount = frameTo - frameFrom + 1;
    // nothing to export
    if (amount <= 0) {
        return;
    }
    ProgressDialog::incBar(tr("Exporting audio..."), amount);
    // single frame
    if (amount == 1 && frameFrom == 0) {
        // one file for the only frame (not indexed)
        QString &outputFileName = fileNameBase;
        saveAudio(gfx->getFrame(0)->getFrameAudio(), outputFileName, params);
        return;
    }
    // multiple frames
    if (amount == 1 || params.multi) {
        // one file for each frame (indexed)
        for (int i = frameFrom; i <= frameTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return;
            // }
            QString outputFileName = fileNameBase + "_frame" + QString("%1").arg(i, 4, 10, QChar('0'));
            saveAudio(gfx->getFrame(i)->getFrameAudio(), outputFileName, params);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
        return;
    }
    // one file for all frames
    D1SmkAudioData *tempAudioData = nullptr;
    unsigned long lengths[D1SMK_TRACKS] = { 0 };
    for (int i = frameFrom; i <= frameTo; i++) {
        D1SmkAudioData *frameAudio = gfx->getFrame(i)->getFrameAudio();
        if (tempAudioData != nullptr) {
            if (frameAudio->getChannels() != tempAudioData->getChannels() || frameAudio->getBitDepth() != tempAudioData->getBitDepth() || frameAudio->getBitRate() != tempAudioData->getBitRate()) {
                delete tempAudioData;
                dProgressFail() << QApplication::tr("Mismatching audio formats (ch%1,w%2,%3 vs. ch%1,w%2,%3).").arg(frameAudio->getChannels()).arg(frameAudio->getBitDepth()).arg(frameAudio->getBitRate()).arg(tempAudioData->getChannels()).arg(tempAudioData->getBitDepth()).arg(tempAudioData->getBitRate());
                return;
            }
        } else {
            tempAudioData = new D1SmkAudioData(frameAudio->getChannels(), frameAudio->getBitDepth(), frameAudio->getBitRate());
        }
        for (int n = 0; n < D1SMK_TRACKS; n++) {
            unsigned long len;
            frameAudio->getAudio(n, &len);
            lengths[n] += len;
        }
    }

    for (int n = 0; n < D1SMK_TRACKS; n++) {
        if (lengths[n] == 0) {
            continue;
        }
        if (ProgressDialog::wasCanceled()) {
            delete tempAudioData;
            return;
        }
        uint8_t* audioMem = (uint8_t*)malloc(lengths[n]);
        unsigned long cursor = 0;
        for (int i = frameFrom; i <= frameTo; i++) {
            D1SmkAudioData *frameAudio = gfx->getFrame(i)->getFrameAudio();
            unsigned long len;
            uint8_t* trackAudio = frameAudio->getAudio(n, &len);
            if (len != 0) {
                memcpy(&audioMem[cursor], trackAudio, len);
                cursor += len;
            }
        }
        tempAudioData->setAudio(n, audioMem, cursor);
        if (!ProgressDialog::incValue()) {
            delete tempAudioData;
            return;
        }
    }

    if (frameFrom != 0 || frameTo < count - 1) {
        fileNameBase += QString::number(frameFrom + 1) + "_" + QString::number(frameTo + 1);
    }
    QString &outputFileName = fileNameBase;
    saveAudio(tempAudioData, outputFileName, params);

    delete tempAudioData;
}

void ExportDialog::on_exportButton_clicked()
{
    ExportParam params;
    params.outFolder = this->ui->outputFolderEdit->text();
    if (params.outFolder.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Output folder is missing, please choose an output folder."));
        return;
    }
    params.outFileExtension = "." + this->ui->formatComboBox->currentText().toLower();
    QString tpIndexStr = this->ui->contentCanvasIndexLineEdit->text();
    int tpIndex = D1PAL_COLORS;
    if (!tpIndexStr.isEmpty()) {
        tpIndex = tpIndexStr.toUShort();
    }
    params.transparentIndex = tpIndex;
    params.transparentColor = QColor(this->ui->contentCanvasColorLineEdit->text());
    if (!params.transparentColor.isValid()) {
        params.transparentColor = QColor(Qt::transparent);
    }
    params.rangeFrom = this->ui->contentRangeFromEdit->nonNegInt();
    params.rangeTo = this->ui->contentRangeToEdit->nonNegInt();
    params.placement = this->ui->contentPlacementComboBox->currentIndex();
    params.multi = this->ui->filesCountComboBox->currentIndex() != 0;
    int type = this->ui->contentTypeComboBox->currentIndex();
    if (this->ui->contentTypeComboBox->currentText() == "Audio") {
        type = 4;
    }

    this->hide();

    std::function<void()> func = [type, this, params]() {
        switch (type) {
        case 0:
            ExportDialog::exportFrames(this->gfx, params);
            break;
        case 1:
            ExportDialog::exportLevelSubtiles(this->tileset->min, this->gfx, params);
            break;
        case 2:
            ExportDialog::exportLevelTiles(this->tileset->til, this->gfx, params);
            break;
        case 3:
            ExportDialog::exportLevelTiles25D(this->tileset->til, this->gfx, params);
            break;
        default: // case 4:
            ExportDialog::exportAudio(this->gfx, params);
            break;
        }
    };
    ProgressDialog::startAsync(PROGRESS_DIALOG_STATE::ACTIVE, tr("Export"), 1, PAF_NONE, std::move(func));
}

void ExportDialog::on_exportCancelButton_clicked()
{
    this->close();
}

void ExportDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        this->ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}
