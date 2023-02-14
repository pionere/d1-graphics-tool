#pragma once

#include <QDialog>

#include "d1gfx.h"
#include "d1tileset.h"

typedef struct ExportParam {
    QString outFolder;
    QString outFileExtension;
    bool multi;
    int rangeFrom;
    int rangeTo;
    int placement;
} ExportParam;

namespace Ui {
class ExportDialog;
}

// subtiles per line if the output is groupped, an odd number to ensure it is not recognized as a flat tile
#define EXPORT_SUBTILES_PER_LINE 15

// frames per line if the output of a tileset-frames is groupped, an odd number to ensure it is not recognized as a flat tile or as subtiles
#define EXPORT_LVLFRAMES_PER_LINE 31

class ExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(QWidget *parent);
    ~ExportDialog();

    void initialize(D1Gfx *gfx, D1Tileset *tileset);

private slots:
    void on_outputFolderBrowseButton_clicked();
    void on_exportButton_clicked();
    void on_exportCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    static void exportLevelTiles25D(const D1Til *til, const ExportParam &params);
    static void exportLevelTiles(const D1Til *til, const ExportParam &params);
    static void exportLevelSubtiles(const D1Min *min, const ExportParam &params);
    static void exportFrames(const D1Gfx *gfx, const ExportParam &params);

    Ui::ExportDialog *ui;

    D1Gfx *gfx = nullptr;
    D1Tileset *tileset = nullptr;
};
