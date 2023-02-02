#include "exportdialog.h"

#include <QApplication>
#include <QFileDialog>
#include <QImageWriter>
#include <QMessageBox>
#include <QPainter>
#include <algorithm>

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

void ExportDialog::initialize(D1Gfx *g, D1Min *m, D1Til *t, D1Sol *s, D1Amp *a)
{
    this->gfx = g;
    this->min = m;
    this->til = t;
    this->sol = s;
    this->amp = a;

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
    bool isTileset = this->gfx->getType() == D1CEL_TYPE::V1_LEVEL;
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

QString ExportDialog::getFileFormatExtension()
{
    return "." + this->ui->formatComboBox->currentText().toLower();
}

void ExportDialog::on_outputFolderBrowseButton_clicked()
{
    QString selectedDirectory = QFileDialog::getExistingDirectory(
        this, tr("Select Output Folder"), QString(), QFileDialog::ShowDirsOnly);

    if (selectedDirectory.isEmpty())
        return;

    ui->outputFolderEdit->setText(selectedDirectory);
}

static void saveImage(const QImage &image, const QString &path)
{
    image.save(path);
    dProgress() << QApplication::tr("%1 created.").arg(QDir::toNativeSeparators(path));
}

bool ExportDialog::exportLevelTiles25D(const QString &outFolder)
{
    QString fileName = QFileInfo(this->til->getFilePath()).fileName();

    QString outputFilePathBase = outFolder + "/" + fileName.replace(".", "_25d_");

    int count = this->til->getTileCount();
    int tileFrom = this->ui->contentRangeFromEdit->text().toUInt();
    if (tileFrom != 0) {
        tileFrom--;
    }
    int tileTo = this->ui->contentRangeToEdit->text().toUInt();
    if (tileTo == 0 || tileTo > count) {
        tileTo = count;
    }
    tileTo--;
    int amount = tileTo - tileFrom + 1;
    // nothing to export
    if (amount == 0) {
        return true;
    }
    ProgressDialog::incBar(tr("Exporting 2.5d tiles..."), amount);
    // single tile
    if (amount == 1 && tileFrom == 0) {
        // one file for the only tile (not indexed)
        QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
        saveImage(this->til->getTileImage(0), outputFilePath);
        return true;
    }

    // multiple tiles
    if (amount == 1 || this->ui->filesCountComboBox->currentIndex() != 0) {
        // one file for each tile (indexed)
        for (int i = tileFrom; i <= tileTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return false;
            // }

            QString outputFilePath = outputFilePathBase
                + QString("%1").arg(i, 4, 10, QChar('0')) + this->getFileFormatExtension();

            saveImage(this->til->getTileImage(i), outputFilePath);
            if (!ProgressDialog::incValue()) {
                return false;
            }
        }
        return true;
    }
    // one file for all tiles
    if (tileFrom != 0 || tileTo < count - 1) {
        outputFilePathBase += QString::number(tileFrom + 1) + "_" + QString::number(tileTo + 1);
    }
    QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();

    unsigned tileWidth = this->min->getSubtileWidth() * 2 * MICRO_WIDTH;
    unsigned tileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT + 32;

    constexpr unsigned TILES_PER_LINE = 8;
    QImage tempOutputImage;
    unsigned tempOutputImageWidth = 0;
    unsigned tempOutputImageHeight = 0;
    int placement = this->ui->contentPlacementComboBox->currentIndex();
    if (placement == 0) { // grouped
        tempOutputImageWidth = tileWidth * TILES_PER_LINE;
        tempOutputImageHeight = tileHeight * ((amount + (TILES_PER_LINE - 1)) / TILES_PER_LINE);
    } else if (placement == 2) { // tiles on one column
        tempOutputImageWidth = tileWidth;
        tempOutputImageHeight = tileHeight * amount;
    } else { // placement == 1 -- tiles on one line
        tempOutputImageWidth = tileWidth * amount;
        tempOutputImageHeight = tileHeight;
    }

    tempOutputImage = QImage(tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32);
    tempOutputImage.fill(Qt::transparent);

    QPainter painter(&tempOutputImage);

    if (placement == 0) { // grouped
        unsigned dx = 0, dy = 0;
        for (int i = tileFrom; i <= tileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return false;
            }

            const QImage image = this->til->getTileImage(i);

            painter.drawImage(dx, dy, image);

            dx += image.width();
            if (dx >= tempOutputImageWidth) {
                dx = 0;
                dy += image.height();
            }
            if (!ProgressDialog::incValue()) {
                return false;
            }
        }
    } else {
        int cursor = 0;
        for (int i = tileFrom; i <= tileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return false;
            }
            const QImage image = this->til->getTileImage(i);
            if (placement == 2) { // tiles on one column
                painter.drawImage(0, cursor, image);
                cursor += image.height();
            } else { // placement == 1 -- tiles on one line
                painter.drawImage(cursor, 0, image);
                cursor += image.width();
            }
            if (!ProgressDialog::incValue()) {
                return false;
            }
        }
    }

    painter.end();

    saveImage(tempOutputImage, outputFilePath);
    return true;
}

