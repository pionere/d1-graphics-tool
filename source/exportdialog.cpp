#include "exportdialog.h"

#include <algorithm>

#include <QApplication>
#include <QFileDialog>
#include <QImageWriter>
#include <QMessageBox>
#include <QPainter>
#include <QtConcurrent>

#include "mainwindow.h"
#include "progressdialog.h"
#include "ui_exportdialog.h"

ExportDialog::ExportDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ExportDialog())
{
    ui->setupUi(this);
}

ExportDialog::~ExportDialog()
{
    delete ui;
}

void ExportDialog::initialize(D1Gfx *g, D1Tileset *ts)
{
    this->gfx = g;
    this->tileset = ts;

    // initialize the format combobox
    QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    QStringList formatTxts;
    for (QByteArray &format : formats) {
        QString fmt = format;
        formatTxts.append(fmt.toUpper());
    }
    // - remember the last selected format
    QComboBox *fmtBox = this->ui->formatComboBox;
    QString lastFmt = fmtBox->currentText();
    if (lastFmt.isEmpty()) {
        lastFmt = "PNG";
    }
    fmtBox->clear();
    fmtBox->addItems(formatTxts);
    fmtBox->setCurrentIndex(fmtBox->findText(lastFmt));

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
}

void ExportDialog::on_outputFolderBrowseButton_clicked()
{
    QString dirPath = dMainWindow().folderDialog(tr("Select Output Folder"));

    if (selectedDirectory.isEmpty())
        return;

    this->ui->outputFolderEdit->setText(selectedDirectory);
}

static void saveImage(const QImage &image, const QString &path)
{
    if (image.save(path))
        dProgress() << QApplication::tr("%1 created.").arg(QDir::toNativeSeparators(path));
    else
        dProgressFail() << QApplication::tr("Failed to create %1.").arg(QDir::toNativeSeparators(path));
}

