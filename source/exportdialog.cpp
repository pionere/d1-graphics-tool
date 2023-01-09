#include "exportdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <algorithm>

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

void ExportDialog::initialize(QJsonObject *cfg, D1Gfx *g, D1Min *m, D1Til *t, D1Sol *s, D1Amp *a)
{
    this->configuration = cfg;

    this->gfx = g;
    this->min = m;
    this->til = t;
    this->sol = s;
    this->amp = a;

    /*bool multiFrame = this->gfx->getFrameCount() > 1;
    // disable if there's only one frame
    ui->filesSettingWidget->setEnabled(multiFrame);

    // disable if there is only one frame or not all frames have the same width/height
    ui->spritesSettingsWidget->setEnabled(multiFrame && this->gfx->isFrameSizeConstant() && ui->oneFileForAllFramesRadioButton->isChecked());

    // If there's only one group
    if (this->gfx->getGroupCount() == 1) {
        ui->allFramesOnOneLineRadioButton->setChecked(true);
        ui->oneFrameGroupPerLineRadioButton->setEnabled(false);
    } else {
        ui->oneFrameGroupPerLineRadioButton->setChecked(true);
        ui->oneFrameGroupPerLineRadioButton->setEnabled(true);
    }

    // disable if not a CEL level file or data is missing
    ui->levelFramesSettingsWidget->setEnabled(this->gfx->getType() == D1CEL_TYPE::V1_LEVEL && this->min != nullptr && this->til != nullptr);*/

    // initialize the format combobox
    QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    QStringList formatTxts;
    for (QByteArray &format : formats) {
        QString fmt = format;
        formatTxts.append(fmt.toUpper());
    }
    // - remember the last selected format
    QComboBox &fmtBox = this->ui->formatComboBox;
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
    return "." + this->ui->formatComboBox->currentText()->toLower();
    /*if (ui->pngRadioButton->isChecked())
        return ".png";
    if (ui->bmpRadioButton->isChecked())
        return ".bmp";

    return QString();*/
}

void ExportDialog::on_outputFolderBrowseButton_clicked()
{
    QString selectedDirectory = QFileDialog::getExistingDirectory(
        this, "Select Output Folder", QString(), QFileDialog::ShowDirsOnly);

    if (selectedDirectory.isEmpty())
        return;

    ui->outputFolderEdit->setText(selectedDirectory);
}

bool ExportDialog::exportLevelTiles25D(QProgressDialog &progress)
{
    progress.setLabelText("Exporting " + QFileInfo(this->til->getFilePath()).fileName() + " level tiles...");

    QString outputFilePathBase = ui->outputFolderEdit->text() + "/"
        + QFileInfo(this->til->getFilePath()).fileName().replace(".", "_");

    int n = this->til->getTileCount();
    // nothing to export
    if (n == 0) {
        return true;
    }
    // single tile
    if (n == 1) {
        // one file for the only tile (not indexed)
        QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
        this->til->getTileImage(0).save(outputFilePath);
        return true;
    }

    // multiple tiles
    // if (!ui->oneFileForAllFramesRadioButton->isChecked()) {
    if (this->ui->filesCountComboBox->currentIndex() != 0) {
        // one file for each tile (indexed)
        for (int i = 0; i < n; i++) {
            if (progress.wasCanceled()) {
                return false;
            }

            progress.setValue(100 * i / n);

            QString outputFilePath = outputFilePathBase + "_tile"
                + QString("%1").arg(i, 4, 10, QChar('0')) + this->getFileFormatExtension();

            this->til->getTileImage(i).save(outputFilePath);
        }
        return true;
    }
    // one file for all tiles

    unsigned tileWidth = this->min->getSubtileWidth() * 2 * MICRO_WIDTH;
    unsigned tileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT + 32;

    // If only one file will contain all tiles
    constexpr unsigned TILES_PER_LINE = 8;
    QImage tempOutputImage;
    unsigned tempOutputImageWidth = 0;
    unsigned tempOutputImageHeight = 0;
    // bool oneFileForAll = ui->oneFileForAllFramesRadioButton->isChecked();
    int placement = this->ui->contentPlacementComboBox->currentIndex();
    if (placement == 0) { // groupped
        tempOutputImageWidth = tileWidth * TILES_PER_LINE;
        tempOutputImageHeight = tileHeight * ((n + (TILES_PER_LINE - 1)) / TILES_PER_LINE);
    //} else if (ui->allFramesOnOneColumnRadioButton->isChecked()) {
    } else if (placement == 2) { // tiles on one column
        tempOutputImageWidth = tileWidth;
        tempOutputImageHeight = tileHeight * n;
    //} else if (ui->allFramesOnOneLineRadioButton->isChecked()) {
    } else { // placement == 1 -- tiles on one line
        tempOutputImageWidth = tileWidth * n;
        tempOutputImageHeight = tileHeight;
    }

    tempOutputImage = QImage(tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32);
    tempOutputImage.fill(Qt::transparent);

    QPainter painter(&tempOutputImage);

    if (placement == 0) { // groupped
        unsigned dx = 0, dy = 0;
        for (unsigned i = 0; i < n; i++) {
            if (progress.wasCanceled()) {
                return false;
            }
            progress.setValue(100 * i / n);

            const QImage image = this->til->getTileImage(i);

            painter.drawImage(dx, dy, image);

            dx += image.width();
            if (dx >= tempOutputImageWidth) {
                dx = 0;
                dy += image.height();
            }
        }
    } else {
        quint32 cursor = 0;
        for (int i = 0; i < n; i++) {
            if (progress.wasCanceled()) {
                return false;
            }
            progress.setValue(100 * i / n);

            //if (ui->allFramesOnOneColumnRadioButton->isChecked()) {
            const QImage image = this->til->getTileImage(i);
            if (placement == 2) { // tiles on one column
                painter.drawImage(0, cursor, image);
                cursor += image.height();
            } else { // placement == 1 -- tiles on one line
                painter.drawImage(cursor, 0, image);
                cursor += image.width();
            }
        }
    }

    if (oneFileForAll) {
        painter.end();
        QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
        tempOutputImage.save(outputFilePath);
    }
    return true;
}

