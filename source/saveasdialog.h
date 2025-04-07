#pragma once

#include <QDialog>
#include <QString>

class D1Gfx;
class D1Tileset;
class D1Gfxset;
class D1Tableset;
class D1Dun;
class D1Cpp;

enum class SAVE_CLIPPED_TYPE {
    AUTODETECT,
    TRUE,
    FALSE,
};

enum class SAVE_UPSCALED_TYPE {
    AUTODETECT,
    TRUE,
    FALSE,
};

class SaveAsParam {
public:
    QString celFilePath;
    QString clsFilePath;
    QString tilFilePath;
    QString minFilePath;
    QString slaFilePath;
    QString tlaFilePath;
    QString dunFilePath;
    uint8_t dunLayerNum = UINT8_MAX;
    int groupNum = 0;
    SAVE_CLIPPED_TYPE clipped = SAVE_CLIPPED_TYPE::AUTODETECT;
    SAVE_UPSCALED_TYPE upscaled = SAVE_UPSCALED_TYPE::AUTODETECT;
    bool autoOverwrite = false;
    QString tblFilePath;
};

namespace Ui {
class SaveAsDialog;
}

class SaveAsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SaveAsDialog(QWidget *parent);
    ~SaveAsDialog();

    void initialize(D1Gfx *gfx, D1Tileset *tileset, D1Gfxset *gfxset, D1Dun *dun, D1Tableset *tableset, D1Cpp *cpp);

private slots:
    void on_outputCelFileBrowseButton_clicked();
    void on_outputClsFileBrowseButton_clicked();
    void on_outputMinFileBrowseButton_clicked();
    void on_outputTilFileBrowseButton_clicked();
    void on_outputSlaFileBrowseButton_clicked();
    void on_outputTlaFileBrowseButton_clicked();
    void on_outputDunFileBrowseButton_clicked();
    void on_tblFileBrowseButton_clicked();
    void on_saveButton_clicked();
    void on_saveCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::SaveAsDialog *ui;
    D1Gfx *gfx = nullptr;
    bool isTileset;
    bool isDun;
    bool isGfxset;
    bool isTableset;
    bool isCpp;
    bool isSmk;
};