bool ExportDialog::exportLevelTiles(const QString &outFolder)
{
    QString fileName = QFileInfo(this->til->getFilePath()).fileName();

    QString outputFilePathBase = outFolder + "/" + fileName.replace(".", "_flat_");

    int count = this->til->getTileCount();
    int tileFrom = this->ui->contentRangeFromEdit->text().toUInt();
    if (tileFrom != 0) {
        tileFrom--;
    }
    int tileTo = this->ui->contentRangeToEdit->text().toUInt();
    if (tileTo == 0 || tileTo > count) {
        tileTo = count;
    }
    tileTo--;
    int amount = tileTo - tileFrom + 1;
    // nothing to export
    if (amount <= 0) {
        return true;
    }
    ProgressDialog::incBar(tr("Exporting flat tiles...").arg(fileName), amount);
    // single tile
    if (amount == 1 && tileFrom == 0) {
        // one file for the only tile (not indexed)
        QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
        saveImage(this->til->getFlatTileImage(0), outputFilePath);
        return true;
    }
    // multiple tiles
    if (amount == 1 || this->ui->filesCountComboBox->currentIndex() != 0) {
        // one file for each tile (indexed)
        for (int i = tileFrom; i <= tileTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return false;
            // }
            QString outputFilePath = outputFilePathBase
                + QString("%1").arg(i, 4, 10, QChar('0')) + this->getFileFormatExtension();

            saveImage(this->til->getFlatTileImage(i), outputFilePath);
            if (!ProgressDialog::incValue()) {
                return false;
            }
        }
        return true;
    }
    // one file for all tiles
    if (tileFrom != 0 || tileTo < count - 1) {
        outputFilePathBase += QString::number(tileFrom + 1) + "_" + QString::number(tileTo + 1);
    }
    QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();

    unsigned tileWidth = this->min->getSubtileWidth() * MICRO_WIDTH * TILE_WIDTH * TILE_HEIGHT;
    unsigned tileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    constexpr unsigned TILES_PER_LINE = 4;
    QImage tempOutputImage;
    unsigned tempOutputImageWidth = 0;
    unsigned tempOutputImageHeight = 0;
    int placement = this->ui->contentPlacementComboBox->currentIndex();
    if (placement == 0) { // grouped
        tempOutputImageWidth = tileWidth * TILES_PER_LINE;
        tempOutputImageHeight = tileHeight * ((amount + (TILES_PER_LINE - 1)) / TILES_PER_LINE);
    } else if (placement == 2) { // tiles on one column
        tempOutputImageWidth = tileWidth;
        tempOutputImageHeight = tileHeight * amount;
    } else { // placement == 1 -- tiles on one line
        tempOutputImageWidth = tileWidth * amount;
        tempOutputImageHeight = tileHeight;
    }

    tempOutputImage = QImage(tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32);
    tempOutputImage.fill(Qt::transparent);

    QPainter painter(&tempOutputImage);

    if (placement == 0) { // grouped
        unsigned dx = 0, dy = 0;
        for (int i = tileFrom; i <= tileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return false;
            }

            const QImage image = this->til->getFlatTileImage(i);

            painter.drawImage(dx, dy, image);

            dx += image.width();
            if (dx >= tempOutputImageWidth) {
                dx = 0;
                dy += image.height();
            }
            if (!ProgressDialog::incValue()) {
                return false;
            }
        }
    } else {
        int cursor = 0;
        for (int i = tileFrom; i <= tileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return false;
            }
            const QImage image = this->til->getFlatTileImage(i);
            if (placement == 2) { // tiles on one column
                painter.drawImage(0, cursor, image);
                cursor += image.height();
            } else { // placement == 1 -- tiles on one line
                painter.drawImage(cursor, 0, image);
                cursor += image.width();
            }
            if (!ProgressDialog::incValue()) {
                return false;
            }
        }
    }

    painter.end();

    saveImage(tempOutputImage, outputFilePath);
    return true;
}