int getTileCount();
    QList<quint16> &getSubtileIndices(int tileIndex);

bool ExportDialog::exportLevelTiles(QProgressDialog &progress)
{
    progress.setLabelText("Exporting " + QFileInfo(this->min->getFilePath()).fileName() + " flat level tiles...");

    QString outputFilePathBase = ui->outputFolderEdit->text() + "/"
        + QFileInfo(this->til->getFilePath()).fileName().replace(".", "_");

    int n = this->til->getTileCount();
    // nothing to export
    if (n == 0) {
        return true;
    }
    // single tile
    if (n == 1) {
        // one file for the only subtile (not indexed)
        QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
        this->til->getFlatTileImage(0).save(outputFilePath);
        return true;
    }
    // multiple tiles
    // if (!ui->oneFileForAllFramesRadioButton->isChecked()) {
    if (this->ui->filesCountComboBox->currentIndex() != 0) {
        // one file for each subtile (indexed)
        for (int i = 0; i < n; i++) {
            if (progress.wasCanceled()) {
                return false;
            }

            progress.setValue(100 * i / n);

            QString outputFilePath = outputFilePathBase + "_tile_"
                + QString("%1").arg(i, 4, 10, QChar('0')) + this->getFileFormatExtension();

            this->til->getFlatTileImage(i).save(outputFilePath);
        }
        return true;
    }
    // one file for all subtiles

    unsigned tileWidth = this->min->getSubtileWidth() * MICRO_WIDTH * TILE_WIDTH * TILE_HEIGHT;
    unsigned tileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    // If only one file will contain all tiles
    constexpr unsigned TILES_PER_LINE = 4;
    QImage tempOutputImage;
    unsigned tempOutputImageWidth = 0;
    unsigned tempOutputImageHeight = 0;
    // bool oneFileForAll = ui->oneFileForAllFramesRadioButton->isChecked();
    int placement = this->ui->contentPlacementComboBox->currentIndex();
    if (placement == 0) { // groupped
        tempOutputImageWidth = tileWidth * TILES_PER_LINE;
        tempOutputImageHeight = tileHeight * ((n + (TILES_PER_LINE - 1)) / TILES_PER_LINE);
    //} else if (ui->allFramesOnOneColumnRadioButton->isChecked()) {
    } else if (placement == 2) { // subtiles on one column
        tempOutputImageWidth = tileWidth;
        tempOutputImageHeight = tileHeight * n;
    //} else if (ui->allFramesOnOneLineRadioButton->isChecked()) {
    } else { // placement == 1 -- subtiles on one line
        tempOutputImageWidth = tileWidth * n;
        tempOutputImageHeight = tileHeight;
    }

    tempOutputImage = QImage(tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32);
    tempOutputImage.fill(Qt::transparent);

    QPainter painter(&tempOutputImage);

    if (placement == 0) { // groupped
        unsigned dx = 0, dy = 0;
        for (unsigned i = 0; i < n; i++) {
            if (progress.wasCanceled()) {
                return false;
            }
            progress.setValue(100 * i / n);

            const QImage image = this->til->getFlatTileImage(i);

            painter.drawImage(dx, dy, image);

            dx += image.width();
            if (dx >= tempOutputImageWidth) {
                dx = 0;
                dy += image.height();
            }
        }
    } else {
        int cursor = 0;
        for (int i = 0; i < n; i++) {
            if (progress.wasCanceled()) {
                return false;
            }
            progress.setValue(100 * i / n);

            //if (ui->allFramesOnOneColumnRadioButton->isChecked()) {
            const QImage image = this->til->getFlatTileImage(i);
            if (placement == 2) { // subtiles on one column
                painter.drawImage(0, cursor, image);
                cursor += image.height();
            } else { // placement == 1 -- subtiles on one line
                painter.drawImage(cursor, 0, image);
                cursor += image.width();
            }
        }
    }

    painter.end();

    QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
    tempOutputImage.save(outputFilePath);
    return true;
}