void ExportDialog::exportLevelTiles25D(const D1Til *til, const ExportParam &params)
{
    QString fileName = QFileInfo(til->getFilePath()).fileName();

    QString outputFilePathBase = params.outFolder + "/" + fileName.replace(".", "_25d_");

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
    if (amount == 1 && tileFrom == 0) {
        // one file for the only tile (not indexed)
        QString outputFilePath = outputFilePathBase + params.outFileExtension;
        saveImage(til->getTileImage(0), outputFilePath);
        return;
    }

    // multiple tiles
    if (amount == 1 || params.multi) {
        // one file for each tile (indexed)
        for (int i = tileFrom; i <= tileTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return;
            // }

            QString outputFilePath = outputFilePathBase
                + QString("%1").arg(i, 4, 10, QChar('0')) + params.outFileExtension;

            saveImage(til->getTileImage(i), outputFilePath);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
        return;
    }
    // one file for all tiles
    if (tileFrom != 0 || tileTo < count - 1) {
        outputFilePathBase += QString::number(tileFrom + 1) + "_" + QString::number(tileTo + 1);
    }
    QString outputFilePath = outputFilePathBase + params.outFileExtension;

    unsigned tileWidth = til->getMin()->getSubtileWidth() * 2 * MICRO_WIDTH;
    unsigned tileHeight = til->getMin()->getSubtileHeight() * MICRO_HEIGHT + 32;

    constexpr unsigned TILES_PER_LINE = 8;
    QImage tempOutputImage;
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

    tempOutputImage = QImage(tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32);
    if (tempOutputImage.isNull()) {
        dProgressFail() << QApplication::tr("Failed to create image with (%1:%2) dimensions.").arg(tempOutputImageWidth).arg(tempOutputImageHeight);
        return;
    }
    tempOutputImage.fill(Qt::transparent);

    QPainter painter(&tempOutputImage);

    if (params.placement == 0) { // grouped
        unsigned dx = 0, dy = 0;
        for (int i = tileFrom; i <= tileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }

            const QImage image = til->getTileImage(i);

            painter.drawImage(dx, dy, image);

            dx += image.width();
            if (dx >= tempOutputImageWidth) {
                dx = 0;
                dy += image.height();
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
            const QImage image = til->getTileImage(i);
            if (params.placement == 2) { // tiles on one column
                painter.drawImage(0, cursor, image);
                cursor += image.height();
            } else { // params.placement == 1 -- tiles on one line
                painter.drawImage(cursor, 0, image);
                cursor += image.width();
            }
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
    }

    painter.end();

    saveImage(tempOutputImage, outputFilePath);
}

void ExportDialog::exportLevelTiles(const D1Til *til, const ExportParam &params)
{
    QString fileName = QFileInfo(til->getFilePath()).fileName();

    QString outputFilePathBase = params.outFolder + "/" + fileName.replace(".", "_flat_");

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
    ProgressDialog::incBar(tr("Exporting flat tiles...").arg(fileName), amount);
    // single tile
    if (amount == 1 && tileFrom == 0) {
        // one file for the only tile (not indexed)
        QString outputFilePath = outputFilePathBase + params.outFileExtension;
        saveImage(til->getFlatTileImage(0), outputFilePath);
        return;
    }
    // multiple tiles
    if (amount == 1 || params.multi) {
        // one file for each tile (indexed)
        for (int i = tileFrom; i <= tileTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return;
            // }
            QString outputFilePath = outputFilePathBase
                + QString("%1").arg(i, 4, 10, QChar('0')) + params.outFileExtension;

            saveImage(til->getFlatTileImage(i), outputFilePath);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
        return;
    }
    // one file for all tiles
    if (tileFrom != 0 || tileTo < count - 1) {
        outputFilePathBase += QString::number(tileFrom + 1) + "_" + QString::number(tileTo + 1);
    }
    QString outputFilePath = outputFilePathBase + params.outFileExtension;

    unsigned tileWidth = til->getMin()->getSubtileWidth() * MICRO_WIDTH * TILE_WIDTH * TILE_HEIGHT;
    unsigned tileHeight = til->getMin()->getSubtileHeight() * MICRO_HEIGHT;

    constexpr unsigned TILES_PER_LINE = 4;
    QImage tempOutputImage;
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

    tempOutputImage = QImage(tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32);
    if (tempOutputImage.isNull()) {
        dProgressFail() << QApplication::tr("Failed to create image with (%1:%2) dimensions.").arg(tempOutputImageWidth).arg(tempOutputImageHeight);
        return;
    }
    tempOutputImage.fill(Qt::transparent);

    QPainter painter(&tempOutputImage);

    if (params.placement == 0) { // grouped
        unsigned dx = 0, dy = 0;
        for (int i = tileFrom; i <= tileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }

            const QImage image = til->getFlatTileImage(i);

            painter.drawImage(dx, dy, image);

            dx += image.width();
            if (dx >= tempOutputImageWidth) {
                dx = 0;
                dy += image.height();
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
            const QImage image = til->getFlatTileImage(i);
            if (params.placement == 2) { // tiles on one column
                painter.drawImage(0, cursor, image);
                cursor += image.height();
            } else { // params.placement == 1 -- tiles on one line
                painter.drawImage(cursor, 0, image);
                cursor += image.width();
            }
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
    }

    painter.end();

    saveImage(tempOutputImage, outputFilePath);
}

void ExportDialog::exportLevelSubtiles(const D1Min *min, const ExportParam &params)
{
    QString fileName = QFileInfo(min->getFilePath()).fileName();

    QString outputFilePathBase = params.outFolder + "/" + fileName.replace(".", "_");

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
    if (amount == 1 && subtileFrom == 0) {
        // one file for the only subtile (not indexed)
        QString outputFilePath = outputFilePathBase + params.outFileExtension;
        saveImage(min->getSubtileImage(0), outputFilePath);
        return;
    }
    // multiple subtiles
    if (amount == 1 || params.multi) {
        // one file for each subtile (indexed)
        for (int i = subtileFrom; i <= subtileTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return;
            // }
            QString outputFilePath = outputFilePathBase + "_subtile"
                + QString("%1").arg(i, 4, 10, QChar('0')) + params.outFileExtension;

            saveImage(min->getSubtileImage(i), outputFilePath);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
        return;
    }
    // one file for all subtiles
    if (subtileFrom != 0 || subtileTo < count - 1) {
        outputFilePathBase += QString::number(subtileFrom + 1) + "_" + QString::number(subtileTo + 1);
    }
    QString outputFilePath = outputFilePathBase + params.outFileExtension;

    unsigned subtileWidth = min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = min->getSubtileHeight() * MICRO_HEIGHT;

    QImage tempOutputImage;
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

    tempOutputImage = QImage(tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32);
    if (tempOutputImage.isNull()) {
        dProgressFail() << QApplication::tr("Failed to create image with (%1:%2) dimensions.").arg(tempOutputImageWidth).arg(tempOutputImageHeight);
        return;
    }
    tempOutputImage.fill(Qt::transparent);

    QPainter painter(&tempOutputImage);

    if (params.placement == 0) { // grouped
        unsigned dx = 0, dy = 0;
        for (int i = subtileFrom; i <= subtileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return;
            }
            const QImage image = min->getSubtileImage(i);

            painter.drawImage(dx, dy, image);

            dx += image.width();
            if (dx >= tempOutputImageWidth) {
                dx = 0;
                dy += image.height();
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
            const QImage image = min->getSubtileImage(i);
            if (params.placement == 2) { // subtiles on one column
                painter.drawImage(0, cursor, image);
                cursor += image.height();
            } else { // params.placement == 1 -- subtiles on one line
                painter.drawImage(cursor, 0, image);
                cursor += image.width();
            }
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
    }

    painter.end();

    saveImage(tempOutputImage, outputFilePath);
}

void ExportDialog::exportFrames(const D1Gfx *gfx, const ExportParam &params)
{
    QString fileName = QFileInfo(gfx->getFilePath()).fileName();

    QString outputFilePathBase = params.outFolder + "/" + fileName.replace(".", "_");

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
        QString outputFilePath = outputFilePathBase + params.outFileExtension;
        saveImage(gfx->getFrameImage(0), outputFilePath);
        return;
    }
    // multiple frames
    if (amount == 1 || params.multi) {
        // one file for each frame (indexed)
        for (int i = frameFrom; i <= frameTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return;
            // }
            QString outputFilePath = outputFilePathBase + "_frame"
                + QString("%1").arg(i, 4, 10, QChar('0')) + params.outFileExtension;

            saveImage(gfx->getFrameImage(i), outputFilePath);
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
        return;
    }
    // one file for all frames
    if (frameFrom != 0 || frameTo < count - 1) {
        outputFilePathBase += QString::number(frameFrom + 1) + "_" + QString::number(frameTo + 1);
    }
    QString outputFilePath = outputFilePathBase + params.outFileExtension;

    QImage tempOutputImage;
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
    tempOutputImage = QImage(tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32);
    if (tempOutputImage.isNull()) {
        dProgressFail() << QApplication::tr("Failed to create image with (%1:%2) dimensions.").arg(tempOutputImageWidth).arg(tempOutputImageHeight);
        return;
    }
    tempOutputImage.fill(Qt::transparent);

    QPainter painter(&tempOutputImage);

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

                const QImage image = gfx->getFrameImage(i);
                painter.drawImage(cursorX, cursorY, image);

                cursorX += image.width();
                groupImageHeight = std::max(image.height(), groupImageHeight);
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
                    const QImage image = gfx->getFrameImage(j);
                    painter.drawImage(cursorX, cursorY, image);
                    cursorX += image.width();
                    groupImageHeight = std::max(image.height(), groupImageHeight);
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
            const QImage image = gfx->getFrameImage(i);
            if (params.placement == 2) { // frames on one column
                painter.drawImage(0, cursor, image);
                cursor += image.height();
            } else { // params.placement == 1 -- frames on one line
                painter.drawImage(cursor, 0, image);
                cursor += image.width();
            }
            if (!ProgressDialog::incValue()) {
                return;
            }
        }
    }

    painter.end();

    saveImage(tempOutputImage, outputFilePath);
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
    params.rangeFrom = this->ui->contentRangeFromEdit->text().toUInt();
    params.rangeTo = this->ui->contentRangeToEdit->text().toUInt();
    params.placement = this->ui->contentPlacementComboBox->currentIndex();
    params.multi = this->ui->filesCountComboBox->currentIndex() != 0;
    int type = this->ui->contentTypeComboBox->currentIndex();

    this->hide();

    std::function<void()> func = [type, this, params]() {
        switch (type) {
        case 0:
            ExportDialog::exportFrames(this->gfx, params);
            break;
        case 1:
            ExportDialog::exportLevelSubtiles(this->tileset->min, params);
            break;
        case 2:
            ExportDialog::exportLevelTiles(this->tileset->til, params);
            break;
        default: // case 3:
            ExportDialog::exportLevelTiles25D(this->tileset->til, params);
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
