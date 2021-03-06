#include "exportdialog.h"
#include "ui_exportdialog.h"

ExportDialog::ExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportDialog),
    cel( NULL ),
    min( NULL ),
    til( NULL )
{
    ui->setupUi(this);
}

ExportDialog::~ExportDialog()
{
    delete ui;
}

void ExportDialog::setCel( D1CelBase* c )
{
    this->cel = c;

    // If there's only one frame
    if( this->cel->getFrameCount() == 1 )
        ui->filesSettingWidget->setEnabled( false );
    else
        ui->filesSettingWidget->setEnabled( true );

    // If all frames have the same width/height
    if( this->cel->getFrameCount() > 1 && this->cel->isFrameSizeConstant()
        && ui->oneFileForAllFramesRadioButton->isChecked() )
    {
        ui->spritesSettingsWidget->setEnabled( true );
    }

    // If there's only one group
    if( this->cel->getGroupCount() == 1 )
    {
        ui->allFramesOnOneLineRadioButton->setChecked( true );
        ui->oneFrameGroupPerLineRadioButton->setEnabled( false );
    }
    else
    {
        ui->oneFrameGroupPerLineRadioButton->setChecked( true );
        ui->oneFrameGroupPerLineRadioButton->setEnabled( true );
    }

    // If it's a CEL level file
    if( this->cel->getType() == D1CEL_TYPE::V1_LEVEL && this->min && this->til )
        ui->levelFramesSettingsWidget->setEnabled( true );
    else
        ui->levelFramesSettingsWidget->setEnabled( false );
}

void ExportDialog::setMin( D1Min* m )
{
    this->min = m;
}

void ExportDialog::setTil( D1Til* t )
{
    this->til = t;
}

QString ExportDialog::getFileFormatExtension()
{
    if( ui->pngRadioButton->isChecked() )
        return ".png";
    if( ui->bmpRadioButton->isChecked() )
        return ".bmp";

    return QString();
}

void ExportDialog::on_outputFolderBrowseButton_clicked()
{
    QString selectedFolder = QFileDialog::getExistingDirectory(
        this, "Select Output Folder", QString(), QFileDialog::ShowDirsOnly );

    ui->outputFolderEdit->setText( selectedFolder );
}