bool ExportDialog::exportLevelSubtiles(QProgressDialog &progress)
{
    progress.setLabelText("Exporting " + QFileInfo(this->min->getFilePath()).fileName() + " level subtiles...");

    QString outputFilePathBase = ui->outputFolderEdit->text() + "/"
        + QFileInfo(this->min->getFilePath()).fileName().replace(".", "_");

    int n = this->min->getSubtileCount();
    // nothing to export
    if (n == 0) {
        return true;
    }
    // single subtile
    if (n == 1) {
        // one file for the only subtile (not indexed)
        QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
        this->min->getSubtileImage(0).save(outputFilePath);
        return true;
    }
    // multiple subtiles
    // if (!ui->oneFileForAllFramesRadioButton->isChecked()) {
    if (this->ui->filesCountComboBox->currentIndex() != 0) {
        // one file for each subtile (indexed)
        for (int i = 0; i < n; i++) {
            if (progress.wasCanceled()) {
                return false;
            }

            progress.setValue(100 * i / n);

            QString outputFilePath = outputFilePathBase + "_subtile"
                + QString("%1").arg(i, 4, 10, QChar('0')) + this->getFileFormatExtension();

            this->min->getSubtileImage(i).save(outputFilePath);
        }
        return true;
    }
    // one file for all subtiles

    unsigned subtileWidth = this->min->getSubtileWidth() * MICRO_WIDTH;
    unsigned subtileHeight = this->min->getSubtileHeight() * MICRO_HEIGHT;

    // If only one file will contain all subtiles
    QImage tempOutputImage;
    unsigned tempOutputImageWidth = 0;
    unsigned tempOutputImageHeight = 0;
    // bool oneFileForAll = ui->oneFileForAllFramesRadioButton->isChecked();
    int placement = this->ui->contentPlacementComboBox->currentIndex();
    if (placement == 0) { // groupped
        tempOutputImageWidth = subtileWidth * EXPORT_SUBTILES_PER_LINE;
        tempOutputImageHeight = subtileHeight * ((n + (EXPORT_SUBTILES_PER_LINE - 1)) / EXPORT_SUBTILES_PER_LINE);
    //} else if (ui->allFramesOnOneColumnRadioButton->isChecked()) {
    } else if (placement == 2) { // subtiles on one column
        tempOutputImageWidth = subtileWidth;
        tempOutputImageHeight = subtileHeight * n;
    //} else if (ui->allFramesOnOneLineRadioButton->isChecked()) {
    } else { // placement == 1 -- subtiles on one line
        tempOutputImageWidth = subtileWidth * n;
        tempOutputImageHeight = subtileHeight;
    }

    tempOutputImage = QImage(tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32);
    tempOutputImage.fill(Qt::transparent);

    QPainter painter(&tempOutputImage);

    if (placement == 0) { // groupped
        unsigned dx = 0, dy = 0;
        for (unsigned i = 0; i < n; i++) {
            if (progress.wasCanceled()) {
                return false;
            }
            progress.setValue(100 * i / n);

            const QImage image = this->min->getSubtileImage(i);

            painter.drawImage(dx, dy, image);

            dx += image.width();
            if (dx >= tempOutputImageWidth) {
                dx = 0;
                dy += image.height();
            }
        }
    } else {
        int cursor = 0;
        for (int i = 0; i < n; i++) {
            if (progress.wasCanceled()) {
                return false;
            }
            progress.setValue(100 * i / n);

            //if (ui->allFramesOnOneColumnRadioButton->isChecked()) {
            const QImage image = this->min->getSubtileImage(i);
            if (placement == 2) { // subtiles on one column
                painter.drawImage(0, cursor, image);
                cursor += image.height();
            } else { // placement == 1 -- subtiles on one line
                painter.drawImage(cursor, 0, image);
                cursor += image.width();
            }
        }
    }

    painter.end();

    QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
    tempOutputImage.save(outputFilePath);
    return true;
}