bool ExportDialog::exportLevelSubtiles(const QString &outFolder)
{
    QString fileName = QFileInfo(this->min->getFilePath()).fileName();

    QString outputFilePathBase = outFolder + "/" + fileName.replace(".", "_");

    int count = this->min->getSubtileCount();
    int subtileFrom = this->ui->contentRangeFromEdit->text().toUInt();
    if (subtileFrom != 0) {
        subtileFrom--;
    }
    int subtileTo = this->ui->contentRangeToEdit->text().toUInt();
    if (subtileTo == 0 || subtileTo > count) {
        subtileTo = count;
    }
    subtileTo--;
    int amount = subtileTo - subtileFrom + 1;
    // nothing to export
    if (amount <= 0) {
        return true;
    }
    ProgressDialog::incBar(tr("Exporting subtiles..."), amount);
    // single subtile
    if (amount == 1 && subtileFrom == 0) {
        // one file for the only subtile (not indexed)
        QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
        saveImage(this->min->getSubtileImage(0), outputFilePath);
        return true;
    }
    // multiple subtiles
    if (amount == 1 || this->ui->filesCountComboBox->currentIndex() != 0) {
        // one file for each subtile (indexed)
        for (int i = subtileFrom; i <= subtileTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return false;
            // }
            QString outputFilePath = outputFilePathBase + "_subtile"
                + QString("%1").arg(i, 4, 10, QChar('0')) + this->getFileFormatExtension();

            saveImage(this->min->getSubtileImage(i), outputFilePath);
            if (!ProgressDialog::incValue()) {
                return false;
            }
        }
        return true;
    }
    // one file for all subtiles
    if (subtileFrom != 0 || subtileTo < count - 1) {
        outputFilePathBase += QString::number(subtileFrom + 1) + "_" + QString::number(subtileTo + 1);
    }
    QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();

    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    QImage tempOutputImage;
    unsigned tempOutputImageWidth = 0;
    unsigned tempOutputImageHeight = 0;
    int placement = this->ui->contentPlacementComboBox->currentIndex();
    if (placement == 0) { // grouped
        tempOutputImageWidth = subtileWidth * EXPORT_SUBTILES_PER_LINE;
        tempOutputImageHeight = subtileHeight * ((amount + (EXPORT_SUBTILES_PER_LINE - 1)) / EXPORT_SUBTILES_PER_LINE);
    } else if (placement == 2) { // subtiles on one column
        tempOutputImageWidth = subtileWidth;
        tempOutputImageHeight = subtileHeight * amount;
    } else { // placement == 1 -- subtiles on one line
        tempOutputImageWidth = subtileWidth * amount;
        tempOutputImageHeight = subtileHeight;
        if ((amount % (TILE_WIDTH * TILE_HEIGHT)) == 0) {
            tempOutputImageWidth += subtileWidth; // add an extra subtile to ensure it is not recognized as a flat tile
        }
    }

    tempOutputImage = QImage(tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32);
    tempOutputImage.fill(Qt::transparent);

    QPainter painter(&tempOutputImage);

    if (placement == 0) { // grouped
        unsigned dx = 0, dy = 0;
        for (int i = subtileFrom; i <= subtileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return false;
            }
            const QImage image = this->min->getSubtileImage(i);

            painter.drawImage(dx, dy, image);

            dx += image.width();
            if (dx >= tempOutputImageWidth) {
                dx = 0;
                dy += image.height();
            }
            if (!ProgressDialog::incValue()) {
                return false;
            }
        }
    } else {
        int cursor = 0;
        for (int i = subtileFrom; i <= subtileTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return false;
            }
            const QImage image = this->min->getSubtileImage(i);
            if (placement == 2) { // subtiles on one column
                painter.drawImage(0, cursor, image);
                cursor += image.height();
            } else { // placement == 1 -- subtiles on one line
                painter.drawImage(cursor, 0, image);
                cursor += image.width();
            }
            if (!ProgressDialog::incValue()) {
                return false;
            }
        }
    }

    painter.end();

    saveImage(tempOutputImage, outputFilePath);
    return true;
}

