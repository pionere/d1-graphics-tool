#pragma once

#include <QDialog>
#include <QList>
#include <QListWidgetItem>
#include <QString>

#include "d1pal.h"
#include "openasdialog.h"
#include "saveasdialog.h"
#include "upscaledialog.h"

class UpscaleTaskParam {
public:
    QString listfilesFile;
    QList<QModelIndex> skipSteps;
    QString assetsFolder;
    QString outFolder;
    int multiplier;
    bool autoOverwrite;
};

namespace Ui {
class UpscaleTaskDialog;
}

class UpscaleTaskDialog : public QDialog {
    Q_OBJECT

public:
    explicit UpscaleTaskDialog(QWidget *parent = nullptr);
    ~UpscaleTaskDialog();

private:
    static bool loadCustomPal(const char *path, int numcolors, int fixcolors, const UpscaleTaskParam &params, D1Pal &pal, UpscaleParam &upParams);
    static void upscaleCel(const QString &path, D1Pal *pal, const UpscaleTaskParam &params, const OpenAsParam &opParams, const UpscaleParam &upParams, SaveAsParam &saParams);
    static void upscaleCl2(const QString &path, D1Pal *pal, const UpscaleTaskParam &params, const OpenAsParam &opParams, const UpscaleParam &upParams, SaveAsParam &saParams);
    static void upscaleMin(D1Pal *pal, const UpscaleTaskParam &params, const OpenAsParam &opParams, const UpscaleParam &upParams, SaveAsParam &saParams);
    static void runTask(const UpscaleTaskParam &params);

private slots:
    void on_listfilesFileBrowseButton_clicked();
    void on_assetsFolderBrowseButton_clicked();
    void on_outputFolderBrowseButton_clicked();
    void on_upscaleButton_clicked();
    void on_upscaleCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event);

private:
    Ui::UpscaleTaskDialog *ui;
};