void ExportDialog::on_exportButton_clicked()
{
    if( ui->outputFolderEdit->text() == "" )
    {
        QMessageBox::warning( this, "Warning", "Output folder is missing, please choose an output folder." );
        return;
    }

    QString outputFilePathBase;
    QImage tempOutputImage;
    quint16 tempOutputImageWidth = 0;
    quint16 tempOutputImageHeight = 0;
    bool exportSuccess = true;

    try
    {
        // Displaying the progress dialog box
        QProgressDialog progress( "Exporting...", "Cancel", 0, 100, this );
        progress.setWindowModality( Qt::WindowModal );
        progress.setMinimumDuration( 0 );
        progress.setWindowTitle( "Export" );
        progress.setLabelText("Exporting");
        progress.setValue( 0 );
        progress.show();

        // If it's a CEL/CL2 file
        if( this->cel )
        {
            // If it's a CEL level file
            if( this->cel->getType() == D1CEL_TYPE::V1_LEVEL
                && this->min && this->til && !ui->exportLevelFrames->isChecked() )
            {
                if( ui->exportLevelTiles->isChecked() )
                {
                    progress.setLabelText( "Exporting " + QFileInfo(this->til->getFilePath()).fileName() + " level tiles..." );

                    outputFilePathBase = ui->outputFolderEdit->text() + "/"
                        + QFileInfo(this->til->getFilePath()).fileName().replace(".","_");

                    quint16 tileWidth = this->min->getSubtileWidth()*2*32;
                    quint16 tileHeight = this->min->getSubtileHeight()*32+32;

                    // If only one file will contain all tiles
                    if( ui->oneFileForAllFramesRadioButton->isChecked() )
                    {
                        tempOutputImageWidth = tileWidth * 8;
                        tempOutputImageHeight = tileHeight * (quint32)( this->til->getTileCount()/8 );
                        if( this->til->getTileCount()%8 != 0 )
                            tempOutputImageHeight += tileHeight;
                        tempOutputImage = QImage( tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32 );
                        tempOutputImage.fill( Qt::transparent );
                    }

                    QPainter painter( &tempOutputImage );
                    quint8 tileXIndex = 0;
                    quint8 tileYIndex = 0;
                    for( unsigned int i = 0; i < this->til->getTileCount(); i++ )
                    {
                        if( progress.wasCanceled() )
                            QMessageBox::warning( this, "Export Canceled", "Export was canceled." );

                        progress.setValue( 100*i/this->til->getTileCount() );

                        // If only one file will contain all tiles
                        if( ui->oneFileForAllFramesRadioButton->isChecked() )
                        {
                            painter.drawImage( tileXIndex*tileWidth,
                                tileYIndex*tileHeight, this->til->getTileImage(i) );

                            tileXIndex++;
                            if( tileXIndex >= 8 )
                            {
                                tileXIndex = 0;
                                tileYIndex++;
                            }
                        }
                        else
                        {
                            QString outputFilePath = outputFilePathBase + "_tile"
                                + QString("%1").arg(i,4,10,QChar('0')) + this->getFileFormatExtension();

                            this->til->getTileImage( i ).save( outputFilePath );
                        }
                    }

                    if( ui->oneFileForAllFramesRadioButton->isChecked() )
                    {
                        painter.end();
                        QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
                        tempOutputImage.save( outputFilePath );
                    }
                }
                else if( ui->exportLevelSubtiles->isChecked() )
                {
                    progress.setLabelText( "Exporting " + QFileInfo(this->min->getFilePath()).fileName() + " level sub-tiles..." );

                    outputFilePathBase = ui->outputFolderEdit->text() + "/"
                        + QFileInfo(this->min->getFilePath()).fileName().replace(".","_");

                    quint16 subtileWidth = this->min->getSubtileWidth()*32;
                    quint16 subtileHeight = this->min->getSubtileHeight()*32;

                    // If only one file will contain all sub-tiles
                    if( ui->oneFileForAllFramesRadioButton->isChecked() )
                    {
                        tempOutputImageWidth = subtileWidth * 16;
                        tempOutputImageHeight = subtileHeight * (quint32)( this->min->getSubtileCount()/16 );
                        if( this->min->getSubtileCount()%16 != 0 )
                            tempOutputImageHeight += subtileHeight;
                        tempOutputImage = QImage( tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32 );
                        tempOutputImage.fill( Qt::transparent );
                    }

                    QPainter painter( &tempOutputImage );
                    quint8 subtileXIndex = 0;
                    quint8 subtileYIndex = 0;
                    for( unsigned int i = 0; i < this->min->getSubtileCount(); i++ )
                    {
                        if( progress.wasCanceled() )
                            QMessageBox::warning( this, "Export Canceled", "Export was canceled." );

                        progress.setValue( 100*i/this->min->getSubtileCount() );

                        // If only one file will contain all sub-tiles
                        if( ui->oneFileForAllFramesRadioButton->isChecked() )
                        {
                            painter.drawImage( subtileXIndex*subtileWidth,
                                subtileYIndex*subtileHeight, this->min->getSubtileImage(i) );

                            subtileXIndex++;
                            if( subtileXIndex >= 16 )
                            {
                                subtileXIndex = 0;
                                subtileYIndex++;
                            }
                        }
                        else
                        {
                            QString outputFilePath = outputFilePathBase + "_sub-tile"
                                + QString("%1").arg(i,4,10,QChar('0')) + this->getFileFormatExtension();

                            this->min->getSubtileImage( i ).save( outputFilePath );
                        }
                    }

                    if( ui->oneFileForAllFramesRadioButton->isChecked() )
                    {
                        painter.end();
                        QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
                        tempOutputImage.save( outputFilePath );
                    }
                }
            }
            // If it's a standard CEL/CL2 file
            else
            {
                progress.setLabelText( "Exporting " + QFileInfo(this->cel->getFilePath()).fileName() + " frames..." );

                outputFilePathBase = ui->outputFolderEdit->text() + "/"
                    + QFileInfo(this->cel->getFilePath()).fileName().replace(".","_");

                // If only one file will contain all frames
                if( ui->oneFileForAllFramesRadioButton->isChecked() )
                {
                    if( ui->oneFrameGroupPerLineRadioButton->isChecked() )
                    {
                        tempOutputImageWidth = this->cel->getFrameWidth(0) * (
                            this->cel->getGroupFrameIndices(0).second - this->cel->getGroupFrameIndices(0).first + 1 );
                        tempOutputImageHeight = this->cel->getFrameHeight(0) * this->cel->getGroupCount();
                    }
                    else if( ui->allFramesOnOneColumnRadioButton->isChecked() )
                    {
                        tempOutputImageWidth = this->cel->getFrameWidth(0);
                        tempOutputImageHeight = this->cel->getFrameHeight(0) * this->cel->getFrameCount();
                    }
                    else if( ui->allFramesOnOneLineRadioButton->isChecked() )
                    {
                        tempOutputImageWidth = this->cel->getFrameWidth(0) * this->cel->getFrameCount();
                        tempOutputImageHeight = this->cel->getFrameHeight(0);
                    }
                    tempOutputImage = QImage( tempOutputImageWidth, tempOutputImageHeight, QImage::Format_ARGB32 );
                    tempOutputImage.fill( Qt::transparent );
                }

                if( this->cel->getFrameCount() == 1 )
                {
                    QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
                    this->cel->getFrameImage( 0 ).save( outputFilePath );
                }
                else
                {
                    // If only one file will contain all frames
                    if( ui->oneFileForAllFramesRadioButton->isChecked() )
                    {
                        QPainter painter( &tempOutputImage );

                        if( ui->oneFrameGroupPerLineRadioButton->isChecked() )
                        {
                            for( unsigned int i = 0; i < this->cel->getGroupCount(); i++ )
                            {
                                quint8 groupFrameIndex = 0;

                                for( unsigned int j = this->cel->getGroupFrameIndices(i).first;
                                    j <= this->cel->getGroupFrameIndices(i).second; j++ )
                                {
                                    if( progress.wasCanceled() )
                                        QMessageBox::warning( this, "Export Canceled", "Export was canceled." );

                                    progress.setValue( 100*j/this->cel->getFrameCount() );

                                    painter.drawImage( groupFrameIndex*this->cel->getFrameWidth(0),
                                        i*this->cel->getFrameHeight(0), this->cel->getFrameImage(j) );
                                    groupFrameIndex++;
                                }
                            }
                        }
                        else
                        {
                            for( unsigned int i = 0; i < this->cel->getFrameCount(); i++ )
                            {
                                if( progress.wasCanceled() )
                                    QMessageBox::warning( this, "Export Canceled", "Export was canceled." );

                                progress.setValue( 100*i/this->cel->getFrameCount() );

                                if( ui->allFramesOnOneColumnRadioButton->isChecked() )
                                    painter.drawImage( 0, i*this->cel->getFrameHeight(0), this->cel->getFrameImage(i) );
                                else
                                    painter.drawImage( i*this->cel->getFrameWidth(0), 0, this->cel->getFrameImage(i) );
                            }
                        }

                        painter.end();

                        QString outputFilePath = outputFilePathBase + this->getFileFormatExtension();
                        tempOutputImage.save( outputFilePath );
                    }
                    else
                    {
                        for( unsigned int i = 0; i < this->cel->getFrameCount(); i++ )
                        {
                            if( progress.wasCanceled() )
                                QMessageBox::warning( this, "Export Canceled", "Export was canceled." );

                            progress.setValue( 100*i/this->cel->getFrameCount() );

                            QString outputFilePath = outputFilePathBase + "_frame"
                                + QString("%1").arg(i,4,10,QChar('0')) + this->getFileFormatExtension();

                            this->cel->getFrameImage( i ).save( outputFilePath );
                        }
                    }
                }
            }

        }
    }
    catch(...)
    {
        exportSuccess = false;
    }

    if( exportSuccess )
    {
        QMessageBox::information( this, "Information", "Export successful." );
        this->close();
    }
    else
        QMessageBox::critical( this, "Error", "Export Failed." );
}

void ExportDialog::on_exportCancelButton_clicked()
{
    this->close();
}

void ExportDialog::on_oneFileForAllFramesRadioButton_toggled( bool checked )
{
    if( checked && !ui->levelFramesSettingsWidget->isEnabled() )
        ui->spritesSettingsWidget->setEnabled( true );
    else
        ui->spritesSettingsWidget->setEnabled( false );
}