bool ExportDialog::exportFrames(const QString &outFolder)
{
    QString fileName = QFileInfo(this->gfx->getFilePath()).fileName();

    QString outputFilePathBase = outFolder + "/" + fileName.replace(".", "_");

    int count = this->gfx->getFrameCount();
    int frameFrom = this->ui->contentRangeFromEdit->text().toUInt();
    if (frameFrom != 0) {
        frameFrom--;
    }
    int frameTo = this->ui->contentRangeToEdit->text().toUInt();
    if (frameTo == 0 || frameTo > count) {
        frameTo = count;
    }
    frameTo--;
    int amount = frameTo - frameFrom + 1;
    // nothing to export
    if (amount <= 0) {
        return true;
    }
    ProgressDialog::incBar(tr("Exporting frames..."), amount);
    // single frame
    if (amount == 1 && frameFrom == 0) {
        // one file for the only frame (not indexed)
        QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
        saveImage(this->gfx->getFrameImage(0), outputFilePath);
        return true;
    }
    // multiple frames
    if (amount == 1 || this->ui->filesCountComboBox->currentIndex() != 0) {
        // one file for each frame (indexed)
        for (int i = frameFrom; i <= frameTo; i++) {
            // if (ProgressDialog::wasCanceled()) {
            //    return false;
            // }
            QString outputFilePath = outputFilePathBase + "_frame"
                + QString("%1").arg(i, 4, 10, QChar('0')) + this->getFileFormatExtension();

            saveImage(this->gfx->getFrameImage(i), outputFilePath);
            if (!ProgressDialog::incValue()) {
                return false;
            }
        }
        return true;
    }
    // one file for all frames
    if (frameFrom != 0 || frameTo < count - 1) {
        outputFilePathBase += QString::number(frameFrom + 1) + "_" + QString::number(frameTo + 1);
    }
    QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();

    QImage tempOutputImage;
    int tempOutputImageWidth = 0;
    int tempOutputImageHeight = 0;

    int placement = this->ui->contentPlacementComboBox->currentIndex();
    if (placement == 0) { // grouped
        if (this->gfx->getType() == D1CEL_TYPE::V1_LEVEL) {
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
                groupImageWidth += this->gfx->getFrameWidth(i);
                groupImageHeight = std::max(this->gfx->getFrameHeight(i), groupImageHeight);
            }
            tempOutputImageWidth = std::max(groupImageWidth, tempOutputImageWidth);
            tempOutputImageHeight += groupImageHeight;
        } else {
            for (int i = 0; i < this->gfx->getGroupCount(); i++) {
                int groupImageWidth = 0;
                int groupImageHeight = 0;
                for (unsigned int j = this->gfx->getGroupFrameIndices(i).first;
                     j <= this->gfx->getGroupFrameIndices(i).second; j++) {
                    if (j < (unsigned)frameFrom || j > (unsigned)frameTo) {
                        continue;
                    }
                    groupImageWidth += this->gfx->getFrameWidth(j);
                    groupImageHeight = std::max(this->gfx->getFrameHeight(j), groupImageHeight);
                }
                tempOutputImageWidth = std::max(groupImageWidth, tempOutputImageWidth);
                tempOutputImageHeight += groupImageHeight;
            }
        }
    } else if (placement == 2) { // frames on one column
        for (int i = frameFrom; i <= frameTo; i++) {
            tempOutputImageWidth = std::max(this->gfx->getFrameWidth(i), tempOutputImageWidth);
            tempOutputImageHeight += this->gfx->getFrameHeight(i);
        }
    } else { // placement == 1 -- frames on one line
        for (int i = frameFrom; i <= frameTo; i++) {
            tempOutputImageWidth += this->gfx->getFrameWidth(i);
            tempOutputImageHeight = std::max(this->gfx->getFrameHeight(i), tempOutputImageHeight);
        }
    }
    tempOutputImage = QImage(tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32);
    tempOutputImage.fill(Qt::transparent);

    QPainter painter(&tempOutputImage);

    if (placement == 0) { // grouped
        if (this->gfx->getType() == D1CEL_TYPE::V1_LEVEL) {
            // artifical grouping of a tileset
            int cursorY = 0;
            int cursorX = 0;
            int groupImageHeight = 0;
            for (int i = frameFrom; i <= frameTo; i++) {
                if (ProgressDialog::wasCanceled()) {
                    return false;
                }

                if (((i - frameFrom) % EXPORT_LVLFRAMES_PER_LINE) == 0) {
                    cursorY += groupImageHeight;
                    cursorX = 0;
                    groupImageHeight = 0;
                }

                const QImage image = this->gfx->getFrameImage(i);
                painter.drawImage(cursorX, cursorY, image);

                cursorX += image.width();
                groupImageHeight = std::max(image.height(), groupImageHeight);
                if (!ProgressDialog::incValue()) {
                    return false;
                }
            }
        } else {
            int cursorY = 0;
            for (int i = 0; i < this->gfx->getGroupCount(); i++) {
                int cursorX = 0;
                int groupImageHeight = 0;
                for (unsigned int j = this->gfx->getGroupFrameIndices(i).first;
                     j <= this->gfx->getGroupFrameIndices(i).second; j++) {
                    if (j < (unsigned)frameFrom || j > (unsigned)frameTo) {
                        continue;
                    }
                    if (ProgressDialog::wasCanceled()) {
                        return false;
                    }
                    const QImage image = this->gfx->getFrameImage(j);
                    painter.drawImage(cursorX, cursorY, image);
                    cursorX += image.width();
                    groupImageHeight = std::max(image.height(), groupImageHeight);
                    if (!ProgressDialog::incValue()) {
                        return false;
                    }
                }
                cursorY += groupImageHeight;
            }
        }
    } else {
        int cursor = 0;
        for (int i = frameFrom; i <= frameTo; i++) {
            if (ProgressDialog::wasCanceled()) {
                return false;
            }
            const QImage image = this->gfx->getFrameImage(i);
            if (placement == 2) { // frames on one column
                painter.drawImage(0, cursor, image);
                cursor += image.height();
            } else { // placement == 1 -- frames on one line
                painter.drawImage(cursor, 0, image);
                cursor += image.width();
            }
            if (!ProgressDialog::incValue()) {
                return false;
            }
        }
    }

    painter.end();

    saveImage(tempOutputImage, outputFilePath);
    return true;
}

void ExportDialog::on_exportButton_clicked()
{
    QString outFolder = this->ui->outputFolderEdit->text();
    if (outFolder.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Output folder is missing, please choose an output folder."));
        return;
    }
    int type = this->ui->contentTypeComboBox->currentIndex();
    this->hide();
    ProgressDialog::start(PROGRESS_DIALOG_STATE::ACTIVE, tr("Export"), 1);
    bool result;
    switch (type) {
    case 0:
        result = this->exportFrames(outFolder);
        break;
    case 1:
        result = this->exportLevelSubtiles(outFolder);
        break;
    case 2:
        result = this->exportLevelTiles(outFolder);
        break;
    default: // case 3:
        result = this->exportLevelTiles25D(outFolder);
        break;
    }
    ProgressDialog::done();
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
