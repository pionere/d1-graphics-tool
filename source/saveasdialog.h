#pragma once

#include <QDialog>
#include <QString>

class D1Gfx;

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
    int groupNum = 0;
    SAVE_CLIPPED_TYPE clipped = SAVE_CLIPPED_TYPE::AUTODETECT;
    bool autoOverwrite = false;
};

namespace Ui {
class SaveAsDialog;
}

class SaveAsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SaveAsDialog(QWidget *parent);
    ~SaveAsDialog();

    void initialize(D1Gfx *gfx);

private slots:
    void on_outputCelFileBrowseButton_clicked();
    void on_saveButton_clicked();
    void on_saveCancelButton_clicked();

    // this event is called, when a new translator is loaded or the system language is changed
    void changeEvent(QEvent *event) override;

private:
    Ui::SaveAsDialog *ui;
    D1Gfx *gfx = nullptr;
};