bool ExportDialog::exportFrames(QProgressDialog &progress)
{
    progress.setLabelText("Exporting " + QFileInfo(this->gfx->getFilePath()).fileName() + " frames...");

    QString outputFilePathBase = ui->outputFolderEdit->text() + "/"
        + QFileInfo(this->gfx->getFilePath()).fileName().replace(".", "_");

    int n = this->gfx->getFrameCount();
    // nothing to export
    if (n == 0) {
        return true;
    }
    // single frame
    if (n == 1) {
        // one file for the only frame (not indexed)
        QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
        this->gfx->getFrameImage(0).save(outputFilePath);
        return true;
    }
    // multiple frames
    // if (!ui->oneFileForAllFramesRadioButton->isChecked()) {
    if (this->ui->filesCountComboBox->currentIndex() != 0) {
        // one file for each frame (indexed)
        for (int i = 0; i < n; i++) {
            if (progress.wasCanceled()) {
                return false;
            }

            progress.setValue(100 * i / n);

            QString outputFilePath = outputFilePathBase + "_frame"
                + QString("%1").arg(i, 4, 10, QChar('0')) + this->getFileFormatExtension();

            this->gfx->getFrameImage(i).save(outputFilePath);
        }
        return true;
    }
    // one file for all frames
    QImage tempOutputImage;
    quint32 tempOutputImageWidth = 0;
    quint32 tempOutputImageHeight = 0;
    // If only one file will contain all frames
    // if (ui->oneFrameGroupPerLineRadioButton->isChecked()) {
    int placement = this->ui->contentPlacementComboBox->currentIndex();
    if (placement == 0) { // groupped
        if (this->gfx->getType() == D1CEL_TYPE::V1_LEVEL) {
            // artifical grouping of a tileset
            quint32 groupImageWidth = 0;
            quint32 groupImageHeight = 0;
            for (int i = 0; i < n; i++) {
                if ((i % EXPORT_LVLFRAMES_PER_LINE) == 0) {
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
                quint32 groupImageWidth = 0;
                quint32 groupImageHeight = 0;
                for (unsigned int j = this->gfx->getGroupFrameIndices(i).first;
                     j <= this->gfx->getGroupFrameIndices(i).second; j++) {
                    groupImageWidth += this->gfx->getFrameWidth(j);
                    groupImageHeight = std::max(this->gfx->getFrameHeight(j), groupImageHeight);
                }
                tempOutputImageWidth = std::max(groupImageWidth, tempOutputImageWidth);
                tempOutputImageHeight += groupImageHeight;
            }
        }
    //} else if (ui->allFramesOnOneColumnRadioButton->isChecked()) {
    } else if (placement == 2) { // frames on one column
        for (int i = 0; i < n; i++) {
            tempOutputImageWidth = std::max(this->gfx->getFrameWidth(i), tempOutputImageWidth);
            tempOutputImageHeight += this->gfx->getFrameHeight(i);
        }
    //} else if (ui->allFramesOnOneLineRadioButton->isChecked()) {
    } else { // placement == 1 -- frames on one line
        for (int i = 0; i < n; i++) {
            tempOutputImageWidth += this->gfx->getFrameWidth(i);
            tempOutputImageHeight = std::max(this->gfx->getFrameHeight(i), tempOutputImageHeight);
        }
    }
    tempOutputImage = QImage(tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32);
    tempOutputImage.fill(Qt::transparent);

    QPainter painter(&tempOutputImage);

    // if (ui->oneFrameGroupPerLineRadioButton->isChecked()) {
    if (placement == 0) { // groupped
        if (this->gfx->getType() == D1CEL_TYPE::V1_LEVEL) {
            // artifical grouping of a tileset
            int cursorY = 0;
            int cursorX = 0;
            int groupImageHeight = 0;
            for (int i = 0; i < n; i++) {
                if (progress.wasCanceled()) {
                    return false;
                }
                progress.setValue(100 * i / n);

                if ((i % EXPORT_LVLFRAMES_PER_LINE) == 0) {
                    cursorY += groupImageHeight;
                    cursorX = 0;
                    groupImageHeight = 0;
                }

                const QImage image = this->gfx->getFrameImage(i);
                painter.drawImage(cursorX, cursorY, image);

                cursorX += image.width();
                groupImageHeight = std::max(image.height(), groupImageHeight);
            }
        } else {
            int cursorY = 0;
            for (int i = 0; i < this->gfx->getGroupCount(); i++) {
                int groupImageHeight = 0;
                for (unsigned int j = this->gfx->getGroupFrameIndices(i).first;
                     j <= this->gfx->getGroupFrameIndices(i).second; j++) {
                    if (progress.wasCanceled()) {
                        return false;
                    }
                    progress.setValue(100 * j / n);

                    const QImage image = this->gfx->getFrameImage(j);
                    painter.drawImage(cursorX, cursorY, image);
                    cursorX += image.width();
                    groupImageHeight = std::max(image.height(), groupImageHeight);
                }
                cursorY += groupImageHeight;
            }
        }
    } else {
        int cursor = 0;
        for (int i = 0; i < n; i++) {
            if (progress.wasCanceled()) {
                return false;
            }
            progress.setValue(100 * i / n);

            //if (ui->allFramesOnOneColumnRadioButton->isChecked()) {
            const QImage image = this->gfx->getFrameImage(i);
            if (placement == 2) { // frames on one column
                painter.drawImage(0, cursor, image);
                cursor += image.height();
            } else { // placement == 1 -- frames on one line
                painter.drawImage(cursor, 0, image);
                cursor += image.width();
            }
        }
    }

    painter.end();

    QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
    tempOutputImage.save(outputFilePath);
    return true;
}

void ExportDialog::on_exportButton_clicked()
{
    if (ui->outputFolderEdit->text() == "") {
        QMessageBox::warning(this, "Warning", "Output folder is missing, please choose an output folder.");
        return;
    }

    if (this->gfx == nullptr) {
        QMessageBox::critical(this, "Warning", "No graphics loaded.");
        return;
    }

    bool result;
    try {
        // Displaying the progress dialog box
        QProgressDialog progress("Exporting...", "Cancel", 0, 100, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);
        progress.setWindowTitle("Export");
        progress.setLabelText("Exporting");
        progress.setValue(0);
        progress.show();

        switch (this->ui->contentTypeComboBox->currentIndex()) {
        case 0:
            result = this->exportFrames(progress);
            break;
        case 1:
            result = this->exportLevelSubtiles(progress);
            break;
        case 2:
            result = this->exportLevelTiles(progress);
            break;
        default: // case 3:
            result = this->exportLevelTiles25D(progress);
            break;
        }
    } catch (...) {
        QMessageBox::critical(this, "Error", "Export Failed.");
        return;
    }
    if (result) {
        QMessageBox::information(this, "Information", "Export successful.");
        this->close();
    } else {
        QMessageBox::warning(this, "Export Canceled", "Export was canceled.");
    }
}

void ExportDialog::on_exportCancelButton_clicked()
{
    this->close();
}

/*void ExportDialog::on_oneFileForAllFramesRadioButton_toggled(bool checked)
{
    ui->spritesSettingsWidget->setEnabled(checked && ui->levelFramesSettingsWidget->isEnabled());
}*/
