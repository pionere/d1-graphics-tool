#pragma once

#include <QDialog>
#include <QList>
#include <QListWidgetItem>
#include <QString>

#include "d1pal.h"
#include "openasdialog.h"
#include "saveasdialog.h"
#include "upscaledialog.h"

typedef enum task_step {
    REGULAR_CEL,
    OBJECT_CEL,
    CUTSCENE,
    ART_CEL,
    REGULAR_CL2_MISSILES,
    REGULAR_CL2_MONSTERS,
    REGULAR_CL2_PLRGFX,
    FIXED_CL2,
    TILESET,
    // FIXED_TILESET,
    NUM_STEPS,
} task_step;

class UpscaleTaskParam {
public:
    QString listfilesFile;
    bool steps[NUM_STEPS];
    bool patchGraphics;
    bool patchTilesets;
    QString assetsFolder;
    QString outFolder;
    int multiplier;
};

namespace Ui {
class UpscaleTaskDialog;
}

class UpscaleTaskDialog : public QDialog {
    Q_OBJECT

public:
    explicit UpscaleTaskDialog(QWidget *parent);
    ~UpscaleTaskDialog();

private:
    static bool loadCustomPal(const QString &path, int numcolors, int fixcolors, const UpscaleTaskParam &params, D1Pal &pal, UpscaleParam &upParams);
    static void upscaleCel(const QString &path, D1Pal *pal, const UpscaleTaskParam &params, const OpenAsParam &opParams, const UpscaleParam &upParams, SaveAsParam &saParams);
    static void upscaleCl2(const QString &path, D1Pal *pal, const UpscaleTaskParam &params, const OpenAsParam &opParams, const UpscaleParam &upParams, SaveAsParam &saParams);
    static void upscaleMin(D1Pal *pal, const UpscaleTaskParam &params, OpenAsParam &opParams, const UpscaleParam &upParams, SaveAsParam &saParams, int dunType);
    static void runTask(const UpscaleTaskParam &params);

private slots:
    void on_listfilesFileBrowseButton_clicked();
    void on_selectAllButton_clicked();
    void on_deselectAllButton_clicked();
    void on_assetsFolderBrowseButton_clicked();
    void on_outputFolderBrowseButton_clicked();
    void on_upscaleButton_clicked();
    void on_upscaleCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::UpscaleTaskDialog *ui;
};
